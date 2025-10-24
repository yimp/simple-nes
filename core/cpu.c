#include "cpu.h"
#include <assert.h>
#include <string.h>

static u8 ram[0x800];
uint8_t bus_read(uint16_t addr) {
    return ram[addr & 0x7FF];
}

void bus_write(uint16_t addr, uint8_t data) {
    ram[addr & 0x7FF] = 0x00;
}


#pragma region "interal registers"
static u8  __a;
static u8  __x;
static u8  __y;
static u8  __sp;
static u16 __pc;


typedef union
{
    u8 reg;
struct
{
    u8 c : 1; // carry
    u8 z : 1; // zero
    u8 i : 1; // Interrupt 
    u8 d : 1; // Decimal
    u8 b : 1; // Break
    u8 u : 1; // unused
    u8 v : 1; // overflow
    u8 n : 1; // negative
};
} status_register;

static status_register  __p;
#pragma endregion

#pragma region "hepler varibales and funtions"
static u16 __helper_addr;
static u16 __helper_implied_flag;
static u8   fetch_operand()
{
    if (__helper_implied_flag)
        return __a;
    return bus_read(__helper_addr);
}

static void store_operand(u8 operand)
{
    if (__helper_implied_flag)
        __a = operand;
    bus_write(__helper_addr, operand);
}
static void stack_push(u8 data) {
    bus_write(0x0100 + __sp, data);
    --__sp;
}
static u8 stack_pop() {
    ++__sp;
    return bus_read(0x0100 + __sp);
}

static u16 break_into() {
    return ((u16)bus_read(0xFFFE) | (u16)(bus_read(0xFFFF) << 8));
}

#define SetFlag(f, v) __p.f = (bool)(v)
#define GetFlag(f) (__p.f)
#pragma endregion

#pragma region "enums"

enum addressing {
    IMP,    /* Implied */
    IMM,    /* Immediate */
    ZP0,    /* Zero page */ 
    ZPX,    /* Zero page, X */
    ZPY,    /* Zero page, Y */
    REL,    /* Relative */
    ABS,    /* Absolute */ 
    ABX,    /* Absolute, X */
    ABY,    /* Absolute, X */
    IND,    /* Indirect */
    IZX,    /* Indexed indirect zero page, X */
    IZY,    /* Indexed indirect zero page, Y */
};

// Official instructions listed by type
enum instruction {
    /* Access */
    LDA, STA, LDX, STX, LDY, STY, 

    /* Transfer */
    TAX, TXA, TAY, TYA,

    /* Arithmetic */
    ADC, SBC, INC, DEC, INX, DEX, INY, DEY,

    /* Shift */
    ASL, LSR, ROL, ROR,

    /* Bitwise */
    AND, ORA, EOR, BIT,

    /* Compare */
    CMP, CPX, CPY,

    /* Branch */
    BCC, BCS, BEQ, BNE, BPL, BMI, BVC, BVS,

    /* Jump */
    JMP, JSR, RTS, BRK, RTI,

    /* Stack */
    PHA, PLA, PHP, PLP, TXS, TSX,

    /* Flags */
    CLC, SEC, CLI, SEI, CLD, SED, CLV,

    /* Others */
    NOP, XXX
};


#pragma endregion

#pragma region "dec"
#define A(mode) static void addressing##mode()
#define I(op) static void instruction##op()
#define OP(inst, addr, cycle) {                 \
	.instruction_func   = instruction##inst,    \
	.addressing_func    = addressing##addr,     \
	.instruction_code   = inst,                 \
	.addressing_code    = addr,                 \
	.cycles             = cycle                 \
}

/* addressing modes */
A(IMP);    /* Implied */
A(IMM);    /* Immediate */
A(ZP0);    /* Zero page */ 
A(ZPX);    /* Zero page, X */
A(ZPY);    /* Zero page, Y */
A(REL);    /* Relative */
A(ABS);    /* Absolute */ 
A(ABX);    /* Absolute, X */
A(ABY);    /* Absolute, X */
A(IND);    /* Indirect */
A(IZX);    /* Indexed indirect zero page, X */
A(IZY);    /* Indexed indirect zero page, Y */

 /* Access */ 
