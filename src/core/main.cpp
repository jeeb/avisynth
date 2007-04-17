// Avisynth v2.5.  Copyright 2007 Ben Rudiak-Gould et al.
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


#define INITGUID
#define FP_STATE 0x9001f

#define WIN32_LEAN_AND_MEAN
#include <objbase.h>
#include <vfw.h>
#include <windows.h>
#include <stdio.h>
#include <malloc.h>
#include <math.h>

#include "../internal.h"
#include "../audio/audio.h"
#include <float.h>


#ifndef _DEBUG
// Release mode logging
//#define OPT_RELS_LOGGING
#ifdef OPT_RELS_LOGGING

#undef _RPT0
#undef _RPT1
#undef _RPT2
#undef _RPT3
#undef _RPT4

#define _RPT0(rptno, msg)                     ReportMe(msg)                         
#define _RPT1(rptno, msg, a1)                 ReportMe(msg, a1)                  
#define _RPT2(rptno, msg, a1, a2)             ReportMe(msg, a1, a2)            
#define _RPT3(rptno, msg, a1, a2, a3)         ReportMe(msg, a1, a2, a3)      
#define _RPT4(rptno, msg, a1, a2, a3, a4)     ReportMe(msg, a1, a2, a3, a4)
#define _RPT5(rptno, msg, a1, a2, a3, a4, a5) ReportMe(msg, a1, a2, a3, a4, a5)

void ReportMe(const char * msg, ...) {
  static char buf[256] = "";
  va_list args;
  int l = strlen(buf);

  va_start(args, msg);
  l = _vsnprintf(buf+l, sizeof(buf)-1-l, msg, args);
  buf[sizeof(buf)-1] = 0;
  va_end(args);

  if (l == -1 || strchr(buf, '\n')) {
    OutputDebugString(buf);
    buf[0] = 0;
  }
}

#else
#define _RPT5(rptno, msg, a1, a2, a3, a4, a5)
#endif
#else
#define _RPT5(rptno, msg, a1, a2, a3, a4, a5) _RPT4(rptno, msg, a1, a2, a3, a4)
#endif

const char _AVS_VERSTR[]    = AVS_VERSTR;
const char _AVS_COPYRIGHT[] = AVS_VERSTR AVS_COPYRIGHT;

static long gRefCnt=0;


extern "C" const GUID CLSID_CAVIFileSynth   // {E6D6B700-124D-11D4-86F3-DB80AFD98778}
  = {0xe6d6b700, 0x124d, 0x11d4, {0x86, 0xf3, 0xdb, 0x80, 0xaf, 0xd9, 0x87, 0x78}};

extern "C" const GUID IID_IAvisynthClipInfo   // {E6D6B708-124D-11D4-86F3-DB80AFD98778}
  = {0xe6d6b708, 0x124d, 0x11d4, {0x86, 0xf3, 0xdb, 0x80, 0xaf, 0xd9, 0x87, 0x78}};


struct IAvisynthClipInfo : IUnknown {
  virtual int __stdcall GetError(const char** ppszMessage) = 0;
  virtual bool __stdcall GetParity(int n) = 0;
  virtual bool __stdcall IsFieldBased() = 0;
};


class CAVIFileSynth: public IAVIFile, public IPersistFile, public IClassFactory, public IAvisynthClipInfo {
  friend class CAVIStreamSynth;
private:
	long m_refs;

    char* szScriptName;
    IScriptEnvironment* env;
    PClip filter_graph;
    const VideoInfo* vi;
    const char* error_msg;

    bool DelayInit();

    void MakeErrorStream(const char* msg);

public:

	CAVIFileSynth(const CLSID& rclsid);
	~CAVIFileSynth();

	static HRESULT Create(const CLSID& rclsid, const IID& riid, void **ppv);

	//////////// IUnknown

	STDMETHODIMP QueryInterface(const IID& iid, void **ppv);
	STDMETHODIMP_(ULONG) AddRef();
	STDMETHODIMP_(ULONG) Release();

	//////////// IClassFactory

	STDMETHODIMP CreateInstance (LPUNKNOWN pUnkOuter, REFIID riid,  void * * ppvObj) ;
	STDMETHODIMP LockServer (BOOL fLock) ;

	//////////// IPersistFile

	STDMETHODIMP GetClassID(LPCLSID lpClassID);  // IPersist

	STDMETHODIMP IsDirty();
	STDMETHODIMP Load(LPCOLESTR lpszFileName, DWORD grfMode);
	STDMETHODIMP Save(LPCOLESTR lpszFileName, BOOL fRemember);
	STDMETHODIMP SaveCompleted(LPCOLESTR lpszFileName);
	STDMETHODIMP GetCurFile(LPOLESTR *lplpszFileName);

	//////////// IAVIFile

	STDMETHODIMP CreateStream(PAVISTREAM *ppStream, AVISTREAMINFOW *psi);       // 5
	STDMETHODIMP EndRecord();                                                   // 8
	STDMETHODIMP GetStream(PAVISTREAM *ppStream, DWORD fccType, LONG lParam);   // 4
	STDMETHODIMP Info(AVIFILEINFOW *psi, LONG lSize);                           // 3

	STDMETHODIMP Open(LPCSTR szFile, UINT mode, LPCOLESTR lpszFileName);        // ???
    STDMETHODIMP Save(LPCSTR szFile, AVICOMPRESSOPTIONS FAR *lpOptions,         // ???
				AVISAVECALLBACK lpfnCallback);

	STDMETHODIMP ReadData(DWORD fcc, LPVOID lp, LONG *lpcb);                    // 7
	STDMETHODIMP WriteData(DWORD fcc, LPVOID lpBuffer, LONG cbBuffer);          // 6
	STDMETHODIMP DeleteStream(DWORD fccType, LONG lParam);                      // 9

	//////////// IAvisynthClipInfo

	int __stdcall GetError(const char** ppszMessage);
	bool __stdcall GetParity(int n);
	bool __stdcall IsFieldBased();
};

///////////////////////////////////

class CAVIStreamSynth;

class CAVIStreamSynth: public IAVIStream, public IAVIStreaming {
public:

	//////////// IUnknown

	STDMETHODIMP QueryInterface(const IID& iid, void **ppv);
	STDMETHODIMP_(ULONG) AddRef();
	STDMETHODIMP_(ULONG) Release();

	CAVIStreamSynth(CAVIFileSynth *parentPtr, bool isAudio);
	~CAVIStreamSynth();

