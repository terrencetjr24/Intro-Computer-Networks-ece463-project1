TARGET = server

GCC = gcc

SRCS = server.randall7.c

OBJS = $(SRCS:%.c=%.o)


$(TARGET): $(OBJS)
$(GCC) $(OBJS) -o $(TARGET)


test1: $(TARGET)
    ./shift_client localhost 2079 <pathname> 4

test2: $(TARGET)
    ./shift_client localhost 2079 <pathname> 7

.c.o:
$(GCC) -c $*.c

clean:
rm -f $(OBJS) $(TARGET)

