# Default target
all: release

# Build rules
%.o: %.cpp
	$(CC) -c $(CFLAGS) -o $@ $<
$(TARGET): $(DEPS) $(OBJECTS)
	$(CC) $^ $(LDFLAGS) -o $@

release: CFLAGS += -Wall -pedantic-errors -O3
release: LDFLAGS += -O3
release: $(TARGET)
debug: CFLAGS += -g
debug: $(TARGET)

# Rules for updating lexer and parser
%.lexer.cpp: %.l
	$(LEX) -o $@ $<
%.parser.cpp: %.y
	$(YACC) -d -o $@ $<
generate: $(BISON_SOURCES:.y=.parser.cpp) $(FLEX_SOURCES:.l=.lexer.cpp)

# Clean rule
clean:
	rm -f $(OBJECTS) $(DEPS) $(TARGET)

# Pattern to replace All with strategies using sed
REPLACE_ALL := "s/All/BestFS BFS DFS RDFS/"

# Check the build
check: $(TARGET)
	@failed=0; \
	for f in Tests/*.xml; do																	\
		for s in `echo $$f | cut -d- -f3 | cut -d. -f1 | sed -e $(REPLACE_ALL)`; do 			\
			echo "----------------------------------------------------------------------";		\
			echo "Testing $$f using $$s";														\
			./$< -s $$s -m 256 $$f $$f.q;												\
			if [ $$? -ne `echo $$f | cut -d- -f2` ]; then	 									\
				echo " --- Test Failed!"; 														\
				failed=$$(($$failed + 1));														\
			else																				\
				echo " +++ Test Succeeded"; 													\
			fi 																					\
		done																					\
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

.PHONY: all generate clean check test
