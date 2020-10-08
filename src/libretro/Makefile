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

ifneq (,$(findstring unix,$(platform)))
	EXT ?= so
   TARGET := $(TARGET_NAME)_libretro.$(EXT)
   fpic := -fPIC
   SHARED := -shared -Wl,--version-script=$(CORE_DIR)/link.T -Wl,--no-undefined
   LIBS := -ldl -lpthread
   COMMON_CFLAGS += -Dstricmp=strcasecmp -Dstrnicmp=strncasecmp
else ifeq ($(platform), linux-portable)
   TARGET := $(TARGET_NAME)_libretro.$(EXT)
   fpic := -fPIC -nostdlib
   SHARED := -shared -Wl,--version-script=$(CORE_DIR)/link.T
	LIBM :=
   COMMON_CFLAGS += -Dstricmp=strcasecmp -Dstrnicmp=strncasecmp
else ifneq (,$(findstring osx,$(platform)))
   TARGET := $(TARGET_NAME)_libretro.dylib
   fpic := -fPIC
   SHARED := -dynamiclib
   COMMON_CFLAGS += -Dstricmp=strcasecmp -Dstrnicmp=strncasecmp
else ifneq (,$(findstring ios,$(platform)))
   TARGET := $(TARGET_NAME)_libretro_ios.dylib
	fpic := -fPIC
	SHARED := -dynamiclib
   COMMON_CFLAGS += -Dstricmp=strcasecmp -Dstrnicmp=strncasecmp
   ifeq ($(IOSSDK),)
      IOSSDK := $(shell xcodebuild -version -sdk iphoneos Path)
   endif
   DEFINES := -DIOS
   CC = cc -arch arm64 -isysroot $(IOSSDK)
   CC += -miphoneos-version-min=8.0
   CXXFLAGS += -miphoneos-version-min=8.0
else ifeq ($(platform), tvos-arm64)
	TARGET := $(TARGET_NAME)_libretro_tvos.dylib
	fpic := -fPIC
	SHARED := -dynamiclib
	CFLAGS += -DIOS
	ifeq ($(IOSSDK),)
		IOSSDK := $(shell xcodebuild -version -sdk appletvos Path)
	endif
   CC = cc -arch arm64 -isysroot $(IOSSDK)
   CC += -mtvos-version-min=11.0
else ifneq (,$(findstring qnx,$(platform)))
	TARGET := $(TARGET_NAME)_libretro_qnx.so
   fpic := -fPIC
   COMMON_CFLAGS += -Dstricmp=strcasecmp -Dstrnicmp=strncasecmp
   SHARED := -shared -Wl,--version-script=$(CORE_DIR)/link.T -Wl,--no-undefined
   CC = qcc -Vgcc_ntoarmv7le
   CXX = QCC -Vgcc_ntoarmv7le
else ifeq ($(platform), emscripten)
   TARGET := $(TARGET_NAME)_libretro_emscripten.bc
   fpic := -fPIC
   SHARED := -shared -Wl,--version-script=$(CORE_DIR)/link.T -Wl,--no-undefined
   COMMON_CFLAGS += -Dstricmp=strcasecmp -Dstrnicmp=strncasecmp
else ifeq ($(platform), ps2)
   TARGET := $(TARGET_NAME)_ps2.a
   CC = ee-gcc
   CXX = ee-g++
   AR = ee-ar
   COMMON_CFLAGS += -DPS2 -G0 -D__sun
   COMMON_CFLAGS += -Dstricmp=strcasecmp -Dstrnicmp=strncasecmp
   STATIC_LINKING = 1
else ifeq ($(platform), psp1)
   TARGET := $(TARGET_NAME)_libretro_psp1.a
   CC = psp-gcc
   CXX = psp-g++
   AR = psp-ar
   COMMON_CFLAGS += -DPSP -G0
   COMMON_CFLAGS += -I$(shell psp-config --pspsdk-path)/include
   COMMON_CFLAGS += -Dstricmp=strcasecmp -Dstrnicmp=strncasecmp
   STATIC_LINKING = 1
else ifeq ($(platform), vita)
   TARGET := $(TARGET_NAME)_libretro_vita.a
   CC = arm-vita-eabi-gcc
   CXX = arm-vita-eabi-g++
   AR = arm-vita-eabi-ar
   DEFINES += -DVITA=1
   COMMON_CFLAGS += $(DEFINES)  -Dstricmp=strcasecmp -Dstrnicmp=strncasecmp
   CXXFLAGS += -Wl,-q -Wall -O2
   STATIC_LINKING = 1
# Nintendo WiiU
else ifeq ($(platform), wiiu)
   TARGET := $(TARGET_NAME)_libretro_$(platform).a
   CC = $(DEVKITPPC)/bin/powerpc-eabi-gcc$(EXE_EXT)
   CXX = $(DEVKITPPC)/bin/powerpc-eabi-g++$(EXE_EXT)
   AR = $(DEVKITPPC)/bin/powerpc-eabi-ar$(EXE_EXT)
   PLATFORM_DEFINES := -DGEKKO -DHW_RVL -DWIIU -mcpu=750 -meabi -mhard-float
   PLATFORM_DEFINES += -Dstricmp=strcasecmp -Dstrnicmp=strncasecmp
   COMMON_CFLAGS += $(PLATFORM_DEFINES)
   STATIC_LINKING=1
