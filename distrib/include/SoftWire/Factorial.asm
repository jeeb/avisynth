Factorial:
	push	esi

	mov		esi, [esp+8]
	cmp		esi, 1
	jne		Iteration

	mov		eax, esi
	pop		esi

	ret

Iteration:
	lea		eax, DWORD PTR [esi-1]
	push	eax
	call	Factorial
	imul	eax, esi
	add		esp, 4
	pop		esi

	ret
