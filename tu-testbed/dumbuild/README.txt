dumbuild
========

dumbuild is a minimal build tool for building C++ programs.  The goals
include:

* Small and self-contained.

* Simple and sane config language.

* Easily hackable and extendable by C++ programmers.

* Support Windows, Linux and Mac OSX out of the box.

* Not gratuituously slow.


Usage
=====

> dmb [options] [target-name]

// TODO flesh this out (and implement!)

Files:

root.dmb -- put this in the root directory of your project.  All the
  pathnames in dumbuild config are relative to this root.  This is
  also where you put the compiler configuration (command-line
  templates and such).

build.dmb -- these files specify your actual "targets" (i.e. libs and
  executables).  Put these throughout your project tree, wherever you
  need them.  The recommended pattern is to put one in each source
  directory.  You can have more than one target in a single directory;
  this is normal if you want some structure inside the directory.  A
  target can actually pull sources from outside the directory its
  build.dmb file is in, though this is more intended for special
  circumstances, like building sources from a foreign code tree.


Design Principles
=================

Keep It Simple
--------------

1. If there is a choice between a simple, conservative 80% solution
   and a more complex 100% solution, prefer the 80% solution.

2. Don't build too much compiler/platform knowledge into the tool.
   Push that into config files.  But the config files need to be brain
   dead and explicit.

3. Don't support every clever build-tool feature.  That means no
   fine-grained implicit dependency checking, no parallel build
   support, no networked object cache.  At least not initially.

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

Coarse-grained
--------------

The basic unit of dependency checking in dumbuild is the "target".
Libraries and executables are types of targets.  The user has to
explicitly declare the dependencies between targets.

dumbuild does not try to do file-level dependency checking.  I think
that would be complicated and a waste of time.  Here's a simple
conservative 80% rule to use instead: if a target depends on another
target, or declares an include dir, dumbuild just checks to see if
*any* header file in those dependency directories is new -- if so, it
assumes that the target is stale.  (NOTE: not implemented yet!!)

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

* For coarse-grained dependency checking, it would be good to enforce
  explicit declaration of dependencies by using symlinks to build
  targets in a little pseudo-tree where only the dependencies are
  mapped in.  That way, if you forget to declare a dependency, the
  compiler won't find the header file and you'll get an error message.
