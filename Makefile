CC=gcc
RM=rm
CP=cp
MKDIR=mkdir -p
STRIP=strip

PREFIX ?= /usr

SRCDIR = src
BUILDDIR = obj

vpath %.c $(SRCDIR)

MAPPINGS_PATH = $(PREFIX)/share/komplementary-kontrol/mappings
PRESETS_PATH = $(PREFIX)/share/komplementary-kontrol/presets

CFLAGS+=-Wall
CFLAGS+=-DMAPPINGS_PATH="\"$(MAPPINGS_PATH)\""
CFLAGS+=-DPRESETS_PATH="\"$(PRESETS_PATH)\""

# Linking flags for `komplement` tool.
KOMPLEMENT_LFLAGS=-lhidapi-libusb -lasound

# Linker flags for `konfigure` tool.
KONFIGURE_LFLAGS=-lasound

KOMPLEMENT_SOURCES=$(SRCDIR)/komplement.c $(SRCDIR)/button_names.c $(SRCDIR)/uinput_stuff.c\
	$(SRCDIR)/mapping.c $(SRCDIR)/config.c $(SRCDIR)/hid.c $(SRCDIR)/button_leds.c\
	$(SRCDIR)/alsa.c $(SRCDIR)/mmc_stuff.c

KONFIGURE_SOURCES=$(SRCDIR)/konfigure.c $(SRCDIR)/konfigure_parser.c

KOMPLEMENT_OBJECTS=$(KOMPLEMENT_SOURCES:src/%.c=obj/%.o)
KONFIGURE_OBJECTS=$(KONFIGURE_SOURCES::src/%.c=obj/%.o)

all: checkdir komplement konfigure

checkdir: $(BUILDDIR)

$(BUILDDIR):
	$(MKDIR) $@

install: komplement konfigure
	$(MKDIR) $(MAPPINGS_PATH)
	$(CP) mappings/*.map $(MAPPINGS_PATH)
	$(MKDIR) $(PRESETS_PATH)
	$(CP) presets/*.pst $(PRESETS_PATH)
	$(STRIP) komplement
	$(STRIP) konfigure
	$(CP) komplement $(PREFIX)/bin/
	$(CP) konfigure $(PREFIX)/bin/

uninstall:
	$(RM) -f $(PREFIX)/bin/komplement
	$(RM) -f $(PREFIX)/bin/konfigure
	@echo "NOTE: The files in $(MAPPINGS_PATH) and $(PRESETS_PATH) have not been deleted."

clean:
	$(RM) -f $(BUILDDIR)/*.o komplement konfigure

$(BUILDDIR)/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/alsa.o: $(SRCDIR)/alsa.h $(SRCDIR)/alsa.c $(SRCDIR)/defs.h

$(BUILDDIR)/mmc_stuff.o: $(SRCDIR)/mmc_stuff.h $(SRCDIR)/mmc_stuff.c $(SRCDIR)/defs.h

$(BUILDDIR)/config.o: $(SRCDIR)/config.h $(SRCDIR)/config.c $(SRCDIR)/uinput_stuff.h $(SRCDIR)/button_names.h $(SRCDIR)/defs.h $(SRCDIR)/mmc_stuff.h

$(BUILDDIR)/mapping.o: $(SRCDIR)/mapping.h $(SRCDIR)/mapping.c $(SRCDIR)/defs.h

$(BUILDDIR)/komplement.o: $(SRCDIR)/button_names.h $(SRCDIR)/komplement.c $(SRCDIR)/button_leds.h $(SRCDIR)/version.h $(SRCDIR)/defs.h $(SRCDIR)/alsa.h $(SRCDIR)/mmc_stuff.h

$(BUILDDIR)/button_names.o: $(SRCDIR)/button_names.c $(SRCDIR)/button_names.h $(SRCDIR)/defs.h 

$(BUILDDIR)/hid.o: $(SRCDIR)/hid.c $(SRCDIR)/hid.h

$(BUILDDIR)/button_leds.o: $(SRCDIR)/button_leds.c $(SRCDIR)/button_leds.h $(SRCDIR)/defs.h $(SRCDIR)/hid.h

$(BUILDDIR)/uinput_stuff.o: $(SRCDIR)/uinput_stuff.c $(SRCDIR)/uinput_stuff.h $(SRCDIR)/defs.h

# `komplementary` is the user space utility that translates
# HID events to keypresses.
komplement: $(KOMPLEMENT_OBJECTS)
	$(CC) -o komplement $(KOMPLEMENT_OBJECTS) $(KOMPLEMENT_LFLAGS)

# `konfigure` is a binary that creates SysEx to send to the
# device to configure the mappings.
$(BUILDDIR)/konfigure.o: $(SRCDIR)/konfigure.c $(SRCDIR)/konfigure.h $(SRCDIR)/konfigure_parser.h $(SRCDIR)/version.h

$(BUILDDIR)/konfigure_parser.o: $(SRCDIR)/konfigure_parser.c $(SRCDIR)/konfigure_parser.h

konfigure: $(KONFIGURE_OBJECTS)
	$(CC) -o konfigure $(KONFIGURE_OBJECTS) $(KONFIGURE_LFLAGS)
