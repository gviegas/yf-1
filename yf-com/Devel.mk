#!/usr/bin/env -S make -f

#
# YF
# Devel.mk
#
# Copyright © 2020 Gustavo C. Viegas.
#

SHELL := /bin/sh
.SUFFIXES: .c .o .d

INCLUDE_DIR := include/
SRC_DIR := src/
TEST_DIR := test/
ETC_DIR := etc/
BIN_DIR := bin/
BUILD_DIR := build/

SRC := \
	$(wildcard $(SRC_DIR)*.c) \
	$(wildcard $(TEST_DIR)*.c) \
	$(wildcard $(ETC_DIR)*.c)

OBJ := $(subst $(SRC_DIR),$(BUILD_DIR),$(SRC:.c=.o))
OBJ := $(subst $(TEST_DIR),$(BUILD_DIR),$(OBJ))
OBJ := $(subst $(ETC_DIR),$(BUILD_DIR),$(OBJ))

DEP := $(OBJ:.o=.d)

CC := /usr/bin/cc
CC_FLAGS := -std=gnu17 -Wpedantic -Wall -Wextra -g

LD_LIBS := -lm
LD_FLAGS := -iquote $(INCLUDE_DIR) -iquote $(SRC_DIR)

PP := $(CC) -E
PP_FLAGS := -D YF_COM -D YF_DEVEL -D YF_DEBUG

OUT := $(BIN_DIR)devel

devel: $(OBJ)
	$(CC) $(CC_FLAGS) $(LD_FLAGS) $^ $(LD_LIBS) -o $(OUT)

-include $(DEP)

.PHONY: clean-out
clean-out:
	rm -f $(OUT)

.PHONY: clean-obj
clean-obj:
	rm -f $(OBJ)

.PHONY: clean-dep
clean-dep:
	rm -f $(DEP)

.PHONY: clean
clean: clean-out clean-obj clean-dep

$(BUILD_DIR)%.o: $(SRC_DIR)%.c
	$(CC) $(CC_FLAGS) $(LD_FLAGS) $(PP_FLAGS) -c $< -o $@

$(BUILD_DIR)%.o: $(TEST_DIR)%.c
	$(CC) $(CC_FLAGS) $(LD_FLAGS) $(PP_FLAGS) -c $< -o $@

$(BUILD_DIR)%.o: $(ETC_DIR)%.c
	$(CC) $(CC_FLAGS) $(LD_FLAGS) $(PP_FLAGS) -c $< -o $@

$(BUILD_DIR)%.d: $(SRC_DIR)%.c
	@$(PP) $(LD_FLAGS) $(PP_FLAGS) $< -MM -MT $(@:.d=.o) > $@

$(BUILD_DIR)%.d: $(TEST_DIR)%.c
	@$(PP) $(LD_FLAGS) $(PP_FLAGS) $< -MM -MT $(@:.d=.o) > $@

$(BUILD_DIR)%.d: $(ETC_DIR)%.c
	@$(PP) $(LD_FLAGS) $(PP_FLAGS) $< -MM -MT $(@:.d=.o) > $@
