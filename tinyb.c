#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char help[] =
    "Tiny Brainfuck Interpreter\n"
    "Usage: tbc infile(s)...\n"
    ;

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
#define STACK_LENGTH  30000

struct program {
    unsigned char pc[PC_LENGTH];
    unsigned char stack[STACK_LENGTH];

    unsigned int  pc_cursor;
    unsigned int  stack_cursor;
};

unsigned char *parse_ins(struct program *p, FILE *fp);
int interpret(struct program *p, unsigned char *ins);

int main(int argc, char **argv)
{
    struct program *p = malloc(sizeof(struct program));
    memset(p->pc,    0, PC_LENGTH);
    memset(p->stack, 0, STACK_LENGTH);

    p->pc_cursor    = 0;
    p->stack_cursor = 0;

    FILE *fp;

    if(argc != 2 || (fp = fopen(argv[1], "r")) == NULL) {
        fputs(help, stderr);
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
                  ins_len    = 8,
                  sp         = 0;
    unsigned char *ins = malloc(ins_len);
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
                sp++;
                break;
            case ']':
                ins[ins_cursor++] = OP_JMP_BCK;
                sp--;
                break;
            default:
                // ignore
                break;
        }
    }

    if(sp != 0) {
        fprintf(stderr, "Oops: unmatched '[' found\n");
        exit(1);
    }

    ins_len = ins_cursor + 2;
    ins     = realloc(ins, ins_len);
  
    ins[++ins_cursor] = OP_EOF;

    return ins;
}

int interpret(struct program *p, unsigned char *ins)
{
    size_t  inp_cursor = 0,
            inp_len    = 0;
    char *inp          = NULL;

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
                    fprintf(stderr, "Oops: memory underflowed\n");
                    return 1;
                } else {
                    p->pc_cursor--;
                }

                break;

            case OP_PTR_RIGHT:
                if(p->pc_cursor >= PC_LENGTH) {
                    fprintf(stderr, "Oops: memory overflowed\n");
                    return 1;
                } else {
                    p->pc_cursor++;
                }

                break;

            case OP_GETCH:
                if(inp == NULL)
                    getline(&inp, &inp_len, stdin);

                // do nothing if we have already read the whole
                // input string
                if(inp_cursor < strlen(inp))
                    p->pc[p->pc_cursor] = inp[inp_cursor++];

                break;

            case OP_PUTCH:
                putc(p->pc[p->pc_cursor], stdout);
                break;

            case OP_JMP_FWD:
                if(p->stack_cursor > STACK_LENGTH - 1)
                    continue;
                
                if(p->pc[p->pc_cursor] == 0) {
                    int depth = 1;

                    while(depth > 0 && ins[++i] != OP_EOF) {
                        if(ins[i] == OP_JMP_FWD) depth++;
                        else if(ins[i] == OP_JMP_BCK) depth--;
                    }
                } else {
                    p->stack[p->stack_cursor++] = i;
                }

                break;
            
            case OP_JMP_BCK:
                if(p->stack_cursor <= 0)
                    continue;

                if(p->pc[p->pc_cursor] != 0)
                    i = p->stack[p->stack_cursor - 1];
                else
                    p->stack_cursor--;

                break;

            default:
                continue;
        }
    }

    free(inp);

    return 0;
}
