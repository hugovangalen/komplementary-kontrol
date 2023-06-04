GCC=gcc
RM=rm
CP=cp
MKDIR=mkdir -p
STRIP=strip

PREFIX ?= /usr

MAPPINGS_PATH = $(PREFIX)/share/komplement
PRESETS_PATH = $(PREFIX)/share/konfigure

CFLAGS+=-DMAPPINGS_PATH="\"$(MAPPINGS_PATH)\""
CFLAGS+=-DPRESETS_PATH="\"$(PRESETS_PATH)\""

# Linking flags for `komplement` tool.
KOMPLEMENT_LFLAGS=-lhidapi-libusb -lasound

# Linker flags for `konfigure` tool.
KONFIGURE_LFLAGS=-lasound

all: komplement konfigure

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
	$(RM) -f *.o komplement konfigure

.c.o:
	$(GCC) $(CFLAGS) -c $<

alsa.o: alsa.h alsa.c defs.h

mmc_stuff.o: mmc_stuff.h mmc_stuff.c defs.h

config.o: config.h config.c uinput_stuff.h button_names.h defs.h mmc_stuff.h

mapping.o: mapping.h mapping.c defs.h

komplement.o: button_names.h komplement.c button_leds.h version.h defs.h alsa.h mmc_stuff.h

button_names.o: button_names.c button_names.h defs.h 

hid.o: hid.c hid.h

button_leds.o: button_leds.c button_leds.h defs.h hid.h

uinput_stuff.o: uinput_stuff.c uinput_stuff.h defs.h

# `komplementary` is the user space utility that translates
# HID events to keypresses.
komplement: komplement.o button_names.o uinput_stuff.o mapping.o config.o hid.o button_leds.o alsa.o mmc_stuff.o
	$(GCC) -o komplement \
		komplement.o button_names.o uinput_stuff.o alsa.o \
		mapping.o config.o hid.o button_leds.o mmc_stuff.o \
		$(KOMPLEMENT_LFLAGS)

# `konfigure` is a binary that creates SysEx to send to the
# device to configure the mappings.
konfigure.o: konfigure.c konfigure.h konfigure_parser.h version.h

konfigure_parser.o: konfigure_parser.c konfigure_parser.h

konfigure: konfigure.o konfigure_parser.o
	$(GCC) -o konfigure konfigure.o konfigure_parser.o $(KONFIGURE_LFLAGS)
