# 6502_emulator

## Overview
this is an 8 bit cpu emulator. It has a very easily changeable instruction set (at the moment it is the base 6502 instruction set [[link](https://www.masswerk.at/6502/6502_instruction_set.html)] with the 65C02 DEC function [[link](https://www.westerndesigncenter.com/wdc/documentation/w65c02s.pdf)] added in the correct place)

# Contents
## grid.txt
### Overview
this file contains the instructions the processor follows. it is a slightly modified version of the standard 6502 instruction set (all 6502 commands work but 1 65C02 instruction was added in one of the unused command spaces). All commands follow these rules:

### Addressing Modes
- seperated by a tab ('\t') character
- the command is 3 capital letters
- if addressing is not implied, there must be a space and then one of the following addressing modes:
 - A     .... Accumulator
 - abs   .... Absolute
 - abs,X .... Absolute, X-indexed
 - abs,Y .... Absolute, Y-indexed
 - \#    .... immediate
 - impl  .... implied (oftern not neccisary)
 - ind   .... indirect
 - X,ind .... X-indexed, indirect
 - ind,Y .... indirect, Y-indexed
 - rel   .... relative
 - zpg   .... zeropage
 - zpg,X .... zeropage, X-indexed
 - zpg,Y .... zeropage, Y-indexed

### Modified Instruction
the few alterations from the standard 6502 instructions: 
-  The illegal opcode $EB - "USBC #" has been replaced with "USB #" in grid.txt since the emulator uses 3 digit opcodes
- Another illegal opcode $3A - "NOP impl" has been replaced with the 65C02 instruction in $3A - "DEC A", as this is used in Ben Eaters modified WOZMON (see my other repos for the project I am working on with that)

## getLinks.py
if you wish to add a new command, there is this python file that you can run, type in the command and it will output a case you can add to the switch in 6502/class_6502.cpp

## makefile
compiles and links using g++

# Platform Limitations
at the moment this can only be compiled and ran on linux, as it uses the termios.h library, which only works in linux. I windows version in on my to do list, just not yet

# Reserved Addresses
## 6502 Standard Addressing
The 6502 has 2 "special" memory areas:
- $0000-$00FF - zero page
- $0100-$01FF - processor stack

## ASIC Addresses
this emulator uses an ASIC interface to communicate with the terminal, the addresses below are used by this interface:
- $5000 - read/write data to/from the terminal
- $5001 - the status line, will output $00 when there is no data to read, $08 when there is data to read
- $5002 - cmd data, used in a real ASIC, but not here, though it is reserved if needed to be implemented later
- $5003 - ctrl data, same as situation as cmd
