Lua Module experimentation

I've hacked together some binaries and a sample module using Ignacio
Castaño's prototype luselib code.  I've made Windows and Linux
versions that seem to work.  There is:

* a "luse" executable, which is a very basic Lua 4.0 toplevel (no
  command-line niceties), that provides the use() Lua function to load
  a Lua module DLL.

* lua.dll for Win32, which is used by luse.exe (includes both core
  Lua, and the libraries).

* (drumroll) a sample Lua Module that provides an SDL binding!
  Comprised of SDL.lm (Lua code), libluaSDL.so (for Linux) and
  luaSDL.dll (for Win32).  Lua programs that want SDL just need to do 

* liblua.so and liblualib.so, used by the Linux 'luse' executable.

All files for both platforms are just jumbled together in the same
directory.  

WIN32:

To run the sample, just unzip, change to the directory, and type "luse
meteor_shower.lua", or "go.bat".

LINUX:

Unzip the files somewhere.  Move liblua.so, liblualib.so, and
libluaSDL.so to somewhere in your library path.  For example, I put
them in ~/lib , and then do "export LD_LIBRARY_PATH=$HOME/lib".  Leave
everything else in the luaSDL/ directory.  Change to luaSDL/, and do
"./luse meteor_shower.lua" to run the sample.

BOTH PLATFORMS:

You can also run 'luse' interactively -- just don't give any
command-line args.  It's vanilla Lua 4.0, plus the use() function.

NOTES:

* I had to comment out the part of Ignacio's code which registers some
  library info in a Lua table -- it was causing crashes on both
  platforms.  So use()'ing a library more than once may create
  problems!

* My build process includes manual steps, so you probably won't be
  able to reproduce what I did from source.  This is just an
  experiment at this point.

* My SDL module does not yet use the excellent recommendations from
  Roberto's LTN7 (the core idea being, put all module symbols in a
  table, to prevent namespace problems).

* This is very cool!  Once we nail down some standards and get the
  kinks worked out, I'm looking forward to using people's modules!
  We'll all be writing portable Lua code that combines SDL with
  OpenGL, GUI toolkits, db access, CGI and whatever, without touching
  a C compiler!
