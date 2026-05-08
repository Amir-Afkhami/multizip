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

std::vector<std::string> FSController::readDir(const std::string& path) {
    std::vector<std::string> entries;

    for (const auto& entry : std::filesystem::recursive_directory_iterator(path)) {
        if(entry.is_regular_file())
        {
            entries.push_back(entry.path().string());
        }
    }
    return entries;
}

std::ofstream FSController::writeFile(const std::string& path) {
    std::ofstream file(path);

    if (!file.is_open()) {
        throw std::runtime_error("Cannot create file: " + path);
    }
    return file;
}

bool FSController::makeDir(const std::string& path) {
    return std::filesystem::create_directories(path);
}
