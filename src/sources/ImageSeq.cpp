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



#include "stdafx.h"

#include "ImageSeq.h"

#define TEXT_COLOR 0xf0f080


/********************************************************************
***** Declare index of new filters for Avisynth's filter engine *****
********************************************************************/

extern const AVSFunction Image_filters[] = {
  { "ImageWriter", "c[file]s[start]i[end]i[type]s[info]b", ImageWriter::Create },
    // clip, base filename, start, end, image format/extension, info
  { "ImageReader", "[file]s[start]i[end]i[fps]f[use_devil]b[info]b[pixel_type]s", ImageReader::Create },
    // base filename (sprintf-style), start, end, frames per second, default reader to use, info, pixel_type
  { "ImageSource", "[file]s[start]i[end]i[fps]f[use_devil]b[info]b[pixel_type]s", ImageReader::Create },
  { "ImageSourceAnim", "[file]s[fps]f[info]b[pixel_type]s", ImageReader::CreateAnimated },
  { 0 }
};

// Since devIL isn't threadsafe, we need to ensure that only one thread at the time requests frames
CRITICAL_SECTION FramesCriticalSection;
volatile long refcount = 0;
volatile ILint DevIL_Version = 0;

static char* GetWorkingDir(char* buf, size_t bufSize)
{
    assert(buf != NULL);
    DWORD len = GetCurrentDirectory(bufSize - 1, buf);

    // if the retrieved directory name doesn't end in a trailing slash, add one
    if (len > 0 && len < bufSize - 1 && buf[len - 1] != '\\')
    {
        buf[len] = '\\';
        buf[len + 1] = '\0';
        ++len;
    }
    return buf;
}


static bool IsAbsolutePath(const char* path)
{
    assert(path != NULL);
    return    strchr(path, ':') != NULL
              // Allow UNC paths too
           || (path[0] == '\\' && path[1] == '\\')
           || (path[0] == '/' && path[1] == '/');
}


/*****************************
 *******   Image Writer ******
 ****************************/

