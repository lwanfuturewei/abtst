CUR_DIR := $(patsubst %/,%,$(dir $(abspath $(lastword $(MAKEFILE_LIST)))))
include ../../Common.mk
TOP_DIR := $(patsubst %/,%,$(dir $(abspath $(lastword $(MAKEFILE_LIST)))))
$(info "TOP DIR" $(TOP_DIR))
BUILD_DIR := $(OUT_DIR)/obj/app

#Fill in
FILES := arm_profile.c prof_bus.c prof_coh.c prof_instr.c prof_memory.c prof_topdown.c \
	prof_cache.c prof_cpu.c prof_lock.c prof_predict.c prof_topo.c prof_hotspot.c prof_spe.c

SRCS := $(CUR_DIR)/$(FILES)
OBJS := $(addprefix $(BUILD_DIR)/,$(patsubst %.c, %.o, $(FILES)))
# Fill in
BINS = $(OUT_DIR)/armprof

LINKFLAGS := $(OUT_DIR)/libutil.a $(OUT_DIR)/libarch.a $(LINKFLAGS)

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
	$(CC) -o $(BINS) $(OBJS) $(LINKFLAGS)

clean:
	rm $(OBJS) $(BINS)
