#pragma once

#include <Algorithm.h>
#include <BoundedStream.h>
#include <fstream>
#include <memory>
#include <vector>

class FileHandler
{
protected:
    std::unique_ptr<Algorithm> algorithm;
    std::ostream* outStream = nullptr;
    std::istream* inStream = nullptr;

public:
    virtual ~FileHandler() = default;

    FileHandler(std::ostream& out, std::unique_ptr<Algorithm> algo)
        : algorithm(std::move(algo)), outStream(&out) {}

    FileHandler(std::istream& in, std::unique_ptr<Algorithm> algo)
        : algorithm(std::move(algo)), inStream(&in) {}

    Algorithm& getAlgorithm() { return *algorithm; }

    virtual void compressEntry(std::istream& in, const std::string& virtualPath)
    {
        std::ostream& virtualOut = createVirtualFile(virtualPath);
        algorithm->encode(in, virtualOut);
        finishVirtualFile();
    }

    void extractEntry(const std::string& virtualPath, std::ostream& out)
    {
        BoundedStream virtualIn = readVirtualFile(virtualPath);
        algorithm->decode(virtualIn, out);
    }

    virtual void init() = 0;
    virtual std::ostream& createVirtualFile(const std::string& path) = 0;
    virtual void finishVirtualFile() = 0;
    virtual void createDirectory(const std::string& path) = 0;
    virtual std::vector<std::string> getVirtualFiles() = 0;
    virtual BoundedStream readVirtualFile(const std::string& path) = 0;
    virtual void close() = 0;
};
