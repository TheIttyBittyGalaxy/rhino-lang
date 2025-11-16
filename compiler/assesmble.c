#include "assemble.h"

#include "fatal_error.h"

// ASSEMBLER //

typedef struct
{
    void *node;
    uint8_t reg;
} NodeRegister;

typedef struct Assembler Assembler;

struct Assembler
{
    const char *source_text;
    Program *apm;

    Assembler *parent;
    Unit *unit;

    uint8_t active_registers;

    NodeRegister node_register[256];
    size_t node_register_count;

    size_t loop_depth;
    size_t jump_to_end_of_loop[128][128];
    size_t jump_to_end_of_loop_count[128];
};

void init_assembler_and_create_unit(Assembler *a, Assembler *parent, Program *apm, const char *source_text)
{
    // TODO: Implement a proper system for managing this memory
    a->unit = (Unit *)malloc(sizeof(Unit));
    init_unit(a->unit);

    a->parent = parent;
    a->apm = apm;
    a->source_text = source_text;

    a->active_registers = 0;

    a->node_register_count = 0;

    a->loop_depth = 0;
}

vm_reg reserve_register(Assembler *a)
{
    return a->active_registers++;
}

vm_reg reserve_register_for_node(Assembler *a, void *node)
{
    vm_reg reg = reserve_register(a);
    a->node_register[a->node_register_count++] = (NodeRegister){
        .node = node,
        .reg = reg,
    };
    return reg;
}

void release_register(Assembler *a)
{
    assert(a->active_registers > 0);
    a->active_registers--;
}

vm_reg get_node_register(Assembler *a, void *node)
{
    for (size_t i = 0; i < a->active_registers; i++)
        if (a->node_register[i].node == node)
            return i;

    fatal_error("Could not get register for APM node %p.", node);
    unreachable;
}

// EMIT INSTRUCTION //

size_t emit(Unit *unit, OpCode op)
{
    size_t i = unit->count++;
    unit->instruction[i].op = op;
    return i;
}

size_t emit_a(Unit *unit, OpCode op, vm_reg a)
{
    size_t i = unit->count++;
    unit->instruction[i].op = op;
    unit->instruction[i].a = a;
    return i;
}

size_t emit_x(Unit *unit, OpCode op, uint16_t x)
{
    size_t i = unit->count++;
    unit->instruction[i].op = op;
    unit->instruction[i].x = x;
    return i;
}

size_t emit_ab(Unit *unit, OpCode op, vm_reg a, vm_reg b)
{
    size_t i = unit->count++;
    unit->instruction[i].op = op;
    unit->instruction[i].a = a;
    unit->instruction[i].b = b;
    return i;
}

size_t emit_ab(Unit *unit, OpCode op, vm_reg a, vm_reg b, vm_reg c)
{
    size_t i = unit->count++;
    unit->instruction[i].op = op;
    unit->instruction[i].a = a;
    unit->instruction[i].b = b;
    unit->instruction[i].c = c;
    return i;
}

size_t emit_ax(Unit *unit, OpCode op, vm_reg a, uint16_t x)
{
    size_t i = unit->count++;
    unit->instruction[i].op = op;
    unit->instruction[i].a = a;
    unit->instruction[i].x = x;
    return i;
}

#define EMIT(op) emit(unit, op)
#define EMIT_A(op, a) emit_a(unit, op, a)
#define EMIT_X(op, x) emit_x(unit, op, x)
#define EMIT_AB(op, a, b) emit_ab(unit, op, a, b)
#define EMIT_ABC(op, a, b, c) emit_ab(unit, op, a, b, c)
#define EMIT_AX(op, a, x) emit_ax(unit, op, a, x)

// EMIT DATA //

#define EMIT_DATA(T, value)                                     \
    {                                                           \
        assert(wordsizeof(T) <= 2);                             \
        union                                                   \
        {                                                       \
            T data;                                             \
            uint32_t word[2];                                   \
        } as = {.data = value};                                 \
                                                                \
        unit->instruction[unit->count++].word = as.word[0];     \
        if (wordsizeof(T) == 2)                                 \
            unit->instruction[unit->count++].word = as.word[1]; \
    }

// PATCH INSTRUCTION //

void patch(Unit *unit, Instruction ins, size_t location)
{
    unit->instruction[location] = ins;
}

// ASSEMBLE EXPRESSION //
vm_reg get_register_of_expression(Assembler *a, Expression *expr)
{
    switch (expr->kind)
    {
    case IDENTITY_LITERAL:
        fatal_error("Attempt to determine the register of an IDENTITY_LITERAL expression. By this pass we would have expected this to become a VARIABLE_REFERENCE.");
        break;

    case VARIABLE_REFERENCE:
        return get_node_register(a, expr->variable);
        break;

    case PARAMETER_REFERENCE:
        return get_node_register(a, expr->parameter);
        break;

        // TODO: Implement
        // case INDEX_BY_FIELD:

    default:
        fatal_error("Attempt to determine the register of a %s expression.", expression_kind_string(expr->kind));
        break;
    }

    unreachable;
}

