STATIC_LINKING := 0
AR             := ar
NO_STORE       := 1

ifneq ($(V),1)
   Q := @
endif

ifneq ($(SANITIZER),)
   CFLAGS   := -fsanitize=$(SANITIZER) $(CFLAGS)
   CXXFLAGS := -fsanitize=$(SANITIZER) $(CXXFLAGS)
   LDFLAGS  := -fsanitize=$(SANITIZER) $(LDFLAGS)
endif

ifeq ($(platform),)
platform = unix
ifeq ($(shell uname -a),)
   platform = win
else ifneq ($(findstring MINGW,$(shell uname -a)),)
   platform = win
else ifneq ($(findstring Darwin,$(shell uname -a)),)
   platform = osx
else ifneq ($(findstring win,$(shell uname -a)),)
   platform = win
endif
endif

# system platform
system_platform = unix
ifeq ($(shell uname -a),)
	EXE_EXT = .exe
	system_platform = win
else ifneq ($(findstring Darwin,$(shell uname -a)),)
	system_platform = osx
	arch = intel
ifeq ($(shell uname -p),powerpc)
	arch = ppc
endif
else ifneq ($(findstring MINGW,$(shell uname -a)),)
	system_platform = win
endif

CORE_DIR    += ../..
TARGET_NAME := ecwolf
LIBM		    = -lm
DISABLE_ADLIB = 0

ifeq ($(ARCHFLAGS),)
ifeq ($(archs),ppc)
   ARCHFLAGS = -arch ppc -arch ppc64
else
   ARCHFLAGS = -arch i386 -arch x86_64
endif
endif

ifeq ($(platform), osx)
ifndef ($(NOUNIVERSAL))
   CXXFLAGS += $(ARCHFLAGS)
   LFLAGS += $(ARCHFLAGS)
endif
endif

ifeq ($(STATIC_LINKING), 1)
EXT := a
endif

ifeq ($(platform), unix)
	EXT ?= so
   TARGET := $(TARGET_NAME)_libretro.$(EXT)
   fpic := -fPIC
   SHARED := -shared -Wl,--version-script=$(CORE_DIR)/link.T -Wl,--no-undefined
   LIBS := -ldl -lpthread

	CFLAGS += -Dstricmp=strcasecmp -Dstrnicmp=strncasecmp
	CXXFLAGS += -Dstricmp=strcasecmp -Dstrnicmp=strncasecmp
else ifeq ($(platform), linux-portable)
   TARGET := $(TARGET_NAME)_libretro.$(EXT)
   fpic := -fPIC -nostdlib
   SHARED := -shared -Wl,--version-script=$(CORE_DIR)/link.T
	LIBM :=
	CFLAGS += -Dstricmp=strcasecmp -Dstrnicmp=strncasecmp
	CXXFLAGS += -Dstricmp=strcasecmp -Dstrnicmp=strncasecmp
else ifneq (,$(findstring osx,$(platform)))
   TARGET := $(TARGET_NAME)_libretro.dylib
   fpic := -fPIC
   SHARED := -dynamiclib
	CFLAGS += -Dstricmp=strcasecmp -Dstrnicmp=strncasecmp
	CXXFLAGS += -Dstricmp=strcasecmp -Dstrnicmp=strncasecmp
else ifneq (,$(findstring ios,$(platform)))
   TARGET := $(TARGET_NAME)_libretro_ios.dylib
	fpic := -fPIC
	SHARED := -dynamiclib
	CFLAGS += -Dstricmp=strcasecmp -Dstrnicmp=strncasecmp
	CXXFLAGS += -Dstricmp=strcasecmp -Dstrnicmp=strncasecmp

ifeq ($(IOSSDK),)
   IOSSDK := $(shell xcodebuild -version -sdk iphoneos Path)
endif

	DEFINES := -DIOS
	CC = cc -arch armv7 -isysroot $(IOSSDK)
ifeq ($(platform),ios9)
CC     += -miphoneos-version-min=8.0
CXXFLAGS += -miphoneos-version-min=8.0
else
CC     += -miphoneos-version-min=5.0
CXXFLAGS += -miphoneos-version-min=5.0
endif
else ifneq (,$(findstring qnx,$(platform)))
	TARGET := $(TARGET_NAME)_libretro_qnx.so
   fpic := -fPIC
   SHARED := -shared -Wl,--version-script=$(CORE_DIR)/link.T -Wl,--no-undefined
else ifeq ($(platform), emscripten)
   TARGET := $(TARGET_NAME)_libretro_emscripten.bc
   fpic := -fPIC
   SHARED := -shared -Wl,--version-script=$(CORE_DIR)/link.T -Wl,--no-undefined
   CFLAGS += -Dstricmp=strcasecmp -Dstrnicmp=strncasecmp
   CXXFLAGS += -Dstricmp=strcasecmp -Dstrnicmp=strncasecmp
