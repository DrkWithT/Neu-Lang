#include "strs.h"
#include "ast.h"
#include "parser.h"



const lex_item neu_lexical_specials[] = {
    (lex_item) {.s = {.s = "def", .n = 3}, .tag = nl_tk_kw_def},
    (lex_item) {.s = {.s = "fun", .n = 3}, .tag = nl_tk_kw_fun},
    (lex_item) {.s = {.s = "cond", .n = 4}, .tag = nl_tk_kw_cond },
    (lex_item) {.s = {.s = "else", .n = 4}, .tag = nl_tk_kw_else },
    (lex_item) {.s = {.s = "loop", .n = 4}, .tag = nl_tk_kw_loop },
    (lex_item) {.s = {.s = "break", .n = 5}, .tag = nl_tk_kw_break },
    (lex_item) {.s = {.s = "continue", .n = 8}, .tag = nl_tk_kw_continue },
    (lex_item) {.s = {.s = "ret", .n = 3}, .tag = nl_tk_kw_ret },
    (lex_item) {.s = {.s = "nil", .n = 3}, .tag = nl_tk_kw_nil },
    (lex_item) {.s = {.s = "true", .n = 4}, .tag = nl_tk_kw_true },
    (lex_item) {.s = {.s = "false", .n = 5}, .tag = nl_tk_kw_false },
    (lex_item) {.s = {.s = "?", .n = 1}, .tag = nl_tk_op_nullish },
    (lex_item) {.s = {.s = "??", .n = 2}, .tag = nl_tk_op_coalesce },
    (lex_item) {.s = {.s = "%", .n = 1}, .tag = nl_tk_op_mod },
    (lex_item) {.s = {.s = "*", .n = 1}, .tag = nl_tk_op_mul },
    (lex_item) {.s = {.s = "/", .n = 1}, .tag = nl_tk_op_div },
    (lex_item) {.s = {.s = "+", .n = 1}, .tag = nl_tk_op_add },
    (lex_item) {.s = {.s = "-", .n = 1}, .tag = nl_tk_op_sub },
    (lex_item) {.s = {.s = "==", .n = 2}, .tag = nl_tk_op_equals },
    (lex_item) {.s = {.s = "!=", .n = 2}, .tag = nl_tk_op_bang_equals },
    (lex_item) {.s = {.s = "<", .n = 1}, .tag = nl_tk_op_lt },
    (lex_item) {.s = {.s = ">", .n = 1}, .tag = nl_tk_op_gt },
    (lex_item) {.s = {.s = "<=", .n = 2}, .tag = nl_tk_op_lte },
    (lex_item) {.s = {.s = ">=", .n = 2}, .tag = nl_tk_op_gte },
    (lex_item) {.s = {.s = "&&", .n = 2}, .tag = nl_tk_op_and },
    (lex_item) {.s = {.s = "||", .n = 2}, .tag = nl_tk_op_or },
    (lex_item) {.s = {.s = "=", .n = 1}, .tag = nl_tk_op_bind },
    (lex_item) {.s = {.s = "->", .n = 2}, .tag = nl_tk_arrow },
    (lex_item) {.s = {.s = nullptr, .n = 0}, .tag = nl_tk_unknown }
};



[[nodiscard]]
static string read_file(const char* file_path) {
    constexpr size_t chomp_size = 64;

    FILE* reader = fopen(file_path, "r");
    auto s = string_dud();

    if (!reader) {
        return s;
    }

    char chomp_buffer[chomp_size + 1] = {0};
    size_t temp_rc;

    while (!feof(reader)) {
        temp_rc = fread(chomp_buffer, sizeof(char), chomp_size, reader);

        if (temp_rc == 0) {
            break;
        }

        string_append_raw(&s, chomp_buffer, temp_rc);
    }

    return s;
}

int main(int argc, const char* argv[]) {
    auto show_help = false, dump_bc = false, run_bc = false, invalid_args = false;
    const char* file_path = nullptr;

    for (int arg_offset = 0; arg_offset < argc - 1; arg_offset++) {
        auto arg_view = str_from_cstr(argv[1 + arg_offset]);

        if (str_eq_raw(&arg_view, "-h", 2)) {
            show_help = true;
        } else if (str_eq_raw(&arg_view, "-d", 2)) {
            dump_bc = true;
        } else if (str_eq_raw(&arg_view, "-r", 2)) {
            run_bc = true;
        } else if (!file_path && (dump_bc || run_bc)) {
            file_path = arg_view.s;
            break;
        } else {
            invalid_args = true;
            break;
        }
    }

    if (show_help) {
        puts("Neu Lang v0.0.1\nBy: DerkPlusT\nOptions:\n\t-h: show usage information.\n\t-d: dump bytecode or debug logs.\n\t-r: run a Neu file.\n");
        return 0;
    } else if (invalid_args || !file_path) {
        puts("Neu Lang v0.0.1\nBy: DerkPlusT\nOptions:\n\t-h: show usage information.\n\t-d: dump bytecode or debug logs.\n\t-r: run a Neu file.\n");
        puts("\x1b[1;33mNOTE:\x1b[0m Did you pass a valid .neu file?");
        return 1;
    }

    auto loaded_src = read_file(file_path);

    if (string_empty(&loaded_src)) {
        puts("\x1b[1;33mNOTE:\x1b[0m Did you pass a non-empty .neu file?");
        string_del(&loaded_src);

        return 1;
    }

    string_output(stdout, &loaded_src);

    const auto target_source = str_from_string(&loaded_src);
    auto ast = nl_parse_source(neu_lexical_specials, &target_source);

    const bool ast_ok = !str_empty(&ast.source);

    ast_unit_del(&ast);
    string_del(&loaded_src);

    return ast_ok ? 0 : 1;
}