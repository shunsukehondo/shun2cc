#include "shun2cc.h"

char *regs[] = {"rdi", "rsi", "r10", "r11", "r12", "r13", "r14", "r15", NULL};
static bool used[sizeof(regs) / sizeof(*regs)];
static int *reg_map;

static int alloc(int ir_reg)
{
    if (reg_map[ir_reg] != -1) {
        int r = reg_map[ir_reg];
        assert(used[r]);
        return r;
    }

    for (int i=0; i<sizeof(regs) / sizeof(*regs); i++) {
        if (used[i]) {
            continue;
        }
        used[i] = true;
        reg_map[ir_reg] = i;
        return i;
    }

    error("Register exhausted.");
}

static void kill(int r) {
    assert(used[r]);
    used[r] = false;
}

void alloc_regs(Vector *irv)
{
    reg_map = malloc(sizeof(int) * irv->len);
    for (int i=0; i<irv->len; i++) {
        reg_map[i] = -1;
    }

    for (int i=0; i<irv->len; i++) {
        IR *ir = irv->data[i];
        IRInfo *info = get_irinfo(ir);

        switch (info->type) {
            case IR_TY_REG:
            case IR_TY_REG_IMM:
            case IR_TY_REG_LABEL:
                ir->left = alloc(ir->left);
                break;
            case IR_TY_REG_REG:
                ir->left = alloc(ir->left);
                ir->right = alloc(ir->right);
                break;
            case IR_TY_CALL:
                ir->left = alloc(ir->left);
                for (int i = 0; i < ir->nargs; i++) {
                    ir->args[i] = alloc(ir->args[i]);
                }
                break;
        }

        if (ir->op == IR_KILL) {
            kill(ir->left);
            ir->op = IR_NOP;
        }
    }
}