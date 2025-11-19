#ifndef INTERPRET_H
#define INTERPRET_H

#include "core/core.h"
#include "data/byte_code.h"

void interpret(ByteCode *byte_code, RunOnString *output_string);

#endif