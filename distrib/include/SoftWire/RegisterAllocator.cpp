#include "RegisterAllocator.hpp"

#include "Error.hpp"

namespace SoftWire
{
	RegisterAllocator::Allocation RegisterAllocator::ERX[8];
	RegisterAllocator::Allocation RegisterAllocator::MM[8];
	RegisterAllocator::Allocation RegisterAllocator::XMM[8];

	bool RegisterAllocator::autoEMMS = false;
	bool RegisterAllocator::copyPropagation = true;
	bool RegisterAllocator::loadElimination = true;
	bool RegisterAllocator::spillElimination = true;
	bool RegisterAllocator::minimalRestore = true;
	bool RegisterAllocator::dropUnmodified = true;

	RegisterAllocator::RegisterAllocator()
	{
		// Completely eraze allocation state
		for(int i = 0; i < 8; i++)
		{
			ERX[i].free();
			MM[i].free();
			XMM[i].free();
		}
	}

	RegisterAllocator::~RegisterAllocator()
	{
		// Completely eraze allocation state
		for(int i = 0; i < 8; i++)
		{
			ERX[i].free();
			MM[i].free();
			XMM[i].free();
		}
	}

	const OperandREG8 RegisterAllocator::r8(const OperandREF &ref, bool copy)
	{
		OperandREG32 reg = r32(ref, copy);

		// Make sure we only have al, cl, dl or bl
		if(reg.reg >= 4)
		{
			spill(reg);

			// Need to spill one of al, cl, dl or bl
			int candidate = 0;
			unsigned int priority = 0xFFFFFFFF;

			for(int i = 0; i < 4; i++)
			{
				if(ERX[i].priority < priority)
				{
					priority = ERX[i].priority;
					candidate = i;
				}
			}

			spill(OperandREG32(candidate));

			return (OperandREG8)allocate32(candidate, ref, copy, 1);
		}

		return (OperandREG8)reg;
	}

	const OperandR_M8 RegisterAllocator::m8(const OperandREF &ref)
	{
		return (OperandR_M8)m32(ref, 1);
	}

	const OperandREG16 RegisterAllocator::r16(const OperandREF &ref, bool copy)
	{
		return (OperandREG16)r32(ref, copy, 2);
	}

	const OperandR_M16 RegisterAllocator::m16(const OperandREF &ref)
	{
		return (OperandR_M16)m32(ref, 2);
	}

	OperandREG32 RegisterAllocator::r32(const OperandREF &ref, bool copy, int partial)
	{
		if(ref == 0 && copy) throw Error("Cannot dereference 0");

		// Check if already allocated
		for(int i = 0; i < 8; i++)
		{
			if(i == Encoding::ESP || i == Encoding::EBP) continue;

			if(ERX[i].reference == ref)
			{
				return prioritize32(i);
			}
		}

		// Check spilled but unused registers
		if(spillElimination)
		{
			for(int i = 0; i < 8; i++)
			{
				if(i == Encoding::ESP || i == Encoding::EBP) continue;

				if(ERX[i].priority == 0 && ERX[i].spill.reference == ref)
				{
					if(ERX[i].spillInstruction)
					{
						ERX[i].spillInstruction->reserve();
					}

					ERX[i].reference = ERX[i].spill.reference;
					ERX[i].partial = ERX[i].spill.partial;
					ERX[i].priority = ERX[i].spill.priority;
					ERX[i].copyInstruction = ERX[i].spill.copyInstruction;
					ERX[i].loadInstruction = ERX[i].spill.loadInstruction;
					ERX[i].spillInstruction = ERX[i].spill.spillInstruction;

					ERX[i].spill.free();

					return prioritize32(i);
				}
			}
		}

		// Search for free registers
		for(int i = 0; i < 8; i++)
		{
			if(i == Encoding::ESP || i == Encoding::EBP) continue;

			if(ERX[i].priority == 0 && ERX[i].spill.priority == 0)
			{
				return allocate32(i, ref, copy, partial);
			}
		}
		
		for(int i = 0; i < 8; i++)
		{
			if(i == Encoding::ESP || i == Encoding::EBP) continue;

			if(ERX[i].priority == 0)
			{
				return allocate32(i, ref, copy, partial);
			}
		}

		// Need to spill one
		int candidate = 0;
		int betterCandidate = -1;
		unsigned int priority = 0xFFFFFFFF;

		for(int i = 0; i < 8; i++)
		{
			if(i == Encoding::ESP || i == Encoding::EBP) continue;

			if(ERX[i].priority < priority)
			{
				priority = ERX[i].priority;
				candidate = i;

				if(!ERX[i].modified && ERX[i].priority < 0xFFFFFFFF - 2)
				{
					betterCandidate = i;
				}
			}
		}

		if(betterCandidate != -1)
		{
			candidate = betterCandidate;
		}

		Encoding *spillInstruction = spill32(candidate);

		ERX[candidate].spill.reference = ERX[candidate].reference;
		ERX[candidate].spill.priority = ERX[candidate].priority;
		ERX[candidate].spill.partial = ERX[candidate].partial;
		ERX[candidate].spill.copyInstruction = ERX[candidate].copyInstruction;
		ERX[candidate].spill.loadInstruction = ERX[candidate].loadInstruction;
		ERX[candidate].spill.spillInstruction = ERX[candidate].spillInstruction;

		ERX[candidate].reference = 0;
		ERX[candidate].priority = 0;
		ERX[candidate].partial = 0;
		ERX[candidate].copyInstruction = 0;
		ERX[candidate].loadInstruction = 0;
		ERX[candidate].spillInstruction = spillInstruction;

		return allocate32(candidate, ref, copy, partial);
	}

