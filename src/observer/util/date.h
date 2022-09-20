//
// Created by root on 9/19/22.
//

#ifndef MINIDB_DATE_H
#define MINIDB_DATE_H
#include "rc.h"
#include "sql/parser/parse_defs.h"

inline bool is_leap_year(int year);
bool check_date(int y, int m, int d);
RC string_to_date(const char* str, long &date);
RC init_conditions_date(Condition conditions[], int condition_num);
#endif  // MINIDB_DATE_H
