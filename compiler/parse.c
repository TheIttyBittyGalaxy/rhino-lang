#include "data/substr.h"
#include "fatal_error.h"
#include "parse.h"

#include <assert.h>

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
void attempt_to_advance_to_next_code_block(Compiler *c);

// Peek APM
bool peek_expression(Compiler *c);
bool peek_statement(Compiler *c);

// Parse APM
void parse(Compiler *compiler, Program *apm);
void parse_program(Compiler *c, Program *apm);
void parse_function(Compiler *c, Program *apm, Block *parent, StatementListAllocator *parent_statements);
void parse_enum_type(Compiler *c, Program *apm, Block *parent, StatementListAllocator *parent_statements);
void parse_struct_type(Compiler *c, Program *apm, Block *parent, StatementListAllocator *parent_statements);
void parse_variable_declaration(Compiler *c, Program *apm, Block *parent, StatementListAllocator *parent_statements, Statement *declaration, bool declare_symbol_in_parent);

void parse_program_block(Compiler *c, Program *apm);
Block *parse_block(Compiler *c, Program *apm, Block *parent);
Statement *parse_statement(Compiler *c, Program *apm, StatementListAllocator *allocator, Block *block);
Expression *parse_expression(Compiler *c, Program *apm);

// MACROS //

#define PEEK(token_kind) peek(c, token_kind)
#define PEEK_NEXT(token_kind) peek_next(c, token_kind)
#define ADVANCE() advance(c)
#define EAT(token_kind) eat(c, token_kind)
#define TOKEN_STRING() token_string(c)

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

// NOTE: Can return with status OKAY or PANIC
void attempt_to_advance_to_next_code_block(Compiler *c)
{
    if (PEEK(CURLY_L) || PEEK(COLON))
    {
        c->parse_status = OKAY;
        return;
    }

    c->parse_status = PANIC;

    size_t start = c->next_token;
    while (!peek(c, END_OF_FILE))
    {
        advance(c);

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
            continue;
        }

        // FIXME: When is the right point to give up looking for the next code block?
        else if (peek(c, SEMI_COLON))
            break;
    }
    c->next_token = start;
}

// PEEK APM  //

bool peek_expression(Compiler *c)
{
    return PEEK(IDENTITY) ||
           PEEK(KEYWORD_TRUE) ||
           PEEK(KEYWORD_FALSE) ||
           PEEK(INTEGER) ||
           PEEK(RATIONAL) ||
           PEEK(STRING) ||
           PEEK(BROKEN_STRING) ||
           PEEK(PLUS) ||
           PEEK(MINUS) ||
           PEEK(KEYWORD_NOT);
}

bool peek_statement(Compiler *c)
{
    return PEEK(CURLY_L) ||
           PEEK(COLON) ||
           PEEK(KEYWORD_IF) ||
           PEEK(KEYWORD_FOR) ||
           PEEK(KEYWORD_LOOP) ||
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
    apm->global_symbol_table = allocate_symbol_table(&apm->symbol_table, NULL);
    parse_program_block(c, apm);
}

// NOTE: Can return with status OKAY or RECOVERED
void parse_function(Compiler *c, Program *apm, Block *parent, StatementListAllocator *parent_statements)
{
    Function *funct = append_function(&apm->function);
    funct->has_return_type_expression = false;
    funct->return_type.sort = UNINITIALISED_SORT;
    START_SPAN(funct);

    Statement *declaration = append_statement(parent_statements);
    declaration->kind = FUNCTION_DECLARATION;
    declaration->function = funct;

    EAT(KEYWORD_FN);

    funct->identity = TOKEN_STRING();
    EAT(IDENTITY);

    ParameterListAllocator param_allocator;
    init_parameter_list_allocator(&param_allocator, &apm->parameter_lists, 512); // FIXME: 512 was chosen arbitrarily

    EAT(PAREN_L);
    while (peek_expression(c))
    {
        Parameter *parameter = append_parameter(&param_allocator);
        START_SPAN(parameter);

        parameter->type_expression = parse_expression(c, apm);
        parameter->identity = TOKEN_STRING();
        EAT(IDENTITY);

        END_SPAN(parameter);

        if (!PEEK(COMMA))
            break;
        EAT(COMMA);
    }
    EAT(PAREN_R);

    funct->parameters = get_parameter_list(param_allocator);

    if (peek_expression(c))
    {
        funct->has_return_type_expression = true;
        funct->return_type_expression = parse_expression(c, apm);
    }

    // Adding the symbol here allows the function body to recursively refer to the function,
    // while preventing the function parameters or return type attempting to refer to it.
    declare_symbol(apm, parent->symbol_table, FUNCTION_SYMBOL, funct, funct->identity);

    attempt_to_advance_to_next_code_block(c);
    funct->body = parse_block(c, apm, parent);

    END_SPAN(funct);
    declaration->span = funct->span;
}

