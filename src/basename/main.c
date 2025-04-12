/**
 * basename - extract filename from pathname
 *
 * Part of ASD CoreUtils package
 * Copyright (c) 2025 AnmiTaliDev
 * Licensed under Apache License 2.0
 *
 * A more flexible, functional and faster alternative to GNU CoreUtils basename
 */

 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <stdbool.h>
 #include <unistd.h>
 #include <getopt.h>
 
 #define VERSION "1.0.0"
 
 void print_usage(const char *program_name) {
     fprintf(stderr, "Usage: %s [OPTION]... NAME [SUFFIX]\n", program_name);
     fprintf(stderr, "Print NAME with any leading directory components removed.\n");
     fprintf(stderr, "If specified, also remove a trailing SUFFIX.\n\n");
     fprintf(stderr, "Options:\n");
     fprintf(stderr, "  -a, --multiple       support multiple arguments and treat each as a NAME\n");
     fprintf(stderr, "  -s, --suffix=SUFFIX  remove a trailing SUFFIX; implies -a\n");
     fprintf(stderr, "  -z, --zero           end each output line with NUL, not newline\n");
     fprintf(stderr, "      --help           display this help and exit\n");
     fprintf(stderr, "      --version        output version information and exit\n");
 }
 
 void print_version() {
     printf("basename (ASD CoreUtils) %s\n", VERSION);
     printf("Copyright (c) 2025 AnmiTaliDev\n");
     printf("Licensed under the Apache License, Version 2.0\n");
     printf("\n");
     printf("Written by AnmiTaliDev.\n");
 }
 
 char *extract_basename(const char *path) {
     if (!path || *path == '\0') {
         return strdup("");
     }
     
     // Find the last non-trailing slash
     const char *last_slash = path;
     const char *p = path;
     
     while (*p) {
         if (*p == '/' && *(p + 1) != '\0') {
             last_slash = p + 1;
         }
         p++;
     }
     
     // Handle trailing slashes
     size_t len = strlen(last_slash);
     if (len > 0 && last_slash[len - 1] == '/') {
         len--;
     }
     
     // Handle the case of "/" or "//"
     if (len == 0 && last_slash > path) {
         return strdup("/");
     }
     
     char *result = malloc(len + 1);
     if (!result) {
         perror("malloc");
         exit(EXIT_FAILURE);
     }
     
     strncpy(result, last_slash, len);
     result[len] = '\0';
     
     return result;
 }
 
 void remove_suffix(char *filename, const char *suffix) {
     if (!suffix || *suffix == '\0') {
         return;
     }
     
     size_t name_len = strlen(filename);
     size_t suffix_len = strlen(suffix);
     
     if (name_len >= suffix_len) {
         char *suffix_pos = filename + name_len - suffix_len;
         if (strcmp(suffix_pos, suffix) == 0) {
             *suffix_pos = '\0';
         }
     }
 }
 
 int main(int argc, char *argv[]) {
     bool multiple_args = false;
     bool zero_terminated = false;
     char *suffix = NULL;
     
     static struct option long_options[] = {
         {"multiple", no_argument, NULL, 'a'},
         {"suffix", required_argument, NULL, 's'},
         {"zero", no_argument, NULL, 'z'},
         {"help", no_argument, NULL, 'h'},
         {"version", no_argument, NULL, 'v'},
         {NULL, 0, NULL, 0}
     };
     
     int opt;
     while ((opt = getopt_long(argc, argv, "as:z", long_options, NULL)) != -1) {
         switch (opt) {
             case 'a':
                 multiple_args = true;
                 break;
             case 's':
                 suffix = optarg;
                 multiple_args = true;  // -s implies -a
                 break;
             case 'z':
                 zero_terminated = true;
                 break;
             case 'h':
                 print_usage(argv[0]);
                 return EXIT_SUCCESS;
             case 'v':
                 print_version();
                 return EXIT_SUCCESS;
             default:
                 print_usage(argv[0]);
                 return EXIT_FAILURE;
         }
     }
     
     // Check if we have enough arguments
     if (optind >= argc) {
         fprintf(stderr, "%s: missing operand\n", argv[0]);
         fprintf(stderr, "Try '%s --help' for more information.\n", argv[0]);
         return EXIT_FAILURE;
     }
     
     // Handle the case with one NAME and one SUFFIX when not in multiple mode
     if (!multiple_args && optind + 2 == argc) {
         suffix = argv[optind + 1];
     }
 
     // Process each NAME argument
     for (int i = optind; i < argc; i++) {
         // Skip SUFFIX when not in multiple mode
         if (!multiple_args && i == optind + 1)
             continue;
             
         char *basename = extract_basename(argv[i]);
         
         if (suffix) {
             remove_suffix(basename, suffix);
         }
         
         printf("%s%c", basename, zero_terminated ? '\0' : '\n');
         free(basename);
     }
     
     return EXIT_SUCCESS;
 }