	//////////// IAVIStream

	STDMETHODIMP Create(LONG lParam1, LONG lParam2);
	STDMETHODIMP Delete(LONG lStart, LONG lSamples);
	STDMETHODIMP_(LONG) Info(AVISTREAMINFOW *psi, LONG lSize);
	STDMETHODIMP_(LONG) FindSample(LONG lPos, LONG lFlags);
	STDMETHODIMP Read(LONG lStart, LONG lSamples, LPVOID lpBuffer, LONG cbBuffer, LONG *plBytes, LONG *plSamples);
	STDMETHODIMP ReadData(DWORD fcc, LPVOID lp, LONG *lpcb);
	STDMETHODIMP ReadFormat(LONG lPos, LPVOID lpFormat, LONG *lpcbFormat);
	STDMETHODIMP SetFormat(LONG lPos, LPVOID lpFormat, LONG cbFormat);
	STDMETHODIMP Write(LONG lStart, LONG lSamples, LPVOID lpBuffer,
		LONG cbBuffer, DWORD dwFlags, LONG FAR *plSampWritten, 
		LONG FAR *plBytesWritten);
	STDMETHODIMP WriteData(DWORD fcc, LPVOID lpBuffer, LONG cbBuffer);
	STDMETHODIMP SetInfo(AVISTREAMINFOW *psi, LONG lSize);

	//////////// IAVIStreaming

	STDMETHODIMP Begin(LONG lStart, LONG lEnd, LONG lRate);
	STDMETHODIMP End();

private:
	long m_refs;

	CAVIFileSynth *parent;
	BOOL fAudio;
    
    char *sName;

	//////////// internal

	void ReadHelper(void* lpBuffer, int lStart, int lSamples, unsigned code[4]);
	void ReadFrame(void* lpBuffer, int n);

	HRESULT Read2(LONG lStart, LONG lSamples, LPVOID lpBuffer, LONG cbBuffer, LONG *plBytes, LONG *plSamples);
};


BOOL APIENTRY DllMain(HANDLE hModule, ULONG ulReason, LPVOID lpReserved) {

  _RPT4(0,"DllMain: hModule=0x%08x, ulReason=%x, lpReserved=0x%08x, gRefCnt = %ld\n",
        hModule, ulReason, lpReserved, gRefCnt);

  return TRUE;
}

// From the Microsoft AVIFile docs.  Dense code...

extern "C" STDAPI DllGetClassObject(const CLSID& rclsid, const IID& riid, void **ppv);

STDAPI DllGetClassObject(const CLSID& rclsid, const IID& riid, void **ppv) {

	if (rclsid != CLSID_CAVIFileSynth) {
        _RPT0(0,"DllGetClassObject() CLASS_E_CLASSNOTAVAILABLE\n");
        return CLASS_E_CLASSNOTAVAILABLE;
	}
    _RPT0(0,"DllGetClassObject() CLSID: CAVIFileSynth\n");

    HRESULT hresult = CAVIFileSynth::Create(rclsid, riid, ppv);

	_RPT2(0,"DllGetClassObject() result=0x%X, object=%p\n", hresult, *ppv);

	return hresult;
}

extern "C" STDAPI DllCanUnloadNow();

STDAPI DllCanUnloadNow() {
	_RPT1(0,"DllCanUnloadNow(): gRefCnt = %ld\n", gRefCnt);

	return gRefCnt ? S_FALSE : S_OK;
}


///////////////////////////////////////////////////////////////////////////
//
//	CAVIFileSynth
//
///////////////////////////////////////////////////////////////////////////
//////////// IClassFactory

STDMETHODIMP CAVIFileSynth::CreateInstance (LPUNKNOWN pUnkOuter, REFIID riid,  void * * ppvObj) {

	if (pUnkOuter) {
      _RPT1(0,"%p->CAVIFileSynth::CreateInstance() CLASS_E_NOAGGREGATION\n", this);
      return CLASS_E_NOAGGREGATION;
    }
    _RPT1(0,"%p->CAVIFileSynth::CreateInstance()\n", this);

	HRESULT hresult = Create(CLSID_CAVIFileSynth, riid, ppvObj);

	_RPT3(0,"%p->CAVIFileSynth::CreateInstance() result=0x%X, object=%p\n", this, hresult, *ppvObj);

	return hresult;
}

STDMETHODIMP CAVIFileSynth::LockServer (BOOL fLock) {
	_RPT2(0,"%p->CAVIFileSynth::LockServer(%u)\n", this, fLock);
	return S_OK;
}

///////////////////////////////////////////////////
//////////// IPersistFile

STDMETHODIMP CAVIFileSynth::GetClassID(LPCLSID lpClassID) {  // IPersist
	_RPT1(0,"%p->CAVIFileSynth::GetClassID()\n", this);

    if (!lpClassID) return E_POINTER;

    *lpClassID = CLSID_CAVIFileSynth;

	return S_OK;
}

STDMETHODIMP CAVIFileSynth::IsDirty() {
	_RPT1(0,"%p->CAVIFileSynth::IsDirty()\n", this);
	return S_FALSE;
}

STDMETHODIMP CAVIFileSynth::Load(LPCOLESTR lpszFileName, DWORD grfMode) {
	char filename[MAX_PATH];

	WideCharToMultiByte(AreFileApisANSI() ? CP_ACP : CP_OEMCP, 0, lpszFileName, -1, filename, sizeof filename, NULL, NULL); 

	_RPT3(0,"%p->CAVIFileSynth::Load(\"%s\", 0x%X)\n", this, filename, grfMode);

	return Open(filename, grfMode, lpszFileName);
}

STDMETHODIMP CAVIFileSynth::Save(LPCOLESTR lpszFileName, BOOL fRemember) {
	_RPT1(0,"%p->CAVIFileSynth::Save()\n", this);
	return E_FAIL;
}

STDMETHODIMP CAVIFileSynth::SaveCompleted(LPCOLESTR lpszFileName) {
	_RPT1(0,"%p->CAVIFileSynth::SaveCompleted()\n", this);
	return S_OK;
}

STDMETHODIMP CAVIFileSynth::GetCurFile(LPOLESTR *lplpszFileName) {
	_RPT1(0,"%p->CAVIFileSynth::GetCurFile()\n", this);
	*lplpszFileName = NULL;

	return E_FAIL;
}

///////////////////////////////////////////////////
/////// static local

