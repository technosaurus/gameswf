// context.cpp -- Thatcher Ulrich <tu@tulrich.com> 2008

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#include <vector>
#include "context.h"
#include "config.h"
#include "path.h"
#include "target.h"

Context::~Context() {
  for (std::map<std::string, Target*>::iterator it = targets_.begin();
       it != targets_.end();
       ++it) {
    delete it->second;
  }
  for (std::map<std::string, Config*>::iterator it = configs_.begin();
       it != configs_.end();
       ++it) {
    delete it->second;
  }
}

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
