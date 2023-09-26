SRC := writer.c
TARGET = writer
OBJS := $(SRC:.c=.o)

CC := $(if $(CROSS_COMPILE),$(CROSS_COMPILE)gcc, $(CC))


$(info 'CC is $(CC)')

$(TARGET) : $(OBJS)
	$(CC) $(CFLAGS) $(INCLUDES) $(OBJS) -o $(TARGET) $(LDFLAGS)

clean:
	-rm -f *.o $(TARGET) *.elf *.map