dumbuild
========

dumbuild is a minimal build tool for building C++ programs.  The goals
include:

* Small and self-contained.  Easy to embed in larger open-source
  projects -- no external dependencies, no license restrictions.

* Simple and sane config language.

* Easily hackable and extendable by C++ programmers.

* Support Windows, Linux and Mac OSX out of the box.  Bootstraps
  itself if necessary using a simple batch file or shell script.

* Reliable and fast.  Use it as your everyday build tool.

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

# dmb [options] [target-name]

(Where "dmb" refers to wherever the dmb script or executable exists.)
For example:

# dmb

Builds the default target using the default configuration.  The output
goes in dmb-out/&lt;config-name&gt;/

# dmb -c :vc8-release

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
intended users, C++ programmers, easily understand and hack the code
if necessary.

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

dumbuild wants to be usable as an everyday build tool.  The compile
tools are invoked hermetically; i.e. dumbuild does not let your
environment variables leak into the build.  It uses content-based
dependency checking (using SHA1 hashes).  It understands C/C++ syntax
just enough to collect the #include files for any source file, in
order to implement fine-grained dependency checking.  When something
changes, it quickly and accurately determines what files need to be
recompiled.  (WORK IN PROGRESS)

Fine-grained
------------

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

* I think specifying config is a hard problem, that I haven't really
  tackled yet.  Inheritance seems like a good way to avoid repetition
  for config, but inheritance can also sometimes be obtuse.  One
  alternative is to support script-like expressions for combining
  configs & variables, but I really want to avoid the scripting
  rathole.

  My current plan is for each target type to support a fixed set of
  parameters, which are initialized in a target-dependent way, and
  substituted into template strings supplied in the project
  configuration.  We'll see how far that goes.

* The .exe is currently TOO BIG!  I think the main culprit is STL.  I
  need to replace STL with tu-testbed base containers, which compile
  relatively small, and should provide everything necessary.

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
clue and it's probably better in almost every dimension than dumbuild.
So if you just want a good build tool, I would start there.

[[http://www.scons.org/][Scons]] is much better-developed and
feature-filled.  But, it's written in Python, it's smallish but not
tiny, and unfortunately it can be pretty slow on big projects.  I
don't think it's a great choice for embedding in a small or
medium-sized open-source project, mainly because it would be bigger
than the typical host project.

GNU make is what I usually use for my own open-source stuff.  I don't
mind it too much, but it's not the most elegant thing, and has some
real shortcomings with dependency checking and it seems to perplex
most Windows programmers.
