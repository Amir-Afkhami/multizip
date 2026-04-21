#include <MultizipUI.h>
#include <iostream>

void
MultizipUI::init (int argc, char* argv[])
{
    std::vector<std::string> args(argv, argv + argc);

    for(std::string s : args)
    {
        std::cout << s << std::endl;
    }
}