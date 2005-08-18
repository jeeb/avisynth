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
 

#include "directshow_source.h"

#ifdef _DEBUG
char* PrintGUID(const GUID *g) {

  static char buf[40];

  if (g) {
	_snprintf(buf, 40, "{%08x-%04hx-%04hx-%02x%02x-%02x%02x%02x%02x%02x%02x}\0",
			  g->Data1,    g->Data2,    g->Data3,    g->Data4[0], g->Data4[1],
			  g->Data4[2], g->Data4[3], g->Data4[4], g->Data4[5], g->Data4[6], g->Data4[7]);
  }
  else {
    strcpy(buf, "<null>");
  }
  return buf;
}
#endif

inline void InitMediaType(AM_MEDIA_TYPE* &media_type, const GUID &major, const GUID &sub) {
  media_type = new AM_MEDIA_TYPE;
  memset(media_type, 0, sizeof(AM_MEDIA_TYPE));
  media_type->majortype  = major;
  media_type->subtype    = sub;
  media_type->formattype = GUID_NULL;
}


/************************************
 *             GetSample            *
 ************************************/


GetSample::GetSample(bool _load_audio, bool _load_video, unsigned _media)
  : load_audio(_load_audio), load_video(_load_video), media(_media),
    streamName(_load_audio ? "audio" : "video") {
	am_media_type = 0;
    refcnt = 1;
    source_pin = 0;
    filter_graph = 0;
    pclock = 0;
    m_pPos =0;
    state = State_Stopped;
	a_sample_bytes = 0;
    av_buffer = 0;
    flushing = end_of_stream = false;
    memset(&vi, 0, sizeof(vi));
    sample_end_time = sample_start_time = segment_start_time = 0;
    evtDoneWithSample = ::CreateEvent(NULL, FALSE, FALSE, NULL);
    evtNewSampleReady = ::CreateEvent(NULL, FALSE, FALSE, NULL);

    if (load_audio) {
	  unsigned i=0;
	  InitMediaType(my_media_types[i++], MEDIATYPE_Audio, MEDIASUBTYPE_PCM);
	  no_my_media_types = i;
	}
	else {
	  // Make sure my_media_types[5] is long enough!
	  unsigned i=0;
	  if (media & mediaYV12)   InitMediaType(my_media_types[i++], MEDIATYPE_Video, MEDIASUBTYPE_YV12);
	  if (media & mediaYUY2)   InitMediaType(my_media_types[i++], MEDIATYPE_Video, MEDIASUBTYPE_YUY2);
	  if (media & mediaARGB)   InitMediaType(my_media_types[i++], MEDIATYPE_Video, MEDIASUBTYPE_ARGB32);
	  if (media & mediaRGB32)  InitMediaType(my_media_types[i++], MEDIATYPE_Video, MEDIASUBTYPE_RGB32);
	  if (media & mediaRGB24)  InitMediaType(my_media_types[i++], MEDIATYPE_Video, MEDIASUBTYPE_RGB24);
	  no_my_media_types = i;
	  if (media == mediaNONE) media = mediaAUTO;
	}
  }

  GetSample::~GetSample() {
	if (am_media_type)
	  DeleteMediaType(am_media_type);
    am_media_type = 0;

    PulseEvent(evtDoneWithSample);
    CloseHandle(evtDoneWithSample);
    PulseEvent(evtNewSampleReady);
    CloseHandle(evtNewSampleReady);

	for (unsigned i=0; i<no_my_media_types; i++)
	  delete my_media_types[i];
  }


  const AM_MEDIA_TYPE *GetSample::GetMediaType(unsigned pos) {
	return my_media_types[(pos < no_my_media_types) ? pos : 0];
  }


  bool GetSample::WaitForStart(DWORD &timeout) {

    // Give the graph an opportunity to start before we return empty data

    if (state == State_Stopped) {
      _RPT1(0,"DSS WaitForStart() state == State_Stopped (%s)\n", streamName); // Opps should never happen!
      return false;
    }

    if (graphTimeout)
      graphTimeout = (WaitForSingleObject(evtNewSampleReady, timeout) == WAIT_TIMEOUT);
    if (graphTimeout) {
      _RPT0(0,"DSS ** TIMEOUT ** waiting for Graph to start!\n");
	  timeout = 55; // 1 windows tick
    }
	return graphTimeout;
  }


  PVideoFrame GetSample::GetCurrentFrame(IScriptEnvironment* env, int n, bool _TrapTimeouts, DWORD &timeout) {

    if (WaitForStart(timeout))
	  if (_TrapTimeouts)
	    env->ThrowError("DirectShowSource : Timeout waiting for video.");

    PVideoFrame pvf = env->NewVideoFrame(vi);;

    if (av_buffer) {

      // Put any knowledge of video packing and alignment here and
      // keep it independant of any AviSynth packing and alignment.

      PBYTE buf = av_buffer;

      if (!vi.IsPlanar()) { // Packed formats have rows 32bit aligned

	    const int rowsize = pvf->GetRowSize();
        env->BitBlt(pvf->GetWritePtr(), pvf->GetPitch(), buf, (rowsize+3)&~3, rowsize, pvf->GetHeight());
      }
      else {

	    const int rowsize = pvf->GetRowSize(PLANAR_Y);
		const int height  = pvf->GetHeight(PLANAR_Y);

        // All planar formats have Y rows 32bit aligned
        env->BitBlt(pvf->GetWritePtr(PLANAR_Y), pvf->GetPitch(PLANAR_Y), buf, (rowsize+3)&~3, rowsize, height);

	    const int UVrowsize = pvf->GetRowSize(PLANAR_V);
		const int UVheight  = pvf->GetHeight(PLANAR_V);

        // YV12 format has UV rows 16bit aligned with
        // V plane first, after aligned end of Y plane
		buf += ((rowsize+3)&~3) * height;
        env->BitBlt(pvf->GetWritePtr(PLANAR_V), pvf->GetPitch(PLANAR_V), buf, (UVrowsize+1)&~1, UVrowsize, UVheight);

        // And U plane last, after aligned end of V plane
		buf += ((UVrowsize+1)&~1) * UVheight;
        env->BitBlt(pvf->GetWritePtr(PLANAR_U), pvf->GetPitch(PLANAR_U), buf, (UVrowsize+1)&~1, UVrowsize, UVheight);
      }
    }
    else {
      _RPT1(0,"DSS GetCurrentFrame() Returning ** BLANK ** frame %d!\n", n);

      // If the graph still hasn't started yet we won't have a current frame
      // so dummy up a grey frame so at least things won't be fatal.

      memset(pvf->GetWritePtr(), 128, pvf->GetPitch()*(pvf->GetHeight()+pvf->GetHeight(PLANAR_U)));
    }
    return pvf;
  }


  HRESULT GetSample::StartGraph() {
    _RPT1(0,"DSS StartGraph(%s) enter...\n", streamName);
    ResetEvent(evtDoneWithSample);  // Nuke any unused SetEvents
    ResetEvent(evtNewSampleReady);  // Nuke any unused SetEvents
    IMediaControl* mc;
    filter_graph->QueryInterface(&mc);
    flushing = end_of_stream = false;
    HRESULT hr = mc->Run();
	_RPT2(0,"DSS StartGraph(%s) mc->Run() = 0x%x\n", streamName, hr);
	if (hr == S_FALSE) {
	  // Damn! graph is stuffing around and has not started (yet?)
	  OAFilterState fs = State_Stopped;
	  hr = mc->GetState(5000, &fs); // Give it 5 seconds to sort itself out
	  _RPT3(0,"DSS StartGraph(%s) mc->GetState(&%d) = 0x%x\n", streamName, fs, hr);
	  if ((fs == State_Running) && ((hr == S_OK) || (hr == VFW_S_STATE_INTERMEDIATE)))
	    hr = S_OK; // It is good or still may become good
	  else if (SUCCEEDED(hr))
	    hr = E_FAIL;  // It's playing possum and about to lock up
//    else
//      it's totally screwed
	}
//	else
//	  hr == S_OK or some serious error like the infamous E_FAIL
    mc->Release();
	graphTimeout = true;
    _RPT2(0,"DSS StartGraph(%s) ... exit 0x%x\n", streamName, hr);
	return hr;
  }

  void GetSample::StopGraph() {
    _RPT1(0,"DSS StopGraph() indicating done with sample - state:%d\n",state);
    IMediaControl* mc;
    filter_graph->QueryInterface(&mc);
    SetEvent(evtDoneWithSample); // Free task if waiting
    mc->Stop();
    mc->Release();
    if (m_pPos)
      m_pPos->Release();
    m_pPos = 0;
  }

  void GetSample::PauseGraph() {
    _RPT0(0,"DSS PauseGraph()\n");
    IMediaControl* mc;
    filter_graph->QueryInterface(&mc);
    mc->Pause();
    mc->Release();
  }

  HRESULT GetSample::SeekTo(__int64 pos) {
    _RPT1(0,"DSS SeekTo() seeking to new position %I64d\n", pos);

    IMediaControl* mc;
    filter_graph->QueryInterface(&mc);
    mc->Stop();
    mc->Release();

    IMediaSeeking* ms;
    filter_graph->QueryInterface(&ms);

//  It looks like GetCapabilities() is a useless liar. So it seems the
//  best thing is to just try the seeks and test the result code
//
//  ms->GetCapabilities(&dwCaps);
    
    ms->SetTimeFormat(&TIME_FORMAT_MEDIA_TIME); // Setting can give E_NOTIMPL

    HRESULT hr;
    LONGLONG pCurrent = -1, pStop;
    GUID time_f;

    ms->GetTimeFormat(&time_f);                 // so check what it currently is
    if (time_f != TIME_FORMAT_MEDIA_TIME) {
      // Probably should implement code for all the time formats  :: FIXME
      _RPT0(0,"DSS Could not set time format to media time!\n");
    }

	if (SUCCEEDED(hr = ms->SetPositions(&pos, AM_SEEKING_AbsolutePositioning, NULL, AM_SEEKING_NoPositioning)))
		goto SeekExit;
	_RPT1(0,"DSS Absolute seek failed! 0x%x\n", hr); // and now trying relative seek

	if (FAILED(hr = ms->GetPositions(&pCurrent, &pStop)) || (pCurrent == -1)) {
	  _RPT1(0,"DSS GetPositions failed! 0x%x\n", hr);
	  pCurrent = GetSampleEndTime(); // Wing it from last the sample delived
	}

    pCurrent = pos - pCurrent;
	if (FAILED(hr = ms->SetPositions(&pCurrent, AM_SEEKING_RelativePositioning, NULL, AM_SEEKING_NoPositioning))) {
	  _RPT1(0,"DSS Relative seek failed! 0x%x\n", hr); }

SeekExit:
    ms->Release();

	hr = SUCCEEDED(hr) ? S_OK : S_FALSE;

    HRESULT hr1 = StartGraph();
	if (FAILED(hr1))
	  hr = hr1;  // pretty serious the Graph is not running

    return hr;
  }


  bool GetSample::NextSample(DWORD &timeout) {

    if (end_of_stream) {
      _RPT1(0,"DSS NextSample() end of stream (%s)\n", streamName);
      return false;
    }

    if (state == State_Stopped) {
      _RPT1(0,"DSS NextSample() state == State_Stopped (%s)\n", streamName); // Opps should never happen!
      return false;
    }

    // If the graph didn't start, check if it has yet. We absolutly have to keep in lock
    // step with the evtNewSampleReady and evtDoneWithSample synchronizaton objects. This
    // is a bit kludgy because when it happens blank frames or silence are returned but
    // it is better than returning totally corrupt data.

    if (WaitForStart(timeout)) return false;

    _RPT1(0,"DSS NextSample() indicating done with previous sample...(%s)\n", streamName);

    SetEvent(evtDoneWithSample);  // We indicate that Receive can run again. We have now finished using the frame.

	_RPT1(0,"DSS ...NextSample() waiting for new sample...(%s)\n", streamName);
    if (WaitForSingleObject(evtNewSampleReady, timeout) == WAIT_TIMEOUT) {
	  _RPT1(0,"DSS ...NextSample() TIMEOUT waiting for new sample (%s)\n", streamName);
	  timeout = 55;
	  graphTimeout = true;
	  return false;
	}

    _RPT1(0,"DSS ...NextSample() done waiting for new sample (%s)\n", streamName);
	return !end_of_stream;
  }

  // IUnknown

  ULONG __stdcall GetSample::AddRef() { 
    InterlockedIncrement(&refcnt); 
    _RPT1(0,"DSS GetSample::AddRef() -> %d\n", refcnt); 
    return refcnt; 
  }

  ULONG __stdcall GetSample::Release() { 
    InterlockedDecrement(&refcnt); 
    _RPT1(0,"DSS GetSample::Release() -> %d\n", refcnt); 
    return refcnt; 
  }

  HRESULT __stdcall GetSample::QueryInterface(REFIID iid, void** ppv) {
    if      (iid == IID_IUnknown)     *ppv = static_cast<IUnknown*>(static_cast<IBaseFilter*>(this));
    else if (iid == IID_IPersist)     *ppv = static_cast<IPersist*>(this);
    else if (iid == IID_IMediaFilter) *ppv = static_cast<IMediaFilter*>(this);
    else if (iid == IID_IBaseFilter)  *ppv = static_cast<IBaseFilter*>(this);
    else if (iid == IID_IPin)         *ppv = static_cast<IPin*>(this);
    else if (iid == IID_IMemInputPin) *ppv = static_cast<IMemInputPin*>(this);
    else if (iid == IID_IMediaSeeking || iid == IID_IMediaPosition) {
      if (!source_pin) {
        _RPT1(0,"DSS GetSample::QueryInterface(%s, ppv) ** E_NOINTERFACE **, No Source Pin!\n", PrintGUID(&iid));
        return E_NOINTERFACE;
      }
      if (m_pPos == NULL)  {
        // We have not created the CPosPassThru object yet. Do so now.
        HRESULT hr = S_OK;
        hr = CreatePosPassThru(NULL , FALSE, static_cast<IPin*>(this), &m_pPos);

        if (FAILED(hr))  {
          _RPT1(0,"DSS GetSample::QueryInterface(%s, ppv), Failed CreatePosPassThru!\n", PrintGUID(&iid)); 
          return hr;
        }
      }
      _RPT1(0,"DSS GetSample::QueryInterface(%s, ppv) -> m_pPos\n", PrintGUID(&iid)); 
      return m_pPos->QueryInterface(iid, ppv);
    }
	else {
      *ppv = 0;
      _RPT1(0,"DSS GetSample::QueryInterface(%s, ppv) ** E_NOINTERFACE **\n", PrintGUID(&iid));
      return E_NOINTERFACE;
    }
    AddRef();
    _RPT1(0,"DSS GetSample::QueryInterface(%s, ppv)\n", PrintGUID(&iid)); 
    return S_OK;
  }

  // IPersist

  HRESULT __stdcall GetSample::GetClassID(CLSID* pClassID) {
    _RPT0(0,"DSS GetSample::GetClassID() E_NOTIMPL\n");
    return E_NOTIMPL;
  }

  // IMediaFilter

  HRESULT __stdcall GetSample::Stop() {
    _RPT1(0,"DSS GetSample::Stop(), state was %d\n", state);
    state = State_Stopped;
    SetEvent(evtDoneWithSample);
    return S_OK;
  }

  HRESULT __stdcall GetSample::Pause() {
    _RPT1(0,"DSS GetSample::Pause(), state was %d\n", state);
    state = State_Paused;
    return S_OK;
  }

  HRESULT __stdcall GetSample::Run(REFERENCE_TIME tStart) {
    _RPT2(0,"DSS GetSample::Run(%I64d), state was %d\n", tStart, state);
    state = State_Running;
    return S_OK;
  }

  HRESULT __stdcall GetSample::GetState(DWORD dwMilliSecsTimeout, FILTER_STATE* State) {
    if (!State) {
      _RPT1(0,"DSS GetSample::GetState() ** E_POINTER **, state is %d\n", state);
      return E_POINTER;
    }
    _RPT1(0,"DSS GetSample::GetState(), state is %d\n", state);
    *State = state;
    return S_OK;
  }

  HRESULT __stdcall GetSample::SetSyncSource(IReferenceClock* pClock) {
    _RPT2(0,"DSS GetSample::SetSyncSource(0x%08x), was 0x%08x\n", pClock, pclock);
    pclock = pClock;
    return S_OK;
  }

  HRESULT __stdcall GetSample::GetSyncSource(IReferenceClock** ppClock) {
    if (!ppClock) {
      _RPT1(0,"DSS GetSample::GetSyncSource() ** E_POINTER **, is 0x%08x\n", pclock);
      return E_POINTER;
    }
    _RPT1(0,"DSS GetSample::GetSyncSource(), is 0x%08x\n", pclock);
    *ppClock = pclock;
    if (pclock) pclock->AddRef();
    return S_OK;
  }

  // IBaseFilter

  HRESULT __stdcall GetSample::EnumPins(IEnumPins** ppEnum) {
    if (!ppEnum) {
      _RPT0(0,"DSS GetSample::EnumPins() ** E_POINTER **\n");
      return E_POINTER;
    }
    _RPT0(0,"DSS GetSample::EnumPins()\n");
    *ppEnum = new GetSampleEnumPins(this);
    return *ppEnum ? S_OK : E_OUTOFMEMORY;
  }

  HRESULT __stdcall GetSample::FindPin(LPCWSTR Id, IPin** ppPin) { // See QueryID
    _RPT1(0,"DSS GetSample::FindPin(%ls, ppPin) E_NOTIMPL\n", Id);
    return E_NOTIMPL;
  }

  HRESULT __stdcall GetSample::QueryFilterInfo(FILTER_INFO* pInfo) {
    if (!pInfo) {
      _RPT0(0,"DSS GetSample::QueryFilterInfo() ** E_POINTER **\n");
      return E_POINTER;
    }
    _RPT0(0,"DSS GetSample::QueryFilterInfo()\n");
    lstrcpyW(pInfo->achName, L"GetSample");
    pInfo->pGraph = filter_graph;
    if (filter_graph) filter_graph->AddRef();
    return S_OK;
  }

  HRESULT __stdcall GetSample::JoinFilterGraph(IFilterGraph* pGraph, LPCWSTR pName) {
    _RPT2(0,"DSS GetSample::JoinFilterGraph(0x%08x, %ls)\n", pGraph, pName);
    filter_graph = pGraph;
    return S_OK;
  }

  HRESULT __stdcall GetSample::QueryVendorInfo(LPWSTR* pVendorInfo) {
    _RPT0(0,"DSS GetSample::QueryVendorInfo() E_NOTIMPL\n");
    return E_NOTIMPL;
  }

  // IPin

  HRESULT __stdcall GetSample::Connect(IPin* pReceivePin, const AM_MEDIA_TYPE* pmt) {
    _RPT0(0,"DSS GetSample::Connect() E_UNEXPECTED\n");
    return E_UNEXPECTED;
  }

  HRESULT __stdcall GetSample::ReceiveConnection(IPin* pConnector, const AM_MEDIA_TYPE* pmt) {
    if (!pConnector || !pmt) {
      _RPT0(0,"DSS GetSample::ReceiveConnection() ** E_POINTER **\n");
      return E_POINTER;
    }
    if (source_pin) {
      _RPT0(0,"DSS GetSample::ReceiveConnection() ** VFW_E_ALREADY_CONNECTED **\n");
      return VFW_E_ALREADY_CONNECTED;
    }
    if (state != State_Stopped) {
      _RPT0(0,"DSS GetSample::ReceiveConnection() ** VFW_E_NOT_STOPPED **\n");
      return VFW_E_NOT_STOPPED;
    }
    if (GetSample::QueryAccept(pmt) != S_OK) {
	  _RPT0(0,"DSS GetSample::ReceiveConnection() ** VFW_E_TYPE_NOT_ACCEPTED **\n");
	  return VFW_E_TYPE_NOT_ACCEPTED;
	}
    _RPT1(0,"DSS GetSample::ReceiveConnection(0x%08x, pmt)\n", pConnector);
    source_pin = pConnector;
	if (am_media_type)
	  DeleteMediaType(am_media_type);
	am_media_type = CreateMediaType(pmt);
    return S_OK;
  }

  HRESULT __stdcall GetSample::Disconnect() {
    if (state != State_Stopped) {
      _RPT0(0,"DSS GetSample::Disconnect() ** VFW_E_NOT_STOPPED **\n");
      return VFW_E_NOT_STOPPED;
    }
    if (!source_pin) {
      _RPT0(0,"DSS GetSample::Disconnect() ** S_FALSE **\n");
      return S_FALSE;
    }
    source_pin = 0;
	if (am_media_type)
	  DeleteMediaType(am_media_type);
    am_media_type = 0;
    _RPT0(0,"DSS GetSample::Disconnect()\n");
    return S_OK;
  }

  HRESULT __stdcall GetSample::ConnectedTo(IPin** ppPin) {
    if (!ppPin) {
      _RPT0(0,"DSS GetSample::ConnectedTo() ** E_POINTER **\n");
	  return E_POINTER;
	}
    *ppPin = source_pin;
    if (!source_pin) {
      _RPT0(0,"DSS GetSample::ConnectedTo() ** VFW_E_NOT_CONNECTED **\n");
	  return VFW_E_NOT_CONNECTED;
	}
	source_pin->AddRef();
	_RPT1(0,"DSS GetSample::ConnectedTo() is 0x%08x\n", source_pin);
    return S_OK;
  }

  HRESULT __stdcall GetSample::ConnectionMediaType(AM_MEDIA_TYPE* pmt) {
    if (!pmt) {
      _RPT0(0,"DSS GetSample::ConnectionMediaType() ** E_POINTER **\n");
	  return E_POINTER;
	}
    if (!source_pin || !am_media_type) {
      _RPT0(0,"DSS GetSample::ConnectionMediaType() ** VFW_E_NOT_CONNECTED **\n");
	  return VFW_E_NOT_CONNECTED;
	}
	FreeMediaType(*pmt);
	CopyMediaType(pmt, am_media_type);
    _RPT0(0,"DSS GetSample::ConnectionMediaType()\n");
	
    return S_OK;
  }

  HRESULT __stdcall GetSample::QueryPinInfo(PIN_INFO* pInfo) {
    if (!pInfo) {
      _RPT0(0,"DSS GetSample::QueryPinInfo() ** E_POINTER **\n");
	  return E_POINTER;
	}
    pInfo->pFilter = static_cast<IBaseFilter*>(this);
    AddRef();
    pInfo->dir = PINDIR_INPUT;
    lstrcpyW(pInfo->achName, L"GetSample");
	_RPT1(0,"DSS GetSample::QueryPinInfo() 0x08%x\n", this);
    return S_OK;
  }

  HRESULT __stdcall GetSample::QueryDirection(PIN_DIRECTION* pPinDir) {
    if (!pPinDir) {
      _RPT0(0,"DSS GetSample::QueryDirection() ** E_POINTER **\n");
	  return E_POINTER;
	}
    *pPinDir = PINDIR_INPUT;
	_RPT0(0,"DSS GetSample::QueryDirection()\n");
    return S_OK;
  }

  HRESULT __stdcall GetSample::QueryId(LPWSTR* Id) { // See FindPin
    _RPT0(0,"DSS GetSample::QueryId() E_NOTIMPL\n");
    return E_NOTIMPL;
  }

  HRESULT __stdcall GetSample::QueryAccept(const AM_MEDIA_TYPE* pmt) {
    if (!pmt) {
      _RPT0(0,"DSS GetSample::QueryAccept() ** E_POINTER **\n");
	  return E_POINTER;
	}

	if      (pmt->majortype == MEDIATYPE_Video) {
	  _RPT1(0,"DSS GetSample::QueryAccept(%s) MEDIATYPE_Video\n", streamName);
	  if (load_audio) return S_FALSE;
	}
	else if (pmt->majortype == MEDIATYPE_Audio) {
	  _RPT1(0,"DSS GetSample::QueryAccept(%s) MEDIATYPE_Audio\n", streamName);
	  if (load_video) return S_FALSE;
	}
	else {
	  _RPT2(0,"DSS GetSample::QueryAccept(%s) %s\n", streamName, PrintGUID(&pmt->majortype));
	  return S_FALSE;
	}

// Handle audio:

    if (pmt->majortype == MEDIATYPE_Audio) {
//      if (pmt->subtype != MEDIASUBTYPE_PCM  || pmt->subtype != MEDIASUBTYPE_IEEE_FLOAT )
      if (pmt->subtype != MEDIASUBTYPE_PCM ) {
        _RPT1(0, "Subtype rejected - %s\n", PrintGUID(&pmt->subtype));
        return S_FALSE;
      }

      WAVEFORMATEX* wex = (WAVEFORMATEX*)pmt->pbFormat;

      if ((wex->wFormatTag != WAVE_FORMAT_PCM) && (wex->wFormatTag != WAVE_FORMAT_EXTENSIBLE)) {
        _RPT1(0, "Not PCM after all??? - %s\n", PrintGUID(&pmt->formattype));
        return S_FALSE;
      }

      vi.nchannels = wex->nChannels;
      switch (wex->wBitsPerSample) {
        case 8:
          vi.sample_type = SAMPLE_INT8;
          break;
        case 16:
          vi.sample_type = SAMPLE_INT16;
          break;
        case 24:
          vi.sample_type = SAMPLE_INT24;
          break;
        case 32:
          vi.sample_type = SAMPLE_INT32;
          break;
        default:
        _RPT0(0, "*** Audio: Unsupported number of bits per \n");
        return S_FALSE;
      }

      if (wex->wFormatTag == WAVE_FORMAT_EXTENSIBLE) {
        if (wex->cbSize < 22) {
          _RPT0(0, "*** Audio: Extended wave format structure does not have the correct size!\n");
          return S_FALSE;
        }
        // Override settings with extended data (float or >2 ch).
        WAVEFORMATEXTENSIBLE* _wext =  (WAVEFORMATEXTENSIBLE*)pmt->pbFormat;
        WAVEFORMATEXTENSIBLE wext =  *_wext;
        
        if (wext.Samples.wValidBitsPerSample != wext.Format.wBitsPerSample) {  // FIXME:  Allow bit padding!
          _RPT2(0, "*** Audio: Cannot accept sound, if ValidBitsPerSample(%d) != BitsPerSample(%d)!\n",
		           wext.Samples.wValidBitsPerSample, wext.Format.wBitsPerSample);
          return S_FALSE;
        }

        if (wext.SubFormat == SUBTYPE_IEEE_AVSFLOAT) {  // We have float audio.
          vi.sample_type = SAMPLE_FLOAT;
        } else if (wext.SubFormat != SUBTYPE_IEEE_AVSPCM) {
          _RPT1(0, "*** Audio: Extended WAVE format must be float or PCM. %s\n", PrintGUID(&wext.SubFormat));
          return S_FALSE;
        }
      }

      vi.audio_samples_per_second = wex->nSamplesPerSec;

      _RPT3(0, "*** Audio Accepted!  - Channels:%d.  Samples/sec:%d.  Bits/sample:%d.\n",
            wex->nChannels, wex->nSamplesPerSec, wex->wBitsPerSample);      
      return S_OK;
    }

// Handle video:

	if (pmt->majortype == MEDIATYPE_Video) {
	  if        (pmt->subtype == MEDIASUBTYPE_YV12) {  
		if (!(media & mediaYV12)) {
		  _RPT0(0, "Subtype rejected - YV12\n");
		  return S_FALSE;
		}
		vi.pixel_type = VideoInfo::CS_YV12;

	  } else if (pmt->subtype == MEDIASUBTYPE_YUY2) {
		if (!(media & mediaYUY2)) {
		  _RPT0(0, "Subtype rejected - YUY2\n");
		  return S_FALSE;
		}
		vi.pixel_type = VideoInfo::CS_YUY2;

	  } else if (pmt->subtype == MEDIASUBTYPE_RGB24) {
		if (!(media & mediaRGB24)) {
		  _RPT0(0, "Subtype rejected - RGB24\n");
		  return S_FALSE;
		}
		vi.pixel_type = VideoInfo::CS_BGR24;

	  } else if (pmt->subtype == MEDIASUBTYPE_RGB32) {
		if (!(media & mediaRGB32)) {
		  _RPT0(0, "Subtype rejected - RGB32\n");
		  return S_FALSE;
		}
		vi.pixel_type = VideoInfo::CS_BGR32;

	  } else if (pmt->subtype == MEDIASUBTYPE_ARGB32) {
		if (!(media & mediaARGB)) {
		  _RPT0(0, "Subtype rejected - ARGB32\n");
		  return S_FALSE;
		}
		vi.pixel_type = VideoInfo::CS_BGR32;

	  } else {
		_RPT1(0, "Subtype rejected - %s\n", PrintGUID(&pmt->subtype));
		return S_FALSE;
	  }

	  BITMAPINFOHEADER* pbi;
	  unsigned _avg_time_per_frame;

	  if (pmt->formattype == FORMAT_VideoInfo) {
		VIDEOINFOHEADER* vih = (VIDEOINFOHEADER*)pmt->pbFormat;
		_avg_time_per_frame = unsigned(vih->AvgTimePerFrame);
		pbi = &vih->bmiHeader;
	  }
	  else if (pmt->formattype == FORMAT_VideoInfo2) {
		VIDEOINFOHEADER2* vih = (VIDEOINFOHEADER2*)pmt->pbFormat;
		_avg_time_per_frame = unsigned(vih->AvgTimePerFrame);
		pbi = &vih->bmiHeader;
  //      if (vih->dwInterlaceFlags & AMINTERLACE_1FieldPerSample) {
  //        vi.SetFieldBased(true);
  //      }
	  }
	  else {
		_RPT1(0, "format rejected - %s\n", PrintGUID(&pmt->formattype));
		return S_FALSE;
	  }

	  vi.width = pbi->biWidth;
	  vi.height = pbi->biHeight;

	  if (_avg_time_per_frame) {
		vi.SetFPS(10000000, _avg_time_per_frame);
	  } else {
		vi.fps_numerator = 1;
		vi.fps_denominator = 0;
	  }

	  _RPT4(0, "*** format accepted: %dx%d, pixel_type %x, avg_time_per_frame %dx100ns\n",
	      	vi.width, vi.height, vi.pixel_type, _avg_time_per_frame);
	  _RPT3(0, "*** bFixedSizeSamples=%d, bTemporalCompression=%d, lSampleSize=%d\n",
	        pmt->bFixedSizeSamples, pmt->bTemporalCompression, pmt->lSampleSize);

	  return S_OK;
	}
	return S_FALSE;
  }

  HRESULT __stdcall GetSample::EnumMediaTypes(IEnumMediaTypes** ppEnum) {
    if (!ppEnum) {
      _RPT0(0,"DSS GetSample::EnumMediaTypes() ** E_POINTER **\n");
      return E_POINTER;
    }
    if (no_my_media_types == 0) {
      _RPT0(0,"DSS GetSample::EnumMediaTypes() ** E_NOTIMPL **\n");
      return E_NOTIMPL;
    }
    _RPT0(0,"DSS GetSample::EnumMediaTypes()\n");
    *ppEnum = new GetSampleEnumMediaTypes(this, no_my_media_types);
    return *ppEnum ? S_OK : E_OUTOFMEMORY;
  }

  HRESULT __stdcall GetSample::QueryInternalConnections(IPin** apPin, ULONG* nPin) {
	*nPin = 0;
    _RPT0(0,"DSS GetSample::QueryInternalConnections()\n");
    return S_OK;
  }

  HRESULT __stdcall GetSample::EndOfStream() {
    _RPT0(0,"DSS GetSample::EndOfStream()\n");
    end_of_stream = true;
    if (filter_graph) {
      IMediaEventSink* mes;
      if (SUCCEEDED(filter_graph->QueryInterface(&mes))) {
        mes->Notify(EC_COMPLETE, (long)S_OK, (long)static_cast<IBaseFilter*>(this));
        mes->Release();
      }
    }
    _RPT0(0,"DSS EndOfStream() indicating new sample ready\n");
    SetEvent(evtNewSampleReady);
    return S_OK;
  }

  HRESULT __stdcall GetSample::BeginFlush() {
    _RPT0(0,"DSS GetSample::BeginFlush()\n");
    flushing = true;
    SetEvent(evtDoneWithSample);
    return S_OK;
  }

  HRESULT __stdcall GetSample::EndFlush() {
    _RPT0(0,"DSS GetSample::EndFlush()\n");
    flushing = false;
    return S_OK;
  }

  HRESULT __stdcall GetSample::NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate) {
    _RPT4(0,"DSS GetSample::NewSegment(%I64d, %I64d, %f) (%s)\n", tStart, tStop, dRate, streamName);
	segment_start_time = tStart;
    sample_end_time = sample_start_time = 0;
    return S_OK;
  }

  // IMemInputPin

  HRESULT __stdcall GetSample::GetAllocator(IMemAllocator** ppAllocator) {
    _RPT0(0,"DSS GetSample::GetAllocator() VFW_E_NO_ALLOCATOR\n");
	return VFW_E_NO_ALLOCATOR;
  }

  HRESULT __stdcall GetSample::NotifyAllocator(IMemAllocator* pAllocator, BOOL bReadOnly) {
    _RPT0(0,"DSS GetSample::NotifyAllocator()\n");
	return S_OK;
  }

  HRESULT __stdcall GetSample::GetAllocatorRequirements(ALLOCATOR_PROPERTIES* pProps) {
    _RPT0(0,"DSS GetSample::GetAllocatorRequirements() E_NOTIMPL\n");
	return E_NOTIMPL;
  }

  HRESULT __stdcall GetSample::Receive(IMediaSample* pSamples) {
    if (state == State_Stopped) {
      _RPT1(0,"DSS discarding sample (State_Stopped) (%s)\n", streamName);
      return VFW_E_WRONG_STATE;
    }
    if (flushing) {
      _RPT1(0,"DSS discarding sample (flushing) (%s)\n", streamName);
      return S_FALSE;
    }
    if (S_OK == pSamples->IsPreroll()) {
      _RPT1(0,"DSS discarding sample (preroll) (%s)\n", streamName);
      return S_OK;
    }

	_RPT1(0,"DSS ...Receive() running. (%s)\n", streamName);

    pSamples->GetPointer(&av_buffer);
	int deltaT = avg_time_per_frame;
    if (load_audio) {  // audio
      a_sample_bytes = pSamples->GetActualDataLength();
	  deltaT = MulDiv(a_sample_bytes, 10000000, vi.BytesPerAudioSample()*vi.SamplesPerSecond());
      _RPT1(0,"DSS Receive: Got %d bytes of audio data.\n",a_sample_bytes);
    }

    HRESULT result = pSamples->GetTime(&sample_start_time, &sample_end_time);
    if (result == VFW_S_NO_STOP_TIME) {
      _RPT0(0,"DSS VFW_S_NO_STOP_TIME!\n");
	  sample_end_time = sample_start_time + deltaT; // wing it
    }
	else if (FAILED(result)) {
      _RPT0(0,"DSS GetTime failed!\n");
	  sample_start_time = sample_end_time;
	  sample_end_time += deltaT; // wing it
    }
	_RPT3(0,"DSS %I64d - %I64d (%d)\n", sample_start_time, sample_end_time,
                                          DWORD(sample_end_time - sample_start_time));
    HRESULT wait_result;
    SetEvent(evtNewSampleReady);  // New sample is finished - wait releasing it
	                              // until it has been fetched (DoneWithSample).
    do {
	  _RPT1(0,"DSS ...Receive() waiting for DoneWithSample. (%s)\n", streamName);
      wait_result = WaitForSingleObject(evtDoneWithSample, 15000);
    } while ((wait_result == WAIT_TIMEOUT) && (state != State_Stopped));

    av_buffer = 0;
    a_sample_bytes = 0;

	_RPT1(0,"DSS Receive() - returning. (%s)\n", streamName);

    return S_OK;
  }

  HRESULT __stdcall GetSample::ReceiveMultiple(IMediaSample** ppSamples, long nSamples, long* nSamplesProcessed) {
    _RPT0(0,"DSS GetSample::ReceiveMultiple()\n");
    for (int i=0; i<nSamples; ++i) {
      HRESULT hr = Receive(ppSamples[i]);
      if (FAILED(hr)) {
        *nSamplesProcessed = i;
        return hr;
      }
    }
    *nSamplesProcessed = nSamples;
    return S_OK;
  }

  HRESULT __stdcall GetSample::ReceiveCanBlock() {
    _RPT0(0,"DSS GetSample::ReceiveCanBlock()\n");
	return S_OK;
  }



