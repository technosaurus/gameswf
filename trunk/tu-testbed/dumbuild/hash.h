// hash.h -- Thatcher Ulrich <tu@tulrich.com> 2008

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Thin wrapper around stb's sha1 code.

#ifndef HASH_H_
#define HASH_H_

#include <assert.h>
#include "dmb_types.h"
#include "res.h"

class Hash {
 public:
  Hash();

  bool InitFromReadable(const char readable[27]);
  
  void operator=(const Hash& b);
  void Reset();
  Res AppendFile(const char* filename);
  Res AppendFile(const string& filename);
  void AppendData(const char* data, int size);
  void AppendString(const string& str);
  void AppendHash(const Hash& h) {
    AppendData((const char*) h.data(), h.size());
  }

  bool operator==(const Hash& b) const;
  bool operator!=(const Hash& b) const {
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

// Helper class. Holds a map from string to computed hash.
class HashCache {
 public:
  void Insert(const string& key, const Hash& h) {
    map_.insert(std::make_pair(key, h));
  }

  bool Get(const string& key, Hash* h) {
    assert(h);
    map<string, Hash>::const_iterator it = map_.find(key);
    if (it != map_.end()) {
      *h = it->second;
      return true;
    }
    return false;
  }

 private:
  map<string, Hash> map_;
};

#endif  // HASH_H_
