# Host bench: BC1-BC5 -> ETC2/EAC transcoder, lowmem (old) vs bc2 (new).
# The transcoders are GLSL compute shaders written in the GLSL/C subset; their
# bodies are compiled here as C (same technique as tests/bc-host).
#   make OLD=/home/flux/src/mesa-lowmem NEW=/home/flux/src/mesa-bc
#   ./bc_bench old|new [reps] [dim]
OLD ?= /home/flux/src/mesa-lowmem
NEW ?= /home/flux/src/mesa-bc
CC ?= gcc
CFLAGS ?= -O2
H = ../../tests/bc-host
CFLAGS += -I. -I$(H) -I$(H)/ref -Wall -Wno-unused-function -Wno-unused-variable

all: bc_bench

etc2_new.h: $(NEW)/src/panfrost/vulkan/bc/panvk_bc_etc2.comp
	tail -n +2 $< > $@
etc2_old.h: $(OLD)/src/panfrost/vulkan/bc/panvk_bc_etc2.comp
	tail -n +2 $< > $@

xc_new.o: $(H)/xc_wrap.c etc2_new.h
	$(CC) $(CFLAGS) -DXC_NEW -DXC_BODY='"etc2_new.h"' -DXC_FN=xc_new -c $< -o $@
	objcopy --keep-global-symbol=xc_new --keep-global-symbol=xc_new_dec $@
xc_old.o: $(H)/xc_wrap.c etc2_old.h
	$(CC) $(CFLAGS) -DXC_BODY='"etc2_old.h"' -DXC_FN=xc_old -c $< -o $@
	objcopy --keep-global-symbol=xc_old $@

bc_bench: bc_bench.c xc_new.o xc_old.o
	$(CC) $(CFLAGS) $< xc_new.o xc_old.o -lm -o $@

clean:
	rm -f *.o etc2_new.h etc2_old.h bc_bench