	OperandR_M32 RegisterAllocator::m32(const OperandREF &ref, int partial)
	{
		if(ref == 0) throw Error("Cannot dereference 0");

		// Check if already allocated
		for(int i = 0; i < 8; i++)
		{
			if(i == Encoding::ESP || i == Encoding::EBP) continue;

			if(ERX[i].reference == ref)
			{
				return prioritize32(i);
			}
		}

		// Check spilled but unused registers
		if(spillElimination)
		{
			for(int i = 0; i < 8; i++)
			{
				if(i == Encoding::ESP || i == Encoding::EBP) continue;

				if(ERX[i].priority == 0 && ERX[i].spill.reference == ref)
				{
					if(ERX[i].spillInstruction)
					{
						ERX[i].spillInstruction->reserve();
					}

					ERX[i].reference = ERX[i].spill.reference;
					ERX[i].partial = ERX[i].spill.partial;
					ERX[i].priority = ERX[i].spill.priority;
					ERX[i].copyInstruction = ERX[i].spill.copyInstruction;
					ERX[i].loadInstruction = ERX[i].spill.loadInstruction;
					ERX[i].spillInstruction = ERX[i].spill.spillInstruction;

					ERX[i].spill.free();

					return prioritize32(i);
				}
			}
		}

		return (OperandR_M32)dword_ptr [ref];
	}

	OperandREG32 RegisterAllocator::allocate32(int i, const OperandREF &ref, bool copy, int partial)
	{
		ERX[i].reference = ref;
		ERX[i].partial = partial;

		prioritize32(i);

		Encoding *loadInstruction = 0;
		Encoding *spillInstruction = ERX[i].spillInstruction;
		AllocationData spillAllocation = ERX[i].spill;

		if(copy)
		{
			     if(partial == 1) loadInstruction = mov(OperandREG8(i), byte_ptr [ref]);
			else if(partial == 2) loadInstruction = mov(OperandREG16(i), word_ptr [ref]); 
			else                  loadInstruction = mov(OperandREG32(i), dword_ptr [ref]);
		}

		ERX[i].loadInstruction = loadInstruction;
		ERX[i].spillInstruction = spillInstruction;
		ERX[i].spill = spillAllocation;
		ERX[i].modified = false;

		return OperandREG32(i);
	}

	OperandREG32 RegisterAllocator::prioritize32(int i)
	{
		// Give highest priority
		ERX[i].priority = 0xFFFFFFFF;

		// Decrease priority of other registers
		for(int j = 0; j < 8; j++)
		{
			if(i == Encoding::ESP || i == Encoding::EBP) continue;

			if(j != i && ERX[j].priority)
			{
				ERX[j].priority--;
			}
		}

		return OperandREG32(i);
	}

	void RegisterAllocator::free32(int i)
	{
		if(ERX[i].loadInstruction && loadElimination)
		{
			ERX[i].loadInstruction->reserve();
			ERX[i].loadInstruction = 0;
		}

		if(ERX[i].copyInstruction && copyPropagation)
		{
			ERX[i].copyInstruction->reserve();
			ERX[i].copyInstruction = 0;
		}

		ERX[i].reference = 0;
		ERX[i].partial = 0;
		ERX[i].priority = 0;
	}

	Encoding *RegisterAllocator::spill32(int i)
	{
		// Register loaded but not used, eliminate load and don't spill
		if(ERX[i].loadInstruction && loadElimination)
		{
			ERX[i].loadInstruction->reserve();
			ERX[i].loadInstruction = 0;
	
			ERX[i].reference = 0;
			ERX[i].priority = 0;
			ERX[i].partial = 0;
			ERX[i].copyInstruction = 0;
			ERX[i].loadInstruction = 0;
		//	ERX[i].spillInstruction = 0;   // NOTE: Keep previous spill info
	
			return 0;
		}

		Encoding *spillInstruction = 0;

		if(ERX[i].reference != 0 && (ERX[i].modified || !dropUnmodified))
		{
			     if(ERX[i].partial == 1) spillInstruction = mov(byte_ptr [ERX[i].reference], OperandREG8(i));
			else if(ERX[i].partial == 2) spillInstruction = mov(word_ptr [ERX[i].reference], OperandREG16(i));
			else                         spillInstruction = mov(dword_ptr [ERX[i].reference], OperandREG32(i));
		}

		ERX[i].free();

		return spillInstruction;
	}

	void RegisterAllocator::free(const OperandREG32 &r32)
	{
		free32(r32.reg);
	}

	void RegisterAllocator::spill(const OperandREG32 &r32)
	{
		spill32(r32.reg);
	}

	OperandMMREG RegisterAllocator::r64(const OperandREF &ref, bool copy)
	{
		if(ref == 0 && copy) throw Error("Cannot dereference 0");

		// Check if already allocated
		for(int i = 0; i < 8; i++)
		{
			if(MM[i].reference == ref)
			{
				return prioritize64(i);
			}
		}

		// Check spilled but unused registers
		if(spillElimination)
		{
			for(int i = 0; i < 8; i++)
			{
				if(MM[i].priority == 0 && MM[i].spill.reference == ref)
				{
					if(MM[i].spillInstruction)
					{
						MM[i].spillInstruction->reserve();
					}

					MM[i].reference = MM[i].spill.reference;
					MM[i].partial = MM[i].spill.partial;
					MM[i].priority = MM[i].spill.priority;
					MM[i].copyInstruction = MM[i].spill.copyInstruction;
					MM[i].loadInstruction = MM[i].spill.loadInstruction;
					MM[i].spillInstruction = MM[i].spill.spillInstruction;

					MM[i].spill.free();

					return prioritize64(i);
				}
			}
		}

		// Search for free registers
		for(int i = 0; i < 8; i++)
		{
			if(MM[i].priority == 0 && MM[i].spill.priority == 0)
			{
				return allocate64(i, ref, copy);
			}
		}

		for(int i = 0; i < 8; i++)
		{
			if(MM[i].priority == 0)
			{
				return allocate64(i, ref, copy);
			}
		}

		// Need to spill one
		int candidate = 0;
		int betterCandidate = -1;
		unsigned int priority = 0xFFFFFFFF;

		for(int i = 0; i < 8; i++)
		{
			if(MM[i].priority < priority)
			{
				priority = MM[i].priority;
				candidate = i;

				if(!MM[i].modified && MM[i].priority < 0xFFFFFFFF - 2)
				{
					betterCandidate = i;
				}
			}
		}

		if(betterCandidate != -1)
		{
			candidate = betterCandidate;
		}

		Encoding *spillInstruction = spill64(candidate);

		MM[candidate].spill.reference = MM[candidate].reference;
		MM[candidate].spill.priority = MM[candidate].priority;
		MM[candidate].spill.partial = MM[candidate].partial;
		MM[candidate].spill.copyInstruction = MM[candidate].copyInstruction;
		MM[candidate].spill.loadInstruction = MM[candidate].loadInstruction;
		MM[candidate].spill.spillInstruction = MM[candidate].spillInstruction;

		MM[candidate].reference = 0;
		MM[candidate].priority = 0;
		MM[candidate].partial = 0;
		MM[candidate].copyInstruction = 0;
		MM[candidate].loadInstruction = 0;
		MM[candidate].spillInstruction = spillInstruction;

		return allocate64(candidate, ref, copy);
	}

