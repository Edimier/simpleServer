CC = gcc
CFLAGS = -g -O2 -Wall -lpthread
TARGET = client

.PHONY : all clean

all : $(TARGET)

$(TARGET) : client.c
	$(CC) $(CFLAGS) $(SHARED) $^ -o $@

clean :
	@rm -f $(TARGET)
