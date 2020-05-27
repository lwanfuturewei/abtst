CUR_DIR := $(patsubst %/,%,$(dir $(abspath $(lastword $(MAKEFILE_LIST)))))
include ../../Common.mk
TOP_DIR := $(patsubst %/,%,$(dir $(abspath $(lastword $(MAKEFILE_LIST)))))
BUILD_DIR := $(OUT_DIR)/obj/libmapping

FILES := mapping.c hash/hash.c random/random.c

SRCS := $(CUR_DIR)/$(FILES)
OBJS := $(addprefix $(BUILD_DIR)/,$(patsubst %.c, %.o, $(FILES)))
LIBS = $(OUT_DIR)/libmapping.a

.PHONY: all libs clean

default: libs

.PRECIOUS: $(BUILD_DIR)/. $(BUILD_DIR)%/.

$(BUILD_DIR)/.:
	mkdir -p $@

$(BUILD_DIR)%/.:
	mkdir -p $@

.SECONDEXPANSION:

$(BUILD_DIR)/%.o: $(CUR_DIR)/%.c | $$(@D)/.
	$(CC) $(CFLAGS) -o $@  -c $<

libs: $(OBJS)
	$(AR) $(STATICFLAGS) $(LIBS) $(OBJS)


all: default

clean:
	rm $(OBJS) $(LIBS)
