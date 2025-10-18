#include "data/substr.h"
#include "fatal_error.h"
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
void parse_function(Compiler *c, Program *apm, Block *parent, StatementListAllocator *parent_statements);
void parse_enum_type(Compiler *c, Program *apm, Block *parent, StatementListAllocator *parent_statements);
void parse_struct_type(Compiler *c, Program *apm, Block *parent, StatementListAllocator *parent_statements);

Block *parse_top_level_declarations(Compiler *c, Program *apm);
Block *parse_block(Compiler *c, Program *apm, Block *parent);
Statement *parse_statement(Compiler *c, Program *apm, StatementListAllocator *allocator, Block *block);
Expression *parse_expression(Compiler *c, Program *apm);

// MACROS //

#define PEEK(token_kind) peek(c, token_kind)
#define PEEK_NEXT(token_kind) peek_next(c, token_kind)
#define ADVANCE() advance(c)
#define EAT(token_kind) eat(c, token_kind)
#define TOKEN_STRING() token_string(c)

#define PARAMETER(index) get_parameter(apm->parameter, index)
#define ARGUMENT(index) get_argument(apm->argument, index)
#define PROPERTY(index) get_property(apm->property, index)
#define STATEMENT(index) get_statement(apm->statement, index)
#define SYMBOL_TABLE(index) get_symbol_table(apm->symbol_table, index)

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
           PEEK(INTEGER) ||
           PEEK(RATIONAL) ||
           PEEK(STRING) ||
           PEEK(BROKEN_STRING);
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
    // Initialise global scopes
    apm->global_symbol_table = add_symbol_table(&apm->symbol_table);
    init_symbol_table(SYMBOL_TABLE(apm->global_symbol_table));

    // Parse top-level of program
    apm->program_block = parse_top_level_declarations(c, apm);
}

// NOTE: Can return with status OKAY or RECOVERED
void parse_function(Compiler *c, Program *apm, Block *parent, StatementListAllocator *parent_statements)
{
    Function *funct = append_function(&apm->function);
    funct->has_return_type_expression = false;
    funct->return_type.sort = INVALID_SORT;

    START_SPAN(funct);

    EAT(KEYWORD_FN);

    substr identity = TOKEN_STRING();
    funct->identity = identity;
    EAT(IDENTITY);

    // TODO: Handle this scenario correctly
    assert(c->parse_status == OKAY);

    size_t first_parameter = apm->parameter.count;
    funct->parameters.first = first_parameter;

    EAT(PAREN_L);
    while (peek_expression(c))
    {
        size_t parameter = add_parameter(&apm->parameter);
        START_SPAN(PARAMETER(parameter));

        Expression *type_expression = parse_expression(c, apm);
        PARAMETER(parameter)->type_expression = type_expression;

        substr identity = TOKEN_STRING();
        PARAMETER(parameter)->identity = identity;
        EAT(IDENTITY);

        END_SPAN(PARAMETER(parameter));

        if (!PEEK(COMMA))
            break;

        EAT(COMMA);
    }
    EAT(PAREN_R);

    size_t parameter_count = apm->parameter.count - first_parameter;
    funct->parameters.count = parameter_count;

    if (peek_expression(c))
    {
        funct->has_return_type_expression = true;
        funct->return_type_expression = parse_expression(c, apm);
    }

    attempt_to_recover_at_next_code_block(c);
    funct->body = parse_block(c, apm, parent);

    END_SPAN(funct);

    append_symbol(apm, parent->symbol_table, FUNCTION_SYMBOL, (SymbolPointer){.function = funct}, identity);
    Statement *declaration = append_statement(parent_statements);
    declaration->kind = FUNCTION_DECLARATION;
    declaration->function = funct;
    declaration->span = funct->span;
}

