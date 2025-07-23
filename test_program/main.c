#include <dirent.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// STRING BUFFERS //

char active_path[512];

char rhino_cmd[512];
size_t rhino_cmd_arg_start;

// STRING UTILITY METHODS //

bool is_comment(char *str)
{
    return str[0] == '/' && str[1] == '/';
}

void strip_newline(char *str)
{
    size_t n = strlen(str);
    if (str[n - 1] == '\n')
        str[n - 1] = '\0';
}

// RESULTS //

typedef enum
{
    INVALID,
    NOT_FOUND,
    SUCCESS,
    ERRORS,
    FATAL_ERROR
} Outcome;

const char *outcome_string(Outcome result)
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

typedef struct
{
    Outcome outcome;
    char output[64][512];
    size_t output_count;
} Results;

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
    Results program_result;
    program_result.outcome = INVALID;
    program_result.output_count = 0;

    Results expected_result;
    expected_result.outcome = INVALID;
    expected_result.output_count = 0;

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
    char outcome_buffer[1024];
    if (fgets(outcome_buffer, sizeof(outcome_buffer), rhino_cmd_stream) != NULL)
    {
        if (outcome_buffer[0] == 'S')
            program_result.outcome = SUCCESS;
        else if (outcome_buffer[0] == 'E')
            program_result.outcome = ERRORS;
        else if (outcome_buffer[0] == 'F')
            program_result.outcome = FATAL_ERROR;
    }

    // Get outputs
    if (program_result.outcome == SUCCESS)
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
                if (fgets(program_result.output[i], sizeof(program_result.output[i]), run_cmd_stream) == NULL)
                {
                    program_result.output_count = i;
                    break;
                }

                strip_newline(program_result.output[i]);
            }

            pclose(run_cmd_stream);
        }
        else
        {
            program_result.outcome = FATAL_ERROR;
            program_result.output_count = 1;
            strcpy(program_result.output[0], "Error during compilation of the C program.");
        }
    }
    else if (program_result.outcome == ERRORS || program_result.outcome == FATAL_ERROR)
    {
        for (size_t i = 0; i < 64; i++)
        {
            if (fgets(program_result.output[i], sizeof(program_result.output[i]), rhino_cmd_stream) == NULL)
            {
                program_result.output_count = i;
                break;
            }

            strip_newline(program_result.output[i]);
        }
    }

    pclose(rhino_cmd_stream);
    printf("\n");

    // Get program expectation
    FILE *source_file = fopen(active_path, "r");

    char line[512];
    while (fgets(line, sizeof(line), source_file))
    {
        if (!is_comment(line))
            continue;

        if (expected_result.outcome == INVALID)
        {
            if (line[3] == 'S')
            {
                expected_result.outcome = SUCCESS;
                continue;
            }

            if (line[3] == 'E')
            {
                expected_result.outcome = ERRORS;
                continue;
            }

            expected_result.outcome = NOT_FOUND;
        }

        strip_newline(line);
        strcpy(expected_result.output[expected_result.output_count], line + 3);
        expected_result.output_count++;
    }

    if (expected_result.outcome == INVALID)
        expected_result.outcome = NOT_FOUND;

    fclose(source_file);

    // Determine if test passed
    bool test_passed = false;
    if (program_result.outcome != expected_result.outcome)
        goto failed_test;

    if (program_result.output_count != expected_result.output_count)
        goto failed_test;

    for (size_t i = 0; i < program_result.output_count; i++)
    {
        if (strcmp(program_result.output[i], expected_result.output[i]) != 0)
            goto failed_test;
    }

    test_passed = true;

failed_test:

    // Output results to HTML
    OUTPUT("\n<tr>");
    OUTPUT("<td><a href=\"%s\">%s</a></td>", active_path, active_path);

    if (test_passed)
        OUTPUT("<td class=\"result\">YES</td>");
    else if (expected_result.outcome == NOT_FOUND)
        OUTPUT("<td class=\"result result-fail\"><a href=\"#F%03d\">NOT FOUND</a></td>", failed_test_id);
    else if (program_result.outcome == FATAL_ERROR)
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

        APPEND_OUTPUT("<b>%s</b>", outcome_string(expected_result.outcome));
        for (size_t i = 0; i < expected_result.output_count; i++)
        {
            APPEND_OUTPUT("<br>");
            APPEND_OUTPUT(expected_result.output[i]);
        }

        APPEND_OUTPUT("</td><td>");

        APPEND_OUTPUT("<b>%s</b>", outcome_string(program_result.outcome));
        for (size_t i = 0; i < program_result.output_count; i++)
        {
            APPEND_OUTPUT("<br>");
            APPEND_OUTPUT(program_result.output[i]);
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