#pragma once
#include <vector>
#include "BitTracker.h"
#include <map>

static const int lengthBase[29] = { 3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31, 35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258 };
static const int lengthBaseExtra[29] = { 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0 };

static const int dist[30] =      { 1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193, 257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577 };
static const int distExtra[30] = { 0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4 ,4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13 };

struct Code {
	uint32_t code;
	uint32_t length;
};

struct TableEntry {
	uint16_t symbol;
	uint8_t length;
};

enum HuffmanType {
	UNCOMPRESSED = 0b00,
	STATIC = 0b01,
	DYNAMIC = 0b10,
	INVALID = 0b11,

};

class Decompressor 
{
public:
	Decompressor(std::vector<char>* p_data_buffer, int uncompressedSize);
	std::vector<char> DecodeDeflate();
	bool decompressDeflateBlock();
	void generateHuffmanLengths(std::vector<int>& HLITLengths, std::vector<int>& HDISTLengths, std::vector<TableEntry>* lookupTable, int HLIT, int HDIST);
	std::vector<int> generateCLLengths(int HCLEN);
	std::vector<Code> generateCanonicalCodes(std::vector<int>& lengths);
	std::vector<TableEntry>* generateLookupTable(std::vector<Code>& codes);
	void decodeHuffmanCode(std::vector<TableEntry>* hlitLookupTable, std::vector<TableEntry>* hdistLookupTable);
	static const int MAX_BITS = 15;
	
private:
	int HLIT = 0;
	int HDIST = 0;
	int HCLEN = 0;

	static std::vector<TableEntry>* m_staticHLITLookupTable;
	static std::vector<TableEntry>* m_staticHDISTLookupTable;

	BitTracker m_tracker;
	std::vector<char> m_outBuffer;
};