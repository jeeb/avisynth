// Camel - CPU Identifying Tool
// Copyright (C) 2003, Iain Chesworth
// 
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

#ifndef _CPUINFO_H_
#define _CPUINFO_H_

#ifdef _WIN32
	// Include Windows header files.
	#include <windows.h>
#endif // _WIN32

// Include generic C / C++ header files.
#include <memory.h>
#include <stdlib.h>
#include <stdio.h>

#define STORE_CLASSICAL_NAME(x)		sprintf (ChipID.ProcessorName, x)
#define STORE_TLBCACHE_INFO(x,y)	x = (x < y) ? y : x
#define VENDOR_STRING_LENGTH		(12 + 1)
#define CHIPNAME_STRING_LENGTH		(48 + 1)
#define SERIALNUMBER_STRING_LENGTH	(29 + 1)
#define TLBCACHE_INFO_UNITS			(15)
#define CLASSICAL_CPU_FREQ_LOOP		10000000
#define CLASSICAL_CPU_FREQ_LOOP_486	100000
#define RDTSC_INSTRUCTION			_asm _emit 0x0f _asm _emit 0x31
#define	CPUSPEED_I32TO64(x, y)		(((__int64) x << 32) + y)

#define CPUID_AWARE_COMPILER
#ifdef CPUID_AWARE_COMPILER
	#define CPUID_INSTRUCTION		cpuid
#else
	#define CPUID_INSTRUCTION		_asm _emit 0x0f _asm _emit 0xa2
#endif

// Standard feature flags.
#define HAS_FPU						0x000000000001
#define HAS_VME						0x000000000002
#define HAS_DE						0x000000000004
#define HAS_PSE						0x000000000008
#define HAS_TSC						0x000000000010
#define HAS_MSR						0x000000000020
#define HAS_PAE						0x000000000040
#define HAS_MCE						0x000000000080
#define HAS_CX8						0x000000000100
#define HAS_APIC					0x000000000200
#define HAS_SEP						0x000000000400
#define HAS_MTRR					0x000000000800
#define HAS_PGE						0x000000001000
#define HAS_MCA						0x000000002000
#define HAS_CMOV					0x000000004000
#define HAS_PAT						0x000000008000
#define HAS_PSE36					0x000000010000
#define HAS_PSN						0x000000020000
#define HAS_CLFSH					0x000000040000
#define HAS_DTS						0x000000080000
#define HAS_ACPI					0x000000100000
#define HAS_MMX						0x000000200000
#define HAS_FXSR					0x000000400000
#define HAS_SSE						0x000000800000
#define HAS_SSE2					0x000001000000
#define HAS_SS						0x000002000000
#define HAS_HTT						0x000004000000
#define HAS_TM						0x000008000000
#define HAS_IA64					0x000010000000

// Other feature flags.
#define HAS_SSEFP					0x000020000000
#define HAS_SSEMMX					0x000040000000
#define HAS_3DNOW					0x000080000000
#define HAS_3DNOWPLUS				0x000100000000
#define HAS_MMXPLUS					0x000200000000
#define MP_CAPABLE					0x000400000000
#define L1CACHE_FEATURE				0x000800000000
#define L2CACHE_FEATURE				0x001000000000
#define L3CACHE_FEATURE				0x002000000000
#define HAS_TSD						0x004000000000
#define HAS_FID						0x008000000000
#define HAS_VID						0x010000000000

// Newly added feature flags.
#define HAS_SBF						0x020000000000
#define HAS_TM2						0x040000000000
#define HAS_EST						0x080000000000
#define HAS_CID						0x100000000000
#define HAS_DAZ						0x200000000000

typedef	void (*DELAY_FUNC)(unsigned int uiMS);

class CPUSpeed {
public:
	CPUSpeed ();
	~CPUSpeed ();
		
	// Variables.
	int CPUSpeedInMHz;

	// Functions.
	__int64 __cdecl GetCyclesDifference (DELAY_FUNC, unsigned int);
		
private:
	// Functions.
	static void Delay (unsigned int);
	static void DelayOverhead (unsigned int);

protected:
	
};

class CPUInfo {
public:
	CPUInfo ();
	~CPUInfo ();

	char * GetVendorString ();
	char * GetVendorID ();
	int GetTypeID ();
	int GetFamilyID ();
	int GetModelID ();
	int GetBrandID ();
	int GetSteppingCode ();
	char * GetExtendedProcessorName ();
	char * GetProcessorSerialNumber ();
	int GetLogicalProcessorsPerPhysical ();
	int GetProcessorClockFrequency ();
	int GetProcessorAPICID ();
	int GetProcessorCacheXSize (__int64);
	int GetCacheLineSize ();
	bool DoesCPUSupportFeature (__int64);

	bool __cdecl DoesCPUSupportCPUID ();

private:
	typedef struct tagID {
		int Type;
		int Family;
		int Model;
		int Revision;
		int ExtendedFamily;
		int ExtendedModel;
		char ProcessorName[CHIPNAME_STRING_LENGTH];
		char Vendor[VENDOR_STRING_LENGTH];
		char SerialNumber[SERIALNUMBER_STRING_LENGTH];

		tagID () {
			Type = 0;
			Family = 0;
			Model = 0;
			Revision = 0;
			ExtendedFamily = 0;
			ExtendedModel = 0;
		}
	} ID;

