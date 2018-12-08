#ifndef REAPACK_HASH_HPP
#define REAPACK_HASH_HPP

#include <memory>
#include <string>

class Hash {
public:
  enum Algorithm {
    SHA256 = 0x12,
  };

  static bool getAlgorithm(const std::string &hash, Algorithm *out);

  Hash(Algorithm);
  Hash(const Hash &) = delete;

  void addData(const char *data, size_t len);
  const std::string &digest();

private:
  class Context {
  public:
    virtual ~Context() = default;
    virtual size_t hashSize() const = 0;
    virtual void addData(const char *data, size_t len) = 0;
    virtual void getHash(unsigned char *out) = 0;
  };

  class CNGContext;
  class SHA256Context;

  Algorithm m_algo;
  std::string m_value;
  std::unique_ptr<Context> m_context;
};

#endif
