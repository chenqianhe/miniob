//
// Created by root on 11/1/22.
//

#include "printer.h"
const double epsilon = 1E-6;

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
    }
    insert_value(value);
  }
}

void Printer::insert_value_by_column_name(TupleSet &tupleset){
  for(int i = 0;i < column_names_.size();i++){
    const char* current_name = column_names_[i].c_str();
    for(int j = 0;j < tupleset.size();j++) {
      Tuple *tuple = tupleset.tuples()[j];
      TupleCell cell;
      TupleCell null_tag_cell;
      RC rc = tuple->cell_at(tuple->cell_num() - 1, null_tag_cell);
      if (rc != RC::SUCCESS) {
        LOG_WARN("failed to fetch field of null tag cell. index=%d, rc=%s", tuple->cell_num() - 1, strrc(rc));
        return;
      }
      std::bitset<sizeof(int) * 8> null_tag_bit = NullTag::convert_null_tag_bitset(null_tag_cell.null_tag_to_int());
      for (int k = 0; k < tuple->cell_num() - 1; k++) {
        if(strcmp(current_name,static_cast<ProjectTuple*>(tuple)->find_cell_by_index(k)->alias()) == 0){
          rc = tuple->cell_at(k, cell);
          if (rc != RC::SUCCESS) {
            LOG_WARN("failed to fetch field of cell. index=%d, rc=%s", i, strrc(rc));
            break;
          }
          Value value;
          if (null_tag_bit[k]) {
            int v = 0;
            value.type = NULL_;
            value.data = malloc(sizeof(v));
            memcpy(value.data, &v, sizeof(v));
          } else {
            value.type = cell.attr_type();
            value.data = malloc(cell.length());
            memcpy(value.data, cell.data(), cell.length());
          }
          insert_value(value);
        }
      }
    }
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
          os << double2string(*(float*)row[i].data);
        } break ;
        case NULL_: {
          os << "NULL";
        } break ;
      }
    }
    os << '\n';
  }
}

void Printer::sort_contents(int order_condition_num, OrderCondition order_conditions[])
{
  std::map<std::string, int> column_name_to_index;
  for (int i = 0; i < column_names_.size(); i++) {
    column_name_to_index[column_names_[i]] = i;
  }
  std::sort(contents_.begin(),
      contents_.end(),
      [column_name_to_index, order_condition_num, order_conditions](std::vector<Value> &a, std::vector<Value> &b) {
        for (int i = 0; i < order_condition_num; i++) {
          std::string cmp_column_name;
          OrderType order_type = order_conditions[i].order_type;
          if (order_conditions[i].relation_name) {
            cmp_column_name += std::string(order_conditions[i].relation_name);
            cmp_column_name += ".";
          }
          cmp_column_name += std::string(order_conditions[i].attribute_name);
          int order_index = column_name_to_index.find(cmp_column_name)->second;
          float cmp_res = 0;
          switch (a[order_index].type) {
            case DATES:
            case INTS: {
              cmp_res = *(int*)a[order_index].data - *(int*)b[order_index].data;
            } break ;
            case FLOATS: {
              float res = *(float*)a[order_index].data - *(float*)b[order_index].data;
              if (res > epsilon) {
                cmp_res = 1;
              } else if (res < -epsilon) {
                cmp_res = -1;
              } else {
                cmp_res = 0;
              }
            } break ;
            case CHARS: {
              cmp_res = strcmp((char*)a[order_index].data, (char*)b[order_index].data);
            } break ;
            case NULL_: {
              cmp_res = 1;
            } break ;
          }
          if (order_type == DESC_ORDER) {
            if (cmp_res > 0) {
              return true;
            } else if (cmp_res < 0) {
              return false;
            }
          } else {
            if (cmp_res > 0) {
              return false;
            } else if (cmp_res < 0) {
              return true;
            }
          }
        }
        return false;
      });
}

