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
    proglen = 0; // Make for loop exit early without extra condition
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





int main(int argc, char* argv[]) {
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


    s32* bracetable = genbracetable(prog, proglen); // Generate the jump table for loops


    setvbuf(stdout, NULL, _IONBF, 0); // Disable buffering for realtime output

    signal(SIGINT, keyinthandler); // Catch CTRL+C


    for (inst = 0; inst < proglen; inst++) {
        switch (prog[inst]) {
            case '+':
                mem[cell]++;
                break;

            case '-':
                mem[cell]--;
                break;

            case '>':
                cell++;
                break;

            case '<':
                cell--;
                break;

            case '[':
                if (mem[cell] == 0)
                    inst = bracetable[inst];
                break;

            case ']':
                if (mem[cell] != 0)
                    inst = bracetable[inst];
                break;

            case '.':
                putchar(mem[cell]);
                break;

            case ',':
                mem[cell] = readinp(mem[cell]);
                break;

            default:
                numinst--; // Discount non-BF instructions
                break;
        }
        numinst++;
    }


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
