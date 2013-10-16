// Avisynth v2.5.  Copyright 2002 Ben Rudiak-Gould et al.
// http://www.avisynth.org

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



#include <avisynth.h>
#include "ImageSeq.h"
#include <algorithm>

#define TEXT_COLOR 0xf0f080

using namespace std;


ImageWriter::ImageWriter(PClip _child, const char * _base_name, const int _start, const int _end,
                         const char * _ext, bool _info, IScriptEnvironment* env)
 : GenericVideoFilter(_child), ext(_ext), info(_info)
{
  // Make sure we have an absolute path.
  DWORD len = GetFullPathName(_base_name, 0, base_name, NULL);
  if (len > sizeof(base_name))
    env->ThrowError("Path to %s too long.", _base_name);
  (void)GetFullPathName(_base_name, len, base_name, NULL);

  if (strchr(base_name, '%') == NULL) {
    base_name[(sizeof base_name)-8] = '\0';
    strcat(base_name, "%06d.%s"); // Append default formating
  }

  if (!lstrcmpi(ext, "ebmp"))
  {
    // construct file header
    fileHeader.bfType = ('M' << 8) + 'B'; // I hate little-endian
    fileHeader.bfSize = vi.BMPSize(); // includes 4-byte padding
    fileHeader.bfReserved1 = 0;
    fileHeader.bfReserved2 = 0;
    fileHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    fileHeader.bfSize += fileHeader.bfOffBits;

    // construct info header
    infoHeader.biSize = sizeof(BITMAPINFOHEADER);
    infoHeader.biWidth = vi.width;
    infoHeader.biHeight = vi.height;
    infoHeader.biPlanes = (vi.IsPlanar() && !vi.IsY8()) ? 3 : 1;
    infoHeader.biBitCount = vi.BitsPerPixel();
    infoHeader.biCompression = 0;
    infoHeader.biSizeImage = fileHeader.bfSize - fileHeader.bfOffBits;
    infoHeader.biXPelsPerMeter = 0;
    infoHeader.biYPelsPerMeter = 0;
    infoHeader.biClrUsed = 0;
    infoHeader.biClrImportant = 0;
  }
  else {
    if (!(vi.IsY8()||vi.IsRGB()))
      env->ThrowError("ImageWriter: DevIL requires RGB or Y8 input");

    if (InterlockedIncrement(&refcount) == 1) {
      if (!InitializeCriticalSectionAndSpinCount(&FramesCriticalSection, 1000) ) {
        DWORD error = GetLastError();
        if (error) {
          InterlockedExchange(&refcount, 0);
          env->ThrowError("ImageWriter: Could not initialize critical section, 0x%x", error);
        }
      }
    }

    EnterCriticalSection(&FramesCriticalSection);
    ilInit();
    LeaveCriticalSection(&FramesCriticalSection);
  }

  start = max(_start, 0);

  if (_end==0)
    end = vi.num_frames-1;
  else if (_end<0)
    end = start - _end - 1;
  else
    end = _end;

  end = max(end, start);
}


ImageWriter::~ImageWriter()
{
  if (!!lstrcmpi(ext, "ebmp")) {
    EnterCriticalSection(&FramesCriticalSection);
    ilShutDown();
    LeaveCriticalSection(&FramesCriticalSection);

    if (InterlockedDecrement(&refcount) == 0)
      DeleteCriticalSection(&FramesCriticalSection);
  }
}



