// Avisynth C Interface Version 0.10
// Copyright 2003 Kevin Atkinson
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//


#include "stdafx.h"

#include "internal.h"
#include "avisynth_c.h"


struct AVS_Clip 
{
	PClip clip;
	IScriptEnvironment * env;
	const char * error;
	AVS_Clip() : env(0), error(0) {}
};

struct AVS_ScriptEnvironment
{
	IScriptEnvironment * env;
	const char * error;
	AVS_ScriptEnvironment(IScriptEnvironment * e = 0) 
		: env(e), error(0) {}
};

class C_VideoFilter : public IClip {
public: // but don't use
	AVS_Clip child;
	AVS_ScriptEnvironment env;
	AVS_FilterInfo d;
public:
	C_VideoFilter() {memset(&d,0,sizeof(d));}
	PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
	void __stdcall GetAudio(void * buf, __int64 start, __int64 count, IScriptEnvironment* env);
	const VideoInfo & __stdcall GetVideoInfo();
	bool __stdcall GetParity(int n);
	void __stdcall SetCacheHints(int cachehints,int frame_range);
	__stdcall ~C_VideoFilter();
};

/////////////////////////////////////////////////////////////////////
//
//
//

extern "C"
void avs_release_video_frame(AVS_VideoFrame * f)
{
	((PVideoFrame *)&f)->~PVideoFrame();
}

/////////////////////////////////////////////////////////////////////
//
// C_VideoFilter
//

PVideoFrame C_VideoFilter::GetFrame(int n, IScriptEnvironment* env) 
{
	if (d.get_frame) {
		d.error = 0;
		AVS_VideoFrame * f = d.get_frame(&d, n);
		if (d.error)
			throw AvisynthError(d.error);
		return PVideoFrame((VideoFrame *)f);
	} else {
		return d.child->clip->GetFrame(n, env); 
	}
}

void __stdcall C_VideoFilter::GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env) 
{
	if (d.get_audio) {
		d.error = 0;
		d.get_audio(&d, buf, start, count);
		if (d.error)
			throw AvisynthError(d.error);
	} else {
		d.child->clip->GetAudio(buf, start, count, env);
	}
}

const VideoInfo& __stdcall C_VideoFilter::GetVideoInfo() 
{
	return *(VideoInfo *)&d.vi; 
}

bool __stdcall C_VideoFilter::GetParity(int n) 
{
	if (d.get_parity) {
		d.error = 0;
		int res = d.get_parity(&d, n);
		if (d.error)
			throw AvisynthError(d.error);
		return !!res;
	} else {
		return d.child->clip->GetParity(n);
	}
}

void __stdcall C_VideoFilter::SetCacheHints(int cachehints, int frame_range) 
{
	if (d.set_cache_hints) {
		d.error = 0;
		d.set_cache_hints(&d, cachehints, frame_range);
		if (d.error)
			throw AvisynthError(d.error);
	}
	// We do not pass cache requests upwards, only to the next filter.
}

C_VideoFilter::~C_VideoFilter()
{
	if (d.free_filter)
		d.free_filter(&d);
}

/////////////////////////////////////////////////////////////////////
//
// AVS_Clip
//

extern "C"
void avs_release_clip(AVS_Clip * p)
{
	delete p;
}

AVS_Clip * avs_copy_clip(AVS_Clip * p)
{
	return new AVS_Clip(*p);
}

extern "C"
const char * avs_clip_get_error(AVS_Clip * p) // return 0 if no error
{
	return p->error;
}

extern "C"
AVS_VideoFrame * avs_get_frame(AVS_Clip * p, int n)
{
	p->error = 0;
	try {
		PVideoFrame f0 = p->clip->GetFrame(n,p->env);
		AVS_VideoFrame * f;
		//f = *(AVS_VideoFrame * *)&f0;
		new((PVideoFrame *)&f) PVideoFrame(f0);
		return f;
	} catch (AvisynthError err) {
		p->error = err.msg;
		return 0;
	} 
}

extern "C"
int avs_get_parity(AVS_Clip * p, int n) // return field parity if field_based, else parity of first field in frame
{
	try {
		p->error = 0;
		return p->clip->GetParity(n);
	} catch (AvisynthError err) {
		p->error = err.msg;
		return -1;
	} 
}

extern "C"
int avs_get_audio(AVS_Clip * p, void * buf, INT64 start, INT64 count) // start and count are in samples
{
	try {
		p->error = 0;
		p->clip->GetAudio(buf, start, count, p->env);
		return 0;
	} catch (AvisynthError err) {
		p->error = err.msg;
		return -1;
	} 
}

extern "C"
int avs_set_cache_hints(AVS_Clip * p, int cachehints, int frame_range)  // We do not pass cache requests upwards, only to the next filter.
{
	try {
		p->error = 0;
		p->clip->SetCacheHints(cachehints, frame_range);
		return 0;
	} catch (AvisynthError err) {
		p->error = err.msg;
		return -1;
	}
}

