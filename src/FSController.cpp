#include <FSController.h>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <stdexcept>

FSController& FSController::getInstance() {
    static FSController instance;
    return instance;
}

std::ifstream FSController::readFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file: " + path);
    }
    return file;
}

std::vector<std::filesystem::path> FSController::readDir(const std::string& path) {
    std::vector<std::filesystem::path> entries;

    for (const auto& entry : std::filesystem::recursive_directory_iterator(path)) {
        if(entry.is_regular_file())
        {
            entries.push_back(entry.path());
        }
    }
    return entries;
}

std::ofstream FSController::createFile(const std::string& path) {
    std::ofstream file(path);

    if (!file.is_open()) {
        throw std::runtime_error("Cannot create file: " + path);
    }
    return file;
}

bool FSController::makeDir(const std::string& path) {
    return std::filesystem::create_directories(path);
}

bool
FSController::isDir(const std::string &path)
{
    return std::filesystem::is_directory(path);
}
