// content_hash.h -- Thatcher Ulrich <tu@tulrich.com> 2008

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Thin wrapper around stb's sha1 code.

#ifndef CONTENT_HASH_H_
#define CONTENT_HASH_H_

class ContentHash {
 public:
  ContentHash();

  void Reset();
  void InitFromFile(const char* filename);
  void operator=(const ContentHash& b);

  bool IsZero() const;
  bool operator==(const ContentHash& b) const;

  unsigned char* data() {
    return &h_[0];
  }
  int size() {
    return sizeof(h_);
  }

 private:
  unsigned char h_[20];  // sha1
};

#endif  // CONTENT_HASH_H_
