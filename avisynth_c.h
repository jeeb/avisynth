// Avisynth C Interface Version 0.10
// Copyright 2003 Kevin Atkinson

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

#ifndef __AVISYNTH_C__
#define __AVISYNTH_C__

#ifdef AVISYNTH_C_EXPORTS
#define AVISYNTH_C_API __declspec(dllexport)
#else
#define AVISYNTH_C_API __declspec(dllimport)
#endif


typedef unsigned char BYTE;
#ifdef __GNUC__
	typedef long long int INT64;
#else
	typedef __int64 INT64;
#endif

#ifdef __cplusplus
extern "C" 
{
#endif

/////////////////////////////////////////////////////////////////////
//
// Constants
//

#ifndef __AVISYNTH_H__
enum { AVISYNTH_INTERFACE_VERSION = 2 };
#endif

enum {AVS_SAMPLE_INT8  = 1<<0,
      AVS_SAMPLE_INT16 = 1<<1, 
      AVS_SAMPLE_INT24 = 1<<2,    // Int24 is a very stupid thing to code, but it's supported by some hardware.
      AVS_SAMPLE_INT32 = 1<<3,
      AVS_SAMPLE_FLOAT = 1<<4};

enum {
   AVS_PLANAR_Y=1<<0,
   AVS_PLANAR_U=1<<1,
   AVS_PLANAR_V=1<<2,
   AVS_PLANAR_ALIGNED=1<<3,
   AVS_PLANAR_Y_ALIGNED=AVS_PLANAR_Y|AVS_PLANAR_ALIGNED,
   AVS_PLANAR_U_ALIGNED=AVS_PLANAR_U|AVS_PLANAR_ALIGNED,
   AVS_PLANAR_V_ALIGNED=AVS_PLANAR_V|AVS_PLANAR_ALIGNED};

  // Colorspace properties.
enum {
    AVS_CS_BGR = 1<<28,  
    AVS_CS_YUV = 1<<29,
    AVS_CS_INTERLEAVED = 1<<30,
    AVS_CS_PLANAR = 1<<31};

  // Specific colorformats
enum { AVS_CS_UNKNOWN = 0,
         AVS_CS_BGR24 = 1<<0 | AVS_CS_BGR | AVS_CS_INTERLEAVED,
         AVS_CS_BGR32 = 1<<1 | AVS_CS_BGR | AVS_CS_INTERLEAVED,
         AVS_CS_YUY2 = 1<<2 | AVS_CS_YUV | AVS_CS_INTERLEAVED,
         AVS_CS_YV12 = 1<<3 | AVS_CS_YUV | AVS_CS_PLANAR,  // y-v-u, planar
         AVS_CS_I420 = 1<<4 | AVS_CS_YUV | AVS_CS_PLANAR,  // y-u-v, planar
         AVS_CS_IYUV = 1<<4 | AVS_CS_YUV | AVS_CS_PLANAR  // same as above
         };

enum {
    AVS_IT_BFF = 1<<0,
    AVS_IT_TFF = 1<<1,
    AVS_IT_FIELDBASED = 1<<2};

enum {
  AVS_FILTER_TYPE=1,
  AVS_FILTER_INPUT_COLORSPACE=2,
  AVS_FILTER_OUTPUT_TYPE=9,
  AVS_FILTER_NAME=4,
  AVS_FILTER_AUTHOR=5,
  AVS_FILTER_VERSION=6,
  AVS_FILTER_ARGS=7,
  AVS_FILTER_ARGS_INFO=8,
  AVS_FILTER_ARGS_DESCRIPTION=10,
  AVS_FILTER_DESCRIPTION=11};

enum {  //SUBTYPES
  AVS_FILTER_TYPE_AUDIO=1,
  AVS_FILTER_TYPE_VIDEO=2,
  AVS_FILTER_OUTPUT_TYPE_SAME=3,
  AVS_FILTER_OUTPUT_TYPE_DIFFERENT=4};

enum {
  AVS_CACHE_NOTHING=0,
  AVS_CACHE_RANGE=1 };

#define AVS_FRAME_ALIGN 16 

typedef struct AVS_Clip AVS_Clip;
typedef struct AVS_ScriptEnvironment AVS_ScriptEnvironment;

/////////////////////////////////////////////////////////////////////
//
// AVS_VideoInfo
//

// AVS_VideoInfo is layed out identicly to VideoInfo
typedef struct AVS_VideoInfo {
  int width, height;    // width=0 means no video
  unsigned fps_numerator, fps_denominator;
  int num_frames;

  int pixel_type;
  
  int audio_samples_per_second;   // 0 means no audio
  int sample_type;
  INT64 num_audio_samples;
  int nchannels;

  // Imagetype properties

  int image_type;
} AVS_VideoInfo;

// useful functions of the above
static inline int avs_has_video(const AVS_VideoInfo * p) 
	{ return (p->width!=0); }

static inline int avs_has_audio(const AVS_VideoInfo * p) 
	{ return (p->audio_samples_per_second!=0); }

static inline int avs_is_rgb(const AVS_VideoInfo * p) 
	{ return (p->pixel_type&AVS_CS_BGR); }

static inline int avs_is_rgb24(const AVS_VideoInfo * p) 
	{ return (p->pixel_type&AVS_CS_BGR24)==AVS_CS_BGR24; } // Clear out additional properties

static inline int avs_is_rgb32(const AVS_VideoInfo * p) 
	{ return (p->pixel_type & AVS_CS_BGR32) == AVS_CS_BGR32 ; }

static inline int avs_is_yuy(const AVS_VideoInfo * p) 
	{ return (p->pixel_type&AVS_CS_YUV ); }

static inline int avs_is_yuy2(const AVS_VideoInfo * p) 
	{ return (p->pixel_type & AVS_CS_YUY2) == AVS_CS_YUY2; }  

static inline int avs_is_yv12(const AVS_VideoInfo * p) 
	{ return ((p->pixel_type & AVS_CS_YV12) == AVS_CS_YV12)||((p->pixel_type & AVS_CS_I420) == AVS_CS_I420); }

static inline int avs_is_color_space(const AVS_VideoInfo * p, int c_space) 
	{ return ((p->pixel_type & c_space) == c_space); }

static inline int avs_is_property(const AVS_VideoInfo * p, int property) 
	{ return ((p->pixel_type & property)==property ); }

static inline int avs_is_planar(const AVS_VideoInfo * p) 
	{ return (p->pixel_type & AVS_CS_PLANAR); }

static inline int avs_is_field_based(const AVS_VideoInfo * p) 
	{ return (p->image_type & AVS_IT_FIELDBASED); }

static inline int avs_is_parity_known(const AVS_VideoInfo * p) 
	{ return ((p->image_type & AVS_IT_FIELDBASED)&&(p->image_type & (AVS_IT_BFF||AVS_IT_TFF))); }

static inline int avs_is_bff(const AVS_VideoInfo * p) 
	{ return (p->pixel_type & AVS_IT_BFF); }

static inline int avs_is_tff(const AVS_VideoInfo * p) 
	{ return (p->pixel_type & AVS_IT_TFF); }

static inline int avs_is_v_plane_first(const AVS_VideoInfo * p) 
	{return ((p->pixel_type & AVS_CS_YV12) == AVS_CS_YV12); }  // Don't use this 

static inline int avs_bits_per_pixel(const AVS_VideoInfo * p) 
{ 
    switch (p->pixel_type) {
      case AVS_CS_BGR24: return 24;
      case AVS_CS_BGR32: return 32;
      case AVS_CS_YUY2:  return 16;
      case AVS_CS_YV12:
      case AVS_CS_I420:  return 12;
      default:           return 0;
    }
}
static inline int avs_bytes_from_pixels(const AVS_VideoInfo * p, int pixels) 
	{ return pixels * (avs_bits_per_pixel(p)>>3); }   // Will not work on planar images, but will return only luma planes

static inline int avs_row_size(const AVS_VideoInfo * p) 
	{ return avs_bytes_from_pixels(p,p->width); }  // Also only returns first plane on planar images

static inline int avs_bmp_size(const AVS_VideoInfo * vi)		
	{ if (avs_is_planar(vi)) {int p = vi->height * ((avs_row_size(vi)+3) & ~3); p+=p>>1; return p;  } return vi->height * ((avs_row_size(vi)+3) & ~3); }

static inline int avs_samples_per_second(const AVS_VideoInfo * p) 
	{ return p->audio_samples_per_second; }


static inline int avs_bytes_per_channel_sample(const AVS_VideoInfo * p) 
{
    switch (p->sample_type) {
      case AVS_SAMPLE_INT8:  return sizeof(signed char);
      case AVS_SAMPLE_INT16: return sizeof(signed short);
      case AVS_SAMPLE_INT24: return 3;
      case AVS_SAMPLE_INT32: return sizeof(signed int);
      case AVS_SAMPLE_FLOAT: return sizeof(float);
      default: return 0;
    }
}
static inline int avs_bytes_per_audio_sample(const AVS_VideoInfo * p)	
	{ return p->nchannels*avs_bytes_per_channel_sample(p);}

static inline INT64 avs_audio_samples_from_frames(const AVS_VideoInfo * p, INT64 frames) 
	{ return ((INT64)(frames) * p->audio_samples_per_second * p->fps_denominator / p->fps_numerator); }

static inline int avs_frames_from_audio_samples(const AVS_VideoInfo * p, INT64 samples) 
	{ return (int)(samples * (INT64)p->fps_numerator / (INT64)p->fps_denominator / (INT64)p->audio_samples_per_second); }

static inline INT64 avs_audio_samples_from_bytes(const AVS_VideoInfo * p, INT64 bytes) 
	{ return bytes / avs_bytes_per_audio_sample(p); }

static inline INT64 avs_bytes_from_audio_samples(const AVS_VideoInfo * p, INT64 samples) 
	{ return samples * avs_bytes_per_audio_sample(p); }

static inline int avs_audio_channels(const AVS_VideoInfo * p) 
	{ return p->nchannels; }

static inline int avs_sample_type(const AVS_VideoInfo * p)
	{ return p->sample_type;}

// useful mutator
static inline void avs_set_property(AVS_VideoInfo * p, int property)  
	{ p->image_type|=property; }

static inline void avd_clear_property(AVS_VideoInfo * p, int property)  
	{ p->image_type&=~property; }

static inline void avs_set_field_based(AVS_VideoInfo * p, int isfieldbased)  
	{ if (isfieldbased) p->image_type|=AVS_IT_FIELDBASED; else p->image_type&=~AVS_IT_FIELDBASED; }

static inline void set_fps(AVS_VideoInfo * p, unsigned numerator, unsigned denominator) 
{
    unsigned x=numerator, y=denominator;
    while (y) {   // find gcd
      unsigned t = x%y; x = y; y = t;
    }
    p->fps_numerator = numerator/x;
    p->fps_denominator = denominator/x;
}

/////////////////////////////////////////////////////////////////////
//
// AVS_VideoFrame
//

// VideoFrameBuffer holds information about a memory block which is used
// for video data.  For efficiency, instances of this class are not deleted
// when the refcount reaches zero; instead they're stored in a linked list
// to be reused.  The instances are deleted when the corresponding AVS
// file is closed.

// AVS_VideoFrameBuffer is layed out identicly to VideoFrameBuffer
// DO NOT USE THIS STRUCTURE DIRECTLY
typedef struct AVS_VideoFrameBuffer {
  BYTE * data;
  int data_size;
  // sequence_number is incremented every time the buffer is changed, so
  // that stale views can tell they're no longer valid.
  long sequence_number;

  long refcount;
} AVS_VideoFrameBuffer;

// VideoFrame holds a "window" into a VideoFrameBuffer.

// AVS_VideoFrame is layed out identicly to IVideoFrame
typedef struct AVS_VideoFrame {
  // Do Not Use These Values
  int refcount;
  AVS_VideoFrameBuffer * vfb;
  // Its OK to LOOK at these values
  int offset, pitch, row_size, height, offsetU, offsetV, pitchUV;  // U&V offsets are from top of picture.
} AVS_VideoFrame;

// Access functions for AVS_VideoFrame
static inline int avs_get_pitch(const AVS_VideoFrame * p) {
	return p->pitch;}

static inline int avs_get_pitch_p(const AVS_VideoFrame * p, int plane) { 
	switch (plane) {
		case AVS_PLANAR_U: case AVS_PLANAR_V: return p->pitchUV;}
	return p->pitch;}

static inline int avs_get_row_size(const AVS_VideoFrame * p) {
	return p->row_size; }

static inline int avs_get_row_size_p(const AVS_VideoFrame * p, int plane) { 
	int r;
    switch (plane) {
    case AVS_PLANAR_U: case AVS_PLANAR_V: 
		if (p->pitchUV) return p->row_size>>1; 
		else            return 0;
    case AVS_PLANAR_U_ALIGNED: case AVS_PLANAR_V_ALIGNED: 
		if (p->pitchUV) { 
			int r = ((p->row_size+AVS_FRAME_ALIGN-1)&(~(AVS_FRAME_ALIGN-1)) )>>1; // Aligned rowsize
			if (r < p->pitchUV) 
				return r; 
			return p->row_size>>1; 
		} else return 0;
    case AVS_PLANAR_Y_ALIGNED:
		r = (p->row_size+AVS_FRAME_ALIGN-1)&(~(AVS_FRAME_ALIGN-1)); // Aligned rowsize
		if (r <= p->pitch) 
			return r; 
		return p->row_size;
    }
    return p->row_size;
}

static inline int avs_get_height(const AVS_VideoFrame * p) {
	return p->height;}

static inline int avs_get_height_p(const AVS_VideoFrame * p, int plane) {
	switch (plane) {
		case AVS_PLANAR_U: case AVS_PLANAR_V: 
			if (p->pitchUV) return p->height>>1;
			return 0;
	}
	return p->height;}

static inline const BYTE* avs_get_read_ptr(const AVS_VideoFrame * p) {
	return p->vfb->data + p->offset;}

static inline void avs_get_read_ptrs(const AVS_VideoFrame * p,
									 const BYTE * * y, const BYTE * * u, 
									 const BYTE * * v) 
{
	++p->vfb->sequence_number;
	*y = p->vfb->data + p->offset;
	*u = p->vfb->data + p->offsetU;
	*v = p->vfb->data + p->offsetV;
}

static inline const BYTE* avs_get_read_ptr_p(const AVS_VideoFrame * p, int plane) 
{
	switch (plane) {
		case AVS_PLANAR_U: return p->vfb->data + p->offsetU;
		case AVS_PLANAR_V: return p->vfb->data + p->offsetV;
		default:           return p->vfb->data + p->offset;}
}

static inline int avs_is_writable(const AVS_VideoFrame * p) {
	return (p->refcount == 1 && p->vfb->refcount == 1);}

static inline BYTE* avs_get_write_ptr(const AVS_VideoFrame * p) 
{
	if (avs_is_writable(p)) {
		++p->vfb->sequence_number;
		return p->vfb->data + p->offset;
	} else
		return 0;
}

static inline int avs_get_write_ptrs(const AVS_VideoFrame * p,
									  BYTE * * y, BYTE * * u, BYTE * * v) 
{
	if (avs_is_writable(p)) {
		++p->vfb->sequence_number;
		*y = p->vfb->data + p->offset;
		*u = p->vfb->data + p->offsetU;
		*v = p->vfb->data + p->offsetV;
		return 1;
	} else
		return 0;
}

static inline BYTE* avs_get_write_ptr_p(const AVS_VideoFrame * p, int plane) 
{
	if (plane==AVS_PLANAR_Y && avs_is_writable(p)) {
		++p->vfb->sequence_number;
		return p->vfb->data + p->offset;
	} else if (plane==AVS_PLANAR_Y) {
		return 0;
	} else {
		switch (plane) {
			case AVS_PLANAR_U: return p->vfb->data + p->offsetU;
			case AVS_PLANAR_V: return p->vfb->data + p->offsetV;
			default:       return p->vfb->data + p->offset;
		}
	}
}

AVISYNTH_C_API void avs_release_video_frame(AVS_VideoFrame *);
AVISYNTH_C_API void avs_copy_video_frame(AVS_VideoFrame * dest, 
										 AVS_VideoFrame * src);

/////////////////////////////////////////////////////////////////////
//
// AVS_Value
//

// AVS_Value is layed out identicly to AVSValue
typedef struct AVS_Value AVS_Value;
struct AVS_Value {
  short type;  // 'a'rray, 'c'lip, 'b'ool, 'i'nt, 'f'loat, 's'tring, 'v'oid, or 'l'ong
               // for some function e'rror
  short array_size;
  union {
    void * clip; // do not use directly, use avs_take_clip
    int integer;
    float floating_pt;
    const char * string;
    const AVS_Value * array;
  } d;
};

AVISYNTH_C_API AVS_Clip * avs_take_clip(AVS_Value, AVS_ScriptEnvironment *);
AVISYNTH_C_API void avs_set_to_clip(AVS_Value *, AVS_Clip *);
AVISYNTH_C_API void avs_copy_value(AVS_Value * dest, AVS_Value src);
AVISYNTH_C_API void avs_release_value(AVS_Value);

/////////////////////////////////////////////////////////////////////
//
// AVS_Clip
//

AVISYNTH_C_API void avs_release_clip(AVS_Clip *);
AVISYNTH_C_API AVS_Clip * avs_copy_clip(AVS_Clip *);

AVISYNTH_C_API const char * avs_clip_get_error(AVS_Clip *); // return 0 if no error
 
AVISYNTH_C_API AVS_VideoFrame * avs_get_frame(AVS_Clip *, int n);
// The returned video frame must be released with avs_release_video_frame

AVISYNTH_C_API int avs_get_parity(AVS_Clip *, int n); 
// return field parity if field_based, else parity of first field in frame

AVISYNTH_C_API int avs_get_audio(AVS_Clip *, void * buf, 
								 INT64 start, INT64 count); 
// start and count are in samples

AVISYNTH_C_API int avs_set_cache_hints(AVS_Clip *, 
									   int cachehints, int frame_range);

// This is the callback type used by avs_add_function
typedef AVS_Value (__cdecl * AVS_ApplyFunc)
			(AVS_ScriptEnvironment *, AVS_Value args, void * user_data);

typedef struct AVS_FilterInfo AVS_FilterInfo;
struct AVS_FilterInfo
{
	// these members should not be modified outside of the AVS_ApplyFunc callback
	AVS_Clip * child;
	AVS_VideoInfo vi;
	AVS_ScriptEnvironment * env;
	AVS_VideoFrame * (* get_frame)(AVS_FilterInfo *, int n);
	int (* get_parity)(AVS_FilterInfo *, int n);
	int (* get_audio)(AVS_FilterInfo *, void * buf, INT64 start, INT64 count);
	int (* set_cache_hints)(AVS_FilterInfo *, int cachehints, int frame_range);
	void (* free_filter)(AVS_FilterInfo *);
	
	// Should be set when ever there is an error to report.
	// It is cleared before any of the above methods are called
	const char * error;
	// this is to store whatever and may be modified at will
	void * user_data;
};

// Create a new filter
// fi is set to point to the AVS_FilterInfo so that you can
//   modify it once it is initilized.
// store_child should generally be set to true.  If it is not
//    set than ALL methods (the function pointers) must be defined
// If it is set than you do not need to worry about freeing the child
//    clip.
AVISYNTH_C_API
AVS_Clip * avs_new_c_filter(AVS_ScriptEnvironment * e,
							AVS_FilterInfo * * fi,
							AVS_Value child, int store_child);


/////////////////////////////////////////////////////////////////////
//
// AVS_ScriptEnvironment
//

AVISYNTH_C_API
long avs_get_cpu_flags(AVS_ScriptEnvironment *);

AVISYNTH_C_API
char * avs_save_string(AVS_ScriptEnvironment *, const char* s, int length);
AVISYNTH_C_API
char * avs_sprintf(AVS_ScriptEnvironment *, const char * fmt, ...);

AVISYNTH_C_API
char * avs_vsprintf(AVS_ScriptEnvironment *, const char * fmt, void* val);
 // note: val is really a va_list; I hope everyone typedefs va_list to a pointer

AVISYNTH_C_API
int avs_add_function(AVS_ScriptEnvironment *, 
					 const char * name, const char * params, 
					 AVS_ApplyFunc apply, void * user_data);

AVISYNTH_C_API
int avs_function_exists(AVS_ScriptEnvironment *, const char * name);

AVISYNTH_C_API
AVS_Value avs_invoke(AVS_ScriptEnvironment *, const char * name, AVS_Value args, const char** arg_names);
// The returned value must be be released with avs_release_value

AVISYNTH_C_API
AVS_Value avs_get_var(AVS_ScriptEnvironment *, const char* name);
// The returned value must be be released with avs_release_value

AVISYNTH_C_API
int avs_set_var(AVS_ScriptEnvironment *, const char* name, AVS_Value val);

AVISYNTH_C_API
int avs_set_global_var(AVS_ScriptEnvironment *, const char* name, const AVS_Value val);

//void avs_push_context(AVS_ScriptEnvironment *, int level=0);
//void avs_pop_context(AVS_ScriptEnvironment *);

// align should be 4 or 8
AVISYNTH_C_API
AVS_VideoFrame * avs_new_video_frame(AVS_ScriptEnvironment *, const AVS_VideoInfo * vi, int align);
// The returned video frame must be be released

AVISYNTH_C_API
int avs_make_writable(AVS_ScriptEnvironment *, AVS_VideoFrame * * pvf);

AVISYNTH_C_API
void avs_bit_blt(AVS_ScriptEnvironment *, BYTE* dstp, int dst_pitch, const BYTE* srcp, int src_pitch, int row_size, int height);

//typedef void (*AVS_ShutdownFunc)(void* user_data, AVS_ScriptEnvironment * env);
//void avs_at_exit(AVS_ScriptEnvironment *, AVS_ShutdownFunc function, void * user_data);

AVISYNTH_C_API
int avs_check_version(AVS_ScriptEnvironment *, int version);

AVISYNTH_C_API
AVS_VideoFrame * avs_subframe(AVS_ScriptEnvironment *, AVS_VideoFrame * src, int rel_offset, int new_pitch, int new_row_size, int new_height);
// The returned video frame must be be released

AVISYNTH_C_API
int avs_set_memory_max(AVS_ScriptEnvironment *, int mem);

AVISYNTH_C_API
int avs_set_working_dir(AVS_ScriptEnvironment *, const char * newdir);

// this symbol is the entry point for the plugin and must
// be defined
__declspec(dllexport)
const char * avisynth_c_plugin_init(AVS_ScriptEnvironment* env);


// avisynth.dll exports this; it's a way to use it as a library, without
// writing an AVS script or without going through AVIFile.
AVISYNTH_C_API
AVS_ScriptEnvironment * avs_create_script_environment(int version);

#ifdef __cplusplus
}
#endif

#endif
