#pragma once

#include <FileHandler.h>
#include <cstdint>
#include <sstream>
#include <string>
#include <vector>

class TarFileHandler : public FileHandler
{
    struct Entry {
        std::string name;
        std::streamoff dataOffset = 0;
        std::uint64_t size = 0;
        bool isDirectory = false;
    };

    std::vector<Entry> entries;
    std::ostringstream entryBuffer;
    std::string currentEntryName;
    bool entryOpen = false;
    bool isCurrentDirectory = false;

    void writeHeader(const Entry& entry);
    void writePadding(std::uint64_t size);
    void parseArchive();

public:
    using FileHandler::FileHandler;

    void init() override;
    std::ostream& createVirtualFile(const std::string& path) override;
    void finishVirtualFile() override;
    void createDirectory(const std::string& path) override;
    std::vector<std::string> getVirtualFiles() override;
    BoundedStream readVirtualFile(const std::string& path) override;
    void close() override;
};
