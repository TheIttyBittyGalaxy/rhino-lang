// This file was generated automatically by build_program/build.py

// Make a function call to Unit P.
size_t emit_call(Unit* unit, Unit* P)
{
	size_t i = unit->count++;
	unit->instruction[i].op = OP_CALL;
	
	union
	{
		Unit* data;
		uint32_t word[wordsizeof(Unit*)];
	} as = {.data = P};
	for (size_t i = 0; i < wordsizeof(Unit*); i++)
		unit->instruction[unit->count++].word = as.word[i];
	
	return i;
}

// Set the Program Counter to Y.
size_t emit_jump(Unit* unit, uint16_t Y)
{
	size_t i = unit->count++;
	unit->instruction[i].op = OP_JUMP;
	unit->instruction[i].y = y;
	return i;
}

// Set the Program Counter to Y if the value in register X is false or none.
size_t emit_jump_if(Unit* unit, vm_reg X, uint16_t Y)
{
	size_t i = unit->count++;
	unit->instruction[i].op = OP_JUMP_IF;
	unit->instruction[i].x = x;
	unit->instruction[i].y = y;
	return i;
}

// Copy the value in register (A, X) to register (B, X).
size_t emit_copy(Unit* unit, uint8_t X, vm_reg A, vm_reg B)
{
	size_t i = unit->count++;
	unit->instruction[i].op = OP_COPY;
	unit->instruction[i].x = x;
	unit->instruction[i].a = a;
	unit->instruction[i].b = b;
	return i;
}

// Copy the value in register (A, X) to register B.
size_t emit_copy_down(Unit* unit, uint8_t X, vm_reg A, vm_reg B)
{
	size_t i = unit->count++;
	unit->instruction[i].op = OP_COPY_DOWN;
	unit->instruction[i].x = x;
	unit->instruction[i].a = a;
	unit->instruction[i].b = b;
	return i;
}

// Copy the value in register A to register (B, X).
size_t emit_copy_up(Unit* unit, uint8_t X, vm_reg A, vm_reg B)
{
	size_t i = unit->count++;
	unit->instruction[i].op = OP_COPY_UP;
	unit->instruction[i].x = x;
	unit->instruction[i].a = a;
	unit->instruction[i].b = b;
	return i;
}

// (A, X) = none
size_t emit_load_none(Unit* unit, uint8_t X, vm_reg A)
{
	size_t i = unit->count++;
	unit->instruction[i].op = OP_LOAD_NONE;
	unit->instruction[i].x = x;
	unit->instruction[i].a = a;
	return i;
}

// (A, X) = true
size_t emit_load_true(Unit* unit, uint8_t X, vm_reg A)
{
	size_t i = unit->count++;
	unit->instruction[i].op = OP_LOAD_TRUE;
	unit->instruction[i].x = x;
	unit->instruction[i].a = a;
	return i;
}

// (A, X) = false
size_t emit_load_false(Unit* unit, uint8_t X, vm_reg A)
{
	size_t i = unit->count++;
	unit->instruction[i].op = OP_LOAD_FALSE;
	unit->instruction[i].x = x;
	unit->instruction[i].a = a;
	return i;
}

// (A, X) = P
size_t emit_load_int(Unit* unit, uint8_t X, vm_reg A, int P)
{
	size_t i = unit->count++;
	unit->instruction[i].op = OP_LOAD_INT;
	unit->instruction[i].x = x;
	unit->instruction[i].a = a;
	
	union
	{
		int data;
		uint32_t word[wordsizeof(int)];
	} as = {.data = P};
	for (size_t i = 0; i < wordsizeof(int); i++)
		unit->instruction[unit->count++].word = as.word[i];
	
	return i;
}

// (A, X) = P
size_t emit_load_num(Unit* unit, uint8_t X, vm_reg A, double P)
{
	size_t i = unit->count++;
	unit->instruction[i].op = OP_LOAD_NUM;
	unit->instruction[i].x = x;
	unit->instruction[i].a = a;
	
	union
	{
		double data;
		uint32_t word[wordsizeof(double)];
	} as = {.data = P};
	for (size_t i = 0; i < wordsizeof(double); i++)
		unit->instruction[unit->count++].word = as.word[i];
	
	return i;
}

// (A, X) = P
size_t emit_load_str(Unit* unit, uint8_t X, vm_reg A, char* P)
{
	size_t i = unit->count++;
	unit->instruction[i].op = OP_LOAD_STR;
	unit->instruction[i].x = x;
	unit->instruction[i].a = a;
	
	union
	{
		char* data;
		uint32_t word[wordsizeof(char*)];
	} as = {.data = P};
	for (size_t i = 0; i < wordsizeof(char*); i++)
		unit->instruction[unit->count++].word = as.word[i];
	
	return i;
}

// Output the value in register (A, X).
size_t emit_out(Unit* unit, uint8_t X, vm_reg A)
{
	size_t i = unit->count++;
	unit->instruction[i].op = OP_OUT;
	unit->instruction[i].x = x;
	unit->instruction[i].a = a;
	return i;
}

// B = (A, X) ; (A, X) = (A, X) + 1
size_t emit_inca(Unit* unit, uint8_t X, vm_reg A, vm_reg B)
{
	size_t i = unit->count++;
	unit->instruction[i].op = OP_INCA;
	unit->instruction[i].x = x;
	unit->instruction[i].a = a;
	unit->instruction[i].b = b;
	return i;
}

