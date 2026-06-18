//Must be made and run in Linux AArch64

#include <iostream>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <vector>
#include <stack>
#include <fstream>
#include <sstream>
#include <ctime>
#include <chrono>

#include "AArch64_jit.h"

uint8_t buff[0x20000];

enum op {
    op_add,  // buff[pointer]++
    op_sub,  // buff[pointer]--
    op_addp, // pointer++
    op_subp, // pointer--
    op_setz, // set zero
    op_jt,
    op_jf,
    op_in,
    op_out
};

struct opcode {
    uint8_t op;
    uint32_t num;
};

std::vector<opcode> scanner(const std::string& s) {
    std::vector<opcode> code;
    std::stack<size_t> stk;
    uint32_t cnt = 0;
    int line = 0;
    for (size_t i = 0; i < s.length(); ++i) {
        switch (s[i]) {
            case '+':
                cnt = 0;
                while (s[i] == '+') {
                    ++cnt;
                    ++i;
                }
                --i;
                code.push_back({op_add, cnt & 0xff});
                break;
            case '-':
                cnt = 0;
                while (s[i] == '-') {
                    ++cnt;
                    ++i;
                }
                --i;
                code.push_back({op_sub, cnt & 0xff});
                break;
            case '>':
                cnt = 0;
                while (s[i] == '>') {
                    ++cnt;
                    ++i;
                }
                --i;
                code.push_back({op_addp, cnt});
                break;
            case '<':
                cnt = 0;
                while(s[i] == '<') {
                    ++cnt;
                    ++i;
                }
                --i;
                code.push_back({op_subp, cnt});
                break;
            case '[':
                if (i + 2 < s.length() && (s[i + 1] == '-' || s[i + 1] == '+') && s[i + 2] == ']') {
                    code.push_back({op_setz, 0});
                    i += 2;
                } else {
                    stk.push(code.size());
                    code.push_back({op_jf, 0});
                }
                break;
            case ']':
                if (stk.empty()) {
                    std::cout << "empty stack at line " << line << "\n";
                    std::exit(-1);
                }
                code[stk.top()].num = code.size() & 0xffffffff;
                code.push_back({op_jt, (uint32_t)stk.top()});
                stk.pop();
                break;
            case ',': code.push_back({op_in, 0}); break;
            case '.': code.push_back({op_out, 0}); break;
            case '\n': ++line; break;
        }
    }
    if (!stk.empty()) {
        std::cout << "lack ]\n";
        std::exit(-1);
    }
    return code;
}

void interpreter(const std::vector<opcode>& code) {
    using hrc = std::chrono::high_resolution_clock;
    auto begin = hrc::now();
    memset(buff, 0, sizeof(buff));
    uint32_t p = 0;
    for (size_t i = 0; i < code.size(); ++i) {
        switch (code[i].op) {
            case op_add:  buff[p] += code[i].num; break;
            case op_sub:  buff[p] -= code[i].num; break;
            case op_addp: p += code[i].num; break;
            case op_subp: p -= code[i].num; break;
            case op_setz: buff[p] = 0; break;
            case op_jt:   if (buff[p]) i = code[i].num; break;
            case op_jf:   if (!buff[p]) i = code[i].num; break;
            case op_in:   buff[p] = getchar(); break;
            case op_out:  putchar(buff[p]); break;
        }
    }
    auto end = hrc::now();
    //std::cout << "\ninterpreter time usage: ";
    //std::cout << (end - begin).count() * 1.0 / hrc::duration::period::den << "s\n";
    //These two lines of commented-out code are used to print the execution time consumed by the interpreter mode
}

