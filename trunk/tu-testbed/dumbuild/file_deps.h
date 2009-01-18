// file_deps.h -- Thatcher Ulrich <tu@tulrich.com> 2009

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Utilities for checking C/C++ inter-file dependencies (i.e. tracking
// #include files).

#ifndef FILE_DEPS_H_
#define FILE_DEPS_H_

#include "dmb_types.h"
#include "res.h"

class Hash;
class Context;
class Target;

bool AnyIncludesChanged(const Target* t, const Context* context,
                        const string& inc_dirs_str,
                        const string& src_path,
                        const Hash& src_hash);

Res GenerateDepsFile(const Target* t, const Context* context,
                     const string& inc_dirs_str,
                     const string& src_path,
                     const Hash& src_hash);

// TODO: above code is not correct yet.  Need to do this instead:

// CumHash == Cumulative Hash ==
//   cumulative hash of all content that goes into some build product
//
// CumHash(src_file) = Hash(src_file) + sum(for d in deps: CumHash(d))
//
// When CumHash(file) changes, anything that depends on file must be
// rebuilt.  CumHash() can be used as a reliable id for a build
// product.

// CumHash(obj_file) = Hash(compile_flags & environment) + CumHash(src_file)

// CumHash(lib_or_exe_file) = Hash(link_flags & environment) +
//    sum(d in deps: CumHash(d)) +

// A build cache would be: map<CumHash, content>

// Deps(src_file):
//   for each #include in src_file:
//     full inc_filename
// (these get cached in the ostore, indexed by Hash(contents of src_file))

// Deps(obj_file): implicitly, src_file

// Deps(lib_file or exe_file):
//   list of obj files
// (these come directly out of build.dmb files)

// Build marker: under obj_file_name.dmb.hash
//   CumHash(obj_file)

// Build marker: under lib_or_exe_file_name.dmb.hash
//   CumHash(lib_or_exe_target)

// Rebuild rule:
//   rebuild(target) = BuildMarker(target) != CumHash(target)

//Res GetCumHashForObjectFile(const Target* t, const Context* context,
//			      const string& src_path);

//Res AccumulateDepsCumHash(, hash* h);

//Res AppendDeps(src_file, FILE* fp_out);

#endif  // FILE_DEPS_H_
