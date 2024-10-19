#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <memory>
#include <map>
#include <unordered_map>
#include <vector>
#include <string>
#include <type_traits>
#include <numeric>
#include <cmath>
#include <atomic>
#include <thread>


#include "typedefs.h"


constexpr s32 mem_size = 30000;

u8 mem[mem_size] = {};
s64 cell = 0;
u64 inst = 0;

u64 num_inst = 0;

std::string input;

enum eof_bhv {
    MINUS_ONE, ZERO, UNCHANGED
};

struct {
    eof_bhv eofbhv;
    bool debug;
    bool show_inst;
    bool input_in_args;
} flags = {UNCHANGED, false, false, false};




enum instruction {
    PLUS, MINUS, LEFT, RIGHT, LBRACKET, RBRACKET, COMMA, DOT, END, NO_OP
};

constexpr s32 num_insts = 9;


instruction toinst(char c) {
    switch (c) {
    case '+': return PLUS;
    case '-': return MINUS;
    case '<': return LEFT; 
    case '>': return RIGHT; 
    case '[': return LBRACKET;
    case ']': return RBRACKET;
    case ',': return COMMA;
    case '.': return DOT;
    default:  return NO_OP;
    }
}


std::vector<instruction> genoptable(const std::string& prog) {
    std::vector<instruction> table;

    for (const char& c: prog) {
        if (toinst(c) != NO_OP) {
            table.push_back(toinst(c));
        }
    }

    table.push_back(END);
    return table;
}


std::vector<void*> genjumptable(const std::vector<instruction>& optable, const std::array<void*, num_insts>& gototable) {
    std::vector<void*> table;

    for (const instruction& op: optable) {
        table.push_back(gototable[op]);
    }

    return table;
}


s32 jmpbal(instruction ins) { 
    switch (ins) {
    case LBRACKET:
        return -1;
    case RBRACKET:
        return 1;
    default:
        return 0;
    }
}

u64 jumpf(u64 ins, const std::vector<instruction>& optable) {
    s32 bal = -1;
    while (bal != 0) {
        ins++;
        bal += jmpbal(optable[ins]);
    }
    return ins;
}

u64 jumpb(u64 ins, const std::vector<instruction>& optable) {
    s32 bal = 1;
    while (bal != 0) {
        ins--;
        bal += jmpbal(optable[ins]);
    }
    return ins;
}

u64 jumpd(u64 ins, const std::vector<instruction>& optable) {
    switch (optable[ins]) {
    case LBRACKET:
        return jumpf(ins, optable);
    case RBRACKET:
        return jumpb(ins, optable);
    default:
        return 0;
    }
}


std::vector<u64> genbracetable(const std::vector<instruction>& optable) {
    std::vector<u64> table;

    for (const instruction& ins: optable) {
        table.push_back(jumpd(ins, optable));
    }

    return table;
}


std::string readfile(const std::string& name) {
    std::ifstream f(name);
    std::stringstream buf;
    buf << f.rdbuf();

    return buf.str();
}






void readinp(u8& cell) {
    static u64 index;
    if (flags.input_in_args) {
        if (index < input.length()) {
            cell = input[index++];
        } else goto eof;
    } else {
        s32 chr = getchar();
        if (chr != EOF) {
            cell = chr;
        } else goto eof;
    }
eof:
    switch (flags.eofbhv) {
    case UNCHANGED: break;
    case ZERO:
        cell = 0;   break;
    case MINUS_ONE:
        cell = -1;  break;
    }
}


#define DISPATCH ++num_inst; goto *jumptablep[++inst]

void interpret(const std::vector<instruction>& optable, const std::vector<u64>& bracetable) {
    std::array<void*, num_insts> gototable = {&&plus, &&minus, &&left, &&right, &&lbracket, &&rbracket, &&comma, &&dot};

    const std::vector<void*> jumptable = genjumptable(optable, gototable);
    
    void* const* jumptablep = jumptable.data();
    const u64* bracetablep = bracetable.data();

    goto *jumptablep[0];

    plus:
        ++mem[cell];
        DISPATCH;
    minus:
        --mem[cell];
        DISPATCH;
    left:
        ++cell;
        DISPATCH;
    right:
        --cell;
        DISPATCH;
    lbracket:
        if (mem[cell] == 0)
            inst = bracetablep[inst];
        DISPATCH;
    rbracket:
        if (mem[cell] != 0)
            inst = bracetablep[inst];
        DISPATCH;
    comma:
        readinp(mem[cell]);
        DISPATCH;
    dot:
        putchar(mem[cell]);
        DISPATCH;

end:;
}





int main(int argc, char* argv[]) {



}