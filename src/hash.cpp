#include "hash.hpp"

#include <cstdio>

Hash::Hash(const Algorithm algo)
  : m_algo(algo)
{
  switch(algo) {
  case SHA256:
#ifdef __APPLE__
    CC_SHA256_Init(&m_sha256Context);
#endif
    break;
  }
}

void Hash::write(const char *data, const size_t len)
{
  switch(m_algo) {
  case SHA256:
#ifdef __APPLE__
    CC_SHA256_Update(&m_sha256Context, data, len);
#endif
    break;
  }
}

const std::string &Hash::digest()
{
  if(!m_value.empty())
    return m_value;

  switch(m_algo) {
  case SHA256:
#ifdef __APPLE__
    unsigned char hash[2 + CC_SHA256_DIGEST_LENGTH] =
      {SHA256, CC_SHA256_DIGEST_LENGTH};
    CC_SHA256_Final(&hash[2], &m_sha256Context);
    setValue(hash, sizeof(hash));
#endif
    break;
  }

  return m_value;
}

void Hash::setValue(const unsigned char *hash, size_t len)
{
  m_value.resize(len * 2);

  for(size_t i = 0; i < len; ++i)
    sprintf(&m_value[i * 2], "%02x", hash[i]);
}
