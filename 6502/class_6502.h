#ifndef CLASS_6502_H
#define CLASS_6502_H

// #define LOGING

#include <fstream>
#include <iostream>
#include <cstring>
#include <thread>

//https://www.masswerk.at/6502/6502_instruction_set.html

typedef enum{
    _NULL,
    Acc,
    abso,
    absX,
    absY,
    immediate,
    impl,
    ind,
    Xind,
    indY,
    rel,
    zpg,
    zpgX,
    zpgY,
} addressingMode;

typedef struct{
    unsigned short command;
    addressingMode addrMode;
} instruction;


typedef struct{
    bool C;
    bool Z;
    bool I;
    bool D;
    bool B;
    bool IGNORE_BIT;
    bool V;
    bool N;

    unsigned char getChar(){
        unsigned char output = (N)?1:0;
        output <<= 1;
        output += (V)?1:0;
        output <<= 1;
        output += (IGNORE_BIT)?1:0;
        output <<= 1;
        output += (B)?1:0;
        output <<= 1;
        output += (D)?1:0;
        output <<= 1;
        output += (I)?1:0;
        output <<= 1;
        output += (Z)?1:0;
        output <<= 1;
        output += (C)?1:0;
        return output;
    }
    void setChar(unsigned char data){
        N = (data & 0x80) ? 1 : 0;
        V = (data & 0x40) ? 1 : 0;
        IGNORE_BIT = (data & 0x20) ? 1 : 0;
        B = (data & 0x10) ? 1 : 0;
        D = (data & 0x08) ? 1 : 0;
        I = (data & 0x04) ? 1 : 0;
        Z = (data & 0x02) ? 1 : 0;
        C = (data & 0x01) ? 1 : 0;
    }
}flags;

typedef union{
    struct{
        unsigned char L;
        unsigned char H;
    };
    unsigned short Word;
} Word;

typedef struct{
    unsigned char* read;
    unsigned char* write;
} RWPackage;

class C6502{
    private:
    unsigned char X, Y, A, S, bucket;
    Word PC;
    flags P;
    unsigned char ram[128][256];
    unsigned char rom[128][256];
    instruction command[256];
    std::ofstream log;

    //linked values
    unsigned char ACIA_cmd;
    unsigned char ACIA_ctrl;
    unsigned char dataIn;
    unsigned char dataOut;
    bool dataInSig;
    long long totalOps;

    public:
    C6502();
    C6502(const char* romMap);
    ~C6502();
    unsigned char* getIn();
    unsigned char* getOut();
    bool* getDataInSig();

    private:
    void incPC();
    void def();
    void make(const char* romMap);
    bool loopup(std::ifstream& file, char* word);
    instruction makeInstruction(char* fromGrid);
    RWPackage getData(unsigned char north, unsigned char south);
    addressingMode getMode(char* command);
    void byteCheck(unsigned char c);
    void PushStack(unsigned char data);
    unsigned char PullStack();
    void interupt();
    void returnFromInterupt();

    RWPackage addressData(addressingMode);
    void branch(unsigned char);

    void ADC(addressingMode);
    void AND(addressingMode);
    void ASL(addressingMode);
    void BCC();
    void BCS();
    void BEQ();
    void BIT(addressingMode);
    void BMI();
    void BNE();
    void BPL();
    void BRK();
    void BVC();
    void BVS();
    void CLC();
    void CLD();
    void CLI();
    void CLV();
    void CMP(addressingMode);
    void CPX(addressingMode);
    void CPY(addressingMode);
    void DEC(addressingMode);
    void DEX();
    void DEY();
    void EOR(addressingMode);
    void INC(addressingMode);
    void INX();
    void INY();
    void JMP(addressingMode);
    void JSR();
    void LDA(addressingMode);
    void LDX(addressingMode);
    void LDY(addressingMode);
    void LSR(addressingMode);
    void NOP();
    void ORA(addressingMode);
    void PHA();
    void PHP();
    void PLA();
    void PLP();
    void ROL(addressingMode);
    void ROR(addressingMode);
    void RTI();
    void RTS();
    void SBC(addressingMode);
    void SEC();
    void SED();
    void SEI();
    void STA(addressingMode);
    void STX(addressingMode);
    void STY(addressingMode);
    void TAX();
    void TAY();
    void TSX();
    void TXA();
    void TXS();
    void TYA();

    public:
    void tick();
    void run(bool*);
};

#endif