# Makefile for test_kd_tree programs


TOP = ../..
include $(TOP)/config

INCLUDE_PATHS := $(ENGINE_INCLUDE) $(ZLIB_INCLUDE)

CFLAGS := $(CFLAGS) $(INCLUDE_PATHS) $(CC_DEBUG_FLAGS)
LDFLAGS := $(SDL_LDFLAGS) $(LIB_DEBUG_FLAGS) $(LDFLAGS)

LIBS := $(ENGINE) $(LIBS) $(JPEGLIB) $(ZLIB)


all: engine test_kd_tree$(EXE_EXT)


test_kd_tree$(EXE_EXT): test_kd_tree.$(OBJ_EXT) $(ENGINE)
	$(CC) -o $@ $^ $(LIBS) $(LDFLAGS)


engine:
	make -C $(TOP)/engine


clean:
	make -C $(TOP)/engine clean
	rm test_kd_tree.$(OBJ_EXT) test_kd_tree$(EXE_EXT)