	OperandR_M64 RegisterAllocator::m64(const OperandREF &ref)
	{
		if(ref == 0) throw Error("Cannot dereference 0");

		// Check if already allocated
		for(int i = 0; i < 8; i++)
		{
			if(MM[i].reference == ref)
			{
				return prioritize64(i);
			}
		}

		// Check spilled but unused registers
		if(spillElimination)
		{
			for(int i = 0; i < 8; i++)
			{
				if(MM[i].priority == 0 && MM[i].spill.reference == ref)
				{
					if(MM[i].spillInstruction)
					{
						MM[i].spillInstruction->reserve();
					}

					MM[i].reference = MM[i].spill.reference;
					MM[i].partial = MM[i].spill.partial;
					MM[i].priority = MM[i].spill.priority;
					MM[i].copyInstruction = MM[i].spill.copyInstruction;
					MM[i].loadInstruction = MM[i].spill.loadInstruction;
					MM[i].spillInstruction = MM[i].spill.spillInstruction;

					MM[i].spill.free();

					return prioritize64(i);
				}
			}
		}

		return (OperandR_M64)qword_ptr [ref];
	}

	OperandMMREG RegisterAllocator::allocate64(int i, const OperandREF &ref, bool copy)
	{
		MM[i].reference = ref;

		prioritize64(i);

		Encoding *loadInstruction = 0;
		Encoding *spillInstruction = MM[i].spillInstruction;
		AllocationData spillAllocation = MM[i].spill;

		if(copy)
		{
			loadInstruction = movq(OperandMMREG(i), qword_ptr [ref]);
		}

		MM[i].loadInstruction = loadInstruction;
		MM[i].spillInstruction = spillInstruction;
		MM[i].spill = spillAllocation;
		MM[i].modified = false;

		return OperandMMREG(i);
	}

	OperandMMREG RegisterAllocator::prioritize64(int i)
	{
		// Give highest priority
		MM[i].priority = 0xFFFFFFFF;

		// Decrease priority of other registers
		for(int j = 0; j < 8; j++)
		{
			if(j != i && MM[j].priority)
			{
				MM[j].priority--;
			}
		}

		return OperandMMREG(i);
	}

	void RegisterAllocator::free64(int i)
	{
		bool free = (MM[i].priority != 0);

		if(MM[i].loadInstruction && loadElimination)
		{
			MM[i].loadInstruction->reserve();
			MM[i].loadInstruction = 0;
		}

		if(MM[i].copyInstruction && copyPropagation)
		{
			MM[i].copyInstruction->reserve();
			MM[i].copyInstruction = 0;
		}

		MM[i].reference = 0;
		MM[i].partial = 0;
		MM[i].priority = 0;

		if(free && autoEMMS)
		{
			for(int i = 0; i < 8; i++)
			{
				if(MM[i].priority != 0)
				{
					return;
				}
			}

			// Last one freed
			emms();

			// Completely eraze MMX allocation state
			for(int i = 0; i < 8; i++)
			{
				MM[i].free();
			}
		}
	}

	Encoding *RegisterAllocator::spill64(int i)
	{
		// Register loaded but not used, eliminate load and don't spill
		if(MM[i].loadInstruction && loadElimination)
		{
			MM[i].loadInstruction->reserve();
			MM[i].loadInstruction = 0;
	
			MM[i].reference = 0;
			MM[i].priority = 0;
			MM[i].partial = 0;
			MM[i].copyInstruction = 0;
			MM[i].loadInstruction = 0;
		//	MM[i].spillInstruction = 0;   // NOTE: Keep previous spill info
	
			return 0;
		}

		Encoding *spillInstruction = 0;

		if(MM[i].reference != 0 && (MM[i].modified || !dropUnmodified))
		{
			spillInstruction = movq(qword_ptr [MM[i].reference], OperandMMREG(i));
		}

		MM[i].free();

		return spillInstruction;
	}

	void RegisterAllocator::free(const OperandMMREG &r64)
	{
		free64(r64.reg);
	}

	void RegisterAllocator::spill(const OperandMMREG &r64)
	{
		spill64(r64.reg);
	}

