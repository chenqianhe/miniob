//
// Created by root on 11/1/22.
//

#include "printer.h"


void Printer::insert_value_from_tuple(const Tuple &tuple)
{
  TupleCell cell;
  TupleCell null_tag_cell;
  RC rc = tuple.cell_at(tuple.cell_num()-1, null_tag_cell);
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to fetch field of null tag cell. index=%d, rc=%s", tuple.cell_num()-1, strrc(rc));
    return ;
  }
  std::bitset<sizeof(int) * 8> null_tag_bit = NullTag::convert_null_tag_bitset(null_tag_cell.null_tag_to_int());
  for (int i = 0; i < tuple.cell_num()-1; i++) {
    rc = tuple.cell_at(i, cell);
    if (rc != RC::SUCCESS) {
      LOG_WARN("failed to fetch field of cell. index=%d, rc=%s", i, strrc(rc));
      break;
    }
    Value value;
    if (null_tag_bit[i]) {
      int v = 0;
      value.type = NULL_;
      value.data = malloc(sizeof(v));
      memcpy(value.data, &v, sizeof(v));
    } else {
      value.type = cell.attr_type();
      value.data = malloc(cell.length());
      memcpy(value.data, cell.data(), cell.length());
      LOG_INFO("length: %d", cell.length());
    }
    insert_value(value);
  }
}

void Printer::print_headers(std::ostream &os) {
  for (int i = 0; i < column_names_.size(); i++) {
    if (i != 0) {
      os << " | ";
    }
    os << column_names_[i];
  }
  os << '\n';
}

void Printer::print_contents(std::ostream &os)
{
  for (auto row : contents_) {
    for (int i = 0; i < column_names_.size(); i++) {
      if (i != 0) {
        os << " | ";
      }
      switch (row[i].type) {
        case INTS: {
          os << std::to_string(*(int*)row[i].data);
        } break ;
        case CHARS: {
          for (int j = 0; j < 4; j++) {
            if (((char *)(row[i].data))[j] == '\0') {
              break;
            }
            os << ((char *)(row[i].data))[j];
          }
        } break ;
        case DATES: {
          char buf[16] = {0};
          snprintf(buf,sizeof(buf),"%04d-%02d-%02d",*(int *)(row[i].data)/10000,(*(int *)(row[i].data)%10000)/100,*(int *)(row[i].data)%100);
          for (char j : buf) {
            if (j == '\0') {
              break;
            }
            os << j;
          }
        } break ;
        case FLOATS: {
          int temp = *(float*)row[i].data;
          if (temp == *(float*)row[i].data) {
            os << std::to_string(temp);
          } else {
            char buf[MAX_NUM] = {0};
            snprintf(buf,sizeof(buf),"%.1lf", *(float*)row[i].data);
            os << std::string(buf);
          }
        } break ;
        case NULL_: {
          os << "NULL";
        } break ;
      }
    }
    os << '\n';
  }
}

void Printer::sort_contents(int order_condition_num, OrderCondition oder_conditions[])
{

}
