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
  virtual ~TupleSet()=default;
  void add_tuple(Tuple *tuple){
    tuple_set_.push_back(tuple);
  }
  int size(){
    return tuple_set_.size();
  }
  std::vector<Tuple *> tuples(){
    return tuple_set_;
  }
  Tuple* get(int index){
    return tuple_set_[index];
  }
//  int tuple_cell_num(){
//    return tuple_set_[0]->tuple_size();
//  }

private:
  std::vector<Tuple *> tuple_set_;
};

#endif  // MINIDB_TUPLESET_H
