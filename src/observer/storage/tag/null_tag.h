//
// Created by root on 10/26/22.
//

#ifndef MINIDB_NULL_TAG_H
#define MINIDB_NULL_TAG_H
#include "sql/parser/parse.h"


class NullTag {
public:
  NullTag();
  ~NullTag();

  static const char *null_tag_field_name();
  static AttrType null_tag_type();
  static int null_tag_field_len();


private:
  int null_tag = 0;
};

#endif  // MINIDB_NULL_TAG_H
