#ifndef TRANSPILE_H
#define TRANSPILE_H

#include "data/apm.h"
#include "data/cpm.h"
#include "data/compiler.h"

void transpile(Compiler *compiler, Program *apm, CProgram *cpm);

#endif