	OperandXMMREG RegisterAllocator::r128(const OperandREF &ref, bool copy, bool ss)
	{
		if(ref == 0 && copy) throw Error("Cannot dereference 0");

		// Check if already allocated
		for(int i = 0; i < 8; i++)
		{
			if(XMM[i].reference == ref)
			{
				return prioritize128(i);
			}
		}

		// Check spilled but unused registers
		if(spillElimination)
		{
			for(int i = 0; i < 8; i++)
			{
				if(XMM[i].priority == 0 && XMM[i].spill.reference == ref)
				{
					if(XMM[i].spillInstruction)
					{
						XMM[i].spillInstruction->reserve();
					}

					XMM[i].reference = XMM[i].spill.reference;
					XMM[i].partial = XMM[i].spill.partial;
					XMM[i].priority = XMM[i].spill.priority;
					XMM[i].copyInstruction = XMM[i].spill.copyInstruction;
					XMM[i].loadInstruction = XMM[i].spill.loadInstruction;
					XMM[i].spillInstruction = XMM[i].spill.spillInstruction;

					XMM[i].spill.free();

					return prioritize128(i);
				}
			}
		}

		// Search for free registers
		for(int i = 0; i < 8; i++)
		{
			if(XMM[i].priority == 0 && XMM[i].spill.priority == 0)
			{
				return allocate128(i, ref, copy, ss);
			}
		}
		
		for(int i = 0; i < 8; i++)
		{
			if(XMM[i].priority == 0)
			{
				return allocate128(i, ref, copy, ss);
			}
		}

		// Need to spill one
		int candidate = 0;
		int betterCandidate = -1;
		unsigned int priority = 0xFFFFFFFF;

		for(int i = 0; i < 8; i++)
		{
			if(XMM[i].priority < priority)
			{
				priority = XMM[i].priority;
				candidate = i;

				if(!XMM[i].modified && XMM[i].priority < 0xFFFFFFFF - 2)
				{
					betterCandidate = i;
				}
			}
		}

		if(betterCandidate != -1)
		{
			candidate = betterCandidate;
		}

		Encoding *spillInstruction = spill128(candidate);

		XMM[candidate].spill.reference = XMM[candidate].reference;
		XMM[candidate].spill.priority = XMM[candidate].priority;
		XMM[candidate].spill.partial = XMM[candidate].partial;
		XMM[candidate].spill.copyInstruction = XMM[candidate].copyInstruction;
		XMM[candidate].spill.loadInstruction = XMM[candidate].loadInstruction;
		XMM[candidate].spill.spillInstruction = XMM[candidate].spillInstruction;

		XMM[candidate].reference = 0;
		XMM[candidate].priority = 0;
		XMM[candidate].partial = 0;
		XMM[candidate].copyInstruction = 0;
		XMM[candidate].loadInstruction = 0;
		XMM[candidate].spillInstruction = spillInstruction;

		return allocate128(candidate, ref, copy, ss);
	}

	OperandR_M128 RegisterAllocator::m128(const OperandREF &ref, bool ss)
	{
		if(ref == 0) throw Error("Cannot dereference 0");

		// Check if already allocated
		for(int i = 0; i < 8; i++)
		{
			if(XMM[i].reference == ref)
			{
				return prioritize128(i);
			}
		}

		// Check spilled but unused registers
		if(spillElimination)
		{
			for(int i = 0; i < 8; i++)
			{
				if(XMM[i].priority == 0 && XMM[i].spill.reference == ref)
				{
					if(XMM[i].spillInstruction)
					{
						XMM[i].spillInstruction->reserve();
					}

					XMM[i].reference = XMM[i].spill.reference;
					XMM[i].partial = XMM[i].spill.partial;
					XMM[i].priority = XMM[i].spill.priority;
					XMM[i].copyInstruction = XMM[i].spill.copyInstruction;
					XMM[i].loadInstruction = XMM[i].spill.loadInstruction;
					XMM[i].spillInstruction = XMM[i].spill.spillInstruction;

					XMM[i].spill.free();

					return prioritize128(i);
				}
			}
		}

		return (OperandR_M128)xword_ptr [ref];
	}

	OperandXMMREG RegisterAllocator::allocate128(int i, const OperandREF &ref, bool copy, bool ss)
	{
		XMM[i].reference = ref;
		XMM[i].partial = ss ? 4 : 0;

		prioritize128(i);

		Encoding *loadInstruction = 0;
		Encoding *spillInstruction = XMM[i].spillInstruction;
		AllocationData spillAllocation = XMM[i].spill;

		if(copy)
		{
			if(ss) loadInstruction = movss(OperandXMMREG(i), dword_ptr [ref]);
			else   loadInstruction = movaps(OperandXMMREG(i), xword_ptr [ref]);
		}

		XMM[i].loadInstruction = loadInstruction;
		XMM[i].spillInstruction = spillInstruction;
		XMM[i].spill = spillAllocation;
		XMM[i].modified = false;

		return OperandXMMREG(i);
	}

	OperandXMMREG RegisterAllocator::prioritize128(int i)
	{
		// Give highest priority
		XMM[i].priority = 0xFFFFFFFF;

		// Decrease priority of other registers
		for(int j = 0; j < 8; j++)
		{
			if(j != i && XMM[j].priority)
			{
				XMM[j].priority--;
			}
		}

		return OperandXMMREG(i);
	}

	void RegisterAllocator::free128(int i)
	{
		if(XMM[i].loadInstruction && loadElimination)
		{
			XMM[i].loadInstruction->reserve();
			XMM[i].loadInstruction = 0;
		}

		if(XMM[i].copyInstruction && copyPropagation)
		{
			XMM[i].copyInstruction->reserve();
			XMM[i].copyInstruction = 0;
		}

		XMM[i].reference = 0;
		XMM[i].partial = 0;
		XMM[i].priority = 0;
	}

