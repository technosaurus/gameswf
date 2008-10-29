// util.cpp  -- Thatcher Ulrich <tu@tulrich.com> 2008

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#include "util.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdarg.h>

std::string StripExt(const std::string& filename) {
  size_t last_slash = filename.rfind('/');
  size_t last_dot = filename.rfind('.');
  if (last_dot != std::string::npos
      && (last_slash == std::string::npos || last_slash < last_dot)) {
    return std::string(filename, 0, last_dot);
  }
  
  return filename;
}

static bool IsValidVarname(const std::string& varname) {
  if (varname.length() == 0) {
    return false;
  }
  for (const char* q = varname.c_str(); *q; q++) {
    if ((*q >= 'a' && *q <= 'z')
        || (*q >= 'A' && *q <= 'Z')
        || (*q >= '0' && *q <= '9')
        || (*q == '-')
        || (*q == '_')) {
      // Ok.
    } else {
      // Bad varname character.
      return false;
    }
  }
  return true;
}

Res FillTemplate(const std::string& template_string,
		 const std::map<std::string, std::string>& vars,
		 std::string* out) {
  assert(out);
  out->clear();

  const char* p = template_string.c_str();

  for (;;) {
    const char* begin = strstr(p, "${");
    if (!begin) {
      // Done.
      *out += p;
      break;
    }
    const char* end = strchr(p, '}');
    if (!end) {
      // Done.
      *out += p;
      break;
    }

    std::string varname(begin + 2, end - (begin + 2));
    if (IsValidVarname(varname)) {
      // See if we can replace this slot.
      std::map<std::string, std::string>::const_iterator it =
        vars.find(varname);
      if (it == vars.end()) {
        out->clear();
        return Res(ERR, "Template contained varname '" + varname +
                   "' but no variable of that name is defined.");
      }
      // Insert text up to the var.
      out->append(p, begin - p);
      // Insert the replacement text.
      out->append(it->second);
      p = end + 1;
    } else {
      // Non-varname character; don't replace anything here.
      out->append(p, end + 1 - p);
      p = end + 1;
      break;
    }
  }
  return Res(OK);
}

Res ParseValueStringOrMap(const Json::Value& val,
                          const std::string& equals_string,
			  const std::string& separator_string,
                          std::string* out) {
  if (val.isString()) {
    *out = val.asString();
  } else if (val.isArray()) {
    // TODO
    assert(0);
  } else if (val.isObject()) {
    bool done_first = false;
    out->clear();
    for (Json::Value::iterator it = val.begin(); it != val.end(); ++it) {
      if (!it.key().isString()) {
        return Res(ERR_PARSE,
		   "ParseValueStringOrMap(): key is not a string: '" +
                   it.key().toStyledString());
      }
      if (!(*it).isString()) {
        return Res(ERR_PARSE, "ParseValueStringOrMap(): key '" +
                   it.key().asString() +
                   "' does not have a string value: " +
                   (*it).toStyledString());
      }
      if (done_first) {
        *out += separator_string;
      }
      *out += it.key().asString();
      *out += equals_string;
      *out += (*it).asString();
      done_first = true;
    }
  }
  return Res(OK);
}

#ifdef _WIN32
#define VSNPRINTF _vsnprintf
#else
#define VSNPRINTF vsnprintf
#endif

std::string StringPrintf(const char* format, ...) {
  const int BUFSIZE = 4096;
  char buf[BUFSIZE];

  va_list args;
  va_start(args, format);
  
  int result = VSNPRINTF(buf, BUFSIZE, format, args);
  if (result == -1 || result >= BUFSIZE) {
    // Result truncated.  Fail.
    //
    // TODO: could try again with larger buffer, but for normal uses
    // of this function there is likely to be some other problem.
    return std::string("(StringPrintf truncated)");
  }
  // Make extra double certain the string is terminated.
  buf[BUFSIZE - 1] = 0;

  return std::string(buf);
}

bool FileExists(const std::string& path) {
  struct stat stat_info;
  int err = stat(path.c_str(), &stat_info);
  if (err) {
    return false;
  }
  return true;
}

