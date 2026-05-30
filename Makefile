CC      = gcc
CFLAGS  = -g -Wall -Werror -pthread -std=c99
TARGET  = test
SRCS    = test.c concurrent_list.c
DEPS    = concurrent_list.h

all: $(TARGET)

$(TARGET): $(SRCS) $(DEPS)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS)

clean:
	rm -f $(TARGET)

.PHONY: all clean