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

void *NullTag::get_null_tag() const
{
  return (void*)&null_tag_;
}

void NullTag::set_null_tag_bit(int index)
{
  null_tag_[index] = true;
}

NullTag::NullTag()
{}

NullTag::~NullTag()
{}


