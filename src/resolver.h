#ifndef sg_resolver_h
#define sg_resolver_h

#include "stmt.h"

typedef struct Interpreter Interpreter;

void resolve(Interpreter* interpreter, StmtList* statements);

#endif
