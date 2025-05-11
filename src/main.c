#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define VERSION "0.1.0"

#define TOK_NOP -1   /* so we can do nothing */
#define TOK_NUL 0    /* EOF */
#define TOK_NUM 1    /* 0-9* */
#define TOK_ADD 2    /* + */
#define TOK_SUB 3    /* - */
#define TOK_MUL 4    /* * */
#define TOK_DIV 5    /* / */
#define TOK_LPA 6    /* ( */
#define TOK_RPA 7    /* ) */
#define TOK_LBL 8    /* { */
#define TOK_RBL 9    /* } */
#define TOK_CMR 10   /* ~ */
#define TOK_COM 11   /* , */
#define TOK_IDE 12   /* A-Za-z* */
#define TOK_COL 13   /* : */
#define TOK_END 14   /* ; */
#define TOK_INT 15   /* int */
#define TOK_FUN 16   /* function */
#define TOK_RET 17   /* return */
#define TOK_GOT 18   /* goto */
#define TOK_LAB 19   /* label */
#define TOK_IF 20    /* if */
#define TOK_ELS 21   /* else */
#define TOK_WHILE 22 /* while */
#define TOK_IS 23    /* == */
#define TOK_ISNOT 24 /* != */
#define TOK_SET 25   /* = */
#define TOK_ABOVE 26 /* > */
#define TOK_BELOW 27 /* < */
#define TOK_NOT 28   /* ! */
#define TOK_STR 29   /* string literal */
#define TOK_ASM 30   /* __asm */
#define TOK_AND 31   /* & */
#define TOK_LSQ 32   /* [ */
#define TOK_RSQ 33   /* ] */

int expr();
int eval();

int first = 0;
int num;
int tok;
char id[32];
char idstack[256][32];
char strstack[256][32];
int sp;
int strsp;

/* functions and variables */
char offsets[2048];
char is_function[2048];
char names[2048][32];
int variable_count;
int label;
int string_literal_count;

FILE *fp = NULL;
FILE *fo = NULL;

int evalindex();
int parenths();

enum
{
        ARCH_LINUX,
        ARCH_OSA_86,
        ARCH_OSA_CE,
};

int arch = ARCH_LINUX;

#define ex() __ex(__func__)

__ex(const char *caller)
{
        printf("! STOPPED COMPILATION DUE TO ERROR !\n");
        printf("raised by: %s\n", caller);
        while (1)
                ;
}

copyname(char *d, char *s)
{
        int i;
        for (i = 0; i < 32; ++i)
        {
                d[i] = s[i];
        }

        return 1;
}

push()
{
        copyname(idstack[sp], id);
        sp += 1;
        return 0;
}

pop()
{
        sp -= 1;
        copyname(id, idstack[sp]);
        return 0;
}

pushstr()
{
        copyname(strstack[strsp], id);
        strsp += 1;
        return 0;
}

popstr()
{
        strsp -= 1;
        copyname(id, strstack[strsp]);
        return 0;
}

iseq(char *a, char *b)
{
        while (*a == *b)
        {
                if (*a == 0)
                {
                        return 1;
                }

                a = a + 1;
                b = b + 1;
        }

        return 0;
}

int offset = 4;
create_variable(char *name, int count)
{
        copyname(names[variable_count], name);
        is_function[variable_count] = 0;
        offsets[variable_count] = offset;
        offset += 4 * count;
        variable_count = variable_count + 1;
        return 1;
}

create_function(char *name)
{
        copyname(names[variable_count], name);
        is_function[variable_count] = 1;
        variable_count = variable_count + 1;
        return 1;
}

find_variable(char *name, int *is_func, int *off)
{
        printf("finding=%s\n",name);
        int i;
        for (i = 0; i < 2048; ++i)
        {
                if (iseq(names[i], name))
                {
                        *is_func = is_function[i];
                        *off = offsets[i];
                        return i;
                }

                if (i < variable_count)
                        printf("var: %s\n", names[i]);
        }

        *off = 0;
        return -1;
}

