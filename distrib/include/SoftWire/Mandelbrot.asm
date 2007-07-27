chars:		DB " \xB0\xB1\xB2\xDB\xB2\xB1\xB0 \xB0\xB1\xB2\xDB\xB2\xB1\xB0 "
cString:	DB "%c"
newLine:	DB "\n"

_1_2f:		DD 1.2
_4f:		DD 4.0
_0_05f:		DD 0.05

#define Im	dword [ebp-4]
#define Re	dword [ebp-8]
#define Zr	dword [ebp-12]
#define Zi	dword [ebp-16]
#define Zr2	dword [ebp-20]
#define Zi2	dword [ebp-24]

Mandelbrot:
	push	ebp
	mov		ebp, esp
	sub		esp, 24	

	mov		Im, 0xBF99999A   // -1.2

imLoop:
	mov		Re, 0xC0000000   // - 2.0

reLoop:
	mov		eax, Re
	mov		Zr, eax
	mov		eax, Im
	mov		Zi, eax
	xor		ecx, ecx

innerLoop:
	fld		Zr
	fmul	st, st0
	fstp	Zr2
	fld		Zi
	fmul	st, st0
	fstp	Zi2

	fld		Zi2
	fadd	Zr2
	fcomp	dword [_4f]
	fnstsw	ax
	test	ax, 0x4100
	je		break

	fld		Zr
	fadd	st, st0
	fmul	Zi
	fadd	Im
	fstp	Zi

	fld		Zr2
	fsub	Zi2
	fadd	Re
	fstp	Zr

	inc		ecx
	cmp		ecx, 16
	jl		innerLoop

break:
	movsx	eax, byte ptr [chars+ecx]
	push	eax
	push	cString
	call	printf
	add		esp, 8

	fld		Re
	fadd	dword [_0_05f]
	fst		Re
	fcomp	dword [_1_2f]
	fnstsw	ax
	test	ah, 0x41
	jnp		reLoop

	push	newLine
	call	printf
	add		esp, 4

	fld		Im
	fadd	dword [_0_05f]
	fst		Im
	fcomp	dword [_1_2f]
	fnstsw	ax
	test	ah, 0x41
	jnp		imLoop

	mov		esp, ebp
	pop		ebp
	ret		0