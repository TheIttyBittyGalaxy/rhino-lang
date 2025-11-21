// This file was generated automatically by build_program/build.py

size_t printf_instruction(Unit* unit, size_t i)
{
	Instruction ins = unit->instruction[i];
	printf("\x1b[90m%02X %02X %02X %02X  ", ins.b, ins.a, ins.x, ins.op);
	printf("\x1b[37m%04X  ", i);
	printf("\x1b[36m%-*s", 13, op_code_string((OpCode)ins.op));
	i++;

	switch (ins.op) {
	case OP_CALL:
	{
		printf("  \x1b[90mx \x1b[0m%2d  \x1b[90ma \x1b[0m%2d  \x1b[90mb \x1b[0m%2d\n", ins.x, ins.a, ins.b);

		union
		{
			Unit* value;
			uint32_t word[wordsizeof(Unit*)];
			uint8_t byte[wordsizeof(Unit*)][4];
		} as;
		for (size_t j = 0; j < wordsizeof(Unit*); j++)
			as.word[j] = unit->instruction[i++].word;

		printf("\x1b[90m%02X %02X %02X %02X        0x%p\n", as.byte[0][3], as.byte[0][2], as.byte[0][1], as.byte[0][0], as.value);
		for (size_t j = 1; j < wordsizeof(Unit*); j++)
			printf("%02X %02X %02X %02X\n", as.byte[j][3], as.byte[j][2], as.byte[j][1], as.byte[j][0]);
		break;
	}
	case OP_RUN:
	{
		printf("        \x1b[90mb \x1b[0m%2d\n", ins.b);

		union
		{
			Unit* value;
			uint32_t word[wordsizeof(Unit*)];
			uint8_t byte[wordsizeof(Unit*)][4];
		} as;
		for (size_t j = 0; j < wordsizeof(Unit*); j++)
			as.word[j] = unit->instruction[i++].word;

		printf("\x1b[90m%02X %02X %02X %02X        0x%p\n", as.byte[0][3], as.byte[0][2], as.byte[0][1], as.byte[0][0], as.value);
		for (size_t j = 1; j < wordsizeof(Unit*); j++)
			printf("%02X %02X %02X %02X\n", as.byte[j][3], as.byte[j][2], as.byte[j][1], as.byte[j][0]);
		break;
	}
	case OP_RTNN: printf("\n"); break;
	case OP_RTNV: printf("  \x1b[90mx \x1b[0m%2d  \x1b[90ma \x1b[0m%2d\n", ins.x, ins.a); break;
	case OP_JUMP: printf("        \x1b[90my \x1b[0m%04X\n", ins.y); break;
	case OP_JUMP_IF: printf("  \x1b[90mx \x1b[0m%2d  \x1b[90my \x1b[0m%04X\n", ins.x, ins.y); break;
	case OP_COPY: printf("  \x1b[90mx \x1b[0m%2d  \x1b[90ma \x1b[0m%2d  \x1b[90mb \x1b[0m%2d\n", ins.x, ins.a, ins.b); break;
	case OP_COPY_UP: printf("  \x1b[90mx \x1b[0m%2d  \x1b[90ma \x1b[0m%2d  \x1b[90mb \x1b[0m%2d\n", ins.x, ins.a, ins.b); break;
	case OP_COPY_DN: printf("  \x1b[90mx \x1b[0m%2d  \x1b[90ma \x1b[0m%2d  \x1b[90mb \x1b[0m%2d\n", ins.x, ins.a, ins.b); break;
	case OP_LOAD_NONE: printf("  \x1b[90mx \x1b[0m%2d  \x1b[90ma \x1b[0m%2d\n", ins.x, ins.a); break;
	case OP_LOAD_TRUE: printf("  \x1b[90mx \x1b[0m%2d  \x1b[90ma \x1b[0m%2d\n", ins.x, ins.a); break;
	case OP_LOAD_FALSE: printf("  \x1b[90mx \x1b[0m%2d  \x1b[90ma \x1b[0m%2d\n", ins.x, ins.a); break;
	case OP_LOAD_NUM:
	{
		printf("  \x1b[90mx \x1b[0m%2d  \x1b[90ma \x1b[0m%2d\n", ins.x, ins.a);

		union
		{
			double value;
			uint32_t word[wordsizeof(double)];
			uint8_t byte[wordsizeof(double)][4];
		} as;
		for (size_t j = 0; j < wordsizeof(double); j++)
			as.word[j] = unit->instruction[i++].word;

		printf("\x1b[90m%02X %02X %02X %02X        %f\n", as.byte[0][3], as.byte[0][2], as.byte[0][1], as.byte[0][0], as.value);
		for (size_t j = 1; j < wordsizeof(double); j++)
			printf("%02X %02X %02X %02X\n", as.byte[j][3], as.byte[j][2], as.byte[j][1], as.byte[j][0]);
		break;
	}
	case OP_LOAD_STR:
	{
		printf("  \x1b[90mx \x1b[0m%2d  \x1b[90ma \x1b[0m%2d\n", ins.x, ins.a);

		union
		{
			char* value;
			uint32_t word[wordsizeof(char*)];
			uint8_t byte[wordsizeof(char*)][4];
		} as;
		for (size_t j = 0; j < wordsizeof(char*); j++)
			as.word[j] = unit->instruction[i++].word;

		printf("\x1b[90m%02X %02X %02X %02X        0x%p\n", as.byte[0][3], as.byte[0][2], as.byte[0][1], as.byte[0][0], as.value);
		for (size_t j = 1; j < wordsizeof(char*); j++)
			printf("%02X %02X %02X %02X\n", as.byte[j][3], as.byte[j][2], as.byte[j][1], as.byte[j][0]);
		break;
	}
	case OP_LOAD_ENUM: printf("  \x1b[90mx \x1b[0m%2d  \x1b[90ma \x1b[0m%2d  \x1b[90mb \x1b[0m%2d\n", ins.x, ins.a, ins.b); break;
	case OP_OUT: printf("  \x1b[90mx \x1b[0m%2d  \x1b[90ma \x1b[0m%2d\n", ins.x, ins.a); break;
	case OP_INC: printf("  \x1b[90mx \x1b[0m%2d  \x1b[90ma \x1b[0m%2d\n", ins.x, ins.a); break;
	case OP_DEC: printf("  \x1b[90mx \x1b[0m%2d  \x1b[90ma \x1b[0m%2d\n", ins.x, ins.a); break;
	case OP_NEG: printf("  \x1b[90mx \x1b[0m%2d  \x1b[90ma \x1b[0m%2d  \x1b[90mb \x1b[0m%2d\n", ins.x, ins.a, ins.b); break;
	case OP_NOT: printf("  \x1b[90mx \x1b[0m%2d  \x1b[90ma \x1b[0m%2d  \x1b[90mb \x1b[0m%2d\n", ins.x, ins.a, ins.b); break;
	case OP_ADD: printf("  \x1b[90mx \x1b[0m%2d  \x1b[90ma \x1b[0m%2d  \x1b[90mb \x1b[0m%2d\n", ins.x, ins.a, ins.b); break;
	case OP_SUB: printf("  \x1b[90mx \x1b[0m%2d  \x1b[90ma \x1b[0m%2d  \x1b[90mb \x1b[0m%2d\n", ins.x, ins.a, ins.b); break;
	case OP_MUL: printf("  \x1b[90mx \x1b[0m%2d  \x1b[90ma \x1b[0m%2d  \x1b[90mb \x1b[0m%2d\n", ins.x, ins.a, ins.b); break;
	case OP_DIV: printf("  \x1b[90mx \x1b[0m%2d  \x1b[90ma \x1b[0m%2d  \x1b[90mb \x1b[0m%2d\n", ins.x, ins.a, ins.b); break;
	case OP_REM: printf("  \x1b[90mx \x1b[0m%2d  \x1b[90ma \x1b[0m%2d  \x1b[90mb \x1b[0m%2d\n", ins.x, ins.a, ins.b); break;
	case OP_EQLA: printf("  \x1b[90mx \x1b[0m%2d  \x1b[90ma \x1b[0m%2d  \x1b[90mb \x1b[0m%2d\n", ins.x, ins.a, ins.b); break;
	case OP_EQLN: printf("  \x1b[90mx \x1b[0m%2d  \x1b[90ma \x1b[0m%2d  \x1b[90mb \x1b[0m%2d\n", ins.x, ins.a, ins.b); break;
	case OP_LESS_THN: printf("  \x1b[90mx \x1b[0m%2d  \x1b[90ma \x1b[0m%2d  \x1b[90mb \x1b[0m%2d\n", ins.x, ins.a, ins.b); break;
	case OP_LESS_EQL: printf("  \x1b[90mx \x1b[0m%2d  \x1b[90ma \x1b[0m%2d  \x1b[90mb \x1b[0m%2d\n", ins.x, ins.a, ins.b); break;
	case OP_AND: printf("  \x1b[90mx \x1b[0m%2d  \x1b[90ma \x1b[0m%2d  \x1b[90mb \x1b[0m%2d\n", ins.x, ins.a, ins.b); break;
	case OP_OR: printf("  \x1b[90mx \x1b[0m%2d  \x1b[90ma \x1b[0m%2d  \x1b[90mb \x1b[0m%2d\n", ins.x, ins.a, ins.b); break;
	case OP_AS_STR: printf("  \x1b[90mx \x1b[0m%2d  \x1b[90ma \x1b[0m%2d  \x1b[90mb \x1b[0m%2d\n", ins.x, ins.a, ins.b); break;
	}

	printf("\x1b[0m");
	return i;
}

