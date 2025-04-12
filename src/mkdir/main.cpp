#include <iostream>
#include <string>
#include <vector>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <cstdlib>
#include <cstring>
#include <unistd.h>

//
// mkdir - create directories
//
// Part of the ASD CoreUtils package
// Author: AnmiTaliDev
// License: Apache License 2.0
//

namespace ASDCoreUtils {
namespace Mkdir {

constexpr const char* version = "1.0";

void print_usage() {
    std::cout << "Usage: mkdir [OPTION]... DIRECTORY..." << std::endl;
    std::cout << "Create the DIRECTORY(ies), if they do not already exist." << std::endl;
    std::cout << std::endl;
    std::cout << "OPTIONS:" << std::endl;
    std::cout << "  -m, --mode=MODE   set file mode (as in chmod), not a=rwx - umask" << std::endl;
    std::cout << "  -p, --parents     no error if existing, make parent directories as needed" << std::endl;
    std::cout << "  -v, --verbose     print a message for each created directory" << std::endl;
    std::cout << "  --version         output version information and exit" << std::endl;
    std::cout << "  -h, --help        display this help and exit" << std::endl;
    std::cout << std::endl;
    std::cout << "Examples:" << std::endl;
    std::cout << "  mkdir test        Create a directory named 'test'." << std::endl;
    std::cout << "  mkdir -p a/b/c    Create directories 'a', 'a/b', and 'a/b/c'." << std::endl;
    std::cout << "  mkdir -m 755 dir   Create a directory 'dir' with mode 755." << std::endl;
}

int create_directory(const std::string& path, mode_t mode, bool parents, bool verbose) {
    if (!parents) {
        if (mkdir(path.c_str(), mode) == 0) {
            if (verbose) {
                std::cout << "mkdir: created directory '" << path << "'" << std::endl;
            }
            return 0;
        } else {
            std::cerr << "mkdir: cannot create directory '" << path << "': " << strerror(errno) << std::endl;
            return 1;
        }
    } else {
        std::string current_path;
        for (size_t i = 0; i < path.length(); ++i) {
            current_path += path[i];
            if (path[i] == '/' || i == path.length() - 1) {
                if (!current_path.empty()) {
                    if (mkdir(current_path.c_str(), mode) != 0 && errno != EEXIST) {
                        std::cerr << "mkdir: cannot create directory '" << current_path << "': " << strerror(errno) << std::endl;
                        return 1;
                    } else if (errno != EEXIST && verbose) {
                        std::cout << "mkdir: created directory '" << current_path << "'" << std::endl;
                    }
                }
            }
        }
        return 0;
    }
}

mode_t parse_mode(const std::string& mode_str) {
    mode_t mode = 0;
    for (char c : mode_str) {
        if (c >= '0' && c <= '7') {
            mode = (mode << 3) | (c - '0');
        } else {
            std::cerr << "mkdir: invalid mode: '" << mode_str << "'" << std::endl;
            return -1; // Indicate an error
        }
    }
    return mode;
}

} // namespace Mkdir
} // namespace ASDCoreUtils

int main(int argc, char *argv[]) {
    bool parents = false;
    bool verbose = false;
    mode_t mode = S_IRWXU | S_IRWXG | S_IRWXO; // a=rwx by default
    std::vector<std::string> paths;

    int opt;
    while ((opt = getopt(argc, argv, "pm:vh")) != -1) {
        switch (opt) {
            case 'p':
                parents = true;
                break;
            case 'm': {
                mode_t parsed_mode = ASDCoreUtils::Mkdir::parse_mode(optarg);
                if (parsed_mode == static_cast<mode_t>(-1)) {
                    return 1;
                }
                mode = parsed_mode;
                break;
            }
            case 'v':
                verbose = true;
                break;
            case 'h':
                ASDCoreUtils::Mkdir::print_usage();
                return 0;
            case '?':
                std::cerr << "mkdir: invalid option: '" << (char)optopt << "'" << std::endl;
                ASDCoreUtils::Mkdir::print_usage();
                return 1;
            default:
                // Should not reach here
                break;
        }
    }

    for (int i = optind; i < argc; ++i) {
        paths.push_back(argv[i]);
    }

    if (argc == 1) {
        ASDCoreUtils::Mkdir::print_usage();
        return 1;
    }

    if (paths.empty()) {
        std::cerr << "mkdir: missing operand" << std::endl;
        ASDCoreUtils::Mkdir::print_usage();
        return 1;
    }

    int exit_code = 0;
    for (const auto& path : paths) {
        if (ASDCoreUtils::Mkdir::create_directory(path, mode, parents, verbose) != 0) {
            exit_code = 1;
        }
    }

    return exit_code;
}