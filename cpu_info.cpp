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

#include "cpu_info.h"

// --------------------------------------------------------
//
//         Constructor Functions - CPUInfo Class
//
// --------------------------------------------------------

CPUInfo::CPUInfo ()
{
	// This is to de-allocate memory when necessary...
	Speed = NULL;

	// Check to see if this processor supports CPUID.
	if (DoesCPUSupportCPUID ()) {
		// Retrieve the CPU details.
		RetrieveCPUIdentity ();
		RetrieveCPUFeatures ();
		if (!RetrieveCPUClockSpeed ()) RetrieveClassicalCPUClockSpeed (this->ChipID.Family);

		// Attempt to retrieve cache information.
		if (!RetrieveCPUCacheDetails ()) RetrieveClassicalCPUCacheDetails ();

		// Retrieve the extended CPU details.
		if (!RetrieveExtendedCPUIdentity ()) RetrieveClassicalCPUIdentity ();
		RetrieveExtendedCPUFeatures ();

		// Retrieve the CPU power management features.
		RetrieveCPUPowerManagement ();

		// Now attempt to retrieve the serial number (if possible).
		RetrieveProcessorSerialNumber ();
	} else {
		try {
			int nFamily;
			UCHAR nPEFlag;
	
			// Attempt to determine what the processor is given that we do not have CPUID.
			__asm {
                ; Intel 8086 processor check
				; Bits 12-15 of the FLAGS register are always set on the
				; 8086 processor.
				;
				pushf						; push original FLAGS
				pop ax						; get original FLAGS
				mov cx, ax					; save original FLAGS
				and ax, 0fffh				; clear bits 12-15 in FLAGS
				push ax						; save new FLAGS value on stack
				popf						; replace current FLAGS value
				pushf						; get new FLAGS
				pop ax						; store new FLAGS in AX
				and ax, 0f000h				; if bits 12-15 are set, then
				cmp ax, 0f000h				; processor is an 8086/8088
				mov nFamily, 0				; turn on 8086/8088 flag
				jne Check_80286				; go check for 80286
				push sp						; double check with push sp
				pop dx						; if value pushed was different
				cmp dx, sp					; means it's really an 8086
				jne Terminate_CPU_Check		; jump if processor is 8086/8088
				mov nFamily, 10h			; indicate unknown processor
				jmp Terminate_CPU_Check

				; Intel 286 processor check
				; Bits 12-15 of the FLAGS register are always clear on the
				; Intel 286 processor in real-address mode.

				Check_80286:
					smsw ax					; save machine status word
					and ax, 1				; isolate PE bit of MSW
					mov nPEFlag, al			; save PE bit to indicate V86
					or cx, 0f000h			; try to set bits 12-15
					push cx					; save new FLAGS value on stack
					popf					; replace current FLAGS value
					pushf					; get new FLAGS
					pop ax					; store new FLAGS in AX
					and ax, 0f000h			; if bits 12-15 are clear
					mov nFamily, 2			; processor=80286, turn on 80286 flag
					jz Terminate_CPU_Check	; jump if processor is 80286

				Terminate_CPU_Check:
			}

			// Save the family ID.
			if (nFamily == 0x10) {
				// The processor is classed as an unknown.
				ChipID.Family = -1;
			} else ChipID.Family = nFamily;
		}

		catch (...) {
			// Something went wrong attempting to detect the processor.
			throw;
		}
	}
}

CPUInfo::~CPUInfo ()
{
	// Clean up any memory allocations.
	if (Speed != NULL) delete Speed;
}

// --------------------------------------------------------
//
//         Public Functions - CPUInfo Class
//
// --------------------------------------------------------

char * CPUInfo::GetVendorString ()
{
	// Return the vendor string.
	return ChipID.Vendor;
}

char * CPUInfo::GetVendorID ()
{
	// Return the vendor ID.
	switch (ChipManufacturer) {
		case Intel:
			return "Intel Corporation";
		case AMD:
			return "Advanced Micro Devices";
		case NSC:
			return "National Semiconductor";
		case Cyrix:
			return "Cyrix Corp., VIA Inc.";
		case NexGen:
			return "NexGen Inc., Advanced Micro Devices";
		case IDT:
			return "IDT\\Centaur, Via Inc.";
		case UMC:
			return "United Microelectronics Corp.";
		case Rise:
			return "Rise";
		case Transmeta:
			return "Transmeta";
		default:
			return "Unknown Manufacturer";
	}
}

int CPUInfo::GetTypeID ()
{
	// Return the type ID of the CPU.
	return ChipID.Type;
}

int CPUInfo::GetFamilyID ()
{
	// Return the family of the CPU present.
	return ChipID.Family;
}

int CPUInfo::GetModelID ()
{
	// Return the model of CPU present.
	return ChipID.Model;
}

int CPUInfo::GetBrandID ()
{
	// Return the brand of CPU present.
	return Features.ExtendedFeatures.BrandID; 
}

int CPUInfo::GetSteppingCode ()
{
	// Return the stepping code of the CPU present.
	return ChipID.Revision;
}

char * CPUInfo::GetExtendedProcessorName ()
{
	// Return the stepping code of the CPU present.
	return ChipID.ProcessorName;
}

char * CPUInfo::GetProcessorSerialNumber ()
{
	// Return the serial number of the processor in hexadecimal: xxxx-xxxx-xxxx-xxxx-xxxx-xxxx.
	return ChipID.SerialNumber;
}

int CPUInfo::GetLogicalProcessorsPerPhysical ()
{
	// Return the logical processors per physical.
	return Features.ExtendedFeatures.LogicalProcessorsPerPhysical;
}

int CPUInfo::GetProcessorClockFrequency ()
{
	// Return the processor clock frequency.
	if (Speed != NULL)
		return Speed->CPUSpeedInMHz;
	else if (Features.CPUSpeed > 0)
		return Features.CPUSpeed;
	else 
		// Display the error condition.
		return -1;
}

int CPUInfo::GetProcessorAPICID ()
{
	// Return the APIC ID.
	return Features.ExtendedFeatures.APIC_ID;
}

int CPUInfo::GetProcessorCacheXSize (__int64 dwCacheID)
{
	// Return the chosen cache size.
	switch (dwCacheID) {
		case L1CACHE_FEATURE:
			return Features.L1CacheSize;

		case L2CACHE_FEATURE:
			return Features.L2CacheSize;

		case L3CACHE_FEATURE:
			return Features.L3CacheSize;
	}

	// The user did something strange just return and error.
	return -1;
}

int CPUInfo::GetCacheLineSize ()
{
	// Return the cache line size.
	return Features.ExtendedFeatures.CLFlush_Line_Size;
}

