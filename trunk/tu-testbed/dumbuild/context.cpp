// context.cpp -- Thatcher Ulrich <tu@tulrich.com> 2008

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#include <vector>
#include "context.h"
#include "path.h"

Res Context::Init(const std::string& root_path,
                  const std::string& config_name) {
  tree_root_ = root_path;
  config_name_ = config_name;
  out_root_ = PathJoin("dmb_out", CanonicalFilePart(config_name_));

  return Res(OK);
}

std::string Context::AbsoluteFile(const std::string& canonical_path,
                                  const std::string& filename) {
  return PathJoin(tree_root(), PathJoin(canonical_path, filename));
}

void Context::Log(const std::string& msg) const {
  fputs(msg.c_str(), stdout);
  fflush(stdout);
}

void Context::LogVerbose(const std::string& msg) const {
  if (log_verbose_) {
    Log(msg);
  }
}
