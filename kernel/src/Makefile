src_c := $(filter-out %.mod.c,$(notdir $(wildcard $(src)/*.c)))

hostprogs-y := #test 
always := $(hostprogs-y)

hostprogs_c := $(patsubst %,%.c,$(hostprogs-y))

obj_c := $(filter-out $(hostprogs_c), $(src_c))
obj-m := $(patsubst %.c,%.o,$(obj_c))

PHONE += modules
modules:
modules clean modules_install help:
	$(MAKE) -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) $@