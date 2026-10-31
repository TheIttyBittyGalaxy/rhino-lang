package rhino

import "core:fmt"

TokenKind :: enum {
	KEYWORD_PROCEDURE,
	IDENTITY,
	STRING_LITERAL,
	SEMI_COLON,
	BRACKET_L,
	BRACKET_R,
	CURLY_L,
	CURLY_R,
}

Token :: struct {
	pos: int,
	len: int,
}

tokenise :: proc(source_text: string) {
	for c, i in source_text {
		fmt.println(i, c)
	}
}
