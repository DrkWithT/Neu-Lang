#include <stdio.h>
#include "bcc.h"
#include "var_scope.h"


static constexpr size_t bcc_initial_chunk_cap_v = 24;
static constexpr size_t bcc_initial_chunk_k_cap_v = 8;
static constexpr size_t bcc_initial_chunk_ic_cap_v = 2;
static constexpr size_t bcc_initial_chunk_var_ent_cap_v = 2;
static constexpr size_t bcc_bc_byte_sz = sizeof(uint8_t);
static constexpr uint16_t bcc_bc_max_cap_v = 0xffff;

[[nodiscard]]
bool bcc_flag_check_n(uint16_t flags, const uint16_t* checks, uint8_t n) {
    for (uint8_t chk_i = 0; chk_i < n; chk_i++) {
        if ((flags & checks[chk_i]) != 0) {
            return true;
        }
    }

    return false;
}

[[nodiscard]]
uint16_t bcc_flag_set_n(uint16_t flags, const uint16_t* sets, uint8_t n) {
    for (uint8_t i = 0; i < n; i++) {
        flags |= sets[i];
    }

    return flags;
}

[[nodiscard]]
uint16_t bcc_flag_unset_n(uint16_t flags, const uint16_t* omits, uint8_t n) {
    for (uint8_t i = 0; i < n; i++) {
        flags &= ~omits[i];
    }

    return flags;
}


static const auto bcc_dud_result_v = (bcc_result) {.n = 0, .flags = 0x00, .domain = nl_bcc_sym_invalid};


typedef bcc_result(*bcc_emitter_t)(bcc_state* bcc, const ast_node* node, uint16_t flags);

[[nodiscard]]
bcc_result bcc_emit_invalid(bcc_state* bcc, const ast_node* node, uint16_t flags);
[[nodiscard]]
bcc_result bcc_emit_literal(bcc_state* bcc, const ast_node* node, uint16_t flags);
[[nodiscard]]
bcc_result bcc_emit_literal_array(bcc_state* bcc, const ast_node* node, uint16_t flags);
[[nodiscard]]
bcc_result bcc_emit_literal_table(bcc_state* bcc, const ast_node* node, uint16_t flags);
[[nodiscard]]
bcc_result bcc_emit_expr_func(bcc_state* bcc, const ast_node* node, uint16_t flags);
[[nodiscard]]
bcc_result bcc_emit_expr_access(bcc_state* bcc, const ast_node* node, uint16_t flags);
[[nodiscard]]
bcc_result bcc_emit_expr_call(bcc_state* bcc, const ast_node* node, uint16_t flags);
[[nodiscard]]
bcc_result bcc_emit_unary(bcc_state* bcc, const ast_node* node, uint16_t flags);
[[nodiscard]]
bcc_result bcc_emit_binary_normal(bcc_state* bcc, const ast_node* node, uint16_t flags);
[[nodiscard]]
bcc_result bcc_emit_binary_logical(bcc_state* bcc, const ast_node* node, uint16_t flags);
[[nodiscard]]
bcc_result bcc_emit_expr_assign(bcc_state* bcc, const ast_node* node, uint16_t flags);
[[nodiscard]]
bcc_result bcc_emit_cond(bcc_state* bcc, const ast_node* node, uint16_t flags);
[[nodiscard]]
bcc_result bcc_emit_loop(bcc_state* bcc, const ast_node* node, uint16_t flags);
[[nodiscard]]
bcc_result bcc_emit_loop_abator(bcc_state* bcc, const ast_node* node, uint16_t flags);
[[nodiscard]]
bcc_result bcc_emit_stmt_def(bcc_state* bcc, const ast_node* node, uint16_t flags);
[[nodiscard]]
bcc_result bcc_emit_block(bcc_state* bcc, const ast_node* node, uint16_t flags);
[[nodiscard]]
bcc_result bcc_emit_stmt_ret(bcc_state* bcc, const ast_node* node, uint16_t flags);
[[nodiscard]]
bcc_result bcc_emit_expr_stmt(bcc_state* bcc, const ast_node* node, uint16_t flags);

