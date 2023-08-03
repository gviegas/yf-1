#!/usr/bin/env -S make -f

SHELL := /bin/sh

.SUFFIXES: .c .o .d

VAR_DIR := ~/.local/share/yf/
INCLUDE_DIR := include/
SRC_DIR := src/
CACHE_DIR := $(VAR_DIR)cache/core/
LIB_DIR := $(VAR_DIR)
LIB_BIN_DIR := $(LIB_DIR)lib/
LIB_INC_DIR := $(LIB_DIR)include/yf/core/

LIB_A := $(LIB_BIN_DIR)libyf-core.a

SRC := $(wildcard $(SRC_DIR)*.c)

OBJ := $(subst $(SRC_DIR),$(CACHE_DIR),$(SRC:.c=.o))

DEP := $(OBJ:.o=.d)

CC := /usr/bin/cc
CC_FLAGS := -std=gnu17 -Wpedantic -Wall -Wextra -O3

LD_FLAGS := -I $(VAR_DIR)include/ \
	    -iquote $(INCLUDE_DIR) \
	    -iquote $(SRC_DIR) \
	    -L $(LIB_BIN_DIR)

PP := $(CC) -E
PP_FLAGS := -D YF -D YF_CORE

lib: $(LIB_A)
	mkdir -pv $(LIB_INC_DIR)
	cp -v $(INCLUDE_DIR)*.h $(LIB_INC_DIR)

$(LIB_A): $(OBJ)
	ar rcs $@ $^

compile: $(OBJ)
	@echo Done.

.PHONY: clean-obj
clean-obj:
	rm -fv $(OBJ)

.PHONY: clean-dep
clean-dep:
	rm -fv $(DEP)

.PHONY: clean-lib
clean-lib:
	rm -fv $(LIB_A) $(LIB_INC_DIR)*.h
	rmdir -v --ignore-fail-on-non-empty $(LIB_INC_DIR)

.PHONY: clean
clean: clean-obj clean-dep clean-lib

$(CACHE_DIR)%.o: $(SRC_DIR)%.c
	$(CC) $(CC_FLAGS) $(LD_FLAGS) $(PP_FLAGS) -c $< -o $@

$(CACHE_DIR)%.d: $(SRC_DIR)%.c
	@$(PP) $(LD_FLAGS) $(PP_FLAGS) $< -MM -MT $(@:.d=.o) > $@

-include $(DEP)
