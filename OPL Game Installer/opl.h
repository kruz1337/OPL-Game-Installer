#pragma once

#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>

namespace OPL {
	bool writeCfg(const char* path, const char* key, const char* value);
	int downloadArts(const char* dir, const char* gameId);
	bool fetchImage(const char* url, std::vector<unsigned char>& buffer);

	int writeUl(const char* drive, const char* game_name, const char* game_id, const char* media, int parts);
	int crc32Hex(const char* string);
	int createVmc(const char* filename, int size_kb, int blocksize);
};