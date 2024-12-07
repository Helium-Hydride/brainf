#include <fstream>
#include <print>
#include <memory>
#include <set>
#include <vector>
#include <stack>
#include <string>
#include <expected>
#include <csignal>
#include <filesystem>
#include <chrono>
#include <unistd.h>


#include "typedefs.h"
#include "helpers.h"



std::unique_ptr<u8[]> mem;
s64 cell = 0;
u64 inst = 0;

u64 num_insts = 0;

std::string_view input;
std::string_view prog_from_args;

typedef void (*instfunc)();

std::vector<u64> bracetable;
std::vector<instfunc> jumptable;



struct {
    std::chrono::steady_clock::time_point begin;
    std::chrono::steady_clock::time_point end;
} timer;



enum eof_bhv {
    UNCHANGED, ZERO, MINUS_ONE
};

struct {
    eof_bhv eofbhv;
    bool show_inst;
    bool input_in_args;
    bool prog_in_args;
    bool timer;
    u64 mem_size;
} flags = {UNCHANGED, false, false, false, false, 30000};

void parse_args(s32 argc, char** argv) {
    s32 opt;
    while ((opt = getopt(argc, argv, "i:ne:p:m:t")) != -1) {
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
        case 'm':
            flags.mem_size = atol(optarg);
            break;
        case 't':
            flags.timer = true;
            break;
        }
    }
}





void plus(), minus(), right(), left(), lbracket(), rbracket(), dot(), comma(), end();

instfunc to_inst(char c) {
    switch (c) {
    case '+': return plus;
    case '-': return minus;
    case '>': return right; 
    case '<': return left; 
    case '[': return lbracket;
    case ']': return rbracket;
    case '.': return dot;
    case ',': return comma;
    default:  return nullptr;
    }
}


void shorten(std::string& prog) {
    std::erase_if(prog, [] (char c) {
        return !std::string("+-><[].,").contains(c);
    });
}


std::vector<instfunc> genjumptable(std::string_view prog) {
    std::vector<instfunc> table;

    for (const char& c: prog) {
        table.push_back(to_inst(c));
    }

    table.push_back(end);

    return table;
}


std::expected<std::vector<u64>, std::string> makebracetable(std::string_view prog) {
    std::vector<u64> table;
    table.resize(prog.size());

    std::stack<u64> lstack;
    
    for (u64 i = 0; i < prog.size(); i++) {
        switch (prog[i]) {
        case '[':
            lstack.push(i);
            break;
        case ']':
            if (lstack.empty())
                return std::unexpected("Error: mismatched ]");

            table[i] = lstack.top();
            table[lstack.top()] = i;
            lstack.pop();
        }
    }

    if (!lstack.empty())
        return std::unexpected("Error: mismatched [");
        
    return table;
}


std::vector<u64> genbracetable(std::string_view prog) {
    return value_or_else(makebracetable(prog), [] (auto err) -> std::vector<u64> {
        error("{}", err);
    });
}



std::expected<std::string, std::string> get_prog(s32 argc, char** argv) {
    std::string progname;
    std::string prog;
    
    if (!flags.prog_in_args) {
        if (argc < 2 || optind == argc) {
            return std::unexpected("Error: program not given");
        }

        progname = argv[optind];

        if (!std::filesystem::exists(progname)) {
            return std::unexpected("Error: program not found");
        }

        prog = readfile(progname);
    } else {
        prog = prog_from_args;
    }

    return prog;
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




#ifdef __clang__
#define MUSTTAIL __attribute__((musttail))
#else
#define MUSTTAIL
#endif

// GCC will only optimize this with O2
#define DISPATCH ++num_insts; MUSTTAIL return jumptable[++inst]();


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
        inst = bracetable[inst];
    DISPATCH;
}

void rbracket() {
    if (mem[cell] != 0)
        inst = bracetable[inst];
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


void end() {
    if (flags.show_inst) 
        std::print("\nNumber of instructions: {}", num_insts);
    
    if (flags.timer) {
        timer.end = std::chrono::steady_clock::now();
        auto nanosecs = std::chrono::duration_cast<std::chrono::nanoseconds> (timer.end - timer.begin).count();
        auto secs = nanosecs / 1000000000.0;

        std::print("\nTime: {} seconds", secs);

        std::print("\nInstructions per second: {}", num_insts / secs);
    }

    std::println();
}


void interpret() {
    jumptable[0]();
}



int main(int argc, char* argv[]) {
    parse_args(argc, argv);

    setbuf(stdout, nullptr); // Disable buffering


    std::string prog = value_or_else(get_prog(argc, argv), [] (auto err) -> std::string {
        error("{}", err);
    });


    shorten(prog);


    jumptable = genjumptable(prog);
    bracetable = genbracetable(prog);

    mem = std::make_unique<u8[]>(flags.mem_size);


    signal(SIGINT, [] (int) {
        jumptable[inst] = end;
    });

    signal(SIGSEGV, [] (int) {
        error("\nSegmentation fault!");
    });



    if (flags.timer) {
        timer.begin = std::chrono::steady_clock::now();
    }

    interpret();
}