bool CPUInfo::DoesCPUSupportFeature (__int64 dwFeature)
{
	bool bHasFeature = false;

	// Check for FPU on-chip.
	if (((dwFeature & HAS_FPU) != 0) && Features.Features.bHasFPU) bHasFeature = true;
	// Check for Virtual Mode Extension.
	if (((dwFeature & HAS_VME) != 0) && Features.Features.bHasVME) bHasFeature = true;
	// Check for Debugging Extensions.
	if (((dwFeature & HAS_DE) != 0) && Features.Features.bHasDE) bHasFeature = true;
	// Check for Page Size Extensions.
	if (((dwFeature & HAS_PSE) != 0) && Features.Features.bHasPSE) bHasFeature = true;
	// Check for Time Stamp Counter.
	if (((dwFeature & HAS_TSC) != 0) && Features.Features.bHasTSC) bHasFeature = true;
	// Check for Model Specific Registers.
	if (((dwFeature & HAS_MSR) != 0) && Features.Features.bHasMSR) bHasFeature = true;
	// Check for Physical Address Extensions.
	if (((dwFeature & HAS_PAE) != 0) && Features.Features.bHasPAE) bHasFeature = true;
	// Check for Machine Check Exception.
	if (((dwFeature & HAS_MCE) != 0) && Features.Features.bHasMCE) bHasFeature = true;
	// Check for CMPXCHG8 Instruction.
	if (((dwFeature & HAS_CX8) != 0) && Features.Features.bHasCX8) bHasFeature = true;
	// Check for on-chip APIC hardware.
	if (((dwFeature & HAS_APIC) != 0) && Features.Features.bHasAPIC) bHasFeature = true;
	// Check for Fast-System Call.
	if (((dwFeature & HAS_SEP) != 0) && Features.Features.bHasSEP) bHasFeature = true;
	// Check for Memory Type Range Registers.
	if (((dwFeature & HAS_MTRR) != 0) && Features.Features.bHasMTRR) bHasFeature = true;
	// Check for Page Global Enable.
	if (((dwFeature & HAS_PGE) != 0) && Features.Features.bHasPGE) bHasFeature = true;
	// Check for Machine Check Architecture.
	if (((dwFeature & HAS_MCA) != 0) && Features.Features.bHasMCA) bHasFeature = true;
	// Check for Conditional Move Instruction.
	if (((dwFeature & HAS_CMOV) != 0) && Features.Features.bHasCMOV) bHasFeature = true;
	// Check for Page Attribute Tables.
	if (((dwFeature & HAS_PAT) != 0) && Features.Features.bHasPAT) bHasFeature = true;
	// Check for 36-bit Page Size Extensions.
	if (((dwFeature & HAS_PSE36) != 0) && Features.Features.bHasPSE36) bHasFeature = true;
	// Check for 96-bit Processor Serial Number.
	if (((dwFeature & HAS_PSN) != 0) && Features.Features.bHasPSN) bHasFeature = true;
	// Check for CLFLUSH Instruction.
	if (((dwFeature & HAS_CLFSH) != 0) && Features.Features.bHasCLFSH) bHasFeature = true;
	// Check for Debug Trace Store.
	if (((dwFeature & HAS_DTS) != 0) && Features.Features.bHasDTS) bHasFeature = true;
	// Check for ACPI Supported.
	if (((dwFeature & HAS_ACPI) != 0) && Features.Features.bHasACPI) bHasFeature = true;
	// Check for MMX instructions
	if (((dwFeature & HAS_MMX) != 0) && Features.Features.bHasMMX) bHasFeature = true;
	// Check for Fast Save and Restore.
	if (((dwFeature & HAS_FXSR) != 0) && Features.Features.bHasFXSR) bHasFeature = true;
	// Check for Streaming SIMD Extenstions.
	if (((dwFeature & HAS_SSE) != 0) && Features.Features.bHasSSE) bHasFeature = true;
	// Check for Streaming SIMD Extenstions 2.
	if (((dwFeature & HAS_SSE2) != 0) && Features.Features.bHasSSE2) bHasFeature = true;
	// Check for Self Snoop
	if (((dwFeature & HAS_SS) != 0) && Features.Features.bHasSS) bHasFeature = true;
	// Check for Hyper-Threading Technology.
	if (((dwFeature & HAS_HTT) != 0) && Features.Features.bHasHTT) bHasFeature = true;
	// Check for Thermal Monitor Support.
	if (((dwFeature & HAS_TM) != 0) && Features.Features.bHasTM) bHasFeature = true;
	// Check for IA-64 Capability.
	if (((dwFeature & HAS_IA64) != 0) && Features.Features.bIsIA64) bHasFeature = true;

	// Check for SSE FP instructions.
	if (((dwFeature & HAS_SSEFP) != 0) && Features.bHasSSEFP) bHasFeature = true;
	// Check for SSE MMX instructions.
	if (((dwFeature & HAS_SSEMMX) != 0) && Features.ExtendedFeatures.bHasSSEMMX) bHasFeature = true;
	// Check for 3DNow! instructions.
	if (((dwFeature & HAS_3DNOW) != 0) && Features.ExtendedFeatures.bHas3DNow) bHasFeature = true;
	// Check for 3DNow+ instructions.
	if (((dwFeature & HAS_3DNOWPLUS) != 0) && Features.ExtendedFeatures.bHas3DNowPlus) bHasFeature = true;
	// Check for MMX+ instructions.
	if (((dwFeature & HAS_MMXPLUS) != 0) && Features.ExtendedFeatures.bHasMMXPlus) bHasFeature = true;
	// Check for MP capable.
	if (((dwFeature & MP_CAPABLE) != 0) && Features.ExtendedFeatures.SupportsMP) bHasFeature = true;
	// Check for L1 cache size.
	if (((dwFeature & L1CACHE_FEATURE) != 0) && (Features.L1CacheSize != -1)) bHasFeature = true;
	// Check for L2 cache size.
	if (((dwFeature & L2CACHE_FEATURE) != 0) && (Features.L2CacheSize != -1)) bHasFeature = true;
	// Check for L3 cache size.
	if (((dwFeature & L3CACHE_FEATURE) != 0) && (Features.L3CacheSize != -1)) bHasFeature = true;
	// Check for temperature sensing diode support.
	if (((dwFeature & HAS_TSD) != 0) && Features.ExtendedFeatures.PowerManagement.bHasTSD) bHasFeature = true;
	// Check for frequency ID support.
	if (((dwFeature & HAS_FID) != 0) && Features.ExtendedFeatures.PowerManagement.bHasFID) bHasFeature = true;
	// Check for voltage ID support.
	if (((dwFeature & HAS_VID) != 0) && Features.ExtendedFeatures.PowerManagement.bHasVID) bHasFeature = true;

	// Check for Signal Break on FERR.
	if (((dwFeature & HAS_SBF) != 0) && Features.Features.bHasSBF) bHasFeature = true;
	// Check for Thermal Monitor 2.
	if (((dwFeature & HAS_TM2) != 0) && Features.Features2.bHasTM2) bHasFeature = true;
	// Check for Enhanced SpeedStep Technology.
	if (((dwFeature & HAS_EST) != 0) && Features.Features2.bHasEST) bHasFeature = true;
	// Check for Context ID.
	if (((dwFeature & HAS_CID) != 0) && Features.Features2.bHasCID) bHasFeature = true;
	// Check for Denormals Are Zero.
	if (((dwFeature & HAS_DAZ) != 0) && Features.ExtendedFeatures.bHasDAZ) bHasFeature = true;

	return bHasFeature;
}

// --------------------------------------------------------
//
//         Private Functions - CPUInfo Class
//
// --------------------------------------------------------

bool __cdecl CPUInfo::DoesCPUSupportCPUID ()
{
	int CPUIDPresent = 0;

#ifdef _WIN32 
	
	// Use SEH to determine CPUID presence
    try {
        _asm {
#ifndef CPUID_AWARE_COMPILER
 			; we must push/pop the registers <<CPUID>> writes to, as the
			; optimiser doesn't know about <<CPUID>>, and so doesn't expect
			; these registers to change.
			push eax
			push ebx
			push ecx
			push edx
#endif
			; <<CPUID>> 
            mov eax, 0
			CPUID_INSTRUCTION

#ifndef CPUID_AWARE_COMPILER
			pop edx
			pop ecx
			pop ebx
			pop eax
#endif
        }
    }

	// A generic catch-all just to be sure...
	catch (...) {
        // Stop the class from trying to use CPUID again!
        CPUIDPresent = false;
		return false;
    }
#else
    // The "right" way, which doesn't work under certain Windows versions
	try {
		_asm {
			pushfd                      ; save EFLAGS to stack.
			pop     eax                 ; store EFLAGS in eax.
			mov     edx, eax            ; save in ebx for testing later.
			xor     eax, 0200000h       ; switch bit 21.
			push    eax                 ; copy "changed" value to stack.
			popfd                       ; save "changed" eax to EFLAGS.
			pushfd
			pop     eax
			xor     eax, edx            ; See if bit changeable.
			jnz     short cpuid_present ; if so, mark 
			mov     eax, -1             ; CPUID not present - disable its usage
			jmp     no_features

	cpuid_present:
			mov		eax, 0				; CPUID capable CPU - enable its usage.
			
	no_features:
			mov     CPUIDPresent, eax	; Save the value in eax to a variable.
		}
	}
	
	// A generic catch-all just to be sure...
	catch (...) {
		// Stop the class from trying to use CPUID again!
        CPUIDPresent = false;
		return false;
    }
		
#endif

	// Return true to indicate support or false to indicate lack.
	return (CPUIDPresent == 0) ? true : false;
}

bool __cdecl CPUInfo::RetrieveCPUFeatures ()
{
	int CPUAdvanced = 0;
	int nSEPChecker = 0;
	CPUFeatures_EDX CPUFeatures;
	CPUFeatures_ECX CPUFeatures2;

	// Use assembly to detect CPUID information...
	try {
		_asm {
#ifndef CPUID_AWARE_COMPILER
 			; we must push/pop the registers <<CPUID>> writes to, as the
			; optimiser doesn't know about <<CPUID>>, and so doesn't expect
			; these registers to change.
			push eax
			push ebx
			push ecx
			push edx
#endif
			; <<CPUID>> 
			; eax = 1 --> eax: CPU ID - bits 31..16 - unused, bits 15..12 - type, bits 11..8 - family, bits 7..4 - model, bits 3..0 - mask revision
			;			  ebx: 31..24 - default APIC ID, 23..16 - logical processsor ID, 15..8 - CFLUSH chunk size , 7..0 - brand ID
			;			  edx: CPU feature flags
			mov eax, 1
			CPUID_INSTRUCTION
			mov CPUFeatures, edx
			mov CPUAdvanced, ebx
			mov CPUFeatures2, ecx
			mov nSEPChecker, eax

#ifndef CPUID_AWARE_COMPILER
			pop edx
			pop ecx
			pop ebx
			pop eax
#endif
		}
	}

	// A generic catch-all just to be sure...
	catch (...) {
		return false;
	}

	// Copy the CPU features into the appropriate holder.
	Features.Features = CPUFeatures;
	Features.Features2 = CPUFeatures2;

	// Retrieve extended SSE capabilities if SSE is available.
	if (Features.Features.bHasSSE) {
		
		// Attempt to try some SSE FP instructions.
		try {
			// Perform: orps xmm0, xmm0
			_asm {
				_emit 0x0f
	    		_emit 0x56
	    		_emit 0xc0	
			}

			// SSE FP capable processor.
			Features.bHasSSEFP = true;
	    }
	    
		// A generic catch-all just to be sure...
		catch (...) {
	    	// bad instruction - processor or OS cannot handle SSE FP.
			Features.bHasSSEFP = false;
		}
	} else {
		// Set the advanced SSE capabilities to not available.
		Features.bHasSSEFP = false;
	}

	// Check the presence of the SEP bit:
	//		The SYSENTER Present (SEP) bit 11 of CPUID indicates the presence of this facility. An operating system that
	//		detects the presence of the SEP bit must also qualify the processor family and model to ensure that the
	//		SYSENTER/SYSEXIT instructions are actually present:
	//
	//				IF (CPUID SEP bit is set) {
	//					IF ((Processor Signature (EAX) & 0x0FFF3FFF) < 0x00000633)
	//						Fast System Call is NOT supported
	//					ELSE
	//						Fast System Call is supported
	//				}
	//
	// NOTE: The Pentium Pro processor (Model = 1) returns a set SEP CPUID feature bit, but should not be used by software.
	if (Features.Features.bHasSEP) {
		if ((nSEPChecker & 0x0FFF3FFF) < 0x00000633) {
			Features.Features.bHasSEP = false;
		}
	}

	// Retrieve Intel specific extended features.
	if (ChipManufacturer == Intel) {
		Features.ExtendedFeatures.LogicalProcessorsPerPhysical = (Features.Features.bHasHTT) ? ((CPUAdvanced & 0x00FF0000) >> 16) : 1;
		
		if (Features.Features.bHasAPIC){
			// Retrieve APIC information as there is one present.
			Features.ExtendedFeatures.APIC_ID = ((CPUAdvanced & 0xFF000000) >> 24);
		}

		// Save the brand ID.
		Features.ExtendedFeatures.BrandID = (CPUAdvanced & 0x000000FF);

		// Check to see if the CPU supports "denormals are zero".  Start by looking for EDX bit 24 and bits 25 or 26
		if ((CPUAdvanced & 0x01000000) && ((CPUAdvanced & 0x02000000) || (CPUAdvanced & 0x04000000))) {
			try {
				int nFXSavePoint;
				short nBuffer;
				bool bDAZ;

				__asm {
					mov bx, nBuffer
					and bx, 0FFF0h
					add bx, 8000h
					
					// Zero a 16-byte aligned, 512-byte area of memory. This is necessary 
					// since some implementations of FXSAVE do not modify reserved areas 
					// within the image.

					push ds
					pop es
					mov di, bx
					xor ax, ax
					mov cx, 512/2
					cld
					rep stosw
					
					// Execute an FXSAVE into the cleared area.
					
					fxsave nFXSavePoint

					// Bytes 28-31 of the FXSAVE image are defined to contain the MXCSR_MASK. If 
					// this value is 0, then the processor's MXCSR_MASK is 0xFFBF, otherwise MXCSR_MASK 
					// is the value of this dword.
					
					mov eax, DWORD PTR nFXSavePoint [8000000h]
					cmp eax, 0	
					jne check_mxcsr_mask
					mov eax, 0FFBFh	
				
				check_mxcsr_mask:
					
					// If bit 6 of the MXCSR_MASK is set, then DAZ is supported.

					bt eax, 20h
					jc supported
					mov bDAZ, 0
					jmp CleanUp
					
				supported:
					mov bDAZ, 1

				CleanUp:
					// Just try and tidy up here.
				}

				// Save the DAZ bit.
				Features.ExtendedFeatures.bHasDAZ = bDAZ;
			}
			catch (...) {
				// Something went wrong with the DAZ confirmation.
			}
		}
	}

	// Retrieve the CLFLUSH line size.
	if (Features.Features.bHasCLFSH) {
		Features.ExtendedFeatures.CLFlush_Line_Size = ((CPUAdvanced & 0x0000FF00) >> 8) * 8;
	}

	return true;
}

