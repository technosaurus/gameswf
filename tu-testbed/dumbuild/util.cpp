// util.cpp  -- Thatcher Ulrich <tu@tulrich.com> 2008

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdarg.h>

#include "util.h"
#include "eval.h"

string Join(const char* delimiter, const vector<string>& to_join) {
  string result;
  if (to_join.size()) {
    result = to_join[0];
  }
  for (size_t i = 1; i < to_join.size(); i++) {
    result += delimiter;
    result += to_join[i];
  }
  return result;
}

string StripExt(const string& filename) {
  size_t last_slash = filename.rfind('/');
  size_t last_dot = filename.rfind('.');
  if (last_dot != string::npos
      && (last_slash == string::npos || last_slash < last_dot)) {
    return string(filename, 0, last_dot);
  }
  
  return filename;
}

string GetExt(const string& filename) {
  size_t last_slash = filename.rfind('/');
  size_t last_dot = filename.rfind('.');
  if (last_dot != string::npos
      && (last_slash == string::npos || last_slash < last_dot)) {
    return string(filename, last_dot + 1, filename.length());
  }
  
  return string();
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

bool IsValidVarname(const string& varname) {
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
                 bool tolerate_missing_vars,
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
        if (tolerate_missing_vars) {
          // Pass the variable through, without replacing it.
          out->append(p, end + 1 - p);
          p = end + 1;
          continue;
        } else {
          out->clear();
          return Res(ERR, "Template contained varname '" + varname +
                     "' but no variable of that name is defined.");
        }
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
    }
  }
  return Res(OK);
}

Res ParseValueStringOrMap(const Context* ctx,
                          Config* cfg,
                          const Json::Value& val,
                          const string& equals_string,
                          const string& separator_string,
                          string* out) {
  if (val.isString()) {
    *out = val.asString();
  } else if (val.isArray()) {
    // Evaluate as a lisp expression.
    Res err = EvalToString(ctx, cfg, val, out);
    if (!err.Ok()) {
      err.AppendDetail("in ParseValueStringOrMap(): array eval of '" +
                       val.toStyledString() + "'");
      return err;
    }
  } else if (val.isObject()) {
    // Evaluate as a named-value map.
    bool done_first = false;
    out->clear();
    for (Json::Value::iterator it = val.begin(); it != val.end(); ++it) {
      if (!it.key().isString()) {
        return Res(ERR_PARSE,
                   "ParseValueStringOrMap(): key is not a string: '" +
                   it.key().toStyledString());
      }
      const Json::Value& map_val = *it;
      string result;
      Res err = EvalToString(ctx, cfg, map_val, &result);
      if (!err.Ok()) {
        err.AppendDetail("in ParseValueStringOrMap(): key '" +
                         it.key().toStyledString() + "'");
        return err;
      }
      if (done_first) {
        *out += separator_string;
      }
      *out += it.key().asString();
      *out += equals_string;
      *out += result;
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
  const int BUFSIZE = 65536;
  char buf[BUFSIZE];

  va_list args;
  va_start(args, format);
  
  int result = VSNPRINTF(buf, BUFSIZE, format, args);
  if (result == -1 || result >= BUFSIZE) {
    // Result truncated.  Fail.
    return string("(StringPrintf truncated)");
  }
  // Make extra double certain the string is terminated.
  buf[BUFSIZE - 1] = 0;

  return string(buf);
}

void TestJoin() {
  vector<string> vec;
  assert("" == Join("", vec));
  assert("" == Join(" ", vec));
  vec.push_back("a");
  assert("a" == Join("", vec));
  assert("a" == Join(" ", vec));
  vec.push_back("b");
  assert("ab" == Join("", vec));
  assert("a b" == Join(" ", vec));
  vec.push_back("cadabra");
  assert("abcadabra" == Join("", vec));
  assert("a b cadabra" == Join(" ", vec));
  assert("a, b, cadabra" == Join(", ", vec));
}

void TestUtil() {
  TestJoin();
}
