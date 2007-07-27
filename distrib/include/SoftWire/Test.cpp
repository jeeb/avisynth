#include "CodeGenerator.hpp"

#include <stdio.h>

#ifdef WIN32
	#include <conio.h>
#else
	inline int getch() {return fgetc(stdin);}
#endif

void testHelloWorld()
{
	printf("HelloWorld is a function which uses printf to print a message, ");
	printf("assembled from a source file.\n\n");
	printf("Press any key to start assembling\n\n");
	getch();
	printf("Assembling HelloWorld.asm...\n\n");
	
	SoftWire::ASM_EXPORT(printf);
	SoftWire::Assembler x86("HelloWorld.asm");

	void (*helloWorld)() = (void(*)())x86.callable();

	if(helloWorld)
	{
		printf("%s\n\n", x86.getListing());
		printf("Execute code (y/n)?\n\n");

		int c;
		do
		{
			c = getch();
		}
		while(c != 'y' && c != 'n');

		if(c == 'y')
		{
			printf("output: ");
			helloWorld();
			printf("\n\n");
		}
	}
	else
	{
		printf(x86.getErrors());
	}
}

void testHelloUniverse()
{
	printf("HelloUniverse is a function which uses printf to print a message, ");
	printf("assembled from a source string.\n\n");
	printf("Press any key to start assembling\n\n");
	getch();
	printf("Assembling source string...\n\n");

	const char* source = "string: DB \"Hello universe!\"\nHelloUniverse:\npush string\n"
	                     "call printf\nadd esp, 4\nret";

	SoftWire::ASM_EXPORT(printf);

	SoftWire::Assembler x86(source, "HelloUniverse");

	void (*helloUniverse)() = (void(*)())x86.callable();

	if(helloUniverse)
	{
		printf("%s\n\n", x86.getListing());
		printf("Execute code (y/n)?\n\n");

		int c;
		do
		{
			c = getch();
		}
		while(c != 'y' && c != 'n');

		if(c == 'y')
		{
			printf("output: ");
			helloUniverse();
			printf("\n\n");
		}
	}
	else
	{
		printf(x86.getErrors());
	}
}


void testSetBits()
{
	printf("SetBits is a function which sets a number of bits in a buffer starting from a given bit. In this example it starts at bit 5 and sets 44 bits (viewed right to left).\n\n");
	printf("Press any key to start assembling\n\n");
	getch();
	printf("Assembling SetBits.asm...\n\n");

	SoftWire::Assembler x86("SetBits.asm");

	void (*setBits)(unsigned int*, int, int) = (void(*)(unsigned int*, int, int))x86.callable();

	if(setBits)
	{
		printf("%s\n\n", x86.getListing());
		printf("Execute code (y/n)?\n\n");

		int c;
		do
		{
			c = getch();
		}
		while(c != 'y' && c != 'n');

		if(c == 'y')
		{
			unsigned int bitBuffer[] = {0x00000000, 0x00000000};

			setBits(bitBuffer, 5, 44);

			printf("output: %.8X %.8X\n\n", bitBuffer[1], bitBuffer[0]);
		}
	}
	else
	{
		printf(x86.getErrors());
	}
}

void testCrossProduct()
{
	printf("CrossProduct is a function which computes the cross product of two vectors. In this example it computes (1, 0, 0) x (0, 1, 0).\n\n");
	printf("Press any key to start assembling\n\n");
	getch();
	printf("Assembling CrossProduct.asm...\n\n");

	SoftWire::Assembler x86("CrossProduct.asm");

	void (*crossProduct)(float*, float*, float*) = (void(*)(float*, float*, float*))x86.callable();

	if(crossProduct)
	{
		printf("%s\n\n", x86.getListing());
		printf("Execute code (y/n)?\n\n");

		int c;
		do
		{
			c = getch();
		}
		while(c != 'y' && c != 'n');

		if(c == 'y')
		{
			float V0[3] = {1, 0, 0};
			float V1[3] = {0, 1, 0};
			float V2[3];

			crossProduct(V0, V1, V2);

			printf("output: (%.3f, %.3f, %.3f)\n\n", V2[0], V2[1], V2[2]);
		}
	}
	else
	{
		printf(x86.getErrors());
	}
}

