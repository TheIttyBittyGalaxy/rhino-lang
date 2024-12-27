#include "parse.h"

// Parse utility
size_t current_pos(Compiler *c)
{
    if (c->next_token == 0)
        return 0;

    Token t = c->token_array.tokens[c->next_token - 1];
    return t.str.pos + t.str.len;
}

bool peek(Compiler *c, TokenKind token_kind)
{
    return c->token_array.tokens[c->next_token].kind == token_kind;
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
        advance(c);
    }
    else
    {
        CompilationErrorCode code = (CompilationErrorCode)((int)EXPECTED_INVALID_TOKEN + (int)token_kind);
        raise_compilation_error(c, code, current_pos(c));
    }
}

substr token_string(Compiler *c)
{
    return c->token_array.tokens[c->next_token].str;
}

// Macros
#define ADVANCE() advance(c)
#define PEEK(token_kind) peek(c, token_kind)
#define EAT(token_kind) eat(c, token_kind)
#define TOKEN_STRING() token_string(c)

#define EXPRESSION(index) get_expression(apm->expression, index)
#define STATEMENT(index) get_statement(apm->statement, index)
#define FUNCTION(index) get_function(apm->function, index)

// Parse APM
size_t parse_expression(Compiler *c, Program *apm)
{
    if (PEEK(KEYWORD_TRUE))
    {
        size_t expr = add_expression(&apm->expression);
        EXPRESSION(expr)->kind = BOOLEAN_LITERAL;
        EXPRESSION(expr)->bool_value = true;

        ADVANCE();
        return expr;
    }
    else if (PEEK(KEYWORD_FALSE))
    {
        size_t expr = add_expression(&apm->expression);
        EXPRESSION(expr)->kind = BOOLEAN_LITERAL;
        EXPRESSION(expr)->bool_value = false;

        ADVANCE();
        return expr;
    }
    else if (PEEK(STRING) || PEEK(BROKEN_STRING))
    {
        substr str = TOKEN_STRING();
        str.pos++;
        str.len -= PEEK(STRING) ? 2 : 1;

        size_t expr = add_expression(&apm->expression);
        EXPRESSION(expr)->kind = STRING_LITERAL;
        EXPRESSION(expr)->string_value = str;

        ADVANCE();
        return expr;
    }

    size_t expr = add_expression(&apm->expression);
    EXPRESSION(expr)->kind = INVALID_EXPRESSION;

    raise_compilation_error(c, EXPECTED_EXPRESSION, current_pos(c));

    ADVANCE();
    return expr;
}

bool peek_statement(Compiler *c)
{
    return PEEK(CURLY_L) ||
           PEEK(COLON) ||
           PEEK(KEYWORD_IF) ||
           PEEK(ARROW_R);
}

size_t parse_code_block(Compiler *c, Program *apm, bool allow_single);

size_t parse_statement(Compiler *c, Program *apm)
{
    if (PEEK(CURLY_L) || PEEK(COLON))
    {
        return parse_code_block(c, apm, false);
    }

    size_t stmt = add_statement(&apm->statement);

    if (PEEK(KEYWORD_IF)) // IF_STATEMENT
    {
        EAT(KEYWORD_IF);
        size_t condition = parse_expression(c, apm);
        size_t body = parse_code_block(c, apm, true);

        // TODO: Parse else blocks

        STATEMENT(stmt)->kind = IF_STATEMENT;
        STATEMENT(stmt)->condition = condition;
        STATEMENT(stmt)->body = body;
    }

    else if (PEEK(ARROW_R)) // OUTPUT_STATEMENT
    {
        EAT(ARROW_R);
        size_t value = parse_expression(c, apm);
        EAT(SEMI_COLON);

        STATEMENT(stmt)->kind = OUTPUT_STATEMENT;
        STATEMENT(stmt)->value = value;
    }

    else // INVALID_STATEMENT
    {
        STATEMENT(stmt)->kind = INVALID_STATEMENT;

        raise_compilation_error(c, EXPECTED_STATEMENT, current_pos(c));

        ADVANCE();
    }

    return stmt;
}

size_t parse_code_block(Compiler *c, Program *apm, bool allow_single)
{
    size_t code_block = add_statement(&apm->statement);
    size_t first_statement = apm->statement.count;
    bool is_single = allow_single && PEEK(COLON);

    if (is_single)
    {
        EAT(COLON);
        parse_statement(c, apm);
    }
    else
    {
        EAT(CURLY_L);
        while (peek_statement(c))
            parse_statement(c, apm);
        EAT(CURLY_R);
    }

    size_t statement_count = apm->statement.count - first_statement;

    STATEMENT(code_block)->kind = is_single ? SINGLE_BLOCK : CODE_BLOCK;
    STATEMENT(code_block)->statements.first = first_statement;
    STATEMENT(code_block)->statements.count = statement_count;

    return code_block;
}

void parse_function(Compiler *c, Program *apm)
{
    substr identity = TOKEN_STRING();
    EAT(IDENTITY);
    EAT(PAREN_L);
    EAT(PAREN_R);

    size_t body = parse_code_block(c, apm, true);

    size_t funct = add_function(&apm->function);
    FUNCTION(funct)->identity = identity;
    FUNCTION(funct)->body = body;
}

void parse_program(Compiler *c, Program *apm)
{
    while (true)
    {
        if (PEEK(END_OF_FILE))
            break;
        else if (PEEK(IDENTITY))
            parse_function(c, apm);
        else
        {
            raise_compilation_error(c, EXPECTED_FUNCTION, current_pos(c));
            ADVANCE();
        }
    }
}

void parse(Compiler *compiler, Program *apm)
{
    compiler->next_token = 0;
    parse_program(compiler, apm);
}