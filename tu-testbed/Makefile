# Makefile for tu-testbed project.

# Very miscellaneous at the moment.


all:
	make -C engine
	make -C demos/chunklod

clean:
	make -C engine clean
	make -C demos/chunklod clean


basic_files = \
		tu-testbed/README	\
		tu-testbed/Makefile \
		tu-testbed/config \
		tu-testbed/docs/AUTHORS	\
		tu-testbed/docs/LICENSE	\
		tu-testbed/docs/MANIFESTO	\
		tu-testbed/docs/README	\
		tu-testbed/docs/STYLE

engine_files = \
		tu-testbed/engine/Makefile \
		tu-testbed/engine/*.cpp	\
		tu-testbed/engine/*.h	\
		tu-testbed/engine/TODO

chunklod_files = \
		tu-testbed/demos/chunklod/chunkdemo.exe \
		tu-testbed/demos/chunklod/heightfield_chunker.exe \
		tu-testbed/demos/chunklod/heightfield_shader.exe \
		tu-testbed/demos/chunklod/maketqt.exe \
		tu-testbed/demos/chunklod/makebt.exe \
		tu-testbed/demos/chunklod/decimate_texture.exe \
		tu-testbed/demos/chunklod/dem2bt.pl \
		tu-testbed/demos/chunklod/Makefile \
		tu-testbed/demos/chunklod/*.cpp \
		tu-testbed/demos/chunklod/*.h \
		tu-testbed/demos/chunklod/README	\
		tu-testbed/demos/chunklod/TODO	\
		tu-testbed/demos/chunklod/README-SDL.txt	\
		tu-testbed/demos/chunklod/*.dll

copperwine_data_files = \
		tu-testbed/go_copperwine.bat	\
		tu-testbed/demos/chunklod/copperwine_e1_d8.chu	\
		tu-testbed/demos/chunklod/copperwine_8k_d7.tqt


# Need a better way to do this.
chunkdemo:
	cd ..; zip tu-testbed/chunkdemo.zip \
		tu-testbed/README	\
		tu-testbed/Makefile \
		tu-testbed/config \
		tu-testbed/docs/LICENSE	\
		tu-testbed/docs/MANIFESTO	\
		tu-testbed/docs/README	\
		tu-testbed/docs/STYLE	\
		tu-testbed/demos/chunklod/chunkdemo.exe \
		tu-testbed/demos/chunklod/heightfield_chunker.exe \
		tu-testbed/demos/chunklod/heightfield_shader.exe \
		tu-testbed/demos/chunklod/Makefile \
		tu-testbed/demos/chunklod/*.cpp \
		tu-testbed/demos/chunklod/*.h \
		tu-testbed/demos/chunklod/README	\
		tu-testbed/demos/chunklod/TODO	\
		tu-testbed/demos/chunklod/crater.bt	\
		tu-testbed/demos/chunklod/crater/crater.chu	\
		tu-testbed/demos/chunklod/crater/crater.jpg	\
		tu-testbed/demos/chunklod/test.jpg \
		tu-testbed/demos/chunklod/testmap.jpg \
		tu-testbed/demos/chunklod/tile*.jpg \
		tu-testbed/demos/chunklod/maketest.bat \
		tu-testbed/demos/chunklod/README-SDL.txt	\
		tu-testbed/demos/chunklod/*.dll	\
		tu-testbed/engine/Makefile \
		tu-testbed/engine/*.cpp	\
		tu-testbed/engine/*.h	\
		tu-testbed/engine/TODO


copperwine_demo.zip:
	cd ..; zip tu-testbed/copperwine_demo.zip	\
		$(basic_files) $(engine_files) $(chunklod_files) $(copperwine_data_files)


bspdemo.zip:
	cd ..; zip tu-testbed/bspdemo.zip \
		tu-testbed/Makefile \
		tu-testbed/config \
		tu-testbed/demos/bsp/bspdemo.exe \
		tu-testbed/demos/bsp/Makefile \
		tu-testbed/demos/bsp/*.cpp \
		tu-testbed/demos/bsp/*.h \
		tu-testbed/demos/bsp/README	\
		tu-testbed/demos/bsp/TODO	\
		tu-testbed/demos/bsp/*.ply	\
		tu-testbed/demos/bsp/*.pl	\
		tu-testbed/engine/Makefile \
		tu-testbed/engine/*.cpp	\
		tu-testbed/engine/*.h	\
		tu-testbed/engine/TODO
	# Include the lua stuff to make it easier to build the demo.
	cd ..; zip tu-testbed/bspdemo.zip -r lua/*
