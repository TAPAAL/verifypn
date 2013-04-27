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
LDFLAGS			= -O3 /usr/lib/liblpsolve55.a -lcolamd -ldl

#Target
TARGET			= verifypn

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
	$(RM) -f $(OBJECTS) $(TARGET)

#Check the build
check: $(TARGET)
	@failed=0; \
	for f in Tests/*.xml; do																	\
		echo "----------------------------------------------------------------------";			\
		echo "Testing $$f:";																	\
		./$(TARGET) -m 256 $$f $$f.q;															\
		if [ $$? -ne `echo $$f | $(SED) -e "s/.*-\([0-9]\)\.xml/\1/"` ]; then 					\
			echo " --- Test Failed!"; 															\
			failed=$$(($$failed + 1));															\
		else																					\
			echo " +++ Test Succeeded"; 														\
		fi 																						\
	done; 																						\
	if [ "$$failed" -ne "0" ]; then 															\
		echo "----------------------------------------------------------------------"; 			\
		echo "\033[1m                      $$failed test(s) Failed \033[0m"; 					\
		echo "----------------------------------------------------------------------"; 			\
	else 																						\
		echo "----------------------------------------------------------------------"; 			\
		echo "\033[1m                      All tests was successful \033[0m"; 					\
		echo "----------------------------------------------------------------------"; 			\
	fi



.PHONY: all generate clean check
