#include <MultizipUI.h>
#include <MultizipController.h>
#include <Input.h>
#include <iostream>
#include <algorithm>

void
MultizipUI::init (int argc, char* argv[])
{
    std::vector<std::string> args(argv, argv + argc);

    Input input = {};

    bool readNextInput = false;
    bool readNextOutput = false;

    for(int i = 1; i < args.size(); i++)
    {
        if(args.at(i).at(0) != '-')
        {
            // If 'i' or 'o' was specified then read the next path
            if(readNextInput)
            {
                input.inputPath = args.at(i);
                continue;
            }
            if(readNextOutput)
            {
                input.outputPath = args.at(i);
                continue;
            }

            // If no 'i' or 'o' was specified then resort or order
            if(input.inputPath.size() == 0)
            {
                input.inputPath = args.at(i);
            }
            else if (input.outputPath.size() == 0)
            {
                input.outputPath = args.at(i);
            }
            else
            {
                std::cerr << "ERROR: Incorrect arguments specified. Please see usage: multizip -h" << std::endl;
                return;
            }
        }
        else
        {
            switch(args.at(i).c_str()[1])
            {
                case 'h':
                    this->printHelp();
                    return;
                case 'i':
                    readNextOutput = false;
                    readNextInput = true;
                    break;
                case 'o':
                    readNextInput = false;
                    readNextOutput = true;
                    break;
                case 'c':
                    input.options.push_back("c");
                    break;
                case 'u':
                    input.options.push_back("u");
                    break;
                default:
                    std::cerr << "ERROR: The option specified doesn't exist. Please see help: multizip -h" << std::endl;
                    return;
            }
        }
    }

    if(input.inputPath.size() == 0 || input.outputPath.size() == 0)
    {
        std::cerr << "ERROR: No input was specified. Please see help: multizip -h" << std::endl;
        return;
    }

    MultizipController c;
    bool result;

    if(std::find(input.options.begin(), input.options.end(), "c") != input.options.end())
    {
        result = c.compress(input);
    }
    else if(std::find(input.options.begin(), input.options.end(), "u") != input.options.end())
    {
        result = c.extract(input);
    }
    else
    {
        result = c.compress(input);
    }
    
    if (!result)
    {
        std::cerr << "ERROR: Operation Failed" << std::endl;
    }
}

void
MultizipUI::printHelp ()
{
    std::cout << "Usage: multizip [OPTIONS] <input> <output>\n"
              << "\nOptions:\n"
              << "  -h                Show this help message\n"
              << "  -i <path>         Specify input file path\n"
              << "  -o <path>         Specify output file path\n"
              << "  -c                Compress files (default)\n"
              << "  -u                Extract/uncompress files\n"
              << "\nExamples:\n"
              << "  multizip input.txt output.zip -c\n"
              << "  multizip -i archive.zip -o extracted -u\n";
}