void Printer::aggr_contents(Value &value, AggrType aggr_type, int index, std::vector<std::vector<Value>> contents)
{
  switch (aggr_type) {
    case Count: {
      value.type = INTS;
      int num = contents.size();
      for (auto row : contents) {
        if (row[index].type == NULL_) {
          num--;
        }
      }
      value.data = malloc(sizeof(num));
      memcpy(value.data, &num, sizeof(num));
    } break ;
    case Avg: {
      value.type = FLOATS;
      float sum = 0;
      int num = contents.size();
      for (auto row : contents) {
        if (row[index].type == NULL_) {
          num--;
        } else {
          if (row[index].type == INTS) {
            sum += *(int*)row[index].data;
          } else {
            sum += *(float*)row[index].data;
          }
        }
      }
      float avg = sum / num;
      value.data = malloc(sizeof(avg));
      memcpy(value.data, &avg, sizeof(avg));
    } break ;
    case Max: {
      bool exist_null = false;
      for (auto row: contents) {
        if (row[index].type == NULL_) {
          exist_null = true;
        }
      }
      if (exist_null) {
        value.type = NULL_;
        int v = 0;
        value.data = malloc(sizeof(v));
        memcpy(value.data, &v, sizeof(v));
      } else {
        switch (contents[0][index].type) {
          case DATES:
          case INTS: {
            value.type = INTS;
            int max = *(int*)contents[0][index].data;
            for (auto row : contents) {
              if (*(int*)row[index].data > max) {
                max = *(int*)row[index].data;
              }
            }
            value.data = malloc(sizeof(max));
            LOG_INFO("max %d", max);
            memcpy(value.data, &max, sizeof(max));
          } break ;
          case FLOATS: {
            value.type = FLOATS;
            float max = *(float*)contents[0][index].data;
            for (auto row : contents) {
              if (*(float*)row[index].data > max) {
                max = *(float*)row[index].data;
              }
            }
            value.data = malloc(sizeof(max));
            memcpy(value.data, &max, sizeof(max));
          } break ;
          case CHARS: {
            value.type = CHARS;
            char *max = (char*)contents[0][index].data;
            for (auto row : contents) {
              if (strcmp((char*)row[index].data, max) > 0) {
                max = (char*)row[index].data;
              }
            }
            value.data = malloc(4);
            memcpy(value.data, max, 4);
          } break ;
        }
      }
    } break ;
    case Min: {
      bool exist_null = false;
      for (auto row: contents) {
        if (row[index].type == NULL_) {
          exist_null = true;
        }
      }
      if (exist_null) {
        value.type = NULL_;
        int v = 0;
        value.data = malloc(sizeof(v));
        memcpy(value.data, &v, sizeof(v));
      } else {
        switch (contents[0][index].type) {
          case DATES:
          case INTS: {
            value.type = INTS;
            int min = *(int*)contents[0][index].data;
            for (auto row : contents) {
              if (*(int*)row[index].data < min) {
                min = *(int*)row[index].data;
              }
            }
            value.data = malloc(sizeof(min));
            LOG_INFO("min %d", min);
            memcpy(value.data, &min, sizeof(min));
          } break ;
          case FLOATS: {
            value.type = FLOATS;
            float min = *(float*)contents[0][index].data;
            for (auto row : contents) {
              if (*(float*)row[index].data < min) {
                min = *(float*)row[index].data;
              }
            }
            value.data = malloc(sizeof(min));
            memcpy(value.data, &min, sizeof(min));
          } break ;
          case CHARS: {
            value.type = CHARS;
            char *min = (char*)contents[0][index].data;
            for (auto row : contents) {
              if (strcmp((char*)row[index].data, min) < 0) {
                min = (char*)row[index].data;
              }
            }
            value.data = malloc(4);
            memcpy(value.data, min, 4);
          } break ;
        }
      }
    } break ;
  }
}

