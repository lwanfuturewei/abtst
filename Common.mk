SRC_DIR = $(TOP_DIR)/src
OUT_DIR = $(TOP_DIR)/build
INCLUDE = $(TOP_DIR)/include
ARGOBOTS_SRC = $(TOP_DIR)/argobots
ARGOBOTS_BIN = $(TOP_DIR)/argobots_bin
$(info argobots bin is $(ARGOBOTS_BIN))

CC = gcc
CFLAGS = -g -ggdb -Wall -O0 -I$(INCLUDE) -I$(ARGOBOTS_SRC)/include
AR = ar
STATICFLAGS := rcs
LINKFLAGS = -g -O0 -ggdb $(ARGOBOTS_SRC)/src/.libs/libabt.a -lm -lpthread
