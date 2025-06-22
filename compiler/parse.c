#include "data/substr.h"
#include "parse.h"

// FORWARD DECLARATIONS //

// Token consumption
bool peek(Compiler *c, TokenKind token_kind);
bool peek_next(Compiler *c, TokenKind token_kind);
void advance(Compiler *c);
void eat(Compiler *c, TokenKind token_kind);
substr token_string(Compiler *c);

// Error and recovery
void raise_parse_error(Compiler *c, CompilationErrorCode code);
void recover_from_panic(Compiler *c);
void attempt_to_recover_at_next_code_block(Compiler *c);

// Peek APM
bool peek_expression(Compiler *c);
bool peek_statement(Compiler *c);

// Parse APM
void parse(Compiler *compiler, Program *apm);
void parse_program(Compiler *c, Program *apm);
void parse_function(Compiler *c, Program *apm);
size_t parse_statement(Compiler *c, Program *apm);
size_t parse_code_block(Compiler *c, Program *apm);
size_t parse_expression(Compiler *c, Program *apm);

// MACROS //

#define PEEK(token_kind) peek(c, token_kind)
#define PEEK_NEXT(token_kind) peek_next(c, token_kind)
#define ADVANCE() advance(c)
#define EAT(token_kind) eat(c, token_kind)
#define TOKEN_STRING() token_string(c)

#define FUNCTION(index) get_function(apm->function, index)
#define STATEMENT(index) get_statement(apm->statement, index)
#define EXPRESSION(index) get_expression(apm->expression, index)
#define VARIABLE(index) get_variable(apm->variable, index)

#define START_SPAN(node_ptr) node_ptr->span.pos = token_string(c).pos;
#define END_SPAN(node_ptr) node_ptr->span.len = token_string(c).pos - node_ptr->span.pos;

// TOKEN CONSUMPTION //

bool peek(Compiler *c, TokenKind token_kind)
{
    return c->tokens[c->next_token].kind == token_kind;
}

bool peek_next(Compiler *c, TokenKind token_kind)
{
    if (peek(c, END_OF_FILE))
        return token_kind == END_OF_FILE;
    return c->tokens[c->next_token + 1].kind == token_kind;
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

// ERROR AND RECOVERY //

void raise_parse_error(Compiler *c, CompilationErrorCode code)
{
    if (c->parse_status == OKAY)
    {
        raise_compilation_error(c, code, c->tokens[c->next_token].str);
    }

    c->parse_status = PANIC;
}

void recover_from_panic(Compiler *c)
{
    if (c->parse_status == PANIC)
        c->parse_status = RECOVERED;
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

// PEEK APM  //

bool peek_expression(Compiler *c)
{
    return PEEK(IDENTITY) ||
           PEEK(KEYWORD_TRUE) ||
           PEEK(KEYWORD_FALSE) ||
           PEEK(NUMBER) ||
           PEEK(STRING) ||
           PEEK(BROKEN_STRING);
}

bool peek_statement(Compiler *c)
{
    return PEEK(CURLY_L) ||
           PEEK(COLON) ||
           PEEK(KEYWORD_IF) ||
           PEEK(KEYWORD_DEF) ||
           PEEK(ARROW_R) ||
           peek_expression(c);
}

// PARSE APM //

void parse(Compiler *compiler, Program *apm)
{
    compiler->next_token = 0;
    compiler->parse_status = OKAY;
    parse_program(compiler, apm);
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
            raise_parse_error(c, EXPECTED_FUNCTION);

            size_t depth = 0;
            while (c->parse_status == PANIC)
            {
                if (PEEK(END_OF_FILE))
                    return;

                // FIXME: This method of tracking the depth works for multi statement `{}` blocks, but not for single statement `:` blocks
                else if (PEEK(CURLY_L))
                    depth++;
                else if (PEEK(CURLY_R) && depth > 0)
                    depth--;

                if (PEEK(CURLY_R) && depth == 0)
                    c->parse_status = RECOVERED;

                ADVANCE();

                if (PEEK(KEYWORD_FN))
                    c->parse_status = RECOVERED;
            }
        }
    }
}

// NOTE: Can return with status OKAY or RECOVERED
void parse_function(Compiler *c, Program *apm)
{
    size_t funct = add_function(&apm->function);
    START_SPAN(FUNCTION(funct));

    EAT(KEYWORD_FN);

    substr identity = TOKEN_STRING();
    FUNCTION(funct)->identity = identity;
    EAT(IDENTITY);

    // TODO: Handle this scenario correctly
    assert(c->parse_status == OKAY);

    EAT(PAREN_L);
    EAT(PAREN_R);

    attempt_to_recover_at_next_code_block(c);
    size_t body = parse_code_block(c, apm);
    FUNCTION(funct)->body = body;

    END_SPAN(FUNCTION(funct));
}

