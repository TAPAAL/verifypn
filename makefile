#Programs for processing
LEX				= flex
YACC			= bison
CC				= g++
RM				= rm
FIND			= find
GREP			= grep
SED				= sed

#Compiler and linker flags
CFLAGS			= -O3 -I.
LDFLAGS			= -static -O3 -llpsolve55 -lcolamd -ldl

#Target
TARGET			= VerifyPN

#Source files
FLEX_SOURCES	= $(shell $(FIND) * -name "*.l")
BISON_SOURCES	= $(shell $(FIND) * -name "*.y")
SOURCES			= $(shell $(FIND) * -name "*.cpp" | $(GREP) -v ".\\(parser\\|lexer\\).cpp")	\
				  $(BISON_SOURCES:.y=.parser.cpp)											\
				  $(FLEX_SOURCES:.l=.lexer.cpp)

#Intermediate files
OBJECTS			= $(SOURCES:.cpp=.o)

#Default target
all: $(TARGET)

#Rules for updating lexer and parser
%.lexer.cpp: %.l
	$(LEX) -o $@ $<
%.parser.cpp: %.y
	$(YACC) -d -o $@ $<
generate: $(BISON_SOURCES:.y=.parser.cpp) $(FLEX_SOURCES:.l=.lexer.cpp)

#Build rules
%.o: %.cpp
	$(CC) -c $(CFLAGS) -o $@ $<
$(TARGET): $(OBJECTS)
	$(CC) $^ $(LDFLAGS) -o $@

#Clean rule
clean:
	$(RM) -f $(OBJECTS)

#Check the build
check: $(TARGET)
	@for f in Tests/*.xml; do																	\
		echo "Testing $$f:";																	\
		./$(TARGET) -m 256 $$f $$f.q;															\
		if [ `echo $$f | $(SED) -e "s/.*-\([0-9]\)\.xml/\1/"` -ne $$? ]; then 					\
			echo " --- Test Failed!"; 															\
		else																					\
			echo " +++ Test Succeeded"; 														\
		fi 																						\
	done 

.PHONY: all generate clean check
