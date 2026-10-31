package rhino

import "core:flags"
import "core:fmt"
import "core:os"

Options :: struct {
	file_path:    string `args:"pos=0,required" usage:"File path to parse"`,
	compile_only: bool `usage:"If true, the interpreter will not run"`,
}

main :: proc() {
	options: Options = ---
	flags.parse_or_exit(&options, os.args)

	source_text, read_err := os.read_entire_file(options.file_path, context.allocator)
	if read_err != nil {
		fmt.eprintfln("Could not load file %s %v", options.file_path, read_err)
		return
	}
	defer delete(source_text)

	fmt.println("Tokenising")
	tokenise(string(source_text))

	fmt.println("Parsing")
	parse()

	if (options.compile_only) do return

	fmt.println("Interpreting")
	interpret()
}
