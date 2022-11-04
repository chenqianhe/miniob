//
// Created by root on 11/1/22.
//

#ifndef MINIDB_PRINTER_H
#define MINIDB_PRINTER_H
#include <algorithm>
#include <sstream>
#include "vector"
#include "string"
#include "sql/parser/parse_defs.h"
#include "sql/expr/tuple.h"
#include "sql/expr/TupleSet.h"
#include "util/util.h"

class Printer {
public:
  Printer()=default;
  ~Printer()=default;

  void insert_column_name(const std::string& name) { column_names_.emplace_back(name); };
  void set_column_names(const std::vector<std::string>& names) { column_names_ = names; };
  void expand_rows() { contents_.emplace_back(std::vector<Value>()); }
  void insert_value_from_tuple(const Tuple &tuple);
  void insert_value_by_column_name(TupleSet &tupleset);
  void insert_value(Value value) { contents_[contents_.size()-1].emplace_back(value); };
  void print_headers(std::ostream &os);
  void print_contents(std::ostream &os);
  void sort_contents(int order_condition_num, OrderCondition order_conditions[]);
  void group_contents(int group_condition_num, GroupCondition group_conditions[], RelAttr attributes[], int attr_num);

  void clear() {
    column_names_.clear();
    contents_.clear();
  }
private:
  void aggr_contents(Value &value, AggrType aggr_type, int index, std::vector<std::vector<Value>> contents);
  int cmp_non_null_value(Value &v1, Value &v2);

  std::vector<std::string> column_names_;
  std::vector<std::vector<Value>> contents_;

};

#endif  // MINIDB_PRINTER_H