/***********************************************
 *             GetSampleEnumPins               *
 ***********************************************/



GetSampleEnumPins::GetSampleEnumPins(GetSample* _parent, int _pos) : parent(_parent) { pos=_pos; refcnt = 1; }

HRESULT __stdcall GetSampleEnumPins::Next(ULONG cPins, IPin** ppPins, ULONG* pcFetched) {
  if (!ppPins || !pcFetched) return E_POINTER;
  int copy = *pcFetched = min(int(cPins), 1-pos);
  if (copy>0) {
    *ppPins = static_cast<IPin*>(parent);
    parent->AddRef();
  }
  pos += copy;
  return int(cPins) > copy ? S_FALSE : S_OK;
}


/***********************************************
 *          GetSampleEnumMediaTypes            *
 ***********************************************/



GetSampleEnumMediaTypes::GetSampleEnumMediaTypes(GetSample* _parent, unsigned _count, unsigned _pos)
 : parent(_parent) {
  pos = 0;
  count = _count;
  refcnt = 1;
}

HRESULT __stdcall GetSampleEnumMediaTypes::Next(ULONG cMediaTypes, AM_MEDIA_TYPE** ppMediaTypes, ULONG* pcFetched) {
  if (!ppMediaTypes) {
	_RPT0(0,"DSS GetSampleEnumMediaTypes::Next(ppMediaTypes) E_POINTER\n");
	return E_POINTER;
  }
  if (!pcFetched && (cMediaTypes != 1)) {
	_RPT0(0,"DSS GetSampleEnumMediaTypes::Next(pcFetched) E_POINTER\n");
	return E_POINTER;
  }
  _RPT2(0,"DSS GetSampleEnumMediaTypes::Next(%u) pos=%u\n", cMediaTypes, pos);
  unsigned copy = min(cMediaTypes, count-pos);
  if (pcFetched) *pcFetched = copy;
  while (copy-->0) {
    *ppMediaTypes++ = CreateMediaType(parent->GetMediaType(pos++)) ;
  }
  return (pos >= count) ? S_FALSE : S_OK;
}