// B = (A, X) ; (A, X) = (A, X) - 1
size_t emit_deca(Unit* unit, uint8_t X, vm_reg A, vm_reg B)
{
	size_t i = unit->count++;
	unit->instruction[i].op = OP_DECA;
	unit->instruction[i].x = x;
	unit->instruction[i].a = a;
	unit->instruction[i].b = b;
	return i;
}

// (A, X) = (A, X) + 1 ; B = (A, X)
size_t emit_incb(Unit* unit, uint8_t X, vm_reg A, vm_reg B)
{
	size_t i = unit->count++;
	unit->instruction[i].op = OP_INCB;
	unit->instruction[i].x = x;
	unit->instruction[i].a = a;
	unit->instruction[i].b = b;
	return i;
}

// (A, X) = (A, X) - 1 ; B = (A, X)
size_t emit_decb(Unit* unit, uint8_t X, vm_reg A, vm_reg B)
{
	size_t i = unit->count++;
	unit->instruction[i].op = OP_DECB;
	unit->instruction[i].x = x;
	unit->instruction[i].a = a;
	unit->instruction[i].b = b;
	return i;
}

// B = - (A, X)
size_t emit_neg(Unit* unit, uint8_t X, vm_reg A, vm_reg B)
{
	size_t i = unit->count++;
	unit->instruction[i].op = OP_NEG;
	unit->instruction[i].x = x;
	unit->instruction[i].a = a;
	unit->instruction[i].b = b;
	return i;
}

// B = NOT (A, X)
size_t emit_not(Unit* unit, uint8_t X, vm_reg A, vm_reg B)
{
	size_t i = unit->count++;
	unit->instruction[i].op = OP_NOT;
	unit->instruction[i].x = x;
	unit->instruction[i].a = a;
	unit->instruction[i].b = b;
	return i;
}

// X = A + B
size_t emit_add(Unit* unit, vm_reg X, vm_reg A, vm_reg B)
{
	size_t i = unit->count++;
	unit->instruction[i].op = OP_ADD;
	unit->instruction[i].x = x;
	unit->instruction[i].a = a;
	unit->instruction[i].b = b;
	return i;
}

// X = A - B
size_t emit_sub(Unit* unit, vm_reg X, vm_reg A, vm_reg B)
{
	size_t i = unit->count++;
	unit->instruction[i].op = OP_SUB;
	unit->instruction[i].x = x;
	unit->instruction[i].a = a;
	unit->instruction[i].b = b;
	return i;
}

// X = A * B
size_t emit_mul(Unit* unit, vm_reg X, vm_reg A, vm_reg B)
{
	size_t i = unit->count++;
	unit->instruction[i].op = OP_MUL;
	unit->instruction[i].x = x;
	unit->instruction[i].a = a;
	unit->instruction[i].b = b;
	return i;
}

// X = A / B
size_t emit_div(Unit* unit, vm_reg X, vm_reg A, vm_reg B)
{
	size_t i = unit->count++;
	unit->instruction[i].op = OP_DIV;
	unit->instruction[i].x = x;
	unit->instruction[i].a = a;
	unit->instruction[i].b = b;
	return i;
}

// X = the remainder of A / B
size_t emit_rem(Unit* unit, vm_reg X, vm_reg A, vm_reg B)
{
	size_t i = unit->count++;
	unit->instruction[i].op = OP_REM;
	unit->instruction[i].x = x;
	unit->instruction[i].a = a;
	unit->instruction[i].b = b;
	return i;
}

// X = A == B
size_t emit_eqla(Unit* unit, vm_reg X, vm_reg A, vm_reg B)
{
	size_t i = unit->count++;
	unit->instruction[i].op = OP_EQLA;
	unit->instruction[i].x = x;
	unit->instruction[i].a = a;
	unit->instruction[i].b = b;
	return i;
}

// X = NOT A == B
size_t emit_eqln(Unit* unit, vm_reg X, vm_reg A, vm_reg B)
{
	size_t i = unit->count++;
	unit->instruction[i].op = OP_EQLN;
	unit->instruction[i].x = x;
	unit->instruction[i].a = a;
	unit->instruction[i].b = b;
	return i;
}

// X = A < B
size_t emit_less(Unit* unit, vm_reg X, vm_reg A, vm_reg B)
{
	size_t i = unit->count++;
	unit->instruction[i].op = OP_LESS;
	unit->instruction[i].x = x;
	unit->instruction[i].a = a;
	unit->instruction[i].b = b;
	return i;
}

// X = A <= B
size_t emit_less_equal(Unit* unit, vm_reg X, vm_reg A, vm_reg B)
{
	size_t i = unit->count++;
	unit->instruction[i].op = OP_LESS_EQUAL;
	unit->instruction[i].x = x;
	unit->instruction[i].a = a;
	unit->instruction[i].b = b;
	return i;
}

// X = A AND B
size_t emit_and(Unit* unit, vm_reg X, vm_reg A, vm_reg B)
{
	size_t i = unit->count++;
	unit->instruction[i].op = OP_AND;
	unit->instruction[i].x = x;
	unit->instruction[i].a = a;
	unit->instruction[i].b = b;
	return i;
}

// X = A OR B
size_t emit_or(Unit* unit, vm_reg X, vm_reg A, vm_reg B)
{
	size_t i = unit->count++;
	unit->instruction[i].op = OP_OR;
	unit->instruction[i].x = x;
	unit->instruction[i].a = a;
	unit->instruction[i].b = b;
	return i;
}