void assemble_default_value(Assembler *a, RhinoType ty, vm_reg dest)
{
    Unit *unit = a->unit;
    Program *apm = a->apm;

    switch (ty.tag)
    {
    case RHINO_NATIVE_TYPE:
        if (IS_BOOL_TYPE(ty))
        {
            EMIT_A(LOAD_FALSE, dest);
        }
        else if (IS_INT_TYPE(ty))
        {
            EMIT_A(LOAD_INT, dest);
            EMIT_DATA(int, 0);
        }
        else if (IS_NUM_TYPE(ty))
        {
            EMIT_A(LOAD_NUM, dest);
            EMIT_DATA(double, 0);
        }
        // else if (IS_STR_TYPE(ty))
        else
        {
            fatal_error("Could not transpile default value for value of native type %s.", ty.native_type->name);
        }

        break;

        // TODO: Implement
        // case RHINO_ENUM_TYPE:

        // TODO: Implement
        // case RHINO_STRUCT_TYPE:

    default:
        fatal_error("Could not transpile default value for value of type %s.", rhino_type_string(apm, ty));
    }
}

void assemble_expression(Assembler *a, Expression *expr, vm_reg dest)
{
    Unit *unit = a->unit;

    switch (expr->kind)
    {
    case IDENTITY_LITERAL:
        fatal_error("Attempt to assemble an IDENTITY_LITERAL expression. By this pass we would have expected this to become a VARIABLE_REFERENCE.");
        break;

    case NONE_LITERAL:
        EMIT_A(LOAD_NONE, dest);
        break;

    case BOOLEAN_LITERAL:
        EMIT_A(expr->bool_value ? LOAD_TRUE : LOAD_FALSE, dest);
        break;

    case INTEGER_LITERAL:
        EMIT_A(LOAD_INT, dest);
        EMIT_DATA(int, expr->integer_value);
        break;

    case FLOAT_LITERAL:
        EMIT_A(LOAD_NUM, dest);
        EMIT_DATA(double, expr->float_value);
        break;

    case STRING_LITERAL:
    {
        // TODO: Do something more efficient than this!!
        substr sub = expr->string_value;
        char *buffer = (char *)malloc(sizeof(char) * (sub.len + 1));
        memcpy(buffer, a->source_text + sub.pos, sub.len);
        buffer[sub.len] = '\0';

        EMIT_A(LOAD_STR, dest);
        EMIT_DATA(char *, buffer);
        break;
    }

        // TODO: Implement
        // case ENUM_VALUE_LITERAL:

    case VARIABLE_REFERENCE:
        EMIT_AB(MOVE, dest, get_node_register(a, expr->variable));
        break;

        // TODO: Implement
        // case PARAMETER_REFERENCE:

        // TODO: Implement
        // case FUNCTION_CALL:

        // TODO: Implement
        // case INDEX_BY_FIELD:

    case UNARY_POS:
        assemble_expression(a, expr->operand, dest);
        break;

    case UNARY_NEG:
        assemble_expression(a, expr->operand, dest);
        EMIT_AB(OP_NEG, dest, dest);
        break;

    case UNARY_NOT:
        assemble_expression(a, expr->operand, dest);
        EMIT_AB(OP_NOT, dest, dest);
        break;

    case UNARY_INCREMENT:
    {
        vm_reg reg = get_register_of_expression(a, expr->subject);
        EMIT_AB(MOVE, dest, reg);
        EMIT_A(INCREMENT, reg);
        break;
    }

    case UNARY_DECREMENT:
    {
        vm_reg reg = get_register_of_expression(a, expr->subject);
        EMIT_AB(MOVE, dest, reg);
        EMIT_A(DECREMENT, reg);
        break;
    }

    // FIXME: I'm fairly certain we don't actually need to reserve two registers for this, but I can't figure out the logic for that right now
#define CASE_BINARY(expr_kind, ins)             \
    case expr_kind:                             \
    {                                           \
        vm_reg lhs = reserve_register(a);       \
        assemble_expression(a, expr->lhs, lhs); \
                                                \
        vm_reg rhs = reserve_register(a);       \
        assemble_expression(a, expr->rhs, rhs); \
                                                \
        EMIT_ABC(ins, dest, lhs, rhs);          \
                                                \
        release_register(a);                    \
        release_register(a);                    \
        break;                                  \
    }

        CASE_BINARY(BINARY_MULTIPLY, OP_MULTIPLY)
        CASE_BINARY(BINARY_DIVIDE, OP_DIVIDE)
        CASE_BINARY(BINARY_REMAINDER, OP_REMAINDER)
        CASE_BINARY(BINARY_ADD, OP_ADD)
        CASE_BINARY(BINARY_SUBTRACT, OP_SUBTRACT)
        CASE_BINARY(BINARY_LESS_THAN, OP_LESS_THAN)
        CASE_BINARY(BINARY_GREATER_THAN, OP_GREATER_THAN)
        CASE_BINARY(BINARY_LESS_THAN_EQUAL, OP_LESS_THAN_EQUAL)
        CASE_BINARY(BINARY_GREATER_THAN_EQUAL, OP_GREATER_THAN_EQUAL)
        CASE_BINARY(BINARY_EQUAL, OP_EQUAL)
        CASE_BINARY(BINARY_NOT_EQUAL, OP_NOT_EQUAL)
        CASE_BINARY(BINARY_LOGICAL_AND, OP_LOGICAL_AND)
        CASE_BINARY(BINARY_LOGICAL_OR, OP_LOGICAL_OR)

#undef CASE_BINARY

    default:
        fatal_error("Could not assemble %s expression.", expression_kind_string(expr->kind));
        break;
    }
}