PVideoFrame ImageWriter::GetFrame(int n, IScriptEnvironment* env)
{
  PVideoFrame frame = child->GetFrame(n, env);

  // check bounds
  if ((n<start)||(n>end))
  {
    if (info) {
      ostringstream ss;
      ss << "ImageWriter: frame " << n << " not in range";
      env->MakeWritable(&frame);
      env->ApplyMessage(&frame, vi, ss.str().c_str(), vi.width/4, TEXT_COLOR, 0, 0);
    }
    return frame;
  }

  // construct filename
  char filename[MAX_PATH + 1];
  _snprintf(filename, MAX_PATH, base_name, n, ext, 0, 0);
  filename[MAX_PATH] = '\0';

  if (!lstrcmpi(ext, "ebmp"))  /* Use internal 'ebmp' writer */
  {
    // initialize file object
    ofstream file(filename, ios::out | ios::trunc | ios::binary);
    if (!file)
    {
      ostringstream ss;
      ss << "ImageWriter: could not create file '" << filename << "'";
      env->MakeWritable(&frame);
      env->ApplyMessage(&frame, vi, ss.str().c_str(), vi.width/4, TEXT_COLOR, 0, 0);
      return frame;
    }

    // write headers
    file.write(reinterpret_cast<const char *>( &fileHeader ), sizeof(BITMAPFILEHEADER));
    file.write(reinterpret_cast<const char *>( &infoHeader ), sizeof(BITMAPINFOHEADER));

    // write raster
    const BYTE * srcPtr = frame->GetReadPtr();
    int pitch = frame->GetPitch();
    int row_size = frame->GetRowSize();
    int height = frame->GetHeight();

    if (vi.IsY8())
    {
      // write upside down
      const BYTE * endPtr = srcPtr + pitch * (height-1);
      fileWrite(file, endPtr, -pitch, row_size, height);
    }
    else
    {
      fileWrite(file, srcPtr, pitch, row_size, height);

      if (vi.IsPlanar())
      {
        srcPtr = frame->GetReadPtr(PLANAR_U);
        pitch = frame->GetPitch(PLANAR_U);
        row_size = frame->GetRowSize(PLANAR_U);
        height = frame->GetHeight(PLANAR_U);
        fileWrite(file, srcPtr, pitch, row_size, height);

        srcPtr = frame->GetReadPtr(PLANAR_V);
        fileWrite(file, srcPtr, pitch, row_size, height);
      }
    }

    // clean up
    file.close();
  }
  else { /* Use DevIL library */
    EnterCriticalSection(&FramesCriticalSection);

    // Set up DevIL
    ILuint myImage=0;
    ilGenImages(1, &myImage); // Initialize 1 image structure
    ilBindImage(myImage);     // Set this as the current image

    const ILenum il_format = vi.IsY8() ? IL_LUMINANCE : ( vi.IsRGB32() ? IL_BGRA : IL_BGR );

    // Set image parameters
    if (IL_TRUE == ilTexImage(vi.width, vi.height, 1, vi.BitsPerPixel() / 8, il_format, IL_UNSIGNED_BYTE, NULL)) {

      // Program actual image raster
      const BYTE * srcPtr = frame->GetReadPtr();
      int pitch = frame->GetPitch();
      for (int y=0; y<vi.height; ++y)
      {
        ilSetPixels(0, y, 0, vi.width, 1, 1, il_format, IL_UNSIGNED_BYTE, (void*) srcPtr);
        srcPtr += pitch;
      }

      // DevIL writer fails if the file exists, so delete first
      DeleteFile(filename);

      // Save to disk (format automatically inferred from extension)
      ilSaveImage(filename);
    }

    // Get errors if any
    ILenum err = ilGetError();

    // Clean up
    ilDeleteImages(1, &myImage);

    LeaveCriticalSection(&FramesCriticalSection);

    if (err != IL_NO_ERROR)
    {
      ostringstream ss;
      ss << "ImageWriter: error '" << getErrStr(err) << "' in DevIL library\n"
	        "writing file \"" << filename << "\"\n"
            "DevIL version " << DevIL_Version << ".";
      env->MakeWritable(&frame);
      env->ApplyMessage(&frame, vi, ss.str().c_str(), vi.width/4, TEXT_COLOR, 0, 0);
      return frame;
    }
  }

  if (info) {
    // overlay on video output: progress indicator
    ostringstream text;
    text << "Frame " << n << " written to: " << filename;
    env->MakeWritable(&frame);
    env->ApplyMessage(&frame, vi, text.str().c_str(), vi.width/4, TEXT_COLOR, 0, 0);
  }

  return frame;
}


void ImageWriter::fileWrite(ostream & file, const BYTE * srcPtr, const int pitch, const int row_size, const int height)
{
  int dummy = 0;
  int padding = (4 - (row_size % 4)) % 4;

  for(int i=0; i < height; ++i)
  {
    file.write(reinterpret_cast<const char *>( srcPtr ), row_size);
    file.write(reinterpret_cast<char *>( &dummy ), padding); // pad with 0's to mod-4
    srcPtr += pitch;
  }
}
