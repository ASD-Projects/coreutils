#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdexcept>

#ifdef _WIN32
# include <windows.h>
#else
# include <utime.h>
#endif

const std::string VERSION = "1.0.0";
const std::string HELP_TEXT = R"(
touch - update file timestamps or create new files

USAGE:
   touch [OPTIONS] FILE...

OPTIONS:
   -a             change only the access time
   -c             do not create any files
   -m             change only the modification time
   -r REF         use this file's times instead of current time
   -t STAMP       use [[CC]YY]MMDDhhmm[.ss] instead of current time
   -h, --help     display this help and exit
   -v             output version information and exit

Examples:
   touch file.txt             Create file.txt or update its timestamps
   touch -c existing.txt     Update timestamps only if file exists
   touch -r ref.txt file     Copy timestamps from ref.txt to file
   touch -t 202301011200 f   Set timestamp to Jan 1, 2023, 12:00
)";

struct Options {
    bool access = false;
    bool create = true;
    bool modify = false;
    std::string reference = "";
    std::string timestamp = "";
    bool help = false;
    bool version = false;
    std::vector<std::string> files;
};

time_t parseTimestamp(const std::string& stamp) {
    std::tm t{};
    std::string s = stamp;

    if (s.length() >= 12) { // CCYYMMDDhhmm
        std::istringstream ss(s.substr(0, 4));
        ss >> t.tm_year;
        if (ss.fail()) throw std::runtime_error("Invalid year in timestamp");
        t.tm_year -= 1900; // tm_year is years since 1900
        s = s.substr(4);
    } else if (s.length() >= 10) { // YYMMDDhhmm
        int year_short;
        std::istringstream ss(s.substr(0, 2));
        ss >> year_short;
        if (ss.fail()) throw std::runtime_error("Invalid year in timestamp");
        t.tm_year = (year_short >= 70 ? 1900 : 2000) + year_short - 1900;
        s = s.substr(2);
    }

    if (s.length() >= 8) {
        std::istringstream ss_month(s.substr(0, 2));
        ss_month >> t.tm_mon;
        if (ss_month.fail() || t.tm_mon < 1 || t.tm_mon > 12) throw std::runtime_error("Invalid month in timestamp");
        t.tm_mon--; // tm_mon is 0-indexed

        std::istringstream ss_day(s.substr(2, 2));
        ss_day >> t.tm_mday;
        if (ss_day.fail() || t.tm_mday < 1 || t.tm_mday > 31) throw std::runtime_error("Invalid day in timestamp");

        std::istringstream ss_hour(s.substr(4, 2));
        ss_hour >> t.tm_hour;
        if (ss_hour.fail() || t.tm_hour < 0 || t.tm_hour > 23) throw std::runtime_error("Invalid hour in timestamp");

        std::istringstream ss_minute(s.substr(6, 2));
        ss_minute >> t.tm_min;
        if (ss_minute.fail() || t.tm_min < 0 || t.tm_min > 59) throw std::runtime_error("Invalid minute in timestamp");

        if (s.length() > 8 && s[8] == '.') {
            std::istringstream ss_second(s.substr(9));
            ss_second >> t.tm_sec;
            if (ss_second.fail() || t.tm_sec < 0 || t.tm_sec > 59) throw std::runtime_error("Invalid second in timestamp");
        } else {
            t.tm_sec = 0;
        }
    } else if (!s.empty()) {
        throw std::runtime_error("Invalid timestamp format");
    } else {
        t.tm_sec = 0;
    }

    t.tm_isdst = -1; // Let the system determine if DST is in effect
    std::time_t tt = std::mktime(&t);
    if (tt == -1) {
        throw std::runtime_error("Error converting timestamp");
    }
    return tt;
}

