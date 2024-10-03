#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>


#include "typedefs.h"



#define MEMSIZE 30000

u8 mem[MEMSIZE] = {};
s32 cell = 0;
u32 inst = 0;
char* input = NULL;
char* prog = NULL;

s32* bracetable;
s32* jumptable;

s32 eofbhv = 0; // Behavior of input after EOF
s32 debug = 0;
s32 showinst = 0;
s64 numinst = 0; // Number of instructions interpreted

char* errstr;



// [ = -1, ] = 1
s32 jmpbal(char chr) { 
    if (chr == '[' || chr == ']')
        return chr - 92; // [\]
    return 0;
}


u64 jumpf(u64 ins, char* prg) {
    s32 bal = -1;
    while (bal != 0) {
        ins++;
        bal += jmpbal(prg[ins]);
    }
    return ins;
}


u64 jumpb(u64 ins, char* prg) {
    s32 bal = 1;
    while (bal != 0) {
        ins--;
        bal += jmpbal(prg[ins]);
    }
    return ins;
}



u8 readinp(u8 cell) {
    s32 c;
    if (input != NULL) {
        if (*input != '\0') 
            return *(input++); // Advance input
        goto eof;
    } else {
        c = getchar();
        if (c != EOF) 
            return c;
        goto eof;
    }
eof:
    switch (eofbhv) {
    default:
    case 0: // Leave value unchanged
        return cell;
    case 1: // Set to 0
        return 0;
    case 2: // Set to -1
        return -1;
    }
}



void keyinthandler(s32 sig) {
    jumptable[inst] = 8; // Make it goto the end
}



// Read file and null terminate.
char* readf(char* filename, s64 *len) {
    FILE* fil = fopen(filename, "r+");

    if (fil == NULL) {
        errstr = "Error opening file";
        goto fopenerr;
    }

    fseek(fil, 0, SEEK_END);
    s64 flen = ftell(fil);
    if (len != NULL)
        *len = flen;
    rewind(fil);

    char* filestr = malloc(flen + 1);
    u64 numread = fread(filestr, flen, 1, fil);

    if (numread != 1) {
        if (feof(fil)) {
            errstr = "Error reading file: unexpected end";
        } else {
            errstr = "Error reading file";
        }
        goto err;
    }

    fclose(fil);

    filestr[flen] = '\0';

    return filestr;
err:
    free(filestr);
fopenerr:
    fclose(fil);
    return NULL;
}



void genshortprog(char* prog) { // Remove non-BF characters
    char* prg = prog;
    char c;
    u64 cur = 0;
    while (c = *prg++) {
        switch(c) {
        case '+':
        case '-':
        case '>':
        case '<':
        case '[':
        case ']':
        case '.':
        case ',':
            prog[cur++] = c; 
        }
    }
    prog[cur] = '\0';
}



void genbracetable(char* prog, s32* bracetable) {
    for (u64 ins = 0; prog[ins]; ins++) {
        switch (prog[ins]) {
        case '[':
            bracetable[ins] = jumpf(ins, prog);
            break;
        case ']':
            bracetable[ins] = jumpb(ins, prog);
            break;
        }
    }
}


void genjumptable(char* prog, s32* jumptable) {
    s32 opcode;
    char c;

    while (c = *prog++) {
        opcode = 8;
        switch(c) {
        case '+': opcode--;
        case '-': opcode--;
        case '>': opcode--;
        case '<': opcode--;
        case '[': opcode--;
        case ']': opcode--;
        case '.': opcode--;
        case ',': opcode--;
            *jumptable = opcode;
        }
        jumptable++;
    }
    *jumptable = 8;
}




int main(int argc, char* argv[]) {
    setvbuf(stdout, NULL, _IONBF, 0); // Disable buffering for realtime output
    signal(SIGINT, keyinthandler); // Catch CTRL+C
    
    s32 opt;
    while ((opt = getopt(argc, argv, "di:ne:")) != -1) {
        switch (opt) {
        case 'd': // Debug
            debug = 1;
            break;
        case 'i': // Input in args instead of stdin
            input = optarg;
            break;
        case 'n': // Show number of instructions
            showinst = 1;
            break;
        case 'e': // Behavior of input after EOF
            eofbhv = atoi(optarg);
            break;
        case '?':
            if (optopt == 'i') {
                input = "";
            }
            break;
        }
    }

    if (argc < 2 || optind == argc) {
        errstr = "Error: no program given";
        goto err;
    }

    char* progname = argv[optind];

    if (access(progname, F_OK) != 0) {
        errstr = "Error: program not found";
        goto err;
    }

    prog = readf(progname, NULL);

    if (prog == NULL) {
        goto err;
    }


    genshortprog(prog); // Shorten by removing unused instructions

    bracetable = malloc(strlen(prog) * sizeof(s32));
    jumptable = malloc((strlen(prog) + 1) * sizeof(s32));

    genbracetable(prog, bracetable); // Generate the jump table for loops
    genjumptable(prog, jumptable); // Generate the dispatch table


    void* gototable[10] = {&&plus, &&minus, &&right, &&left, &&lbracket, &&rbracket, &&dot, &&comma, &&end};


#define DISPATCH goto *gototable[jumptable[++inst]]

    goto *gototable[jumptable[0]];

    plus:
        mem[cell]++;
        numinst++;
        DISPATCH;

    minus:
        mem[cell]--;
        numinst++;
        DISPATCH;

    right:
        cell++;
        numinst++;
        DISPATCH;

    left:
        cell--;
        numinst++;
        DISPATCH;

    lbracket:
        if (mem[cell] == 0)
            inst = bracetable[inst];
        numinst++;
        DISPATCH;

    rbracket:
        if (mem[cell] != 0)
            inst = bracetable[inst];
        numinst++;
        DISPATCH;

    dot:
        putchar(mem[cell]);
        numinst++;
        DISPATCH;

    comma:
        mem[cell] = readinp(mem[cell]);
        numinst++;
        DISPATCH;

end:


    if (showinst)
        printf("\nNumber of instructions: %ld", numinst);

    if (debug) {
        putchar('\n');
        for (s32 i = 0; i < MEMSIZE; i++) {
            printf("%02X ", mem[i]);
        }
    }
    putchar('\n');
    exit(0);
err:
    fputs(errstr, stderr); fputc('\n', stderr);
    exit(1);
}