// ASSEMBLE CODE BLOCK //

void assemble_code_block(Assembler *a, Block *block)
{
    assert(!block->declaration_block);
    Unit *unit = a->unit;

    Statement *stmt;
    StatementIterator it = statement_iterator(block->statements);
    while (stmt = next_statement_iterator(&it))
    {
        // Track JUMP instructions to the end of the loop
        if (stmt->kind == FOR_LOOP ||
            stmt->kind == WHILE_LOOP ||
            stmt->kind == BREAK_LOOP)
        {
            a->loop_depth++;
            a->jump_to_end_of_loop_count[a->loop_depth] = 0;
        }

        // Generate the statement
        switch (stmt->kind)
        {

        case FUNCTION_DECLARATION:
        case ENUM_TYPE_DECLARATION:
        case STRUCT_TYPE_DECLARATION:
            break;

        case CODE_BLOCK:
            assemble_code_block(a, stmt->block);
            break;

        case IF_SEGMENT:
        {
            size_t jump_to_end[128];
            size_t jump_to_end_count = 0;

            Statement *segment = stmt;
            while (segment)
            {
                if (segment->kind == ELSE_SEGMENT)
                {
                    assemble_code_block(a, segment->block);
                    break;
                }

                vm_reg condition_result = reserve_register(a);
                assemble_expression(a, segment->condition, condition_result);
                release_register(a);

                size_t jump_to_next_segment = EMIT_AX(JUMP_IF_FALSE, condition_result, 0xFFFF); // Jump over this segment if the condition fails

                assemble_code_block(a, segment->body);
                if (segment->next) // Jump to the end of the if statement
                    jump_to_end[jump_to_end_count++] = EMIT_X(JUMP, 0xFFFF);

                // FIXME: Create a helper function for this
                unit->instruction[jump_to_next_segment].x = unit->count;

                segment = segment->next;
            }

            for (size_t i = 0; i < jump_to_end_count; i++)
                unit->instruction[jump_to_end[i]].x = unit->count; // FIXME: Create a helper function for this

            break;
        }

        case ELSE_IF_SEGMENT:
        case ELSE_SEGMENT:
            break;

        case BREAK_LOOP:
        {
            size_t jump_to_start = unit->count;
            assemble_code_block(a, stmt->block);
            EMIT_X(JUMP, jump_to_start);
            break;
        }

        case FOR_LOOP:
        {
            Variable *iterator = stmt->iterator;
            Expression *iterable = stmt->iterable;

            // FIXME: Can the two jumps in this loop be combined into one?
            if (iterable->kind == RANGE_LITERAL)
            {
                // Reserve a register for the iterator
                vm_reg iterator_reg = reserve_register_for_node(a, (void *)iterator);

                // Initialise iterator to the first value in the range
                assemble_expression(a, iterable->first, iterator_reg);

                size_t start_of_loop = unit->count;

                // Check condition and jump to end if false
                vm_reg condition_reg = reserve_register(a);
                assemble_expression(a, iterable->last, condition_reg);
                EMIT_ABC(OP_LESS_THAN_EQUAL, condition_reg, iterator_reg, condition_reg);

                size_t jump_to_end = EMIT_AX(JUMP_IF_FALSE, condition_reg, 0xFFFF);
                release_register(a); // condition_reg

                // Assemble block, incrementing the iterator once done
                assemble_code_block(a, stmt->body);
                EMIT_A(INCREMENT, iterator_reg);

                // Jump back to the start of the loop
                EMIT_X(JUMP, start_of_loop);

                // FIXME: Create a helper function for this
                unit->instruction[jump_to_end].x = unit->count;

                release_register(a); // iterator_reg

                break;
            }

            fatal_error("Could not assemble for loop with %s iterable.", expression_kind_string(iterable->kind));
            break;
        }

        case WHILE_LOOP:
        {
            size_t start_of_loop = unit->count;

            vm_reg condition_reg = reserve_register(a);
            assemble_expression(a, stmt->condition, condition_reg);
            size_t jump_to_end = EMIT_AX(JUMP_IF_FALSE, condition_reg, 0xFFFF);
            release_register(a);

            assemble_code_block(a, stmt->block);
            EMIT_X(JUMP, start_of_loop);

            // FIXME: Create a helper function for this
            unit->instruction[jump_to_end].x = unit->count;
            break;
        }

        case BREAK_STATEMENT:
        {
            size_t unpatched_jump = EMIT_X(JUMP, 0xFFFF);
            size_t i = a->jump_to_end_of_loop_count[a->loop_depth]++;
            a->jump_to_end_of_loop[a->loop_depth][i] = unpatched_jump;
            break;
        }

        case ASSIGNMENT_STATEMENT:
        {
            vm_reg dest = get_register_of_expression(a, stmt->assignment_lhs);
            vm_reg src = reserve_register(a);
            assemble_expression(a, stmt->assignment_rhs, src);
            release_register(a);
            EMIT_AB(MOVE, dest, src);
            break;
        }

        case VARIABLE_DECLARATION:
        {
            // FIXME: I think it's fine that we never release this, but check?
            vm_reg variable_reg = reserve_register_for_node(a, (void *)stmt->variable);

            if (stmt->initial_value)
                assemble_expression(a, stmt->initial_value, variable_reg);
            else
                assemble_default_value(a, stmt->variable->type, variable_reg);

            break;
        }

        case OUTPUT_STATEMENT:
        {
            vm_reg reg = reserve_register(a);
            assemble_expression(a, stmt->expression, reg);
            EMIT_A(OUTPUT_VALUE, reg);
            release_register(a);
            break;
        }

        // TODO: Is there a way of doing this without the reserving a register?
        case EXPRESSION_STMT:
        {
            vm_reg reg = reserve_register(a);
            assemble_expression(a, stmt->expression, reg);
            release_register(a);
            break;
        }

            // TODO: Implement
            // case RETURN_STATEMENT:

        default:
            fatal_error("Could not assemble %s statement in code block.", statement_kind_string(stmt->kind));
            break;
        }

        // Hot patch any jumps to the end of the block
        if (stmt->kind == FOR_LOOP ||
            stmt->kind == WHILE_LOOP ||
            stmt->kind == BREAK_LOOP)
        {
            for (size_t i = 0; i < a->jump_to_end_of_loop_count[a->loop_depth]; i++)
            {
                size_t j = a->jump_to_end_of_loop[a->loop_depth][i];
                // FIXME: Create a helper function for this
                unit->instruction[j].x = unit->count;
            }
            a->loop_depth--;
        }
    }
}

