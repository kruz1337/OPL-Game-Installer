#pragma once

#include <iostream>
#include <sstream>

extern "C" {
#include "lib9660/lib9660.h"
}

namespace IsoParser {
	char* readIsoFile(const char* path, const char* insidePath);
	char* parseConfig(const char* conf, const char* option);
	char* getGameID(const char* isoPath);
};