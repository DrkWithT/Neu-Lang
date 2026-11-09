#include "ast.h"



void nl_ast_invalid_del([[maybe_unused]] void* self);
void nl_ast_literal_del([[maybe_unused]] void* self);
void nl_ast_literal_array_del(void* self);
void nl_ast_table_prop_del(void* self);
void nl_ast_literal_table_del(void* self);
void nl_ast_expr_func_del(void* self);
void nl_ast_part_access_del(void* self);
void nl_ast_expr_access_del(void* self);
void nl_ast_expr_call_del(void* self);
void nl_ast_unary_del(void* self);
void nl_ast_binary_normal_del(void* self);
void nl_ast_binary_logical_del(void* self);
void nl_ast_expr_assign_del(void* self);
void nl_ast_cond_entry_del(void* self);
void nl_ast_cond_del(void* self);
void nl_ast_loop_del(void* self);
void nl_ast_loop_abator_del(void* self);
void nl_ast_def_binding_del(void* self);
void nl_ast_stmt_def_del(void* self);
void nl_ast_block_del(void* self);
void nl_ast_stmt_ret_del(void* self);
void nl_ast_expr_stmt_del(void* self);

void ast_node_del(void* node);

[[nodiscard]]
literal nl_ast_literal_new(tk token) {
    return (literal) {
        .token = token
    };
}



const nl_ast_node_deleter_t ast_deleters[] = {
    nl_ast_invalid_del,
    nl_ast_literal_del,
    nl_ast_literal_array_del,
    nl_ast_literal_table_del,
    nl_ast_expr_func_del,
    nl_ast_expr_access_del,
    nl_ast_expr_call_del,
    nl_ast_unary_del,
    nl_ast_binary_normal_del,
    nl_ast_binary_logical_del,
    nl_ast_expr_assign_del,
    nl_ast_cond_del,
    nl_ast_loop_del,
    nl_ast_loop_abator_del,
    nl_ast_stmt_def_del,
    nl_ast_block_del,
    nl_ast_stmt_ret_del,
    nl_ast_expr_stmt_del
};

void nl_ast_invalid_del([[maybe_unused]] void* self) {}

void nl_ast_literal_del([[maybe_unused]] void* self) {}

void nl_ast_literal_array_del(void* self) {
    auto array_self = (array*)self;

    dynbuf_del(&array_self->items);
}

void nl_ast_table_prop_del(void* self) {
    auto table_prop_self = (table_prop*)self;

    ast_node_del(table_prop_self->key);
    ast_node_del(table_prop_self->expr);
}

void nl_ast_literal_table_del(void* self) {
    auto table_self = (table*)self;

    dynbuf_del(&table_self->props);
}

void nl_ast_expr_func_del(void* self) {
    auto func_self = (expr_func*)self;

    dynbuf_del(&func_self->param_tkv);
    ast_node_del(func_self->body);
}

void nl_ast_part_access_del(void* self) {
    auto part_access_self = (part_access*)self;

    if (!part_access_self->is_named) {
        ast_node_del(part_access_self->data.comp_key);
    }
}

void nl_ast_expr_access_del(void* self) {
    auto access_self = (expr_access*)self;

    dynbuf_del(&access_self->accesses);
    ast_node_del(access_self->target);
}

void nl_ast_expr_call_del(void* self) {
    auto call_self = (expr_call*)self;

    dynbuf_del(&call_self->arg_exprs);
    ast_node_del(call_self->callee);
}

void nl_ast_unary_del(void* self) {
    auto unary_self = (expr_unary*)self;

    ast_node_del(unary_self->inner);
}

void nl_ast_binary_normal_del(void* self) {
    auto binary_self = (expr_binary_normal*)self;

    ast_node_del(binary_self->lhs);
    ast_node_del(binary_self->rhs);
}

void nl_ast_binary_logical_del(void* self) {
    auto binary_self = (expr_binary_logical*)self;

    ast_node_del(binary_self->lhs);
    ast_node_del(binary_self->rhs);
}

void nl_ast_expr_assign_del(void* self) {
    auto assign_self = (expr_assign*)self;

    ast_node_del(assign_self->dest);
    ast_node_del(assign_self->src);
}

void nl_ast_cond_entry_del(void* self) {
    auto cond_entry_self = (cond_entry*)self;

    ast_node_del(cond_entry_self->check);
    ast_node_del(cond_entry_self->body);
}

void nl_ast_cond_del(void* self) {
    auto cond_self = (cond*)self;

    dynbuf_del(&cond_self->entries);
}

void nl_ast_loop_del(void* self) {
    auto loop_self = (loop*)self;

    ast_node_del(loop_self->init_expr);
    ast_node_del(loop_self->check_expr);
    ast_node_del(loop_self->update_expr);
    ast_node_del(loop_self->body);
    ast_node_del(loop_self->result_expr);
}

void nl_ast_loop_abator_del(void* self) {
    auto loop_abator_self = (loop_abator*)self;

    ast_node_del(loop_abator_self->inner);
}

void nl_ast_def_binding_del(void* self) {
    auto binding_self = (def_binding*)self;

    ast_node_del(binding_self->init);
}

void nl_ast_stmt_def_del(void* self) {
    auto def_self = (stmt_def*)self;

    dynbuf_del(&def_self->bindings);
}

void nl_ast_block_del(void* self) {
    auto block_self = (block*)self;

    dynbuf_del(&block_self->items);
}

void nl_ast_stmt_ret_del(void* self) {
    auto stmt_ret_self = (stmt_ret*)self;

    ast_node_del(stmt_ret_self->result);
}

void nl_ast_expr_stmt_del(void* self) {
    auto expr_stmt_self = (expr_stmt*)self;

    ast_node_del(expr_stmt_self->expr);
}

void ast_node_del(void* node) {
    if (!node) return;

    auto target_node = (ast_node*)node;

    ast_deleters[(size_t)target_node->tag](target_node);
}

// ! NOTE: use `ast_node_del(ast_node* node)` as `deleter_fn` within the AST roots' `dynbuf`.
void ast_unit_del(void* self) {
    auto ast_node_self = (ast_unit*)self;

    dynbuf_del(&ast_node_self->roots);
}
