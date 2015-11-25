#ifndef REAPACK_DATABASE_HPP
#define REAPACK_DATABASE_HPP

#include <memory>

class TiXmlElement;

class Database;
typedef std::shared_ptr<Database> DatabasePtr;

class Database {
public:
  static DatabasePtr load(const char *, const char **error);

  Database();
  ~Database();

private:
  static DatabasePtr loadV1(TiXmlElement *, const char **);
};

#endif
