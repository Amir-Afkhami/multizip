#include <FileHandlers/ZipFileHandler.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <stdexcept>

namespace {

// ZIP record signatures (little-endian "PK" prefixes).
constexpr std::array<unsigned char, 4> kLocalFileMagic = {0x50, 0x4B, 0x03, 0x04};
constexpr std::array<unsigned char, 4> kCentralDirectoryMagic = {0x50, 0x4B, 0x01, 0x02};
constexpr std::array<unsigned char, 4> kEndCentralDirectoryMagic = {0x50, 0x4B, 0x05, 0x06};
constexpr std::streamoff kEndCentralDirectorySize = 22; // minimum EOCD record size

constexpr std::uint32_t kCrc32Polynomial = 0xEDB88320u;

bool
matchesMagic(const unsigned char* bytes, const std::array<unsigned char, 4>& expected)
{
    return std::memcmp(bytes, expected.data(), expected.size()) == 0;
}

// CRC-32 used by the ZIP format for uncompressed data integrity.
std::uint32_t crc32(const std::uint8_t* data, std::size_t length)
{
    static std::array<std::uint32_t, 256> table = [] {
        std::array<std::uint32_t, 256> values{};
        for (std::uint32_t i = 0; i < 256; ++i) {
            std::uint32_t crc = i;
            for (int bit = 0; bit < 8; ++bit)
                crc = (crc & 1) ? (crc >> 1) ^ kCrc32Polynomial : (crc >> 1);
            values[i] = crc;
        }
        return values;
    }();

    std::uint32_t crc = 0xFFFFFFFFu;
    for (std::size_t i = 0; i < length; ++i)
        crc = table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
    return crc ^ 0xFFFFFFFFu;
}

// ZIP integers are little-endian.
void writeLe16(std::ostream& out, std::uint16_t value)
{
    out.put(static_cast<char>(value & 0xFF));
    out.put(static_cast<char>((value >> 8) & 0xFF));
}

void writeLe32(std::ostream& out, std::uint32_t value)
{
    for (int shift = 0; shift < 32; shift += 8)
        out.put(static_cast<char>((value >> shift) & 0xFF));
}

std::uint16_t readLe16(std::istream& in)
{
    unsigned char bytes[2];
    in.read(reinterpret_cast<char*>(bytes), 2);
    return static_cast<std::uint16_t>(bytes[0]) |
           (static_cast<std::uint16_t>(bytes[1]) << 8);
}

std::uint32_t readLe32(std::istream& in)
{
    unsigned char bytes[4];
    in.read(reinterpret_cast<char*>(bytes), 4);
    return static_cast<std::uint32_t>(bytes[0]) |
           (static_cast<std::uint32_t>(bytes[1]) << 8) |
           (static_cast<std::uint32_t>(bytes[2]) << 16) |
           (static_cast<std::uint32_t>(bytes[3]) << 24);
}

// ZIP stores modification time/date in MS-DOS format.
void writeDosTimestamp(std::ostream& out)
{
    using namespace std::chrono;
    const auto now = system_clock::now();
    const std::time_t t = system_clock::to_time_t(now);
    std::tm localTime{};
    localtime_r(&t, &localTime);

    const std::uint16_t dosTime =
        static_cast<std::uint16_t>((localTime.tm_hour << 11) | (localTime.tm_min << 5) |
                                   (localTime.tm_sec / 2));
    const std::uint16_t dosDate =
        static_cast<std::uint16_t>(((localTime.tm_year - 80) << 9) | ((localTime.tm_mon + 1) << 5) |
                                   localTime.tm_mday);

    writeLe16(out, dosTime);
    writeLe16(out, dosDate);
}

// Archive paths use forward slashes and have no leading slash.
std::string normalizeZipPath(const std::string& path)
{
    std::string normalized = path;
    std::replace(normalized.begin(), normalized.end(), '\\', '/');
    while (!normalized.empty() && normalized.front() == '/')
        normalized.erase(normalized.begin());
    return normalized;
}

} // namespace

// Writes the header that precedes each entry's compressed bytes.
void
ZipFileHandler::writeLocalHeader(const Entry& entry)
{
    Zip::LocalHeader header{};
    header.version[0] = 20;
    header.version[1] = 0;
    header.flag[0] = 0;
    header.flag[1] = 0;

    outStream->write(reinterpret_cast<const char*>(header.magic), 4);
    outStream->write(reinterpret_cast<const char*>(header.version), 2);
    outStream->write(reinterpret_cast<const char*>(header.flag), 2);
    writeLe16(*outStream, entry.compressionMethod);
    writeDosTimestamp(*outStream);
    writeLe32(*outStream, entry.crc32);
    writeLe32(*outStream, entry.compressedSize);
    writeLe32(*outStream, entry.uncompressedSize);
    writeLe16(*outStream, static_cast<std::uint16_t>(entry.name.size()));
    writeLe16(*outStream, 0);
    outStream->write(entry.name.data(), static_cast<std::streamsize>(entry.name.size()));
}

