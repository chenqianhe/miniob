//
// Created by root on 10/17/22.
//
#include "sql/operator/pred_mutitable_operator.h"
RC PredMutiOperator::open()
{
  if (children_.size() != 1) {
    LOG_WARN("predicate operator must has one child");
    return RC::INTERNAL;
  }

  return children_[0]->open();
}

RC PredMutiOperator::next()
{

}

RC PredMutiOperator::close()
{
  children_[0]->close();
  return RC::SUCCESS;
}

Tuple * PredMutiOperator::current_tuple()
{
  return children_[0]->current_tuple();
}
bool PredMutiOperator:: do_predicate(RowTuple &tuple)
{
  if (filter_stmt_ == nullptr || filter_stmt_->filter_units().empty()) {
    return true;
  }
  bool filter_result = false;
  for (const FilterUnit *filter_unit : filter_stmt_->filter_units()) {
    if (filter_unit->left()->type() == ExprType::FIELD && filter_unit->right()->type() == ExprType::FIELD){
      continue;
    }
    Expression *left_expr = filter_unit->left();
    Expression *right_expr = filter_unit->right();
    if (left_expr->type() == ExprType::FIELD && right_expr->type() == ExprType::FIELD){
      const char *left_name = ((FieldExpr*)left_expr)->table_name();
      const char *right_name = ((FieldExpr*)right_expr)->table_name();
      if(strcmp(left_name,right_name)==0){
        continue;
      }
    }
    CompOp comp = filter_unit->comp();
    TupleCell left_cell;
    TupleCell right_cell;
    left_expr->get_value(tuple, left_cell);
    right_expr->get_value(tuple, right_cell);

    const int compare = left_cell.compare(right_cell);

    switch (comp) {
      case EQUAL_TO: {
        filter_result = (0 == compare);
      } break;
      case LESS_EQUAL: {
        filter_result = (compare <= 0);
      } break;
      case NOT_EQUAL: {
        filter_result = (compare != 0);
      } break;
      case LESS_THAN: {
        filter_result = (compare < 0);
      } break;
      case GREAT_EQUAL: {
        filter_result = (compare >= 0);
      } break;
      case GREAT_THAN: {
        filter_result = (compare > 0);
      } break;
      default: {
        LOG_WARN("invalid compare type: %d", comp);
      } break;
    }
  }
  return filter_result;
}

void PredMutiOperator::get_result(TupleSet *result)
{
  for(int i = 0;i<tuple_set_->size();i++){
    RowTuple *tuple = (*tuple_set_)[i];
    if(do_predicate(*tuple)){
      result->add_tuple(tuple);
    }
  }
}