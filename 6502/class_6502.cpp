#include "./class_6502.h"

// #define LOGING

C6502::C6502(){
    throw std::runtime_error("cannot create 6502 emulator without memory map");
}

C6502::C6502(const char* romMap){
    def();
    make(romMap);
    
    PC.H = rom[0x7F][0xFD];
    PC.L = rom[0x7F][0xFC];

    totalOps = 0;

    #ifdef LOGING
    log.open("log.txt");
    #endif
}

C6502::~C6502(){
    std::cout << std::hex;
    for(int i = 0; i < 3; i++){
        for(int j = 0; j < 256; j++){
            std::cout << (int)ram[i][j] << " ";
        }
        std::cout << "\n";
    }
    std::cout << (int) P.getChar() << std::dec << " " << totalOps << "\n";
}

unsigned char* C6502::getIn(){
    dataIn = 0xFF;
    return &dataIn;
}

unsigned char* C6502::getOut(){
    dataOut = 0xFF;
    return &dataOut;
}

bool* C6502::getDataInSig(){
    return &dataInSig;
}

void C6502::def(){
    for(int i = 0; i < 128; i++){
        for(int j = 0; j < 256; j++){
            ram[i][j] = 0;
            rom[i][j] = 0;
        }
    }

    A = 0;
    X = 0;
    Y = 0;
    PC.H = 0XFF;
    PC.L = 0XFC;
    P.setChar(0);
    S = 0;
    dataIn = 0;
    dataOut = 0;
    dataInSig = false;
}

bool C6502::loopup(std::ifstream& file, char* word){
    int i = 0;
    char data;
    static bool ready = true;
    if(!ready) return false;
    while(file.get(data)){
        switch(data){
            case '\t':
            case '\n':
                word[i] = 0;
                return true;
            case 0:
                ready = false;
                word[i] = 0;
                return true;
            default:
                word[i] = data;
                break;
        }
        i++;
    }
    return false;
}

instruction C6502::makeInstruction(char* fromGrid){

    // 3 digit str into 15 bits (short)
    instruction output;
    output.command = fromGrid[0]-'A';
    output.command <<= 5;
    output.command += fromGrid[1]-'A';
    output.command <<= 5;
    output.command += fromGrid[2]-'A';

    // get mode from command (data in the str past 3 chars)
    output.addrMode = getMode(fromGrid);

    // return
    return output;
}

void C6502::make(const char* name){

    // loads boot file (OS etc)
    std::ifstream romData(name);
    int little = 0;
    int big = 0;
    char data;
    while(big < 128 && romData.get(data)){
        rom[big][little] = data;
        little++;
        if(little > 255){
            little = 0;
            big++;
        }
    }


    // loads data for the commands
    std::ifstream commandGrid("grid.txt");
    char currWord[32];
    int i = 0;
    while(loopup(commandGrid, currWord)){
        command[i] = makeInstruction(currWord);
        i++;
    }
    
}

RWPackage C6502::getData(unsigned char north, unsigned char south){
    RWPackage output;
    if(north == 0x50){
        switch(south){
            case 0://data
                bucket = dataIn;
                output.read = &bucket;
                output.write = &dataOut;
                #ifdef LOGING
                log << "i/o buffer: In: " << (int) dataIn << " Out: " << (int) dataOut << "\n";
                #endif
                break;
            case 1://status
                bucket = 0;
                if(dataInSig){
                    dataInSig = false;
                    bucket = 0x08;
                }
                output.read = &bucket;
                output.write = output.read;
                break;
            case 2://cmd
                output.read = &ACIA_cmd;
                output.write = output.read;
                break;
            case 3://ctrl
                output.read = &ACIA_ctrl;
                output.write = output.read;
                break;
            default:
                output.read = &ram[north][south];
                output.write = output.read;
                break;
        }
        return output;
    } else if(north & 0x80){
        //rom
        unsigned char tempNorth = north & 0x7F;
        output.read = &rom[tempNorth][south];
        output.write = &bucket;
        return output;
    } else {
        //ram
        output.read = &ram[north][south];
        output.write = output.read;
        return output;
    }
}