//////////////////////////////////////////////////////////////////
//
//
//
extern "C"
AVS_Clip * avs_take_clip(AVS_Value v, AVS_ScriptEnvironment * env)
{
	AVS_Clip * c = new AVS_Clip;
	c->env  = env->env;
	c->clip = (IClip *)v.d.clip;
	return c;
}

extern "C"
void avs_set_to_clip(AVS_Value * v, AVS_Clip * c)
{
	new(v) AVSValue(c->clip);
}

extern "C"
void avs_copy_value(AVS_Value * dest, AVS_Value src)
{
	new(dest) AVSValue(*(const AVSValue *)&src);
}

extern "C"
void avs_release_value(AVS_Value v)
{
	((AVSValue *)&v)->~AVSValue();
}

//////////////////////////////////////////////////////////////////
//
//
//

extern "C"
AVS_Clip * avs_new_c_filter(AVS_ScriptEnvironment * e,
							AVS_FilterInfo * * fi,
							AVS_Value child, int store_child)
{
	C_VideoFilter * f = new C_VideoFilter();
	AVS_Clip * ff = new AVS_Clip();
	ff->clip = f;
	ff->env  = e->env;
	f->env.env = e->env;
	f->d.env = &f->env;
	if (store_child) {
//		assert(child.type == 'c');
		f->child.clip = (IClip *)child.d.clip;
		f->child.env  = e->env;
		f->d.child = &f->child;
	}
	*fi = &f->d;
	if (child.type == 'c')
		f->d.vi = *(const AVS_VideoInfo *)(&((IClip *)child.d.clip)->GetVideoInfo());
	return ff;
}

/////////////////////////////////////////////////////////////////////
//
// AVS_ScriptEnvironment::add_function
//

struct C_VideoFilter_UserData {
	void * user_data;
	AVS_ApplyFunc func;
};

AVSValue __cdecl create_c_video_filter(AVSValue args, void * user_data, 
									   IScriptEnvironment * e0)
{
	C_VideoFilter_UserData * d = (C_VideoFilter_UserData *)user_data;
	AVS_ScriptEnvironment env(e0);
	OutputDebugString("OK");
	AVS_Value res = (d->func)(&env, *(AVS_Value *)&args, d->user_data);
	AVSValue val(*(const AVSValue *)&res);
	((AVSValue *)&res)->~AVSValue();
	if (res.type == 'e') {
		throw AvisynthError(res.d.string);
	} else {
		return val;
	}
}

extern "C"
int avs_add_function(AVS_ScriptEnvironment * env, const char * name, const char * params, 
					  AVS_ApplyFunc applyf, void * user_data)
{
	C_VideoFilter_UserData * d = new C_VideoFilter_UserData;
	// When do I free d ????
	env->error = 0;
	d->func = applyf;
	d->user_data = user_data;
	try {
		env->env->AddFunction(name, params, create_c_video_filter, d);
	} catch (AvisynthError & err) {
		env->error = err.msg;
		return -1;
	} 
	return 0;
}

/////////////////////////////////////////////////////////////////////
//
// AVS_ScriptEnvironment
//

extern "C"
long avs_get_cpu_flags(AVS_ScriptEnvironment * p)
{
	p->error = 0;
	return p->env->GetCPUFlags();
}

extern "C"
char * avs_save_string(AVS_ScriptEnvironment * p, const char* s, int length)
{
	p->error = 0;
	return p->env->SaveString(s, length);
}

extern "C"
char * avs_sprintf(AVS_ScriptEnvironment * p, const char* fmt, ...)
{
	p->error = 0;
	abort(); // FIXME
}

 // note: val is really a va_list; I hope everyone typedefs va_list to a pointer
extern "C"
char * avs_vsprintf(AVS_ScriptEnvironment * p, const char* fmt, void* val)
{
	p->error = 0;
	return p->env->VSprintf(fmt, val);
}

extern "C"
int avs_function_exists(AVS_ScriptEnvironment * p, const char * name)
{
	p->error = 0;
	return p->env->FunctionExists(name);
}

extern "C"
AVS_Value avs_invoke(AVS_ScriptEnvironment * p, const char * name, AVS_Value args, const char * * arg_names)
{
	AVS_Value v = {0,0};
	p->error = 0;
	try {
		AVSValue v0 = p->env->Invoke(name, *(AVSValue *)&args, arg_names);
		v = *(AVS_Value *)&v0;
	} catch (IScriptEnvironment::NotFound) {
	} catch (AvisynthError err) {
		p->error = err.msg;
	}
	return v;
}

extern "C"
AVS_Value avs_get_var(AVS_ScriptEnvironment * p, const char* name)
{
	AVS_Value v = {0,0};
	p->error = 0;
	try {
		AVSValue v0 = p->env->GetVar(name);
		new ((AVSValue *)&v) AVSValue(v0);
	} catch (IScriptEnvironment::NotFound) {}
	return v;
}