HRESULT CAVIFileSynth::Create(const CLSID& rclsid, const IID& riid, void **ppv) {
	HRESULT hresult;

//	_RPT0(0,"CAVIFileSynth::Create()\n");

	CAVIFileSynth* pAVIFileSynth = new CAVIFileSynth(rclsid);

	if (!pAVIFileSynth) return E_OUTOFMEMORY;

	hresult = pAVIFileSynth->QueryInterface(riid, ppv);
	pAVIFileSynth->Release();

//	_RPT1(0,"CAVIFileSynth::Create() exit, result=0x%X\n", hresult);

	return hresult;
}

///////////////////////////////////////////////////
//////////// IUnknown

STDMETHODIMP CAVIFileSynth::QueryInterface(const IID& iid, void **ppv) {

	if (!ppv) {
        _RPT1(0,"%p->CAVIFileSynth::QueryInterface() E_POINTER\n", this);
        return E_POINTER;
    }

	_RPT1(0,"%p->CAVIFileSynth::QueryInterface() ", this);
	_RPT3(0,"{%08lx-%04x-%04x-", iid.Data1, iid.Data2, iid.Data3);
	_RPT4(0,"%02x%02x-%02x%02x", iid.Data4[0], iid.Data4[1], iid.Data4[2], iid.Data4[3]);
	_RPT4(0,"%02x%02x%02x%02x} (", iid.Data4[4], iid.Data4[5], iid.Data4[6], iid.Data4[7]);

	if (iid == IID_IUnknown) {
		*ppv = (IUnknown *)(IAVIFile *)this;
		_RPT0(0,"IUnknown)\n");
	} else if (iid == IID_IClassFactory) {
		*ppv = (IClassFactory *)this;
		_RPT0(0,"IClassFactory)\n");
	} else if (iid == IID_IPersist) {
		*ppv = (IPersist *)this;
		_RPT0(0,"IPersist)\n");
	} else if (iid == IID_IPersistFile) {
		*ppv = (IPersistFile *)this;
		_RPT0(0,"IPersistFile)\n");
	} else if (iid == IID_IAVIFile) {
		*ppv = (IAVIFile *)this;
		_RPT0(0,"IAVIFile)\n");
	} else if (iid == IID_IAvisynthClipInfo) {
		*ppv = (IAvisynthClipInfo *)this;
		_RPT0(0,"IAvisynthClipInfo)\n");
	} else {
		_RPT0(0,"unsupported!)\n");
		*ppv = NULL;
		return E_NOINTERFACE;
	}

	AddRef();

	return S_OK;
}

STDMETHODIMP_(ULONG) CAVIFileSynth::AddRef() {

	const int refs = InterlockedIncrement(&m_refs);
    InterlockedIncrement(&gRefCnt);

	_RPT3(0,"%p->CAVIFileSynth::AddRef() gRefCnt=%d, m_refs=%d\n", this, gRefCnt, refs);

	return refs;
}

STDMETHODIMP_(ULONG) CAVIFileSynth::Release() {

    const int refs = InterlockedDecrement(&m_refs);
    InterlockedDecrement(&gRefCnt);

	_RPT3(0,"%p->CAVIFileSynth::Release() gRefCnt=%d, m_refs=%d\n", this, gRefCnt, refs);

	if (!refs) delete this;
	return refs;
}

////////////////////////////////////////////////////////////////////////
//
//		CAVIStreamSynth
//
////////////////////////////////////////////////////////////////////////
//////////// IUnknown

STDMETHODIMP CAVIStreamSynth::QueryInterface(const IID& iid, void **ppv) {

	if (!ppv) {
        _RPT2(0,"%p->CAVIStreamSynth::QueryInterface() (%s) E_POINTER\n", this, sName);
        return E_POINTER;
    }

	_RPT2(0,"%p->CAVIStreamSynth::QueryInterface() (%s) ", this, sName);
	_RPT3(0,"{%08lx-%04x-%04x-", iid.Data1, iid.Data2, iid.Data3);
	_RPT4(0,"%02x%02x-%02x%02x", iid.Data4[0], iid.Data4[1], iid.Data4[2], iid.Data4[3]);
	_RPT4(0,"%02x%02x%02x%02x} (", iid.Data4[4], iid.Data4[5], iid.Data4[6], iid.Data4[7]);

	if (iid == IID_IUnknown) {
		*ppv = (IUnknown *)(IAVIStream *)this;
		_RPT0(0,"IUnknown)\n");
	} else if (iid == IID_IAVIStream) {
		*ppv = (IAVIStream *)this;
		_RPT0(0,"IAVIStream)\n");
	} else if (iid == IID_IAVIStreaming) {
		*ppv = (IAVIStreaming *)this;
		_RPT0(0,"IAVIStreaming)\n");
	} else {
		_RPT0(0,"unsupported!)\n");
		*ppv = NULL;
		return E_NOINTERFACE;
	}

	AddRef();

	return S_OK;
}

STDMETHODIMP_(ULONG) CAVIStreamSynth::AddRef() {

	const int refs = InterlockedIncrement(&m_refs);
    InterlockedIncrement(&gRefCnt);

	_RPT4(0,"%p->CAVIStreamSynth::AddRef() (%s) gRefCnt=%d, m_refs=%d\n", this, sName, gRefCnt, refs);

	return refs;
}

STDMETHODIMP_(ULONG) CAVIStreamSynth::Release() {

    const int refs = InterlockedDecrement(&m_refs);
    InterlockedDecrement(&gRefCnt);

	_RPT4(0,"%p->CAVIStreamSynth::Release() (%s) gRefCnt=%d, m_refs=%d\n", this, sName, gRefCnt, refs);

	if (!refs) delete this;
	return refs;
}

////////////////////////////////////////////////////////////////////////
//
//		CAVIFileSynth
//
////////////////////////////////////////////////////////////////////////
//////////// IAVIFile

STDMETHODIMP CAVIFileSynth::CreateStream(PAVISTREAM *ppStream, AVISTREAMINFOW *psi) {
	_RPT1(0,"%p->CAVIFileSynth::CreateStream()\n", this);
	*ppStream = NULL;
	return S_OK;//AVIERR_READONLY;
}

STDMETHODIMP CAVIFileSynth::EndRecord() {
	_RPT1(0,"%p->CAVIFileSynth::EndRecord()\n", this);
	return AVIERR_READONLY;
}

