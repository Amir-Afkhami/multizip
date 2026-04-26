#pragma once

#include <Algorithm.h>
#include <string>

class LZW : public Algorithm {
public:
    virtual ~LZW() = default;
    
    virtual std::string encode(std::string s) override;
    virtual std::string decode(std::string s) override;
};