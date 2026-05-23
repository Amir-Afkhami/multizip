#include <FileHandlers/TarFileHandler.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <stdexcept>

namespace {

constexpr std::size_t kTarBlockSize = 512;
constexpr std::array<char, 6> kUstarMagic = {'u', 's', 't', 'a', 'r', '\0'};
constexpr std::array<char, 2> kUstarVersion = {'0', '0'};

std::array<char, kTarBlockSize>
zeroTarBlock()
{
    std::array<char, kTarBlockSize> block;
    block.fill('\0');
    return block;
}

bool
isZeroBlock(const unsigned char* block)
{
    for (std::size_t i = 0; i < kTarBlockSize; ++i) {
        if (block[i] != 0)
            return false;
    }
    return true;
}

std::string
normalizeTarPath(const std::string& path)
{
    std::string normalized = path;
    std::replace(normalized.begin(), normalized.end(), '\\', '/');
    while (!normalized.empty() && normalized.front() == '/')
        normalized.erase(normalized.begin());
    return normalized;
}

void
splitUstarPath(const std::string& path, std::string& prefix, std::string& name)
{
    if (path.size() <= 100) {
        prefix.clear();
        name = path;
        return;
    }

    std::size_t splitAt = path.size() - 100;
    while (splitAt < path.size() && path[splitAt] != '/')
        ++splitAt;

    if (splitAt >= path.size())
        throw std::runtime_error("tar path too long: " + path);

    prefix = path.substr(0, splitAt);
    name = path.substr(splitAt + 1);

    if (prefix.size() > 155 || name.size() > 100)
        throw std::runtime_error("tar path too long: " + path);
}

void
writeOctalField(unsigned char* field, std::size_t width, std::uint64_t value)
{
    std::snprintf(reinterpret_cast<char*>(field), width, "%0*lo",
                  static_cast<int>(width - 1), static_cast<unsigned long>(value));
}

std::uint64_t
readOctalField(const unsigned char* field, std::size_t width)
{
    std::string digits(reinterpret_cast<const char*>(field), width);
    while (!digits.empty() && (digits.back() == '\0' || digits.back() == ' '))
        digits.pop_back();

    if (digits.empty())
        return 0;

    return std::strtoull(digits.c_str(), nullptr, 8);
}

std::uint32_t
computeTarChecksum(unsigned char* block)
{
    std::memset(block + 148, ' ', 8);
    std::uint32_t sum = 0;
    for (std::size_t i = 0; i < kTarBlockSize; ++i)
        sum += block[i];
    return sum;
}

std::string
joinUstarPath(const std::string& prefix, const std::string& name)
{
    if (prefix.empty())
        return name;
    return prefix + "/" + name;
}

// TAR header fields are fixed-width, space-padded, and may contain a trailing NUL.
std::string
readTarField(const unsigned char* field, std::size_t width)
{
    const char* data = reinterpret_cast<const char*>(field);
    const char* end = static_cast<const char*>(std::memchr(data, '\0', width));
    const std::size_t length = end ? static_cast<std::size_t>(end - data) : width;
    return std::string(data, length);
}

} // namespace

void
TarFileHandler::writeHeader(const Entry& entry)
{
    unsigned char block[kTarBlockSize] = {};
    std::string prefix;
    std::string name;
    splitUstarPath(entry.name, prefix, name);

    std::memcpy(block, name.data(), std::min(name.size(), std::size_t{100}));
    writeOctalField(block + 100, 8, entry.isDirectory ? 0755 : 0644);
    writeOctalField(block + 108, 8, 0);
    writeOctalField(block + 116, 8, 0);
    writeOctalField(block + 124, 12, entry.size);

    using namespace std::chrono;
    const auto now = system_clock::now();
    const std::time_t t = system_clock::to_time_t(now);
    writeOctalField(block + 136, 12, static_cast<std::uint64_t>(t));

    block[156] = entry.isDirectory ? '5' : '0';
    std::memcpy(block + 257, kUstarMagic.data(), kUstarMagic.size());
    std::memcpy(block + 263, kUstarVersion.data(), kUstarVersion.size());
    std::memcpy(block + 345, prefix.data(), std::min(prefix.size(), std::size_t{155}));

    const std::uint32_t checksum = computeTarChecksum(block);
    std::snprintf(reinterpret_cast<char*>(block + 148), 8, "%06o", checksum);

    outStream->write(reinterpret_cast<const char*>(block),
                     static_cast<std::streamsize>(kTarBlockSize));
}

