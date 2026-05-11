#pragma once
#include <FileHandler.h>

class ZipFileHandler : public FileHandler 
{
public:
    using FileHandler::FileHandler;

    virtual void init() override;
    virtual std::ofstream& createVirtualFile(const std::string& path) override;
    virtual void createDirectory(const std::string& path) override;
    virtual std::vector<std::string> getVirtualFiles() override;
    virtual BoundedStream readVirtualFile(const std::string) override;
    virtual void close() override;
};