void testAlphaBlend()
{
	printf("AlphaBlend is a function which blends two RGBA colors. SoftWire will conditionally compile for Katmai instructions. In this example, 0x00FF00FF is blended with 0x7F007F00 with a blending factor of 64 / 256.\n\n");
	printf("Press any key to start assembling\n\n");
	getch();
	printf("Assembling AlphaBlend.asm...\n\n");

	bool katmai;

	printf("Do you have a Katmai compatible processor (y/n)?\n\n");

	int c;
	do
	{
		c = getch();
	}
	while(c != 'y' && c != 'n');

	if(c == 'y')
	{
		katmai = true;
	}
	else
	{
		katmai = false;
	}

	SoftWire::ASM_DEFINE(katmai);
	SoftWire::Assembler x86("AlphaBlend.asm");

	int (*alphaBlend)(int, int, int) = (int(*)(int, int, int))x86.callable();

	if(alphaBlend)
	{
		printf("%s\n\n", x86.getListing());
		printf("Execute code (y/n)?\n\n");

		int c;
		do
		{
			c = getch();
		}
		while(c != 'y' && c != 'n');

		if(c == 'y')
		{
			int x = alphaBlend(0x00FF00FF, 0x7F007F00, 64);

			printf("output: %.8X\n\n", x);
		}
	}
	else
	{
		printf(x86.getErrors());
	}
}

void testFactorial()
{
	printf("Factorial is a function which computes the factorial of an integer using recursion. In this example, 5! is computed.\n\n");
	printf("Press any key to start assembling\n\n");
	getch();
	printf("Assembling Factorial.asm...\n\n");

	SoftWire::Assembler x86("Factorial.asm");

	int (*factorial)(int) = (int(*)(int))x86.callable();

	if(factorial)
	{
		printf("%s\n\n", x86.getListing());
		printf("Execute code (y/n)?\n\n");

		int c;
		do
		{
			c = getch();
		}
		while(c != 'y' && c != 'n');

		if(c == 'y')
		{
			int x = factorial(5);

			printf("output: %d\n\n", x);
		}
	}
	else
	{
		printf(x86.getErrors());
	}
}

void testMandelbrot()
{
	printf("Mandelbrot is a function which draws the Mandelbrot fractal.\n\n");
	printf("Press any key to start assembling\n\n");
	getch();
	printf("Assembling Mandelbrot.asm...\n\n");

	SoftWire::ASM_EXPORT(printf);

	SoftWire::Assembler x86("Mandelbrot.asm");

	int (*mandelbrot)() = (int(*)())x86.callable();

	if(mandelbrot)
	{
		printf("%s\n\n", x86.getListing());
		printf("Execute code (y/n)?\n\n");

		int c;
		do
		{
			c = getch();
		}
		while(c != 'y' && c != 'n');

		if(c == 'y')
		{
			mandelbrot();
		}

		printf("\n\n");
	}
	else
	{
		printf(x86.getErrors());
	}
}

void testIntrinsics()
{
	printf("Testing run-time intrinsics.\n\n");
	printf("Press any key to start assembling\n\n");
	getch();

	SoftWire::Assembler x86;

	static char string[] = "All working!";

	x86.push((int)string);
	x86.call((int)printf);
	x86.add(x86.esp, 4);
	x86.ret();

	void (*emulator)() = (void(*)())x86.callable();

	if(emulator)
	{
		printf("%s\n\n", x86.getListing());
		printf("Execute code (y/n)?\n\n");

		int c;
		do
		{
			c = getch();
		}
		while(c != 'y' && c != 'n');

		if(c == 'y')
		{
			printf("output: ");
			emulator();
			printf("\n\n");
		}
	}
	else
	{
		printf(x86.getErrors());
	}
}