const bcc_emitter_t bcc_emitters[] = {
    bcc_emit_invalid,
    bcc_emit_literal,
    bcc_emit_literal_array,
    bcc_emit_literal_table,
    bcc_emit_expr_func,
    bcc_emit_expr_access,
    bcc_emit_expr_call,
    bcc_emit_unary,
    bcc_emit_binary_normal,
    bcc_emit_binary_logical,
    bcc_emit_expr_assign,
    bcc_emit_cond,
    bcc_emit_loop,
    bcc_emit_loop_abator,
    bcc_emit_stmt_def,
    bcc_emit_block,
    bcc_emit_stmt_ret,
    bcc_emit_expr_stmt,
};

void symscope_del(void* ssc) {
    auto symscope_self = (symscope*)ssc;

    dynbuf_del(&symscope_self->infos);
}



// ! IMPORTANT: This returns the parent chunk ID before entering this new child chunk.
[[nodiscard]]
uint16_t bcc_start_chunk(bcc_state* bcc) {
    const uint16_t next_cid = bcc->chunks.n;

    chunk next_chunk = {
        .vinfo = dynbuf_new(sizeof(var_ent), bcc_initial_chunk_var_ent_cap_v, nullptr),
        .bc = calloc(bcc_initial_chunk_cap_v, bcc_bc_byte_sz),
        .code_cap = bcc_initial_chunk_cap_v,
        .code_len = 0,
        .kd = calloc(bcc_initial_chunk_k_cap_v, sizeof(value)),
        .k_cap = bcc_initial_chunk_k_cap_v,
        .k_len = 0,
        .icv = calloc(bcc_initial_chunk_ic_cap_v, sizeof(ic)),
        .ic_cap = bcc_initial_chunk_ic_cap_v,
        .ic_len = 0,
        .regs_n = 0,
        .arity = 0,
    };

    dynbuf_push(&bcc->chunks, &next_chunk);

    return next_cid;
}

static inline void bcc_resume_chunk(bcc_state* bcc, uint16_t cid) {
    bcc->curr_cid = cid;
}

[[nodiscard]]
static inline uint16_t nl_bcc_next_ip(const bcc_state* bcc) {
    const auto curr_chunk = (const chunk*) dynbuf_get(&bcc->chunks, bcc->curr_cid);

    return curr_chunk->code_len;
}

void nl_bcc_emit_byte(bcc_state* bcc, uint8_t b) {
    auto curr_chunk = (chunk*) dynbuf_get_mut(&bcc->chunks, bcc->curr_cid);
    const uint16_t next_len = curr_chunk->code_len;
    const uint16_t curr_cap = curr_chunk->code_cap;
    const uint16_t next_cap = curr_chunk->code_cap * 2;

    if (next_len >= bcc_bc_max_cap_v) {
        NL_PANIC("Exhausted u16-max capacity of chunk in bcc here.", __FILE__, __LINE__)
    }

    if (next_len >= curr_cap) {
        auto next_bc_buf = realloc(curr_chunk->bc, bcc_bc_byte_sz * next_cap);

        if (!next_bc_buf) {
            NL_PANIC("Failed to reallocate bytecode buffer in bcc here.", __FILE__, __LINE__)
        }

        curr_chunk->bc = next_bc_buf;
        curr_chunk->code_cap = next_cap;
    }

    curr_chunk->bc[next_len] = b;
    curr_chunk->code_len++;
}

[[nodiscard]]
uint16_t nl_bcc_emit_inst_unary(bcc_state* bcc, op o, uint16_t a0) {
    const uint16_t bc_ipos = ((const chunk*)dynbuf_get(&bcc->chunks, bcc->curr_cid))->code_len;

    nl_bcc_emit_byte(bcc, (uint8_t)o);
    nl_bcc_emit_byte(bcc, (uint8_t)(a0 & 0xff));
    nl_bcc_emit_byte(bcc, (uint8_t)((a0 & 0xff00) >> 8));

    return bc_ipos;
}