getchr()
{
        int x = fgetc(fp);
        if (x == EOF)
        {
                return 0;
        }
        return x;
}

next()
{
        int x;
        int i;
        x = getchr();
        num = 0;
        for (i = 0; i < 32; i = i + 1)
        {
                id[i] = 0;
        }
        while (x == 32 || x == 9 || x == 13 || x == 10)
        {
                x = getchr();
        }

        if (tok == TOK_RPA && x == '{')
        {
                ungetc(x, fp);
                tok = TOK_END;
                return tok;
        }
        else if (x == '+')
        {
                tok = TOK_ADD;
        }
        else if (x == '"')
        {
                tok = TOK_STR;
                x = getchr();
                i = 0;
                while (x != '"')
                {
                        id[i] = x;
                        x = getchr();
                        if (x == '\\')
                        {
                                x = getchr();
                                if (x == 'n')
                                        x = '\n';
                                if (x == 'r')
                                        x = '\r';
                                if (x == 't')
                                        x = '\t';
                                if (x == 'a')
                                        x = '\a';
                                if (x == '0')
                                        x = '\0';
                        }
                        i = i + 1;
                }
                pushstr();
                string_literal_count += 1;
                return tok;
        }
        else if (x == '-')
        {
                tok = TOK_SUB;
        }
        else if (x == '&')
        {
                tok = TOK_AND;
        }
        else if (x == '[')
        {
                tok = TOK_LSQ;
        }
        else if (x == ']')
        {
                tok = TOK_RSQ;
        }
        else if (x == '=')
        {
                x = getchr();
                if (x == '=')
                {
                        tok = TOK_IS;
                }
                else
                {
                        ungetc(x, fp);
                        tok = TOK_SET;
                }
        }
        else if (x == '!')
        {
                x = getchr();
                if (x == '=')
                {
                        tok = TOK_ISNOT;
                }
                else
                {
                        ungetc(x, fp);
                        tok = TOK_NOT;
                }
        }
        else if (x == '*')
        {
                tok = TOK_MUL;
        }
        else if (x == '>')
        {
                tok = TOK_ABOVE;
        }
        else if (x == '<')
        {
                tok = TOK_BELOW;
        }
        else if (x == '(')
        {
                tok = TOK_LPA;
        }
        else if (x == ')')
        {
                tok = TOK_RPA;
        }
        else if (x == '{')
        {
                tok = TOK_LBL;
        }
        else if (x == '}')
        {
                tok = TOK_RBL;
        }
        else if (x == '~')
        {
                tok = TOK_CMR;
        }
        else if (x == ',')
        {
                tok = TOK_COM;
        }
        else if (x == ';')
        {
                tok = TOK_END;
        }
        else if (x == ':')
        {
                tok = TOK_COL;
        }
        else if (x == '/')
        {
                x = getchr();
                if (x == '*')
                {
                        while (1)
                        {
                                x = getchr();
                                if (x == '*' && getchr() == '/')
                                {
                                        goto end;
                                }
                        }
                }
                else
                {
                        ungetc(x, fp);
                        tok = TOK_DIV;
                }
        }

        /* IDENTIFIERS AND KEYWORDS */
        else if ((x >= 'A' && x <= 'Z') || (x >= 'a' && x <= 'z') || x == '_')
        {
                tok = TOK_IDE;
                i = 0;
                while ((x >= 'A' && x <= 'Z') || (x >= 'a' && x <= 'z') || x == '_' || (x >= '0' && x <= '9'))
                {
                        id[i] = x;
                        x = getchr();
                        i = i + 1;
                }

                if (iseq(id, "int"))
                {
                        tok = TOK_INT;
                }
                else if (iseq(id, "return"))
                {
                        tok = TOK_RET;
                }
                else if (iseq(id, "goto"))
                {
                        tok = TOK_GOT;
                }
                else if (iseq(id, "if"))
                {
                        tok = TOK_IF;
                }
                else if (iseq(id, "else"))
                {
                        tok = TOK_ELS;
                }
                else if (iseq(id, "while"))
                {
                        tok = TOK_WHILE;
                }
                else if (iseq(id, "__asm"))
                {
                        tok = TOK_ASM;
                }
                else if (x == '(')
                {
                        tok = TOK_FUN;
                }
                else if (x == ':')
                {
                        tok = TOK_LAB;
                }

                ungetc(x, fp);
        }

        /* numbers */
        else if (x >= '0' && x <= '9')
        {
                tok = TOK_NUM;
                while (x >= '0' && x <= '9')
                {
                        num *= 10;
                        num += x - '0';
                        x = getchr();
                }
                ungetc(x, fp);
        }
        else
        {
                tok = TOK_NUL;
        }
end:
        return tok;
}

