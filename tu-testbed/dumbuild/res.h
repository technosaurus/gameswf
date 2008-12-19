// res.h -- Thatcher Ulrich <tu@tulrich.com> 2008

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Res is a utility class for holding error/success results from
// dumbuild operations.

#ifndef RES_H_
#define RES_H_

#include <assert.h>
#include <map>
#include <string>

// For error codes, with optional additional info.
enum ResValue {
  OK = 0,
  ERR,
  ERR_COMMAND_LINE,
  ERR_PARSE,
  ERR_SUBCOMMAND_FAILED,
  ERR_FILE_NOT_FOUND,
  ERR_UNDEFINED_TARGET,
  ERR_DEPENDENCY_CYCLE,
  ERR_UNKNOWN_TARGET,
  ERR_DONT_REBUILD,

  RES_VALUE_COUNT
};

class ResValueStrings {
 public:
  ResValueStrings() {
#define ADD_STR(value) strings_[value] = #value
    ADD_STR(OK);
    ADD_STR(ERR);
    ADD_STR(ERR_COMMAND_LINE);
    ADD_STR(ERR_PARSE);
    ADD_STR(ERR_SUBCOMMAND_FAILED);
    ADD_STR(ERR_FILE_NOT_FOUND);
    ADD_STR(ERR_UNDEFINED_TARGET);
    ADD_STR(ERR_DEPENDENCY_CYCLE);
    ADD_STR(ERR_UNKNOWN_TARGET);
    ADD_STR(ERR_DONT_REBUILD);
#undef ADD_STR
    assert(strings_.size() == RES_VALUE_COUNT);
  }

  const char* ValueString(ResValue val) {
    std::map<int, std::string>::iterator it = strings_.find(val);
    if (it == strings_.end()) {
      assert(0);
    }
    return it->second.c_str();
  }

 private:
  std::map<int, std::string> strings_;
};
extern ResValueStrings g_res_value_strings;

// This class is used throughout dumbuild to return result codes.
class Res {
 public:
  Res() : value_(OK), detail_(NULL) {
  }
  Res(ResValue val) : value_(val), detail_(NULL) {
  }
  Res(ResValue val, const char* detail)
      : value_(val), detail_(new std::string(detail)) {
  }
  Res(ResValue val, const std::string& detail)
      : value_(val), detail_(new std::string(detail)) {
  }
  Res(const Res& res) : value_(res.value_), detail_(NULL) {
    if (res.detail_) {
      detail_ = new std::string(*res.detail_);
    }
  }

  ~Res() {
    ReleaseDetail();
  }

  void operator=(const Res& res) {
    ReleaseDetail();
    value_ = res.value();
    if (res.detail_) {
      detail_ = new std::string(*res.detail_);
    }
  }

  ResValue value() const {
    return value_;
  }

  const char* detail() const {
    if (detail_) {
      return detail_->c_str();
    }
    return "";
  }

  const char* ValueString() const {
    return g_res_value_strings.ValueString(value());
  }

  bool Ok() const {
    return value_ == OK;
  }

  void AppendDetail(const std::string& str) {
    if (!detail_) {
      detail_ = new std::string(str);
    } else {
      *detail_ += str;
    }
  }

 private:
  void AllocateDetailIfNecessary() {
  }

  void ReleaseDetail() {
    if (detail_) {
      delete detail_;
      detail_ = NULL;
    }
  }

  ResValue value_;
  std::string* detail_;
};

#endif  // RES_H_