I(LDA); I(STA); I(LDX); I(STX); I(LDY); I(STY); 

 /* Transfer */ 
I(TAX); I(TXA); I(TAY); I(TYA);

/* Arithmetic */ 
I(ADC); I(SBC); I(INC); I(DEC); I(INX); I(DEX); I(INY); I(DEY);

/* Shift */
I(ASL); I(LSR); I(ROL); I(ROR);

/* Bitwise */ 
I(AND); I(ORA); I(EOR); I(BIT);

/* Compare */  
I(CMP); I(CPX); I(CPY);

/* Branch */
I(BCC); I(BCS); I(BEQ); I(BNE); I(BPL); I(BMI); I(BVC); I(BVS);

/* Jump */
I(JMP); I(JSR); I(RTS); I(BRK); I(RTI);

/* Stack */
I(PHA); I(PLA); I(PHP); I(PLP); I(TXS); I(TSX);

/* Flags */
I(CLC); I(SEC); I(CLI); I(SEI); I(CLD); I(SED); I(CLV);

/* Others */
I(NOP) { } I(XXX) { assert(false); }

struct operation {
    void (*addressing_func)(void);
    void (*instruction_func)(void);
    u64  instruction_code : 8;
    u64  addressing_code : 8;
    u64  cycles : 8;
    u64  __padding : 40;
};

