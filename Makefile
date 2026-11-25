CC = gcc
RC = windres
STRIP = strip

CFLAGS = -std=c99 -Wall -Wextra -Wpedantic -mwindows -Os
LDFLAGS = -static -static-libgcc

BIN_NAME = TodoList.exe

C_SOURCES = main.c app_state.c text_list.c task_list_file.c layout.c util.c simple_ini.c
C_HEADERS = resource.h app_state.h text_list.h task_list_file.h layout.h util.h simple_ini.h

RESOURCE_FILE = resource.rc
RESOURCE_FILE_O = resource.o

all: $(BIN_NAME) $(C_SOURCES) $(RESOURCE_FILE)

$(BIN_NAME): $(C_SOURCES) $(C_HEADERS) $(RESOURCE_FILE)
	$(RC) $(RESOURCE_FILE) $(RESOURCE_FILE_O)
	$(CC) $(CFLAGS) -o $(BIN_NAME) $(C_SOURCES) $(RESOURCE_FILE_O) $(LDFLAGS)
	$(STRIP) $(BIN_NAME)

clean:
	del $(RESOURCE_FILE_O) || ver >nul
	del $(BIN_NAME) || ver >nul

