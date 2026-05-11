#pragma once

#include <fstream>
#include <cstddef>

class BoundedStream : public std::ifstream {
private:
    std::size_t max_length;
    std::size_t bytes_read;

public:
    BoundedStream() : max_length(0), bytes_read(0) {}

    explicit BoundedStream(const char* filename, std::size_t limit)
        : max_length(limit), bytes_read(0) 
    {
        open(filename);
    }

    void set_limit(std::size_t limit) 
    {
        max_length = limit;
        bytes_read = 0;
    }

    std::streamsize read_bounded(char* buffer, std::streamsize count) 
    {
        std::streamsize to_read = count;
        if (bytes_read + to_read > max_length) {
            to_read = max_length - bytes_read;
        }
        
        read(buffer, to_read);
        std::streamsize actual = gcount();
        bytes_read += actual;
        return actual;
    }

    std::size_t get_bytes_read() const 
    {
        return bytes_read;
    }

    bool reached_limit() const 
    {
        return bytes_read >= max_length;
    }
};
