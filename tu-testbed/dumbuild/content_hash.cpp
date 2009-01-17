// content_hash.cpp -- Thatcher Ulrich <tu@tulrich.com> 2008

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#include <string.h>

#include "content_hash.h"
#include "sha1.h"

ContentHash::ContentHash() {
  Reset();
}

void ContentHash::Reset() {
  stb_sha1_init(h_);
}

bool ContentHash::InitFromReadable(const char readable[27]) {
  return stb_sha1_from_readable(readable, h_);
}

Res ContentHash::AppendFile(const char* filename) {
  if (!stb_sha1_file(h_, filename)) {
    Reset();
    return Res(ERR_FILE_ERROR, filename);
  }
  return Res(OK);
}

Res ContentHash::AppendFile(const std::string& filename) {
  return AppendFile(filename.c_str());
}

void ContentHash::AppendData(const char* data, int size) {
  stb_sha1(h_, (const unsigned char*) data, size);
}

void ContentHash::AppendString(const std::string& str) {
  AppendData(str.c_str(), str.length());
}

void ContentHash::operator=(const ContentHash& b) {
  memcpy(h_, b.h_, sizeof(h_));
}

bool ContentHash::operator==(const ContentHash& b) const {
  for (int i = 0; i < sizeof(h_); i++) {
    if (h_[i] != b.h_[i]) {
      return false;
    }
  }
  return true;
}

void ContentHash::GetReadable(char readable[27]) {
  stb_sha1_readable(readable, h_);
}
