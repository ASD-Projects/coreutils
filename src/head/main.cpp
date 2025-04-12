/**
 * ASD CoreUtils - Head Utility
 * Copyright (c) 2025 AnmiTaliDev
 * License: Apache License 2.0
 * 
 * A flexible, functional and fast alternative to GNU CoreUtils Head
 * Part of the ASD CoreUtils package
 */

 #include <iostream>
 #include <fstream>
 #include <string>
 #include <vector>
 #include <chrono>
 #include <algorithm>
 #include <unistd.h>
 #include <getopt.h>
 
 class ASDHead {
 private:
     // Default settings
     int64_t numLines = 10;
     int64_t numBytes = -1;
     bool quietMode = false;
     bool verboseMode = false;
     bool showFilename = false;
     std::vector<std::string> filenames;
     std::chrono::high_resolution_clock::time_point startTime;
 
     void printUsage() {
         std::cerr << "ASD Head: Display the beginning of files\n"
                   << "Usage: asd-head [OPTION]... [FILE]...\n"
                   << "Print the first 10 lines of each FILE to standard output.\n"
                   << "With more than one FILE, prefix each with a header giving the file name.\n\n"
                   << "Options:\n"
                   << "  -n, --lines=N         print the first N lines instead of the first 10\n"
                   << "  -c, --bytes=N         print the first N bytes\n"
                   << "  -q, --quiet           never print headers giving file names\n"
                   << "  -v, --verbose         always print headers giving file names\n"
                   << "  -h, --help            display this help and exit\n"
                   << "  -V, --version         output version information and exit\n\n"
                   << "If no FILE is specified, or when FILE is -, read standard input.\n\n"
                   << "Part of ASD CoreUtils - https://github.com/ASD-Projects/coreutils\n";
     }
 
     void printVersion() {
         std::cout << "ASD Head 1.0.0\n"
                   << "Copyright (c) 2025 AnmiTaliDev\n"
                   << "License: Apache License 2.0\n"
                   << "Part of ASD CoreUtils\n";
     }
 
     void reportPerformance() {
         auto endTime = std::chrono::high_resolution_clock::now();
         auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
         std::cerr << "Processing completed in " << duration.count() / 1000.0 << " ms\n";
     }
 
     void processFile(const std::string& filename) {
         std::istream* input;
         std::ifstream file;
         
         if (filename == "-") {
             input = &std::cin;
         } else {
             file.open(filename);
             if (!file) {
                 std::cerr << "asd-head: cannot open '" << filename << "' for reading: No such file or directory\n";
                 return;
             }
             input = &file;
         }
 
         // Print header if needed
         if ((showFilename || (filenames.size() > 1 && !quietMode)) && filename != "-") {
             std::cout << "==> " << filename << " <==\n";
         } else if ((showFilename || (filenames.size() > 1 && !quietMode)) && filename == "-") {
             std::cout << "==> standard input <==\n";
         }
 
         // Process by bytes if specified
         if (numBytes >= 0) {
             std::vector<char> buffer(numBytes);
             input->read(buffer.data(), numBytes);
             size_t bytesRead = input->gcount();
             std::cout.write(buffer.data(), bytesRead);
             if (filenames.size() > 1 && !quietMode && filename != filenames.back()) {
                 std::cout << "\n";
             }
             return;
         }
 
         // Process by lines
         std::string line;
         int64_t lineCount = 0;
         
         while (lineCount < numLines && std::getline(*input, line)) {
             std::cout << line << "\n";
             lineCount++;
         }
         
         if (filenames.size() > 1 && !quietMode && filename != filenames.back()) {
             std::cout << "\n";
         }
     }
 
 public:
     ASDHead() {
         startTime = std::chrono::high_resolution_clock::now();
     }
 
     bool parseArgs(int argc, char* argv[]) {
         static struct option longOptions[] = {
             {"bytes", required_argument, 0, 'c'},
             {"lines", required_argument, 0, 'n'},
             {"quiet", no_argument, 0, 'q'},
             {"verbose", no_argument, 0, 'v'},
             {"help", no_argument, 0, 'h'},
             {"version", no_argument, 0, 'V'},
             {0, 0, 0, 0}
         };
 
         int optionIndex = 0;
         int opt;
 
         while ((opt = getopt_long(argc, argv, "c:n:qvhV", longOptions, &optionIndex)) != -1) {
             switch (opt) {
                 case 'c':
                     numBytes = std::stoll(optarg);
                     numLines = -1; // If bytes specified, don't use lines
                     break;
                 case 'n':
                     numLines = std::stoll(optarg);
                     numBytes = -1; // If lines specified, don't use bytes
                     break;
                 case 'q':
                     quietMode = true;
                     verboseMode = false;
                     break;
                 case 'v':
                     verboseMode = true;
                     showFilename = true;
                     quietMode = false;
                     break;
                 case 'h':
                     printUsage();
                     return false;
                 case 'V':
                     printVersion();
                     return false;
                 default:
                     printUsage();
                     return false;
             }
         }
 
         // Collect filenames
         for (int i = optind; i < argc; i++) {
             filenames.push_back(argv[i]);
         }
 
         // If no files specified, use stdin
         if (filenames.empty()) {
             filenames.push_back("-");
         }
 
         return true;
     }
 
     void execute() {
         for (const auto& filename : filenames) {
             processFile(filename);
         }
         
         if (verboseMode) {
             reportPerformance();
         }
     }
 };
 
 int main(int argc, char* argv[]) {
     ASDHead head;
     
     if (!head.parseArgs(argc, argv)) {
         return 1;
     }
     
     head.execute();
     return 0;
 }