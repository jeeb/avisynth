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

AVSFunction Image_filters[] = {
  { "ImageWriter", "c[file]s[start]i[end]i[type]s[info]b", ImageWriter::Create }, 
    // clip, base filename, start, end, image format/extension, info
  { "ImageReader", "[file]s[start]i[end]i[fps]f[use_devil]b[info]b[pixel_type]s", ImageReader::Create }, 
    // base filename (sprintf-style), start, end, frames per second, default reader to use, info, pixel_type
  { "ImageSource", "[file]s[start]i[end]i[fps]f[use_devil]b[info]b[pixel_type]s", ImageReader::Create },
  { 0 }
};


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
 : GenericVideoFilter(_child), base_name(_base_name), ext(_ext), info(_info)
{  
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
    infoHeader.biPlanes = vi.IsPlanar() ? 3 : 1;
    infoHeader.biBitCount = vi.BitsPerPixel();
    infoHeader.biCompression = 0;
    infoHeader.biSizeImage = fileHeader.bfSize - fileHeader.bfOffBits;
    infoHeader.biXPelsPerMeter = 0;
    infoHeader.biYPelsPerMeter = 0;
    infoHeader.biClrUsed = 0;
    infoHeader.biClrImportant = 0;
  }
  else {
    if (!vi.IsRGB())
      env->ThrowError("ImageWriter: DevIL requires RGB input");

    ilInit();
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
  if (!lstrcmpi(ext, "ebmp"))
  {
  }
  else {
    ilShutDown();
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
      ApplyMessage(&frame, vi, ss.str().c_str(), vi.width/4, TEXT_COLOR,0,0 , env);
    }
    return frame;  
  }

  // construct filename
  ostringstream fn_oss;
  fn_oss << base_name << setfill('0') << setw(6) << n << '.' << ext;
  string filename = fn_oss.str();

  if (!lstrcmpi(ext, "ebmp"))  /* Use internal 'ebmp' writer */
  {
    // initialize file object
    ofstream file(filename.c_str(), ios::out | ios::trunc | ios::binary);  
    if (!file)
    {
      ostringstream ss;
      ss << "ImageWriter: could not create file '" << filename << "'";
      env->MakeWritable(&frame);
      ApplyMessage(&frame, vi, ss.str().c_str(), vi.width/4, TEXT_COLOR,0,0 , env);
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
    
	if (0) // (vi.IsY8())
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

    // Set up DevIL    
    ILuint myImage=0;
    ilGenImages(1, &myImage); // Initialize 1 image structure
    ilBindImage(myImage);     // Set this as the current image
    
	const ILenum il_format = vi.IsRGB32() ? IL_BGRA : IL_BGR;

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
	  DeleteFile(filename.c_str());
	  
	  // Save to disk (format automatically inferred from extension)
	  ilSaveImage(const_cast<char * const> (filename.c_str()) );
	}

    // Get errors if any
    ILenum err = ilGetError();
    
    // Clean up
    ilDeleteImages(1, &myImage);

    if (err != IL_NO_ERROR)
    {   
      ostringstream ss;
      ss << "ImageWriter: error '" << getErrStr(err) << "' in DevIL library\n writing file " << filename;
      env->MakeWritable(&frame);
      ApplyMessage(&frame, vi, ss.str().c_str(), vi.width/4, TEXT_COLOR,0,0 , env);
      return frame;
    }
  }  
    
  if (info) {    
    // overlay on video output: progress indicator
    ostringstream text;
    text << "Frame " << n << " written to: " << filename;
    env->MakeWritable(&frame);
    ApplyMessage(&frame, vi, text.str().c_str(), vi.width/4, TEXT_COLOR,0,0 , env);
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
                         const float _fps, bool _use_DevIL, bool _info, const char * _pixel,
						 IScriptEnvironment* env)
 : base_name(), start(_start), use_DevIL(_use_DevIL), info(_info), framecopies(0)
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
  _snprintf(filename, sizeof filename, base_name, start);

  memset(&vi, 0, sizeof(vi));

  // Invariants
  vi.num_frames = -start + _end + 1;  // make sure each frame can be requested
  vi.audio_samples_per_second = 0;  
  double num = _fps;  // calculate fps as num/denom for vi
  int denom = 1;
  while (num < 16777216 && denom < 16777216) { num*=2; denom*=2; } // float mantissa is only 24 bits
  vi.fps_numerator = int(num+0.5);
  vi.fps_denominator = denom;
    
  // undecorated filename means they want a single, static image
  if( strcmp(filename, base_name) == 0 ) framecopies = vi.num_frames;

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
	  if (infoHeader.biCompression != 0)
		  // use_DevIL = true; // Not a type we know, give it to DevIL
		  env->ThrowError("ImageReader: EBMP reader cannot handle compressed images.");

	  vi.width = infoHeader.biWidth;
	  vi.height = infoHeader.biHeight;

	  if (infoHeader.biPlanes == 1) {
		if (infoHeader.biBitCount == 32)
		  vi.pixel_type = VideoInfo::CS_BGR32;
		else if (infoHeader.biBitCount == 24)
		  vi.pixel_type = VideoInfo::CS_BGR24;
		else if (infoHeader.biBitCount == 16)
		  vi.pixel_type = VideoInfo::CS_YUY2;
		else
		  // use_DevIL = true; // Not a type we know, give it to DevIL
		  // DevIL 1.6.6 has a major coronary with palletted BMP files so don't fail thru to it
		  env->ThrowError("ImageReader: %d bit BMP is unsupported.", infoHeader.biBitCount);
	  }
	  else if (infoHeader.biPlanes == 3) {
		if (infoHeader.biBitCount == 12)
			vi.pixel_type = VideoInfo::CS_YV12;
		else
		  env->ThrowError("ImageReader: %d bit, 3 plane EBMP is unsupported.", infoHeader.biBitCount);
	  }
	  else
		env->ThrowError("ImageReader: %d plane BMP is unsupported.", infoHeader.biPlanes);

	  if (infoHeader.biWidth <= 0)
		// use_DevIL = true; // Not a type we know, give it to DevIL
		env->ThrowError("ImageReader: Unsupported width %d", infoHeader.biWidth);
	  if (infoHeader.biHeight <= 0)
		// use_DevIL = true; // Not a type we know, give it to DevIL
		env->ThrowError("ImageReader: Unsupported height %d", infoHeader.biHeight);
	}
	else {
	  use_DevIL = true; // Not a BMP, give it to DevIL
	}
  }

  if (use_DevIL == true) {  // attempt to open via DevIL

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
	else {
	  env->ThrowError("ImageReader: supports the following pixel types: RGB24 or RGB32");
	}

    // Get errors if any
    // (note: inability to parse an (e)bmp will show up here as a DevIL error)
    ILenum err = ilGetError();

    ilDeleteImages(1, &myImage);

    if (err != IL_NO_ERROR)
    {
      ostringstream ss;
      ss << "ImageReader: error '" << getErrStr(err) << "' in DevIL library\n reading file " << filename;
      env->ThrowError(ss.str().c_str());
    }
    // work around DevIL upside-down bug with compressed images
	should_flip = false;
	const char * ext = strrchr(_base_name, '.') + 1;
	if (  !lstrcmpi(ext, "jpeg") || !lstrcmpi(ext, "jpg") || !lstrcmpi(ext, "jpe") || !lstrcmpi(ext, "dds") || 
		  !lstrcmpi(ext, "pal") || !lstrcmpi(ext, "pal") || !lstrcmpi(ext, "pcx") || !lstrcmpi(ext, "png") || 
		  !lstrcmpi(ext, "pbm") || !lstrcmpi(ext, "pgm") || !lstrcmpi(ext, "ppm") || !lstrcmpi(ext, "tga")    )
	{
	  should_flip = true;
	}
  }
}


