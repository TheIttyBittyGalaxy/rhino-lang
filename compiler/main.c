#include "data/compiler.h"

#include "tokenise.h"
#include "parse.h"
#include "analyse.h"
#include "transpile.h"

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

bool flag_test_mode = false;
bool flag_token_dump = false;
bool flag_parse_dump = false;
bool flag_analyse_dump = false;
bool flag_dump_tree = false;

bool process_arguments(int argc, char *argv[])
{
    if (argc < 2)
        return false;

    for (int i = 2; i < argc; i++)
    {
        if (strcmp(argv[i], "-test") == 0)
            flag_test_mode = true;
        else if ((strcmp(argv[i], "-t") == 0) || strcmp(argv[i], "-token") == 0)
            flag_token_dump = true;
        else if ((strcmp(argv[i], "-p") == 0) || strcmp(argv[i], "-parse") == 0)
            flag_parse_dump = true;
        else if ((strcmp(argv[i], "-a") == 0) || strcmp(argv[i], "-analyse") == 0)
            flag_analyse_dump = true;
        else if ((strcmp(argv[i], "-n") == 0) || strcmp(argv[i], "-nice") == 0)
            flag_dump_tree = true;
        else
            return false;
    }

    return true;
}

int main(int argc, char *argv[])
{
    // Arguments
    bool valid_arguments = process_arguments(argc, argv);
    if (!valid_arguments)
    {
        fprintf(stderr, "Usage: %s <file_path> [-test] [-token] [-parse] [-analyse] [-nice]\n", argv[0]);
        return EXIT_FAILURE;
    }

    // Test mode
    if (flag_test_mode)
    {
        Compiler compiler;
        init_compiler(&compiler);

        compiler.source_path = argv[1];
        compiler.source_text = read_file(compiler.source_path);
        if (compiler.source_text == NULL)
            return EXIT_FAILURE;

        tokenise(&compiler);

        Program apm;
        init_program(&apm);
        parse(&compiler, &apm);
        analyse(&compiler, &apm);

        if (compiler.error_count > 0)
        {
            printf("ERRORS\n");
            determine_error_positions(&compiler);
            for (size_t i = 0; i < compiler.error_count; i++)
            {
                CompilationError error = compiler.errors[i];
                printf("%s:%d:%d\n",
                       compilation_error_code_string(error.code),
                       error.line,
                       error.column);
            }

            return EXIT_FAILURE;
        }

        transpile(&compiler, &apm);

        printf("SUCCESS\n");

        return EXIT_SUCCESS;
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
    if (flag_token_dump)
    {
        for (size_t i = 0; i < compiler.token_count; i++)
        {
            Token t = compiler.tokens[i];
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
    if (flag_parse_dump)
    {
        if (flag_dump_tree)
            print_parsed_apm(&apm, compiler.source_text);
        else
            dump_apm(&apm, compiler.source_text);
    }

    HEADING("Analyse");
    analyse(&compiler, &apm);
    if (flag_analyse_dump)
    {
        if (flag_dump_tree)
            print_analysed_apm(&apm, compiler.source_text);
        else
            dump_apm(&apm, compiler.source_text);
    }

    // Report errors
    if (compiler.error_count > 0)
    {
        HEADING("Errors");
        determine_error_positions(&compiler);
        for (size_t i = 0; i < compiler.error_count; i++)
            printf_compilation_error(&compiler, i);
        return EXIT_FAILURE;
    }

    // Transpile
    HEADING("Transpile");
    transpile(&compiler, &apm);

    HEADING("Complete");
    return EXIT_SUCCESS;
}