# Makefile for tu-testbed project.

# Very miscellaneous at the moment.


all:
	cd engine; make; cd ..
	cd demos; make; cd ..


# Need a better way to do this.
chunkdemo.zip:
	cd ..; zip tu-testbed/chunkdemo.zip \
		tu-testbed/Makefile \
		tu-testbed/config \
		tu-testbed/demos/chunklod/chunkdemo.exe \
		tu-testbed/demos/chunklod/Makefile \
		tu-testbed/demos/chunklod/*.cpp \
		tu-testbed/demos/chunklod/*.h \
		tu-testbed/demos/chunklod/README	\
		tu-testbed/demos/chunklod/TODO	\
		tu-testbed/demos/chunklod/crater/crater.chu	\
		tu-testbed/demos/chunklod/crater/crater.jpg	\
		tu-testbed/engine/Makefile \
		tu-testbed/engine/*.cpp	\
		tu-testbed/engine/*.h	\
		tu-testbed/engine/TODO