extern "C"
int avs_set_var(AVS_ScriptEnvironment * p, const char* name, AVS_Value val)
{
	p->error = 0;
	try {
		return p->env->SetVar(name, *(const AVSValue *)(&val));
	} catch (AvisynthError err) {
		p->error = err.msg;
		return -1;
	}
}

extern "C"
int avs_set_global_var(AVS_ScriptEnvironment * p, const char* name, AVS_Value val)
{
	p->error = 0;
	try {
		return p->env->SetGlobalVar(name, *(const AVSValue *)(&val));
	} catch (AvisynthError err) {
		p->error = err.msg;
		return -1;
	}
}

// align should be 4 or 8
extern "C"
AVS_VideoFrame * avs_new_video_frame(AVS_ScriptEnvironment * p, const AVS_VideoInfo *  vi, int align)
{
	p->error = 0;
	PVideoFrame f0 = p->env->NewVideoFrame(*(const VideoInfo *)vi, align);
	AVS_VideoFrame * f;
	new((PVideoFrame *)&f) PVideoFrame(f0);
	return f;
}

extern "C"
int avs_make_writable(AVS_ScriptEnvironment * p, AVS_VideoFrame * * pvf)
{
	p->error = 0;
	int res = p->env->MakeWritable((PVideoFrame *)(pvf));
	return res;
}

extern "C"
void avs_bit_blt(AVS_ScriptEnvironment * p, BYTE * dstp, int dst_pitch, const BYTE * srcp, int src_pitch, int row_size, int height)
{
	p->error = 0;
	p->env->BitBlt(dstp, dst_pitch, srcp, src_pitch, row_size, height);
}

//typedef void (*AVS_ShutdownFunc)(void* user_data, AVS_ScriptEnvironment * env)
//void avs_at_exit(AVS_ScriptEnvironment *, AVS_ShutdownFunc function, void * user_data)

extern "C"
int avs_check_version(AVS_ScriptEnvironment * p, int version)
{
	p->error = 0;
	try {
		p->env->CheckVersion(version);
		return 0;
	} catch (AvisynthError err) {
		p->error = err.msg;
		return -1;
	}
}

extern "C"
AVS_VideoFrame * avs_subframe(AVS_ScriptEnvironment * p, AVS_VideoFrame * src0, 
							  int rel_offset, int new_pitch, int new_row_size, int new_height)
{
	p->error = 0;
	try {
		PVideoFrame f0 = p->env->Subframe((VideoFrame *)src0, rel_offset, new_pitch, new_row_size, new_height);
		AVS_VideoFrame * f;
		new((PVideoFrame *)&f) PVideoFrame(f0);
		return f;
	} catch (AvisynthError err) {
		p->error = err.msg;
		return 0;
	}
}

extern "C"
int avs_set_memory_max(AVS_ScriptEnvironment * p, int mem)
{
	p->error = 0;
	try {
		return p->env->SetMemoryMax(mem);
	} catch (AvisynthError err) {
		p->error = err.msg;
		return -1;
	}
}

extern "C"
int avs_set_working_dir(AVS_ScriptEnvironment * p, const char * newdir)
{
	p->error = 0;
	try {
		return p->env->SetWorkingDir(newdir);
	} catch (AvisynthError err) {
		p->error = err.msg;
		return -1;
	}
}
/////////////////////////////////////////////////////////////////////
//
// 
//

AVS_ScriptEnvironment * avs_create_script_environment(int version)
{
	abort();
	//AVS_ScriptEnvironment * e = new AVS_ScriptEnvironment;
	//e->env = CreateScriptEnvironment(version);
	//return e;
}


/////////////////////////////////////////////////////////////////////
//
// 
//

typedef const char * (__cdecl *AvisynthCPluginInitFunc)(AVS_ScriptEnvironment* env);

AVSValue __cdecl load_c_plugin(AVSValue args, void * user_data, 
					           IScriptEnvironment * env)
{
	// load dll
	// call entry point
	const char * filename = args[0].AsString();
	HMODULE plugin = LoadLibrary(filename);
	if (!plugin)
		env->ThrowError("Unable to load C Plugin: %s", filename);
    AvisynthCPluginInitFunc func = (AvisynthCPluginInitFunc)GetProcAddress(plugin, "avisynth_c_plugin_init");
	if (!func)
		env->ThrowError("Not An Avisynth 2 C Plugin: %s", filename);
	AVS_ScriptEnvironment e;
	e.env = env;
	const char * s = func(&e);
	if (s == 0)
		throw AvisynthError(s);
	return AVSValue(s);
}

AVSFunction CPlugin_filters[] = {
    {"LoadCPlugin", "s", load_c_plugin },
    { 0 }
};

