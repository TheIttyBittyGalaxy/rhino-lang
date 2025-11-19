#ifndef INTERPRET_H
#define INTERPRET_H

#include "core.h"
#include "data/byte_code.h"
#include "data/run_on_string.h"

void interpret(ByteCode *byte_code, RunOnString *output_string);

#endif