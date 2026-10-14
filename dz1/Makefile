# Каждый вариант E0..E4 — отдельный объектник: исходник может быть общим,
# но ключи компиляции разные, имена функций разводит -DKERNEL_TAG.

CC     := clang
MCPU   := -mcpu=apple-m4
CFLAGS := -std=gnu11 -Wall -Wextra -Isrc

TAGS := e0_O0 e1_O2 e1_O3 e1_Ofast \
        e2_i1 e2_i2 e2_i4 e2_i8 \
        e3_u1 e3_u2 e3_u4 e3_u8 \
        e4_p0 e4_p256 e4_p1024

FLAGS_e0_O0    := -O0 -fno-vectorize -fno-slp-vectorize -fno-unroll-loops
FLAGS_e1_O2    := -O2 $(MCPU)
FLAGS_e1_O3    := -O3 $(MCPU) -funroll-loops
FLAGS_e1_Ofast := -Ofast -Wno-deprecated-ofast $(MCPU) -funroll-loops
FLAGS_e2_i1    := -O3 $(MCPU) -DINTERLEAVE=1
FLAGS_e2_i2    := -O3 $(MCPU) -DINTERLEAVE=2
FLAGS_e2_i4    := -O3 $(MCPU) -DINTERLEAVE=4
FLAGS_e2_i8    := -O3 $(MCPU) -DINTERLEAVE=8
FLAGS_e3_u1    := -O3 $(MCPU) -DUNROLL=1
FLAGS_e3_u2    := -O3 $(MCPU) -DUNROLL=2
FLAGS_e3_u4    := -O3 $(MCPU) -DUNROLL=4
FLAGS_e3_u8    := -O3 $(MCPU) -DUNROLL=8
FLAGS_e4_p0    := -O3 $(MCPU) -DPF_DIST=0
FLAGS_e4_p256  := -O3 $(MCPU) -DPF_DIST=256
FLAGS_e4_p1024 := -O3 $(MCPU) -DPF_DIST=1024

# Исходник определяется префиксом тега; E1 собирается из того же файла, что E0.
src_of = src/kernel_$(patsubst e1,e0,$(firstword $(subst _, ,$1))).c

OBJS := $(TAGS:%=build/%.o) build/bench.o
ASM  := asm/e0_O0.s asm/e1_O3.s asm/e2_i4.s asm/e3_u4.s asm/e4_p0.s

.PHONY: all listings clean

all: build/bench

build/bench: $(OBJS)
	$(CC) $^ -o $@

build/bench.o: src/bench.c src/common.h | build
	$(CC) $(CFLAGS) -O2 $(MCPU) -c $< -o $@

define kernel_rule
build/$(1).o: $(call src_of,$(1)) src/common.h | build
	$$(CC) $$(CFLAGS) $$(FLAGS_$(1)) -DKERNEL_TAG=$(1) -c $$< -o $$@

asm/$(1).s: $(call src_of,$(1)) src/common.h | asm
	$$(CC) $$(CFLAGS) $$(FLAGS_$(1)) -DKERNEL_TAG=$(1) -S $$< -o $$@
endef
$(foreach t,$(TAGS),$(eval $(call kernel_rule,$(t))))

# Ассемблерные листинги для отчёта (по одному на этап).
listings: $(ASM)

build asm:
	mkdir -p $@

clean:
	rm -rf build asm
