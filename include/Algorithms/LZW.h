#pragma once

#include <string>
#include <Algorithm.h>

class LZW : public Algorithm {
public:
    virtual ~LZW() = default;
    
    virtual void encode(std::ifstream& file, std::ofstream& out) override;
    virtual void decode(BoundedStream& in, std::ofstream& file) override;
};