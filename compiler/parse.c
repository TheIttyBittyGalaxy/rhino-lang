#include "data/substr.h"
#include "parse.h"

// FORWARD DECLARATIONS //

// Parse utility
void raise_parse_error(Compiler *c, CompilationErrorCode code);
void recover_from_panic(Compiler *c);
bool peek(Compiler *c, TokenKind token_kind);
void advance(Compiler *c);
void eat(Compiler *c, TokenKind token_kind);
substr token_string(Compiler *c);
void attempt_to_recover_at_next_code_block(Compiler *c);

// Parse APM
bool peek_expression(Compiler *c);
size_t parse_expression(Compiler *c, Program *apm);
bool peek_statement(Compiler *c);
size_t parse_statement(Compiler *c, Program *apm);
size_t parse_code_block(Compiler *c, Program *apm, bool allow_single);
void parse_function(Compiler *c, Program *apm);
void parse_program(Compiler *c, Program *apm);
void parse(Compiler *compiler, Program *apm);

// MACROS //

#define ADVANCE() advance(c)
#define PEEK(token_kind) peek(c, token_kind)
#define EAT(token_kind) eat(c, token_kind)
#define TOKEN_STRING() token_string(c)
#define RAISE_PARSE_ERROR(code) raise_parse_error(c, code)

#define EXPRESSION(index) get_expression(apm->expression, index)
#define STATEMENT(index) get_statement(apm->statement, index)
#define FUNCTION(index) get_function(apm->function, index)

// PARSE UTILITY //

void raise_parse_error(Compiler *c, CompilationErrorCode code)
{
    if (c->parse_status == OKAY)
    {
        size_t pos = 0;
        if (c->next_token > 0)
        {
            Token t = c->tokens[c->next_token - 1];
            pos = t.str.pos + t.str.len;
        }
        raise_compilation_error(c, code, pos);
    }

    c->parse_status = PANIC;
}

void recover_from_panic(Compiler *c)
{
    if (c->parse_status == PANIC)
        c->parse_status = RECOVERED;
}

bool peek(Compiler *c, TokenKind token_kind)
{
    return c->tokens[c->next_token].kind == token_kind;
}

void advance(Compiler *c)
{
    if (!peek(c, END_OF_FILE))
        c->next_token++;
}

void eat(Compiler *c, TokenKind token_kind)
{
    if (peek(c, token_kind))
    {
        c->parse_status = OKAY;
        advance(c);
    }
    else
    {
        raise_parse_error(c, (CompilationErrorCode)((int)EXPECTED_INVALID_TOKEN + (int)token_kind));
    }
}

substr token_string(Compiler *c)
{
    return c->tokens[c->next_token].str;
}

void attempt_to_recover_at_next_code_block(Compiler *c)
{
    if (c->parse_status != PANIC)
        return;

    size_t start = c->next_token;
    while (!peek(c, END_OF_FILE))
    {
        if (peek(c, CURLY_L))
        {
            c->parse_status = OKAY;
            return;
        }

        else if (peek(c, COLON))
        {
            advance(c);
            if (peek_statement(c))
            {
                c->parse_status = OKAY;
                c->next_token--; // Leave COLON to be consumed by parse_code_block
                return;
            }
        }

        else if (peek(c, SEMI_COLON))
            break;

        else
            advance(c);
    }
    c->next_token = start;
}

// PARSE APM //

bool peek_expression(Compiler *c)
{
    return PEEK(IDENTITY) ||
           PEEK(KEYWORD_TRUE) ||
           PEEK(KEYWORD_FALSE) ||
           PEEK(STRING) ||
           PEEK(BROKEN_STRING);
}

