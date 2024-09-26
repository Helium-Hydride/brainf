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
char curinst = 0xFF;
char* input = NULL;
char* prog = NULL;


s32 eofbhv = 0; // Behavior of input after EOF
s32 debug = 0;
s32 showinst = 0;
s64 numinst = 0; // Number of instructions interpreted
s64 maxinst = INT64_MAX;
volatile s32 breaked = 0;

char errstr[100] = {};



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


u8 readinp(u8 cell) {
    s32 c;
    if (input != NULL) {
        if (*input != '\0') {
            return *(input++); // Advance input
        } else {
            goto eof;
        }
    } else {
        c = getchar();
        if (c != EOF) {
            return c;
        } else {
            goto eof;
        }
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
    breaked = 1;
}


// Read file and null terminate.
char* readf(char* filename) {
    FILE* fil = fopen(filename, "r+");

    if (fil == NULL) {
        strcpy(errstr, "Error opening file");
        goto fopenerr;
    }

    fseek(fil, 0, SEEK_END);
    s64 flen = ftell(fil);
    rewind(fil);

    char* filestr = malloc(flen + 1);
    u64 bytesread = fread(filestr, flen, 1, fil);

    if (bytesread != flen) {
        if (ferror(fil)) {
            strcpy(errstr, "Error reading file");
        } else if (feof(fil)) {
            strcpy(errstr, "Error reading file: unexpected end");
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





int main(int argc, char* argv[]) {
    GETOPT("dm:i:ne:") {
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
        case 'e': // Behavior of input after EOF
            eofbhv = atoi(optarg);
            break;
        case '?':
            if (optopt == 'm') {
                maxinst = 0;
            } else if (optopt == 'i') {
                input = "";
            }
            break;
    }

    if (argc < 2 || optind == argc) {
        strcpy(errstr, "Error: no program given");
        goto err;
    }

    char* progname = argv[optind];

    if (access(progname, F_OK) != 0) {
        strcpy(errstr, "Error: program not found");
        goto err;
    }

    prog = readf(progname);

    if (prog == NULL) {
        goto err;
    }


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
                mem[cell] = readinp(mem[cell]);
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
    exit(0);
err:
    fputs(errstr, stderr); fputc('\n', stderr);
    exit(1);
}
