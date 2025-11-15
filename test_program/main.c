#include <dirent.h>
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// TODO: Update the test program to work with the new interpreter (once it exists...)

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

bool results_match(Results a, Results b)
{
    if (a.outcome != b.outcome)
        return false;

    if (a.output_count != b.output_count)
        return false;

    for (size_t i = 0; i < a.output_count; i++)
        if (strcmp(a.output[i], b.output[i]) != 0)
            return false;

    return true;
}

// OUTPUT FILE //

typedef struct Page Page;

struct Page
{
    Page *next_page;
    char *content;
    size_t length;
};

Page *first_page;
Page *current_page;
size_t failed_test_id;

Page *create_page(size_t size)
{
    Page *page = (Page *)malloc(sizeof(Page));

    page->next_page = NULL;
    page->content = (char *)malloc(size);
    page->length = 0;

    page->content[0] = '\0';

    return page;
}

void write(Page *page, const char *str, ...)
{
    va_list args;
    va_start(args, str);
    int chars_used = vsprintf(page->content + page->length, str, args);
    va_end(args);

    // FIXME: If chars_used is negative, that tells us that an error occurred

    page->length += chars_used;
    page->content[page->length] = '\0';
}

// RUN COMMANDS //

typedef enum
{
    COMMAND_SUCCESS,
    COMMAND_FAIL,
    COULD_NOT_RUN_COMMAND
} CommandResult;

#define CHECK_CMD_RESULT(result)                      \
    if (result == COULD_NOT_RUN_COMMAND)              \
    {                                                 \
        printf("ERROR: Could not execute command\n"); \
        return;                                       \
    }

CommandResult run_rhino_compiler_cmd(Results *result, size_t active_path_len)
{
    // Update string
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
    FILE *stream = popen(rhino_cmd, "r");
    if (!stream)
        return COULD_NOT_RUN_COMMAND;

    // Determine the outcome
    char buffer[2048];
    if (fgets(buffer, sizeof(buffer), stream) != NULL)
    {
        if (buffer[0] == 'S')
            result->outcome = SUCCESS;
        else if (buffer[0] == 'E')
            result->outcome = ERRORS;
        else if (buffer[0] == 'F')
            result->outcome = FATAL_ERROR;
    }

    // Determine outputs
    if (result->outcome == SUCCESS)
    {
        pclose(stream);
        return COMMAND_SUCCESS;
    }

    size_t i = 0;
    while (fgets(result->output[i], sizeof(result->output[i]), stream))
    {
        strip_newline(result->output[i]);
        i++;
    }
    result->output_count = i;

    pclose(stream);
    return COMMAND_FAIL;
}

CommandResult run_c_compiler_cmd()
{
    // Run the command
    const char *cmd = "g++ -o _out.exe _out.c -w";
    printf("> %s\n", cmd);
    FILE *stream = popen(cmd, "r");

    if (!stream)
        return COULD_NOT_RUN_COMMAND;

    // Return success or fail depending on if G++ signaled there was a compilation error
    int build_status = pclose(stream);

    if (build_status == 0)
        return COMMAND_SUCCESS;

    return COMMAND_FAIL;
}

CommandResult run_compiled_test_cmd(Results *result)
{
    // Run the command
    const char *cmd = "_out.exe";
    printf("> %s\n", cmd);
    FILE *stream = popen(cmd, "r");

    if (!stream)
        return COULD_NOT_RUN_COMMAND;

    // Determine outputs
    size_t i = 0;
    while (fgets(result->output[i], sizeof(result->output[i]), stream))
    {
        strip_newline(result->output[i]);
        i++;
    }
    result->output_count = i;

    pclose(stream);
    return COMMAND_SUCCESS;
}

// DETERMINE EXPECTATION //

Results determine_expectation()
{
    Results result;
    result.outcome = NOT_FOUND;
    result.output_count = 0;

    char line[512];
    FILE *source_file = fopen(active_path, "r");

    while (fgets(line, sizeof(line), source_file))
    {
        if (!is_comment(line))
            continue;

        if (line[3] == 'S')
            result.outcome = SUCCESS;
        else if (line[3] == 'E')
            result.outcome = ERRORS;
        else
            goto treat_as_output;

        break;
    }

    while (fgets(line, sizeof(line), source_file))
    {
        if (!is_comment(line))
            continue;

    treat_as_output:
        strip_newline(line);
        strcpy(result.output[result.output_count], line + 3);
        result.output_count++;
    }

    fclose(source_file);
    return result;
}