bool __cdecl CPUInfo::RetrieveCPUIdentity ()
{
	int CPUVendor[3];
	int CPUSignature;

	// Use assembly to detect CPUID information...
	try {
		_asm {
#ifndef CPUID_AWARE_COMPILER
 			; we must push/pop the registers <<CPUID>> writes to, as the
			; optimiser doesn't know about <<CPUID>>, and so doesn't expect
			; these registers to change.
			push eax
			push ebx
			push ecx
			push edx
#endif
			; <<CPUID>>
			; eax = 0 --> eax: maximum value of CPUID instruction.
			;			  ebx: part 1 of 3; CPU signature.
			;			  edx: part 2 of 3; CPU signature.
			;			  ecx: part 3 of 3; CPU signature.
			mov eax, 0
			CPUID_INSTRUCTION
			mov CPUVendor[0 * TYPE int], ebx
			mov CPUVendor[1 * TYPE int], edx
			mov CPUVendor[2 * TYPE int], ecx

			; <<CPUID>> 
			; eax = 1 --> eax: CPU ID - bits 31..16 - unused, bits 15..12 - type, bits 11..8 - family, bits 7..4 - model, bits 3..0 - mask revision
			;			  ebx: 31..24 - default APIC ID, 23..16 - logical processsor ID, 15..8 - CFLUSH chunk size , 7..0 - brand ID
			;			  edx: CPU feature flags
			mov eax,1
			CPUID_INSTRUCTION
			mov CPUSignature, eax

#ifndef CPUID_AWARE_COMPILER
			pop edx
			pop ecx
			pop ebx
			pop eax
#endif
		}
	}

	// A generic catch-all just to be sure...
	catch (...) {
		return false;
	}

	// Process the returned information.
	memcpy (ChipID.Vendor, &(CPUVendor[0]), sizeof (int));
	memcpy (&(ChipID.Vendor[4]), &(CPUVendor[1]), sizeof (int));
	memcpy (&(ChipID.Vendor[8]), &(CPUVendor[2]), sizeof (int));
	ChipID.Vendor[12] = '\0';

	// Attempt to retrieve the manufacturer from the vendor string.
	if (strcmp (ChipID.Vendor, "GenuineIntel") == 0)		ChipManufacturer = Intel;				// Intel Corp.
	else if (strcmp (ChipID.Vendor, "UMC UMC UMC ") == 0)	ChipManufacturer = UMC;					// United Microelectronics Corp.
	else if (strcmp (ChipID.Vendor, "AuthenticAMD") == 0)	ChipManufacturer = AMD;					// Advanced Micro Devices
	else if (strcmp (ChipID.Vendor, "AMD ISBETTER") == 0)	ChipManufacturer = AMD;					// Advanced Micro Devices (1994)
	else if (strcmp (ChipID.Vendor, "CyrixInstead") == 0)	ChipManufacturer = Cyrix;				// Cyrix Corp., VIA Inc.
	else if (strcmp (ChipID.Vendor, "NexGenDriven") == 0)	ChipManufacturer = NexGen;				// NexGen Inc. (now AMD)
	else if (strcmp (ChipID.Vendor, "CentaurHauls") == 0)	ChipManufacturer = IDT;					// IDT/Centaur (now VIA)
	else if (strcmp (ChipID.Vendor, "RiseRiseRise") == 0)	ChipManufacturer = Rise;				// Rise
	else if (strcmp (ChipID.Vendor, "GenuineTMx86") == 0)	ChipManufacturer = Transmeta;			// Transmeta
	else if (strcmp (ChipID.Vendor, "TransmetaCPU") == 0)	ChipManufacturer = Transmeta;			// Transmeta
	else if (strcmp (ChipID.Vendor, "Geode By NSC") == 0)	ChipManufacturer = NSC;					// National Semiconductor
	else													ChipManufacturer = UnknownManufacturer;	// Unknown manufacturer
	
	// Retrieve the family of CPU present.
	ChipID.ExtendedFamily =		((CPUSignature & 0x0FF00000) >> 20);	// Bits 27..20 Used
	ChipID.ExtendedModel =		((CPUSignature & 0x000F0000) >> 16);	// Bits 19..16 Used
	ChipID.Type =				((CPUSignature & 0x0000F000) >> 12);	// Bits 15..12 Used
	ChipID.Family =				((CPUSignature & 0x00000F00) >> 8);		// Bits 11..8 Used
	ChipID.Model =				((CPUSignature & 0x000000F0) >> 4);		// Bits 7..4 Used
	ChipID.Revision =			((CPUSignature & 0x0000000F) >> 0);		// Bits 3..0 Used

	return true;
}

bool __cdecl CPUInfo::RetrieveCPUCacheDetails ()
{
	int L1Cache[4] = { 0, 0, 0, 0 };
	int L2Cache[4] = { 0, 0, 0, 0 };

	// Check to see if what we are about to do is supported...
	if (RetrieveCPUExtendedLevelSupport (0x80000005)) {
		// Use assembly to retrieve the L1 cache information ...
		try {
			_asm {
#ifndef CPUID_AWARE_COMPILER
 				; we must push/pop the registers <<CPUID>> writes to, as the
				; optimiser doesn't know about <<CPUID>>, and so doesn't expect
				; these registers to change.
				push eax
				push ebx
				push ecx
				push edx
#endif
				; <<CPUID>>
				; eax = 0x80000005 --> eax: L1 cache information - Part 1 of 4.
				;					   ebx: L1 cache information - Part 2 of 4.
				;					   edx: L1 cache information - Part 3 of 4.
				;			 		   ecx: L1 cache information - Part 4 of 4.
				mov eax, 0x80000005
				CPUID_INSTRUCTION
				mov L1Cache[0 * TYPE int], eax
				mov L1Cache[1 * TYPE int], ebx
				mov L1Cache[2 * TYPE int], ecx
				mov L1Cache[3 * TYPE int], edx

#ifndef CPUID_AWARE_COMPILER
			pop edx
			pop ecx
			pop ebx
			pop eax
#endif
			}
		}

		// A generic catch-all just to be sure...
		catch (...) {
			return false;
		}

		// Save the L1 data cache size (in KB) from ecx: bits 31..24 as well as data cache size from edx: bits 31..24.
		Features.L1CacheSize = ((L1Cache[2] & 0xFF000000) >> 24);
		Features.L1CacheSize += ((L1Cache[3] & 0xFF000000) >> 24);
	} else {
		// Store -1 to indicate the cache could not be queried.
		Features.L1CacheSize = -1;
	}

	// Check to see if what we are about to do is supported...
	if (RetrieveCPUExtendedLevelSupport (0x80000006)) {
		// Use assembly to retrieve the L2 cache information ...
		try {
			_asm {
#ifndef CPUID_AWARE_COMPILER
 				; we must push/pop the registers <<CPUID>> writes to, as the
				; optimiser doesn't know about <<CPUID>>, and so doesn't expect
				; these registers to change.
				push eax
				push ebx
				push ecx
				push edx
#endif
				; <<CPUID>>
				; eax = 0x80000006 --> eax: L2 cache information - Part 1 of 4.
				;					   ebx: L2 cache information - Part 2 of 4.
				;					   edx: L2 cache information - Part 3 of 4.
				;			 		   ecx: L2 cache information - Part 4 of 4.
				mov eax, 0x80000006
				CPUID_INSTRUCTION
				mov L2Cache[0 * TYPE int], eax
				mov L2Cache[1 * TYPE int], ebx
				mov L2Cache[2 * TYPE int], ecx
				mov L2Cache[3 * TYPE int], edx

#ifndef CPUID_AWARE_COMPILER
				pop edx
				pop ecx
				pop ebx
				pop eax
#endif			
			}
		}

		// A generic catch-all just to be sure...
		catch (...) {
			return false;
		}

		// Save the L2 unified cache size (in KB) from ecx: bits 31..16.
		Features.L2CacheSize = ((L2Cache[2] & 0xFFFF0000) >> 16);
	} else {
		// Store -1 to indicate the cache could not be queried.
		Features.L2CacheSize = -1;
	}
	
	// Define L3 as being not present as we cannot test for it.
	Features.L3CacheSize = -1;

	// Return failure if we cannot detect either cache with this method.
	return ((Features.L1CacheSize == -1) && (Features.L2CacheSize == -1)) ? false : true;
}

