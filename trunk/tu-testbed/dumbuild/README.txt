dumbuild
========

dumbuild is a small build tool for building C++ programs, written in
C++.  The goals include:

* Small and self-contained.  The code is Public Domain.  Easy to embed
  in larger open-source projects -- no external dependencies, no
  license restrictions.

* Simple and sane config language (based on JSON syntax).

* Written in garden-variety C++ -- hackable and extendable by C++
  programmers.

* Support Windows, Linux and Mac OSX out of the box.  Bootstraps
  itself if necessary using a small included batch file or shell
  script.

* Reliable and fast.  May be usable as your everyday build tool.

Getting dumbuild
================

The code is here:
http://tu-testbed.svn.sourceforge.net/viewvc/tu-testbed/trunk/tu-testbed/dumbuild/

You can grab it out of SVN or just poke around in there if you're
curious.

When it's more mature I'll provide .zip and .tar.gz archives.

A prebuilt statically-linked windows executable is here:
http://tulrich.com/geekstuff/dumbuild/dmb.exe

Building dumbuild
=================

From the dumbuild/ directory, run the build script.  On Windows:

> dmb.bat

On Linux etc:

# ./dmb

If all goes well, you should get dmb.exe (Windows) or dmb (Linux etc)
in the subdirectory dmb-out/bootstrap .

Usage
=====

On Windows:

> dmb [options] [target-name]

(Where "dmb" refers to the dmb.bat file in the dumbuild source tree.)

On Linux etc:

# ./dmb [options] [target-name]

(Where "./dmb" refers to the dmb shell script in the dumbuild source
tree.)

 ----
Options:

  -C <dir>      Change to the specified directory before starting work.
                This should have the effect of invoking dmb from that
                directory.

  -c <config>   Specify the name of a build configuration (i.e. compiler & mode)
                Supplied configurations in the default root.dmb include
                  gcc-debug
                  gcc-release
                  vc8-debug
                  vc8-release

  -r            Rebuild all, whether or not source files have changed.

  -v            Verbose.  Does a lot of extra logging.

 ----

If necessary, the dmb script first bootstraps a build of the dmb
executable.  After the executable is built, the dmb script uses it for
subsequent builds.  You can also invoke the executable directly if you
want, though the wrapper script overhead is very small.

Without any options, "dmb" builds the default target using the default
configuration.  The output goes in dmb-out/&lt;config-name&gt;/

Example:

> dmb -c :vc8-release

Scans build.dmb for the default target, and builds it using the
:vc8-release configuration.

Files:

