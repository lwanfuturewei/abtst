CUR_DIR := $(patsubst %/,%,$(dir $(abspath $(lastword $(MAKEFILE_LIST)))))
include ../../Common.mk
TOP_DIR := $(patsubst %/,%,$(dir $(abspath $(lastword $(MAKEFILE_LIST)))))
$(info "TOP DIR" $(TOP_DIR))
BUILD_DIR := $(OUT_DIR)/obj/app

#Fill in
FILES := abt_st.c

SRCS := $(CUR_DIR)/$(FILES)
OBJS := $(addprefix $(BUILD_DIR)/,$(patsubst %.c, %.o, $(FILES)))
# Fill in
BINS = $(OUT_DIR)/abtst

LINKS = $(OUT_DIR)/libload.a $(OUT_DIR)/libstream.a $(OUT_DIR)/libmapping.a \
	$(OUT_DIR)/libsched.a $(OUT_DIR)/librebalance.a $(OUT_DIR)/libarch.a 

$(info link path is $(LINKFLAGS))

.PHONY: all clean

default: all 

.PRECIOUS: $(BUILD_DIR)/. $(BUILD_DIR)%/.

$(BUILD_DIR)/.:
	mkdir -p $@

$(BUILD_DIR)%/.:
	mkdir -p $@

.SECONDEXPANSION:

#$(BUILD_DIR)/%.o: $(CUR_DIR)/%.c  | $$(@D)/.
$(BUILD_DIR)/%.o: $(CUR_DIR)/%.c | $$(@D)/.
	$(CC) $(CFLAGS) -o $@  -c $<

all: $(OBJS)
	$(CC) -o $(BINS) $(OBJS) $(LINKS) $(LINKS) $(LINKFLAGS)

clean:
	rm $(OBJS) $(BINS)
