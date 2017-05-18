
CC = g++
CFLAGS = -O3 -std=c++14 -Wextra -Wall 
outFile = testProg
OBJS =  objs/autoArgParse/argParser.o  objs/exampleUsage.o 


all: $(outFile)
	@echo "Built example program ./testProg"

objs/autoArgParse/argParser.o: autoArgParse/argParser.cpp autoArgParse/argHandlers.h autoArgParse/argParser.h autoArgParse/indentedLine.h autoArgParse/parseException.h 
	mkdir -p $(@D) &&  $(CC) $(CFLAGS) -c -o objs/autoArgParse/argParser.o ./autoArgParse/argParser.cpp




objs/exampleUsage.o: exampleUsage.cpp autoArgParse/argParser.h autoArgParse/indentedLine.h
	mkdir -p $(@D) &&  $(CC) $(CFLAGS) -c -o objs/exampleUsage.o ./exampleUsage.cpp



$(outFile): $(OBJS) 
	$(CC) $(CFLAGS) $(OBJS) -o $(outFile)


clean:
	rm -fr objs $(outFile) 
