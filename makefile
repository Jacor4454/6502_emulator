COMPILER = g++
6502DIR = ./6502/
OBJECTDIR = ./objects/

project: main.o class_6502.o
	$(COMPILER) -o a $(OBJECTDIR)main.o $(OBJECTDIR)class_6502.o

main.o: main.cpp
	$(COMPILER) -c -o $(OBJECTDIR)main.o main.cpp

class_6502.o: $(6502DIR)class_6502.cpp
	$(COMPILER) -c -o $(OBJECTDIR)class_6502.o $(6502DIR)class_6502.cpp
