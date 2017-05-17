
CC = g++
CFLAGS = -O3 -std=c++14 -Wextra -Wall 
outFile = testProg
OBJS =  objs/autoArgParse/argParser.o objs/exampleUsage.o


all: $(outFile)

objs/autoArgParse/argParser.o: src/autoArgParse/argParser.cpp src/autoArgParse/argParser.h src/autoArgParse/indentedLine.h src/autoArgParse/parseException.h
	mkdir -p $(@D) &&  $(CC) $(CFLAGS) -c -o objs/autoArgParse/argParser.o ./src/autoArgParse/argParser.cpp



objs/exampleUsage.o: src/exampleUsage.cpp src/autoArgParse/argParser.h src/autoArgParse/indentedLine.h
	mkdir -p $(@D) &&  $(CC) $(CFLAGS) -c -o objs/exampleUsage.o ./src/exampleUsage.cpp



$(outFile): $(OBJS) 
	$(CC) $(CFLAGS) $(OBJS) -o $(outFile)


clean:
	rm -fr objs $(outFile) 
