#include "assemble.h"

#include "fatal_error.h"

// ASSEMBLER //

typedef struct
{
    uint8_t up;
    vm_reg reg;
} vm_loc;

typedef struct
{
    void *node;
    vm_reg reg;
} NodeRegister;

typedef struct
{
    void *type;
    Unit *value_to_str;
} TypeData;

typedef struct
{
    Function *funct;
    Unit *unit;
} FunctionUnit;

typedef struct
{
    Unit *unit;
    size_t instruction;
    Function *funct;
} CallPatch;

typedef struct
{
    const char *source_text;
    Program *apm;

    Unit *last_unit;

    // TODO: Make this a dynamically sized array
    CallPatch call_patch[128];
    size_t call_patch_count;

    // TODO: Make this a dynamically sized array
    FunctionUnit function_unit[128];
    size_t function_unit_count;

    // TODO: Make this a dynamically sized hash map or the like
    EnumValue *enum_int[256];
    size_t enum_int_count;

    TypeData type_data[256];
    size_t type_data_count;
} GlobalAssemblerData;

typedef struct Assembler Assembler;

struct Assembler
{
    GlobalAssemblerData *data;
    Assembler *parent;

    Unit *unit;

    uint8_t active_registers;

    NodeRegister node_register[256];
    size_t node_register_count;

    size_t loop_depth;
    size_t jump_to_end_of_loop[128][128];
    size_t jump_to_end_of_loop_count[128];
};

void init_assembler_and_create_unit(Assembler *a, Assembler *parent, GlobalAssemblerData *data)
{
    // TODO: Implement a proper system for managing this memory
    a->unit = (Unit *)malloc(sizeof(Unit));
    init_unit(a->unit);

    a->parent = parent;
    if (parent)
    {
        assert(data == NULL);
        a->data = parent->data;
        a->unit->nested_in = parent->unit;
    }
    else
    {
        assert(data != NULL);
        a->data = data;
        // a->unit->nested_in = NULL; // Not needed, already done in init_unit
    }

    if (a->data->last_unit)
        a->data->last_unit->next = a->unit;
    a->data->last_unit = a->unit;

    a->active_registers = 0;

    a->node_register_count = 0;

    a->loop_depth = 0;
}

