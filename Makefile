# Copyright (c) 2016, Chris Smeele.

# Package metadata.
NAME := picus

# Toolkit.
TARGET  := arm-none-eabi
CXX     := $(TARGET)-g++
LD      := $(TARGET)-g++
AR      := $(TARGET)-ar
OBJCOPY := $(TARGET)-objcopy

# Directories.
SRCDIR := ./src
OBJDIR := ./obj
BINDIR := ./bin
INCDIR := ./include

EXT_INCDIR := ./external-include
INCDIRS    :=                             \
	$(INCDIR)                             \
	$(EXT_INCDIR)                         \
	$(EXT_INCDIR)/sam/libsam/include      \
	$(EXT_INCDIR)/sam/CMSIS/CMSIS/Include \
	$(EXT_INCDIR)/sam/CMSIS/Device/ATMEL


EXT_LIBDIR := ./external-lib
LIBDIRS    := $(EXT_LIBDIR)

# Source files.
CXXFILES := $(shell find $(SRCDIR) -iname "*.cc" -print)
HXXFILES := $(shell find $(SRCDIR) $(INCDIR) -name "*.hh" -print)

# Intermediate files.
OBJFILES := $(CXXFILES:$(SRCDIR)/%.cc=$(OBJDIR)/%.o)
ELFFILE  := $(BINDIR)/$(NAME).elf

# Output files.
BINFILE := $(BINDIR)/$(NAME).bin

# Compiler flags.
WARNINGS :=              \
	all                  \
	extra                \
	pedantic             \
	shadow               \
	cast-align           \
	missing-declarations \
	redundant-decls      \
	uninitialized        \
	conversion

MACROS +=

CXXFLAGS :=                             \
	$(addprefix -W, $(WARNINGS))        \
	$(addprefix -D, $(MACROS))          \
	$(addprefix -I, $(INCDIRS))         \
	-std=c++11                          \
	-Os                                 \
	-g0                                 \
	-mcpu=cortex-m3                     \
	-mthumb                             \
	-nostdlib                           \
	-fno-rtti                           \
	-fno-exceptions

# Linker flags.
LINKFILE := flash.ld

LIBS :=                 \
	mustore             \
	sam_sam3x8e_gcc_rel \
	gcc\
	m

LDFLAGS :=                              \
	$(addprefix -L, $(LIBDIRS))         \
	-mcpu=cortex-m3                     \
	-mthumb                             \
	-mlong-calls                        \
	-Wl,--check-sections                \
	-Wl,--gc-sections                   \
	-Wl,--entry=Reset_Handler           \
	-Wl,--unresolved-symbols=report-all \
	-Wl,--warn-common                   \
	-Wl,--warn-section-align            \
	-Wl,--warn-unresolved-symbols

# Bossa flags.
BOSSAC := bossac

# These can be overridden on the commandline.
PORT     ?= ttyACM0
PORT_DEV ?= /dev/$(PORT)
BAUDRATE ?= 115200

UPLOAD_STTY_FLAGS := raw ispeed 1200 ospeed 1200 cs8 -cstopb eol 255 eof 255 1200

BOSSAFLAGS +=              \
	--port=$(PORT)         \
	--force_usb_port=false \
	--erase                \
	--write                \
	--boot                 \
	--reset
#--verify               \

.PHONY: all install upload run test clean doc

all: $(BINFILE)

install: upload

upload: $(BINFILE)
	stty -F $(PORT_DEV) $(UPLOAD_STTY_FLAGS)
	printf "\x00" > $(PORT_DEV)
	$(BOSSAC) $(BOSSAFLAGS) $(BINFILE)

run:
	stty -F $(PORT_DEV) $(BAUDRATE)
	minicom -D $(PORT_DEV) -b$(BAUDRATE)

test: upload run

doc: $(HXXFILES) $(CXXFILES) doxygen.conf
	doxygen doxygen.conf

clean:
	rm -rvf $(OBJDIR) $(BINDIR)

$(OBJDIR)/%.o: $(SRCDIR)/%.cc $(HXXFILES)
	@mkdir -p $(OBJDIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(ELFFILE): $(OBJFILES)
	@mkdir -p $(BINDIR)
	$(LD) $(LDFLAGS) -T$(LINKFILE) -o $@ -Wl,--start-group $^ $(addprefix -l, $(LIBS)) -Wl,--end-group

$(BINFILE): $(ELFFILE)
	@mkdir -p $(BINDIR)
	$(OBJCOPY) -O binary $< $@