ImageWriter::ImageWriter(PClip _child, const char * _base_name, const int _start, const int _end,
                         const char * _ext, bool _info, IScriptEnvironment* env)
 : GenericVideoFilter(_child), ext(_ext), info(_info)
{
  // Generate full name
  if (IsAbsolutePath(_base_name))
  {
    base_name[0] = '\0';
    strncat(base_name, _base_name, sizeof base_name);
  }
  else
  {
    char cwd[MAX_PATH + 1];
    GetWorkingDir(cwd, sizeof cwd);
    _snprintf(base_name, sizeof base_name, "%s%s", cwd, _base_name);
  }

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
    infoHeader.biBitCount = WORD(vi.BitsPerPixel());
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
    if (IL_TRUE == ilTexImage(vi.width, vi.height, 1, ILubyte(vi.BitsPerPixel() / 8), il_format, IL_UNSIGNED_BYTE, NULL)) {

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


AVSValue __cdecl ImageWriter::Create(AVSValue args, void*, IScriptEnvironment* env)
{
  return new ImageWriter(args[0].AsClip(),
                         env->SaveString(args[1].AsString("c:\\")),
                         args[2].AsInt(0),
                         args[3].AsInt(0),
                         env->SaveString(args[4].AsString("ebmp")),
                         args[5].AsBool(false), env);
}




/*****************************
 *******   Image Reader ******
 ****************************/
ImageReader::ImageReader(const char * _base_name, const int _start, const int _end,
                         const double _fps, bool _use_DevIL, bool _info, const char * _pixel,
                         bool _animation, IScriptEnvironment* env)
 : start(_start), use_DevIL(_use_DevIL), info(_info), animation(_animation), framecopies(0)
{
  if (DevIL_Version == 0) // Init the DevIL.dll version
    DevIL_Version = ilGetInteger(IL_VERSION_NUM);

  // Generate full name
  if (IsAbsolutePath(_base_name))
  {
    base_name[0] = '\0';
    strncat(base_name, _base_name, sizeof base_name);
  }
  else
  {
    char cwd[MAX_PATH + 1];
    GetWorkingDir(cwd, sizeof cwd);
    _snprintf(base_name, sizeof base_name, "%s%s", cwd, _base_name);
  }
  _snprintf(filename, (sizeof filename)-1, base_name, start);

  memset(&vi, 0, sizeof(vi));

  // Invariants
  vi.num_frames = -start + _end + 1;  // make sure each frame can be requested
  vi.audio_samples_per_second = 0;
  double num = _fps;  // calculate fps as num/denom for vi
  unsigned denom = 1;
  while (num < 16777216 && denom < 16777216) { num*=2; denom*=2; } // float mantissa is only 24 bits
  vi.SetFPS(unsigned(num+0.5), denom); // And normalize

  if (use_DevIL == false)
  {
    fileHeader.bfType = 0;
    // Try to parse as bmp/ebmp
    ifstream file(filename, ios::binary);
    file.read( reinterpret_cast<char *> (&fileHeader), sizeof(fileHeader) );
    file.read( reinterpret_cast<char *> (&infoHeader), sizeof(infoHeader) );
    file.close();

    if ( fileHeader.bfType == ('M' << 8) + 'B')
    {
      if (infoHeader.biCompression == 0) {
        vi.width = infoHeader.biWidth;
        vi.height = infoHeader.biHeight;

        if (infoHeader.biPlanes == 1) {
          if (infoHeader.biBitCount == 32)
            vi.pixel_type = VideoInfo::CS_BGR32;
          else if (infoHeader.biBitCount == 24)
            vi.pixel_type = VideoInfo::CS_BGR24;
          else if (infoHeader.biBitCount == 16)
            vi.pixel_type = VideoInfo::CS_YUY2;
          else if (infoHeader.biBitCount == 8)
            vi.pixel_type = VideoInfo::CS_Y8;
          else if (DevIL_Version <= 166)
            // DevIL 1.6.6 has a major coronary with palletted BMP files so don't fail thru to it
            env->ThrowError("ImageReader: %d bit BMP is unsupported.", infoHeader.biBitCount);
          else
            use_DevIL = true; // give it to DevIL (for example: biBitCount == 1 or 4 bit)
        }
        else if (infoHeader.biPlanes == 3) {
          if (infoHeader.biBitCount == 24)
            vi.pixel_type = VideoInfo::CS_YV24;
          else if (infoHeader.biBitCount == 16)
            vi.pixel_type = VideoInfo::CS_YV16;
          else if (infoHeader.biBitCount == 12) {
            if (!lstrcmpi(_pixel, "rgb24")) // Hack - the default text is "rgb24"
              vi.pixel_type = VideoInfo::CS_YV12;
            else if (!lstrcmpi(_pixel, "yv12"))
              vi.pixel_type = VideoInfo::CS_YV12;
            else if (!lstrcmpi(_pixel, "yv411"))
              vi.pixel_type = VideoInfo::CS_YV411;
            else
              env->ThrowError("ImageReader: 12 bit, 3 plane EBMP: Pixel_type must be \"YV12\" or \"YV411\".");
          }
          else
            env->ThrowError("ImageReader: %d bit, 3 plane EBMP is unsupported.", infoHeader.biBitCount);
        }
        else
          env->ThrowError("ImageReader: %d plane BMP is unsupported.", infoHeader.biPlanes);

        if (DevIL_Version > 166 && (infoHeader.biWidth <= 0 || infoHeader.biHeight <= 0)) {
            use_DevIL = true; // Not values we know, give it to DevIL
        }
        else {
          if (infoHeader.biWidth <= 0)
            // use_DevIL = true; // Not a type we know, give it to DevIL
            env->ThrowError("ImageReader: Unsupported width %d", infoHeader.biWidth);
          if (infoHeader.biHeight <= 0)
            // use_DevIL = true; // Not a type we know, give it to DevIL
            env->ThrowError("ImageReader: Unsupported height %d", infoHeader.biHeight);
        }
      }
      else {
        // DevIL 1.6.6 has a major coronary with compressed BMP files so don't fail thru to it
        if (DevIL_Version <= 166)
          env->ThrowError("ImageReader: EBMP reader cannot handle compressed images.");

        use_DevIL = true; // give it to DevIL (image is compressed)
      }
    }
    else {
      use_DevIL = true; // Not a BMP, give it to DevIL
    }
  }

  if (use_DevIL == true) {  // attempt to open via DevIL

    if (InterlockedIncrement(&refcount) == 1) {
      if (!InitializeCriticalSectionAndSpinCount(&FramesCriticalSection, 1000) ) {
        DWORD error = GetLastError();
        if (error) {
          InterlockedExchange(&refcount, 0);
          env->ThrowError("ImageReader: Could not initialize critical section, 0x%x", error);
        }
      }
    }

    EnterCriticalSection(&FramesCriticalSection);

    ilInit();

    ILuint myImage=0;
    ilGenImages(1, &myImage);
    ilBindImage(myImage);

    ilLoadImage(filename);

    vi.width = ilGetInteger(IL_IMAGE_WIDTH);
    vi.height = ilGetInteger(IL_IMAGE_HEIGHT);

    if (!lstrcmpi(_pixel, "rgb")) {
      vi.pixel_type = VideoInfo::CS_BGR32;
    }
    else if (!lstrcmpi(_pixel, "rgb32")) {
      vi.pixel_type = VideoInfo::CS_BGR32;
    }
    else if (!lstrcmpi(_pixel, "rgb24")) {
      vi.pixel_type = VideoInfo::CS_BGR24;
    }
    else if (!lstrcmpi(_pixel, "y8")) {
      vi.pixel_type = VideoInfo::CS_Y8;
    }
    else {
      LeaveCriticalSection(&FramesCriticalSection);
      env->ThrowError("ImageReader: supports the following pixel types: RGB24, RGB32 or Y8");
    }

    if (animation) {
      vi.num_frames = ilGetInteger(IL_NUM_IMAGES) + 1; // bug in DevIL (ilGetInteger is one off in 166 en 178)
      if (vi.num_frames <= 0) {
        LeaveCriticalSection(&FramesCriticalSection);
        env->ThrowError("ImageSourceAnim: DevIL can't detect the number of images in the animation");
      }

      unsigned duration_ms = (unsigned)ilGetInteger(IL_IMAGE_DURATION);
      if (duration_ms != 0 && _fps == 24.0) { // overwrite framerate in case of non-zero duration
          vi.SetFPS(1000, duration_ms);
      }
    }

    // Get errors if any
    // (note: inability to parse an (e)bmp will show up here as a DevIL error)
    ILenum err = ilGetError();

    ilDeleteImages(1, &myImage);

    LeaveCriticalSection(&FramesCriticalSection);

    if (err != IL_NO_ERROR) {
      env->ThrowError("ImageReader: error '%s' in DevIL library.\nreading file \"%s\"\nDevIL version %d.", getErrStr(err), filename, DevIL_Version);
    }
    // work around DevIL upside-down bug with compressed images
    should_flip = false;
    const char * ext = strrchr(_base_name, '.') + 1;
    if (  !lstrcmpi(ext, "jpeg") || !lstrcmpi(ext, "jpg") || !lstrcmpi(ext, "jpe") || !lstrcmpi(ext, "dds") ||
          !lstrcmpi(ext, "pal") || !lstrcmpi(ext, "psd") || !lstrcmpi(ext, "pcx") || !lstrcmpi(ext, "png") ||
          !lstrcmpi(ext, "pbm") || !lstrcmpi(ext, "pgm") || !lstrcmpi(ext, "ppm") || !lstrcmpi(ext, "gif") ||
          !lstrcmpi(ext, "exr") || !lstrcmpi(ext, "jp2") || !lstrcmpi(ext, "hdr") )
    {
      should_flip = true;
    }
    else if ((DevIL_Version > 166) && (!lstrcmpi(ext, "tif") || !lstrcmpi(ext, "tiff")))
    {
      should_flip = true;
    }
    // flip back for Y8
    if (vi.IsY8()) {
        should_flip = !should_flip;
    }
  }

  // undecorated filename means they want a single, static image or an animation
  if (strcmp(filename, base_name) == 0) {
    if (animation)
      framecopies = 1;
    else
      framecopies = vi.num_frames;
  }
}


ImageReader::~ImageReader()
{
  if (use_DevIL) {
    EnterCriticalSection(&FramesCriticalSection);
    ilShutDown();
    LeaveCriticalSection(&FramesCriticalSection);

    if (InterlockedDecrement(&refcount) == 0)
      DeleteCriticalSection(&FramesCriticalSection);
  }
}

/*  Notes to clear thinking!

  vi.num_frames = end-start+1
  0 <= n < vi.num_frames

  for animation=true: vi.num_frames = ilGetInteger(IL_NUM_IMAGES)
*/

PVideoFrame ImageReader::GetFrame(int n, IScriptEnvironment* env)
{
  ILenum err = IL_NO_ERROR;

  PVideoFrame frame = env->NewVideoFrame(vi);
  BYTE * dstPtr = frame->GetWritePtr();
  BYTE * const WritePtr = dstPtr;

  const int pitch = frame->GetPitch();
  const int row_size = frame->GetRowSize();
  const int height = frame->GetHeight();
  const int width = vi.width;

  _snprintf(filename, (sizeof filename)-1, base_name, n+start);

  if (use_DevIL)  /* read using DevIL */
  {
    EnterCriticalSection(&FramesCriticalSection);

    // Setup
    ILuint myImage=0;
    ilGenImages(1, &myImage);
    ilBindImage(myImage);

    if (ilLoadImage(filename) == IL_FALSE) {
      // Get errors if any
      err = ilGetError();

      // Cleanup
      ilDeleteImages(1, &myImage);

      LeaveCriticalSection(&FramesCriticalSection);

      memset(WritePtr, 0, pitch * height);  // Black frame
      if ((info) || (err != IL_COULD_NOT_OPEN_FILE)) {
        ostringstream ss;
        ss << "ImageReader: error '" << getErrStr(err) << "' in DevIL library\n"
		      "opening file \"" << filename << "\"\n"
              "DevIL version " << DevIL_Version << ".";
        env->ApplyMessage(&frame, vi, ss.str().c_str(), vi.width/4, TEXT_COLOR, 0, 0);
      }
      return frame;
    }

    if (animation) {
      if (ilActiveImage(n) == IL_FALSE) { // load image N from file
        // Get errors if any
        err = ilGetError();

        // Cleanup
        ilDeleteImages(1, &myImage);

        LeaveCriticalSection(&FramesCriticalSection);

        memset(WritePtr, 0, pitch * height);  // Black frame
        if (info) {
          ostringstream ss;
          ss << "ImageSourceAnim: error '" << getErrStr(err) << "' in DevIL library\n"
                "processing image " << n << " from file \"" << filename << "\"\n"
                "DevIL version " << DevIL_Version << ".";
          env->ApplyMessage(&frame, vi, ss.str().c_str(), vi.width/4, TEXT_COLOR, 0, 0);
        }
        return frame;
      }
    }
    else {
      // Check some parameters
      if (ilGetInteger(IL_IMAGE_HEIGHT) != height)
      {
        // Cleanup
        ilDeleteImages(1, &myImage);

        LeaveCriticalSection(&FramesCriticalSection);

        memset(WritePtr, 0, pitch * height);
        env->ApplyMessage(&frame, vi, "ImageReader: images must have identical heights", vi.width/4, TEXT_COLOR, 0, 0);
        return frame;
      }

      if (ilGetInteger(IL_IMAGE_WIDTH) != width)
      {
        // Cleanup
        ilDeleteImages(1, &myImage);

        LeaveCriticalSection(&FramesCriticalSection);

        memset(WritePtr, 0, pitch * height);
        env->ApplyMessage(&frame, vi, "ImageReader: images must have identical widths", vi.width/4, TEXT_COLOR, 0, 0);
        return frame;
      }
    }

    const ILenum il_format = vi.IsY8() ? IL_LUMINANCE : ( vi.IsRGB32() ? IL_BGRA : IL_BGR );
    const ILenum linesize = width * ( vi.IsY8() ? 1 : ( vi.IsRGB32() ? 4 : 3 ) );
    const int height_image = min(height, ilGetInteger(IL_IMAGE_HEIGHT));
    const int width_image = min(width, ilGetInteger(IL_IMAGE_WIDTH));
    const ILenum linesize_image = width_image * ( vi.IsY8() ? 1 : ( vi.IsRGB32() ? 4 : 3 ) );

    if (!vi.IsY8()) {
      // fill bottom with black pixels
      memset(dstPtr, 0, pitch * (height-height_image));
      dstPtr += pitch * (height-height_image);
    }

    // Copy raster to AVS frame
////if (ilGetInteger(IL_ORIGIN_MODE) == IL_ORIGIN_UPPER_LEFT, IL_ORIGIN_LOWER_LEFT ???
    if (should_flip)
    {
      // Copy upside down
      for (int y=height_image-1; y>=0; --y)
      {
        if (ilCopyPixels(0, y, 0, width_image, 1, 1, il_format, IL_UNSIGNED_BYTE, dstPtr) > linesize)
          break; // Try not to spew all over memory
        memset(dstPtr+linesize_image, 0, linesize-linesize_image);
        dstPtr += pitch;
      }
    }
    else {
      // Copy right side up
      for (int y=0; y<height_image; ++y)
      {
        if (ilCopyPixels(0, y, 0, width_image, 1, 1, il_format, IL_UNSIGNED_BYTE, dstPtr) > linesize)
          break; // Try not to spew all over memory
        memset(dstPtr+linesize_image, 0, linesize-linesize_image);
        dstPtr += pitch;
      }
    }

    if (vi.IsY8()) {
      // fill bottom with black pixels
      memset(dstPtr, 0, pitch * (height-height_image));
    }

    // Get errors if any
    err = ilGetError();

    // Cleanup
    ilDeleteImages(1, &myImage);

    LeaveCriticalSection(&FramesCriticalSection);

    if (err != IL_NO_ERROR)
    {
      memset(WritePtr, 0, pitch * height);
      ostringstream ss;
      ss << "ImageReader: error '" << getErrStr(err) << "' in DevIL library\n"
            "reading file \"" << filename << "\"\n"
            "DevIL version " << DevIL_Version << ".";
      env->ApplyMessage(&frame, vi, ss.str().c_str(), vi.width/4, TEXT_COLOR, 0, 0);
      return frame;
    }
  }
  else {  /* treat as ebmp  */
    // Open file, ensure it has the expected properties
    ifstream file(filename, ios::binary);
    if (!checkProperties(file, frame, env)) {
      file.close();
      return frame;
    }

    // Seek past padding
    file.seekg (fileHeader.bfOffBits, ios::beg);

    // Read in raster
    if (vi.IsY8())
    {
      // read upside down
      BYTE * endPtr = dstPtr + pitch * (height-1);
      fileRead(file, endPtr, -pitch, row_size, height);
    }
    else
    {
      fileRead(file, dstPtr, pitch, row_size, height);

      if (vi.IsPlanar())
      {
        dstPtr = frame->GetWritePtr(PLANAR_U);
        const int pitchUV = frame->GetPitch(PLANAR_U);
        const int row_sizeUV = frame->GetRowSize(PLANAR_U);
        const int heightUV = frame->GetHeight(PLANAR_U);
        fileRead(file, dstPtr, pitchUV, row_sizeUV, heightUV);

        dstPtr = frame->GetWritePtr(PLANAR_V);
        fileRead(file, dstPtr, pitchUV, row_sizeUV, heightUV);
      }
    }

    file.close();
  }

  if (info) {
    // overlay on video output: progress indicator
    ostringstream text;
    text << "Frame " << n << ".\n"
            "Read from \"" << filename << "\"\n"
            "DevIL version " << DevIL_Version << ".";
    env->ApplyMessage(&frame, vi, text.str().c_str(), vi.width/4, TEXT_COLOR, 0, 0);
  }

  return frame;
}


void ImageReader::fileRead(istream & file, BYTE * dstPtr, const int pitch, const int row_size, const int height)
{
  int padding = (4 - (row_size % 4)) % 4;
  for (int y=0; y<height; ++y)
  {
    file.read( reinterpret_cast<char *> (dstPtr), row_size);
    file.seekg(padding, ios_base::cur);
    dstPtr += pitch;
  }
}


void ImageReader::BlankFrame(PVideoFrame & frame)
{
  const int size = frame->GetPitch() * frame->GetHeight();

  if (vi.IsRGB() || vi.IsY8()) {
    memset(frame->GetWritePtr(), 0, size); // Black frame
  }
  else {
    memset(frame->GetWritePtr(), 128, size); // Grey frame

    const int UVpitch = frame->GetPitch(PLANAR_U);
    if (UVpitch) {
      const int UVsize = UVpitch * frame->GetHeight(PLANAR_U);

      memset(frame->GetWritePtr(PLANAR_U), 128, UVsize);
      memset(frame->GetWritePtr(PLANAR_V), 128, UVsize);
    }
  }
}


void ImageReader::BlankApplyMessage(PVideoFrame & frame, const char * text, IScriptEnvironment * env)
{
  BlankFrame(frame);
  env->ApplyMessage(&frame, vi, text, vi.width/4, TEXT_COLOR, 0, 0);
}


bool ImageReader::checkProperties(ifstream & file, PVideoFrame & frame, IScriptEnvironment * env)
{
  if (!file.is_open())
  {
    if (info)
      BlankApplyMessage(frame, "ImageReader: cannot open file", env);
    else
      BlankFrame(frame);

    return false;
  }

  BITMAPFILEHEADER tempFileHeader;
  BITMAPINFOHEADER tempInfoHeader;

  file.read( reinterpret_cast<char *> (&tempFileHeader), sizeof(tempFileHeader) );
  file.read( reinterpret_cast<char *> (&tempInfoHeader), sizeof(tempInfoHeader) );

  if (tempFileHeader.bfType != fileHeader.bfType)
  {
    BlankApplyMessage(frame, "ImageReader: invalid (E)BMP file", env);
    return false;
  }

  if (tempInfoHeader.biWidth != infoHeader.biWidth)
  {
    BlankApplyMessage(frame, "ImageReader: image widths must be identical", env);
    return false;
  }

  if (tempInfoHeader.biHeight != infoHeader.biHeight)
  {
    BlankApplyMessage(frame, "ImageReader: image heights must be identical", env);
    return false;
  }

  if (tempInfoHeader.biPlanes != infoHeader.biPlanes)
  {
    BlankApplyMessage(frame, "ImageReader: images must have the same number of planes", env);
    return false;
  }

  if (tempInfoHeader.biBitCount != infoHeader.biBitCount)
  {
    BlankApplyMessage(frame, "ImageReader: images must have identical bits per pixel", env);
    return false;
  }

  if (tempFileHeader.bfSize != fileHeader.bfSize)
  {
    BlankApplyMessage(frame, "ImageReader: raster sizes must be identical", env);
    return false;
  }

  if (tempInfoHeader.biCompression != 0)
  {
    BlankApplyMessage(frame, "ImageReader: EBMP reader cannot handle compressed images", env);
    return false;
  }

  return true;
}

AVSValue __cdecl ImageReader::Create(AVSValue args, void*, IScriptEnvironment* env)
{
  const char * path = args[0].AsString("c:\\%06d.ebmp");

  ImageReader *IR = new ImageReader(path, args[1].AsInt(0), args[2].AsInt(1000), args[3].AsDblDef(24.0),
                                    args[4].AsBool(false), args[5].AsBool(false), args[6].AsString("rgb24"),
                                    /*animation*/ false, env);
  // If we are returning a stream of 2 or more copies of the same image
  // then use FreezeFrame and the Cache to minimise any reloading.
  if (IR->framecopies > 1) {
    AVSValue cache = env->Invoke("Cache", AVSValue(IR));
    AVSValue ff_args[4] = { cache, 0, IR->framecopies-1, 0 };
    return env->Invoke("FreezeFrame", AVSValue(ff_args, 4)).AsClip();
  }

  return IR;

}

AVSValue __cdecl ImageReader::CreateAnimated(AVSValue args, void*, IScriptEnvironment* env)
{
  if (!args[0].IsString())
    env->ThrowError("ImageSourceAnim: You must specify a filename.");

  return new ImageReader(args[0].AsString(), 0, 0, args[1].AsDblDef(24.0), /*use_DevIL*/ true,
                         args[2].AsBool(false), args[3].AsString("rgb32"), /*animation*/ true, env);
}

const char *const getErrStr(ILenum err)
{
  if (err == IL_INVALID_ENUM)
    return "Invalid Enum";
  if (err == IL_OUT_OF_MEMORY)
    return "Out of memory";
  if (err == IL_FORMAT_NOT_SUPPORTED)
    return "Format not supported";
  if (err == IL_INTERNAL_ERROR)
    return "Internal error";
  if (err == IL_INVALID_VALUE)
    return "Invalid value";
  if (err == IL_ILLEGAL_OPERATION)
    return "Illegal operation";
  if (err == IL_ILLEGAL_FILE_VALUE)
    return "Illegal file value";
  if (err == IL_INVALID_FILE_HEADER)
    return "Illegal file header";
  if (err == IL_INVALID_PARAM)
    return "Invalid Parameter";
  if (err == IL_COULD_NOT_OPEN_FILE)
    return "Could not open file";
  if (err == IL_INVALID_EXTENSION)
    return "Invalid extension";
  if (err == IL_FILE_ALREADY_EXISTS)
    return "File already exists";
  if (err == IL_OUT_FORMAT_SAME)
    return "Output format same";
  if (err == IL_STACK_OVERFLOW)
    return "Stack overflow";
  if (err == IL_STACK_UNDERFLOW)
    return "Stack underflow";
  if (err == IL_INVALID_CONVERSION)
    return "Invalid conversion";
  if (err == IL_BAD_DIMENSIONS)
    return "Bad dimensions";
  if (err == IL_FILE_READ_ERROR)
    return "File read error";
  if (err == IL_FILE_WRITE_ERROR)
    return "File write error";
  if (err == IL_LIB_GIF_ERROR)
    return "LibGif error";
  if (err == IL_LIB_JPEG_ERROR)
    return "LibJpeg error";
  if (err == IL_LIB_PNG_ERROR)
    return "LibPng error";
  if (err == IL_LIB_TIFF_ERROR)
    return "LibTiff error";
  if (err == IL_LIB_MNG_ERROR)
    return "LibMng error";
#if IL_VERSION >= 178
  if (err == IL_LIB_JP2_ERROR)
    return "LibJP2 error";
  if (err == IL_LIB_EXR_ERROR)
    return "LibExr error";
#endif
  if (err == IL_UNKNOWN_ERROR)
    return "Unknown error";

  return "Unknown error";
}

