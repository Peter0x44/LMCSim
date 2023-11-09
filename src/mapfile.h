#pragma once

#include "lmc.h"

int s8FileMap(const char* fileName, s8* contents);
void s8FileUnmap(int fd, s8 fileContents);
