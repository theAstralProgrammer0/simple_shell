#c compiler
CC = gcc

#compiler flags
CFLAGS = -Wall -Werror -Wextra -pedantic -std=gnu89 -g

#list of source files
SRCS = shell.c alias.c alias_handler.c alias_utils.c env.c

#object files from srcs
OBJS = $(SRCS:.c=.o)

#specify target file
TARGET = hsh

#Build rule for the target file
$(TARGET): $(OBJS)
	     $(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

#Generic build run for exec to obj
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@
#clean rule
clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: clean
