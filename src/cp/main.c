#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <libgen.h>
#include <time.h>

#define BUFFER_SIZE (1024 * 1024)  // 1MB buffer for efficient copying
#define PATH_MAX 4096

typedef struct {
    int recursive;           // -r, -R option
    int force;              // -f option
    int interactive;        // -i option
    int preserve;           // -p option
    int verbose;            // -v option
    int no_target_dir;      // -T option
    int update;             // -u option
} CopyOptions;

// Function prototypes
static int copy_file(const char *src, const char *dst, const CopyOptions *options);
static int copy_directory(const char *src, const char *dst, const CopyOptions *options);
static int create_path(const char *path);
static void print_usage(const char *program_name);
static int check_update_condition(const char *src, const char *dst);

// Implementation of main copy logic
static int copy_file(const char *src, const char *dst, const CopyOptions *options) {
    char buffer[BUFFER_SIZE];
    struct stat src_stat;
    int src_fd, dst_fd;
    ssize_t bytes_read, bytes_written;
    mode_t mode;

    if (stat(src, &src_stat) == -1) {
        fprintf(stderr, "Error: Cannot stat source file '%s': %s\n", 
                src, strerror(errno));
        return -1;
    }

    // Check if destination exists and handle according to options
    if (access(dst, F_OK) == 0) {
        if (options->interactive) {
            printf("overwrite '%s'? (y/n [n]) ", dst);
            char response = getchar();
            while (getchar() != '\n'); // Clear input buffer
            if (response != 'y' && response != 'Y') {
                return 0;
            }
        } else if (!options->force) {
            if (options->update && !check_update_condition(src, dst)) {
                return 0;
            }
        }
    }

    // Open source file
    src_fd = open(src, O_RDONLY);
    if (src_fd == -1) {
        fprintf(stderr, "Error: Cannot open source file '%s': %s\n", 
                src, strerror(errno));
        return -1;
    }

    // Create destination file
    mode = options->preserve ? src_stat.st_mode : 0666;
    dst_fd = open(dst, O_WRONLY | O_CREAT | O_TRUNC, mode);
    if (dst_fd == -1) {
        fprintf(stderr, "Error: Cannot create destination file '%s': %s\n", 
                dst, strerror(errno));
        close(src_fd);
        return -1;
    }

    // Copy file contents
    while ((bytes_read = read(src_fd, buffer, BUFFER_SIZE)) > 0) {
        bytes_written = write(dst_fd, buffer, bytes_read);
        if (bytes_written != bytes_read) {
            fprintf(stderr, "Error: Write failed for '%s': %s\n", 
                    dst, strerror(errno));
            close(src_fd);
            close(dst_fd);
            return -1;
        }
    }

    // Preserve attributes if requested
    if (options->preserve) {
        struct timespec times[2];
        times[0] = src_stat.st_atim;
        times[1] = src_stat.st_mtim;
        if (futimens(dst_fd, times) == -1) {
            fprintf(stderr, "Warning: Could not preserve timestamps for '%s'\n", dst);
        }
        if (fchown(dst_fd, src_stat.st_uid, src_stat.st_gid) == -1) {
            fprintf(stderr, "Warning: Could not preserve ownership for '%s'\n", dst);
        }
    }

    close(src_fd);
    close(dst_fd);

    if (options->verbose) {
        printf("'%s' -> '%s'\n", src, dst);
    }

    return 0;
}

// Directory copying implementation
static int copy_directory(const char *src, const char *dst, const CopyOptions *options) {
    DIR *dir;
    struct dirent *entry;
    char src_path[PATH_MAX];
    char dst_path[PATH_MAX];
    struct stat statbuf;

    // Create destination directory if it doesn't exist
    if (mkdir(dst, 0777) == -1 && errno != EEXIST) {
        fprintf(stderr, "Error: Cannot create directory '%s': %s\n", 
                dst, strerror(errno));
        return -1;
    }

    dir = opendir(src);
    if (!dir) {
        fprintf(stderr, "Error: Cannot open directory '%s': %s\n", 
                src, strerror(errno));
        return -1;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        snprintf(src_path, PATH_MAX, "%s/%s", src, entry->d_name);
        snprintf(dst_path, PATH_MAX, "%s/%s", dst, entry->d_name);

        if (lstat(src_path, &statbuf) == -1) {
            fprintf(stderr, "Error: Cannot stat '%s': %s\n", 
                    src_path, strerror(errno));
            continue;
        }

        if (S_ISDIR(statbuf.st_mode)) {
            if (options->recursive) {
                if (copy_directory(src_path, dst_path, options) == -1) {
                    closedir(dir);
                    return -1;
                }
            }
        } else {
            if (copy_file(src_path, dst_path, options) == -1) {
                closedir(dir);
                return -1;
            }
        }
    }

    closedir(dir);
    return 0;
}