addressingMode C6502::getMode(char* command){
    // catch a str with no addressing more
    if(command[3] == 0) return _NULL;
    
    // determin the addressing mode from the str
    switch(command[4]){
        case 'A':
            return Acc;
        case 'X':
            return Xind;
        case 'r':
            return rel;
        case '#':
            return immediate;
        case 'a':
            //abs,/X,/Y
            if(command[7] == 0) return abso;
            else {
                if(command[8] == 'X') return absX;
                else return absY;
            }
        case 'i':
            //impl,ind,/Y
            if(command[7] == 0) return ind;
            else if(command[7] == ',') return indY;
            else return impl;
        case 'z':
            //zpg./X./Y
            if(command[7] == 0) return zpg;
            else {
                if(command[8] == 'X') return zpgX;
                else return zpgY;
            }
        default:
            std::string output = "addressing mode not recognised ";
            output += command;
            throw std::runtime_error(output);
    }
}

RWPackage C6502::addressData(addressingMode mode){
    // get data based on addressing mode

    // get some memory to get the data with (ind etc)
    Word W;
    Word W_;

    // make rw package
    RWPackage output;

    switch (mode){
        case Acc:
            output.read = &A;
            output.write = &A;
            break;
        case abso:
            W.L = *getData(PC.H, PC.L).read;
            incPC();
            W.H = *getData(PC.H, PC.L).read;
            incPC();
            output = getData(W.H, W.L);
            break;
        case absX:
            W.L = *getData(PC.H, PC.L).read;
            incPC();
            W.H = *getData(PC.H, PC.L).read;
            incPC();
            W.Word += X;
            output = getData(W.H, W.L);
            break;
        case absY:
            W.L = *getData(PC.H, PC.L).read;
            incPC();
            W.H = *getData(PC.H, PC.L).read;
            incPC();
            W.Word += Y;
            output = getData(W.H, W.L);
            break;
        case rel:
        case immediate:
            output = getData(PC.H, PC.L);
            incPC();
            break;
        case ind:
            throw std::runtime_error("so indirect???");
            W_.L = *getData(PC.H, PC.L).read;
            incPC();
            W_.H = *getData(PC.H, PC.L).read;
            incPC();
            W.L = *getData(W_.H, W_.L).read;
            W_.Word++;
            W.H = *getData(W_.H, W_.L).read;
            output = getData(W.H,W.L);
            break;
        case Xind:
            W_.L = *getData(PC.H, PC.L).read;
            incPC();
            W_.L += X;
            W.L = *getData(0, W_.L).read;
            W_.L++;
            W.H = *getData(0, W_.L).read;
            output = getData(W.H,W.L);
            break;
        case indY:
            W_.L = *getData(PC.H, PC.L).read;
            incPC();
            W_.L = *getData(PC.H, PC.L).read;
            incPC();
            W_.Word += Y;
            W.L = *getData(W_.H, W_.L).read;
            W_.Word++;
            W.H = *getData(W_.H, W_.L).read;
            output = getData(W.H,W.L);
            break;
        case zpg:
            W.L = *getData(PC.H, PC.L).read;
            incPC();
            output = getData(0, W.L); 
            break;
        case zpgX:
            W.L = *getData(PC.H, PC.L).read;
            incPC();
            W.L += X;
            output = getData(0, W.L); 
            break;
        case zpgY:
            W.L = *getData(PC.H, PC.L).read;
            incPC();
            W.L += Y;
            output = getData(0, W.L); 
            break;
        default:
            throw std::runtime_error("invalid addressing mode");
    }

    return output;
}

void C6502::incPC(){
    PC.Word++;
}

void C6502::byteCheck(unsigned char c){
    P.Z = (c == 0) ? 1 : 0;
    P.N = (c & 0x80) ? 1 : 0;
}

void C6502::PushStack(unsigned char data){
    ram[1][S] = data;
    S--;
}
unsigned char C6502::PullStack(){
    S++;
    return ram[1][S];
}

void C6502::ADC(addressingMode mode){
    unsigned short M = *addressData(mode).read;
    unsigned short A_ = A + M + P.C;
    P.C = (A_ > 0xFF) ? true : false;
    // P.V = (((A_ & 0x80) && !(A & 0x80) && !(M & 0x80)) || (!(A_ & 0x80) && (A & 0x80) && (M & 0x80))) ? true : false;
    A = A_;
    byteCheck(A);
}

void C6502::AND(addressingMode mode){
    unsigned char M = *addressData(mode).read;
    A &= M;
    byteCheck(A);
}

