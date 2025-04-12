#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <libgen.h>
#include <getopt.h>

#define BUFFER_SIZE 8192
#define VERSION "1.0.0"

// Flags for move operations
static int force_flag = 0;
static int interactive_flag = 0;
static int verbose_flag = 0;
static int no_clobber_flag = 0;

void print_usage(const char *program_name) {
    fprintf(stderr, "Usage: %s [OPTION]... SOURCE DEST\n", program_name);
    fprintf(stderr, "   or: %s [OPTION]... SOURCE... DIRECTORY\n", program_name);
    fprintf(stderr, "\nOptions:\n");
    fprintf(stderr, "  -f, --force         force move, override destination if exists\n");
    fprintf(stderr, "  -i, --interactive   prompt before overwrite\n");
    fprintf(stderr, "  -n, --no-clobber    do not overwrite existing files\n");
    fprintf(stderr, "  -v, --verbose       explain what is being done\n");
    fprintf(stderr, "      --version       display version information\n");
    fprintf(stderr, "      --help          display this help\n");
    exit(EXIT_FAILURE);
}

void print_version(void) {
    printf("ASD mv %s - part of ASD CoreUtils\n", VERSION);
    printf("Copyright (C) 2025 AnmiTaliDev\n");
    printf("License: Apache 2.0\n");
    exit(EXIT_SUCCESS);
}

int confirm_overwrite(const char *dest) {
    char response[10];
    printf("mv: overwrite '%s'? ", dest);
    fflush(stdout);
    
    if (fgets(response, sizeof(response), stdin)) {
        return response[0] == 'y' || response[0] == 'Y';
    }
    return 0;
}

int check_same_file(const char *source, const char *dest) {
    struct stat src_stat, dst_stat;
    
    if (stat(source, &src_stat) == 0 && stat(dest, &dst_stat) == 0) {
        return (src_stat.st_dev == dst_stat.st_dev && 
                src_stat.st_ino == dst_stat.st_ino);
    }
    return 0;
}

int move_file(const char *source, const char *dest) {
    // Try simple rename first
    if (rename(source, dest) == 0) {
        if (verbose_flag) {
            printf("'%s' -> '%s'\n", source, dest);
        }
        return 0;
    }
    
    // If rename fails due to cross-device move, fallback to copy and delete
    if (errno == EXDEV) {
        int source_fd, dest_fd;
        ssize_t bytes_read, bytes_written;
        char buffer[BUFFER_SIZE];
        struct stat source_stat;
        
        // Get source file stats
        if (stat(source, &source_stat) != 0) {
            perror("mv: failed to stat source file");
            return -1;
        }
        
        // Open source file
        source_fd = open(source, O_RDONLY);
        if (source_fd == -1) {
            perror("mv: failed to open source file");
            return -1;
        }
        
        // Create destination file with same permissions
        dest_fd = open(dest, O_WRONLY | O_CREAT | O_TRUNC, source_stat.st_mode);
        if (dest_fd == -1) {
            close(source_fd);
            perror("mv: failed to create destination file");
            return -1;
        }
        
        // Copy data
        while ((bytes_read = read(source_fd, buffer, BUFFER_SIZE)) > 0) {
            bytes_written = write(dest_fd, buffer, bytes_read);
            if (bytes_written != bytes_read) {
                close(source_fd);
                close(dest_fd);
                unlink(dest);
                perror("mv: write error");
                return -1;
            }
        }
        
        // Clean up
        close(source_fd);
        close(dest_fd);
        
        // Copy succeeded, remove source file
        if (unlink(source) != 0) {
            perror("mv: failed to remove source file");
            return -1;
        }
        
        if (verbose_flag) {
            printf("'%s' -> '%s'\n", source, dest);
        }
        return 0;
    }
    
    perror("mv: rename failed");
    return -1;
}

int main(int argc, char *argv[]) {
    int c;
    struct stat dest_stat;
    
    static struct option long_options[] = {
        {"force", no_argument, 0, 'f'},
        {"interactive", no_argument, 0, 'i'},
        {"no-clobber", no_argument, 0, 'n'},
        {"verbose", no_argument, 0, 'v'},
        {"version", no_argument, 0, 'V'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };
    
    while ((c = getopt_long(argc, argv, "finv", long_options, NULL)) != -1) {
        switch (c) {
            case 'f':
                force_flag = 1;
                interactive_flag = 0;
                no_clobber_flag = 0;
                break;
            case 'i':
                interactive_flag = 1;
                force_flag = 0;
                no_clobber_flag = 0;
                break;
            case 'n':
                no_clobber_flag = 1;
                force_flag = 0;
                interactive_flag = 0;
                break;
            case 'v':
                verbose_flag = 1;
                break;
            case 'V':
                print_version();
                break;
            case 'h':
                print_usage(argv[0]);
                break;
            default:
                print_usage(argv[0]);
        }
    }
    
    // Check if we have enough arguments
    if (argc - optind < 2) {
        print_usage(argv[0]);
    }
    
    // Get the last argument (destination)
    const char *dest = argv[argc - 1];
    int dest_exists = (stat(dest, &dest_stat) == 0);
    int dest_is_dir = dest_exists && S_ISDIR(dest_stat.st_mode);
    
    // If we have multiple sources, destination must be a directory
    if (argc - optind > 2 && !dest_is_dir) {
        fprintf(stderr, "mv: target '%s' is not a directory\n", dest);
        exit(EXIT_FAILURE);
    }
    
    // Process each source file
    for (int i = optind; i < argc - 1; i++) {
        const char *source = argv[i];
        char *final_dest;
        
        // Skip if source doesn't exist
        if (access(source, F_OK) != 0) {
            fprintf(stderr, "mv: cannot stat '%s': No such file or directory\n", source);
            continue;
        }
        
        if (dest_is_dir) {
            char *base = basename(strdup(source));
            size_t dest_len = strlen(dest);
            size_t base_len = strlen(base);
            final_dest = malloc(dest_len + base_len + 2);
            sprintf(final_dest, "%s/%s", dest, base);
            free(base);
        } else {
            final_dest = strdup(dest);
        }
        
        // Check if source and destination are the same file
        if (check_same_file(source, final_dest)) {
            fprintf(stderr, "mv: '%s' and '%s' are the same file\n", source, final_dest);
            free(final_dest);
            continue;
        }
        
        // Check if destination exists
        if (access(final_dest, F_OK) == 0) {
            if (no_clobber_flag) {
                if (verbose_flag) {
                    printf("mv: not overwriting '%s'\n", final_dest);
                }
                free(final_dest);
                continue;
            }
            
            if (interactive_flag && !confirm_overwrite(final_dest)) {
                free(final_dest);
                continue;
            }
            
            if (!force_flag && access(final_dest, W_OK) != 0) {
                fprintf(stderr, "mv: cannot remove '%s': Permission denied\n", final_dest);
                free(final_dest);
                continue;
            }
        }
        
        if (move_file(source, final_dest) != 0) {
            fprintf(stderr, "mv: failed to move '%s' to '%s'\n", source, final_dest);
        }
        
        free(final_dest);
    }
    
    return EXIT_SUCCESS;
}