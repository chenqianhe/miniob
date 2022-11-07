//
// Created by root on 10/16/22.
//

#ifndef MINIDB_DESCARTES_H
#define MINIDB_DESCARTES_H
#include "../sql/expr/tuple.h"
#include "../sql/expr/TupleSet.h"
#include "../sql/stmt/filter_stmt.h"
#include <vector>
void descartesRecursive(std::vector<TupleSet*>& originalList,int posotion,std::vector<TupleSet*>& returnList,TupleSet& line,FilterStmt *filter_stmt);
std::vector<TupleSet*> getDescartes(std::vector<TupleSet*>& list,FilterStmt *filter_stmt);
bool do_predicate_descarte(TupleSet &descarte_tuple,Tuple * new_tuple,FilterStmt *filter_stmt);
#endif  // MINIDB_DESCARTES_H