void C6502::ASL(addressingMode mode){
    RWPackage Mp = addressData(mode);
    unsigned char M = *Mp.read;
    P.C = (M & 0x80) ? 1 : 0;
    M <<= 1;
    *Mp.write = M;
};

void C6502::branch(unsigned char offset){
    if(offset & 0x80){
        offset = ~offset + 1;
        PC.Word -= offset;
    } else {
        PC.Word += offset;
    }
}

void C6502::BCC(){
    unsigned char M = *addressData(rel).read;
    if(P.C == 0) branch(M);
}
void C6502::BCS(){
    unsigned char M = *addressData(rel).read;
    if(P.C == 1) branch(M);
}
void C6502::BEQ(){
    unsigned char M = *addressData(rel).read;
    if(P.Z == 1) branch(M);
}

void C6502::BIT(addressingMode mode){
    unsigned char M = *addressData(mode).read;
    P.N = (M & 0x80) ? 1 : 0;
    P.V = (M & 0x40) ? 1 : 0;
    M &= A;
    P.Z = (M) ? 1 : 0;
}

void C6502::BMI(){
    unsigned char M = *addressData(rel).read;
    if(P.N == 1) branch(M);
}
void C6502::BNE(){
    unsigned char M = *addressData(rel).read;
    if(P.Z == 0) branch(M);
}
void C6502::BPL(){
    unsigned char M = *addressData(rel).read;
    if(P.N == 0) branch(M);
}
void C6502::BRK(){
    std::cout << "you are broken bby: " << (int) PC.Word << "\n";
    this->~C6502();
    exit(1);
    
    incPC();
    incPC();
    interupt();
    P.I = 1;
}

void C6502::interupt(){
    PushStack(PC.H);
    PushStack(PC.L);
    PushStack(P.getChar());
    PC.L = rom[0x7F][0xFE];
    PC.H = rom[0x7F][0xFF];
}

void C6502::returnFromInterupt(){
    P.setChar(PullStack());
    PC.L = PullStack();
    PC.H = PullStack();
}

void C6502::BVC(){
    unsigned char M = *addressData(rel).read;
    if(P.V == 0) branch(M);
}
void C6502::BVS(){
    unsigned char M = *addressData(rel).read;
    if(P.V == 1) branch(M);
}

void C6502::CLC(){
    P.C = 0;
}
void C6502::CLD(){
    P.D = 0;
}
void C6502::CLI(){
    P.I = 0;
}
void C6502::CLV(){
    P.V = 0;
}

void C6502::CMP(addressingMode mode){
    unsigned char M = *addressData(mode).read;
    if(A < M){
        P.Z = 0;
        P.C = 0;
        P.N = ((A-M) & 0x80) ? 1 : 0;
    } else if(M == A){
        P.Z = 1;
        P.C = 1;
        P.N = 0;
    } else {
        P.Z = 0;
        P.C = 1;
        P.N = ((A-M) & 0x80) ? 1 : 0;
    }
}
void C6502::CPX(addressingMode mode){
    unsigned char M = *addressData(mode).read;
    if(M < X){
        P.Z = 0;
        P.C = 0;
        P.N = ((X-M) & 0x80) ? 1 : 0;
    } else if(M == X){
        P.Z = 1;
        P.C = 1;
        P.N = 0;
    } else {
        P.Z = 0;
        P.C = 1;
        P.N = ((X-M) & 0x80) ? 1 : 0;
    }
}
void C6502::CPY(addressingMode mode){
    unsigned char M = *addressData(mode).read;
    if(M < Y){
        P.Z = 0;
        P.C = 0;
        P.N = ((Y-M) & 0x80) ? 1 : 0;
    } else if(M == Y){
        P.Z = 1;
        P.C = 1;
        P.N = 0;
    } else {
        P.Z = 0;
        P.C = 1;
        P.N = ((Y-M) & 0x80) ? 1 : 0;
    }
}

void C6502::DEC(addressingMode mode){
    RWPackage Mp = addressData(mode);
    unsigned char M = *Mp.read;
    M--;
    *Mp.write = M;
    byteCheck(M);
}

void C6502::DEX(){
    X--;
    byteCheck(X);
}
void C6502::DEY(){
    Y--;
    byteCheck(Y);
}

void C6502::EOR(addressingMode mode){
    unsigned char M = *addressData(mode).read;
    A ^= M;
    byteCheck(A);
}

