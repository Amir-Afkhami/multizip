#include <memory>
#include <filesystem>
#include <algorithm>
#include <iostream>
#include <MultizipController.h>
#include <FSController.h>
#include <FileHandler.h>
#include <FileHandlers/ZipFileHandler.h>
#include <Algorithm.h>
#include <Algorithms/LZW.h>
#include <Algorithms/Store.h>

namespace {

std::unique_ptr<Algorithm>
createAlgorithm(const Input& input)
{
    if (std::find(input.options.begin(), input.options.end(), "lzw") != input.options.end())
        return std::make_unique<LZW>();
    return std::make_unique<Store>();
}

bool
hasExtension(const std::string& path, const std::string& extension)
{
    return path.size() >= extension.size() &&
           path.compare(path.size() - extension.size(), extension.size(), extension) == 0;
}

std::unique_ptr<FileHandler>
createFileHandler(std::ostream& out, const Input& input)
{
    if (hasExtension(input.outputPath, ".zip"))
        return std::make_unique<ZipFileHandler>(out, createAlgorithm(input));

    return nullptr;
}

std::unique_ptr<FileHandler>
createFileHandler(std::istream& in, const Input& input)
{
    if (hasExtension(input.inputPath, ".zip"))
        return std::make_unique<ZipFileHandler>(in, createAlgorithm(input));

    return nullptr;
}

} // namespace

bool
MultizipController::compress(Input input)
{
    FSController& fs = FSController::getInstance();

    const bool isDirectory = fs.isDir(input.inputPath);

    std::ofstream outputFile;
    try {
        outputFile = fs.createFile(input.outputPath);
    } catch (const std::runtime_error& e) {
        std::cerr << e.what() << std::endl;
        return false;
    }

    std::unique_ptr<FileHandler> archive = createFileHandler(outputFile, input);
    if (!archive) {
        std::cerr << "Unsupported output archive format" << std::endl;
        return false;
    }

    archive->init();
    try {
        if (!isDirectory) {
            std::ifstream inStream = fs.readFile(input.inputPath);
            const std::filesystem::path path(input.inputPath);
            archive->compressEntry(inStream, path.filename().string());
        } else {
            const std::vector<std::filesystem::path> entries = fs.readDir(input.inputPath);

            for (const auto& entry : entries) {
                std::ifstream inStream = fs.readFile(entry.string());
                const std::string relativePath =
                    std::filesystem::relative(entry, input.inputPath).generic_string();
                archive->compressEntry(inStream, relativePath);
            }
        }
    } catch (const std::runtime_error& e) {
        std::cerr << e.what() << std::endl;
        return false;
    }

    archive->close();
    return true;
}

bool
MultizipController::extract(Input input)
{
    FSController& fs = FSController::getInstance();

    std::ifstream inputFile;
    try {
        inputFile = fs.readFile(input.inputPath);
    } catch (const std::runtime_error& e) {
        std::cerr << e.what() << std::endl;
        return false;
    }

    std::unique_ptr<FileHandler> archive = createFileHandler(inputFile, input);
    if (!archive) {
        std::cerr << "Unsupported input archive format" << std::endl;
        return false;
    }

    archive->init();
    try {
        const std::vector<std::string> virtualEntries = archive->getVirtualFiles();

        for (const auto& virtualEntry : virtualEntries) {
            const std::filesystem::path outputPath =
                std::filesystem::path(input.outputPath) / virtualEntry;
            if (outputPath.has_parent_path())
                fs.makeDir(outputPath.parent_path().string());

            std::ofstream outStream = fs.createFile(outputPath.string());
            archive->extractEntry(virtualEntry, outStream);
        }
    } catch (const std::runtime_error& e) {
        std::cerr << e.what() << std::endl;
        return false;
    }

    archive->close();
    return true;
}
