#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <getopt.h>

#define BUFFER_SIZE (16 * 1024)  // 16KB buffer size for optimal performance
#define VERSION "1.0.0"

// Structure to hold program options
struct Options {
    bool show_ends;        // -E flag
    bool show_tabs;        // -T flag
    bool show_nonprinting; // -v flag
    bool squeeze_blank;    // -s flag
    bool show_all;         // -A flag
    bool number_lines;     // -n flag
    bool number_nonblank;  // -b flag
};

// Function prototypes
static void usage(const char *program_name);
static void version(void);
static int process_file(const char *filename, const struct Options *opts);
static int process_stdin(const struct Options *opts);
static void display_char(unsigned char c, const struct Options *opts);

int main(int argc, char *argv[]) {
    struct Options opts = {0};
    int c;
    int exit_status = EXIT_SUCCESS;

    static struct option long_options[] = {
        {"show-ends",       no_argument, 0, 'E'},
        {"number",         no_argument, 0, 'n'},
        {"squeeze-blank",  no_argument, 0, 's'},
        {"show-tabs",      no_argument, 0, 'T'},
        {"show-nonprinting", no_argument, 0, 'v'},
        {"show-all",       no_argument, 0, 'A'},
        {"number-nonblank", no_argument, 0, 'b'},
        {"help",           no_argument, 0, 'h'},
        {"version",        no_argument, 0, 'V'},
        {0, 0, 0, 0}
    };

    while ((c = getopt_long(argc, argv, "EnsTvAbhV", long_options, NULL)) != -1) {
        switch (c) {
            case 'E': opts.show_ends = true; break;
            case 'n': opts.number_lines = true; break;
            case 's': opts.squeeze_blank = true; break;
            case 'T': opts.show_tabs = true; break;
            case 'v': opts.show_nonprinting = true; break;
            case 'A': opts.show_all = true; break;
            case 'b': opts.number_nonblank = true; break;
            case 'h': usage(argv[0]); return EXIT_SUCCESS;
            case 'V': version(); return EXIT_SUCCESS;
            default: usage(argv[0]); return EXIT_FAILURE;
        }
    }

    // If no files specified, process stdin
    if (optind == argc) {
        return process_stdin(&opts);
    }

    // Process each file
    for (; optind < argc; optind++) {
        if (process_file(argv[optind], &opts) != EXIT_SUCCESS) {
            exit_status = EXIT_FAILURE;
        }
    }

    return exit_status;
}

static void usage(const char *program_name) {
    fprintf(stderr, "Usage: %s [OPTION]... [FILE]...\n", program_name);
    fprintf(stderr, "Concatenate FILE(s) to standard output.\n\n");
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -A, --show-all          equivalent to -vET\n");
    fprintf(stderr, "  -b, --number-nonblank   number nonempty output lines\n");
    fprintf(stderr, "  -E, --show-ends         display $ at end of each line\n");
    fprintf(stderr, "  -n, --number            number all output lines\n");
    fprintf(stderr, "  -s, --squeeze-blank     suppress repeated empty output lines\n");
    fprintf(stderr, "  -T, --show-tabs         display TAB characters as ^I\n");
    fprintf(stderr, "  -v, --show-nonprinting  use ^ and M- notation, except for LFD and TAB\n");
    fprintf(stderr, "      --help              display this help and exit\n");
    fprintf(stderr, "      --version           output version information and exit\n");
}

static void version(void) {
    printf("asd-cat %s\n", VERSION);
    printf("Part of ASD CoreUtils - https://github.com/ASD-Projects/coreutils\n");
    printf("Copyright (C) 2025 ASD CoreUtils Contributors\n");
    printf("License: Apache 2.0\n");
}

static int process_file(const char *filename, const struct Options *opts) {
    int fd;
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;
    static int line_number = 1;
    bool last_was_blank = false;
    
    fd = strcmp(filename, "-") == 0 ? STDIN_FILENO : open(filename, O_RDONLY);
    if (fd == -1) {
        fprintf(stderr, "asd-cat: %s: %s\n", filename, strerror(errno));
        return EXIT_FAILURE;
    }

    while ((bytes_read = read(fd, buffer, BUFFER_SIZE)) > 0) {
        for (ssize_t i = 0; i < bytes_read; i++) {
            unsigned char c = buffer[i];
            
            // Handle line numbering
            if ((opts->number_lines || opts->number_nonblank) && 
                (i == 0 || buffer[i-1] == '\n')) {
                if (!opts->number_nonblank || c != '\n') {
                    printf("%6d\t", line_number++);
                }
            }

            // Handle squeeze blank lines
            if (opts->squeeze_blank && c == '\n') {
                if (last_was_blank) {
                    continue;
                }
                last_was_blank = true;
            } else {
                last_was_blank = false;
            }

            display_char(c, opts);
        }
    }

    if (bytes_read == -1) {
        fprintf(stderr, "asd-cat: %s: %s\n", filename, strerror(errno));
        if (fd != STDIN_FILENO) {
            close(fd);
        }
        return EXIT_FAILURE;
    }

    if (fd != STDIN_FILENO) {
        close(fd);
    }
    return EXIT_SUCCESS;
}

static int process_stdin(const struct Options *opts) {
    return process_file("-", opts);
}

static void display_char(unsigned char c, const struct Options *opts) {
    if (opts->show_nonprinting) {
        if (c < 32 && c != '\t' && c != '\n') {
            putchar('^');
            putchar(c + 64);
            return;
        } else if (c >= 127) {
            putchar('M');
            putchar('-');
            if (c >= 128 + 32) {
                putchar(c - 128);
            } else {
                putchar('^');
                putchar(c - 128 + 64);
            }
            return;
        }
    }

    if (opts->show_tabs && c == '\t') {
        printf("^I");
        return;
    }

    putchar(c);

    if (opts->show_ends && c == '\n') {
        putchar('$');
    }
}