// TODO: Check that call calls to parse_expression are accounted for
// NOTE: Can return with status OKAY, RECOVERED, or PANIC
size_t parse_expression(Compiler *c, Program *apm)
{
    size_t expr = add_expression(&apm->expression);

    // Left-hand side of expression
    if (PEEK(IDENTITY))
    {
        EXPRESSION(expr)->kind = IDENTITY_LITERAL;
        EXPRESSION(expr)->identity = TOKEN_STRING();
        ADVANCE();
    }
    else if (PEEK(KEYWORD_TRUE))
    {
        EXPRESSION(expr)->kind = BOOLEAN_LITERAL;
        EXPRESSION(expr)->bool_value = true;
        ADVANCE();
    }
    else if (PEEK(KEYWORD_FALSE))
    {
        EXPRESSION(expr)->kind = BOOLEAN_LITERAL;
        EXPRESSION(expr)->bool_value = false;
        ADVANCE();
    }
    else if (PEEK(STRING) || PEEK(BROKEN_STRING))
    {
        substr str = TOKEN_STRING();
        str.pos++;
        str.len -= PEEK(STRING) ? 2 : 1;

        EXPRESSION(expr)->kind = STRING_LITERAL;
        EXPRESSION(expr)->string_value = str;
        ADVANCE();
    }
    else
    {
        EXPRESSION(expr)->kind = INVALID_EXPRESSION;
        RAISE_PARSE_ERROR(EXPECTED_EXPRESSION);

        // NOTE: parse_expression can put the parser in panic mode and then return.
        //       This means anyone who calls parse_expression needs to be able to recover.
        return expr;
    }

    // Function call
    if (PEEK(PAREN_L))
    {
        size_t callee = expr;
        expr = add_expression(&apm->expression);
        EXPRESSION(expr)->kind = FUNCTION_CALL;
        EXPRESSION(expr)->callee = callee;

        ADVANCE();
        EAT(PAREN_R);
        recover_from_panic(c);
    }

    return expr;
}

bool peek_statement(Compiler *c)
{
    return PEEK(CURLY_L) ||
           PEEK(COLON) ||
           PEEK(KEYWORD_IF) ||
           PEEK(ARROW_R) ||
           peek_expression(c);
}

// NOTE: Can return with status OKAY or RECOVERED
size_t parse_statement(Compiler *c, Program *apm)
{
    if (PEEK(CURLY_L) || PEEK(COLON))
    {
        return parse_code_block(c, apm, false); // Can return with status OKAY or RECOVERED
    }

    size_t stmt = add_statement(&apm->statement);

    if (PEEK(KEYWORD_IF)) // IF_STATEMENT
    {
        STATEMENT(stmt)->kind = IF_STATEMENT;

        EAT(KEYWORD_IF);
        size_t condition = parse_expression(c, apm);
        STATEMENT(stmt)->condition = condition;

        attempt_to_recover_at_next_code_block(c);
        if (c->parse_status == PANIC)
            goto recover;

        size_t body = parse_code_block(c, apm, true);
        STATEMENT(stmt)->body = body;

        while (PEEK(KEYWORD_ELSE))
        {
            EAT(KEYWORD_ELSE);
            size_t segment_stmt = add_statement(&apm->statement);

            if (PEEK(KEYWORD_IF))
            {
                EAT(KEYWORD_IF);

                STATEMENT(segment_stmt)->kind = ELSE_IF_STATEMENT;

                size_t condition = parse_expression(c, apm);
                STATEMENT(segment_stmt)->condition = condition;

                attempt_to_recover_at_next_code_block(c);
                if (c->parse_status == PANIC)
                    goto recover;

                size_t body = parse_code_block(c, apm, true);
                STATEMENT(segment_stmt)->body = body;
            }
            else
            {
                STATEMENT(segment_stmt)->kind = ELSE_STATEMENT;

                size_t body = parse_code_block(c, apm, true);
                STATEMENT(segment_stmt)->body = body;

                break;
            }
        }
    }

    else if (PEEK(ARROW_R)) // OUTPUT_STATEMENT
    {
        STATEMENT(stmt)->kind = OUTPUT_STATEMENT;

        EAT(ARROW_R);
        size_t value = parse_expression(c, apm);
        STATEMENT(stmt)->expression = value;
        if (c->parse_status == PANIC)
            goto recover;

        EAT(SEMI_COLON);
    }

    else if (peek_expression(c)) // EXPRESSION STATEMENT
    {
        STATEMENT(stmt)->kind = EXPRESSION_STMT;

        size_t value = parse_expression(c, apm);
        STATEMENT(stmt)->expression = value;
        if (c->parse_status == PANIC)
            goto recover;

        EAT(SEMI_COLON);
    }

    else // INVALID_STATEMENT
    {
        STATEMENT(stmt)->kind = INVALID_STATEMENT;
        RAISE_PARSE_ERROR(EXPECTED_STATEMENT);
    }

recover:
    while (c->parse_status == PANIC)
    {
        if (PEEK(END_OF_FILE))
        {
            c->parse_status = RECOVERED;
        }

        else if (PEEK(SEMI_COLON))
        {
            ADVANCE();
            c->parse_status = OKAY;
        }

        // TODO: If we stored where newlines appears in the program, something like this would allow for better recovery
        // else if (peek_newline) {
        //     c->parse_status = RECOVERED;
        // }

        else if (PEEK(CURLY_R))
        {
            ADVANCE();
            c->parse_status = OKAY;
        }

        else if (PEEK(CURLY_L))
        {
            ADVANCE();
            c->parse_status = RECOVERED;
        }

        else
        {
            ADVANCE();
        }
    }

    return stmt;
}

