#pragma once

#include <BoundedStream.h>
#include <cstdint>
#include <istream>
#include <ostream>

class Algorithm {
public:
    virtual ~Algorithm() = default;

    virtual void encode(std::istream& file, std::ostream& out) = 0;
    virtual void decode(BoundedStream& in, std::ostream& file) = 0;
    virtual uint16_t zipCompressionMethod() const { return 99; }
};
