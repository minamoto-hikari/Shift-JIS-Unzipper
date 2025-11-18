#include "crc.h"

void createCRCTable() 
{
    crc_table.resize(256);
    for (uint32_t i = 0; i < 256; i++) {
        uint32_t c = i;
        for (int k = 0; k < 8; k++) {
            if (c & 1)
                c = 0xEDB88320u ^ (c >> 1);
            else
                c >>= 1;
        }
        crc_table[i] = c;
    }
}

uint32_t crc32(const unsigned char* buf, size_t len)
{
	if (crc_table.size() < 256)
		createCRCTable();

    uint32_t crc = 0xFFFFFFFF;

    for (size_t i = 0; i < len; i++) {
        crc = crc_table[(crc ^ buf[i]) & 0xFF] ^ (crc >> 8);
    }

    return crc ^ 0xFFFFFFFF;
}
