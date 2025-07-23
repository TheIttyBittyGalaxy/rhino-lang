#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
    FILE *read_stream = fopen("compiler/rhino/runtime.c", "r");
    if (read_stream == NULL)
    {
        printf("Could not open compiler/rhino/runtime.c for reading");
        return EXIT_FAILURE;
    }

    FILE *write_stream = fopen("compiler/rhino/runtime_as_str.c", "w");
    if (write_stream == NULL)
    {
        pclose(read_stream);
        printf("Could not open compiler/rhino/runtime_as_str.c for writing");
        return EXIT_FAILURE;
    }

    bool in_comment = false;

    fputc('"', write_stream);
    while (true)
    {
        char c = fgetc(read_stream);
        if (c == EOF)
            break;

        if (in_comment)
        {
            if (c == '\n')
                in_comment = false;
            continue;
        }

        if (c == '\n')
        {
            fputc('\\', write_stream);
            fputc('n', write_stream);
            fputc('"', write_stream);

            fputc('\n', write_stream);
            fputc('"', write_stream);
        }
        else if (c == '\\' || c == '%')
        {
            fputc(c, write_stream);
            fputc(c, write_stream);
        }
        else if (c == '/')
        {
            char n = fgetc(read_stream);
            if (n == '/')
            {
                in_comment = true;
            }
            else
            {
                fputc(c, write_stream);
                if (n == EOF)
                {
                    break;
                }

                if (n == '\n')
                {
                    fputc('\\', write_stream);
                    fputc('n', write_stream);
                    fputc('"', write_stream);

                    fputc('\n', write_stream);
                    fputc('"', write_stream);
                }
                else if (n == '\\' || n == '%')
                {
                    fputc(n, write_stream);
                    fputc(n, write_stream);
                }
                else
                {
                    fputc(n, write_stream);
                }
            }
        }
        else
        {
            fputc(c, write_stream);
        }
    }
    fputc('\\', write_stream);
    fputc('n', write_stream);
    fputc('"', write_stream);

    pclose(write_stream);
    pclose(read_stream);

    return EXIT_SUCCESS;
}