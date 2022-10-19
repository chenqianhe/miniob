//
// Created by root on 10/16/22.
//
#include "descartes.h"
void descartesRecursive(std::vector<TupleSet>& originalList,int position,TupleSet& returnList,ProjectTuple& line){
  TupleSet table = originalList[position];
  for(int i = 0; i < table.size(); i++){
    line.add(*table.get(i));
    if(position == originalList.size() - 1){
      ProjectTuple backend(line);
      returnList.add_tuple(&backend);
      line.remove(*table.get(i));
      continue;
    }
    descartesRecursive(originalList,position+1,returnList,line);
    line.remove(*table.get(i));
  }
}

TupleSet getDescartes(std::vector<TupleSet>& list){
  TupleSet returnList;
  ProjectTuple line;
  descartesRecursive(list,0,returnList,line);
  return returnList;
}