match(x)
{
        return next() == x;
}

expect(x)
{
        if (match(x))
        {
                return 1;
        }

        printf("failed match with %d\n", x);
        ex();
        return 0;
}

factor()
{
        int tmp;
        int tmp2;
        tmp = 0;
        tmp2 = 0;
        switch (tok)
        {
        case TOK_MUL:
                if (first)
                {
                        next();
                        expr();
                        fprintf(fo, "\n\tpop eax");
                        fprintf(fo, "\n\tpush dword[eax]");
                }
                break;
        case TOK_NUM:
                fprintf(fo, "\n\tpush %d", num);
                first = 0;
                break;
        case TOK_STR:
                fprintf(fo, "\n\tpush __strlit%d", string_literal_count - 1);
                first = 0;
                break;
        case TOK_LSQ:
        {
                pop();
                evalindex();
                fprintf(fo, "\n\tpop ebx");
                fprintf(fo, "\n\tshl ebx,2");
                fprintf(fo, "\n\tsub edx,ebx");
                int off = ftell(fp);
                next();
                if (tok == TOK_LSQ)
                {
                        push();
                }
                else if (tok == TOK_SET)
                {
                        push();
                }
                else
                {
                        fprintf(fo, "\n\tpush dword[edx]");
                }
                fseek(fp, off, SEEK_SET);
                return 0;
        }
        case TOK_IDE:
        {
                char ide[sizeof(id)];
                copyname(ide, id);
                find_variable(ide, &tmp, &tmp2);
                int off = ftell(fp);
                next();
                fseek(fp, off, SEEK_SET);
                first = 0;
                if (tok == TOK_LSQ || tok == TOK_SET)
                {
                        fprintf(fo, "\n\tlea edx,[ebp-%d]", tmp2);
                        push();
                }
                else if (tmp)
                {
                        fprintf(fo, "\n\tpush %s", ide);
                }
                else
                {
                        fprintf(fo, "\n\tpush dword[ebp-%d]", tmp2);
                }
                return 0;
        }
        case TOK_LPA:
                parenths();
                break;
        case TOK_END:
        case TOK_RBL:
        case TOK_LBL:
                first = 1;
                return 0;
        case TOK_COM:
        case TOK_RPA:
        case TOK_RSQ:
        case TOK_NUL:
                return 0;
        default:
                printf("expected number, identifier, '(' or '{', got %d,%d,%s\n", tok, num, id);
                ex();
        }

        next();
        return 0;
}

term()
{
        factor();
        while (1)
        {
                switch (tok)
                {
                case TOK_MUL:
                        next();
                        factor();
                        fprintf(fo, "\n\tpop ebx");
                        fprintf(fo, "\n\tpop eax");
                        fprintf(fo, "\n\timul eax,ebx");
                        fprintf(fo, "\n\tpush eax");
                        break;
                case TOK_DIV:
                        next();
                        factor();
                        fprintf(fo, "\n\tpop ebx");
                        fprintf(fo, "\n\tpop eax");
                        fprintf(fo, "\n\tidiv eax,ebx");
                        fprintf(fo, "\n\tpush eax");
                        break;
                default:
                        return 0;
                }
        }
}

