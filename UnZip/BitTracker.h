#pragma once
#include <vector>

struct BitStreamPosition {
	BitStreamPosition(int BitPos, int BytePos);
	int BitPos; 
	int BytePos;
};


class BitTracker {
public:
	BitTracker(std::vector<char>* p_buffer);
	BitStreamPosition getPosition();
	int getBytePos();
	void nextByte();
	int getBitPos();
	void incrementBits(int i);
	void decrement(int i);
	uint32_t readBit();
	uint32_t readBits(int amountOfBits);
	uint32_t peekBits(int amountOfBits);
	std::vector<char> readBytes(int amountOfBytes);
private:
	std::vector<char>* m_buffer;
	int m_BytePos = 0;
	int m_BitPos = 0;
};