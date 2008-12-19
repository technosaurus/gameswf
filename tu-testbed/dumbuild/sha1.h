// sha1.h -- Thatcher Ulrich <tu@tulrich.com> 2008

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// (This is ripped off from Sean Barrett's stb.h; see sha1.cpp.)

#ifndef SHA1_H_
#define SHA1_H_

int stb_sha1_file(unsigned char output[20], const char* filename);
void stb_sha1_readable(char display[27], unsigned char sha[20]);
void stb_sha1(unsigned char output[20],
	      unsigned char* buffer, unsigned int len);

#endif  // SHA1_H_