// NOTE: Can return with status OKAY or RECOVERED
size_t parse_code_block(Compiler *c, Program *apm, bool allow_single)
{
    size_t code_block = add_statement(&apm->statement);
    STATEMENT(code_block)->kind = CODE_BLOCK;

    size_t first_statement = apm->statement.count;
    STATEMENT(code_block)->statements.first = first_statement;

    if (PEEK(COLON))
    {
        EAT(COLON);
        STATEMENT(code_block)->kind = SINGLE_BLOCK;
        parse_statement(c, apm); // Can return with status OKAY or RECOVERED
    }
    else
    {
        EAT(CURLY_L);
        recover_from_panic(c);

        while (peek_statement(c))
            parse_statement(c, apm); // Can return with status OKAY or RECOVERED

        EAT(CURLY_R);
        recover_from_panic(c);
    }

    size_t statement_count = apm->statement.count - first_statement;
    STATEMENT(code_block)->statements.count = statement_count;

    return code_block;
}

// NOTE: Can return with status OKAY or RECOVERED
void parse_function(Compiler *c, Program *apm)
{
    size_t funct = add_function(&apm->function);

    EAT(KEYWORD_FN);

    substr identity = TOKEN_STRING();
    FUNCTION(funct)->identity = identity;
    EAT(IDENTITY);
    assert(c->parse_status == OKAY);

    EAT(PAREN_L);
    EAT(PAREN_R);

    attempt_to_recover_at_next_code_block(c);
    size_t body = parse_code_block(c, apm, true);
    FUNCTION(funct)->body = body;
}

void parse_program(Compiler *c, Program *apm)
{
    while (true)
    {
        if (PEEK(END_OF_FILE))
            return;
        else if (PEEK(KEYWORD_FN))
            parse_function(c, apm);
        else
        {
            RAISE_PARSE_ERROR(EXPECTED_FUNCTION);

            size_t depth = 0;
            while (c->parse_status == PANIC)
            {
                if (PEEK(END_OF_FILE))
                    return;

                else if (PEEK(CURLY_L))
                    depth++;

                else if (PEEK(CURLY_R) && (depth == 0 || --depth == 0))
                    c->parse_status = RECOVERED;

                ADVANCE();
            }
        }
    }
}

void parse(Compiler *compiler, Program *apm)
{
    compiler->next_token = 0;
    compiler->parse_status = OKAY;
    parse_program(compiler, apm);
}