HRESULT __stdcall GetSampleEnumMediaTypes::Skip(ULONG cMediaTypes) {
  _RPT2(0,"DSS GetSampleEnumMediaTypes::Skip(%u) pos=%u\n", cMediaTypes, pos);
  pos += cMediaTypes;
  if (pos >= count) {
	pos = count;
	return S_FALSE;
  }
  return S_OK;
}

HRESULT __stdcall GetSampleEnumMediaTypes::Reset() {
  pos=0;
  return S_OK;
}

HRESULT __stdcall GetSampleEnumMediaTypes::Clone(IEnumMediaTypes** ppEnum) {
  if (!ppEnum) {
	_RPT0(0,"DSS GetSampleEnumMediaTypes::Clone() E_POINTER\n");
	return E_POINTER;
  }
  _RPT0(0,"DSS GetSampleEnumMediaTypes::Clone()\n");
  *ppEnum = new GetSampleEnumMediaTypes(parent, count, pos);
  return *ppEnum ? S_OK : E_OUTOFMEMORY;
}


/***********************************************
 *    DirectShowSource Helper Functions.       *
 ***********************************************/



static bool HasNoConnectedOutputPins(IBaseFilter* bf) {
  IEnumPins* ep;
  if (FAILED(bf->EnumPins(&ep)))
    return true;
  ULONG fetched=1;
  IPin* pin;
  while (S_OK == ep->Next(1, &pin, &fetched)) {
    PIN_DIRECTION dir;
    pin->QueryDirection(&dir);
    if (dir == PINDIR_OUTPUT) {
      IPin* other;
      pin->ConnectedTo(&other);
      if (other) {
        other->Release();
        pin->Release();
        ep->Release();
        return false;
      }
    }
    pin->Release();
  }
  ep->Release();
  return true;
}



