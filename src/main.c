#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>


#include "typedefs.h"



#define GETOPT(opts) \
s32 _opt; \
while ((_opt = getopt(argc, argv, (opts))) != -1) switch (_opt)



#define MEMSIZE 30000

u8 mem[MEMSIZE] = {};
s32 cell = 0;
u32 inst = 0;
char* input = NULL;
char* prog = NULL;
char* shortprog;
s64 proglen;


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


s32 jumpf(s32 ins, char* prg) {
    s32 bal = -1;
    while (bal != 0) {
        ins++;
        bal += jmpbal(prg[ins]);
    }
    return ins;
}


s32 jumpb(s32 ins, char* prg) {
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
    inst = proglen - 1; // Make it goto the end
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


char* genshortprog(char* prg, s64 proglen, s64 *newlen) { // Remove non-BF characters
    char* newprog = malloc(proglen + 1);
    s64 cur = 0;

    for (s32 ins = 0; ins < proglen; ins++) {
        switch(prg[ins]) {
        case '+':
        case '-':
        case '>':
        case '<':
        case '[':
        case ']':
        case '.':
        case ',':
            newprog[cur] = prg[ins];
            cur++;
            break;
        }
    }
    newprog[cur + 1] = '\0';
    *newlen = cur;
    newprog = realloc(newprog, cur + 1);
    return newprog;
}






s32* genbracetable(char* prg, s64 proglen) {
    s32 *bracetable = malloc(proglen * sizeof(s32));

    for (s32 ins = 0; ins < proglen; ins++) {
        switch (prg[ins]) {
            case '[':
                bracetable[ins] = jumpf(ins, prg);
                break;
            case ']':
                bracetable[ins] = jumpb(ins, prg);
                break;
        }
    }
    return bracetable;
}


s32* genjumptable(char* prg, s64 proglen) {
    s32* jumptable = malloc((proglen + 1) * sizeof(s32));

    for (s32 ins = 0; ins < proglen; ins++) {
        switch(prg[ins]) {
        case '+':
            jumptable[ins] = 0;
            break;
        case '-':
            jumptable[ins] = 1;
            break;
        case '>':
            jumptable[ins] = 2;
            break;
        case '<':
            jumptable[ins] = 3;
            break;
        case '[':
            jumptable[ins] = 4;
            break;
        case ']':
            jumptable[ins] = 5;
            break;
        case '.':
            jumptable[ins] = 6;
            break;
        case ',':
            jumptable[ins] = 7;
            break;
        default:
            jumptable[ins] = 8;
            break;
        }
    }
    jumptable[proglen] = 9;

    return jumptable;
}




int main(int argc, char* argv[]) {
    setvbuf(stdout, NULL, _IONBF, 0); // Disable buffering for realtime output
    signal(SIGINT, keyinthandler); // Catch CTRL+C
    GETOPT("di:ne:") {
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

    if (argc < 2 || optind == argc) {
        errstr = "Error: no program given";
        goto err;
    }

    char* progname = argv[optind];

    if (access(progname, F_OK) != 0) {
        errstr = "Error: program not found";
        goto err;
    }

    prog = readf(progname, &proglen);

    if (prog == NULL) {
        goto err;
    }


    shortprog = genshortprog(prog, proglen, &proglen); // Shorten by removing unused instructions


    s32* bracetable = genbracetable(shortprog, proglen); // Generate the jump table for loops
    s32* jumptable = genjumptable(shortprog, proglen); // Generate the dispatch table


    void* gototable[10] = {&&plus, &&minus, &&right, &&left, &&lbracket, &&rbracket, &&dot, &&comma, &&def, &&end};


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

    def:
        numinst--; // Discount non-BF instructions
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
