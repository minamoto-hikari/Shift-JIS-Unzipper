#include "decompressor.h"
#include <iostream>

std::vector<TableEntry>* Decompressor::m_staticHLITLookupTable = nullptr;
std::vector<TableEntry>* Decompressor::m_staticHDISTLookupTable = nullptr;

Decompressor::Decompressor(std::vector<char>* p_data_buffer, int uncompressedSize):
	m_tracker(p_data_buffer)
{
	m_outBuffer.reserve(uncompressedSize);
}

std::vector<char> Decompressor::DecodeDeflate()
{
	while(decompressDeflateBlock());

	return m_outBuffer;
}

bool Decompressor::decompressDeflateBlock()
{
	int bFinal = m_tracker.readBits(1);
	int bType = m_tracker.readBits(2);

	if (bType == HuffmanType::DYNAMIC) {
		int HLIT = m_tracker.readBits(5) + 257;
		int HDIST = m_tracker.readBits(5) + 1;
		int HCLEN = m_tracker.readBits(4) + 4;

		std::vector<int> cl_lengths = generateCLLengths(HCLEN);
		std::vector<Code> clCodes = generateCanonicalCodes(cl_lengths);
		std::vector<TableEntry>* lookupTable = generateLookupTable(clCodes);

		std::vector<int> HLITLengths;
		std::vector<int> HDISTLengths;

		generateHuffmanLengths(HLITLengths, HDISTLengths, lookupTable, HLIT, HDIST);
		delete lookupTable;

		std::vector<Code> hlitCodes = generateCanonicalCodes(HLITLengths);
		std::vector<TableEntry>* hlitLookupTable = generateLookupTable(hlitCodes);

		std::vector<Code> hdistCodes = generateCanonicalCodes(HDISTLengths);
		std::vector<TableEntry>* hdistLookupTable = generateLookupTable(hdistCodes);

		decodeHuffmanCode(hlitLookupTable, hdistLookupTable);

		delete hlitLookupTable;
		delete hdistLookupTable;
	}
	else if (bType == HuffmanType::STATIC) {
		if (m_staticHDISTLookupTable == nullptr || m_staticHLITLookupTable == nullptr) {
			std::vector<int> staticLITLengths(288);

			for (int i = 0; i <= 143; i++) staticLITLengths[i] = 8;
			for (int i = 144; i <= 255; i++) staticLITLengths[i] = 9;
			for (int i = 256; i <= 279; i++) staticLITLengths[i] = 7;
			for (int i = 280; i <= 287; i++) staticLITLengths[i] = 8;

			std::vector<int> staticHDISTLengths(32, 5);

			std::vector<Code> staticHLITCodes = generateCanonicalCodes(staticLITLengths);
			m_staticHLITLookupTable = generateLookupTable(staticHLITCodes);

			std::vector<Code> staticHDISTCodes = generateCanonicalCodes(staticHDISTLengths);
			m_staticHDISTLookupTable = generateLookupTable(staticHDISTCodes);
		}

		decodeHuffmanCode(m_staticHLITLookupTable, m_staticHDISTLookupTable);
	}
	else if (bType == HuffmanType::UNCOMPRESSED) {
		if (m_tracker.getBitPos() != 0)
			m_tracker.nextByte(); //Jump to the next byte for uncompressed data if we're out of alignment => Data is byte aligned.

		uint16_t LEN = m_tracker.readBits(16);
		uint16_t NLEN = m_tracker.readBits(16);
		std::vector<char> bytes = m_tracker.readBytes(LEN);

		m_outBuffer.insert(m_outBuffer.end(), bytes.begin(), bytes.end());
	}
	else {
		throw std::invalid_argument("Invalid DEFLATE block");
	}
	
	return !(bool)bFinal;
}

void Decompressor::generateHuffmanLengths(std::vector<int>& HLITLengths, std::vector<int>& HDISTLengths, std::vector<TableEntry>* lookupTable, int HLIT, int HDIST)
{
	std::vector<int> buffer;
	buffer.reserve(HLIT + HDIST); //Reserve enough space so no reallocation is needed

	while (buffer.size() < HLIT + HDIST) {
		int next_bits = m_tracker.peekBits(MAX_BITS);
		TableEntry entry = (*lookupTable)[next_bits];
		m_tracker.incrementBits(entry.length); //Discard bits already -> update cursor to be after the symbol code

		if (entry.symbol < 16)
		{
			buffer.push_back(entry.symbol);
		}
		else if (entry.symbol == 16) {

			if (buffer.empty())
				throw std::runtime_error("First Symbol cannot be 16.");

			int reps = 3 + m_tracker.readBits(2); // 3 - 6, readBits(2) will give 0-3

			int last_symbol = buffer.back();
			for (int n = 0; n < reps; n++) {
				buffer.push_back(last_symbol);
			}
		}
		else if (entry.symbol == 17) {
			int reps = 3 + m_tracker.readBits(3); // 3 - 10, readBits(2) will give 0-7
			for (int n = 0; n < reps; n++) {
				buffer.push_back(0);
			}
		}
		else if (entry.symbol == 18) {
			int reps = 11 + m_tracker.readBits(7); // 11 - 138, readBits(7) will give 0-127
			for (int n = 0; n < reps; n++) {
				buffer.push_back(0);
			}
		}
	}
	if (buffer.size() != HLIT + HDIST) {
		throw std::runtime_error("Buffer length not matching.");
	}

	HLITLengths = std::vector<int>(buffer.begin(), buffer.begin() + HLIT);
	HDISTLengths = std::vector<int>(buffer.begin() + HLIT, buffer.begin() + HLIT + HDIST);
}

