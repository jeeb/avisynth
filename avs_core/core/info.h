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

#ifndef __INFO_H__
#define __INFO_H__

#ifdef AVS_LINUX
#include <uchar.h>
#endif
#include <sstream>
#include "internal.h"
#include <unordered_map>
#include <array>

// unfortunately this is still specific definition
// are for maximum 16x20 pixel
using fixedFontRec_t = struct fixedFontRec_t {
  char32_t code; // really it's char16_t
  std::array<uint16_t, 20> bitmap; // WxH 10x20 pixel
};

using fixedFontArray = std::vector<fixedFontRec_t>;

class BitmapFont
{
  void makeMapping();

public:

  const fixedFontArray* fonts;
  const int w;
  const int h;

  std::unordered_map<uint32_t, int> charReMap; // unicode code point vs. font image index

  BitmapFont(const fixedFontArray* _fonts, int _w, int _h) : fonts(_fonts), w(_w), h(_h)
  {
    makeMapping();
  }

  // helper function for remapping a char16_t string to font index entry list
  std::vector<int> remap(const std::u16string& s16);

  // generate outline on-the-fly
  void generateOutline(fixedFontRec_t& f, int x) const;
};

void SimpleTextOutW(const VideoInfo& vi, PVideoFrame& frame, int real_x, int real_y, std::u16string& text, bool fadeBackground, int textcolor, int halocolor, bool useHaloColor, int align);
void SimpleTextOutW_multi(const VideoInfo& vi, PVideoFrame& frame, int real_x, int real_y, std::u16string& text, bool fadeBackground, int textcolor, int halocolor, bool useHaloColor, int align, int lsp);

// legacy function w/o outline, originally with ASCII input, background fading
void DrawStringPlanar(VideoInfo& vi, PVideoFrame& dst, int x, int y, const char* s);
void DrawStringYUY2(VideoInfo& vi, PVideoFrame& dst, int x, int y, const char* s);
void DrawStringRGB32(VideoInfo& vi, PVideoFrame& dst, int x, int y, const char* s);
void DrawStringRGB24(VideoInfo& vi, PVideoFrame& dst, int x, int y, const char* s);

#endif  // __INFO_H__
