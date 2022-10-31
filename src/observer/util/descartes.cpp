//
// Created by root on 10/16/22.
//
#include "descartes.h"
void descartesRecursive(std::vector<TupleSet*>& originalList,int position,std::vector<TupleSet *>& returnList,TupleSet& line)
{
  TupleSet *table = originalList[position];
  int size = table->size();
  LOG_INFO("table size is %d",size);
  for(int i = 0; i < size; i++){
    LOG_INFO("get tuple %d",i);
    Tuple *tmp = table->tuples()[i];
    LOG_INFO("add tuple %d",i);
    line.add_tuple(tmp);
    if(position == originalList.size() - 1){
      LOG_INFO("iter to get one result");
      TupleSet *final = new TupleSet();
      for(int j = 0; j < line.size();j++){
        final->add_tuple(line.tuples()[j]);
      }
      returnList.push_back(final);
      line.remove();
      LOG_INFO("get one result,size is %d",final->size());
      continue;
    }
    descartesRecursive(originalList,position+1,returnList,line);
    line.remove();
  }
}

std::vector<TupleSet*> getDescartes(std::vector<TupleSet*>& list){
  std::vector<TupleSet*> returnList;
  TupleSet *line = new TupleSet();
  descartesRecursive(list,0,returnList,*line);
  LOG_INFO("descartesRecursive finished,number of tuple is %d",returnList.size());
  LOG_INFO("descartesRecursive finished,size of tuple is %d",returnList[0]->size());
  delete line;
  return returnList;
}