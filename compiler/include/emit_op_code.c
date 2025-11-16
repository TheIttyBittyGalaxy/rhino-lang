// This file was generated automatically by build_program/build.py

// CALL
// Make a function call to Unit P.
size_t emit_call(Unit* unit, Unit* p)
{
	size_t i = unit->count++;
	unit->instruction[i].op = OP_CALL;
	
	union
	{
		Unit* data;
		uint32_t word[wordsizeof(Unit*)];
	} as = {.data = p};
	for (size_t i = 0; i < wordsizeof(Unit*); i++)
		unit->instruction[unit->count++].word = as.word[i];
	
	return i;
}

// JUMP
// Set the Program Counter to Y.
size_t emit_jump(Unit* unit, uint16_t y)
{
	size_t i = unit->count++;
	unit->instruction[i].op = OP_JUMP;
	unit->instruction[i].y = y;
	return i;
}

// JUMP_IF
// Set the Program Counter to Y if the value in register X is false or none.
size_t emit_jump_if(Unit* unit, vm_reg x, uint16_t y)
{
	size_t i = unit->count++;
	unit->instruction[i].op = OP_JUMP_IF;
	unit->instruction[i].x = x;
	unit->instruction[i].y = y;
	return i;
}

// COPY
// (A, X) = (B, X)
size_t emit_copy(Unit* unit, uint8_t x, vm_reg a, vm_reg b)
{
	size_t i = unit->count++;
	unit->instruction[i].op = OP_COPY;
	unit->instruction[i].x = x;
	unit->instruction[i].a = a;
	unit->instruction[i].b = b;
	return i;
}

// COPY_UP
// (A, X) = B
size_t emit_copy_up(Unit* unit, uint8_t x, vm_reg a, vm_reg b)
{
	size_t i = unit->count++;
	unit->instruction[i].op = OP_COPY_UP;
	unit->instruction[i].x = x;
	unit->instruction[i].a = a;
	unit->instruction[i].b = b;
	return i;
}

// COPY_DN
// A = (B, X)
size_t emit_copy_dn(Unit* unit, uint8_t x, vm_reg a, vm_reg b)
{
	size_t i = unit->count++;
	unit->instruction[i].op = OP_COPY_DN;
	unit->instruction[i].x = x;
	unit->instruction[i].a = a;
	unit->instruction[i].b = b;
	return i;
}

// LOAD_NONE
// (A, X) = none
size_t emit_load_none(Unit* unit, uint8_t x, vm_reg a)
{
	size_t i = unit->count++;
	unit->instruction[i].op = OP_LOAD_NONE;
	unit->instruction[i].x = x;
	unit->instruction[i].a = a;
	return i;
}

// LOAD_TRUE
// (A, X) = true
size_t emit_load_true(Unit* unit, uint8_t x, vm_reg a)
{
	size_t i = unit->count++;
	unit->instruction[i].op = OP_LOAD_TRUE;
	unit->instruction[i].x = x;
	unit->instruction[i].a = a;
	return i;
}

// LOAD_FALSE
// (A, X) = false
size_t emit_load_false(Unit* unit, uint8_t x, vm_reg a)
{
	size_t i = unit->count++;
	unit->instruction[i].op = OP_LOAD_FALSE;
	unit->instruction[i].x = x;
	unit->instruction[i].a = a;
	return i;
}

// LOAD_NUM
// (A, X) = P
size_t emit_load_num(Unit* unit, uint8_t x, vm_reg a, double p)
{
	size_t i = unit->count++;
	unit->instruction[i].op = OP_LOAD_NUM;
	unit->instruction[i].x = x;
	unit->instruction[i].a = a;
	
	union
	{
		double data;
		uint32_t word[wordsizeof(double)];
	} as = {.data = p};
	for (size_t i = 0; i < wordsizeof(double); i++)
		unit->instruction[unit->count++].word = as.word[i];
	
	return i;
}

// LOAD_STR
// (A, X) = P
size_t emit_load_str(Unit* unit, uint8_t x, vm_reg a, char* p)
{
	size_t i = unit->count++;
	unit->instruction[i].op = OP_LOAD_STR;
	unit->instruction[i].x = x;
	unit->instruction[i].a = a;
	
	union
	{
		char* data;
		uint32_t word[wordsizeof(char*)];
	} as = {.data = p};
	for (size_t i = 0; i < wordsizeof(char*); i++)
		unit->instruction[unit->count++].word = as.word[i];
	
	return i;
}

// OUT
// Output the value in register (A, X).
size_t emit_out(Unit* unit, uint8_t x, vm_reg a)
{
	size_t i = unit->count++;
	unit->instruction[i].op = OP_OUT;
	unit->instruction[i].x = x;
	unit->instruction[i].a = a;
	return i;
}

// INC
// (A, X) = (A, X) + 1
size_t emit_inc(Unit* unit, uint8_t x, vm_reg a)
{
	size_t i = unit->count++;
	unit->instruction[i].op = OP_INC;
	unit->instruction[i].x = x;
	unit->instruction[i].a = a;
	return i;
}

