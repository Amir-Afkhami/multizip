#include <FSController.h>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <stdexcept>

FSController& FSController::getInstance() {
    static FSController instance;
    return instance;
}

std::string FSController::readFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file: " + path);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

std::vector<std::string> FSController::readDir(const std::string& path) {
    std::vector<std::string> entries;
    for (const auto& entry : std::filesystem::directory_iterator(path)) {
        entries.push_back(entry.path().filename().string());
    }
    return entries;
}

void FSController::writeFile(const std::string& path, const std::string& content) {
    std::ofstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot create file: " + path);
    }
    file << content;
}

void FSController::makeDir(const std::string& path) {
    if (!std::filesystem::create_directories(path)) {
        throw std::runtime_error("Cannot create directory: " + path);
    }
}