expr()
{
        term();
        while (1)
        {
                switch (tok)
                {
                case TOK_IS:
                        first = 1;
                        next();
                        term();
                        fprintf(fo, "\n\tpop ebx");
                        fprintf(fo, "\n\tpop eax");
                        fprintf(fo, "\n\tcmp eax,ebx");
                        fprintf(fo, "\n\tsete al");
                        fprintf(fo, "\n\tmovzx eax,al");
                        fprintf(fo, "\n\tpush eax");
                        break;
                case TOK_ISNOT:
                        first = 1;
                        next();
                        term();
                        fprintf(fo, "\n\tpop ebx");
                        fprintf(fo, "\n\tpop eax");
                        fprintf(fo, "\n\tcmp eax,ebx");
                        fprintf(fo, "\n\tsetne al");
                        fprintf(fo, "\n\tmovzx eax,al");
                        fprintf(fo, "\n\tpush eax");
                        break;
                case TOK_ABOVE:
                        first = 1;
                        next();
                        term();
                        fprintf(fo, "\n\tpop ebx");
                        fprintf(fo, "\n\tpop eax");
                        fprintf(fo, "\n\tcmp eax,ebx");
                        fprintf(fo, "\n\tseta al");
                        fprintf(fo, "\n\tmovzx eax,al");
                        fprintf(fo, "\n\tpush eax");
                        break;
                case TOK_BELOW:
                        first = 1;
                        next();
                        term();
                        fprintf(fo, "\n\tpop ebx");
                        fprintf(fo, "\n\tpop eax");
                        fprintf(fo, "\n\tcmp eax,ebx");
                        fprintf(fo, "\n\tsetb al");
                        fprintf(fo, "\n\tmovzx eax,al");
                        fprintf(fo, "\n\tpush eax");
                        break;
                case TOK_ADD:
                        first = 1;
                        next();
                        term();
                        fprintf(fo, "\n\tpop ebx");
                        fprintf(fo, "\n\tpop eax");
                        fprintf(fo, "\n\tadd eax,ebx");
                        fprintf(fo, "\n\tpush eax");
                        break;
                case TOK_AND:
                        first = 1;
                        next();
                        term();
                        fprintf(fo, "\n\tpop ebx");
                        fprintf(fo, "\n\tpop eax");
                        fprintf(fo, "\n\tand eax,ebx");
                        fprintf(fo, "\n\tpush eax");
                        break;
                case TOK_SUB:
                        first = 1;
                        next();
                        term();
                        fprintf(fo, "\n\tpop ebx");
                        fprintf(fo, "\n\tpop eax");
                        fprintf(fo, "\n\tsub eax,ebx");
                        fprintf(fo, "\n\tpush eax");
                        break;
                case TOK_NOT:
                        first = 1;
                        next();
                        term();
                        fprintf(fo, "\n\tpop eax");
                        fprintf(fo, "\n\txor eax,1");
                        fprintf(fo, "\n\tand eax,1");
                        fprintf(fo, "\n\tpush eax");
                        break;
                default:
                        return 0;
                }
        }

        return 0;
}

block()
{
        int tmp = ++label;
        fprintf(fo, "\n.m%d:", tmp);
        while (tok != TOK_RBL && tok != TOK_NUL)
        {
                first = 1;
                eval();
        }
        if (tok != TOK_RBL)
        {
                printf("missing '}'\n");
                ex();
        }
        tok = TOK_NOP;
        first = 1;
        fprintf(fo, "\n.m%d.end:", tmp);
        return 1;
}

parenths()
{
        int tmp = ++label;
        fprintf(fo, "\n.m%d:", tmp);
        while (tok != TOK_RPA && tok != TOK_NUL)
        {
                first = 1;
                eval();
        }
        if (tok != TOK_RPA)
        {
                printf("missing ')'\n");
                ex();
        }
        tok = TOK_NOP;
        first = 1;
        fprintf(fo, "\n.m%d.end:", tmp);
        return 1;
}

evalindex()
{
        while (tok != TOK_RSQ && tok != TOK_NUL)
        {
                eval();
        }
        if (tok != TOK_RSQ)
        {
                printf("missing ']'\n");
                ex();
        }
        tok = TOK_NOP;
        return 1;
}