bool __cdecl CPUInfo::RetrieveClassicalCPUCacheDetails ()
{
	int TLBCode = -1, TLBData = -1, L1Code = -1, L1Data = -1, L1Trace = -1, L2Unified = -1, L3Unified = -1;
	int TLBCacheData[4] = { 0, 0, 0, 0 };
	int TLBPassCounter = 0;
	int TLBCacheUnit = 0;

	do {
		// Use assembly to retrieve the L2 cache information ...
		try {
			_asm {
#ifndef CPUID_AWARE_COMPILER
 				; we must push/pop the registers <<CPUID>> writes to, as the
				; optimiser doesn't know about <<CPUID>>, and so doesn't expect
				; these registers to change.
				push eax
				push ebx
				push ecx
				push edx
#endif
				; <<CPUID>>
				; eax = 2 --> eax: TLB and cache information - Part 1 of 4.
				;			  ebx: TLB and cache information - Part 2 of 4.
				;			  ecx: TLB and cache information - Part 3 of 4.
				;			  edx: TLB and cache information - Part 4 of 4.
				mov eax, 2
				CPUID_INSTRUCTION
				mov TLBCacheData[0 * TYPE int], eax
				mov TLBCacheData[1 * TYPE int], ebx
				mov TLBCacheData[2 * TYPE int], ecx
				mov TLBCacheData[3 * TYPE int], edx

#ifndef CPUID_AWARE_COMPILER
				pop edx
				pop ecx
				pop ebx
				pop eax
#endif
			}
		}

		// A generic catch-all just to be sure...
		catch (...) {
			return false;
		}

		int bob = ((TLBCacheData[0] & 0x00FF0000) >> 16);

		// Process the returned TLB and cache information.
		for (int nCounter = 0; nCounter < TLBCACHE_INFO_UNITS; nCounter ++) {
			
			// First of all - decide which unit we are dealing with.
			switch (nCounter) {
				
				// eax: bits 8..15 : bits 16..23 : bits 24..31
				case 0: TLBCacheUnit = ((TLBCacheData[0] & 0x0000FF00) >> 8); break;
				case 1: TLBCacheUnit = ((TLBCacheData[0] & 0x00FF0000) >> 16); break;
				case 2: TLBCacheUnit = ((TLBCacheData[0] & 0xFF000000) >> 24); break;

				// ebx: bits 0..7 : bits 8..15 : bits 16..23 : bits 24..31
				case 3: TLBCacheUnit = ((TLBCacheData[1] & 0x000000FF) >> 0); break;
				case 4: TLBCacheUnit = ((TLBCacheData[1] & 0x0000FF00) >> 8); break;
				case 5: TLBCacheUnit = ((TLBCacheData[1] & 0x00FF0000) >> 16); break;
				case 6: TLBCacheUnit = ((TLBCacheData[1] & 0xFF000000) >> 24); break;

				// ecx: bits 0..7 : bits 8..15 : bits 16..23 : bits 24..31
				case 7: TLBCacheUnit = ((TLBCacheData[2] & 0x000000FF) >> 0); break;
				case 8: TLBCacheUnit = ((TLBCacheData[2] & 0x0000FF00) >> 8); break;
				case 9: TLBCacheUnit = ((TLBCacheData[2] & 0x00FF0000) >> 16); break;
				case 10: TLBCacheUnit = ((TLBCacheData[2] & 0xFF000000) >> 24); break;

				// edx: bits 0..7 : bits 8..15 : bits 16..23 : bits 24..31
				case 11: TLBCacheUnit = ((TLBCacheData[3] & 0x000000FF) >> 0); break;
				case 12: TLBCacheUnit = ((TLBCacheData[3] & 0x0000FF00) >> 8); break;
				case 13: TLBCacheUnit = ((TLBCacheData[3] & 0x00FF0000) >> 16); break;
				case 14: TLBCacheUnit = ((TLBCacheData[3] & 0xFF000000) >> 24); break;

				// Default case - an error has occured.
				default: return false;
			}

			// Now process the resulting unit to see what it means....
			switch (TLBCacheUnit) {
				case 0x00: break;
				case 0x01: STORE_TLBCACHE_INFO (TLBCode, 4); break;
				case 0x02: STORE_TLBCACHE_INFO (TLBCode, 4096); break;
				case 0x03: STORE_TLBCACHE_INFO (TLBData, 4); break;
				case 0x04: STORE_TLBCACHE_INFO (TLBData, 4096); break;
				case 0x06: STORE_TLBCACHE_INFO (L1Code, 8); break;
				case 0x08: STORE_TLBCACHE_INFO (L1Code, 16); break;
				case 0x0a: STORE_TLBCACHE_INFO (L1Data, 8); break;
				case 0x0c: STORE_TLBCACHE_INFO (L1Data, 16); break;
				case 0x10: 
					// IA-64 Only
					if ((ChipID.Model == 0xf) && (ChipID.ExtendedFamily == 1))
						STORE_TLBCACHE_INFO (L1Data, 16); 
					break;			
				case 0x15:  
					// IA-64 Only
					if ((ChipID.Model == 0xf) && (ChipID.ExtendedFamily == 1))
						STORE_TLBCACHE_INFO (L1Code, 16); 
					break;
				case 0x1a:  
					// IA-64 Only
					if ((ChipID.Model == 0xf) && (ChipID.ExtendedFamily == 1))
						STORE_TLBCACHE_INFO (L2Unified, 96); 
					break;
				case 0x22: STORE_TLBCACHE_INFO (L3Unified, 512); break;
				case 0x23: STORE_TLBCACHE_INFO (L3Unified, 1024); break;
				case 0x25: STORE_TLBCACHE_INFO (L3Unified, 2048); break;
				case 0x29: STORE_TLBCACHE_INFO (L3Unified, 4096); break;
				case 0x39: STORE_TLBCACHE_INFO (L2Unified, 128); break;
				case 0x3c: STORE_TLBCACHE_INFO (L2Unified, 256); break;
				case 0x40: 
					// No integrated L2 cache (P6 core) or L3 cache (P4 core).
					if (ChipID.Model == 6)
						STORE_TLBCACHE_INFO (L2Unified, 0);
					else if ((ChipID.Model == 0xf) && (ChipID.ExtendedFamily == 0))
						STORE_TLBCACHE_INFO (L3Unified, 0);
					break;
				case 0x41: STORE_TLBCACHE_INFO (L2Unified, 128); break;
				case 0x42: STORE_TLBCACHE_INFO (L2Unified, 256); break;
				case 0x43: STORE_TLBCACHE_INFO (L2Unified, 512); break;
				case 0x44: STORE_TLBCACHE_INFO (L2Unified, 1024); break;
				case 0x45: STORE_TLBCACHE_INFO (L2Unified, 2048); break;
				case 0x50: STORE_TLBCACHE_INFO (TLBCode, 4096); break;
				case 0x51: STORE_TLBCACHE_INFO (TLBCode, 4096); break;
				case 0x52: STORE_TLBCACHE_INFO (TLBCode, 4096); break;
				case 0x5b: STORE_TLBCACHE_INFO (TLBData, 4096); break;
				case 0x5c: STORE_TLBCACHE_INFO (TLBData, 4096); break;
				case 0x5d: STORE_TLBCACHE_INFO (TLBData, 4096); break;
				case 0x66: STORE_TLBCACHE_INFO (L1Data, 8); break;
				case 0x67: STORE_TLBCACHE_INFO (L1Data, 16); break;
				case 0x68: STORE_TLBCACHE_INFO (L1Data, 32); break;
				case 0x70: STORE_TLBCACHE_INFO (L1Trace, 12); break;
				case 0x71: STORE_TLBCACHE_INFO (L1Trace, 16); break;
				case 0x72: STORE_TLBCACHE_INFO (L1Trace, 32); break;
				case 0x77:  
					// IA-64 Only
					if ((ChipID.Model == 0xf) && (ChipID.ExtendedFamily == 1))
						STORE_TLBCACHE_INFO (L1Code, 16); 
					break;
				case 0x79: STORE_TLBCACHE_INFO (L2Unified, 128); break;
				case 0x7a: STORE_TLBCACHE_INFO (L2Unified, 256); break;
				case 0x7b: STORE_TLBCACHE_INFO (L2Unified, 512); break;
				case 0x7c: STORE_TLBCACHE_INFO (L2Unified, 1024); break;
				case 0x7e: STORE_TLBCACHE_INFO (L2Unified, 256); break;
				case 0x81: STORE_TLBCACHE_INFO (L2Unified, 128); break;
				case 0x82: STORE_TLBCACHE_INFO (L2Unified, 256); break;
				case 0x83: STORE_TLBCACHE_INFO (L2Unified, 512); break;
				case 0x84: STORE_TLBCACHE_INFO (L2Unified, 1024); break;
				case 0x85: STORE_TLBCACHE_INFO (L2Unified, 2048); break;
				case 0x88:  
					// IA-64 Only
					if ((ChipID.Model == 0xf) && (ChipID.ExtendedFamily == 1))
						STORE_TLBCACHE_INFO (L3Unified, 2048); 
					break;
				case 0x89:  
					// IA-64 Only
					if ((ChipID.Model == 0xf) && (ChipID.ExtendedFamily == 1))
						STORE_TLBCACHE_INFO (L3Unified, 4096); 
					break;
				case 0x8a:  
					// IA-64 Only
					if ((ChipID.Model == 0xf) && (ChipID.ExtendedFamily == 1))
						STORE_TLBCACHE_INFO (L3Unified, 8192); 
					break;
				case 0x8d:  
					// IA-64 Only
					if ((ChipID.Model == 0xf) && (ChipID.ExtendedFamily == 1))
						STORE_TLBCACHE_INFO (L3Unified, 3096); 
					break;
				case 0x90:  
					// IA-64 Only
					if ((ChipID.Model == 0xf) && (ChipID.ExtendedFamily == 1))
						STORE_TLBCACHE_INFO (TLBCode, 262144); 
					break;
				case 0x96:  
					// IA-64 Only
					if ((ChipID.Model == 0xf) && (ChipID.ExtendedFamily == 1))
						STORE_TLBCACHE_INFO (TLBCode, 262144); 
					break;
				case 0x9b:  
					// IA-64 Only
					if ((ChipID.Model == 0xf) && (ChipID.ExtendedFamily == 1))
						STORE_TLBCACHE_INFO (TLBCode, 262144); 
					break;
				
				// Default case - an error has occured.
				default: return false;
			}
		}

		// Increment the TLB pass counter.
		TLBPassCounter ++;
	
	} while ((TLBCacheData[0] & 0x000000FF) > TLBPassCounter);

	// Ok - we now have the maximum TLB, L1, L2, and L3 sizes...
	if ((L1Code == -1) && (L1Data == -1) && (L1Trace == -1)) Features.L1CacheSize = -1;
	else if ((L1Code == -1) && (L1Data == -1) && (L1Trace != -1)) Features.L1CacheSize = L1Trace;
	else if ((L1Code != -1) && (L1Data == -1)) Features.L1CacheSize = L1Code;
	else if ((L1Code == -1) && (L1Data != -1)) Features.L1CacheSize = L1Data;
	else if ((L1Code != -1) && (L1Data != -1)) Features.L1CacheSize = L1Code + L1Data;
	else Features.L1CacheSize = -1;

	// Ok - we now have the maximum TLB, L1, L2, and L3 sizes...
	if (L2Unified == -1) Features.L2CacheSize = -1;
	else Features.L2CacheSize = L2Unified;

	// Ok - we now have the maximum TLB, L1, L2, and L3 sizes...
	if (L3Unified == -1) Features.L3CacheSize = -1;
	else Features.L3CacheSize = L3Unified;

	return true;
}