Options parseOptions(int argc, char *argv[]) {
    Options options;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-a") {
            options.access = true;
        } else if (arg == "-c") {
            options.create = false;
        } else if (arg == "-m") {
            options.modify = true;
        } else if (arg == "-r") {
            if (i + 1 < argc) {
                options.reference = argv[++i];
            } else {
                std::cerr << "touch: option '-r' requires an argument" << std::endl;
                exit(1);
            }
        } else if (arg == "-t") {
            if (i + 1 < argc) {
                options.timestamp = argv[++i];
            } else {
                std::cerr << "touch: option '-t' requires an argument" << std::endl;
                exit(1);
            }
        } else if (arg == "-h" || arg == "--help") {
            options.help = true;
        } else if (arg == "-v") {
            options.version = true;
        } else if (arg[0] == '-') {
            std::cerr << "touch: invalid option '" << arg << "'" << std::endl;
            std::cerr << "Try 'touch --help' for more information." << std::endl;
            exit(1);
        } else {
            options.files.push_back(arg);
        }
    }

    if (!options.access && !options.modify) {
        options.access = true;
        options.modify = true;
    }

    return options;
}

bool fileExists(const std::string& path) {
    std::ifstream f(path.c_str());
    return f.good();
}

time_t getLastAccessTime(const std::string& path) {
    struct stat fileInfo;
    if (stat(path.c_str(), &fileInfo) == 0) {
#ifdef _WIN32
        FILETIME ftAccess;
        HANDLE hFile = CreateFileA(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile != INVALID_HANDLE_VALUE) {
            if (GetFileTime(hFile, NULL, &ftAccess, NULL)) {
                ULARGE_INTEGER ui;
                ui.LowPart = ftAccess.dwLowDateTime;
                ui.HighPart = ftAccess.dwHighDateTime;
                CloseHandle(hFile);
                return (ui.QuadPart - 116444736000000000ULL) / 10000000ULL;
            }
            CloseHandle(hFile);
        }
        return 0; // Error case
#else
        return fileInfo.st_atime;
#endif
    } else {
        return 0; // Error case
    }
}

time_t getLastModificationTime(const std::string& path) {
    struct stat fileInfo;
    if (stat(path.c_str(), &fileInfo) == 0) {
#ifdef _WIN32
        FILETIME ftWrite;
        HANDLE hFile = CreateFileA(path.c_str(), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile != INVALID_HANDLE_VALUE) {
            if (GetFileTime(hFile, NULL, NULL, &ftWrite)) {
                ULARGE_INTEGER ui;
                ui.LowPart = ftWrite.dwLowDateTime;
                ui.HighPart = ftWrite.dwHighDateTime;
                CloseHandle(hFile);
                return (ui.QuadPart - 116444736000000000ULL) / 10000000ULL;
            }
            CloseHandle(hFile);
        }
        return 0; // Error case
#else
        return fileInfo.st_mtime;
#endif
    } else {
        return 0; // Error case
    }
}

bool setFileTimes(const std::string& path, time_t accessTime, time_t modifyTime) {
#ifdef _WIN32
    FILETIME ftAccess, ftWrite;
    ULARGE_INTEGER uiAccess, uiWrite;

    uiAccess.QuadPart = (static_cast<ULONGLONG>(accessTime) * 10000000ULL) + 116444736000000000ULL;
    ftAccess.dwLowDateTime = uiAccess.LowPart;
    ftAccess.dwHighDateTime = uiAccess.HighPart;

    uiWrite.QuadPart = (static_cast<ULONGLONG>(modifyTime) * 10000000ULL) + 116444736000000000ULL;
    ftWrite.dwLowDateTime = uiWrite.LowPart;
    ftWrite.dwHighDateTime = uiWrite.HighPart;

    HANDLE hFile = CreateFileA(path.c_str(), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile != INVALID_HANDLE_VALUE) {
        BOOL result = SetFileTime(hFile, NULL, &ftAccess, &ftWrite);
        CloseHandle(hFile);
        return result != 0;
    }
    return false;
#else
    struct utimbuf times;
    times.actime = accessTime;
    times.modtime = modifyTime;
    return utime(path.c_str(), &times) == 0;
#endif
}

bool setLastAccessTime(const std::string& path, time_t accessTime) {
#ifdef _WIN32
    FILETIME ftAccess;
    ULARGE_INTEGER uiAccess;

    uiAccess.QuadPart = (static_cast<ULONGLONG>(accessTime) * 10000000ULL) + 116444736000000000ULL;
    ftAccess.dwLowDateTime = uiAccess.LowPart;
    ftAccess.dwHighDateTime = uiAccess.HighPart;

    HANDLE hFile = CreateFileA(path.c_str(), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile != INVALID_HANDLE_VALUE) {
        FILETIME ftCreation, ftWrite;
        if (GetFileTime(hFile, &ftCreation, NULL, &ftWrite)) {
            BOOL result = SetFileTime(hFile, &ftCreation, &ftAccess, &ftWrite);
            CloseHandle(hFile);
            return result != 0;
        }
        CloseHandle(hFile);
    }
    return false;
#else
    struct stat fileInfo;
    if (stat(path.c_str(), &fileInfo) == 0) {
        struct utimbuf times;
        times.actime = accessTime;
        times.modtime = fileInfo.st_mtime;
        return utime(path.c_str(), &times) == 0;
    }
    return false;
#endif
}