// Writes one central-directory record per entry (metadata index at end of archive).
void
ZipFileHandler::writeCentralDirectory()
{
    for (const Entry& entry : entries) {
        Zip::CentralDirectory header{};
        header.versionby[0] = 20;
        header.versionby[1] = 0;
        header.version[0] = 20;
        header.version[1] = 0;

        outStream->write(reinterpret_cast<const char*>(header.magic), 4);
        outStream->write(reinterpret_cast<const char*>(header.versionby), 2);
        outStream->write(reinterpret_cast<const char*>(header.version), 2);
        writeLe16(*outStream, 0);
        writeLe16(*outStream, entry.compressionMethod);
        writeDosTimestamp(*outStream);
        writeLe32(*outStream, entry.crc32);
        writeLe32(*outStream, entry.compressedSize);
        writeLe32(*outStream, entry.uncompressedSize);
        writeLe16(*outStream, static_cast<std::uint16_t>(entry.name.size()));
        writeLe16(*outStream, 0);
        writeLe16(*outStream, 0);
        writeLe16(*outStream, 0);
        writeLe16(*outStream, 0);
        writeLe32(*outStream, entry.isDirectory ? 0x10 : 0); // 0x10 = directory attribute
        writeLe32(*outStream, entry.localHeaderOffset);
        outStream->write(entry.name.data(), static_cast<std::streamsize>(entry.name.size()));
    }
}

// Final record; points back to the central directory so readers can find entries.
void
ZipFileHandler::writeEndOfCentralDirectory(std::uint32_t centralDirOffset, std::uint16_t entryCount)
{
    const std::uint32_t centralDirSize =
        static_cast<std::uint32_t>(outStream->tellp()) - centralDirOffset;

    Zip::EndCentralDirectory record{};
    outStream->write(reinterpret_cast<const char*>(record.magic), 4);
    writeLe16(*outStream, 0);
    writeLe16(*outStream, 0);
    writeLe16(*outStream, entryCount);
    writeLe16(*outStream, entryCount);
    writeLe32(*outStream, centralDirSize);
    writeLe32(*outStream, centralDirOffset);
    writeLe16(*outStream, 0);
}

// Read path: locate the end-of-central-directory record, then load entry metadata.
void
ZipFileHandler::parseArchive()
{
    if (!inStream)
        return;

    inStream->seekg(0, std::ios::end);
    const auto fileSizePos = inStream->tellg();
    const std::streamoff fileSize = static_cast<std::streamoff>(fileSizePos);
    if (fileSize < static_cast<std::streamoff>(sizeof(Zip::EndCentralDirectory)))
        throw std::runtime_error("Invalid zip archive: file too small");

    bool foundEndRecord = false;
    std::streamoff searchStart = fileSize - kEndCentralDirectorySize;
    if (searchStart < 0)
        searchStart = 0;

    std::uint32_t centralDirOffset = 0;
    std::uint16_t entryCount = 0;

    // EOCD has no fixed offset; scan backward from the end (comment may precede it).
    for (std::streamoff pos = fileSize - kEndCentralDirectorySize; pos >= searchStart; --pos) {
        inStream->seekg(pos);
        unsigned char magic[4];
        inStream->read(reinterpret_cast<char*>(magic), 4);
        if (matchesMagic(magic, kEndCentralDirectoryMagic)) {
            inStream->seekg(pos + 16); // offset of central directory
            centralDirOffset = readLe32(*inStream);
            inStream->seekg(pos + 10); // entry count on this disk
            entryCount = readLe16(*inStream);
            foundEndRecord = true;
            break;
        }
        if (pos == 0)
            break;
    }

    if (!foundEndRecord)
        throw std::runtime_error("Invalid zip archive: missing end record");

    inStream->seekg(static_cast<std::streamoff>(centralDirOffset));
    for (std::uint16_t i = 0; i < entryCount; ++i) {
        unsigned char magic[4];
        inStream->read(reinterpret_cast<char*>(magic), 4);
        if (!matchesMagic(magic, kCentralDirectoryMagic))
            throw std::runtime_error("Invalid zip archive: corrupt central directory");

        inStream->seekg(6, std::ios::cur); // skip version fields and general-purpose flag
        const std::uint16_t compressionMethod = readLe16(*inStream);
        inStream->seekg(4, std::ios::cur); // skip last-mod time and date
        const std::uint32_t crc = readLe32(*inStream);
        const std::uint32_t compressedSize = readLe32(*inStream);
        const std::uint32_t uncompressedSize = readLe32(*inStream);
        const std::uint16_t nameLength = readLe16(*inStream);
        const std::uint16_t extraLength = readLe16(*inStream);
        const std::uint16_t commentLength = readLe16(*inStream);
        inStream->seekg(8, std::ios::cur); // skip disk number and internal/external attrs
        const std::uint32_t localHeaderOffset = readLe32(*inStream);

        std::string name(nameLength, '\0');
        inStream->read(name.data(), nameLength);
        inStream->seekg(extraLength + commentLength, std::ios::cur);

        Entry entry;
        entry.name = name;
        entry.localHeaderOffset = localHeaderOffset;
        entry.crc32 = crc;
        entry.compressedSize = compressedSize;
        entry.uncompressedSize = uncompressedSize;
        entry.compressionMethod = compressionMethod;
        entry.isDirectory = !name.empty() && name.back() == '/'; // ZIP convention for dirs
        entries.push_back(entry);
    }
}

