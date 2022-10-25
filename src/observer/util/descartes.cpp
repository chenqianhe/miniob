//
// Created by root on 10/16/22.
//
#include "descartes.h"
void descartesRecursive(std::vector<TupleSet*>& originalList,int position,std::vector<RowTuple *>& returnList,RowTuple& line)
{
  TupleSet *table = originalList[position];
//  int size = table.tuples()[0]->cell_num();
//  LOG_INFO("cell num of tupleset[%d] is %d",position,size);
  for(int i = 0; i < table->size(); i++){
    RowTuple *tmp = static_cast<RowTuple*>(table->tuples()[i]);
    int size = tmp->cell_num();
//    LOG_INFO("cell num of tupleset[%d] is %d",position,size);
    for(int j = 0;j < size;j++){
      line.add_cell_spec(tmp->get(j));
    }
//    LOG_INFO("add success,size of tuple is %d",line.tuple_size());
    if(position == originalList.size() - 1){
      LOG_INFO("already get one result");
      LOG_INFO("add success,size of tuple is %d",line.tuple_size());
      returnList.push_back(&line);
      line.remove(size);
      continue;
    }
//    LOG_INFO("start to iterator");
    descartesRecursive(originalList,position+1,returnList,line);
//    LOG_INFO("finish iterator");
    line.remove(size);
  }
}

std::vector<RowTuple*> getDescartes(std::vector<TupleSet*>& list){
  std::vector<RowTuple*> returnList;
  RowTuple line ;
  descartesRecursive(list,0,returnList,line);
//  LOG_INFO("descartesRecursive finished,size of tuple is %d",returnList.tuples()[0]->tuple_size());
  return returnList;
}