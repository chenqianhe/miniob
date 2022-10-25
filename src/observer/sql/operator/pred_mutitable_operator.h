//
// Created by root on 10/17/22.
//

#ifndef MINIDB_PRED_MUTITABLE_OPERATOR_H
#define MINIDB_PRED_MUTITABLE_OPERATOR_H
#pragma once

#include "sql/operator/operator.h"
#include "sql/expr/TupleSet.h"
#include "sql/stmt/filter_stmt.h"

class FilterStmt;

/**
 * 多个表数据过滤
 */
class PredMutiOperator : public Operator
{
public:
  PredMutiOperator(FilterStmt *filter_stmt,TupleSet *tuple_set)
      : filter_stmt_(filter_stmt),tuple_set_(tuple_set)
  {}
  virtual ~PredMutiOperator() = default;

  RC open() override;
  RC next() override;
  RC close() override;
  Tuple * current_tuple() override;
  bool do_predicate(ProjectTuple &tuple);
  void get_result(TupleSet *result);

private:
  FilterStmt *filter_stmt_ = nullptr;
  TupleSet *tuple_set_ = nullptr;
};
#endif  // MINIDB_PRED_MUTITABLE_OPERATOR_H
