local tests = {}

function scan_dir(dir_path)
    local cmd = [[dir "test]] .. dir_path .. [[" /b]]

    for file_name in io.popen(cmd .. " /a-d"):lines() do
        if file_name:sub(-4) == ".rhi" then
            local file_path = dir_path .. "/" .. file_name
            print(file_path)
            local _, _, exit_code = os.execute('main.exe "test' .. file_path .. '" -TEST')

            -- TODO: This should actually compare the result of each program to it's expected result
            --       and use that to determine if the test was successful or not.

            table.insert(tests, {
                file_path = file_path,
                result = exit_code == 0
            })
        end
    end

    for subdir_name in io.popen(cmd .. " /ad"):lines() do
        local subdir_path = dir_path .. "/" .. subdir_name
        scan_dir(subdir_path)
    end
end

scan_dir("")

local file = io.open("test-results.html", "w")
if not file then
    error("Could not open test-results.html")
end

file:write([[<!DOCTYPE html>
<html lang="en">

<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Test Results</title>
    <style>
        body {
            font-family: monospace;
            font-size: 120%;
        }

        table {
            border-collapse: collapse;
            background-color: #EBEBEB;
        }

        tr { border: 3px solid white; }
        th, td { padding: 0.3em; }

        tr:first-child {
            background-color: #373F51;
            color: white;
        }

        .result { text-align: center; }
        .result-fail { background-color: #DE1A1A; }
    </style>
</head>

<body>
    <table>
        <tr><th>Test</th><th>Pass</th></tr>
]])

for _, test in ipairs(tests) do
    file:write("<tr>")
    file:write("<td>" .. test.file_path .. "</td>")
    if test.result then
        file:write('<td class="result result-pass">PASS</td>')
    else
        file:write('<td class="result result-fail">FAIL</td>')
    end
    file:write("</tr>")
end

file:write("</table></body></html>")
file:close()
