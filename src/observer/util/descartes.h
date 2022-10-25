//
// Created by root on 10/16/22.
//

#ifndef MINIDB_DESCARTES_H
#define MINIDB_DESCARTES_H
#include "../sql/expr/tuple.h"
#include "../sql/expr/TupleSet.h"
#include <vector>
void descartesRecursive(std::vector<TupleSet>& originalList,int posotion,TupleSet& returnList,Tuple& line);
TupleSet getDescartes(std::vector<TupleSet>& list);

#endif  // MINIDB_DESCARTES_H
