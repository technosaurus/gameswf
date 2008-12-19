// compile_util.h -- Thatcher Ulrich <tu@tulrich.com> 2008

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#ifndef COMPILE_UTIL_H_
#define COMPILE_UTIL_H_

#include <map>
#include <string>
#include <utility>  // for std::pair
#include <vector>

#include "content_hash.h"
#include "res.h"

class Config;
class Target;

struct CompileInfo {
  // Variables for template replacement.
  std::map<std::string, std::string> vars_;
  // Specific source files to be compiled.
  std::vector<std::pair<std::string, ContentHash> > src_list_;
  // TODO: header_dir list?
};

// Set up the standard variables for building a compiler command line.
Res PrepareCompileVars(const Target* t, const Config* config,
                       CompileInfo* compile_info);

Res DoCompile(const Target* t, const Config* config,
              const CompileInfo& compile_info);

#endif  // COMPILE_UTIL_H_