std::vector<int> Decompressor::generateCLLengths(int HCLEN)
{
	std::vector<int> lengths(19, 0);
	int order[19] = { 16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15 };
	for (int i = 0; i < HCLEN; i++) {
		uint8_t value = m_tracker.readBits(3);
		lengths[order[i]] = value;
	};
	return lengths;
}

std::vector<Code> Decompressor::generateCanonicalCodes(std::vector<int>& lengths)
{
	std::vector<Code> codes(lengths.size());
	std::vector<int> bl_count(MAX_BITS+1, 0);

	for (int l : lengths)
		if (l > 0)
			bl_count[l]++;

	int code = 0;
	std::vector<int> next_code(32, 0);
	for (unsigned int bits = 1; bits <= MAX_BITS; bits++) {
		code = (code + bl_count[bits - 1]) << 1;
		next_code[bits] = code;
	}

	for (unsigned int symbol = 0; symbol < lengths.size(); symbol++) {
		int length = lengths[symbol];
		if (length > 0) {
			codes[symbol].code = next_code[length];
			codes[symbol].length = length;
			next_code[length]++;
		}
	}

	return codes;
}
uint32_t reverseBits(uint32_t code, uint32_t length)
{
	uint32_t reversed = 0;
	for (int i = 0; i < length; i++) {
		reversed |= ((code >> i) & 1) << (length - 1 - i);
	}
	return reversed;
}

std::vector<TableEntry>* Decompressor::generateLookupTable(std::vector<Code>& codes)
{
	std::vector<TableEntry>* lookupTable = new std::vector<TableEntry>();
	lookupTable->resize(1u << (MAX_BITS)); //Shifts by 15 = 32768 entries

	for (uint32_t symbol = 0; symbol < codes.size(); symbol++) {
		if (codes[symbol].length <= 0) continue;

		uint32_t code = reverseBits(codes[symbol].code, codes[symbol].length);

		uint32_t base = code;
		uint32_t numberOfCodeMatches = 1u << (MAX_BITS - codes[symbol].length);

		for (uint32_t n = 0; n < numberOfCodeMatches; n++) {
			uint32_t index = base | (n << codes[symbol].length);
			(*lookupTable)[index].symbol = symbol;
			(*lookupTable)[index].length = codes[symbol].length;
		}
	}

	return lookupTable;
}

void Decompressor::decodeHuffmanCode(std::vector<TableEntry>* hlitLookupTable, std::vector<TableEntry>* hdistLookupTable)
{
	while (true) {
		int next_bits = m_tracker.peekBits(MAX_BITS);
		TableEntry entry = (*hlitLookupTable)[next_bits];
		m_tracker.incrementBits(entry.length);

		if (entry.symbol < 256)
		{
			m_outBuffer.push_back((char)entry.symbol);
		}
		else if (entry.symbol == 256) 
		{
			break;
		}
		else if (entry.symbol > 256 && entry.symbol <= 285) {
			int lengthIndex = entry.symbol - 257;
			int length = lengthBase[lengthIndex];
			int extraLengthBits = lengthBaseExtra[lengthIndex];
			if (extraLengthBits > 0) {
				length += m_tracker.readBits(extraLengthBits);
			}

			int next_bits = m_tracker.peekBits(MAX_BITS);
			TableEntry distEntry = (*hdistLookupTable)[next_bits];
			m_tracker.incrementBits(distEntry.length);
			
			int distance = dist[distEntry.symbol];
			int extraDistanceBits = distExtra[distEntry.symbol];
			if (extraDistanceBits > 0) {
				distance += m_tracker.readBits(extraDistanceBits);
			}

			for (int i = 0; i < length; i++) 
				m_outBuffer.push_back(m_outBuffer[m_outBuffer.size() - distance]);
			
		}
		else {
			throw std::runtime_error("Symbol out of range!");
		}
	}
}