// ASSEMBLE FUNCTION //

Unit *assemble_function(Assembler *global, ByteCode *bc, Function *funct)
{
    Assembler a;
    init_assembler_and_create_unit(&a, global, global->apm, global->source_text);
    assemble_code_block(&a, funct->body);
    return a.unit;
}

// ASSEMBLE PROGRAM //

void assemble_program(ByteCode *bc, Program *apm, const char *source_text)
{
    // Create init unit
    Assembler assembler;
    Assembler *a = &assembler;

    init_assembler_and_create_unit(a, NULL, apm, source_text);
    Unit *unit = assembler.unit;

    bc->init = unit;

    // Initialise global variables
    Statement *stmt;
    StatementIterator it = statement_iterator(apm->program_block->statements);
    while (stmt = next_statement_iterator(&it))
    {
        if (stmt->kind != VARIABLE_DECLARATION)
            continue;

        // TODO: This code was copy/pasted from assemble_code_block - is there a better way to factor this?
        // FIXME: I think it's fine that we never release this, but check?
        vm_reg variable_reg = reserve_register_for_node(a, (void *)stmt->variable);

        if (stmt->initial_value)
            assemble_expression(a, stmt->initial_value, variable_reg);
        else
            assemble_default_value(a, stmt->variable->type, variable_reg);
    }

    // Assemble main
    bc->main = assemble_function(a, bc, apm->main);

    // Call to main from the init unit
    EMIT(CALL);
    EMIT_DATA(Unit *, bc->main);
}

// ASSEMBLE //

void assemble(Compiler *compiler, Program *apm, ByteCode *byte_code)
{
    assemble_program(byte_code, apm, compiler->source_text);
}