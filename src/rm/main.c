#define _XOPEN_SOURCE 500
#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <ftw.h>
#include <getopt.h>
#include <sys/stat.h>

#define PROGRAM_NAME "rm"
#define AUTHORS "AnmiTaliDev"

// Configuration structure
struct rm_options {
    int recursive;      // -r, --recursive
    int force;         // -f, --force
    int verbose;       // -v, --verbose
    int interactive;   // -i, --interactive
    int preserve_root; // --preserve-root
    int no_preserve_root; // --no-preserve-root
};

static struct rm_options options = {
    .recursive = 0,
    .force = 0,
    .verbose = 0,
    .interactive = 0,
    .preserve_root = 1,
    .no_preserve_root = 0
};

// Function prototypes
static int remove_file(const char *path);
static int remove_directory_recursive(const char *path);
static void usage(int status);

// Custom implementation of remove_directory_recursive using nftw
static int handle_remove(const char *fpath, const struct stat *sb, 
                        int typeflag, struct FTW *ftwbuf) {
    (void)sb;      // Suppress unused parameter warning
    (void)typeflag; // Suppress unused parameter warning
    (void)ftwbuf;  // Suppress unused parameter warning
    
    int rv = remove(fpath);
    
    if (rv && !options.force) {
        fprintf(stderr, "%s: failed to remove '%s': %s\n", 
                PROGRAM_NAME, fpath, strerror(errno));
        return -1;
    }
    
    if (options.verbose) {
        printf("removed '%s'\n", fpath);
    }
    
    return 0;
}

// Remove a single file
static int remove_file(const char *path) {
    if (options.interactive && !options.force) {
        char response;
        printf("rm: remove regular file '%s'? ", path);
        response = getchar();
        if (response != 'y' && response != 'Y') {
            return 0;
        }
        while (response != '\n' && response != EOF) {
            response = getchar();
        }
    }

    if (remove(path) != 0) {
        if (!options.force) {
            fprintf(stderr, "%s: cannot remove '%s': %s\n", 
                    PROGRAM_NAME, path, strerror(errno));
            return 1;
        }
        return 0;
    }

    if (options.verbose) {
        printf("removed '%s'\n", path);
    }

    return 0;
}

// Remove directory recursively using nftw
static int remove_directory_recursive(const char *path) {
    // FTW_DEPTH: Perform post-order traversal
    // FTW_PHYS: Do not follow symbolic links
    return nftw(path, handle_remove, 64, FTW_DEPTH | FTW_PHYS);
}

// Print usage information
static void usage(int status) {
    if (status != EXIT_SUCCESS) {
        fprintf(stderr, "Try '%s --help' for more information.\n", PROGRAM_NAME);
    } else {
        printf("Usage: %s [OPTION]... [FILE]...\n", PROGRAM_NAME);
        printf("Remove (unlink) the FILE(s).\n\n");
        printf("  -f, --force           ignore nonexistent files and arguments, never prompt\n");
        printf("  -i, --interactive     prompt before every removal\n");
        printf("  -r, -R, --recursive   remove directories and their contents recursively\n");
        printf("  -v, --verbose         explain what is being done\n");
        printf("      --help            display this help and exit\n");
        printf("      --preserve-root   do not remove '/' (default)\n");
        printf("      --no-preserve-root  do not treat '/' specially\n");
        printf("\nBy default, rm does not remove directories. Use the --recursive (-r or -R)\n");
        printf("option to remove each listed directory, too, along with all of its contents.\n\n");
    }
    exit(status);
}

int main(int argc, char *argv[]) {
    int c;
    int exit_status = EXIT_SUCCESS;

    static struct option long_options[] = {
        {"force",       no_argument, NULL, 'f'},
        {"interactive", no_argument, NULL, 'i'},
        {"recursive",   no_argument, NULL, 'r'},
        {"verbose",     no_argument, NULL, 'v'},
        {"help",        no_argument, NULL, 'h'},
        {"preserve-root", no_argument, &options.preserve_root, 1},
        {"no-preserve-root", no_argument, &options.no_preserve_root, 1},
        {NULL, 0, NULL, 0}
    };

    while ((c = getopt_long(argc, argv, "firRv", long_options, NULL)) != -1) {
        switch (c) {
            case 'f':
                options.force = 1;
                options.interactive = 0;
                break;
            case 'i':
                options.interactive = 1;
                options.force = 0;
                break;
            case 'r':
            case 'R':
                options.recursive = 1;
                break;
            case 'v':
                options.verbose = 1;
                break;
            case 'h':
                usage(EXIT_SUCCESS);
                break;
            case 0:
                // Flag setting handled automatically for long options
                break;
            default:
                usage(EXIT_FAILURE);
        }
    }

    if (optind >= argc) {
        fprintf(stderr, "%s: missing operand\n", PROGRAM_NAME);
        usage(EXIT_FAILURE);
    }

    // Process each file argument
    for (int i = optind; i < argc; i++) {
        struct stat st;
        
        // Check if path exists
        if (lstat(argv[i], &st) == -1) {
            if (!options.force) {
                fprintf(stderr, "%s: cannot remove '%s': %s\n",
                        PROGRAM_NAME, argv[i], strerror(errno));
                exit_status = EXIT_FAILURE;
            }
            continue;
        }

        // Check for root directory protection
        if (options.preserve_root && !options.no_preserve_root && 
            strcmp(argv[i], "/") == 0) {
            fprintf(stderr, "%s: refusing to remove root directory '/'\n", 
                    PROGRAM_NAME);
            exit_status = EXIT_FAILURE;
            continue;
        }

        if (S_ISDIR(st.st_mode)) {
            if (!options.recursive) {
                fprintf(stderr, "%s: cannot remove '%s': Is a directory\n",
                        PROGRAM_NAME, argv[i]);
                exit_status = EXIT_FAILURE;
                continue;
            }
            if (remove_directory_recursive(argv[i]) != 0) {
                exit_status = EXIT_FAILURE;
            }
        } else {
            if (remove_file(argv[i]) != 0) {
                exit_status = EXIT_FAILURE;
            }
        }
    }

    return exit_status;
}