[[nodiscard]]
uint16_t nl_bcc_emit_inst_binary(bcc_state* bcc, op o, uint16_t a0, uint16_t a1) {
    const uint16_t bc_ipos = ((const chunk*)dynbuf_get(&bcc->chunks, bcc->curr_cid))->code_len;

    nl_bcc_emit_byte(bcc, (uint8_t)o);
    nl_bcc_emit_byte(bcc, (uint8_t)(a0 & 0xff));
    nl_bcc_emit_byte(bcc, (uint8_t)((a0 & 0xff00) >> 8));
    nl_bcc_emit_byte(bcc, (uint8_t)(a1 & 0xff));
    nl_bcc_emit_byte(bcc, (uint8_t)((a1 & 0xff00) >> 8));

    return bc_ipos;
}

[[nodiscard]]
uint16_t nl_bcc_emit_inst_ternary(bcc_state* bcc, op o, uint16_t a0, uint16_t a1, uint16_t a2) {
    const uint16_t bc_ipos = ((const chunk*)dynbuf_get(&bcc->chunks, bcc->curr_cid))->code_len;

    nl_bcc_emit_byte(bcc, (uint8_t)o);
    nl_bcc_emit_byte(bcc, (uint8_t)(a0 & 0xff));
    nl_bcc_emit_byte(bcc, (uint8_t)((a0 & 0xff00) >> 8));
    nl_bcc_emit_byte(bcc, (uint8_t)(a1 & 0xff));
    nl_bcc_emit_byte(bcc, (uint8_t)((a1 & 0xff00) >> 8));
    nl_bcc_emit_byte(bcc, (uint8_t)(a2 & 0xff));
    nl_bcc_emit_byte(bcc, (uint8_t)((a2 & 0xff00) >> 8));

    return bc_ipos;
}

void nl_patch_cond_jump(bcc_state* bcc, uint16_t target_ip, uint16_t dest_ipos) {
    auto curr_chunk = (chunk*) dynbuf_get_mut(&bcc->chunks, bcc->curr_cid);
    const op checked_opcode = curr_chunk->bc[target_ip];

    if (checked_opcode == nl_op_jmp_if || checked_opcode == nl_op_jmp_else) {
        // ? These offsets encode the u16 in operand `a1`: the absolute IP
        curr_chunk->bc[target_ip + 3] = (dest_ipos & 0xff);
        curr_chunk->bc[target_ip + 4] = (dest_ipos & 0xff00) >> 8;
    }
}

void nl_patch_jump(bcc_state* bcc, uint16_t target_ip, uint16_t dest_ipos) {
    auto curr_chunk = (chunk*) dynbuf_get_mut(&bcc->chunks, bcc->curr_cid);
    const op checked_opcode = curr_chunk->bc[target_ip];

    if (checked_opcode == nl_op_jmp) {
        // ? These offsets encode the u16 in operand `a0`: the absolute IP
        curr_chunk->bc[target_ip + 1] = (dest_ipos & 0xff);
        curr_chunk->bc[target_ip + 2] = (dest_ipos & 0xff00) >> 8;
    }
}

void nl_bcc_place_ic(bcc_state* bcc, uint16_t ip, ic temp_ic) {
    auto curr_chunk = (chunk*) dynbuf_get_mut(&bcc->chunks, bcc->curr_cid);
    const uint16_t next_ic_len = curr_chunk->ic_len;
    const uint16_t curr_ic_cap = curr_chunk->ic_cap;
    const uint16_t next_ic_cap = curr_chunk->ic_cap * 2;

    if (next_ic_len >= bcc_bc_max_cap_v) {
        NL_PANIC("Exhausted u16-max capacity of chunk in bcc here.", __FILE__, __LINE__)
    }

    if (next_ic_len >= curr_ic_cap) {
        auto next_ic_buf = realloc(curr_chunk->icv, sizeof(ic) * next_ic_cap);

        if (!next_ic_buf) {
            NL_PANIC("Failed to reallocate bytecode buffer in bcc here.", __FILE__, __LINE__)
        }

        curr_chunk->icv = next_ic_buf;
        curr_chunk->code_cap = next_ic_cap;
    }

    curr_chunk->icv[next_ic_len] = temp_ic;
    curr_chunk->ic_len++;
}