static void DisconnectAllPinsAndRemoveFilter(IGraphBuilder* gb, IBaseFilter* bf) {
  IEnumPins* ep;
  if (SUCCEEDED(bf->EnumPins(&ep))) {
    ULONG fetched=1;
    IPin* pin;
    while (S_OK == ep->Next(1, &pin, &fetched)) {
      IPin* other;
      pin->ConnectedTo(&other);
      if (other) {
        gb->Disconnect(other);
        gb->Disconnect(pin);
        other->Release();
      }
      pin->Release();
    }
    ep->Release();
  }
  gb->RemoveFilter(bf);
}


static void RemoveUselessFilters(IGraphBuilder* gb, IBaseFilter* not_this_one, IBaseFilter* nor_this_one) {
  IEnumFilters* ef;
  if (FAILED(gb->EnumFilters(&ef)))
    return;
  ULONG fetched=1;
  IBaseFilter* bf;
  while (S_OK == ef->Next(1, &bf, &fetched)) {
    if (bf != not_this_one && bf != nor_this_one) {
      if (HasNoConnectedOutputPins(bf)) {
        DisconnectAllPinsAndRemoveFilter(gb, bf);
        ef->Reset();
      }
    }
    bf->Release();
  }
  ef->Release();
}

static HRESULT AttemptConnectFilters(IGraphBuilder* gb, IBaseFilter* connect_filter) {
  IEnumFilters* ef;

  if (FAILED(gb->EnumFilters(&ef)))
    return E_UNEXPECTED;

  HRESULT hr;
  ULONG fetched=1;
  IBaseFilter* bf;
  IEnumPins* ep_conn;

  connect_filter->EnumPins(&ep_conn);
  IPin* p_conn;

  if (FAILED(ep_conn->Next(1, &p_conn, &fetched))) 
    return E_UNEXPECTED;

  while (S_OK ==(ef->Next(1, &bf, &fetched))) {
    if (bf != connect_filter) {
      IEnumPins* ep;

      bf->EnumPins(&ep);
      IPin* pPin;
      while (S_OK == (ep->Next(1, &pPin, &fetched)))  {

        PIN_DIRECTION PinDirThis;
        pPin->QueryDirection(&PinDirThis);

        if (PinDirThis == PINDIR_OUTPUT) {
          hr = gb->ConnectDirect(pPin, p_conn, NULL);
          if (SUCCEEDED(hr)) {
            pPin->Release();
            ep->Release();
            bf->Release();
            ep_conn->Release();
            ef->Release();
            return S_OK;
          }
        }
      }
      pPin->Release();
      ep->Release();
    }
    bf->Release();
  }
  ep_conn->Release();
  ef->Release();
  return S_OK;
}

