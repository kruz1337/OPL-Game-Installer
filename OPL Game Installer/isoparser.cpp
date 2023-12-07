#include "isoparser.h"
#include "utilities.h"

static FILE* isof;

static bool readsect(l9660_fs* fs, void* buf, uint32_t sector)
{
    if (_fseeki64(isof, 2048 * sector, SEEK_SET)) {
        fprintf(stderr, "Reading sector %u\n", sector);
        perror("fseek");
        return false;
    }

    if (fread(buf, 2048, 1, isof) != 1) {
        fprintf(stderr, "Reading sector %u\n", sector);
        perror("fread");
        return false;
    }

    return true;
}

char* IsoParser::readIsoFile(const char* path, const char* insidePath) {
    isof = fopen(path, "rb");
    if (!isof) {
        printf("[-] Failed to open '%s' path.\n", path);
        return nullptr;
    }

    l9660_fs fs;
    l9660_dir dir;
    l9660_openfs(&fs, readsect);

    l9660_fs_open_root(&dir, &fs);

    l9660_file file;
    l9660_openat(&file, &dir, insidePath);

    size_t totalRead = 0;

    char* content = nullptr;

    while (true) {
        char buf[128];
        size_t bytesRead;
        l9660_read(&file, buf, 128, &bytesRead);

        if (bytesRead == 0) {
            break;
        }

        content = new char[bytesRead];
        std::memcpy(content, buf, bytesRead);
        totalRead += bytesRead;
    }

    if (totalRead > 0) {
        content[totalRead] = '\0';
    }

    fclose(isof);

    return content;
}

char* IsoParser::parseConfig(const char* conf, const char* option) {
    std::string strBuffer(conf);

    size_t found = strBuffer.find(option);
    if (found == std::string::npos) {
        return nullptr;
    }

    std::string afterOption = strBuffer.substr(found);

    size_t endOfLine = afterOption.find_first_of("\n");
    if (endOfLine == std::string::npos) {
        return nullptr;
    }

    std::string removeLines = strBuffer.substr(found, afterOption.find_first_of("\n") - 1);
    std::string afterProp = removeLines.substr(strlen(option) + 3);

    return stringToCharp(afterProp);
}

char* IsoParser::getGameID(const char* isoPath) {

    char* config = IsoParser::readIsoFile(isoPath, "SYSTEM.CNF");
    if (!config) {
        return nullptr;
    }

    std::string boot = IsoParser::parseConfig(config, "BOOT2");
    size_t p = boot.find_first_of("\\");
    if (p == std::string::npos) {
        return nullptr;
    }

    std::string boot2 = boot.substr(p + 1);
    std::string gameId = boot2.substr(0, boot2.length() - 2);

    return stringToCharp(gameId);
}