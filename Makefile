# tr-shell Makefile
#
#   make            derle (build/ altına, çıktı: ./trsh)
#   make run        derle ve çalıştır
#   make test       davranış testlerini çalıştır
#   make debug      AddressSanitizer + UBSan ile hata ayıklama sürümü
#   make install    ~/.local/bin altına kur (PREFIX ile değiştirilebilir)
#   make uninstall  kurulumu kaldır
#   make clean      derleme çıktılarını sil

TARGET   := trsh
SRC_DIR  := src
BUILD    := build

CC       ?= cc
CFLAGS   ?= -O2
WARN     := -std=c11 -Wall -Wextra -Werror -pedantic
CPPFLAGS += -MMD -MP
LDFLAGS  ?=
LDLIBS   ?=

PREFIX   ?= $(HOME)/.local
BINDIR   := $(PREFIX)/bin

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

uninstall:
	rm -f $(BINDIR)/$(TARGET)

clean:
	rm -rf $(BUILD) $(TARGET)

-include $(DEPS)
