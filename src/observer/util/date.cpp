//
// Created by root on 9/19/22.
//

#include <cstdio>
#include "date.h"

inline bool is_leap_year(int year) {
  return (year%400==0 || (year%100 && year%4==0));
}

bool check_date(int y, int m, int d)
{
  static int mon[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  bool leap = is_leap_year(y);
  return y > 0
         && (m > 0)&&(m <= 12)
         && (d > 0)&&(d <= ((m==2 && leap)?1:0) + mon[m]);
}

RC string_to_date(const char* str, int &date) {
  int y,m,d;
  int ret = sscanf(str, "%d-%d-%d", &y, &m, &d);
  if (ret != 3) {
    return RC::INVALID_ARGUMENT;
  }
  bool b = check_date(y,m,d);
  if(!b) {
    return RC::INVALID_ARGUMENT;
  }
  date = y*10000+m*100+d;
  return RC::SUCCESS;
}