void nl_bcc_start_scope(bcc_state* bcc, str scope_name) {
    symscope next_scope = {
        .infos = dynbuf_new(sizeof(syminfo), 8, nullptr),
        .name = scope_name,
        .top_konst = 0,
        .next_ic = 0,
        .cid = bcc->curr_cid
    };

    dynbuf_push(&bcc->scopes, &next_scope);
}

void nl_bcc_end_scope(bcc_state* bcc) {
    dynbuf_pop(&bcc->scopes);
}

static inline void nl_bcc_report_err(bcc_state* bcc, const char* msg, uint16_t line, uint16_t col) {
    bcc->errors++;

    fprintf(
        stdout,
        "\x1b[1;31mCompile Error [%d]\x1b[0m ~ ln \x1b[1;36m%d\x1b[0m, col \x1b[1;36m%d\x1b[0m:\n\t%s\n",
        bcc->errors,
        line,
        col,
        msg
    );
}

// ! For symbol scope lookup mode: `G` is global-only, `L` is local-only, and `B` is bubbling.
[[nodiscard]]
const syminfo* nl_bcc_lookup_symbol(bcc_state* bcc, str s, symtag domain, char mode) {
    int32_t scope_depth = (mode == 'G') ? 0 : bcc->scopes.n - 1;
    const auto should_bubble_lookup = mode == 'B';

    for (; scope_depth >= 0; scope_depth--) {
        const auto curr_scope = (const symscope*)dynbuf_get(&bcc->scopes, scope_depth);
        const auto scope_entries = (const syminfo*)dynbuf_data(&curr_scope->infos);
        const int32_t scope_entry_n = dynbuf_size(&curr_scope->infos);

        for (int32_t entry_pos = 0; entry_pos < scope_entry_n; entry_pos++) {
            const auto curr_entry = scope_entries + entry_pos;

            if (str_eq_str(&curr_entry->name, &s) && curr_entry->tag == domain) {
                return curr_entry;
            }
        }

        if (!should_bubble_lookup) {
            break;
        }
    }

    return nullptr;
}

[[nodiscard]]
const syminfo* nl_bcc_push_symbol_to(bcc_state* bcc, syminfo info, char should_use_global) {
    auto target_scope = (symscope*) dynbuf_get_mut(&bcc->scopes, (should_use_global == 'G') ? 0 : bcc->scopes.n - 1 );

    return (const syminfo*) dynbuf_push(&target_scope->infos, &info);
}

[[nodiscard]]
uint16_t bcc_push_konst(bcc_state* bcc, value v) {
    auto curr_chunk = (chunk*) dynbuf_get_mut(&bcc->chunks, bcc->curr_cid);

    const auto next_k_id = curr_chunk->k_len;
    const auto curr_k_cap = curr_chunk->k_cap;
    const auto new_k_cap = curr_chunk->k_cap * 2;

    if (next_k_id >= curr_k_cap) {
        auto new_kd_buf = realloc(curr_chunk->kd, sizeof(value) * new_k_cap);

        if (!new_kd_buf) {
            NL_PANIC("Failed to grow constants buffer of chunk here.", __FILE__, __LINE__)
        }

        curr_chunk->kd = new_kd_buf;
        curr_chunk->k_cap = new_k_cap;
    }

    curr_chunk->kd[next_k_id] = v;
    curr_chunk->k_len++;

    return next_k_id;
}

