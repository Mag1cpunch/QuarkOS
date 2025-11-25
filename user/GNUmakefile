# Nuke built-in rules and variables.
MAKEFLAGS += -rR
.SUFFIXES:

override OUTPUT := user

CC      := cc
CFLAGS  := -g -O2 -pipe
CPPFLAGS :=
LDFLAGS :=

# Внутренние флаги для user-space
override CFLAGS += \
    -Wall -Wextra \
    -std=gnu11 \
    -ffreestanding \
    -fno-stack-protector \
    -fno-stack-check \
    -fno-PIC \
    -ffunction-sections \
    -fdata-sections \
    -m64 \
    -march=x86-64 \
    -mno-80387 \
    -mno-mmx \
    -mno-sse \
    -mno-sse2 \
    -Wno-unused-function \
    -Wno-unused-parameter \
    -Wno-deprecated-declarations \
    -Wno-ignored-attributes \
    -Wno-unused-variable \
    -Wno-error=implicit-function-declaration \
    -Wno-return-type

override CPPFLAGS := \
    -Isrc \
    $(CPPFLAGS) \
    -MMD -MP

override LDFLAGS += \
    -Wl,-m,elf_x86_64 \
    -nostdlib -static \
    -z max-page-size=0x1000 \
    -Wl,--gc-sections \
    -T linker.ld

# ищем исходники только внутри user/src
SRCFILES   := $(shell find -L src -type f | LC_ALL=C sort)
CFILES     := $(filter %.c,$(SRCFILES))
ASFILES    := $(filter %.S,$(SRCFILES))
OBJ        := $(addprefix obj/,$(CFILES:.c=.c.o) $(ASFILES:.S=.S.o))
HEADER_DEPS:= $(addprefix obj/,$(CFILES:.c=.c.d) $(ASFILES:.S=.S.d))

.PHONY: all
all: bin/$(OUTPUT)

-include $(HEADER_DEPS)

bin/$(OUTPUT): GNUmakefile linker.ld $(OBJ)
	mkdir -p "$$(dirname $@)"
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJ) -o $@

obj/%.c.o: %.c GNUmakefile
	mkdir -p "$$(dirname $@)"
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

obj/%.S.o: %.S GNUmakefile
	mkdir -p "$$(dirname $@)"
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -rf bin obj

.PHONY: distclean
distclean: clean
