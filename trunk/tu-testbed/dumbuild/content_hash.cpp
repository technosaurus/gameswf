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
  memset(h_, 0, sizeof(h_));
}

void ContentHash::InitFromFile(const char* filename) {
  if (!stb_sha1_file(h_, filename)) {
    Reset();
  }
}

void ContentHash::operator=(const ContentHash& b) {
  memcpy(h_, b.h_, sizeof(h_));
}

bool ContentHash::IsZero() const {
  for (int i = 0; i < sizeof(h_); i++) {
    if (h_[i]) {
      return false;
    }
  }
  return true;
}


bool ContentHash::operator==(const ContentHash& b) const {
  for (int i = 0; i < sizeof(h_); i++) {
    if (h_[i] != b.h_[i]) {
      return false;
    }
  }
  return true;
}