[[nodiscard]]
const syminfo* nl_bcc_record_string_k(bcc_state* bcc, str s) {
    const auto pre_found_info = nl_bcc_lookup_symbol(bcc, s, nl_bcc_sym_k, 'G');
    if (pre_found_info) {
        return pre_found_info;
    }

    // ? All string literals are global, so use the bottom-most, global scope context to track each.
    auto curr_scope = (symscope*) dynbuf_get_mut(&bcc->scopes, 0);
    auto s_owned = string_from_str(&s);
    auto interned_sd = ent_pool_add(&bcc->pool_s, &s_owned);

    return nl_bcc_push_symbol_to(bcc, (syminfo) {
        .name = s,
        .n = bcc_push_konst(bcc, (value) {
            .n = interned_sd.id,
            .tag = nl_vtag_str
        }),
        .cid = 0,
        .tag = nl_bcc_sym_k
    }, 'G');
}

[[nodiscard]]
const syminfo* nl_bcc_record_global(bcc_state* bcc, str s, value v) {
    const auto pre_found_info = nl_bcc_lookup_symbol(bcc, s, nl_bcc_sym_global_k, 'G');
    if (pre_found_info) {
        return pre_found_info;
    }

    const auto next_global_v_id = bcc->gvals_n;
    const auto old_global_cap = bcc->gvals_cap;
    const auto new_global_cap = bcc->gvals_cap * 2;

    if (next_global_v_id >= old_global_cap) {
        auto temp_gval_buf = realloc(bcc->gvals, sizeof(value) * new_global_cap);

        if (!temp_gval_buf) {
            NL_PANIC("Failed to grow superglobal buffer in BCC here.", __FILE__, __LINE__)
        }

        bcc->gvals = temp_gval_buf;
        bcc->gvals_cap = new_global_cap;
    }

    bcc->gvals[next_global_v_id] = v;
    bcc->gvals_n++;

    return nl_bcc_push_symbol_to(bcc, (syminfo) {
        .name = s,
        .n = next_global_v_id,
        .cid = 0,
        .tag = nl_bcc_sym_global_k
    }, 'G');
}

[[nodiscard]]
const syminfo* nl_bcc_record_local(bcc_state* bcc, str name) {
    const auto pre_found_info = nl_bcc_lookup_symbol(bcc, name, nl_bcc_sym_local_r, 'L');
    if (pre_found_info) {
        return pre_found_info;
    }

    auto curr_scope = (symscope*) dynbuf_last_mut(&bcc->scopes);

    const auto top_reg_for_local = bcc->top_reg;
    bcc->top_reg++;

    return nl_bcc_push_symbol_to(bcc, (syminfo) {
        .name = name,
        .n = top_reg_for_local,
        .cid = bcc->curr_cid,
        .tag = nl_bcc_sym_local_r
    }, 'G');
}

// ! IMPORTANT: upvalues are pre-recorded per chunk in vinfos as a var_ent for upvalue having `cid = assoc-scope -> assoc-function -> func-chunk-ID` and local-reg "offset"
[[nodiscard]]
const var_ent* nl_bcc_push_chunk_var_ent(bcc_state* bcc, var_ent entry) {
    auto curr_chunk = (chunk*) dynbuf_get_mut(&bcc->chunks, bcc->curr_cid);

    return (const var_ent*) dynbuf_push(&curr_chunk->vinfo, &entry);
}


[[nodiscard]]
bcc_result bcc_emit_invalid(bcc_state* bcc, const ast_node* node, uint16_t flags) {
    return (bcc_result) {
        .n = 0,
        .flags = bcc_flag_unset_n(flags, (uint16_t[]) { nl_bcc_cf_ok }, 1),
        .domain = nl_bcc_sym_invalid,
    };
}