bool __cdecl CPUInfo::RetrieveCPUClockSpeed ()
{
	// First of all we check to see if the RDTSC (0x0F, 0x31) instruction is supported.
	if (!Features.Features.bHasTSC) return false;

	// Get the clock speed.
	Speed = new CPUSpeed ();
	if (Speed == NULL) return false;

	return true;
}

bool __cdecl CPUInfo::RetrieveClassicalCPUClockSpeed (int nChipFamily)
{
	LARGE_INTEGER liStart, liEnd, liCountsPerSecond;
	double dFrequency, dDifference;

	// Attempt to get a starting tick count.
	QueryPerformanceCounter (&liStart);

	try {
		_asm {
			mov eax, 0x80000000
			
			cmp nChipFamily, 4
			jle Slow_Processors

			mov ebx, CLASSICAL_CPU_FREQ_LOOP
			jmp Timer_Loop
			
			Slow_Processors:
			mov ebx, CLASSICAL_CPU_FREQ_LOOP_486
			
			Timer_Loop: 
			bsf ecx,eax
			dec ebx
			jnz Timer_Loop
		}	
	}

	// A generic catch-all just to be sure...
	catch (...) {
		return false;
	}

	// Attempt to get a starting tick count.
	QueryPerformanceCounter (&liEnd);

	// Get the difference...  NB: This is in seconds....
	QueryPerformanceFrequency (&liCountsPerSecond);
	dDifference = (((double) liEnd.QuadPart - (double) liStart.QuadPart) / (double) liCountsPerSecond.QuadPart);

	// Calculate the clock speed.
	if (ChipID.Family == 3) {
		// 80386 processors....  Loop time is 115 cycles!
		dFrequency = (((CLASSICAL_CPU_FREQ_LOOP * 115) / dDifference) / 1048576);
	} else if (ChipID.Family == 4) {
		// 80486 processors....  Loop time is 47 cycles!
		dFrequency = (((CLASSICAL_CPU_FREQ_LOOP * 47) / dDifference) / 1048576);
	} else if (ChipID.Family == 5) {
		// Pentium processors....  Loop time is 43 cycles!
		dFrequency = (((CLASSICAL_CPU_FREQ_LOOP * 43) / dDifference) / 1048576);
	}
	
	// Save the clock speed.
	Features.CPUSpeed = (int) dFrequency;

	return true;
}

bool __cdecl CPUInfo::RetrieveCPUExtendedLevelSupport (int CPULevelToCheck)
{
	int MaxCPUExtendedLevel = 0;

	// The extended CPUID is supported by various vendors starting with the following CPU models: 
	//
	//		Manufacturer & Chip Name			|		Family		 Model		Revision
	//
	//		AMD K6, K6-2						|		   5		   6			x		
	//		Cyrix GXm, Cyrix III "Joshua"		|		   5		   4			x
	//		IDT C6-2							|		   5		   8			x
	//		VIA Cyrix III						|		   6		   5			x
	//		Transmeta Crusoe					|		   5		   x			x
	//		Intel Pentium 4						|		   f		   x			x
	//

	// We check to see if a supported processor is present...
	if (ChipManufacturer == AMD) {
		if (ChipID.Family < 5) return false;
		if ((ChipID.Family == 5) && (ChipID.Model < 6)) return false;
	} else if (ChipManufacturer == Cyrix) {
		if (ChipID.Family < 5) return false;
		if ((ChipID.Family == 5) && (ChipID.Model < 4)) return false;
		if ((ChipID.Family == 6) && (ChipID.Model < 5)) return false;
	} else if (ChipManufacturer == IDT) {
		if (ChipID.Family < 5) return false;
		if ((ChipID.Family == 5) && (ChipID.Model < 8)) return false;
	} else if (ChipManufacturer == Transmeta) {
		if (ChipID.Family < 5) return false;
	} else if (ChipManufacturer == Intel) {
		if (ChipID.Family < 0xf) return false;
	}
		
	// Use assembly to detect CPUID information...
	try {
		_asm {
#ifndef CPUID_AWARE_COMPILER
 			; we must push/pop the registers <<CPUID>> writes to, as the
			; optimiser doesn't know about <<CPUID>>, and so doesn't expect
			; these registers to change.
			push eax
			push ebx
			push ecx
			push edx
#endif
			; <<CPUID>> 
			; eax = 0x80000000 --> eax: maximum supported extended level
			mov eax,0x80000000
			CPUID_INSTRUCTION
			mov MaxCPUExtendedLevel, eax

#ifndef CPUID_AWARE_COMPILER
			pop edx
			pop ecx
			pop ebx
			pop eax
#endif
		}
	}

	// A generic catch-all just to be sure...
	catch (...) {
		return false;
	}

	// Now we have to check the level wanted vs level returned...
	int nLevelWanted = (CPULevelToCheck & 0x7FFFFFFF);
	int nLevelReturn = (MaxCPUExtendedLevel & 0x7FFFFFFF);

	// Check to see if the level provided is supported...
	if (nLevelWanted > nLevelReturn) return false;

	return true;
}

