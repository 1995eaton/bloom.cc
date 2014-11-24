#include "bloom.hh"

static inline uint32_t ROT32(uint32_t x, uint32_t k) {
  return (x << k) | (x >> (32 - k));
}

template<typename T, typename std::enable_if<std::is_arithmetic<T>::value>::type* = nullptr>
static inline void JENKINS_MIX(T& a, T& b, T& c) {
  a -= c;  a ^= ROT32(c, 4);  c += b;
  b -= a;  b ^= ROT32(a, 6);  a += c;
  c -= b;  c ^= ROT32(b, 8);  b += a;
  a -= c;  a ^= ROT32(c,16);  c += b;
  b -= a;  b ^= ROT32(a,19);  a += c;
  c -= b;  c ^= ROT32(b, 4);  b += a;
}

void BloomFilter::add(const std::string& _key) {
  const size_t len = _key.size();
  const char *key = _key.c_str();
  size_t i = 0;
  for (const auto& hash: hashes) {
    for (const auto& seed: seeds) {
      data[hash(key, len, seed) % m] = true;
      if (++i == k) {
        return;
      }
    }
  }
}

bool BloomFilter::has(const std::string& _key) {
  const size_t len = _key.size();
  const char *key = _key.c_str();
  size_t i = 0;
  for (const auto& hash: hashes) {
    for (const auto& seed: seeds) {
      if (data[hash(key, len, seed) % m] != true)
        return false;
      if (++i == k)
        return true;
    }
  }
  return true;
}

