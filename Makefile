TARGET = XJIRRAX_CITY

# Automatically include all C source files in repository root
OBJS = $(patsubst %.c,%.o,$(wildcard *.c))

INCDIR = -I. -Isrc
CFLAGS = -O2 -G0 -Wall -Wextra
CXXFLAGS = $(CFLAGS)
ASFLAGS = $(CFLAGS)

LIBS = -lpspgum -lpspgu -lpspge -lpspdisplay -lpspctrl -lm

BUILD_PRX = 1
PSP_FW_VERSION = 660
EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = X-JIRRAX CITY

PSPSDK = $(shell psp-config --pspsdk-path)

include $(PSPSDK)/lib/build.mak
