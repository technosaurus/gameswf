// dumbuild.cpp -- Thatcher Ulrich <tu@tulrich.com> 2008

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// C++ build tool written in C++.  See README.txt for more info.

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
#include "eval.h"
#include "os.h"
#include "res.h"
#include "target.h"
#include "util.h"

void PrintUsage() {
  printf("dmb [options] <target>\n"
         "\n"
         "options include:\n"
         "\n"
         "  -C <dir>      Change to specified directory before starting.\n"
         "  -c <config>   Specify a named configuration.\n"
         "  -n            Don't actually execute the commands.\n"
         "  -r            Rebuild (don't reuse previous build products).\n"
         "  -v            Verbose mode (show what commands would be executed).\n"
         "  --test        Run internal unit tests.\n"
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

void InitStatics() {
  InitEval();
}

int main(int argc, const char** argv) {
  InitStatics();

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

  res = context.Resolve();
  ExitIfError(res);

  res = context.ProcessTargets();
  ExitIfError(res);

  context.Log("dmb OK\n");

  return 0;
}
