// content_hash.h -- Thatcher Ulrich <tu@tulrich.com> 2008

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Thin wrapper around stb's sha1 code.

#ifndef CONTENT_HASH_H_
#define CONTENT_HASH_H_

#include <assert.h>
#include <map>
#include <string>
#include "res.h"

class ContentHash {
 public:
  ContentHash();

  bool InitFromReadable(const char readable[27]);
  
  void operator=(const ContentHash& b);
  void Reset();
  Res AppendFile(const char* filename);
  Res AppendFile(const std::string& filename);
  void AppendData(const char* data, int size);
  void AppendString(const std::string& str);
  void AppendHash(const ContentHash& h) {
    AppendData((const char*) h.data(), h.size());
  }

  bool operator==(const ContentHash& b) const;
  bool operator!=(const ContentHash& b) const {
    return !(*this == b);
  }

  const unsigned char* data() const {
    return &h_[0];
  }
  unsigned char* mutable_data() {
    return &h_[0];
  }
  int size() const {
    return sizeof(h_);
  }

  void GetReadable(char readable[27]);

 private:
  unsigned char h_[20];  // sha1
};

// Helper class. Holds a map from filename to computed content hash.
class ContentHashCache {
 public:
  void Insert(const std::string& filename, const ContentHash& h) {
    map_.insert(std::make_pair(filename, h));
  }

  bool Get(const std::string& filename, ContentHash* h) {
    assert(h);
    std::map<std::string, ContentHash>::const_iterator it = map_.find(filename);
    if (it != map_.end()) {
      *h = it->second;
      return true;
    }
    return false;
  }

 private:
  std::map<std::string, ContentHash> map_;
};

#endif  // CONTENT_HASH_H_
