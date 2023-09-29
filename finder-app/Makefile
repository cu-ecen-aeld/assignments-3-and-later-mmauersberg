SRC := writer.c
TARGET = writer
OBJS := $(SRC:.c=.o)

CC := $(if $(CROSS_COMPILE),$(CROSS_COMPILE)gcc, $(CC))

$(info 'CC is $(CC)')

$(TARGET) : $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

.PHONY: all clean

all: $(TARGET)

clean:
	-rm -f *.o $(TARGET) *.elf *.map
