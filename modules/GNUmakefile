MAKEFLAGS += -rR

OUTPUT_DIR := bin

CC := cc
LD := ld

# Userspace module flags (NOT kernel module flags!)
CFLAGS := -Wall -Wextra -std=gnu11 \
          -ffreestanding -fno-stack-protector -fno-stack-check \
          -fPIE -m64 -march=x86-64 \
          -mno-80387 -mno-mmx -mno-sse -mno-sse2 \
          -mno-red-zone -Wno-unused-function -Wno-unused-parameter -Wno-deprecated-declarations -Wno-ignored-attributes -Wno-unused-variable \
		  -Wno-error=implicit-function-declaration -Wno-return-type

# Full executable linking (NOT -r)
LDFLAGS := -nostdlib -static -pie \
           -z max-page-size=0x1000

MODULE_DIRS := $(shell find . -maxdepth 1 -type d -not -path '.' -not -path './$(OUTPUT_DIR)')
MODULE_NAMES := $(patsubst ./%,%,$(MODULE_DIRS))

.PHONY: all clean $(MODULE_NAMES)

all: $(OUTPUT_DIR) $(MODULE_NAMES)

$(OUTPUT_DIR):
	mkdir -p $(OUTPUT_DIR)

define BUILD_MODULE_RULE
$(1):
	@echo " [ MOD ] Building $(1)..."
	@mkdir -p $(1)/obj
	$(eval SRCS := $(shell find $(1)/src -name '*.c'))
	$(eval OBJS := $(patsubst $(1)/src/%.c, $(1)/obj/%.o, $(SRCS)))
	@$(foreach src,$(SRCS), \
	    mkdir -p $$(dirname $(patsubst $(1)/src/%.c, $(1)/obj/%.o, $(src))); \
	    $(CC) $(CFLAGS) -c $(src) -o $(patsubst $(1)/src/%.c, $(1)/obj/%.o, $(src)); \
	)
	@$(LD) $(LDFLAGS) -T $(1)/linker.ld $(OBJS) -o $(OUTPUT_DIR)/$(1)
	@echo " [ MOD ] Finished $(OUTPUT_DIR)/$(1)"
endef

$(foreach mod,$(MODULE_NAMES),$(eval $(call BUILD_MODULE_RULE,$(mod))))

clean:
	rm -rf $(OUTPUT_DIR)
	$(foreach mod,$(MODULE_NAMES),rm -rf $(mod)/obj)