// This file was generated automatically by build_program/build.py

void patch_unit_ptr_payload(Unit* unit, size_t location, Unit* p)
{
	union
	{
		Unit* data;
		uint32_t word[wordsizeof(Unit*)];
	} as = {.data = p};
	for (size_t i = 0; i < wordsizeof(Unit*); i++)
		unit->instruction[location + i].word = as.word[i];
}

void patch_double_payload(Unit* unit, size_t location, double p)
{
	union
	{
		double data;
		uint32_t word[wordsizeof(double)];
	} as = {.data = p};
	for (size_t i = 0; i < wordsizeof(double); i++)
		unit->instruction[location + i].word = as.word[i];
}

void patch_char_ptr_payload(Unit* unit, size_t location, char* p)
{
	union
	{
		char* data;
		uint32_t word[wordsizeof(char*)];
	} as = {.data = p};
	for (size_t i = 0; i < wordsizeof(char*); i++)
		unit->instruction[location + i].word = as.word[i];
}

