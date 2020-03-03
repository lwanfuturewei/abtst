TOP_DIR := $(patsubst %/,%,$(dir $(abspath $(lastword $(MAKEFILE_LIST)))))
include $(TOP_DIR)/Common.mk

third_party := argobots

lib_arch := src/arch
lib_sched := src/sched
lib_stream := src/stream
lib_load := src/load
lib_mapping := src/mapping
lib_rebalance := src/rebalance
app := src/app
libs := $(lib_arch) $(lib_sched) $(lib_stream) $(lib_load) $(lib_mapping) $(lib_rebalance)

.PHONY: all libs third_party clean $(app) $(libs) $(third_party)

all: $(app)

libs: $(libs)

third_party: $(third_party)

$(app) $(libs):
	$(MAKE)  --directory=$@ 

$(third_party):
	$(info building argobots...)
	if [ ! -d "$(ARGOBOTS_SRC)" ];then	\
		git submodule init;		\
	fi	
	cd $(ARGOBOTS_SRC);		\
	./autogen.sh;			\
	./configure --prefix=$(ARGOBOTS_BIN);	\
	make -j;	\
	make -j install;	\
	
$(app): $(libs)
$(libs): $(third_party)

clean:
	[ "$(OUT_DIR)" != "" ] && rm -rf $(OUT_DIR)/*
