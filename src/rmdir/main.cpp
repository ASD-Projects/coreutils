// ASD CoreUtils - rmdir
// Author: AnmiTaliDev
// License: Apache 2.0

#include <iostream>
#include <filesystem>
#include <vector>
#include <string>
#include <algorithm>
#include <stdexcept>
#include <cstring>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

namespace fs = std::filesystem;

// Helper function to remove a directory
bool remove_directory(const fs::path& path, bool verbose) {
    if (rmdir(path.c_str()) == 0) {
        if (verbose) {
            std::cout << "rmdir: removed '" << path.string() << "'" << std::endl;
        }
        return true;
    } else {
        if (errno == ENOTEMPTY || errno == EEXIST) {
            if (verbose) {
                std::cerr << "rmdir: failed to remove '" << path.string() << "': Directory not empty" << std::endl;
            }
        } else if (errno == ENOENT) {
            if (verbose) {
                std::cerr << "rmdir: failed to remove '" << path.string() << "': No such file or directory" << std::endl;
            }
        } else {
            if (verbose) {
                std::cerr << "rmdir: failed to remove '" << path.string() << "': Error " << strerror(errno) << std::endl;
            }
        }
        return false;
    }
}

int main(int argc, char *argv[]) {
    bool verbose = false;
    bool ignore_fail_non_empty = false;
    bool parents = false;
    std::vector<fs::path> directories_to_remove;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-v" || arg == "--verbose") {
            verbose = true;
        } else if (arg == "--ignore-fail-on-non-empty") {
            ignore_fail_non_empty = true;
        } else if (arg == "-p" || arg == "--parents") {
            parents = true;
        } else if (arg[0] == '-') {
            std::cerr << "rmdir: invalid option -- '" << arg << "'" << std::endl;
            std::cerr << "Try 'rmdir --help' for more information." << std::endl;
            return 1;
        } else {
            directories_to_remove.push_back(arg);
        }
    }

    if (argc == 1) {
        std::cout << "Usage: rmdir [OPTION]... DIRECTORY..." << std::endl;
        std::cout << "Remove empty directories." << std::endl;
        std::cout << std::endl;
        std::cout << "  -p, --parents        remove DIRECTORY and its ancestors if they are empty" << std::endl;
        std::cout << "      --ignore-fail-on-non-empty" << std::endl;
        std::cout << "                         ignore each failure that is solely because a directory is non-empty" << std::endl;
        std::cout << "  -v, --verbose        output a diagnostic for every directory processed" << std::endl;
        std::cout << "      --help           display this help and exit" << std::endl;
        std::cout << "      --version        output version information and exit" << std::endl;
        std::cout << std::endl;
        std::cout << "Author AnmiTaliDev." << std::endl;
        std::cout << "Copyright (C) 2025 ASD CoreUtils contributors" << std::endl;
        std::cout << "License Apache 2.0 <https://www.apache.org/licenses/LICENSE-2.0>." << std::endl;
        return 0;
    }

    if (std::any_of(argv + 1, argv + argc, [](const char* arg) { return std::string(arg) == "--version"; })) {
        std::cout << "rmdir (ASD CoreUtils) 1.0" << std::endl;
        std::cout << "Author AnmiTaliDev." << std::endl;
        std::cout << "License Apache 2.0" << std::endl;
        return 0;
    }

    int exit_code = 0;

    if (parents) {
        for (const auto& target_dir : directories_to_remove) {
            fs::path current_dir = target_dir;
            while (!current_dir.empty()) {
                std::error_code ec;
                if (fs::is_directory(current_dir, ec)) {
                    if (!fs::is_empty(current_dir, ec)) {
                        if (verbose) {
                            std::cerr << "rmdir: failed to remove '" << current_dir.string() << "': Directory not empty" << std::endl;
                        }
                        exit_code = 1;
                        break; // Don't try to remove parent directories if the current one is not empty
                    }
                    if (!remove_directory(current_dir, verbose)) {
                        exit_code = 1;
                        break; // Stop removing parent directories on error
                    }
                    current_dir = current_dir.parent_path();
                } else if (fs::exists(current_dir, ec)) {
                    if (verbose) {
                        std::cerr << "rmdir: failed to remove '" << current_dir.string() << "': Not a directory" << std::endl;
                    }
                    exit_code = 1;
                    break;
                } else {
                    if (verbose) {
                        std::cerr << "rmdir: failed to remove '" << current_dir.string() << "': No such file or directory" << std::endl;
                    }
                    exit_code = 1;
                    break;
                }
                if (ec) {
                    std::cerr << "rmdir: error checking '" << current_dir.string() << "': " << ec.message() << std::endl;
                    exit_code = 1;
                    break;
                }
            }
        }
    } else {
        for (const auto& target_dir : directories_to_remove) {
            std::error_code ec;
            if (!fs::is_directory(target_dir, ec)) {
                if (fs::exists(target_dir, ec)) {
                    std::cerr << "rmdir: failed to remove '" << target_dir.string() << "': Not a directory" << std::endl;
                } else {
                    std::cerr << "rmdir: failed to remove '" << target_dir.string() << "': No such file or directory" << std::endl;
                }
                exit_code = 1;
                continue;
            }
            if (!fs::is_empty(target_dir, ec)) {
                if (!ignore_fail_non_empty) {
                    std::cerr << "rmdir: failed to remove '" << target_dir.string() << "': Directory not empty" << std::endl;
                    exit_code = 1;
                } else if (verbose) {
                    std::cout << "rmdir: failed to remove '" << target_dir.string() << "': Directory not empty (ignored)" << std::endl;
                }
                continue;
            }
            if (!remove_directory(target_dir, verbose)) {
                exit_code = 1;
            }
            if (ec) {
                std::cerr << "rmdir: error operating on '" << target_dir.string() << "': " << ec.message() << std::endl;
                exit_code = 1;
            }
        }
    }

    return exit_code;
}