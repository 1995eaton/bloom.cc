#ifndef BLOOM_HH
#define BLOOM_HH

#include <random>
#include <vector>
#include <algorithm>
#include <functional>
#include <cmath>

class BloomFilter {
  private:
    size_t n;
    double p;
    size_t m, k;
    std::mt19937 mt_engine;
    static const std::vector<std::function<
      uint32_t(const void*, size_t, uint32_t)>> hashes;
    std::vector<uint32_t> seeds;
  public:
    std::vector<bool> data;
    BloomFilter(size_t _n, double _p) :
      n(_n), p(_p),
      m( ceil(n * log(p) / log(1 / pow(2, log(2)))) ),
      k( std::max(static_cast<size_t>(1),
         static_cast<size_t>(round(log(2) * m / n))) ) {
      mt_engine.seed(time(nullptr));
      std::generate_n(std::back_inserter(seeds),
          ceil(k / static_cast<double>(hashes.size())), mt_engine);
      data.resize(m);
    }
    void add(const std::string& _key);
    bool has(const std::string& _key);
};

#endif