vm_reg reserve_register(Assembler *a)
{
    vm_reg reg = a->active_registers++;
    if (a->active_registers > a->unit->register_count)
        a->unit->register_count = a->active_registers;
    return reg;
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

#define local(r) ((vm_loc){.up = 0, .reg = r})

void release_register(Assembler *a)
{
    assert(a->active_registers > 0);
    a->active_registers--;
}

Unit *get_unit_of_function(Assembler *a, Function *funct)
{
    for (size_t i = 0; i < a->data->function_unit_count; i++)
        if (a->data->function_unit[i].funct == funct)
            return a->data->function_unit[i].unit;

    fatal_error("Could not get unit for APM function %p.", funct);
    unreachable;
}

vm_loc get_node_location(Assembler *a, void *node)
{
    size_t up = 0;
    while (a)
    {
        for (size_t i = 0; i < a->node_register_count; i++)
            if (a->node_register[i].node == node)
                return (vm_loc){.up = (uint8_t)up, .reg = (vm_reg)i};

        a = a->parent;
        assert(up < 255); // FIXME: Handle this properly
        up++;
    }

    fatal_error("Could not get register for APM node %p.", node);
    unreachable;
}

size_t get_enum_int(Assembler *a, EnumValue *enum_value)
{
    for (size_t i = 0; i < a->data->enum_int_count; i++)
        if (a->data->enum_int[i] == enum_value)
            return i;

    fatal_error("Could not get int of enum value %p.", enum_value);
    unreachable;
}

TypeData get_type_data(Assembler *a, void *type)
{
    for (size_t i = 0; i < a->data->type_data_count; i++)
        if (a->data->type_data[i].type == type)
            return a->data->type_data[i];

    fatal_error("Could not get type data for %p.", type);
    unreachable;
}

// EMIT INSTRUCTIONS //

#include "include/emit_op_code.c"

// Emit instructions that will set registers dst = src
size_t emit_copy_instructions(Assembler *a, vm_loc dst, vm_loc src)
{
    Unit *unit = a->unit;

    if (src.up == dst.up && src.reg == dst.reg)
        return unit->count;

    if (src.up == dst.up)
        return emit_copy(unit, dst.up, dst.reg, src.reg);
    else if (src.up == 0)
        return emit_copy_up(unit, dst.up, dst.reg, src.reg);
    else if (dst.up == 0)
        return emit_copy_dn(unit, src.up, dst.reg, src.reg);

    vm_reg tmp = reserve_register(a);
    emit_copy_instructions(a, local(tmp), src);
    size_t last_ins = emit_copy_instructions(a, dst, local(tmp));
    release_register(a);

    return last_ins;
}

// PATCH INSTRUCTIONS //

void patch_y(Unit *unit, size_t location, uint16_t y)
{
    unit->instruction[location].y = y;
}

#include "include/patch_payload.c"

// ASSEMBLE EXPRESSION //

vm_loc get_register_of_expression(Assembler *a, Expression *expr)
{
    switch (expr->kind)
    {
    case IDENTITY_LITERAL:
        fatal_error("Attempt to determine the register of an IDENTITY_LITERAL expression. By this pass we would have expected this to become a VARIABLE_REFERENCE.");
        break;

    case VARIABLE_REFERENCE:
        return get_node_location(a, expr->variable);
        break;

    case PARAMETER_REFERENCE:
        return get_node_location(a, expr->parameter);
        break;

        // TODO: Implement
        // case INDEX_BY_FIELD:

    default:
        fatal_error("Attempt to determine the register of a %s expression.", expression_kind_string(expr->kind));
        break;
    }

    unreachable;
}

void assemble_default_value(Assembler *a, RhinoType ty, vm_loc loc)
{
    Unit *unit = a->unit;
    Program *apm = a->data->apm;

    switch (ty.tag)
    {
    case RHINO_NATIVE_TYPE:
        if (IS_BOOL_TYPE(ty))
            emit_load_false(unit, loc.up, loc.reg);
        else if (IS_INT_TYPE(ty))
            emit_load_num(unit, loc.up, loc.reg, 0);
        else if (IS_NUM_TYPE(ty))
            emit_load_num(unit, loc.up, loc.reg, 0);
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

// TODO: Rework this method to return a register location, rather than receiving one
void assemble_expression(Assembler *a, Expression *expr, vm_loc dst)
{
    Unit *unit = a->unit;
    Program *apm = a->data->apm;

    switch (expr->kind)
    {
    case IDENTITY_LITERAL:
        fatal_error("Attempt to assemble an IDENTITY_LITERAL expression. By this pass we would have expected this to become a VARIABLE_REFERENCE.");
        break;

    case NONE_LITERAL:
        emit_load_none(unit, dst.up, dst.reg);
        break;

    case BOOLEAN_LITERAL:
        if (expr->bool_value)
            emit_load_true(unit, dst.up, dst.reg);
        else
            emit_load_false(unit, dst.up, dst.reg);
        break;

    case INTEGER_LITERAL:
        emit_load_num(unit, dst.up, dst.reg, expr->integer_value);
        break;

    case FLOAT_LITERAL:
        emit_load_num(unit, dst.up, dst.reg, expr->float_value);
        break;

    case STRING_LITERAL:
    {
        // TODO: Do something more efficient than this!!
        substr sub = expr->string_value;
        char *buffer = (char *)malloc(sizeof(char) * (sub.len + 1));
        memcpy(buffer, a->data->source_text + sub.pos, sub.len);
        buffer[sub.len] = '\0';

        emit_load_str(unit, dst.up, dst.reg, buffer);
        break;
    }

    case ENUM_VALUE_LITERAL:
        emit_load_enum(unit, dst.up, dst.reg, get_enum_int(a, expr->enum_value));
        break;

    case VARIABLE_REFERENCE:
    {
        vm_loc var = get_node_location(a, expr->variable);
        emit_copy_instructions(a, dst, var);
        break;
    }

    case PARAMETER_REFERENCE:
    {
        vm_loc par = get_node_location(a, expr->parameter);
        emit_copy_instructions(a, dst, par);
        break;
    }

    case FUNCTION_CALL:
    {
        if (expr->callee->kind != FUNCTION_REFERENCE)
            fatal_error("Could not assemble CALL expression whose callee is a %s.", expression_kind_string(expr->callee->kind));

        vm_reg first_arg_reg = a->active_registers;
        for (size_t i = 0; i < expr->arguments.count; i++)
        {
            Expression *arg = get_argument(expr->arguments, i)->expr;
            vm_reg arg_reg = reserve_register(a);
            assemble_expression(a, arg, local(arg_reg));
        }

        // TODO: Use OP_RUN in any case where the return value is not needed

        size_t call_ins = emit_call(unit, dst.up, dst.reg, first_arg_reg, 0X0);

        for (size_t i = 0; i < expr->arguments.count; i++)
            release_register(a);

        a->data->call_patch[a->data->call_patch_count++] = (CallPatch){
            .unit = unit,
            .instruction = call_ins + 1,
            .funct = expr->callee->function,
        };

        break;
    }

        // TODO: Implement
        // case INDEX_BY_FIELD:

    case UNARY_POS:
        assemble_expression(a, expr->operand, dst);
        break;

    case UNARY_NEG:
        assemble_expression(a, expr->operand, dst);
        emit_neg(unit, dst.up, dst.reg, dst.reg);
        break;

    case UNARY_NOT:
        assemble_expression(a, expr->operand, dst);
        emit_not(unit, dst.up, dst.reg, dst.reg);
        break;

    case UNARY_INCREMENT:
    {
        vm_loc src = get_register_of_expression(a, expr->subject);
        emit_copy_instructions(a, dst, src);
        emit_inc(unit, src.up, src.reg);
        break;
    }

    case UNARY_DECREMENT:
    {
        vm_loc src = get_register_of_expression(a, expr->subject);
        emit_copy_instructions(a, dst, src);
        emit_dec(unit, src.up, src.reg);
        break;
    }

    // FIXME: I'm fairly certain we don't actually need to reserve two registers for this, but I can't figure out the logic for that right now

    // FIXME: Currently we are not correctly accounting for the possibility that any of the values involved might be in an upwards scope
#define CASE_BINARY(expr_kind, emit_ins)               \
    case expr_kind:                                    \
    {                                                  \
        assert(dst.up == 0);                           \
                                                       \
        vm_reg lhs = reserve_register(a);              \
        assemble_expression(a, expr->lhs, local(lhs)); \
                                                       \
        vm_reg rhs = reserve_register(a);              \
        assemble_expression(a, expr->rhs, local(rhs)); \
                                                       \
        emit_ins(unit, dst.reg, lhs, rhs);             \
                                                       \
        release_register(a);                           \
        release_register(a);                           \
        break;                                         \
    }

        CASE_BINARY(BINARY_MULTIPLY, emit_mul)
        CASE_BINARY(BINARY_DIVIDE, emit_div)
        CASE_BINARY(BINARY_REMAINDER, emit_rem)
        CASE_BINARY(BINARY_ADD, emit_add)
        CASE_BINARY(BINARY_SUBTRACT, emit_sub)

        CASE_BINARY(BINARY_LESS_THAN, emit_less_thn)
        CASE_BINARY(BINARY_LESS_THAN_EQUAL, emit_less_eql)

        CASE_BINARY(BINARY_EQUAL, emit_eqla)
        CASE_BINARY(BINARY_NOT_EQUAL, emit_eqln)

        CASE_BINARY(BINARY_LOGICAL_AND, emit_and)
        CASE_BINARY(BINARY_LOGICAL_OR, emit_or)

    case BINARY_GREATER_THAN:
    {
        assert(dst.up == 0);

        vm_reg lhs = reserve_register(a);
        assemble_expression(a, expr->lhs, local(lhs));

        vm_reg rhs = reserve_register(a);
        assemble_expression(a, expr->rhs, local(rhs));

        emit_less_thn(unit, dst.reg, rhs, lhs);

        release_register(a);
        release_register(a);
        break;
    }

    case BINARY_GREATER_THAN_EQUAL:
    {
        assert(dst.up == 0);

        vm_reg lhs = reserve_register(a);
        assemble_expression(a, expr->lhs, local(lhs));

        vm_reg rhs = reserve_register(a);
        assemble_expression(a, expr->rhs, local(rhs));

        emit_less_eql(unit, dst.reg, rhs, lhs);

        release_register(a);
        release_register(a);
        break;
    }

#undef CASE_BINARY

    case TYPE_CAST:
    {
        RhinoType cast_from = get_expression_type(apm, a->data->source_text, expr->cast_expr);
        RhinoType cast_to = expr->cast_type;
        if (cast_from.tag == RHINO_NATIVE_TYPE && is_native_type(cast_to, &apm->str_type))
        {
            if (dst.up == 0)
            {
                assemble_expression(a, expr->cast_expr, local(dst.reg));
                emit_as_str(unit, 0, dst.reg, dst.reg);
            }
            else
            {
                vm_reg temp = reserve_register(a);
                assemble_expression(a, expr->cast_expr, local(temp));
                emit_as_str(unit, 0, temp, temp);
                emit_copy_instructions(a, dst, local(temp));
            }
        }
        else if (cast_from.tag == RHINO_ENUM_TYPE && is_native_type(cast_to, &apm->str_type))
        {
            EnumType *enum_type = cast_from.enum_type;
            Unit *value_to_str = get_type_data(a, (void *)enum_type).value_to_str;

            vm_reg param = dst.reg;
            if (dst.up != 0)
                param = reserve_register(a);
            assemble_expression(a, expr->cast_expr, local(param));
            emit_call(unit, dst.up, dst.reg, param, value_to_str);
            if (dst.up != 0)
                release_register(a);
        }
        else
        {
            fatal_error("Could not assemble type cast from %s to %s.", rhino_type_string(apm, cast_from), rhino_type_string(apm, cast_to));
        }
        break;
    }

    default:
        fatal_error("Could not assemble %s expression.", expression_kind_string(expr->kind));
        break;
    }
}

// META-DATA FOR ENUM VALUES //

void assemble_enum_types(Assembler *parent, Block *block)
{
    GlobalAssemblerData *d = parent->data;

    Statement *stmt;
    StatementIterator it = statement_iterator(block->statements);
    while (stmt = next_statement_iterator(&it))
    {
        if (stmt->kind != ENUM_TYPE_DECLARATION)
            continue;

        EnumType *enum_type = stmt->enum_type;

        Assembler value_to_str;
        init_assembler_and_create_unit(&value_to_str, parent, NULL);
        value_to_str.unit->parameter_count = 1;

        size_t type_id = d->type_data_count++;
        d->type_data[type_id].type = (void *)enum_type;
        d->type_data[type_id].value_to_str = value_to_str.unit;

        vm_reg parameter_reg = reserve_register(&value_to_str);
        vm_reg condition_reg = reserve_register(&value_to_str);

        for (size_t i = 0; i < enum_type->values.count; i++)
        {
            // Assign an int value to each enum
            size_t id = d->enum_int_count++;
            EnumValue *enum_value = get_enum_value(enum_type->values, i);
            d->enum_int[id] = enum_value;

            // Add to value to string unit
            emit_load_enum(value_to_str.unit, 0, condition_reg, id);
            emit_eqla(value_to_str.unit, condition_reg, condition_reg, parameter_reg);
            size_t jump_to_next_check = emit_jump_if(value_to_str.unit, condition_reg, 0XFFFF);

            substr sub = enum_value->identity; // TODO: Do something more efficient than this!!
            char *buffer = (char *)malloc(sizeof(char) * (sub.len + 1));
            memcpy(buffer, d->source_text + sub.pos, sub.len);
            buffer[sub.len] = '\0';
            emit_load_str(value_to_str.unit, 0, condition_reg, buffer);

            emit_rtnv(value_to_str.unit, 0, condition_reg);
            patch_y(value_to_str.unit, jump_to_next_check, value_to_str.unit->count);
        }
    }
}

// ASSEMBLE PROGRAM //

void assemble_code_block(Assembler *a, Block *block);
void assemble_function(Assembler *parent, Function *funct);

void assemble_code_block(Assembler *a, Block *block)
{
    assert(!block->declaration_block);
    Unit *unit = a->unit;

    Statement *stmt;
    StatementIterator it;

    // Create representations for all enum values declared in this block
    size_t initial_enum_int_count = a->data->enum_int_count;
    assemble_enum_types(a, block);

    // Assemble statements
    it = statement_iterator(block->statements);
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
                assemble_expression(a, segment->condition, local(condition_result));
                release_register(a);

                size_t jump_to_next_segment = emit_jump_if(unit, condition_result, 0xFFFF); // Jump over this segment if the condition fails

                assemble_code_block(a, segment->body);
                if (segment->next) // Jump to the end of the if statement
                    jump_to_end[jump_to_end_count++] = emit_jump(unit, 0xFFFF);

                patch_y(unit, jump_to_next_segment, unit->count);

                segment = segment->next;
            }

            for (size_t i = 0; i < jump_to_end_count; i++)
                patch_y(unit, jump_to_end[i], unit->count);

            break;
        }

        case ELSE_IF_SEGMENT:
        case ELSE_SEGMENT:
            break;

        case BREAK_LOOP:
        {
            size_t jump_to_start = unit->count;
            assemble_code_block(a, stmt->block);
            emit_jump(unit, jump_to_start);
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
                assemble_expression(a, iterable->first, local(iterator_reg));

                size_t start_of_loop = unit->count;

                // Check condition and jump to end if false
                vm_reg condition_reg = reserve_register(a);
                assemble_expression(a, iterable->last, local(condition_reg));
                emit_less_eql(unit, condition_reg, iterator_reg, condition_reg);

                size_t jump_to_end = emit_jump_if(unit, condition_reg, 0xFFFF);
                release_register(a); // condition_reg

                // Assemble block, incrementing the iterator once done
                assemble_code_block(a, stmt->body);
                emit_inc(unit, 0, iterator_reg);

                // Jump back to the start of the loop
                emit_jump(unit, start_of_loop);

                patch_y(unit, jump_to_end, unit->count);

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
            assemble_expression(a, stmt->condition, local(condition_reg));
            size_t jump_to_end = emit_jump_if(unit, condition_reg, 0xFFFF);
            release_register(a);

            assemble_code_block(a, stmt->block);
            emit_jump(unit, start_of_loop);

            patch_y(unit, jump_to_end, unit->count);
            break;
        }

        case BREAK_STATEMENT:
        {
            size_t unpatched_jump = emit_jump(unit, 0xFFFF);
            size_t i = a->jump_to_end_of_loop_count[a->loop_depth]++;
            a->jump_to_end_of_loop[a->loop_depth][i] = unpatched_jump;
            break;
        }

        case ASSIGNMENT_STATEMENT:
        {
            vm_reg src = reserve_register(a);
            assemble_expression(a, stmt->assignment_rhs, local(src));
            release_register(a);

            vm_loc dst = get_register_of_expression(a, stmt->assignment_lhs);
            emit_copy_instructions(a, dst, local(src));
            break;
        }

        case VARIABLE_DECLARATION:
        {
            // NOTE: This register is never released
            vm_reg variable_reg = reserve_register_for_node(a, (void *)stmt->variable);

            if (stmt->initial_value)
                assemble_expression(a, stmt->initial_value, local(variable_reg));
            else
                assemble_default_value(a, stmt->variable->type, local(variable_reg));

            break;
        }

        case OUTPUT_STATEMENT:
        {
            vm_reg reg = reserve_register(a);
            assemble_expression(a, stmt->expression, local(reg));
            emit_out(unit, 0, reg);
            release_register(a);
            break;
        }

        // TODO: Is there a way of doing this without the reserving a register?
        case EXPRESSION_STMT:
        {
            vm_reg reg = reserve_register(a);
            assemble_expression(a, stmt->expression, local(reg));
            release_register(a);
            break;
        }

        case RETURN_STATEMENT:
        {
            if (stmt->expression)
            {
                vm_reg reg = reserve_register(a);
                assemble_expression(a, stmt->expression, local(reg));
                emit_rtnv(unit, reg, 0);
                release_register(a);
            }
            else
            {
                emit_rtnn(unit);
            }
            break;
        }

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
                patch_y(unit, j, unit->count);
            }
            a->loop_depth--;
        }
    }

    // Assemble nested functions
    it = statement_iterator(block->statements);
    while (stmt = next_statement_iterator(&it))
    {
        if (stmt->kind == FUNCTION_DECLARATION)
            assemble_function(a, stmt->function);
    }

    // Release representations for enum values
    a->data->enum_int_count = initial_enum_int_count;
}

