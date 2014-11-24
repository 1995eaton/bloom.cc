#include <iostream>
#include <fstream>
#include "bloom.hh"

std::vector<std::string> readFile(const char *name) {
  std::vector<std::string> words;
  std::ifstream stream(name);
  for (std::string line; !stream.eof() && std::getline(stream, line);) {
    std::transform(line.begin(), line.end(), line.begin(), ::tolower);
    words.push_back(line);
  }
  std::sort(words.begin(), words.end());
  words.erase(std::unique(words.begin(), words.end()), words.end());
  return words;
}

int main(int argc, char **argv) {
  if (argc == 2) {
    auto words = readFile("american-english");
    BloomFilter bf(words.size(), 0.001);
    for (const auto& e: words) {
      bf.add(e);
    }
    std::cout << bf.has(argv[1]) << std::endl;
  }
}
