# Makefile	-- Thatcher Ulrich <tu@tulrich.com> 2003

# This source code has been donated to the Public Domain.  Do
# whatever you want with it.

# Makefile for font_output program

TOP = ../..
include $(TOP)/config

INCLUDE_PATHS := $(ENGINE_INCLUDE) -I/usr/include/freetype2

CFLAGS := $(CFLAGS) $(INCLUDE_PATHS) $(CC_DEBUG_FLAGS)
LDFLAGS := $(LIB_DEBUG_FLAGS) $(LDFLAGS) -lfreetype

LIBS := $(LIB_OUT) $(BASE_LIB) $(LIBS) $(ZLIB)


all: font_output$(EXE_EXT)


font_output$(EXE_EXT): font_output.$(OBJ_EXT) $(BASE_LIB)
	$(CC) -o $@ $^ $(LIBS) $(LDFLAGS)

clean:
	$(MAKE) -C $(TOP)/base clean
	rm font_output.$(OBJ_EXT)



# g++ -I../.. font_output.cpp -I/usr/include/freetype2 -lfreetype