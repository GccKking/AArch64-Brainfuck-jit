#pragma once

//Must be made and run in Linux AArch64

#include <iostream>
#include <iomanip>
#include <cstdint>
#include <cstring>
#include <vector>
#include <stack>
#include <sys/mman.h>

typedef void (*func)();

class aarch64jit {

public:
    uint8_t* mem;
    uint8_t* ptr;
    size_t size;
    std::stack<uint8_t*> stk;
    aarch64jit(const size_t);
    ~aarch64jit();
    void err();
    void exec();
    void print();
    aarch64jit& push(std::initializer_list<uint8_t>);
    aarch64jit& push8(uint8_t);
    aarch64jit& push16(uint16_t);
    aarch64jit& push32(uint32_t);
    aarch64jit& push64(uint64_t);
    aarch64jit& BEQ();
    aarch64jit& BNE();
    int64_t bl_putchar_add();
    int64_t bl_getchar_add();

};

aarch64jit::aarch64jit(const size_t _size){

    size = _size;
    mem = (uint8_t*)mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (!mem) {
        std::cout << "failed to allocate memory\n";
        std::exit(-1);
    }
    memset(mem, 0, size);
    ptr = mem;

}

aarch64jit::~aarch64jit() {

    munmap(mem, size);
    mem = nullptr;

}

void aarch64jit::err() {

    std::cout << "data overflow, please try a memory size greater than" << size << '\n';
    std::exit(-1);

}

void aarch64jit::exec() {

    //std::cout << "getchar : 0x" << std::hex << std::setw(16) << std::setfill('0') << (uint64_t)getchar << std::dec << std::endl;
    //std::cout << "putchar : 0x" << std::hex << std::setw(16) << std::setfill('0') << (uint64_t)putchar << std::dec << std::endl;
    //std::cout << "memory  : 0x" << std::hex << std::setw(16) << std::setfill('0') << (uint64_t)mem << std::dec << std::endl;
    mprotect(mem, size, PROT_READ | PROT_EXEC);
    ((func)mem)();

}

void aarch64jit::print() {

    const char tbl[] = "0123456789abcdef";
    std::cout << "size: " << (uint64_t)(ptr - mem) << std::endl;
    for (uint8_t* i = mem; i < ptr; ++i) {
        printf("%c%c%c", tbl[((*i) >> 4) & 0x0f], tbl[(*i) & 0x0f], " \n"[!((i - mem + 1) & 0xf)]);
    }
    printf("\n");

}

aarch64jit& aarch64jit::push(std::initializer_list<uint8_t> codes) {

    for (auto c : codes) {
        ptr[0] = c;
        ++ptr;
        if (ptr >= mem + size) {
            err();
        }
    }
    return *this;

}

aarch64jit& aarch64jit::push8(uint8_t n) {

    if (ptr + 1 >= mem + size) {
        err();
    }
    ptr[0] = n;
    ++ptr;
    return *this;

}

aarch64jit& aarch64jit::push16(uint16_t n) {

    if ( ptr + 2 >= mem + size) {
        err();
    }
    ptr[0] = n & 0xff;
    ptr[1] = (n >> 8) & 0xff;
    ptr += 2;
    return *this;

}

aarch64jit& aarch64jit::push32(uint32_t n) {

    if (ptr + 4 >= mem + size) {
        err();
    }
    ptr[0] = n & 0xff;
    ptr[1] = (n >> 8) & 0xff;
    ptr[2] = (n >> 16) & 0xff;
    ptr[3] = (n >> 24) & 0xff;
    ptr += 4;
    return *this;

}

aarch64jit& aarch64jit::push64(uint64_t n) {

    if (ptr + 8 >= mem + size) {
        err();
    }
    ptr[0] = n & 0xff;
    ptr[1] = (n >> 8) & 0xff;
    ptr[2] = (n >> 16) & 0xff;
    ptr[3] = (n >> 24) & 0xff;
    ptr[4] = (n >> 32) & 0xff;
    ptr[5] = (n >> 40) & 0xff;
    ptr[6] = (n >> 48) & 0xff;
    ptr[7] = (n >> 56) & 0xff;
    ptr += 8;
    return *this;

}

aarch64jit& aarch64jit::BEQ() {

    push32(0x0); // foo
    stk.push(ptr);
    return *this;

}

aarch64jit& aarch64jit::BNE() {

    uint8_t* beq_ptr = stk.top();
    stk.pop();
    
    uint8_t* bne_ptr = ptr;
    int64_t p0 = bne_ptr - beq_ptr;
    int64_t p1 = beq_ptr - bne_ptr;
    uint32_t beq_code = (0b01010100 << 24) | ((( p0 / 4 ) & 0x7FFFF) << 5) | 0x0;  //BEQ machine code
    uint32_t bne_code = (0b01010100 << 24) | ((( p1 / 4 ) & 0x7FFFF) << 5) | 0x1;  //BNE machine code

    bne_ptr[0] = bne_code & 0xff;
    bne_ptr[1] = (bne_code >> 8) & 0xff;
    bne_ptr[2] = (bne_code >> 16) & 0xff;
    bne_ptr[3] = (bne_code >> 24) & 0xff;
    beq_ptr[-4] = beq_code & 0xff;
    beq_ptr[-3] = (beq_code >> 8) & 0xff;
    beq_ptr[-2] = (beq_code >> 16) & 0xff;
    beq_ptr[-1] = (beq_code >> 24) & 0xff;

    ptr += 4;

    return *this;
    
}

int64_t aarch64jit::bl_putchar_add() {

    int64_t add = (int64_t)(((uint64_t)putchar - (uint64_t)ptr) >> 2) & 0x3ffffff;
    return add;

}

int64_t aarch64jit::bl_getchar_add() {

    int64_t add = (int64_t)(((uint64_t)getchar - (uint64_t)ptr) >> 2) & 0x3ffffff;
    return add;

}