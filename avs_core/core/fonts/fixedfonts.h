// original IT0051 by thejam79
// add YV12 mode by minamina 2003/05/01
//
// Borrowed from the author of IT.dll, whose name I
// could not determine. Modified for YV12 by Donald Graft.
// RGB32 Added by Klaus Post
// Converted to generic planar, and now using exact coordinates - NOT character coordinates by Klaus Post
// Refactor, DrawString...() is the primative, Oct 2010 Ian Brabham
// TO DO: Clean up - and move functions to a .c file.

// pinterf:
// planar high bit depth, planar RGB, RGB48/64
// utf8 option, internally unicode, Latin-1 Supplement 00A0-00FF
// Original hexadecimal bitmap definitions changed to binary literals
// Add some new characters from Latin Extended A
// Configurable color
// Configurable halocolor (text outline)
// Configurable background fading
// Alignment
// multiline

#ifndef __FIXEDFONTS_H__
#define __FIXEDFONTS_H__

typedef struct {
  const char* const fontname;
  const char* const fontname_internal;
  int charcount;
  int width;
  int height;
  bool bold;
} FixedFont_info_t;

constexpr int PREDEFINED_FONT_COUNT = 18 + 2; // 2x9 Terminus + info_h

#endif  // __FIXEDFONTS_H__