void assemble_function(Assembler *parent, Function *funct)
{
    Assembler a;
    init_assembler_and_create_unit(&a, parent, NULL);

    a.data->function_unit[a.data->function_unit_count++] = (FunctionUnit){
        .funct = funct,
        .unit = a.unit,
    };

    a.unit->parameter_count = funct->parameters.count;

    // NOTE: These registers are never released
    Parameter *parameter;
    ParameterIterator it = parameter_iterator(funct->parameters);
    while (parameter = next_parameter_iterator(&it))
        reserve_register_for_node(&a, (void *)parameter);

    assemble_code_block(&a, funct->body);
}

void assemble_program(Assembler *a, ByteCode *bc, Program *apm)
{
    Unit *unit = a->unit;
    Statement *stmt;
    StatementIterator it;

    // Create representations for enum values
    assemble_enum_types(a, apm->program_block);

    // Initialise global variables in the init unit
    // FIXME: This approach cannot handle global variable that have been declared "out of order"
    it = statement_iterator(apm->program_block->statements);
    while (stmt = next_statement_iterator(&it))
    {
        if (stmt->kind != VARIABLE_DECLARATION)
            continue;

        // TODO: This code was copy/pasted from assemble_code_block - is there a better way to factor this?
        // FIXME: I think it's fine that we never release this, but check?
        vm_reg variable_reg = reserve_register_for_node(a, (void *)stmt->variable);

        if (stmt->initial_value)
            assemble_expression(a, stmt->initial_value, local(variable_reg));
        else
            assemble_default_value(a, stmt->variable->type, local(variable_reg));
    }

    // Assemble all functions declared in the global scope
    it = statement_iterator(apm->program_block->statements);
    while (stmt = next_statement_iterator(&it))
    {
        if (stmt->kind == FUNCTION_DECLARATION)
            assemble_function(a, stmt->function);
    }

    bc->main = get_unit_of_function(a, apm->main);

    // Call to main from the init unit
    emit_run(unit, 0, bc->main);

    // Patch all function calls
    for (size_t i = 0; i < a->data->call_patch_count; i++)
    {
        CallPatch patch = a->data->call_patch[i];
        patch_unit_ptr_payload(patch.unit, patch.instruction, get_unit_of_function(a, patch.funct));
    }
}

// ASSEMBLE //

void assemble(Compiler *compiler, Program *apm, ByteCode *byte_code)
{
    // Data used by all unit assemblers
    GlobalAssemblerData data;

    data.apm = apm;
    data.source_text = compiler->source_text;

    data.last_unit = NULL;

    data.call_patch_count = 0;
    data.function_unit_count = 0;
    data.enum_int_count = 0;
    data.type_data_count = 0;

    // Create init unit
    Assembler assembler;
    init_assembler_and_create_unit(&assembler, NULL, &data);

    byte_code->init = assembler.unit;
    assemble_program(&assembler, byte_code, apm);
}