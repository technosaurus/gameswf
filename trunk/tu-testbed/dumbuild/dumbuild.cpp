// dumbuild.cpp -- Thatcher Ulrich <tu@tulrich.com> 2008

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Dead simple dumb C++ build tool.  Is it useful?
//
// Basic idea: take all the C++ files, concatenate them together (via
// #includes), and run that through the compiler.  The goal is fast
// compilation, and no need to figure out which cpp files depend on
// which headers (so as to pick which cpp files to compile).  Instead,
// everything gets compiled all the time.
//
// Build files are declared using JSON syntax.
//

// TODO:
// * write a turd for each .cpp and .h, containing its size and content hash
//   - that's the basis for change detection
// * header-changed checks:
//   - when a header file in a target changes, set header_changed flag
//   - when a direct dep of a target has header_changed, rebuild whole target
//   - when a header inside a target changes, rebuild whole target
//   - All headers in a dir are assumed to be part of targets in that dir?
//     Or list headers explicitly?
// * source changed check:
//   - when a cpp file in a target changes, recompile that cpp
// * config system
//   - config gets an "inherit" keyword, give ordered list of parents
//   - for string vars, use string template substitution.  "... ${varname} ..."
//   - ${varname} -- inherits based on declared order
//   - ${parent_x.varname} -- to inherit from an explicitly named parent?
//   - for list vars -- may need to implement actual operators
//     in the config language.  Maybe just punt for now.
// * dll_target

#include <assert.h>
#include <json/json.h>
#include <stdio.h>
#ifdef _WIN32
#include <direct.h>
#else  // not _WIN32
#include <unistd.h>
#endif  // not _WIN32

#include "config.h"
#include "context.h"
#include "dmb_types.h"
#include "os.h"
#include "res.h"
#include "target.h"
#include "util.h"

void PrintUsage() {
  printf("dmb [options] <target>\n"
         "\n"
         "options include:\n"
         "\n"
         "  -c      clean\n"
         "  -n      don't actually execute the commands\n"
         "  -v      verbose mode (show what commands would be executed)\n"
         );
}


/* example:

   [
   { "top": ".." },
  
   { "name": "myprog",
   "type": "exe",
   "src": [
   "file1.cpp",
   "file2.cpp",
   ],
   "dep": [
   "prebuild_myprog",
   ],
   },

   { "name": "mylib",
   "type": "lib",
   "src": [
   "libsrc1.cpp",
   "libsrc2.cpp",
   ],
   "dep": [
   "/proj/subdir/somelib",
   ],
   },
   ]

*/

void ExitIfError(const Res& res) {
  if (res.Ok()) {
    return;
  }

  fprintf(stderr, "dmb error\n");
  fprintf(stderr, "%s: %s\n", res.ValueString(), res.detail());

  exit(1);
}

// On success, currdir_relative_to_root will be a canonical path.
Res FindRoot(string* absolute_root,
             string* currdir_relative_to_root) {
  string currdir = GetCurrentDir();

  // Look for root dir.  Root dir is marked by the presence of a
  // "root.dmb" file.
  string root = currdir;
  for (;;) {
    if (FileExists(PathJoin(root, "root.dmb"))) {
      break;
    }
    if (HasParentDir(root)) {
      root = ParentDir(root);
    } else {
      return Res(ERR, "Can't find root.dmb in ancestor directories");
    }
  }
  *absolute_root = root;

  const char* remainder = currdir.c_str() + root.length();
  if (remainder[0] == '/') {
    remainder++;
  }
  *currdir_relative_to_root = remainder;
  assert(currdir_relative_to_root->size() == 0 ||
         (*currdir_relative_to_root)[currdir_relative_to_root->length() - 1]
         != '/');

  return Res(OK);
}

int main(int argc, const char** argv) {
  Context context;
  Res res;

  res = context.ProcessArgs(argc, argv);
  ExitIfError(res);

  string absolute_root;
  string canonical_currdir;
  res = FindRoot(&absolute_root, &canonical_currdir);
  ExitIfError(res);

  context.LogVerbose(StringPrintf("absolute_root = %s\n",
				  absolute_root.c_str()));
  context.LogVerbose(StringPrintf("canonical_currdir = %s\n",
				  canonical_currdir.c_str()));

  res = context.Init(absolute_root, canonical_currdir);
  ExitIfError(res);

  assert(context.GetConfig());
  assert(context.main_target());

  res = context.main_target()->Resolve(&context);
  ExitIfError(res);

  res = context.ProcessTargets();
  ExitIfError(res);

  context.Log("dmb OK\n");

  return 0;
}
