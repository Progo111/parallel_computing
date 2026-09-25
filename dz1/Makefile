# Makefile — ДЗ №1, вариант 3 (сложение/вычитание плотных матриц).
#
# Ключевое требование ТЗ: каждый этап — отдельная цель со своими ключами,
# при этом E0 и E1 собираются ИЗ ОДНОГО И ТОГО ЖЕ исходника (kernel_e0.c),
# отличаются только флагами. Разделение символов — через -DKERNEL_TAG=<tag>.
#
# Список тегов обязан совпадать с VARIANT_LIST в src/common.h.

CC        := clang
SRCDIR    := src
OBJDIR    := build
ASMDIR    := asm
BIN       := $(OBJDIR)/bench

MCPU      := -mcpu=apple-m4
CSTD      := -std=gnu11
WARN      := -Wall -Wextra
CPPFLAGS  := -I$(SRCDIR) -D_DARWIN_C_SOURCE
LDFLAGS   := -lm

# Флаги обвязки (таймер/аллокатор/верификация/main) — на скорость ядер не влияют,
# ядра вызываются через указатель на функцию из отдельной единицы трансляции.
HARNESS_FLAGS := -O2 $(MCPU)

# --------------------------------------------------------------------------
# Реестр вариантов: источник + флаги. Синхронизирован с src/common.h.
# --------------------------------------------------------------------------
TAGS := e0_O0 e0_O1 e1_O2 e1_O3 e1_Ofast e1_ffm \
        e2_w4i1 e2_w4i2 e2_w4i4 e2_w4i8 \
        e3_u1 e3_u2 e3_u4 e3_u8 \
        e4_d0 e4_d64 e4_d128 e4_d256 e4_d512 e4_d1024 e4_d2048

NOVEC := -fno-vectorize -fno-slp-vectorize -fno-unroll-loops

SRC_e0_O0    := kernel_e0.c
FLAGS_e0_O0  := -O0 $(NOVEC)
SRC_e0_O1    := kernel_e0.c
FLAGS_e0_O1  := -O1 $(NOVEC)

SRC_e1_O2       := kernel_e0.c
FLAGS_e1_O2     := -O2 $(MCPU)
SRC_e1_O3       := kernel_e0.c
FLAGS_e1_O3     := -O3 $(MCPU) -funroll-loops
SRC_e1_Ofast    := kernel_e0.c
# -Wno-deprecated-ofast: Apple clang 17 помечает -Ofast устаревшим; вариант
# оставлен намеренно — он требуется таблицей подбора ключей §10.1 ТЗ.
FLAGS_e1_Ofast  := -Ofast -Wno-deprecated-ofast $(MCPU) -funroll-loops
SRC_e1_ffm      := kernel_e0.c
FLAGS_e1_ffm    := -O3 -ffast-math $(MCPU) -funroll-loops

SRC_e2_w4i1     := kernel_e2.c
FLAGS_e2_w4i1   := -O3 $(MCPU) -DVEC_WIDTH=4 -DINTERLEAVE=1
SRC_e2_w4i2     := kernel_e2.c
FLAGS_e2_w4i2   := -O3 $(MCPU) -DVEC_WIDTH=4 -DINTERLEAVE=2
SRC_e2_w4i4     := kernel_e2.c
FLAGS_e2_w4i4   := -O3 $(MCPU) -DVEC_WIDTH=4 -DINTERLEAVE=4
SRC_e2_w4i8     := kernel_e2.c
FLAGS_e2_w4i8   := -O3 $(MCPU) -DVEC_WIDTH=4 -DINTERLEAVE=8

SRC_e3_u1       := kernel_e3.c
FLAGS_e3_u1     := -O3 $(MCPU) -DUNROLL=1
SRC_e3_u2       := kernel_e3.c
FLAGS_e3_u2     := -O3 $(MCPU) -DUNROLL=2
SRC_e3_u4       := kernel_e3.c
FLAGS_e3_u4     := -O3 $(MCPU) -DUNROLL=4
SRC_e3_u8       := kernel_e3.c
FLAGS_e3_u8     := -O3 $(MCPU) -DUNROLL=8