__eval()
{
        int tmp = 0;
        switch (tok)
        {
        case TOK_ASM:
                expect(TOK_STR);
                popstr();
                string_literal_count -= 1;
                fprintf(fo, "\n\t%s", id);
                break;
        case TOK_INT:
        {
                expect(TOK_IDE);
                char ide[32];
                copyname(ide,id);
                int count = 1;
                int p = ftell(fp);
                if (match(TOK_LSQ))
                {
                        expect(TOK_NUM);
                        count *= num;
                        expect(TOK_RSQ);
                        while (match(TOK_LSQ)) /* if is array */
                        {
                                expect(TOK_NUM);
                                count *= num;
                                expect(TOK_RSQ);
                        }
                }
                else
                {
                        fseek(fp,p,SEEK_SET);

                }
                create_variable(ide, count);
                fprintf(fo, "\n\tsub esp,%d", 4 * count);
                break;
        }
        case TOK_RET:
                eval();
                fprintf(fo, "\n\tpop eax");
                fprintf(fo, "\n\tjmp .end");
                break;
        case TOK_IF:
                eval();
                fprintf(fo, "\n\tpop eax");
                fprintf(fo, "\n\ttest eax,eax");
                fprintf(fo, "\n\tjz .m%d.end", label + 1);
                block();
                break;
        case TOK_WHILE:
                tmp = ++label;
                fprintf(fo, "\n.m%d:", tmp);
                eval();
                fprintf(fo, "\n\tpop eax");
                fprintf(fo, "\n\ttest eax,eax");
                fprintf(fo, "\n\tjz .m%d.end", tmp);
                block();
                fprintf(fo, "\n\tjmp .m%d", tmp);
                fprintf(fo, "\n.m%d.end:", tmp);
                break;
        case TOK_SET:
        {
                pop();
                fprintf(fo, "\n\tmov edi,edx");
                first = 1;
                eval();
                fprintf(fo, "\n\tpop dword[edi]");
                break;
        }
        case TOK_GOT:
                expect(TOK_IDE);
                fprintf(fo, "\n\tjmp %s", id);
                next();
                break;
        case TOK_LAB:
                fprintf(fo, "%s:", id);
                expect(TOK_COL);
                break;
        case TOK_LBL:
                block();
                break;
        case TOK_FUN:
        {
                char tmp[32];
                copyname(tmp, id);
                int is_function = 0;
                int off = 4;
                int idx = find_variable(id, &is_function, &off);
                if (idx == -1)
                {
                        int count = 0;
                        create_function(tmp);
                        fprintf(fo, "\n%s:", tmp);
                        fprintf(fo, "\n\tpush ebp");
                        fprintf(fo, "\n\tmov ebp,esp");
                        next(); /* collect args */
                        int pos = ftell(fp);
                        while (tok != TOK_RPA)
                        {
                                next();
                                if (tok == TOK_IDE)
                                {
                                        count = count + 1;
                                }
                        }
                        fseek(fp, pos, SEEK_SET);
                        offset = -(4 * count + 4);
                        tok = TOK_NOP;
                        while (tok != TOK_RPA)
                        {
                                next();
                                if (tok == TOK_IDE)
                                {
                                        create_variable(id, 1);
                                }
                        }
                        offset = 4;
                        next();
                        next();
                        block();
                        fprintf(fo, "\n\tmov eax,0");
                        fprintf(fo, "\n.end:");
                        fprintf(fo, "\n\tmov esp,ebp");
                        fprintf(fo, "\n\tpop ebp");
                        fprintf(fo, "\n\tret");
                        break;
                }

                if (is_function)
                {
                        int i = 0;
                        next();
                        while (tok != TOK_RPA && tok != TOK_NUL)
                        {
                                first = 1;
                                eval();
                                i = i + 1;
                        }
                        next();
                        fprintf(fo, "\n\tcall %s", names[idx]);
                        fprintf(fo, "\n\tadd esp,%d", i * 4);
                        fprintf(fo, "\n\tpush eax");
                }
                else
                {
                        int i = 0;
                        next();
                        while (tok != TOK_RPA)
                        {
                                eval();
                                i = i + 1;
                        }
                        next();
                        fprintf(fo, "\n\tmov ebx, dword[ebp-%d]", (idx * 4) + 4);
                        fprintf(fo, "\n\tcall ebx");
                        fprintf(fo, "\n\tadd esp,%d", i * 4);
                        fprintf(fo, "\n\tpush eax");
                }
                break;
        }
        default:
                expr();
                break;
        }
        return 0;
}

