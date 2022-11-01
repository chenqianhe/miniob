//
// Created by root on 11/1/22.
//

#ifndef MINIDB_PRINTER_H
#define MINIDB_PRINTER_H
#include <sstream>
#include "vector"
#include "string"
#include "sql/parser/parse_defs.h"
#include "sql/expr/tuple.h"

class Printer {
public:
  Printer()=default;
  ~Printer()=default;

  void insert_column_name(std::string name) { column_names_.emplace_back(name); };
  void expand_rows() { contents_.emplace_back(std::vector<Value>()); }
  void insert_value_from_tuple(const Tuple &tuple);
  void insert_value(Value value) { contents_[contents_.size()-1].emplace_back(value); };
  void print_headers(std::ostream &os);
  void print_contents(std::ostream &os);
  void sort_contents(int order_condition_num, OrderCondition oder_conditions[]);
private:
  std::vector<std::string> column_names_;
  std::vector<std::vector<Value>> contents_;

};

#endif  // MINIDB_PRINTER_H
