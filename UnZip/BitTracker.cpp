#include "BitTracker.h"
#include <iostream>

BitTracker::BitTracker(std::vector<char>* p_buffer):
	m_buffer(p_buffer)
{
}

BitStreamPosition BitTracker::getPosition()
{
	return BitStreamPosition(m_BitPos, m_BytePos);
}

int BitTracker::getBytePos()
{
	return m_BytePos;
}

void BitTracker::nextByte()
{
	m_BytePos += 1;
	m_BitPos = 0;
}

int BitTracker::getBitPos()
{
	return m_BitPos;
}

void BitTracker::incrementBits(int i)
{
	for (int _i = 0; _i < i; _i++) {
		m_BitPos += 1;

		if (m_BitPos > 7) {
			m_BitPos = 0;
			m_BytePos += 1;
		}
	}
}

void BitTracker::decrement(int i)
{
	for (int _i = 0; _i < i; _i++) {
		m_BitPos -= 1;

		if (m_BitPos < 0) {
			m_BitPos = 7;
			m_BytePos -= 1;
		}
	}
}
uint32_t BitTracker::readBit()
{
	uint32_t data = 0;
	uint8_t currentByte = (*m_buffer)[m_BytePos];

	data |= ((currentByte >> m_BitPos) & 1);
	incrementBits(1);

	return data;
}

uint32_t BitTracker::readBits(int amountOfBits)
{
	uint32_t data = 0;

	for (int i = 0; i < amountOfBits; i++) {
		uint8_t currentByte = (*m_buffer)[m_BytePos];
		
		data |= ((currentByte >> m_BitPos) & 1) << i;

		incrementBits(1);
	}

	return data;
}

uint32_t BitTracker::peekBits(int amountOfBits)
{
	uint32_t data = 0;

	uint32_t bytePos = m_BytePos;
	uint32_t bitPos = m_BitPos;

	for (int i = 0; i < amountOfBits; i++) {
		uint8_t currentByte = (*m_buffer)[bytePos];

		data |= ((currentByte >> bitPos) & 1) << i;
		
		bitPos += 1;
		if (bitPos > 7) {
			bitPos = 0;
			bytePos += 1;
			if (bytePos >= m_buffer->size())
				break;
		}
	}

	return data;
}

std::vector<char> BitTracker::readBytes(int amountOfBytes)
{
	std::vector<char> data = std::vector<char>(m_buffer->begin() + m_BytePos, m_buffer->begin() + m_BytePos + (amountOfBytes));
	m_BytePos += amountOfBytes;
	return data;
}

BitStreamPosition::BitStreamPosition(int InBitPos, int InBytePos):
	BitPos(InBitPos),
	BytePos(InBytePos)
{
}