// TODO: Ensure this can only return with status OKAY or RECOVERED
void parse_enum_type(Compiler *c, Program *apm, Block *parent, StatementListAllocator *parent_statements)
{
    EnumType *enum_type = append_enum_type(&apm->enum_type);
    START_SPAN(enum_type);

    EAT(KEYWORD_ENUM);

    substr identity = TOKEN_STRING();
    enum_type->identity = identity;
    EAT(IDENTITY);

    // TODO: Handle this scenario correctly
    assert(c->parse_status == OKAY);

    EnumValueListAllocator value_allocator;
    init_enum_value_list_allocator(&value_allocator, &apm->enum_value_lists, 512); // FIXME: 512 was chosen arbitrarily

    EAT(CURLY_L);
    while (true)
    {
        EnumValue *enum_value = append_enum_value(&value_allocator);
        enum_value->type_of_enum_value = enum_type;
        START_SPAN(enum_value);

        substr identity = TOKEN_STRING();
        enum_value->identity = identity;
        EAT(IDENTITY);

        END_SPAN(enum_value);

        if (!PEEK(COMMA))
            break;

        EAT(COMMA);
    }
    EAT(CURLY_R);

    END_SPAN(enum_type);

    enum_type->values = get_enum_value_list(value_allocator);

    append_symbol(apm, parent->symbol_table, ENUM_TYPE_SYMBOL, (SymbolPointer){.enum_type = enum_type}, identity);
    Statement *declaration = append_statement(parent_statements);
    declaration->kind = ENUM_TYPE_DECLARATION;
    declaration->enum_type = enum_type;
    declaration->span = enum_type->span;
}