void C6502::INC(addressingMode mode){
    RWPackage Mp = addressData(mode);
    unsigned char M = *Mp.read;
    M++;
    *Mp.write = M;
    byteCheck(M);
}

void C6502::INX(){
    X++;
    byteCheck(X);
}
void C6502::INY(){
    Y++;
    byteCheck(Y);
}

void C6502::JMP(addressingMode mode){
    Word W;
    if(mode == ind){
        W.L = *getData(PC.H, PC.L).read;
        incPC();
        W.H = *getData(PC.H, PC.L).read;
        PC.L = *getData(W.H, W.L).read;
        W.Word++;
        PC.H = *getData(W.H, W.L).read;
    } else {
        W.L = *getData(PC.H, PC.L).read;
        incPC();
        W.H = *getData(PC.H, PC.L).read;
        PC = W;
    }
}

void C6502::JSR(){
    Word W;
    W.L = *getData(PC.H, PC.L).read;
    incPC();
    W.H = *getData(PC.H, PC.L).read;
    incPC();
    PushStack(PC.H);
    PushStack(PC.L);
    PC = W;
}

void C6502::LDA(addressingMode mode){
    A = *addressData(mode).read;
    byteCheck(A);
}
void C6502::LDX(addressingMode mode){
    X = *addressData(mode).read;
    byteCheck(X);
}
void C6502::LDY(addressingMode mode){
    Y = *addressData(mode).read;
    byteCheck(Y);
}

void C6502::LSR(addressingMode mode){
    RWPackage Mp = addressData(mode);
    unsigned char M = *Mp.read;
    P.C = (M & 0x01) ? 1 : 0;
    M = M >> 1;
    *Mp.write = M;
    byteCheck(M);
}

void C6502::NOP(){
    //there was nothing there
}

void C6502::ORA(addressingMode mode){
    unsigned char M = *addressData(mode).read;
    A |= M;
    byteCheck(A);
}

void C6502::PHA(){
    PushStack(A);
}
void C6502::PHP(){
    PushStack(S);
}
void C6502::PLA(){
    A = PullStack();
    byteCheck(A);
}
void C6502::PLP(){
    S = PullStack();
}

void C6502::ROL(addressingMode mode){
    RWPackage Mp = addressData(mode);
    unsigned char M = *Mp.read;
    unsigned char TC = P.C;
    P.C = (M & 0x80) ? 1 : 0;
    M <<= 1;
    M += TC;
    *Mp.write = M;
    byteCheck(M);
}

void C6502::ROR(addressingMode mode){
    RWPackage Mp = addressData(mode);
    unsigned char M = *Mp.read;
    unsigned char TC = P.C;
    TC <<= 7;
    P.C = (M & 0x01) ? 1 : 0;
    M >>= 1;
    M += TC;
    *Mp.write = M;
    byteCheck(M);
}

void C6502::RTI(){
    returnFromInterupt();
}
void C6502::RTS(){
    PC.L = PullStack();
    PC.H = PullStack();
}

void C6502::SBC(addressingMode mode){
    unsigned char M = *addressData(mode).read;
    unsigned short A_ = A + ~M + P.C;
    // std::cout << "  A: " << (int) A << " M: " << (int) M << " ~M: " << (int) ~M << " A_: " << (int) A_ << " P.C: " << P.C << "\n";
    P.C = (A_ > 0xFF) ? false : true;
    A = A_;
    byteCheck(A);
}

void C6502::SEC(){
    P.C = 1;
}
void C6502::SED(){
    P.D = 1;
}
void C6502::SEI(){
    P.I = 1;
}
void C6502::STA(addressingMode mode){
    *addressData(mode).write = A;
}
void C6502::STX(addressingMode mode){
    *addressData(mode).write = X;
}
void C6502::STY(addressingMode mode){
    *addressData(mode).write = Y;
}
void C6502::TAX(){
    X = A;
    byteCheck(X);
}
void C6502::TAY(){
    Y = A;
    byteCheck(Y);
}
void C6502::TSX(){
    X = S;
    byteCheck(X);
}
void C6502::TXA(){
    A = X;
    byteCheck(A);
}
void C6502::TXS(){
    S = X;
}
void C6502::TYA(){
    A = Y;
    byteCheck(A);
}

