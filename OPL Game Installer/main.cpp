#include <iostream>
#include <future>

#include "isoparser.h"
#include "opl.h"
#include "utilities.h"

#ifdef _WIN32
#include <windows.h>
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING  0x0004
#endif
#define WAIT(ms) Sleep(ms)
#define MKDIR(dir) CreateDirectoryA(dir, NULL)
#define SETUP_CONSOLE() \
    do { \
        DWORD outMode = 0; \
        HANDLE stdoutHandle = GetStdHandle(STD_OUTPUT_HANDLE); \
        \
        if (stdoutHandle == INVALID_HANDLE_VALUE || !GetConsoleMode(stdoutHandle, &outMode)) { \
            exit(GetLastError()); \
        } \
        \
        DWORD outModeInit = outMode; \
        outMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING; \
        \
        if (!SetConsoleMode(stdoutHandle, outMode)) { \
            exit(GetLastError()); \
        } \
    } while(0)
#else
#include <unistd.h>
#define WAIT(ms) usleep(ms * 1000)
#define MKDIR(dir) mkdir(dir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == 0
#define SETUP_CONSOLE()
#endif
#define PAUSE() printf("\n\nPress any key to continue . . ."); (void)getchar();
#define CHECK_YES_NO_INPUT(type) (__strcmpi(type, "y") == 0 || __strcmpi(type, "yes") == 0 ? 1 : (__strcmpi(type, "n") == 0 || __strcmpi(type, "no") == 0 ? 0 : -1))
#define CLEAR() printf("\033[H\033[2J\033[3J")
#define CREATE_ASCII() printf("\x1B[31m%s\n\033[0m\t\t\n", R"(
                               ____                             __ _  __    ____                  
                              / __ \___  ____ ___  _____  _____/ /| |/ /   / __ \___ _   __  
                             / /_/ / _ \/ __ `/ / / / _ \/ ___/ __|   /   / / / / _ | | / /  
                            / _, _/  __/ /_/ / /_/ /  __(__  / /_/   |   / /_/ /  __| |/ _    
                           /_/ |_|\___/\__, /\__,_/\___/____/\__/_/|_|  /_____/\___/|___(_))")
#define CREATE_ERROR(msg, pause) CLEAR(); CREATE_ASCII(); printf(LIGHT_RED"%s" RESET, msg); if(pause) {PAUSE();}
#define SET_TITLE(title) printf("\033]0;%s\007", title);
#define RESET "\x1B[0m"
#define HIDE_CURSOR "\033[?25l"
#define SHOW_CURSOR "\033[?25h"
#define BLACK "\x1B[30m"
#define RED "\x1B[31m"
#define GREEN "\x1B[32m"
#define YELLOW "\x1B[33m"
#define BLUE "\x1B[34m"
#define MAGENTA "\x1B[35m"
#define CYAN "\x1B[36m"
#define WHITE "\x1B[37m"
#define LIGHT_BLACK "\x1B[90m"
#define LIGHT_RED "\x1B[91m"
#define LIGHT_GREEN "\x1B[92m"
#define LIGHT_YELLOW "\x1B[93m"
#define LIGHT_BLUE "\x1B[94m"
#define LIGHT_MAGENTA "\x1B[95m"
#define LIGHT_CYAN "\x1B[96m"
#define LIGHT_WHITE "\x1B[97m"
#define BG_BLACK "\x1B[40m"
#define BG_RED "\x1B[41m"
#define BG_GREEN "\x1B[42m"
#define BG_YELLOW "\x1B[43m"
#define BG_BLUE "\x1B[44m"
#define BG_MAGENTA "\x1B[45m"
#define BG_CYAN "\x1B[46m"
#define BG_WHITE "\x1B[47m"
#define BG_LIGHT_BLACK "\x1B[100m"
#define BG_LIGHT_RED "\x1B[101m"
#define BG_LIGHT_GREEN "\x1B[102m"
#define BG_LIGHT_YELLOW "\x1B[103m"
#define BG_LIGHT_BLUE "\x1B[104m"
#define BG_LIGHT_MAGENTA "\x1B[105m"
#define BG_LIGHT_CYAN "\x1B[106m"
#define BG_LIGHT_WHITE "\x1B[107m"

const std::streamsize chunkSize = 1LL * (1024 * 1024 * 1024); // (byte) equals 1gb > number of bytes to seperate files
const std::streamsize readSize = 4LL * (1024); // (byte) equals 4096b > total number of bytes to read each
const std::streamsize maxSize = 4LL * (1024 * 1024 * 1024); // (byte) equals 4gb > fat32 supported max size

int main(int argc, char* argv[]) {
    SETUP_CONSOLE();
    CREATE_ASCII();
    SET_TITLE("OPL Game Installer - github.com/kruz1337");

    std::string str_isoPath, str_copyFile, str_mediaType, str_outputDir, str_addCover, str_createVmc, str_sizeOfVmc;

    {
        if (argc > 1)
        {
            if (!argv[1] || !argv[2] || !argv[3]) {
                CREATE_ERROR("[-] Invalid argument type.", 0);
                return 1;
            }

            str_isoPath = argv[1];
            str_outputDir = argv[2];
            str_mediaType = argv[3];
            str_copyFile = findFromCharArray(argv, "--copy", argc) == -1 ? "n" : "y";
            str_addCover = findFromCharArray(argv, "--add-cover", argc) == -1 ? "n" : "y";

            int vmcArgPos = findFromCharArray(argv, "--create-vmc", argc);
            if (vmcArgPos == -1) {
                str_createVmc = "n";
            }
            else {
                str_createVmc = "y";
                str_sizeOfVmc = argv[vmcArgPos + 1];
            }
        }
    }

    if (argc <= 1) {
        printf("[?] Iso file path: \n> ");
        std::getline(std::cin, str_isoPath);
    }

    const char* isoPath = str_isoPath.c_str();
    std::ifstream input(isoPath, std::ios::binary);
    if (!input.is_open()) {
        CREATE_ERROR("[-] Failed to open input file.", 1);
        return 1;
    }

    char* gameId = IsoParser::getGameID(isoPath);
    if (!gameId)
    {
        CREATE_ERROR("[-] Invalid iso file.", 1);
        return 1;
    }

    input.seekg(0, input.end);
    std::streamsize isoSize = input.tellg();
    input.seekg(0, input.beg);

    int copyFile = 0;

    if (isoSize <= maxSize) {
        if (argc <= 1) {
            printf(YELLOW"\n[*] The file can be copied directly because the file size does not exceed the limit.\n" RESET);
            printf("[?] Do you want the file copied? (y/N) [Default: y]\n> ");
            std::getline(std::cin, str_copyFile);
        }

        copyFile = str_copyFile == "" ? 1 : CHECK_YES_NO_INPUT(str_copyFile.c_str());
        if (copyFile == -1) {
            CREATE_ERROR("[-] Invalid result type.", 1);
            return 1;
        }
    }

    if (argc <= 1) {
        printf("\n[?] Media type: (CD/DVD) [Default: DVD]\n> ");
        std::getline(std::cin, str_mediaType);
    }

    str_mediaType = str_mediaType == "" ? "DVD" : toUpperCase(str_mediaType);

    const char* mediaType = str_mediaType.c_str();
    if (strcmp(mediaType, "DVD") != 0 && strcmp(mediaType, "CD") != 0) {
        CREATE_ERROR("[-] Invalid result type.", 1);
        return 1;
    }

    if (argc <= 1) {
        printf("\n[?] OPL Root path: \n> ");
        std::getline(std::cin, str_outputDir);
    }

    const char* outputDir = str_outputDir.c_str();
    if (!isDirectory(outputDir)) {
        CREATE_ERROR("[-] Failed to open root dir.", 1);
        return 1;
    }

    if (argc <= 1) {
        printf(YELLOW "\n[*] This process need to internet connection." RESET);
        printf("\n[?] Do you want to add cover image? (y/N) [Default: n]\n> ");
        std::getline(std::cin, str_addCover);
    }

    int addCover = str_addCover == "" ? 0 : CHECK_YES_NO_INPUT(str_addCover.c_str());
    if (addCover == -1) {
        CREATE_ERROR("[-] Invalid result type.", 1);
        return 1;
    }

    if (argc <= 1) {
        printf(YELLOW "\n[*] It doesn't delete existing memory cards, but replaces the first slot with a newly created memory card." RESET);
        printf("\n[?] Do you want to create vmc for this game? (y/N) [Default: n]\n> ");
        std::getline(std::cin, str_createVmc);
    }

    int sizeOfVmc = 8;
    int createVmc = str_createVmc == "" ? 0 : CHECK_YES_NO_INPUT(str_createVmc.c_str());
    if (createVmc == -1) {
        CREATE_ERROR("[-] Invalid result type.", 1);
        return 1;
    }
    else if (createVmc == 1) {
        if (argc <= 1) {
            printf(YELLOW "\n[*] Memory card size should be minimum 1 mb and maximum 1024 mb." RESET);
            printf("\n[?] How much should be megabyte for memory card? [Default: 8]\n> ");
            std::getline(std::cin, str_sizeOfVmc);
        }

        if (str_sizeOfVmc != "") {
            try {
                sizeOfVmc = std::stoi(str_sizeOfVmc);
            }
            catch (...) {
                CREATE_ERROR("[-] Invalid memory card size.", 1);
                return 1;
            }
        }

        if ((sizeOfVmc < 1) || (sizeOfVmc > 1024)) {
            CREATE_ERROR("[-] Invalid memory card size.", 1);
            return 1;
        }
    }

    char* fileName = getFilenameByPath(isoPath, true);
    char* gameName = getFilenameByPath(isoPath);
    char* crcHex = decimalToHex(OPL::crc32Hex(gameName), 8);

    const char* newIsoDir = locationFix(std::string(outputDir) + "\\" + mediaType);
    const char* newIsoPath = locationFix(std::string(newIsoDir) + "\\" + fileName);
    const char* vmcName = locationFix(replaceStr(gameId, ".", ""));
    const char* vmcDir = locationFix(std::string(outputDir) + "\\VMC");
    const char* vmcPath = locationFix((std::string(vmcDir) + "\\" + vmcName + ".bin"));
    const char* cfgDir = locationFix(std::string(outputDir) + "\\CFG");
    const char* cfgPath = locationFix(std::string(cfgDir) + "\\" + gameId + ".cfg");
    const char* artDir = locationFix(std::string(outputDir) + "\\ART");

    CLEAR();
    CREATE_ASCII();
    printf("[*] Process started.\n[*] Game title is \"%s\", copying to \"%s\" directory.\n" YELLOW"[*] Switch off the \"Check USB game fragmentation\" option in OPL settings, otherwise it will not run fragmented games.\n[*] Do not eject the disc during the process and do not interfere with the files in use.\n\n" RESET, gameName, newIsoDir);

    {
        auto start = std::chrono::high_resolution_clock::now();
        std::streamsize totalReadByte = 0;

        bool writeJob_IsCompleted = false;

        auto writeJob = [&]() {
            std::ofstream output;
            char buffer[readSize];

            if (copyFile == 0) {
                int chunkCount = 0;

                while (!input.eof()) {
                    static std::streamsize partRead = chunkSize;
                    input.read(buffer, partRead + readSize > chunkSize ? abs(chunkSize - partRead) : readSize);

                    if (partRead >= chunkSize) {
                        partRead = 0;

                        std::string path = std::string(outputDir) + "\\ul." + crcHex + "." + gameId + "." + padStart(std::to_string(chunkCount), 2, '0');

                        output.close();
                        output.open(path, std::ios::binary);

                        if (!output.is_open()) {
                            writeJob_IsCompleted = true;
                            input.close();
                            printf(LIGHT_RED "\n\n[-] Failed to open part path." RESET "\n[!] Process finished with errors.");
                            PAUSE();
                            exit(1);
                        }

                        chunkCount++;
                    }

                    std::streamsize bytesRead = input.gcount();
                    output.write(buffer, bytesRead);
                    totalReadByte += bytesRead;
                    partRead += bytesRead;
                }

                int ulResult = OPL::writeUl(outputDir, gameName, gameId, mediaType, chunkCount);
                if (ulResult < 0) {
                    writeJob_IsCompleted = true;
                    output.close();
                    input.close();
                    printf(LIGHT_RED "\n\n[-] Failed to create defragged game." RESET "\n[!] Process finished with errors.");
                    PAUSE();
                    exit(1);
                }
            }
            else if (copyFile == 1) {
                if (!isDirectory(newIsoDir) && !MKDIR(newIsoDir)) {
                    writeJob_IsCompleted = true;
                    input.close();
                    printf(LIGHT_RED "\n\n[-] Failed to create media directory." RESET "\n[!] Process finished with errors.");
                    PAUSE();
                    exit(1);
                }

                output.open(newIsoPath, std::ios::binary);
                if (!output.is_open()) {
                    writeJob_IsCompleted = true;
                    input.close();
                    printf(LIGHT_RED "\n\n[-] Failed to open part path." RESET "\n[!] Process finished with errors.");
                    PAUSE();
                    exit(1);
                }

                while (!input.eof()) {
                    input.read(buffer, readSize);

                    std::streamsize bytesRead = input.gcount();
                    output.write(buffer, bytesRead);
                    totalReadByte += bytesRead;
                }
            }

            input.close();
            output.close();
            writeJob_IsCompleted = true;
        };

        std::thread writeJob_Thread(writeJob);

        while (!writeJob_IsCompleted) {
            double percentage = (double)totalReadByte / isoSize * 100;
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

            const char chars[] = { '\\', '|', '/', '-' };
            static int index = 0;
            index = (index + 1) % 4;

            printf(LIGHT_BLACK HIDE_CURSOR "%c %llu bytes transferred in %llu ms from total of %llu bytes (%%%i completed) %c" RESET"\t\r", chars[index], totalReadByte, duration.count(), isoSize, (int)round(percentage), chars[index]);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        writeJob_Thread.join();

        printf(SHOW_CURSOR RESET "\n");
    }

    if (createVmc == 1) {
        if (!isDirectory(vmcDir) && !MKDIR(vmcDir)) {
            printf(LIGHT_RED "\n[-] Failed to create virtual memory directory." RESET "\n[!] Process finished with errors.");
            PAUSE();
            return 1;
        }

        int vmcResult = OPL::createVmc(vmcPath, sizeOfVmc * 1024, 16);
        if (vmcResult < 0) {
            printf(LIGHT_RED "\n[-] Failed to create virtual memory card." RESET "\n[!] Process finished with errors.");
            PAUSE();
            return 1;
        }

        if (!isDirectory(cfgDir) && !MKDIR(cfgDir)) {
            printf(LIGHT_RED "\n[-] Failed to create config directory." RESET "\n[!] Process finished with errors.");
            PAUSE();
            return 1;
        }

        bool cfgResult = OPL::writeCfg(cfgPath, "$VMC_0", vmcName);
        if (!cfgResult) {
            printf(LIGHT_RED "\n[-] Failed to set memory card slot." RESET "\n[!] Process finished with errors.");
            PAUSE();
            return 1;
        }

        printf(YELLOW"\n[*] Virtual memory card created and selected." RESET);
    }

    if (addCover == 1) {
        if (!isDirectory(artDir) && !MKDIR(artDir)) {
            printf(LIGHT_RED "\n[-] Failed to create art directory." RESET "\n[!] Process finished with errors.");
            PAUSE();
            return 1;
        }
 
        int fetchResult = OPL::downloadArts(artDir, gameId);
        if (fetchResult == -1) {
            printf(LIGHT_RED "\n[-] Failed to write image data." RESET "\n[!] Process finished with errors.");
            PAUSE();
            return 1;
        }
        else if (fetchResult == -2) {
            printf(LIGHT_RED "\n[-] There is no game image on the server." RESET "\n[!] Process finished with errors.");
            PAUSE();
            return 1;
        }

        printf(YELLOW"\n[*] Game cover art installed." RESET);

    }

    printf("\n[+] Process succesfully finished.");
    PAUSE();

    return 0;
}