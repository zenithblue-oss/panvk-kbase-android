# Host tests of the panvk BC transcoder shaders (bodies compiled as C).
#   make MESA=/path/to/patched/mesa         build
#   make run                                run all checks
MESA ?= /home/flux/src/mesa-lowmem
BC = $(MESA)/src/panfrost/vulkan/bc
CFLAGS ?= -O2 -g
CFLAGS += -I. -Iref -Wall -Wno-unused-function -Wno-unused-variable
ASTCENC ?=

all: test_etc2 bench_etc2
astc: test_astc

etc2_new.h: $(BC)/panvk_bc_etc2.comp
	tail -n +2 $< > $@
etc2_197.h: ref/panvk_bc_etc2_197.comp
	tail -n +2 $< > $@

xc_new.o: xc_wrap.c etc2_new.h
	$(CC) $(CFLAGS) -DXC_BODY='"etc2_new.h"' -DXC_FN=xc_new -c $< -o $@
	objcopy --keep-global-symbol=xc_new $@
xc_old.o: xc_wrap.c etc2_197.h
	$(CC) $(CFLAGS) -DXC_BODY='"etc2_197.h"' -DXC_FN=xc_old -c $< -o $@
	objcopy --keep-global-symbol=xc_old $@

bench_etc2: bench_etc2.c xc_new.o xc_old.o pics.h
	$(CC) $(CFLAGS) $< xc_new.o xc_old.o -lm -o $@

test_etc2: test_etc2.c etc2_new.h pics.h
	$(CC) $(CFLAGS) $< -lm -o $@

# BC6H -> ASTC HDR test: needs the ARM astc-encoder sources (ASTCENC=dir of
# a checkout, e.g. tag 4.8.0); the core codec is built into libastcenc.a.
ASTCENC_SRC = $(filter-out %cli_entry.cpp,$(wildcard $(ASTCENC)/Source/astcenc_*.cpp))
libastcenc.a: $(ASTCENC_SRC)
	@test -n "$(ASTCENC)" || { echo "set ASTCENC=<astc-encoder checkout>"; exit 1; }
	rm -rf astcenc-obj && mkdir astcenc-obj
	cd astcenc-obj && for f in $(ASTCENC_SRC); do \
	   $(CXX) -O2 -std=c++14 -DASTCENC_NEON=1 -DASTCENC_SSE=0 -DASTCENC_AVX=0 \
	      -DASTCENC_POPCNT=0 -DASTCENC_F16C=0 -c $$f || exit 1; done
	ar rcs $@ astcenc-obj/*.o

MESA_DEFS = -DHAVE_PTHREAD -D_GNU_SOURCE -DHAVE_ENDIAN_H -I$(MESA)/src -I$(MESA)/include
astc_ref.o: astc_ref.cpp libastcenc.a
	$(CXX) -O2 -std=c++14 -I$(ASTCENC)/Source -c $< -o $@
test_astc: test_astc.c $(BC)/panvk_bc_astc_enc.h astc_ref.o libastcenc.a
	$(CC) $(CFLAGS) -I$(BC) $(MESA_DEFS) $< $(MESA)/src/util/half_float.c \
	   astc_ref.o libastcenc.a -lstdc++ -lm -o $@

run: all
	./test_etc2
	./bench_etc2

clean:
	rm -rf *.o etc2_new.h etc2_197.h test_etc2 bench_etc2 test_astc libastcenc.a astcenc-obj
