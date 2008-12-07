// compile_util.h -- Thatcher Ulrich <tu@tulrich.com> 2008

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#ifndef COMPILE_UTIL_H_
#define COMPILE_UTIL_H_

#include <map>
#include <string>
#include "res.h"

class Config;
class Target;

// Set up the standard variables for building a compiler command line.
Res PrepareCompileVars(const Target* t, const Config* config,
		       std::map<std::string, std::string>* vars);

Res DoCompile(const Target* t, const Config* config,
	      const std::map<std::string, std::string>& vars);

#endif  // COMPILE_UTIL_H_
