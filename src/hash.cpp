#include "hash.hpp"

#include <cstdio>
#include <vector>

#ifdef _WIN32

#  include <windows.h>

class Hash::CNGContext : public Hash::Context {
public:
  CNGContext(const Algorithm algo) : m_algo(), m_hash(), m_hashLength() {
    const wchar_t *algoName;

    switch(algo) {
    case SHA256:
      algoName = BCRYPT_SHA256_ALGORITHM;
      break;
    default:
      return;
    }

    BCryptOpenAlgorithmProvider(&m_algo, algoName,
      MS_PRIMITIVE_PROVIDER, 0);

    unsigned long bytesWritten;
    BCryptGetProperty(m_algo, BCRYPT_HASH_LENGTH,
      reinterpret_cast<PUCHAR>(&m_hashLength), sizeof(m_hashLength),
      &bytesWritten, 0);

    BCryptCreateHash(m_algo, &m_hash, nullptr, 0, nullptr, 0, 0);
  }

  ~CNGContext() override {
    if(m_algo)
      BCryptCloseAlgorithmProvider(m_algo, 0);
    if(m_hash)
      BCryptDestroyHash(m_hash);
  }

  size_t hashSize() const override { return m_hashLength; }

  void addData(const char *data, const size_t len) override {
    BCryptHashData(m_hash,
      reinterpret_cast<unsigned char *>(const_cast<char *>(data)),
      static_cast<unsigned long>(len), 0);
  }

  void getHash(unsigned char *out) {
    BCryptFinishHash(m_hash, out, m_hashLength, 0);
  }

private:
  BCRYPT_ALG_HANDLE m_algo;
  BCRYPT_HASH_HANDLE m_hash;
  unsigned long m_hashLength;
};

#else // Unix systems

#  ifdef __APPLE__
#    include <CommonCrypto/CommonDigest.h>
#    define CC(s) CC_##s
#  else
#    include <openssl/sha.h>
#    define CC(s) s
#  endif

class Hash::SHA256Context : public Hash::Context {
public:
  SHA256Context() { CC(SHA256_Init)(&m_context); }

  size_t hashSize() const override {
    return CC(SHA256_DIGEST_LENGTH);
  }

  void addData(const char *data, const size_t len) override {
    CC(SHA256_Update)(&m_context, data, len);
  }

  void getHash(unsigned char *out) override {
    CC(SHA256_Final)(out, &m_context);
  }

private:
  CC(SHA256_CTX) m_context;
};

#endif

Hash::Hash(const Algorithm algo)
  : m_algo(algo)
{
#ifdef _WIN32
  m_context = std::make_unique<CNGContext>(algo);
#else
  switch(algo) {
  case SHA256:
    m_context = std::make_unique<SHA256Context>();
    break;
  }
#endif
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

  // Assuming m_algo and hashSize can fit in one byte. We'll need to implement
  // multihash's varint if we need larger values in the future.
  const size_t hashSize = m_context->hashSize();

  std::vector<unsigned char> multihash(2 + hashSize);
  multihash[0] = m_algo;
  multihash[1] = static_cast<unsigned char>(hashSize);
  m_context->getHash(&multihash[2]);

  m_value.resize(multihash.size() * 2);

  for(size_t i = 0; i < multihash.size(); ++i)
    sprintf(&m_value[i * 2], "%02x", multihash[i]);

  return m_value;
}