// Helper function to check if source is newer than destination
static int check_update_condition(const char *src, const char *dst) {
    struct stat src_stat, dst_stat;

    if (stat(src, &src_stat) == -1 || stat(dst, &dst_stat) == -1) {
        return 1;  // If we can't stat, assume update is needed
    }

    return src_stat.st_mtime > dst_stat.st_mtime;
}

// Create directory path recursively
static int create_path(const char *path) {
    char tmp[PATH_MAX];
    char *p = NULL;
    size_t len;

    snprintf(tmp, sizeof(tmp), "%s", path);
    len = strlen(tmp);
    if (tmp[len - 1] == '/')
        tmp[len - 1] = 0;
    for (p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = 0;
            if (mkdir(tmp, 0777) == -1 && errno != EEXIST)
                return -1;
            *p = '/';
        }
    }
    return mkdir(tmp, 0777);
}

static void print_usage(const char *program_name) {
    fprintf(stderr, "Usage: %s [OPTION]... SOURCE DEST\n", program_name);
    fprintf(stderr, "  or:  %s [OPTION]... SOURCE... DIRECTORY\n", program_name);
    fprintf(stderr, "\nOptions:\n");
    fprintf(stderr, "  -f, --force         force overwrite of destination file\n");
    fprintf(stderr, "  -i, --interactive   prompt before overwrite\n");
    fprintf(stderr, "  -p, --preserve      preserve file attributes\n");
    fprintf(stderr, "  -r, -R, --recursive copy directories recursively\n");
    fprintf(stderr, "  -u, --update        copy only when source is newer\n");
    fprintf(stderr, "  -v, --verbose       explain what is being done\n");
    fprintf(stderr, "  -T, --no-target-directory  treat DEST as a normal file\n");
    fprintf(stderr, "      --help          display this help and exit\n");
}

int main(int argc, char *argv[]) {
    CopyOptions options = {0};
    int i;
    char *target;
    struct stat target_stat;

    // Parse command line options
    for (i = 1; i < argc && argv[i][0] == '-'; i++) {
        char *opt = argv[i];
        if (strcmp(opt, "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        }
        while (*++opt) {
            switch (*opt) {
                case 'r':
                case 'R': options.recursive = 1; break;
                case 'f': options.force = 1; break;
                case 'i': options.interactive = 1; break;
                case 'p': options.preserve = 1; break;
                case 'v': options.verbose = 1; break;
                case 'T': options.no_target_dir = 1; break;
                case 'u': options.update = 1; break;
                default:
                    fprintf(stderr, "Unknown option: -%c\n", *opt);
                    print_usage(argv[0]);
                    return 1;
            }
        }
    }

    // Check for sufficient arguments
    if (argc - i < 2) {
        fprintf(stderr, "Error: Missing operand\n");
        print_usage(argv[0]);
        return 1;
    }

    // Get target (last argument)
    target = argv[argc - 1];

    // Multiple source files case
    if (argc - i > 2) {
        if (!options.no_target_dir && stat(target, &target_stat) == 0 && S_ISDIR(target_stat.st_mode)) {
            // Copy multiple files to directory
            for (; i < argc - 1; i++) {
                char dst_path[PATH_MAX];
                char *basename_ptr = basename(argv[i]);
                snprintf(dst_path, sizeof(dst_path), "%s/%s", target, basename_ptr);
                
                struct stat src_stat;
                if (stat(argv[i], &src_stat) == 0) {
                    if (S_ISDIR(src_stat.st_mode)) {
                        if (options.recursive) {
                            if (copy_directory(argv[i], dst_path, &options) == -1)
                                return 1;
                        } else {
                            fprintf(stderr, "Error: Omitting directory '%s'\n", argv[i]);
                        }
                    } else {
                        if (copy_file(argv[i], dst_path, &options) == -1)
                            return 1;
                    }
                }
            }
        } else {
            fprintf(stderr, "Error: Target '%s' is not a directory\n", target);
            return 1;
        }
    } else {
        // Single source file case
        struct stat src_stat;
        if (stat(argv[i], &src_stat) == 0) {
            if (S_ISDIR(src_stat.st_mode)) {
                if (options.recursive) {
                    if (copy_directory(argv[i], target, &options) == -1)
                        return 1;
                } else {
                    fprintf(stderr, "Error: Omitting directory '%s'\n", argv[i]);
                    return 1;
                }
            } else {
                if (copy_file(argv[i], target, &options) == -1)
                    return 1;
            }
        } else {
            fprintf(stderr, "Error: Cannot stat '%s': %s\n", argv[i], strerror(errno));
            return 1;
        }
    }

    return 0;
}