ImageReader::~ImageReader()
{
  if (use_DevIL) {
    ilShutDown();
  }
}

/*  Notes to clear thinking!

  vi.num_frames = end-start+1
  0 <= n < vi.num_frames

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

  _snprintf(filename, sizeof filename, base_name, n+start);
  
  if (use_DevIL)  /* read using DevIL */
  {    
    // Setup
    ILuint myImage=0;
    ilGenImages(1, &myImage);
    ilBindImage(myImage);

    if (ilLoadImage(filename) == IL_FALSE) {
	  // Get errors if any
	  err = ilGetError();

	  // Cleanup
	  ilDeleteImages(1, &myImage);

	  memset(WritePtr, 0, pitch * height);  // Black frame
	  if ((info) || (err != IL_COULD_NOT_OPEN_FILE)) {
		ostringstream ss;
		ss << "ImageReader: error '" << getErrStr(err) << "' in DevIL library\n opening file \"" << filename << "\"";
		ApplyMessage(&frame, vi, ss.str().c_str(), vi.width/4, TEXT_COLOR,0,0 , env);
	  }
	  return frame;
	}

    // Check some parameters
    if ( ilGetInteger(IL_IMAGE_HEIGHT) != height)
    {
	  // Cleanup
	  ilDeleteImages(1, &myImage);

      memset(WritePtr, 0, pitch * height);       
      ApplyMessage(&frame, vi, "ImageReader: images must have identical heights", vi.width/4, TEXT_COLOR,0,0 , env);
      return frame;
    }    
    if ( ilGetInteger(IL_IMAGE_WIDTH) != width)
    {
	  // Cleanup
	  ilDeleteImages(1, &myImage);

      memset(WritePtr, 0, pitch * height);       
      ApplyMessage(&frame, vi, "ImageReader: images must have identical widths", vi.width/4, TEXT_COLOR,0,0 , env);
      return frame;
    }

	const ILenum il_format = vi.IsRGB32() ? IL_BGRA : IL_BGR;
	const ILenum linesize = width * (vi.IsRGB32() ? 4 : 3);

	// Copy raster to AVS frame
////if (ilGetInteger(IL_ORIGIN_MODE) == IL_ORIGIN_UPPER_LEFT, IL_ORIGIN_LOWER_LEFT ???
	if (should_flip)
	{
	  // Copy upside down
	  for (int y=height-1; y>=0; --y)
	  {
		if (ilCopyPixels(0, y, 0, width, 1, 1, il_format, IL_UNSIGNED_BYTE, dstPtr) > linesize)
		  break; // Try not to spew all over memory
		dstPtr += pitch;
	  }
	}
	else {
	  // Copy right side up
	  for (int y=0; y<height; ++y)
	  {
		if (ilCopyPixels(0, y, 0, width, 1, 1, il_format, IL_UNSIGNED_BYTE, dstPtr) > linesize)
		  break; // Try not to spew all over memory
		dstPtr += pitch;
	  }
	}

    // Get errors if any    
    err = ilGetError();

    // Cleanup
    ilDeleteImages(1, &myImage);

    if (err != IL_NO_ERROR)
    {
      memset(WritePtr, 0, pitch * height); 
      ostringstream ss;
      ss << "ImageReader: error '" << getErrStr(err) << "' in DevIL library\n reading file \"" << filename << "\"";
      ApplyMessage(&frame, vi, ss.str().c_str(), vi.width/4, TEXT_COLOR,0,0 , env);
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
    if (0) // (vi.IsY8())
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
    text << "Frame " << n << ".\nRead from \"" << filename << "\"";
    ApplyMessage(&frame, vi, text.str().c_str(), vi.width/4, TEXT_COLOR,0,0 , env);
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


bool ImageReader::checkProperties(ifstream & file, PVideoFrame & frame, IScriptEnvironment * env)
{
  if (!file.is_open())
  {    
    if (info) ApplyMessage(&frame, vi, "ImageReader: cannot open file", vi.width/4, TEXT_COLOR,0,0 , env);
    return false;
  }

  BITMAPFILEHEADER tempFileHeader;
  BITMAPINFOHEADER tempInfoHeader;
  
  file.read( reinterpret_cast<char *> (&tempFileHeader), sizeof(tempFileHeader) );
  file.read( reinterpret_cast<char *> (&tempInfoHeader), sizeof(tempInfoHeader) );
 
  if (tempFileHeader.bfType != fileHeader.bfType)
  {    
    ApplyMessage(&frame, vi, "ImageReader: invalid (E)BMP file", vi.width/4, TEXT_COLOR,0,0 , env);
    return false;
  }

  if (tempInfoHeader.biWidth != infoHeader.biWidth)
  {    
    ApplyMessage(&frame, vi, "ImageReader: image widths must be identical", vi.width/4, TEXT_COLOR,0,0 , env);
    return false;
  }

  if (tempInfoHeader.biHeight != infoHeader.biHeight)
  {    
    ApplyMessage(&frame, vi, "ImageReader: image heights must be identical", vi.width/4, TEXT_COLOR,0,0 , env);
    return false;
  }

  if (tempInfoHeader.biPlanes != infoHeader.biPlanes)
  {    
    ApplyMessage(&frame, vi, "ImageReader: images must have the same number of planes", vi.width/4, TEXT_COLOR,0,0 , env);
    return false;
  }

  if (tempInfoHeader.biBitCount != infoHeader.biBitCount)
  {    
    ApplyMessage(&frame, vi, "ImageReader: images must have identical bits per pixel", vi.width/4, TEXT_COLOR,0,0 , env);
    return false;
  }

  if (tempFileHeader.bfSize != fileHeader.bfSize)
  {    
    ApplyMessage(&frame, vi, "ImageReader: raster sizes must be identical", vi.width/4, TEXT_COLOR,0,0 , env);
    return false;
  }

  if (tempInfoHeader.biCompression != 0)
  {    
    ApplyMessage(&frame, vi, "ImageReader: EBMP reader cannot handle compressed images", vi.width/4, TEXT_COLOR,0,0 , env);
    return false;
  }

  return true;
}

AVSValue __cdecl ImageReader::Create(AVSValue args, void*, IScriptEnvironment* env) 
{
  const char * path = args[0].AsString("c:\\%06d.ebmp");

  ImageReader *IR = new ImageReader(path, args[1].AsInt(0), args[2].AsInt(1000), args[3].AsFloat(24.0f), 
                                    args[4].AsBool(false), args[5].AsBool(false), args[6].AsString("rgb24"), env);
  // If we are returning a stream of 2 or more copies of the same image
  // then use FreezeFrame and the Cache to minimise any reloading.
  if (IR->framecopies > 1) {
	AVSValue cache_args[1] = { IR };
    AVSValue cache = env->Invoke("Cache", AVSValue(cache_args, 1));
	AVSValue ff_args[4] = { cache, 0, IR->framecopies-1, 0 };
    return env->Invoke("FreezeFrame", AVSValue(ff_args, 4)).AsClip();
  }

  return IR;

}


string getErrStr(ILenum err)
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
    return "Illegal file";
  if (err == IL_INVALID_FILE_HEADER)
    return "Illegal file header";
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
    return "LifJpeg error";
  if (err == IL_LIB_PNG_ERROR)
    return "LibPng error";
  if (err == IL_LIB_TIFF_ERROR)
    return "LibTiff error";
  if (err == IL_LIB_MNG_ERROR)
    return "LibMng error";
  if (err == IL_UNKNOWN_ERROR)
    return "Unknown error";

  return "Unknown error";
}