bool __cdecl CPUInfo::RetrieveExtendedCPUFeatures ()
{
	int CPUExtendedFeatures = 0;
	
	// Initialise the variables.
	Features.ExtendedFeatures.bHasMMXPlus = false;
	Features.ExtendedFeatures.bHas3DNow = false;
	Features.ExtendedFeatures.bHas3DNowPlus  = false;
	Features.ExtendedFeatures.bHasSSEMMX  = false;
	Features.ExtendedFeatures.SupportsMP  = false;
	
	// Check that we are not using an Intel processor as it does not support this.
	if (ChipManufacturer == Intel) return false;

	// Check to see if what we are about to do is supported...
	if (!RetrieveCPUExtendedLevelSupport (0x80000001)) return false;

	// Use assembly to detect CPUID information...
	try {
		_asm {
#ifndef CPUID_AWARE_COMPILER
 			; we must push/pop the registers <<CPUID>> writes to, as the
			; optimiser doesn't know about <<CPUID>>, and so doesn't expect
			; these registers to change.
			push eax
			push ebx
			push ecx
			push edx
#endif
			; <<CPUID>> 
			; eax = 0x80000001 --> eax: CPU ID - bits 31..16 - unused, bits 15..12 - type, bits 11..8 - family, bits 7..4 - model, bits 3..0 - mask revision
			;					   ebx: 31..24 - default APIC ID, 23..16 - logical processsor ID, 15..8 - CFLUSH chunk size , 7..0 - brand ID
			;					   edx: CPU feature flags
			mov eax,0x80000001
			CPUID_INSTRUCTION
			mov CPUExtendedFeatures, edx

#ifndef CPUID_AWARE_COMPILER
			pop edx
			pop ecx
			pop ebx
			pop eax
#endif
		}
	}

	// A generic catch-all just to be sure...
	catch (...) {
		return false;
	}

	// Retrieve the extended features of CPU present.
	Features.ExtendedFeatures.bHas3DNow =		((CPUExtendedFeatures & 0x80000000) != 0);	// 3DNow Present --> Bit 31.
	Features.ExtendedFeatures.bHas3DNowPlus =	((CPUExtendedFeatures & 0x40000000) != 0);	// 3DNow+ Present -- > Bit 30.
	Features.ExtendedFeatures.bHasSSEMMX =		((CPUExtendedFeatures & 0x00400000) != 0);	// SSE MMX Present --> Bit 22.
	Features.ExtendedFeatures.SupportsMP =		((CPUExtendedFeatures & 0x00080000) != 0);	// MP Capable -- > Bit 19.
	
	// Retrieve AMD specific extended features.
	if (ChipManufacturer == AMD) {
		Features.ExtendedFeatures.bHasMMXPlus =	((CPUExtendedFeatures &	0x00400000) != 0);	// AMD specific: MMX-SSE --> Bit 22
	}

	// Retrieve Cyrix specific extended features.
	if (ChipManufacturer == Cyrix) {
		Features.ExtendedFeatures.bHasMMXPlus =	((CPUExtendedFeatures &	0x01000000) != 0);	// Cyrix specific: Extended MMX --> Bit 24
	}

	return true;
}

bool __cdecl CPUInfo::RetrieveProcessorSerialNumber ()
{
	int SerialNumber[3];

	// Check to see if the processor supports the processor serial number.
	if (!Features.Features.bHasPSN) return false;

		// Use assembly to detect CPUID information...
	try {
		_asm {
#ifndef CPUID_AWARE_COMPILER
 			; we must push/pop the registers <<CPUID>> writes to, as the
			; optimiser doesn't know about <<CPUID>>, and so doesn't expect
			; these registers to change.
			push eax
			push ebx
			push ecx
			push edx
#endif
			; <<CPUID>>
			; eax = 3 --> ebx: top 32 bits are the processor signature bits --> NB: Transmeta only ?!?
			;			  ecx: middle 32 bits are the processor signature bits
			;			  edx: bottom 32 bits are the processor signature bits
			;
			; According to the Intel specs. - The above is wrong and we should be accessing first
			; eax (after cpuid with 1) and edx then ecx (after cpuid with 3).
			;
			; <<CPUID>>
			; eax = 1 --> eax: top 32 bits are the processor signature bits
			; eax = 3 --> ecx: middle 32 bits are the processor signature bits
			;			  edx: bottom 32 bits are the processor signature bits
			mov eax, 1
			CPUID_INSTRUCTION
			mov SerialNumber[0 * TYPE int], eax

			mov eax, 3
			CPUID_INSTRUCTION
			mov SerialNumber[1 * TYPE int], edx
			mov SerialNumber[2 * TYPE int], ecx

#ifndef CPUID_AWARE_COMPILER
			pop edx
			pop ecx
			pop ebx
			pop eax
#endif
		}
	}

	// A generic catch-all just to be sure...
	catch (...) {
		return false;
	}

	// Process the returned information.
	sprintf (ChipID.SerialNumber, "%.2x%.2x-%.2x%.2x-%.2x%.2x-%.2x%.2x-%.2x%.2x-%.2x%.2x",
			 ((SerialNumber[0] & 0xff000000) >> 24),
			 ((SerialNumber[0] & 0x00ff0000) >> 16),
			 ((SerialNumber[0] & 0x0000ff00) >> 8),
			 ((SerialNumber[0] & 0x000000ff) >> 0),
			 ((SerialNumber[1] & 0xff000000) >> 24),
			 ((SerialNumber[1] & 0x00ff0000) >> 16),
			 ((SerialNumber[1] & 0x0000ff00) >> 8),
			 ((SerialNumber[1] & 0x000000ff) >> 0),
			 ((SerialNumber[2] & 0xff000000) >> 24),
			 ((SerialNumber[2] & 0x00ff0000) >> 16),
			 ((SerialNumber[2] & 0x0000ff00) >> 8),
			 ((SerialNumber[2] & 0x000000ff) >> 0));

	return true;
}

bool __cdecl CPUInfo::RetrieveCPUPowerManagement ()
{	
	int CPUPowerManagement = 0;

	// Check to see if what we are about to do is supported...
	if (!RetrieveCPUExtendedLevelSupport (0x80000007)) {
		Features.ExtendedFeatures.PowerManagement.bHasFID = false;
		Features.ExtendedFeatures.PowerManagement.bHasVID = false;
		Features.ExtendedFeatures.PowerManagement.bHasTSD = false;
		return false;
	}

	// Use assembly to detect CPUID information...
	try {
		_asm {
#ifndef CPUID_AWARE_COMPILER
 			; we must push/pop the registers <<CPUID>> writes to, as the
			; optimiser doesn't know about <<CPUID>>, and so doesn't expect
			; these registers to change.
			push eax
			push ebx
			push ecx
			push edx
#endif
			; <<CPUID>> 
			; eax = 0x80000007 --> edx: get processor power management
			mov eax,0x80000007
			CPUID_INSTRUCTION
			mov CPUPowerManagement, edx
			
#ifndef CPUID_AWARE_COMPILER
			pop edx
			pop ecx
			pop ebx
			pop eax
#endif
		}
	}

	// A generic catch-all just to be sure...
	catch (...) {
		return false;
	}

	// Check for the power management capabilities of the CPU.
	Features.ExtendedFeatures.PowerManagement.bHasTSD = ((CPUPowerManagement & 0x00000001) != 0);
	Features.ExtendedFeatures.PowerManagement.bHasFID = ((CPUPowerManagement & 0x00000002) != 0);
	Features.ExtendedFeatures.PowerManagement.bHasVID = ((CPUPowerManagement & 0x00000004) != 0);
	
	return true;
}

bool __cdecl CPUInfo::RetrieveExtendedCPUIdentity ()
{
	int ProcessorNameStartPos = 0;
	int CPUExtendedIdentity[12];

	// Check to see if what we are about to do is supported...
	if (!RetrieveCPUExtendedLevelSupport (0x80000002)) return false;
	if (!RetrieveCPUExtendedLevelSupport (0x80000003)) return false;
	if (!RetrieveCPUExtendedLevelSupport (0x80000004)) return false;

	// Use assembly to detect CPUID information...
	try {
		_asm {
#ifndef CPUID_AWARE_COMPILER
 			; we must push/pop the registers <<CPUID>> writes to, as the
			; optimiser doesn't know about <<CPUID>>, and so doesn't expect
			; these registers to change.
			push eax
			push ebx
			push ecx
			push edx
#endif
			; <<CPUID>> 
			; eax = 0x80000002 --> eax, ebx, ecx, edx: get processor name string (part 1)
			mov eax,0x80000002
			CPUID_INSTRUCTION
			mov CPUExtendedIdentity[0 * TYPE int], eax
			mov CPUExtendedIdentity[1 * TYPE int], ebx
			mov CPUExtendedIdentity[2 * TYPE int], ecx
			mov CPUExtendedIdentity[3 * TYPE int], edx

			; <<CPUID>> 
			; eax = 0x80000003 --> eax, ebx, ecx, edx: get processor name string (part 2)
			mov eax,0x80000003
			CPUID_INSTRUCTION
			mov CPUExtendedIdentity[4 * TYPE int], eax
			mov CPUExtendedIdentity[5 * TYPE int], ebx
			mov CPUExtendedIdentity[6 * TYPE int], ecx
			mov CPUExtendedIdentity[7 * TYPE int], edx

			; <<CPUID>> 
			; eax = 0x80000004 --> eax, ebx, ecx, edx: get processor name string (part 3)
			mov eax,0x80000004
			CPUID_INSTRUCTION
			mov CPUExtendedIdentity[8 * TYPE int], eax
			mov CPUExtendedIdentity[9 * TYPE int], ebx
			mov CPUExtendedIdentity[10 * TYPE int], ecx
			mov CPUExtendedIdentity[11 * TYPE int], edx

#ifndef CPUID_AWARE_COMPILER
			pop edx
			pop ecx
			pop ebx
			pop eax
#endif
		}
	}

	// A generic catch-all just to be sure...
	catch (...) {
		return false;
	}

	// Process the returned information.
	memcpy (ChipID.ProcessorName, &(CPUExtendedIdentity[0]), sizeof (int));
	memcpy (&(ChipID.ProcessorName[4]), &(CPUExtendedIdentity[1]), sizeof (int));
	memcpy (&(ChipID.ProcessorName[8]), &(CPUExtendedIdentity[2]), sizeof (int));
	memcpy (&(ChipID.ProcessorName[12]), &(CPUExtendedIdentity[3]), sizeof (int));
	memcpy (&(ChipID.ProcessorName[16]), &(CPUExtendedIdentity[4]), sizeof (int));
	memcpy (&(ChipID.ProcessorName[20]), &(CPUExtendedIdentity[5]), sizeof (int));
	memcpy (&(ChipID.ProcessorName[24]), &(CPUExtendedIdentity[6]), sizeof (int));
	memcpy (&(ChipID.ProcessorName[28]), &(CPUExtendedIdentity[7]), sizeof (int));
	memcpy (&(ChipID.ProcessorName[32]), &(CPUExtendedIdentity[8]), sizeof (int));
	memcpy (&(ChipID.ProcessorName[36]), &(CPUExtendedIdentity[9]), sizeof (int));
	memcpy (&(ChipID.ProcessorName[40]), &(CPUExtendedIdentity[10]), sizeof (int));
	memcpy (&(ChipID.ProcessorName[44]), &(CPUExtendedIdentity[11]), sizeof (int));
	ChipID.ProcessorName[48] = '\0';

	// Because some manufacturers (<cough>Intel</cough>) have leading white space - we have to post-process the name.
	if (ChipManufacturer == Intel) {
		for (int nCounter = 0; nCounter < CHIPNAME_STRING_LENGTH; nCounter ++) {
			// There will either be NULL (\0) or spaces ( ) as the leading characters.
			if ((ChipID.ProcessorName[nCounter] != '\0') && (ChipID.ProcessorName[nCounter] != ' ')) {
				// We have found the starting position of the name.
				ProcessorNameStartPos = nCounter;
				
				// Terminate the loop.
				break;
			}
		}

		// Check to see if there is any white space at the start.
		if (ProcessorNameStartPos == 0) return true;

		// Now move the name forward so that there is no white space.
		memmove (ChipID.ProcessorName, &(ChipID.ProcessorName[ProcessorNameStartPos]), (CHIPNAME_STRING_LENGTH - ProcessorNameStartPos));
	}

	return true;
}