SRC_e4_d0       := kernel_e4.c
FLAGS_e4_d0     := -O3 $(MCPU) -DPF_DIST=0
SRC_e4_d64      := kernel_e4.c
FLAGS_e4_d64    := -O3 $(MCPU) -DPF_DIST=64
SRC_e4_d128     := kernel_e4.c
FLAGS_e4_d128   := -O3 $(MCPU) -DPF_DIST=128
SRC_e4_d256     := kernel_e4.c
FLAGS_e4_d256   := -O3 $(MCPU) -DPF_DIST=256
SRC_e4_d512     := kernel_e4.c
FLAGS_e4_d512   := -O3 $(MCPU) -DPF_DIST=512
SRC_e4_d1024    := kernel_e4.c
FLAGS_e4_d1024  := -O3 $(MCPU) -DPF_DIST=1024
SRC_e4_d2048    := kernel_e4.c
FLAGS_e4_d2048  := -O3 $(MCPU) -DPF_DIST=2048

KERNEL_OBJS  := $(addprefix $(OBJDIR)/,$(addsuffix .o,$(TAGS)))
HARNESS_OBJS := $(OBJDIR)/alloc.o $(OBJDIR)/timer.o $(OBJDIR)/verify.o $(OBJDIR)/bench.o
ASM_FILES    := $(addprefix $(ASMDIR)/,$(addsuffix .s,$(TAGS)))

# bw_probe (STREAM-замер ПСП, §2.2.2) собирается отдельным скриптом
# scripts/bw_probe.sh — он не входит в бинарь bench и не зависит от ядер.
.PHONY: all clean asm optrec dirs
all: $(BIN)

dirs:
	@mkdir -p $(OBJDIR) $(ASMDIR) results/opt-records

$(OBJDIR) results/opt-records:
	@mkdir -p $@

$(BIN): $(KERNEL_OBJS) $(HARNESS_OBJS)
	$(CC) $(KERNEL_OBJS) $(HARNESS_OBJS) -o $@ $(LDFLAGS)

# --- обвязка ---------------------------------------------------------------
$(OBJDIR)/%.o: $(SRCDIR)/%.c $(SRCDIR)/common.h | $(OBJDIR)
	$(CC) $(CSTD) $(WARN) $(CPPFLAGS) $(HARNESS_FLAGS) -c $< -o $@

# --- ядра: по правилу на тег ----------------------------------------------
define KERNEL_RULE
$(OBJDIR)/$(1).o: $(SRCDIR)/$(SRC_$(1)) $(SRCDIR)/common.h | $(OBJDIR)
	$(CC) $(CSTD) $(WARN) $(CPPFLAGS) $(FLAGS_$(1)) -DKERNEL_TAG=$(1) \
	      -c $(SRCDIR)/$(SRC_$(1)) -o $(OBJDIR)/$(1).o

$(ASMDIR)/$(1).s: $(SRCDIR)/$(SRC_$(1)) $(SRCDIR)/common.h
	@mkdir -p $(ASMDIR)
	$(CC) $(CSTD) $(CPPFLAGS) $(FLAGS_$(1)) -DKERNEL_TAG=$(1) \
	      -S -fverbose-asm $(SRCDIR)/$(SRC_$(1)) -o $(ASMDIR)/$(1).s
endef
$(foreach t,$(TAGS),$(eval $(call KERNEL_RULE,$(t))))

asm: $(ASM_FILES)

# --- отчёты векторизатора (E1/E2) -----------------------------------------
optrec: | results/opt-records
	@for t in e1_O3 e2_w4i4 e3_u4 e4_d0; do \
	  src=$$(make -s --no-print-directory print-SRC_$$t); \
	  flg=$$(make -s --no-print-directory print-FLAGS_$$t); \
	  echo "== $$t ($$src) =="; \
	  $(CC) $(CSTD) $(CPPFLAGS) $$flg -DKERNEL_TAG=$$t \
	    -Rpass=loop-vectorize -Rpass-missed=loop-vectorize \
	    -Rpass-analysis=loop-vectorize \
	    -fsave-optimization-record \
	    -foptimization-record-file=results/opt-records/$$t.opt.yaml \
	    -c $(SRCDIR)/$$src -o /dev/null 2> results/opt-records/$$t.remarks.txt; \
	  echo "  -> results/opt-records/$$t.opt.yaml"; \
	done

print-%:
	@echo '$($*)'

clean:
	rm -rf $(OBJDIR) $(ASMDIR)/*.s