else ifeq ($(platform), wii)
   EXT=a
   TARGET := $(TARGET_NAME)_libretro_$(platform).$(EXT)
   CC = $(DEVKITPPC)/bin/powerpc-eabi-gcc$(EXE_EXT)
   CXX = $(DEVKITPPC)/bin/powerpc-eabi-g++$(EXE_EXT)
   AR = $(DEVKITPPC)/bin/powerpc-eabi-ar$(EXE_EXT)
   COMMON_CFLAGS += -DGEKKO -DHW_RVL -mrvl -mcpu=750 -meabi -mhard-float
   COMMON_CFLAGS += -Dstricmp=strcasecmp -Dstrnicmp=strncasecmp -DGAMECUBE
   CXXFLAGS += -Wl,-q -Wall -O2
   STATIC_LINKING = 1
else ifeq ($(platform), ngc)
   EXT=a
   TARGET := $(TARGET_NAME)_libretro_$(platform).$(EXT)
   CC = $(DEVKITPPC)/bin/powerpc-eabi-gcc$(EXE_EXT)
   CXX = $(DEVKITPPC)/bin/powerpc-eabi-g++$(EXE_EXT)
   AR = $(DEVKITPPC)/bin/powerpc-eabi-ar$(EXE_EXT)
   COMMON_CFLAGS += -DGEKKO -DHW_DOL -mogc -mcpu=750 -meabi -mhard-float
   COMMON_CFLAGS += -Dstricmp=strcasecmp -Dstrnicmp=strncasecmp
   CXXFLAGS += -Wl,-q -Wall -O2
   STATIC_LINKING = 1
# Lightweight PS3 Homebrew SDK
else ifeq ($(platform), psl1ght)
   EXT=a
   TARGET := $(TARGET_NAME)_libretro_$(platform).$(EXT)
   CC = $(PS3DEV)/ppu/bin/ppu-gcc$(EXE_EXT)
   CXX = $(PS3DEV)/ppu/bin/ppu-g++$(EXE_EXT)
   CC_AS = $(PS3DEV)/ppu/bin/ppu-gcc$(EXE_EXT)
   AR = $(PS3DEV)/ppu/bin/ppu-ar$(EXE_EXT)
   COMMON_CFLAGS += -Dstricmp=strcasecmp -Dstrnicmp=strncasecmp -D__CELLOS_LV2__ -D__PSL1GHT__ -mcpu=cell
   STATIC_LINKING = 1
# Nintendo Switch (libnx)
else ifeq ($(platform), libnx)
    include $(DEVKITPRO)/libnx/switch_rules
    EXT=a
    TARGET := $(TARGET_NAME)_libretro_$(platform).$(EXT)
    DEFINES := -DSWITCH=1 -D__SWITCH__=1 -DHAVE_LIBNX=1
    COMMON_CFLAGS	:=	 $(DEFINES) -g -fPIE -I$(LIBNX)/include/ -I$(PORTLIBS)/include/ -ffunction-sections -fdata-sections -ftls-model=local-exec
    COMMON_CFLAGS += $(INCDIRS) -I$(CORE_DIR)/zlib -I$(CORE_DIR)/libogg/include -I$(CORE_DIR)/libvorbis/include \
		-I$(CORE_DIR)/jpeg-8c -fvisibility=hidden
    COMMON_CFLAGS	+= -march=armv8-a -mtune=cortex-a57 -mtp=soft -mcpu=cortex-a57+crc+fp+simd -Dstricmp=strcasecmp -Dstrnicmp=strncasecmp
    STATIC_LINKING = 1
# CTR (3DS)
else ifeq ($(platform), ctr)
	TARGET := $(TARGET_NAME)_libretro_$(platform).a
	CC = $(DEVKITARM)/bin/arm-none-eabi-gcc$(EXE_EXT)
	CXX = $(DEVKITARM)/bin/arm-none-eabi-g++$(EXE_EXT)
	AR = $(DEVKITARM)/bin/arm-none-eabi-ar$(EXE_EXT)
	DEFINES += -D_3DS -DARM11 -march=armv6k -mtune=mpcore -mfloat-abi=hard
	COMMON_CFLAGS += $(DEFINES) -Dstricmp=strcasecmp -Dstrnicmp=strncasecmp
	STATIC_LINKING = 1
else
   CC ?= gcc
   TARGET := $(TARGET_NAME)_libretro.dll
   COMMON_CFLAGS += -DUSE_WINDOWS_DWORD
   SHARED := -shared -static-libgcc -static-libstdc++ -s -Wl,--version-script=$(CORE_DIR)/link.T -Wl,--no-undefined
   LDFLAGS += -lwsock32 -lws2_32
endif

HAVE_OPENGL = 1

LDFLAGS += $(LIBM)

ifeq ($(DEBUG), 1)
   COMMON_CFLAGS += -O0 -g -DDEBUG
else
   COMMON_CFLAGS += -O2
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

COMMON_CFLAGS   += -Wall -DLIBRETRO -D__LIBRETRO__ $(fpic) -DCORE=ON -DSDL2=ON -DBASE=ON -DUSE_FILE32API -ffast-math \
			-fno-strict-aliasing -fvisibility=hidden -fomit-frame-pointer \
			-Wno-sign-compare -Wno-switch -Wno-format-security -I$(TOPDIR) -I$(TOPDIR)/sys/libretro/config
CXXFLAGS += -fpermissive -std=gnu++98

ifneq ($(platform), emscripten)
COMMON_CFLAGS += -fno-unsafe-math-optimizations
endif

COMMON_CFLAGS += -DNO_STORE

CFLAGS += $(COMMON_CFLAGS)
CXXFLAGS += $(COMMON_CFLAGS)

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
	$(Q)$(CXX) $(CXXFLAGS) $(fpic) -c -o $@ $<

%.o: %.c
	@$(if $(Q), $(shell echo echo CC $<),)
	$(Q)$(CC) $(CFLAGS) $(fpic) -c -o $@ $<

clean:
	rm -f $(OBJECTS) $(TARGET)

.PHONY: clean

print-%:
	@echo '$*=$($*)'
