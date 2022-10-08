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

RC init_conditions_date(Condition conditions[], int condition_num)
{
  RC rc = RC::SUCCESS;
  for (int i = 0; i < condition_num; i++) {
    /**
     * 左值 date 转化
     */
    if (!conditions[i].left_is_attr) {
      if (conditions[i].left_value.type == DATES) {
        int date = -1;
        rc = string_to_date((char *)conditions[i].left_value.data, date);
        if (rc != RC::SUCCESS) {
          return rc;
        }
        value_destroy(&conditions[i].left_value);
        long_value_init_date(&conditions[i].left_value, date);
      }
    }
    /**
     * 右值 date 转化
     */
    if (!conditions[i].right_is_attr) {
      if (conditions[i].right_value.type == DATES) {
        int date = -1;
        rc = string_to_date((char *)conditions[i].right_value.data, date);
        if (rc != RC::SUCCESS) {
          return rc;
        }
        value_destroy(&conditions[i].right_value);
        long_value_init_date(&conditions[i].right_value, date);
      }
    }
  }
  return rc;
}