// TODO: Ensure this can only return with status OKAY or RECOVERED
void parse_enum_type(Compiler *c, Program *apm, Block *parent, StatementListAllocator *parent_statements)
{
    EnumType *enum_type = append_enum_type(&apm->enum_type);
    START_SPAN(enum_type);

    Statement *declaration = append_statement(parent_statements);
    declaration->kind = ENUM_TYPE_DECLARATION;
    declaration->enum_type = enum_type;

    EAT(KEYWORD_ENUM);

    enum_type->identity = TOKEN_STRING();
    EAT(IDENTITY);

    EnumValueListAllocator value_allocator;
    init_enum_value_list_allocator(&value_allocator, &apm->enum_value_lists, 512); // FIXME: 512 was chosen arbitrarily

    EAT(CURLY_L);
    while (true)
    {
        EnumValue *enum_value = append_enum_value(&value_allocator);
        enum_value->type_of_enum_value = enum_type;
        START_SPAN(enum_value);

        enum_value->identity = TOKEN_STRING();
        EAT(IDENTITY);

        END_SPAN(enum_value);

        if (!PEEK(COMMA))
            break;
        EAT(COMMA);
    }
    EAT(CURLY_R);

    enum_type->values = get_enum_value_list(value_allocator);

    END_SPAN(enum_type);
    declaration->span = enum_type->span;

    declare_symbol(apm, parent->symbol_table, ENUM_TYPE_SYMBOL, enum_type, enum_type->identity);
}

// TODO: Ensure this can only return with status OKAY or RECOVERED
void parse_struct_type(Compiler *c, Program *apm, Block *parent, StatementListAllocator *parent_statements)
{
    Block *body = append_block(&apm->block);
    body->declaration_block = true;
    body->singleton_block = false;
    body->symbol_table = allocate_symbol_table(&apm->symbol_table, parent->symbol_table);

    StructType *struct_type = append_struct_type(&apm->struct_type);
    struct_type->body = body;
    START_SPAN(struct_type);

    Statement *declaration = append_statement(parent_statements);
    declaration->kind = STRUCT_TYPE_DECLARATION;
    declaration->struct_type = struct_type;

    EAT(KEYWORD_STRUCT);

    struct_type->identity = TOKEN_STRING();
    EAT(IDENTITY);

    // Declaring the symbol here allows the struct to recursively refer to itself.
    // This is illegal for structs, but not for objects, and so for structs is an error.
    declare_symbol(apm, parent->symbol_table, STRUCT_TYPE_SYMBOL, struct_type, struct_type->identity);

    StatementListAllocator statement_allocator;
    init_statement_list_allocator(&statement_allocator, &apm->statement_lists, 512); // FIXME: 512 was chosen arbitrarily
    PropertyListAllocator property_allocator;
    init_property_list_allocator(&property_allocator, &apm->property_lists, 512); // FIXME: 512 was chosen arbitrarily

    // TODO: Should we just be using a general "parse declaration block" here?
    EAT(CURLY_L);
    while (!PEEK(CURLY_R))
    {
        if (peek_expression(c))
        {
            Property *property = append_property(&property_allocator);
            START_SPAN(property);

            property->type_expression = parse_expression(c, apm);
            property->identity = TOKEN_STRING();
            EAT(IDENTITY);
            EAT(SEMI_COLON);

            END_SPAN(property);
        }
        else if (PEEK(KEYWORD_ENUM))
        {
            parse_enum_type(c, apm, body, &statement_allocator);
        }
        else if (PEEK(KEYWORD_STRUCT))
        {
            parse_struct_type(c, apm, body, &statement_allocator);
        }
        else
        {
            raise_parse_error(c, EXPECTED_TYPE_EXPRESSION);
            c->parse_status = PANIC;
            // TODO: Recover to end of line
            break;
        }
    }
    EAT(CURLY_R);

    body->statements = get_statement_list(statement_allocator);
    struct_type->properties = get_property_list(property_allocator);

    END_SPAN(struct_type);
    declaration->span = struct_type->span;
}

