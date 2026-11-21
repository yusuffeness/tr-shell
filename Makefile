# tr-shell Makefile
#
#   make            build ./trsh (objects go to build/)
#   make run        build and start the shell
#   make test       run the behaviour tests
#   make debug      build with AddressSanitizer and UBSan
#   make install    install trsh and its man page under ~/.local (PREFIX=...)
#   make uninstall  remove what make install put there
#   make clean      remove build output

TARGET   := trsh
SRC_DIR  := src
BUILD    := build

CC       ?= cc
CFLAGS   ?= -O2
WARN     := -std=c11 -Wall -Wextra -Werror -pedantic
CPPFLAGS += -MMD -MP
LDFLAGS  ?=
LDLIBS   ?=
LDLIBS   += -lm

PREFIX   ?= $(HOME)/.local
BINDIR   := $(PREFIX)/bin
MANDIR   := $(PREFIX)/share/man/man1

# GNU readline gives line editing, history on the arrow keys and Tab
# completion. It is used when found; `make READLINE=0` builds without it.
# (Debian/Ubuntu/WSL: sudo apt install libreadline-dev)
READLINE_TEST := '\#include <stdio.h>\n\#include <readline/readline.h>\nint main(void) { rl_callback_sigcleanup(); return 0; }\n'
READLINE ?= $(shell printf $(READLINE_TEST) | $(CC) -x c - -lreadline -o /dev/null 2>/dev/null && echo 1 || echo 0)

ifeq ($(READLINE),1)
CPPFLAGS += -DHAVE_READLINE
LDLIBS   += -lreadline
endif

SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(SRCS:$(SRC_DIR)/%.c=$(BUILD)/%.o)
DEPS := $(OBJS:.o=.d)

.PHONY: all run test debug install uninstall clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(BUILD)/%.o: $(SRC_DIR)/%.c | $(BUILD)
	$(CC) $(CPPFLAGS) $(WARN) $(CFLAGS) -c -o $@ $<

$(BUILD):
	mkdir -p $@

run: $(TARGET)
	./$(TARGET)

test: $(TARGET)
	bash tests/run.sh

debug:
	$(MAKE) clean
	$(MAKE) CFLAGS="-O0 -g -fsanitize=address,undefined" \
	        LDFLAGS="-fsanitize=address,undefined"

install: $(TARGET)
	install -d $(BINDIR)
	install -m 755 $(TARGET) $(BINDIR)/$(TARGET)
	install -d $(MANDIR)
	install -m 644 docs/trsh.1 $(MANDIR)/trsh.1

uninstall:
	rm -f $(BINDIR)/$(TARGET) $(MANDIR)/trsh.1

clean:
	rm -rf $(BUILD) $(TARGET)

-include $(DEPS)
