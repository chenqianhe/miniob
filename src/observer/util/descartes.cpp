//
// Created by root on 10/16/22.
//
#include "descartes.h"
void descartesRecursive(std::vector<TupleSet*>& originalList,int position,std::vector<TupleSet *>& returnList,TupleSet& line,FilterStmt *filter_stmt)
{
  TupleSet *table = originalList[position];
  int size = table->size();
  for(int i = 0; i < size; i++){
    Tuple *tmp = table->tuples()[i];
    if(!do_predicate_descarte(line,tmp,filter_stmt)){
      LOG_INFO("do_predicate is false");
      continue;
    }
    line.add_tuple(tmp);
    if(position == originalList.size() - 1){
      TupleSet *final = new TupleSet();
      for(int j = 0; j < line.size();j++){
        final->add_tuple(line.tuples()[j]);
      }
      returnList.push_back(final);
      line.remove();
      continue;
    }
    descartesRecursive(originalList,position+1,returnList,line,filter_stmt);
    line.remove();
  }
}

std::vector<TupleSet*> getDescartes(std::vector<TupleSet*>& list,FilterStmt *filter_stmt){
  std::vector<TupleSet*> returnList;
  TupleSet *line = new TupleSet();
  descartesRecursive(list,0,returnList,*line,filter_stmt);
  delete line;
  return returnList;
}

bool do_predicate_descarte(TupleSet &descarte_tuple,Tuple * new_tuple,FilterStmt *filter_stmt){
  if (filter_stmt == nullptr || filter_stmt->filter_units().empty()) {
    return true;
  }
  for (const FilterUnit *filter_unit : filter_stmt->filter_units()) {
    if (filter_unit->left()->type() != ExprType::FIELD || filter_unit->right()->type() != ExprType::FIELD){
      continue;
    }
    Expression *left_expr = filter_unit->left();
    Expression *right_expr = filter_unit->right();
    CompOp comp = filter_unit->comp();
    TupleCell left_cell;
    TupleCell right_cell;
    const char* left_name = static_cast<FieldExpr &>(*left_expr).table_name();
    const char* right_name = static_cast<FieldExpr &>(*right_expr).table_name();

    Tuple *left_tuple = nullptr;
    Tuple *right_tuple = nullptr;

    RowTuple *new_row = static_cast<RowTuple*>(static_cast<ProjectTuple*>(new_tuple)->tuple());
        LOG_INFO("left name is %s ,right name is %s",left_name,right_name);
    if(strcmp(left_name,new_row->table_name())!=0 && strcmp(right_name,new_row->table_name())!=0){
      continue;
    }else if(strcmp(left_name,new_row->table_name())!=0 && strcmp(right_name,new_row->table_name())==0){
      right_tuple = static_cast<ProjectTuple*>(new_tuple)->tuple();
      LOG_INFO("new table is right");
    }else if(strcmp(left_name,new_row->table_name())==0 && strcmp(right_name,new_row->table_name())!=0){
      left_tuple = static_cast<ProjectTuple*>(new_tuple)->tuple();
      LOG_INFO("new table is right");
    }else{

    }

    for(Tuple *tuple:descarte_tuple.tuples()){
      ProjectTuple *tmp_proj = static_cast<ProjectTuple*>(tuple);
      RowTuple *tmp = static_cast<RowTuple*>(static_cast<ProjectTuple*>(tuple)->tuple());
      if(strcmp(left_name,tmp->table_name())==0){
        left_tuple = tmp_proj->tuple();
      } else if(strcmp(right_name,tmp->table_name())==0){
        right_tuple = tmp_proj->tuple();
      }else{
        //        LOG_ERROR("failed to find table");
      }
    }
    if(left_tuple != nullptr && right_tuple != nullptr){
      left_expr->get_value(*left_tuple, left_cell);
      right_expr->get_value(*right_tuple, right_cell);
      int compare = -1;
      bool filter_result = false;
      if (left_cell.attr_type() != NULL_ && right_cell.attr_type() != NULL_) {
        compare = left_cell.compare(right_cell);
        switch (comp) {
          case IS_SAME:
          case EQUAL_TO: {
            filter_result = (0 == compare);
          } break;
          case LESS_EQUAL: {
            filter_result = (compare <= 0);
          } break;
          case IS_NOT_SAME:
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
      } else {
        if (left_cell.attr_type() == NULL_ && right_cell.attr_type() == NULL_) {
          if (comp == IS_SAME) {
            filter_result = true;
          } else {
            filter_result = false;
          }
        } else {
          if (comp == IS_NOT_SAME) {
            filter_result = true;
          } else {
            filter_result = false;
          }
        }
      }
      if (!filter_result) {
        return false;
      }
    }

  }
  return true;
}