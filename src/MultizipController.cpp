#include <memory>
#include <filesystem>
#include <algorithm>
#include <iostream>
#include <MultizipController.h>
#include <FSController.h>
#include <FileHandler.h>
#include <FileHandlers/ZipFileHandler.h>
#include <Algorithm.h>
#include <Algorithms/LZW.h>

bool
MultizipController::compress(Input input)
{
  FSController& fs = FSController::getInstance();

  // Set up file streams
  bool isDirectory = fs.isDir(input.inputPath);
  std::ofstream outputFile;
  try
  {
    outputFile = fs.createFile(input.outputPath);
  }
  catch(const std::runtime_error& e)
  {
    std::cerr << e.what() << std::endl;
    return false;
  }
  
   
  std::unique_ptr<FileHandler> out;

  // Check to see if user expects a zip as output
  size_t osize = input.outputPath.size();
  std::string zip = ".zip";
  bool isZip = (osize >= zip.size() && input.outputPath.compare(osize-zip.size(), zip.size(), zip) == 0);
  if(isZip)
    out = std::make_unique<ZipFileHandler>(outputFile);

  // Set up compression algorithm
  std::unique_ptr<Algorithm> algo;

  if(std::find(input.options.begin(), input.options.end(), "lzw") != input.options.end())
  {
    algo = std::make_unique<LZW>();
  }

  out->init();
  try
  {
    if(!isDirectory)
    {
      std::filesystem::path path(input.inputPath);

      std::ifstream inStream = fs.readFile(input.inputPath);

      std::ofstream& outStream = out->createVirtualFile(path.filename());

      algo->encode(inStream, outStream);
    }
    else
    {
      std::vector<std::filesystem::path> entries = fs.readDir(input.inputPath);

      for(auto& entry : entries)
      {
        std::ifstream inStream = fs.readFile(entry.string());

        size_t inputPathLength = input.inputPath.size();
        std::string relativePath = entry.string().substr(inputPathLength, entry.string().size());

        std::ofstream& outStream = out->createVirtualFile(relativePath);

        algo->encode(inStream, outStream);
      }
    }
  }
  catch(const std::runtime_error& e)
  {
    std::cerr << e.what() << std::endl;
    return false;
  }
  out->close();
  return true;
}

bool
MultizipController::extract (Input input)
{
  FSController& fs = FSController::getInstance();

  // Set up file streams
  std::ifstream inputFile;
  try
  {
    inputFile = fs.readFile(input.inputPath);
  }
  catch(const std::runtime_error& e)
  {
    std::cerr << e.what() << std::endl;
    return false;
  }
   
  std::unique_ptr<FileHandler> in;

  // Check to see if user expects a zip as input
  size_t osize = input.inputPath.size();
  std::string zip = ".zip";
  bool isZip = (osize >= zip.size() && input.outputPath.compare(osize-zip.size(), zip.size(), zip) == 0);
  if(isZip)
    in = std::make_unique<ZipFileHandler>(inputFile);

  // Set up compression algorithm
  std::unique_ptr<Algorithm> algo;

  if(std::find(input.options.begin(), input.options.end(), "lzw") != input.options.end())
  {
    algo = std::make_unique<LZW>();
  }

  in->init();
  try
  {
    std::vector<std::string> vEntries = in->getVirtualFiles();

    for(auto& vEntry : vEntries)
    {
      BoundedStream vFile = in->readVirtualFile(vEntry);
      std::ofstream outStream = fs.createFile(vEntry);

      algo->decode(vFile, outStream);
    }
  }
  catch(const std::runtime_error& e)
  {
    std::cerr << e.what() << std::endl;
    return false;
  }
  in->close();
  return true;
}