	Encoding *RegisterAllocator::spill128(int i)
	{
		// Register loaded but not used, eliminate load and don't spill
		if(XMM[i].loadInstruction && loadElimination)
		{
			XMM[i].loadInstruction->reserve();
			XMM[i].loadInstruction = 0;
	
			XMM[i].reference = 0;
			XMM[i].priority = 0;
			XMM[i].partial = 0;
			XMM[i].copyInstruction = 0;
			XMM[i].loadInstruction = 0;
		//	XMM[i].spillInstruction = 0;   // NOTE: Keep previous spill info
	
			return 0;
		}

		Encoding *spillInstruction = 0;

		if(XMM[i].reference != 0 && (XMM[i].modified || !dropUnmodified))
		{
			if(XMM[i].partial) spillInstruction = movss(dword_ptr [XMM[i].reference], OperandXMMREG(i));
			else               spillInstruction = movaps(xword_ptr [XMM[i].reference], OperandXMMREG(i));
		}
		
		XMM[i].free();

		return spillInstruction;
	}

	void RegisterAllocator::free(const OperandXMMREG &r128)
	{
		free128(r128.reg);
	}

	void RegisterAllocator::spill(const OperandXMMREG &r128)
	{
		spill128(r128.reg);
	}

	OperandXMMREG RegisterAllocator::rSS(const OperandREF &ref, bool copy, bool ss)
	{
		return r128(ref, copy, ss);
	}

	OperandXMM32 RegisterAllocator::mSS(const OperandREF &ref, bool ss)
	{
		return (OperandXMM32)m128(ref, ss);
	}

	void RegisterAllocator::free(const OperandREF &ref)
	{
		for(int i = 0; i < 8; i++)
		{
			if(i == Encoding::ESP || i == Encoding::EBP) continue;

			if(ERX[i].reference == ref)
			{
				free32(i);
			}
		}

		for(int i = 0; i < 8; i++)
		{
			if(MM[i].reference == ref)
			{
				free64(i);
			}
		}

		for(int i = 0; i < 8; i++)
		{
			if(XMM[i].reference == ref)
			{
				free128(i);
			}
		}
	}

	void RegisterAllocator::spill(const OperandREF &ref)
	{
		for(int i = 0; i < 8; i++)
		{
			if(i == Encoding::ESP || i == Encoding::EBP) continue;

			if(ERX[i].reference == ref)
			{
				spill32(i);
			}
		}

		for(int i = 0; i < 8; i++)
		{
			if(MM[i].reference == ref)
			{
				spill64(i);
			}
		}

		for(int i = 0; i < 8; i++)
		{
			if(XMM[i].reference == ref)
			{
				spill128(i);
			}
		}
	}

	void RegisterAllocator::freeAll()
	{
		for(int i = 0; i < 8; i++)
		{
			if(i == Encoding::ESP || i == Encoding::EBP) continue;

			free32(i);
		}

		for(int i = 0; i < 8; i++)
		{
			free64(i);
		}

		for(int i = 0; i < 8; i++)
		{
			free128(i);
		}
	}

	void RegisterAllocator::spillAll()
	{
		for(int i = 0; i < 8; i++)
		{
			// Prevent optimizations
			markModified(OperandREG32(i));
			markModified(OperandMMREG(i));
			markModified(OperandXMMREG(i));
		}

		for(int i = 0; i < 8; i++)
		{
			spill32(i);
			spill64(i);
			spill128(i);
		}

		for(int i = 0; i < 8; i++)
		{
			// Prevent optimizations
			markModified(OperandREG32(i));
			markModified(OperandMMREG(i));
			markModified(OperandXMMREG(i));
		}
	}

	void RegisterAllocator::spillMMX()
	{
		for(int i = 0; i < 8; i++)
		{
			spill64(i);
		}
	}

	void RegisterAllocator::spillMMXcept(const OperandMMREG &r64)
	{
		for(int i = 0; i < 8; i++)
		{
			if(r64.reg != i)
			{
				spill64(i);
			}
		}

		emms();
	}

	const RegisterAllocator::State RegisterAllocator::capture()
	{
		State state;

		if(!minimalRestore)
		{
			spillAll();
			return state;   // Empty state
		}

		for(int i = 0; i < 8; i++)
		{
			// Prevent optimizations
			markModified(OperandREG32(i));
			markModified(OperandMMREG(i));
			markModified(OperandXMMREG(i));
		}

		for(int i = 0; i < 8; i++)
		{
			state.ERX[i] = ERX[i];
			state.MM[i] = MM[i];
			state.XMM[i] = XMM[i];
		}

		return state;
	}

	void RegisterAllocator::restore(const State &state)
	{
		if(!minimalRestore)
		{
			spillAll();
			return;
		}

		for(int i = 0; i < 8; i++)
		{
			if(ERX[i].reference != state.ERX[i].reference)
			{
				spill32(i);
			}

			if(MM[i].reference != state.MM[i].reference)
			{
				spill64(i);
			}

			if(XMM[i].reference != state.XMM[i].reference)
			{
				spill128(i);
			}
		}

		for(int i = 0; i < 8; i++)
		{
			if(ERX[i].reference != state.ERX[i].reference && state.ERX[i].reference != 0)
			{
				allocate32(i, state.ERX[i].reference, true, state.ERX[i].partial);
			}

			if(MM[i].reference != state.MM[i].reference && state.MM[i].reference != 0)
			{
				allocate64(i, state.MM[i].reference, true);
			}

			if(XMM[i].reference != state.XMM[i].reference && state.XMM[i].reference != 0)
			{
				allocate128(i, state.XMM[i].reference, true, state.XMM[i].partial != 0);
			}
		}

		for(int i = 0; i < 8; i++)
		{
			// Prevent optimizations
			markModified(OperandREG32(i));
			markModified(OperandMMREG(i));
			markModified(OperandXMMREG(i));
		}
	}

	void RegisterAllocator::exclude(const OperandREG32 &r32)
	{
		spill(r32);
		prioritize32(r32.reg);
	}

