#pragma once

#include <FileHandler.h>
#include <cstdint>
#include <sstream>
#include <string>
#include <vector>

namespace Zip
{
    struct LocalHeader
    {
        unsigned char magic[4] = {0x50, 0x4B, 0x03, 0x04};
        unsigned char version[2];
        unsigned char flag[2];
        unsigned char cmethod[2];
        unsigned char lastmodt[2];
        unsigned char lastmodd[2];
        unsigned char crc32_unc[4];
        unsigned char compsize[4];
        unsigned char uncompsize[4];
        unsigned char fnamelen[2];
        unsigned char efieldlen[2];
    };

    struct CentralDirectory
    {
        unsigned char magic[4] = {0x50, 0x4B, 0x01, 0x02};
        unsigned char versionby[2];
        unsigned char version[2];
        unsigned char flag[2];
        unsigned char cmethod[2];
        unsigned char lastmodt[2];
        unsigned char lastmodd[2];
        unsigned char crc32_unc[4];
        unsigned char compsize[4];
        unsigned char uncompsize[4];
        unsigned char fnamelen[2];
        unsigned char efieldlen[2];
        unsigned char fcommentlen[2];
        unsigned char disknum[2];
        unsigned char internal_attr[2];
        unsigned char external_attr[4];
        unsigned char reloffset[4];
    };

    struct EndCentralDirectory
    {
        unsigned char magic[4] = {0x50, 0x4B, 0x05, 0x06};
        unsigned char disknum[2];
        unsigned char central_disk[2];
        unsigned char num_centraldirs[2];
        unsigned char total_centraldirs[2];
        unsigned char size_centraldirs[4];
        unsigned char offest_centraldir[4];
        unsigned char commentlen[2];
    };
}

class ZipFileHandler : public FileHandler
{
    struct Entry {
        std::string name;
        std::uint32_t localHeaderOffset = 0;
        std::uint32_t crc32 = 0;
        std::uint32_t compressedSize = 0;
        std::uint32_t uncompressedSize = 0;
        std::uint16_t compressionMethod = 0;
        bool isDirectory = false;
    };

    std::vector<Entry> entries;
    std::ostringstream entryBuffer;
    std::string currentEntryName;
    bool entryOpen = false;
    std::uint32_t pendingCrc32 = 0;
    std::uint32_t pendingUncompressedSize = 0;
    void writeLocalHeader(const Entry& entry);
    void writeCentralDirectory();
    void writeEndOfCentralDirectory(std::uint32_t centralDirOffset, std::uint16_t entryCount);
    void parseArchive();

public:
    using FileHandler::FileHandler;

    void init() override;
    void compressEntry(std::istream& in, const std::string& virtualPath) override;
    std::ostream& createVirtualFile(const std::string& path) override;
    void finishVirtualFile() override;
    void createDirectory(const std::string& path) override;
    std::vector<std::string> getVirtualFiles() override;
    BoundedStream readVirtualFile(const std::string& path) override;
    void close() override;
};
