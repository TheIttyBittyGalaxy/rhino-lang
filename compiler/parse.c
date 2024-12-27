#include "parse.h"

// Parser state
typedef struct
{
    TokenArray token_array;
    size_t next_token;
} Parser;

// Parse utility
bool peek(Parser *p, TokenKind token_kind)
{
    return p->token_array.tokens[p->next_token].kind == token_kind;
}

void advance(Parser *p)
{
    if (!peek(p, END_OF_FILE))
        p->next_token++;
}

void eat(Parser *p, TokenKind token_kind)
{
    if (peek(p, token_kind))
    {
        advance(p);
    }
    else
    {
        // TODO: Error
    }
}

substr token_string(Parser *p)
{
    return p->token_array.tokens[p->next_token].str;
}

// Macros
#define ADVANCE() advance(p)
#define PEEK(token_kind) peek(p, token_kind)
#define EAT(token_kind) eat(p, token_kind)
#define TOKEN_STRING() token_string(p)

#define EXPRESSION(index) get_expression(apm->expression, index)
#define STATEMENT(index) get_statement(apm->statement, index)
#define FUNCTION(index) get_function(apm->function, index)

// Parse APM
size_t parse_expression(Parser *p, Program *apm)
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

    // TODO: Error

    size_t expr = add_expression(&apm->expression);
    EXPRESSION(expr)->kind = INVALID_EXPRESSION;

    ADVANCE();
    return expr;
}

bool peek_statement(Parser *p)
{
    return PEEK(CURLY_L) ||
           PEEK(COLON) ||
           PEEK(KEYWORD_IF) ||
           PEEK(ARROW_R);
}

size_t parse_code_block(Parser *p, Program *apm, bool allow_single);

size_t parse_statement(Parser *p, Program *apm)
{
    if (PEEK(CURLY_L) || PEEK(COLON))
    {
        return parse_code_block(p, apm, false);
    }

    size_t stmt = add_statement(&apm->statement);

    if (PEEK(KEYWORD_IF)) // IF_STATEMENT
    {
        EAT(KEYWORD_IF);
        size_t condition = parse_expression(p, apm);
        size_t body = parse_code_block(p, apm, true);

        // TODO: Parse else blocks

        STATEMENT(stmt)->kind = IF_STATEMENT;
        STATEMENT(stmt)->condition = condition;
        STATEMENT(stmt)->body = body;
    }

    else if (PEEK(ARROW_R)) // OUTPUT_STATEMENT
    {
        EAT(ARROW_R);
        size_t value = parse_expression(p, apm);
        EAT(SEMI_COLON);

        STATEMENT(stmt)->kind = OUTPUT_STATEMENT;
        STATEMENT(stmt)->value = value;
    }

    else // INVALID_STATEMENT
    {
        // TODO: Error

        ADVANCE();

        STATEMENT(stmt)->kind = INVALID_STATEMENT;
    }

    return stmt;
}

size_t parse_code_block(Parser *p, Program *apm, bool allow_single)
{
    size_t code_block = add_statement(&apm->statement);
    size_t first_statement = apm->statement.count;
    bool is_single = allow_single && PEEK(COLON);

    if (is_single)
    {
        EAT(COLON);
        parse_statement(p, apm);
    }
    else
    {
        EAT(CURLY_L);
        while (peek_statement(p))
            parse_statement(p, apm);
        EAT(CURLY_R);
    }

    size_t statement_count = apm->statement.count - first_statement;

    STATEMENT(code_block)->kind = is_single ? SINGLE_BLOCK : CODE_BLOCK;
    STATEMENT(code_block)->statements.first = first_statement;
    STATEMENT(code_block)->statements.count = statement_count;

    return code_block;
}

void parse_function(Parser *p, Program *apm)
{
    substr identity = TOKEN_STRING();
    EAT(IDENTITY);
    EAT(PAREN_L);
    EAT(PAREN_R);

    size_t body = parse_code_block(p, apm, true);

    size_t funct = add_function(&apm->function);
    FUNCTION(funct)->identity = identity;
    FUNCTION(funct)->body = body;
}

void parse_program(Parser *p, Program *apm)
{
    while (true)
    {
        if (PEEK(END_OF_FILE))
            break;
        else if (PEEK(IDENTITY))
            parse_function(p, apm);
        else
        {
            // TODO: Error
        }
    }
}

void parse(Program *apm, TokenArray token_array)
{
    Parser parser;
    parser.token_array = token_array;
    parser.next_token = 0;
    parse_program(&parser, apm);
}