#pragma once
#include <vector>

static std::vector<uint32_t> crc_table;

uint32_t crc32(const unsigned char* buf, size_t len);