[[nodiscard]]
bcc_result bcc_preemit_identifier(bcc_state* bcc, const str* identifier, uint16_t flags) {
    const syminfo* found_named_info = nullptr;
    const auto ident_str = *identifier;

    if ((found_named_info = nl_bcc_lookup_symbol(bcc, ident_str, nl_bcc_sym_global_k, 'G')) != nullptr) {
        ; // ...
    } else if ((found_named_info = nl_bcc_lookup_symbol(bcc, ident_str, nl_bcc_sym_k, 'L'))) {
        ; // ...
    } else if ((found_named_info = nl_bcc_lookup_symbol(bcc, ident_str, nl_bcc_sym_local_r, 'L'))) {
        ; // ...
    } else if ((found_named_info = nl_bcc_lookup_symbol(bcc, ident_str, nl_bcc_sym_local_r, 'B'))) {
        ; // ...
    }

    if (!found_named_info) {
        nl_bcc_report_err(bcc, "Failed to resolve this name...", bcc->line, bcc->col);
        fprintf(stdout, "\tnote: was the name below defined in the function / outer scopes? '\x1b[1;31m");
        str_output(stdout, identifier);
        fprintf(stdout, "\x1b[0m'\n\n");
        return bcc_dud_result_v;
    }

    return (bcc_result) {
        .n = found_named_info->n,
        .flags = flags,
        .domain = found_named_info->tag
    };
}

[[nodiscard]]
bcc_result bcc_emit_literal(bcc_state* bcc, const ast_node* node, uint16_t flags);

[[nodiscard]]
bcc_result bcc_emit_literal_array(bcc_state* bcc, const ast_node* node, uint16_t flags);

[[nodiscard]]
bcc_result bcc_emit_literal_table(bcc_state* bcc, const ast_node* node, uint16_t flags);

[[nodiscard]]
bcc_result bcc_emit_expr_func(bcc_state* bcc, const ast_node* node, uint16_t flags);

[[nodiscard]]
bcc_result bcc_emit_expr_access(bcc_state* bcc, const ast_node* node, uint16_t flags);

[[nodiscard]]
bcc_result bcc_emit_expr_call(bcc_state* bcc, const ast_node* node, uint16_t flags);

[[nodiscard]]
bcc_result bcc_emit_unary(bcc_state* bcc, const ast_node* node, uint16_t flags);

[[nodiscard]]
bcc_result bcc_emit_binary_normal(bcc_state* bcc, const ast_node* node, uint16_t flags);

[[nodiscard]]
bcc_result bcc_emit_binary_logical(bcc_state* bcc, const ast_node* node, uint16_t flags);

[[nodiscard]]
bcc_result bcc_emit_expr_assign(bcc_state* bcc, const ast_node* node, uint16_t flags);

[[nodiscard]]
bcc_result bcc_emit_cond(bcc_state* bcc, const ast_node* node, uint16_t flags);

[[nodiscard]]
bcc_result bcc_emit_loop(bcc_state* bcc, const ast_node* node, uint16_t flags);

[[nodiscard]]
bcc_result bcc_emit_loop_abator(bcc_state* bcc, const ast_node* node, uint16_t flags);

[[nodiscard]]
bcc_result bcc_emit_stmt_def(bcc_state* bcc, const ast_node* node, uint16_t flags);

[[nodiscard]]
bcc_result bcc_emit_block(bcc_state* bcc, const ast_node* node, uint16_t flags);

[[nodiscard]]
bcc_result bcc_emit_stmt_ret(bcc_state* bcc, const ast_node* node, uint16_t flags);

[[nodiscard]]
bcc_result bcc_emit_expr_stmt(bcc_state* bcc, const ast_node* node, uint16_t flags);

[[nodiscard]]
bcc_result bcc_emit_next(bcc_state* bcc, const ast_node* node, uint16_t flags);



[[nodiscard]]
bcc_state nl_bcc_state_new(int32_t max_strings, int32_t max_objects, int32_t max_closures);

void nl_bcc_state_del(bcc_state* bcc);

[[nodiscard]]
program nl_bcc_compile_tu(const ast_unit* tu);
