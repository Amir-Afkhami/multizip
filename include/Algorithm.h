#pragma once

#include <string>

class Algorithm {
public:
    virtual ~Algorithm() = default;
    
    virtual std::string encode(std::string s) = 0;
    virtual std::string decode(std::string s) = 0;
};