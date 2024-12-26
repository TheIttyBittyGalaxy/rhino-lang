#include "tokenise.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// OUTPUT MARCOS //

#define HEADING(text) printf("\x1b[32m" text "\n\x1b[0m")

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

int main(int argc, char *argv[])
{
    // Parse and validate arguments
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s <file_path> [-debug]\n", argv[0]);
        return EXIT_FAILURE;
    }

    for (int i = 2; i < argc; i++)
    {
        if (strcmp(argv[i], "-debug") == 0)
            flag_debug = true;
        else
        {
            fprintf(stderr, "Usage: %s <file_path> [-debug]\n", argv[0]);
            return EXIT_FAILURE;
        }
    }

    HEADING("Reading source file");
    const char *source_path = argv[1];
    char *source_text = read_file(source_path);
    if (source_text == NULL)
        return EXIT_FAILURE;

    HEADING("Tokenise");
    TokenArray token_array = tokenise(source_text);
    if (flag_debug)
    {
        for (size_t i = 0; i < token_array.count; i++)
        {
            Token t = token_array.tokens[i];
            printf("%03d\t%-*s\t%3d %2d\t%.*s\n", i, 13, token_kind_string(t.kind), t.pos, t.len, t.len, source_text + t.pos);
        }
    }

    // HEADING("Parse");
    // Program *apm = create_apm();
    // parse(apm, tokens);
    // if (flag_debug)
    //     print_apm(apm);

    // HEADING("Resolve");
    // resolve(apm);
    // if (flag_debug)
    //     print_apm(apm);

    // HEADING("Check");
    // resolve(apm);
    // if (flag_debug)
    //     print_apm(apm);

    // if (errors)
    // {
    //     HEADING("Errors");
    //     print_errors();
    //     return EXIT_SUCCESS;
    // }

    // HEADING("Running program");
    // interpret(apm);
}