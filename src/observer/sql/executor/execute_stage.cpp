/* Copyright (c) 2021 Xie Meiyi(xiemeiyi@hust.edu.cn) and OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

//
// Created by Meiyi & Longda on 2021/4/13.
//

#include <string>
#include <sstream>

#include "execute_stage.h"

#include "common/io/io.h"
#include "common/log/log.h"
#include "common/lang/defer.h"
#include "common/seda/timer_stage.h"
#include "common/lang/string.h"
#include "session/session.h"
#include "event/storage_event.h"
#include "event/sql_event.h"
#include "event/session_event.h"
#include "sql/expr/tuple.h"
#include "sql/operator/table_scan_operator.h"
#include "sql/operator/index_scan_operator.h"
#include "sql/operator/pred_mutitable_operator.h"
#include "sql/operator/predicate_operator.h"
#include "sql/operator/delete_operator.h"
#include "sql/operator/project_operator.h"
#include "sql/operator/update_operator.h"
#include "sql/stmt/stmt.h"
#include "sql/stmt/select_stmt.h"
#include "sql/stmt/update_stmt.h"
#include "sql/stmt/delete_stmt.h"
#include "sql/stmt/insert_stmt.h"
#include "sql/stmt/filter_stmt.h"
#include "storage/common/table.h"
#include "storage/common/field.h"
#include "storage/index/index.h"
#include "storage/default/default_handler.h"
#include "storage/common/condition_filter.h"
#include "storage/trx/trx.h"
#include "sql/expr/TupleSet.h"
#include "../../util/descartes.h"
#include "printer.h"

using namespace common;

//RC create_selection_executor(
//   Trx *trx, const Selects &selects, const char *db, const char *table_name, SelectExeNode &select_node);

//! Constructor
ExecuteStage::ExecuteStage(const char *tag) : Stage(tag)
{}

//! Destructor
ExecuteStage::~ExecuteStage()
{}

//! Parse properties, instantiate a stage object
Stage *ExecuteStage::make_stage(const std::string &tag)
{
  ExecuteStage *stage = new (std::nothrow) ExecuteStage(tag.c_str());
  if (stage == nullptr) {
    LOG_ERROR("new ExecuteStage failed");
    return nullptr;
  }
  stage->set_properties();
  return stage;
}

//! Set properties for this object set in stage specific properties
bool ExecuteStage::set_properties()
{
  //  std::string stageNameStr(stageName);
  //  std::map<std::string, std::string> section = theGlobalProperties()->get(
  //    stageNameStr);
  //
  //  std::map<std::string, std::string>::iterator it;
  //
  //  std::string key;

  return true;
}

//! Initialize stage params and validate outputs
bool ExecuteStage::initialize()
{
  LOG_TRACE("Enter");

  std::list<Stage *>::iterator stgp = next_stage_list_.begin();
  default_storage_stage_ = *(stgp++);
  mem_storage_stage_ = *(stgp++);

  LOG_TRACE("Exit");
  return true;
}

//! Cleanup after disconnection
void ExecuteStage::cleanup()
{
  LOG_TRACE("Enter");

  LOG_TRACE("Exit");
}

void ExecuteStage::handle_event(StageEvent *event)
{
  LOG_TRACE("Enter\n");

  handle_request(event);

  LOG_TRACE("Exit\n");
  return;
}

void ExecuteStage::callback_event(StageEvent *event, CallbackContext *context)
{
  LOG_TRACE("Enter\n");

  // here finish read all data from disk or network, but do nothing here.

  LOG_TRACE("Exit\n");
  return;
}

void ExecuteStage::handle_request(common::StageEvent *event)
{
  SQLStageEvent *sql_event = static_cast<SQLStageEvent *>(event);
  SessionEvent *session_event = sql_event->session_event();
  Stmt *stmt = sql_event->stmt();
  Session *session = session_event->session();
  Query *sql = sql_event->query();

  if (stmt != nullptr) {
    switch (stmt->type()) {
    case StmtType::SELECT: {
       do_select(sql_event);
    } break;
    case StmtType::INSERT: {
      do_insert(sql_event);
    } break;
    case StmtType::UPDATE: {
      do_update(sql_event);
    } break;
    case StmtType::DELETE: {
      do_delete(sql_event);
    } break;
    }
  } else {
    switch (sql->flag) {
    case SCF_HELP: {
      do_help(sql_event);
    } break;
    case SCF_CREATE_TABLE: {
      do_create_table(sql_event);
    } break;
    case SCF_CREATE_INDEX: {
      do_create_index(sql_event);
    } break;
    case SCF_CREATE_UNIQUE_INDEX: {
      do_create_unique_index(sql_event);
    } break;
    case SCF_SHOW_TABLES: {
      do_show_tables(sql_event);
    } break;
    case SCF_DESC_TABLE: {
      do_desc_table(sql_event);
    } break;

    case SCF_DROP_TABLE: {
      do_drop_table(sql_event);
    } break;
    case SCF_DROP_INDEX:
    case SCF_LOAD_DATA: {
      default_storage_stage_->handle_event(event);
    } break;
    case SCF_SYNC: {
      RC rc = DefaultHandler::get_default().sync();
      session_event->set_response(strrc(rc));
    } break;
    case SCF_BEGIN: {
      session_event->set_response("SUCCESS\n");
    } break;
    case SCF_COMMIT: {
      Trx *trx = session->current_trx();
      RC rc = trx->commit();
      session->set_trx_multi_operation_mode(false);
      session_event->set_response(strrc(rc));
    } break;
    case SCF_ROLLBACK: {
      Trx *trx = session_event->get_client()->session->current_trx();
      RC rc = trx->rollback();
      session->set_trx_multi_operation_mode(false);
      session_event->set_response(strrc(rc));
    } break;
    case SCF_EXIT: {
      // do nothing
      const char *response = "Unsupported\n";
      session_event->set_response(response);
    } break;
    default: {
      LOG_ERROR("Unsupported command=%d\n", sql->flag);
    }
    }
  }
}

void end_trx_if_need(Session *session, Trx *trx, bool all_right)
{
  if (!session->is_trx_multi_operation_mode()) {
    if (all_right) {
      trx->commit();
    } else {
      trx->rollback();
    }
  }
}

void print_tuple_header(std::ostream &os, const ProjectOperator &oper)
{
  // null_tag不展示
  const int show_cell_num = oper.tuple_cell_num() - 1;
  const TupleCellSpec *cell_spec = nullptr;
  for (int i = 0; i < show_cell_num; i++) {
    oper.tuple_cell_spec_at(i, cell_spec);
    if (i != 0) {
      os << " | ";
    }

    if (cell_spec->alias()) {
      os << cell_spec->alias();
    }
  }

  if (show_cell_num > 0) {
    os << '\n';
  }
}

void print_project_tuple_header(std::ostream &os, const ProjectTuple &tuple)
{
  const int show_cell_num = tuple.cell_num() - 1;
  const TupleCellSpec *cell_spec = nullptr;
  for (int i = 0; i < show_cell_num; i++) {
    tuple.cell_spec_at(i, cell_spec);
    if (i != 0) {
      os << " | ";
    }

    if (cell_spec->alias()) {
      os << cell_spec->alias();
    }
  }

  if (show_cell_num > 0) {
    os << '\n';
  }
}



void print_aggr(std::ostream &os, std::vector<std::string> content)
{
  for (int i = 0; i < content.size(); i++) {
    if (i != 0) {
      os << " | ";
    }
    os << content[i];
  }
  os << '\n';
}

void tuple_to_string(std::ostream &os, const Tuple &tuple)
{
  TupleCell cell;
  TupleCell null_tag_cell;
  RC rc = tuple.cell_at(tuple.cell_num()-1, null_tag_cell);
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to fetch field of null tag cell. index=%d, rc=%s", tuple.cell_num()-1, strrc(rc));
    return ;
  }
  std::bitset<sizeof(int) * 8> null_tag_bit = NullTag::convert_null_tag_bitset(null_tag_cell.null_tag_to_int());
  bool first_field = true;
  for (int i = 0; i < tuple.cell_num()-1; i++) {
    rc = tuple.cell_at(i, cell);
    if (rc != RC::SUCCESS) {
      LOG_WARN("failed to fetch field of cell. index=%d, rc=%s", i, strrc(rc));
      break;
    }

    if (!first_field) {
      os << " | ";
    } else {
      first_field = false;
    }
    cell.to_string(os, null_tag_bit[i]);
  }
}

void print_descarte_tuple_header(std::ostream &os, const ProjectTuple &tuple)
{
  const int show_cell_num = tuple.cell_num() - 1;
  const TupleCellSpec *cell_spec = nullptr;
  for (int i = 0; i < show_cell_num; i++) {
    tuple.cell_spec_at(i, cell_spec);
    if (i != 0) {
      os << " | ";
    }

    if (cell_spec->alias()) {
      os << cell_spec->alias();
    }
  }
}

void print_descartes_tuple_header(std::ostream &os,TupleSet &tupleset){
  for(int i = 0;i < tupleset.size();i++){
    Tuple * tuple = tupleset.tuples()[i];
    print_descarte_tuple_header(os,static_cast<ProjectTuple&>(*tuple));
    if(i != tupleset.size() - 1 && tuple->cell_num() > 1){
      os << " | ";
    }
  }
  if(tupleset.size() > 0){
    os<<'\n';
  }
}
void print_descartes_tuple(std::ostream &os,TupleSet &tupleset){
  for(int i = 0;i < tupleset.size();i++){
    Tuple * tuple = tupleset.tuples()[i];
    tuple_to_string(os,*tuple);
    if(i != tupleset.size() - 1 && tuple->cell_num() > 1){
      os << " | ";
    }
  }
}

IndexScanOperator *try_to_create_index_scan_operator(FilterStmt *filter_stmt)
{
  const std::vector<FilterUnit *> &filter_units = filter_stmt->filter_units();
  if (filter_units.empty() ) {
    return nullptr;
  }

  // 在所有过滤条件中，找到字段与值做比较的条件，然后判断字段是否可以使用索引
  // 如果是多列索引，这里的处理需要更复杂。
  // 这里的查找规则是比较简单的，就是尽量找到使用相等比较的索引
  // 如果没有就找范围比较的，但是直接排除不等比较的索引查询. (你知道为什么?)
  const FilterUnit *better_filter = nullptr;
  for (const FilterUnit * filter_unit : filter_units) {
    if (filter_unit->comp() == NOT_EQUAL) {
      continue;
    }

    Expression *left = filter_unit->left();
    Expression *right = filter_unit->right();
    if (left->type() == ExprType::FIELD && right->type() == ExprType::VALUE) {
    } else if (left->type() == ExprType::VALUE && right->type() == ExprType::FIELD) {
      std::swap(left, right);
    } else if (left->type() == ExprType::VALUE && right->type() == ExprType::VALUE) {
      continue;
    }
    FieldExpr &left_field_expr = *(FieldExpr *)left;
    const Field &field = left_field_expr.field();
    const Table *table = field.table();
    Index *index = table->find_index_by_field(field.field_name());
    if (index != nullptr) {
      if (better_filter == nullptr) {
        better_filter = filter_unit;
      } else if (filter_unit->comp() == EQUAL_TO) {
        better_filter = filter_unit;
    	break;
      }
    }
  }

  if (better_filter == nullptr) {
    return nullptr;
  }

  Expression *left = better_filter->left();
  Expression *right = better_filter->right();
  CompOp comp = better_filter->comp();
  if (left->type() == ExprType::VALUE && right->type() == ExprType::FIELD) {
    std::swap(left, right);
    switch (comp) {
    case EQUAL_TO:    { comp = EQUAL_TO; }    break;
    case LESS_EQUAL:  { comp = GREAT_THAN; }  break;
    case NOT_EQUAL:   { comp = NOT_EQUAL; }   break;
    case LESS_THAN:   { comp = GREAT_EQUAL; } break;
    case GREAT_EQUAL: { comp = LESS_THAN; }   break;
    case GREAT_THAN:  { comp = LESS_EQUAL; }  break;
    default: {
    	LOG_WARN("should not happen");
    }
    }
  }


  FieldExpr &left_field_expr = *(FieldExpr *)left;
  const Field &field = left_field_expr.field();
  const Table *table = field.table();
  Index *index = table->find_index_by_field(field.field_name());
  assert(index != nullptr);

  ValueExpr &right_value_expr = *(ValueExpr *)right;
  TupleCell value;
  right_value_expr.get_tuple_cell(value);

  const TupleCell *left_cell = nullptr;
  const TupleCell *right_cell = nullptr;
  bool left_inclusive = false;
  bool right_inclusive = false;

  switch (comp) {
  case EQUAL_TO: {
    left_cell = &value;
    right_cell = &value;
    left_inclusive = true;
    right_inclusive = true;
  } break;

  case LESS_EQUAL: {
    left_cell = nullptr;
    left_inclusive = false;
    right_cell = &value;
    right_inclusive = true;
  } break;

  case LESS_THAN: {
    left_cell = nullptr;
    left_inclusive = false;
    right_cell = &value;
    right_inclusive = false;
  } break;

  case GREAT_EQUAL: {
    left_cell = &value;
    left_inclusive = true;
    right_cell = nullptr;
    right_inclusive = false;
  } break;

  case GREAT_THAN: {
    left_cell = &value;
    left_inclusive = false;
    right_cell = nullptr;
    right_inclusive = false;
  } break;

  default: {
    LOG_WARN("should not happen. comp=%d", comp);
  } break;
  }

  IndexScanOperator *oper = new IndexScanOperator(table, index,
       left_cell, left_inclusive, right_cell, right_inclusive);

  LOG_INFO("use index for scan: %s in table %s", index->index_meta().name(), table->name());
  return oper;
}

IndexScanOperator *try_to_create_index_scan_operator_with_table(FilterStmt *filter_stmt,Table *default_table)
{
  const std::vector<FilterUnit *> &filter_units = filter_stmt->filter_units();
  if (filter_units.empty() ) {
    return nullptr;
  }

  // 在所有过滤条件中，找到字段与值做比较的条件，然后判断字段是否可以使用索引
  // 如果是多列索引，这里的处理需要更复杂。
  // 这里的查找规则是比较简单的，就是尽量找到使用相等比较的索引
  // 如果没有就找范围比较的，但是直接排除不等比较的索引查询. (你知道为什么?)
  const FilterUnit *better_filter = nullptr;
  for (const FilterUnit * filter_unit : filter_units) {
    if (filter_unit->comp() == NOT_EQUAL) {
      continue;
    }

    Expression *left = filter_unit->left();
    Expression *right = filter_unit->right();
    if (left->type() == ExprType::FIELD && right->type() == ExprType::FIELD){
      continue;
    }
    if (left->type() == ExprType::FIELD && right->type() == ExprType::VALUE) {
    } else if (left->type() == ExprType::VALUE && right->type() == ExprType::FIELD) {
      std::swap(left, right);
    }
    FieldExpr &left_field_expr = *(FieldExpr *)left;
    const Field &field = left_field_expr.field();
    const Table *table = field.table();
    if(strcmp(default_table->name(),table->name())!=0){
      continue;
    }
    Index *index = table->find_index_by_field(field.field_name());
    if (index != nullptr) {
      if (better_filter == nullptr) {
        better_filter = filter_unit;
      } else if (filter_unit->comp() == EQUAL_TO) {
        better_filter = filter_unit;
        break;
      }
    }
  }

  if (better_filter == nullptr) {
    return nullptr;
  }

  Expression *left = better_filter->left();
  Expression *right = better_filter->right();
  CompOp comp = better_filter->comp();
  if (left->type() == ExprType::VALUE && right->type() == ExprType::FIELD) {
    std::swap(left, right);
    switch (comp) {
      case EQUAL_TO:    { comp = EQUAL_TO; }    break;
      case LESS_EQUAL:  { comp = GREAT_THAN; }  break;
      case NOT_EQUAL:   { comp = NOT_EQUAL; }   break;
      case LESS_THAN:   { comp = GREAT_EQUAL; } break;
      case GREAT_EQUAL: { comp = LESS_THAN; }   break;
      case GREAT_THAN:  { comp = LESS_EQUAL; }  break;
      default: {
        LOG_WARN("should not happen");
      }
    }
  }


  FieldExpr &left_field_expr = *(FieldExpr *)left;
  const Field &field = left_field_expr.field();
  const Table *table = field.table();
  Index *index = table->find_index_by_field(field.field_name());
  assert(index != nullptr);

  ValueExpr &right_value_expr = *(ValueExpr *)right;
  TupleCell value;
  right_value_expr.get_tuple_cell(value);

  const TupleCell *left_cell = nullptr;
  const TupleCell *right_cell = nullptr;
  bool left_inclusive = false;
  bool right_inclusive = false;

  switch (comp) {
    case EQUAL_TO: {
      left_cell = &value;
      right_cell = &value;
      left_inclusive = true;
      right_inclusive = true;
    } break;

    case LESS_EQUAL: {
      left_cell = nullptr;
      left_inclusive = false;
      right_cell = &value;
      right_inclusive = true;
    } break;

    case LESS_THAN: {
      left_cell = nullptr;
      left_inclusive = false;
      right_cell = &value;
      right_inclusive = false;
    } break;

    case GREAT_EQUAL: {
      left_cell = &value;
      left_inclusive = true;
      right_cell = nullptr;
      right_inclusive = false;
    } break;

    case GREAT_THAN: {
      left_cell = &value;
      left_inclusive = false;
      right_cell = nullptr;
      right_inclusive = false;
    } break;

    default: {
      LOG_WARN("should not happen. comp=%d", comp);
    } break;
  }

  IndexScanOperator *oper = new IndexScanOperator(table, index,
      left_cell, left_inclusive, right_cell, right_inclusive);

  LOG_INFO("use index for scan: %s in table %s", index->index_meta().name(), table->name());
  return oper;
}

bool do_predicate(TupleSet &descarte_tuple,FilterStmt *filter_stmt){
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
//    LOG_INFO("left name is %s ,right name is %s",left_name,right_name);
    Tuple *left_tuple = nullptr;
    Tuple *right_tuple = nullptr;
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
    }
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
  return true;
}

std::string ExecuteStage::format(double raw_data, bool is_date)
{
  if (!is_date) {
    int temp = raw_data;
    if (temp == raw_data) {
      return std::to_string(temp);
    } else {
      char buf[MAX_NUM] = {0};
      snprintf(buf,sizeof(buf),"%.1lf", raw_data);
      return std::string(buf);
    }
  } else {
    char buf[16] = {0};
    int data_ = raw_data;
    snprintf(buf,sizeof(buf),"%04d-%02d-%02d", data_/10000, (data_%10000)/100, data_%100);
    return std::string(buf);
  }
}

RC ExecuteStage::do_select(SQLStageEvent *sql_event)
{
  Printer printer = Printer();
  SelectStmt *select_stmt = (SelectStmt *)(sql_event->stmt());
  SessionEvent *session_event = sql_event->session_event();
  bool group_by = select_stmt->group_condition_num() > 0;
  bool order_by = select_stmt->order_condition_num() > 0;
  bool aggr_mode = select_stmt->aggr_attribute_num() > 0;
  RC rc = RC::SUCCESS;

  if (select_stmt->tables().size() > 1) {
    //对每个表进行表内条件过滤，得到每个表差寻出来的结果（一个tupleset），存储到tuple_sets
    std::vector<TupleSet*> tuple_sets;
    std::vector<Operator*> scan_opers;
    std::vector<PredicateOperator*> pred_opers;
    std::vector<ProjectOperator*> proj_opers;
    std::vector<std::string *> alias_set;
    for(int index = select_stmt->tables().size() - 1;index >= 0;index--){
//    for(int index = 0;index < select_stmt->tables().size();index++){
      Table *table = select_stmt->tables()[index];
      Operator *scan_oper = try_to_create_index_scan_operator_with_table(select_stmt->filter_stmt(),table);
      if (nullptr == scan_oper) {
        scan_oper = new TableScanOperator(table);
      }
      scan_opers.push_back(scan_oper);
//      DEFER([&] () {delete scan_oper;});
      PredicateOperator *pred_oper = new PredicateOperator(select_stmt->filter_stmt());
      pred_oper->add_child(scan_oper);
      pred_opers.push_back(pred_oper);
      ProjectOperator *project_oper = new ProjectOperator();
      project_oper->add_child(pred_oper);
      for (const Field &field : select_stmt->query_fields()) {
        if(strcmp(field.table_name(),table->name())!=0){
          continue;
        }
        project_oper->add_projection(field.table(), field.meta());
      }
      proj_opers.push_back(project_oper);
      rc = project_oper->open();
      if (rc != RC::SUCCESS) {
        LOG_WARN("failed to open operator");
        return rc;
      }

      TupleSet *tuple_set = new TupleSet();
      while ((rc = project_oper->next()) == RC::SUCCESS) {
        // get current record
        // write to response
        Tuple * tuple = project_oper->current_tuple();
        if (nullptr == tuple) {
          rc = RC::INTERNAL;
          LOG_WARN("failed to get current record. rc=%s", strrc(rc));
          break;
        }
        //拿到深拷贝的tuple

        ProjectTuple *tmp_proj = static_cast<ProjectTuple*>(tuple);
        RowTuple *tmp_row = static_cast<RowTuple*>(tmp_proj->tuple());
        Record tmp_record = tmp_row->record();
        Record *record = new Record();
        const char* data = tmp_record.data();
        const RID rid = tmp_record.rid();
        record->set_data((char*)data);
        record->set_rid(rid);
        RowTuple *rtuple = new RowTuple();
        rtuple->set_record(record);
        rtuple->set_schema(table,table->table_meta().field_metas());
        ProjectTuple *ptuple = new ProjectTuple();
        ptuple->set_tuple(rtuple);
        for (const Field &field : select_stmt->query_fields()) {
          if(strcmp(field.table_name(),table->name())!=0){
            continue;
          }
          TupleCellSpec *spec = new TupleCellSpec(new FieldExpr(field.table(), field.meta()));
          std::string *alias = new std::string();
          alias->append(field.table_name());
          alias->append(".");
          alias->append(field.field_name());
          spec->set_alias(alias->c_str());
          alias_set.push_back(alias);
//          LOG_INFO("field is %s,size is %d",alias->c_str(),alias->length());
          ptuple->add_cell_spec(spec);
        }
        tuple_set->add_tuple(ptuple);
      }
      tuple_sets.push_back(tuple_set);
    }


    bool is_empty = false;
    for(TupleSet *tuple_set:tuple_sets){
      if(tuple_set->size() == 0){
        is_empty = true;
      }
    }

    std::stringstream ss;
    for (int i = 0;i < select_stmt->query_fields().size() - select_stmt->tables().size();i++) {
      const Field &field = select_stmt->query_fields()[i];
      std::string alias = field.table_name();
      alias.append(".");
      alias.append(field.field_name());
      printer.insert_column_name(alias);
    }


    if(!is_empty){
      //对得到的tuple_sets求笛卡尔积
      LOG_INFO("try to get descartesSet");
      std::vector<TupleSet*> descartesSet = getDescartes(tuple_sets,select_stmt->filter_stmt());
      LOG_INFO("get descartesSet,size is %d",descartesSet.size());
      //处理表间查询
      for(TupleSet *tupleset:descartesSet){
//        if(do_predicate(*tupleset,select_stmt->filter_stmt())){
//          printer.expand_rows();
//          printer.insert_value_by_column_name(*tupleset);
//        }
        printer.expand_rows();
        printer.insert_value_by_column_name(*tupleset);
      }
      if (order_by) {
        printer.sort_contents(select_stmt->order_condition_num(), select_stmt->get_order_conditions());
      }
    }
    if (group_by) {
      printer.group_contents(select_stmt->group_condition_num(), select_stmt->get_group_conditions(), select_stmt->attributes(), select_stmt->aggr_attribute_num());
    }
    printer.print_headers(ss);
    printer.print_contents(ss);
    session_event->set_response(ss.str());


    //内存释放
    for(Operator *scan_oper:scan_opers){
      delete scan_oper;
      scan_oper = nullptr;
    }
    for(PredicateOperator *pred_oper:pred_opers){
      delete pred_oper;
      pred_oper = nullptr;
    }
    for(ProjectOperator *proj_oper:proj_opers){
      delete proj_oper;
      proj_oper = nullptr;
    }
    return rc;
  }

  Operator *scan_oper = try_to_create_index_scan_operator(select_stmt->filter_stmt());
  if (nullptr == scan_oper) {
    scan_oper = new TableScanOperator(select_stmt->tables()[0]);
  }
  DEFER([&] () {delete scan_oper;});

  // count, avg/idx, min, max
  std::vector<std::vector<double>> count;
  std::vector<std::vector<std::string>> count_char;
  std::vector<std::vector<bool>> count_null;

  PredicateOperator pred_oper(select_stmt->filter_stmt());
  pred_oper.add_child(scan_oper);
  ProjectOperator project_oper;
  project_oper.add_child(&pred_oper);
  for (const Field &field : select_stmt->query_fields()) {
    if (field.meta()->name() != std::string(NullTag::null_tag_field_name())) {
      printer.insert_column_name(field.meta()->name());
    }
    project_oper.add_projection(field.table(), field.meta());
    count.emplace_back(std::vector<double>(2, 0));
    count_null.emplace_back(std::vector<bool>(4, false));
    count[count.size()-1].emplace_back(std::numeric_limits<float>::max());
    count[count.size()-1].emplace_back(std::numeric_limits<float>::lowest());
    if(field.meta()->type() == CHARS) {
      count_char.emplace_back(std::vector<std::string>(2));
    }
  }

  rc = project_oper.open();
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to open operator");
    return rc;
  }

  std::stringstream ss;
  if (aggr_mode && !group_by) {
    printer.clear();
    printer.expand_rows();
    while ((rc = project_oper.next()) == RC::SUCCESS) {
      // get current record
      Tuple * tuple = project_oper.current_tuple();
      if (nullptr == tuple) {
        rc = RC::INTERNAL;
        LOG_WARN("failed to get current record. rc=%s", strrc(rc));
        return rc;
      }
      TupleCell cell;
      bool first_field = true;
      int count_char_index = 0;
      // 不统计 __null_tag
      for (int i = 0; i < tuple->cell_num()-1; i++) {
        rc = tuple->cell_at(i, cell);
        if (rc != RC::SUCCESS) {
          LOG_WARN("failed to fetch field of cell. index=%d, rc=%s", i, strrc(rc));
          return rc;
        }
        switch (cell.attr_type()) {
          case CHARS:{
            count[i][0] ++;
            count[i][1] = count_char_index;
            if (count_char[count_char_index][0].empty() || count_char[count_char_index][1].empty()) {
              count_char[count_char_index][0] = std::string(cell.data());
              count_char[count_char_index][1] = std::string(cell.data());
            } else {
              if (std::string(cell.data()) < count_char[count_char_index][0]) {
                count_char[count_char_index][0] = std::string(cell.data());
              }
              if (std::string(cell.data()) > count_char[count_char_index][0]) {
                count_char[count_char_index][1] = std::string(cell.data());
              }
            }
            count_char_index ++;
          }
            break;
          case INTS:{
            count[i][0] ++;
            count[i][1] += *(int*)cell.data();
            count[i][2] = *(int*)cell.data() < count[i][2] ? *(int*)cell.data() : count[i][2];
            count[i][3] = *(int*)cell.data() > count[i][3] ? *(int*)cell.data() : count[i][3];
          }
            break;
          case FLOATS:{
            count[i][0] ++;
            count[i][1] += *(float*)cell.data();
            count[i][2] = *(float*)cell.data() < count[i][2] ? *(float*)cell.data() : count[i][2];
            count[i][3] = *(float*)cell.data() > count[i][3] ? *(float*)cell.data() : count[i][3];
          }
            break;
          case DATES:{
            count[i][0] ++;
            count[i][2] = *(int*)cell.data() < count[i][2] ? *(int*)cell.data() : count[i][2];
            count[i][3] = *(int*)cell.data() > count[i][3] ? *(int*)cell.data() : count[i][3];
          }
            break;
          case NULL_: {
            count_null[i][2] = true;
            count_null[i][3] = true;
          }
            break;
          case UNDEFINED:
            break;
        }
        count_char_index = 0;
      }
    }

    std::vector<std::string> names;
    for (int i = 0; i < select_stmt->aggr_attribute_num(); ++i) {
      RelAttr relation_attr = select_stmt->get_aggr_attribute(i);
      Value value;
      switch (relation_attr.aggr_type) {
        case Count: {
          names.emplace_back("COUNT");
          if (!strcmp(relation_attr.attribute_name, "*")) {
            int count_max = 0;
            for(auto c: count) {
              if (c[0] > count_max) {
                count_max = (int)c[0];
              }
            }
            value_init_integer(&value, count_max);
          } else {
            value_init_integer(&value, count[i][0]);
          }
        } break;
        case Avg: {
          names.emplace_back("AVG");
          if (count[i][0] == 0) {
            value_init_null(&value);
          } else {
            value_init_float(&value, count[i][1] / count[i][0]);
          }
        } break;
        case Max: {
          names.emplace_back("MAX");
          if (count_null[i][3]) {
            value_init_null(&value);
          } else {
            if (select_stmt->query_fields()[i].meta()->type() == CHARS) {
              value_init_string(&value, count_char[count[i][1]][1].c_str());
            } else {
              if (select_stmt->query_fields()[i].meta()->type() == FLOATS) {
                value_init_float(&value, count[i][3]);
              } else {
                value_init_integer(&value, count[i][3]);
              }
            }
          }
        } break;
        case Min: {
          names.emplace_back("MIN");
          if (count_null[i][2]) {
            value_init_null(&value);
          } else {
            if (select_stmt->query_fields()[i].meta()->type() == CHARS) {
              value_init_string(&value, count_char[count[i][1]][0].c_str());
            } else {
              if (select_stmt->query_fields()[i].meta()->type() == FLOATS) {
                value_init_float(&value, count[i][2]);
              } else {
                value_init_integer(&value, count[i][2]);
              }
            }
          }
        } break;
        case None:
          break;
      }
      names[names.size()-1] += "(";
      if (relation_attr.relation_name) {
        names[names.size()-1] += relation_attr.relation_name;
        names[names.size()-1] += ".";
      }
      names[names.size()-1] += relation_attr.attribute_name;
      names[names.size()-1] += ")";
      printer.insert_value(value);
    }
    printer.set_column_names(names);
  } else {
    while ((rc = project_oper.next()) == RC::SUCCESS) {
      printer.expand_rows();
      // get current record
      // write to response
      Tuple * tuple = project_oper.current_tuple();
      if (nullptr == tuple) {
        rc = RC::INTERNAL;
        LOG_WARN("failed to get current record. rc=%s", strrc(rc));
        break;
      }
      printer.insert_value_from_tuple(*tuple);
    }
  }
  if (order_by) {
    printer.sort_contents(select_stmt->order_condition_num(), select_stmt->get_order_conditions());
  }
  if (group_by) {
    printer.group_contents(select_stmt->group_condition_num(), select_stmt->get_group_conditions(), select_stmt->attributes(), select_stmt->aggr_attribute_num());
  }
  printer.print_headers(ss);
  printer.print_contents(ss);
  if (rc != RC::RECORD_EOF) {
    LOG_WARN("something wrong while iterate operator. rc=%s", strrc(rc));
    project_oper.close();
  } else {
    rc = project_oper.close();
  }
  session_event->set_response(ss.str());
  return rc;
}

RC ExecuteStage::do_help(SQLStageEvent *sql_event)
{
  SessionEvent *session_event = sql_event->session_event();
  const char *response = "show tables;\n"
                         "desc `table name`;\n"
                         "create table `table name` (`column name` `column type`, ...);\n"
                         "create index `index name` on `table` (`column`);\n"
                         "insert into `table` values(`value1`,`value2`);\n"
                         "update `table` set column=value [where `column`=`value`];\n"
                         "delete from `table` [where `column`=`value`];\n"
                         "select [ * | `columns` ] from `table`;\n";
  session_event->set_response(response);
  return RC::SUCCESS;
}

RC ExecuteStage::do_create_table(SQLStageEvent *sql_event)
{
  const CreateTable &create_table = sql_event->query()->sstr.create_table;
  SessionEvent *session_event = sql_event->session_event();
  Db *db = session_event->session()->get_current_db();
  RC rc = db->create_table(create_table.relation_name,
			create_table.attribute_count, create_table.attributes);
  if (rc == RC::SUCCESS) {
    session_event->set_response("SUCCESS\n");
  } else {
    session_event->set_response("FAILURE\n");
  }
  return rc;
}

RC ExecuteStage::do_drop_table(SQLStageEvent *sql_event)
{
  const DropTable &drop_table = sql_event->query()->sstr.drop_table;
  SessionEvent *session_event = sql_event->session_event();
  Db *db = session_event->session()->get_current_db();
  RC rc = db->drop_table(drop_table.relation_name);
  if (rc == RC::SUCCESS) {
    session_event->set_response("SUCCESS\n");
  } else {
    session_event->set_response("FAILURE\n");
  }
  return rc;
}

RC ExecuteStage::do_create_index(SQLStageEvent *sql_event)
{
  SessionEvent *session_event = sql_event->session_event();
  Db *db = session_event->session()->get_current_db();
  const CreateIndex &create_index = sql_event->query()->sstr.create_index;
  Table *table = db->find_table(create_index.relation_name);
  if (nullptr == table) {
    session_event->set_response("FAILURE\n");
    return RC::SCHEMA_TABLE_NOT_EXIST;
  }

  RC rc = table->create_index(nullptr, create_index.index_name, create_index.attribute_name);
  sql_event->session_event()->set_response(rc == RC::SUCCESS ? "SUCCESS\n" : "FAILURE\n");
  return rc;
}

RC ExecuteStage::do_create_unique_index(SQLStageEvent *sql_event) {
  SessionEvent *session_event = sql_event->session_event();
  Db *db = session_event->session()->get_current_db();
  const CreateIndex &create_index = sql_event->query()->sstr.create_index;
  Table *table = db->find_table(create_index.relation_name);
  if (nullptr == table) {
    session_event->set_response("FAILURE\n");
    return RC::SCHEMA_TABLE_NOT_EXIST;
  }

  RC rc = table->create_index(nullptr, create_index.index_name, create_index.attribute_name, 1);
  sql_event->session_event()->set_response(rc == RC::SUCCESS ? "SUCCESS\n" : "FAILURE\n");
  return rc;
}

RC ExecuteStage::do_show_tables(SQLStageEvent *sql_event)
{
  SessionEvent *session_event = sql_event->session_event();
  Db *db = session_event->session()->get_current_db();
  std::vector<std::string> all_tables;
  db->all_tables(all_tables);
  if (all_tables.empty()) {
    session_event->set_response("No table\n");
  } else {
    std::stringstream ss;
    for (const auto &table : all_tables) {
      ss << table << std::endl;
    }
    session_event->set_response(ss.str().c_str());
  }
  return RC::SUCCESS;
}

RC ExecuteStage::do_desc_table(SQLStageEvent *sql_event)
{
  Query *query = sql_event->query();
  Db *db = sql_event->session_event()->session()->get_current_db();
  const char *table_name = query->sstr.desc_table.relation_name;
  Table *table = db->find_table(table_name);
  std::stringstream ss;
  if (table != nullptr) {
    table->table_meta().desc(ss);
  } else {
    ss << "No such table: " << table_name << std::endl;
  }
  sql_event->session_event()->set_response(ss.str().c_str());
  return RC::SUCCESS;
}

RC ExecuteStage::do_insert(SQLStageEvent *sql_event)
{
  Stmt *stmt = sql_event->stmt();
  SessionEvent *session_event = sql_event->session_event();

  if (stmt == nullptr) {
    LOG_WARN("cannot find statement");
    return RC::GENERIC_ERROR;
  }

  InsertStmt *insert_stmt = (InsertStmt *)stmt;
  const int values_group_num = insert_stmt->values_group_num();
  const int value_amount = insert_stmt->value_amount();

  Table *table = insert_stmt->table();
  Value *temp_values = (Value*)malloc(sizeof(Value) * value_amount);
  Trx temp_trx[values_group_num];
  Record temp_record[values_group_num];
  RC rc = RC::SUCCESS;
  int value_group_index;
  for (value_group_index=0; value_group_index < values_group_num; value_group_index++) {
    temp_trx[value_group_index] = *new Trx();
    temp_record[value_group_index] = *new Record();
    for (int i = 0; i < value_amount; ++i) {
      temp_values[i].type = insert_stmt->values()[value_group_index*value_amount+i].type;
      temp_values[i].data = insert_stmt->values()[value_group_index*value_amount+i].data;
    }
    rc = insert_record(table, &temp_trx[value_group_index], &temp_record[value_group_index], value_amount, temp_values);
    if (rc != RC::SUCCESS) {
      break;
    }
  }
  if (rc == RC::SUCCESS) {
    session_event->set_response("SUCCESS\n");
  } else {
    for (int i = 0; i < value_group_index; ++i) {
      table->rollback_insert(&temp_trx[i], temp_record[i].rid());
    }
    session_event->set_response("FAILURE\n");
  }
  free(temp_values);
  return rc;
}

RC ExecuteStage::insert_record(Table *table, Trx *trx, Record *record, int value_num, const Value *values)
{
  RC rc = table->check_unique_index_record(value_num, values);
  if (rc != RC::SUCCESS) {
    return rc;
  }
  rc = table->insert_record(trx, record, value_num, values);
  return rc;
}

RC ExecuteStage::do_delete(SQLStageEvent *sql_event)
{
  Stmt *stmt = sql_event->stmt();
  SessionEvent *session_event = sql_event->session_event();

  if (stmt == nullptr) {
    LOG_WARN("cannot find statement");
    return RC::GENERIC_ERROR;
  }

  DeleteStmt *delete_stmt = (DeleteStmt *)stmt;
  TableScanOperator scan_oper(delete_stmt->table());
  PredicateOperator pred_oper(delete_stmt->filter_stmt());
  pred_oper.add_child(&scan_oper);
  DeleteOperator delete_oper(delete_stmt);
  delete_oper.add_child(&pred_oper);

  RC rc = delete_oper.open();
  if (rc != RC::SUCCESS) {
    session_event->set_response("FAILURE\n");
  } else {
    session_event->set_response("SUCCESS\n");
  }
  return rc;
}

RC ExecuteStage::do_update(SQLStageEvent *sql_event)
{
  Stmt *stmt = sql_event->stmt();
  SessionEvent *session_event = sql_event->session_event();

  if (stmt == nullptr) {
    LOG_WARN("cannot find statement");
    return RC::GENERIC_ERROR;
  }

  UpdateStmt *update_stmt = (UpdateStmt *)stmt;
  TableScanOperator scan_oper(update_stmt->table());
  PredicateOperator pred_oper(update_stmt->filter_stmt());
  pred_oper.add_child(&scan_oper);
  UpdateOperator update_oper(update_stmt);
  update_oper.add_child(&pred_oper);

  RC rc = update_oper.open();
//  while((rc = update_oper.next())== RC::SUCCESS){
//
//  };


  if (rc == RC::SUCCESS) {
    session_event->set_response("SUCCESS\n");
  } else {
    session_event->set_response("FAILURE\n");
  }
  return rc;

}