void parse_variable_declaration(Compiler *c, Program *apm, Block *parent, StatementListAllocator *parent_statements, Statement *declaration, bool declare_symbol_in_parent)
{
    Variable *var = append_variable(&apm->variable);
    var->type.sort = UNINITIALISED_SORT;

    if (declaration == NULL)
    {
        declaration = append_statement(parent_statements);
        START_SPAN(declaration);
    }

    declaration->kind = VARIABLE_DECLARATION;
    declaration->variable = var;
    declaration->type_expression = NULL;
    declaration->initial_value = NULL;
    declaration->has_valid_identity = false;

    if (PEEK(KEYWORD_DEF))
        EAT(KEYWORD_DEF);
    else
        declaration->type_expression = parse_expression(c, apm);

    if (PEEK(IDENTITY))
        goto parse_identity;

    raise_parse_error(c, EXPECTED_VARIABLE_NAME);
    while (true)
    {
        if (PEEK(END_OF_FILE))
            goto finish;
        else if (PEEK(SEMI_COLON))
            goto finish;
        else if (PEEK(IDENTITY))
            goto parse_identity;
        else if (PEEK(EQUAL))
            goto parse_initial_value;

        ADVANCE();
    }

parse_identity:
    declaration->has_valid_identity = true;
    var->identity = TOKEN_STRING();
    EAT(IDENTITY);

parse_initial_value:
    if (PEEK(EQUAL))
    {
        EAT(EQUAL);
        declaration->initial_value = parse_expression(c, apm);
    }

    if (declare_symbol_in_parent && declaration->has_valid_identity)
        declare_symbol(apm, parent->symbol_table, VARIABLE_SYMBOL, var, var->identity);

    if (c->parse_status == PANIC)
        return;

finish:
    EAT(SEMI_COLON);
    END_SPAN(declaration); // NOTE: parse_statement will repeat this, but I don't think that's an issue?
}