bool setLastModificationTime(const std::string& path, time_t modifyTime) {
#ifdef _WIN32
    FILETIME ftWrite;
    ULARGE_INTEGER uiWrite;

    uiWrite.QuadPart = (static_cast<ULONGLONG>(modifyTime) * 10000000ULL) + 116444736000000000ULL;
    ftWrite.dwLowDateTime = uiWrite.LowPart;
    ftWrite.dwHighDateTime = uiWrite.HighPart;

    HANDLE hFile = CreateFileA(path.c_str(), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile != INVALID_HANDLE_VALUE) {
        FILETIME ftCreation, ftAccess;
        if (GetFileTime(hFile, &ftCreation, &ftAccess, NULL)) {
            BOOL result = SetFileTime(hFile, &ftCreation, &ftAccess, &ftWrite);
            CloseHandle(hFile);
            return result != 0;
        }
        CloseHandle(hFile);
    }
    return false;
#else
    struct stat fileInfo;
    if (stat(path.c_str(), &fileInfo) == 0) {
        struct utimbuf times;
        times.actime = fileInfo.st_atime;
        times.modtime = modifyTime;
        return utime(path.c_str(), &times) == 0;
    }
    return false;
#endif
}

void touchFile(const std::string& path, const Options& options) {
    if (!fileExists(path) && !options.create) {
        return;
    }

    time_t accessTime;
    time_t modifyTime;

    if (!options.reference.empty()) {
        if (!fileExists(options.reference)) {
            std::cerr << "Error: reference file '" << options.reference << "' not found" << std::endl;
            return;
        }
        accessTime = getLastAccessTime(options.reference);
        modifyTime = getLastModificationTime(options.reference);
    } else if (!options.timestamp.empty()) {
        try {
            time_t parsedTime = parseTimestamp(options.timestamp);
            accessTime = parsedTime;
            modifyTime = parsedTime;
        } catch (const std::runtime_error& e) {
            std::cerr << "Error: " << e.what() << std::endl;
            exit(1);
        }
    } else {
        time(&accessTime);
        modifyTime = accessTime;
    }

    if (!fileExists(path)) {
        std::ofstream f(path);
        if (f.fail()) {
            std::cerr << "Error touching file: " << path << std::endl;
            return;
        }
        f.close();
    }

    if (options.access && options.modify) {
        if (!setFileTimes(path, accessTime, modifyTime)) {
            std::cerr << "Error touching file: " << path << std::endl;
        }
    } else if (options.access) {
        if (!setLastAccessTime(path, accessTime)) {
            std::cerr << "Error touching file: " << path << std::endl;
        }
    } else if (options.modify) {
        if (!setLastModificationTime(path, modifyTime)) {
            std::cerr << "Error touching file: " << path << std::endl;
        }
    }
}

int main(int argc, char *argv[]) {
    Options options = parseOptions(argc, argv);

    if (options.help) {
        std::cout << HELP_TEXT << std::endl;
        return 0;
    }

    if (options.version) {
        std::cout << "ASD CoreUtils touch " << VERSION << std::endl;
        std::cout << "Copyright © 2025 AnmiTaliDev" << std::endl;
        std::cout << "License: Apache 2.0" << std::endl;
        std::cout << "This is free software: you are free to change and distribute it." << std::endl;
        std::cout << "There is NO WARRANTY, to the extent permitted by law." << std::endl;
        return 0;
    }

    if (options.files.empty()) {
        std::cerr << "touch: missing file operand" << std::endl;
        std::cerr << "Try 'touch --help' for more information." << std::endl;
        return 1;
    }
    for (const auto& file : options.files) {
        touchFile(file, options);
    }
    return 0;
}