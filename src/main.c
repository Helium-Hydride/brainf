#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>


#include "typedefs.h"
#include "common.h"

#define MEMSIZE 30000

u8 mem[MEMSIZE] = {};
s32 cell = 0;
u32 inst = 0;
char curinst = 0xFF;
char* input = NULL;
char* prog = NULL;


s32 debug = 0;
s32 showinst = 0;
s64 numinst = 0; // Number of instructions interpreted
s64 maxinst = INT64_MAX;
volatile s32 breaked = 0;

s32 tmp;


// [ = -1, ] = 1
s32 jmpbal(char chr) { 
    if (chr == '[' || chr == ']') {
        return chr - 92; // [\]
    } else {
        return 0;
    }
}


void jumpf(void) {
    s32 bal = -1;
    while (bal != 0) {
        inst++;
        bal += jmpbal(prog[inst]);
    }
}


void jumpb(void) {
    s32 bal = 1;
    while (bal != 0) {
        inst--;
        bal += jmpbal(prog[inst]);
    }
}



void keyinthandler(s32 sig) {
    breaked = 1;
}





int main(int argc, char* argv[]) {
    s32 opt;
    while ((opt = getopt(argc, argv, "dm:i:n")) != -1) {
        switch (opt) {
            case 'd': // Debug
                debug = 1;
                break;
            case 'm': // Maximum instructions
                maxinst = atol(optarg);
                break;
            case 'i': // Input in args instead of stdin
                input = optarg;
                break;
            case 'n': // Show number of instructions
                showinst = 1;
                break;
            case '?':
                if (optopt == 'm') {
                    maxinst = 0;
                } else if (optopt == 'i') {
                    input = "";
                }
                break;
        }
    }

    if (argc < 2 || optind == argc) {
        fputs("Error: no program given\n", stderr);
        exit(1);
    }

    char* progname = argv[optind];

    if (access(progname, F_OK) != 0) {
        fputs("Error: program not found\n", stderr);
        exit(1);
    }

    FILE* progf = fopen(progname, "r+");

    fseek(progf, 0, SEEK_END);
    s64 proglen = ftell(progf);
    rewind(progf);

    prog = malloc(proglen + 1);
    fread(prog, proglen, 1, progf);
    fclose(progf);

    prog[proglen] = '\0';

    

    setvbuf(stdout, NULL, _IONBF, 0); // Disable buffering for realtime output

    signal(SIGINT, keyinthandler); // Catch CTRL+C


    for (inst = 0; curinst != '\0'; inst++) {
        curinst = prog[inst];
        //printf("%c", curinst);
        switch (curinst) {
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
                if (mem[cell] == 0) {
                    jumpf();
                }
                break;

            case ']':
                if (mem[cell] != 0) {
                    jumpb();
                }
                break;

            case '.':
                putchar(mem[cell]);
                //usleep(1000 * 1);
                break;

            case ',':
                //...
                break;

            default:
                numinst--; // Discount non-BF instructions
                break;
        }
        numinst++;


        if (breaked) {
            break;
        }

        if (numinst >= maxinst) {
            break;
        }
    }

    if (showinst) {
        printf("\nNumber of instructions: %ld", numinst);
    }
    if (debug) {
        putchar('\n');
        for (s32 i = 0; i < MEMSIZE; i++) {
            printf("%02X ", mem[i]);
        }
    }
    putchar('\n');


}
