#pragma once

#include <string>
#include <BoundedStream.h>

class Algorithm {
public:
    virtual ~Algorithm() = default;
    
    virtual void encode(std::ifstream& file, std::ofstream& out) = 0;
    virtual void decode(BoundedStream& in, std::ofstream& file) = 0;
};