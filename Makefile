CC       := gcc
CFLAGS   := -c -Wall -Wextra
TARGET   := ex3_lb
# TESTPATH := ~nimrodav/grep_tests/run_all.sh

all: $(TARGET)

clean:
	@$(RM) -f *.o $(TARGET)

# test:
# 	@$(TESTPATH)

$(TARGET): config.o match.o main.o
	$(CC) -o $(TARGET) $^

main.o: main.c
	$(CC) -o $@ $(CFLAGS) $(*F).c

.PHONY: all clean test