struct operation __operations[] = { 
    OP( BRK, IMM, 7 ), OP( ORA, IZX, 6 ), OP( XXX, IMP, 2 ), OP( XXX, IMP, 8 ),
    OP( NOP, IMP, 3 ), OP( ORA, ZP0, 3 ), OP( ASL, ZP0, 5 ), OP( XXX, IMP, 5 ),
    OP( PHP, IMP, 3 ), OP( ORA, IMM, 2 ), OP( ASL, IMP, 2 ), OP( XXX, IMP, 2 ),
    OP( NOP, IMP, 4 ), OP( ORA, ABS, 4 ), OP( ASL, ABS, 6 ), OP( XXX, IMP, 6 ),
    OP( BPL, REL, 2 ), OP( ORA, IZY, 5 ), OP( XXX, IMP, 2 ), OP( XXX, IMP, 8 ),
    OP( NOP, IMP, 4 ), OP( ORA, ZPX, 4 ), OP( ASL, ZPX, 6 ), OP( XXX, IMP, 6 ),
    OP( CLC, IMP, 2 ), OP( ORA, ABY, 4 ), OP( NOP, IMP, 2 ), OP( XXX, IMP, 7 ),
    OP( NOP, IMP, 4 ), OP( ORA, ABX, 4 ), OP( ASL, ABX, 7 ), OP( XXX, IMP, 7 ),
    OP( JSR, ABS, 6 ), OP( AND, IZX, 6 ), OP( XXX, IMP, 2 ), OP( XXX, IMP, 8 ),
    OP( BIT, ZP0, 3 ), OP( AND, ZP0, 3 ), OP( ROL, ZP0, 5 ), OP( XXX, IMP, 5 ),
    OP( PLP, IMP, 4 ), OP( AND, IMM, 2 ), OP( ROL, IMP, 2 ), OP( XXX, IMP, 2 ),
    OP( BIT, ABS, 4 ), OP( AND, ABS, 4 ), OP( ROL, ABS, 6 ), OP( XXX, IMP, 6 ),
    OP( BMI, REL, 2 ), OP( AND, IZY, 5 ), OP( XXX, IMP, 2 ), OP( XXX, IMP, 8 ),
    OP( NOP, IMP, 4 ), OP( AND, ZPX, 4 ), OP( ROL, ZPX, 6 ), OP( XXX, IMP, 6 ),
    OP( SEC, IMP, 2 ), OP( AND, ABY, 4 ), OP( NOP, IMP, 2 ), OP( XXX, IMP, 7 ),
    OP( NOP, IMP, 4 ), OP( AND, ABX, 4 ), OP( ROL, ABX, 7 ), OP( XXX, IMP, 7 ),
    OP( RTI, IMP, 6 ), OP( EOR, IZX, 6 ), OP( XXX, IMP, 2 ), OP( XXX, IMP, 8 ),
    OP( NOP, IMP, 3 ), OP( EOR, ZP0, 3 ), OP( LSR, ZP0, 5 ), OP( XXX, IMP, 5 ),
    OP( PHA, IMP, 3 ), OP( EOR, IMM, 2 ), OP( LSR, IMP, 2 ), OP( XXX, IMP, 2 ),
    OP( JMP, ABS, 3 ), OP( EOR, ABS, 4 ), OP( LSR, ABS, 6 ), OP( XXX, IMP, 6 ),
    OP( BVC, REL, 2 ), OP( EOR, IZY, 5 ), OP( XXX, IMP, 2 ), OP( XXX, IMP, 8 ),
    OP( NOP, IMP, 4 ), OP( EOR, ZPX, 4 ), OP( LSR, ZPX, 6 ), OP( XXX, IMP, 6 ),
    OP( CLI, IMP, 2 ), OP( EOR, ABY, 4 ), OP( NOP, IMP, 2 ), OP( XXX, IMP, 7 ),
    OP( NOP, IMP, 4 ), OP( EOR, ABX, 4 ), OP( LSR, ABX, 7 ), OP( XXX, IMP, 7 ),
    OP( RTS, IMP, 6 ), OP( ADC, IZX, 6 ), OP( XXX, IMP, 2 ), OP( XXX, IMP, 8 ),
    OP( NOP, IMP, 3 ), OP( ADC, ZP0, 3 ), OP( ROR, ZP0, 5 ), OP( XXX, IMP, 5 ),
    OP( PLA, IMP, 4 ), OP( ADC, IMM, 2 ), OP( ROR, IMP, 2 ), OP( XXX, IMP, 2 ),
    OP( JMP, IND, 5 ), OP( ADC, ABS, 4 ), OP( ROR, ABS, 6 ), OP( XXX, IMP, 6 ),
    OP( BVS, REL, 2 ), OP( ADC, IZY, 5 ), OP( XXX, IMP, 2 ), OP( XXX, IMP, 8 ),
    OP( NOP, IMP, 4 ), OP( ADC, ZPX, 4 ), OP( ROR, ZPX, 6 ), OP( XXX, IMP, 6 ),
    OP( SEI, IMP, 2 ), OP( ADC, ABY, 4 ), OP( NOP, IMP, 2 ), OP( XXX, IMP, 7 ),
    OP( NOP, IMP, 4 ), OP( ADC, ABX, 4 ), OP( ROR, ABX, 7 ), OP( XXX, IMP, 7 ),
    OP( NOP, IMP, 2 ), OP( STA, IZX, 6 ), OP( NOP, IMP, 2 ), OP( XXX, IMP, 6 ),
    OP( STY, ZP0, 3 ), OP( STA, ZP0, 3 ), OP( STX, ZP0, 3 ), OP( XXX, IMP, 3 ),
    OP( DEY, IMP, 2 ), OP( NOP, IMP, 2 ), OP( TXA, IMP, 2 ), OP( XXX, IMP, 2 ),
    OP( STY, ABS, 4 ), OP( STA, ABS, 4 ), OP( STX, ABS, 4 ), OP( XXX, IMP, 4 ),
    OP( BCC, REL, 2 ), OP( STA, IZY, 6 ), OP( XXX, IMP, 2 ), OP( XXX, IMP, 6 ),
    OP( STY, ZPX, 4 ), OP( STA, ZPX, 4 ), OP( STX, ZPY, 4 ), OP( XXX, IMP, 4 ),
    OP( TYA, IMP, 2 ), OP( STA, ABY, 5 ), OP( TXS, IMP, 2 ), OP( XXX, IMP, 5 ),
    OP( NOP, IMP, 5 ), OP( STA, ABX, 5 ), OP( XXX, IMP, 5 ), OP( XXX, IMP, 5 ),
    OP( LDY, IMM, 2 ), OP( LDA, IZX, 6 ), OP( LDX, IMM, 2 ), OP( XXX, IMP, 6 ),
    OP( LDY, ZP0, 3 ), OP( LDA, ZP0, 3 ), OP( LDX, ZP0, 3 ), OP( XXX, IMP, 3 ),
    OP( TAY, IMP, 2 ), OP( LDA, IMM, 2 ), OP( TAX, IMP, 2 ), OP( XXX, IMP, 2 ),
    OP( LDY, ABS, 4 ), OP( LDA, ABS, 4 ), OP( LDX, ABS, 4 ), OP( XXX, IMP, 4 ),
    OP( BCS, REL, 2 ), OP( LDA, IZY, 5 ), OP( XXX, IMP, 2 ), OP( XXX, IMP, 5 ),
    OP( LDY, ZPX, 4 ), OP( LDA, ZPX, 4 ), OP( LDX, ZPY, 4 ), OP( XXX, IMP, 4 ),
    OP( CLV, IMP, 2 ), OP( LDA, ABY, 4 ), OP( TSX, IMP, 2 ), OP( XXX, IMP, 4 ),
    OP( LDY, ABX, 4 ), OP( LDA, ABX, 4 ), OP( LDX, ABY, 4 ), OP( XXX, IMP, 4 ),
    OP( CPY, IMM, 2 ), OP( CMP, IZX, 6 ), OP( NOP, IMP, 2 ), OP( XXX, IMP, 8 ),
    OP( CPY, ZP0, 3 ), OP( CMP, ZP0, 3 ), OP( DEC, ZP0, 5 ), OP( XXX, IMP, 5 ),
    OP( INY, IMP, 2 ), OP( CMP, IMM, 2 ), OP( DEX, IMP, 2 ), OP( XXX, IMP, 2 ),
    OP( CPY, ABS, 4 ), OP( CMP, ABS, 4 ), OP( DEC, ABS, 6 ), OP( XXX, IMP, 6 ),
    OP( BNE, REL, 2 ), OP( CMP, IZY, 5 ), OP( XXX, IMP, 2 ), OP( XXX, IMP, 8 ),
    OP( NOP, IMP, 4 ), OP( CMP, ZPX, 4 ), OP( DEC, ZPX, 6 ), OP( XXX, IMP, 6 ),
    OP( CLD, IMP, 2 ), OP( CMP, ABY, 4 ), OP( NOP, IMP, 2 ), OP( XXX, IMP, 7 ),
    OP( NOP, IMP, 4 ), OP( CMP, ABX, 4 ), OP( DEC, ABX, 7 ), OP( XXX, IMP, 7 ),
    OP( CPX, IMM, 2 ), OP( SBC, IZX, 6 ), OP( NOP, IMP, 2 ), OP( XXX, IMP, 8 ),
    OP( CPX, ZP0, 3 ), OP( SBC, ZP0, 3 ), OP( INC, ZP0, 5 ), OP( XXX, IMP, 5 ),
    OP( INX, IMP, 2 ), OP( SBC, IMM, 2 ), OP( NOP, IMP, 2 ), OP( SBC, IMP, 2 ),
    OP( CPX, ABS, 4 ), OP( SBC, ABS, 4 ), OP( INC, ABS, 6 ), OP( XXX, IMP, 6 ),
    OP( BEQ, REL, 2 ), OP( SBC, IZY, 5 ), OP( XXX, IMP, 2 ), OP( XXX, IMP, 8 ),
    OP( NOP, IMP, 4 ), OP( SBC, ZPX, 4 ), OP( INC, ZPX, 6 ), OP( XXX, IMP, 6 ),
    OP( SED, IMP, 2 ), OP( SBC, ABY, 4 ), OP( NOP, IMP, 2 ), OP( XXX, IMP, 7 ),
    OP( NOP, IMP, 4 ), OP( SBC, ABX, 4 ), OP( INC, ABX, 7 ), OP( XXX, IMP, 7 )
};

