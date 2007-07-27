inline det(a, b, c, d, x)   // Determinant of 2x2 matrix, result in x
{
	fld		a
	fmul	b
	fld		c
	fmul	d
	fsubp	st1, st0
	fstp	x
}

#define x(v)	dword [v+0]
#define y(v)	dword [v+4]
#define z(v)	dword [v+8]

inline cross(v1, v2, v3)
{
	det(y(v1), z(v2), z(v1), y(v2), x(v3))   // x component
	det(z(v1), x(v2), x(v1), z(v2), y(v3))   // y component
	det(x(v1), y(v2), y(v1), x(v2), z(v3))   // z component
}

CrossProduct:
	mov		eax, [esp+4]
	mov		ecx, [esp+8]
	mov		edx, [esp+12]

	cross(eax, ecx, edx)

	ret
