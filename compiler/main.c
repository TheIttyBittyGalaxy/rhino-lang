#include "data/compiler.h"
#include "interpret.h"
#include "parse.h"
#include "tokenise.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// OUTPUT MARCOS //

#define HEADING(text) printf("\n\x1b[32m" text "\n\x1b[0m")

// READ FILE //

char *read_file(const char *path)
{
    FILE *handle = fopen(path, "rb");

    if (handle == NULL)
    {
        fprintf(stderr, "Error reading file %s\n", path);
        return NULL;
    }

    fseek(handle, 0, SEEK_END);
    long file_size = ftell(handle);

    char *file_text = (char *)malloc(file_size + 1);
    if (file_text == NULL)
    {
        fclose(handle);
        fprintf(stderr, "Unable to allocate memory to store source text\n");
        return NULL;
    }

    fseek(handle, 0, SEEK_SET);
    fread(file_text, 1, file_size, handle);
    file_text[file_size] = '\0';

    fclose(handle);

    return file_text;
}

// MAIN //
bool flag_debug = false;
bool flag_tree = false;

int main(int argc, char *argv[])
{
    // Parse and validate arguments
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s <file_path> [-debug] [-tree]\n", argv[0]);
        return EXIT_FAILURE;
    }

    for (int i = 2; i < argc; i++)
    {
        if (strcmp(argv[i], "-debug") == 0)
            flag_debug = true;
        else if (strcmp(argv[i], "-tree") == 0)
            flag_tree = true;
        else
        {
            fprintf(stderr, "Usage: %s <file_path> [-debug]\n", argv[0]);
            return EXIT_FAILURE;
        }
    }

    // Compile
    Compiler compiler;
    init_compiler(&compiler);

    HEADING("Reading source file");
    compiler.source_path = argv[1];
    compiler.source_text = read_file(compiler.source_path);
    if (compiler.source_text == NULL)
        return EXIT_FAILURE;

    HEADING("Tokenise");
    tokenise(&compiler);
    if (flag_debug)
    {
        for (size_t i = 0; i < compiler.token_array.count; i++)
        {
            Token t = compiler.token_array.tokens[i];
            printf("%03d\t", i);
            printf("%-*s\t", 13, token_kind_string(t.kind));
            printf("%3d %2d\t", t.str.pos, t.str.len);
            printf("%.*s\n", t.str.len, compiler.source_text + t.str.pos);
        }
    }

    HEADING("Parse");
    Program apm;
    init_program(&apm);
    parse(&compiler, &apm);
    if (flag_tree)
        print_apm(&apm, compiler.source_text);
    else if (flag_debug)
        dump_apm(&apm, compiler.source_text);

    // HEADING("Resolve");
    // resolve(apm);
    // if (flag_debug)
    //     print_apm(apm);

    // HEADING("Check");
    // resolve(apm);
    // if (flag_debug)
    //     print_apm(apm);

    // Report errors
    if (compiler.error_count > 0)
    {
        HEADING("Errors");
        determine_error_positions(&compiler);
        for (size_t i = 0; i < compiler.error_count; i++)
        {
            CompilationError error = compiler.errors[i];
            printf("%s:%d:%d\t%s\n", compiler.source_path, error.line, error.column, compilation_error_code_string(error.code));
        }
        return EXIT_FAILURE;
    }

    // Interpret
    HEADING("Running program");
    interpret(&apm, compiler.source_text);

    return EXIT_SUCCESS;
}