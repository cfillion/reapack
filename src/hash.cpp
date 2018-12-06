#include "hash.hpp"

#include <cstdio>
#include <vector>

#ifdef __APPLE__
#  include <CommonCrypto/CommonDigest.h>
#  define CC(s) CC_##s
#else
#  include <openssl/sha.h>
#  define CC(s) s
#endif

class Hash::SHA256Context : public Hash::Context {
public:
  SHA256Context() { CC(SHA256_Init)(&m_context); }
  size_t hashSize() const { return CC(SHA256_DIGEST_LENGTH); }
  void addData(const char *data, const size_t len) {
    CC(SHA256_Update)(&m_context, data, len);
  }
  void getHash(unsigned char *out) { CC(SHA256_Final)(out, &m_context); }

private:
  CC(SHA256_CTX) m_context;
};

Hash::Hash(const Algorithm algo)
  : m_algo(algo)
{
  switch(algo) {
  case SHA256:
    m_context = std::make_unique<SHA256Context>();
    break;
  }
}

void Hash::write(const char *data, const size_t len)
{
  if(m_context)
    m_context->addData(data, len);
}

const std::string &Hash::digest()
{
  if(!m_context || !m_value.empty())
    return m_value;

  const size_t hashSize = m_context->hashSize();

  std::vector<unsigned char> multihash(2 + hashSize);
  multihash[0] = m_algo;
  multihash[1] = hashSize;
  m_context->getHash(&multihash[2]);

  m_value.resize(multihash.size() * 2);

  for(size_t i = 0; i < multihash.size(); ++i)
    sprintf(&m_value[i * 2], "%02x", multihash[i]);

  return m_value;
}