#pragma endregion

#pragma region "impl"
#pragma region "addressing"
A(IMP) { }
A(IMM) { __helper_addr = __pc++; }
A(ZP0) { __helper_addr = bus_read(__pc++); }
A(ZPX) { __helper_addr = bus_read(__pc++) + __x; }
A(ZPY) { __helper_addr = bus_read(__pc++) + __y; }
A(REL) {
    __helper_addr = bus_read(__pc++);
    if (__helper_addr & 0x80) {
        __helper_addr |= 0xFF00;
    }
}
A(ABS) {
    u16 lo = bus_read(__pc++);
    u16 hi = bus_read(__pc++);
    __helper_addr = (hi << 8) | lo;
}
A(ABX) {
    u16 lo = bus_read(__pc++);
    u16 hi = bus_read(__pc++);
    __helper_addr = __x + ((hi << 8) | lo);
}
A(ABY) {
    u16 lo = bus_read(__pc++);
    u16 hi = bus_read(__pc++);
    __helper_addr = __y + ((hi << 8) | lo);
}
A(IND) {
    u16 lo = bus_read(__pc++);
    u16 hi = bus_read(__pc++);
    u16 addr = ((hi << 8) | lo);
    lo = bus_read(addr);
    if ((addr & 0x00FF) == 0x00FF) {
        hi = bus_read(addr & 0xFF00);
    }
    else {
        hi = bus_read(addr + 1);
    }
    __helper_addr = ((hi << 8) | lo);
}
A(IZX) {
    u16 off = bus_read(__pc++);
    u16 lo = bus_read((off + __x) & 0x00FF);
    u16 hi = bus_read((off + __x + 1) & 0x00FF);
    __helper_addr = ((hi << 8) | lo);
} 
A(IZY) {
    u16 off = bus_read(__pc++);
    u16 lo = bus_read((off) & 0x00FF);
    u16 hi = bus_read((off + 1) & 0x00FF);
    __helper_addr = ((hi << 8) | lo) + __y;
}
#pragma endregion 

