#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// +-<>.,[]
#define OP_CELL_INC     0
#define OP_CELL_DEC     1
#define OP_PTR_LEFT     2
#define OP_PTR_RIGHT    3
#define OP_GETCH        4
#define OP_PUTCH        5
#define OP_JMP_FWD      6
#define OP_JMP_BCK      7
#define OP_EOF          8

#define PC_LENGTH     30000
#define STACK_LENGTH  1024

#define ERROR(fmt, ...)         \
    do {                        \
        fprintf(stderr,         \
                "\x1b[1;30;41m" \
                " ERROR "       \
                "\x1b[0;31m"    \
                " " fmt         \
                "\x1b[0m\n",    \
                ##__VA_ARGS__); \
    } while(0);

struct program {
    unsigned char pc[PC_LENGTH];
    unsigned char stack[STACK_LENGTH];
    ssize_t       jmp_table[PC_LENGTH];

    ssize_t pc_cursor;
    ssize_t stack_cursor;
};

unsigned char *parse_ins(struct program *p, FILE *fp);
int interpret(struct program *p, unsigned char *ins);

static const char help[] =
    "Tiny Brainfuck Interpreter\n"
    "Usage: tinyb infile(s)...\n"
    ;

int main(int argc, char **argv)
{
    if(argc != 2) {
        fputs(help, stderr);
        return 1;
    }

    if(!(strcmp(argv[1], "--help")) || !(strcmp(argv[1], "-h"))) {
        fputs(help, stdout);
        return 0;
    }

    struct program *p = malloc(sizeof(struct program));
    memset(p->pc,         0, PC_LENGTH * sizeof(unsigned char));
    memset(p->jmp_table, -1, PC_LENGTH * sizeof(ssize_t));
    memset(p->stack,      0, STACK_LENGTH * sizeof(unsigned char));

    p->pc_cursor    = 0;
    p->stack_cursor = 0;

    FILE *fp;

    if((fp = fopen(argv[1], "r")) == NULL) {
        ERROR("unable to open file '%s'", argv[1]);
        return 1;
    }
    
    unsigned char *ins = parse_ins(p, fp);
    int ip = interpret(p, ins);

    fclose(fp);
    free(ins);
    free(p);
     
    return ip;
}

unsigned char *parse_ins(struct program *p, FILE *fp)
{
    unsigned int  ins_cursor = 0,
                  ins_len    = 8;
    unsigned char *ins       = malloc(ins_len);
    char ch;

    while((ch = getc(fp)) != EOF) {
        if(ins_cursor == ins_len) {
            ins_len *= 2;
            ins = realloc(ins, ins_len);
        }

        switch(ch) {
            case '+':
                ins[ins_cursor++] = OP_CELL_INC;
                break;
            case '-':
                ins[ins_cursor++] = OP_CELL_DEC;
                break;
            case '<':
                ins[ins_cursor++] = OP_PTR_LEFT;
                break;
            case '>':
                ins[ins_cursor++] = OP_PTR_RIGHT;
                break;
            case ',':
                ins[ins_cursor++] = OP_GETCH;
                break;
            case '.':
                ins[ins_cursor++] = OP_PUTCH;
                break;
            case '[':
                ins[ins_cursor++] = OP_JMP_FWD;
                p->stack[p->stack_cursor++] = ins_cursor;
                break;
            case ']':
                ins[ins_cursor++] = OP_JMP_BCK;
                p->jmp_table[ins_cursor] = p->stack[--p->stack_cursor];
                p->jmp_table[p->jmp_table[ins_cursor]] = ins_cursor;
                break;
            default:
                // ignore
                break;
        }
    }

    if(p->stack_cursor > 0) {
        ERROR("unmatched '[' found during parsing");
        exit(1);
    } else if(p->stack_cursor < 0) {
        ERROR("unmatched ']' found during parsing");
        exit(1);
    }

    ins_len = ins_cursor + 2;
    ins     = realloc(ins, ins_len);
  
    ins[++ins_cursor] = OP_EOF;

    return ins;
}

int interpret(struct program *p, unsigned char *ins)
{
    size_t i = 0;

    while(ins[i] != OP_EOF) {
        switch(ins[i++]) {
            case OP_CELL_INC:
                if(p->pc[p->pc_cursor] >= 0xff)
                    p->pc[p->pc_cursor] = 0x00;
                else
                    p->pc[p->pc_cursor]++;
                break;

            case OP_CELL_DEC:
                if(p->pc[p->pc_cursor] <= 0x00)
                    p->pc[p->pc_cursor] = 0xff;
                else
                    p->pc[p->pc_cursor]--;
                break;

            case OP_PTR_LEFT:
                if(p->pc_cursor == 0) {
                    ERROR("attempted to move the tape pointer out of lower bounds");
                    return 1;
                } else {
                    p->pc_cursor--;
                }
                break;

            case OP_PTR_RIGHT:
                if(p->pc_cursor >= PC_LENGTH) {
                    ERROR("attempted to move the tape pointer out of upper bounds");
                    return 1;
                } else {
                    p->pc_cursor++;
                }
                break;

            case OP_GETCH:
                p->pc[p->pc_cursor] = getc(stdin);
                break;

            case OP_PUTCH:
                putc(p->pc[p->pc_cursor], stdout);
                break;

            case OP_JMP_FWD:
                if(p->pc[p->pc_cursor] == 0)
                    i = p->jmp_table[i];
                break;

            case OP_JMP_BCK:
                if(p->pc[p->pc_cursor] != 0)
                    i = p->jmp_table[i];
                break;

            default:
                continue;
        }
    }

    return 0;
}