// NOTE: Can return with status OKAY or RECOVERED
size_t parse_statement(Compiler *c, Program *apm)
{
    if (PEEK(CURLY_L))
    {
        return parse_code_block(c, apm); // Can return with status OKAY or RECOVERED
    }

    size_t stmt = add_statement(&apm->statement);
    START_SPAN(STATEMENT(stmt));

    // IF_STATEMENT
    if (PEEK(KEYWORD_IF))
    {
        STATEMENT(stmt)->kind = IF_SEGMENT;

        EAT(KEYWORD_IF);
        size_t condition = parse_expression(c, apm);
        STATEMENT(stmt)->condition = condition;

        attempt_to_recover_at_next_code_block(c);
        if (c->parse_status == PANIC)
            goto recover;

        size_t body = parse_code_block(c, apm);
        STATEMENT(stmt)->body = body;

        while (PEEK(KEYWORD_ELSE))
        {
            size_t segment_stmt = add_statement(&apm->statement);
            START_SPAN(STATEMENT(segment_stmt));

            EAT(KEYWORD_ELSE);

            if (PEEK(KEYWORD_IF))
            {
                EAT(KEYWORD_IF);

                STATEMENT(segment_stmt)->kind = ELSE_IF_SEGMENT;

                size_t condition = parse_expression(c, apm);
                STATEMENT(segment_stmt)->condition = condition;

                attempt_to_recover_at_next_code_block(c);
                if (c->parse_status == PANIC)
                    goto recover;

                size_t body = parse_code_block(c, apm);
                STATEMENT(segment_stmt)->body = body;

                END_SPAN(STATEMENT(segment_stmt));
            }
            else
            {
                STATEMENT(segment_stmt)->kind = ELSE_SEGMENT;

                size_t body = parse_code_block(c, apm);
                STATEMENT(segment_stmt)->body = body;

                END_SPAN(STATEMENT(segment_stmt));
                break;
            }
        }

        goto finish;
    }

    // Inferred VARIABLE_DECLARATION
    if (PEEK(KEYWORD_DEF))
    {
        size_t var = add_variable(&apm->variable);

        STATEMENT(stmt)->kind = VARIABLE_DECLARATION;
        STATEMENT(stmt)->variable = var;
        STATEMENT(stmt)->has_initial_value = false;
        STATEMENT(stmt)->has_type_expression = false;

        EAT(KEYWORD_DEF);

        substr identity = TOKEN_STRING();
        VARIABLE(var)->identity = identity;
        EAT(IDENTITY);

        if (PEEK(EQUAL))
        {
            EAT(EQUAL);

            size_t initial_value = parse_expression(c, apm);
            STATEMENT(stmt)->initial_value = initial_value;
            if (c->parse_status == PANIC)
                goto recover;

            STATEMENT(stmt)->has_initial_value = true;
        }
        else
        {
            // TODO: Error
        }

        EAT(SEMI_COLON);

        if (c->in_scope_var_count == c->in_scope_var_capacity)
        {
            c->in_scope_var_capacity = c->in_scope_var_capacity * 2;
            c->in_scope_vars = (VariableData *)realloc(c->in_scope_vars, sizeof(VariableData) * c->in_scope_var_capacity);
        }

        c->in_scope_vars[c->in_scope_var_count].identity = identity;
        c->in_scope_vars[c->in_scope_var_count].index = var;
        c->in_scope_var_count++;

        goto finish;
    }

    // OUTPUT_STATEMENT
    if (PEEK(ARROW_R))
    {
        STATEMENT(stmt)->kind = OUTPUT_STATEMENT;

        EAT(ARROW_R);
        size_t value = parse_expression(c, apm);
        STATEMENT(stmt)->expression = value;
        if (c->parse_status == PANIC)
            goto recover;

        EAT(SEMI_COLON);
        goto finish;
    }

    // EXPRESSION_STMT / ASSIGNMENT_STATEMENT / Typed VARIABLE_DECLARATION
    else if (peek_expression(c))
    {
        STATEMENT(stmt)->kind = EXPRESSION_STMT;

        // EXPRESSION_STMT
        size_t value = parse_expression(c, apm);
        STATEMENT(stmt)->expression = value;
        if (c->parse_status == PANIC)
            goto recover;

        if (peek_expression(c) && EXPRESSION(value)->kind == IDENTITY_LITERAL) // Typed VARIABLE_DECLARATION
        {
            size_t var = add_variable(&apm->variable);

            STATEMENT(stmt)->kind = VARIABLE_DECLARATION;
            STATEMENT(stmt)->variable = var;
            STATEMENT(stmt)->type_expression = value;
            STATEMENT(stmt)->has_type_expression = true;
            STATEMENT(stmt)->has_initial_value = false;

            substr identity = TOKEN_STRING();
            VARIABLE(var)->identity = identity;
            EAT(IDENTITY);

            // TODO: Parse assignment

            if (c->in_scope_var_count == c->in_scope_var_capacity)
            {
                c->in_scope_var_capacity = c->in_scope_var_capacity * 2;
                c->in_scope_vars = (VariableData *)realloc(c->in_scope_vars, sizeof(VariableData) * c->in_scope_var_capacity);
            }

            c->in_scope_vars[c->in_scope_var_count].identity = identity;
            c->in_scope_vars[c->in_scope_var_count].index = var;
            c->in_scope_var_count++;
        }
        else if (PEEK(EQUAL)) // ASSIGNMENT_STATEMENT
        {
            STATEMENT(stmt)->kind = ASSIGNMENT_STATEMENT;
            STATEMENT(stmt)->assignment_lhs = value;

            EAT(EQUAL);

            size_t rhs = parse_expression(c, apm);
            STATEMENT(stmt)->assignment_rhs = rhs;
            if (c->parse_status == PANIC)
                goto recover;
        }

        EAT(SEMI_COLON);
        goto finish;
    }

    // INVALID_STATEMENT
    STATEMENT(stmt)->kind = INVALID_STATEMENT;
    raise_parse_error(c, EXPECTED_STATEMENT);

finish:
    END_SPAN(STATEMENT(stmt));

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
size_t parse_code_block(Compiler *c, Program *apm)
{
    size_t code_block = add_statement(&apm->statement);
    STATEMENT(code_block)->kind = CODE_BLOCK;
    START_SPAN(STATEMENT(code_block));

    size_t first_statement = apm->statement.count;
    STATEMENT(code_block)->statements.first = first_statement;

    size_t in_scope_var_count = c->in_scope_var_count;

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

    c->in_scope_var_count = in_scope_var_count; // Discard variables that were declared in nested scopes

    size_t statement_count = apm->statement.count - first_statement;
    STATEMENT(code_block)->statements.count = statement_count;

    END_SPAN(STATEMENT(code_block));
    return code_block;
}

// NOTE: Can return with status OKAY, RECOVERED, or PANIC
size_t parse_expression(Compiler *c, Program *apm)
{
    size_t expr = add_expression(&apm->expression);
    START_SPAN(EXPRESSION(expr));

    // Left-hand side of expression
    if (PEEK(IDENTITY))
    {
        size_t is_unknown_literal = true;
        for (size_t i = 0; i < c->in_scope_var_count; i++)
        {
            size_t j = c->in_scope_var_count - 1 - i; // Start from the end of the array so that nested variables are checked first
            if (substr_match(c->source_text, TOKEN_STRING(), c->in_scope_vars[j].identity))
            {
                EXPRESSION(expr)->kind = VARIABLE_REFERENCE;
                EXPRESSION(expr)->variable = c->in_scope_vars[j].index;
                is_unknown_literal = false;
                break;
            }
        }

        if (is_unknown_literal)
        {
            EXPRESSION(expr)->kind = IDENTITY_LITERAL;
            EXPRESSION(expr)->identity = TOKEN_STRING();
            EXPRESSION(expr)->identity_resolved = false;
        }

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
    else if (PEEK(NUMBER))
    {
        substr str = TOKEN_STRING();
        int num = 0;
        for (size_t i = str.pos; i < str.pos + str.len; i++)
            num = num * 10 + (c->source_text[i] - 48);

        EXPRESSION(expr)->kind = NUMBER_LITERAL;
        EXPRESSION(expr)->number_value = num;
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
        raise_parse_error(c, EXPECTED_EXPRESSION);

        // NOTE: parse_expression can put the parser in panic mode and then return.
        //       This means anyone who calls parse_expression needs to be able to recover.
        return expr;
    }

    END_SPAN(EXPRESSION(expr));

    // Function call
    if (PEEK(PAREN_L))
    {
        size_t callee = expr;
        expr = add_expression(&apm->expression);
        EXPRESSION(expr)->span.pos = EXPRESSION(callee)->span.pos;

        EXPRESSION(expr)->kind = FUNCTION_CALL;
        EXPRESSION(expr)->callee = callee;

        ADVANCE();
        EAT(PAREN_R);
        END_SPAN(EXPRESSION(expr));

        recover_from_panic(c);
    }

    return expr;
}