#pragma region "intruction"
#pragma region "Access"
I(LDA) {
    __a = fetch_operand();
    SetFlag(z, __a == 0x00);
    SetFlag(n, __a & 0x80);

}
I(STA) {
    store_operand(__a);
}
I(LDX) {
    __x = fetch_operand();
    SetFlag(z, __x == 0);
    SetFlag(n, __x & 0x80);
}
I(STX) {
    store_operand(__x);
}
I(LDY)
{
    __y = fetch_operand();
    SetFlag(z, __y == 0);
    SetFlag(n, __y & 0x80);
}
I(STY) {
    store_operand(__y);
}
#pragma endregion

#pragma region "Transfer"
I(TAX) {
    __x = __a;
    SetFlag(z, __x == 0x00);
    SetFlag(n, __x & 0x80);
} 
I(TXA) {
    __a = __x;
    SetFlag(z, __a == 0);
    SetFlag(n, __a & 0x80);
}
I(TAY) {
    __y = __a;
    SetFlag(z, __y == 0);
    SetFlag(n, __y & 0x80);
}
I(TYA) {
    __a = __y;
    SetFlag(z, __a == 0);
    SetFlag(n, __a & 0x80);
}
#pragma endregion
#pragma region "Arithmetic"
I(ADC) {
    u8 operand = fetch_operand();
    u8 temp = __a + operand + GetFlag(c);
    SetFlag(c, temp < __a || temp < operand);
    SetFlag(z, temp == 0x00);
    SetFlag(v, 0x80 & (temp ^ __a) & (temp ^ operand));
    SetFlag(n, temp & 0x80);
    __a = temp;
}
I(SBC) {
    u8 operand = fetch_operand();
    operand = ~operand;
    u8 temp = __a + operand + GetFlag(c);
    SetFlag(c, temp < __a || temp < operand);
    SetFlag(z, temp == 0x00);
    SetFlag(v, 0x80 & (temp ^ __a) & (temp ^ operand));
    SetFlag(n, temp & 0x80);
    __a = temp;
} 
I(INC) {
    u8 operand = fetch_operand();
    store_operand(++operand);
    SetFlag(z, operand == 0x00);
    SetFlag(n, operand & 0x80);
}
I(DEC) {
    u8 operand = fetch_operand();
    store_operand(--operand);
    SetFlag(z, operand == 0x00);
    SetFlag(n, operand & 0x80);
}
I(INX) {
    ++__x;
    SetFlag(z, __x == 0x00);
    SetFlag(n, __x & 0x80);
}
I(DEX) {
	--__x;
	SetFlag(z, __x == 0x00);
	SetFlag(n, __x & 0x80);
}
I(INY) {
	++__y;
	SetFlag(z, __y == 0x00);
	SetFlag(n, __y & 0x80);
}
I(DEY) {
	--__y;
	SetFlag(z, __y == 0x00);
	SetFlag(n, __y & 0x80);
}

