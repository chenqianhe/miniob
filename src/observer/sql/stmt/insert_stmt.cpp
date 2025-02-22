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

#include "sql/stmt/insert_stmt.h"
#include "common/log/log.h"
#include "storage/common/db.h"
#include "storage/common/table.h"
#include "util/date.h"

InsertStmt::InsertStmt(Table *table, const Value *values, int value_amount, int values_group_num)
  : table_ (table), values_(values), value_amount_(value_amount), values_group_num_(values_group_num)
{}

RC InsertStmt::create(Db *db, Inserts &inserts, Stmt *&stmt)
{
  const char *table_name = inserts.relation_name;
  if (nullptr == db || nullptr == table_name || inserts.value_num <= 0) {
    LOG_WARN("invalid argument. db=%p, table_name=%p, value_num=%d", 
             db, table_name, inserts.value_num);
    return RC::INVALID_ARGUMENT;
  }

  // check whether the table exists
  Table *table = db->find_table(table_name);
  if (nullptr == table) {
    LOG_WARN("no such table. db=%s, table_name=%s", db->name(), table_name);
    return RC::SCHEMA_TABLE_NOT_EXIST;
  }

  // check the fields number
  Value *values = inserts.values;
  const int value_num = inserts.value_num;
  const int values_group_num = inserts.values_group_num;
  const TableMeta &table_meta = table->table_meta();
  const int field_num = table_meta.field_num() - table_meta.sys_field_num();
  if (field_num * values_group_num != value_num) {
    LOG_WARN("schema mismatch. value num=%d, field num in schema=%d", value_num, field_num * values_group_num);
    return RC::SCHEMA_FIELD_MISSING;
  }

  // check fields type
  const int sys_field_num = table_meta.sys_field_num();
  for (int value_group_index=0; value_group_index < values_group_num; value_group_index++) {
    for (int i = 0; i < field_num; i++) {
      const FieldMeta *field_meta = table_meta.field(i + sys_field_num);
      const AttrType field_type = field_meta->type();
      const AttrType value_type = values[value_group_index*field_num+i].type;
      if (field_type != value_type) { // TODO try to convert the value type to field type
        if (value_type == NULL_) {
          if (field_meta->null_able()) {
            continue;
          } else {
            LOG_WARN("Field null_able mismatch. table=%s, field=%s, field type=%d, value_type=%d, field null_able=%d",
                     table_name, field_meta->name(), field_type, value_type, field_meta->null_able());
            return RC::RECEIVED_NULL_BUT_NOT_NULLABLE;
          }
        }
        LOG_WARN("field type mismatch. table=%s, field=%s, field type=%d, value_type=%d",
            table_name, field_meta->name(), field_type, value_type);
        return RC::SCHEMA_FIELD_TYPE_MISMATCH;
      }
    }
  }

  // everything alright
  stmt = new InsertStmt(table, values, field_num, values_group_num);
  return RC::SUCCESS;
}
