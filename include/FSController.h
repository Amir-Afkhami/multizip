#pragma once

#include <string>
#include <vector>

class FSController {
public:
    static FSController& getInstance();
    
    std::string readFile(const std::string& path);
    std::vector<std::string> readDir(const std::string& path);
    void writeFile(const std::string& path, const std::string& content);
    void makeDir(const std::string& path);
    
    // Delete copy constructor and assignment operator
    FSController(const FSController&) = delete;
    FSController& operator=(const FSController&) = delete;
    
private:
    FSController() = default;
    ~FSController() = default;
};