// NOTE: Can return with status OKAY or RECOVERED
Statement *parse_statement(Compiler *c, Program *apm, StatementListAllocator *allocator, Block *block)
{
    Statement *stmt = append_statement(allocator);
    START_SPAN(stmt);

    // CODE_BLOCK
    if (PEEK(CURLY_L))
    {
        stmt->kind = CODE_BLOCK;
        stmt->block = parse_block(c, apm, block); // Can return with status OKAY or RECOVERED

        goto finish;
    }

    // IF_STATEMENT
    if (PEEK(KEYWORD_IF))
    {
        stmt->kind = IF_SEGMENT;

        EAT(KEYWORD_IF);
        stmt->condition = parse_expression(c, apm);

        attempt_to_advance_to_next_code_block(c);
        if (c->parse_status == PANIC)
            goto recover;

        stmt->body = parse_block(c, apm, block);

        while (PEEK(KEYWORD_ELSE))
        {
            Statement *segment_stmt = append_statement(allocator);
            START_SPAN(segment_stmt);

            EAT(KEYWORD_ELSE);

            if (PEEK(KEYWORD_IF))
            {
                segment_stmt->kind = ELSE_IF_SEGMENT;

                EAT(KEYWORD_IF);
                segment_stmt->condition = parse_expression(c, apm);

                attempt_to_advance_to_next_code_block(c);
                if (c->parse_status == PANIC)
                    goto recover;

                segment_stmt->body = parse_block(c, apm, block);

                END_SPAN(segment_stmt);
            }
            else
            {
                segment_stmt->kind = ELSE_SEGMENT;

                segment_stmt->body = parse_block(c, apm, block);

                END_SPAN(segment_stmt);
                break;
            }
        }

        goto finish;
    }

    // BREAK_LOOP
    if (PEEK(KEYWORD_LOOP))
    {
        stmt->kind = BREAK_LOOP;
        EAT(KEYWORD_LOOP);
        stmt->body = parse_block(c, apm, block);

        goto finish;
    }

    // FOR_STATEMENT
    if (PEEK(KEYWORD_FOR))
    {
        stmt->kind = FOR_LOOP;

        EAT(KEYWORD_FOR);

        // Iterator
        Variable *iterator = append_variable(&apm->variable);
        stmt->iterator = iterator;

        iterator->identity = TOKEN_STRING();
        iterator->type.sort = UNINITIALISED_SORT;
        EAT(IDENTITY);

        EAT(KEYWORD_IN);

        // Iterable
        Expression *iterable = parse_expression(c, apm);
        stmt->iterable = iterable;

        // Body
        attempt_to_advance_to_next_code_block(c);
        if (c->parse_status == PANIC)
            goto recover;

        stmt->body = parse_block(c, apm, block);

        goto finish;
    }

    // BREAK_STATEMENT
    if (PEEK(KEYWORD_BREAK))
    {
        stmt->kind = BREAK_STATEMENT;
        EAT(KEYWORD_BREAK);

        EAT(SEMI_COLON);
        goto finish;
    }

    // VARIABLE_DECLARATION with inferred type
    if (PEEK(KEYWORD_DEF))
    {
        parse_variable_declaration(c, apm, block, allocator, stmt, false);
        goto finish;
    }

    // OUTPUT_STATEMENT
    if (PEEK(ARROW_R))
    {
        stmt->kind = OUTPUT_STATEMENT;

        EAT(ARROW_R);
        stmt->expression = parse_expression(c, apm);

        if (c->parse_status == PANIC)
            goto recover;

        EAT(SEMI_COLON);
        goto finish;
    }

    // RETURN_STATEMENT
    if (PEEK(KEYWORD_RETURN))
    {
        stmt->kind = RETURN_STATEMENT;
        stmt->expression = NULL;

        EAT(KEYWORD_RETURN);

        if (peek_expression(c))
        {
            stmt->expression = parse_expression(c, apm);
            if (c->parse_status == PANIC)
                goto recover;
        }

        EAT(SEMI_COLON);
        goto finish;
    }

    // EXPRESSION_STMT / ASSIGNMENT_STATEMENT / VARIABLE_DECLARATION with stated type
    else if (peek_expression(c))
    {
        size_t start_of_statement = c->next_token;
        Expression *expr = parse_expression(c, apm);

        if (c->parse_status == PANIC)
            goto recover;

        // VARIABLE_DECLARATION with stated type
        if (PEEK(IDENTITY))
        {
            c->next_token = start_of_statement;
            parse_variable_declaration(c, apm, block, allocator, stmt, false);

            if (c->parse_status == PANIC)
                goto recover;

            goto finish;
        }

        // ASSIGNMENT_STATEMENT
        else if (PEEK(EQUAL))
        {
            stmt->kind = ASSIGNMENT_STATEMENT;

            stmt->assignment_lhs = expr;
            EAT(EQUAL);
            stmt->assignment_rhs = parse_expression(c, apm);

            if (c->parse_status == PANIC)
                goto recover;

            EAT(SEMI_COLON);
            goto finish;
        }

        // EXPRESSION_STMT
        else
        {
            stmt->kind = EXPRESSION_STMT;
            stmt->expression = expr;

            EAT(SEMI_COLON);
            goto finish;
        }

        __builtin_unreachable();
    }

    // INVALID_STATEMENT
    stmt->kind = INVALID_STATEMENT;
    raise_parse_error(c, EXPECTED_STATEMENT);

    // FIXME: Not sure why we allow a situation where a statement might "recover" without ever ending it's span?

finish:
    END_SPAN(stmt);

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

void parse_program_block(Compiler *c, Program *apm)
{
    Block *program_block = append_block(&apm->block);
    program_block->declaration_block = true;
    program_block->singleton_block = false;
    program_block->symbol_table = allocate_symbol_table(&apm->symbol_table, apm->global_symbol_table);

    apm->program_block = program_block;

    StatementListAllocator statements_allocator;
    init_statement_list_allocator(&statements_allocator, &apm->statement_lists, 512); // FIXME: 512 was chosen arbitrarily

    while (true)
    {
        if (PEEK(END_OF_FILE))
            break;

        else if (PEEK(KEYWORD_FN))
            parse_function(c, apm, program_block, &statements_allocator);
        else if (PEEK(KEYWORD_ENUM))
            parse_enum_type(c, apm, program_block, &statements_allocator);
        else if (PEEK(KEYWORD_STRUCT))
            parse_struct_type(c, apm, program_block, &statements_allocator);
        else if (PEEK(KEYWORD_DEF) || peek_expression(c))
            parse_variable_declaration(c, apm, program_block, &statements_allocator, NULL, true);

        else
        {
            raise_parse_error(c, UNEXPECTED_TOKEN_IN_PROGRAM);

            size_t depth = 0;
            while (c->parse_status == PANIC)
            {
                if (PEEK(END_OF_FILE))
                    break;

                // FIXME: This method of tracking the depth works for multi statement `{}` blocks, but not for single statement `:` blocks
                else if (PEEK(CURLY_L))
                    depth++;
                else if (PEEK(CURLY_R) && depth > 0)
                    depth--;

                if (PEEK(CURLY_R) && depth == 0)
                    c->parse_status = RECOVERED;

                if (PEEK(SEMI_COLON) && depth == 0)
                    c->parse_status = OKAY;

                ADVANCE();

                if (depth > 0)
                    continue;

                if (PEEK(KEYWORD_FN) ||
                    PEEK(KEYWORD_ENUM) ||
                    PEEK(KEYWORD_STRUCT) ||
                    PEEK(KEYWORD_DEF))
                    c->parse_status = RECOVERED;
            }
        }
    }

    program_block->statements = get_statement_list(statements_allocator);
}

// NOTE: Can return with status OKAY or RECOVERED
Block *parse_block(Compiler *c, Program *apm, Block *parent)
{
    Block *block = append_block(&apm->block);
    block->declaration_block = false;
    block->singleton_block = false;
    block->symbol_table = parent->symbol_table;

    StatementListAllocator statement_allocator;
    init_statement_list_allocator(&statement_allocator, &apm->statement_lists, 512); // FIXME: 512 was chosen arbitrarily

    if (PEEK(COLON))
    {
        EAT(COLON);
        block->singleton_block = true;
        parse_statement(c, apm, &statement_allocator, block); // Can return with status OKAY or RECOVERED
    }
    else
    {
        EAT(CURLY_L);
        recover_from_panic(c);

        // TODO: Make this more efficient. Currently we create a new symbol table for
        //       every block, meaning we create numerous completely empty tables.
        block->symbol_table = allocate_symbol_table(&apm->symbol_table, parent->symbol_table);

        while (true)
        {
            if (PEEK(KEYWORD_FN))
                parse_function(c, apm, block, &statement_allocator); // Can return with status OKAY or RECOVERED
            else if (PEEK(KEYWORD_ENUM))
                parse_enum_type(c, apm, block, &statement_allocator); // TODO: Do we need to handle a PANIC status?
            else if (PEEK(KEYWORD_STRUCT))
                parse_struct_type(c, apm, block, &statement_allocator); // TODO: Do we need to handle a PANIC status?
            else if (peek_statement(c))
                parse_statement(c, apm, &statement_allocator, block); // Can return with status OKAY or RECOVERED
            else
                break;
        }

        EAT(CURLY_R);
        recover_from_panic(c);
    }

    block->statements = get_statement_list(statement_allocator);

    return block;
}

// NOTE: Can return with status OKAY, RECOVERED, or PANIC
Expression *parse_expression_with_precedence(Compiler *c, Program *apm, ExprPrecedence caller_precedence);

Expression *parse_expression(Compiler *c, Program *apm)
{
    return parse_expression_with_precedence(c, apm, PRECEDENCE_NONE);
}

#define LEFT_ASSOCIATIVE_OPERATOR_BINDS(operator_precedence, caller_precedence) operator_precedence > caller_precedence
#define RIGHT_ASSOCIATIVE_OPERATOR_BINDS(operator_precedence, caller_precedence) operator_precedence >= caller_precedence

#define PARSE_BINARY_OPERATION(token_kind, expr_kind)                                                          \
    else if (PEEK(token_kind) && LEFT_ASSOCIATIVE_OPERATOR_BINDS(precedence_of(expr_kind), caller_precedence)) \
    {                                                                                                          \
        expr->kind = expr_kind;                                                                                \
        expr->lhs = lhs;                                                                                       \
        ADVANCE();                                                                                             \
        expr->rhs = parse_expression_with_precedence(c, apm, precedence_of(expr_kind));                        \
    }

Expression *parse_expression_with_precedence(Compiler *c, Program *apm, ExprPrecedence caller_precedence)
{
    Expression *lhs = append_expression(&apm->expression);
    START_SPAN(lhs);

    // Left-hand side of expression
    if (PEEK(IDENTITY))
    {
        lhs->kind = IDENTITY_LITERAL;
        lhs->identity = TOKEN_STRING();
        lhs->given_error = false;
        ADVANCE();
    }
    else if (PEEK(KEYWORD_TRUE))
    {
        lhs->kind = BOOLEAN_LITERAL;
        lhs->bool_value = true;
        ADVANCE();
    }
    else if (PEEK(KEYWORD_FALSE))
    {
        lhs->kind = BOOLEAN_LITERAL;
        lhs->bool_value = false;
        ADVANCE();
    }
    else if (PEEK(INTEGER))
    {
        substr str = TOKEN_STRING();

        int num = 0;
        for (size_t i = str.pos; i < str.pos + str.len; i++)
            num = num * 10 + (c->source_text[i] - 48);

        lhs->kind = INTEGER_LITERAL;
        lhs->integer_value = num;
        ADVANCE();
    }
    else if (PEEK(RATIONAL))
    {
        substr str = TOKEN_STRING();

        int int_part = 0;
        for (size_t i = str.pos; i < str.pos + str.len; i++)
        {
            if (c->source_text[i] == '.')
                break;
            int_part = int_part * 10 + (c->source_text[i] - 48);
        }

        double float_part = 0;
        for (size_t i = str.pos + str.len - 1; i >= str.pos; i--)
        {
            if (c->source_text[i] == '.')
                break;
            float_part = (float_part + (c->source_text[i] - 48)) / 10;
        }

        lhs->kind = FLOAT_LITERAL;
        lhs->float_value = int_part + float_part;
        ADVANCE();
    }
    else if (PEEK(STRING) || PEEK(BROKEN_STRING))
    {
        substr str = TOKEN_STRING();
        str.pos++;
        str.len -= PEEK(STRING) ? 2 : 1;

        lhs->kind = STRING_LITERAL;
        lhs->string_value = str;
        ADVANCE();
    }
    else if (PEEK(PLUS))
    {
        lhs->kind = UNARY_POS;
        ADVANCE();
        lhs->operand = parse_expression_with_precedence(c, apm, precedence_of(UNARY_POS));
    }
    else if (PEEK(MINUS))
    {
        lhs->kind = UNARY_NEG;
        ADVANCE();
        lhs->operand = parse_expression_with_precedence(c, apm, precedence_of(UNARY_NEG));
    }
    else if (PEEK(KEYWORD_NOT))
    {
        lhs->kind = UNARY_NOT;
        ADVANCE();
        lhs->operand = parse_expression_with_precedence(c, apm, precedence_of(UNARY_NOT));
    }
    else
    {
        lhs->kind = INVALID_EXPRESSION;
        END_SPAN(lhs);

        raise_parse_error(c, EXPECTED_EXPRESSION);

        // NOTE: parse_expression can put the parser in panic mode and then return.
        //       This means anyone who calls parse_expression needs to be able to recover.
        return lhs;
    }

    END_SPAN(lhs);

    // Postfix operator OR infix operator and right-hand side expression
    while (true)
    {
        // Open `expr`
        Expression *expr = append_expression(&apm->expression);
        expr->span.pos = lhs->span.pos;

        // Function call
        if (PEEK(PAREN_L) && LEFT_ASSOCIATIVE_OPERATOR_BINDS(precedence_of(FUNCTION_CALL), caller_precedence))
        {
            expr->kind = FUNCTION_CALL;
            expr->callee = lhs;

            ArgumentListAllocator arg_allocator;
            init_argument_list_allocator(&arg_allocator, &apm->arguments_lists, 512); // FIXME: 512 was chosen arbitrarily

            EAT(PAREN_L);
            while (peek_expression(c))
            {
                Argument *arg = append_argument(&arg_allocator);
                arg->expr = parse_expression(c, apm);

                if (!PEEK(COMMA))
                    break;
                EAT(COMMA);
            }
            EAT(PAREN_R);

            expr->arguments = get_argument_list(arg_allocator);
        }

        // Increment
        else if (PEEK(TWO_PLUS) && RIGHT_ASSOCIATIVE_OPERATOR_BINDS(precedence_of(UNARY_INCREMENT), caller_precedence))
        {
            expr->kind = UNARY_INCREMENT;
            expr->operand = lhs;
            ADVANCE();
        }

        // Decrement
        else if (PEEK(TWO_MINUS) && RIGHT_ASSOCIATIVE_OPERATOR_BINDS(precedence_of(UNARY_DECREMENT), caller_precedence))
        {
            expr->kind = UNARY_DECREMENT;
            expr->operand = lhs;
            ADVANCE();
        }

        // Index by field
        else if (PEEK(DOT) && LEFT_ASSOCIATIVE_OPERATOR_BINDS(precedence_of(INDEX_BY_FIELD), caller_precedence))
        {
            expr->kind = INDEX_BY_FIELD;

            expr->subject = lhs;
            EAT(DOT);
            expr->field = TOKEN_STRING();
            EAT(IDENTITY);
        }

        else if (PEEK(TWO_DOT) && LEFT_ASSOCIATIVE_OPERATOR_BINDS(precedence_of(RANGE_LITERAL), caller_precedence))
        {
            expr->kind = RANGE_LITERAL;

            expr->first = lhs;
            EAT(TWO_DOT);
            expr->last = parse_expression_with_precedence(c, apm, precedence_of(RANGE_LITERAL));
        }

        // Factor (multiplication, division, remainder)
        PARSE_BINARY_OPERATION(STAR, BINARY_MULTIPLY)
        PARSE_BINARY_OPERATION(SLASH, BINARY_DIVIDE)
        PARSE_BINARY_OPERATION(PERCENTAGE, BINARY_REMAINDER)

        // Term (addition and subtraction)
        PARSE_BINARY_OPERATION(PLUS, BINARY_ADD)
        PARSE_BINARY_OPERATION(MINUS, BINARY_SUBTRACT)

        // Compare relative (greater than, less than, etc)
        PARSE_BINARY_OPERATION(ARROW_L, BINARY_LESS_THAN)
        PARSE_BINARY_OPERATION(ARROW_R, BINARY_GREATER_THAN)
        PARSE_BINARY_OPERATION(ARROW_L_EQUAL, BINARY_LESS_THAN_EQUAL)
        PARSE_BINARY_OPERATION(ARROW_R_EQUAL, BINARY_GREATER_THAN_EQUAL)

        // Compare equal (equal to, not equal to)
        PARSE_BINARY_OPERATION(TWO_EQUAL, BINARY_EQUAL)
        PARSE_BINARY_OPERATION(EXCLAIM_EQUAL, BINARY_NOT_EQUAL)

        // Logical and
        PARSE_BINARY_OPERATION(KEYWORD_AND, BINARY_LOGICAL_AND)

        // Logical or
        PARSE_BINARY_OPERATION(KEYWORD_OR, BINARY_LOGICAL_OR)

        // Discard `expr` and finish parsing expression
        else
        {
            // FIXME: This wastes the memory that was allocated to this expression
            expr->kind = INVALID_EXPRESSION;
            break;
        }

        // Close `expr` and continue parsing expression
        END_SPAN(expr);
        recover_from_panic(c);
        lhs = expr;
    }

    return lhs;
}

#undef LEFT_ASSOCIATIVE_OPERATOR_BINDS
#undef RIGHT_ASSOCIATIVE_OPERATOR_BINDS
#undef PARSE_BINARY_OPERATION
