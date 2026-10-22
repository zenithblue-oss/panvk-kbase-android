# Host tests of the BC6H/BC7 decoder shaders (csf-v11/210-211): the GLSL
# runs as C++ through glsl2cpp.py + glsl_shim.hpp, against the decoders it
# replaces and Mesa's CPU BPTC decoder.
#   make MESA=/home/flux/src/mesa-bc run
MESA ?= /home/flux/src/mesa-bc
BC = $(MESA)/src/panfrost/vulkan/bc
CXXFLAGS ?= -O2
CXXFLAGS += -std=c++20 -fwrapv -Wno-attributes -I. -I$(O)
O ?= out
REF = -DHAVE_PTHREAD -DUTIL_ARCH_LITTLE_ENDIAN=1 -DUTIL_ARCH_BIG_ENDIAN=0 \
      -include stdlib.h -I$(MESA)/src -I$(MESA)/include

all: $(O)/test_bc6 $(O)/test_bc7

$(O)/ref_bptc.o: ref_bptc.c
	@mkdir -p $(O)
	gcc -O2 $(REF) -c $< -o $@
$(O)/half_float.o: $(MESA)/src/util/half_float.c
	@mkdir -p $(O)
	gcc -O2 $(REF) -c $< -o $@
$(O)/softfloat.o: $(MESA)/src/util/softfloat.c
	@mkdir -p $(O)
	gcc -O2 $(REF) -c $< -o $@
REFO = $(O)/ref_bptc.o $(O)/half_float.o $(O)/softfloat.o

$(O)/bc6tex.cpp: $(BC)/panvk_bc_bc6.comp glsl2cpp.py
	@mkdir -p $(O)
	python3 glsl2cpp.py $< bc6tex > $@
$(O)/bc6blk.cpp: $(BC)/panvk_bc_bc6.comp glsl2cpp.py
	@mkdir -p $(O)
	python3 glsl2cpp.py $< bc6blk PANVK_BC_BLOCK > $@
$(O)/test_bc6: test_bc6.cpp $(O)/bc6tex.cpp $(O)/bc6blk.cpp glsl_shim.hpp $(REFO)
	g++ $(CXXFLAGS) $< $(REFO) -lm -o $@

$(O)/bc7tex.cpp: $(BC)/panvk_bc_bc7.comp glsl2cpp.py
	@mkdir -p $(O)
	python3 glsl2cpp.py $< bc7tex > $@
$(O)/bc7fast.cpp: $(BC)/panvk_bc_bc7.comp glsl2cpp.py
	@mkdir -p $(O)
	python3 glsl2cpp.py $< bc7fast PANVK_BC_FAST > $@
$(O)/test_bc7: test_bc7.cpp $(O)/bc7tex.cpp $(O)/bc7fast.cpp glsl_shim.hpp $(REFO)
	g++ $(CXXFLAGS) $< $(REFO) -lm -o $@

run: all
	$(O)/test_bc6
	$(O)/test_bc7

clean:
	rm -rf $(O)
.PHONY: all run clean
