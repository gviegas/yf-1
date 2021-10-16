#!/usr/bin/env -S make -f

#
# YF
# Diag.mk
#
# Copyright © 2021 Gustavo C. Viegas.
#

SHELL := /bin/sh
.SUFFIXES: .c .o .d

INCLUDE_DIR := include/
SRC_DIR := src/
ETC_DIR := etc/
BUILD_DIR := build/

SRC := \
	$(wildcard $(SRC_DIR)*.c) \
	$(wildcard $(ETC_DIR)*.c)

OBJ := $(subst $(SRC_DIR),$(BUILD_DIR),$(SRC:.c=.o))
OBJ := $(subst $(ETC_DIR),$(BUILD_DIR),$(OBJ))

DEP := $(OBJ:.o=.d)

CC := /usr/bin/cc
CC_FLAGS := -std=gnu17 -Wpedantic -Wall -Wextra -g -fanalyzer

LD_LIBS := -lm -lyf-com -lyf-core -lyf-wsys
LD_FLAGS := -iquote $(INCLUDE_DIR) -iquote $(SRC_DIR)

PP := $(CC) -E
PP_FLAGS := -D YF_NGN -D YF_DEVEL

diag: $(OBJ)
	@echo Done.

-include $(DEP)

.PHONY: clean-obj
clean-obj:
	rm -f $(OBJ)

.PHONY: clean-dep
clean-dep:
	rm -f $(DEP)

.PHONY: clean
clean: clean-obj clean-dep

$(BUILD_DIR)%.o: $(SRC_DIR)%.c
	$(CC) $(CC_FLAGS) $(LD_FLAGS) $(PP_FLAGS) -c $< -o $@

$(BUILD_DIR)%.o: $(ETC_DIR)%.c
	$(CC) $(CC_FLAGS) $(LD_FLAGS) $(PP_FLAGS) -c $< -o $@

$(BUILD_DIR)%.d: $(SRC_DIR)%.c
	@$(PP) $(LD_FLAGS) $(PP_FLAGS) $< -MM -MT $(@:.d=.o) > $@

$(BUILD_DIR)%.d: $(ETC_DIR)%.c
	@$(PP) $(LD_FLAGS) $(PP_FLAGS) $< -MM -MT $(@:.d=.o) > $@