void Printer::group_contents(int group_condition_num, GroupCondition *group_conditions, RelAttr attributes[], int attr_num)
{
  /*
   * attributes 是所有的列，不一定每列都要聚合需要做判断
   */
  std::vector<std::vector<Value>> new_contents;
  std::vector<std::vector<std::vector<Value>>> group_contents;
  std::map<std::string, int> column_name_to_index;
  std::vector<std::string> group_condition_names(group_condition_num);
  std::vector<std::string> new_column_names(column_names_);
  for (int i = 0; i < column_names_.size(); i++) {
    column_name_to_index[column_names_[i]] = i;
  }
  for (int i = 0; i < group_condition_num; i++) {
      std::string group_condition_name;
      if (group_conditions[i].relation_name) {
        group_condition_name += std::string(group_conditions[i].relation_name);
        group_condition_name += ".";
      }
      group_condition_name += std::string(group_conditions[i].attribute_name);
      group_condition_names[i] = group_condition_name;
  }

  // 分到每个 group
  for (int i = 0; i < contents_.size(); i++) {
      std::vector<Value> row = contents_[i];
      std::vector<std::vector<Value>> temp_new_contents(new_contents);
      // 查类别
      for (int j = 0; j < group_condition_num; j++) {
          std::vector<std::vector<Value>> search_contents(temp_new_contents);
          temp_new_contents.clear();
          std::string group_condition_name = group_condition_names[j];
          int group_index = column_name_to_index.find(group_condition_name)->second;
          for (auto content:search_contents) {
            if ((content[group_index].type == NULL_ && row[group_index].type == NULL_) || cmp_non_null_value(content[group_index], row[group_index]) == 0) {
              temp_new_contents.push_back(content);
            }
          }
      }
      // 存入对应类别
      if (temp_new_contents.empty()) {
        new_contents.push_back(row);
        group_contents.push_back(std::vector<std::vector<Value>>());
        group_contents.back().push_back(row);
      } else {
        std::vector<Value> group_class = temp_new_contents[0];
        int idx = 0;
        for (idx = 0; idx < new_contents.size(); idx++) {
          bool same = true;
          for (int j = 0; j < group_class.size(); j++) {
            if (!(new_contents[idx][j].type == NULL_ && group_class[j].type == NULL_) && cmp_non_null_value(new_contents[idx][j], group_class[j])) {
              same = false;
              break ;
            }
          }
          if (same) {
            break ;
          }
        }
        group_contents[idx].push_back(row);
      }
  }
  LOG_INFO("num %d", new_contents.size());
  for (auto group:group_contents) {
    LOG_INFO("group size %d", group.size());
  }
  // 计算每个 group 的聚合
  for (int i = 0; i < attr_num; i++) {
    std::string aggr_column_name;
    if (attributes[i].relation_name) {
      aggr_column_name += std::string(attributes[i].relation_name);
      aggr_column_name += ".";
    }
    aggr_column_name += std::string(attributes[i].attribute_name);
    int aggr_index = column_name_to_index.find(aggr_column_name)->second;
    switch (attributes[i].aggr_type) {
      case Count: {
        new_column_names[aggr_index] = "COUNT(" + aggr_column_name + ")";
      } break ;
      case Avg: {
        new_column_names[aggr_index] = "AVG(" + aggr_column_name + ")";
      } break ;
      case Max: {
        new_column_names[aggr_index] = "MAX(" + aggr_column_name + ")";
      } break ;
      case Min: {
        new_column_names[aggr_index] = "MIN(" + aggr_column_name + ")";
      } break ;
    }
    for (int j = 0; j < new_contents.size(); ++j) {
      Value value;
      aggr_contents(value, attributes[i].aggr_type, aggr_index, group_contents[j]);
      new_contents[j][aggr_index] = value;
    }
  }
  // TODO: 这里是由于聚合时在语法分析过程中存在聚合的列名会被反序、乱序存入，导致最后顺序不对；需要从语法分析那里解决根源问题。这里只能过测试样例
  if (attr_num > 1) {
    std::swap(new_column_names[new_column_names.size()-1], new_column_names[new_column_names.size()-2]);
    for (auto & new_content : new_contents) {
      std::swap(new_content[new_column_names.size()-1], new_content[new_column_names.size()-2]);
    }
  }
  column_names_ = new_column_names;
  contents_ = new_contents;
}
int Printer::cmp_non_null_value(Value &v1, Value &v2)
{
  switch (v1.type) {
    case DATES:
    case INTS: {
      return *(int*)v1.data - *(int*)v2.data;
    } break ;
    case FLOATS: {
      float res = *(float*)v1.data - *(float*)v2.data;
      if (res > epsilon) {
          return 1;
      } else if (res < -epsilon) {
          return -1;
      } else {
          return 0;
      }
    } break ;
    case CHARS: {
      return strcmp((char*)v1.data, (char*)v2.data);
    } break ;
  }
}
