//
// Created by root on 10/26/22.
//

#include "null_tag.h"


const char * NullTag::null_tag_field_name()
{
  return "__null_tag";
}

AttrType NullTag::null_tag_type()
{
  return INTS;
}

int NullTag::null_tag_field_len()
{
  return sizeof(int);
}

NullTag::NullTag()
{}

NullTag::~NullTag()
{}


