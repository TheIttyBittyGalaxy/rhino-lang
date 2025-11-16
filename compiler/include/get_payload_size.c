// This file was generated automatically by build_program/build.py

size_t get_playload_size(OpCode op)
{
	if (op == OP_CALL)
		return wordsizeof(Unit*);
	if (op == OP_LOAD_INT)
		return wordsizeof(int);
	if (op == OP_LOAD_NUM)
		return wordsizeof(double);
	if (op == OP_LOAD_STR)
		return wordsizeof(char*);

	return 0;
}