void C6502::tick(){
    #ifdef LOGING
    log << "PC: " << std::hex << PC.Word << std::dec << " (" << (int) PC.H << ":" << (int) PC.L << ")\n";
    #endif
    unsigned char rawCommand = *getData(PC.H,PC.L).read;
    incPC();
    instruction commandWord = command[rawCommand];

    // log
    #ifdef LOGING
    log << "(" << commandWord.command << ":" << commandWord.addrMode << ")\n";
    #endif

    // does the encoded command
    switch(commandWord.command){
        case 98://ADC
            ADC(commandWord.addrMode);
            break;
        case 419://AND
            AND(commandWord.addrMode);
            break;
        case 587://ASL
            ASL(commandWord.addrMode);
            break;
        case 1090://BCC
            BCC();
            break;
        case 1106://BCS
            BCS();
            break;
        case 1168://BEQ
            BEQ();
            break;
        case 1299://BIT
            BIT(commandWord.addrMode);
            break;
        case 1416://BMI
            BMI();
            break;
        case 1444://BNE
            BNE();
            break;
        case 1515://BPL
            BPL();
            break;
        case 1578://BRK
            BRK();
            break;
        case 1698://BVC
            BVC();
            break;
        case 1714://BVS
            BVS();
            break;
        case 2402://CLC
            CLC();
            break;
        case 2403://CLD
            CLD();
            break;
        case 2408://CLI
            CLI();
            break;
        case 2421://CLV
            CLV();
            break;
        case 2447://CMP
            CMP(commandWord.addrMode);
            break;
        case 2551://CPX
            CPX(commandWord.addrMode);
            break;
        case 2552://CPY
            CPY(commandWord.addrMode);
            break;
        case 3202://DEC
            DEC(commandWord.addrMode);
            break;
        case 3223://DEX
            DEX();
            break;
        case 3224://DEY
            DEY();
            break;
        case 4561://EOR
            EOR(commandWord.addrMode);
            break;
        case 8610://INC
            INC(commandWord.addrMode);
            break;
        case 8631://INX
            INX();
            break;
        case 8632://INY
            INY();
            break;
        case 9615://JMP
            JMP(commandWord.addrMode);
            break;
        case 9809://JSR
            JSR();
            break;
        case 11360://LDA
            LDA(commandWord.addrMode);
            break;
        case 11383://LDX
            LDX(commandWord.addrMode);
            break;
        case 11384://LDY
            LDY(commandWord.addrMode);
            break;
        case 11857://LSR
            LSR(commandWord.addrMode);
            break;
        case 13775://NOP
            NOP();
            break;
        case 14880://ORA
            ORA(commandWord.addrMode);
            break;
        case 15584://PHA
            PHA();
            break;
        case 15599://PHP
            PHP();
            break;
        case 15712://PLA
            PLA();
            break;
        case 15727://PLP
            PLP();
            break;
        case 17867://ROL
            ROL(commandWord.addrMode);
            break;
        case 17873://ROR
            ROR(commandWord.addrMode);
            break;
        case 18024://RTI
            RTI();
            break;
        case 18034://RTS
            RTS();
            break;
        case 18466://SBC
            SBC(commandWord.addrMode);
            break;
        case 18562://SEC
            SEC();
            break;
        case 18563://SED
            SED();
            break;
        case 18568://SEI
            SEI();
            break;
        case 19040://STA
            STA(commandWord.addrMode);
            break;
        case 19063://STX
            STX(commandWord.addrMode);
            break;
        case 19064://STY
            STY(commandWord.addrMode);
            break;
        case 19479://TAX
            TAX();
            break;
        case 19480://TAY
            TAY();
            break;
        case 20055://TSX
            TSX();
            break;
        case 20192://TXA
            TXA();
            break;
        case 20210://TXS
            TXS();
            break;
        case 20224://TYA
            TYA();
            break;
        default:
            std::string output = "command not recognised: ";
            output += commandWord.command;
            output += " : ";
            output += commandWord.addrMode;
            output += " in line: ";
            output += std::to_string(PC.Word-1);
            throw std::runtime_error(output);
    }

    // std::this_thread::sleep_for(std::chrono::milliseconds(1));
}

void C6502::run(bool* go){

    // loop till go says stop
    while(*go){
        totalOps++;
        tick();
    }

    // for(int i = 0; i < 0xFFFF; i++){
    //     tick();
    // }
}

