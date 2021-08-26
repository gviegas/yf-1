#!/usr/bin/env -S make -f

#
# YF
# Lib.mk
#
# Copyright © 2020-2021 Gustavo C. Viegas.
#

SHELL := /bin/sh
.SUFFIXES: .c .o .d

INCLUDE_DIR := include/
SRC_DIR := src/
ETC_DIR := etc/
BUILD_DIR := build/
INSTALL_DIR := /usr/local/
INCLIB_DIR := $(INSTALL_DIR)include/yf/com/

SO_LINK := libyf-com.so
SO_NAME := $(SO_LINK).0
SO_FILE := $(SO_NAME).1.0

LIB_FILE := $(INSTALL_DIR)lib/$(SO_FILE)
LIB_NAME := $(INSTALL_DIR)lib/$(SO_NAME)
LIB_LINK := $(INSTALL_DIR)lib/$(SO_LINK)

SRC := \
	$(wildcard $(SRC_DIR)*.c) \
	$(wildcard $(ETC_DIR)*.c)

OBJ := $(subst $(SRC_DIR),$(BUILD_DIR),$(SRC:.c=.o))
OBJ := $(subst $(ETC_DIR),$(BUILD_DIR),$(OBJ))

DEP := $(OBJ:.o=.d)

CC := /usr/bin/cc
CC_FLAGS := -std=gnu17 -Wpedantic -Wall -Wextra -g #-O3

LD_LIBS := -lm
LD_FLAGS := -iquote $(INCLUDE_DIR) -iquote $(SRC_DIR)

PP := $(CC) -E
PP_FLAGS := -D YF_COM

all:

install: $(LIB_FILE)
	mkdir -p $(INCLIB_DIR)
	cp $(INCLUDE_DIR)*.h $(INCLIB_DIR)
	ln -sf $(LIB_FILE) $(LIB_LINK)
	ldconfig -n $(INSTALL_DIR)lib/

$(LIB_FILE): $(OBJ)
	$(CC) -shared -Wl,-soname,$(SO_NAME) \
	$(CC_FLAGS) $(LD_FLAGS) $^ $(LD_LIBS) -o $@

compile: $(OBJ)
	@echo Done.

-include $(DEP)

.PHONY: uninstall
uninstall:
	rm -f $(LIB_LINK) $(LIB_NAME) $(LIB_FILE) $(INCLIB_DIR)*.h
	rmdir --ignore-fail-on-non-empty $(INCLIB_DIR)

.PHONY: clean-obj
clean-obj:
	rm -f $(OBJ)

.PHONY: clean-dep
clean-dep:
	rm -f $(DEP)

.PHONY: clean
clean: clean-obj clean-dep

$(BUILD_DIR)%.o: $(SRC_DIR)%.c
	$(CC) $(CC_FLAGS) $(LD_FLAGS) $(PP_FLAGS) -fPIC -c $< -o $@

$(BUILD_DIR)%.o: $(ETC_DIR)%.c
	$(CC) $(CC_FLAGS) $(LD_FLAGS) $(PP_FLAGS) -fPIC -c $< -o $@

$(BUILD_DIR)%.d: $(SRC_DIR)%.c
	@$(PP) $(LD_FLAGS) $(PP_FLAGS) $< -MM -MT $(@:.d=.o) > $@

$(BUILD_DIR)%.d: $(ETC_DIR)%.c
	@$(PP) $(LD_FLAGS) $(PP_FLAGS) $< -MM -MT $(@:.d=.o) > $@
