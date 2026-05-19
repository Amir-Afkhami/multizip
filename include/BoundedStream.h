#pragma once

#include <cstddef>
#include <istream>
#include <streambuf>

class SegmentStreambuf : public std::streambuf {
    std::istream& parent;
    std::size_t remaining;

public:
    SegmentStreambuf(std::istream& in, std::streamoff offset, std::size_t size)
        : parent(in), remaining(size)
    {
        parent.seekg(offset);
    }

protected:
    int underflow() override
    {
        if (remaining == 0)
            return traits_type::eof();

        int ch = parent.get();
        if (!parent)
            return traits_type::eof();

        --remaining;
        return ch;
    }

    std::streamsize xsgetn(char* s, std::streamsize count) override
    {
        std::streamsize total = 0;
        while (total < count && remaining > 0) {
            std::streamsize chunk = count - total;
            if (static_cast<std::size_t>(chunk) > remaining)
                chunk = static_cast<std::streamsize>(remaining);

            parent.read(s + total, chunk);
            std::streamsize got = parent.gcount();
            if (got <= 0)
                break;

            total += got;
            remaining -= static_cast<std::size_t>(got);
        }
        return total;
    }
};

class BoundedStream : public std::istream {
    SegmentStreambuf buffer;
    std::size_t max_length;
    std::size_t bytes_read;

public:
    BoundedStream(std::istream& in, std::streamoff offset, std::size_t limit)
        : std::istream(&buffer), buffer(in, offset, limit), max_length(limit), bytes_read(0)
    {
        rdbuf(&buffer);
    }

    std::streamsize read_bounded(char* data, std::streamsize count)
    {
        std::streamsize to_read = count;
        if (bytes_read + static_cast<std::size_t>(to_read) > max_length)
            to_read = static_cast<std::streamsize>(max_length - bytes_read);

        read(data, to_read);
        std::streamsize actual = gcount();
        bytes_read += static_cast<std::size_t>(actual);
        return actual;
    }

    std::size_t get_bytes_read() const { return bytes_read; }

    bool reached_limit() const { return bytes_read >= max_length; }
};
