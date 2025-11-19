#ifndef ASSEMBLE_H
#define ASSEMBLE_H

#include "core/core.h"
#include "data/apm.h"
#include "data/compiler.h"
#include "data/byte_code.h"

void assemble(Compiler *compiler, Program *apm, ByteCode *byte_code);

#endif