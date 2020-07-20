CUR_DIR := $(patsubst %/,%,$(dir $(abspath $(lastword $(MAKEFILE_LIST)))))
include ../../Common.mk
TOP_DIR := $(patsubst %/,%,$(dir $(abspath $(lastword $(MAKEFILE_LIST)))))
$(info "TOP DIR" $(TOP_DIR))
BUILD_DIR := $(OUT_DIR)/obj/test

#Fill in
FILES_L1 := rebalance_test_l1.c ../app/abt_st.c ../app/st.c
SRCS_L1 := $(CUR_DIR)/$(FILES_L1)
OBJS_L1 := $(addprefix $(BUILD_DIR)/,$(patsubst %.c, %.o, $(FILES_L1)))
BINS_L1 = reb1_test

FILES_L2 := rebalance_test_l2.c ../app/abt_st.c ../app/st.c
SRCS_L2 := $(CUR_DIR)/$(FILES_L2)
OBJS_L2 := $(addprefix $(BUILD_DIR)/,$(patsubst %.c, %.o, $(FILES_L2)))
BINS_L2 = reb2_test

FILES_L3 := rebalance_test_l3.c ../app/abt_st.c ../app/st.c
SRCS_L3 := $(CUR_DIR)/$(FILES_L3)
OBJS_L3 := $(addprefix $(BUILD_DIR)/,$(patsubst %.c, %.o, $(FILES_L3)))
BINS_L3 = reb3_test

LINKS = $(OUT_DIR)/libload.a $(OUT_DIR)/libstream.a $(OUT_DIR)/libmapping.a \
	$(OUT_DIR)/libsched.a $(OUT_DIR)/librebalance.a $(OUT_DIR)/libpartition.a $(OUT_DIR)/libpower.a $(OUT_DIR)/libarch.a 

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

reb1: $(OBJS_L1)
	$(CC) -o $(BINS_L1) $(OBJS_L1) $(LINKS) $(LINKS) $(LINKFLAGS)

reb2: $(OBJS_L2)
	$(CC) -o $(BINS_L2) $(OBJS_L2) $(LINKS) $(LINKS) $(LINKFLAGS)

reb3: $(OBJS_L3)
	$(CC) -o $(BINS_L3) $(OBJS_L3) $(LINKS) $(LINKS) $(LINKFLAGS)

all: reb1 reb2 reb3

clean:
	rm $(OBJS_L1) $(BINS_L1) $(OBJS_L2) $(BINS_L2) $(OBJS_L3) $(BINS_L13)
