#include "shun2cc.h"

IRInfo irinfo[] = {
    {'+', "+", IR_TY_REG_REG},
    {'-', "-", IR_TY_REG_REG},
    {'*', "*", IR_TY_REG_REG},
    {'/', "/", IR_TY_REG_REG},
    {IR_IMM, "MOV", IR_TY_REG_IMM},
    {IR_ADD_IMM, "ADD", IR_TY_REG_IMM},
    {IR_MOV, "MOV", IR_TY_REG_REG},
    {IR_LABEL, "", IR_TY_LABEL},
    {IR_UNLESS, "UNLESS", IR_TY_REG_LABEL},
    {IR_RETURN, "RET", IR_TY_REG},
    {IR_ALLOCA, "ALLOCA", IR_TY_REG_IMM},
    {IR_LOAD, "LOAD", IR_TY_REG_REG},
    {IR_STORE, "STORE", IR_TY_REG_REG},
    {IR_KILL, "KILL", IR_TY_REG},
    {IR_NOP, "NOP", IR_TY_NOARG},
    {0, NULL, 0},
};

static Vector *code;
static int regno;
static int basereg;

static Map *vars;
static int bpoff;

static int label;

IRInfo *get_irinfo(IR *ir) {
    for (IRInfo *info = irinfo; info->op; info++) {
        if (info->op == ir->op) {
            return info;
        }
    }

    assert(0 && "invalid instruction");
}


static char *tostr(IR *ir) {
    IRInfo *info = get_irinfo(ir);
    switch (info->type) {
        case IR_TY_LABEL:
            return format("%s:\n", ir->left);
        case IR_TY_REG:
            return format("%s r%d\n", info->name, ir->left);
        case IR_TY_REG_REG:
            return format("%s r%d, r%d\n", info->name, ir->left, ir->right);
        case IR_TY_REG_IMM:
            return format("%s r%d, %d\n", info->name, ir->left, ir->right);
        case IR_TY_REG_LABEL:
            return format("%s r%d, .L%s\n", info->name, ir->left, ir->right);
        default:
            assert(info->type == IR_TY_NOARG);
            return format("%s\n", info->name);

    }
}

void dump_ir(Vector *irv) {
    for (int i=0; i<irv->len; i++) {
        fprintf(stderr, "%s", tostr(irv->data[i]));
    }
}

static IR *add(int op, int left, int right)
{
    // 値を初期化しないとhas_immが非0になる可能性がある
    IR *ir = calloc(1, sizeof(IR));
    ir->op = op;
    ir->left = left;
    ir->right = right;
    vec_push(code, ir);
    return ir;
}


static int gen_lval(Node *node)
{
    if (node->type != ND_IDENT) {
        error("not an lvalue");
    }

    if (!map_exists(vars, node->name)) {
        map_put(vars, node->name, (void *)(intptr_t)bpoff);
        bpoff += 8;
    }

    int r = regno++;
    int off = (intptr_t)map_get(vars, node->name);
    add(IR_MOV, r, basereg);
    add(IR_ADD_IMM, r, off);
    return r;
}

static int gen_expr(Node *node)
{
    if (node->type == ND_NUM) {
        int r = regno++;
        add(IR_IMM, r, node->value);
        return r;
    }

    if (node->type == ND_IDENT) {
        int r = gen_lval(node);
        add(IR_LOAD, r, r);
        return r;
    }

    if (node->type == '=') {
        int right = gen_expr(node->right);
        int left = gen_lval(node->left);
        add(IR_STORE, left, right);
        add(IR_KILL, right, -1);
        return left;
    }

    assert(strchr("+-*/", node->type));

    int left = gen_expr(node->left);
    int right = gen_expr(node->right);

    add(node->type, left, right);
    add(IR_KILL, right, -1);
    return left;
}

static void gen_stmt(Node *node)
{
    if (node->type == ND_IF) {
        int r = gen_expr(node->cond);
        int x = label++;
        add(IR_UNLESS, r, x);
        add(IR_KILL, r, -1);
        gen_stmt(node->then);
        add(IR_LABEL, x, -1);
        return;
    }

    if (node->type == ND_RETURN) {
        int r = gen_expr(node->expr);
        add(IR_RETURN, r, -1);
        add(IR_KILL, r, -1);
        return;
    }

    if (node->type == ND_EXPR_STMT) {
        int r = gen_expr(node->expr);
        add(IR_KILL, r, -1);
        return;
    }

    if (node->type == ND_COMP_STMT) {
        for (int i=0; i<node->stmts->len; i++) {
            gen_stmt(node->stmts->data[i]);
        }

        return;
    }

    error("unknown node: %d", node->type);
}

Vector *gen_ir(Node *node)
{
    assert(node->type == ND_COMP_STMT);
    code = new_vec();
    regno = 1;
    basereg = 0;
    vars = new_map();
    bpoff = 0;
    label = 0;

    IR *alloca = add(IR_ALLOCA, basereg, -1);
    gen_stmt(node);
    alloca->right =bpoff;
    add(IR_KILL, basereg, -1);
    return code;
}
