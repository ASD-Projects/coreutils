/*
 * ASD CoreUtils - tail
 * Copyright (c) 2025 AnmiTaliDev
 * Licensed under the Apache License, Version 2.0
 *
 * A flexible, functional and high-performance alternative to GNU CoreUtils tail
 */

 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <unistd.h>
 #include <fcntl.h>
 #include <errno.h>
 #include <sys/stat.h>
 #include <ctype.h>
 #include <getopt.h>
 
 #define DEFAULT_LINES 10
 #define BUFFER_SIZE 8192
 #define MAX_FILENAME_LENGTH 4096
 
 typedef struct {
     int lines;              // Number of lines to output (-n option)
     int bytes;              // Number of bytes to output (-c option)
     int follow;             // Whether to follow file (-f option)
     int quiet;              // Suppress headers (-q option)
     int sleep_interval;     // Sleep interval for -f option (in seconds)
     char **files;           // Array of filenames
     int file_count;         // Number of files
 } options_t;
 
 // Print usage information
 void print_usage(const char *program_name) {
     fprintf(stderr, "ASD CoreUtils Tail - Faster alternative to GNU tail\n");
     fprintf(stderr, "Usage: %s [OPTION]... [FILE]...\n", program_name);
     fprintf(stderr, "Print the last 10 lines of each FILE to standard output.\n");
     fprintf(stderr, "With more than one FILE, precede each with a header giving the file name.\n\n");
     fprintf(stderr, "Options:\n");
     fprintf(stderr, "  -n, --lines=NUM       output the last NUM lines, instead of the last 10\n");
     fprintf(stderr, "  -c, --bytes=NUM       output the last NUM bytes\n");
     fprintf(stderr, "  -f, --follow          output appended data as the file grows\n");
     fprintf(stderr, "  -q, --quiet           never output headers giving file names\n");
     fprintf(stderr, "  -s, --sleep-interval=NUM  with -f, sleep for approximately NUM seconds\n");
     fprintf(stderr, "      --help            display this help and exit\n");
     fprintf(stderr, "      --version         output version information and exit\n\n");
     fprintf(stderr, "If the first character of NUM is '+', output starts with the NUMth item.\n");
     fprintf(stderr, "With no FILE, or when FILE is -, read standard input.\n\n");
 }
 
 // Print version information
 void print_version() {
     printf("ASD CoreUtils Tail 1.0\n");
     printf("Copyright (C) 2025 AnmiTaliDev\n");
     printf("License Apache 2.0: Apache License, Version 2.0\n");
     printf("This is free software: you are free to change and redistribute it.\n");
     printf("There is NO WARRANTY, to the extent permitted by law.\n\n");
     printf("Written by AnmiTaliDev.\n");
 }
 
 // Parse command-line options
 options_t parse_options(int argc, char *argv[]) {
     options_t opts = {
         .lines = DEFAULT_LINES,
         .bytes = -1,
         .follow = 0,
         .quiet = 0,
         .sleep_interval = 1,
         .files = NULL,
         .file_count = 0
     };
 
     static struct option long_options[] = {
         {"lines", required_argument, 0, 'n'},
         {"bytes", required_argument, 0, 'c'},
         {"follow", no_argument, 0, 'f'},
         {"quiet", no_argument, 0, 'q'},
         {"silent", no_argument, 0, 'q'},
         {"sleep-interval", required_argument, 0, 's'},
         {"help", no_argument, 0, 'h'},
         {"version", no_argument, 0, 'v'},
         {0, 0, 0, 0}
     };
 
     int c;
     while ((c = getopt_long(argc, argv, "n:c:fqs:hv", long_options, NULL)) != -1) {
         switch (c) {
             case 'n':
                 if (optarg[0] == '+') {
                     opts.lines = -atoi(optarg + 1);
                 } else {
                     opts.lines = atoi(optarg);
                 }
                 break;
             case 'c':
                 if (optarg[0] == '+') {
                     opts.bytes = -atoi(optarg + 1);
                 } else {
                     opts.bytes = atoi(optarg);
                 }
                 break;
             case 'f':
                 opts.follow = 1;
                 break;
             case 'q':
                 opts.quiet = 1;
                 break;
             case 's':
                 opts.sleep_interval = atoi(optarg);
                 if (opts.sleep_interval < 0) {
                     opts.sleep_interval = 1;
                 }
                 break;
             case 'h':
                 print_usage(argv[0]);
                 exit(EXIT_SUCCESS);
             case 'v':
                 print_version();
                 exit(EXIT_SUCCESS);
             case '?':
                 print_usage(argv[0]);
                 exit(EXIT_FAILURE);
         }
     }
 
     // Handle input files
     opts.file_count = argc - optind;
     
     if (opts.file_count > 0) {
         opts.files = &argv[optind];
     } else {
         // If no files specified, use stdin
         static char *stdin_file[] = {"-"};
         opts.files = stdin_file;
         opts.file_count = 1;
     }
 
     return opts;
 }
 
 // Count lines in a buffer
 int count_lines(const char *buffer, size_t size) {
     int count = 0;
     for (size_t i = 0; i < size; i++) {
         if (buffer[i] == '\n') {
             count++;
         }
     }
     return count;
 }
 
 // Tail a file by lines
 void tail_by_lines(FILE *file, int lines, int start_from_line) {
     if (lines <= 0) {
         return;
     }
 
     // If start_from_line is provided (+ syntax was used)
     if (start_from_line > 0) {
         int current_line = 0;
         char buffer[BUFFER_SIZE];
         
         // Skip lines until we reach the start line
         while (current_line < start_from_line - 1 && fgets(buffer, sizeof(buffer), file) != NULL) {
             if (strchr(buffer, '\n') != NULL) {
                 current_line++;
             }
         }
         
         // Output the rest of the file
         while (fgets(buffer, sizeof(buffer), file) != NULL) {
             fputs(buffer, stdout);
         }
         return;
     }
 
     // For regular tail behavior, using a circular buffer approach
     char **line_buffer = (char **)malloc(lines * sizeof(char *));
     if (!line_buffer) {
         perror("Memory allocation error");
         return;
     }
     
     for (int i = 0; i < lines; i++) {
         line_buffer[i] = NULL;
     }
     
     int line_count = 0;
     int pos = 0;
     char buffer[BUFFER_SIZE];
     
     while (fgets(buffer, sizeof(buffer), file) != NULL) {
         size_t len = strlen(buffer);
         
         // Free previous line if it exists
         if (line_buffer[pos]) {
             free(line_buffer[pos]);
         }
         
         // Store the new line
         line_buffer[pos] = (char *)malloc(len + 1);
         if (!line_buffer[pos]) {
             perror("Memory allocation error");
             break;
         }
         
         strcpy(line_buffer[pos], buffer);
         pos = (pos + 1) % lines;
         if (line_count < lines) {
             line_count++;
         }
     }
     
     // Output the lines in the right order
     for (int i = 0; i < line_count; i++) {
         int idx = (pos + i) % lines;
         fputs(line_buffer[idx], stdout);
     }
     
     // Free all allocated memory
     for (int i = 0; i < lines; i++) {
         if (line_buffer[i]) {
             free(line_buffer[i]);
         }
     }
     free(line_buffer);
 }
 
 // Tail a file by bytes
 void tail_by_bytes(FILE *file, int bytes, int start_from_byte) {
     if (bytes <= 0) {
         return;
     }
 
     // If start_from_byte is provided (+ syntax was used)
     if (start_from_byte > 0) {
         // Skip to the specified byte position
         fseek(file, start_from_byte - 1, SEEK_SET);
         
         // Output the rest of the file
         char buffer[BUFFER_SIZE];
         size_t bytes_read;
         while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
             fwrite(buffer, 1, bytes_read, stdout);
         }
         return;
     }
 
     // For regular tail behavior
     int fd = fileno(file);
     off_t file_size;
     
     // Get file size
     file_size = lseek(fd, 0, SEEK_END);
     if (file_size == -1) {
         perror("Error seeking file");
         return;
     }
     
     // If bytes is greater than file size, just output the whole file
     if (bytes >= file_size) {
         lseek(fd, 0, SEEK_SET);
         FILE *new_file = fdopen(dup(fd), "r");
         if (!new_file) {
             perror("Error duplicating file descriptor");
             return;
         }
         
         char buffer[BUFFER_SIZE];
         size_t bytes_read;
         while ((bytes_read = fread(buffer, 1, sizeof(buffer), new_file)) > 0) {
             fwrite(buffer, 1, bytes_read, stdout);
         }
         fclose(new_file);
         return;
     }
     
     // Seek to the right position
     if (lseek(fd, -bytes, SEEK_END) == -1) {
         perror("Error seeking file");
         return;
     }
     
     // Read and output the last 'bytes' bytes
     char *buffer = (char *)malloc(bytes);
     if (!buffer) {
         perror("Memory allocation error");
         return;
     }
     
     ssize_t bytes_read = read(fd, buffer, bytes);
     if (bytes_read > 0) {
         fwrite(buffer, 1, bytes_read, stdout);
     }
     free(buffer);
 }
 
 // Follow a file for changes
 void follow_file(const char *filename, const options_t *opts) {
     FILE *file;
     struct stat stat_buf;
     off_t last_size;
     
     // Open the file
     if (strcmp(filename, "-") == 0) {
         file = stdin;
         fprintf(stderr, "Cannot follow standard input\n");
         return;
     } else {
         file = fopen(filename, "r");
         if (!file) {
             perror(filename);
             return;
         }
     }
     
     // Get initial file size
     int fd = fileno(file);
     if (fstat(fd, &stat_buf) == -1) {
         perror("fstat");
         fclose(file);
         return;
     }
     last_size = stat_buf.st_size;
     
     // First output the end of the file
     if (opts->bytes > 0) {
         tail_by_bytes(file, opts->bytes, 0);
     } else {
         tail_by_lines(file, opts->lines, 0);
     }
     fflush(stdout);
     
     // Then keep checking for changes
     while (1) {
         sleep(opts->sleep_interval);
         
         // Check if file has changed
         if (fstat(fd, &stat_buf) == -1) {
             perror("fstat");
             break;
         }
         
         // If file size increased, output the new data
         if (stat_buf.st_size > last_size) {
             if (fseek(file, last_size, SEEK_SET) == -1) {
                 perror("fseek");
                 break;
             }
             
             char buffer[BUFFER_SIZE];
             size_t bytes_read;
             while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
                 fwrite(buffer, 1, bytes_read, stdout);
             }
             fflush(stdout);
             
             last_size = stat_buf.st_size;
         } else if (stat_buf.st_size < last_size) {
             // File was truncated, reopen it
             fclose(file);
             file = fopen(filename, "r");
             if (!file) {
                 perror(filename);
                 break;
             }
             fd = fileno(file);
             last_size = stat_buf.st_size;
         }
     }
     
     fclose(file);
 }
 
 // Main function
 int main(int argc, char *argv[]) {
     options_t opts = parse_options(argc, argv);
     
     for (int i = 0; i < opts.file_count; i++) {
         FILE *file;
         
         // Print header if multiple files and not quiet
         if (opts.file_count > 1 && !opts.quiet) {
             printf("\n==> %s <==\n", strcmp(opts.files[i], "-") == 0 ? "standard input" : opts.files[i]);
         }
         
         // Open file
         if (strcmp(opts.files[i], "-") == 0) {
             file = stdin;
         } else {
             file = fopen(opts.files[i], "r");
             if (!file) {
                 perror(opts.files[i]);
                 continue;
             }
         }
         
         // Follow file if requested
         if (opts.follow) {
             follow_file(opts.files[i], &opts);
         } else {
             // Either tail by bytes or lines
             if (opts.bytes > 0) {
                 tail_by_bytes(file, opts.bytes, 0);
             } else if (opts.bytes < 0) {
                 tail_by_bytes(file, opts.bytes, -opts.bytes);
             } else if (opts.lines < 0) {
                 tail_by_lines(file, -1, -opts.lines);
             } else {
                 tail_by_lines(file, opts.lines, 0);
             }
         }
         
         // Close file if not stdin
         if (file != stdin) {
             fclose(file);
         }
     }
     
     return EXIT_SUCCESS;
 }