// TODO: Ensure this can only return with status OKAY or RECOVERED
void parse_struct_type(Compiler *c, Program *apm, Block *parent, StatementListAllocator *parent_statements)
{
    StructType *struct_type = append_struct_type(&apm->struct_type);
    START_SPAN(struct_type);

    EAT(KEYWORD_STRUCT);
    substr identity = TOKEN_STRING();
    struct_type->identity = identity;
    EAT(IDENTITY);

    // TODO: Handle this scenario correctly
    assert(c->parse_status == OKAY);

    Block *declarations = append_block(&apm->block);
    struct_type->declarations = declarations;
    declarations->declaration_block = true;
    declarations->singleton_block = false;

    declarations->symbol_table = add_symbol_table(&apm->symbol_table);
    init_symbol_table(SYMBOL_TABLE(declarations->symbol_table));

    StatementListAllocator block_declarations;
    init_statement_list_allocator(&block_declarations, &apm->statement_lists, 512); // FIXME: 512 was chosen arbitrarily

    size_t first_property = apm->property.count;
    struct_type->properties.first = first_property;

    EAT(CURLY_L);
    while (!PEEK(CURLY_R))
    {
        if (peek_expression(c))
        {

            size_t property = add_property(&apm->property);
            START_SPAN(PROPERTY(property));

            Expression *type_expression = parse_expression(c, apm);
            PROPERTY(property)->type_expression = type_expression;

            substr identity = TOKEN_STRING();
            PROPERTY(property)->identity = identity;
            EAT(IDENTITY);

            END_SPAN(PROPERTY(property));

            EAT(SEMI_COLON);
        }
        else if (PEEK(KEYWORD_ENUM))
        {
            parse_enum_type(c, apm, declarations, &block_declarations);
        }
        else if (PEEK(KEYWORD_STRUCT))
        {
            parse_struct_type(c, apm, declarations, &block_declarations);
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

    size_t property_count = apm->property.count - first_property;
    struct_type->properties.count = property_count;

    declarations->statements = get_statement_list(block_declarations);

    size_t last_table = declarations->symbol_table;
    while (SYMBOL_TABLE(last_table)->next)
        last_table = SYMBOL_TABLE(last_table)->next;
    SYMBOL_TABLE(last_table)->next = parent->symbol_table;

    END_SPAN(struct_type);

    append_symbol(apm, parent->symbol_table, STRUCT_TYPE_SYMBOL, (SymbolPointer){.struct_type = struct_type}, identity);
    Statement *declaration = append_statement(parent_statements);
    declaration->kind = STRUCT_TYPE_DECLARATION;
    declaration->struct_type = struct_type;
    declaration->span = struct_type->span;
}

// NOTE: Can return with status OKAY or RECOVERED
Statement *parse_statement(Compiler *c, Program *apm, StatementListAllocator *allocator, Block *block)
{
    Statement *stmt = append_statement(allocator);
    START_SPAN(stmt);

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
        Expression *condition = parse_expression(c, apm);
        stmt->condition = condition;

        attempt_to_recover_at_next_code_block(c);
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
                EAT(KEYWORD_IF);

                segment_stmt->kind = ELSE_IF_SEGMENT;

                Expression *condition = parse_expression(c, apm);
                segment_stmt->condition = condition;

                attempt_to_recover_at_next_code_block(c);
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

        // For variable
        EAT(KEYWORD_FOR);

        Variable *iterator = append_variable(&apm->variable);
        stmt->iterator = iterator;

        substr identity = TOKEN_STRING();
        iterator->identity = identity;
        iterator->type.sort = INVALID_SORT;
        EAT(IDENTITY);

        // Iterable
        EAT(KEYWORD_IN);
        Expression *iterable = parse_expression(c, apm);
        stmt->iterable = iterable;

        // Body
        attempt_to_recover_at_next_code_block(c);
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
        Variable *var = append_variable(&apm->variable);
        var->type.sort = INVALID_SORT;

        stmt->kind = VARIABLE_DECLARATION;
        stmt->variable = var;
        stmt->has_initial_value = false;
        stmt->has_type_expression = false;

        EAT(KEYWORD_DEF);

        substr identity = TOKEN_STRING();
        var->identity = identity;
        EAT(IDENTITY);

        if (PEEK(EQUAL))
        {
            EAT(EQUAL);

            Expression *initial_value = parse_expression(c, apm);
            stmt->initial_value = initial_value;
            if (c->parse_status == PANIC)
                goto recover;

            stmt->has_initial_value = true;
        }
        else
        {
            // TODO: Error
        }

        EAT(SEMI_COLON);

        goto finish;
    }

    // OUTPUT_STATEMENT
    if (PEEK(ARROW_R))
    {
        stmt->kind = OUTPUT_STATEMENT;

        EAT(ARROW_R);
        Expression *value = parse_expression(c, apm);
        stmt->expression = value;
        if (c->parse_status == PANIC)
            goto recover;

        EAT(SEMI_COLON);
        goto finish;
    }

    // RETURN_STATEMENT
    if (PEEK(KEYWORD_RETURN))
    {
        stmt->kind = RETURN_STATEMENT;

        EAT(KEYWORD_RETURN);
        Expression *value = parse_expression(c, apm);
        stmt->expression = value;
        if (c->parse_status == PANIC)
            goto recover;

        EAT(SEMI_COLON);
        goto finish;
    }

    // EXPRESSION_STMT / ASSIGNMENT_STATEMENT / VARIABLE_DECLARATION with stated type
    else if (peek_expression(c))
    {
        stmt->kind = EXPRESSION_STMT;

        // EXPRESSION_STMT
        Expression *value = parse_expression(c, apm);
        stmt->expression = value;
        if (c->parse_status == PANIC)
            goto recover;

        if (PEEK(IDENTITY)) // VARIABLE_DECLARATION with stated type
        {
            Variable *var = append_variable(&apm->variable);
            var->type.sort = INVALID_SORT;

            stmt->kind = VARIABLE_DECLARATION;
            stmt->variable = var;
            stmt->type_expression = value;
            stmt->has_type_expression = true;
            stmt->has_initial_value = false;

            substr identity = TOKEN_STRING();
            var->identity = identity;
            EAT(IDENTITY);

            if (PEEK(EQUAL))
            {
                EAT(EQUAL);

                Expression *initial_value = parse_expression(c, apm);
                stmt->initial_value = initial_value;
                if (c->parse_status == PANIC)
                    goto recover;

                stmt->has_initial_value = true;
            }
        }
        else if (PEEK(EQUAL)) // ASSIGNMENT_STATEMENT
        {
            stmt->kind = ASSIGNMENT_STATEMENT;
            stmt->assignment_lhs = value;

            EAT(EQUAL);

            Expression *rhs = parse_expression(c, apm);
            stmt->assignment_rhs = rhs;
            if (c->parse_status == PANIC)
                goto recover;
        }

        EAT(SEMI_COLON);
        goto finish;
    }

    // INVALID_STATEMENT
    stmt->kind = INVALID_STATEMENT;
    raise_parse_error(c, EXPECTED_STATEMENT);

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

Block *parse_top_level_declarations(Compiler *c, Program *apm)
{
    Block *top_block = append_block(&apm->block);
    top_block->declaration_block = true;
    top_block->singleton_block = false;

    top_block->symbol_table = add_symbol_table(&apm->symbol_table);
    init_symbol_table(SYMBOL_TABLE(top_block->symbol_table));

    StatementListAllocator statements_allocator;
    init_statement_list_allocator(&statements_allocator, &apm->statement_lists, 512); // FIXME: 512 was chosen arbitrarily

    while (true)
    {
        if (PEEK(END_OF_FILE))
            break;

        else if (PEEK(KEYWORD_FN))
            parse_function(c, apm, top_block, &statements_allocator);
        else if (PEEK(KEYWORD_ENUM))
            parse_enum_type(c, apm, top_block, &statements_allocator);
        else if (PEEK(KEYWORD_STRUCT))
            parse_struct_type(c, apm, top_block, &statements_allocator);

        else if (PEEK(KEYWORD_DEF))
        {
            // TODO: This is mostly copy/pasted from parse_statement, though with
            //       some modifications. Find an effective way to clean up?

            Statement *stmt = append_statement(&statements_allocator);
            START_SPAN(stmt);

            Variable *var = append_variable(&apm->variable);
            var->type.sort = INVALID_SORT;

            stmt->kind = VARIABLE_DECLARATION;
            stmt->variable = var;
            stmt->has_initial_value = false;
            stmt->has_type_expression = false;

            EAT(KEYWORD_DEF);

            substr identity = TOKEN_STRING();
            var->identity = identity;
            EAT(IDENTITY);

            append_symbol(apm, top_block->symbol_table, VARIABLE_SYMBOL, (SymbolPointer){.variable = var}, identity);

            if (PEEK(EQUAL))
            {
                EAT(EQUAL);

                Expression *initial_value = parse_expression(c, apm);
                stmt->initial_value = initial_value;
                if (c->parse_status == PANIC)
                {
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

                    continue;
                }

                stmt->has_initial_value = true;
            }
            else
            {
                // TODO: Error
            }

            EAT(SEMI_COLON);
        }

        else
        {
            // FIXME: What is the appropriate error to express here?
            raise_parse_error(c, EXPECTED_FUNCTION);

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

                ADVANCE();

                if (PEEK(KEYWORD_FN))
                    c->parse_status = RECOVERED;

                if (PEEK(KEYWORD_ENUM))
                    c->parse_status = RECOVERED;
            }
        }
    }

    top_block->statements = get_statement_list(statements_allocator);

    size_t last_table = top_block->symbol_table;
    while (SYMBOL_TABLE(last_table)->next)
        last_table = SYMBOL_TABLE(last_table)->next;
    SYMBOL_TABLE(last_table)->next = apm->global_symbol_table;

    return top_block;
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

        // TODO: Make this more efficient
        //       Currently we create a new symbol table for every block,
        //       meaning we create numerous completely empty tables.
        block->symbol_table = add_symbol_table(&apm->symbol_table);
        SYMBOL_TABLE(block->symbol_table)->next = 0;
        SYMBOL_TABLE(block->symbol_table)->symbol_count = 0;

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

        size_t last_table = block->symbol_table;
        while (SYMBOL_TABLE(last_table)->next)
            last_table = SYMBOL_TABLE(last_table)->next;
        SYMBOL_TABLE(last_table)->next = parent->symbol_table;

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
                                                                                                               \
        ADVANCE();                                                                                             \
                                                                                                               \
        expr->lhs = lhs;                                                                                       \
        Expression *rhs = parse_expression_with_precedence(c, apm, precedence_of(expr_kind));                  \
        expr->rhs = rhs;                                                                                       \
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

        Expression *operand = parse_expression_with_precedence(c, apm, precedence_of(UNARY_POS));
        lhs->operand = operand;
    }
    else if (PEEK(MINUS))
    {
        lhs->kind = UNARY_NEG;
        ADVANCE();

        Expression *operand = parse_expression_with_precedence(c, apm, precedence_of(UNARY_NEG));
        lhs->operand = operand;
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

            size_t first_argument = apm->argument.count;
            expr->arguments.first = first_argument;

            ADVANCE();
            while (peek_expression(c))
            {
                size_t arg = add_argument(&apm->argument);
                ARGUMENT(arg)->expr = parse_expression(c, apm);

                if (!PEEK(COMMA))
                    break;

                EAT(COMMA);
            }
            EAT(PAREN_R);

            size_t argument_count = apm->argument.count - first_argument;
            expr->arguments.count = argument_count;
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

            ADVANCE();

            expr->field = TOKEN_STRING();
            EAT(IDENTITY);
        }

        else if (PEEK(TWO_DOT) && LEFT_ASSOCIATIVE_OPERATOR_BINDS(precedence_of(RANGE_LITERAL), caller_precedence))
        {
            expr->kind = RANGE_LITERAL;

            ADVANCE();

            expr->first = lhs;
            Expression *last = parse_expression_with_precedence(c, apm, precedence_of(RANGE_LITERAL));
            expr->last = last;
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
            // NOTE: We don't actually make the memory that was reserved available again.
            //       We _could_ do this, but it's enough of an edge case I don't think it's needed.
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
