// Avisynth+
// https://avs-plus.net
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA, or visit
// http://www.gnu.org/copyleft/gpl.html .
//
// Linking Avisynth statically or dynamically with other modules is making a
// combined work based on Avisynth.  Thus, the terms and conditions of the GNU
// General Public License cover the whole combination.
//
// As a special exception, the copyright holders of Avisynth give you
// permission to link Avisynth with independent modules that communicate with
// Avisynth solely through the interfaces defined in avisynth.h, regardless of the license
// terms of these independent modules, and to copy and distribute the
// resulting combined work under terms of your choice, provided that
// every copy of the combined work is accompanied by a complete copy of
// the source code of Avisynth (the version of Avisynth used to produce the
// combined work), being distributed under the terms of the GNU General
// Public License plus this exception.  An independent module is a module
// which is not derived from or based on Avisynth, such as 3rd-party filters,
// import and export plugins, or graphical user interfaces.

#ifndef __INFO_H__
#define __INFO_H__

#ifdef AVS_LINUX
#include <uchar.h>
#endif
#include <sstream>
#include "internal.h"
#include <unordered_map>
#include <array>
#include <iomanip>
#include <vector>
#include <cstring>

class PreRendered {
  const bool useHalocolor;
  const int width;
  const int height;

public:
  int x, y;
  int len;
  int xstart;
  int text_width; // draw this amount of pixels starting from horizontal index xstart
  int ystart; // vertical visibility: starting row in stringbitmap
  int yend;   // vertical visibility: ending row in stringbitmap
  int stringbitmap_height; // font height plus optinally added top/bottom line
  const int safety_bits_x; // extra left and right playground for chroma rendering

  std::vector<std::vector<uint8_t>> stringbitmap;
  std::vector<std::vector<uint8_t>> stringbitmap_outline;

  PreRendered(
    const uint8_t* fonts,
    const int fontline_bytes,
    const int _width, const int _height,
    int _x, int _y, // they may change
    std::vector<int>& s, // it may get shortened
    const int align,
    const bool _useHalocolor,
    const int FONT_WIDTH, const int FONT_HEIGHT,
    const int _safety_bits_x);

  void make_outline();
};

class BitmapFont {

  int number_of_chars;
  std::string font_name;
  std::string font_filename;

public:
  const int width;
  const int height;
  const bool bold;

  std::vector<uint8_t> font_bitmaps;
  const int fontline_bytes;

  std::unordered_map<uint16_t, int> charReMap; // unicode code point vs. font image index

  void SaveAsC(const uint16_t* _codepoints);

  BitmapFont(int _number_of_chars,
    const uint16_t* _src_font_bitmaps_internaluint16, 
    const uint8_t* _src_font_bitmaps, 
    const int _fontline_bytes, 
    const uint16_t* _codepoints, 
    int _w, int _h, std::string _font_name, std::string _font_filename, bool _bold, bool debugSave) :
    number_of_chars(_number_of_chars),
    font_name(_font_name),
    font_filename(_font_filename),
    width(_w), height(_h),
    fontline_bytes(_fontline_bytes),
    bold(_bold)
    //font_bitmaps(_font_bitmaps),
  {
    //fixme: do not copy data
    const int charline_count = height * number_of_chars;
    font_bitmaps.resize(charline_count * fontline_bytes);
    if (_src_font_bitmaps != nullptr) 
      std::memcpy(font_bitmaps.data(), _src_font_bitmaps, font_bitmaps.size());
    else {
      // this must be an internal, predefined array
      // copy uint16_t array to byte array MSB-LSB order 
      // fontline_bytes is 2
      const uint16_t* src = _src_font_bitmaps_internaluint16;
      uint8_t* dst = font_bitmaps.data();
      for (auto i = 0; i < charline_count; i++) {
        uint16_t one_fontline = src[i];
        dst[i * 2] = (uint8_t)(one_fontline >> 8);
        dst[i * 2 + 1] = (uint8_t)(one_fontline & 0xFF);
      }
    }

    for (int i = 0; i < _number_of_chars; i++) {
      charReMap[_codepoints[i]] = i;
    }

    if (debugSave)
      SaveAsC(_codepoints);
  }

  // helper function for remapping a wchar_t string to font index entry list
  std::vector<int> remap(const std::wstring& s16);

};

std::unique_ptr<BitmapFont> GetBitmapFont(int size, const char* name, bool bold, bool debugSave);

void SimpleTextOutW(BitmapFont* current_font, const VideoInfo& vi, PVideoFrame& frame, int real_x, int real_y, std::wstring& text, bool fadeBackground, int textcolor, int halocolor, bool useHaloColor, int align);
void SimpleTextOutW_multi(BitmapFont* current_font, const VideoInfo& vi, PVideoFrame& frame, int real_x, int real_y, std::wstring& text, bool fadeBackground, int textcolor, int halocolor, bool useHaloColor, int align, int lsp);

// legacy function w/o outline, originally with ASCII input, background fading
void DrawStringPlanar(VideoInfo& vi, PVideoFrame& dst, int x, int y, const char* s);
void DrawStringYUY2(VideoInfo& vi, PVideoFrame& dst, int x, int y, const char* s);
void DrawStringRGB32(VideoInfo& vi, PVideoFrame& dst, int x, int y, const char* s);
void DrawStringRGB24(VideoInfo& vi, PVideoFrame& dst, int x, int y, const char* s);

#endif  // __INFO_H__
