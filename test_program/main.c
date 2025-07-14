#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char active_path[512];

char cmd[512];
size_t cmd_arg_start;

#define BUFFER_SIZE 2048
char buffer[BUFFER_SIZE];

void test_program_at_active_path(size_t active_path_len)
{
    // printf("%s\n", active_path);

    // Update system command
    {
        size_t c = cmd_arg_start;
        memcpy(cmd + c, active_path, active_path_len - 1);
        c += active_path_len - 1;
        cmd[c++] = ' ';
        cmd[c++] = '-';
        cmd[c++] = 't';
        cmd[c++] = 'e';
        cmd[c++] = 's';
        cmd[c++] = 't';
        cmd[c++] = '\0';
    }

    // Run the command
    printf("> %s\n", cmd);
    FILE *stream = popen(cmd, "r");

    if (!stream)
    {
        printf("ERROR: Unable to run command\n");
        return;
    }

    while (!feof(stream))
    {
        if (fgets(buffer, BUFFER_SIZE, stream) != NULL)
            printf("%s", buffer);
    }

    pclose(stream);
    printf("\n");
}

void scan_directory(DIR *dir, size_t path_len)
{
    // Check directory is valid
    if (dir == NULL)
    {
        printf("ERROR: Could not scan directory %s\n", active_path);
        return;
    }

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
    memcpy(cmd, exe_path, exe_path_len);
    cmd[exe_path_len] = ' ';
    cmd_arg_start = exe_path_len + 1;

    // Prepare active path
    memcpy(active_path, test_path, test_path_len + 1);

    // Scan tests directory
    DIR *dir = opendir(active_path);
    scan_directory(dir, test_path_len + 1);
    closedir(dir);

    return 0;
}