#include "zip_header.h"
#include <iostream>
#include "decompressor.h"

static const size_t HEADER_OFFSET = 30;

ZipFile::ZipFile(ZipFileHeader header,std::vector<char>& buffer, size_t offset):
    m_info(header)
{
    if (buffer[offset] != 0x50 || buffer[offset + 1] != 0x4B || buffer[offset + 2] != 0x03 || buffer[offset + 3] != 0x04) {
        std::cout << "Error Reading file at offset " << offset << std::endl;
        return;
    }

    if (m_info.m_uncompressedCRC == 0) {
        printCurrentFileToConsole();
    }

    std::vector<char> fileNameData(m_info.m_fileNameLength);
    fileNameData.assign(buffer.begin() + offset + HEADER_OFFSET, buffer.begin() + offset + HEADER_OFFSET + m_info.m_fileNameLength);
    fileNameData.push_back('\0');
    m_FileName = fileNameData.data();

    if (m_info.m_extraFieldLength > 0) {
        std::vector<char> m_ExtraFieldData(m_info.m_extraFieldLength);
        m_ExtraFieldData.assign(buffer.begin() + offset + HEADER_OFFSET + m_info.m_fileNameLength, buffer.begin() + offset + HEADER_OFFSET + m_info.m_fileNameLength + m_info.m_extraFieldLength);
    }

    if (m_info.m_compressedSize == 0xFFFFFFFF || m_info.m_uncompressedSize == 0xFFFFFFFF) {
        size_t i = 0;
        while (i + 4 < m_ExtraFieldData.size()) {
            uint16_t headerID = *reinterpret_cast<uint16_t*>(&m_ExtraFieldData[i]);
            uint16_t dataSize = *reinterpret_cast<uint16_t*>(&m_ExtraFieldData[i + 2]);

            if (headerID == 0x0001) {
                size_t offset = i + 4;
                if (m_info.m_uncompressedSize == 0xFFFFFFFF) {
                    m_info.m_uncompressedSize = *reinterpret_cast<uint64_t*>(&m_ExtraFieldData[offset]);
                    offset += 8; // Shift offset by bytes read
                }
                if (m_info.m_compressedSize == 0xFFFFFFFF) {
                    m_info.m_compressedSize = *reinterpret_cast<uint64_t*>(&m_ExtraFieldData[offset]);
                    offset += 8; // Shift offset by bytes read
                }
            }

            i += 4 + dataSize;
        }
    }

    if (m_info.m_compressedSize > 0)
        m_fileType = ZIP_FILE_TYPE::_FILE;
    
    m_fileData.resize(m_info.m_compressedSize);
    m_fileData.assign(buffer.begin() + offset + HEADER_OFFSET + m_info.m_fileNameLength + m_info.m_extraFieldLength, buffer.begin() + offset + HEADER_OFFSET + m_info.m_fileNameLength + m_info.m_extraFieldLength + m_info.m_compressedSize);
}

bool ZipFile::isDirectory()
{
    return m_fileType == ZIP_FILE_TYPE::_DIRECTORY;
}

std::string ZipFile::getFileName()
{
    return m_FileName;
}

void ZipFile::printCurrentFileToConsole()
{
    std::cout << "===Checking===" << std::endl;
    std::cout << "CRC: " << m_info.m_uncompressedCRC << std::endl;
    std::cout << "FileNameLength: " << m_info.m_fileNameLength << std::endl;
    std::cout << "FileName: " << m_FileName << std::endl;
    std::cout << "CompressionMethod: " << m_info.m_compressionMethod << std::endl;
    std::cout << "FileSize (Compressed): " << m_info.m_compressedSize << std::endl;
    std::cout << "FileSize (Uncompressed): " << m_info.m_uncompressedSize << std::endl;
    std::cout << "ExtraFieldLength: " << m_info.m_extraFieldLength << std::endl;
    if (m_ExtraFieldData.size() > 0)
        std::cout << "ExtraFieldData: " << m_ExtraFieldData.data() << std::endl;
    std::cout << "DataSize: " << m_fileData.size() << std::endl;
    if (!m_fileData.empty())
        std::cout << "LastSymbol: " << (int)m_fileData.back() << std::endl;

    std::cout << "Type: " << m_fileType << std::endl;
    std::cout << "===EOF========" << std::endl << std::endl;
}

std::vector<char>* ZipFile::decompress()
{
    if (m_info.m_compressionMethod == ZIP_COMPRESSION_TYPE::DEFLATE) {
        //Decompress Data and write it back into m_fileData
        Decompressor decompressor(&m_fileData, m_info.m_uncompressedSize);
        m_fileData = decompressor.DecodeDeflate();
    }  

    return &m_fileData;
    
    //return std::vector<char>();
}

ZipFileHeader::ZipFileHeader()
{
}

ZipFileHeader::ZipFileHeader(std::vector<char>& buffer, size_t offset)
{
    m_magicNumber = *reinterpret_cast<uint32_t*>(&buffer[offset + 0]);
    m_version = *reinterpret_cast<uint16_t*>(&buffer[offset + 4]);
    m_gpbt = *reinterpret_cast<uint16_t*>(&buffer[offset + 6]);
    m_compressionMethod = *reinterpret_cast<uint16_t*>(&buffer[offset + 8]);
    m_lastModifiedTime = *reinterpret_cast<uint16_t*>(&buffer[offset + 10]);
    m_lastModifiedDate = *reinterpret_cast<uint16_t*>(&buffer[offset + 12]);
    m_uncompressedCRC = *reinterpret_cast<uint32_t*>(&buffer[offset + 14]);
    m_compressedSize = *reinterpret_cast<uint32_t*>(&buffer[offset + 18]);
    m_uncompressedSize = *reinterpret_cast<uint32_t*>(&buffer[offset + 22]);
    m_fileNameLength = *reinterpret_cast<uint16_t*>(&buffer[offset + 26]);
    m_extraFieldLength = *reinterpret_cast<uint16_t*>(&buffer[offset + 28]);
}

bool ZipFileHeader::IsValid()
{
    return (m_magicNumber == 0x04034b50 && m_compressionMethod <= 8 && m_fileNameLength < 1024);
}

