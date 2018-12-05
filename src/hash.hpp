#ifndef REAPACK_HASH_HPP
#define REAPACK_HASH_HPP

#include <string>

#ifdef __APPLE__
#  include <CommonCrypto/CommonDigest.h>
#endif

class Hash {
public:
  enum Algorithm {
    SHA256 = 0x12,
  };

  Hash(Algorithm);
  void write(const char *data, size_t len);
  const std::string &digest();

private:
  void setValue(const unsigned char *hash, size_t len);

  Algorithm m_algo;
  std::string m_value;

#ifdef __APPLE__
  CC_SHA256_CTX m_sha256Context;
#endif
};

#endif
