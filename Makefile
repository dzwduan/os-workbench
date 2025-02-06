# ----- DO NOT MODIFY -----

ifeq ($(NAME),)
$(error Should make in each lab's directory)
endif

SRCS   := $(shell find . -maxdepth 1 -name "*.c")
DEPS   := $(shell find . -maxdepth 1 -name "*.h") $(SRCS)
CFLAGS += -O1 -std=gnu11 -ggdb -Wall -Werror -Wno-unused-result -Wno-unused-value -Wno-unused-variable
CFLAGS += -g

.PHONY: all git test clean commit-and-make

.DEFAULT_GOAL := commit-and-make
commit-and-make: git all

$(NAME)-64: $(DEPS) # 64bit binary
	gcc -m64 $(CFLAGS) $(SRCS) -o $@ $(LDFLAGS)

$(NAME)-32: $(DEPS) # 32bit binary
	gcc -m32 $(CFLAGS) $(SRCS) -o $@ $(LDFLAGS)
 
$(NAME)-64.so: $(DEPS) # 64bit shared library
	gcc -fPIC -shared -m64 $(CFLAGS) $(SRCS) -o $@ $(LDFLAGS)

$(NAME)-32.so: $(DEPS) # 32bit shared library
	gcc -fPIC -shared -m32 $(CFLAGS) $(SRCS) -o $@ $(LDFLAGS)

check-64: $(NAME)-64
	valgrind --tool=memcheck --leak-check=full --show-leak-kinds=all --track-origins=yes ./$(NAME)-64

check-32: $(NAME)-32
	valgrind --tool=memcheck --leak-check=full --show-leak-kinds=all --track-origins=yes ./$(NAME)-32

clean:
	rm -f $(NAME)-64 $(NAME)-32 $(NAME)-64.so $(NAME)-32.so

include ../oslabs.mk
