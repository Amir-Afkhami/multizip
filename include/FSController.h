#pragma once

#include <string>
#include <vector>

class FSController {
public:
    static FSController& getInstance();
    
    std::ifstream readFile(const std::string& path);
    std::vector<std::filesystem::path> readDir(const std::string& path);
    std::ofstream createFile(const std::string& path);
    bool makeDir(const std::string& path);
    bool isDir(const std::string& path);
    
    // Delete copy constructor and assignment operator
    FSController(const FSController&) {};
    
private:
    FSController() = default;
    ~FSController() = default;
};