root.dmb -- put this in the root directory of your project.  All the
  pathnames in dumbuild config are relative to this root.  This is
  also where you probably want to put the compiler configuration
  (command-line templates and such).
  [[http://tu-testbed.svn.sourceforge.net/viewvc/tu-testbed/trunk/tu-testbed/dumbuild/root.dmb?view=markup][Example]]

build.dmb -- these files specify your actual "targets" (i.e. libs and
  executables).  Put these throughout your project tree, wherever you
  need them.  The recommended pattern is to put one in each source
  directory.  You can have more than one target in a single directory;
  this is normal if you want some additional structure inside the
  directory.  A target can actually pull sources from outside the
  directory its build.dmb file is in, though this is more intended for
  special circumstances, like building sources from a foreign code
  tree.
  [[http://tu-testbed.svn.sourceforge.net/viewvc/tu-testbed/trunk/tu-testbed/dumbuild/build.dmb?view=markup][Example]]


Design Principles
=================

Keep It Simple
--------------

This is why I called it "dumbuild":

1. Small source, small executable.

2. Contains no scripting language.

3. No extraneous dependencies.  If you have a desktop OS and a
   compiler, you should be able to download dumbuild and then use it
   with a single command.

4. Config files are brain-dead simple and explicit.

Written in C++
--------------

dumbuild is written 100% in straightforward C++, and there is no
scripting language lurking within it.  This allows it to be
distributed (if desired) as a single statically-linked executable.  It
also helps guard against slowness and bloat.  And it helps the
intended users, C++ programmers, to be able to understand and hack the
code if necessary.

I'm using the Google C++ Style Guide:
http://google-styleguide.googlecode.com/svn/trunk/cppguide.xml

Simple Config Format
--------------------

The configuration syntax is Javascript Object Notation (aka JSON).
JSON is well specified, not too ugly, and pretty darn simple.  See
http://json.org for more on JSON.

I'm trying with the configuration semantics to stay close to the task
of building C++ libraries and executables.  I want to avoid a lot of
implicit behavior, without making it too general and verbose.

Fast and Reliable
-----------------

dumbuild wants to be usable as an everyday build tool.

* The compile tools are invoked hermetically; i.e. dumbuild does not
  let your environment variables leak into the build.

* It understands C/C++ syntax just enough to collect the #include
  files for any source file, in order to implement fine-grained
  dependency checking.  (You can deceive the #include file finder with
  tricky syntax, like using macros in the include file name.  So don't
  do that!)

* It uses content-based dependency checking (using SHA1 hashes).  When
  something changes, it quickly and accurately determines what files
  need to be recompiled and relinked.

Lib and Exe Oriented
--------------------

Dumbuild doesn't dignify individual .obj files as distinct targets.
The two main target types are "lib" (library) and "exe" (executable),
which can have any number of source files, and can depend on any
number of other targets.

The intention is to make it really easy to declare targets, so the
user can organize their code into libraries and executables in the
most logical way.

Notes
=====

* I'm not 100% satisfied with the JSON parser I'm using.  I think it
  could be smaller.  It chokes if you put comments in the wrong place,
  and (a pet peeve of mine) it can't tolerate a comma after the last
  element in an object or array.  On the plus side, it basically
  works, it's written in C++, and it's Public Domain.  No urgent need
  to fix or replace.

* I think specifying config is a hard problem, that I have not really
  tackled yet.  Inheritance seems like a good way to avoid repetition
  for config, but inheritance can also sometimes be obtuse.  One
  alternative is to support script-like expressions for combining
  configs & variables, but I really want to avoid the scripting
  rathole.

  My current plan is for each target type to support a fixed set of
  parameters, which are initialized in a target-dependent way, and
  substituted into template strings supplied in the project
  configuration.  We'll see how far that goes.

* The .exe is currently TOO BIG (> 250K)!  And takes TOO LONG to
  compile (> 10 seconds)!  I think the main culprit is STL.  I need to
  replace STL with tu-testbed base containers, which compile
  relatively small, and should provide everything necessary.

* Partly inspired by git, I use SHA1 hashes to identify a build
  product, by hashing the contents of all the ingredients that go into
  the build product.  I call that a "DepHash".  DepHash definitely
  simplified the code, and it should make dependency checking very
  reliable.

* I'm intrigued by the notion of mating dumbuild to a git backend,
  where git would serve two purposes: 1) a networked/distributed cache
  of built products (i.e. so that if someone has already built a
  particular version of a library, you just pull the pre-built lib
  file instead of rebuilding it locally) and 2) a hash-aware
  filesystem, that would help to quickly compute the DepHash for a lib
  or exe, without necessarily having to pull the actual source file
  contents.  Imagine building a complete modified Linux kernel very
  quickly, while only having a few source files stored locally.

  I don't hack the Linux kernel, so I'm not even sure if that would be
  useful, but I can definitely picture other situations where it would
  be handy.

Why?
====

Good question.  Because it's more fun to start a project than to
finish it?

Basically, I was so sick of miserable build tools I thought the world
might need another one.  Anyway, it's a little hobby project and has
been fun to mess around with.  At some point I might try to use it as
the canonical build tool for
[[http://tulrich.com/geekstuff/gameswf.html][gameswf]] (if the other
devs will let me).

Alternatives
------------

[[http://redmine.jamplex.org/projects/show/jamplus][JamPlus]] wasn't
released when I started working on dumbuild, but the developers have a
clue and it's probably better than dumbuild in most dimensions.  So if
you just want a good build tool, I would start there.

[[http://www.scons.org/][Scons]] is much better-developed and
feature-filled.  But, it's written in Python, it's smallish but not
tiny, and unfortunately it can be pretty slow on big projects.  I
don't think it's a great choice for embedding in a small or
medium-sized open-source project, mainly because it would be bigger
than the typical host project.  It's also no speed demon.

GNU make is what I usually use for my own open-source stuff.  I don't
mind it too much, but it's not the most elegant thing, and has some
real shortcomings with dependency checking and it seems to perplex
most Windows programmers.
