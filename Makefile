# Makefile для сборки программы db_sync

CC = gcc
CFLAGS = -Wall -Wextra -O2
TARGET = db_sync
SRC = db_sync.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

run: $(TARGET)
	touch progfile
	./$(TARGET)

clean:
	rm -f $(TARGET) progfile
