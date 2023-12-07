#pragma once

#include <fstream>
#include <string>
#include <iomanip>
#include <algorithm>
#include <initializer_list>

static std::string padStart(std::string input, size_t length, char padChar) {
    return input.length() >= length ? input : std::string(length - input.length(), padChar) + input;
}

static std::string toUpperCase(std::string input) {
    for (char& c : input) {
        c = std::toupper(c);
    }

    return input;
}

static std::string replaceStr(std::string str, const std::string& from, const std::string& to) {
    size_t start_pos = 0;
    start_pos = str.find(from, start_pos);
    if (start_pos != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length();
    }
    return str;
}

static std::string replaceMultiStr(std::string str, std::initializer_list<std::string> from, std::initializer_list<std::string> to) {

    for (int i = 0; i < from.size(); i++) {
        size_t start_pos = 0;
        start_pos = str.find(from.begin()[i], start_pos);

        if (start_pos != std::string::npos) {
            str.replace(start_pos, from.begin()[i].length(), to.begin()[i]);
            start_pos += to.begin()[i].length();
        }
    }

    return str;
}

static std::string eraseUntilChar(std::string str, const std::string& from, const std::string& to) {
    size_t firstPos = str.find(from);
    size_t secondPos = str.find(to, firstPos);

    if (firstPos != std::string::npos && secondPos != std::string::npos && secondPos > firstPos) {
        str.erase(firstPos, secondPos - firstPos);
    }

    return str;
}

static char* locationFix(std::string path) {
    char* fixed = new char[path.length() + 1];
    std::strcpy(fixed, path.c_str());

    std::replace(fixed, fixed + strlen(fixed), '\\', '/');

    auto last = std::unique(fixed, fixed + strlen(fixed), [](char a, char b) {
        return a == '/' && b == '/';
        });
    *last = '\0';

#ifdef _WIN32
    if (fixed[1] == ':') {
        fixed[0] = std::toupper(fixed[0]);
    }
#endif

    return fixed;
}


static char* generateHex(int length) {
    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    static const char hex_characters[] = "0123456789ABCDEF";

    char* result = new char[length + 1];
    for (int i = 0; i < length; ++i) {
        int randomIndex = std::rand() % 16;
        result[i] = hex_characters[randomIndex];
    }

    result[length] = '\0';
    return result;
}

static bool isDirectory(const char* path) {
    struct stat fileInfo;
    if (stat(path, &fileInfo) != 0) {
        return false;
    }

    return (fileInfo.st_mode & S_IFDIR);
}

static char* stringToCharp(std::string str) {
    char* data = new char[str.size()];
    std::copy(str.begin(), str.end(), data);
    data[str.size()] = '\0';
    return data;
}

static char* getFilenameByPath(const char* path, bool ext = false) {

    std::string pathStr(path);
    size_t found = pathStr.find_last_of("/\\");
    if (found == std::string::npos) {
        return nullptr;
    }

    std::string gameName = pathStr.substr(found + 1);
    if (!ext) {
        size_t last = gameName.find_last_of(".");
        if (last != std::string::npos) {
            gameName = gameName.substr(0, last);
        }
    }

    return stringToCharp(gameName);
}

static char* decimalToHex(int number, int lenght) {
    std::ostringstream stream;
    stream << std::setw(lenght) << std::setfill('0') << std::uppercase << std::hex << number;
    return stringToCharp(stream.str());
}

static int __strcmpi(const char* str1, const char* str2) {
    int result;
#ifdef _WIN32
    result = _strcmpi(str1, str2);
#else
    result = strcasecmp(str1, str2);
#endif

    return result;
}
static size_t findFromCharArray(char* arr[], const char* searchText, size_t arrSize) {
    for (size_t i = 0; i < arrSize; ++i) {
        if (strstr(arr[i], searchText) != nullptr) {
            return i;
        }
    }

    return -1;
}