	typedef struct tagCPUPowerManagement {
		bool bHasVID;
		bool bHasFID;
		bool bHasTSD;

		tagCPUPowerManagement () {
			bHasVID = false;
			bHasFID = false;
			bHasTSD = false;
		}
	} CPUPowerManagement;

	typedef struct tagCPUExtendedFeatures {
		bool bHas3DNow;
		bool bHas3DNowPlus;
		bool SupportsMP;
		bool bHasMMXPlus;
		bool bHasSSEMMX;
		bool bHasDAZ;
		int LogicalProcessorsPerPhysical;
		int APIC_ID;
		int BrandID;
		int CLFlush_Line_Size;
		CPUPowerManagement PowerManagement;

		tagCPUExtendedFeatures () {
			bHas3DNow = false;
			bHas3DNowPlus = false;
			SupportsMP = false;
			bHasMMXPlus = false;
			bHasSSEMMX = false;
			bHasDAZ = false;
			LogicalProcessorsPerPhysical = 0;
			APIC_ID = 0;
			BrandID = 0;
			CLFlush_Line_Size = 0;
		}
	} CPUExtendedFeatures;	
	
	typedef struct tagCPUFeatures_EDX {
		bool bHasFPU 	: 1;
		bool bHasVME 	: 1;
		bool bHasDE 	: 1;
		bool bHasPSE 	: 1;
		bool bHasTSC 	: 1;
		bool bHasMSR 	: 1;
		bool bHasPAE 	: 1;
		bool bHasMCE 	: 1;
		bool bHasCX8 	: 1;
		bool bHasAPIC 	: 1;
		bool 			: 1;
		bool bHasSEP	: 1;
		bool bHasMTRR	: 1;
		bool bHasPGE	: 1;
		bool bHasMCA	: 1;
		bool bHasCMOV	: 1;
		bool bHasPAT	: 1;
		bool bHasPSE36	: 1;
		bool bHasPSN	: 1;
		bool bHasCLFSH	: 1;
		bool 			: 1;
		bool bHasDTS	: 1;
		bool bHasACPI	: 1;
		bool bHasMMX	: 1;
		bool bHasFXSR	: 1;
		bool bHasSSE	: 1;
		bool bHasSSE2	: 1;
		bool bHasSS		: 1;
		bool bHasHTT	: 1;
		bool bHasTM		: 1;
		bool bIsIA64	: 1;
		bool bHasSBF	: 1;

		tagCPUFeatures_EDX () {
			bHasFPU = false;
			bHasVME = false;
			bHasDE = false;
			bHasPSE = false;
			bHasTSC = false;
			bHasMSR = false;
			bHasPAE = false;
			bHasMCE = false;
			bHasCX8 = false;
			bHasAPIC = false;
			bHasSEP = false;
			bHasMTRR = false;
			bHasPGE = false;
			bHasMCA = false;
			bHasCMOV = false;
			bHasPAT = false;
			bHasPSE36 = false;
			bHasPSN = false;
			bHasCLFSH = false;
			bHasDTS = false;
			bHasACPI = false;
			bHasMMX = false;
			bHasFXSR = false;
			bHasSSE = false;
			bHasSSE2 = false;
			bHasSS = false;
			bHasHTT = false;
			bHasTM = false;
			bIsIA64 = false;
		}
	} CPUFeatures_EDX;

	typedef struct tagCPUFeatures_ECX {
		long			: 7;
		bool bHasTM2	: 1;
		bool bHasEST	: 1;
		bool			: 1;
		bool bHasCID	: 1;
		long			: 21;

		tagCPUFeatures_ECX () {
			bHasTM2 = false;
			bHasEST = false;
			bHasCID = false;
		}
	} CPUFeatures_ECX;

	typedef struct tagCPUFeatures {
		int CPUSpeed;
		int L1CacheSize;
		int L2CacheSize;
		int L3CacheSize;
		bool bHasSSEFP;
		CPUFeatures_EDX Features;
		CPUFeatures_ECX Features2;
		CPUExtendedFeatures ExtendedFeatures;

		tagCPUFeatures () {
			CPUSpeed = 0;
			L1CacheSize = 0;
			L2CacheSize = 0;
			L3CacheSize = 0;
		}
	} CPUFeatures;
    
	enum Manufacturer {
		AMD, Intel, NSC, UMC, Cyrix, NexGen, IDT, Rise, Transmeta, UnknownManufacturer
	};

	// Functions.
	bool __cdecl RetrieveCPUFeatures ();
	bool __cdecl RetrieveCPUIdentity ();
	bool __cdecl RetrieveCPUCacheDetails ();
	bool __cdecl RetrieveClassicalCPUCacheDetails ();
	bool __cdecl RetrieveCPUClockSpeed ();
	bool __cdecl RetrieveClassicalCPUClockSpeed (int);
	bool __cdecl RetrieveCPUExtendedLevelSupport (int);
	bool __cdecl RetrieveExtendedCPUFeatures ();
	bool __cdecl RetrieveProcessorSerialNumber ();
	bool __cdecl RetrieveCPUPowerManagement ();
	bool __cdecl RetrieveClassicalCPUIdentity ();
	bool __cdecl RetrieveExtendedCPUIdentity ();
	
	// Variables.
	Manufacturer ChipManufacturer;
	CPUFeatures Features;
	CPUSpeed * Speed;
	ID ChipID;
	
protected:

};

#endif // _CPUINFO_H_