// DEC
// (A, X) = (A, X) - 1
size_t emit_dec(Unit* unit, uint8_t x, vm_reg a)
{
	size_t i = unit->count++;
	unit->instruction[i].op = OP_DEC;
	unit->instruction[i].x = x;
	unit->instruction[i].a = a;
	return i;
}

// NEG
// A = - (B, X)
size_t emit_neg(Unit* unit, uint8_t x, vm_reg a, vm_reg b)
{
	size_t i = unit->count++;
	unit->instruction[i].op = OP_NEG;
	unit->instruction[i].x = x;
	unit->instruction[i].a = a;
	unit->instruction[i].b = b;
	return i;
}

// NOT
// A = NOT (B, X)
size_t emit_not(Unit* unit, uint8_t x, vm_reg a, vm_reg b)
{
	size_t i = unit->count++;
	unit->instruction[i].op = OP_NOT;
	unit->instruction[i].x = x;
	unit->instruction[i].a = a;
	unit->instruction[i].b = b;
	return i;
}

// ADD
// X = A + B
size_t emit_add(Unit* unit, vm_reg x, vm_reg a, vm_reg b)
{
	size_t i = unit->count++;
	unit->instruction[i].op = OP_ADD;
	unit->instruction[i].x = x;
	unit->instruction[i].a = a;
	unit->instruction[i].b = b;
	return i;
}

// SUB
// X = A - B
size_t emit_sub(Unit* unit, vm_reg x, vm_reg a, vm_reg b)
{
	size_t i = unit->count++;
	unit->instruction[i].op = OP_SUB;
	unit->instruction[i].x = x;
	unit->instruction[i].a = a;
	unit->instruction[i].b = b;
	return i;
}

// MUL
// X = A * B
size_t emit_mul(Unit* unit, vm_reg x, vm_reg a, vm_reg b)
{
	size_t i = unit->count++;
	unit->instruction[i].op = OP_MUL;
	unit->instruction[i].x = x;
	unit->instruction[i].a = a;
	unit->instruction[i].b = b;
	return i;
}

// DIV
// X = A / B
size_t emit_div(Unit* unit, vm_reg x, vm_reg a, vm_reg b)
{
	size_t i = unit->count++;
	unit->instruction[i].op = OP_DIV;
	unit->instruction[i].x = x;
	unit->instruction[i].a = a;
	unit->instruction[i].b = b;
	return i;
}

// REM
// X = the remainder of A / B
size_t emit_rem(Unit* unit, vm_reg x, vm_reg a, vm_reg b)
{
	size_t i = unit->count++;
	unit->instruction[i].op = OP_REM;
	unit->instruction[i].x = x;
	unit->instruction[i].a = a;
	unit->instruction[i].b = b;
	return i;
}

// EQLA
// X = A == B
size_t emit_eqla(Unit* unit, vm_reg x, vm_reg a, vm_reg b)
{
	size_t i = unit->count++;
	unit->instruction[i].op = OP_EQLA;
	unit->instruction[i].x = x;
	unit->instruction[i].a = a;
	unit->instruction[i].b = b;
	return i;
}

// EQLN
// X = NOT A == B
size_t emit_eqln(Unit* unit, vm_reg x, vm_reg a, vm_reg b)
{
	size_t i = unit->count++;
	unit->instruction[i].op = OP_EQLN;
	unit->instruction[i].x = x;
	unit->instruction[i].a = a;
	unit->instruction[i].b = b;
	return i;
}

// LESS_THN
// X = A < B
size_t emit_less_thn(Unit* unit, vm_reg x, vm_reg a, vm_reg b)
{
	size_t i = unit->count++;
	unit->instruction[i].op = OP_LESS_THN;
	unit->instruction[i].x = x;
	unit->instruction[i].a = a;
	unit->instruction[i].b = b;
	return i;
}

// LESS_EQL
// X = A <= B
size_t emit_less_eql(Unit* unit, vm_reg x, vm_reg a, vm_reg b)
{
	size_t i = unit->count++;
	unit->instruction[i].op = OP_LESS_EQL;
	unit->instruction[i].x = x;
	unit->instruction[i].a = a;
	unit->instruction[i].b = b;
	return i;
}

// AND
// X = A AND B
size_t emit_and(Unit* unit, vm_reg x, vm_reg a, vm_reg b)
{
	size_t i = unit->count++;
	unit->instruction[i].op = OP_AND;
	unit->instruction[i].x = x;
	unit->instruction[i].a = a;
	unit->instruction[i].b = b;
	return i;
}

// OR
// X = A OR B
size_t emit_or(Unit* unit, vm_reg x, vm_reg a, vm_reg b)
{
	size_t i = unit->count++;
	unit->instruction[i].op = OP_OR;
	unit->instruction[i].x = x;
	unit->instruction[i].a = a;
	unit->instruction[i].b = b;
	return i;
}