STDMETHODIMP CAVIFileSynth::Save(LPCSTR szFile, AVICOMPRESSOPTIONS FAR *lpOptions,
				AVISAVECALLBACK lpfnCallback) {
	_RPT1(0,"%p->CAVIFileSynth::Save()\n", this);
	return AVIERR_READONLY;
}

STDMETHODIMP CAVIFileSynth::ReadData(DWORD fcc, LPVOID lp, LONG *lpcb) {
	_RPT1(0,"%p->CAVIFileSynth::ReadData()\n", this);
	return AVIERR_NODATA;
}

STDMETHODIMP CAVIFileSynth::WriteData(DWORD fcc, LPVOID lpBuffer, LONG cbBuffer) {
	_RPT1(0,"%p->CAVIFileSynth::WriteData()\n", this);
	return AVIERR_READONLY;
}

STDMETHODIMP CAVIFileSynth::DeleteStream(DWORD fccType, LONG lParam) {
	_RPT1(0,"%p->CAVIFileSynth::DeleteStream()\n", this);
	return AVIERR_READONLY;
}


///////////////////////////////////////////////////
/////// local

CAVIFileSynth::CAVIFileSynth(const CLSID& rclsid) {
	_RPT1(0,"%p->CAVIFileSynth::CAVIFileSynth()\n", this);

	m_refs = 0; AddRef();

    szScriptName = 0;
    env = 0;

    error_msg = 0;
}

CAVIFileSynth::~CAVIFileSynth() {
	_RPT2(0,"%p->CAVIFileSynth::~CAVIFileSynth(), gRefCnt = %d\n", this, gRefCnt);

    delete[] szScriptName;

    filter_graph = 0;
    
	delete env;
}


STDMETHODIMP CAVIFileSynth::Open(LPCSTR szFile, UINT mode, LPCOLESTR lpszFileName) {

//	_RPT3(0,"%p->CAVIFileSynth::Open(\"%s\", 0x%08lx)\n", this, szFile, mode);

	if (mode & (OF_CREATE|OF_WRITE))
      return E_FAIL;

    delete env;   // just in case
    env = 0;
    filter_graph = 0;
    vi = 0;

    szScriptName = new char[lstrlen(szFile)+1];
    if (!szScriptName)
      return AVIERR_MEMORY;
    lstrcpy(szScriptName, szFile);

    return S_OK;
}

bool CAVIFileSynth::DelayInit() {
// _RPT1(0,"Original: 0x%.4x\n", _control87( 0, 0 ) );
 int fp_state = _control87( 0, 0 );
 _control87( FP_STATE, 0xffffffff );
 if (szScriptName) {
#ifndef _DEBUG
    try {
#endif
      try {
        // create a script environment and load the script into it
        env = CreateScriptEnvironment();
        if (!env) return false;
      }
      catch (AvisynthError error) {
        error_msg = error.msg;
		return false;
	  }
	  try {
        AVSValue return_val = env->Invoke("Import", szScriptName);
        // store the script's return value (a video clip)
        if (return_val.IsClip()) {

          // Allow WAVE_FORMAT_IEEE_FLOAT audio output
          bool AllowFloatAudio = false;

          try {
            AVSValue v = env->GetVar("OPT_AllowFloatAudio");
            AllowFloatAudio = v.IsBool() ? v.AsBool() : false;
          }
          catch (IScriptEnvironment::NotFound) { }

          filter_graph = return_val.AsClip();

          if (!AllowFloatAudio) // Ensure samples are int     
            filter_graph = ConvertAudio::Create(filter_graph, SAMPLE_INT8|SAMPLE_INT16|SAMPLE_INT24|SAMPLE_INT32, SAMPLE_INT16);

          filter_graph = Cache::Create_Cache(AVSValue(filter_graph), 0, env).AsClip();

          filter_graph->SetCacheHints(CACHE_ALL, 999); // Give the top level cache a big head start!!
        }
        else
          throw AvisynthError("The script's return value was not a video clip");

        if (!filter_graph)
          throw AvisynthError("The returned video clip was nil (this is a bug)");

        // get information about the clip
        vi = &filter_graph->GetVideoInfo();

        if (vi->IsYV12()&&(vi->width&3))
          throw AvisynthError("Avisynth error: YV12 images for output must have a width divisible by 4 (use crop)!");
        if (vi->IsYUY2()&&(vi->width&3))
          throw AvisynthError("Avisynth error: YUY2 images for output must have a width divisible by 4 (use crop)!");
      }
      catch (AvisynthError error) {
        error_msg = error.msg;
        AVSValue args[2] = { error.msg, 0xff3333 };
        static const char* arg_names[2] = { 0, "text_color" };
        try {
          filter_graph = env->Invoke("MessageClip", AVSValue(args, 2), arg_names).AsClip();
          vi = &filter_graph->GetVideoInfo();
        }
		catch (AvisynthError) {
          filter_graph = 0;
        }
      }

      if (szScriptName)
        delete[] szScriptName;
      szScriptName = 0;
      _clear87();
      __asm {emms};
      _control87( fp_state, 0xffffffff );
      return true;
#ifndef _DEBUG
    }
    catch (...) {
      _RPT0(1,"DelayInit() caught general exception!\n");
      _clear87();
      __asm {emms};
      _control87( fp_state, 0xffffffff );
      return false;
    }
#endif
  } else {
    _clear87();
    __asm {emms};
    _control87( fp_state, 0xffffffff );
    return (env && filter_graph && vi);
  }
}


void CAVIFileSynth::MakeErrorStream(const char* msg) {
  error_msg = msg;
  filter_graph = Create_MessageClip(msg, vi->width, vi->height, vi->pixel_type, false, 0xFF3333, 0, 0, env);
}

///////////////////////////////////////////////////
//////////// IAVIFile