static void SetMicrosoftDVtoFullResolution(IGraphBuilder* gb) {
  // Microsoft's DV codec defaults to half-resolution, to everyone's
  // great annoyance.  This will set it to full res if possible.
  // Note that IIPDVDec is not declared in older versions of
  // strmif.h; you may need the Win2000 platform SDK.
  IEnumFilters* ef;
  if (FAILED(gb->EnumFilters(&ef)))
    return;
  ULONG fetched=1;
  IBaseFilter* bf;
  while (S_OK == ef->Next(1, &bf, &fetched)) {
    IIPDVDec* pDVDec;
    if (SUCCEEDED(bf->QueryInterface(&pDVDec))) {
      pDVDec->put_IPDisplay(DVRESOLUTION_FULL); // DVDECODERRESOLUTION_720x480);   // yes, this includes 720x576
      pDVDec->Release();
    }
    bf->Release();
  }
  ef->Release();
  
}

// The following constant is from "wmcodecconst.h" in the 
// "Windows Media Audio and Video Codec Interfaces download package"
// available for download from MSDN.
static const WCHAR *g_wszWMVCDecoderDeinterlacing = L"_DECODERDEINTERLACING";

static void DisableDeinterlacing(IFilterGraph *pGraph)
{
    IEnumFilters *pEnum = NULL;
    IBaseFilter *pFilter;
    ULONG cFetched;
 
    HRESULT hr = pGraph->EnumFilters(&pEnum);
    if (FAILED(hr))
        return;

    while(pEnum->Next(1, &pFilter, &cFetched) == S_OK) {
        FILTER_INFO FilterInfo;
        hr = pFilter->QueryFilterInfo(&FilterInfo);
        if (FAILED(hr))
            continue;  // Maybe the next one will work.

        if (wcscmp(FilterInfo.achName, L"WMVideo Decoder DMO") == 0) {
            IPropertyBag *pPropertyBag = NULL;
            hr = pFilter->QueryInterface(IID_IPropertyBag, (void**)&pPropertyBag);
            if(SUCCEEDED(hr)) {
            VARIANT myVar;
            VariantInit(&myVar);
            // Disable decoder deinterlacing
            myVar.vt   = VT_BOOL;
            myVar.lVal = FALSE;
            pPropertyBag->Write(g_wszWMVCDecoderDeinterlacing, &myVar);
            }
        }

        // The FILTER_INFO structure holds a pointer to the Filter Graph
        // Manager, with a reference count that must be released.
        if (FilterInfo.pGraph != NULL)
            FilterInfo.pGraph->Release();
        pFilter->Release();
    }

    pEnum->Release();
}


