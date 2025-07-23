#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// TODO: For each test program, scrape the comments for the expectation
//       and then compare the results with the expectation to determine
//       if the test has passed.

// STRING BUFFERS //

char active_path[512];

char rhino_cmd[512];
size_t rhino_cmd_arg_start;

#define RESULT_BUFFER_SIZE 2048
char result_buffer[RESULT_BUFFER_SIZE];

// COMPILER RESULT ENUM //

typedef enum
{
    INVALID,
    NOT_FOUND,
    SUCCESS,
    ERRORS,
    FATAL_ERROR
} CompilerResult;

const char *compiler_result_string(CompilerResult result)
{
    switch (result)
    {
    case NOT_FOUND:
        return "NOT FOUND";
    case SUCCESS:
        return "SUCCESS";
    case ERRORS:
        return "ERRORS";
    case FATAL_ERROR:
        return "FATAL ERROR";
    default:
        return "INVALID COMPILER OUTPUT";
    }
}

// OUTPUT FILE //

FILE *output_file;
size_t failed_test_id;

char output_append[65536];
size_t output_append_start;

#define OUTPUT(...) fprintf(output_file, __VA_ARGS__)
#define APPEND_OUTPUT(...) output_append_start += sprintf(output_append + output_append_start, __VA_ARGS__)

// TEST PROGRAM //

void test_program_at_active_path(size_t active_path_len)
{
    // Update system command
    {
        size_t c = rhino_cmd_arg_start;
        memcpy(rhino_cmd + c, active_path, active_path_len - 1);
        c += active_path_len - 1;
        rhino_cmd[c++] = ' ';
        rhino_cmd[c++] = '-';
        rhino_cmd[c++] = 't';
        rhino_cmd[c++] = 'e';
        rhino_cmd[c++] = 's';
        rhino_cmd[c++] = 't';
        rhino_cmd[c++] = '\0';
    }

    // Run the command
    printf("> %s\n", rhino_cmd);
    FILE *rhino_cmd_stream = popen(rhino_cmd, "r");

    if (!rhino_cmd_stream)
    {
        printf("ERROR: Unable to run command\n");
        return;
    }

    // Determine result
    CompilerResult compiler_result = INVALID;

    if (fgets(result_buffer, RESULT_BUFFER_SIZE, rhino_cmd_stream) != NULL)
    {
        if (result_buffer[0] == 'S')
            compiler_result = SUCCESS;
        else if (result_buffer[0] == 'E')
            compiler_result = ERRORS;
        else if (result_buffer[0] == 'F')
            compiler_result = FATAL_ERROR;
    }

    // Get outputs
    char output_buffer[64][512];
    size_t output_count = 0;

    if (compiler_result == SUCCESS)
    {
        // Run the build command
        const char *build_cmd = "g++ -o _out.exe _out.c -w";
        printf("> %s\n", build_cmd);
        FILE *build_cmd_stream = popen(build_cmd, "r");

        if (!build_cmd_stream)
        {
            printf("ERROR: Unable to run build command\n");
            return;
        }

        int build_status = pclose(build_cmd_stream);
        if (build_status == 0)
        {

            // Run the output
            const char *run_cmd = "_out.exe";
            printf("> %s\n", run_cmd);
            FILE *run_cmd_stream = popen(run_cmd, "r");

            if (!run_cmd_stream)
            {
                printf("ERROR: Unable to run output\n");
                return;
            }

            for (size_t i = 0; i < 64; i++)
            {
                if (fgets(output_buffer[i], sizeof(output_buffer[i]), run_cmd_stream) != NULL)
                {
                    size_t n = strlen(output_buffer[i]);
                    if (output_buffer[i][n - 1] == '\n')
                        output_buffer[i][n - 1] = '\0';
                }
                else
                {
                    output_count = i;
                    break;
                }
            }

            pclose(run_cmd_stream);
        }
        else
        {
            compiler_result = FATAL_ERROR;
            output_count = 1;
            strcpy(output_buffer[0], "Error during compilation of the C program.");
        }
    }
    else if (compiler_result == ERRORS || compiler_result == FATAL_ERROR)
    {
        for (size_t i = 0; i < 64; i++)
        {
            if (fgets(output_buffer[i], sizeof(output_buffer[i]), rhino_cmd_stream) != NULL)
            {
                size_t n = strlen(output_buffer[i]);
                if (output_buffer[i][n - 1] == '\n')
                    output_buffer[i][n - 1] = '\0';
            }
            else
            {
                output_count = i;
                break;
            }
        }
    }

    pclose(rhino_cmd_stream);
    printf("\n");

    // Get program expectation
    CompilerResult expected_compiler_result = INVALID;
    char expected_output_buffer[64][512];
    size_t expected_output_count = 0;
    FILE *source_file = fopen(active_path, "r");

    char line[512];
    while (fgets(line, sizeof(line), source_file))
    {
        if (line[0] != '/' || line[1] != '/')
            continue;

        if (expected_compiler_result == INVALID)
        {
            if (line[3] == 'S')
            {
                expected_compiler_result = SUCCESS;
                continue;
            }

            if (line[3] == 'E')
            {
                expected_compiler_result = ERRORS;
                continue;
            }

            expected_compiler_result = NOT_FOUND;
        }

        size_t n = strlen(line);
        if (line[n - 1] == '\n')
            line[n - 1] = '\0';

        strcpy(expected_output_buffer[expected_output_count], line + 3);

        expected_output_count++;
    }

    if (expected_compiler_result == INVALID)
        expected_compiler_result = NOT_FOUND;

    fclose(source_file);

    // Determine if test passed
    bool test_passed = false;
    if (compiler_result != expected_compiler_result)
        goto failed_test;

    if (output_count != expected_output_count)
        goto failed_test;

    for (size_t i = 0; i < output_count; i++)
    {
        if (strcmp(output_buffer[i], expected_output_buffer[i]) != 0)
            goto failed_test;
    }

    test_passed = true;

failed_test:

    // Output results to HTML
    OUTPUT("\n<tr>");
    OUTPUT("<td><a href=\"%s\">%s</a></td>", active_path, active_path);

    if (test_passed)
        OUTPUT("<td class=\"result\">YES</td>");
    else if (expected_compiler_result == NOT_FOUND)
        OUTPUT("<td class=\"result result-fail\"><a href=\"#F%03d\">NOT FOUND</a></td>", failed_test_id);
    else if (compiler_result == FATAL_ERROR)
        OUTPUT("<td class=\"result result-fail\"><a href=\"#F%03d\">FATAL</a></td>", failed_test_id);
    else
        OUTPUT("<td class=\"result result-fail\"><a href=\"#F%03d\">NO</a></td>", failed_test_id);

    OUTPUT("</tr>");

    if (!test_passed)
    {
        APPEND_OUTPUT("\n<h2 id=\"F%03d\"><a href=\"%s\">%s</a></h2>", failed_test_id, active_path, active_path);

        APPEND_OUTPUT("<table>");
        APPEND_OUTPUT("<tr><th>Expected</th><th>Actual</th></tr>");
        APPEND_OUTPUT("<tr><td>");

        APPEND_OUTPUT("<b>%s</b>", compiler_result_string(expected_compiler_result));
        for (size_t i = 0; i < expected_output_count; i++)
        {
            APPEND_OUTPUT("<br>");
            APPEND_OUTPUT(expected_output_buffer[i]);
        }

        APPEND_OUTPUT("</td><td>");

        APPEND_OUTPUT("<b>%s</b>", compiler_result_string(compiler_result));
        for (size_t i = 0; i < output_count; i++)
        {
            APPEND_OUTPUT("<br>");
            APPEND_OUTPUT(output_buffer[i]);
        }

        APPEND_OUTPUT("</td></tr></table>");
        failed_test_id++;
    }
}