bool __cdecl CPUInfo::RetrieveClassicalCPUIdentity ()
{
	// Start by decided which manufacturer we are using....
	switch (ChipManufacturer) {
		case Intel:
			// Check the family / model / revision to determine the CPU ID.
			switch (ChipID.Family) {
				case 3:
					switch (ChipID.Type) {
						case 0: 
							if (ChipID.Revision == 0)
								STORE_CLASSICAL_NAME ("Intel386™ DX");
							else if (ChipID.Revision == 4)
								STORE_CLASSICAL_NAME ("RapidCAD® Co-Processor");
							break;						
						case 2: STORE_CLASSICAL_NAME ("Intel386™ SX/CX/EX"); break;
						case 4: STORE_CLASSICAL_NAME ("Intel386™ SL"); break;
						default: STORE_CLASSICAL_NAME ("Newer i80386 family"); break;
					}
					break;
				case 4:
					switch (ChipID.Model) {
						case 0: STORE_CLASSICAL_NAME ("i80486DX-25/33"); break;
						case 1: STORE_CLASSICAL_NAME ("i80486DX-50"); break;
						case 2: STORE_CLASSICAL_NAME ("i80486SX"); break;
						case 3: STORE_CLASSICAL_NAME ("i80486DX2"); break;
						case 4: STORE_CLASSICAL_NAME ("i80486SL"); break;
						case 5: STORE_CLASSICAL_NAME ("i80486SX2"); break;
						case 7: STORE_CLASSICAL_NAME ("i80486DX2 WriteBack"); break;
						case 8: STORE_CLASSICAL_NAME ("i80486DX4"); break;
						case 9: STORE_CLASSICAL_NAME ("i80486DX4 WriteBack"); break;
						default: STORE_CLASSICAL_NAME ("Unknown 80486 family"); return false;
					}
					break;
				case 5:
					switch (ChipID.Model) {
						case 0: STORE_CLASSICAL_NAME ("P5 A-Step"); break;
						case 1: STORE_CLASSICAL_NAME ("P5"); break;
						case 2: STORE_CLASSICAL_NAME ("P54C"); break;
						case 3: STORE_CLASSICAL_NAME ("P24T OverDrive"); break;
						case 4: STORE_CLASSICAL_NAME ("P55C"); break;
						case 7: STORE_CLASSICAL_NAME ("P54C"); break;
						case 8: STORE_CLASSICAL_NAME ("P55C (0.25µm)"); break;
						default: STORE_CLASSICAL_NAME ("Unknown Pentium® family"); return false;
					}
					break;
				case 6:
					switch (ChipID.Model) {
						case 0: STORE_CLASSICAL_NAME ("P6 A-Step"); break;
						case 1: STORE_CLASSICAL_NAME ("Pentium® Pro"); break;
						case 3: STORE_CLASSICAL_NAME ("Pentium® II (0.28 µm)"); break;
						case 5: 
							if (Features.L2CacheSize == 0) 
								STORE_CLASSICAL_NAME ("Celeron®");
							else 
								STORE_CLASSICAL_NAME ("Pentium® II (0.25 µm)");
							break;
						case 6: 
							if (Features.L2CacheSize == 128) 
								STORE_CLASSICAL_NAME ("Celeron® 'A'");
							else 
								STORE_CLASSICAL_NAME ("Pentium® II With On-Die L2 Cache");
							break;
						case 7: 
							if (Features.ExtendedFeatures.BrandID == 0x03)
								STORE_CLASSICAL_NAME ("Pentium® III Xeon"); 
							else if (Features.ExtendedFeatures.BrandID == 0x06)
								STORE_CLASSICAL_NAME ("Pentium® III (Mobile)"); 
							else
								STORE_CLASSICAL_NAME ("Pentium® III (0.25 µm)"); 
							break;
						case 8:
							if (Features.L2CacheSize == 128) 
								STORE_CLASSICAL_NAME ("Celeron® 'A' With SSE");
							else 
								if (Features.ExtendedFeatures.BrandID == 0x03)
									STORE_CLASSICAL_NAME ("Pentium® III Xeon"); 
								else if (Features.ExtendedFeatures.BrandID == 0x06)
									STORE_CLASSICAL_NAME ("Pentium® III (Mobile)"); 
								else
									STORE_CLASSICAL_NAME ("Pentium® III (0.18 µm) With 256 KB On-Die L2 Cache ");
							break;
						case 0xa: STORE_CLASSICAL_NAME ("Pentium® III Xeon (0.18 µm) With 1 Or 2 MB On-Die L2 Cache "); break;
						case 0xb: 
							if (Features.ExtendedFeatures.BrandID == 0x03)
								STORE_CLASSICAL_NAME ("Pentium® III Xeon"); 
							else if (Features.ExtendedFeatures.BrandID == 0x06)
								STORE_CLASSICAL_NAME ("Pentium® III (Mobile)"); 
							else
								STORE_CLASSICAL_NAME ("Pentium® III (0.13 µm) With 256 Or 512 KB On-Die L2 Cache "); 
							break;
						default: STORE_CLASSICAL_NAME ("Unknown P6 family"); return false;
					}
					break;
				case 7:
					STORE_CLASSICAL_NAME ("Intel Merced (IA-64)");
					break;
				case 0xf:
					// Check the extended family bits...
					switch (ChipID.ExtendedFamily) {
						case 0:
							switch (ChipID.Model) {
								case 0: STORE_CLASSICAL_NAME ("Pentium® IV (0.18 µm)"); break;
								case 1: STORE_CLASSICAL_NAME ("Pentium® IV (0.18 µm)"); break;
								case 2: STORE_CLASSICAL_NAME ("Pentium® IV (0.13 µm)"); break;
								default: STORE_CLASSICAL_NAME ("Unknown Pentium 4 family"); return false;
							}
							break;
						case 1:
							STORE_CLASSICAL_NAME ("Intel McKinley (IA-64)");
							break;
					}
					break;
				default:
					STORE_CLASSICAL_NAME ("Unknown Intel family");
					return false;
			}
			break;

		case AMD:
			// Check the family / model / revision to determine the CPU ID.
			switch (ChipID.Family) {
				case 4:
					switch (ChipID.Model) {
						case 3: STORE_CLASSICAL_NAME ("80486DX2"); break;
						case 7: STORE_CLASSICAL_NAME ("80486DX2 WriteBack"); break;
						case 8: STORE_CLASSICAL_NAME ("80486DX4"); break;
						case 9: STORE_CLASSICAL_NAME ("80486DX4 WriteBack"); break;
						case 0xe: STORE_CLASSICAL_NAME ("5x86"); break;
						case 0xf: STORE_CLASSICAL_NAME ("5x86WB"); break;
						default: STORE_CLASSICAL_NAME ("Unknown 80486 family"); return false;
					}
					break;
				case 5:
					switch (ChipID.Model) {
						case 0: STORE_CLASSICAL_NAME ("SSA5 (PR75, PR90, PR100)"); break;
						case 1: STORE_CLASSICAL_NAME ("5k86 (PR120, PR133)"); break;
						case 2: STORE_CLASSICAL_NAME ("5k86 (PR166)"); break;
						case 3: STORE_CLASSICAL_NAME ("5k86 (PR200)"); break;
						case 6: STORE_CLASSICAL_NAME ("K6 (0.30 µm)"); break;
						case 7: STORE_CLASSICAL_NAME ("K6 (0.25 µm)"); break;
						case 8: STORE_CLASSICAL_NAME ("K6-2"); break;
						case 9: STORE_CLASSICAL_NAME ("K6-III"); break;
						case 0xd: STORE_CLASSICAL_NAME ("K6-2+ or K6-III+ (0.18 µm)"); break;
						default: STORE_CLASSICAL_NAME ("Unknown 80586 family"); return false;
					}
					break;
				case 6:
					switch (ChipID.Model) {
						case 1: STORE_CLASSICAL_NAME ("Athlon™ (0.25 µm)"); break;
						case 2: STORE_CLASSICAL_NAME ("Athlon™ (0.18 µm)"); break;
						case 3: STORE_CLASSICAL_NAME ("Duron™ (SF core)"); break;
						case 4: STORE_CLASSICAL_NAME ("Athlon™ (Thunderbird core)"); break;
						case 6: 
							// AMD Athlon MP Model 6
							// AMD Athlon XP Model 6
							// Mobile AMD Athlon 4 Model 6
							// AMD Duron Model 6
							// Mobile AMD Duron Model 6
							if (Features.L2CacheSize <= 192) STORE_CLASSICAL_NAME ("Duron™");
							else STORE_CLASSICAL_NAME ("Athlon™ (Palomino core)"); 
							break;
						case 7: 
							// AMD Duron Model 7
							// Mobile AMD Duron Model 7
							STORE_CLASSICAL_NAME ("Duron™ (Morgan core)"); 
							break;
						case 8:
							// AMD Athlon XP Model 8
							// AMD Athlon MP Model 8
							if (Features.ExtendedFeatures.SupportsMP)
								STORE_CLASSICAL_NAME ("Athlon™ MP (Thoroughbred 'A' core)"); 
							else STORE_CLASSICAL_NAME ("Athlon™ XP (Thoroughbred 'A' core)");
							break;
						case 0xa:
							// Mobile AMD Athlon XP–M Model 8
							// Mobile AMD Athlon XP–M (LV) Model 8
							// AMD Athlon XP Model 10
							// AMD Athlon MP Model 10
							// Mobile AMD Athlon XP–M Model 10
							// Mobile AMD Athlon XP–M (LV) Model 10
							if (Features.ExtendedFeatures.SupportsMP)
								STORE_CLASSICAL_NAME ("Athlon™ MP (Thoroughbred 'B' core)"); 
							else STORE_CLASSICAL_NAME ("Athlon™ XP (Thoroughbred 'B' core)");
							break;
						default: STORE_CLASSICAL_NAME ("Unknown K7 family"); return false;
					}
					break;
				case 0xf:
					switch (ChipID.Model) {
						case 5: STORE_CLASSICAL_NAME ("Opteron™"); break;
						default: STORE_CLASSICAL_NAME ("Unknown AMD-64 family"); return false;
					}
					break;
				default:
					STORE_CLASSICAL_NAME ("Unknown AMD family");
					return false;
			}
			break;

		case Transmeta:
			switch (ChipID.Family) {	
				case 5:
					switch (ChipID.Model) {
						case 4: STORE_CLASSICAL_NAME ("Crusoe TM3x00 and TM5x00"); break;
						default: STORE_CLASSICAL_NAME ("Unknown Crusoe family"); return false;
					}
					break;
				default:
					STORE_CLASSICAL_NAME ("Unknown Transmeta family");
					return false;
			}
			break;

		case Rise:
			switch (ChipID.Family) {	
				case 5:
					switch (ChipID.Model) {
						case 0: STORE_CLASSICAL_NAME ("mP6 (0.25 µm)"); break;
						case 2: STORE_CLASSICAL_NAME ("mP6 (0.18 µm)"); break;
						default: STORE_CLASSICAL_NAME ("Unknown Rise family"); return false;
					}
					break;
				default:
					STORE_CLASSICAL_NAME ("Unknown Rise family");
					return false;
			}
			break;

		case UMC:
			switch (ChipID.Family) {	
				case 4:
					switch (ChipID.Model) {
						case 1: STORE_CLASSICAL_NAME ("U5D"); break;
						case 2: STORE_CLASSICAL_NAME ("U5S"); break;
						default: STORE_CLASSICAL_NAME ("Unknown UMC family"); return false;
					}
					break;
				default:
					STORE_CLASSICAL_NAME ("Unknown UMC family");
					return false;
			}
			break;

		case IDT:
			switch (ChipID.Family) {	
				case 5:
					switch (ChipID.Model) {
						case 4: STORE_CLASSICAL_NAME ("C6"); break;
						case 8: STORE_CLASSICAL_NAME ("C2"); break;
						case 9: STORE_CLASSICAL_NAME ("C3"); break;
						default: STORE_CLASSICAL_NAME ("Unknown IDT\\Centaur family"); return false;
					}
					break;
				case 6:
					switch (ChipID.Model) {
						case 6: STORE_CLASSICAL_NAME ("VIA Cyrix III - Samuel"); break;
						default: STORE_CLASSICAL_NAME ("Unknown IDT\\Centaur family"); return false;
					}
					break;
				default:
					STORE_CLASSICAL_NAME ("Unknown IDT\\Centaur family");
					return false;
			}
			break;

		case Cyrix:
			switch (ChipID.Family) {	
				case 4:
					switch (ChipID.Model) {
						case 4: STORE_CLASSICAL_NAME ("MediaGX GX, GXm"); break;
						case 9: STORE_CLASSICAL_NAME ("5x86"); break;
						default: STORE_CLASSICAL_NAME ("Unknown Cx5x86 family"); return false;
					}
					break;
				case 5:
					switch (ChipID.Model) {
						case 2: STORE_CLASSICAL_NAME ("Cx6x86"); break;
						case 4: STORE_CLASSICAL_NAME ("MediaGX GXm"); break;
						default: STORE_CLASSICAL_NAME ("Unknown Cx6x86 family"); return false;
					}
					break;
				case 6:
					switch (ChipID.Model) {
						case 0: STORE_CLASSICAL_NAME ("6x86MX"); break;
						case 5: STORE_CLASSICAL_NAME ("Cyrix M2 Core"); break;
						case 6: STORE_CLASSICAL_NAME ("WinChip C5A Core"); break;
						case 7: STORE_CLASSICAL_NAME ("WinChip C5B\\C5C Core"); break;
						case 8: STORE_CLASSICAL_NAME ("WinChip C5C-T Core"); break;
						default: STORE_CLASSICAL_NAME ("Unknown 6x86MX\\Cyrix III family"); return false;
					}
					break;
				default:
					STORE_CLASSICAL_NAME ("Unknown Cyrix family");
					return false;
			}
			break;

		case NexGen:
			switch (ChipID.Family) {	
				case 5:
					switch (ChipID.Model) {
						case 0: STORE_CLASSICAL_NAME ("Nx586 or Nx586FPU"); break;
						default: STORE_CLASSICAL_NAME ("Unknown NexGen family"); return false;
					}
					break;
				default:
					STORE_CLASSICAL_NAME ("Unknown NexGen family");
					return false;
			}
			break;

		case NSC:
			STORE_CLASSICAL_NAME ("Cx486SLC \\ DLC \\ Cx486S A-Step");
			break;

		default:
			// We cannot identify the processor.
			STORE_CLASSICAL_NAME ("Unknown processor family");
			return false;
	}

	return true;
}