/************************************************
 *               DirectShowSource               *
 ***********************************************/


DirectShowSource::DirectShowSource(const char* filename, int _avg_time_per_frame, int _seekmode, bool _enable_audio,
                                   bool _enable_video, bool _convert_fps, unsigned _media, int _timeout, IScriptEnvironment* env)
  : get_sample(_enable_audio, _enable_video, _media), seekmode(_seekmode), convert_fps(_convert_fps),
    gb(NULL), TrapTimeouts(_timeout < 0), WaitTimeout(abs(_timeout)) {

  IMediaFilter*  mf = 0;
  IMediaSeeking* ms = 0;

  try {
    CheckHresult(env, CoCreateInstance(CLSID_FilterGraphNoThread, 0, CLSCTX_INPROC_SERVER, IID_IGraphBuilder, (void**)&gb),
                 "couldn't create filter graph");

    WCHAR filenameW[MAX_PATH];
    MultiByteToWideChar(CP_ACP, 0, filename, -1, filenameW, MAX_PATH);

    CheckHresult(env, gb->AddFilter(static_cast<IBaseFilter*>(&get_sample), L"GetSample"), "couldn't add GetSample filter");

    bool load_grf = !strcmpi(filename+strlen(filename)-3,"grf");  // Detect ".GRF" extension and load as graph if so.

    if (load_grf) {
      CheckHresult(env, LoadGraphFile(gb, filenameW),"Couldn't open GRF file.",filename);
      // Try connecting to any open pins.
      AttemptConnectFilters(gb, &get_sample);
    } else {
      CheckHresult(env, gb->RenderFile(filenameW, NULL), "couldn't open file ", filename);
    }

    if (!get_sample.IsConnected()) {
      env->ThrowError("DirectShowSource: the filter graph manager won't talk to me");
    }

    RemoveUselessFilters(gb, &get_sample, &get_sample);

    SetMicrosoftDVtoFullResolution(gb);
    DisableDeinterlacing(gb);

    // Prevent the graph from trying to run in "real time"
    // ... Disabled because it breaks ASF.  Now I know why
    // Avery swears so much.
    CheckHresult(env, gb->QueryInterface(&mf), "couldn't get IMediaFilter interface");
    CheckHresult(env, mf->SetSyncSource(NULL), "couldn't set null sync source");
    SAFE_RELEASE(mf);

    CheckHresult(env, gb->QueryInterface(&ms), "couldn't get IMediaSeeking interface");

    vi = get_sample.GetVideoInfo();

    if (vi.HasVideo()) {
      bool fc_failed = true, mt_failed = true;
      __int64 frame_count = 0, duration = 0;
      GUID time_fmt = {0, 0, 0, {0, 0, 0, 0, 0, 0, 0, 0}};

      // SetTimeFormat can give E_NOTIMPL
      ms->SetTimeFormat(&TIME_FORMAT_MEDIA_TIME);
      // so check what it currently is
      ms->GetTimeFormat(&time_fmt);

      if (time_fmt == TIME_FORMAT_MEDIA_TIME)
        mt_failed = (FAILED(ms->GetDuration(&duration)) || duration == 0);

      // run in frame mode if we can set time format to frame
      frame_units = SUCCEEDED(ms->SetTimeFormat(&TIME_FORMAT_FRAME));

      if (frame_units)
        fc_failed = (FAILED(ms->GetDuration(&frame_count)) || frame_count == 0);

	  _RPT2(0,"DSS New Video: duration %I64d, frame_count %I64d.\n", duration, frame_count);

      if (convert_fps || fc_failed) frame_units = false;

      if (mt_failed && !frame_units) {
        env->ThrowError("DirectShowSource: unable to determine the duration of the video.");
      }

      if (_avg_time_per_frame) { // User specified FPS
        get_sample.avg_time_per_frame = _avg_time_per_frame;
        vi.SetFPS(10000000, _avg_time_per_frame);
        vi.num_frames = int(frame_units ? frame_count : duration / _avg_time_per_frame);
      }
      else {
        // this is exact (no rounding needed) because of the way the fps is set in GetSample
        get_sample.avg_time_per_frame = 10000000 / vi.fps_numerator * vi.fps_denominator;
        if (get_sample.avg_time_per_frame != 0) {
          vi.num_frames = int(frame_units ? frame_count : duration / get_sample.avg_time_per_frame);
        }
        else {
          // Try duration divided by frame count
          if (fc_failed || mt_failed) {
            env->ThrowError("DirectShowSource: I can't determine the frame rate\n"
                            "of the video, you must use the \"fps\" parameter."); // Note must match message below
          }
          else {
            get_sample.avg_time_per_frame = int((duration + (frame_count>>1)) / frame_count);
            vi.num_frames = int(frame_count);

            unsigned __int64 numerator   = 10000000 * frame_count;
            unsigned __int64 denominator = duration;

            unsigned __int64 x=numerator, y=denominator;
            while (y) {   // find gcd
              unsigned __int64 t = x%y; x = y; y = t;
            }
            numerator   /= x; // normalize
            denominator /= x;

            unsigned __int64 temp = numerator | denominator; // Just looking for top bit
            unsigned u = 0;
            while (temp & 0xffffffff80000000) {
              temp = Int64ShrlMod32(temp, 1);
              u++;
            }
            if (u) { // Scale to fit
              const unsigned round = 1 << (u-1);
              vi.SetFPS( (unsigned)Int64ShrlMod32(numerator   + round, u),
                         (unsigned)Int64ShrlMod32(denominator + round, u) );
            }
            else {
              vi.fps_numerator   = (unsigned)numerator;
              vi.fps_denominator = (unsigned)denominator;
            }
          }
        }
      }
    }

    if (vi.HasAudio()) {
      GUID time_fmt = {0, 0, 0, {0, 0, 0, 0, 0, 0, 0, 0}};
      __int64 audio_dur = 0;

      // SetTimeFormat can give E_NOTIMPL
      ms->SetTimeFormat(&TIME_FORMAT_MEDIA_TIME);
      // so check what it currently is
      ms->GetTimeFormat(&time_fmt);

      if ( (time_fmt != TIME_FORMAT_MEDIA_TIME)
        || FAILED(ms->GetDuration(&audio_dur))
        || (audio_dur == 0) ) {
          env->ThrowError("DirectShowSource: unable to determine the duration of the audio.");
      }

	  _RPT1(0,"DSS New Audio: audio_dur %I64d.\n", audio_dur);

      vi.num_audio_samples = (audio_dur * vi.audio_samples_per_second + 5000000) / 10000000;
    }
    SAFE_RELEASE(ms);

	CheckHresult(env, get_sample.StartGraph(), "DirectShowSource : Graph refused to run.");

	if (!TrapTimeouts) {
	  DWORD timeout = max(5000, min(300000, WaitTimeout));   // 5 seconds to 5 minutes
	  if (get_sample.WaitForStart(timeout)) // If returning grey frames, trap on init!
	    env->ThrowError("DirectShowSource : Timeout waiting for graph to start.");
	}

    cur_frame = 0;
    audio_bytes_read = 0;
    next_sample = 0;
  }
  catch (...) {
    SAFE_RELEASE(mf);
    SAFE_RELEASE(ms);
    cleanUp();
    throw;
  }
}


  DirectShowSource::~DirectShowSource() {
    cleanUp();
  }

  void DirectShowSource::cleanUp() {
    if (gb) {
      IMediaControl* mc;
      if (SUCCEEDED(gb->QueryInterface(&mc))) {
        OAFilterState st;
        mc->GetState(1000, &st);
        if (st != State_Stopped) mc->Stop();
        mc->Release();
      }
      get_sample.StopGraph();
      SAFE_RELEASE(gb);
    }
  }

  PVideoFrame __stdcall DirectShowSource::GetFrame(int n, IScriptEnvironment* env) {
	DWORD timeout = WaitTimeout;
    n = max(min(n, vi.num_frames-1), 0); 

    // Ask for the frame whose [start_time ->T<- end_time] spans sample_time
    const __int64 sample_time = Int32x32To64(n, get_sample.avg_time_per_frame) + (get_sample.avg_time_per_frame>>1);

	if ( (seekmode == 0 && n >= cur_frame) || (seekmode == 2) || (n >= cur_frame && n <= cur_frame+10) ) {
	  // seekzero==true+forwards or seek==false or a short hop forwards
      if (convert_fps) {
        // automatic fps conversion: trust only sample time
		while (get_sample.GetSampleEndTime() <= sample_time) {
		  if(!get_sample.NextSample(timeout)) break;
		}
		cur_frame = n;
      }
      else {
        while (cur_frame < n) {
          if (!get_sample.NextSample(timeout)) break;
          cur_frame++;
        }
      }
    }
	else {
	  HRESULT hr;
	  if (seekmode == 0) {
		// Seekzero=true and stepping back
		hr = get_sample.SeekTo(0);
		if (hr == S_OK) hr = S_FALSE;
	  }
	  else {
		// Seek=true and stepping back or a long hop forwards
		hr = get_sample.SeekTo(sample_time);
	  }

	  if (hr == S_OK) {
		// seek ok!
		cur_frame = n;
	  }
	  else if (hr == S_FALSE) {
		// seekzero or seek failed!
		if (!get_sample.WaitForStart(timeout)) {
		  // We have stopped and started the graph many unseekable streams
		  // reset to 0, others don't move. Try to get our position
		  cur_frame = int(get_sample.GetSampleStartTime() / get_sample.avg_time_per_frame);
		  if (frame_units) {
			while (cur_frame < n) {
			  if (!get_sample.NextSample(timeout)) break;
			  cur_frame++;
			}
		  }
		  else {
			while (get_sample.GetSampleEndTime() <= sample_time) {
			  if(!get_sample.NextSample(timeout)) break;
			}
			cur_frame = n;
		  }
		}
	  }
	  else {
		env->ThrowError("DirectShowSource : The video Graph failed to restart after seeking. Status = 0x%x", hr);
	  }
	}
    return get_sample.GetCurrentFrame(env, n, TrapTimeouts, timeout);
  }


  void __stdcall DirectShowSource::GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env) {
	DWORD timeout = WaitTimeout;
    int bytes_filled = 0;
 
    if (next_sample != start) {  // We have been searching!  Skip until sync!

      _RPT2(0,"DSS GetAudio: Seeking %I64d previous %I64d samples.\n", start, next_sample);

      // Backup to begining of current buffer
      next_sample -= vi.AudioSamplesFromBytes(audio_bytes_read);
      audio_bytes_read = 0;

      const __int64 avail_samples = vi.AudioSamplesFromBytes(get_sample.a_sample_bytes);
      if ( ((seekmode != 2) && (start < next_sample))
		// Seek=true and Seekzero=true and stepping back
	    || ((seekmode == 1) && (start >= next_sample+avail_samples+50000))) {
		// Seek=true and a long hop forwards
        const __int64 seekTo = (seekmode == 0) ? 0 : (start*10000000 + (vi.audio_samples_per_second>>1)) / vi.audio_samples_per_second;
        _RPT1(0,"DSS GetAudio: SeekTo %I64d media time.\n", seekTo);

		HRESULT hr = get_sample.SeekTo(seekTo);

		if (seekmode == 0 && hr == S_OK) hr = S_FALSE;

		if (hr == S_OK) {
          // Seek succeeded!
          next_sample = start;
          audio_bytes_read = 0;
        } 
		else if (hr == S_FALSE) {
		  // seek failed!
		  if (!get_sample.WaitForStart(timeout)) {
			// We have stopped and started the graph many unseekable streams
			// reset to 0, others don't move. Try to get our position
			next_sample = (get_sample.GetSampleStartTime() * vi.audio_samples_per_second + 5000000) / 10000000;
		  }
		}
		else {
		  env->ThrowError("DirectShowSource : The audio Graph failed to restart after seeking. Status = 0x%x", hr);
		}
      } 

      if (start < next_sample) { // We are behind sync - pad with 0
        const int fill_nsamples  = (int)min(next_sample - start, count);
        _RPT1(0,"DSS GetAudio: Padding %d samples.\n", fill_nsamples);

        // We cannot seek.
        if (vi.sample_type == SAMPLE_FLOAT) {
          float* samps = (float*)buf;
          for (int i = 0; i < fill_nsamples; i++)
            samps[i] = 0.0f;
        }
		else {
          memset(buf,0, (unsigned int)vi.BytesFromAudioSamples(fill_nsamples));
        }

        if (fill_nsamples == count)  // Buffer is filled - return
          return;
        start += fill_nsamples;
        count -= fill_nsamples;
        bytes_filled += (int)vi.BytesFromAudioSamples(fill_nsamples);
      }

      if (start > next_sample) {  // Skip forward (decode)
        // Should we search?
        int skip_left = (int)vi.BytesFromAudioSamples(start - next_sample);
        _RPT1(0,"DSS GetAudio: Skipping %d bytes.\n", skip_left);

		if (get_sample.WaitForStart(timeout))
		  if (TrapTimeouts)
			env->ThrowError("DirectShowSource : Timeout waiting for audio.");

        while (skip_left > 0) {
          if (get_sample.a_sample_bytes-audio_bytes_read >= skip_left) {
            audio_bytes_read += skip_left;
            break;
          }
          skip_left -= get_sample.a_sample_bytes-audio_bytes_read;
          audio_bytes_read = get_sample.a_sample_bytes;
          
          if (get_sample.NextSample(timeout))
            audio_bytes_read = 0;
          else
             break;  // EndOfStream? Timeout?
        } // end while
        next_sample = start;
      }
    }

    BYTE* samples = (BYTE*)buf;
    int bytes_left = (int)vi.BytesFromAudioSamples(count);
    _RPT2(0,"DSS GetAudio: Reading %I64d samples, %d bytes.\n", count, bytes_left);

    if (get_sample.WaitForStart(timeout))
	  if (TrapTimeouts)
	    env->ThrowError("DirectShowSource : Timeout waiting for audio.");
    while (bytes_left) {
      // Can we read from the Directshow filters buffer?
      if (get_sample.a_sample_bytes - audio_bytes_read > 0) { // Copy as many bytes as needed.

        // This many bytes can be safely read.
        const int available_bytes = min(bytes_left, get_sample.a_sample_bytes - audio_bytes_read);
        _RPT2(0,"DSS GetAudio: Memcpy %d offset, %d bytes.\n", bytes_filled, available_bytes);

        memcpy(&samples[bytes_filled], &get_sample.av_buffer[audio_bytes_read], available_bytes);

        bytes_left -= available_bytes;
        bytes_filled += available_bytes;
        audio_bytes_read += available_bytes;

      }
      else { // Read more samples
        if (get_sample.NextSample(timeout)) {
          audio_bytes_read = 0;
        }
		else { // Pad with 0
		  if (TrapTimeouts)
			if (get_sample.WaitForStart(timeout))
			  env->ThrowError("DirectShowSource : Timeout waiting for audio.");

          _RPT2(0,"DSS GetAudio: Memset %d offset, %d bytes.\n", bytes_filled, bytes_left);
          if (vi.sample_type == SAMPLE_FLOAT) {
            float* samps = (float*)((int)(&samples[bytes_filled]) & ~3); // Aligned just to be sure
            const int samples_left = (bytes_left+sizeof(float)-1)/sizeof(float);
            for (int i = 0; i < samples_left; i++)
              samps[i] = 0.0f;
          } else {
            memset(&samples[bytes_filled],0,bytes_left);
          }
          bytes_left = 0;
		  break;
        }
      }
    }
    next_sample +=count;
  }



