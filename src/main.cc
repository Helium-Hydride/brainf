#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <print>
#include <memory>
#include <array>
#include <map>
#include <unordered_map>
#include <vector>
#include <stack>
#include <string>
#include <type_traits>
#include <numeric>
#include <cmath>
#include <atomic>
#include <thread>
#include <functional>
#include <exception>
#include <expected>
#include <csignal>
#include <filesystem>
#include <unistd.h>


#include "typedefs.h"


constexpr s32 mem_size = 30000;

u8 mem[mem_size] = {};
s64 cell = 0;
u64 inst = 0;

u64 num_insts = 0;

std::string_view input;
std::string_view prog_from_args;

typedef void (*instfunc)();

const u64* bracetablep;
instfunc* jumptablep;




enum eof_bhv {
    UNCHANGED, ZERO, MINUS_ONE
};

struct {
    eof_bhv eofbhv;
    bool show_inst;
    bool input_in_args;
    bool prog_in_args;
} flags = {UNCHANGED, false, false, false};




enum instruction {
    PLUS, MINUS, RIGHT, LEFT, LBRACKET, RBRACKET, DOT, COMMA, END, NO_OP
};

constexpr s32 num_ops = 9;


instruction toinst(char c) {
    switch (c) {
    case '+': return PLUS;
    case '-': return MINUS;
    case '>': return RIGHT; 
    case '<': return LEFT; 
    case '[': return LBRACKET;
    case ']': return RBRACKET;
    case '.': return DOT;
    case ',': return COMMA;
    default:  return NO_OP;
    }
}


std::vector<instruction> genoptable(std::string_view prog) {
    std::vector<instruction> table;

    for (const char& c: prog) {
        if (toinst(c) != NO_OP) {
            table.push_back(toinst(c));
        }
    }

    table.push_back(END);
    return table;
}


std::vector<instfunc> genjumptable(const std::vector<instruction>& optable, const std::array<instfunc, num_ops>& funcptable) {
    std::vector<instfunc> table;

    for (const instruction& op: optable) {
        table.push_back(funcptable[op]);
    }

    return table;
}


std::expected<std::vector<u64>, std::string> genbracetable(const std::vector<instruction>& optable) {
    std::vector<u64> table;
    table.reserve(optable.size());

    std::stack<u64> lstack;
    
    for (u64 i = 0; i < optable.size(); i++) {
        switch (optable[i]) {
        case LBRACKET:
            lstack.push(i);
            break;
        case RBRACKET:
            if (lstack.empty())
                return std::unexpected("Error: mismatched ]");

            table[i] = lstack.top();
            table[lstack.top()] = i;
            lstack.pop();
        default:;
        }
    }

    if (!lstack.empty())
        return std::unexpected("Error: mismatched [");

    return table;
}




std::string readfile(const std::string& name) {
    std::ifstream f {name};
    std::ostringstream buf;
    buf << f.rdbuf();

    return buf.str();
}



void set_cell_on_eof(u8& cell) {
    switch (flags.eofbhv) {
    case UNCHANGED: break;
    case ZERO:
        cell = 0;   break;
    case MINUS_ONE:
        cell = -1;  break;
    }
}

void readinp(u8& cell) {
    static u64 index;
    if (flags.input_in_args) {
        if (index < input.length()) {
            cell = input[index++];
            return;
        }
    } else {
        s32 chr = getchar();
        if (chr != EOF) {
            cell = chr;
            return;
        }
    }

    set_cell_on_eof(cell);
}




std::function<void(int)> sig_handler_helper;
void sig_handler(s32 sig) {sig_handler_helper(sig);}


// Hope GCC does the tail calls (Only works with -O2)
#define MUSTTAIL 

#define DISPATCH ++num_insts; MUSTTAIL return jumptablep[++inst]();



void plus() {
    mem[cell]++;
    DISPATCH;
}

void minus() {
    mem[cell]--;
    DISPATCH;
}

void right() {
    cell++;
    DISPATCH;
}

void left() {
    cell--;
    DISPATCH;
}

void lbracket() {
    if (mem[cell] == 0)
        inst = bracetablep[inst];
    DISPATCH;
}

void rbracket() {
    if (mem[cell] != 0)
        inst = bracetablep[inst];
    DISPATCH;
}

void dot() {
    putchar(mem[cell]);
    DISPATCH;
}

void comma() {
    readinp(mem[cell]);
    DISPATCH;
}

void end() {}


std::array<instfunc, num_ops> funcptable = {plus, minus, right, left, lbracket, rbracket, dot, comma, end};


void interpret() {
    sig_handler_helper = [&] (s32 sig) {
        if (sig == SIGINT) {
            jumptablep[inst] = end;
        } 
        
        if (sig == SIGSEGV) {
            std::println(stderr, "\nSegmentation fault!");
            exit(EXIT_FAILURE);
        }
    };


    jumptablep[0]();
}





void parse_args(s32 argc, char** argv) {
    s32 opt;
    while ((opt = getopt(argc, argv, "i:ne:p:")) != -1) {
        switch (opt) {
        case 'i': // Input in args instead of stdin
            flags.input_in_args = true;
            input = optarg;
            break;
        case 'n': // Show number of instructions
            flags.show_inst = true;
            break;
        case 'e': // Behavior of input after EOF
            flags.eofbhv = static_cast<eof_bhv>(atoi(optarg));
            break;
        case 'p':
            flags.prog_in_args = true;
            prog_from_args = optarg;
            break;
        }
    }
}



int main(int argc, char* argv[]) {
    parse_args(argc, argv);

    setbuf(stdout, nullptr); // Disable buffering


    std::string progname;
    std::string prog;
    
    if (!flags.prog_in_args) {
        if (argc < 2 || optind == argc) {
            std::println(stderr, "Error: program not given");
            exit(EXIT_FAILURE);
        }

        progname = argv[optind];

        if (!std::filesystem::exists(progname)) {
            std::println(stderr, "Error: program not found");
            exit(EXIT_FAILURE);
        }

        prog = readfile(progname);
    } else {
        prog = prog_from_args;
    }


    auto optable = genoptable(prog);
    auto bracetable = genbracetable(optable);

    if (!bracetable) {
        std::println(stderr, "{0}", bracetable.error());
        exit(EXIT_FAILURE);
    }

    std::vector<instfunc> jumptable = genjumptable(optable, funcptable);

    jumptablep = jumptable.data();
    bracetablep = bracetable->data();


    std::signal(SIGINT, sig_handler); // Handle CTRL+C
    std::signal(SIGSEGV, sig_handler);


    interpret();


    if (flags.show_inst) 
        std::println("\nNumber of instructions: {0}", num_insts);
}