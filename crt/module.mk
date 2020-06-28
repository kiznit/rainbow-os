SOURCES := \
	stdio/fprintf.c \
	stdio/printf.c \
	stdio/puts.c \
	stdio/stdio.c \
	stdio/vfprintf.c \
	stdio/vprintf.c \
	string/strlen.c

# TODO: remove this and make it happen
CFLAGS += -U_FORTIFY_SOURCE