// --------------------------------------------------------
//
//         Constructor Functions - CPUSpeed Class
//
// --------------------------------------------------------

CPUSpeed::CPUSpeed ()
{
	unsigned int uiRepetitions = 1;
	unsigned int uiMSecPerRepetition = 50;
	__int64	i64Total = 0, i64Overhead = 0;

	for (unsigned int nCounter = 0; nCounter < uiRepetitions; nCounter ++) {
		i64Total += GetCyclesDifference (CPUSpeed::Delay, uiMSecPerRepetition);
		i64Overhead += GetCyclesDifference (CPUSpeed::DelayOverhead, uiMSecPerRepetition);
	}

	// Calculate the MHz speed.
	i64Total -= i64Overhead;
	i64Total /= uiRepetitions;
	i64Total /= uiMSecPerRepetition;
	i64Total /= 1000;

	// Save the CPU speed.
	CPUSpeedInMHz = (int) i64Total;
}

CPUSpeed::~CPUSpeed ()
{
}

__int64	__cdecl CPUSpeed::GetCyclesDifference (DELAY_FUNC DelayFunction, unsigned int uiParameter)
{
	unsigned int edx1, eax1;
	unsigned int edx2, eax2;
		
	// Calculate the frequency of the CPU instructions.
	try {
		_asm {
			push uiParameter		; push parameter param
			mov ebx, DelayFunction	; store func in ebx

			RDTSC_INSTRUCTION

			mov esi, eax			; esi = eax
			mov edi, edx			; edi = edx

			call ebx				; call the delay functions

			RDTSC_INSTRUCTION

			pop ebx

			mov edx2, edx			; edx2 = edx
			mov eax2, eax			; eax2 = eax

			mov edx1, edi			; edx2 = edi
			mov eax1, esi			; eax2 = esi
		}
	}

	// A generic catch-all just to be sure...
	catch (...) {
		return -1;
	}

	return (CPUSPEED_I32TO64 (edx2, eax2) - CPUSPEED_I32TO64 (edx1, eax1));
}

void CPUSpeed::Delay (unsigned int uiMS)
{
	LARGE_INTEGER Frequency, StartCounter, EndCounter;
	__int64 x;

	// Get the frequency of the high performance counter.
	if (!QueryPerformanceFrequency (&Frequency)) return;
	x = Frequency.QuadPart / 1000 * uiMS;

	// Get the starting position of the counter.
	QueryPerformanceCounter (&StartCounter);

	do {
		// Get the ending position of the counter.	
		QueryPerformanceCounter (&EndCounter);
	} while (EndCounter.QuadPart - StartCounter.QuadPart < x);
}

void CPUSpeed::DelayOverhead (unsigned int uiMS)
{
	LARGE_INTEGER Frequency, StartCounter, EndCounter;
	__int64 x;

	// Get the frequency of the high performance counter.
	if (!QueryPerformanceFrequency (&Frequency)) return;
	x = Frequency.QuadPart / 1000 * uiMS;

	// Get the starting position of the counter.
	QueryPerformanceCounter (&StartCounter);
	
	do {
		// Get the ending position of the counter.	
		QueryPerformanceCounter (&EndCounter);
	} while (EndCounter.QuadPart - StartCounter.QuadPart == x);
}