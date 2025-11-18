#include <iostream>
#include <filesystem>
#include <fstream>
#include <windows.h>
#include <conio.h>
#include "zip_header.h"
#include "crc.h"

void printWString(const std::wstring& str) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hConsole != INVALID_HANDLE_VALUE) {
        DWORD written;
        WriteConsoleW(hConsole, str.c_str(), (DWORD)str.size(), &written, nullptr);
    }
}

std::wstring convertToWString(const std::string& sjis) 
{
    int len = MultiByteToWideChar(932, 0, sjis.c_str(), (int)sjis.size(), nullptr, 0);
    if (len == 0) throw std::runtime_error("BAD!!");

    std::wstring wide_str(len, 0);
    MultiByteToWideChar(932, 0, sjis.c_str(), (int)sjis.size(), wide_str.data(), len);

    return wide_str;
}

std::filesystem::path shiftJISToPath(const std::string& fullPathName) 
{
    std::filesystem::path finalPath;

    std::vector<std::wstring> components;
    size_t last_cut = 0;

    for (int i = 0; i < fullPathName.size(); i++) {
        if (fullPathName[i] == '/') {
            if (i > last_cut)
                components.push_back(convertToWString(fullPathName.substr(last_cut, i - last_cut)));
            last_cut = i + 1;
        }
    }

    if (last_cut < fullPathName.size())
        components.push_back(convertToWString(fullPathName.substr(last_cut)));

    for (const std::wstring& component : components)
        finalPath /= component;
    
    return finalPath;
}

int main(int argc, char* argv[])
{
    std::filesystem::path path;

    if (argc > 1)
        path = argv[1];

    std::cout << path << std::endl;

    if (!std::filesystem::exists(path) || std::filesystem::is_directory(path))
        return 1;

    std::string fileName = path.filename().string();

    if (path.has_extension()) {
        size_t extension_pos = path.filename().string().rfind('.');
        fileName = fileName.erase(extension_pos);
    }
    std::filesystem::create_directories("./" + fileName);
   
    std::ifstream file(path, std::ios_base::binary | std::ios::ate);

    size_t length = file.tellg();
    file.seekg(0, std::ios_base::beg);
    std::cout << length << std::endl;
    
    std::vector<char> buffer(length);
    file.read(buffer.data(), length);
    file.close();

    std::vector<size_t> file_offsets;

    for (size_t i = 0; i < buffer.size() - 4; i++) {
        if (buffer[i] == 0x50 && buffer[i+1] == 0x4B && buffer[i + 2] == 0x03 && buffer[i + 3] == 0x04) {
            file_offsets.push_back(i);
        }
    }

    for (size_t i = 0; i < file_offsets.size(); i += 1) {
        ZipFileHeader zipFileHeader(buffer, file_offsets[i]);
        if (!zipFileHeader.IsValid()) continue;
        ZipFile zipFile(zipFileHeader, buffer, file_offsets[i]);
        std::string name = zipFile.getFileName();

        if (zipFile.isDirectory()) {
            
            if (!name.empty()) {
                std::filesystem::path file_path = shiftJISToPath(fileName + "/" + name);
                std::filesystem::create_directories(file_path);
            }
        }
        else 
        {
            std::vector<char>* zipFileData = zipFile.decompress();
            if (zipFileHeader.m_uncompressedCRC != crc32(reinterpret_cast<unsigned char*>(zipFileData->data()), zipFileData->size())) {
                std::cout << "CRC Mismatched! Potenially corrupt data!" << std::endl;
                std::cout << "Expected: " << std::uppercase << std::hex << zipFileHeader.m_uncompressedCRC << std::endl;
                std::cout << "Actual: " << std::uppercase << std::hex << crc32(reinterpret_cast<unsigned char*>(zipFileData->data()), zipFileData->size()) << std::endl;
                std::cout << std::endl << "Press enter to continue extracting (likely corrupted files)..." << std::endl;
                _getch();
            }
            
            std::filesystem::path file_path = shiftJISToPath(fileName + "/" + name);
            std::filesystem::create_directories(file_path.parent_path());

            std::ofstream file(file_path, std::ios_base::in | std::ios_base::out | std::ios_base::app | std::ios_base::binary);

            file.write(zipFileData->data(), zipFileData->size());
            printWString(L"Extracted file \"" + file_path.filename().wstring() + L"\" to " + file_path.parent_path().wstring() + L"\n");
            file.flush();

            file.close();
        }
    }

    return 0;
}