void
TarFileHandler::writePadding(std::uint64_t size)
{
    const std::uint64_t remainder = size % kTarBlockSize;
    if (remainder == 0)
        return;

    const std::size_t pad = kTarBlockSize - static_cast<std::size_t>(remainder);
    const std::array<char, kTarBlockSize> padding = zeroTarBlock();
    outStream->write(padding.data(), static_cast<std::streamsize>(pad));
}

void
TarFileHandler::parseArchive()
{
    if (!inStream)
        return;

    inStream->seekg(0);
    while (true) {
        unsigned char block[kTarBlockSize];
        inStream->read(reinterpret_cast<char*>(block), static_cast<std::streamsize>(kTarBlockSize));
        if (inStream->gcount() != static_cast<std::streamsize>(kTarBlockSize))
            break;

        if (isZeroBlock(block))
            break;

        Entry entry;
        const std::string nameField = readTarField(block, 100);
        const std::string prefixField = readTarField(block + 345, 155);
        entry.name = normalizeTarPath(joinUstarPath(prefixField, nameField));

        entry.size = readOctalField(block + 124, 12);
        const char typeflag = static_cast<char>(block[156]);
        entry.isDirectory =
            typeflag == '5' || (!entry.name.empty() && entry.name.back() == '/');

        entry.dataOffset = inStream->tellg();
        entries.push_back(entry);

        if (!entry.isDirectory && entry.size > 0) {
            const std::streamoff paddedSize =
                static_cast<std::streamoff>(((entry.size + kTarBlockSize - 1) / kTarBlockSize) *
                                            kTarBlockSize);
            inStream->seekg(paddedSize, std::ios::cur);
        }
    }
}

void
TarFileHandler::init()
{
    entries.clear();
    entryOpen = false;
    isCurrentDirectory = false;
    entryBuffer.str("");
    entryBuffer.clear();

    if (inStream)
        parseArchive();
}

std::ostream&
TarFileHandler::createVirtualFile(const std::string& path)
{
    if (!outStream)
        throw std::runtime_error("Tar archive is not open for writing");

    finishVirtualFile();

    currentEntryName = normalizeTarPath(path);
    isCurrentDirectory = false;
    entryBuffer.str("");
    entryBuffer.clear();
    entryOpen = true;
    return entryBuffer;
}

void
TarFileHandler::finishVirtualFile()
{
    if (!entryOpen || !outStream)
        return;

    Entry entry;
    entry.name = currentEntryName;
    entry.isDirectory = isCurrentDirectory;

    if (!entry.isDirectory) {
        const std::string compressed = entryBuffer.str();
        entry.size = compressed.size();
        writeHeader(entry);
        if (entry.size > 0) {
            outStream->write(compressed.data(), static_cast<std::streamsize>(compressed.size()));
            writePadding(entry.size);
        }
    } else {
        entry.size = 0;
        writeHeader(entry);
    }

    entries.push_back(entry);
    entryOpen = false;
    isCurrentDirectory = false;
    entryBuffer.str("");
    entryBuffer.clear();
}

void
TarFileHandler::createDirectory(const std::string& path)
{
    std::string dirPath = normalizeTarPath(path);
    if (!dirPath.empty() && dirPath.back() != '/')
        dirPath.push_back('/');

    finishVirtualFile();

    currentEntryName = dirPath;
    isCurrentDirectory = true;
    entryOpen = true;
    finishVirtualFile();
}

std::vector<std::string>
TarFileHandler::getVirtualFiles()
{
    std::vector<std::string> names;
    names.reserve(entries.size());
    for (const Entry& entry : entries) {
        if (!entry.isDirectory && !entry.name.empty())
            names.push_back(entry.name);
    }
    return names;
}

BoundedStream
TarFileHandler::readVirtualFile(const std::string& path)
{
    if (!inStream)
        throw std::runtime_error("Tar archive is not open for reading");

    const std::string normalized = normalizeTarPath(path);
    const auto it = std::find_if(entries.begin(), entries.end(), [&](const Entry& entry) {
        return entry.name == normalized;
    });
    if (it == entries.end())
        throw std::runtime_error("Tar entry not found: " + path);
    if (it->isDirectory)
        throw std::runtime_error("Tar entry is a directory: " + path);

    return BoundedStream(*inStream, it->dataOffset, static_cast<std::size_t>(it->size));
}

void
TarFileHandler::close()
{
    if (outStream) {
        finishVirtualFile();

        const std::array<char, kTarBlockSize> endBlock = zeroTarBlock();
        outStream->write(endBlock.data(), static_cast<std::streamsize>(kTarBlockSize));
        outStream->write(endBlock.data(), static_cast<std::streamsize>(kTarBlockSize));
        outStream->flush();
    }
}