const std::vector<std::function<uint32_t(const void*, size_t, uint32_t)>>
BloomFilter::hashes = {

  [](auto key, auto len, auto seed) {
    const uint32_t m = 0x5bd1e995;
    const int r = 24;
    uint32_t h = seed ^ len;
    const uint8_t *data = static_cast<const uint8_t*>(key);
    while (len >= 4) {
      uint32_t k = *reinterpret_cast<const uint32_t*>(data);
      k *= m;
      k ^= k >> r;
      k *= m * m;
      h ^= k;
      data += 4;
      len -= 4;
    }
    switch (len) {
      case 3: h ^= data[2] << 16;
      case 2: h ^= data[1] << 8;
      case 1: h ^= data[0];
              h *= m;
    }
    h ^= h >> 13;
    h *= m;
    h ^= h >> 15;
    return h;
  },

  [](auto key, auto len, auto seed) {
    const uint8_t *data = static_cast<const uint8_t*>(key);
    const int nblocks = len / 4;
    uint32_t h = seed, k;
    const uint32_t *blocks = reinterpret_cast<const uint32_t*>(data + nblocks * 4);
    for (int i = -nblocks; i; i++) {
      k = blocks[i];
      k *= 0xcc9e2d51;
      k = (k << 15) | (k >> (32 - 15));
      k *= 0x1b873593;

      h ^= k;
      h = (h << 13) | (h >> (32 - 13));
      h = h*5+0xe6546b64;
    }
    const uint8_t *tail = static_cast<const uint8_t*>(data + nblocks*4);
    k = 0;
    switch (len & 3) {
      case 3:
        k ^= tail[2] << 16;
      case 2:
        k ^= tail[1] << 8;
      case 1:
        k ^= tail[0];
        k *= 0xcc9e2d51;
        k = (k << 15) | (k >> (32 - 15));
        k *= 0x1b873593; h ^= k;
    }
    h ^= len;
    h ^= h >> 16;
    h *= 0x85ebca6b;
    h ^= h >> 13;
    h *= 0xc2b2ae35;
    h ^= h >> 16;
    return h;
  },

  [](auto key, auto len, auto seed) {
    uint32_t a, b, c;
    a = b = c = 0xdeadbeef + static_cast<uint32_t>(len) + seed;
    size_t u = *static_cast<const size_t*>(key);
    if (!(u & 3)) {
      const uint32_t *k = (const uint32_t *)key;
      while (len > 12) {
        a += k[0];
        b += k[1];
        c += k[2];
        JENKINS_MIX(a,b,c);
        len -= 12;
        k += 3;
      }

      switch (len) {
        case 12: c += k[2]; b += k[1]; a += k[0]; break;
        case 11: c += k[2] & 0xffffff; b += k[1]; a += k[0]; break;
        case 10: c += k[2] & 0xffff; b += k[1]; a += k[0]; break;
        case 9:  c += k[2] & 0xff; b += k[1]; a += k[0]; break;
        case 8:  b += k[1]; a += k[0]; break;
        case 7:  b += k[1] & 0xffffff; a += k[0]; break;
        case 6:  b += k[1] & 0xffff; a += k[0]; break;
        case 5:  b += k[1] & 0xff; a += k[0]; break;
        case 4:  a += k[0]; break;
        case 3:  a += k[0] & 0xffffff; break;
        case 2:  a += k[0] & 0xffff; break;
        case 1:  a += k[0] & 0xff; break;
        case 0:  return c;
      }
    } else if (!(u & 1)) {
      const uint16_t *k = static_cast<const uint16_t*>(key);
      const uint8_t *k8;
      while (len > 12) {
        a += k[0] + (static_cast<uint32_t>(k[1]) << 16);
        b += k[2] + (static_cast<uint32_t>(k[3]) << 16);
        c += k[4] + (static_cast<uint32_t>(k[5]) << 16);
        JENKINS_MIX(a,b,c);
        len -= 12;
        k += 6;
      }

      k8 = (const uint8_t *)k;
      switch(len) {
        case 12:
          c += k[4] + (static_cast<uint32_t>(k[5]) << 16);
          b += k[2] + (static_cast<uint32_t>(k[3]) << 16);
          a += k[0] + (static_cast<uint32_t>(k[1]) << 16);
          break;
        case 11:
          c += static_cast<uint32_t>(k8[10]) << 16;
        case 10:
          c += k[4];
          b += k[2] + (static_cast<uint32_t>(k[3]) << 16);
          a += k[0] + (static_cast<uint32_t>(k[1]) << 16);
          break;
        case 9:
          c += k8[8];
        case 8:
          b += k[2] + (static_cast<uint32_t>(k[3]) << 16);
          a += k[0] + (static_cast<uint32_t>(k[1]) << 16);
          break;
        case 7:
          b += static_cast<uint32_t>(k8[6]) << 16;
        case 6:
          b += k[2];
          a += k[0] + (static_cast<uint32_t>(k[1]) << 16);
          break;
        case 5:
          b += k8[4];
        case 4:
          a += k[0] + (static_cast<uint32_t>(k[1]) << 16);
          break;
        case 3:
          a += static_cast<uint32_t>(k8[2]) << 16;
        case 2:
          a += k[0];
          break;
        case 1:
          a += k8[0];
          break;
        case 0:
          return c;
      }
    } else {
      const uint8_t *k = static_cast<const uint8_t*>(key);
      while (len > 12) {
        a += k[0];
        a += static_cast<uint32_t>(k[1]) << 8;
        a += static_cast<uint32_t>(k[2]) << 16;
        a += static_cast<uint32_t>(k[3]) << 24;
        b += k[4];
        b += static_cast<uint32_t>(k[5]) << 8;
        b += static_cast<uint32_t>(k[6]) << 16;
        b += static_cast<uint32_t>(k[7]) << 24;
        c += k[8];
        c += static_cast<uint32_t>(k[9]) << 8;
        c += static_cast<uint32_t>(k[10]) << 16;
        c += static_cast<uint32_t>(k[11]) << 24;
        JENKINS_MIX(a,b,c);
        len -= 12;
        k += 12;
      }
      switch (len) {
        case 12: c += static_cast<uint32_t>(k[11]) << 24;
        case 11: c += static_cast<uint32_t>(k[10]) << 16;
        case 10: c += static_cast<uint32_t>(k[9]) << 8;
        case 9:  c += k[8];
        case 8:  b += static_cast<uint32_t>(k[7]) << 24;
        case 7:  b += static_cast<uint32_t>(k[6]) << 16;
        case 6:  b += static_cast<uint32_t>(k[5]) << 8;
        case 5:  b += k[4];
        case 4:  a += static_cast<uint32_t>(k[3]) << 24;
        case 3:  a += static_cast<uint32_t>(k[2]) << 16;
        case 2:  a += static_cast<uint32_t>(k[1]) << 8;
        case 1:  a += k[0];
                 break;
        case 0:  return c;
      }
    }
    c ^= b; c -= ROT32(b,14);
    a ^= c; a -= ROT32(c,11);
    b ^= a; b -= ROT32(a,25);
    c ^= b; c -= ROT32(b,16);
    a ^= c; a -= ROT32(c,4);
    b ^= a; b -= ROT32(a,14);
    c ^= b; c -= ROT32(b,24);
    return c;
  }

};
