#pragma once

#include <string>

class MultizipController {
public:
    bool compress(std::string path);
    bool extract(std::string path);
};

