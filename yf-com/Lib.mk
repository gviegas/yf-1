#!/usr/bin/env -S make -f

SHELL := /bin/sh

.SUFFIXES: .c .o .d

VAR_DIR := ~/.local/share/yf/
INCLUDE_DIR := include/
SRC_DIR := src/
CACHE_DIR := $(VAR_DIR)cache/com/
LIB_DIR := $(VAR_DIR)
LIB_BIN_DIR := $(LIB_DIR)lib/
LIB_INC_DIR := $(LIB_DIR)include/yf/com/

SO_LINK := libyf-com.so
SO_NAME := $(SO_LINK).0
SO_FILE := $(SO_NAME).1.0
LIB_FILE := $(LIB_BIN_DIR)$(SO_FILE)
LIB_NAME := $(LIB_BIN_DIR)$(SO_NAME)
LIB_LINK := $(LIB_BIN_DIR)$(SO_LINK)

SRC := $(wildcard $(SRC_DIR)*.c)

OBJ := $(subst $(SRC_DIR),$(CACHE_DIR),$(SRC:.c=.o))

DEP := $(OBJ:.o=.d)

CC := /usr/bin/cc
CC_FLAGS := -std=gnu17 -Wpedantic -Wall -Wextra -O3

LD_LIBS := -lm
LD_FLAGS := -I $(VAR_DIR)include/ \
	    -iquote $(INCLUDE_DIR) \
	    -iquote $(SRC_DIR) \
	    -L $(LIB_BIN_DIR)

PP := $(CC) -E
PP_FLAGS := -D YF -D YF_COM

lib: $(LIB_FILE)
	mkdir -pv $(LIB_INC_DIR)
	cp -v $(INCLUDE_DIR)*.h $(LIB_INC_DIR)
	ln -sfv $(LIB_FILE) $(LIB_LINK)
	sudo ldconfig $(LIB_BIN_DIR)

$(LIB_FILE): $(OBJ)
	$(CC) -shared -Wl,-soname,$(SO_NAME) \
		$(CC_FLAGS) $(LD_FLAGS) $^ $(LD_LIBS) -o $@

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
	rm -fv $(LIB_LINK) $(LIB_NAME) $(LIB_FILE) $(LIB_INC_DIR)*.h
	rmdir -v --ignore-fail-on-non-empty $(LIB_INC_DIR)

.PHONY: clean
clean: clean-obj clean-dep clean-lib

$(CACHE_DIR)%.o: $(SRC_DIR)%.c
	$(CC) $(CC_FLAGS) $(LD_FLAGS) $(PP_FLAGS) -fPIC -c $< -o $@

$(CACHE_DIR)%.d: $(SRC_DIR)%.c
	@$(PP) $(LD_FLAGS) $(PP_FLAGS) $< -MM -MT $(@:.d=.o) > $@

-include $(DEP)