else ifeq ($(platform), ps2)
   TARGET := $(TARGET_NAME)_ps2.a
   CC = ee-gcc
   CXX = ee-g++
   AR = ee-ar
   CFLAGS += -DPS2 -G0 -D__sun
   CXXFLAGS += -DPS2 -G0 -D__sun
   CFLAGS += -Dstricmp=strcasecmp -Dstrnicmp=strncasecmp
	CXXFLAGS += -Dstricmp=strcasecmp -Dstrnicmp=strncasecmp
   STATIC_LINKING = 1
else ifeq ($(platform), psp1)
   TARGET := $(TARGET_NAME)_psp1.a
   CC = psp-gcc
   CXX = psp-g++
   AR = psp-ar
   CFLAGS += -DPSP -G0 -D__sun
   CXXFLAGS += -DPSP -G0 -D__sun
   CFLAGS += -I$(shell psp-config --pspsdk-path)/include
   CFLAGS += -Dstricmp=strcasecmp -Dstrnicmp=strncasecmp
	CXXFLAGS += -Dstricmp=strcasecmp -Dstrnicmp=strncasecmp
   STATIC_LINKING = 1
else ifeq ($(platform), vita)
   TARGET := $(TARGET_NAME)_libretro_vita.a
   CC = arm-vita-eabi-gcc
   CXX = arm-vita-eabi-g++
   AR = arm-vita-eabi-ar
   DEFINES += -DVITA=1
   CFLAGS += $(DEFINES)  -Dstricmp=strcasecmp -Dstrnicmp=strncasecmp
   CXXFLAGS += $(CFLAGS) -Wl,-q -Wall -O2
   STATIC_LINKING = 1
else ifeq ($(platform), wii)
   EXT=a
   TARGET := $(TARGET_NAME)_libretro_$(platform).$(EXT)
   CC = $(DEVKITPPC)/bin/powerpc-eabi-gcc$(EXE_EXT)
   CXX = $(DEVKITPPC)/bin/powerpc-eabi-g++$(EXE_EXT)
   AR = $(DEVKITPPC)/bin/powerpc-eabi-ar$(EXE_EXT)
   CFLAGS += -DGEKKO -DHW_RVL -mrvl -mcpu=750 -meabi -mhard-float -DMSB_FIRST
   CFLAGS += -Dstricmp=strcasecmp -Dstrnicmp=strncasecmp -DGAMECUBE
   CXXFLAGS += $(CFLAGS) -Wl,-q -Wall -O2
   STATIC_LINKING = 1
else ifeq ($(platform), ngc)
   EXT=a
   TARGET := $(TARGET_NAME)_libretro_$(platform).$(EXT)
   CC = $(DEVKITPPC)/bin/powerpc-eabi-gcc$(EXE_EXT)
   CXX = $(DEVKITPPC)/bin/powerpc-eabi-g++$(EXE_EXT)
   AR = $(DEVKITPPC)/bin/powerpc-eabi-ar$(EXE_EXT)
   CFLAGS += -DGEKKO -DHW_RVL -mrvl -mcpu=750 -meabi -mhard-float -DMSB_FIRST
   CFLAGS += -Dstricmp=strcasecmp -Dstrnicmp=strncasecmp -DWII
   CXXFLAGS += $(CFLAGS) -Wl,-q -Wall -O2
   STATIC_LINKING = 1
# Nintendo Switch (libnx)
else ifeq ($(platform), libnx)
    include $(DEVKITPRO)/libnx/switch_rules
    EXT=a
    TARGET := $(TARGET_NAME)_libretro_$(platform).$(EXT)
    DEFINES := -DSWITCH=1 -D__SWITCH__=1 -DHAVE_LIBNX=1
    CFLAGS	:=	 $(DEFINES) -g -fPIE -I$(LIBNX)/include/ -I$(PORTLIBS)/include/ -ffunction-sections -fdata-sections -ftls-model=local-exec
    CFLAGS += $(INCDIRS) -I$(CORE_DIR)/zlib -I$(CORE_DIR)/libogg/include -I$(CORE_DIR)/libvorbis/include \
		-I$(CORE_DIR)/jpeg-8c -fvisibility=hidden
    CFLAGS	+= -march=armv8-a -mtune=cortex-a57 -mtp=soft -mcpu=cortex-a57+crc+fp+simd -Dstricmp=strcasecmp -Dstrnicmp=strncasecmp
    CXXFLAGS := $(ASFLAGS) $(CFLAGS) -std=gnu++11
    STATIC_LINKING = 1