// TEST PROGRAM //

void test_program_at_active_path(size_t active_path_len)
{
    // Determine expectation
    Results expected_result = determine_expectation();

    // Determine actual results
    Results actual_result;
    actual_result.outcome = INVALID;
    actual_result.output_count = 0;

    CommandResult rhino_build = run_rhino_compiler_cmd(&actual_result, active_path_len);
    CHECK_CMD_RESULT(rhino_build);

    if (rhino_build == COMMAND_SUCCESS)
    {
        CommandResult c_build = run_c_compiler_cmd();
        CHECK_CMD_RESULT(c_build);

        if (c_build == COMMAND_SUCCESS)
        {
            CommandResult compiled_test = run_compiled_test_cmd(&actual_result);
            CHECK_CMD_RESULT(compiled_test);
        }
        else
        {
            actual_result.outcome = FATAL_ERROR;
            actual_result.output_count = 1;
            strcpy(actual_result.output[0], "Error during compilation of the C program.");
        }
    }

    printf("\n");

    // Compare results
    bool test_passed = results_match(expected_result, actual_result);

    // Output results to HTML
    write(first_page, "\n<tr>");
    write(first_page, "<td><a href=\"%s\">%s</a></td>", active_path, active_path);

    if (test_passed)
        write(first_page, "<td class=\"result\">YES</td>");
    else if (expected_result.outcome == NOT_FOUND)
        write(first_page, "<td class=\"result result-fail\"><a href=\"#F%03d\">NOT FOUND</a></td>", failed_test_id);
    else if (actual_result.outcome == FATAL_ERROR)
        write(first_page, "<td class=\"result result-fail\"><a href=\"#F%03d\">FATAL</a></td>", failed_test_id);
    else
        write(first_page, "<td class=\"result result-fail\"><a href=\"#F%03d\">NO</a></td>", failed_test_id);

    write(first_page, "</tr>");

    if (!test_passed)
    {
        Page *error_info = create_page(4096);
        current_page->next_page = error_info;
        current_page = error_info;

        write(error_info, "\n<h2 id=\"F%03d\"><a href=\"%s\">%s</a></h2>", failed_test_id, active_path, active_path);

        write(error_info, "<table>");
        write(error_info, "<tr><th>Expected</th><th>Actual</th></tr>");
        write(error_info, "<tr><td>");

        write(error_info, "<b>%s</b>", outcome_string(expected_result.outcome));
        for (size_t i = 0; i < expected_result.output_count; i++)
        {
            write(error_info, "<br>");
            write(error_info, expected_result.output[i]);
        }

        write(error_info, "</td><td>");

        write(error_info, "<b>%s</b>", outcome_string(actual_result.outcome));
        for (size_t i = 0; i < actual_result.output_count; i++)
        {
            write(error_info, "<br>");
            write(error_info, actual_result.output[i]);
        }

        write(error_info, "</td></tr></table>");
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

    // Prepare pages
    first_page = create_page(65536);
    current_page = first_page;

    // Scan tests directory
    failed_test_id = 0;

    DIR *dir = opendir(active_path);
    scan_directory(dir, test_path_len + 1);
    closedir(dir);

    // End first page
    write(first_page, "</table>");

    // Generate output file
    FILE *output_file = fopen("test-results.html", "w");

    fprintf(output_file,
            "<!DOCTYPE html><html lang=\"en\"><head>"
            "<meta charset=\"UTF-8\">"
            "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">"
            "<title>Test Results</title>"
            "<style>"
            "body {"
            "background-color: black;"
            "font-family: monospace;"
            "font-size: 110%%;"
            "color: white;"
            "}"
            "table { border-collapse: collapse; }"
            "tr { border: 1px solid rgb(111, 195, 223); }"
            "th, td {"
            "padding: 0.3em;"
            "vertical-align: top;"
            "}"
            "tr:first-child {"
            "background-color: #373F51;"
            "color: white;"
            "}"
            ".result { text-align: center; }"
            ".result-fail { background-color: #373F51; }"
            "a { color: inherit; }"
            "h2 { color: rgb(215, 186, 125); }"
            "</style>"
            "</head><body>"
            "<table>"
            "<tr><th>Test</th><th>Passed</th></tr>");

    Page *p = first_page;
    while (p != NULL)
    {
        fprintf(output_file, p->content);
        p = p->next_page;
    }

    fprintf(output_file, "</body></html>");

    fclose(output_file);

    return 0;
}