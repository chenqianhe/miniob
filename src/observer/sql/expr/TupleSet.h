//
// Created by root on 10/15/22.
//

#ifndef MINIDB_TUPLESET_H
#define MINIDB_TUPLESET_H
#include "rc.h"
#include "sql/parser/parse_defs.h"
#include <vector>
#include "sql/expr/tuple.h"

class TupleSet {

public:
  TupleSet()=default;
  virtual ~TupleSet(){
    for (ProjectTuple *tuple : tuple_set_) {
      delete tuple;
    }
    tuple_set_.clear();
  }
  void add_tuple(ProjectTuple *tuple){
    tuple_set_.push_back(tuple);
  }
  int size(){
    return tuple_set_.size();
  }
  std::vector<ProjectTuple *> tuples(){
    return tuple_set_;
  }
  ProjectTuple* get(int index){
    return tuple_set_[index];
  }
  int tuple_cell_num(){
    return tuple_set_[0]->cell_num();
  }

private:
  std::vector<ProjectTuple *> tuple_set_;
};

#endif  // MINIDB_TUPLESET_H