	Encoding *RegisterAllocator::mov(OperandREG32 r32i, OperandREG32 r32j)
	{
		if(r32i == r32j) return 0;

		// Register overwritten, when not used, eliminate load instruction
		if(ERX[r32i.reg].loadInstruction && loadElimination)
		{
			ERX[r32i.reg].loadInstruction->reserve();
			ERX[r32i.reg].loadInstruction = 0;
		}

		// Register overwritten, when not used, eliminate copy instruction
		if(ERX[r32i.reg].copyInstruction && copyPropagation)
		{
			ERX[r32i.reg].copyInstruction->reserve();
			ERX[r32i.reg].copyInstruction = 0;
		}

		Encoding *spillInstruction = ERX[r32i.reg].spillInstruction;
		AllocationData spillAllocation = ERX[r32i.reg].spill;

		Encoding *mov = Assembler::mov(r32i, r32j);
		
		if(ERX[r32i.reg].reference == 0 || ERX[r32j.reg].reference == 0)   // Return if not in allocation table
		{
			return mov;
		}

		// Attempt copy propagation
		if(mov && copyPropagation)
		{
			swap32(r32i.reg, r32j.reg);
			ERX[r32i.reg].copyInstruction = mov;
		}

		ERX[r32i.reg].spillInstruction = spillInstruction;
		ERX[r32i.reg].spill = spillAllocation;
		
		return mov;
	}

	Encoding *RegisterAllocator::mov(OperandREG32 r32, OperandMEM32 m32)
	{
		if(r32.reg == Encoding::ESP || r32.reg == Encoding::EBP)
		{
			return Assembler::mov(r32, m32);
		}

		// Register overwritten, when not used, eliminate load instruction
		if(ERX[r32.reg].loadInstruction && loadElimination)
		{
			ERX[r32.reg].loadInstruction->reserve();
			ERX[r32.reg].loadInstruction = 0;
		}

		// Register overwritten, when not used, eliminate copy instruction
		if(ERX[r32.reg].copyInstruction && copyPropagation)
		{
			ERX[r32.reg].copyInstruction->reserve();
			ERX[r32.reg].copyInstruction = 0;
		}

		Encoding *spillInstruction = ERX[r32.reg].spillInstruction;
		AllocationData spillAllocation = ERX[r32.reg].spill;

		Encoding *mov = Assembler::mov(r32, m32);

		ERX[r32.reg].spillInstruction = spillInstruction;
		ERX[r32.reg].spill = spillAllocation;

		return mov;
	}

	Encoding *RegisterAllocator::mov(OperandREG32 r32, OperandR_M32 r_m32)
	{
		if(r_m32.isSubtypeOf(Operand::OPERAND_REG32))
		{
			return mov(r32, (OperandREG32)r_m32);
		}
		else
		{
			return mov(r32, (OperandMEM32)r_m32);
		}
	}

	Encoding *RegisterAllocator::movq(OperandMMREG r64i, OperandMMREG r64j)
	{
		if(r64i == r64j) return 0;

		// Register overwritten, when not used, eliminate load instruction
		if(MM[r64i.reg].loadInstruction && loadElimination)
		{
			MM[r64i.reg].loadInstruction->reserve();
			MM[r64i.reg].loadInstruction = 0;
		}

		// Register overwritten, when not used, eliminate copy instruction
		if(MM[r64i.reg].copyInstruction && copyPropagation)
		{
			MM[r64i.reg].copyInstruction->reserve();
			MM[r64i.reg].copyInstruction = 0;
		}

		Encoding *spillInstruction = MM[r64i.reg].spillInstruction;
		AllocationData spillAllocation = MM[r64i.reg].spill;

		Encoding *movq = Assembler::movq(r64i, r64j);
		
		if(MM[r64i.reg].reference == 0 || MM[r64j.reg].reference == 0)   // Return if not in allocation table
		{
			return movq;
		}

		// Attempt copy propagation
		if(movq && copyPropagation)
		{
			swap64(r64i.reg, r64j.reg);
			MM[r64i.reg].copyInstruction = movq;
		}

		MM[r64i.reg].spillInstruction = spillInstruction;
		MM[r64i.reg].spill = spillAllocation;
		
		return movq;
	}

	Encoding *RegisterAllocator::movq(OperandMMREG r64, OperandMEM64 m64)
	{
		// Register overwritten, when not used, eliminate load instruction
		if(MM[r64.reg].loadInstruction && loadElimination)
		{
			MM[r64.reg].loadInstruction->reserve();
			MM[r64.reg].loadInstruction = 0;
		}

		// Register overwritten, when not used, eliminate copy instruction
		if(MM[r64.reg].copyInstruction && copyPropagation)
		{
			MM[r64.reg].copyInstruction->reserve();
			MM[r64.reg].copyInstruction = 0;
		}

		Encoding *spillInstruction = MM[r64.reg].spillInstruction;
		AllocationData spillAllocation = MM[r64.reg].spill;

		Encoding *movq = Assembler::movq(r64, m64);

		MM[r64.reg].spillInstruction = spillInstruction;
		MM[r64.reg].spill = spillAllocation;

		return movq;
	}

	Encoding *RegisterAllocator::movq(OperandMMREG r64, OperandR_M64 r_m64)
	{
		if(r_m64.isSubtypeOf(Operand::OPERAND_MMREG))
		{
			return movq(r64, (OperandMMREG)r_m64);
		}
		else
		{
			return movq(r64, (OperandMEM64)r_m64);
		}
	}

