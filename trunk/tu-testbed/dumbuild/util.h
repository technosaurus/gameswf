// util.h  -- Thatcher Ulrich <tu@tulrich.com> 2008

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Misc small utility functions.

#ifndef UTIL_H_
#define UTIL_H_

#include <assert.h>
#include <json/json.h>
#include <map>
#include <string>
#include "res.h"

// Return the filename with the extension (if any) removed.
std::string StripExt(const std::string& filename);

void TrimTrailingWhitespace(std::string* str);

// Does variable replacement on the given template using the given
// vars, putting the results in *out.
//
// The template looks like "blah blah ${varname1} blah blah
// ${varname2} ..."
//
// Occurrences of "${varname}" are replaced by the element of vars
// with key "varname".  "varname" must consist only of characters
// [0-9][A-Z][a-z][-_].
//
// Returns an error if a replacement token occurs with no
// corresponding value in vars.
Res FillTemplate(const std::string& template_string,
		 const std::map<std::string, std::string> & vars,
		 std::string* out);

// This takes a Json::Value that could be a string or an array or a
// map, and parses it into *out.
//
// If val is a string, the string is just copied to *out.
//
// If val is an array, *out gets each element of the array, separated
// by separator_string.
//
// If val is an object, *out gets each key/value pair of the object,
// with equals_string separating the key and value, and
// separator_string separating the pairs.
Res ParseValueStringOrMap(const Json::Value& val,
			  const std::string& equals_string,
			  const std::string& separator_string,
			  std::string* out);

// printf-alike that returns a std::string.
std::string StringPrintf(const char* format, ...);

// Increments on construction; decrements on destruction.
class ScopedIncrement {
 public:
  ScopedIncrement(int* counter) : counter_ptr_(counter) {
    assert(counter_ptr_);
    (*counter_ptr_)++;
  }

  ~ScopedIncrement() {
    (*counter_ptr_)--;
  }

 private:
  int* counter_ptr_;
};

#endif  // UTIL_H_