void DirectShowSource::CheckHresult(IScriptEnvironment* env, HRESULT hr, const char* msg, const char* msg2) {
  if (SUCCEEDED(hr)) return;
//  char buf[1024] = {0};
//  if (!FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, hr, 0, buf, 1024, NULL))
  char buf[MAX_ERROR_TEXT_LEN] = {0};
  if (!AMGetErrorText(hr, buf, MAX_ERROR_TEXT_LEN))
    wsprintf(buf, "error code 0x%x", hr);
  env->ThrowError("DirectShowSource: %s%s:\n%s", msg, msg2, buf);
}

HRESULT DirectShowSource::LoadGraphFile(IGraphBuilder *pGraph, const WCHAR* wszName)
{
    IStorage *pStorage = 0;
    if (S_OK != StgIsStorageFile(wszName))
    {
        return E_FAIL;
    }
    HRESULT hr = StgOpenStorage(wszName, 0, 
        STGM_TRANSACTED | STGM_READ | STGM_SHARE_DENY_WRITE, 
        0, 0, &pStorage);
    if (FAILED(hr))
    {
        return hr;
    }
    IPersistStream *pPersistStream = 0;
    hr = pGraph->QueryInterface(IID_IPersistStream,
             reinterpret_cast<void**>(&pPersistStream));
    if (SUCCEEDED(hr))
    {
        IStream *pStream = 0;
        hr = pStorage->OpenStream(L"ActiveMovieGraph", 0, 
            STGM_READ | STGM_SHARE_EXCLUSIVE, 0, &pStream);
        if(SUCCEEDED(hr))
        {
            hr = pPersistStream->Load(pStream);
            pStream->Release();
        }
        pPersistStream->Release();
    }
    pStorage->Release();
    return hr;
}