STDMETHODIMP CAVIFileSynth::Info(AVIFILEINFOW *pfi, LONG lSize) {

	_RPT2(0,"%p->CAVIFileSynth::Info(pfi, %d)\n", this, lSize);

	if (!pfi) return E_POINTER;

	if (!DelayInit()) return E_FAIL;

	AVIFILEINFOW afi;

    memset(&afi, 0, sizeof(afi));

	afi.dwMaxBytesPerSec	= 0;
	afi.dwFlags				= AVIFILEINFO_HASINDEX | AVIFILEINFO_ISINTERLEAVED;
	afi.dwCaps				= AVIFILECAPS_CANREAD | AVIFILECAPS_ALLKEYFRAMES | AVIFILECAPS_NOCOMPRESSION;
	
	int nrStreams=0;
	if (vi->HasVideo()==true)	nrStreams=1;
	if (vi->HasAudio()==true)	nrStreams++;

	afi.dwStreams				= nrStreams;
	afi.dwSuggestedBufferSize	= 0;
	afi.dwWidth					= vi->width;
	afi.dwHeight				= vi->height;
	afi.dwEditCount				= 0;

    afi.dwRate					= vi->fps_numerator;
	afi.dwScale					= vi->fps_denominator;
	afi.dwLength				= vi->num_frames;

	wcscpy(afi.szFileType, L"Avisynth");

// Maybe should return AVIERR_BUFFERTOOSMALL for lSize < sizeof(afi)
    memset(pfi, 0, lSize);
    memcpy(pfi, &afi, min(size_t(lSize), sizeof(afi)));
	return S_OK;
}

static inline char BePrintable(int ch) {
  ch &= 0xff;
  return isprint(ch) ? ch : '.';
}


STDMETHODIMP CAVIFileSynth::GetStream(PAVISTREAM *ppStream, DWORD fccType, LONG lParam) {
	CAVIStreamSynth *casr;
	char fcc[5];

	fcc[0] = BePrintable(fccType      );
	fcc[1] = BePrintable(fccType >>  8);
	fcc[2] = BePrintable(fccType >> 16);
	fcc[3] = BePrintable(fccType >> 24);
	fcc[4] = 0;

	_RPT4(0,"%p->CAVIFileSynth::GetStream(*, %08x(%s), %ld)\n", this, fccType, fcc, lParam);

	if (!DelayInit()) return E_FAIL;

    *ppStream = NULL;

	if (!fccType) 
	{
// Maybe an Option to set the order of stream discovery
		if ((lParam==0) && (vi->HasVideo()) )
			fccType = streamtypeVIDEO;
		else 
		  if ( ((lParam==1) && (vi->HasVideo())) ||  ((lParam==0) && vi->HasAudio()) )
		  {
			lParam=0;
			fccType = streamtypeAUDIO;
		  }
	}

	if (lParam > 0) return AVIERR_NODATA;

	if (fccType == streamtypeVIDEO) {
		if (!vi->HasVideo())
			return AVIERR_NODATA;

        if ((casr = new CAVIStreamSynth(this, false)) == 0)
			return AVIERR_MEMORY;

		*ppStream = (IAVIStream *)casr;

	} else if (fccType == streamtypeAUDIO) {
		if (!vi->HasAudio())
			return AVIERR_NODATA;

		if ((casr = new CAVIStreamSynth(this, true)) == 0)
			return AVIERR_MEMORY;

		*ppStream = (IAVIStream *)casr;
	} else
		return AVIERR_NODATA;

	return S_OK;
}


////////////////////////////////////////////////////////////////////////
//////////// IAvisynthClipInfo

int __stdcall CAVIFileSynth::GetError(const char** ppszMessage) {
  if (!DelayInit() && !error_msg)
    error_msg = "Avisynth: script open failed!";

  if (ppszMessage)
    *ppszMessage = error_msg;
  return !!error_msg;
}

bool __stdcall CAVIFileSynth::GetParity(int n) {
  if (!DelayInit())
    return false;
  return filter_graph->GetParity(n);
}

bool __stdcall CAVIFileSynth::IsFieldBased() {
  if (!DelayInit())
    return false;
  return vi->IsFieldBased();
}


////////////////////////////////////////////////////////////////////////
//
//		CAVIStreamSynth
//
////////////////////////////////////////////////////////////////////////
//////////// IAVIStreaming

STDMETHODIMP CAVIStreamSynth::Begin(LONG lStart, LONG lEnd, LONG lRate) {
	_RPT5(0,"%p->CAVIStreamSynth::Begin(%ld, %ld, %ld) (%s)\n", this, lStart, lEnd, lRate, sName);
	return S_OK;
}

STDMETHODIMP CAVIStreamSynth::End() {
	_RPT2(0,"%p->CAVIStreamSynth::End() (%s)\n", this, sName);
	return S_OK;
}

//////////// IAVIStream

STDMETHODIMP CAVIStreamSynth::Create(LONG lParam1, LONG lParam2) {
	_RPT1(0,"%p->CAVIStreamSynth::Create()\n", this);
	return AVIERR_READONLY;
}

STDMETHODIMP CAVIStreamSynth::Delete(LONG lStart, LONG lSamples) {
	_RPT1(0,"%p->CAVIStreamSynth::Delete()\n", this);
	return AVIERR_READONLY;
}

STDMETHODIMP CAVIStreamSynth::ReadData(DWORD fcc, LPVOID lp, LONG *lpcb) {
	_RPT1(0,"%p->CAVIStreamSynth::ReadData()\n", this);
	return AVIERR_NODATA;
}

STDMETHODIMP CAVIStreamSynth::SetFormat(LONG lPos, LPVOID lpFormat, LONG cbFormat) {
	_RPT1(0,"%p->CAVIStreamSynth::SetFormat()\n", this);
	return AVIERR_READONLY;
}

STDMETHODIMP CAVIStreamSynth::WriteData(DWORD fcc, LPVOID lpBuffer, LONG cbBuffer) {
	_RPT1(0,"%p->CAVIStreamSynth::WriteData()\n", this);
	return AVIERR_READONLY;
}

STDMETHODIMP CAVIStreamSynth::SetInfo(AVISTREAMINFOW *psi, LONG lSize) {
	_RPT1(0,"%p->CAVIStreamSynth::SetInfo()\n", this);
	return AVIERR_READONLY;
}

////////////////////////////////////////////////////////////////////////
//////////// local

CAVIStreamSynth::CAVIStreamSynth(CAVIFileSynth *parentPtr, bool isAudio) {

  sName = isAudio ? "audio" : "video";

  _RPT2(0,"%p->CAVIStreamSynth(%s)\n", this, sName);

  m_refs = 0; AddRef();

  parent			= parentPtr;
  fAudio			= isAudio;

  parent->AddRef();
}

CAVIStreamSynth::~CAVIStreamSynth() {
  _RPT3(0,"%p->~CAVIStreamSynth() (%s), gRefCnt = %d\n", this, sName, gRefCnt);

  if (parent)
    parent->Release();
}

////////////////////////////////////////////////////////////////////////
//////////// IAVIStream