	Encoding *RegisterAllocator::movaps(OperandXMMREG r128i, OperandXMMREG r128j)
	{
		if(r128i == r128j) return 0;

		// Register overwritten, when not used, eliminate load instruction
		if(XMM[r128i.reg].loadInstruction && loadElimination)
		{
			XMM[r128i.reg].loadInstruction->reserve();
			XMM[r128i.reg].loadInstruction = 0;
		}

		// Register overwritten, when not used, eliminate copy instruction
		if(XMM[r128i.reg].copyInstruction && copyPropagation)
		{
			XMM[r128i.reg].copyInstruction->reserve();
			XMM[r128i.reg].copyInstruction = 0;
		}

		Encoding *spillInstruction = XMM[r128i.reg].spillInstruction;
		AllocationData spillAllocation = XMM[r128i.reg].spill;

		Encoding *movaps = Assembler::movaps(r128i, r128j);
		
		if(XMM[r128i.reg].reference == 0 || XMM[r128j.reg].reference == 0)   // Return if not in allocation table
		{
			return movaps;
		}

		// Attempt copy propagation
		if(movaps && copyPropagation)
		{
			swap128(r128i.reg, r128j.reg);
			XMM[r128i.reg].copyInstruction = movaps;
		}

		XMM[r128i.reg].spillInstruction = spillInstruction;
		XMM[r128i.reg].spill = spillAllocation;
		
		return movaps;
	}

	Encoding *RegisterAllocator::movaps(OperandXMMREG r128, OperandMEM128 m128)
	{
		// Register overwritten, when not used, eliminate load instruction
		if(XMM[r128.reg].loadInstruction && loadElimination)
		{
			XMM[r128.reg].loadInstruction->reserve();
			XMM[r128.reg].loadInstruction = 0;
		}

		// Register overwritten, when not used, eliminate copy instruction
		if(XMM[r128.reg].copyInstruction && copyPropagation)
		{
			XMM[r128.reg].copyInstruction->reserve();
			XMM[r128.reg].copyInstruction = 0;
		}

		Encoding *spillInstruction = XMM[r128.reg].spillInstruction;
		AllocationData spillAllocation = XMM[r128.reg].spill;

		Encoding *movaps = Assembler::movaps(r128, m128);

		XMM[r128.reg].spillInstruction = spillInstruction;
		XMM[r128.reg].spill = spillAllocation;

		return movaps;
	}

	Encoding *RegisterAllocator::movaps(OperandXMMREG r128, OperandR_M128 r_m128)
	{
		if(r_m128.isSubtypeOf(Operand::OPERAND_XMMREG))
		{
			return movaps(r128, (OperandXMMREG)r_m128);
		}
		else
		{
			return movaps(r128, (OperandMEM128)r_m128);
		}
	}

	void RegisterAllocator::enableAutoEMMS()
	{
		autoEMMS = true;
	}

	void RegisterAllocator::disableAutoEMMS()
	{
		autoEMMS = false;
	}

	void RegisterAllocator::enableCopyPropagation()
	{
		copyPropagation = true;
	}

	void RegisterAllocator::disableCopyPropagation()
	{
		copyPropagation = false;
	}

	void RegisterAllocator::enableLoadElimination()
	{
		loadElimination = true;
	}

	void RegisterAllocator::disableLoadElimination()
	{
		loadElimination = false;
	}

	void RegisterAllocator::enableSpillElimination()
	{
		spillElimination = true;
	}

	void RegisterAllocator::disableSpillElimination()
	{
		spillElimination = false;
	}

	void RegisterAllocator::enableMinimalRestore()
	{
		minimalRestore = true;
	}

	void RegisterAllocator::disableMinimalRestore()
	{
		minimalRestore = false;
	}

	void RegisterAllocator::enableDropUnmodified()
	{
		dropUnmodified = true;
	}

	void RegisterAllocator::disableDropUnmodified()
	{
		dropUnmodified = false;
	}

	Encoding *RegisterAllocator::x86(int instructionID, const Operand &firstOperand, const Operand &secondOperand, const Operand &thirdOperand)
	{
		markModified(firstOperand);
		markReferenced(secondOperand);

		return Assembler::x86(instructionID, firstOperand, secondOperand, thirdOperand);
	}

	void RegisterAllocator::markModified(const Operand &op)
	{
		if(Operand::isReg(op))
		{
			if(op.type == Operand::OPERAND_REG32 ||
			   op.type == Operand::OPERAND_REG16 ||
			   op.type == Operand::OPERAND_REG8 ||
			   op.type == Operand::OPERAND_EAX ||
			   op.type == Operand::OPERAND_ECX ||
			   op.type == Operand::OPERAND_AX ||
			   op.type == Operand::OPERAND_DX ||
			   op.type == Operand::OPERAND_CX ||
			   op.type == Operand::OPERAND_AL ||
			   op.type == Operand::OPERAND_CL)
			{
				if(op.reg == Encoding::ESP || op.reg == Encoding::EBP) return;

				if(ERX[op.reg].copyInstruction)
				{
					ERX[op.reg].copyInstruction->retain();
					ERX[op.reg].copyInstruction = 0;
				}

				if(ERX[op.reg].loadInstruction)
				{
					ERX[op.reg].loadInstruction->retain();
					ERX[op.reg].loadInstruction = 0;
				}

				if(ERX[op.reg].spillInstruction)
				{
					ERX[op.reg].spillInstruction->retain();
					ERX[op.reg].spillInstruction = 0;

					ERX[op.reg].spill.free();
				}

				ERX[op.reg].modified = true;
			}
			else if(op.type == Operand::OPERAND_MMREG)
			{
				if(MM[op.reg].copyInstruction)
				{
					MM[op.reg].copyInstruction->retain();
					MM[op.reg].copyInstruction = 0;
				}

				if(MM[op.reg].loadInstruction)
				{
					MM[op.reg].loadInstruction->retain();
					MM[op.reg].loadInstruction = 0;
				}

				if(MM[op.reg].spillInstruction)
				{
					MM[op.reg].spillInstruction->retain();
					MM[op.reg].spillInstruction = 0;

					MM[op.reg].spill.free();
				}

				MM[op.reg].modified = true;
			}
			else if(op.type == Operand::OPERAND_XMMREG)
			{
				if(XMM[op.reg].copyInstruction)
				{
					XMM[op.reg].copyInstruction->retain();
					XMM[op.reg].copyInstruction = 0;
				}

				if(XMM[op.reg].loadInstruction)
				{
					XMM[op.reg].loadInstruction->retain();
					XMM[op.reg].loadInstruction = 0;
				}
			
				if(XMM[op.reg].spillInstruction)
				{
					XMM[op.reg].spillInstruction->retain();
					XMM[op.reg].spillInstruction = 0;

					XMM[op.reg].spill.free();
				}

				XMM[op.reg].modified = true;
			}
			else if(op.isSubtypeOf(Operand::OPERAND_FPUREG))
			{
			}
			else
			{
				throw INTERNAL_ERROR;
			}
		}
		else if(Operand::isMem(op))
		{
			if(op.baseReg != Encoding::REG_UNKNOWN)
			{
				markReferenced(OperandREG32(op.baseReg));
			}

			if(op.indexReg != Encoding::REG_UNKNOWN)
			{
				markReferenced(OperandREG32(op.indexReg));
			}
		}
	}

