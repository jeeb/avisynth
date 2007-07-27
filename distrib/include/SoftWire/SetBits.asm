SetBits:
	push	esi

	; Compute start position and starting bit offset
	mov		esi, [esp+12]
	mov		eax, esi
	cdq
	and		edx, 31
	add		eax, edx
	sar		eax, 5
	and		esi, 0x8000001F
	jns		L0
	dec		esi
	or		esi, -32
	inc		esi

	; Main loop
L0:	mov		edx, [esp+16]
	test	edx, edx
	jle		L1
	mov		ecx, [esp+8]
	push	edi
	lea		edi, [ecx+eax*4]
	
	; Bits to set in dword
L3:	or		eax, -1
	cmp		edx, 32
	jge		L2
	mov		ecx, 32
	or		eax, -1
	sub		ecx, edx
	shr		eax, cl
	
	; Set dword bits
L2:	mov		ecx, esi
	add		edx, esi
	shl		eax, cl
	mov		ecx, [edi]
	sub		edx, 32
	xor		esi, esi
	add		edi, 4
	or		ecx, eax
	mov		[edi-4], ecx
	test	edx, edx
	jg		L3
	
	pop		edi
	
L1: pop		esi
	ret