STDMETHODIMP_(LONG) CAVIStreamSynth::Info(AVISTREAMINFOW *psi, LONG lSize) {
	_RPT4(0,"%p->CAVIStreamSynth::Info(%p, %ld) (%s)\n", this, psi, lSize, sName);

	if (!psi) return E_POINTER;

	AVISTREAMINFOW asi;

    const VideoInfo* const vi = parent->vi;

    memset(&asi, 0, sizeof(asi));
    asi.fccType = fAudio ? streamtypeAUDIO : streamtypeVIDEO;
    asi.dwQuality = DWORD(-1);
    if (fAudio) {
      asi.fccHandler = 0;
      int bytes_per_sample = vi->BytesPerAudioSample();
      asi.dwScale = bytes_per_sample;
      asi.dwRate = vi->audio_samples_per_second * bytes_per_sample;
      asi.dwLength = (unsigned long)vi->num_audio_samples;
      asi.dwSampleSize = bytes_per_sample;
      wcscpy(asi.szName, L"Avisynth audio #1");
    } else {
      const int image_size = vi->BMPSize();
      asi.fccHandler = 'UNKN';
      if (vi->IsRGB()) 
        asi.fccHandler = ' BID';
      else if (vi->IsYUY2())
        asi.fccHandler = '2YUY';
      else if (vi->IsYV12())
        asi.fccHandler = '21VY'; 
      else {
        _ASSERT(FALSE);
      }

      asi.dwScale = vi->fps_denominator;
      asi.dwRate = vi->fps_numerator;
      asi.dwLength = vi->num_frames;
      asi.rcFrame.right = vi->width;
      asi.rcFrame.bottom = vi->height;
      asi.dwSampleSize = image_size;
      asi.dwSuggestedBufferSize = image_size;
      wcscpy(asi.szName, L"Avisynth video #1");
    }

// Maybe should return AVIERR_BUFFERTOOSMALL for lSize < sizeof(asi)
    memset(psi, 0, lSize);
    memcpy(psi, &asi, min(size_t(lSize), sizeof(asi)));
	return S_OK;
}

STDMETHODIMP_(LONG) CAVIStreamSynth::FindSample(LONG lPos, LONG lFlags) {
//	_RPT3(0,"%p->CAVIStreamSynth::FindSample(%ld, %08lx)\n", this, lPos, lFlags);

	if (lFlags & FIND_FORMAT)
		return -1;

	if (lFlags & FIND_FROM_START)
		return 0;

	return lPos;
}


////////////////////////////////////////////////////////////////////////
//////////// local

void CAVIStreamSynth::ReadFrame(void* lpBuffer, int n) {
  PVideoFrame frame = parent->filter_graph->GetFrame(n, parent->env);
  if (!frame)
    parent->env->ThrowError("Avisynth error: generated video frame was nil (this is a bug)");

//  VideoInfo vi = parent->filter_graph->GetVideoInfo();
  const int pitch    = frame->GetPitch();
  const int row_size = frame->GetRowSize();
  const int height   = frame->GetHeight();

  // BMP scanlines are always dword-aligned
  const int out_pitch = (row_size+3) & -4;

  BitBlt((BYTE*)lpBuffer, out_pitch, frame->GetReadPtr(), pitch, row_size, height);

  BitBlt((BYTE*)lpBuffer + (out_pitch*height),
         out_pitch/2,               frame->GetReadPtr(PLANAR_V),
		 frame->GetPitch(PLANAR_V), frame->GetRowSize(PLANAR_V),
		 frame->GetHeight(PLANAR_V) );

  BitBlt((BYTE*)lpBuffer + (out_pitch*height + frame->GetHeight(PLANAR_V)*out_pitch/2),
         out_pitch/2,               frame->GetReadPtr(PLANAR_U),
		 frame->GetPitch(PLANAR_U), frame->GetRowSize(PLANAR_U),
		 frame->GetHeight(PLANAR_U) );
}

#define OPT_OWN_SEH_HANDLER
#ifdef OPT_OWN_SEH_HANDLER
EXCEPTION_DISPOSITION __cdecl _Exp_except_handler2(struct _EXCEPTION_RECORD *ExceptionRecord, void * EstablisherFrame,
												  struct _CONTEXT *ContextRecord, void * DispatcherContext)
{
  struct Est_Frame {  // My extended EXCEPTION_REGISTRATION record
	void	  *prev;
	void	  *handler;
	unsigned  *retarg[4];	  // pointer where to stash exception code
  };

  if (ExceptionRecord->ExceptionFlags == 0)	{  // First pass?
	*(((struct Est_Frame *)EstablisherFrame)->retarg[0]) = ExceptionRecord->ExceptionCode;
	*(((struct Est_Frame *)EstablisherFrame)->retarg[1]) = (unsigned)ExceptionRecord->ExceptionAddress;
	if (ExceptionRecord->NumberParameters >= 2)	{  // Extra Info?
	  *(((struct Est_Frame *)EstablisherFrame)->retarg[2]) = ExceptionRecord->ExceptionInformation[0];
	  *(((struct Est_Frame *)EstablisherFrame)->retarg[3]) = ExceptionRecord->ExceptionInformation[1];
	}
  }
  return ExceptionContinueSearch;
}

void CAVIStreamSynth::ReadHelper(void* lpBuffer, int lStart, int lSamples, unsigned code[4]) {

  DWORD handler = (DWORD)_Exp_except_handler2;

  __asm { // Build EXCEPTION_REGISTRATION record:
  push	code		// Address of return argument
  push	handler	    // Address of handler function
  push	FS:[0]		// Address of previous handler
  mov	FS:[0],esp	// Install new EXCEPTION_REGISTRATION
  }

  if (fAudio)
    parent->filter_graph->GetAudio(lpBuffer, lStart, lSamples, parent->env);
  else
    ReadFrame(lpBuffer, lStart);

  __asm { // Remove our EXCEPTION_REGISTRATION record
  mov	eax,[esp]	// Get pointer to previous record
  mov	FS:[0], eax	// Install previous record
  add	esp, 12		// Clean our EXCEPTION_REGISTRATION off stack
  }
}

