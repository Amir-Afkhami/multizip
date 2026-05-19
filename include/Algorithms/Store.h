#pragma once

#include <Algorithm.h>

class Store : public Algorithm {
public:
    void encode(std::istream& file, std::ostream& out) override;
    void decode(BoundedStream& in, std::ostream& file) override;
    uint16_t zipCompressionMethod() const override { return 0; }
};
