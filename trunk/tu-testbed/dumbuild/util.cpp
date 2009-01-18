// util.cpp  -- Thatcher Ulrich <tu@tulrich.com> 2008

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#include "util.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdarg.h>

string StripExt(const string& filename) {
  size_t last_slash = filename.rfind('/');
  size_t last_dot = filename.rfind('.');
  if (last_dot != string::npos
      && (last_slash == string::npos || last_slash < last_dot)) {
    return string(filename, 0, last_dot);
  }
  
  return filename;
}

void TrimTrailingWhitespace(string* str) {
  while (str->length()) {
    char c = (*str)[str->length() - 1];
    if (c == ' ' || c == '\n' || c == '\r' || c == '\t') {
      str->resize(str->length() - 1);
    } else {
      break;
    }
  }
}

static bool IsValidVarname(const string& varname) {
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

Res FillTemplate(const string& template_string,
		 const map<string, string>& vars,
		 string* out) {
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

    string varname(begin + 2, end - (begin + 2));
    if (IsValidVarname(varname)) {
      // See if we can replace this slot.
      map<string, string>::const_iterator it =
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
                          const string& equals_string,
			  const string& separator_string,
                          string* out) {
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

string StringPrintf(const char* format, ...) {
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
    return string("(StringPrintf truncated)");
  }
  // Make extra double certain the string is terminated.
  buf[BUFSIZE - 1] = 0;

  return string(buf);
}

