#include "database.hpp"

#include <WDL/tinyxml/tinyxml.h>

#include <cstdio>

DatabasePtr Database::load(const char *file, const char **error)
{
  TiXmlDocument doc(file);

  if(!doc.LoadFile()) {
    *error = "failed to read database";
    return 0;
  }

  TiXmlHandle docHandle(&doc);
  TiXmlElement *root = doc.RootElement();

  if(strcmp(root->Value(), "index")) {
    *error = "invalid database";
    return 0;
  }

  int version = 0;
  root->Attribute("version", &version);

  if(!version) {
    *error = "invalid version";
    return 0;
  }

  switch(version) {
  case 1:
    return loadV1(root, error);
  default:
    *error = "unsupported version";
    return 0;
  }
}

Database::Database()
{
  printf("constructed\n");
}

Database::~Database()
{
  printf("destructed\n");
}
