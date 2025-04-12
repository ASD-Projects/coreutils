/**
 * pwd.c - Print Working Directory utility for ASD CoreUtils
 * 
 * A high-performance alternative to GNU CoreUtils pwd
 * Outputs the current working directory.
 * 
 * Features:
 * - Optimized for speed with minimal syscalls
 * - Uses low-level system functions for better performance
 * - Handles long paths gracefully
 * - Supports -L (logical) and -P (physical) path options
 */

 #include <stdio.h>
 #include <stdlib.h>
 #include <unistd.h>
 #include <string.h>
 #include <limits.h>
 #include <getopt.h>
 #include <errno.h>
 
 #define ASD_VERSION "1.0.0"
 
 void print_usage(const char *program_name) {
     fprintf(stderr, "Usage: %s [OPTION]...\n", program_name);
     fprintf(stderr, "Print the full filename of the current working directory.\n\n");
     fprintf(stderr, "  -L, --logical    use PWD from environment, even if it contains symlinks\n");
     fprintf(stderr, "  -P, --physical   avoid all symlinks (default)\n");
     fprintf(stderr, "      --help       display this help and exit\n");
     fprintf(stderr, "      --version    output version information and exit\n");
     exit(EXIT_FAILURE);
 }
 
 void print_version() {
     printf("pwd (ASD CoreUtils) %s\n", ASD_VERSION);
     printf("Copyright (C) 2025 ASD Software\n");
     printf("License: Apache 2.0\n");
     exit(EXIT_SUCCESS);
 }
 
 int main(int argc, char *argv[]) {
     int use_logical = 0;
     char cwd[PATH_MAX];
     
     static struct option long_options[] = {
         {"logical",  no_argument, 0, 'L'},
         {"physical", no_argument, 0, 'P'},
         {"help",     no_argument, 0, 'h'},
         {"version",  no_argument, 0, 'v'},
         {0, 0, 0, 0}
     };
     
     int opt;
     while ((opt = getopt_long(argc, argv, "LP", long_options, NULL)) != -1) {
         switch (opt) {
             case 'L':
                 use_logical = 1;
                 break;
             case 'P':
                 use_logical = 0;
                 break;
             case 'h':
                 print_usage(argv[0]);
                 break;
             case 'v':
                 print_version();
                 break;
             default:
                 print_usage(argv[0]);
         }
     }
     
     if (use_logical) {
         char *pwd = getenv("PWD");
         if (pwd != NULL) {
             printf("%s\n", pwd);
             return EXIT_SUCCESS;
         }
         // Fall back to physical path if PWD is not set
     }
     
     // Get physical path using getcwd()
     if (getcwd(cwd, sizeof(cwd)) != NULL) {
         printf("%s\n", cwd);
         return EXIT_SUCCESS;
     } else {
         if (errno == ERANGE) {
             // Path too long for buffer, allocate dynamically
             char *dynamic_cwd = NULL;
             size_t size = PATH_MAX * 2;
             
             while (1) {
                 dynamic_cwd = (char *)realloc(dynamic_cwd, size);
                 if (dynamic_cwd == NULL) {
                     perror("asd-pwd: memory allocation error");
                     return EXIT_FAILURE;
                 }
                 
                 if (getcwd(dynamic_cwd, size) != NULL) {
                     printf("%s\n", dynamic_cwd);
                     free(dynamic_cwd);
                     return EXIT_SUCCESS;
                 }
                 
                 if (errno != ERANGE) {
                     break;
                 }
                 
                 size *= 2; // Double buffer size and try again
             }
             
             free(dynamic_cwd);
         }
         
         perror("asd-pwd");
         return EXIT_FAILURE;
     }
 }