static const char * const StringSystemError2(const unsigned code)
{
  switch (code) {
  case STATUS_GUARD_PAGE_VIOLATION:      // 0x80000001
    return "Guard Page Violation";
  case STATUS_DATATYPE_MISALIGNMENT:     // 0x80000002
    return "Datatype Misalignment";
  case STATUS_BREAKPOINT:                // 0x80000003
    return "Breakpoint";
  case STATUS_SINGLE_STEP:               // 0x80000004
    return "Single Step";
  default:
    break;
  }
  
  switch (code) {
  case STATUS_ACCESS_VIOLATION:          // 0xc0000005
    return "*Access Violation";
  case STATUS_IN_PAGE_ERROR:             // 0xc0000006
    return "In Page Error";
  case STATUS_INVALID_HANDLE:            // 0xc0000008
    return "Invalid Handle";
  case STATUS_NO_MEMORY:                 // 0xc0000017
    return "No Memory";
  case STATUS_ILLEGAL_INSTRUCTION:       // 0xc000001d
    return "Illegal Instruction";
  case STATUS_NONCONTINUABLE_EXCEPTION:  // 0xc0000025
    return "Noncontinuable Exception";
  case STATUS_INVALID_DISPOSITION:       // 0xc0000026
    return "Invalid Disposition";
  case STATUS_ARRAY_BOUNDS_EXCEEDED:     // 0xc000008c
    return "Array Bounds Exceeded";
  case STATUS_FLOAT_DENORMAL_OPERAND:    // 0xc000008d
    return "Float Denormal Operand";
  case STATUS_FLOAT_DIVIDE_BY_ZERO:      // 0xc000008e
    return "Float Divide by Zero";
  case STATUS_FLOAT_INEXACT_RESULT:      // 0xc000008f
    return "Float Inexact Result";
  case STATUS_FLOAT_INVALID_OPERATION:   // 0xc0000090
    return "Float Invalid Operation";
  case STATUS_FLOAT_OVERFLOW:            // 0xc0000091
    return "Float Overflow";
  case STATUS_FLOAT_STACK_CHECK:         // 0xc0000092
    return "Float Stack Check";
  case STATUS_FLOAT_UNDERFLOW:           // 0xc0000093
    return "Float Underflow";
  case STATUS_INTEGER_DIVIDE_BY_ZERO:    // 0xc0000094
    return "Integer Divide by Zero";
  case STATUS_INTEGER_OVERFLOW:          // 0xc0000095
    return "Integer Overflow";
  case STATUS_PRIVILEGED_INSTRUCTION:    // 0xc0000096
    return "Privileged Instruction";
  case STATUS_STACK_OVERFLOW:            // 0xc00000fd
    return "Stack Overflow";
  default:
    break;
  }
  return 0;
}
#else
void CAVIStreamSynth::ReadHelper(void* lpBuffer, int lStart, int lSamples, unsigned &xcode[4]) {
  // It's illegal to call GetExceptionInformation() inside an __except
  // block!  Hence this variable and the horrible hack below...
#ifndef _DEBUG
  EXCEPTION_POINTERS* ei;
  DWORD code;
  __try { 
#endif
    if (fAudio)
      parent->filter_graph->GetAudio(lpBuffer, lStart, lSamples, parent->env);
    else
      ReadFrame(lpBuffer, lStart);
#ifndef _DEBUG
  }
  __except (ei = GetExceptionInformation(), code = GetExceptionCode(), (code >> 28) == 0xC) {
    switch (code) {
    case EXCEPTION_ACCESS_VIOLATION:
      parent->env->ThrowError("Avisynth: caught an access violation at 0x%08x,\nattempting to %s 0x%08x",
        ei->ExceptionRecord->ExceptionAddress,
        ei->ExceptionRecord->ExceptionInformation[0] ? "write to" : "read from",
        ei->ExceptionRecord->ExceptionInformation[1]);
    case EXCEPTION_ILLEGAL_INSTRUCTION:
      parent->env->ThrowError("Avisynth: illegal instruction at 0x%08x",
        ei->ExceptionRecord->ExceptionAddress);
    case EXCEPTION_INT_DIVIDE_BY_ZERO:
      parent->env->ThrowError("Avisynth: division by zero at 0x%08x",
        ei->ExceptionRecord->ExceptionAddress);
    case EXCEPTION_STACK_OVERFLOW:
      throw AvisynthError("Avisynth: stack overflow");
    default:
      parent->env->ThrowError("Avisynth: unknown exception 0x%08x at 0x%08x",
        code, ei->ExceptionRecord->ExceptionAddress);
    }
  }
#endif
}
#endif

////////////////////////////////////////////////////////////////////////
//////////// IAVIStream

STDMETHODIMP CAVIStreamSynth::Read(LONG lStart, LONG lSamples, LPVOID lpBuffer, LONG cbBuffer, LONG *plBytes, LONG *plSamples) {

  __asm { // Force compiler to protect these registers!
    mov ebx,ebx;
    mov esi,esi;
    mov edi,edi;
  }
  return Read2(lStart, lSamples, lpBuffer, cbBuffer, plBytes, plSamples);
}

