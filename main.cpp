#include "./6502/class_6502.h"

#include <iostream>
#include <cstdlib>
#include <string>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>

void outputFromCpu(unsigned char* dataRecv, bool* loop){
    //initilaise output as unbuffered
    std::cout.setf(std::ios::unitbuf);

    //get outputs
    char outputBuffer;
    while(*loop){
        if(*dataRecv != 0xFF){
            outputBuffer = (char) *dataRecv;
            *dataRecv = 0xFF;
            //translate CR to newline
            if(outputBuffer == 0x0D) std::cout << '\n';
            else std::cout << outputBuffer;
        }
    }
}

int main(){

    //create CPU
    C6502 myCPU("./wozmon/wozmon.bin");

    //set unput and output lanes
    unsigned char* dataSend = myCPU.getIn();
    unsigned char* dataRecv = myCPU.getOut();
    bool* signal = myCPU.getDataInSig();

    //set thread control variables
    bool loopCPU = true;
    bool loopOutput = true;
    bool loopMain = true;

    //create threads
    std::thread t2(outputFromCpu, dataRecv, &loopOutput);
    std::thread t1(&C6502::run, &myCPU, &loopCPU);

    //create input environment (from jsmith message on https://cplusplus.com/forum/beginner/5619/#msg25139)
    struct termios oldSettings, newSettings;
    tcgetattr( fileno( stdin ), &oldSettings );
    newSettings = oldSettings;
    newSettings.c_lflag &= (~ICANON & ~ECHO);
    tcsetattr( fileno( stdin ), TCSANOW, &newSettings );    

    // std::this_thread::sleep_for(std::chrono::seconds(25));
    // loopMain = false;

    while ( loopMain ){
        //get the input controls
        fd_set set;
        FD_ZERO( &set );
        FD_SET( fileno( stdin ), &set );

        //wait till input available
        int res = select( fileno( stdin )+1, &set, NULL, NULL, NULL );

        //get the input
        char c;
        read( fileno( stdin ), &c, 1 );
        //translation for different number set?
        if(c == 0x0A) c = 0x0D;
        if(c == 0x7F) c = 0x08;
            
        //send the data and signal
        *dataSend = c;
        *signal = true;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    tcsetattr( fileno( stdin ), TCSANOW, &oldSettings );

    //shutdown program
    loopCPU = false;
    loopOutput = false;

    //joins
    t1.join();
    t2.join();

    //return no errors
    return 0;
}