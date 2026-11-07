CC = gcc
CFLAGS = -std=c11 -Wall -Wextra
TARGET = diskSim
SOURCE = diskSim.c

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SOURCE) Makefile
	$(CC) $(CFLAGS) -o $@ $(SOURCE)

clean:
	rm -f $(TARGET)
