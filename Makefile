TOP_DIR := $(patsubst %/,%,$(dir $(abspath $(lastword $(MAKEFILE_LIST)))))
include $(TOP_DIR)/Common.mk

third_party := argobots

lib_arch := src/arch
lib_sched := src/sched
lib_stream := src/stream
lib_load := src/load
lib_mapping := src/mapping
lib_rebalance := src/rebalance
lib_partition := src/partition
lib_power := src/power
app := src/app
libs := $(lib_arch) $(lib_sched) $(lib_stream) $(lib_load) $(lib_mapping) $(lib_rebalance) $(lib_partition) $(lib_power)

.PHONY: all libs third_party clean $(app) $(libs) $(third_party)

all: $(app)

libs: $(libs)

third_party: $(third_party)

$(app) $(libs):
	$(MAKE)  --directory=$@ 

$(third_party):
	$(info building argobots with debug on...)
	cd $(ARGOBOTS_SRC);		\
	./autogen.sh;			\
	./configure --enable-debug=most --enable-fast=O0 --disable-shareda --enable-affinity --enable-sched-sleep;	\
	make -j;	\
	
$(app): $(libs)
$(libs): $(third_party)

clean:
	[ "$(OUT_DIR)" != "" ] && rm -rf $(OUT_DIR)/*