#pragma endregion

#pragma region "Shift"
I(ASL) {
    u8 operand = fetch_operand();
    SetFlag(c, operand & 0x80);
    operand <<= 1;
    SetFlag(z, operand == 0x00);
	SetFlag(n, operand & 0x80);
    store_operand(operand);
}
I(LSR) {
    u8 operand = fetch_operand();
    SetFlag(c, operand & 0x01);
    operand >>= 1;
    SetFlag(z, operand == 0x00);
	SetFlag(n, operand & 0x80);
    store_operand(operand);
} 
I(ROL) {
    u8 operand = fetch_operand();
    u8 c = GetFlag(c);
    SetFlag(c, operand & 0x80);
    operand = (operand << 1) | (c & 0x01);
    SetFlag(z, operand == 0x00);
	SetFlag(n, operand & 0x80);
    store_operand(operand);
} 
I(ROR) {
    u8 operand = fetch_operand();
    u8 c = GetFlag(c);
    SetFlag(c, operand & 0x01);
    operand = (operand >> 1) | (c << 7);
    SetFlag(z, operand == 0x00);
	SetFlag(n, operand & 0x80);
    store_operand(operand);
}
#pragma endregion
#pragma region "Bitwise"
I(AND) {
    u8 operand = fetch_operand();
    __a = __a & operand;
    SetFlag(z, __a == 0x00);
    SetFlag(n, __a & 0x80);
}
I(ORA) {
    u8 operand = fetch_operand();
    __a = __a | operand;
    SetFlag(z, __a == 0x00);
    SetFlag(n, __a & 0x80);
} 
I(EOR) {
    u8 operand = fetch_operand();
    __a = __a ^ operand;
    SetFlag(z, __a == 0x00);
    SetFlag(n, __a & 0x80);
} 
I(BIT) {
    u8 operand = fetch_operand();
    SetFlag(z, (__a & operand) == 0x00);
    SetFlag(n, operand & 0x80);
    SetFlag(v, operand & 0x40);
}
#pragma endregion
#pragma region "Compare"
I(CMP) {
    u8 operand = fetch_operand();
    SetFlag(c, __a >= operand);
    SetFlag(z, __a == operand);
    SetFlag(n, 0x80 & (__a - operand));
} 
I(CPX) {
    u8 operand = fetch_operand();
    SetFlag(c, __x >= operand);
    SetFlag(z, __x == operand);
    SetFlag(n, 0x80 & (__x - operand));
}
I(CPY) {
    u8 operand = fetch_operand();
    SetFlag(c, __y >= operand);
    SetFlag(z, __y == operand);
    SetFlag(n, 0x80 & (__y - operand));
}
#pragma endregion
#pragma region "Branch"
/* Branch */
I(BCC) { if (GetFlag(c) == 0) __pc += __helper_addr; } 
I(BCS) { if (GetFlag(c) == 1) __pc += __helper_addr; }  
I(BEQ) { if ((GetFlag(z)) == 1) __pc += __helper_addr; }
I(BNE) { if ((GetFlag(z)) == 0) __pc += __helper_addr; }
I(BPL) { if ((GetFlag(n)) == 0) __pc += __helper_addr; }
I(BMI) { if ((GetFlag(n)) == 1) __pc += __helper_addr; }
I(BVC) { if ((GetFlag(v)) == 0) __pc += __helper_addr; }
I(BVS) { if ((GetFlag(v)) == 1) __pc += __helper_addr; }
#pragma endregion

