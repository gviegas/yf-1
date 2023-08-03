#!/usr/bin/env -S make -f

SHELL := /bin/sh

.SUFFIXES: .c .o .d

VAR_DIR := ~/.local/share/yf/
INCLUDE_DIR := include/
SRC_DIR := src/
TEST_DIR := test/
SCRIPT_DIR := script/
BIN_DIR := $(VAR_DIR)bin/
CACHE_DIR := $(VAR_DIR)cache/ngn/

SRC := $(wildcard $(SRC_DIR)*.c) $(wildcard $(TEST_DIR)*.c)

OBJ := $(subst $(SRC_DIR),$(CACHE_DIR),$(SRC:.c=.o))
OBJ := $(subst $(TEST_DIR),$(CACHE_DIR),$(OBJ))

DEP := $(OBJ:.o=.d)

CC := /usr/bin/cc
CC_FLAGS := -std=gnu17 -Wpedantic -Wall -Wextra -g

LD_LIBS := -lm -lyf-core -lyf-wsys -lyf-com
LD_FLAGS := -I $(VAR_DIR)include/ \
	    -iquote $(INCLUDE_DIR) \
	    -iquote $(SRC_DIR) \
	    -L $(VAR_DIR)lib/

PP := $(CC) -E
PP_FLAGS := -D YF -D YF_NGN -D YF_DEVEL

OUT := $(BIN_DIR)ngn-devel

INC_SCRIPT := $(SCRIPT_DIR)inc.sh

.PHONY: all
all: inc devel

.PHONY: inc
inc:
	./$(INC_SCRIPT)

devel: $(OBJ)
	$(CC) $(CC_FLAGS) $(LD_FLAGS) $^ $(LD_LIBS) -o $(OUT)

compile: $(OBJ)
	@echo Done.

.PHONY: clean-out
clean-out:
	rm -fv $(OUT)

.PHONY: clean-obj
clean-obj:
	rm -fv $(OBJ)

.PHONY: clean-dep
clean-dep:
	rm -fv $(DEP)

.PHONY: clean
clean: clean-out clean-obj clean-dep

$(CACHE_DIR)%.o: $(SRC_DIR)%.c
	$(CC) $(CC_FLAGS) $(LD_FLAGS) $(PP_FLAGS) -c $< -o $@

$(CACHE_DIR)%.o: $(TEST_DIR)%.c
	$(CC) $(CC_FLAGS) $(LD_FLAGS) $(PP_FLAGS) -c $< -o $@

$(CACHE_DIR)%.d: $(SRC_DIR)%.c
	@$(PP) $(LD_FLAGS) $(PP_FLAGS) $< -MM -MT $(@:.d=.o) > $@

$(CACHE_DIR)%.d: $(TEST_DIR)%.c
	@$(PP) $(LD_FLAGS) $(PP_FLAGS) $< -MM -MT $(@:.d=.o) > $@

-include $(DEP)