/* As this is currently implemented we use two separate instance of DSS, one for video and
 * one for audio. This means we create two (2) filter graphs. An alternate implementation
 * would be to have a video GetSample::IPin object and a separate audio GetSample::IPin object
 * in the one filter graph. Possible problems with this idea could be related to independant 
 * positioning of the Video and Audio streams within the one filter graph. */


AVSValue __cdecl Create_DirectShowSource(AVSValue args, void*, IScriptEnvironment* env) {

  if (args[0].ArraySize() != 1)
    env->ThrowError("DirectShowSource: Only 1 filename currently supported!");

  const char* filename = args[0][0].AsString();
  const int _avg_time_per_frame = args[1].Defined() ? int(10000000 / args[1].AsFloat() + 0.5) : 0;
  
  const bool audio    = args[3].AsBool(true);
  const bool video    = args[4].AsBool(true);

  const bool seek     = args[2].AsBool(true);
  const bool seekzero = args[6].AsBool(false);
  const int  seekmode = seek ? (seekzero ? 0 : 1) : 2; // seek_zero, seek, no_search

  const int _timeout = args[7].AsInt(60000); // Default timeout = 1 minute

  unsigned _media = GetSample::mediaNONE;
  if (args[8].Defined()) {
    const char* pixel_type = args[8].AsString();
    if      (!lstrcmpi(pixel_type, "YUY2"))  { _media = GetSample::mediaYUY2; }
	else if (!lstrcmpi(pixel_type, "YV12"))  { _media = GetSample::mediaYV12; }
	else if (!lstrcmpi(pixel_type, "RGB24")) { _media = GetSample::mediaRGB24; }
	else if (!lstrcmpi(pixel_type, "RGB32")) { _media = GetSample::mediaRGB32 | GetSample::mediaARGB; }
	else if (!lstrcmpi(pixel_type, "ARGB"))  { _media = GetSample::mediaARGB; }
	else if (!lstrcmpi(pixel_type, "RGB"))   { _media = GetSample::mediaRGB; }
	else if (!lstrcmpi(pixel_type, "YUV"))   { _media = GetSample::mediaYUV; }
	else if (!lstrcmpi(pixel_type, "AUTO"))  { _media = GetSample::mediaAUTO; }
	else {
      env->ThrowError("DirectShowSource: pixel_type must be \"RGB24\", \"RGB32\", \"ARGB\", "
	                                       "\"YUY2\", \"YV12\", \"RGB\", \"YUV\" or \"AUTO\"");
    }
  }

  if (!(audio || video)) 
    env->ThrowError("DirectShowSource: Both video and audio was disabled!");

  if (!(audio && video)) { // Hey - simple!!
    if (audio) {
      return new DirectShowSource(filename, _avg_time_per_frame, seekmode, true , false,
	                              args[5].AsBool(false), _media, _timeout, env);
    } else {
      return new DirectShowSource(filename, _avg_time_per_frame, seekmode, false , true,
	                              args[5].AsBool(false), _media, _timeout, env);
    }
  }

  PClip DS_audio;
  PClip DS_video;

  bool audio_success = true;
  bool video_success = true;

  const char *a_e_msg;
  const char *v_e_msg;

  try {
    DS_audio = new DirectShowSource(filename, _avg_time_per_frame, seekmode, true , false,
	                                args[5].AsBool(false), _media, _timeout, env);
  } catch (AvisynthError e) {
    a_e_msg = e.msg;
    audio_success = false;
  }

  try {
    DS_video = new DirectShowSource(filename, _avg_time_per_frame, seekmode, false, true,
	                                args[5].AsBool(false), _media, _timeout, env);
  } catch (AvisynthError e) {
    if (!lstrcmpi(e.msg, "DirectShowSource: I can't determine the frame rate\n"
                         "of the video, you must use the \"fps\" parameter.") ) // Note must match message above
      env->ThrowError(e.msg);
    v_e_msg = e.msg;
    video_success = false;
  }

  if (!(audio_success || video_success)) {
    char err[1024] = "DirectShowSource: Could not open as video or audio.\r\n\r\nVideo returned:  \"";
    strcat(err,v_e_msg);
    strcat(err,"\"\r\n\r\nAudio returned:  \"");
    strcat(err,a_e_msg);
    strcat(err,"\"\r\n");
    env->ThrowError(err);
  }

  if (!audio_success)
    return DS_video;

  if (!video_success)
    return DS_audio;

  AVSValue inv_args[2] = { DS_video, DS_audio }; 
  PClip ds_all =  env->Invoke("AudioDub",AVSValue(inv_args,2)).AsClip();

  return ds_all;
}



extern "C" __declspec(dllexport) const char* __stdcall AvisynthPluginInit2(IScriptEnvironment* env)
{
    env->AddFunction("DirectShowSource",
// args				  0      1      2       3       4            5          6         7            8
	                 "s+[fps]f[seek]b[audio]b[video]b[convertfps]b[seekzero]b[timeout]i[pixel_type]s",
					 Create_DirectShowSource, 0);
    return "DirectShowSource";
}

