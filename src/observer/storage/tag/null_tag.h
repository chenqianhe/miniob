//
// Created by root on 10/26/22.
//

#ifndef MINIDB_NULL_TAG_H
#define MINIDB_NULL_TAG_H
#include <bitset>
#include "sql/parser/parse.h"


class NullTag {
public:
  NullTag();
  ~NullTag();

  static const char *null_tag_field_name();
  static AttrType null_tag_type();
  static int null_tag_field_len();
  void *get_null_tag() const;
  void set_null_tag_bit(int index);


private:
  std::bitset<sizeof(int)*8> null_tag_;
};

#endif  // MINIDB_NULL_TAG_H