// SCAN DIRECTORY //

void scan_directory(DIR *dir, size_t path_len)
{
    // Check directory is valid
    if (dir == NULL)
    {
        printf("ERROR: Could not scan directory %s\n", active_path);
        return;
    }

    // Output to table
    bool outputted_table_row = false;

    // Extend active path ready for item paths
    path_len--;
    active_path[path_len++] = '\\';
    active_path[path_len++] = '\0';

    // For each item
    struct dirent *item;
    while ((item = readdir(dir)) != NULL)
    {
        // Ignore "." and ".."
        char *item_name = item->d_name;
        if (item_name[0] == '.')
            continue;

        // Update active path to item path
        size_t subpath_len = path_len - 1;
        {
            size_t c = 0;
            while (item_name[c] != '\0')
                active_path[subpath_len++] = item_name[c++];
            active_path[subpath_len++] = '\0';
        }

        // Open item
        DIR *subdir = opendir(active_path);
        if (subdir)
        {
            scan_directory(subdir, subpath_len);
            closedir(subdir);
        }
        else if (errno == ENOTDIR)
        {
            test_program_at_active_path(subpath_len);
        }
        else
        {
            printf("ERROR: Could not scan file %s %s\n", active_path, strerror(errno));
        }
    }
}

// MAIN //

int main(int argc, char *argv[])
{
    // Process arguments
    if (argc != 3)
    {
        fprintf(stderr, "Usage: %s <executable_path> <test_directory>\n", argv[0]);
        return EXIT_FAILURE;
    }

    char *exe_path = argv[1];
    size_t exe_path_len = 0;
    while (exe_path[++exe_path_len] != '\0')
        ;

    char *test_path = argv[2];
    size_t test_path_len = 0;
    while (test_path[++test_path_len] != '\0')
        ;

    // Prepare system command
    memcpy(rhino_cmd, exe_path, exe_path_len);
    rhino_cmd[exe_path_len] = ' ';
    rhino_cmd_arg_start = exe_path_len + 1;

    // Prepare active path
    memcpy(active_path, test_path, test_path_len + 1);

    // Open output file
    output_file = fopen("test-results.html", "w");
    output_append_start = 0;

    failed_test_id = 0;

    OUTPUT("<!DOCTYPE html><html lang=\"en\"><head>"
           "<meta charset=\"UTF-8\">"
           "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">"
           "<title>Test Results</title>"
           "<style>"
           "body {"
           "font-family: monospace;"
           "font-size: 120%%;"
           "}"
           "table {"
           "border-collapse: collapse;"
           "background-color: #EBEBEB;"
           "}"
           "tr { border: 3px solid white; }"
           "th, td {"
           "padding: 0.3em;"
           "vertical-align: top;"
           "}"
           "tr:first-child {"
           "background-color: #373F51;"
           "color: white;"
           "}"
           ".result { text-align: center; }"
           ".result-fail { background-color: #FF4BD2; }"
           ".result-fail a { color: white; }"
           "</style>"
           "</head><body>");

    OUTPUT("<table>");
    OUTPUT("<tr><th>Test</th><th>Passed</th></tr>");

    // Scan tests directory
    DIR *dir = opendir(active_path);
    scan_directory(dir, test_path_len + 1);
    closedir(dir);

    // Close the output file
    APPEND_OUTPUT("\0");

    OUTPUT("</table>");

    output_append[output_append_start] = '\0';
    OUTPUT(output_append);

    OUTPUT("</body></html>");

    fclose(output_file);

    return 0;
}