class TestRegisterAllocator : public SoftWire::CodeGenerator
{
public:
	TestRegisterAllocator()
	{
		x1 = 1;
		x2 = 2;
		x3 = 3;
		x4 = 4;
		x5 = 5;
		x6 = 6;
		x7 = 7;
		x8 = 8;
		x9 = 9;

		Int t1;
		Int t2;
		Int t3;
		Int t4;
		Int t5;
		Int t6;
		Int t7;
		Int t8;
		Int t9;

		pushad();

		mov(t1, r32(&x1));
		mov(t2, r32(&x2));
		mov(t3, r32(&x3));
		mov(t4, r32(&x4));
		mov(t5, r32(&x5));
		mov(t6, r32(&x6));
		mov(t7, r32(&x7));
		mov(t8, r32(&x8));
		mov(t9, r32(&x9));

		mov(dword_ptr [&x1], t9);
		mov(dword_ptr [&x2], t8);
		mov(dword_ptr [&x3], t7);
		mov(dword_ptr [&x4], t6);
		mov(dword_ptr [&x5], t5);
		mov(dword_ptr [&x6], t4);
		mov(dword_ptr [&x7], t3);
		mov(dword_ptr [&x8], t2);
		mov(dword_ptr [&x9], t1);

		popad();
		ret();
	}

	int x1;
	int x2;
	int x3;
	int x4;
	int x5;
	int x6;
	int x7;
	int x8;
	int x9;
};

void testRegisterAllocator()
{
	printf("Testing register allocator. SoftWire will swap nine numbers using nine virtual general-purpose registers.\n\n");
	printf("Press any key to start assembling\n\n");
	getch();

	TestRegisterAllocator x86;

	void (*script)() = (void(*)())x86.callable();

	if(script)
	{
		printf("%s\n\n", x86.getListing());
		printf("Execute code (y/n)?\n\n");

		int c;
		do
		{
			c = getch();
		}
		while(c != 'y' && c != 'n');

		if(c == 'y')
		{
			printf("Input:  %d %d %d %d %d %d %d %d %d\n", x86.x1, x86.x2, x86.x3, x86.x4, x86.x5, x86.x6, x86.x7, x86.x8, x86.x9);
			script();
			printf("output: %d %d %d %d %d %d %d %d %d\n", x86.x1, x86.x2, x86.x3, x86.x4, x86.x5, x86.x6, x86.x7, x86.x8, x86.x9);
			printf("\n");
		}
	}
	else
	{
		printf(x86.getErrors());
	}
}

class TestOptimizations : public SoftWire::CodeGenerator
{
public:
	TestOptimizations()
	{
		Int y;
		Int z;

		pushad();

		mov(y, 1);
		mov(z, y);
		mov(dword_ptr [&x], z);

		popad();
		ret();
	}

	static x;
};

int TestOptimizations::x = 0;

void testOptimizations()
{
	printf("Testing optimizations. SoftWire will eliminate redundant instructions.\n\n");
	printf("Press any key to start assembling\n\n");
	getch();

	TestOptimizations x86;

	void (*script)() = (void(*)())x86.callable();

	if(script)
	{
		printf("%s\n\n", x86.getListing());
		printf("Execute code (y/n)?\n\n");

		int c;
		do
		{
			c = getch();
		}
		while(c != 'y' && c != 'n');

		if(c == 'y')
		{
			script();

			if(x86.x == 1)
			{
				printf("Success!\n");				
			}
			else
			{
				printf("Failure! (%d)\n", x86.x);
			}
		}
	}
	else
	{
		printf(x86.getErrors());
	}
}

int main()
{
	testHelloWorld();
	testHelloUniverse();
	testSetBits();
	testCrossProduct();
	testAlphaBlend();
	testFactorial();
	testMandelbrot();
	testIntrinsics();
	testRegisterAllocator();
	testOptimizations();

	printf("Press any key to continue\n");
	getch();
	
	return 0;
}
