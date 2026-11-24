CC = gcc

TEST_BIN_NAME = initest.exe

C_SOURCES = simple_ini_test.c ../simple_ini.c
C_HEADERS = ../simple_ini.h

all: $(TEST_BIN_NAME) $(C_SOURCES)

.PHONY: test

$(TEST_BIN_NAME): $(C_SOURCES) $(C_HEADERS)
	$(CC) -o $(TEST_BIN_NAME) $(C_SOURCES)
	.\$(TEST_BIN_NAME)

clean:
	del $(TEST_BIN_NAME) || ver >nul

