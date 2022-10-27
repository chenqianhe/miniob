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
// Created by Wangyunlai on 2022/5/22.
//

#include "sql/stmt/update_stmt.h"
#include "common/log/log.h"
#include "common/lang/string.h"
#include "storage/common/db.h"
#include "storage/common/table.h"
#include "sql/stmt/filter_stmt.h"

UpdateStmt::UpdateStmt(Table *table, const char *attribute_name,const Value *value, FilterStmt *filter_stmt)
  : table_(table), attribute_name_(attribute_name), value_(value), filter_stmt_(filter_stmt)
{}
UpdateStmt::~UpdateStmt()
{
  if (nullptr != filter_stmt_) {
    delete filter_stmt_;
    filter_stmt_ = nullptr;
  }
}
RC UpdateStmt::create(Db *db, const Updates &update_sql, Stmt *&stmt)
{
  if (nullptr == db) {
    LOG_WARN("invalid argument. db is null");
    return RC::INVALID_ARGUMENT;
  }
  //get table information
  const char *table_name = update_sql.relation_name;
  if (nullptr == table_name) {
    LOG_WARN("invalid argument. relation name is null.");
    return RC::INVALID_ARGUMENT;
  }
  Table *table = db->find_table(table_name);
  if (nullptr == table) {
    LOG_WARN("no such table. db=%s, table_name=%s", db->name(), table_name);
    return RC::SCHEMA_TABLE_NOT_EXIST;
  }
  //get attribute and value
  const char *attribute_name = update_sql.attribute_name;
  if (nullptr == attribute_name) {
    LOG_WARN("invalid argument. attribute name is null.");
    return RC::INVALID_ARGUMENT;
  }
  const Value *value = update_sql.value;
  Value *value_copy = new Value();
  switch (value->type) {
    case UNDEFINED:
      break;
    case CHARS:{
      value_copy->type = CHARS;
      value_copy->data = strdup((char *)(value->data));
    }
      break;
    case INTS:{
      value_copy->type = INTS;
      value_copy->data = malloc(sizeof(int));
      memcpy(value_copy->data, value->data, sizeof(int));
    }
      break;
    case FLOATS:{
      value_copy->type = FLOATS;
      value_copy->data = malloc(sizeof(float));
      memcpy(value_copy->data, value->data, sizeof(float));
    }
      break;
    case DATES:{
      value_copy->type = DATES;
      value_copy->data = malloc(sizeof(int));
      memcpy(value_copy->data, value->data, sizeof(int));
    }
      break;
    case NULL_:{
      value_copy->type = NULL_;
    }
      break;
  }
//  LOG_INFO("UpdateStmt::create RECEIVED STRING: %s, TYPE: %d", (char *)(value->data), value->type);
  //get conditions and create filter
  std::unordered_map<std::string, Table *> table_map;
//  LOG_INFO("UpdateStmt::create RECEIVED STRING2: %s, TYPE: %d", value->data, value->type);
  table_map.insert(std::pair<std::string, Table *>(std::string(table_name), table));
  FilterStmt *filter_stmt = nullptr;
  RC rc = FilterStmt::create(db, table, &table_map,
      update_sql.conditions, update_sql.condition_num, filter_stmt);
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to create filter statement. rc=%d:%s", rc, strrc(rc));
    return rc;
  }

  stmt = new UpdateStmt(table,attribute_name,value_copy,filter_stmt);
  return rc;
}