eval()
{
        next();
        return __eval();
}

main(int c, char **v)
{
        char *src = NULL;
        char *out = NULL;
        for (int i = 1; i < c; ++i)
        {
                if (iseq(v[i], "-m") && i < c - 1)
                {
                        ++i;
                        if (iseq(v[i], "OSA/86"))
                        {
                                arch = ARCH_OSA_86;
                        }
                        if (iseq(v[i], "OSA/CE"))
                        {
                                arch = ARCH_OSA_CE;
                        }
                        if (iseq(v[i], "LINUX"))
                        {
                                arch = ARCH_LINUX;
                        }
                }
                else if (iseq(v[i], "-s") && i < c - 1)
                {
                        ++i;
                        src = v[i];
                }
                else if (iseq(v[i], "-o") && i < c - 1)
                {
                        ++i;
                        out = v[i];
                }
        }

        if (src == NULL || out == NULL)
                return 1;

        fp = fopen(src, "r");
        fo = fopen(out, "w");
        if (!fp)
        {
                printf("no file `%s` found\n", src);
                return 0;
        }
        if (!fo)
        {
                printf("no file `%s` found\n", out);
                return 0;
        }
        label = 0;
        fprintf(fo, "\t; EC VERSION %s\n", VERSION);
        fprintf(fo, "\t; source file is %s\n", src);
        fprintf(fo, "\t; output file is %s", out);
        if (arch == ARCH_LINUX)
        {
                fprintf(fo, "\n\t[bits 32]");
                fprintf(fo, "\n\tsection .text");
                fprintf(fo, "\n\tglobal _start");
                fprintf(fo, "\n_start:");
                fprintf(fo, "\n\tcall main");
                fprintf(fo, "\n\tmov ebx,eax");
                fprintf(fo, "\n\tmov eax,1");
                fprintf(fo, "\n\tint 0x80");
        }
        else if (arch == ARCH_OSA_86)
        {
                fprintf(fo, "\n\t[bits 32]");
                fprintf(fo, "\n\tsection .text");
                fprintf(fo, "\n\t%%include \"osa86/header.s\"");
                fprintf(fo, "\n\tglobal _start");
                fprintf(fo, "\n_start:");
                fprintf(fo, "\n\tcall main");
                fprintf(fo, "\n\tpush eax");
                fprintf(fo, "\n\tpush 0x0");
                fprintf(fo, "\n\tint 0x80");
                fprintf(fo, "\n\tjmp $");
        }
        sp = 0;
        strsp = 0;
        variable_count = 0;
        string_literal_count = 0;
        tok = TOK_NOP;
        fseek(fp, 0, SEEK_END);
        int size = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        while (ftell(fp) < size)
                eval();
        fprintf(fo, "\nsection .data\n");
        fprintf(fo, "metadata:\n");
        fprintf(fo, "\tdb 'EC VERSION %s',0\n", VERSION);
        fprintf(fo, "\tdb 'source file is %s',0\n", src);
        fprintf(fo, "\tdb 'output file is %s',0\n", out);
        int i;
        i = 0;
        while (i < string_literal_count)
        {
                popstr();
                fprintf(fo, "__strlit%d:\n\tdb ", i);
                for (int i = 0; i < (int)strlen(id); ++i)
                {
                        fprintf(fo, "%d,", id[i]);
                }
                fprintf(fo, "0\n");
                i += 1;
        }
        fclose(fp);
        fclose(fo);
        return 0;
}