HRESULT CAVIStreamSynth::Read2(LONG lStart, LONG lSamples, LPVOID lpBuffer, LONG cbBuffer, LONG *plBytes, LONG *plSamples) {

//  _RPT3(0,"%p->CAVIStreamSynth::Read(%ld samples at %ld)\n", this, lSamples, lStart);
//  _RPT2(0,"\tbuffer: %ld bytes at %p\n", cbBuffer, lpBuffer);
  int fp_state = _control87( 0, 0 );
  _control87( FP_STATE, 0xffffffff );
  unsigned code[4] = {0, 0, 0, 0};

  if (fAudio) {
    // buffer overflow patch -- Avery Lee - Mar 2006
    if (lSamples == AVISTREAMREAD_CONVENIENT)
      lSamples = (long)parent->vi->AudioSamplesFromFrames(1);
    long bytes = (long)parent->vi->BytesFromAudioSamples(lSamples);
    if (lpBuffer && bytes > cbBuffer) {
      lSamples = (long)parent->vi->AudioSamplesFromBytes(cbBuffer);
      bytes = (long)parent->vi->BytesFromAudioSamples(lSamples);
    }
    if (plBytes) *plBytes = bytes;
    if (plSamples) *plSamples = lSamples;
    if (!lpBuffer)
      return S_OK;

  } else {
    int image_size = parent->vi->BMPSize();
    if (plSamples) *plSamples = 1;
    if (plBytes) *plBytes = image_size;

    if (!lpBuffer) {
      return S_OK;
    } else if (cbBuffer < image_size) {
//      _RPT1(0,"\tBuffer too small; should be %ld samples\n", image_size);
      return AVIERR_BUFFERTOOSMALL;
    }
  }

#ifndef _DEBUG
  try {
    try {
#endif
      // VC compiler says "only one form of exception handling permitted per
      // function."  Sigh...
      ReadHelper(lpBuffer, lStart, lSamples, code);
#ifndef _DEBUG
    }
    catch (AvisynthError error) {
      parent->MakeErrorStream(error.msg);
      if (fAudio)
	    throw;
	  else
	    ReadHelper(lpBuffer, lStart, lSamples, code);
    }
    catch (...) {
#ifdef OPT_OWN_SEH_HANDLER
      if (code[0] != 0xE06D7363 && code[0] != 0) {
		char buf[128];
        const char * const extext = StringSystemError2(code[0]);
        if (extext) {
		  if (extext[0] == '*') {
			const char * const rwtext = code[2] ? "writing to" : "reading from";
			_snprintf(buf, 127, "CAVIStreamSynth: System exception - %s at 0x%x, %s 0x%x", extext+1, code[1], rwtext, code[3]);
		  }
		  else
			_snprintf(buf, 127, "CAVIStreamSynth: System exception - %s at 0x%x", extext, code[1]);
		}
        else {
          _snprintf(buf, 127, "CAVIStreamSynth: Unknown system exception - 0x%x at 0x%x", code[0], code[1]);
        }
		parent->MakeErrorStream(buf);
      }
      else parent->MakeErrorStream("Avisynth: unknown exception");
      code[0] = 0;
#else
      parent->MakeErrorStream("Avisynth: unknown exception");
#endif
      if (fAudio)
	    throw;
	  else
        ReadHelper(lpBuffer, lStart, lSamples, code);
    }
  }
  catch (...) {
    _clear87();
    __asm {emms};
    _control87( fp_state, 0xffffffff );
    return E_FAIL;
  }
#endif
  _clear87();
    __asm {emms};
  _control87( fp_state, 0xffffffff );
  return S_OK;
}

STDMETHODIMP CAVIStreamSynth::ReadFormat(LONG lPos, LPVOID lpFormat, LONG *lpcbFormat) {
  _RPT2(0,"%p->CAVIStreamSynth::ReadFormat() (%s)\n", this, sName);

  if (!lpcbFormat) return E_POINTER;

  if (!lpFormat) {
    *lpcbFormat = fAudio ? sizeof(WAVEFORMATEX) : sizeof(BITMAPINFOHEADER);
	  return S_OK;
  }

  memset(lpFormat, 0, *lpcbFormat);

  const VideoInfo* const vi = parent->vi;

  if (fAudio) {
	bool UseWaveExtensible = false;

	try {
	  AVSValue v = parent->env->GetVar("OPT_UseWaveExtensible");
	  UseWaveExtensible = v.IsBool() ? v.AsBool() : false;
	}
	catch (IScriptEnvironment::NotFound) { }

	if (UseWaveExtensible) {  // Use WAVE_FORMAT_EXTENSIBLE audio output format 
	  const GUID KSDATAFORMAT_SUBTYPE_PCM       = {0x00000001, 0x0000, 0x0010, {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}};
	  const GUID KSDATAFORMAT_SUBTYPE_IEEE_FLOAT= {0x00000003, 0x0000, 0x0010, {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}};
	  WAVEFORMATEXTENSIBLE wfxt;

	  memset(&wfxt, 0, sizeof(wfxt));
	  wfxt.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
	  wfxt.Format.nChannels = vi->AudioChannels();
	  wfxt.Format.nSamplesPerSec = vi->SamplesPerSecond();
	  wfxt.Format.wBitsPerSample = vi->BytesPerChannelSample() * 8;
	  wfxt.Format.nBlockAlign = vi->BytesPerAudioSample();
	  wfxt.Format.nAvgBytesPerSec = wfxt.Format.nSamplesPerSec * wfxt.Format.nBlockAlign;
	  wfxt.Format.cbSize = sizeof(wfxt) - sizeof(wfxt.Format);
	  wfxt.Samples.wValidBitsPerSample = wfxt.Format.wBitsPerSample;
	  wfxt.dwChannelMask = SPEAKER_ALL;
	  wfxt.SubFormat = vi->IsSampleType(SAMPLE_FLOAT) ? KSDATAFORMAT_SUBTYPE_IEEE_FLOAT : KSDATAFORMAT_SUBTYPE_PCM;
	  memcpy(lpFormat, &wfxt, min(size_t(*lpcbFormat), sizeof(wfxt)));
	}
	else {
	  WAVEFORMATEX wfx;
	  memset(&wfx, 0, sizeof(wfx));
	  wfx.wFormatTag = vi->IsSampleType(SAMPLE_FLOAT) ? WAVE_FORMAT_IEEE_FLOAT : WAVE_FORMAT_PCM;
	  wfx.nChannels = vi->AudioChannels();
	  wfx.nSamplesPerSec = vi->SamplesPerSecond();
	  wfx.wBitsPerSample = vi->BytesPerChannelSample() * 8;
	  wfx.nBlockAlign = vi->BytesPerAudioSample();
	  wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;
	  memcpy(lpFormat, &wfx, min(size_t(*lpcbFormat), sizeof(wfx)));
	}
  } else {
    BITMAPINFOHEADER bi;
    memset(&bi, 0, sizeof(bi));
    bi.biSize = sizeof(bi);
    bi.biWidth = vi->width;
    bi.biHeight = vi->height;
    bi.biPlanes = 1;
    bi.biBitCount = vi->BitsPerPixel();
      if (vi->IsRGB()) 
        bi.biCompression = BI_RGB;
      else if (vi->IsYUY2())
        bi.biCompression = '2YUY';
      else if (vi->IsYV12())
        bi.biCompression = '21VY';
      else {
        _ASSERT(FALSE);
      }
    bi.biSizeImage = vi->BMPSize();
    memcpy(lpFormat, &bi, min(size_t(*lpcbFormat), sizeof(bi)));
  }
  return S_OK;
}

STDMETHODIMP CAVIStreamSynth::Write(LONG lStart, LONG lSamples, LPVOID lpBuffer,
	LONG cbBuffer, DWORD dwFlags, LONG FAR *plSampWritten, 
    LONG FAR *plBytesWritten) {

	_RPT1(0,"%p->CAVIStreamSynth::Write()\n", this);

	return AVIERR_READONLY;
}

