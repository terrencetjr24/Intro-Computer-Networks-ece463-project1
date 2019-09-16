TARGET = server

GCC = gcc

SRCS = server.randall7.c

OBJS = $(SRCS:%.c=%.o)


$(TARGET): $(OBJS)
	$(GCC) $(OBJS) -o $(TARGET)

test1:
	./shift_client localhost 2079 testcases/short_encrypted_4 4

test2:
	./shift_client localhost 2079 testcases/short_encrypted_7 7

.c.o:
	$(GCC) -c $*.c

clean:
	rm -f $(OBJS) $(TARGET)