	void RegisterAllocator::markReferenced(const Operand &op)
	{
		if(Operand::isReg(op))
		{
			if(op.type == Operand::OPERAND_REG32 ||
			   op.type == Operand::OPERAND_REG16 ||
			   op.type == Operand::OPERAND_REG8 ||
			   op.type == Operand::OPERAND_EAX ||
			   op.type == Operand::OPERAND_ECX ||
			   op.type == Operand::OPERAND_AX ||
			   op.type == Operand::OPERAND_DX ||
			   op.type == Operand::OPERAND_CX ||
			   op.type == Operand::OPERAND_AL ||
			   op.type == Operand::OPERAND_CL)
			{
				if(op.reg == Encoding::ESP || op.reg == Encoding::EBP) return;

				if(ERX[op.reg].copyInstruction)
				{
					ERX[op.reg].copyInstruction->retain();
					ERX[op.reg].copyInstruction = 0;
				}

				if(ERX[op.reg].loadInstruction)
				{
					ERX[op.reg].loadInstruction->retain();
					ERX[op.reg].loadInstruction = 0;
				}

				if(ERX[op.reg].spillInstruction)
				{
					ERX[op.reg].spillInstruction->retain();
					ERX[op.reg].spillInstruction = 0;

					ERX[op.reg].spill.free();
				}
			}
			else if(op.type == Operand::OPERAND_MMREG)
			{
				if(MM[op.reg].copyInstruction)
				{
					MM[op.reg].copyInstruction->retain();
					MM[op.reg].copyInstruction = 0;
				}

				if(MM[op.reg].loadInstruction)
				{
					MM[op.reg].loadInstruction->retain();
					MM[op.reg].loadInstruction = 0;
				}

				if(MM[op.reg].spillInstruction)
				{
					MM[op.reg].spillInstruction->retain();
					MM[op.reg].spillInstruction = 0;

					MM[op.reg].spill.free();
				}
			}
			else if(op.type == Operand::OPERAND_XMMREG)
			{
				if(XMM[op.reg].copyInstruction)
				{
					XMM[op.reg].copyInstruction->retain();
					XMM[op.reg].copyInstruction = 0;
				}

				if(XMM[op.reg].loadInstruction)
				{
					XMM[op.reg].loadInstruction->retain();
					XMM[op.reg].loadInstruction = 0;
				}

				if(XMM[op.reg].spillInstruction)
				{
					XMM[op.reg].spillInstruction->retain();
					XMM[op.reg].spillInstruction = 0;

					XMM[op.reg].spill.free();
				}
			}
			else if(op.isSubtypeOf(Operand::OPERAND_FPUREG))
			{
			}
			else
			{
				throw INTERNAL_ERROR;
			}
		}
		else if(Operand::isMem(op))
		{
			if(op.baseReg != Encoding::REG_UNKNOWN)
			{
				markReferenced(OperandREG32(op.baseReg));
			}

			if(op.indexReg != Encoding::REG_UNKNOWN)
			{
				markReferenced(OperandREG32(op.indexReg));
			}
		}
	}

	void RegisterAllocator::swap32(int i, int j)
	{
		Allocation *source = &ERX[j];
		Allocation *destination = &ERX[i];

		// Swap references, priorities, etc.
		OperandREF swapRef = source->reference;
		source->reference = destination->reference;
		destination->reference = swapRef;

		int swapPriority = source->priority;
		source->priority = destination->priority;
		destination->priority = swapPriority;

		int swapPartial = source->partial;
		source->partial = destination->partial;
		destination->partial = swapPartial;

		bool swapModified = source->modified;
		source->modified = destination->modified;
		destination->modified = swapModified;
	}

	void RegisterAllocator::swap64(int i, int j)
	{
		Allocation *source = &MM[j];
		Allocation *destination = &MM[i];

		// Swap references, priorities, etc.
		OperandREF swapRef = source->reference;
		source->reference = destination->reference;
		destination->reference = swapRef;

		int swapPriority = source->priority;
		source->priority = destination->priority;
		destination->priority = swapPriority;

		int swapPartial = source->partial;
		source->partial = destination->partial;
		destination->partial = swapPartial;

		bool swapModified = source->modified;
		source->modified = destination->modified;
		destination->modified = swapModified;
	}

	void RegisterAllocator::swap128(int i, int j)
	{
		Allocation *source = &XMM[j];
		Allocation *destination = &XMM[i];

		// Swap references, priorities, etc.
		OperandREF swapRef = source->reference;
		source->reference = destination->reference;
		destination->reference = swapRef;

		int swapPriority = source->priority;
		source->priority = destination->priority;
		destination->priority = swapPriority;

		int swapPartial = source->partial;
		source->partial = destination->partial;
		destination->partial = swapPartial;

		bool swapModified = source->modified;
		source->modified = destination->modified;
		destination->modified = swapModified;
	}
}