# CTR (3DS)
else ifeq ($(platform), ctr)
	TARGET := $(TARGET_NAME)_libretro_$(platform).a
	CC = $(DEVKITARM)/bin/arm-none-eabi-gcc$(EXE_EXT)
	CXX = $(DEVKITARM)/bin/arm-none-eabi-g++$(EXE_EXT)
	AR = $(DEVKITARM)/bin/arm-none-eabi-ar$(EXE_EXT)
	DEFINES += -D_3DS -DARM11 -march=armv6k -mtune=mpcore -mfloat-abi=hard
  CFLAGS += $(DEFINES) -Dstricmp=strcasecmp -Dstrnicmp=strncasecmp
	CXXFLAGS += $(CFLAGS) -std=gnu++11
	STATIC_LINKING = 1
else
   CC = gcc
   TARGET := $(TARGET_NAME)_libretro.dll
	CFLAGS += -DUSE_WINDOWS_DWORD
	CXXFLAGS += -DUSE_WINDOWS_DWORD
   SHARED := -shared -static-libgcc -static-libstdc++ -s -Wl,--version-script=$(CORE_DIR)/link.T -Wl,--no-undefined
   LDFLAGS += -lwsock32 -lWS2_32
endif

HAVE_OPENGL = 1

LDFLAGS += $(LIBM)

ifeq ($(DEBUG), 1)
   CFLAGS += -O0 -g -DDEBUG
   CXXFLAGS += -O0 -g -DDEBUG
else
   CFLAGS += -O2
   CXXFLAGS += -O2
endif

include Makefile.common

OBJECTS := $(SOURCES_C:.c=.o) $(SOURCES_CXX:.cpp=.o)

COMPILE_PLATFORM=$(shell uname | sed -e 's/_.*//' | tr '[:upper:]' '[:lower:]' | sed -e 's/\//_/g')
COMPILE_ARCH=$(shell uname -m | sed -e 's/i.86/x86/' | sed -e 's/^arm.*/arm/')

ifeq ($(COMPILE_PLATFORM),cygwin)
  PLATFORM=mingw32
endif

ifndef PLATFORM
PLATFORM=$(COMPILE_PLATFORM)
endif
export PLATFORM

ifeq ($(PLATFORM),mingw32)
  MINGW=1
endif
ifeq ($(PLATFORM),mingw64)
  MINGW=1
endif

ifeq ($(COMPILE_ARCH),i86pc)
  COMPILE_ARCH=x86
endif

ifeq ($(COMPILE_ARCH),amd64)
  COMPILE_ARCH=x86_64
endif
ifeq ($(COMPILE_ARCH),x64)
  COMPILE_ARCH=x86_64
endif

ifeq ($(COMPILE_ARCH),powerpc)
  COMPILE_ARCH=ppc
endif
ifeq ($(COMPILE_ARCH),powerpc64)
  COMPILE_ARCH=ppc64
endif

ifeq ($(COMPILE_ARCH),axp)
  COMPILE_ARCH=alpha
endif

TOPDIR ?= $(CURDIR)

CFLAGS   += -Wall -DLIBRETRO -D__LIBRETRO__ $(fpic) -DCORE=ON -DSDL2=ON -DBASE=ON -DUSE_FILE32API -ffast-math \
			-fno-strict-aliasing -fvisibility=hidden -fomit-frame-pointer \
			-Wno-sign-compare -Wno-switch -Wno-format-security -I$(TOPDIR) -I$(TOPDIR)/sys/libretro/config
CXXFLAGS += -Wall -DLIBRETRO -D__LIBRETRO__ $(fpic) -fpermissive -std=gnu++11

ifneq ($(platform), emscripten)
CFLAGS += -fno-unsafe-math-optimizations
endif

CFLAGS += -DNO_STORE
CXXFLAGS += -DNO_STORE

ifeq ($(D3XP),ON)
	CFLAGS	+=	-I$(TOPDIR)/d3xp -D_D3XP
else
	CFLAGS	+=	-I$(TOPDIR)/game
endif

CFLAGS     += $(INCFLAGS)
CXXFLAGS   += $(INCFLAGS)

all: $(TARGET)

$(TARGET): $(OBJECTS)
ifeq ($(STATIC_LINKING), 1)
	$(AR) rcs $@ $(OBJECTS)
else
	@$(if $(Q), $(shell echo echo LD $@),)
	$(Q)$(CXX) $(fpic) $(SHARED) -o $@ $(OBJECTS) $(LDFLAGS) $(LIBS)
endif

%.o: %.cpp
	@$(if $(Q), $(shell echo echo CC $<),)
	$(Q)$(CXX) $(CFLAGS) $(CXXFLAGS) $(fpic) -c -o $@ $<

%.o: %.c
	@$(if $(Q), $(shell echo echo CC $<),)
	$(Q)$(CC) $(CFLAGS) $(fpic) -c -o $@ $<

clean:
	rm -f $(OBJECTS) $(TARGET)

.PHONY: clean

print-%:
	@echo '$*=$($*)'
