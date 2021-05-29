CC       := gcc
CFLAGS   := -c -Wall -Wextra
TARGET   := ex3_lb
# TESTPATH := ~nimrodav/grep_tests/run_all.sh

all: $(TARGET)

clean:
	@$(RM) -f *.o $(TARGET)

# test:
# 	@$(TESTPATH)

$(TARGET): main.o http.o connect.o
	$(CC) -o $(TARGET) $^

main.o: main.c http.h connect.h
	$(CC) -o $@ $(CFLAGS) $(*F).c

http.o: http.c http.h connect.h
	$(CC) -o $@ $(CFLAGS) $(*F).c

connect.o: connect.c connect.h
	$(CC) -o $@ $(CFLAGS) $(*F).c

.PHONY: all clean test
