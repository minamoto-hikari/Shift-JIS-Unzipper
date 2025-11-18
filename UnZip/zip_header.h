#pragma once
#include <cstdint>
#include <string>
#include <vector>

struct ZipFileHeader {
	ZipFileHeader();
	ZipFileHeader(std::vector<char>& buffer, size_t offset);
	bool IsValid();
	size_t m_magicNumber;
	size_t m_version;
	size_t m_gpbt;
	size_t m_compressionMethod;
	size_t m_lastModifiedTime;
	size_t m_lastModifiedDate;
	size_t m_uncompressedCRC;
	size_t m_compressedSize;
	size_t m_uncompressedSize;
	size_t m_fileNameLength;
	size_t m_extraFieldLength;
};

enum ZIP_FILE_TYPE {
	_DIRECTORY,
	_FILE
};

enum ZIP_COMPRESSION_TYPE {
	STORE = 0,
	DEFLATE = 8
};

class ZipFile {
public:
	ZipFile(ZipFileHeader header, std::vector<char>& buffer, size_t offset);
	bool isDirectory();
	std::string getFileName();
	void printCurrentFileToConsole();
	std::vector<char>* decompress();
private:
	ZipFileHeader m_info;
	std::string m_FileName;
	std::vector<char> m_ExtraFieldData;
	std::vector<char> m_fileData;
	ZIP_FILE_TYPE m_fileType = ZIP_FILE_TYPE::_DIRECTORY;
};