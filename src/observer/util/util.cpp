//
// Created by root on 11/4/22.
//

#include "util.h"


std::string double2string(double v)
{
  char buf[256];
  snprintf(buf, sizeof(buf), "%.2f", v);
  size_t len = strlen(buf);
  while (buf[len - 1] == '0') {
    len--;

  }
  if (buf[len - 1] == '.') {
    len--;
  }

  return std::string(buf, len);
}