void jit(const std::vector<opcode>& code) {
    aarch64jit mem(65536);
    memset(buff, 0, sizeof(buff));
    //std::cout << "memory  : 0x" << std::hex << std::setw(16) << std::setfill('0') << reinterpret_cast<uint64_t>(mem.mem) << std::dec << std::endl;
    //You can use the above line of code to directly display the starting memory address of the dynamically generated machine code, which is very useful during GDB debugging

    /* set stack and base pointer */
    mem.push({0xf3, 0x53, 0xbf, 0xa9});  // STP X19, X20, [SP, #-16]!
    mem.push({0xf5, 0x5b, 0xbf, 0xa9});  // STP X19, X20, [SP, #-16]!
    mem.push({0xf7, 0x63, 0xbf, 0xa9});  // STP X19, X20, [SP, #-16]!
    mem.push({0xf9, 0x6b, 0xbf, 0xa9});  // STP X19, X20, [SP, #-16]!
    mem.push({0xfb, 0x73, 0xbf, 0xa9});  // STP X19, X20, [SP, #-16]!
    mem.push({0xfd, 0x7b, 0xbf, 0xa9});  // STP X19, X20, [SP, #-16]!

    /* save register context */


    /* set bf machine's paper pointer */
    mem.push32(0xd2800000 | (((uint64_t)buff & 0xffff) << 5 ) | 19);
    mem.push32(0xf2a00000 | ((((uint64_t)buff >> 16) & 0xffff) << 5 ) | 19); 
    mem.push32(0xf2c00000 | ((((uint64_t)buff >> 32) & 0xffff) << 5 ) | 19); 
    mem.push32(0xf2e00000 | ((((uint64_t)buff >> 48) & 0xffff) << 5 ) | 19); 

    for (const auto& op : code) {
        switch (op.op) {
            case op_add: 
                mem.push32(0x39400268);
                mem.push32(0x11000000 | ((uint8_t)(op.num & 0xff) << 10) | (8 << 5) | 8);
                mem.push32(0x39000268);
                break; // ldrb w8, [x19]; add w8, w8, #n; strb w8, [x19]
            case op_sub: 
                mem.push32(0x39400268);
                mem.push32(0x51000000 | ((uint8_t)(op.num & 0xff) << 10) | (8 << 5) | 8);
                mem.push32(0x39000268);
                break; // ldrb w8, [x19]; sub w8, w8, #n; strb w8, [x19]
            case op_addp: 
                mem.push32(0x91000000 | ((op.num) << 10) | (19 << 5) | 19); 
                break;     // add x19, x19, #n
            case op_subp: 
                mem.push32(0xd1000000 | ((op.num) << 10) | (19 << 5) | 19); 
                break;     // sub x19, x19, #n
            case op_setz: 
                mem.push32(0x3900027f); 
                break;     // strb wzr, [x19]
            case op_jt: // if (al)
                mem.push32(0x39400260); // ldrb w0, [x19]
                mem.push32(0x7100001f); // cmp w0, #0
                mem.BNE();
                break;
            case op_jf: // if (!al)
                mem.push32(0x39400260); // ldrb w0, [x19]
                mem.push32(0x7100001f); // cmp w0, #0
                mem.BEQ();
                break;
            case op_in:
                mem.push32(0x94000000 | mem.bl_getchar_add()); // bl $getchar
                mem.push32(0x39000260); // strb w0, [x19]
                break;
            case op_out:
                mem.push32(0x39400260); // ldrb w0, [x19]
                mem.push32(0x94000000 | mem.bl_putchar_add()); // bl $putchar
                break;
        }
    }

    /* restore register context */
    mem.push({0xfd, 0x7b, 0xc1, 0xa8})              
       .push({0xfb, 0x73, 0xc1, 0xa8})              
       .push({0xf9, 0x6b, 0xc1, 0xa8})              
       .push({0xf7, 0x63, 0xc1, 0xa8})              
       .push({0xf5, 0x5b, 0xc1, 0xa8})              
       .push({0xf3, 0x53, 0xc1, 0xa8})
       .push({0xc0, 0x03, 0x5f, 0xd6});  //ret

    using hrc = std::chrono::high_resolution_clock;
    auto begin = hrc::now();
    //mem.print();
    //You can uncomment the line above to print the AArch64 machine code generated during compilation
    mem.exec();
    auto end = hrc::now();
    //std::cout << "\njit-compiler time usage: ";
    //std::cout <<(end - begin).count() * 1.0 / hrc::duration::period::den << "s\n";
    //These two lines of commented-out code are used to print the time taken for JIT compilation
}

void usage() {
    std::cout << "brainfuckjit - a brainfuck JIT compiler\n";
    std::cout << "usage:\n";
    std::cout << "  jit [options] <filename>\n\n";
    std::cout << "options:\n";
    std::cout << "  -i | interpreter mode\n";
    std::cout << "  -j | JIT compiler mode\n";
}

int main(int argc, const char* argv[]) {
    if (argc == 1) {
        usage();
        return 0;
    }

    bool interpreter_mode = false;
    bool jit_compiler_mode = false;
    int filename_index = -1;
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "-i") {
            interpreter_mode = true;
        } else if (std::string(argv[i]) == "-j") {
            jit_compiler_mode = true;
        } else if (argv[i][0] != '-') {
            filename_index = i;
        } else {
            std::cout << "error argument \"" << argv[i] << "\"\n\n";
            usage();
            return -1;
        }
    }

    if (!interpreter_mode && !jit_compiler_mode) {
        std::cout << "please choose interpreter or JIT-compiler\n\n";
        usage();
        return -1;
    }

    if (filename_index < 0) {
        std::cout << "no input file\n";
        usage();
        return -1;
    }

    std::ifstream fin(argv[filename_index]);
    if (fin.fail()) {
        std::cout << "cannot open file <" << argv[filename_index] << ">\n";
        return -1;
    }

    std::stringstream ss;
    ss << fin.rdbuf();
    auto code = scanner(ss.str());
    if (interpreter_mode) {
        interpreter(code);
    }
    if (jit_compiler_mode) {
        jit(code);
    }
    return 0;
}