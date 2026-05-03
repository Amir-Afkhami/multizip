#pragma once

#include <Input.h>
#include <string>

class MultizipController {
public:
    bool compress(Input input);
    bool extract(Input input);
};