// Buffer uncompressed data so we can compute CRC/size before writing the local header.
void
ZipFileHandler::compressEntry(std::istream& in, const std::string& virtualPath)
{
    const std::string uncompressed((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    pendingCrc32 =
        crc32(reinterpret_cast<const std::uint8_t*>(uncompressed.data()), uncompressed.size());
    pendingUncompressedSize = static_cast<std::uint32_t>(uncompressed.size());

    std::istringstream uncompressedStream(uncompressed);
    std::ostream& virtualOut = createVirtualFile(virtualPath);
    algorithm->encode(uncompressedStream, virtualOut); // writes into entryBuffer
    finishVirtualFile();
}

void
ZipFileHandler::init()
{
    entries.clear();
    entryOpen = false;
    entryBuffer.str("");
    entryBuffer.clear();

    if (inStream)
        parseArchive();
}

// Returns a buffer stream; compressed bytes are flushed to the archive in finishVirtualFile().
std::ostream&
ZipFileHandler::createVirtualFile(const std::string& path)
{
    if (!outStream)
        throw std::runtime_error("Zip archive is not open for writing");

    finishVirtualFile(); // finalize previous entry, if any

    currentEntryName = normalizeZipPath(path);
    entryBuffer.str("");
    entryBuffer.clear();
    entryOpen = true;
    return entryBuffer;
}

void
ZipFileHandler::finishVirtualFile()
{
    if (!entryOpen || !outStream)
        return;

    const std::string compressed = entryBuffer.str();
    Entry entry;
    entry.name = currentEntryName;
    entry.localHeaderOffset = static_cast<std::uint32_t>(outStream->tellp()); // for central dir
    entry.crc32 = pendingCrc32;
    entry.uncompressedSize = pendingUncompressedSize;
    entry.compressedSize = static_cast<std::uint32_t>(compressed.size());
    entry.compressionMethod = algorithm->zipCompressionMethod();
    entry.isDirectory = !entry.name.empty() && entry.name.back() == '/';

    writeLocalHeader(entry);
    outStream->write(compressed.data(), static_cast<std::streamsize>(compressed.size()));
    entries.push_back(entry);

    entryOpen = false;
    entryBuffer.str("");
    entryBuffer.clear();
}

// Directories are zero-length entries whose names end with '/'.
void
ZipFileHandler::createDirectory(const std::string& path)
{
    std::string dirPath = normalizeZipPath(path);
    if (!dirPath.empty() && dirPath.back() != '/')
        dirPath.push_back('/');

    pendingCrc32 = 0;
    pendingUncompressedSize = 0;
    currentEntryName = dirPath;
    entryBuffer.str("");
    entryBuffer.clear();
    entryOpen = true;
    finishVirtualFile();
}

std::vector<std::string>
ZipFileHandler::getVirtualFiles()
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
ZipFileHandler::readVirtualFile(const std::string& path)
{
    if (!inStream)
        throw std::runtime_error("Zip archive is not open for reading");

    const std::string normalized = normalizeZipPath(path);
    const auto it = std::find_if(entries.begin(), entries.end(), [&](const Entry& entry) {
        return entry.name == normalized;
    });
    if (it == entries.end())
        throw std::runtime_error("Zip entry not found: " + path);

    inStream->seekg(static_cast<std::streamoff>(it->localHeaderOffset));
    unsigned char magic[4];
    inStream->read(reinterpret_cast<char*>(magic), 4);
    if (!matchesMagic(magic, kLocalFileMagic))
        throw std::runtime_error("Invalid zip entry header: " + path);

    inStream->seekg(22, std::ios::cur); // skip fixed local-header fields after signature
    const std::uint16_t nameLength = readLe16(*inStream);
    const std::uint16_t extraLength = readLe16(*inStream);
    inStream->seekg(nameLength + extraLength, std::ios::cur); // skip variable fields

    const auto dataOffset = inStream->tellg();
    return BoundedStream(*inStream, dataOffset, it->compressedSize); // bounded to entry payload
}

void
ZipFileHandler::close()
{
    if (outStream) {
        finishVirtualFile();
        // ZIP layout: [local headers + data ...] [central directory] [end record]
        const std::uint32_t centralDirOffset = static_cast<std::uint32_t>(outStream->tellp());
        writeCentralDirectory();
        writeEndOfCentralDirectory(centralDirOffset,
                                   static_cast<std::uint16_t>(entries.size()));
        outStream->flush();
    }
}