#pragma region "Jump"
/* Jump */
I(JMP) {
    __pc = __helper_addr;
}
I(JSR) {
    --__pc;
    stack_push((__pc & 0xFF00) >> 8);
    stack_push((__pc & 0x00FF));
    __pc = __helper_addr;
} 
I(RTS) {
    __pc = stack_pop();
    __pc |= (stack_pop() << 8);
    ++__pc;
} 
I(BRK) {
    ++__pc;
    SetFlag(i, 1);
    stack_push((__pc & 0xFF00) >> 8);
    stack_push((__pc & 0x00FF));

    SetFlag(b, 1);
    stack_push(__p.reg);
    SetFlag(b, 0);
    __pc = break_into();
} 
I(RTI) {
    __p.reg = stack_pop();
    SetFlag(b, 0);
    SetFlag(u, 0);
    __pc = stack_pop();
    __pc | (stack_pop() << 8);
}
#pragma endregion

#pragma region "Stack"
I(PHA) {
    stack_push(__a);
}
I(PLA) {
    __a = stack_pop();
    SetFlag(z, __a == 0x00);
    SetFlag(n, __a & 0x80);
}
I(PHP) {
    status_register p = __p; 
    p.b = 1; p.u = 1;
    stack_push(p.reg);
} 
I(PLP) {
    __p.reg = stack_pop();
    SetFlag(u, 1);
} 
I(TXS) {
    __sp = __x;
} 
I(TSX) {
    __x = __sp;
    SetFlag(z, __x == 0x00);
    SetFlag(n, __x & 0x80);
};
#pragma endregion

#pragma region "Flags"
I(CLC) { SetFlag(c, 0); } 
I(SEC) { SetFlag(c, 1); } 
I(CLI) { SetFlag(i, 0); } 
I(SEI) { SetFlag(i, 1); }
I(CLD) { SetFlag(d, 0); }
I(SED) { SetFlag(d, 1); }
I(CLV) { SetFlag(v, 0); }
#pragma endregion

#pragma endregion
#pragma endregion

bool cpu_clock() 
{
    static u32 remain_cycles = 8;
    if (remain_cycles > 0) {
        goto CLK;
    }

    {
        u8 opcode = bus_read(__pc++);
        remain_cycles = __operations[opcode].cycles;
        __helper_implied_flag = __operations[opcode].addressing_code == IMP;
        __operations[opcode].addressing_func();
        __operations[opcode].instruction_func();
    }
CLK:
    --remain_cycles;
    return remain_cycles == 0;
}


void cpu_reset() 
{
/*
Address  Hexdump   Dissassembly
-------------------------------
$0600    a2 00     LDX #$00
$0602    a0 00     LDY #$00
$0604    e8        INX 
$0605    98        TYA 
$0606    49 1f     EOR #$1f
$0608    a8        TAY 
$0609    8a        TXA 
$060a    6d ff ff  ADC $ffff
$060d    aa        TAX 
$060e    88        DEY 
$060f    ea        NOP 
$0610    ea        NOP 
$0611    4c 04 06  JMP $0604
*/
    __pc = 0x0600;
    u8 rom[] = {
        0xa2, 0x00,
        0xa0, 0x00, 
        0xe8, 
        0x98, 
        0x49, 0x1f, 
        0xa8, 
        0x8a, 
        0x6d, 0xff, 0xff, 
        0xaa, 
        0x88, 
        0xea,
        0xea, 
        0x4c, 0x04, 0x06 
    };
    memcpy(&ram[0x600], rom, sizeof(rom));
}

u16 cpu_x_y() {
    return (__x << 8) | (int)__y; 
}
