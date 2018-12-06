#include "hash.hpp"

#include <cstdio>
#include <vector>

#ifdef _WIN32
#  include <map>
#  include <windows.h>

class CNGAlgorithmProvider;
std::map<Hash::Algorithm, std::weak_ptr<CNGAlgorithmProvider>> s_algoCache;

class CNGAlgorithmProvider {
public:
  static std::shared_ptr<CNGAlgorithmProvider> get(const Hash::Algorithm algo)
  {
    auto it = s_algoCache.find(algo);

    if(it != s_algoCache.end() && !it->second.expired())
      return it->second.lock();

    wchar_t *algoName;

    switch(algo) {
    case Hash::SHA256:
      algoName = BCRYPT_SHA256_ALGORITHM;
      break;
    default:
      return nullptr;
    }

    auto provider = std::make_shared<CNGAlgorithmProvider>(algoName);
    s_algoCache[algo] = provider;
    return provider;
  }

  CNGAlgorithmProvider(const wchar_t *algoName)
  {
    BCryptOpenAlgorithmProvider(&m_algo, algoName, MS_PRIMITIVE_PROVIDER, 0);
  }

  ~CNGAlgorithmProvider()
  {
    BCryptCloseAlgorithmProvider(m_algo, 0);
  }

  operator BCRYPT_ALG_HANDLE() const
  {
    return m_algo;
  }

private:
  BCRYPT_ALG_HANDLE m_algo;
};

class Hash::CNGContext : public Hash::Context {
public:
  CNGContext(const std::shared_ptr<CNGAlgorithmProvider> &algo)
    : m_algo(algo), m_hash(), m_hashLength()
  {
    unsigned long bytesWritten;
    BCryptGetProperty(*m_algo, BCRYPT_HASH_LENGTH,
      reinterpret_cast<PUCHAR>(&m_hashLength), sizeof(m_hashLength),
      &bytesWritten, 0);

    BCryptCreateHash(*m_algo, &m_hash, nullptr, 0, nullptr, 0, 0);
  }

  ~CNGContext() override
  {
    BCryptDestroyHash(m_hash);
  }

  size_t hashSize() const override
  {
    return m_hashLength;
  }

  void addData(const char *data, const size_t len) override
  {
    BCryptHashData(m_hash,
      reinterpret_cast<unsigned char *>(const_cast<char *>(data)),
      static_cast<unsigned long>(len), 0);
  }

  void getHash(unsigned char *out)
  {
    BCryptFinishHash(m_hash, out, m_hashLength, 0);
  }

private:
  std::shared_ptr<CNGAlgorithmProvider> m_algo;
  BCRYPT_HASH_HANDLE m_hash;
  unsigned long m_hashLength;
};

#else // Unix systems

#  ifdef __APPLE__
#    define COMMON_DIGEST_FOR_OPENSSL
#    include <CommonCrypto/CommonDigest.h>
#  else
#    include <openssl/sha.h>
#  endif

class Hash::SHA256Context : public Hash::Context {
public:
  SHA256Context()
  {
    SHA256_Init(&m_context);
  }

  size_t hashSize() const override
  {
    return SHA256_DIGEST_LENGTH;
  }

  void addData(const char *data, const size_t len) override
  {
    SHA256_Update(&m_context, data, len);
  }

  void getHash(unsigned char *out) override
  {
    SHA256_Final(out, &m_context);
  }

private:
  SHA256_CTX m_context;
};

#endif

Hash::Hash(const Algorithm algo)
  : m_algo(algo)
{
#ifdef _WIN32
  if(const auto &algoProvider = CNGAlgorithmProvider::get(algo))
    m_context = std::make_unique<CNGContext>(algoProvider);
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
