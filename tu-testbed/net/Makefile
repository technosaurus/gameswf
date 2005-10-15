# Makefile	-- Thatcher Ulrich <tu@tulrich.com> 2005

# This source code has been donated to the Public Domain.  Do
# whatever you want with it.

# Makefile for net library & tests.

TOP = ..
include $(TOP)/config

INCLUDE_PATHS := $(ENGINE_INCLUDE) $(SOCKET_INCLUDE)

CFLAGS := $(CFLAGS) $(INCLUDE_PATHS) $(CC_DEBUG_FLAGS)
LDFLAGS := $(LIB_DEBUG_FLAGS) $(LDFLAGS)

LIB_OUT = $(LIB_PRE)net.$(LIB_EXT)
EXE_OUT = net_test$(EXE_EXT)

LIBS := $(LIB_OUT) $(BASE_LIB) $(LIBS) $(SOCKET_LIBS)

all: base_lib $(LIB_OUT) $(EXE_OUT)


LIB_OBJS = \
	http_file_handler.$(OBJ_EXT)		\
	http_server.$(OBJ_EXT)			\
	net_interface_tcp.$(OBJ_EXT) 		\
	webtweaker.$(OBJ_EXT)			\
	webwidgets.$(OBJ_EXT)


TEST_PROGRAM_OBJS = \
	net_test.$(OBJ_EXT)

OBJS = $(LIB_OBJS) $(TEST_PROGRAM_OBJS)

$(LIB_OUT): $(LIB_OBJS)
	$(AR) $(LIB_OUT_FLAG)$@ $^


$(EXE_OUT): $(TEST_PROGRAM_OBJS) $(LIB_OUT) $(BASE_LIB)
	$(CC) -o $@ $(TEST_PROGRAM_OBJS) $(LIBS) $(LDFLAGS)


base_lib:
	make -C $(TOP)/base

clean:
	make -C $(TOP)/base clean
	-rm $(OBJS) $(LIB_OUT) $(EXE_OUT)

depend:
	makedepend -Y -I.. -f Makefile *.cpp

fresh: clean all
