#ifndef _AVS_SCRIPTENVIRONMENT_H_INCLUDED
#define _AVS_SCRIPTENVIRONMENT_H_INCLUDED

#include <avisynth.h>

// Strictly for Avisynth core only.
// Neither host applications nor plugins should use
// these interfaces.
class InternalEnvironment : public IScriptEnvironment2 {
public:
    virtual __stdcall ~InternalEnvironment() {}

    virtual int __stdcall IncrImportDepth() = 0;
    virtual int __stdcall DecrImportDepth() = 0;
    virtual void __stdcall AdjustMemoryConsumption(size_t amount, bool minus) = 0;
    virtual MtMode __stdcall GetFilterMTMode(const AVSFunction* filter, bool* is_forced) const = 0; // If filter is "", gets the default MT mode
    virtual void __stdcall SetPrefetcher(Prefetcher *p) = 0;
	virtual void __stdcall ClipData(const IClip *clip) = 0;

}; // end class IScriptEnvironment2

#endif // _AVS_SCRIPTENVIRONMENT_H_INCLUDED