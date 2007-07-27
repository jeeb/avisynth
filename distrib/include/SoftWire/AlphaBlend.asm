AlphaBlend:
	; Move alpha to all words and compute complement
	movd		mm0, [esp+12]
#if katmai == 1
	pshufw		mm0, mm0, 00h
#else
	punpcklwd	mm0, mm0
	punpckldq	mm0, mm0
#endif
	pcmpeqb		mm7, mm7
	psrlw		mm7, 8
	psubw		mm7, mm0
	
	; Load colors
	punpcklbw	mm1, [esp+8]
	punpcklbw	mm2, [esp+4]
	
	; Blend operation
#if katmai != 1
	psrlw		mm1, 8
	psrlw		mm2, 8
	pmullw		mm1, mm7
	pmullw		mm2, mm0
	paddw		mm1, mm2
	psrlw		mm1, 8
#else
	pmulhuw		mm1, mm7
	pmulhuw		mm2, mm0
	paddw		mm1, mm2
#endif
	
	; Return result
	packuswb	mm1, mm1
	movd eax,	mm1
	
	emms
	ret