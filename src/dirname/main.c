/**
 * dirname - ASD CoreUtils implementation
 * 
 * A flexible, functional and faster alternative to GNU CoreUtils dirname
 * 
 * Copyright (c) 2025 AnmiTaliDev
 * Licensed under the Apache License, Version 2.0
 */

 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <stdbool.h>
 #include <unistd.h>
 #include <libgen.h>
 #include <getopt.h>
 
 #define VERSION "1.0.0"
 
 void print_usage(const char *program_name) {
     fprintf(stderr, "Usage: %s [OPTION]... NAME...\n", program_name);
     fprintf(stderr, "Output each NAME with its last non-slash component and trailing slashes removed.\n");
     fprintf(stderr, "If NAME contains no /'s, output '.' (meaning the current directory).\n\n");
     fprintf(stderr, "Options:\n");
     fprintf(stderr, "  -z, --zero     end each output line with NUL, not newline\n");
     fprintf(stderr, "  -h, --help     display this help and exit\n");
     fprintf(stderr, "  -v, --version  output version information and exit\n");
 }
 
 void print_version() {
     printf("ASD CoreUtils dirname %s\n", VERSION);
     printf("Copyright (c) 2025 AnmiTaliDev\n");
     printf("Licensed under the Apache License, Version 2.0\n");
     printf("Written by AnmiTaliDev.\n");
 }
 
 char* custom_dirname(char *path) {
     if (!path || !*path) {
         return strdup(".");
     }
 
     char *p = path + strlen(path) - 1;
     
     // Remove trailing slashes
     while (p > path && *p == '/') {
         *p-- = '\0';
     }
     
     // If path is all slashes or empty, return appropriate value
     if (*path == '\0') {
         return strdup("/");
     }
     
     // Find the last slash
     while (p >= path && *p != '/') {
         p--;
     }
     
     // No slashes found
     if (p < path) {
         return strdup(".");
     }
     
     // Remove trailing slashes from the directory part
     while (p > path && *p == '/') {
         p--;
     }
     
     // If we ended up with path being all slashes
     if (p < path) {
         return strdup("/");
     }
     
     // Extract the directory part
     *(p + 1) = '\0';
     return strdup(path);
 }
 
 int main(int argc, char *argv[]) {
     int opt;
     bool use_null_separator = false;
     
     static struct option long_options[] = {
         {"zero", no_argument, 0, 'z'},
         {"help", no_argument, 0, 'h'},
         {"version", no_argument, 0, 'v'},
         {0, 0, 0, 0}
     };
     
     while ((opt = getopt_long(argc, argv, "zhv", long_options, NULL)) != -1) {
         switch (opt) {
             case 'z':
                 use_null_separator = true;
                 break;
             case 'h':
                 print_usage(argv[0]);
                 return 0;
             case 'v':
                 print_version();
                 return 0;
             default:
                 fprintf(stderr, "Try '%s --help' for more information.\n", argv[0]);
                 return 1;
         }
     }
     
     if (optind >= argc) {
         fprintf(stderr, "%s: missing operand\n", argv[0]);
         fprintf(stderr, "Try '%s --help' for more information.\n", argv[0]);
         return 1;
     }
     
     for (int i = optind; i < argc; i++) {
         char *input = strdup(argv[i]);
         if (!input) {
             perror("Memory allocation failed");
             return 1;
         }
         
         char *dir = custom_dirname(input);
         
         printf("%s%c", dir, use_null_separator ? '\0' : '\n');
         
         free(input);
         free(dir);
     }
     
     return 0;
 }