#include "core/core.h"

#include "data/compiler.h"

#include "tokenise.h"
#include "parse.h"
#include "resolve.h"
#include "check.h"
#include "assemble.h"
#include "interpret.h"

#include "debug/memmap.c"

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
bool flag_resolve_dump = false;
bool flag_byte_code_dump = false;
bool flag_memmap = false;

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
        else if ((strcmp(argv[i], "-r") == 0) || strcmp(argv[i], "-resolve") == 0)
            flag_resolve_dump = true;
        else if ((strcmp(argv[i], "-b") == 0) || strcmp(argv[i], "-byte") == 0)
            flag_byte_code_dump = true;
        else if ((strcmp(argv[i], "-memmap") == 0))
            flag_memmap = true;
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
        fprintf(stderr, "Usage: %s <file_path> [-test] [-token] [-parse] [-resolve] [-nice]\n", argv[0]);
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
        parse(&compiler, &apm);
        resolve(&compiler, &apm);
        check(&compiler, &apm);

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

        ByteCode byte_code;
        init_byte_code(&byte_code);
        assemble(&compiler, &apm, &byte_code);

        RunOnString output_buffer;
        init_run_on_string(&output_buffer, 1);
        interpret(&byte_code, &output_buffer);

        printf("SUCCESS\n");
        printf(output_buffer.str);

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
    parse(&compiler, &apm);
    if (flag_parse_dump)
        print_parsed_apm(&apm, compiler.source_text);

    HEADING("Resolve");
    resolve(&compiler, &apm);
    if (flag_resolve_dump)
        print_resolved_apm(&apm, compiler.source_text);

    HEADING("Check");
    check(&compiler, &apm);

    if (flag_memmap)
    {
        HEADING("Memmap");
        memmap(&apm, compiler.apm_allocator);
        HEADING("Complete");
        return EXIT_SUCCESS;
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

    HEADING("Asessble");
    ByteCode byte_code;
    init_byte_code(&byte_code);
    assemble(&compiler, &apm, &byte_code);
    if (flag_byte_code_dump)
        printf_byte_code(&byte_code);

    HEADING("Interpret");
    interpret(&byte_code, NULL);

    HEADING("Complete");
    return EXIT_SUCCESS;
}