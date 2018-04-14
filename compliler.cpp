#include "header.h"

typedef const unsigned char op_code_t;
typedef const unsigned int inf_code_t;

// 0100WRXB  REX intel prefix
unsigned char get_rex(bool w = true, bool r = false, bool x = false, bool b = false) {
    return 0x40 | (w? 8:0) | (r? 4:0) | (x? 2:0) | (b? 1:0);
}

// ModR/M byte
unsigned char reg_reg(inf_code_t mode, inf_code_t reg0, inf_code_t reg_1) {
    return (mode << 6) + (reg0 << 3) + reg_1;
}

unsigned char cmp_reg(inf_code_t reg) {
    return 0xf8 | reg;
}

unsigned char mov_reg_mem(inf_code_t reg) {
    return 0xB8 | reg;
}

unsigned char push_reg(inf_code_t reg) {
    return 0x50 | reg;
}

unsigned char pop_reg(inf_code_t reg) {
    return 0x58 | reg;
}

// instructions
op_code_t mov_rr    = 0x89;
op_code_t ret_0     = 0xC3;
op_code_t nop       = 0x90;
op_code_t _not      = 0xF7;
op_code_t _xor      = 0x31;
op_code_t cmp_rax   = 0x3D;
op_code_t cmp_      = 0x81;
op_code_t jmp_short = 0xEB;
op_code_t jmp_near  = 0xE9;
op_code_t je_short  = 0x74;
op_code_t jne_short = 0x75;
op_code_t jge_short = 0x7D;
op_code_t jle_short = 0x7E;
op_code_t call_reg  = 0xFF;

// registers
inf_code_t r8  = 0;
inf_code_t eax = 0; 
inf_code_t ecx = 1;
inf_code_t edx = 2;
inf_code_t ebx = 3;
inf_code_t esp = 4; 
inf_code_t ebp = 5;
inf_code_t esi = 6;
inf_code_t edi = 7;

// argument regime
inf_code_t reg_naked = 3;

class programme_t {
    unsigned char* pr = (unsigned char*)mmap(0, 512, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
    unsigned char* cur = pr;
public:
    template<class... indices>
    void add(unsigned char ind, indices... inds) {
        add(ind);
        add(inds...);
    }
    void add(unsigned char a) {
        if (cur - pr == 512) {
            critical("Too long instructions list");
        }
        *cur = a; cur++;
    }
    void add_32(unsigned int a) {
        add(a & 0xFF); a >>= 8;
        add(a & 0xFF); a >>= 8;
        add(a & 0xFF); a >>= 8;
        add(a & 0xFF);
    }
    void add_64(unsigned long long a) {
        add_32(a&0xffffffff); a >>= 32;
        add_32(a&0xffffffff);
    }
    void ret_false() {
        add(get_rex(), _xor, reg_reg(reg_naked, eax, eax), ret_0);
    }
    void cmp(inf_code_t reg, int number, inf_code_t rex = get_rex()) {
        add(rex, cmp_, cmp_reg(reg)); add_32(number);
    }
    void print() {
        for (unsigned char* i = pr; i != cur; i++) {
            if (*i == nop) {
                printf("\n");
            } else {
                printf("%2x ", (int)(*i));
            }
        }
        fflush(stdout);
    }
    checker_t get_code() {
        mprotect(pr, 512, PROT_EXEC | PROT_READ);
        return { 
            (boolean(*)(
                char* const, ino64_t, size_t, size_t, void(*)(char* const, char* const)
            ))(void*)pr, 
            512 
        };
    }
};

extern "C"
struct checker_t compileJIT(struct function_t function) {
    programme_t pr;
    for (size_t i = 0; i < function.number; i++) {
        instruction_t op_cur = function.instruction_sequence[i];
		size_t data = op_cur.instruction_data.integer;
		char* string = op_cur.instruction_data.string;
		instruction_code_t op_code = op_cur.instruction;
        if (op_code == G_SIZE || op_code == E_SIZE || op_code == L_SIZE) {
            pr.cmp(edx, data); pr.add(nop);
            pr.add(
                op_code == G_SIZE ? jge_short :
                op_code == E_SIZE ?  je_short :
                                    jle_short, 
            6, nop);
            pr.ret_false(); pr.add(nop);
        } else if (op_code == N_LINK) {
            pr.cmp(ecx, data); pr.add(nop);
            pr.add(je_short, 6, nop);
            pr.ret_false();
        } else if (op_code == E_INUM) {
            pr.cmp(esi, data); pr.add(nop);
            pr.add(je_short, 6, nop);
            pr.ret_false();        
        } else if (op_code == E_NAME) {
            pr.add(get_rex(true, false, false, true), mov_rr, reg_reg(reg_naked, edi, r8), nop);
            for (size_t i = 0; string[i]; i++) {
                pr.add(
                    0x41, 0x80, 0x38, string[i], nop,    // cmp [r8], string[i]
                    je_short, 6, nop
                );
                pr.ret_false(); pr.add(nop);
                pr.add(get_rex(true, false, false, true), 0xFF, 0xC0, nop); // inc r8
            }
        } else if (op_code == EXEC) {
            pr.add(push_reg(esi), nop);
            pr.add(get_rex(), mov_reg_mem(esi)); pr.add_64((long long)string); pr.add(nop);
            pr.add(get_rex(false, false, false, true), call_reg, 0xD0 | r8, nop);
            pr.add(pop_reg(esi), nop);
        } else {
            critical("unknown op_code");
        }
	}
    pr.add(get_rex(), _xor, reg_reg(reg_naked, eax, eax), nop);
    pr.add(get_rex(), _not, 0xD0 | eax, nop);
    pr.add(ret_0, nop);
    pr.print();
    return pr.get_code();
}