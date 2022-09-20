//
// Created by root on 9/19/22.
//

#include "update_operator.h"
#include "sql/stmt/update_stmt.h"
#include "storage/common/table.h"

RC UpdateOperator::open()
{
  if (children_.size() != 1) {
    LOG_WARN("update operator must has 1 child");
    return RC::INTERNAL;
  }

  Operator *child = children_[0];
  RC rc = child->open();
//  LOG_INFO("Update operator opened");
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to open child operator: %s", strrc(rc));
    return rc;
  }

  Table *table = update_stmt_->table();

  while (RC::SUCCESS == (rc = child->next())) {
//    LOG_INFO("operating");
    Tuple *tuple = child->current_tuple();
    if (nullptr == tuple) {
      LOG_WARN("failed to get current record: %s", strrc(rc));
      return rc;
    }

    RowTuple *row_tuple = static_cast<RowTuple *>(tuple);
    Record &record = row_tuple->record();
//    LOG_INFO("Update operator started");
//    LOG_INFO("func");
    rc = table->update_record(nullptr, update_stmt_->attribute_name(),update_stmt_->value(),&record);
    if (rc != RC::SUCCESS) {
      LOG_WARN("failed to update record: %s", strrc(rc));
      return rc;
    }
    LOG_INFO("Update operator finished");
  }
  return RC::SUCCESS;

}

RC UpdateOperator::next()
{
  return RC::RECORD_EOF;
}

RC UpdateOperator::close()
{
  children_[0]->close();
  return RC::SUCCESS;
}