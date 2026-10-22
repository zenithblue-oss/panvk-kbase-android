# Host bench: BC1-BC5 -> ETC2/EAC transcoder, lowmem vs bc3 vs bc4.
# The transcoders are GLSL compute shaders written in the GLSL/C subset; their
# bodies are compiled here as C (same technique as tests/bc-host).
#   make OLD=/home/flux/src/mesa-lowmem NEW=/home/flux/src/mesa-bc
#   ./bc_bench [reps] [dim] [mode mask]
# lowmem = $(OLD) worktree, bc3 = $(NEW) at $(BC2REV) (csf-v11/203; the
# object keeps its old xc_bc2 name), bc4 = $(NEW) worktree (csf-v11/206+).
OLD ?= /home/flux/src/mesa-lowmem
NEW ?= /home/flux/src/mesa-bc
BC2REV ?= 2a99b4a
SHADER = src/panfrost/vulkan/bc/panvk_bc_etc2.comp
CC ?= gcc
CFLAGS ?= -O2
H = ../../tests/bc-host
CFLAGS += -I. -I$(H) -I$(H)/ref -Wall -Wno-unused-function -Wno-unused-variable
# Absolute XC_BODY: a quoted include is looked up next to xc_wrap.c first,
# and tests/bc-host has its own etc2_new.h.
BODY = -DXC_BODY='"$(CURDIR)/$(1)"'
# csf-v11/206+: the new body reads the EAC endpoint pair table. Set
# XC_NEW_FLAGS= for a body without eac_fast().
XC_NEW_FLAGS ?= -DXC_LUT -I$(NEW)/src/panfrost/vulkan/bc

all: bc_bench

etc2_new.h: $(NEW)/$(SHADER)
	tail -n +2 $< > $@
etc2_old.h: $(OLD)/$(SHADER)
	tail -n +2 $< > $@
etc2_bc2.h:
	git -C $(NEW) show $(BC2REV):$(SHADER) | tail -n +2 > $@

xc_new.o: $(H)/xc_wrap.c etc2_new.h
	$(CC) $(CFLAGS) -DXC_NEW $(XC_NEW_FLAGS) $(call BODY,etc2_new.h) -DXC_FN=xc_new -c $< -o $@
	objcopy --keep-global-symbol=xc_new --keep-global-symbol=xc_new_dec $@
xc_bc2.o: $(H)/xc_wrap.c etc2_bc2.h
	$(CC) $(CFLAGS) -DXC_NEW $(call BODY,etc2_bc2.h) -DXC_FN=xc_bc2 -Dxc_new_dec=xc_bc2_dec -c $< -o $@
	objcopy --keep-global-symbol=xc_bc2 $@
xc_old.o: $(H)/xc_wrap.c etc2_old.h
	$(CC) $(CFLAGS) $(call BODY,etc2_old.h) -DXC_FN=xc_old -c $< -o $@
	objcopy --keep-global-symbol=xc_old $@

bc_bench: bc_bench.c xc_new.o xc_bc2.o xc_old.o
	$(CC) $(CFLAGS) $< xc_new.o xc_bc2.o xc_old.o -lm -o $@

clean:
	rm -f *.o etc2_new.h etc2_old.h etc2_bc2.h bc_bench
