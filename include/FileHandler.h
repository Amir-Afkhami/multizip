#pragma once
#include <BoundedStream.h>
#include <fstream>

class FileHandler 
{
private:
    std::fstream& fileStream;
public:
    virtual ~FileHandler() = default;
    FileHandler(std::fstream& o) : fileStream(o) {}
    
    virtual void init() = 0;
    virtual std::ofstream& createVirtualFile(const std::string& path) = 0;
    virtual void createDirectory(const std::string& path) = 0;
    virtual std::vector<std::string> getVirtualFiles() = 0;
    virtual BoundedStream readVirtualFile(const std::string) = 0;
    virtual void close() = 0;
};