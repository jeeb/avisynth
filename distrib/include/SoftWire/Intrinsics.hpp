/* Automatically generated file, do not modify */
/* To regenerate this file uncomment generateIntrinsics() in InstructionSet.cpp */

#ifndef SOFTWIRE_NO_INTRINSICS

typedef OperandIMM IMM;
typedef OperandAL AL;
typedef OperandAX AX;
typedef OperandEAX EAX;
typedef OperandDX DX;
typedef OperandCL CL;
typedef OperandCX CX;
typedef OperandECX ECX;
typedef OperandST0 ST0;
typedef OperandREG8 REG8;
typedef OperandREG16 REG16;
typedef OperandREG32 REG32;
typedef OperandFPUREG FPUREG;
typedef OperandMMREG MMREG;
typedef OperandXMMREG XMMREG;
typedef OperandMEM8 MEM8;
typedef OperandMEM16 MEM16;
typedef OperandMEM32 MEM32;
typedef OperandMEM64 MEM64;
typedef OperandMEM128 MEM128;
typedef OperandR_M8 R_M8;
typedef OperandR_M16 R_M16;
typedef OperandR_M32 R_M32;
typedef OperandR_M64 R_M64;
typedef OperandR_M128 R_M128;
typedef OperandXMM32 XMM32;
typedef OperandXMM64 XMM64;
typedef OperandREF REF;
typedef OperandSTR STR;

int aaa(){return x86(0);}
int aas(){return x86(1);}
int aad(){return x86(2);}
int aad(int a){return x86(3,(IMM)a);}
int aad(char a){return x86(3,(IMM)a);}
int aad(short a){return x86(3,(IMM)a);}
int aad(REF a){return x86(3,a);}
int aam(){return x86(4);}
int aam(int a){return x86(5,(IMM)a);}
int aam(char a){return x86(5,(IMM)a);}
int aam(short a){return x86(5,(IMM)a);}
int aam(REF a){return x86(5,a);}
int adc(AL a,AL b){return x86(6,a,b);}
int adc(AL a,CL b){return x86(6,a,b);}
int adc(AL a,REG8 b){return x86(6,a,b);}
int adc(CL a,AL b){return x86(6,a,b);}
int adc(CL a,CL b){return x86(6,a,b);}
int adc(CL a,REG8 b){return x86(6,a,b);}
int adc(REG8 a,AL b){return x86(6,a,b);}
int adc(REG8 a,CL b){return x86(6,a,b);}
int adc(REG8 a,REG8 b){return x86(6,a,b);}
int adc(MEM8 a,AL b){return x86(6,a,b);}
int adc(MEM8 a,CL b){return x86(6,a,b);}
int adc(MEM8 a,REG8 b){return x86(6,a,b);}
int adc(R_M8 a,AL b){return x86(6,a,b);}
int adc(R_M8 a,CL b){return x86(6,a,b);}
int adc(R_M8 a,REG8 b){return x86(6,a,b);}
int adc(AX a,AX b){return x86(7,a,b);}
int adc(AX a,DX b){return x86(7,a,b);}
int adc(AX a,CX b){return x86(7,a,b);}
int adc(AX a,REG16 b){return x86(7,a,b);}
int adc(DX a,AX b){return x86(7,a,b);}
int adc(DX a,DX b){return x86(7,a,b);}
int adc(DX a,CX b){return x86(7,a,b);}
int adc(DX a,REG16 b){return x86(7,a,b);}
int adc(CX a,AX b){return x86(7,a,b);}
int adc(CX a,DX b){return x86(7,a,b);}
int adc(CX a,CX b){return x86(7,a,b);}
int adc(CX a,REG16 b){return x86(7,a,b);}
int adc(REG16 a,AX b){return x86(7,a,b);}
int adc(REG16 a,DX b){return x86(7,a,b);}
int adc(REG16 a,CX b){return x86(7,a,b);}
int adc(REG16 a,REG16 b){return x86(7,a,b);}
int adc(MEM16 a,AX b){return x86(7,a,b);}
int adc(MEM16 a,DX b){return x86(7,a,b);}
int adc(MEM16 a,CX b){return x86(7,a,b);}
int adc(MEM16 a,REG16 b){return x86(7,a,b);}
int adc(R_M16 a,AX b){return x86(7,a,b);}
int adc(R_M16 a,DX b){return x86(7,a,b);}
int adc(R_M16 a,CX b){return x86(7,a,b);}
int adc(R_M16 a,REG16 b){return x86(7,a,b);}
int adc(EAX a,EAX b){return x86(8,a,b);}
int adc(EAX a,ECX b){return x86(8,a,b);}
int adc(EAX a,REG32 b){return x86(8,a,b);}
int adc(ECX a,EAX b){return x86(8,a,b);}
int adc(ECX a,ECX b){return x86(8,a,b);}
int adc(ECX a,REG32 b){return x86(8,a,b);}
int adc(REG32 a,EAX b){return x86(8,a,b);}
int adc(REG32 a,ECX b){return x86(8,a,b);}
int adc(REG32 a,REG32 b){return x86(8,a,b);}
int adc(MEM32 a,EAX b){return x86(8,a,b);}
int adc(MEM32 a,ECX b){return x86(8,a,b);}
int adc(MEM32 a,REG32 b){return x86(8,a,b);}
int adc(R_M32 a,EAX b){return x86(8,a,b);}
int adc(R_M32 a,ECX b){return x86(8,a,b);}
int adc(R_M32 a,REG32 b){return x86(8,a,b);}
int lock_adc(MEM8 a,AL b){return x86(9,a,b);}
int lock_adc(MEM8 a,CL b){return x86(9,a,b);}
int lock_adc(MEM8 a,REG8 b){return x86(9,a,b);}
int lock_adc(MEM16 a,AX b){return x86(10,a,b);}
int lock_adc(MEM16 a,DX b){return x86(10,a,b);}
int lock_adc(MEM16 a,CX b){return x86(10,a,b);}
int lock_adc(MEM16 a,REG16 b){return x86(10,a,b);}
int lock_adc(MEM32 a,EAX b){return x86(11,a,b);}
int lock_adc(MEM32 a,ECX b){return x86(11,a,b);}
int lock_adc(MEM32 a,REG32 b){return x86(11,a,b);}
int adc(AL a,MEM8 b){return x86(12,a,b);}
int adc(AL a,R_M8 b){return x86(12,a,b);}
int adc(CL a,MEM8 b){return x86(12,a,b);}
int adc(CL a,R_M8 b){return x86(12,a,b);}
int adc(REG8 a,MEM8 b){return x86(12,a,b);}
int adc(REG8 a,R_M8 b){return x86(12,a,b);}
int adc(AX a,MEM16 b){return x86(13,a,b);}
int adc(AX a,R_M16 b){return x86(13,a,b);}
int adc(DX a,MEM16 b){return x86(13,a,b);}
int adc(DX a,R_M16 b){return x86(13,a,b);}
int adc(CX a,MEM16 b){return x86(13,a,b);}
int adc(CX a,R_M16 b){return x86(13,a,b);}
int adc(REG16 a,MEM16 b){return x86(13,a,b);}
int adc(REG16 a,R_M16 b){return x86(13,a,b);}
int adc(EAX a,MEM32 b){return x86(14,a,b);}
int adc(EAX a,R_M32 b){return x86(14,a,b);}
int adc(ECX a,MEM32 b){return x86(14,a,b);}
int adc(ECX a,R_M32 b){return x86(14,a,b);}
int adc(REG32 a,MEM32 b){return x86(14,a,b);}
int adc(REG32 a,R_M32 b){return x86(14,a,b);}
int adc(AL a,char b){return x86(15,a,(IMM)b);}
int adc(CL a,char b){return x86(15,a,(IMM)b);}
int adc(REG8 a,char b){return x86(15,a,(IMM)b);}
int adc(MEM8 a,char b){return x86(15,a,(IMM)b);}
int adc(R_M8 a,char b){return x86(15,a,(IMM)b);}
int adc(AX a,char b){return x86(16,a,(IMM)b);}
int adc(AX a,short b){return x86(16,a,(IMM)b);}
int adc(DX a,char b){return x86(16,a,(IMM)b);}
int adc(DX a,short b){return x86(16,a,(IMM)b);}
int adc(CX a,char b){return x86(16,a,(IMM)b);}
int adc(CX a,short b){return x86(16,a,(IMM)b);}
int adc(REG16 a,char b){return x86(16,a,(IMM)b);}
int adc(REG16 a,short b){return x86(16,a,(IMM)b);}
int adc(MEM16 a,char b){return x86(16,a,(IMM)b);}
int adc(MEM16 a,short b){return x86(16,a,(IMM)b);}
int adc(R_M16 a,char b){return x86(16,a,(IMM)b);}
int adc(R_M16 a,short b){return x86(16,a,(IMM)b);}
int adc(EAX a,int b){return x86(17,a,(IMM)b);}
int adc(EAX a,char b){return x86(17,a,(IMM)b);}
int adc(EAX a,short b){return x86(17,a,(IMM)b);}
int adc(EAX a,REF b){return x86(17,a,b);}
int adc(ECX a,int b){return x86(17,a,(IMM)b);}
int adc(ECX a,char b){return x86(17,a,(IMM)b);}
int adc(ECX a,short b){return x86(17,a,(IMM)b);}
int adc(ECX a,REF b){return x86(17,a,b);}
int adc(REG32 a,int b){return x86(17,a,(IMM)b);}
int adc(REG32 a,char b){return x86(17,a,(IMM)b);}
int adc(REG32 a,short b){return x86(17,a,(IMM)b);}
int adc(REG32 a,REF b){return x86(17,a,b);}
int adc(MEM32 a,int b){return x86(17,a,(IMM)b);}
int adc(MEM32 a,char b){return x86(17,a,(IMM)b);}
int adc(MEM32 a,short b){return x86(17,a,(IMM)b);}
int adc(MEM32 a,REF b){return x86(17,a,b);}
int adc(R_M32 a,int b){return x86(17,a,(IMM)b);}
int adc(R_M32 a,char b){return x86(17,a,(IMM)b);}
int adc(R_M32 a,short b){return x86(17,a,(IMM)b);}
int adc(R_M32 a,REF b){return x86(17,a,b);}
int lock_adc(MEM8 a,char b){return x86(20,a,(IMM)b);}
int lock_adc(MEM16 a,char b){return x86(21,a,(IMM)b);}
int lock_adc(MEM16 a,short b){return x86(21,a,(IMM)b);}
int lock_adc(MEM32 a,int b){return x86(22,a,(IMM)b);}
int lock_adc(MEM32 a,char b){return x86(22,a,(IMM)b);}
int lock_adc(MEM32 a,short b){return x86(22,a,(IMM)b);}
int lock_adc(MEM32 a,REF b){return x86(22,a,b);}
int add(AL a,AL b){return x86(28,a,b);}
int add(AL a,CL b){return x86(28,a,b);}
int add(AL a,REG8 b){return x86(28,a,b);}
int add(CL a,AL b){return x86(28,a,b);}
int add(CL a,CL b){return x86(28,a,b);}
int add(CL a,REG8 b){return x86(28,a,b);}
int add(REG8 a,AL b){return x86(28,a,b);}
int add(REG8 a,CL b){return x86(28,a,b);}
int add(REG8 a,REG8 b){return x86(28,a,b);}
int add(MEM8 a,AL b){return x86(28,a,b);}
int add(MEM8 a,CL b){return x86(28,a,b);}
int add(MEM8 a,REG8 b){return x86(28,a,b);}
int add(R_M8 a,AL b){return x86(28,a,b);}
int add(R_M8 a,CL b){return x86(28,a,b);}
int add(R_M8 a,REG8 b){return x86(28,a,b);}
int add(AX a,AX b){return x86(29,a,b);}
int add(AX a,DX b){return x86(29,a,b);}
int add(AX a,CX b){return x86(29,a,b);}
int add(AX a,REG16 b){return x86(29,a,b);}
int add(DX a,AX b){return x86(29,a,b);}
int add(DX a,DX b){return x86(29,a,b);}
int add(DX a,CX b){return x86(29,a,b);}
int add(DX a,REG16 b){return x86(29,a,b);}
int add(CX a,AX b){return x86(29,a,b);}
int add(CX a,DX b){return x86(29,a,b);}
int add(CX a,CX b){return x86(29,a,b);}
int add(CX a,REG16 b){return x86(29,a,b);}
int add(REG16 a,AX b){return x86(29,a,b);}
int add(REG16 a,DX b){return x86(29,a,b);}
int add(REG16 a,CX b){return x86(29,a,b);}
int add(REG16 a,REG16 b){return x86(29,a,b);}
int add(MEM16 a,AX b){return x86(29,a,b);}
int add(MEM16 a,DX b){return x86(29,a,b);}
int add(MEM16 a,CX b){return x86(29,a,b);}
int add(MEM16 a,REG16 b){return x86(29,a,b);}
int add(R_M16 a,AX b){return x86(29,a,b);}
int add(R_M16 a,DX b){return x86(29,a,b);}
int add(R_M16 a,CX b){return x86(29,a,b);}
int add(R_M16 a,REG16 b){return x86(29,a,b);}
int add(EAX a,EAX b){return x86(30,a,b);}
int add(EAX a,ECX b){return x86(30,a,b);}
int add(EAX a,REG32 b){return x86(30,a,b);}
int add(ECX a,EAX b){return x86(30,a,b);}
int add(ECX a,ECX b){return x86(30,a,b);}
int add(ECX a,REG32 b){return x86(30,a,b);}
int add(REG32 a,EAX b){return x86(30,a,b);}
int add(REG32 a,ECX b){return x86(30,a,b);}
int add(REG32 a,REG32 b){return x86(30,a,b);}
int add(MEM32 a,EAX b){return x86(30,a,b);}
int add(MEM32 a,ECX b){return x86(30,a,b);}
int add(MEM32 a,REG32 b){return x86(30,a,b);}
int add(R_M32 a,EAX b){return x86(30,a,b);}
int add(R_M32 a,ECX b){return x86(30,a,b);}
int add(R_M32 a,REG32 b){return x86(30,a,b);}
int lock_add(MEM8 a,AL b){return x86(31,a,b);}
int lock_add(MEM8 a,CL b){return x86(31,a,b);}
int lock_add(MEM8 a,REG8 b){return x86(31,a,b);}
int lock_add(MEM16 a,AX b){return x86(32,a,b);}
int lock_add(MEM16 a,DX b){return x86(32,a,b);}
int lock_add(MEM16 a,CX b){return x86(32,a,b);}
int lock_add(MEM16 a,REG16 b){return x86(32,a,b);}
int lock_add(MEM32 a,EAX b){return x86(33,a,b);}
int lock_add(MEM32 a,ECX b){return x86(33,a,b);}
int lock_add(MEM32 a,REG32 b){return x86(33,a,b);}
int add(AL a,MEM8 b){return x86(34,a,b);}
int add(AL a,R_M8 b){return x86(34,a,b);}
int add(CL a,MEM8 b){return x86(34,a,b);}
int add(CL a,R_M8 b){return x86(34,a,b);}
int add(REG8 a,MEM8 b){return x86(34,a,b);}
int add(REG8 a,R_M8 b){return x86(34,a,b);}
int add(AX a,MEM16 b){return x86(35,a,b);}
int add(AX a,R_M16 b){return x86(35,a,b);}
int add(DX a,MEM16 b){return x86(35,a,b);}
int add(DX a,R_M16 b){return x86(35,a,b);}
int add(CX a,MEM16 b){return x86(35,a,b);}
int add(CX a,R_M16 b){return x86(35,a,b);}
int add(REG16 a,MEM16 b){return x86(35,a,b);}
int add(REG16 a,R_M16 b){return x86(35,a,b);}
int add(EAX a,MEM32 b){return x86(36,a,b);}
int add(EAX a,R_M32 b){return x86(36,a,b);}
int add(ECX a,MEM32 b){return x86(36,a,b);}
int add(ECX a,R_M32 b){return x86(36,a,b);}
int add(REG32 a,MEM32 b){return x86(36,a,b);}
int add(REG32 a,R_M32 b){return x86(36,a,b);}
int add(AL a,char b){return x86(37,a,(IMM)b);}
int add(CL a,char b){return x86(37,a,(IMM)b);}
int add(REG8 a,char b){return x86(37,a,(IMM)b);}
int add(MEM8 a,char b){return x86(37,a,(IMM)b);}
int add(R_M8 a,char b){return x86(37,a,(IMM)b);}
int add(AX a,char b){return x86(38,a,(IMM)b);}
int add(AX a,short b){return x86(38,a,(IMM)b);}
int add(DX a,char b){return x86(38,a,(IMM)b);}
int add(DX a,short b){return x86(38,a,(IMM)b);}
int add(CX a,char b){return x86(38,a,(IMM)b);}
int add(CX a,short b){return x86(38,a,(IMM)b);}
int add(REG16 a,char b){return x86(38,a,(IMM)b);}
int add(REG16 a,short b){return x86(38,a,(IMM)b);}
int add(MEM16 a,char b){return x86(38,a,(IMM)b);}
int add(MEM16 a,short b){return x86(38,a,(IMM)b);}
int add(R_M16 a,char b){return x86(38,a,(IMM)b);}
int add(R_M16 a,short b){return x86(38,a,(IMM)b);}
int add(EAX a,int b){return x86(39,a,(IMM)b);}
int add(EAX a,char b){return x86(39,a,(IMM)b);}
int add(EAX a,short b){return x86(39,a,(IMM)b);}
int add(EAX a,REF b){return x86(39,a,b);}
int add(ECX a,int b){return x86(39,a,(IMM)b);}
int add(ECX a,char b){return x86(39,a,(IMM)b);}
int add(ECX a,short b){return x86(39,a,(IMM)b);}
int add(ECX a,REF b){return x86(39,a,b);}
int add(REG32 a,int b){return x86(39,a,(IMM)b);}
int add(REG32 a,char b){return x86(39,a,(IMM)b);}
int add(REG32 a,short b){return x86(39,a,(IMM)b);}
int add(REG32 a,REF b){return x86(39,a,b);}
int add(MEM32 a,int b){return x86(39,a,(IMM)b);}
int add(MEM32 a,char b){return x86(39,a,(IMM)b);}
int add(MEM32 a,short b){return x86(39,a,(IMM)b);}
int add(MEM32 a,REF b){return x86(39,a,b);}
int add(R_M32 a,int b){return x86(39,a,(IMM)b);}
int add(R_M32 a,char b){return x86(39,a,(IMM)b);}
int add(R_M32 a,short b){return x86(39,a,(IMM)b);}
int add(R_M32 a,REF b){return x86(39,a,b);}
int lock_add(MEM8 a,char b){return x86(42,a,(IMM)b);}
int lock_add(MEM16 a,char b){return x86(43,a,(IMM)b);}
int lock_add(MEM16 a,short b){return x86(43,a,(IMM)b);}
int lock_add(MEM32 a,int b){return x86(44,a,(IMM)b);}
int lock_add(MEM32 a,char b){return x86(44,a,(IMM)b);}
int lock_add(MEM32 a,short b){return x86(44,a,(IMM)b);}
int lock_add(MEM32 a,REF b){return x86(44,a,b);}
int addpd(XMMREG a,XMMREG b){return x86(50,a,b);}
int addpd(XMMREG a,MEM128 b){return x86(50,a,b);}
int addpd(XMMREG a,R_M128 b){return x86(50,a,b);}
int addps(XMMREG a,XMMREG b){return x86(51,a,b);}
int addps(XMMREG a,MEM128 b){return x86(51,a,b);}
int addps(XMMREG a,R_M128 b){return x86(51,a,b);}
int addsd(XMMREG a,XMMREG b){return x86(52,a,b);}
int addsd(XMMREG a,MEM64 b){return x86(52,a,b);}
int addsd(XMMREG a,XMM64 b){return x86(52,a,b);}
int addss(XMMREG a,XMMREG b){return x86(53,a,b);}
int addss(XMMREG a,MEM32 b){return x86(53,a,b);}
int addss(XMMREG a,XMM32 b){return x86(53,a,b);}
int and(AL a,AL b){return x86(54,a,b);}
int and(AL a,CL b){return x86(54,a,b);}
int and(AL a,REG8 b){return x86(54,a,b);}
int and(CL a,AL b){return x86(54,a,b);}
int and(CL a,CL b){return x86(54,a,b);}
int and(CL a,REG8 b){return x86(54,a,b);}
int and(REG8 a,AL b){return x86(54,a,b);}
int and(REG8 a,CL b){return x86(54,a,b);}
int and(REG8 a,REG8 b){return x86(54,a,b);}
int and(MEM8 a,AL b){return x86(54,a,b);}
int and(MEM8 a,CL b){return x86(54,a,b);}
int and(MEM8 a,REG8 b){return x86(54,a,b);}
int and(R_M8 a,AL b){return x86(54,a,b);}
int and(R_M8 a,CL b){return x86(54,a,b);}
int and(R_M8 a,REG8 b){return x86(54,a,b);}
int and(AX a,AX b){return x86(55,a,b);}
int and(AX a,DX b){return x86(55,a,b);}
int and(AX a,CX b){return x86(55,a,b);}
int and(AX a,REG16 b){return x86(55,a,b);}
int and(DX a,AX b){return x86(55,a,b);}
int and(DX a,DX b){return x86(55,a,b);}
int and(DX a,CX b){return x86(55,a,b);}
int and(DX a,REG16 b){return x86(55,a,b);}
int and(CX a,AX b){return x86(55,a,b);}
int and(CX a,DX b){return x86(55,a,b);}
int and(CX a,CX b){return x86(55,a,b);}
int and(CX a,REG16 b){return x86(55,a,b);}
int and(REG16 a,AX b){return x86(55,a,b);}
int and(REG16 a,DX b){return x86(55,a,b);}
int and(REG16 a,CX b){return x86(55,a,b);}
int and(REG16 a,REG16 b){return x86(55,a,b);}
int and(MEM16 a,AX b){return x86(55,a,b);}
int and(MEM16 a,DX b){return x86(55,a,b);}
int and(MEM16 a,CX b){return x86(55,a,b);}
int and(MEM16 a,REG16 b){return x86(55,a,b);}
int and(R_M16 a,AX b){return x86(55,a,b);}
int and(R_M16 a,DX b){return x86(55,a,b);}
int and(R_M16 a,CX b){return x86(55,a,b);}
int and(R_M16 a,REG16 b){return x86(55,a,b);}
int and(EAX a,EAX b){return x86(56,a,b);}
int and(EAX a,ECX b){return x86(56,a,b);}
int and(EAX a,REG32 b){return x86(56,a,b);}
int and(ECX a,EAX b){return x86(56,a,b);}
int and(ECX a,ECX b){return x86(56,a,b);}
int and(ECX a,REG32 b){return x86(56,a,b);}
int and(REG32 a,EAX b){return x86(56,a,b);}
int and(REG32 a,ECX b){return x86(56,a,b);}
int and(REG32 a,REG32 b){return x86(56,a,b);}
int and(MEM32 a,EAX b){return x86(56,a,b);}
int and(MEM32 a,ECX b){return x86(56,a,b);}
int and(MEM32 a,REG32 b){return x86(56,a,b);}
int and(R_M32 a,EAX b){return x86(56,a,b);}
int and(R_M32 a,ECX b){return x86(56,a,b);}
int and(R_M32 a,REG32 b){return x86(56,a,b);}
int lock_and(MEM8 a,AL b){return x86(57,a,b);}
int lock_and(MEM8 a,CL b){return x86(57,a,b);}
int lock_and(MEM8 a,REG8 b){return x86(57,a,b);}
int lock_and(MEM16 a,AX b){return x86(58,a,b);}
int lock_and(MEM16 a,DX b){return x86(58,a,b);}
int lock_and(MEM16 a,CX b){return x86(58,a,b);}
int lock_and(MEM16 a,REG16 b){return x86(58,a,b);}
int lock_and(MEM32 a,EAX b){return x86(59,a,b);}
int lock_and(MEM32 a,ECX b){return x86(59,a,b);}
int lock_and(MEM32 a,REG32 b){return x86(59,a,b);}
int and(AL a,MEM8 b){return x86(60,a,b);}
int and(AL a,R_M8 b){return x86(60,a,b);}
int and(CL a,MEM8 b){return x86(60,a,b);}
int and(CL a,R_M8 b){return x86(60,a,b);}
int and(REG8 a,MEM8 b){return x86(60,a,b);}
int and(REG8 a,R_M8 b){return x86(60,a,b);}
int and(AX a,MEM16 b){return x86(61,a,b);}
int and(AX a,R_M16 b){return x86(61,a,b);}
int and(DX a,MEM16 b){return x86(61,a,b);}
int and(DX a,R_M16 b){return x86(61,a,b);}
int and(CX a,MEM16 b){return x86(61,a,b);}
int and(CX a,R_M16 b){return x86(61,a,b);}
int and(REG16 a,MEM16 b){return x86(61,a,b);}
int and(REG16 a,R_M16 b){return x86(61,a,b);}
int and(EAX a,MEM32 b){return x86(62,a,b);}
int and(EAX a,R_M32 b){return x86(62,a,b);}
int and(ECX a,MEM32 b){return x86(62,a,b);}
int and(ECX a,R_M32 b){return x86(62,a,b);}
int and(REG32 a,MEM32 b){return x86(62,a,b);}
int and(REG32 a,R_M32 b){return x86(62,a,b);}
int and(AL a,char b){return x86(63,a,(IMM)b);}
int and(CL a,char b){return x86(63,a,(IMM)b);}
int and(REG8 a,char b){return x86(63,a,(IMM)b);}
int and(MEM8 a,char b){return x86(63,a,(IMM)b);}
int and(R_M8 a,char b){return x86(63,a,(IMM)b);}
int and(AX a,char b){return x86(64,a,(IMM)b);}
int and(AX a,short b){return x86(64,a,(IMM)b);}
int and(DX a,char b){return x86(64,a,(IMM)b);}
int and(DX a,short b){return x86(64,a,(IMM)b);}
int and(CX a,char b){return x86(64,a,(IMM)b);}
int and(CX a,short b){return x86(64,a,(IMM)b);}
int and(REG16 a,char b){return x86(64,a,(IMM)b);}
int and(REG16 a,short b){return x86(64,a,(IMM)b);}
int and(MEM16 a,char b){return x86(64,a,(IMM)b);}
int and(MEM16 a,short b){return x86(64,a,(IMM)b);}
int and(R_M16 a,char b){return x86(64,a,(IMM)b);}
int and(R_M16 a,short b){return x86(64,a,(IMM)b);}
int and(EAX a,int b){return x86(65,a,(IMM)b);}
int and(EAX a,char b){return x86(65,a,(IMM)b);}
int and(EAX a,short b){return x86(65,a,(IMM)b);}
int and(EAX a,REF b){return x86(65,a,b);}
int and(ECX a,int b){return x86(65,a,(IMM)b);}
int and(ECX a,char b){return x86(65,a,(IMM)b);}
int and(ECX a,short b){return x86(65,a,(IMM)b);}
int and(ECX a,REF b){return x86(65,a,b);}
int and(REG32 a,int b){return x86(65,a,(IMM)b);}
int and(REG32 a,char b){return x86(65,a,(IMM)b);}
int and(REG32 a,short b){return x86(65,a,(IMM)b);}
int and(REG32 a,REF b){return x86(65,a,b);}
int and(MEM32 a,int b){return x86(65,a,(IMM)b);}
int and(MEM32 a,char b){return x86(65,a,(IMM)b);}
int and(MEM32 a,short b){return x86(65,a,(IMM)b);}
int and(MEM32 a,REF b){return x86(65,a,b);}
int and(R_M32 a,int b){return x86(65,a,(IMM)b);}
int and(R_M32 a,char b){return x86(65,a,(IMM)b);}
int and(R_M32 a,short b){return x86(65,a,(IMM)b);}
int and(R_M32 a,REF b){return x86(65,a,b);}
int lock_and(MEM8 a,char b){return x86(68,a,(IMM)b);}
int lock_and(MEM16 a,char b){return x86(69,a,(IMM)b);}
int lock_and(MEM16 a,short b){return x86(69,a,(IMM)b);}
int lock_and(MEM32 a,int b){return x86(70,a,(IMM)b);}
int lock_and(MEM32 a,char b){return x86(70,a,(IMM)b);}
int lock_and(MEM32 a,short b){return x86(70,a,(IMM)b);}
int lock_and(MEM32 a,REF b){return x86(70,a,b);}
int andnpd(XMMREG a,XMMREG b){return x86(76,a,b);}
int andnpd(XMMREG a,MEM128 b){return x86(76,a,b);}
int andnpd(XMMREG a,R_M128 b){return x86(76,a,b);}
int andnps(XMMREG a,XMMREG b){return x86(77,a,b);}
int andnps(XMMREG a,MEM128 b){return x86(77,a,b);}
int andnps(XMMREG a,R_M128 b){return x86(77,a,b);}
int andpd(XMMREG a,XMMREG b){return x86(78,a,b);}
int andpd(XMMREG a,MEM128 b){return x86(78,a,b);}
int andpd(XMMREG a,R_M128 b){return x86(78,a,b);}
int andps(XMMREG a,XMMREG b){return x86(79,a,b);}
int andps(XMMREG a,MEM128 b){return x86(79,a,b);}
int andps(XMMREG a,R_M128 b){return x86(79,a,b);}
int bound(AX a,MEM8 b){return x86(80,a,b);}
int bound(AX a,MEM16 b){return x86(80,a,b);}
int bound(AX a,MEM32 b){return x86(80,a,b);}
int bound(AX a,MEM64 b){return x86(80,a,b);}
int bound(AX a,MEM128 b){return x86(80,a,b);}
int bound(DX a,MEM8 b){return x86(80,a,b);}
int bound(DX a,MEM16 b){return x86(80,a,b);}
int bound(DX a,MEM32 b){return x86(80,a,b);}
int bound(DX a,MEM64 b){return x86(80,a,b);}
int bound(DX a,MEM128 b){return x86(80,a,b);}
int bound(CX a,MEM8 b){return x86(80,a,b);}
int bound(CX a,MEM16 b){return x86(80,a,b);}
int bound(CX a,MEM32 b){return x86(80,a,b);}
int bound(CX a,MEM64 b){return x86(80,a,b);}
int bound(CX a,MEM128 b){return x86(80,a,b);}
int bound(REG16 a,MEM8 b){return x86(80,a,b);}
int bound(REG16 a,MEM16 b){return x86(80,a,b);}
int bound(REG16 a,MEM32 b){return x86(80,a,b);}
int bound(REG16 a,MEM64 b){return x86(80,a,b);}
int bound(REG16 a,MEM128 b){return x86(80,a,b);}
int bound(EAX a,MEM8 b){return x86(81,a,b);}
int bound(EAX a,MEM16 b){return x86(81,a,b);}
int bound(EAX a,MEM32 b){return x86(81,a,b);}
int bound(EAX a,MEM64 b){return x86(81,a,b);}
int bound(EAX a,MEM128 b){return x86(81,a,b);}
int bound(ECX a,MEM8 b){return x86(81,a,b);}
int bound(ECX a,MEM16 b){return x86(81,a,b);}
int bound(ECX a,MEM32 b){return x86(81,a,b);}
int bound(ECX a,MEM64 b){return x86(81,a,b);}
int bound(ECX a,MEM128 b){return x86(81,a,b);}
int bound(REG32 a,MEM8 b){return x86(81,a,b);}
int bound(REG32 a,MEM16 b){return x86(81,a,b);}
int bound(REG32 a,MEM32 b){return x86(81,a,b);}
int bound(REG32 a,MEM64 b){return x86(81,a,b);}
int bound(REG32 a,MEM128 b){return x86(81,a,b);}
int bsf(AX a,AX b){return x86(82,a,b);}
int bsf(AX a,DX b){return x86(82,a,b);}
int bsf(AX a,CX b){return x86(82,a,b);}
int bsf(AX a,REG16 b){return x86(82,a,b);}
int bsf(AX a,MEM16 b){return x86(82,a,b);}
int bsf(AX a,R_M16 b){return x86(82,a,b);}
int bsf(DX a,AX b){return x86(82,a,b);}
int bsf(DX a,DX b){return x86(82,a,b);}
int bsf(DX a,CX b){return x86(82,a,b);}
int bsf(DX a,REG16 b){return x86(82,a,b);}
int bsf(DX a,MEM16 b){return x86(82,a,b);}
int bsf(DX a,R_M16 b){return x86(82,a,b);}
int bsf(CX a,AX b){return x86(82,a,b);}
int bsf(CX a,DX b){return x86(82,a,b);}
int bsf(CX a,CX b){return x86(82,a,b);}
int bsf(CX a,REG16 b){return x86(82,a,b);}
int bsf(CX a,MEM16 b){return x86(82,a,b);}
int bsf(CX a,R_M16 b){return x86(82,a,b);}
int bsf(REG16 a,AX b){return x86(82,a,b);}
int bsf(REG16 a,DX b){return x86(82,a,b);}
int bsf(REG16 a,CX b){return x86(82,a,b);}
int bsf(REG16 a,REG16 b){return x86(82,a,b);}
int bsf(REG16 a,MEM16 b){return x86(82,a,b);}
int bsf(REG16 a,R_M16 b){return x86(82,a,b);}
int bsf(EAX a,EAX b){return x86(83,a,b);}
int bsf(EAX a,ECX b){return x86(83,a,b);}
int bsf(EAX a,REG32 b){return x86(83,a,b);}
int bsf(EAX a,MEM32 b){return x86(83,a,b);}
int bsf(EAX a,R_M32 b){return x86(83,a,b);}
int bsf(ECX a,EAX b){return x86(83,a,b);}
int bsf(ECX a,ECX b){return x86(83,a,b);}
int bsf(ECX a,REG32 b){return x86(83,a,b);}
int bsf(ECX a,MEM32 b){return x86(83,a,b);}
int bsf(ECX a,R_M32 b){return x86(83,a,b);}
int bsf(REG32 a,EAX b){return x86(83,a,b);}
int bsf(REG32 a,ECX b){return x86(83,a,b);}
int bsf(REG32 a,REG32 b){return x86(83,a,b);}
int bsf(REG32 a,MEM32 b){return x86(83,a,b);}
int bsf(REG32 a,R_M32 b){return x86(83,a,b);}
int bsr(AX a,AX b){return x86(84,a,b);}
int bsr(AX a,DX b){return x86(84,a,b);}
int bsr(AX a,CX b){return x86(84,a,b);}
int bsr(AX a,REG16 b){return x86(84,a,b);}
int bsr(AX a,MEM16 b){return x86(84,a,b);}
int bsr(AX a,R_M16 b){return x86(84,a,b);}
int bsr(DX a,AX b){return x86(84,a,b);}
int bsr(DX a,DX b){return x86(84,a,b);}
int bsr(DX a,CX b){return x86(84,a,b);}
int bsr(DX a,REG16 b){return x86(84,a,b);}
int bsr(DX a,MEM16 b){return x86(84,a,b);}
int bsr(DX a,R_M16 b){return x86(84,a,b);}
int bsr(CX a,AX b){return x86(84,a,b);}
int bsr(CX a,DX b){return x86(84,a,b);}
int bsr(CX a,CX b){return x86(84,a,b);}
int bsr(CX a,REG16 b){return x86(84,a,b);}
int bsr(CX a,MEM16 b){return x86(84,a,b);}
int bsr(CX a,R_M16 b){return x86(84,a,b);}
int bsr(REG16 a,AX b){return x86(84,a,b);}
int bsr(REG16 a,DX b){return x86(84,a,b);}
int bsr(REG16 a,CX b){return x86(84,a,b);}
int bsr(REG16 a,REG16 b){return x86(84,a,b);}
int bsr(REG16 a,MEM16 b){return x86(84,a,b);}
int bsr(REG16 a,R_M16 b){return x86(84,a,b);}
int bsr(EAX a,EAX b){return x86(85,a,b);}
int bsr(EAX a,ECX b){return x86(85,a,b);}
int bsr(EAX a,REG32 b){return x86(85,a,b);}
int bsr(EAX a,MEM32 b){return x86(85,a,b);}
int bsr(EAX a,R_M32 b){return x86(85,a,b);}
int bsr(ECX a,EAX b){return x86(85,a,b);}
int bsr(ECX a,ECX b){return x86(85,a,b);}
int bsr(ECX a,REG32 b){return x86(85,a,b);}
int bsr(ECX a,MEM32 b){return x86(85,a,b);}
int bsr(ECX a,R_M32 b){return x86(85,a,b);}
int bsr(REG32 a,EAX b){return x86(85,a,b);}
int bsr(REG32 a,ECX b){return x86(85,a,b);}
int bsr(REG32 a,REG32 b){return x86(85,a,b);}
int bsr(REG32 a,MEM32 b){return x86(85,a,b);}
int bsr(REG32 a,R_M32 b){return x86(85,a,b);}
int bswap(EAX a){return x86(86,a);}
int bswap(ECX a){return x86(86,a);}
int bswap(REG32 a){return x86(86,a);}
int bt(AX a,AX b){return x86(87,a,b);}
int bt(AX a,DX b){return x86(87,a,b);}
int bt(AX a,CX b){return x86(87,a,b);}
int bt(AX a,REG16 b){return x86(87,a,b);}
int bt(DX a,AX b){return x86(87,a,b);}
int bt(DX a,DX b){return x86(87,a,b);}
int bt(DX a,CX b){return x86(87,a,b);}
int bt(DX a,REG16 b){return x86(87,a,b);}
int bt(CX a,AX b){return x86(87,a,b);}
int bt(CX a,DX b){return x86(87,a,b);}
int bt(CX a,CX b){return x86(87,a,b);}
int bt(CX a,REG16 b){return x86(87,a,b);}
int bt(REG16 a,AX b){return x86(87,a,b);}
int bt(REG16 a,DX b){return x86(87,a,b);}
int bt(REG16 a,CX b){return x86(87,a,b);}
int bt(REG16 a,REG16 b){return x86(87,a,b);}
int bt(MEM16 a,AX b){return x86(87,a,b);}
int bt(MEM16 a,DX b){return x86(87,a,b);}
int bt(MEM16 a,CX b){return x86(87,a,b);}
int bt(MEM16 a,REG16 b){return x86(87,a,b);}
int bt(R_M16 a,AX b){return x86(87,a,b);}
int bt(R_M16 a,DX b){return x86(87,a,b);}
int bt(R_M16 a,CX b){return x86(87,a,b);}
int bt(R_M16 a,REG16 b){return x86(87,a,b);}
int bt(EAX a,EAX b){return x86(88,a,b);}
int bt(EAX a,ECX b){return x86(88,a,b);}
int bt(EAX a,REG32 b){return x86(88,a,b);}
int bt(ECX a,EAX b){return x86(88,a,b);}
int bt(ECX a,ECX b){return x86(88,a,b);}
int bt(ECX a,REG32 b){return x86(88,a,b);}
int bt(REG32 a,EAX b){return x86(88,a,b);}
int bt(REG32 a,ECX b){return x86(88,a,b);}
int bt(REG32 a,REG32 b){return x86(88,a,b);}
int bt(MEM32 a,EAX b){return x86(88,a,b);}
int bt(MEM32 a,ECX b){return x86(88,a,b);}
int bt(MEM32 a,REG32 b){return x86(88,a,b);}
int bt(R_M32 a,EAX b){return x86(88,a,b);}
int bt(R_M32 a,ECX b){return x86(88,a,b);}
int bt(R_M32 a,REG32 b){return x86(88,a,b);}
int btc(AX a,AX b){return x86(91,a,b);}
int btc(AX a,DX b){return x86(91,a,b);}
int btc(AX a,CX b){return x86(91,a,b);}
int btc(AX a,REG16 b){return x86(91,a,b);}
int btc(DX a,AX b){return x86(91,a,b);}
int btc(DX a,DX b){return x86(91,a,b);}
int btc(DX a,CX b){return x86(91,a,b);}
int btc(DX a,REG16 b){return x86(91,a,b);}
int btc(CX a,AX b){return x86(91,a,b);}
int btc(CX a,DX b){return x86(91,a,b);}
int btc(CX a,CX b){return x86(91,a,b);}
int btc(CX a,REG16 b){return x86(91,a,b);}
int btc(REG16 a,AX b){return x86(91,a,b);}
int btc(REG16 a,DX b){return x86(91,a,b);}
int btc(REG16 a,CX b){return x86(91,a,b);}
int btc(REG16 a,REG16 b){return x86(91,a,b);}
int btc(MEM16 a,AX b){return x86(91,a,b);}
int btc(MEM16 a,DX b){return x86(91,a,b);}
int btc(MEM16 a,CX b){return x86(91,a,b);}
int btc(MEM16 a,REG16 b){return x86(91,a,b);}
int btc(R_M16 a,AX b){return x86(91,a,b);}
int btc(R_M16 a,DX b){return x86(91,a,b);}
int btc(R_M16 a,CX b){return x86(91,a,b);}
int btc(R_M16 a,REG16 b){return x86(91,a,b);}
int btc(EAX a,EAX b){return x86(92,a,b);}
int btc(EAX a,ECX b){return x86(92,a,b);}
int btc(EAX a,REG32 b){return x86(92,a,b);}
int btc(ECX a,EAX b){return x86(92,a,b);}
int btc(ECX a,ECX b){return x86(92,a,b);}
int btc(ECX a,REG32 b){return x86(92,a,b);}
int btc(REG32 a,EAX b){return x86(92,a,b);}
int btc(REG32 a,ECX b){return x86(92,a,b);}
int btc(REG32 a,REG32 b){return x86(92,a,b);}
int btc(MEM32 a,EAX b){return x86(92,a,b);}
int btc(MEM32 a,ECX b){return x86(92,a,b);}
int btc(MEM32 a,REG32 b){return x86(92,a,b);}
int btc(R_M32 a,EAX b){return x86(92,a,b);}
int btc(R_M32 a,ECX b){return x86(92,a,b);}
int btc(R_M32 a,REG32 b){return x86(92,a,b);}
int btr(AX a,AX b){return x86(95,a,b);}
int btr(AX a,DX b){return x86(95,a,b);}
int btr(AX a,CX b){return x86(95,a,b);}
int btr(AX a,REG16 b){return x86(95,a,b);}
int btr(DX a,AX b){return x86(95,a,b);}
int btr(DX a,DX b){return x86(95,a,b);}
int btr(DX a,CX b){return x86(95,a,b);}
int btr(DX a,REG16 b){return x86(95,a,b);}
int btr(CX a,AX b){return x86(95,a,b);}
int btr(CX a,DX b){return x86(95,a,b);}
int btr(CX a,CX b){return x86(95,a,b);}
int btr(CX a,REG16 b){return x86(95,a,b);}
int btr(REG16 a,AX b){return x86(95,a,b);}
int btr(REG16 a,DX b){return x86(95,a,b);}
int btr(REG16 a,CX b){return x86(95,a,b);}
int btr(REG16 a,REG16 b){return x86(95,a,b);}
int btr(MEM16 a,AX b){return x86(95,a,b);}
int btr(MEM16 a,DX b){return x86(95,a,b);}
int btr(MEM16 a,CX b){return x86(95,a,b);}
int btr(MEM16 a,REG16 b){return x86(95,a,b);}
int btr(R_M16 a,AX b){return x86(95,a,b);}
int btr(R_M16 a,DX b){return x86(95,a,b);}
int btr(R_M16 a,CX b){return x86(95,a,b);}
int btr(R_M16 a,REG16 b){return x86(95,a,b);}
int btr(EAX a,EAX b){return x86(96,a,b);}
int btr(EAX a,ECX b){return x86(96,a,b);}
int btr(EAX a,REG32 b){return x86(96,a,b);}
int btr(ECX a,EAX b){return x86(96,a,b);}
int btr(ECX a,ECX b){return x86(96,a,b);}
int btr(ECX a,REG32 b){return x86(96,a,b);}
int btr(REG32 a,EAX b){return x86(96,a,b);}
int btr(REG32 a,ECX b){return x86(96,a,b);}
int btr(REG32 a,REG32 b){return x86(96,a,b);}
int btr(MEM32 a,EAX b){return x86(96,a,b);}
int btr(MEM32 a,ECX b){return x86(96,a,b);}
int btr(MEM32 a,REG32 b){return x86(96,a,b);}
int btr(R_M32 a,EAX b){return x86(96,a,b);}
int btr(R_M32 a,ECX b){return x86(96,a,b);}
int btr(R_M32 a,REG32 b){return x86(96,a,b);}
int bts(AX a,AX b){return x86(99,a,b);}
int bts(AX a,DX b){return x86(99,a,b);}
int bts(AX a,CX b){return x86(99,a,b);}
int bts(AX a,REG16 b){return x86(99,a,b);}
int bts(DX a,AX b){return x86(99,a,b);}
int bts(DX a,DX b){return x86(99,a,b);}
int bts(DX a,CX b){return x86(99,a,b);}
int bts(DX a,REG16 b){return x86(99,a,b);}
int bts(CX a,AX b){return x86(99,a,b);}
int bts(CX a,DX b){return x86(99,a,b);}
int bts(CX a,CX b){return x86(99,a,b);}
int bts(CX a,REG16 b){return x86(99,a,b);}
int bts(REG16 a,AX b){return x86(99,a,b);}
int bts(REG16 a,DX b){return x86(99,a,b);}
int bts(REG16 a,CX b){return x86(99,a,b);}
int bts(REG16 a,REG16 b){return x86(99,a,b);}
int bts(MEM16 a,AX b){return x86(99,a,b);}
int bts(MEM16 a,DX b){return x86(99,a,b);}
int bts(MEM16 a,CX b){return x86(99,a,b);}
int bts(MEM16 a,REG16 b){return x86(99,a,b);}
int bts(R_M16 a,AX b){return x86(99,a,b);}
int bts(R_M16 a,DX b){return x86(99,a,b);}
int bts(R_M16 a,CX b){return x86(99,a,b);}
int bts(R_M16 a,REG16 b){return x86(99,a,b);}
int bts(EAX a,EAX b){return x86(100,a,b);}
int bts(EAX a,ECX b){return x86(100,a,b);}
int bts(EAX a,REG32 b){return x86(100,a,b);}
int bts(ECX a,EAX b){return x86(100,a,b);}
int bts(ECX a,ECX b){return x86(100,a,b);}
int bts(ECX a,REG32 b){return x86(100,a,b);}
int bts(REG32 a,EAX b){return x86(100,a,b);}
int bts(REG32 a,ECX b){return x86(100,a,b);}
int bts(REG32 a,REG32 b){return x86(100,a,b);}
int bts(MEM32 a,EAX b){return x86(100,a,b);}
int bts(MEM32 a,ECX b){return x86(100,a,b);}
int bts(MEM32 a,REG32 b){return x86(100,a,b);}
int bts(R_M32 a,EAX b){return x86(100,a,b);}
int bts(R_M32 a,ECX b){return x86(100,a,b);}
int bts(R_M32 a,REG32 b){return x86(100,a,b);}
int bts(AX a,int b){return x86(101,a,(IMM)b);}
int bts(AX a,char b){return x86(101,a,(IMM)b);}
int bts(AX a,short b){return x86(101,a,(IMM)b);}
int bts(AX a,REF b){return x86(101,a,b);}
int bts(DX a,int b){return x86(101,a,(IMM)b);}
int bts(DX a,char b){return x86(101,a,(IMM)b);}
int bts(DX a,short b){return x86(101,a,(IMM)b);}
int bts(DX a,REF b){return x86(101,a,b);}
int bts(CX a,int b){return x86(101,a,(IMM)b);}
int bts(CX a,char b){return x86(101,a,(IMM)b);}
int bts(CX a,short b){return x86(101,a,(IMM)b);}
int bts(CX a,REF b){return x86(101,a,b);}
int bts(REG16 a,int b){return x86(101,a,(IMM)b);}
int bts(REG16 a,char b){return x86(101,a,(IMM)b);}
int bts(REG16 a,short b){return x86(101,a,(IMM)b);}
int bts(REG16 a,REF b){return x86(101,a,b);}
int bts(MEM16 a,int b){return x86(101,a,(IMM)b);}
int bts(MEM16 a,char b){return x86(101,a,(IMM)b);}
int bts(MEM16 a,short b){return x86(101,a,(IMM)b);}
int bts(MEM16 a,REF b){return x86(101,a,b);}
int bts(R_M16 a,int b){return x86(101,a,(IMM)b);}
int bts(R_M16 a,char b){return x86(101,a,(IMM)b);}
int bts(R_M16 a,short b){return x86(101,a,(IMM)b);}
int bts(R_M16 a,REF b){return x86(101,a,b);}
int bts(EAX a,int b){return x86(102,a,(IMM)b);}
int bts(EAX a,char b){return x86(102,a,(IMM)b);}
int bts(EAX a,short b){return x86(102,a,(IMM)b);}
int bts(EAX a,REF b){return x86(102,a,b);}
int bts(ECX a,int b){return x86(102,a,(IMM)b);}
int bts(ECX a,char b){return x86(102,a,(IMM)b);}
int bts(ECX a,short b){return x86(102,a,(IMM)b);}
int bts(ECX a,REF b){return x86(102,a,b);}
int bts(REG32 a,int b){return x86(102,a,(IMM)b);}
int bts(REG32 a,char b){return x86(102,a,(IMM)b);}
int bts(REG32 a,short b){return x86(102,a,(IMM)b);}
int bts(REG32 a,REF b){return x86(102,a,b);}
int bts(MEM32 a,int b){return x86(102,a,(IMM)b);}
int bts(MEM32 a,char b){return x86(102,a,(IMM)b);}
int bts(MEM32 a,short b){return x86(102,a,(IMM)b);}
int bts(MEM32 a,REF b){return x86(102,a,b);}
int bts(R_M32 a,int b){return x86(102,a,(IMM)b);}
int bts(R_M32 a,char b){return x86(102,a,(IMM)b);}
int bts(R_M32 a,short b){return x86(102,a,(IMM)b);}
int bts(R_M32 a,REF b){return x86(102,a,b);}
int lock_btc(MEM16 a,AX b){return x86(103,a,b);}
int lock_btc(MEM16 a,DX b){return x86(103,a,b);}
int lock_btc(MEM16 a,CX b){return x86(103,a,b);}
int lock_btc(MEM16 a,REG16 b){return x86(103,a,b);}
int lock_btc(MEM32 a,EAX b){return x86(104,a,b);}
int lock_btc(MEM32 a,ECX b){return x86(104,a,b);}
int lock_btc(MEM32 a,REG32 b){return x86(104,a,b);}
int lock_btr(MEM16 a,AX b){return x86(107,a,b);}
int lock_btr(MEM16 a,DX b){return x86(107,a,b);}
int lock_btr(MEM16 a,CX b){return x86(107,a,b);}
int lock_btr(MEM16 a,REG16 b){return x86(107,a,b);}
int lock_btr(MEM32 a,EAX b){return x86(108,a,b);}
int lock_btr(MEM32 a,ECX b){return x86(108,a,b);}
int lock_btr(MEM32 a,REG32 b){return x86(108,a,b);}
int lock_bts(MEM16 a,AX b){return x86(111,a,b);}
int lock_bts(MEM16 a,DX b){return x86(111,a,b);}
int lock_bts(MEM16 a,CX b){return x86(111,a,b);}
int lock_bts(MEM16 a,REG16 b){return x86(111,a,b);}
int lock_bts(MEM32 a,EAX b){return x86(112,a,b);}
int lock_bts(MEM32 a,ECX b){return x86(112,a,b);}
int lock_bts(MEM32 a,REG32 b){return x86(112,a,b);}
int lock_bts(MEM16 a,int b){return x86(113,a,(IMM)b);}
int lock_bts(MEM16 a,char b){return x86(113,a,(IMM)b);}
int lock_bts(MEM16 a,short b){return x86(113,a,(IMM)b);}
int lock_bts(MEM16 a,REF b){return x86(113,a,b);}
int lock_bts(MEM32 a,int b){return x86(114,a,(IMM)b);}
int lock_bts(MEM32 a,char b){return x86(114,a,(IMM)b);}
int lock_bts(MEM32 a,short b){return x86(114,a,(IMM)b);}
int lock_bts(MEM32 a,REF b){return x86(114,a,b);}
int call(int a){return x86(115,(IMM)a);}
int call(char a){return x86(115,(IMM)a);}
int call(short a){return x86(115,(IMM)a);}
int call(REF a){return x86(115,a);}
int call(AX a){return x86(116,a);}
int call(DX a){return x86(116,a);}
int call(CX a){return x86(116,a);}
int call(REG16 a){return x86(116,a);}
int call(MEM16 a){return x86(116,a);}
int call(R_M16 a){return x86(116,a);}
int call(EAX a){return x86(117,a);}
int call(ECX a){return x86(117,a);}
int call(REG32 a){return x86(117,a);}
int call(MEM32 a){return x86(117,a);}
int call(R_M32 a){return x86(117,a);}
int cbw(){return x86(118);}
int cwd(){return x86(119);}
int cdq(){return x86(120);}
int cwde(){return x86(121);}
int clc(){return x86(122);}
int cld(){return x86(123);}
int cli(){return x86(124);}
int clflush(MEM8 a){return x86(125,a);}
int clflush(MEM16 a){return x86(125,a);}
int clflush(MEM32 a){return x86(125,a);}
int clflush(MEM64 a){return x86(125,a);}
int clflush(MEM128 a){return x86(125,a);}
int cmc(){return x86(126);}
int cmovo(AX a,AX b){return x86(127,a,b);}
int cmovo(AX a,DX b){return x86(127,a,b);}
int cmovo(AX a,CX b){return x86(127,a,b);}
int cmovo(AX a,REG16 b){return x86(127,a,b);}
int cmovo(AX a,MEM16 b){return x86(127,a,b);}
int cmovo(AX a,R_M16 b){return x86(127,a,b);}
int cmovo(DX a,AX b){return x86(127,a,b);}
int cmovo(DX a,DX b){return x86(127,a,b);}
int cmovo(DX a,CX b){return x86(127,a,b);}
int cmovo(DX a,REG16 b){return x86(127,a,b);}
int cmovo(DX a,MEM16 b){return x86(127,a,b);}
int cmovo(DX a,R_M16 b){return x86(127,a,b);}
int cmovo(CX a,AX b){return x86(127,a,b);}
int cmovo(CX a,DX b){return x86(127,a,b);}
int cmovo(CX a,CX b){return x86(127,a,b);}
int cmovo(CX a,REG16 b){return x86(127,a,b);}
int cmovo(CX a,MEM16 b){return x86(127,a,b);}
int cmovo(CX a,R_M16 b){return x86(127,a,b);}
int cmovo(REG16 a,AX b){return x86(127,a,b);}
int cmovo(REG16 a,DX b){return x86(127,a,b);}
int cmovo(REG16 a,CX b){return x86(127,a,b);}
int cmovo(REG16 a,REG16 b){return x86(127,a,b);}
int cmovo(REG16 a,MEM16 b){return x86(127,a,b);}
int cmovo(REG16 a,R_M16 b){return x86(127,a,b);}
int cmovno(AX a,AX b){return x86(128,a,b);}
int cmovno(AX a,DX b){return x86(128,a,b);}
int cmovno(AX a,CX b){return x86(128,a,b);}
int cmovno(AX a,REG16 b){return x86(128,a,b);}
int cmovno(AX a,MEM16 b){return x86(128,a,b);}
int cmovno(AX a,R_M16 b){return x86(128,a,b);}
int cmovno(DX a,AX b){return x86(128,a,b);}
int cmovno(DX a,DX b){return x86(128,a,b);}
int cmovno(DX a,CX b){return x86(128,a,b);}
int cmovno(DX a,REG16 b){return x86(128,a,b);}
int cmovno(DX a,MEM16 b){return x86(128,a,b);}
int cmovno(DX a,R_M16 b){return x86(128,a,b);}
int cmovno(CX a,AX b){return x86(128,a,b);}
int cmovno(CX a,DX b){return x86(128,a,b);}
int cmovno(CX a,CX b){return x86(128,a,b);}
int cmovno(CX a,REG16 b){return x86(128,a,b);}
int cmovno(CX a,MEM16 b){return x86(128,a,b);}
int cmovno(CX a,R_M16 b){return x86(128,a,b);}
int cmovno(REG16 a,AX b){return x86(128,a,b);}
int cmovno(REG16 a,DX b){return x86(128,a,b);}
int cmovno(REG16 a,CX b){return x86(128,a,b);}
int cmovno(REG16 a,REG16 b){return x86(128,a,b);}
int cmovno(REG16 a,MEM16 b){return x86(128,a,b);}
int cmovno(REG16 a,R_M16 b){return x86(128,a,b);}
int cmovb(AX a,AX b){return x86(129,a,b);}
int cmovb(AX a,DX b){return x86(129,a,b);}
int cmovb(AX a,CX b){return x86(129,a,b);}
int cmovb(AX a,REG16 b){return x86(129,a,b);}
int cmovb(AX a,MEM16 b){return x86(129,a,b);}
int cmovb(AX a,R_M16 b){return x86(129,a,b);}
int cmovb(DX a,AX b){return x86(129,a,b);}
int cmovb(DX a,DX b){return x86(129,a,b);}
int cmovb(DX a,CX b){return x86(129,a,b);}
int cmovb(DX a,REG16 b){return x86(129,a,b);}
int cmovb(DX a,MEM16 b){return x86(129,a,b);}
int cmovb(DX a,R_M16 b){return x86(129,a,b);}
int cmovb(CX a,AX b){return x86(129,a,b);}
int cmovb(CX a,DX b){return x86(129,a,b);}
int cmovb(CX a,CX b){return x86(129,a,b);}
int cmovb(CX a,REG16 b){return x86(129,a,b);}
int cmovb(CX a,MEM16 b){return x86(129,a,b);}
int cmovb(CX a,R_M16 b){return x86(129,a,b);}
int cmovb(REG16 a,AX b){return x86(129,a,b);}
int cmovb(REG16 a,DX b){return x86(129,a,b);}
int cmovb(REG16 a,CX b){return x86(129,a,b);}
int cmovb(REG16 a,REG16 b){return x86(129,a,b);}
int cmovb(REG16 a,MEM16 b){return x86(129,a,b);}
int cmovb(REG16 a,R_M16 b){return x86(129,a,b);}
int cmovc(AX a,AX b){return x86(130,a,b);}
int cmovc(AX a,DX b){return x86(130,a,b);}
int cmovc(AX a,CX b){return x86(130,a,b);}
int cmovc(AX a,REG16 b){return x86(130,a,b);}
int cmovc(AX a,MEM16 b){return x86(130,a,b);}
int cmovc(AX a,R_M16 b){return x86(130,a,b);}
int cmovc(DX a,AX b){return x86(130,a,b);}
int cmovc(DX a,DX b){return x86(130,a,b);}
int cmovc(DX a,CX b){return x86(130,a,b);}
int cmovc(DX a,REG16 b){return x86(130,a,b);}
int cmovc(DX a,MEM16 b){return x86(130,a,b);}
int cmovc(DX a,R_M16 b){return x86(130,a,b);}
int cmovc(CX a,AX b){return x86(130,a,b);}
int cmovc(CX a,DX b){return x86(130,a,b);}
int cmovc(CX a,CX b){return x86(130,a,b);}
int cmovc(CX a,REG16 b){return x86(130,a,b);}
int cmovc(CX a,MEM16 b){return x86(130,a,b);}
int cmovc(CX a,R_M16 b){return x86(130,a,b);}
int cmovc(REG16 a,AX b){return x86(130,a,b);}
int cmovc(REG16 a,DX b){return x86(130,a,b);}
int cmovc(REG16 a,CX b){return x86(130,a,b);}
int cmovc(REG16 a,REG16 b){return x86(130,a,b);}
int cmovc(REG16 a,MEM16 b){return x86(130,a,b);}
int cmovc(REG16 a,R_M16 b){return x86(130,a,b);}
int cmovnea(AX a,AX b){return x86(131,a,b);}
int cmovnea(AX a,DX b){return x86(131,a,b);}
int cmovnea(AX a,CX b){return x86(131,a,b);}
int cmovnea(AX a,REG16 b){return x86(131,a,b);}
int cmovnea(AX a,MEM16 b){return x86(131,a,b);}
int cmovnea(AX a,R_M16 b){return x86(131,a,b);}
int cmovnea(DX a,AX b){return x86(131,a,b);}
int cmovnea(DX a,DX b){return x86(131,a,b);}
int cmovnea(DX a,CX b){return x86(131,a,b);}
int cmovnea(DX a,REG16 b){return x86(131,a,b);}
int cmovnea(DX a,MEM16 b){return x86(131,a,b);}
int cmovnea(DX a,R_M16 b){return x86(131,a,b);}
int cmovnea(CX a,AX b){return x86(131,a,b);}
int cmovnea(CX a,DX b){return x86(131,a,b);}
int cmovnea(CX a,CX b){return x86(131,a,b);}
int cmovnea(CX a,REG16 b){return x86(131,a,b);}
int cmovnea(CX a,MEM16 b){return x86(131,a,b);}
int cmovnea(CX a,R_M16 b){return x86(131,a,b);}
int cmovnea(REG16 a,AX b){return x86(131,a,b);}
int cmovnea(REG16 a,DX b){return x86(131,a,b);}
int cmovnea(REG16 a,CX b){return x86(131,a,b);}
int cmovnea(REG16 a,REG16 b){return x86(131,a,b);}
int cmovnea(REG16 a,MEM16 b){return x86(131,a,b);}
int cmovnea(REG16 a,R_M16 b){return x86(131,a,b);}
int cmovae(AX a,AX b){return x86(132,a,b);}
int cmovae(AX a,DX b){return x86(132,a,b);}
int cmovae(AX a,CX b){return x86(132,a,b);}
int cmovae(AX a,REG16 b){return x86(132,a,b);}
int cmovae(AX a,MEM16 b){return x86(132,a,b);}
int cmovae(AX a,R_M16 b){return x86(132,a,b);}
int cmovae(DX a,AX b){return x86(132,a,b);}
int cmovae(DX a,DX b){return x86(132,a,b);}
int cmovae(DX a,CX b){return x86(132,a,b);}
int cmovae(DX a,REG16 b){return x86(132,a,b);}
int cmovae(DX a,MEM16 b){return x86(132,a,b);}
int cmovae(DX a,R_M16 b){return x86(132,a,b);}
int cmovae(CX a,AX b){return x86(132,a,b);}
int cmovae(CX a,DX b){return x86(132,a,b);}
int cmovae(CX a,CX b){return x86(132,a,b);}
int cmovae(CX a,REG16 b){return x86(132,a,b);}
int cmovae(CX a,MEM16 b){return x86(132,a,b);}
int cmovae(CX a,R_M16 b){return x86(132,a,b);}
int cmovae(REG16 a,AX b){return x86(132,a,b);}
int cmovae(REG16 a,DX b){return x86(132,a,b);}
int cmovae(REG16 a,CX b){return x86(132,a,b);}
int cmovae(REG16 a,REG16 b){return x86(132,a,b);}
int cmovae(REG16 a,MEM16 b){return x86(132,a,b);}
int cmovae(REG16 a,R_M16 b){return x86(132,a,b);}
int cmovnb(AX a,AX b){return x86(133,a,b);}
int cmovnb(AX a,DX b){return x86(133,a,b);}
int cmovnb(AX a,CX b){return x86(133,a,b);}
int cmovnb(AX a,REG16 b){return x86(133,a,b);}
int cmovnb(AX a,MEM16 b){return x86(133,a,b);}
int cmovnb(AX a,R_M16 b){return x86(133,a,b);}
int cmovnb(DX a,AX b){return x86(133,a,b);}
int cmovnb(DX a,DX b){return x86(133,a,b);}
int cmovnb(DX a,CX b){return x86(133,a,b);}
int cmovnb(DX a,REG16 b){return x86(133,a,b);}
int cmovnb(DX a,MEM16 b){return x86(133,a,b);}
int cmovnb(DX a,R_M16 b){return x86(133,a,b);}
int cmovnb(CX a,AX b){return x86(133,a,b);}
int cmovnb(CX a,DX b){return x86(133,a,b);}
int cmovnb(CX a,CX b){return x86(133,a,b);}
int cmovnb(CX a,REG16 b){return x86(133,a,b);}
int cmovnb(CX a,MEM16 b){return x86(133,a,b);}
int cmovnb(CX a,R_M16 b){return x86(133,a,b);}
int cmovnb(REG16 a,AX b){return x86(133,a,b);}
int cmovnb(REG16 a,DX b){return x86(133,a,b);}
int cmovnb(REG16 a,CX b){return x86(133,a,b);}
int cmovnb(REG16 a,REG16 b){return x86(133,a,b);}
int cmovnb(REG16 a,MEM16 b){return x86(133,a,b);}
int cmovnb(REG16 a,R_M16 b){return x86(133,a,b);}
int cmovnc(AX a,AX b){return x86(134,a,b);}
int cmovnc(AX a,DX b){return x86(134,a,b);}
int cmovnc(AX a,CX b){return x86(134,a,b);}
int cmovnc(AX a,REG16 b){return x86(134,a,b);}
int cmovnc(AX a,MEM16 b){return x86(134,a,b);}
int cmovnc(AX a,R_M16 b){return x86(134,a,b);}
int cmovnc(DX a,AX b){return x86(134,a,b);}
int cmovnc(DX a,DX b){return x86(134,a,b);}
int cmovnc(DX a,CX b){return x86(134,a,b);}
int cmovnc(DX a,REG16 b){return x86(134,a,b);}
int cmovnc(DX a,MEM16 b){return x86(134,a,b);}
int cmovnc(DX a,R_M16 b){return x86(134,a,b);}
int cmovnc(CX a,AX b){return x86(134,a,b);}
int cmovnc(CX a,DX b){return x86(134,a,b);}
int cmovnc(CX a,CX b){return x86(134,a,b);}
int cmovnc(CX a,REG16 b){return x86(134,a,b);}
int cmovnc(CX a,MEM16 b){return x86(134,a,b);}
int cmovnc(CX a,R_M16 b){return x86(134,a,b);}
int cmovnc(REG16 a,AX b){return x86(134,a,b);}
int cmovnc(REG16 a,DX b){return x86(134,a,b);}
int cmovnc(REG16 a,CX b){return x86(134,a,b);}
int cmovnc(REG16 a,REG16 b){return x86(134,a,b);}
int cmovnc(REG16 a,MEM16 b){return x86(134,a,b);}
int cmovnc(REG16 a,R_M16 b){return x86(134,a,b);}
int cmove(AX a,AX b){return x86(135,a,b);}
int cmove(AX a,DX b){return x86(135,a,b);}
int cmove(AX a,CX b){return x86(135,a,b);}
int cmove(AX a,REG16 b){return x86(135,a,b);}
int cmove(AX a,MEM16 b){return x86(135,a,b);}
int cmove(AX a,R_M16 b){return x86(135,a,b);}
int cmove(DX a,AX b){return x86(135,a,b);}
int cmove(DX a,DX b){return x86(135,a,b);}
int cmove(DX a,CX b){return x86(135,a,b);}
int cmove(DX a,REG16 b){return x86(135,a,b);}
int cmove(DX a,MEM16 b){return x86(135,a,b);}
int cmove(DX a,R_M16 b){return x86(135,a,b);}
int cmove(CX a,AX b){return x86(135,a,b);}
int cmove(CX a,DX b){return x86(135,a,b);}
int cmove(CX a,CX b){return x86(135,a,b);}
int cmove(CX a,REG16 b){return x86(135,a,b);}
int cmove(CX a,MEM16 b){return x86(135,a,b);}
int cmove(CX a,R_M16 b){return x86(135,a,b);}
int cmove(REG16 a,AX b){return x86(135,a,b);}
int cmove(REG16 a,DX b){return x86(135,a,b);}
int cmove(REG16 a,CX b){return x86(135,a,b);}
int cmove(REG16 a,REG16 b){return x86(135,a,b);}
int cmove(REG16 a,MEM16 b){return x86(135,a,b);}
int cmove(REG16 a,R_M16 b){return x86(135,a,b);}
int cmovz(AX a,AX b){return x86(136,a,b);}
int cmovz(AX a,DX b){return x86(136,a,b);}
int cmovz(AX a,CX b){return x86(136,a,b);}
int cmovz(AX a,REG16 b){return x86(136,a,b);}
int cmovz(AX a,MEM16 b){return x86(136,a,b);}
int cmovz(AX a,R_M16 b){return x86(136,a,b);}
int cmovz(DX a,AX b){return x86(136,a,b);}
int cmovz(DX a,DX b){return x86(136,a,b);}
int cmovz(DX a,CX b){return x86(136,a,b);}
int cmovz(DX a,REG16 b){return x86(136,a,b);}
int cmovz(DX a,MEM16 b){return x86(136,a,b);}
int cmovz(DX a,R_M16 b){return x86(136,a,b);}
int cmovz(CX a,AX b){return x86(136,a,b);}
int cmovz(CX a,DX b){return x86(136,a,b);}
int cmovz(CX a,CX b){return x86(136,a,b);}
int cmovz(CX a,REG16 b){return x86(136,a,b);}
int cmovz(CX a,MEM16 b){return x86(136,a,b);}
int cmovz(CX a,R_M16 b){return x86(136,a,b);}
int cmovz(REG16 a,AX b){return x86(136,a,b);}
int cmovz(REG16 a,DX b){return x86(136,a,b);}
int cmovz(REG16 a,CX b){return x86(136,a,b);}
int cmovz(REG16 a,REG16 b){return x86(136,a,b);}
int cmovz(REG16 a,MEM16 b){return x86(136,a,b);}
int cmovz(REG16 a,R_M16 b){return x86(136,a,b);}
int cmovne(AX a,AX b){return x86(137,a,b);}
int cmovne(AX a,DX b){return x86(137,a,b);}
int cmovne(AX a,CX b){return x86(137,a,b);}
int cmovne(AX a,REG16 b){return x86(137,a,b);}
int cmovne(AX a,MEM16 b){return x86(137,a,b);}
int cmovne(AX a,R_M16 b){return x86(137,a,b);}
int cmovne(DX a,AX b){return x86(137,a,b);}
int cmovne(DX a,DX b){return x86(137,a,b);}
int cmovne(DX a,CX b){return x86(137,a,b);}
int cmovne(DX a,REG16 b){return x86(137,a,b);}
int cmovne(DX a,MEM16 b){return x86(137,a,b);}
int cmovne(DX a,R_M16 b){return x86(137,a,b);}
int cmovne(CX a,AX b){return x86(137,a,b);}
int cmovne(CX a,DX b){return x86(137,a,b);}
int cmovne(CX a,CX b){return x86(137,a,b);}
int cmovne(CX a,REG16 b){return x86(137,a,b);}
int cmovne(CX a,MEM16 b){return x86(137,a,b);}
int cmovne(CX a,R_M16 b){return x86(137,a,b);}
int cmovne(REG16 a,AX b){return x86(137,a,b);}
int cmovne(REG16 a,DX b){return x86(137,a,b);}
int cmovne(REG16 a,CX b){return x86(137,a,b);}
int cmovne(REG16 a,REG16 b){return x86(137,a,b);}
int cmovne(REG16 a,MEM16 b){return x86(137,a,b);}
int cmovne(REG16 a,R_M16 b){return x86(137,a,b);}
int cmovnz(AX a,AX b){return x86(138,a,b);}
int cmovnz(AX a,DX b){return x86(138,a,b);}
int cmovnz(AX a,CX b){return x86(138,a,b);}
int cmovnz(AX a,REG16 b){return x86(138,a,b);}
int cmovnz(AX a,MEM16 b){return x86(138,a,b);}
int cmovnz(AX a,R_M16 b){return x86(138,a,b);}
int cmovnz(DX a,AX b){return x86(138,a,b);}
int cmovnz(DX a,DX b){return x86(138,a,b);}
int cmovnz(DX a,CX b){return x86(138,a,b);}
int cmovnz(DX a,REG16 b){return x86(138,a,b);}
int cmovnz(DX a,MEM16 b){return x86(138,a,b);}
int cmovnz(DX a,R_M16 b){return x86(138,a,b);}
int cmovnz(CX a,AX b){return x86(138,a,b);}
int cmovnz(CX a,DX b){return x86(138,a,b);}
int cmovnz(CX a,CX b){return x86(138,a,b);}
int cmovnz(CX a,REG16 b){return x86(138,a,b);}
int cmovnz(CX a,MEM16 b){return x86(138,a,b);}
int cmovnz(CX a,R_M16 b){return x86(138,a,b);}
int cmovnz(REG16 a,AX b){return x86(138,a,b);}
int cmovnz(REG16 a,DX b){return x86(138,a,b);}
int cmovnz(REG16 a,CX b){return x86(138,a,b);}
int cmovnz(REG16 a,REG16 b){return x86(138,a,b);}
int cmovnz(REG16 a,MEM16 b){return x86(138,a,b);}
int cmovnz(REG16 a,R_M16 b){return x86(138,a,b);}
int cmovbe(AX a,AX b){return x86(139,a,b);}
int cmovbe(AX a,DX b){return x86(139,a,b);}
int cmovbe(AX a,CX b){return x86(139,a,b);}
int cmovbe(AX a,REG16 b){return x86(139,a,b);}
int cmovbe(AX a,MEM16 b){return x86(139,a,b);}
int cmovbe(AX a,R_M16 b){return x86(139,a,b);}
int cmovbe(DX a,AX b){return x86(139,a,b);}
int cmovbe(DX a,DX b){return x86(139,a,b);}
int cmovbe(DX a,CX b){return x86(139,a,b);}
int cmovbe(DX a,REG16 b){return x86(139,a,b);}
int cmovbe(DX a,MEM16 b){return x86(139,a,b);}
int cmovbe(DX a,R_M16 b){return x86(139,a,b);}
int cmovbe(CX a,AX b){return x86(139,a,b);}
int cmovbe(CX a,DX b){return x86(139,a,b);}
int cmovbe(CX a,CX b){return x86(139,a,b);}
int cmovbe(CX a,REG16 b){return x86(139,a,b);}
int cmovbe(CX a,MEM16 b){return x86(139,a,b);}
int cmovbe(CX a,R_M16 b){return x86(139,a,b);}
int cmovbe(REG16 a,AX b){return x86(139,a,b);}
int cmovbe(REG16 a,DX b){return x86(139,a,b);}
int cmovbe(REG16 a,CX b){return x86(139,a,b);}
int cmovbe(REG16 a,REG16 b){return x86(139,a,b);}
int cmovbe(REG16 a,MEM16 b){return x86(139,a,b);}
int cmovbe(REG16 a,R_M16 b){return x86(139,a,b);}
int cmovna(AX a,AX b){return x86(140,a,b);}
int cmovna(AX a,DX b){return x86(140,a,b);}
int cmovna(AX a,CX b){return x86(140,a,b);}
int cmovna(AX a,REG16 b){return x86(140,a,b);}
int cmovna(AX a,MEM16 b){return x86(140,a,b);}
int cmovna(AX a,R_M16 b){return x86(140,a,b);}
int cmovna(DX a,AX b){return x86(140,a,b);}
int cmovna(DX a,DX b){return x86(140,a,b);}
int cmovna(DX a,CX b){return x86(140,a,b);}
int cmovna(DX a,REG16 b){return x86(140,a,b);}
int cmovna(DX a,MEM16 b){return x86(140,a,b);}
int cmovna(DX a,R_M16 b){return x86(140,a,b);}
int cmovna(CX a,AX b){return x86(140,a,b);}
int cmovna(CX a,DX b){return x86(140,a,b);}
int cmovna(CX a,CX b){return x86(140,a,b);}
int cmovna(CX a,REG16 b){return x86(140,a,b);}
int cmovna(CX a,MEM16 b){return x86(140,a,b);}
int cmovna(CX a,R_M16 b){return x86(140,a,b);}
int cmovna(REG16 a,AX b){return x86(140,a,b);}
int cmovna(REG16 a,DX b){return x86(140,a,b);}
int cmovna(REG16 a,CX b){return x86(140,a,b);}
int cmovna(REG16 a,REG16 b){return x86(140,a,b);}
int cmovna(REG16 a,MEM16 b){return x86(140,a,b);}
int cmovna(REG16 a,R_M16 b){return x86(140,a,b);}
int cmova(AX a,AX b){return x86(141,a,b);}
int cmova(AX a,DX b){return x86(141,a,b);}
int cmova(AX a,CX b){return x86(141,a,b);}
int cmova(AX a,REG16 b){return x86(141,a,b);}
int cmova(AX a,MEM16 b){return x86(141,a,b);}
int cmova(AX a,R_M16 b){return x86(141,a,b);}
int cmova(DX a,AX b){return x86(141,a,b);}
int cmova(DX a,DX b){return x86(141,a,b);}
int cmova(DX a,CX b){return x86(141,a,b);}
int cmova(DX a,REG16 b){return x86(141,a,b);}
int cmova(DX a,MEM16 b){return x86(141,a,b);}
int cmova(DX a,R_M16 b){return x86(141,a,b);}
int cmova(CX a,AX b){return x86(141,a,b);}
int cmova(CX a,DX b){return x86(141,a,b);}
int cmova(CX a,CX b){return x86(141,a,b);}
int cmova(CX a,REG16 b){return x86(141,a,b);}
int cmova(CX a,MEM16 b){return x86(141,a,b);}
int cmova(CX a,R_M16 b){return x86(141,a,b);}
int cmova(REG16 a,AX b){return x86(141,a,b);}
int cmova(REG16 a,DX b){return x86(141,a,b);}
int cmova(REG16 a,CX b){return x86(141,a,b);}
int cmova(REG16 a,REG16 b){return x86(141,a,b);}
int cmova(REG16 a,MEM16 b){return x86(141,a,b);}
int cmova(REG16 a,R_M16 b){return x86(141,a,b);}
int cmovnbe(AX a,AX b){return x86(142,a,b);}
int cmovnbe(AX a,DX b){return x86(142,a,b);}
int cmovnbe(AX a,CX b){return x86(142,a,b);}
int cmovnbe(AX a,REG16 b){return x86(142,a,b);}
int cmovnbe(AX a,MEM16 b){return x86(142,a,b);}
int cmovnbe(AX a,R_M16 b){return x86(142,a,b);}
int cmovnbe(DX a,AX b){return x86(142,a,b);}
int cmovnbe(DX a,DX b){return x86(142,a,b);}
int cmovnbe(DX a,CX b){return x86(142,a,b);}
int cmovnbe(DX a,REG16 b){return x86(142,a,b);}
int cmovnbe(DX a,MEM16 b){return x86(142,a,b);}
int cmovnbe(DX a,R_M16 b){return x86(142,a,b);}
int cmovnbe(CX a,AX b){return x86(142,a,b);}
int cmovnbe(CX a,DX b){return x86(142,a,b);}
int cmovnbe(CX a,CX b){return x86(142,a,b);}
int cmovnbe(CX a,REG16 b){return x86(142,a,b);}
int cmovnbe(CX a,MEM16 b){return x86(142,a,b);}
int cmovnbe(CX a,R_M16 b){return x86(142,a,b);}
int cmovnbe(REG16 a,AX b){return x86(142,a,b);}
int cmovnbe(REG16 a,DX b){return x86(142,a,b);}
int cmovnbe(REG16 a,CX b){return x86(142,a,b);}
int cmovnbe(REG16 a,REG16 b){return x86(142,a,b);}
int cmovnbe(REG16 a,MEM16 b){return x86(142,a,b);}
int cmovnbe(REG16 a,R_M16 b){return x86(142,a,b);}
int cmovs(AX a,AX b){return x86(143,a,b);}
int cmovs(AX a,DX b){return x86(143,a,b);}
int cmovs(AX a,CX b){return x86(143,a,b);}
int cmovs(AX a,REG16 b){return x86(143,a,b);}
int cmovs(AX a,MEM16 b){return x86(143,a,b);}
int cmovs(AX a,R_M16 b){return x86(143,a,b);}
int cmovs(DX a,AX b){return x86(143,a,b);}
int cmovs(DX a,DX b){return x86(143,a,b);}
int cmovs(DX a,CX b){return x86(143,a,b);}
int cmovs(DX a,REG16 b){return x86(143,a,b);}
int cmovs(DX a,MEM16 b){return x86(143,a,b);}
int cmovs(DX a,R_M16 b){return x86(143,a,b);}
int cmovs(CX a,AX b){return x86(143,a,b);}
int cmovs(CX a,DX b){return x86(143,a,b);}
int cmovs(CX a,CX b){return x86(143,a,b);}
int cmovs(CX a,REG16 b){return x86(143,a,b);}
int cmovs(CX a,MEM16 b){return x86(143,a,b);}
int cmovs(CX a,R_M16 b){return x86(143,a,b);}
int cmovs(REG16 a,AX b){return x86(143,a,b);}
int cmovs(REG16 a,DX b){return x86(143,a,b);}
int cmovs(REG16 a,CX b){return x86(143,a,b);}
int cmovs(REG16 a,REG16 b){return x86(143,a,b);}
int cmovs(REG16 a,MEM16 b){return x86(143,a,b);}
int cmovs(REG16 a,R_M16 b){return x86(143,a,b);}
int cmovns(AX a,AX b){return x86(144,a,b);}
int cmovns(AX a,DX b){return x86(144,a,b);}
int cmovns(AX a,CX b){return x86(144,a,b);}
int cmovns(AX a,REG16 b){return x86(144,a,b);}
int cmovns(AX a,MEM16 b){return x86(144,a,b);}
int cmovns(AX a,R_M16 b){return x86(144,a,b);}
int cmovns(DX a,AX b){return x86(144,a,b);}
int cmovns(DX a,DX b){return x86(144,a,b);}
int cmovns(DX a,CX b){return x86(144,a,b);}
int cmovns(DX a,REG16 b){return x86(144,a,b);}
int cmovns(DX a,MEM16 b){return x86(144,a,b);}
int cmovns(DX a,R_M16 b){return x86(144,a,b);}
int cmovns(CX a,AX b){return x86(144,a,b);}
int cmovns(CX a,DX b){return x86(144,a,b);}
int cmovns(CX a,CX b){return x86(144,a,b);}
int cmovns(CX a,REG16 b){return x86(144,a,b);}
int cmovns(CX a,MEM16 b){return x86(144,a,b);}
int cmovns(CX a,R_M16 b){return x86(144,a,b);}
int cmovns(REG16 a,AX b){return x86(144,a,b);}
int cmovns(REG16 a,DX b){return x86(144,a,b);}
int cmovns(REG16 a,CX b){return x86(144,a,b);}
int cmovns(REG16 a,REG16 b){return x86(144,a,b);}
int cmovns(REG16 a,MEM16 b){return x86(144,a,b);}
int cmovns(REG16 a,R_M16 b){return x86(144,a,b);}
int cmovp(AX a,AX b){return x86(145,a,b);}
int cmovp(AX a,DX b){return x86(145,a,b);}
int cmovp(AX a,CX b){return x86(145,a,b);}
int cmovp(AX a,REG16 b){return x86(145,a,b);}
int cmovp(AX a,MEM16 b){return x86(145,a,b);}
int cmovp(AX a,R_M16 b){return x86(145,a,b);}
int cmovp(DX a,AX b){return x86(145,a,b);}
int cmovp(DX a,DX b){return x86(145,a,b);}
int cmovp(DX a,CX b){return x86(145,a,b);}
int cmovp(DX a,REG16 b){return x86(145,a,b);}
int cmovp(DX a,MEM16 b){return x86(145,a,b);}
int cmovp(DX a,R_M16 b){return x86(145,a,b);}
int cmovp(CX a,AX b){return x86(145,a,b);}
int cmovp(CX a,DX b){return x86(145,a,b);}
int cmovp(CX a,CX b){return x86(145,a,b);}
int cmovp(CX a,REG16 b){return x86(145,a,b);}
int cmovp(CX a,MEM16 b){return x86(145,a,b);}
int cmovp(CX a,R_M16 b){return x86(145,a,b);}
int cmovp(REG16 a,AX b){return x86(145,a,b);}
int cmovp(REG16 a,DX b){return x86(145,a,b);}
int cmovp(REG16 a,CX b){return x86(145,a,b);}
int cmovp(REG16 a,REG16 b){return x86(145,a,b);}
int cmovp(REG16 a,MEM16 b){return x86(145,a,b);}
int cmovp(REG16 a,R_M16 b){return x86(145,a,b);}
int cmovpe(AX a,AX b){return x86(146,a,b);}
int cmovpe(AX a,DX b){return x86(146,a,b);}
int cmovpe(AX a,CX b){return x86(146,a,b);}
int cmovpe(AX a,REG16 b){return x86(146,a,b);}
int cmovpe(AX a,MEM16 b){return x86(146,a,b);}
int cmovpe(AX a,R_M16 b){return x86(146,a,b);}
int cmovpe(DX a,AX b){return x86(146,a,b);}
int cmovpe(DX a,DX b){return x86(146,a,b);}
int cmovpe(DX a,CX b){return x86(146,a,b);}
int cmovpe(DX a,REG16 b){return x86(146,a,b);}
int cmovpe(DX a,MEM16 b){return x86(146,a,b);}
int cmovpe(DX a,R_M16 b){return x86(146,a,b);}
int cmovpe(CX a,AX b){return x86(146,a,b);}
int cmovpe(CX a,DX b){return x86(146,a,b);}
int cmovpe(CX a,CX b){return x86(146,a,b);}
int cmovpe(CX a,REG16 b){return x86(146,a,b);}
int cmovpe(CX a,MEM16 b){return x86(146,a,b);}
int cmovpe(CX a,R_M16 b){return x86(146,a,b);}
int cmovpe(REG16 a,AX b){return x86(146,a,b);}
int cmovpe(REG16 a,DX b){return x86(146,a,b);}
int cmovpe(REG16 a,CX b){return x86(146,a,b);}
int cmovpe(REG16 a,REG16 b){return x86(146,a,b);}
int cmovpe(REG16 a,MEM16 b){return x86(146,a,b);}
int cmovpe(REG16 a,R_M16 b){return x86(146,a,b);}
int cmovnp(AX a,AX b){return x86(147,a,b);}
int cmovnp(AX a,DX b){return x86(147,a,b);}
int cmovnp(AX a,CX b){return x86(147,a,b);}
int cmovnp(AX a,REG16 b){return x86(147,a,b);}
int cmovnp(AX a,MEM16 b){return x86(147,a,b);}
int cmovnp(AX a,R_M16 b){return x86(147,a,b);}
int cmovnp(DX a,AX b){return x86(147,a,b);}
int cmovnp(DX a,DX b){return x86(147,a,b);}
int cmovnp(DX a,CX b){return x86(147,a,b);}
int cmovnp(DX a,REG16 b){return x86(147,a,b);}
int cmovnp(DX a,MEM16 b){return x86(147,a,b);}
int cmovnp(DX a,R_M16 b){return x86(147,a,b);}
int cmovnp(CX a,AX b){return x86(147,a,b);}
int cmovnp(CX a,DX b){return x86(147,a,b);}
int cmovnp(CX a,CX b){return x86(147,a,b);}
int cmovnp(CX a,REG16 b){return x86(147,a,b);}
int cmovnp(CX a,MEM16 b){return x86(147,a,b);}
int cmovnp(CX a,R_M16 b){return x86(147,a,b);}
int cmovnp(REG16 a,AX b){return x86(147,a,b);}
int cmovnp(REG16 a,DX b){return x86(147,a,b);}
int cmovnp(REG16 a,CX b){return x86(147,a,b);}
int cmovnp(REG16 a,REG16 b){return x86(147,a,b);}
int cmovnp(REG16 a,MEM16 b){return x86(147,a,b);}
int cmovnp(REG16 a,R_M16 b){return x86(147,a,b);}
int cmovpo(AX a,AX b){return x86(148,a,b);}
int cmovpo(AX a,DX b){return x86(148,a,b);}
int cmovpo(AX a,CX b){return x86(148,a,b);}
int cmovpo(AX a,REG16 b){return x86(148,a,b);}
int cmovpo(AX a,MEM16 b){return x86(148,a,b);}
int cmovpo(AX a,R_M16 b){return x86(148,a,b);}
int cmovpo(DX a,AX b){return x86(148,a,b);}
int cmovpo(DX a,DX b){return x86(148,a,b);}
int cmovpo(DX a,CX b){return x86(148,a,b);}
int cmovpo(DX a,REG16 b){return x86(148,a,b);}
int cmovpo(DX a,MEM16 b){return x86(148,a,b);}
int cmovpo(DX a,R_M16 b){return x86(148,a,b);}
int cmovpo(CX a,AX b){return x86(148,a,b);}
int cmovpo(CX a,DX b){return x86(148,a,b);}
int cmovpo(CX a,CX b){return x86(148,a,b);}
int cmovpo(CX a,REG16 b){return x86(148,a,b);}
int cmovpo(CX a,MEM16 b){return x86(148,a,b);}
int cmovpo(CX a,R_M16 b){return x86(148,a,b);}
int cmovpo(REG16 a,AX b){return x86(148,a,b);}
int cmovpo(REG16 a,DX b){return x86(148,a,b);}
int cmovpo(REG16 a,CX b){return x86(148,a,b);}
int cmovpo(REG16 a,REG16 b){return x86(148,a,b);}
int cmovpo(REG16 a,MEM16 b){return x86(148,a,b);}
int cmovpo(REG16 a,R_M16 b){return x86(148,a,b);}
int cmovl(AX a,AX b){return x86(149,a,b);}
int cmovl(AX a,DX b){return x86(149,a,b);}
int cmovl(AX a,CX b){return x86(149,a,b);}
int cmovl(AX a,REG16 b){return x86(149,a,b);}
int cmovl(AX a,MEM16 b){return x86(149,a,b);}
int cmovl(AX a,R_M16 b){return x86(149,a,b);}
int cmovl(DX a,AX b){return x86(149,a,b);}
int cmovl(DX a,DX b){return x86(149,a,b);}
int cmovl(DX a,CX b){return x86(149,a,b);}
int cmovl(DX a,REG16 b){return x86(149,a,b);}
int cmovl(DX a,MEM16 b){return x86(149,a,b);}
int cmovl(DX a,R_M16 b){return x86(149,a,b);}
int cmovl(CX a,AX b){return x86(149,a,b);}
int cmovl(CX a,DX b){return x86(149,a,b);}
int cmovl(CX a,CX b){return x86(149,a,b);}
int cmovl(CX a,REG16 b){return x86(149,a,b);}
int cmovl(CX a,MEM16 b){return x86(149,a,b);}
int cmovl(CX a,R_M16 b){return x86(149,a,b);}
int cmovl(REG16 a,AX b){return x86(149,a,b);}
int cmovl(REG16 a,DX b){return x86(149,a,b);}
int cmovl(REG16 a,CX b){return x86(149,a,b);}
int cmovl(REG16 a,REG16 b){return x86(149,a,b);}
int cmovl(REG16 a,MEM16 b){return x86(149,a,b);}
int cmovl(REG16 a,R_M16 b){return x86(149,a,b);}
int cmovnge(AX a,AX b){return x86(150,a,b);}
int cmovnge(AX a,DX b){return x86(150,a,b);}
int cmovnge(AX a,CX b){return x86(150,a,b);}
int cmovnge(AX a,REG16 b){return x86(150,a,b);}
int cmovnge(AX a,MEM16 b){return x86(150,a,b);}
int cmovnge(AX a,R_M16 b){return x86(150,a,b);}
int cmovnge(DX a,AX b){return x86(150,a,b);}
int cmovnge(DX a,DX b){return x86(150,a,b);}
int cmovnge(DX a,CX b){return x86(150,a,b);}
int cmovnge(DX a,REG16 b){return x86(150,a,b);}
int cmovnge(DX a,MEM16 b){return x86(150,a,b);}
int cmovnge(DX a,R_M16 b){return x86(150,a,b);}
int cmovnge(CX a,AX b){return x86(150,a,b);}
int cmovnge(CX a,DX b){return x86(150,a,b);}
int cmovnge(CX a,CX b){return x86(150,a,b);}
int cmovnge(CX a,REG16 b){return x86(150,a,b);}
int cmovnge(CX a,MEM16 b){return x86(150,a,b);}
int cmovnge(CX a,R_M16 b){return x86(150,a,b);}
int cmovnge(REG16 a,AX b){return x86(150,a,b);}
int cmovnge(REG16 a,DX b){return x86(150,a,b);}
int cmovnge(REG16 a,CX b){return x86(150,a,b);}
int cmovnge(REG16 a,REG16 b){return x86(150,a,b);}
int cmovnge(REG16 a,MEM16 b){return x86(150,a,b);}
int cmovnge(REG16 a,R_M16 b){return x86(150,a,b);}
int cmovge(AX a,AX b){return x86(151,a,b);}
int cmovge(AX a,DX b){return x86(151,a,b);}
int cmovge(AX a,CX b){return x86(151,a,b);}
int cmovge(AX a,REG16 b){return x86(151,a,b);}
int cmovge(AX a,MEM16 b){return x86(151,a,b);}
int cmovge(AX a,R_M16 b){return x86(151,a,b);}
int cmovge(DX a,AX b){return x86(151,a,b);}
int cmovge(DX a,DX b){return x86(151,a,b);}
int cmovge(DX a,CX b){return x86(151,a,b);}
int cmovge(DX a,REG16 b){return x86(151,a,b);}
int cmovge(DX a,MEM16 b){return x86(151,a,b);}
int cmovge(DX a,R_M16 b){return x86(151,a,b);}
int cmovge(CX a,AX b){return x86(151,a,b);}
int cmovge(CX a,DX b){return x86(151,a,b);}
int cmovge(CX a,CX b){return x86(151,a,b);}
int cmovge(CX a,REG16 b){return x86(151,a,b);}
int cmovge(CX a,MEM16 b){return x86(151,a,b);}
int cmovge(CX a,R_M16 b){return x86(151,a,b);}
int cmovge(REG16 a,AX b){return x86(151,a,b);}
int cmovge(REG16 a,DX b){return x86(151,a,b);}
int cmovge(REG16 a,CX b){return x86(151,a,b);}
int cmovge(REG16 a,REG16 b){return x86(151,a,b);}
int cmovge(REG16 a,MEM16 b){return x86(151,a,b);}
int cmovge(REG16 a,R_M16 b){return x86(151,a,b);}
int cmovnl(AX a,AX b){return x86(152,a,b);}
int cmovnl(AX a,DX b){return x86(152,a,b);}
int cmovnl(AX a,CX b){return x86(152,a,b);}
int cmovnl(AX a,REG16 b){return x86(152,a,b);}
int cmovnl(AX a,MEM16 b){return x86(152,a,b);}
int cmovnl(AX a,R_M16 b){return x86(152,a,b);}
int cmovnl(DX a,AX b){return x86(152,a,b);}
int cmovnl(DX a,DX b){return x86(152,a,b);}
int cmovnl(DX a,CX b){return x86(152,a,b);}
int cmovnl(DX a,REG16 b){return x86(152,a,b);}
int cmovnl(DX a,MEM16 b){return x86(152,a,b);}
int cmovnl(DX a,R_M16 b){return x86(152,a,b);}
int cmovnl(CX a,AX b){return x86(152,a,b);}
int cmovnl(CX a,DX b){return x86(152,a,b);}
int cmovnl(CX a,CX b){return x86(152,a,b);}
int cmovnl(CX a,REG16 b){return x86(152,a,b);}
int cmovnl(CX a,MEM16 b){return x86(152,a,b);}
int cmovnl(CX a,R_M16 b){return x86(152,a,b);}
int cmovnl(REG16 a,AX b){return x86(152,a,b);}
int cmovnl(REG16 a,DX b){return x86(152,a,b);}
int cmovnl(REG16 a,CX b){return x86(152,a,b);}
int cmovnl(REG16 a,REG16 b){return x86(152,a,b);}
int cmovnl(REG16 a,MEM16 b){return x86(152,a,b);}
int cmovnl(REG16 a,R_M16 b){return x86(152,a,b);}
int cmovle(AX a,AX b){return x86(153,a,b);}
int cmovle(AX a,DX b){return x86(153,a,b);}
int cmovle(AX a,CX b){return x86(153,a,b);}
int cmovle(AX a,REG16 b){return x86(153,a,b);}
int cmovle(AX a,MEM16 b){return x86(153,a,b);}
int cmovle(AX a,R_M16 b){return x86(153,a,b);}
int cmovle(DX a,AX b){return x86(153,a,b);}
int cmovle(DX a,DX b){return x86(153,a,b);}
int cmovle(DX a,CX b){return x86(153,a,b);}
int cmovle(DX a,REG16 b){return x86(153,a,b);}
int cmovle(DX a,MEM16 b){return x86(153,a,b);}
int cmovle(DX a,R_M16 b){return x86(153,a,b);}
int cmovle(CX a,AX b){return x86(153,a,b);}
int cmovle(CX a,DX b){return x86(153,a,b);}
int cmovle(CX a,CX b){return x86(153,a,b);}
int cmovle(CX a,REG16 b){return x86(153,a,b);}
int cmovle(CX a,MEM16 b){return x86(153,a,b);}
int cmovle(CX a,R_M16 b){return x86(153,a,b);}
int cmovle(REG16 a,AX b){return x86(153,a,b);}
int cmovle(REG16 a,DX b){return x86(153,a,b);}
int cmovle(REG16 a,CX b){return x86(153,a,b);}
int cmovle(REG16 a,REG16 b){return x86(153,a,b);}
int cmovle(REG16 a,MEM16 b){return x86(153,a,b);}
int cmovle(REG16 a,R_M16 b){return x86(153,a,b);}
int cmovng(AX a,AX b){return x86(154,a,b);}
int cmovng(AX a,DX b){return x86(154,a,b);}
int cmovng(AX a,CX b){return x86(154,a,b);}
int cmovng(AX a,REG16 b){return x86(154,a,b);}
int cmovng(AX a,MEM16 b){return x86(154,a,b);}
int cmovng(AX a,R_M16 b){return x86(154,a,b);}
int cmovng(DX a,AX b){return x86(154,a,b);}
int cmovng(DX a,DX b){return x86(154,a,b);}
int cmovng(DX a,CX b){return x86(154,a,b);}
int cmovng(DX a,REG16 b){return x86(154,a,b);}
int cmovng(DX a,MEM16 b){return x86(154,a,b);}
int cmovng(DX a,R_M16 b){return x86(154,a,b);}
int cmovng(CX a,AX b){return x86(154,a,b);}
int cmovng(CX a,DX b){return x86(154,a,b);}
int cmovng(CX a,CX b){return x86(154,a,b);}
int cmovng(CX a,REG16 b){return x86(154,a,b);}
int cmovng(CX a,MEM16 b){return x86(154,a,b);}
int cmovng(CX a,R_M16 b){return x86(154,a,b);}
int cmovng(REG16 a,AX b){return x86(154,a,b);}
int cmovng(REG16 a,DX b){return x86(154,a,b);}
int cmovng(REG16 a,CX b){return x86(154,a,b);}
int cmovng(REG16 a,REG16 b){return x86(154,a,b);}
int cmovng(REG16 a,MEM16 b){return x86(154,a,b);}
int cmovng(REG16 a,R_M16 b){return x86(154,a,b);}
int cmovg(AX a,AX b){return x86(155,a,b);}
int cmovg(AX a,DX b){return x86(155,a,b);}
int cmovg(AX a,CX b){return x86(155,a,b);}
int cmovg(AX a,REG16 b){return x86(155,a,b);}
int cmovg(AX a,MEM16 b){return x86(155,a,b);}
int cmovg(AX a,R_M16 b){return x86(155,a,b);}
int cmovg(DX a,AX b){return x86(155,a,b);}
int cmovg(DX a,DX b){return x86(155,a,b);}
int cmovg(DX a,CX b){return x86(155,a,b);}
int cmovg(DX a,REG16 b){return x86(155,a,b);}
int cmovg(DX a,MEM16 b){return x86(155,a,b);}
int cmovg(DX a,R_M16 b){return x86(155,a,b);}
int cmovg(CX a,AX b){return x86(155,a,b);}
int cmovg(CX a,DX b){return x86(155,a,b);}
int cmovg(CX a,CX b){return x86(155,a,b);}
int cmovg(CX a,REG16 b){return x86(155,a,b);}
int cmovg(CX a,MEM16 b){return x86(155,a,b);}
int cmovg(CX a,R_M16 b){return x86(155,a,b);}
int cmovg(REG16 a,AX b){return x86(155,a,b);}
int cmovg(REG16 a,DX b){return x86(155,a,b);}
int cmovg(REG16 a,CX b){return x86(155,a,b);}
int cmovg(REG16 a,REG16 b){return x86(155,a,b);}
int cmovg(REG16 a,MEM16 b){return x86(155,a,b);}
int cmovg(REG16 a,R_M16 b){return x86(155,a,b);}
int cmovnle(AX a,AX b){return x86(156,a,b);}
int cmovnle(AX a,DX b){return x86(156,a,b);}
int cmovnle(AX a,CX b){return x86(156,a,b);}
int cmovnle(AX a,REG16 b){return x86(156,a,b);}
int cmovnle(AX a,MEM16 b){return x86(156,a,b);}
int cmovnle(AX a,R_M16 b){return x86(156,a,b);}
int cmovnle(DX a,AX b){return x86(156,a,b);}
int cmovnle(DX a,DX b){return x86(156,a,b);}
int cmovnle(DX a,CX b){return x86(156,a,b);}
int cmovnle(DX a,REG16 b){return x86(156,a,b);}
int cmovnle(DX a,MEM16 b){return x86(156,a,b);}
int cmovnle(DX a,R_M16 b){return x86(156,a,b);}
int cmovnle(CX a,AX b){return x86(156,a,b);}
int cmovnle(CX a,DX b){return x86(156,a,b);}
int cmovnle(CX a,CX b){return x86(156,a,b);}
int cmovnle(CX a,REG16 b){return x86(156,a,b);}
int cmovnle(CX a,MEM16 b){return x86(156,a,b);}
int cmovnle(CX a,R_M16 b){return x86(156,a,b);}
int cmovnle(REG16 a,AX b){return x86(156,a,b);}
int cmovnle(REG16 a,DX b){return x86(156,a,b);}
int cmovnle(REG16 a,CX b){return x86(156,a,b);}
int cmovnle(REG16 a,REG16 b){return x86(156,a,b);}
int cmovnle(REG16 a,MEM16 b){return x86(156,a,b);}
int cmovnle(REG16 a,R_M16 b){return x86(156,a,b);}
int cmovo(EAX a,EAX b){return x86(157,a,b);}
int cmovo(EAX a,ECX b){return x86(157,a,b);}
int cmovo(EAX a,REG32 b){return x86(157,a,b);}
int cmovo(EAX a,MEM32 b){return x86(157,a,b);}
int cmovo(EAX a,R_M32 b){return x86(157,a,b);}
int cmovo(ECX a,EAX b){return x86(157,a,b);}
int cmovo(ECX a,ECX b){return x86(157,a,b);}
int cmovo(ECX a,REG32 b){return x86(157,a,b);}
int cmovo(ECX a,MEM32 b){return x86(157,a,b);}
int cmovo(ECX a,R_M32 b){return x86(157,a,b);}
int cmovo(REG32 a,EAX b){return x86(157,a,b);}
int cmovo(REG32 a,ECX b){return x86(157,a,b);}
int cmovo(REG32 a,REG32 b){return x86(157,a,b);}
int cmovo(REG32 a,MEM32 b){return x86(157,a,b);}
int cmovo(REG32 a,R_M32 b){return x86(157,a,b);}
int cmovno(EAX a,EAX b){return x86(158,a,b);}
int cmovno(EAX a,ECX b){return x86(158,a,b);}
int cmovno(EAX a,REG32 b){return x86(158,a,b);}
int cmovno(EAX a,MEM32 b){return x86(158,a,b);}
int cmovno(EAX a,R_M32 b){return x86(158,a,b);}
int cmovno(ECX a,EAX b){return x86(158,a,b);}
int cmovno(ECX a,ECX b){return x86(158,a,b);}
int cmovno(ECX a,REG32 b){return x86(158,a,b);}
int cmovno(ECX a,MEM32 b){return x86(158,a,b);}
int cmovno(ECX a,R_M32 b){return x86(158,a,b);}
int cmovno(REG32 a,EAX b){return x86(158,a,b);}
int cmovno(REG32 a,ECX b){return x86(158,a,b);}
int cmovno(REG32 a,REG32 b){return x86(158,a,b);}
int cmovno(REG32 a,MEM32 b){return x86(158,a,b);}
int cmovno(REG32 a,R_M32 b){return x86(158,a,b);}
int cmovb(EAX a,EAX b){return x86(159,a,b);}
int cmovb(EAX a,ECX b){return x86(159,a,b);}
int cmovb(EAX a,REG32 b){return x86(159,a,b);}
int cmovb(EAX a,MEM32 b){return x86(159,a,b);}
int cmovb(EAX a,R_M32 b){return x86(159,a,b);}
int cmovb(ECX a,EAX b){return x86(159,a,b);}
int cmovb(ECX a,ECX b){return x86(159,a,b);}
int cmovb(ECX a,REG32 b){return x86(159,a,b);}
int cmovb(ECX a,MEM32 b){return x86(159,a,b);}
int cmovb(ECX a,R_M32 b){return x86(159,a,b);}
int cmovb(REG32 a,EAX b){return x86(159,a,b);}
int cmovb(REG32 a,ECX b){return x86(159,a,b);}
int cmovb(REG32 a,REG32 b){return x86(159,a,b);}
int cmovb(REG32 a,MEM32 b){return x86(159,a,b);}
int cmovb(REG32 a,R_M32 b){return x86(159,a,b);}
int cmovc(EAX a,EAX b){return x86(160,a,b);}
int cmovc(EAX a,ECX b){return x86(160,a,b);}
int cmovc(EAX a,REG32 b){return x86(160,a,b);}
int cmovc(EAX a,MEM32 b){return x86(160,a,b);}
int cmovc(EAX a,R_M32 b){return x86(160,a,b);}
int cmovc(ECX a,EAX b){return x86(160,a,b);}
int cmovc(ECX a,ECX b){return x86(160,a,b);}
int cmovc(ECX a,REG32 b){return x86(160,a,b);}
int cmovc(ECX a,MEM32 b){return x86(160,a,b);}
int cmovc(ECX a,R_M32 b){return x86(160,a,b);}
int cmovc(REG32 a,EAX b){return x86(160,a,b);}
int cmovc(REG32 a,ECX b){return x86(160,a,b);}
int cmovc(REG32 a,REG32 b){return x86(160,a,b);}
int cmovc(REG32 a,MEM32 b){return x86(160,a,b);}
int cmovc(REG32 a,R_M32 b){return x86(160,a,b);}
int cmovnea(EAX a,EAX b){return x86(161,a,b);}
int cmovnea(EAX a,ECX b){return x86(161,a,b);}
int cmovnea(EAX a,REG32 b){return x86(161,a,b);}
int cmovnea(EAX a,MEM32 b){return x86(161,a,b);}
int cmovnea(EAX a,R_M32 b){return x86(161,a,b);}
int cmovnea(ECX a,EAX b){return x86(161,a,b);}
int cmovnea(ECX a,ECX b){return x86(161,a,b);}
int cmovnea(ECX a,REG32 b){return x86(161,a,b);}
int cmovnea(ECX a,MEM32 b){return x86(161,a,b);}
int cmovnea(ECX a,R_M32 b){return x86(161,a,b);}
int cmovnea(REG32 a,EAX b){return x86(161,a,b);}
int cmovnea(REG32 a,ECX b){return x86(161,a,b);}
int cmovnea(REG32 a,REG32 b){return x86(161,a,b);}
int cmovnea(REG32 a,MEM32 b){return x86(161,a,b);}
int cmovnea(REG32 a,R_M32 b){return x86(161,a,b);}
int cmovae(EAX a,EAX b){return x86(162,a,b);}
int cmovae(EAX a,ECX b){return x86(162,a,b);}
int cmovae(EAX a,REG32 b){return x86(162,a,b);}
int cmovae(EAX a,MEM32 b){return x86(162,a,b);}
int cmovae(EAX a,R_M32 b){return x86(162,a,b);}
int cmovae(ECX a,EAX b){return x86(162,a,b);}
int cmovae(ECX a,ECX b){return x86(162,a,b);}
int cmovae(ECX a,REG32 b){return x86(162,a,b);}
int cmovae(ECX a,MEM32 b){return x86(162,a,b);}
int cmovae(ECX a,R_M32 b){return x86(162,a,b);}
int cmovae(REG32 a,EAX b){return x86(162,a,b);}
int cmovae(REG32 a,ECX b){return x86(162,a,b);}
int cmovae(REG32 a,REG32 b){return x86(162,a,b);}
int cmovae(REG32 a,MEM32 b){return x86(162,a,b);}
int cmovae(REG32 a,R_M32 b){return x86(162,a,b);}
int cmovnb(EAX a,EAX b){return x86(163,a,b);}
int cmovnb(EAX a,ECX b){return x86(163,a,b);}
int cmovnb(EAX a,REG32 b){return x86(163,a,b);}
int cmovnb(EAX a,MEM32 b){return x86(163,a,b);}
int cmovnb(EAX a,R_M32 b){return x86(163,a,b);}
int cmovnb(ECX a,EAX b){return x86(163,a,b);}
int cmovnb(ECX a,ECX b){return x86(163,a,b);}
int cmovnb(ECX a,REG32 b){return x86(163,a,b);}
int cmovnb(ECX a,MEM32 b){return x86(163,a,b);}
int cmovnb(ECX a,R_M32 b){return x86(163,a,b);}
int cmovnb(REG32 a,EAX b){return x86(163,a,b);}
int cmovnb(REG32 a,ECX b){return x86(163,a,b);}
int cmovnb(REG32 a,REG32 b){return x86(163,a,b);}
int cmovnb(REG32 a,MEM32 b){return x86(163,a,b);}
int cmovnb(REG32 a,R_M32 b){return x86(163,a,b);}
int cmovnc(EAX a,EAX b){return x86(164,a,b);}
int cmovnc(EAX a,ECX b){return x86(164,a,b);}
int cmovnc(EAX a,REG32 b){return x86(164,a,b);}
int cmovnc(EAX a,MEM32 b){return x86(164,a,b);}
int cmovnc(EAX a,R_M32 b){return x86(164,a,b);}
int cmovnc(ECX a,EAX b){return x86(164,a,b);}
int cmovnc(ECX a,ECX b){return x86(164,a,b);}
int cmovnc(ECX a,REG32 b){return x86(164,a,b);}
int cmovnc(ECX a,MEM32 b){return x86(164,a,b);}
int cmovnc(ECX a,R_M32 b){return x86(164,a,b);}
int cmovnc(REG32 a,EAX b){return x86(164,a,b);}
int cmovnc(REG32 a,ECX b){return x86(164,a,b);}
int cmovnc(REG32 a,REG32 b){return x86(164,a,b);}
int cmovnc(REG32 a,MEM32 b){return x86(164,a,b);}
int cmovnc(REG32 a,R_M32 b){return x86(164,a,b);}
int cmove(EAX a,EAX b){return x86(165,a,b);}
int cmove(EAX a,ECX b){return x86(165,a,b);}
int cmove(EAX a,REG32 b){return x86(165,a,b);}
int cmove(EAX a,MEM32 b){return x86(165,a,b);}
int cmove(EAX a,R_M32 b){return x86(165,a,b);}
int cmove(ECX a,EAX b){return x86(165,a,b);}
int cmove(ECX a,ECX b){return x86(165,a,b);}
int cmove(ECX a,REG32 b){return x86(165,a,b);}
int cmove(ECX a,MEM32 b){return x86(165,a,b);}
int cmove(ECX a,R_M32 b){return x86(165,a,b);}
int cmove(REG32 a,EAX b){return x86(165,a,b);}
int cmove(REG32 a,ECX b){return x86(165,a,b);}
int cmove(REG32 a,REG32 b){return x86(165,a,b);}
int cmove(REG32 a,MEM32 b){return x86(165,a,b);}
int cmove(REG32 a,R_M32 b){return x86(165,a,b);}
int cmovz(EAX a,EAX b){return x86(166,a,b);}
int cmovz(EAX a,ECX b){return x86(166,a,b);}
int cmovz(EAX a,REG32 b){return x86(166,a,b);}
int cmovz(EAX a,MEM32 b){return x86(166,a,b);}
int cmovz(EAX a,R_M32 b){return x86(166,a,b);}
int cmovz(ECX a,EAX b){return x86(166,a,b);}
int cmovz(ECX a,ECX b){return x86(166,a,b);}
int cmovz(ECX a,REG32 b){return x86(166,a,b);}
int cmovz(ECX a,MEM32 b){return x86(166,a,b);}
int cmovz(ECX a,R_M32 b){return x86(166,a,b);}
int cmovz(REG32 a,EAX b){return x86(166,a,b);}
int cmovz(REG32 a,ECX b){return x86(166,a,b);}
int cmovz(REG32 a,REG32 b){return x86(166,a,b);}
int cmovz(REG32 a,MEM32 b){return x86(166,a,b);}
int cmovz(REG32 a,R_M32 b){return x86(166,a,b);}
int cmovne(EAX a,EAX b){return x86(167,a,b);}
int cmovne(EAX a,ECX b){return x86(167,a,b);}
int cmovne(EAX a,REG32 b){return x86(167,a,b);}
int cmovne(EAX a,MEM32 b){return x86(167,a,b);}
int cmovne(EAX a,R_M32 b){return x86(167,a,b);}
int cmovne(ECX a,EAX b){return x86(167,a,b);}
int cmovne(ECX a,ECX b){return x86(167,a,b);}
int cmovne(ECX a,REG32 b){return x86(167,a,b);}
int cmovne(ECX a,MEM32 b){return x86(167,a,b);}
int cmovne(ECX a,R_M32 b){return x86(167,a,b);}
int cmovne(REG32 a,EAX b){return x86(167,a,b);}
int cmovne(REG32 a,ECX b){return x86(167,a,b);}
int cmovne(REG32 a,REG32 b){return x86(167,a,b);}
int cmovne(REG32 a,MEM32 b){return x86(167,a,b);}
int cmovne(REG32 a,R_M32 b){return x86(167,a,b);}
int cmovnz(EAX a,EAX b){return x86(168,a,b);}
int cmovnz(EAX a,ECX b){return x86(168,a,b);}
int cmovnz(EAX a,REG32 b){return x86(168,a,b);}
int cmovnz(EAX a,MEM32 b){return x86(168,a,b);}
int cmovnz(EAX a,R_M32 b){return x86(168,a,b);}
int cmovnz(ECX a,EAX b){return x86(168,a,b);}
int cmovnz(ECX a,ECX b){return x86(168,a,b);}
int cmovnz(ECX a,REG32 b){return x86(168,a,b);}
int cmovnz(ECX a,MEM32 b){return x86(168,a,b);}
int cmovnz(ECX a,R_M32 b){return x86(168,a,b);}
int cmovnz(REG32 a,EAX b){return x86(168,a,b);}
int cmovnz(REG32 a,ECX b){return x86(168,a,b);}
int cmovnz(REG32 a,REG32 b){return x86(168,a,b);}
int cmovnz(REG32 a,MEM32 b){return x86(168,a,b);}
int cmovnz(REG32 a,R_M32 b){return x86(168,a,b);}
int cmovbe(EAX a,EAX b){return x86(169,a,b);}
int cmovbe(EAX a,ECX b){return x86(169,a,b);}
int cmovbe(EAX a,REG32 b){return x86(169,a,b);}
int cmovbe(EAX a,MEM32 b){return x86(169,a,b);}
int cmovbe(EAX a,R_M32 b){return x86(169,a,b);}
int cmovbe(ECX a,EAX b){return x86(169,a,b);}
int cmovbe(ECX a,ECX b){return x86(169,a,b);}
int cmovbe(ECX a,REG32 b){return x86(169,a,b);}
int cmovbe(ECX a,MEM32 b){return x86(169,a,b);}
int cmovbe(ECX a,R_M32 b){return x86(169,a,b);}
int cmovbe(REG32 a,EAX b){return x86(169,a,b);}
int cmovbe(REG32 a,ECX b){return x86(169,a,b);}
int cmovbe(REG32 a,REG32 b){return x86(169,a,b);}
int cmovbe(REG32 a,MEM32 b){return x86(169,a,b);}
int cmovbe(REG32 a,R_M32 b){return x86(169,a,b);}
int cmovna(EAX a,EAX b){return x86(170,a,b);}
int cmovna(EAX a,ECX b){return x86(170,a,b);}
int cmovna(EAX a,REG32 b){return x86(170,a,b);}
int cmovna(EAX a,MEM32 b){return x86(170,a,b);}
int cmovna(EAX a,R_M32 b){return x86(170,a,b);}
int cmovna(ECX a,EAX b){return x86(170,a,b);}
int cmovna(ECX a,ECX b){return x86(170,a,b);}
int cmovna(ECX a,REG32 b){return x86(170,a,b);}
int cmovna(ECX a,MEM32 b){return x86(170,a,b);}
int cmovna(ECX a,R_M32 b){return x86(170,a,b);}
int cmovna(REG32 a,EAX b){return x86(170,a,b);}
int cmovna(REG32 a,ECX b){return x86(170,a,b);}
int cmovna(REG32 a,REG32 b){return x86(170,a,b);}
int cmovna(REG32 a,MEM32 b){return x86(170,a,b);}
int cmovna(REG32 a,R_M32 b){return x86(170,a,b);}
int cmova(EAX a,EAX b){return x86(171,a,b);}
int cmova(EAX a,ECX b){return x86(171,a,b);}
int cmova(EAX a,REG32 b){return x86(171,a,b);}
int cmova(EAX a,MEM32 b){return x86(171,a,b);}
int cmova(EAX a,R_M32 b){return x86(171,a,b);}
int cmova(ECX a,EAX b){return x86(171,a,b);}
int cmova(ECX a,ECX b){return x86(171,a,b);}
int cmova(ECX a,REG32 b){return x86(171,a,b);}
int cmova(ECX a,MEM32 b){return x86(171,a,b);}
int cmova(ECX a,R_M32 b){return x86(171,a,b);}
int cmova(REG32 a,EAX b){return x86(171,a,b);}
int cmova(REG32 a,ECX b){return x86(171,a,b);}
int cmova(REG32 a,REG32 b){return x86(171,a,b);}
int cmova(REG32 a,MEM32 b){return x86(171,a,b);}
int cmova(REG32 a,R_M32 b){return x86(171,a,b);}
int cmovnbe(EAX a,EAX b){return x86(172,a,b);}
int cmovnbe(EAX a,ECX b){return x86(172,a,b);}
int cmovnbe(EAX a,REG32 b){return x86(172,a,b);}
int cmovnbe(EAX a,MEM32 b){return x86(172,a,b);}
int cmovnbe(EAX a,R_M32 b){return x86(172,a,b);}
int cmovnbe(ECX a,EAX b){return x86(172,a,b);}
int cmovnbe(ECX a,ECX b){return x86(172,a,b);}
int cmovnbe(ECX a,REG32 b){return x86(172,a,b);}
int cmovnbe(ECX a,MEM32 b){return x86(172,a,b);}
int cmovnbe(ECX a,R_M32 b){return x86(172,a,b);}
int cmovnbe(REG32 a,EAX b){return x86(172,a,b);}
int cmovnbe(REG32 a,ECX b){return x86(172,a,b);}
int cmovnbe(REG32 a,REG32 b){return x86(172,a,b);}
int cmovnbe(REG32 a,MEM32 b){return x86(172,a,b);}
int cmovnbe(REG32 a,R_M32 b){return x86(172,a,b);}
int cmovs(EAX a,EAX b){return x86(173,a,b);}
int cmovs(EAX a,ECX b){return x86(173,a,b);}
int cmovs(EAX a,REG32 b){return x86(173,a,b);}
int cmovs(EAX a,MEM32 b){return x86(173,a,b);}
int cmovs(EAX a,R_M32 b){return x86(173,a,b);}
int cmovs(ECX a,EAX b){return x86(173,a,b);}
int cmovs(ECX a,ECX b){return x86(173,a,b);}
int cmovs(ECX a,REG32 b){return x86(173,a,b);}
int cmovs(ECX a,MEM32 b){return x86(173,a,b);}
int cmovs(ECX a,R_M32 b){return x86(173,a,b);}
int cmovs(REG32 a,EAX b){return x86(173,a,b);}
int cmovs(REG32 a,ECX b){return x86(173,a,b);}
int cmovs(REG32 a,REG32 b){return x86(173,a,b);}
int cmovs(REG32 a,MEM32 b){return x86(173,a,b);}
int cmovs(REG32 a,R_M32 b){return x86(173,a,b);}
int cmovns(EAX a,EAX b){return x86(174,a,b);}
int cmovns(EAX a,ECX b){return x86(174,a,b);}
int cmovns(EAX a,REG32 b){return x86(174,a,b);}
int cmovns(EAX a,MEM32 b){return x86(174,a,b);}
int cmovns(EAX a,R_M32 b){return x86(174,a,b);}
int cmovns(ECX a,EAX b){return x86(174,a,b);}
int cmovns(ECX a,ECX b){return x86(174,a,b);}
int cmovns(ECX a,REG32 b){return x86(174,a,b);}
int cmovns(ECX a,MEM32 b){return x86(174,a,b);}
int cmovns(ECX a,R_M32 b){return x86(174,a,b);}
int cmovns(REG32 a,EAX b){return x86(174,a,b);}
int cmovns(REG32 a,ECX b){return x86(174,a,b);}
int cmovns(REG32 a,REG32 b){return x86(174,a,b);}
int cmovns(REG32 a,MEM32 b){return x86(174,a,b);}
int cmovns(REG32 a,R_M32 b){return x86(174,a,b);}
int cmovp(EAX a,EAX b){return x86(175,a,b);}
int cmovp(EAX a,ECX b){return x86(175,a,b);}
int cmovp(EAX a,REG32 b){return x86(175,a,b);}
int cmovp(EAX a,MEM32 b){return x86(175,a,b);}
int cmovp(EAX a,R_M32 b){return x86(175,a,b);}
int cmovp(ECX a,EAX b){return x86(175,a,b);}
int cmovp(ECX a,ECX b){return x86(175,a,b);}
int cmovp(ECX a,REG32 b){return x86(175,a,b);}
int cmovp(ECX a,MEM32 b){return x86(175,a,b);}
int cmovp(ECX a,R_M32 b){return x86(175,a,b);}
int cmovp(REG32 a,EAX b){return x86(175,a,b);}
int cmovp(REG32 a,ECX b){return x86(175,a,b);}
int cmovp(REG32 a,REG32 b){return x86(175,a,b);}
int cmovp(REG32 a,MEM32 b){return x86(175,a,b);}
int cmovp(REG32 a,R_M32 b){return x86(175,a,b);}
int cmovpe(EAX a,EAX b){return x86(176,a,b);}
int cmovpe(EAX a,ECX b){return x86(176,a,b);}
int cmovpe(EAX a,REG32 b){return x86(176,a,b);}
int cmovpe(EAX a,MEM32 b){return x86(176,a,b);}
int cmovpe(EAX a,R_M32 b){return x86(176,a,b);}
int cmovpe(ECX a,EAX b){return x86(176,a,b);}
int cmovpe(ECX a,ECX b){return x86(176,a,b);}
int cmovpe(ECX a,REG32 b){return x86(176,a,b);}
int cmovpe(ECX a,MEM32 b){return x86(176,a,b);}
int cmovpe(ECX a,R_M32 b){return x86(176,a,b);}
int cmovpe(REG32 a,EAX b){return x86(176,a,b);}
int cmovpe(REG32 a,ECX b){return x86(176,a,b);}
int cmovpe(REG32 a,REG32 b){return x86(176,a,b);}
int cmovpe(REG32 a,MEM32 b){return x86(176,a,b);}
int cmovpe(REG32 a,R_M32 b){return x86(176,a,b);}
int cmovnp(EAX a,EAX b){return x86(177,a,b);}
int cmovnp(EAX a,ECX b){return x86(177,a,b);}
int cmovnp(EAX a,REG32 b){return x86(177,a,b);}
int cmovnp(EAX a,MEM32 b){return x86(177,a,b);}
int cmovnp(EAX a,R_M32 b){return x86(177,a,b);}
int cmovnp(ECX a,EAX b){return x86(177,a,b);}
int cmovnp(ECX a,ECX b){return x86(177,a,b);}
int cmovnp(ECX a,REG32 b){return x86(177,a,b);}
int cmovnp(ECX a,MEM32 b){return x86(177,a,b);}
int cmovnp(ECX a,R_M32 b){return x86(177,a,b);}
int cmovnp(REG32 a,EAX b){return x86(177,a,b);}
int cmovnp(REG32 a,ECX b){return x86(177,a,b);}
int cmovnp(REG32 a,REG32 b){return x86(177,a,b);}
int cmovnp(REG32 a,MEM32 b){return x86(177,a,b);}
int cmovnp(REG32 a,R_M32 b){return x86(177,a,b);}
int cmovpo(EAX a,EAX b){return x86(178,a,b);}
int cmovpo(EAX a,ECX b){return x86(178,a,b);}
int cmovpo(EAX a,REG32 b){return x86(178,a,b);}
int cmovpo(EAX a,MEM32 b){return x86(178,a,b);}
int cmovpo(EAX a,R_M32 b){return x86(178,a,b);}
int cmovpo(ECX a,EAX b){return x86(178,a,b);}
int cmovpo(ECX a,ECX b){return x86(178,a,b);}
int cmovpo(ECX a,REG32 b){return x86(178,a,b);}
int cmovpo(ECX a,MEM32 b){return x86(178,a,b);}
int cmovpo(ECX a,R_M32 b){return x86(178,a,b);}
int cmovpo(REG32 a,EAX b){return x86(178,a,b);}
int cmovpo(REG32 a,ECX b){return x86(178,a,b);}
int cmovpo(REG32 a,REG32 b){return x86(178,a,b);}
int cmovpo(REG32 a,MEM32 b){return x86(178,a,b);}
int cmovpo(REG32 a,R_M32 b){return x86(178,a,b);}
int cmovl(EAX a,EAX b){return x86(179,a,b);}
int cmovl(EAX a,ECX b){return x86(179,a,b);}
int cmovl(EAX a,REG32 b){return x86(179,a,b);}
int cmovl(EAX a,MEM32 b){return x86(179,a,b);}
int cmovl(EAX a,R_M32 b){return x86(179,a,b);}
int cmovl(ECX a,EAX b){return x86(179,a,b);}
int cmovl(ECX a,ECX b){return x86(179,a,b);}
int cmovl(ECX a,REG32 b){return x86(179,a,b);}
int cmovl(ECX a,MEM32 b){return x86(179,a,b);}
int cmovl(ECX a,R_M32 b){return x86(179,a,b);}
int cmovl(REG32 a,EAX b){return x86(179,a,b);}
int cmovl(REG32 a,ECX b){return x86(179,a,b);}
int cmovl(REG32 a,REG32 b){return x86(179,a,b);}
int cmovl(REG32 a,MEM32 b){return x86(179,a,b);}
int cmovl(REG32 a,R_M32 b){return x86(179,a,b);}
int cmovnge(EAX a,EAX b){return x86(180,a,b);}
int cmovnge(EAX a,ECX b){return x86(180,a,b);}
int cmovnge(EAX a,REG32 b){return x86(180,a,b);}
int cmovnge(EAX a,MEM32 b){return x86(180,a,b);}
int cmovnge(EAX a,R_M32 b){return x86(180,a,b);}
int cmovnge(ECX a,EAX b){return x86(180,a,b);}
int cmovnge(ECX a,ECX b){return x86(180,a,b);}
int cmovnge(ECX a,REG32 b){return x86(180,a,b);}
int cmovnge(ECX a,MEM32 b){return x86(180,a,b);}
int cmovnge(ECX a,R_M32 b){return x86(180,a,b);}
int cmovnge(REG32 a,EAX b){return x86(180,a,b);}
int cmovnge(REG32 a,ECX b){return x86(180,a,b);}
int cmovnge(REG32 a,REG32 b){return x86(180,a,b);}
int cmovnge(REG32 a,MEM32 b){return x86(180,a,b);}
int cmovnge(REG32 a,R_M32 b){return x86(180,a,b);}
int cmovge(EAX a,EAX b){return x86(181,a,b);}
int cmovge(EAX a,ECX b){return x86(181,a,b);}
int cmovge(EAX a,REG32 b){return x86(181,a,b);}
int cmovge(EAX a,MEM32 b){return x86(181,a,b);}
int cmovge(EAX a,R_M32 b){return x86(181,a,b);}
int cmovge(ECX a,EAX b){return x86(181,a,b);}
int cmovge(ECX a,ECX b){return x86(181,a,b);}
int cmovge(ECX a,REG32 b){return x86(181,a,b);}
int cmovge(ECX a,MEM32 b){return x86(181,a,b);}
int cmovge(ECX a,R_M32 b){return x86(181,a,b);}
int cmovge(REG32 a,EAX b){return x86(181,a,b);}
int cmovge(REG32 a,ECX b){return x86(181,a,b);}
int cmovge(REG32 a,REG32 b){return x86(181,a,b);}
int cmovge(REG32 a,MEM32 b){return x86(181,a,b);}
int cmovge(REG32 a,R_M32 b){return x86(181,a,b);}
int cmovnl(EAX a,EAX b){return x86(182,a,b);}
int cmovnl(EAX a,ECX b){return x86(182,a,b);}
int cmovnl(EAX a,REG32 b){return x86(182,a,b);}
int cmovnl(EAX a,MEM32 b){return x86(182,a,b);}
int cmovnl(EAX a,R_M32 b){return x86(182,a,b);}
int cmovnl(ECX a,EAX b){return x86(182,a,b);}
int cmovnl(ECX a,ECX b){return x86(182,a,b);}
int cmovnl(ECX a,REG32 b){return x86(182,a,b);}
int cmovnl(ECX a,MEM32 b){return x86(182,a,b);}
int cmovnl(ECX a,R_M32 b){return x86(182,a,b);}
int cmovnl(REG32 a,EAX b){return x86(182,a,b);}
int cmovnl(REG32 a,ECX b){return x86(182,a,b);}
int cmovnl(REG32 a,REG32 b){return x86(182,a,b);}
int cmovnl(REG32 a,MEM32 b){return x86(182,a,b);}
int cmovnl(REG32 a,R_M32 b){return x86(182,a,b);}
int cmovle(EAX a,EAX b){return x86(183,a,b);}
int cmovle(EAX a,ECX b){return x86(183,a,b);}
int cmovle(EAX a,REG32 b){return x86(183,a,b);}
int cmovle(EAX a,MEM32 b){return x86(183,a,b);}
int cmovle(EAX a,R_M32 b){return x86(183,a,b);}
int cmovle(ECX a,EAX b){return x86(183,a,b);}
int cmovle(ECX a,ECX b){return x86(183,a,b);}
int cmovle(ECX a,REG32 b){return x86(183,a,b);}
int cmovle(ECX a,MEM32 b){return x86(183,a,b);}
int cmovle(ECX a,R_M32 b){return x86(183,a,b);}
int cmovle(REG32 a,EAX b){return x86(183,a,b);}
int cmovle(REG32 a,ECX b){return x86(183,a,b);}
int cmovle(REG32 a,REG32 b){return x86(183,a,b);}
int cmovle(REG32 a,MEM32 b){return x86(183,a,b);}
int cmovle(REG32 a,R_M32 b){return x86(183,a,b);}
int cmovng(EAX a,EAX b){return x86(184,a,b);}
int cmovng(EAX a,ECX b){return x86(184,a,b);}
int cmovng(EAX a,REG32 b){return x86(184,a,b);}
int cmovng(EAX a,MEM32 b){return x86(184,a,b);}
int cmovng(EAX a,R_M32 b){return x86(184,a,b);}
int cmovng(ECX a,EAX b){return x86(184,a,b);}
int cmovng(ECX a,ECX b){return x86(184,a,b);}
int cmovng(ECX a,REG32 b){return x86(184,a,b);}
int cmovng(ECX a,MEM32 b){return x86(184,a,b);}
int cmovng(ECX a,R_M32 b){return x86(184,a,b);}
int cmovng(REG32 a,EAX b){return x86(184,a,b);}
int cmovng(REG32 a,ECX b){return x86(184,a,b);}
int cmovng(REG32 a,REG32 b){return x86(184,a,b);}
int cmovng(REG32 a,MEM32 b){return x86(184,a,b);}
int cmovng(REG32 a,R_M32 b){return x86(184,a,b);}
int cmovg(EAX a,EAX b){return x86(185,a,b);}
int cmovg(EAX a,ECX b){return x86(185,a,b);}
int cmovg(EAX a,REG32 b){return x86(185,a,b);}
int cmovg(EAX a,MEM32 b){return x86(185,a,b);}
int cmovg(EAX a,R_M32 b){return x86(185,a,b);}
int cmovg(ECX a,EAX b){return x86(185,a,b);}
int cmovg(ECX a,ECX b){return x86(185,a,b);}
int cmovg(ECX a,REG32 b){return x86(185,a,b);}
int cmovg(ECX a,MEM32 b){return x86(185,a,b);}
int cmovg(ECX a,R_M32 b){return x86(185,a,b);}
int cmovg(REG32 a,EAX b){return x86(185,a,b);}
int cmovg(REG32 a,ECX b){return x86(185,a,b);}
int cmovg(REG32 a,REG32 b){return x86(185,a,b);}
int cmovg(REG32 a,MEM32 b){return x86(185,a,b);}
int cmovg(REG32 a,R_M32 b){return x86(185,a,b);}
int cmovnle(EAX a,EAX b){return x86(186,a,b);}
int cmovnle(EAX a,ECX b){return x86(186,a,b);}
int cmovnle(EAX a,REG32 b){return x86(186,a,b);}
int cmovnle(EAX a,MEM32 b){return x86(186,a,b);}
int cmovnle(EAX a,R_M32 b){return x86(186,a,b);}
int cmovnle(ECX a,EAX b){return x86(186,a,b);}
int cmovnle(ECX a,ECX b){return x86(186,a,b);}
int cmovnle(ECX a,REG32 b){return x86(186,a,b);}
int cmovnle(ECX a,MEM32 b){return x86(186,a,b);}
int cmovnle(ECX a,R_M32 b){return x86(186,a,b);}
int cmovnle(REG32 a,EAX b){return x86(186,a,b);}
int cmovnle(REG32 a,ECX b){return x86(186,a,b);}
int cmovnle(REG32 a,REG32 b){return x86(186,a,b);}
int cmovnle(REG32 a,MEM32 b){return x86(186,a,b);}
int cmovnle(REG32 a,R_M32 b){return x86(186,a,b);}
int cmp(AL a,AL b){return x86(187,a,b);}
int cmp(AL a,CL b){return x86(187,a,b);}
int cmp(AL a,REG8 b){return x86(187,a,b);}
int cmp(CL a,AL b){return x86(187,a,b);}
int cmp(CL a,CL b){return x86(187,a,b);}
int cmp(CL a,REG8 b){return x86(187,a,b);}
int cmp(REG8 a,AL b){return x86(187,a,b);}
int cmp(REG8 a,CL b){return x86(187,a,b);}
int cmp(REG8 a,REG8 b){return x86(187,a,b);}
int cmp(MEM8 a,AL b){return x86(187,a,b);}
int cmp(MEM8 a,CL b){return x86(187,a,b);}
int cmp(MEM8 a,REG8 b){return x86(187,a,b);}
int cmp(R_M8 a,AL b){return x86(187,a,b);}
int cmp(R_M8 a,CL b){return x86(187,a,b);}
int cmp(R_M8 a,REG8 b){return x86(187,a,b);}
int cmp(AX a,AX b){return x86(188,a,b);}
int cmp(AX a,DX b){return x86(188,a,b);}
int cmp(AX a,CX b){return x86(188,a,b);}
int cmp(AX a,REG16 b){return x86(188,a,b);}
int cmp(DX a,AX b){return x86(188,a,b);}
int cmp(DX a,DX b){return x86(188,a,b);}
int cmp(DX a,CX b){return x86(188,a,b);}
int cmp(DX a,REG16 b){return x86(188,a,b);}
int cmp(CX a,AX b){return x86(188,a,b);}
int cmp(CX a,DX b){return x86(188,a,b);}
int cmp(CX a,CX b){return x86(188,a,b);}
int cmp(CX a,REG16 b){return x86(188,a,b);}
int cmp(REG16 a,AX b){return x86(188,a,b);}
int cmp(REG16 a,DX b){return x86(188,a,b);}
int cmp(REG16 a,CX b){return x86(188,a,b);}
int cmp(REG16 a,REG16 b){return x86(188,a,b);}
int cmp(MEM16 a,AX b){return x86(188,a,b);}
int cmp(MEM16 a,DX b){return x86(188,a,b);}
int cmp(MEM16 a,CX b){return x86(188,a,b);}
int cmp(MEM16 a,REG16 b){return x86(188,a,b);}
int cmp(R_M16 a,AX b){return x86(188,a,b);}
int cmp(R_M16 a,DX b){return x86(188,a,b);}
int cmp(R_M16 a,CX b){return x86(188,a,b);}
int cmp(R_M16 a,REG16 b){return x86(188,a,b);}
int cmp(EAX a,EAX b){return x86(189,a,b);}
int cmp(EAX a,ECX b){return x86(189,a,b);}
int cmp(EAX a,REG32 b){return x86(189,a,b);}
int cmp(ECX a,EAX b){return x86(189,a,b);}
int cmp(ECX a,ECX b){return x86(189,a,b);}
int cmp(ECX a,REG32 b){return x86(189,a,b);}
int cmp(REG32 a,EAX b){return x86(189,a,b);}
int cmp(REG32 a,ECX b){return x86(189,a,b);}
int cmp(REG32 a,REG32 b){return x86(189,a,b);}
int cmp(MEM32 a,EAX b){return x86(189,a,b);}
int cmp(MEM32 a,ECX b){return x86(189,a,b);}
int cmp(MEM32 a,REG32 b){return x86(189,a,b);}
int cmp(R_M32 a,EAX b){return x86(189,a,b);}
int cmp(R_M32 a,ECX b){return x86(189,a,b);}
int cmp(R_M32 a,REG32 b){return x86(189,a,b);}
int cmp(AL a,MEM8 b){return x86(190,a,b);}
int cmp(AL a,R_M8 b){return x86(190,a,b);}
int cmp(CL a,MEM8 b){return x86(190,a,b);}
int cmp(CL a,R_M8 b){return x86(190,a,b);}
int cmp(REG8 a,MEM8 b){return x86(190,a,b);}
int cmp(REG8 a,R_M8 b){return x86(190,a,b);}
int cmp(AX a,MEM16 b){return x86(191,a,b);}
int cmp(AX a,R_M16 b){return x86(191,a,b);}
int cmp(DX a,MEM16 b){return x86(191,a,b);}
int cmp(DX a,R_M16 b){return x86(191,a,b);}
int cmp(CX a,MEM16 b){return x86(191,a,b);}
int cmp(CX a,R_M16 b){return x86(191,a,b);}
int cmp(REG16 a,MEM16 b){return x86(191,a,b);}
int cmp(REG16 a,R_M16 b){return x86(191,a,b);}
int cmp(EAX a,MEM32 b){return x86(192,a,b);}
int cmp(EAX a,R_M32 b){return x86(192,a,b);}
int cmp(ECX a,MEM32 b){return x86(192,a,b);}
int cmp(ECX a,R_M32 b){return x86(192,a,b);}
int cmp(REG32 a,MEM32 b){return x86(192,a,b);}
int cmp(REG32 a,R_M32 b){return x86(192,a,b);}
int cmp(AL a,char b){return x86(193,a,(IMM)b);}
int cmp(CL a,char b){return x86(193,a,(IMM)b);}
int cmp(REG8 a,char b){return x86(193,a,(IMM)b);}
int cmp(MEM8 a,char b){return x86(193,a,(IMM)b);}
int cmp(R_M8 a,char b){return x86(193,a,(IMM)b);}
int cmp(AX a,char b){return x86(194,a,(IMM)b);}
int cmp(AX a,short b){return x86(194,a,(IMM)b);}
int cmp(DX a,char b){return x86(194,a,(IMM)b);}
int cmp(DX a,short b){return x86(194,a,(IMM)b);}
int cmp(CX a,char b){return x86(194,a,(IMM)b);}
int cmp(CX a,short b){return x86(194,a,(IMM)b);}
int cmp(REG16 a,char b){return x86(194,a,(IMM)b);}
int cmp(REG16 a,short b){return x86(194,a,(IMM)b);}
int cmp(MEM16 a,char b){return x86(194,a,(IMM)b);}
int cmp(MEM16 a,short b){return x86(194,a,(IMM)b);}
int cmp(R_M16 a,char b){return x86(194,a,(IMM)b);}
int cmp(R_M16 a,short b){return x86(194,a,(IMM)b);}
int cmp(EAX a,int b){return x86(195,a,(IMM)b);}
int cmp(EAX a,char b){return x86(195,a,(IMM)b);}
int cmp(EAX a,short b){return x86(195,a,(IMM)b);}
int cmp(EAX a,REF b){return x86(195,a,b);}
int cmp(ECX a,int b){return x86(195,a,(IMM)b);}
int cmp(ECX a,char b){return x86(195,a,(IMM)b);}
int cmp(ECX a,short b){return x86(195,a,(IMM)b);}
int cmp(ECX a,REF b){return x86(195,a,b);}
int cmp(REG32 a,int b){return x86(195,a,(IMM)b);}
int cmp(REG32 a,char b){return x86(195,a,(IMM)b);}
int cmp(REG32 a,short b){return x86(195,a,(IMM)b);}
int cmp(REG32 a,REF b){return x86(195,a,b);}
int cmp(MEM32 a,int b){return x86(195,a,(IMM)b);}
int cmp(MEM32 a,char b){return x86(195,a,(IMM)b);}
int cmp(MEM32 a,short b){return x86(195,a,(IMM)b);}
int cmp(MEM32 a,REF b){return x86(195,a,b);}
int cmp(R_M32 a,int b){return x86(195,a,(IMM)b);}
int cmp(R_M32 a,char b){return x86(195,a,(IMM)b);}
int cmp(R_M32 a,short b){return x86(195,a,(IMM)b);}
int cmp(R_M32 a,REF b){return x86(195,a,b);}
int cmppd(XMMREG a,XMMREG b,char c){return x86(201,a,b,(IMM)c);}
int cmppd(XMMREG a,MEM128 b,char c){return x86(201,a,b,(IMM)c);}
int cmppd(XMMREG a,R_M128 b,char c){return x86(201,a,b,(IMM)c);}
int cmpeqpd(XMMREG a,XMMREG b){return x86(202,a,b);}
int cmpeqpd(XMMREG a,MEM128 b){return x86(202,a,b);}
int cmpeqpd(XMMREG a,R_M128 b){return x86(202,a,b);}
int cmpltpd(XMMREG a,XMMREG b){return x86(203,a,b);}
int cmpltpd(XMMREG a,MEM128 b){return x86(203,a,b);}
int cmpltpd(XMMREG a,R_M128 b){return x86(203,a,b);}
int cmplepd(XMMREG a,XMMREG b){return x86(204,a,b);}
int cmplepd(XMMREG a,MEM128 b){return x86(204,a,b);}
int cmplepd(XMMREG a,R_M128 b){return x86(204,a,b);}
int cmpunordpd(XMMREG a,XMMREG b){return x86(205,a,b);}
int cmpunordpd(XMMREG a,MEM128 b){return x86(205,a,b);}
int cmpunordpd(XMMREG a,R_M128 b){return x86(205,a,b);}
int cmpneqpd(XMMREG a,XMMREG b){return x86(206,a,b);}
int cmpneqpd(XMMREG a,MEM128 b){return x86(206,a,b);}
int cmpneqpd(XMMREG a,R_M128 b){return x86(206,a,b);}
int cmpnltpd(XMMREG a,XMMREG b){return x86(207,a,b);}
int cmpnltpd(XMMREG a,MEM128 b){return x86(207,a,b);}
int cmpnltpd(XMMREG a,R_M128 b){return x86(207,a,b);}
int cmpnlepd(XMMREG a,XMMREG b){return x86(208,a,b);}
int cmpnlepd(XMMREG a,MEM128 b){return x86(208,a,b);}
int cmpnlepd(XMMREG a,R_M128 b){return x86(208,a,b);}
int cmpordpd(XMMREG a,XMMREG b){return x86(209,a,b);}
int cmpordpd(XMMREG a,MEM128 b){return x86(209,a,b);}
int cmpordpd(XMMREG a,R_M128 b){return x86(209,a,b);}
int cmpps(XMMREG a,XMMREG b,char c){return x86(210,a,b,(IMM)c);}
int cmpps(XMMREG a,MEM128 b,char c){return x86(210,a,b,(IMM)c);}
int cmpps(XMMREG a,R_M128 b,char c){return x86(210,a,b,(IMM)c);}
int cmpeqps(XMMREG a,XMMREG b){return x86(211,a,b);}
int cmpeqps(XMMREG a,MEM128 b){return x86(211,a,b);}
int cmpeqps(XMMREG a,R_M128 b){return x86(211,a,b);}
int cmpleps(XMMREG a,XMMREG b){return x86(212,a,b);}
int cmpleps(XMMREG a,MEM128 b){return x86(212,a,b);}
int cmpleps(XMMREG a,R_M128 b){return x86(212,a,b);}
int cmpltps(XMMREG a,XMMREG b){return x86(213,a,b);}
int cmpltps(XMMREG a,MEM128 b){return x86(213,a,b);}
int cmpltps(XMMREG a,R_M128 b){return x86(213,a,b);}
int cmpneqps(XMMREG a,XMMREG b){return x86(214,a,b);}
int cmpneqps(XMMREG a,MEM128 b){return x86(214,a,b);}
int cmpneqps(XMMREG a,R_M128 b){return x86(214,a,b);}
int cmpnleps(XMMREG a,XMMREG b){return x86(215,a,b);}
int cmpnleps(XMMREG a,MEM128 b){return x86(215,a,b);}
int cmpnleps(XMMREG a,R_M128 b){return x86(215,a,b);}
int cmpnltps(XMMREG a,XMMREG b){return x86(216,a,b);}
int cmpnltps(XMMREG a,MEM128 b){return x86(216,a,b);}
int cmpnltps(XMMREG a,R_M128 b){return x86(216,a,b);}
int cmpordps(XMMREG a,XMMREG b){return x86(217,a,b);}
int cmpordps(XMMREG a,MEM128 b){return x86(217,a,b);}
int cmpordps(XMMREG a,R_M128 b){return x86(217,a,b);}
int cmpunordps(XMMREG a,XMMREG b){return x86(218,a,b);}
int cmpunordps(XMMREG a,MEM128 b){return x86(218,a,b);}
int cmpunordps(XMMREG a,R_M128 b){return x86(218,a,b);}
int cmpsb(){return x86(219);}
int cmpsw(){return x86(220);}
int cmpsd(){return x86(221);}
int repe_cmpsb(){return x86(222);}
int repe_cmpsw(){return x86(223);}
int repe_cmpsd(){return x86(224);}
int repne_cmpsb(){return x86(225);}
int repne_cmpsw(){return x86(226);}
int repne_cmpsd(){return x86(227);}
int repz_cmpsb(){return x86(228);}
int repz_cmpsw(){return x86(229);}
int repz_cmpsd(){return x86(230);}
int repnz_cmpsb(){return x86(231);}
int repnz_cmpsw(){return x86(232);}
int repnz_cmpsd(){return x86(233);}
int cmpsd(XMMREG a,XMMREG b,char c){return x86(234,a,b,(IMM)c);}
int cmpsd(XMMREG a,MEM64 b,char c){return x86(234,a,b,(IMM)c);}
int cmpsd(XMMREG a,XMM64 b,char c){return x86(234,a,b,(IMM)c);}
int cmpeqsd(XMMREG a,XMMREG b){return x86(235,a,b);}
int cmpeqsd(XMMREG a,MEM64 b){return x86(235,a,b);}
int cmpeqsd(XMMREG a,XMM64 b){return x86(235,a,b);}
int cmpltsd(XMMREG a,XMMREG b){return x86(236,a,b);}
int cmpltsd(XMMREG a,MEM64 b){return x86(236,a,b);}
int cmpltsd(XMMREG a,XMM64 b){return x86(236,a,b);}
int cmplesd(XMMREG a,XMMREG b){return x86(237,a,b);}
int cmplesd(XMMREG a,MEM64 b){return x86(237,a,b);}
int cmplesd(XMMREG a,XMM64 b){return x86(237,a,b);}
int cmpunordsd(XMMREG a,XMMREG b){return x86(238,a,b);}
int cmpunordsd(XMMREG a,MEM64 b){return x86(238,a,b);}
int cmpunordsd(XMMREG a,XMM64 b){return x86(238,a,b);}
int cmpneqsd(XMMREG a,XMMREG b){return x86(239,a,b);}
int cmpneqsd(XMMREG a,MEM64 b){return x86(239,a,b);}
int cmpneqsd(XMMREG a,XMM64 b){return x86(239,a,b);}
int cmpnltsd(XMMREG a,XMMREG b){return x86(240,a,b);}
int cmpnltsd(XMMREG a,MEM64 b){return x86(240,a,b);}
int cmpnltsd(XMMREG a,XMM64 b){return x86(240,a,b);}
int cmpnlesd(XMMREG a,XMMREG b){return x86(241,a,b);}
int cmpnlesd(XMMREG a,MEM64 b){return x86(241,a,b);}
int cmpnlesd(XMMREG a,XMM64 b){return x86(241,a,b);}
int cmpordsd(XMMREG a,XMMREG b){return x86(242,a,b);}
int cmpordsd(XMMREG a,MEM64 b){return x86(242,a,b);}
int cmpordsd(XMMREG a,XMM64 b){return x86(242,a,b);}
int cmpss(XMMREG a,XMMREG b,char c){return x86(243,a,b,(IMM)c);}
int cmpss(XMMREG a,MEM32 b,char c){return x86(243,a,b,(IMM)c);}
int cmpss(XMMREG a,XMM32 b,char c){return x86(243,a,b,(IMM)c);}
int cmpeqss(XMMREG a,XMMREG b){return x86(244,a,b);}
int cmpeqss(XMMREG a,MEM32 b){return x86(244,a,b);}
int cmpeqss(XMMREG a,XMM32 b){return x86(244,a,b);}
int cmpless(XMMREG a,XMMREG b){return x86(245,a,b);}
int cmpless(XMMREG a,MEM32 b){return x86(245,a,b);}
int cmpless(XMMREG a,XMM32 b){return x86(245,a,b);}
int cmpltss(XMMREG a,XMMREG b){return x86(246,a,b);}
int cmpltss(XMMREG a,MEM32 b){return x86(246,a,b);}
int cmpltss(XMMREG a,XMM32 b){return x86(246,a,b);}
int cmpneqss(XMMREG a,XMMREG b){return x86(247,a,b);}
int cmpneqss(XMMREG a,MEM32 b){return x86(247,a,b);}
int cmpneqss(XMMREG a,XMM32 b){return x86(247,a,b);}
int cmpnless(XMMREG a,XMMREG b){return x86(248,a,b);}
int cmpnless(XMMREG a,MEM32 b){return x86(248,a,b);}
int cmpnless(XMMREG a,XMM32 b){return x86(248,a,b);}
int cmpnltss(XMMREG a,XMMREG b){return x86(249,a,b);}
int cmpnltss(XMMREG a,MEM32 b){return x86(249,a,b);}
int cmpnltss(XMMREG a,XMM32 b){return x86(249,a,b);}
int cmpordss(XMMREG a,XMMREG b){return x86(250,a,b);}
int cmpordss(XMMREG a,MEM32 b){return x86(250,a,b);}
int cmpordss(XMMREG a,XMM32 b){return x86(250,a,b);}
int cmpunordss(XMMREG a,XMMREG b){return x86(251,a,b);}
int cmpunordss(XMMREG a,MEM32 b){return x86(251,a,b);}
int cmpunordss(XMMREG a,XMM32 b){return x86(251,a,b);}
int cmpxchg(AL a,AL b){return x86(252,a,b);}
int cmpxchg(AL a,CL b){return x86(252,a,b);}
int cmpxchg(AL a,REG8 b){return x86(252,a,b);}
int cmpxchg(CL a,AL b){return x86(252,a,b);}
int cmpxchg(CL a,CL b){return x86(252,a,b);}
int cmpxchg(CL a,REG8 b){return x86(252,a,b);}
int cmpxchg(REG8 a,AL b){return x86(252,a,b);}
int cmpxchg(REG8 a,CL b){return x86(252,a,b);}
int cmpxchg(REG8 a,REG8 b){return x86(252,a,b);}
int cmpxchg(MEM8 a,AL b){return x86(252,a,b);}
int cmpxchg(MEM8 a,CL b){return x86(252,a,b);}
int cmpxchg(MEM8 a,REG8 b){return x86(252,a,b);}
int cmpxchg(R_M8 a,AL b){return x86(252,a,b);}
int cmpxchg(R_M8 a,CL b){return x86(252,a,b);}
int cmpxchg(R_M8 a,REG8 b){return x86(252,a,b);}
int cmpxchg(AX a,AX b){return x86(253,a,b);}
int cmpxchg(AX a,DX b){return x86(253,a,b);}
int cmpxchg(AX a,CX b){return x86(253,a,b);}
int cmpxchg(AX a,REG16 b){return x86(253,a,b);}
int cmpxchg(DX a,AX b){return x86(253,a,b);}
int cmpxchg(DX a,DX b){return x86(253,a,b);}
int cmpxchg(DX a,CX b){return x86(253,a,b);}
int cmpxchg(DX a,REG16 b){return x86(253,a,b);}
int cmpxchg(CX a,AX b){return x86(253,a,b);}
int cmpxchg(CX a,DX b){return x86(253,a,b);}
int cmpxchg(CX a,CX b){return x86(253,a,b);}
int cmpxchg(CX a,REG16 b){return x86(253,a,b);}
int cmpxchg(REG16 a,AX b){return x86(253,a,b);}
int cmpxchg(REG16 a,DX b){return x86(253,a,b);}
int cmpxchg(REG16 a,CX b){return x86(253,a,b);}
int cmpxchg(REG16 a,REG16 b){return x86(253,a,b);}
int cmpxchg(MEM16 a,AX b){return x86(253,a,b);}
int cmpxchg(MEM16 a,DX b){return x86(253,a,b);}
int cmpxchg(MEM16 a,CX b){return x86(253,a,b);}
int cmpxchg(MEM16 a,REG16 b){return x86(253,a,b);}
int cmpxchg(R_M16 a,AX b){return x86(253,a,b);}
int cmpxchg(R_M16 a,DX b){return x86(253,a,b);}
int cmpxchg(R_M16 a,CX b){return x86(253,a,b);}
int cmpxchg(R_M16 a,REG16 b){return x86(253,a,b);}
int cmpxchg(EAX a,EAX b){return x86(254,a,b);}
int cmpxchg(EAX a,ECX b){return x86(254,a,b);}
int cmpxchg(EAX a,REG32 b){return x86(254,a,b);}
int cmpxchg(ECX a,EAX b){return x86(254,a,b);}
int cmpxchg(ECX a,ECX b){return x86(254,a,b);}
int cmpxchg(ECX a,REG32 b){return x86(254,a,b);}
int cmpxchg(REG32 a,EAX b){return x86(254,a,b);}
int cmpxchg(REG32 a,ECX b){return x86(254,a,b);}
int cmpxchg(REG32 a,REG32 b){return x86(254,a,b);}
int cmpxchg(MEM32 a,EAX b){return x86(254,a,b);}
int cmpxchg(MEM32 a,ECX b){return x86(254,a,b);}
int cmpxchg(MEM32 a,REG32 b){return x86(254,a,b);}
int cmpxchg(R_M32 a,EAX b){return x86(254,a,b);}
int cmpxchg(R_M32 a,ECX b){return x86(254,a,b);}
int cmpxchg(R_M32 a,REG32 b){return x86(254,a,b);}
int lock_cmpxchg(MEM8 a,AL b){return x86(255,a,b);}
int lock_cmpxchg(MEM8 a,CL b){return x86(255,a,b);}
int lock_cmpxchg(MEM8 a,REG8 b){return x86(255,a,b);}
int lock_cmpxchg(MEM16 a,AX b){return x86(256,a,b);}
int lock_cmpxchg(MEM16 a,DX b){return x86(256,a,b);}
int lock_cmpxchg(MEM16 a,CX b){return x86(256,a,b);}
int lock_cmpxchg(MEM16 a,REG16 b){return x86(256,a,b);}
int lock_cmpxchg(MEM32 a,EAX b){return x86(257,a,b);}
int lock_cmpxchg(MEM32 a,ECX b){return x86(257,a,b);}
int lock_cmpxchg(MEM32 a,REG32 b){return x86(257,a,b);}
int cmpxchg8b(MEM8 a){return x86(258,a);}
int cmpxchg8b(MEM16 a){return x86(258,a);}
int cmpxchg8b(MEM32 a){return x86(258,a);}
int cmpxchg8b(MEM64 a){return x86(258,a);}
int cmpxchg8b(MEM128 a){return x86(258,a);}
int lock_cmpxchg8b(MEM8 a){return x86(259,a);}
int lock_cmpxchg8b(MEM16 a){return x86(259,a);}
int lock_cmpxchg8b(MEM32 a){return x86(259,a);}
int lock_cmpxchg8b(MEM64 a){return x86(259,a);}
int lock_cmpxchg8b(MEM128 a){return x86(259,a);}
int comisd(XMMREG a,XMMREG b){return x86(260,a,b);}
int comisd(XMMREG a,MEM64 b){return x86(260,a,b);}
int comisd(XMMREG a,XMM64 b){return x86(260,a,b);}
int comiss(XMMREG a,XMMREG b){return x86(261,a,b);}
int comiss(XMMREG a,MEM32 b){return x86(261,a,b);}
int comiss(XMMREG a,XMM32 b){return x86(261,a,b);}
int cpuid(){return x86(262);}
int cvtdq2pd(XMMREG a,XMMREG b){return x86(263,a,b);}
int cvtdq2pd(XMMREG a,MEM64 b){return x86(263,a,b);}
int cvtdq2pd(XMMREG a,XMM64 b){return x86(263,a,b);}
int cvtdq2ps(XMMREG a,XMMREG b){return x86(264,a,b);}
int cvtdq2ps(XMMREG a,MEM128 b){return x86(264,a,b);}
int cvtdq2ps(XMMREG a,R_M128 b){return x86(264,a,b);}
int cvtpd2dq(XMMREG a,XMMREG b){return x86(265,a,b);}
int cvtpd2dq(XMMREG a,MEM128 b){return x86(265,a,b);}
int cvtpd2dq(XMMREG a,R_M128 b){return x86(265,a,b);}
int cvtpd2pi(MMREG a,XMMREG b){return x86(266,a,b);}
int cvtpd2pi(MMREG a,MEM128 b){return x86(266,a,b);}
int cvtpd2pi(MMREG a,R_M128 b){return x86(266,a,b);}
int cvtpd2ps(XMMREG a,XMMREG b){return x86(267,a,b);}
int cvtpd2ps(XMMREG a,MEM128 b){return x86(267,a,b);}
int cvtpd2ps(XMMREG a,R_M128 b){return x86(267,a,b);}
int cvtpi2pd(XMMREG a,MMREG b){return x86(268,a,b);}
int cvtpi2pd(XMMREG a,MEM64 b){return x86(268,a,b);}
int cvtpi2pd(XMMREG a,R_M64 b){return x86(268,a,b);}
int cvtps2dq(XMMREG a,XMMREG b){return x86(269,a,b);}
int cvtps2dq(XMMREG a,MEM128 b){return x86(269,a,b);}
int cvtps2dq(XMMREG a,R_M128 b){return x86(269,a,b);}
int cvtps2pd(XMMREG a,XMMREG b){return x86(270,a,b);}
int cvtps2pd(XMMREG a,MEM64 b){return x86(270,a,b);}
int cvtps2pd(XMMREG a,XMM64 b){return x86(270,a,b);}
int cvtsd2si(EAX a,XMMREG b){return x86(271,a,b);}
int cvtsd2si(EAX a,MEM64 b){return x86(271,a,b);}
int cvtsd2si(EAX a,XMM64 b){return x86(271,a,b);}
int cvtsd2si(ECX a,XMMREG b){return x86(271,a,b);}
int cvtsd2si(ECX a,MEM64 b){return x86(271,a,b);}
int cvtsd2si(ECX a,XMM64 b){return x86(271,a,b);}
int cvtsd2si(REG32 a,XMMREG b){return x86(271,a,b);}
int cvtsd2si(REG32 a,MEM64 b){return x86(271,a,b);}
int cvtsd2si(REG32 a,XMM64 b){return x86(271,a,b);}
int cvtsi2sd(XMMREG a,EAX b){return x86(272,a,b);}
int cvtsi2sd(XMMREG a,ECX b){return x86(272,a,b);}
int cvtsi2sd(XMMREG a,REG32 b){return x86(272,a,b);}
int cvtsi2sd(XMMREG a,MEM32 b){return x86(272,a,b);}
int cvtsi2sd(XMMREG a,R_M32 b){return x86(272,a,b);}
int cvtss2sd(XMMREG a,XMMREG b){return x86(273,a,b);}
int cvtss2sd(XMMREG a,MEM32 b){return x86(273,a,b);}
int cvtss2sd(XMMREG a,XMM32 b){return x86(273,a,b);}
int cvttpd2dq(XMMREG a,XMMREG b){return x86(274,a,b);}
int cvttpd2dq(XMMREG a,MEM128 b){return x86(274,a,b);}
int cvttpd2dq(XMMREG a,R_M128 b){return x86(274,a,b);}
int cvttpd2pi(MMREG a,XMMREG b){return x86(275,a,b);}
int cvttpd2pi(MMREG a,MEM128 b){return x86(275,a,b);}
int cvttpd2pi(MMREG a,R_M128 b){return x86(275,a,b);}
int cvttps2dq(XMMREG a,XMMREG b){return x86(276,a,b);}
int cvttps2dq(XMMREG a,MEM128 b){return x86(276,a,b);}
int cvttps2dq(XMMREG a,R_M128 b){return x86(276,a,b);}
int cvttsd2si(EAX a,XMMREG b){return x86(277,a,b);}
int cvttsd2si(EAX a,MEM64 b){return x86(277,a,b);}
int cvttsd2si(EAX a,XMM64 b){return x86(277,a,b);}
int cvttsd2si(ECX a,XMMREG b){return x86(277,a,b);}
int cvttsd2si(ECX a,MEM64 b){return x86(277,a,b);}
int cvttsd2si(ECX a,XMM64 b){return x86(277,a,b);}
int cvttsd2si(REG32 a,XMMREG b){return x86(277,a,b);}
int cvttsd2si(REG32 a,MEM64 b){return x86(277,a,b);}
int cvttsd2si(REG32 a,XMM64 b){return x86(277,a,b);}
int cvtpi2ps(XMMREG a,MMREG b){return x86(278,a,b);}
int cvtpi2ps(XMMREG a,MEM64 b){return x86(278,a,b);}
int cvtpi2ps(XMMREG a,R_M64 b){return x86(278,a,b);}
int cvtps2pi(MMREG a,XMMREG b){return x86(279,a,b);}
int cvtps2pi(MMREG a,MEM64 b){return x86(279,a,b);}
int cvtps2pi(MMREG a,XMM64 b){return x86(279,a,b);}
int cvttps2pi(MMREG a,XMMREG b){return x86(280,a,b);}
int cvttps2pi(MMREG a,MEM64 b){return x86(280,a,b);}
int cvttps2pi(MMREG a,XMM64 b){return x86(280,a,b);}
int cvtsi2ss(XMMREG a,EAX b){return x86(281,a,b);}
int cvtsi2ss(XMMREG a,ECX b){return x86(281,a,b);}
int cvtsi2ss(XMMREG a,REG32 b){return x86(281,a,b);}
int cvtsi2ss(XMMREG a,MEM32 b){return x86(281,a,b);}
int cvtsi2ss(XMMREG a,R_M32 b){return x86(281,a,b);}
int cvtss2si(EAX a,XMMREG b){return x86(282,a,b);}
int cvtss2si(EAX a,MEM32 b){return x86(282,a,b);}
int cvtss2si(EAX a,XMM32 b){return x86(282,a,b);}
int cvtss2si(ECX a,XMMREG b){return x86(282,a,b);}
int cvtss2si(ECX a,MEM32 b){return x86(282,a,b);}
int cvtss2si(ECX a,XMM32 b){return x86(282,a,b);}
int cvtss2si(REG32 a,XMMREG b){return x86(282,a,b);}
int cvtss2si(REG32 a,MEM32 b){return x86(282,a,b);}
int cvtss2si(REG32 a,XMM32 b){return x86(282,a,b);}
int cvttss2si(EAX a,XMMREG b){return x86(283,a,b);}
int cvttss2si(EAX a,MEM32 b){return x86(283,a,b);}
int cvttss2si(EAX a,XMM32 b){return x86(283,a,b);}
int cvttss2si(ECX a,XMMREG b){return x86(283,a,b);}
int cvttss2si(ECX a,MEM32 b){return x86(283,a,b);}
int cvttss2si(ECX a,XMM32 b){return x86(283,a,b);}
int cvttss2si(REG32 a,XMMREG b){return x86(283,a,b);}
int cvttss2si(REG32 a,MEM32 b){return x86(283,a,b);}
int cvttss2si(REG32 a,XMM32 b){return x86(283,a,b);}
int daa(){return x86(284);}
int das(){return x86(285);}
int dec(AX a){return x86(286,a);}
int dec(DX a){return x86(286,a);}
int dec(CX a){return x86(286,a);}
int dec(REG16 a){return x86(286,a);}
int dec(EAX a){return x86(287,a);}
int dec(ECX a){return x86(287,a);}
int dec(REG32 a){return x86(287,a);}
int dec(AL a){return x86(288,a);}
int dec(CL a){return x86(288,a);}
int dec(REG8 a){return x86(288,a);}
int dec(MEM8 a){return x86(288,a);}
int dec(R_M8 a){return x86(288,a);}
int dec(MEM16 a){return x86(289,a);}
int dec(R_M16 a){return x86(289,a);}
int dec(MEM32 a){return x86(290,a);}
int dec(R_M32 a){return x86(290,a);}
int lock_dec(MEM8 a){return x86(291,a);}
int lock_dec(MEM16 a){return x86(292,a);}
int lock_dec(MEM32 a){return x86(293,a);}
int div(AL a){return x86(294,a);}
int div(CL a){return x86(294,a);}
int div(REG8 a){return x86(294,a);}
int div(MEM8 a){return x86(294,a);}
int div(R_M8 a){return x86(294,a);}
int div(AX a){return x86(295,a);}
int div(DX a){return x86(295,a);}
int div(CX a){return x86(295,a);}
int div(REG16 a){return x86(295,a);}
int div(MEM16 a){return x86(295,a);}
int div(R_M16 a){return x86(295,a);}
int div(EAX a){return x86(296,a);}
int div(ECX a){return x86(296,a);}
int div(REG32 a){return x86(296,a);}
int div(MEM32 a){return x86(296,a);}
int div(R_M32 a){return x86(296,a);}
int divpd(XMMREG a,XMMREG b){return x86(297,a,b);}
int divpd(XMMREG a,MEM128 b){return x86(297,a,b);}
int divpd(XMMREG a,R_M128 b){return x86(297,a,b);}
int divps(XMMREG a,XMMREG b){return x86(298,a,b);}
int divps(XMMREG a,MEM128 b){return x86(298,a,b);}
int divps(XMMREG a,R_M128 b){return x86(298,a,b);}
int divsd(XMMREG a,XMMREG b){return x86(299,a,b);}
int divsd(XMMREG a,MEM64 b){return x86(299,a,b);}
int divsd(XMMREG a,XMM64 b){return x86(299,a,b);}
int divss(XMMREG a,XMMREG b){return x86(300,a,b);}
int divss(XMMREG a,MEM32 b){return x86(300,a,b);}
int divss(XMMREG a,XMM32 b){return x86(300,a,b);}
int emms(){return x86(301);}
int f2xm1(){return x86(302);}
int fabs(){return x86(303);}
int fadd(MEM32 a){return x86(304,a);}
int fadd(MEM64 a){return x86(305,a);}
int fadd(ST0 a){return x86(306,a);}
int fadd(FPUREG a){return x86(306,a);}
int fadd(ST0 a,ST0 b){return x86(307,a,b);}
int fadd(ST0 a,FPUREG b){return x86(307,a,b);}
int fadd(FPUREG a,ST0 b){return x86(308,a,b);}
int faddp(){return x86(309);}
int faddp(ST0 a){return x86(310,a);}
int faddp(FPUREG a){return x86(310,a);}
int faddp(ST0 a,ST0 b){return x86(311,a,b);}
int faddp(FPUREG a,ST0 b){return x86(311,a,b);}
int fchs(){return x86(312);}
int fclex(){return x86(313);}
int fnclex(){return x86(314);}
int fcmovb(ST0 a){return x86(315,a);}
int fcmovb(FPUREG a){return x86(315,a);}
int fcmovb(ST0 a,ST0 b){return x86(316,a,b);}
int fcmovb(ST0 a,FPUREG b){return x86(316,a,b);}
int fcmovbe(ST0 a){return x86(317,a);}
int fcmovbe(FPUREG a){return x86(317,a);}
int fcmovbe(ST0 a,ST0 b){return x86(318,a,b);}
int fcmovbe(ST0 a,FPUREG b){return x86(318,a,b);}
int fcmove(ST0 a){return x86(319,a);}
int fcmove(FPUREG a){return x86(319,a);}
int fcmove(ST0 a,ST0 b){return x86(320,a,b);}
int fcmove(ST0 a,FPUREG b){return x86(320,a,b);}
int fcmovnb(ST0 a){return x86(321,a);}
int fcmovnb(FPUREG a){return x86(321,a);}
int fcmovnb(ST0 a,ST0 b){return x86(322,a,b);}
int fcmovnb(ST0 a,FPUREG b){return x86(322,a,b);}
int fcmovnbe(ST0 a){return x86(323,a);}
int fcmovnbe(FPUREG a){return x86(323,a);}
int fcmovnbe(ST0 a,ST0 b){return x86(324,a,b);}
int fcmovnbe(ST0 a,FPUREG b){return x86(324,a,b);}
int fcmovne(ST0 a){return x86(325,a);}
int fcmovne(FPUREG a){return x86(325,a);}
int fcmovne(ST0 a,ST0 b){return x86(326,a,b);}
int fcmovne(ST0 a,FPUREG b){return x86(326,a,b);}
int fcmovnu(ST0 a){return x86(327,a);}
int fcmovnu(FPUREG a){return x86(327,a);}
int fcmovnu(ST0 a,ST0 b){return x86(328,a,b);}
int fcmovnu(ST0 a,FPUREG b){return x86(328,a,b);}
int fcmovu(ST0 a){return x86(329,a);}
int fcmovu(FPUREG a){return x86(329,a);}
int fcmovu(ST0 a,ST0 b){return x86(330,a,b);}
int fcmovu(ST0 a,FPUREG b){return x86(330,a,b);}
int fcom(MEM32 a){return x86(331,a);}
int fcom(MEM64 a){return x86(332,a);}
int fcom(ST0 a){return x86(333,a);}
int fcom(FPUREG a){return x86(333,a);}
int fcom(ST0 a,ST0 b){return x86(334,a,b);}
int fcom(ST0 a,FPUREG b){return x86(334,a,b);}
int fcomp(MEM32 a){return x86(335,a);}
int fcomp(MEM64 a){return x86(336,a);}
int fcomp(ST0 a){return x86(337,a);}
int fcomp(FPUREG a){return x86(337,a);}
int fcomp(ST0 a,ST0 b){return x86(338,a,b);}
int fcomp(ST0 a,FPUREG b){return x86(338,a,b);}
int fcompp(){return x86(339);}
int fcomi(ST0 a){return x86(340,a);}
int fcomi(FPUREG a){return x86(340,a);}
int fcomi(ST0 a,ST0 b){return x86(341,a,b);}
int fcomi(ST0 a,FPUREG b){return x86(341,a,b);}
int fcomip(ST0 a){return x86(342,a);}
int fcomip(FPUREG a){return x86(342,a);}
int fcomip(ST0 a,ST0 b){return x86(343,a,b);}
int fcomip(ST0 a,FPUREG b){return x86(343,a,b);}
int fcos(){return x86(344);}
int fdecstp(){return x86(345);}
int fdisi(){return x86(346);}
int fndisi(){return x86(347);}
int feni(){return x86(348);}
int fneni(){return x86(349);}
int fdiv(MEM32 a){return x86(350,a);}
int fdiv(MEM64 a){return x86(351,a);}
int fdiv(ST0 a){return x86(352,a);}
int fdiv(FPUREG a){return x86(352,a);}
int fdiv(ST0 a,ST0 b){return x86(353,a,b);}
int fdiv(ST0 a,FPUREG b){return x86(353,a,b);}
int fdiv(FPUREG a,ST0 b){return x86(354,a,b);}
int fdivr(MEM32 a){return x86(355,a);}
int fdivr(MEM64 a){return x86(356,a);}
int fdivr(ST0 a){return x86(357,a);}
int fdivr(FPUREG a){return x86(357,a);}
int fdivr(ST0 a,ST0 b){return x86(358,a,b);}
int fdivr(ST0 a,FPUREG b){return x86(358,a,b);}
int fdivr(FPUREG a,ST0 b){return x86(359,a,b);}
int fdivp(){return x86(360);}
int fdivp(ST0 a){return x86(361,a);}
int fdivp(FPUREG a){return x86(361,a);}
int fdivp(ST0 a,ST0 b){return x86(362,a,b);}
int fdivp(FPUREG a,ST0 b){return x86(362,a,b);}
int fdivrp(){return x86(363);}
int fdivrp(ST0 a){return x86(364,a);}
int fdivrp(FPUREG a){return x86(364,a);}
int fdivrp(ST0 a,ST0 b){return x86(365,a,b);}
int fdivrp(FPUREG a,ST0 b){return x86(365,a,b);}
int femms(){return x86(366);}
int ffree(ST0 a){return x86(367,a);}
int ffree(FPUREG a){return x86(367,a);}
int fiadd(MEM16 a){return x86(368,a);}
int fiadd(MEM32 a){return x86(369,a);}
int ficom(MEM16 a){return x86(370,a);}
int ficom(MEM32 a){return x86(371,a);}
int ficomp(MEM16 a){return x86(372,a);}
int ficomp(MEM32 a){return x86(373,a);}
int fidiv(MEM16 a){return x86(374,a);}
int fidiv(MEM32 a){return x86(375,a);}
int fidivr(MEM16 a){return x86(376,a);}
int fidivr(MEM32 a){return x86(377,a);}
int fild(MEM16 a){return x86(378,a);}
int fild(MEM32 a){return x86(379,a);}
int fild(MEM64 a){return x86(380,a);}
int fist(MEM16 a){return x86(381,a);}
int fist(MEM32 a){return x86(382,a);}
int fistp(MEM16 a){return x86(383,a);}
int fistp(MEM32 a){return x86(384,a);}
int fistp(MEM64 a){return x86(385,a);}
int fimul(MEM16 a){return x86(386,a);}
int fimul(MEM32 a){return x86(387,a);}
int fincstp(){return x86(388);}
int finit(){return x86(389);}
int fninit(){return x86(390);}
int fisub(MEM16 a){return x86(391,a);}
int fisub(MEM32 a){return x86(392,a);}
int fisubr(MEM16 a){return x86(393,a);}
int fisubr(MEM32 a){return x86(394,a);}
int fld(MEM32 a){return x86(395,a);}
int fld(MEM64 a){return x86(396,a);}
int fld(ST0 a){return x86(397,a);}
int fld(FPUREG a){return x86(397,a);}
int fld1(){return x86(398);}
int fldl2e(){return x86(399);}
int fldl2t(){return x86(400);}
int fldlg2(){return x86(401);}
int fldln2(){return x86(402);}
int fldpi(){return x86(403);}
int fldz(){return x86(404);}
int fldcw(MEM16 a){return x86(405,a);}
int fldenv(MEM8 a){return x86(406,a);}
int fldenv(MEM16 a){return x86(406,a);}
int fldenv(MEM32 a){return x86(406,a);}
int fldenv(MEM64 a){return x86(406,a);}
int fldenv(MEM128 a){return x86(406,a);}
int fmul(MEM32 a){return x86(407,a);}
int fmul(MEM64 a){return x86(408,a);}
int fmul(){return x86(409);}
int fmul(ST0 a){return x86(410,a);}
int fmul(FPUREG a){return x86(410,a);}
int fmul(ST0 a,ST0 b){return x86(411,a,b);}
int fmul(ST0 a,FPUREG b){return x86(411,a,b);}
int fmul(FPUREG a,ST0 b){return x86(412,a,b);}
int fmulp(ST0 a){return x86(413,a);}
int fmulp(FPUREG a){return x86(413,a);}
int fmulp(ST0 a,ST0 b){return x86(414,a,b);}
int fmulp(FPUREG a,ST0 b){return x86(414,a,b);}
int fmulp(){return x86(415);}
int fnop(){return x86(416);}
int fpatan(){return x86(417);}
int fptan(){return x86(418);}
int fprem(){return x86(419);}
int fprem1(){return x86(420);}
int frndint(){return x86(421);}
int fsave(MEM8 a){return x86(422,a);}
int fsave(MEM16 a){return x86(422,a);}
int fsave(MEM32 a){return x86(422,a);}
int fsave(MEM64 a){return x86(422,a);}
int fsave(MEM128 a){return x86(422,a);}
int fnsave(MEM8 a){return x86(423,a);}
int fnsave(MEM16 a){return x86(423,a);}
int fnsave(MEM32 a){return x86(423,a);}
int fnsave(MEM64 a){return x86(423,a);}
int fnsave(MEM128 a){return x86(423,a);}
int frstor(MEM8 a){return x86(424,a);}
int frstor(MEM16 a){return x86(424,a);}
int frstor(MEM32 a){return x86(424,a);}
int frstor(MEM64 a){return x86(424,a);}
int frstor(MEM128 a){return x86(424,a);}
int fscale(){return x86(425);}
int fsetpm(){return x86(426);}
int fsin(){return x86(427);}
int fsincos(){return x86(428);}
int fsqrt(){return x86(429);}
int fst(MEM32 a){return x86(430,a);}
int fst(MEM64 a){return x86(431,a);}
int fst(ST0 a){return x86(432,a);}
int fst(FPUREG a){return x86(432,a);}
int fstp(MEM32 a){return x86(433,a);}
int fstp(MEM64 a){return x86(434,a);}
int fstp(ST0 a){return x86(435,a);}
int fstp(FPUREG a){return x86(435,a);}
int fstcw(MEM16 a){return x86(436,a);}
int fnstcw(MEM16 a){return x86(437,a);}
int fstenv(MEM8 a){return x86(438,a);}
int fstenv(MEM16 a){return x86(438,a);}
int fstenv(MEM32 a){return x86(438,a);}
int fstenv(MEM64 a){return x86(438,a);}
int fstenv(MEM128 a){return x86(438,a);}
int fnstenv(MEM8 a){return x86(439,a);}
int fnstenv(MEM16 a){return x86(439,a);}
int fnstenv(MEM32 a){return x86(439,a);}
int fnstenv(MEM64 a){return x86(439,a);}
int fnstenv(MEM128 a){return x86(439,a);}
int fstsw(MEM16 a){return x86(440,a);}
int fstsw(AX a){return x86(441,a);}
int fnstsw(MEM16 a){return x86(442,a);}
int fnstsw(AX a){return x86(443,a);}
int fsub(MEM32 a){return x86(444,a);}
int fsub(MEM64 a){return x86(445,a);}
int fsub(ST0 a){return x86(446,a);}
int fsub(FPUREG a){return x86(446,a);}
int fsub(ST0 a,ST0 b){return x86(447,a,b);}
int fsub(ST0 a,FPUREG b){return x86(447,a,b);}
int fsub(FPUREG a,ST0 b){return x86(448,a,b);}
int fsubr(MEM32 a){return x86(449,a);}
int fsubr(MEM64 a){return x86(450,a);}
int fsubr(ST0 a){return x86(451,a);}
int fsubr(FPUREG a){return x86(451,a);}
int fsubr(ST0 a,ST0 b){return x86(452,a,b);}
int fsubr(ST0 a,FPUREG b){return x86(452,a,b);}
int fsubr(FPUREG a,ST0 b){return x86(453,a,b);}
int fsubp(){return x86(454);}
int fsubp(ST0 a){return x86(455,a);}
int fsubp(FPUREG a){return x86(455,a);}
int fsubp(ST0 a,ST0 b){return x86(456,a,b);}
int fsubp(FPUREG a,ST0 b){return x86(456,a,b);}
int fsubrp(){return x86(457);}
int fsubrp(ST0 a){return x86(458,a);}
int fsubrp(FPUREG a){return x86(458,a);}
int fsubrp(ST0 a,ST0 b){return x86(459,a,b);}
int fsubrp(FPUREG a,ST0 b){return x86(459,a,b);}
int ftst(){return x86(460);}
int fucom(ST0 a){return x86(461,a);}
int fucom(FPUREG a){return x86(461,a);}
int fucom(ST0 a,ST0 b){return x86(462,a,b);}
int fucom(ST0 a,FPUREG b){return x86(462,a,b);}
int fucomp(ST0 a){return x86(463,a);}
int fucomp(FPUREG a){return x86(463,a);}
int fucomp(ST0 a,ST0 b){return x86(464,a,b);}
int fucomp(ST0 a,FPUREG b){return x86(464,a,b);}
int fucompp(){return x86(465);}
int fucomi(ST0 a){return x86(466,a);}
int fucomi(FPUREG a){return x86(466,a);}
int fucomi(ST0 a,ST0 b){return x86(467,a,b);}
int fucomi(ST0 a,FPUREG b){return x86(467,a,b);}
int fucomip(ST0 a){return x86(468,a);}
int fucomip(FPUREG a){return x86(468,a);}
int fucomip(ST0 a,ST0 b){return x86(469,a,b);}
int fucomip(ST0 a,FPUREG b){return x86(469,a,b);}
int fwait(){return x86(470);}
int fxam(){return x86(471);}
int fxch(){return x86(472);}
int fxch(ST0 a){return x86(473,a);}
int fxch(FPUREG a){return x86(473,a);}
int fxch(ST0 a,ST0 b){return x86(474,a,b);}
int fxch(FPUREG a,ST0 b){return x86(474,a,b);}
int fxch(ST0 a,FPUREG b){return x86(475,a,b);}
int fxtract(){return x86(476);}
int fyl2x(){return x86(477);}
int fyl2xp1(){return x86(478);}
int hlt(){return x86(479);}
int idiv(AL a){return x86(480,a);}
int idiv(CL a){return x86(480,a);}
int idiv(REG8 a){return x86(480,a);}
int idiv(MEM8 a){return x86(480,a);}
int idiv(R_M8 a){return x86(480,a);}
int idiv(AX a){return x86(481,a);}
int idiv(DX a){return x86(481,a);}
int idiv(CX a){return x86(481,a);}
int idiv(REG16 a){return x86(481,a);}
int idiv(MEM16 a){return x86(481,a);}
int idiv(R_M16 a){return x86(481,a);}
int idiv(EAX a){return x86(482,a);}
int idiv(ECX a){return x86(482,a);}
int idiv(REG32 a){return x86(482,a);}
int idiv(MEM32 a){return x86(482,a);}
int idiv(R_M32 a){return x86(482,a);}
int imul(AL a){return x86(483,a);}
int imul(CL a){return x86(483,a);}
int imul(REG8 a){return x86(483,a);}
int imul(MEM8 a){return x86(483,a);}
int imul(R_M8 a){return x86(483,a);}
int imul(AX a){return x86(484,a);}
int imul(DX a){return x86(484,a);}
int imul(CX a){return x86(484,a);}
int imul(REG16 a){return x86(484,a);}
int imul(MEM16 a){return x86(484,a);}
int imul(R_M16 a){return x86(484,a);}
int imul(EAX a){return x86(485,a);}
int imul(ECX a){return x86(485,a);}
int imul(REG32 a){return x86(485,a);}
int imul(MEM32 a){return x86(485,a);}
int imul(R_M32 a){return x86(485,a);}
int imul(AX a,AX b){return x86(486,a,b);}
int imul(AX a,DX b){return x86(486,a,b);}
int imul(AX a,CX b){return x86(486,a,b);}
int imul(AX a,REG16 b){return x86(486,a,b);}
int imul(AX a,MEM16 b){return x86(486,a,b);}
int imul(AX a,R_M16 b){return x86(486,a,b);}
int imul(DX a,AX b){return x86(486,a,b);}
int imul(DX a,DX b){return x86(486,a,b);}
int imul(DX a,CX b){return x86(486,a,b);}
int imul(DX a,REG16 b){return x86(486,a,b);}
int imul(DX a,MEM16 b){return x86(486,a,b);}
int imul(DX a,R_M16 b){return x86(486,a,b);}
int imul(CX a,AX b){return x86(486,a,b);}
int imul(CX a,DX b){return x86(486,a,b);}
int imul(CX a,CX b){return x86(486,a,b);}
int imul(CX a,REG16 b){return x86(486,a,b);}
int imul(CX a,MEM16 b){return x86(486,a,b);}
int imul(CX a,R_M16 b){return x86(486,a,b);}
int imul(REG16 a,AX b){return x86(486,a,b);}
int imul(REG16 a,DX b){return x86(486,a,b);}
int imul(REG16 a,CX b){return x86(486,a,b);}
int imul(REG16 a,REG16 b){return x86(486,a,b);}
int imul(REG16 a,MEM16 b){return x86(486,a,b);}
int imul(REG16 a,R_M16 b){return x86(486,a,b);}
int imul(EAX a,EAX b){return x86(487,a,b);}
int imul(EAX a,ECX b){return x86(487,a,b);}
int imul(EAX a,REG32 b){return x86(487,a,b);}
int imul(EAX a,MEM32 b){return x86(487,a,b);}
int imul(EAX a,R_M32 b){return x86(487,a,b);}
int imul(ECX a,EAX b){return x86(487,a,b);}
int imul(ECX a,ECX b){return x86(487,a,b);}
int imul(ECX a,REG32 b){return x86(487,a,b);}
int imul(ECX a,MEM32 b){return x86(487,a,b);}
int imul(ECX a,R_M32 b){return x86(487,a,b);}
int imul(REG32 a,EAX b){return x86(487,a,b);}
int imul(REG32 a,ECX b){return x86(487,a,b);}
int imul(REG32 a,REG32 b){return x86(487,a,b);}
int imul(REG32 a,MEM32 b){return x86(487,a,b);}
int imul(REG32 a,R_M32 b){return x86(487,a,b);}
int imul(AX a,char b){return x86(489,a,(IMM)b);}
int imul(AX a,short b){return x86(489,a,(IMM)b);}
int imul(DX a,char b){return x86(489,a,(IMM)b);}
int imul(DX a,short b){return x86(489,a,(IMM)b);}
int imul(CX a,char b){return x86(489,a,(IMM)b);}
int imul(CX a,short b){return x86(489,a,(IMM)b);}
int imul(REG16 a,char b){return x86(489,a,(IMM)b);}
int imul(REG16 a,short b){return x86(489,a,(IMM)b);}
int imul(EAX a,int b){return x86(491,a,(IMM)b);}
int imul(EAX a,char b){return x86(491,a,(IMM)b);}
int imul(EAX a,short b){return x86(491,a,(IMM)b);}
int imul(EAX a,REF b){return x86(491,a,b);}
int imul(ECX a,int b){return x86(491,a,(IMM)b);}
int imul(ECX a,char b){return x86(491,a,(IMM)b);}
int imul(ECX a,short b){return x86(491,a,(IMM)b);}
int imul(ECX a,REF b){return x86(491,a,b);}
int imul(REG32 a,int b){return x86(491,a,(IMM)b);}
int imul(REG32 a,char b){return x86(491,a,(IMM)b);}
int imul(REG32 a,short b){return x86(491,a,(IMM)b);}
int imul(REG32 a,REF b){return x86(491,a,b);}
int imul(AX a,AX b,char c){return x86(492,a,b,(IMM)c);}
int imul(AX a,DX b,char c){return x86(492,a,b,(IMM)c);}
int imul(AX a,CX b,char c){return x86(492,a,b,(IMM)c);}
int imul(AX a,REG16 b,char c){return x86(492,a,b,(IMM)c);}
int imul(AX a,MEM16 b,char c){return x86(492,a,b,(IMM)c);}
int imul(AX a,R_M16 b,char c){return x86(492,a,b,(IMM)c);}
int imul(DX a,AX b,char c){return x86(492,a,b,(IMM)c);}
int imul(DX a,DX b,char c){return x86(492,a,b,(IMM)c);}
int imul(DX a,CX b,char c){return x86(492,a,b,(IMM)c);}
int imul(DX a,REG16 b,char c){return x86(492,a,b,(IMM)c);}
int imul(DX a,MEM16 b,char c){return x86(492,a,b,(IMM)c);}
int imul(DX a,R_M16 b,char c){return x86(492,a,b,(IMM)c);}
int imul(CX a,AX b,char c){return x86(492,a,b,(IMM)c);}
int imul(CX a,DX b,char c){return x86(492,a,b,(IMM)c);}
int imul(CX a,CX b,char c){return x86(492,a,b,(IMM)c);}
int imul(CX a,REG16 b,char c){return x86(492,a,b,(IMM)c);}
int imul(CX a,MEM16 b,char c){return x86(492,a,b,(IMM)c);}
int imul(CX a,R_M16 b,char c){return x86(492,a,b,(IMM)c);}
int imul(REG16 a,AX b,char c){return x86(492,a,b,(IMM)c);}
int imul(REG16 a,DX b,char c){return x86(492,a,b,(IMM)c);}
int imul(REG16 a,CX b,char c){return x86(492,a,b,(IMM)c);}
int imul(REG16 a,REG16 b,char c){return x86(492,a,b,(IMM)c);}
int imul(REG16 a,MEM16 b,char c){return x86(492,a,b,(IMM)c);}
int imul(REG16 a,R_M16 b,char c){return x86(492,a,b,(IMM)c);}
int imul(AX a,AX b,short c){return x86(493,a,b,(IMM)c);}
int imul(AX a,DX b,short c){return x86(493,a,b,(IMM)c);}
int imul(AX a,CX b,short c){return x86(493,a,b,(IMM)c);}
int imul(AX a,REG16 b,short c){return x86(493,a,b,(IMM)c);}
int imul(AX a,MEM16 b,short c){return x86(493,a,b,(IMM)c);}
int imul(AX a,R_M16 b,short c){return x86(493,a,b,(IMM)c);}
int imul(DX a,AX b,short c){return x86(493,a,b,(IMM)c);}
int imul(DX a,DX b,short c){return x86(493,a,b,(IMM)c);}
int imul(DX a,CX b,short c){return x86(493,a,b,(IMM)c);}
int imul(DX a,REG16 b,short c){return x86(493,a,b,(IMM)c);}
int imul(DX a,MEM16 b,short c){return x86(493,a,b,(IMM)c);}
int imul(DX a,R_M16 b,short c){return x86(493,a,b,(IMM)c);}
int imul(CX a,AX b,short c){return x86(493,a,b,(IMM)c);}
int imul(CX a,DX b,short c){return x86(493,a,b,(IMM)c);}
int imul(CX a,CX b,short c){return x86(493,a,b,(IMM)c);}
int imul(CX a,REG16 b,short c){return x86(493,a,b,(IMM)c);}
int imul(CX a,MEM16 b,short c){return x86(493,a,b,(IMM)c);}
int imul(CX a,R_M16 b,short c){return x86(493,a,b,(IMM)c);}
int imul(REG16 a,AX b,short c){return x86(493,a,b,(IMM)c);}
int imul(REG16 a,DX b,short c){return x86(493,a,b,(IMM)c);}
int imul(REG16 a,CX b,short c){return x86(493,a,b,(IMM)c);}
int imul(REG16 a,REG16 b,short c){return x86(493,a,b,(IMM)c);}
int imul(REG16 a,MEM16 b,short c){return x86(493,a,b,(IMM)c);}
int imul(REG16 a,R_M16 b,short c){return x86(493,a,b,(IMM)c);}
int imul(EAX a,EAX b,char c){return x86(494,a,b,(IMM)c);}
int imul(EAX a,ECX b,char c){return x86(494,a,b,(IMM)c);}
int imul(EAX a,REG32 b,char c){return x86(494,a,b,(IMM)c);}
int imul(EAX a,MEM32 b,char c){return x86(494,a,b,(IMM)c);}
int imul(EAX a,R_M32 b,char c){return x86(494,a,b,(IMM)c);}
int imul(ECX a,EAX b,char c){return x86(494,a,b,(IMM)c);}
int imul(ECX a,ECX b,char c){return x86(494,a,b,(IMM)c);}
int imul(ECX a,REG32 b,char c){return x86(494,a,b,(IMM)c);}
int imul(ECX a,MEM32 b,char c){return x86(494,a,b,(IMM)c);}
int imul(ECX a,R_M32 b,char c){return x86(494,a,b,(IMM)c);}
int imul(REG32 a,EAX b,char c){return x86(494,a,b,(IMM)c);}
int imul(REG32 a,ECX b,char c){return x86(494,a,b,(IMM)c);}
int imul(REG32 a,REG32 b,char c){return x86(494,a,b,(IMM)c);}
int imul(REG32 a,MEM32 b,char c){return x86(494,a,b,(IMM)c);}
int imul(REG32 a,R_M32 b,char c){return x86(494,a,b,(IMM)c);}
int imul(EAX a,EAX b,int c){return x86(495,a,b,(IMM)c);}
int imul(EAX a,EAX b,short c){return x86(495,a,b,(IMM)c);}
int imul(EAX a,EAX b,REF c){return x86(495,a,b,c);}
int imul(EAX a,ECX b,int c){return x86(495,a,b,(IMM)c);}
int imul(EAX a,ECX b,short c){return x86(495,a,b,(IMM)c);}
int imul(EAX a,ECX b,REF c){return x86(495,a,b,c);}
int imul(EAX a,REG32 b,int c){return x86(495,a,b,(IMM)c);}
int imul(EAX a,REG32 b,short c){return x86(495,a,b,(IMM)c);}
int imul(EAX a,REG32 b,REF c){return x86(495,a,b,c);}
int imul(EAX a,MEM32 b,int c){return x86(495,a,b,(IMM)c);}
int imul(EAX a,MEM32 b,short c){return x86(495,a,b,(IMM)c);}
int imul(EAX a,MEM32 b,REF c){return x86(495,a,b,c);}
int imul(EAX a,R_M32 b,int c){return x86(495,a,b,(IMM)c);}
int imul(EAX a,R_M32 b,short c){return x86(495,a,b,(IMM)c);}
int imul(EAX a,R_M32 b,REF c){return x86(495,a,b,c);}
int imul(ECX a,EAX b,int c){return x86(495,a,b,(IMM)c);}
int imul(ECX a,EAX b,short c){return x86(495,a,b,(IMM)c);}
int imul(ECX a,EAX b,REF c){return x86(495,a,b,c);}
int imul(ECX a,ECX b,int c){return x86(495,a,b,(IMM)c);}
int imul(ECX a,ECX b,short c){return x86(495,a,b,(IMM)c);}
int imul(ECX a,ECX b,REF c){return x86(495,a,b,c);}
int imul(ECX a,REG32 b,int c){return x86(495,a,b,(IMM)c);}
int imul(ECX a,REG32 b,short c){return x86(495,a,b,(IMM)c);}
int imul(ECX a,REG32 b,REF c){return x86(495,a,b,c);}
int imul(ECX a,MEM32 b,int c){return x86(495,a,b,(IMM)c);}
int imul(ECX a,MEM32 b,short c){return x86(495,a,b,(IMM)c);}
int imul(ECX a,MEM32 b,REF c){return x86(495,a,b,c);}
int imul(ECX a,R_M32 b,int c){return x86(495,a,b,(IMM)c);}
int imul(ECX a,R_M32 b,short c){return x86(495,a,b,(IMM)c);}
int imul(ECX a,R_M32 b,REF c){return x86(495,a,b,c);}
int imul(REG32 a,EAX b,int c){return x86(495,a,b,(IMM)c);}
int imul(REG32 a,EAX b,short c){return x86(495,a,b,(IMM)c);}
int imul(REG32 a,EAX b,REF c){return x86(495,a,b,c);}
int imul(REG32 a,ECX b,int c){return x86(495,a,b,(IMM)c);}
int imul(REG32 a,ECX b,short c){return x86(495,a,b,(IMM)c);}
int imul(REG32 a,ECX b,REF c){return x86(495,a,b,c);}
int imul(REG32 a,REG32 b,int c){return x86(495,a,b,(IMM)c);}
int imul(REG32 a,REG32 b,short c){return x86(495,a,b,(IMM)c);}
int imul(REG32 a,REG32 b,REF c){return x86(495,a,b,c);}
int imul(REG32 a,MEM32 b,int c){return x86(495,a,b,(IMM)c);}
int imul(REG32 a,MEM32 b,short c){return x86(495,a,b,(IMM)c);}
int imul(REG32 a,MEM32 b,REF c){return x86(495,a,b,c);}
int imul(REG32 a,R_M32 b,int c){return x86(495,a,b,(IMM)c);}
int imul(REG32 a,R_M32 b,short c){return x86(495,a,b,(IMM)c);}
int imul(REG32 a,R_M32 b,REF c){return x86(495,a,b,c);}
int in(AL a,char b){return x86(496,a,(IMM)b);}
int in(AL a,DX b){return x86(499,a,b);}
int in(AX a,DX b){return x86(500,a,b);}
int in(EAX a,DX b){return x86(501,a,b);}
int inc(AX a){return x86(502,a);}
int inc(DX a){return x86(502,a);}
int inc(CX a){return x86(502,a);}
int inc(REG16 a){return x86(502,a);}
int inc(EAX a){return x86(503,a);}
int inc(ECX a){return x86(503,a);}
int inc(REG32 a){return x86(503,a);}
int inc(AL a){return x86(504,a);}
int inc(CL a){return x86(504,a);}
int inc(REG8 a){return x86(504,a);}
int inc(MEM8 a){return x86(504,a);}
int inc(R_M8 a){return x86(504,a);}
int inc(MEM16 a){return x86(505,a);}
int inc(R_M16 a){return x86(505,a);}
int inc(MEM32 a){return x86(506,a);}
int inc(R_M32 a){return x86(506,a);}
int lock_inc(MEM8 a){return x86(507,a);}
int lock_inc(MEM16 a){return x86(508,a);}
int lock_inc(MEM32 a){return x86(509,a);}
int insb(){return x86(510);}
int insw(){return x86(511);}
int insd(){return x86(512);}
int rep_insb(){return x86(513);}
int rep_insw(){return x86(514);}
int rep_insd(){return x86(515);}
int int3(){return x86(516);}
int int03(){return x86(517);}
int into(){return x86(518);}
int jcxz(char a){return x86(519,(IMM)a);}
int jecxz(char a){return x86(520,(IMM)a);}
int jmp(int a){return x86(521,(IMM)a);}
int jmp(char a){return x86(521,(IMM)a);}
int jmp(short a){return x86(521,(IMM)a);}
int jmp(REF a){return x86(521,a);}
int jmp(MEM8 a){return x86(523,a);}
int jmp(MEM16 a){return x86(523,a);}
int jmp(MEM32 a){return x86(523,a);}
int jmp(MEM64 a){return x86(523,a);}
int jmp(MEM128 a){return x86(523,a);}
int jmp(AX a){return x86(524,a);}
int jmp(DX a){return x86(524,a);}
int jmp(CX a){return x86(524,a);}
int jmp(REG16 a){return x86(524,a);}
int jmp(R_M16 a){return x86(524,a);}
int jmp(EAX a){return x86(525,a);}
int jmp(ECX a){return x86(525,a);}
int jmp(REG32 a){return x86(525,a);}
int jmp(R_M32 a){return x86(525,a);}
int jo(char a){return x86(526,(IMM)a);}
int jno(char a){return x86(527,(IMM)a);}
int jb(char a){return x86(528,(IMM)a);}
int jc(char a){return x86(529,(IMM)a);}
int jnae(char a){return x86(530,(IMM)a);}
int jae(char a){return x86(531,(IMM)a);}
int jnb(char a){return x86(532,(IMM)a);}
int jnc(char a){return x86(533,(IMM)a);}
int je(char a){return x86(534,(IMM)a);}
int jz(char a){return x86(535,(IMM)a);}
int jne(char a){return x86(536,(IMM)a);}
int jnz(char a){return x86(537,(IMM)a);}
int jbe(char a){return x86(538,(IMM)a);}
int jna(char a){return x86(539,(IMM)a);}
int ja(char a){return x86(540,(IMM)a);}
int jnbe(char a){return x86(541,(IMM)a);}
int js(char a){return x86(542,(IMM)a);}
int jns(char a){return x86(543,(IMM)a);}
int jp(char a){return x86(544,(IMM)a);}
int jpe(char a){return x86(545,(IMM)a);}
int jnp(char a){return x86(546,(IMM)a);}
int jpo(char a){return x86(547,(IMM)a);}
int jl(char a){return x86(548,(IMM)a);}
int jnge(char a){return x86(549,(IMM)a);}
int jge(char a){return x86(550,(IMM)a);}
int jnl(char a){return x86(551,(IMM)a);}
int jle(char a){return x86(552,(IMM)a);}
int jng(char a){return x86(553,(IMM)a);}
int jg(char a){return x86(554,(IMM)a);}
int jnle(char a){return x86(555,(IMM)a);}
int jo(int a){return x86(556,(IMM)a);}
int jo(short a){return x86(556,(IMM)a);}
int jo(REF a){return x86(556,a);}
int jno(int a){return x86(557,(IMM)a);}
int jno(short a){return x86(557,(IMM)a);}
int jno(REF a){return x86(557,a);}
int jb(int a){return x86(558,(IMM)a);}
int jb(short a){return x86(558,(IMM)a);}
int jb(REF a){return x86(558,a);}
int jc(int a){return x86(559,(IMM)a);}
int jc(short a){return x86(559,(IMM)a);}
int jc(REF a){return x86(559,a);}
int jnae(int a){return x86(560,(IMM)a);}
int jnae(short a){return x86(560,(IMM)a);}
int jnae(REF a){return x86(560,a);}
int jae(int a){return x86(561,(IMM)a);}
int jae(short a){return x86(561,(IMM)a);}
int jae(REF a){return x86(561,a);}
int jnb(int a){return x86(562,(IMM)a);}
int jnb(short a){return x86(562,(IMM)a);}
int jnb(REF a){return x86(562,a);}
int jnc(int a){return x86(563,(IMM)a);}
int jnc(short a){return x86(563,(IMM)a);}
int jnc(REF a){return x86(563,a);}
int je(int a){return x86(564,(IMM)a);}
int je(short a){return x86(564,(IMM)a);}
int je(REF a){return x86(564,a);}
int jz(int a){return x86(565,(IMM)a);}
int jz(short a){return x86(565,(IMM)a);}
int jz(REF a){return x86(565,a);}
int jne(int a){return x86(566,(IMM)a);}
int jne(short a){return x86(566,(IMM)a);}
int jne(REF a){return x86(566,a);}
int jnz(int a){return x86(567,(IMM)a);}
int jnz(short a){return x86(567,(IMM)a);}
int jnz(REF a){return x86(567,a);}
int jbe(int a){return x86(568,(IMM)a);}
int jbe(short a){return x86(568,(IMM)a);}
int jbe(REF a){return x86(568,a);}
int jna(int a){return x86(569,(IMM)a);}
int jna(short a){return x86(569,(IMM)a);}
int jna(REF a){return x86(569,a);}
int ja(int a){return x86(570,(IMM)a);}
int ja(short a){return x86(570,(IMM)a);}
int ja(REF a){return x86(570,a);}
int jnbe(int a){return x86(571,(IMM)a);}
int jnbe(short a){return x86(571,(IMM)a);}
int jnbe(REF a){return x86(571,a);}
int js(int a){return x86(572,(IMM)a);}
int js(short a){return x86(572,(IMM)a);}
int js(REF a){return x86(572,a);}
int jns(int a){return x86(573,(IMM)a);}
int jns(short a){return x86(573,(IMM)a);}
int jns(REF a){return x86(573,a);}
int jp(int a){return x86(574,(IMM)a);}
int jp(short a){return x86(574,(IMM)a);}
int jp(REF a){return x86(574,a);}
int jpe(int a){return x86(575,(IMM)a);}
int jpe(short a){return x86(575,(IMM)a);}
int jpe(REF a){return x86(575,a);}
int jnp(int a){return x86(576,(IMM)a);}
int jnp(short a){return x86(576,(IMM)a);}
int jnp(REF a){return x86(576,a);}
int jpo(int a){return x86(577,(IMM)a);}
int jpo(short a){return x86(577,(IMM)a);}
int jpo(REF a){return x86(577,a);}
int jl(int a){return x86(578,(IMM)a);}
int jl(short a){return x86(578,(IMM)a);}
int jl(REF a){return x86(578,a);}
int jnge(int a){return x86(579,(IMM)a);}
int jnge(short a){return x86(579,(IMM)a);}
int jnge(REF a){return x86(579,a);}
int jge(int a){return x86(580,(IMM)a);}
int jge(short a){return x86(580,(IMM)a);}
int jge(REF a){return x86(580,a);}
int jnl(int a){return x86(581,(IMM)a);}
int jnl(short a){return x86(581,(IMM)a);}
int jnl(REF a){return x86(581,a);}
int jle(int a){return x86(582,(IMM)a);}
int jle(short a){return x86(582,(IMM)a);}
int jle(REF a){return x86(582,a);}
int jng(int a){return x86(583,(IMM)a);}
int jng(short a){return x86(583,(IMM)a);}
int jng(REF a){return x86(583,a);}
int jg(int a){return x86(584,(IMM)a);}
int jg(short a){return x86(584,(IMM)a);}
int jg(REF a){return x86(584,a);}
int jnle(int a){return x86(585,(IMM)a);}
int jnle(short a){return x86(585,(IMM)a);}
int jnle(REF a){return x86(585,a);}
int lahf(){return x86(586);}
int lds(AX a,MEM8 b){return x86(587,a,b);}
int lds(AX a,MEM16 b){return x86(587,a,b);}
int lds(AX a,MEM32 b){return x86(587,a,b);}
int lds(AX a,MEM64 b){return x86(587,a,b);}
int lds(AX a,MEM128 b){return x86(587,a,b);}
int lds(DX a,MEM8 b){return x86(587,a,b);}
int lds(DX a,MEM16 b){return x86(587,a,b);}
int lds(DX a,MEM32 b){return x86(587,a,b);}
int lds(DX a,MEM64 b){return x86(587,a,b);}
int lds(DX a,MEM128 b){return x86(587,a,b);}
int lds(CX a,MEM8 b){return x86(587,a,b);}
int lds(CX a,MEM16 b){return x86(587,a,b);}
int lds(CX a,MEM32 b){return x86(587,a,b);}
int lds(CX a,MEM64 b){return x86(587,a,b);}
int lds(CX a,MEM128 b){return x86(587,a,b);}
int lds(REG16 a,MEM8 b){return x86(587,a,b);}
int lds(REG16 a,MEM16 b){return x86(587,a,b);}
int lds(REG16 a,MEM32 b){return x86(587,a,b);}
int lds(REG16 a,MEM64 b){return x86(587,a,b);}
int lds(REG16 a,MEM128 b){return x86(587,a,b);}
int lds(EAX a,MEM8 b){return x86(588,a,b);}
int lds(EAX a,MEM16 b){return x86(588,a,b);}
int lds(EAX a,MEM32 b){return x86(588,a,b);}
int lds(EAX a,MEM64 b){return x86(588,a,b);}
int lds(EAX a,MEM128 b){return x86(588,a,b);}
int lds(ECX a,MEM8 b){return x86(588,a,b);}
int lds(ECX a,MEM16 b){return x86(588,a,b);}
int lds(ECX a,MEM32 b){return x86(588,a,b);}
int lds(ECX a,MEM64 b){return x86(588,a,b);}
int lds(ECX a,MEM128 b){return x86(588,a,b);}
int lds(REG32 a,MEM8 b){return x86(588,a,b);}
int lds(REG32 a,MEM16 b){return x86(588,a,b);}
int lds(REG32 a,MEM32 b){return x86(588,a,b);}
int lds(REG32 a,MEM64 b){return x86(588,a,b);}
int lds(REG32 a,MEM128 b){return x86(588,a,b);}
int les(AX a,MEM8 b){return x86(589,a,b);}
int les(AX a,MEM16 b){return x86(589,a,b);}
int les(AX a,MEM32 b){return x86(589,a,b);}
int les(AX a,MEM64 b){return x86(589,a,b);}
int les(AX a,MEM128 b){return x86(589,a,b);}
int les(DX a,MEM8 b){return x86(589,a,b);}
int les(DX a,MEM16 b){return x86(589,a,b);}
int les(DX a,MEM32 b){return x86(589,a,b);}
int les(DX a,MEM64 b){return x86(589,a,b);}
int les(DX a,MEM128 b){return x86(589,a,b);}
int les(CX a,MEM8 b){return x86(589,a,b);}
int les(CX a,MEM16 b){return x86(589,a,b);}
int les(CX a,MEM32 b){return x86(589,a,b);}
int les(CX a,MEM64 b){return x86(589,a,b);}
int les(CX a,MEM128 b){return x86(589,a,b);}
int les(REG16 a,MEM8 b){return x86(589,a,b);}
int les(REG16 a,MEM16 b){return x86(589,a,b);}
int les(REG16 a,MEM32 b){return x86(589,a,b);}
int les(REG16 a,MEM64 b){return x86(589,a,b);}
int les(REG16 a,MEM128 b){return x86(589,a,b);}
int les(EAX a,MEM8 b){return x86(590,a,b);}
int les(EAX a,MEM16 b){return x86(590,a,b);}
int les(EAX a,MEM32 b){return x86(590,a,b);}
int les(EAX a,MEM64 b){return x86(590,a,b);}
int les(EAX a,MEM128 b){return x86(590,a,b);}
int les(ECX a,MEM8 b){return x86(590,a,b);}
int les(ECX a,MEM16 b){return x86(590,a,b);}
int les(ECX a,MEM32 b){return x86(590,a,b);}
int les(ECX a,MEM64 b){return x86(590,a,b);}
int les(ECX a,MEM128 b){return x86(590,a,b);}
int les(REG32 a,MEM8 b){return x86(590,a,b);}
int les(REG32 a,MEM16 b){return x86(590,a,b);}
int les(REG32 a,MEM32 b){return x86(590,a,b);}
int les(REG32 a,MEM64 b){return x86(590,a,b);}
int les(REG32 a,MEM128 b){return x86(590,a,b);}
int lfs(AX a,MEM8 b){return x86(591,a,b);}
int lfs(AX a,MEM16 b){return x86(591,a,b);}
int lfs(AX a,MEM32 b){return x86(591,a,b);}
int lfs(AX a,MEM64 b){return x86(591,a,b);}
int lfs(AX a,MEM128 b){return x86(591,a,b);}
int lfs(DX a,MEM8 b){return x86(591,a,b);}
int lfs(DX a,MEM16 b){return x86(591,a,b);}
int lfs(DX a,MEM32 b){return x86(591,a,b);}
int lfs(DX a,MEM64 b){return x86(591,a,b);}
int lfs(DX a,MEM128 b){return x86(591,a,b);}
int lfs(CX a,MEM8 b){return x86(591,a,b);}
int lfs(CX a,MEM16 b){return x86(591,a,b);}
int lfs(CX a,MEM32 b){return x86(591,a,b);}
int lfs(CX a,MEM64 b){return x86(591,a,b);}
int lfs(CX a,MEM128 b){return x86(591,a,b);}
int lfs(REG16 a,MEM8 b){return x86(591,a,b);}
int lfs(REG16 a,MEM16 b){return x86(591,a,b);}
int lfs(REG16 a,MEM32 b){return x86(591,a,b);}
int lfs(REG16 a,MEM64 b){return x86(591,a,b);}
int lfs(REG16 a,MEM128 b){return x86(591,a,b);}
int lfs(EAX a,MEM8 b){return x86(592,a,b);}
int lfs(EAX a,MEM16 b){return x86(592,a,b);}
int lfs(EAX a,MEM32 b){return x86(592,a,b);}
int lfs(EAX a,MEM64 b){return x86(592,a,b);}
int lfs(EAX a,MEM128 b){return x86(592,a,b);}
int lfs(ECX a,MEM8 b){return x86(592,a,b);}
int lfs(ECX a,MEM16 b){return x86(592,a,b);}
int lfs(ECX a,MEM32 b){return x86(592,a,b);}
int lfs(ECX a,MEM64 b){return x86(592,a,b);}
int lfs(ECX a,MEM128 b){return x86(592,a,b);}
int lfs(REG32 a,MEM8 b){return x86(592,a,b);}
int lfs(REG32 a,MEM16 b){return x86(592,a,b);}
int lfs(REG32 a,MEM32 b){return x86(592,a,b);}
int lfs(REG32 a,MEM64 b){return x86(592,a,b);}
int lfs(REG32 a,MEM128 b){return x86(592,a,b);}
int lgs(AX a,MEM8 b){return x86(593,a,b);}
int lgs(AX a,MEM16 b){return x86(593,a,b);}
int lgs(AX a,MEM32 b){return x86(593,a,b);}
int lgs(AX a,MEM64 b){return x86(593,a,b);}
int lgs(AX a,MEM128 b){return x86(593,a,b);}
int lgs(DX a,MEM8 b){return x86(593,a,b);}
int lgs(DX a,MEM16 b){return x86(593,a,b);}
int lgs(DX a,MEM32 b){return x86(593,a,b);}
int lgs(DX a,MEM64 b){return x86(593,a,b);}
int lgs(DX a,MEM128 b){return x86(593,a,b);}
int lgs(CX a,MEM8 b){return x86(593,a,b);}
int lgs(CX a,MEM16 b){return x86(593,a,b);}
int lgs(CX a,MEM32 b){return x86(593,a,b);}
int lgs(CX a,MEM64 b){return x86(593,a,b);}
int lgs(CX a,MEM128 b){return x86(593,a,b);}
int lgs(REG16 a,MEM8 b){return x86(593,a,b);}
int lgs(REG16 a,MEM16 b){return x86(593,a,b);}
int lgs(REG16 a,MEM32 b){return x86(593,a,b);}
int lgs(REG16 a,MEM64 b){return x86(593,a,b);}
int lgs(REG16 a,MEM128 b){return x86(593,a,b);}
int lgs(EAX a,MEM8 b){return x86(594,a,b);}
int lgs(EAX a,MEM16 b){return x86(594,a,b);}
int lgs(EAX a,MEM32 b){return x86(594,a,b);}
int lgs(EAX a,MEM64 b){return x86(594,a,b);}
int lgs(EAX a,MEM128 b){return x86(594,a,b);}
int lgs(ECX a,MEM8 b){return x86(594,a,b);}
int lgs(ECX a,MEM16 b){return x86(594,a,b);}
int lgs(ECX a,MEM32 b){return x86(594,a,b);}
int lgs(ECX a,MEM64 b){return x86(594,a,b);}
int lgs(ECX a,MEM128 b){return x86(594,a,b);}
int lgs(REG32 a,MEM8 b){return x86(594,a,b);}
int lgs(REG32 a,MEM16 b){return x86(594,a,b);}
int lgs(REG32 a,MEM32 b){return x86(594,a,b);}
int lgs(REG32 a,MEM64 b){return x86(594,a,b);}
int lgs(REG32 a,MEM128 b){return x86(594,a,b);}
int lss(AX a,MEM8 b){return x86(595,a,b);}
int lss(AX a,MEM16 b){return x86(595,a,b);}
int lss(AX a,MEM32 b){return x86(595,a,b);}
int lss(AX a,MEM64 b){return x86(595,a,b);}
int lss(AX a,MEM128 b){return x86(595,a,b);}
int lss(DX a,MEM8 b){return x86(595,a,b);}
int lss(DX a,MEM16 b){return x86(595,a,b);}
int lss(DX a,MEM32 b){return x86(595,a,b);}
int lss(DX a,MEM64 b){return x86(595,a,b);}
int lss(DX a,MEM128 b){return x86(595,a,b);}
int lss(CX a,MEM8 b){return x86(595,a,b);}
int lss(CX a,MEM16 b){return x86(595,a,b);}
int lss(CX a,MEM32 b){return x86(595,a,b);}
int lss(CX a,MEM64 b){return x86(595,a,b);}
int lss(CX a,MEM128 b){return x86(595,a,b);}
int lss(REG16 a,MEM8 b){return x86(595,a,b);}
int lss(REG16 a,MEM16 b){return x86(595,a,b);}
int lss(REG16 a,MEM32 b){return x86(595,a,b);}
int lss(REG16 a,MEM64 b){return x86(595,a,b);}
int lss(REG16 a,MEM128 b){return x86(595,a,b);}
int lss(EAX a,MEM8 b){return x86(596,a,b);}
int lss(EAX a,MEM16 b){return x86(596,a,b);}
int lss(EAX a,MEM32 b){return x86(596,a,b);}
int lss(EAX a,MEM64 b){return x86(596,a,b);}
int lss(EAX a,MEM128 b){return x86(596,a,b);}
int lss(ECX a,MEM8 b){return x86(596,a,b);}
int lss(ECX a,MEM16 b){return x86(596,a,b);}
int lss(ECX a,MEM32 b){return x86(596,a,b);}
int lss(ECX a,MEM64 b){return x86(596,a,b);}
int lss(ECX a,MEM128 b){return x86(596,a,b);}
int lss(REG32 a,MEM8 b){return x86(596,a,b);}
int lss(REG32 a,MEM16 b){return x86(596,a,b);}
int lss(REG32 a,MEM32 b){return x86(596,a,b);}
int lss(REG32 a,MEM64 b){return x86(596,a,b);}
int lss(REG32 a,MEM128 b){return x86(596,a,b);}
int ldmxcsr(MEM32 a){return x86(597,a);}
int lea(AX a,MEM8 b){return x86(598,a,b);}
int lea(AX a,MEM16 b){return x86(598,a,b);}
int lea(AX a,MEM32 b){return x86(598,a,b);}
int lea(AX a,MEM64 b){return x86(598,a,b);}
int lea(AX a,MEM128 b){return x86(598,a,b);}
int lea(DX a,MEM8 b){return x86(598,a,b);}
int lea(DX a,MEM16 b){return x86(598,a,b);}
int lea(DX a,MEM32 b){return x86(598,a,b);}
int lea(DX a,MEM64 b){return x86(598,a,b);}
int lea(DX a,MEM128 b){return x86(598,a,b);}
int lea(CX a,MEM8 b){return x86(598,a,b);}
int lea(CX a,MEM16 b){return x86(598,a,b);}
int lea(CX a,MEM32 b){return x86(598,a,b);}
int lea(CX a,MEM64 b){return x86(598,a,b);}
int lea(CX a,MEM128 b){return x86(598,a,b);}
int lea(REG16 a,MEM8 b){return x86(598,a,b);}
int lea(REG16 a,MEM16 b){return x86(598,a,b);}
int lea(REG16 a,MEM32 b){return x86(598,a,b);}
int lea(REG16 a,MEM64 b){return x86(598,a,b);}
int lea(REG16 a,MEM128 b){return x86(598,a,b);}
int lea(EAX a,MEM8 b){return x86(599,a,b);}
int lea(EAX a,MEM16 b){return x86(599,a,b);}
int lea(EAX a,MEM32 b){return x86(599,a,b);}
int lea(EAX a,MEM64 b){return x86(599,a,b);}
int lea(EAX a,MEM128 b){return x86(599,a,b);}
int lea(ECX a,MEM8 b){return x86(599,a,b);}
int lea(ECX a,MEM16 b){return x86(599,a,b);}
int lea(ECX a,MEM32 b){return x86(599,a,b);}
int lea(ECX a,MEM64 b){return x86(599,a,b);}
int lea(ECX a,MEM128 b){return x86(599,a,b);}
int lea(REG32 a,MEM8 b){return x86(599,a,b);}
int lea(REG32 a,MEM16 b){return x86(599,a,b);}
int lea(REG32 a,MEM32 b){return x86(599,a,b);}
int lea(REG32 a,MEM64 b){return x86(599,a,b);}
int lea(REG32 a,MEM128 b){return x86(599,a,b);}
int leave(){return x86(600);}
int lfence(){return x86(601);}
int lodsb(){return x86(602);}
int lodsw(){return x86(603);}
int lodsd(){return x86(604);}
int rep_lodsb(){return x86(605);}
int rep_lodsw(){return x86(606);}
int rep_lodsd(){return x86(607);}
int loop(int a){return x86(608,(IMM)a);}
int loop(char a){return x86(608,(IMM)a);}
int loop(short a){return x86(608,(IMM)a);}
int loop(REF a){return x86(608,a);}
int loop(int a,CX b){return x86(609,(IMM)a,b);}
int loop(char a,CX b){return x86(609,(IMM)a,b);}
int loop(short a,CX b){return x86(609,(IMM)a,b);}
int loop(REF a,CX b){return x86(609,a,b);}
int loop(int a,ECX b){return x86(610,(IMM)a,b);}
int loop(char a,ECX b){return x86(610,(IMM)a,b);}
int loop(short a,ECX b){return x86(610,(IMM)a,b);}
int loop(REF a,ECX b){return x86(610,a,b);}
int loope(int a){return x86(611,(IMM)a);}
int loope(char a){return x86(611,(IMM)a);}
int loope(short a){return x86(611,(IMM)a);}
int loope(REF a){return x86(611,a);}
int loope(int a,CX b){return x86(612,(IMM)a,b);}
int loope(char a,CX b){return x86(612,(IMM)a,b);}
int loope(short a,CX b){return x86(612,(IMM)a,b);}
int loope(REF a,CX b){return x86(612,a,b);}
int loope(int a,ECX b){return x86(613,(IMM)a,b);}
int loope(char a,ECX b){return x86(613,(IMM)a,b);}
int loope(short a,ECX b){return x86(613,(IMM)a,b);}
int loope(REF a,ECX b){return x86(613,a,b);}
int loopz(int a){return x86(614,(IMM)a);}
int loopz(char a){return x86(614,(IMM)a);}
int loopz(short a){return x86(614,(IMM)a);}
int loopz(REF a){return x86(614,a);}
int loopz(int a,CX b){return x86(615,(IMM)a,b);}
int loopz(char a,CX b){return x86(615,(IMM)a,b);}
int loopz(short a,CX b){return x86(615,(IMM)a,b);}
int loopz(REF a,CX b){return x86(615,a,b);}
int loopz(int a,ECX b){return x86(616,(IMM)a,b);}
int loopz(char a,ECX b){return x86(616,(IMM)a,b);}
int loopz(short a,ECX b){return x86(616,(IMM)a,b);}
int loopz(REF a,ECX b){return x86(616,a,b);}
int loopne(int a){return x86(617,(IMM)a);}
int loopne(char a){return x86(617,(IMM)a);}
int loopne(short a){return x86(617,(IMM)a);}
int loopne(REF a){return x86(617,a);}
int loopne(int a,CX b){return x86(618,(IMM)a,b);}
int loopne(char a,CX b){return x86(618,(IMM)a,b);}
int loopne(short a,CX b){return x86(618,(IMM)a,b);}
int loopne(REF a,CX b){return x86(618,a,b);}
int loopne(int a,ECX b){return x86(619,(IMM)a,b);}
int loopne(char a,ECX b){return x86(619,(IMM)a,b);}
int loopne(short a,ECX b){return x86(619,(IMM)a,b);}
int loopne(REF a,ECX b){return x86(619,a,b);}
int loopnz(int a){return x86(620,(IMM)a);}
int loopnz(char a){return x86(620,(IMM)a);}
int loopnz(short a){return x86(620,(IMM)a);}
int loopnz(REF a){return x86(620,a);}
int loopnz(int a,CX b){return x86(621,(IMM)a,b);}
int loopnz(char a,CX b){return x86(621,(IMM)a,b);}
int loopnz(short a,CX b){return x86(621,(IMM)a,b);}
int loopnz(REF a,CX b){return x86(621,a,b);}
int loopnz(int a,ECX b){return x86(622,(IMM)a,b);}
int loopnz(char a,ECX b){return x86(622,(IMM)a,b);}
int loopnz(short a,ECX b){return x86(622,(IMM)a,b);}
int loopnz(REF a,ECX b){return x86(622,a,b);}
int maskmovdqu(XMMREG a,XMMREG b){return x86(623,a,b);}
int maskmovq(MMREG a,MMREG b){return x86(624,a,b);}
int maxpd(XMMREG a,XMMREG b){return x86(625,a,b);}
int maxpd(XMMREG a,MEM128 b){return x86(625,a,b);}
int maxpd(XMMREG a,R_M128 b){return x86(625,a,b);}
int maxps(XMMREG a,XMMREG b){return x86(626,a,b);}
int maxps(XMMREG a,MEM128 b){return x86(626,a,b);}
int maxps(XMMREG a,R_M128 b){return x86(626,a,b);}
int maxsd(XMMREG a,XMMREG b){return x86(627,a,b);}
int maxsd(XMMREG a,MEM64 b){return x86(627,a,b);}
int maxsd(XMMREG a,XMM64 b){return x86(627,a,b);}
int maxss(XMMREG a,XMMREG b){return x86(628,a,b);}
int maxss(XMMREG a,MEM128 b){return x86(628,a,b);}
int maxss(XMMREG a,R_M128 b){return x86(628,a,b);}
int mfence(){return x86(629);}
int minpd(XMMREG a,XMMREG b){return x86(630,a,b);}
int minpd(XMMREG a,MEM128 b){return x86(630,a,b);}
int minpd(XMMREG a,R_M128 b){return x86(630,a,b);}
int minps(XMMREG a,XMMREG b){return x86(631,a,b);}
int minps(XMMREG a,MEM128 b){return x86(631,a,b);}
int minps(XMMREG a,R_M128 b){return x86(631,a,b);}
int minsd(XMMREG a,XMMREG b){return x86(632,a,b);}
int minsd(XMMREG a,MEM64 b){return x86(632,a,b);}
int minsd(XMMREG a,XMM64 b){return x86(632,a,b);}
int minss(XMMREG a,XMMREG b){return x86(633,a,b);}
int minss(XMMREG a,MEM32 b){return x86(633,a,b);}
int minss(XMMREG a,XMM32 b){return x86(633,a,b);}
int mov(AL a,AL b){return x86(634,a,b);}
int mov(AL a,CL b){return x86(634,a,b);}
int mov(AL a,REG8 b){return x86(634,a,b);}
int mov(CL a,AL b){return x86(634,a,b);}
int mov(CL a,CL b){return x86(634,a,b);}
int mov(CL a,REG8 b){return x86(634,a,b);}
int mov(REG8 a,AL b){return x86(634,a,b);}
int mov(REG8 a,CL b){return x86(634,a,b);}
int mov(REG8 a,REG8 b){return x86(634,a,b);}
int mov(MEM8 a,AL b){return x86(634,a,b);}
int mov(MEM8 a,CL b){return x86(634,a,b);}
int mov(MEM8 a,REG8 b){return x86(634,a,b);}
int mov(R_M8 a,AL b){return x86(634,a,b);}
int mov(R_M8 a,CL b){return x86(634,a,b);}
int mov(R_M8 a,REG8 b){return x86(634,a,b);}
int mov(AX a,AX b){return x86(635,a,b);}
int mov(AX a,DX b){return x86(635,a,b);}
int mov(AX a,CX b){return x86(635,a,b);}
int mov(AX a,REG16 b){return x86(635,a,b);}
int mov(DX a,AX b){return x86(635,a,b);}
int mov(DX a,DX b){return x86(635,a,b);}
int mov(DX a,CX b){return x86(635,a,b);}
int mov(DX a,REG16 b){return x86(635,a,b);}
int mov(CX a,AX b){return x86(635,a,b);}
int mov(CX a,DX b){return x86(635,a,b);}
int mov(CX a,CX b){return x86(635,a,b);}
int mov(CX a,REG16 b){return x86(635,a,b);}
int mov(REG16 a,AX b){return x86(635,a,b);}
int mov(REG16 a,DX b){return x86(635,a,b);}
int mov(REG16 a,CX b){return x86(635,a,b);}
int mov(REG16 a,REG16 b){return x86(635,a,b);}
int mov(MEM16 a,AX b){return x86(635,a,b);}
int mov(MEM16 a,DX b){return x86(635,a,b);}
int mov(MEM16 a,CX b){return x86(635,a,b);}
int mov(MEM16 a,REG16 b){return x86(635,a,b);}
int mov(R_M16 a,AX b){return x86(635,a,b);}
int mov(R_M16 a,DX b){return x86(635,a,b);}
int mov(R_M16 a,CX b){return x86(635,a,b);}
int mov(R_M16 a,REG16 b){return x86(635,a,b);}
int mov(EAX a,EAX b){return x86(636,a,b);}
int mov(EAX a,ECX b){return x86(636,a,b);}
int mov(EAX a,REG32 b){return x86(636,a,b);}
int mov(ECX a,EAX b){return x86(636,a,b);}
int mov(ECX a,ECX b){return x86(636,a,b);}
int mov(ECX a,REG32 b){return x86(636,a,b);}
int mov(REG32 a,EAX b){return x86(636,a,b);}
int mov(REG32 a,ECX b){return x86(636,a,b);}
int mov(REG32 a,REG32 b){return x86(636,a,b);}
int mov(MEM32 a,EAX b){return x86(636,a,b);}
int mov(MEM32 a,ECX b){return x86(636,a,b);}
int mov(MEM32 a,REG32 b){return x86(636,a,b);}
int mov(R_M32 a,EAX b){return x86(636,a,b);}
int mov(R_M32 a,ECX b){return x86(636,a,b);}
int mov(R_M32 a,REG32 b){return x86(636,a,b);}
int mov(AL a,MEM8 b){return x86(637,a,b);}
int mov(AL a,R_M8 b){return x86(637,a,b);}
int mov(CL a,MEM8 b){return x86(637,a,b);}
int mov(CL a,R_M8 b){return x86(637,a,b);}
int mov(REG8 a,MEM8 b){return x86(637,a,b);}
int mov(REG8 a,R_M8 b){return x86(637,a,b);}
int mov(AX a,MEM16 b){return x86(638,a,b);}
int mov(AX a,R_M16 b){return x86(638,a,b);}
int mov(DX a,MEM16 b){return x86(638,a,b);}
int mov(DX a,R_M16 b){return x86(638,a,b);}
int mov(CX a,MEM16 b){return x86(638,a,b);}
int mov(CX a,R_M16 b){return x86(638,a,b);}
int mov(REG16 a,MEM16 b){return x86(638,a,b);}
int mov(REG16 a,R_M16 b){return x86(638,a,b);}
int mov(EAX a,MEM32 b){return x86(639,a,b);}
int mov(EAX a,R_M32 b){return x86(639,a,b);}
int mov(ECX a,MEM32 b){return x86(639,a,b);}
int mov(ECX a,R_M32 b){return x86(639,a,b);}
int mov(REG32 a,MEM32 b){return x86(639,a,b);}
int mov(REG32 a,R_M32 b){return x86(639,a,b);}
int mov(AL a,char b){return x86(640,a,(IMM)b);}
int mov(CL a,char b){return x86(640,a,(IMM)b);}
int mov(REG8 a,char b){return x86(640,a,(IMM)b);}
int mov(AX a,char b){return x86(641,a,(IMM)b);}
int mov(AX a,short b){return x86(641,a,(IMM)b);}
int mov(DX a,char b){return x86(641,a,(IMM)b);}
int mov(DX a,short b){return x86(641,a,(IMM)b);}
int mov(CX a,char b){return x86(641,a,(IMM)b);}
int mov(CX a,short b){return x86(641,a,(IMM)b);}
int mov(REG16 a,char b){return x86(641,a,(IMM)b);}
int mov(REG16 a,short b){return x86(641,a,(IMM)b);}
int mov(EAX a,int b){return x86(642,a,(IMM)b);}
int mov(EAX a,char b){return x86(642,a,(IMM)b);}
int mov(EAX a,short b){return x86(642,a,(IMM)b);}
int mov(EAX a,REF b){return x86(642,a,b);}
int mov(ECX a,int b){return x86(642,a,(IMM)b);}
int mov(ECX a,char b){return x86(642,a,(IMM)b);}
int mov(ECX a,short b){return x86(642,a,(IMM)b);}
int mov(ECX a,REF b){return x86(642,a,b);}
int mov(REG32 a,int b){return x86(642,a,(IMM)b);}
int mov(REG32 a,char b){return x86(642,a,(IMM)b);}
int mov(REG32 a,short b){return x86(642,a,(IMM)b);}
int mov(REG32 a,REF b){return x86(642,a,b);}
int mov(MEM8 a,char b){return x86(643,a,(IMM)b);}
int mov(R_M8 a,char b){return x86(643,a,(IMM)b);}
int mov(MEM16 a,char b){return x86(644,a,(IMM)b);}
int mov(MEM16 a,short b){return x86(644,a,(IMM)b);}
int mov(R_M16 a,char b){return x86(644,a,(IMM)b);}
int mov(R_M16 a,short b){return x86(644,a,(IMM)b);}
int mov(MEM32 a,int b){return x86(645,a,(IMM)b);}
int mov(MEM32 a,char b){return x86(645,a,(IMM)b);}
int mov(MEM32 a,short b){return x86(645,a,(IMM)b);}
int mov(MEM32 a,REF b){return x86(645,a,b);}
int mov(R_M32 a,int b){return x86(645,a,(IMM)b);}
int mov(R_M32 a,char b){return x86(645,a,(IMM)b);}
int mov(R_M32 a,short b){return x86(645,a,(IMM)b);}
int mov(R_M32 a,REF b){return x86(645,a,b);}
int movapd(XMMREG a,XMMREG b){return x86(646,a,b);}
int movapd(XMMREG a,MEM128 b){return x86(646,a,b);}
int movapd(XMMREG a,R_M128 b){return x86(646,a,b);}
int movapd(MEM128 a,XMMREG b){return x86(647,a,b);}
int movapd(R_M128 a,XMMREG b){return x86(647,a,b);}
int movaps(XMMREG a,XMMREG b){return x86(648,a,b);}
int movaps(XMMREG a,MEM128 b){return x86(648,a,b);}
int movaps(XMMREG a,R_M128 b){return x86(648,a,b);}
int movaps(MEM128 a,XMMREG b){return x86(649,a,b);}
int movaps(R_M128 a,XMMREG b){return x86(649,a,b);}
int movd(MMREG a,EAX b){return x86(650,a,b);}
int movd(MMREG a,ECX b){return x86(650,a,b);}
int movd(MMREG a,REG32 b){return x86(650,a,b);}
int movd(MMREG a,MEM32 b){return x86(650,a,b);}
int movd(MMREG a,R_M32 b){return x86(650,a,b);}
int movd(EAX a,MMREG b){return x86(651,a,b);}
int movd(ECX a,MMREG b){return x86(651,a,b);}
int movd(REG32 a,MMREG b){return x86(651,a,b);}
int movd(MEM32 a,MMREG b){return x86(651,a,b);}
int movd(R_M32 a,MMREG b){return x86(651,a,b);}
int movd(XMMREG a,EAX b){return x86(652,a,b);}
int movd(XMMREG a,ECX b){return x86(652,a,b);}
int movd(XMMREG a,REG32 b){return x86(652,a,b);}
int movd(XMMREG a,MEM32 b){return x86(652,a,b);}
int movd(XMMREG a,R_M32 b){return x86(652,a,b);}
int movd(EAX a,XMMREG b){return x86(653,a,b);}
int movd(ECX a,XMMREG b){return x86(653,a,b);}
int movd(REG32 a,XMMREG b){return x86(653,a,b);}
int movd(MEM32 a,XMMREG b){return x86(653,a,b);}
int movd(R_M32 a,XMMREG b){return x86(653,a,b);}
int movdq2q(MMREG a,XMMREG b){return x86(654,a,b);}
int movdqa(XMMREG a,XMMREG b){return x86(655,a,b);}
int movdqa(XMMREG a,MEM128 b){return x86(655,a,b);}
int movdqa(XMMREG a,R_M128 b){return x86(655,a,b);}
int movdqa(MEM128 a,XMMREG b){return x86(656,a,b);}
int movdqa(R_M128 a,XMMREG b){return x86(656,a,b);}
int movdqu(XMMREG a,XMMREG b){return x86(657,a,b);}
int movdqu(XMMREG a,MEM128 b){return x86(657,a,b);}
int movdqu(XMMREG a,R_M128 b){return x86(657,a,b);}
int movdqu(MEM128 a,XMMREG b){return x86(658,a,b);}
int movdqu(R_M128 a,XMMREG b){return x86(658,a,b);}
int movhpd(XMMREG a,MEM64 b){return x86(659,a,b);}
int movhpd(MEM64 a,XMMREG b){return x86(660,a,b);}
int movhlps(XMMREG a,XMMREG b){return x86(661,a,b);}
int movlpd(XMMREG a,MEM64 b){return x86(662,a,b);}
int movlpd(MEM64 a,XMMREG b){return x86(663,a,b);}
int movhps(XMMREG a,MEM64 b){return x86(664,a,b);}
int movhps(MEM64 a,XMMREG b){return x86(665,a,b);}
int movhps(XMMREG a,XMMREG b){return x86(666,a,b);}
int movlhps(XMMREG a,XMMREG b){return x86(667,a,b);}
int movlps(XMMREG a,MEM64 b){return x86(668,a,b);}
int movlps(MEM64 a,XMMREG b){return x86(669,a,b);}
int movlps(XMMREG a,XMMREG b){return x86(670,a,b);}
int movmskpd(EAX a,XMMREG b){return x86(671,a,b);}
int movmskpd(ECX a,XMMREG b){return x86(671,a,b);}
int movmskpd(REG32 a,XMMREG b){return x86(671,a,b);}
int movmskps(EAX a,XMMREG b){return x86(672,a,b);}
int movmskps(ECX a,XMMREG b){return x86(672,a,b);}
int movmskps(REG32 a,XMMREG b){return x86(672,a,b);}
int movntdq(MEM128 a,XMMREG b){return x86(673,a,b);}
int movnti(MEM32 a,EAX b){return x86(674,a,b);}
int movnti(MEM32 a,ECX b){return x86(674,a,b);}
int movnti(MEM32 a,REG32 b){return x86(674,a,b);}
int movntpd(MEM128 a,XMMREG b){return x86(675,a,b);}
int movntps(MEM128 a,XMMREG b){return x86(676,a,b);}
int movntq(MEM64 a,MMREG b){return x86(677,a,b);}
int movq(MMREG a,MMREG b){return x86(678,a,b);}
int movq(MMREG a,MEM64 b){return x86(678,a,b);}
int movq(MMREG a,R_M64 b){return x86(678,a,b);}
int movq(MEM64 a,MMREG b){return x86(679,a,b);}
int movq(R_M64 a,MMREG b){return x86(679,a,b);}
int movq(XMMREG a,XMMREG b){return x86(680,a,b);}
int movq(XMMREG a,MEM64 b){return x86(680,a,b);}
int movq(XMMREG a,XMM64 b){return x86(680,a,b);}
int movq(MEM64 a,XMMREG b){return x86(681,a,b);}
int movq(XMM64 a,XMMREG b){return x86(681,a,b);}
int movq2dq(XMMREG a,MMREG b){return x86(682,a,b);}
int movsb(){return x86(683);}
int movsw(){return x86(684);}
int movsd(){return x86(685);}
int rep_movsb(){return x86(686);}
int rep_movsw(){return x86(687);}
int rep_movsd(){return x86(688);}
int movsd(XMMREG a,XMMREG b){return x86(689,a,b);}
int movsd(XMMREG a,MEM64 b){return x86(689,a,b);}
int movsd(XMMREG a,XMM64 b){return x86(689,a,b);}
int movsd(MEM64 a,XMMREG b){return x86(690,a,b);}
int movsd(XMM64 a,XMMREG b){return x86(690,a,b);}
int movss(XMMREG a,XMMREG b){return x86(691,a,b);}
int movss(XMMREG a,MEM32 b){return x86(691,a,b);}
int movss(XMMREG a,XMM32 b){return x86(691,a,b);}
int movss(MEM32 a,XMMREG b){return x86(692,a,b);}
int movss(XMM32 a,XMMREG b){return x86(692,a,b);}
int movsx(AX a,AL b){return x86(693,a,b);}
int movsx(AX a,CL b){return x86(693,a,b);}
int movsx(AX a,REG8 b){return x86(693,a,b);}
int movsx(AX a,MEM8 b){return x86(693,a,b);}
int movsx(AX a,R_M8 b){return x86(693,a,b);}
int movsx(DX a,AL b){return x86(693,a,b);}
int movsx(DX a,CL b){return x86(693,a,b);}
int movsx(DX a,REG8 b){return x86(693,a,b);}
int movsx(DX a,MEM8 b){return x86(693,a,b);}
int movsx(DX a,R_M8 b){return x86(693,a,b);}
int movsx(CX a,AL b){return x86(693,a,b);}
int movsx(CX a,CL b){return x86(693,a,b);}
int movsx(CX a,REG8 b){return x86(693,a,b);}
int movsx(CX a,MEM8 b){return x86(693,a,b);}
int movsx(CX a,R_M8 b){return x86(693,a,b);}
int movsx(REG16 a,AL b){return x86(693,a,b);}
int movsx(REG16 a,CL b){return x86(693,a,b);}
int movsx(REG16 a,REG8 b){return x86(693,a,b);}
int movsx(REG16 a,MEM8 b){return x86(693,a,b);}
int movsx(REG16 a,R_M8 b){return x86(693,a,b);}
int movsx(EAX a,AL b){return x86(694,a,b);}
int movsx(EAX a,CL b){return x86(694,a,b);}
int movsx(EAX a,REG8 b){return x86(694,a,b);}
int movsx(EAX a,MEM8 b){return x86(694,a,b);}
int movsx(EAX a,R_M8 b){return x86(694,a,b);}
int movsx(ECX a,AL b){return x86(694,a,b);}
int movsx(ECX a,CL b){return x86(694,a,b);}
int movsx(ECX a,REG8 b){return x86(694,a,b);}
int movsx(ECX a,MEM8 b){return x86(694,a,b);}
int movsx(ECX a,R_M8 b){return x86(694,a,b);}
int movsx(REG32 a,AL b){return x86(694,a,b);}
int movsx(REG32 a,CL b){return x86(694,a,b);}
int movsx(REG32 a,REG8 b){return x86(694,a,b);}
int movsx(REG32 a,MEM8 b){return x86(694,a,b);}
int movsx(REG32 a,R_M8 b){return x86(694,a,b);}
int movsx(EAX a,AX b){return x86(695,a,b);}
int movsx(EAX a,DX b){return x86(695,a,b);}
int movsx(EAX a,CX b){return x86(695,a,b);}
int movsx(EAX a,REG16 b){return x86(695,a,b);}
int movsx(EAX a,MEM16 b){return x86(695,a,b);}
int movsx(EAX a,R_M16 b){return x86(695,a,b);}
int movsx(ECX a,AX b){return x86(695,a,b);}
int movsx(ECX a,DX b){return x86(695,a,b);}
int movsx(ECX a,CX b){return x86(695,a,b);}
int movsx(ECX a,REG16 b){return x86(695,a,b);}
int movsx(ECX a,MEM16 b){return x86(695,a,b);}
int movsx(ECX a,R_M16 b){return x86(695,a,b);}
int movsx(REG32 a,AX b){return x86(695,a,b);}
int movsx(REG32 a,DX b){return x86(695,a,b);}
int movsx(REG32 a,CX b){return x86(695,a,b);}
int movsx(REG32 a,REG16 b){return x86(695,a,b);}
int movsx(REG32 a,MEM16 b){return x86(695,a,b);}
int movsx(REG32 a,R_M16 b){return x86(695,a,b);}
int movzx(AX a,AL b){return x86(696,a,b);}
int movzx(AX a,CL b){return x86(696,a,b);}
int movzx(AX a,REG8 b){return x86(696,a,b);}
int movzx(AX a,MEM8 b){return x86(696,a,b);}
int movzx(AX a,R_M8 b){return x86(696,a,b);}
int movzx(DX a,AL b){return x86(696,a,b);}
int movzx(DX a,CL b){return x86(696,a,b);}
int movzx(DX a,REG8 b){return x86(696,a,b);}
int movzx(DX a,MEM8 b){return x86(696,a,b);}
int movzx(DX a,R_M8 b){return x86(696,a,b);}
int movzx(CX a,AL b){return x86(696,a,b);}
int movzx(CX a,CL b){return x86(696,a,b);}
int movzx(CX a,REG8 b){return x86(696,a,b);}
int movzx(CX a,MEM8 b){return x86(696,a,b);}
int movzx(CX a,R_M8 b){return x86(696,a,b);}
int movzx(REG16 a,AL b){return x86(696,a,b);}
int movzx(REG16 a,CL b){return x86(696,a,b);}
int movzx(REG16 a,REG8 b){return x86(696,a,b);}
int movzx(REG16 a,MEM8 b){return x86(696,a,b);}
int movzx(REG16 a,R_M8 b){return x86(696,a,b);}
int movzx(EAX a,AL b){return x86(697,a,b);}
int movzx(EAX a,CL b){return x86(697,a,b);}
int movzx(EAX a,REG8 b){return x86(697,a,b);}
int movzx(EAX a,MEM8 b){return x86(697,a,b);}
int movzx(EAX a,R_M8 b){return x86(697,a,b);}
int movzx(ECX a,AL b){return x86(697,a,b);}
int movzx(ECX a,CL b){return x86(697,a,b);}
int movzx(ECX a,REG8 b){return x86(697,a,b);}
int movzx(ECX a,MEM8 b){return x86(697,a,b);}
int movzx(ECX a,R_M8 b){return x86(697,a,b);}
int movzx(REG32 a,AL b){return x86(697,a,b);}
int movzx(REG32 a,CL b){return x86(697,a,b);}
int movzx(REG32 a,REG8 b){return x86(697,a,b);}
int movzx(REG32 a,MEM8 b){return x86(697,a,b);}
int movzx(REG32 a,R_M8 b){return x86(697,a,b);}
int movzx(EAX a,AX b){return x86(698,a,b);}
int movzx(EAX a,DX b){return x86(698,a,b);}
int movzx(EAX a,CX b){return x86(698,a,b);}
int movzx(EAX a,REG16 b){return x86(698,a,b);}
int movzx(EAX a,MEM16 b){return x86(698,a,b);}
int movzx(EAX a,R_M16 b){return x86(698,a,b);}
int movzx(ECX a,AX b){return x86(698,a,b);}
int movzx(ECX a,DX b){return x86(698,a,b);}
int movzx(ECX a,CX b){return x86(698,a,b);}
int movzx(ECX a,REG16 b){return x86(698,a,b);}
int movzx(ECX a,MEM16 b){return x86(698,a,b);}
int movzx(ECX a,R_M16 b){return x86(698,a,b);}
int movzx(REG32 a,AX b){return x86(698,a,b);}
int movzx(REG32 a,DX b){return x86(698,a,b);}
int movzx(REG32 a,CX b){return x86(698,a,b);}
int movzx(REG32 a,REG16 b){return x86(698,a,b);}
int movzx(REG32 a,MEM16 b){return x86(698,a,b);}
int movzx(REG32 a,R_M16 b){return x86(698,a,b);}
int movupd(XMMREG a,XMMREG b){return x86(699,a,b);}
int movupd(XMMREG a,MEM128 b){return x86(699,a,b);}
int movupd(XMMREG a,R_M128 b){return x86(699,a,b);}
int movupd(MEM128 a,XMMREG b){return x86(700,a,b);}
int movupd(R_M128 a,XMMREG b){return x86(700,a,b);}
int movups(XMMREG a,XMMREG b){return x86(701,a,b);}
int movups(XMMREG a,MEM128 b){return x86(701,a,b);}
int movups(XMMREG a,R_M128 b){return x86(701,a,b);}
int movups(MEM128 a,XMMREG b){return x86(702,a,b);}
int movups(R_M128 a,XMMREG b){return x86(702,a,b);}
int mul(AL a){return x86(703,a);}
int mul(CL a){return x86(703,a);}
int mul(REG8 a){return x86(703,a);}
int mul(MEM8 a){return x86(703,a);}
int mul(R_M8 a){return x86(703,a);}
int mul(AX a){return x86(704,a);}
int mul(DX a){return x86(704,a);}
int mul(CX a){return x86(704,a);}
int mul(REG16 a){return x86(704,a);}
int mul(MEM16 a){return x86(704,a);}
int mul(R_M16 a){return x86(704,a);}
int mul(EAX a){return x86(705,a);}
int mul(ECX a){return x86(705,a);}
int mul(REG32 a){return x86(705,a);}
int mul(MEM32 a){return x86(705,a);}
int mul(R_M32 a){return x86(705,a);}
int mulpd(XMMREG a,XMMREG b){return x86(706,a,b);}
int mulpd(XMMREG a,MEM128 b){return x86(706,a,b);}
int mulpd(XMMREG a,R_M128 b){return x86(706,a,b);}
int mulps(XMMREG a,XMMREG b){return x86(707,a,b);}
int mulps(XMMREG a,MEM128 b){return x86(707,a,b);}
int mulps(XMMREG a,R_M128 b){return x86(707,a,b);}
int mulsd(XMMREG a,XMMREG b){return x86(708,a,b);}
int mulsd(XMMREG a,MEM64 b){return x86(708,a,b);}
int mulsd(XMMREG a,XMM64 b){return x86(708,a,b);}
int mulss(XMMREG a,XMMREG b){return x86(709,a,b);}
int mulss(XMMREG a,MEM32 b){return x86(709,a,b);}
int mulss(XMMREG a,XMM32 b){return x86(709,a,b);}
int neg(AL a){return x86(710,a);}
int neg(CL a){return x86(710,a);}
int neg(REG8 a){return x86(710,a);}
int neg(MEM8 a){return x86(710,a);}
int neg(R_M8 a){return x86(710,a);}
int neg(AX a){return x86(711,a);}
int neg(DX a){return x86(711,a);}
int neg(CX a){return x86(711,a);}
int neg(REG16 a){return x86(711,a);}
int neg(MEM16 a){return x86(711,a);}
int neg(R_M16 a){return x86(711,a);}
int neg(EAX a){return x86(712,a);}
int neg(ECX a){return x86(712,a);}
int neg(REG32 a){return x86(712,a);}
int neg(MEM32 a){return x86(712,a);}
int neg(R_M32 a){return x86(712,a);}
int lock_neg(MEM8 a){return x86(713,a);}
int lock_neg(MEM16 a){return x86(714,a);}
int lock_neg(MEM32 a){return x86(715,a);}
int not(AL a){return x86(716,a);}
int not(CL a){return x86(716,a);}
int not(REG8 a){return x86(716,a);}
int not(MEM8 a){return x86(716,a);}
int not(R_M8 a){return x86(716,a);}
int not(AX a){return x86(717,a);}
int not(DX a){return x86(717,a);}
int not(CX a){return x86(717,a);}
int not(REG16 a){return x86(717,a);}
int not(MEM16 a){return x86(717,a);}
int not(R_M16 a){return x86(717,a);}
int not(EAX a){return x86(718,a);}
int not(ECX a){return x86(718,a);}
int not(REG32 a){return x86(718,a);}
int not(MEM32 a){return x86(718,a);}
int not(R_M32 a){return x86(718,a);}
int lock_not(MEM8 a){return x86(719,a);}
int lock_not(MEM16 a){return x86(720,a);}
int lock_not(MEM32 a){return x86(721,a);}
int nop(){return x86(722);}
int or(AL a,AL b){return x86(723,a,b);}
int or(AL a,CL b){return x86(723,a,b);}
int or(AL a,REG8 b){return x86(723,a,b);}
int or(CL a,AL b){return x86(723,a,b);}
int or(CL a,CL b){return x86(723,a,b);}
int or(CL a,REG8 b){return x86(723,a,b);}
int or(REG8 a,AL b){return x86(723,a,b);}
int or(REG8 a,CL b){return x86(723,a,b);}
int or(REG8 a,REG8 b){return x86(723,a,b);}
int or(MEM8 a,AL b){return x86(723,a,b);}
int or(MEM8 a,CL b){return x86(723,a,b);}
int or(MEM8 a,REG8 b){return x86(723,a,b);}
int or(R_M8 a,AL b){return x86(723,a,b);}
int or(R_M8 a,CL b){return x86(723,a,b);}
int or(R_M8 a,REG8 b){return x86(723,a,b);}
int or(AX a,AX b){return x86(724,a,b);}
int or(AX a,DX b){return x86(724,a,b);}
int or(AX a,CX b){return x86(724,a,b);}
int or(AX a,REG16 b){return x86(724,a,b);}
int or(DX a,AX b){return x86(724,a,b);}
int or(DX a,DX b){return x86(724,a,b);}
int or(DX a,CX b){return x86(724,a,b);}
int or(DX a,REG16 b){return x86(724,a,b);}
int or(CX a,AX b){return x86(724,a,b);}
int or(CX a,DX b){return x86(724,a,b);}
int or(CX a,CX b){return x86(724,a,b);}
int or(CX a,REG16 b){return x86(724,a,b);}
int or(REG16 a,AX b){return x86(724,a,b);}
int or(REG16 a,DX b){return x86(724,a,b);}
int or(REG16 a,CX b){return x86(724,a,b);}
int or(REG16 a,REG16 b){return x86(724,a,b);}
int or(MEM16 a,AX b){return x86(724,a,b);}
int or(MEM16 a,DX b){return x86(724,a,b);}
int or(MEM16 a,CX b){return x86(724,a,b);}
int or(MEM16 a,REG16 b){return x86(724,a,b);}
int or(R_M16 a,AX b){return x86(724,a,b);}
int or(R_M16 a,DX b){return x86(724,a,b);}
int or(R_M16 a,CX b){return x86(724,a,b);}
int or(R_M16 a,REG16 b){return x86(724,a,b);}
int or(EAX a,EAX b){return x86(725,a,b);}
int or(EAX a,ECX b){return x86(725,a,b);}
int or(EAX a,REG32 b){return x86(725,a,b);}
int or(ECX a,EAX b){return x86(725,a,b);}
int or(ECX a,ECX b){return x86(725,a,b);}
int or(ECX a,REG32 b){return x86(725,a,b);}
int or(REG32 a,EAX b){return x86(725,a,b);}
int or(REG32 a,ECX b){return x86(725,a,b);}
int or(REG32 a,REG32 b){return x86(725,a,b);}
int or(MEM32 a,EAX b){return x86(725,a,b);}
int or(MEM32 a,ECX b){return x86(725,a,b);}
int or(MEM32 a,REG32 b){return x86(725,a,b);}
int or(R_M32 a,EAX b){return x86(725,a,b);}
int or(R_M32 a,ECX b){return x86(725,a,b);}
int or(R_M32 a,REG32 b){return x86(725,a,b);}
int lock_or(MEM8 a,AL b){return x86(726,a,b);}
int lock_or(MEM8 a,CL b){return x86(726,a,b);}
int lock_or(MEM8 a,REG8 b){return x86(726,a,b);}
int lock_or(MEM16 a,AX b){return x86(727,a,b);}
int lock_or(MEM16 a,DX b){return x86(727,a,b);}
int lock_or(MEM16 a,CX b){return x86(727,a,b);}
int lock_or(MEM16 a,REG16 b){return x86(727,a,b);}
int lock_or(MEM32 a,EAX b){return x86(728,a,b);}
int lock_or(MEM32 a,ECX b){return x86(728,a,b);}
int lock_or(MEM32 a,REG32 b){return x86(728,a,b);}
int or(AL a,MEM8 b){return x86(729,a,b);}
int or(AL a,R_M8 b){return x86(729,a,b);}
int or(CL a,MEM8 b){return x86(729,a,b);}
int or(CL a,R_M8 b){return x86(729,a,b);}
int or(REG8 a,MEM8 b){return x86(729,a,b);}
int or(REG8 a,R_M8 b){return x86(729,a,b);}
int or(AX a,MEM16 b){return x86(730,a,b);}
int or(AX a,R_M16 b){return x86(730,a,b);}
int or(DX a,MEM16 b){return x86(730,a,b);}
int or(DX a,R_M16 b){return x86(730,a,b);}
int or(CX a,MEM16 b){return x86(730,a,b);}
int or(CX a,R_M16 b){return x86(730,a,b);}
int or(REG16 a,MEM16 b){return x86(730,a,b);}
int or(REG16 a,R_M16 b){return x86(730,a,b);}
int or(EAX a,MEM32 b){return x86(731,a,b);}
int or(EAX a,R_M32 b){return x86(731,a,b);}
int or(ECX a,MEM32 b){return x86(731,a,b);}
int or(ECX a,R_M32 b){return x86(731,a,b);}
int or(REG32 a,MEM32 b){return x86(731,a,b);}
int or(REG32 a,R_M32 b){return x86(731,a,b);}
int or(AL a,char b){return x86(732,a,(IMM)b);}
int or(CL a,char b){return x86(732,a,(IMM)b);}
int or(REG8 a,char b){return x86(732,a,(IMM)b);}
int or(MEM8 a,char b){return x86(732,a,(IMM)b);}
int or(R_M8 a,char b){return x86(732,a,(IMM)b);}
int or(AX a,char b){return x86(733,a,(IMM)b);}
int or(AX a,short b){return x86(733,a,(IMM)b);}
int or(DX a,char b){return x86(733,a,(IMM)b);}
int or(DX a,short b){return x86(733,a,(IMM)b);}
int or(CX a,char b){return x86(733,a,(IMM)b);}
int or(CX a,short b){return x86(733,a,(IMM)b);}
int or(REG16 a,char b){return x86(733,a,(IMM)b);}
int or(REG16 a,short b){return x86(733,a,(IMM)b);}
int or(MEM16 a,char b){return x86(733,a,(IMM)b);}
int or(MEM16 a,short b){return x86(733,a,(IMM)b);}
int or(R_M16 a,char b){return x86(733,a,(IMM)b);}
int or(R_M16 a,short b){return x86(733,a,(IMM)b);}
int or(EAX a,int b){return x86(734,a,(IMM)b);}
int or(EAX a,char b){return x86(734,a,(IMM)b);}
int or(EAX a,short b){return x86(734,a,(IMM)b);}
int or(EAX a,REF b){return x86(734,a,b);}
int or(ECX a,int b){return x86(734,a,(IMM)b);}
int or(ECX a,char b){return x86(734,a,(IMM)b);}
int or(ECX a,short b){return x86(734,a,(IMM)b);}
int or(ECX a,REF b){return x86(734,a,b);}
int or(REG32 a,int b){return x86(734,a,(IMM)b);}
int or(REG32 a,char b){return x86(734,a,(IMM)b);}
int or(REG32 a,short b){return x86(734,a,(IMM)b);}
int or(REG32 a,REF b){return x86(734,a,b);}
int or(MEM32 a,int b){return x86(734,a,(IMM)b);}
int or(MEM32 a,char b){return x86(734,a,(IMM)b);}
int or(MEM32 a,short b){return x86(734,a,(IMM)b);}
int or(MEM32 a,REF b){return x86(734,a,b);}
int or(R_M32 a,int b){return x86(734,a,(IMM)b);}
int or(R_M32 a,char b){return x86(734,a,(IMM)b);}
int or(R_M32 a,short b){return x86(734,a,(IMM)b);}
int or(R_M32 a,REF b){return x86(734,a,b);}
int lock_or(MEM8 a,char b){return x86(737,a,(IMM)b);}
int lock_or(MEM16 a,char b){return x86(738,a,(IMM)b);}
int lock_or(MEM16 a,short b){return x86(738,a,(IMM)b);}
int lock_or(MEM32 a,int b){return x86(739,a,(IMM)b);}
int lock_or(MEM32 a,char b){return x86(739,a,(IMM)b);}
int lock_or(MEM32 a,short b){return x86(739,a,(IMM)b);}
int lock_or(MEM32 a,REF b){return x86(739,a,b);}
int orpd(XMMREG a,XMMREG b){return x86(745,a,b);}
int orpd(XMMREG a,MEM128 b){return x86(745,a,b);}
int orpd(XMMREG a,R_M128 b){return x86(745,a,b);}
int orps(XMMREG a,XMMREG b){return x86(746,a,b);}
int orps(XMMREG a,MEM128 b){return x86(746,a,b);}
int orps(XMMREG a,R_M128 b){return x86(746,a,b);}
int out(char a,AL b){return x86(747,(IMM)a,b);}
int out(char a,AX b){return x86(748,(IMM)a,b);}
int out(char a,EAX b){return x86(749,(IMM)a,b);}
int out(DX a,AL b){return x86(750,a,b);}
int out(DX a,AX b){return x86(751,a,b);}
int out(DX a,EAX b){return x86(752,a,b);}
int outsb(){return x86(753);}
int outsw(){return x86(754);}
int outsd(){return x86(755);}
int rep_outsb(){return x86(756);}
int rep_outsw(){return x86(757);}
int rep_outsd(){return x86(758);}
int packssdw(MMREG a,MMREG b){return x86(759,a,b);}
int packssdw(MMREG a,MEM64 b){return x86(759,a,b);}
int packssdw(MMREG a,R_M64 b){return x86(759,a,b);}
int packsswb(MMREG a,MMREG b){return x86(760,a,b);}
int packsswb(MMREG a,MEM64 b){return x86(760,a,b);}
int packsswb(MMREG a,R_M64 b){return x86(760,a,b);}
int packuswb(MMREG a,MMREG b){return x86(761,a,b);}
int packuswb(MMREG a,MEM64 b){return x86(761,a,b);}
int packuswb(MMREG a,R_M64 b){return x86(761,a,b);}
int packssdw(XMMREG a,XMMREG b){return x86(762,a,b);}
int packssdw(XMMREG a,MEM128 b){return x86(762,a,b);}
int packssdw(XMMREG a,R_M128 b){return x86(762,a,b);}
int packsswb(XMMREG a,XMMREG b){return x86(763,a,b);}
int packsswb(XMMREG a,MEM128 b){return x86(763,a,b);}
int packsswb(XMMREG a,R_M128 b){return x86(763,a,b);}
int packuswb(XMMREG a,XMMREG b){return x86(764,a,b);}
int packuswb(XMMREG a,MEM128 b){return x86(764,a,b);}
int packuswb(XMMREG a,R_M128 b){return x86(764,a,b);}
int paddb(MMREG a,MMREG b){return x86(765,a,b);}
int paddb(MMREG a,MEM64 b){return x86(765,a,b);}
int paddb(MMREG a,R_M64 b){return x86(765,a,b);}
int paddw(MMREG a,MMREG b){return x86(766,a,b);}
int paddw(MMREG a,MEM64 b){return x86(766,a,b);}
int paddw(MMREG a,R_M64 b){return x86(766,a,b);}
int paddd(MMREG a,MMREG b){return x86(767,a,b);}
int paddd(MMREG a,MEM64 b){return x86(767,a,b);}
int paddd(MMREG a,R_M64 b){return x86(767,a,b);}
int paddb(XMMREG a,XMMREG b){return x86(768,a,b);}
int paddb(XMMREG a,MEM128 b){return x86(768,a,b);}
int paddb(XMMREG a,R_M128 b){return x86(768,a,b);}
int paddw(XMMREG a,XMMREG b){return x86(769,a,b);}
int paddw(XMMREG a,MEM128 b){return x86(769,a,b);}
int paddw(XMMREG a,R_M128 b){return x86(769,a,b);}
int paddd(XMMREG a,XMMREG b){return x86(770,a,b);}
int paddd(XMMREG a,MEM128 b){return x86(770,a,b);}
int paddd(XMMREG a,R_M128 b){return x86(770,a,b);}
int paddq(MMREG a,MMREG b){return x86(771,a,b);}
int paddq(MMREG a,MEM64 b){return x86(771,a,b);}
int paddq(MMREG a,R_M64 b){return x86(771,a,b);}
int paddq(XMMREG a,XMMREG b){return x86(772,a,b);}
int paddq(XMMREG a,MEM128 b){return x86(772,a,b);}
int paddq(XMMREG a,R_M128 b){return x86(772,a,b);}
int paddsb(MMREG a,MMREG b){return x86(773,a,b);}
int paddsb(MMREG a,MEM64 b){return x86(773,a,b);}
int paddsb(MMREG a,R_M64 b){return x86(773,a,b);}
int paddsw(MMREG a,MMREG b){return x86(774,a,b);}
int paddsw(MMREG a,MEM64 b){return x86(774,a,b);}
int paddsw(MMREG a,R_M64 b){return x86(774,a,b);}
int paddsb(XMMREG a,XMMREG b){return x86(775,a,b);}
int paddsb(XMMREG a,MEM128 b){return x86(775,a,b);}
int paddsb(XMMREG a,R_M128 b){return x86(775,a,b);}
int paddsw(XMMREG a,XMMREG b){return x86(776,a,b);}
int paddsw(XMMREG a,MEM128 b){return x86(776,a,b);}
int paddsw(XMMREG a,R_M128 b){return x86(776,a,b);}
int paddusb(MMREG a,MMREG b){return x86(777,a,b);}
int paddusb(MMREG a,MEM64 b){return x86(777,a,b);}
int paddusb(MMREG a,R_M64 b){return x86(777,a,b);}
int paddusw(MMREG a,MMREG b){return x86(778,a,b);}
int paddusw(MMREG a,MEM64 b){return x86(778,a,b);}
int paddusw(MMREG a,R_M64 b){return x86(778,a,b);}
int paddusb(XMMREG a,XMMREG b){return x86(779,a,b);}
int paddusb(XMMREG a,MEM128 b){return x86(779,a,b);}
int paddusb(XMMREG a,R_M128 b){return x86(779,a,b);}
int paddusw(XMMREG a,XMMREG b){return x86(780,a,b);}
int paddusw(XMMREG a,MEM128 b){return x86(780,a,b);}
int paddusw(XMMREG a,R_M128 b){return x86(780,a,b);}
int paddsiw(MMREG a,MMREG b){return x86(781,a,b);}
int paddsiw(MMREG a,MEM64 b){return x86(781,a,b);}
int paddsiw(MMREG a,R_M64 b){return x86(781,a,b);}
int pand(MMREG a,MMREG b){return x86(782,a,b);}
int pand(MMREG a,MEM64 b){return x86(782,a,b);}
int pand(MMREG a,R_M64 b){return x86(782,a,b);}
int pandn(MMREG a,MMREG b){return x86(783,a,b);}
int pandn(MMREG a,MEM64 b){return x86(783,a,b);}
int pandn(MMREG a,R_M64 b){return x86(783,a,b);}
int pand(XMMREG a,XMMREG b){return x86(784,a,b);}
int pand(XMMREG a,MEM128 b){return x86(784,a,b);}
int pand(XMMREG a,R_M128 b){return x86(784,a,b);}
int pandn(XMMREG a,XMMREG b){return x86(785,a,b);}
int pandn(XMMREG a,MEM128 b){return x86(785,a,b);}
int pandn(XMMREG a,R_M128 b){return x86(785,a,b);}
int pause(){return x86(786);}
int paveb(MMREG a,MMREG b){return x86(787,a,b);}
int paveb(MMREG a,MEM64 b){return x86(787,a,b);}
int paveb(MMREG a,R_M64 b){return x86(787,a,b);}
int pavgb(MMREG a,MMREG b){return x86(788,a,b);}
int pavgb(MMREG a,MEM64 b){return x86(788,a,b);}
int pavgb(MMREG a,R_M64 b){return x86(788,a,b);}
int pavgw(MMREG a,MMREG b){return x86(789,a,b);}
int pavgw(MMREG a,MEM64 b){return x86(789,a,b);}
int pavgw(MMREG a,R_M64 b){return x86(789,a,b);}
int pavgb(XMMREG a,XMMREG b){return x86(790,a,b);}
int pavgb(XMMREG a,MEM128 b){return x86(790,a,b);}
int pavgb(XMMREG a,R_M128 b){return x86(790,a,b);}
int pavgw(XMMREG a,XMMREG b){return x86(791,a,b);}
int pavgw(XMMREG a,MEM128 b){return x86(791,a,b);}
int pavgw(XMMREG a,R_M128 b){return x86(791,a,b);}
int pavgusb(MMREG a,MMREG b){return x86(792,a,b);}
int pavgusb(MMREG a,MEM64 b){return x86(792,a,b);}
int pavgusb(MMREG a,R_M64 b){return x86(792,a,b);}
int pcmpeqb(MMREG a,MMREG b){return x86(793,a,b);}
int pcmpeqb(MMREG a,MEM64 b){return x86(793,a,b);}
int pcmpeqb(MMREG a,R_M64 b){return x86(793,a,b);}
int pcmpeqw(MMREG a,MMREG b){return x86(794,a,b);}
int pcmpeqw(MMREG a,MEM64 b){return x86(794,a,b);}
int pcmpeqw(MMREG a,R_M64 b){return x86(794,a,b);}
int pcmpeqd(MMREG a,MMREG b){return x86(795,a,b);}
int pcmpeqd(MMREG a,MEM64 b){return x86(795,a,b);}
int pcmpeqd(MMREG a,R_M64 b){return x86(795,a,b);}
int pcmpgtb(MMREG a,MMREG b){return x86(796,a,b);}
int pcmpgtb(MMREG a,MEM64 b){return x86(796,a,b);}
int pcmpgtb(MMREG a,R_M64 b){return x86(796,a,b);}
int pcmpgtw(MMREG a,MMREG b){return x86(797,a,b);}
int pcmpgtw(MMREG a,MEM64 b){return x86(797,a,b);}
int pcmpgtw(MMREG a,R_M64 b){return x86(797,a,b);}
int pcmpgtd(MMREG a,MMREG b){return x86(798,a,b);}
int pcmpgtd(MMREG a,MEM64 b){return x86(798,a,b);}
int pcmpgtd(MMREG a,R_M64 b){return x86(798,a,b);}
int pcmpeqb(XMMREG a,XMMREG b){return x86(799,a,b);}
int pcmpeqb(XMMREG a,MEM128 b){return x86(799,a,b);}
int pcmpeqb(XMMREG a,R_M128 b){return x86(799,a,b);}
int pcmpeqw(XMMREG a,XMMREG b){return x86(800,a,b);}
int pcmpeqw(XMMREG a,MEM128 b){return x86(800,a,b);}
int pcmpeqw(XMMREG a,R_M128 b){return x86(800,a,b);}
int pcmpeqd(XMMREG a,XMMREG b){return x86(801,a,b);}
int pcmpeqd(XMMREG a,MEM128 b){return x86(801,a,b);}
int pcmpeqd(XMMREG a,R_M128 b){return x86(801,a,b);}
int pcmpgtb(XMMREG a,XMMREG b){return x86(802,a,b);}
int pcmpgtb(XMMREG a,MEM128 b){return x86(802,a,b);}
int pcmpgtb(XMMREG a,R_M128 b){return x86(802,a,b);}
int pcmpgtw(XMMREG a,XMMREG b){return x86(803,a,b);}
int pcmpgtw(XMMREG a,MEM128 b){return x86(803,a,b);}
int pcmpgtw(XMMREG a,R_M128 b){return x86(803,a,b);}
int pcmpgtd(XMMREG a,XMMREG b){return x86(804,a,b);}
int pcmpgtd(XMMREG a,MEM128 b){return x86(804,a,b);}
int pcmpgtd(XMMREG a,R_M128 b){return x86(804,a,b);}
int pdistib(MMREG a,MEM64 b){return x86(805,a,b);}
int pextrw(EAX a,MMREG b,char c){return x86(806,a,b,(IMM)c);}
int pextrw(ECX a,MMREG b,char c){return x86(806,a,b,(IMM)c);}
int pextrw(REG32 a,MMREG b,char c){return x86(806,a,b,(IMM)c);}
int pextrw(EAX a,XMMREG b,char c){return x86(807,a,b,(IMM)c);}
int pextrw(ECX a,XMMREG b,char c){return x86(807,a,b,(IMM)c);}
int pextrw(REG32 a,XMMREG b,char c){return x86(807,a,b,(IMM)c);}
int pf2id(MMREG a,MMREG b){return x86(808,a,b);}
int pf2id(MMREG a,MEM64 b){return x86(808,a,b);}
int pf2id(MMREG a,R_M64 b){return x86(808,a,b);}
int pf2iw(MMREG a,MMREG b){return x86(809,a,b);}
int pf2iw(MMREG a,MEM64 b){return x86(809,a,b);}
int pf2iw(MMREG a,R_M64 b){return x86(809,a,b);}
int pfacc(MMREG a,MMREG b){return x86(810,a,b);}
int pfacc(MMREG a,MEM64 b){return x86(810,a,b);}
int pfacc(MMREG a,R_M64 b){return x86(810,a,b);}
int pfadd(MMREG a,MMREG b){return x86(811,a,b);}
int pfadd(MMREG a,MEM64 b){return x86(811,a,b);}
int pfadd(MMREG a,R_M64 b){return x86(811,a,b);}
int pfcmpeq(MMREG a,MMREG b){return x86(812,a,b);}
int pfcmpeq(MMREG a,MEM64 b){return x86(812,a,b);}
int pfcmpeq(MMREG a,R_M64 b){return x86(812,a,b);}
int pfcmpge(MMREG a,MMREG b){return x86(813,a,b);}
int pfcmpge(MMREG a,MEM64 b){return x86(813,a,b);}
int pfcmpge(MMREG a,R_M64 b){return x86(813,a,b);}
int pfcmpgt(MMREG a,MMREG b){return x86(814,a,b);}
int pfcmpgt(MMREG a,MEM64 b){return x86(814,a,b);}
int pfcmpgt(MMREG a,R_M64 b){return x86(814,a,b);}
int pfmax(MMREG a,MMREG b){return x86(815,a,b);}
int pfmax(MMREG a,MEM64 b){return x86(815,a,b);}
int pfmax(MMREG a,R_M64 b){return x86(815,a,b);}
int pfmin(MMREG a,MMREG b){return x86(816,a,b);}
int pfmin(MMREG a,MEM64 b){return x86(816,a,b);}
int pfmin(MMREG a,R_M64 b){return x86(816,a,b);}
int pfmul(MMREG a,MMREG b){return x86(817,a,b);}
int pfmul(MMREG a,MEM64 b){return x86(817,a,b);}
int pfmul(MMREG a,R_M64 b){return x86(817,a,b);}
int pfnacc(MMREG a,MMREG b){return x86(818,a,b);}
int pfnacc(MMREG a,MEM64 b){return x86(818,a,b);}
int pfnacc(MMREG a,R_M64 b){return x86(818,a,b);}
int pfpnacc(MMREG a,MMREG b){return x86(819,a,b);}
int pfpnacc(MMREG a,MEM64 b){return x86(819,a,b);}
int pfpnacc(MMREG a,R_M64 b){return x86(819,a,b);}
int pfrcp(MMREG a,MMREG b){return x86(820,a,b);}
int pfrcp(MMREG a,MEM64 b){return x86(820,a,b);}
int pfrcp(MMREG a,R_M64 b){return x86(820,a,b);}
int pfrcpit1(MMREG a,MMREG b){return x86(821,a,b);}
int pfrcpit1(MMREG a,MEM64 b){return x86(821,a,b);}
int pfrcpit1(MMREG a,R_M64 b){return x86(821,a,b);}
int pfrcpit2(MMREG a,MMREG b){return x86(822,a,b);}
int pfrcpit2(MMREG a,MEM64 b){return x86(822,a,b);}
int pfrcpit2(MMREG a,R_M64 b){return x86(822,a,b);}
int pfrsqit1(MMREG a,MMREG b){return x86(823,a,b);}
int pfrsqit1(MMREG a,MEM64 b){return x86(823,a,b);}
int pfrsqit1(MMREG a,R_M64 b){return x86(823,a,b);}
int pfrsqrt(MMREG a,MMREG b){return x86(824,a,b);}
int pfrsqrt(MMREG a,MEM64 b){return x86(824,a,b);}
int pfrsqrt(MMREG a,R_M64 b){return x86(824,a,b);}
int pfsub(MMREG a,MMREG b){return x86(825,a,b);}
int pfsub(MMREG a,MEM64 b){return x86(825,a,b);}
int pfsub(MMREG a,R_M64 b){return x86(825,a,b);}
int pfsubr(MMREG a,MMREG b){return x86(826,a,b);}
int pfsubr(MMREG a,MEM64 b){return x86(826,a,b);}
int pfsubr(MMREG a,R_M64 b){return x86(826,a,b);}
int pi2fd(MMREG a,MMREG b){return x86(827,a,b);}
int pi2fd(MMREG a,MEM64 b){return x86(827,a,b);}
int pi2fd(MMREG a,R_M64 b){return x86(827,a,b);}
int pi2fw(MMREG a,MMREG b){return x86(828,a,b);}
int pi2fw(MMREG a,MEM64 b){return x86(828,a,b);}
int pi2fw(MMREG a,R_M64 b){return x86(828,a,b);}
int pinsrw(MMREG a,AX b,char c){return x86(829,a,b,(IMM)c);}
int pinsrw(MMREG a,DX b,char c){return x86(829,a,b,(IMM)c);}
int pinsrw(MMREG a,CX b,char c){return x86(829,a,b,(IMM)c);}
int pinsrw(MMREG a,REG16 b,char c){return x86(829,a,b,(IMM)c);}
int pinsrw(MMREG a,MEM16 b,char c){return x86(829,a,b,(IMM)c);}
int pinsrw(MMREG a,R_M16 b,char c){return x86(829,a,b,(IMM)c);}
int pinsrw(XMMREG a,AX b,char c){return x86(830,a,b,(IMM)c);}
int pinsrw(XMMREG a,DX b,char c){return x86(830,a,b,(IMM)c);}
int pinsrw(XMMREG a,CX b,char c){return x86(830,a,b,(IMM)c);}
int pinsrw(XMMREG a,REG16 b,char c){return x86(830,a,b,(IMM)c);}
int pinsrw(XMMREG a,MEM16 b,char c){return x86(830,a,b,(IMM)c);}
int pinsrw(XMMREG a,R_M16 b,char c){return x86(830,a,b,(IMM)c);}
int pmachriw(MMREG a,MEM64 b){return x86(831,a,b);}
int pmaddwd(MMREG a,MMREG b){return x86(832,a,b);}
int pmaddwd(MMREG a,MEM64 b){return x86(832,a,b);}
int pmaddwd(MMREG a,R_M64 b){return x86(832,a,b);}
int pmaddwd(XMMREG a,XMMREG b){return x86(833,a,b);}
int pmaddwd(XMMREG a,MEM128 b){return x86(833,a,b);}
int pmaddwd(XMMREG a,R_M128 b){return x86(833,a,b);}
int pmagw(MMREG a,MMREG b){return x86(834,a,b);}
int pmagw(MMREG a,MEM64 b){return x86(834,a,b);}
int pmagw(MMREG a,R_M64 b){return x86(834,a,b);}
int pmaxsw(XMMREG a,XMMREG b){return x86(835,a,b);}
int pmaxsw(XMMREG a,MEM128 b){return x86(835,a,b);}
int pmaxsw(XMMREG a,R_M128 b){return x86(835,a,b);}
int pmaxsw(MMREG a,MMREG b){return x86(836,a,b);}
int pmaxsw(MMREG a,MEM64 b){return x86(836,a,b);}
int pmaxsw(MMREG a,R_M64 b){return x86(836,a,b);}
int pmaxub(MMREG a,MMREG b){return x86(837,a,b);}
int pmaxub(MMREG a,MEM64 b){return x86(837,a,b);}
int pmaxub(MMREG a,R_M64 b){return x86(837,a,b);}
int pmaxub(XMMREG a,XMMREG b){return x86(838,a,b);}
int pmaxub(XMMREG a,MEM128 b){return x86(838,a,b);}
int pmaxub(XMMREG a,R_M128 b){return x86(838,a,b);}
int pminsw(MMREG a,MMREG b){return x86(839,a,b);}
int pminsw(MMREG a,MEM64 b){return x86(839,a,b);}
int pminsw(MMREG a,R_M64 b){return x86(839,a,b);}
int pminsw(XMMREG a,XMMREG b){return x86(840,a,b);}
int pminsw(XMMREG a,MEM128 b){return x86(840,a,b);}
int pminsw(XMMREG a,R_M128 b){return x86(840,a,b);}
int pminub(MMREG a,MMREG b){return x86(841,a,b);}
int pminub(MMREG a,MEM64 b){return x86(841,a,b);}
int pminub(MMREG a,R_M64 b){return x86(841,a,b);}
int pminub(XMMREG a,XMMREG b){return x86(842,a,b);}
int pminub(XMMREG a,MEM128 b){return x86(842,a,b);}
int pminub(XMMREG a,R_M128 b){return x86(842,a,b);}
int pmovmskb(EAX a,MMREG b){return x86(843,a,b);}
int pmovmskb(ECX a,MMREG b){return x86(843,a,b);}
int pmovmskb(REG32 a,MMREG b){return x86(843,a,b);}
int pmovmskb(EAX a,XMMREG b){return x86(844,a,b);}
int pmovmskb(ECX a,XMMREG b){return x86(844,a,b);}
int pmovmskb(REG32 a,XMMREG b){return x86(844,a,b);}
int pmulhrwa(MMREG a,MMREG b){return x86(845,a,b);}
int pmulhrwa(MMREG a,MEM64 b){return x86(845,a,b);}
int pmulhrwa(MMREG a,R_M64 b){return x86(845,a,b);}
int pmulhrwc(MMREG a,MMREG b){return x86(846,a,b);}
int pmulhrwc(MMREG a,MEM64 b){return x86(846,a,b);}
int pmulhrwc(MMREG a,R_M64 b){return x86(846,a,b);}
int pmulhriw(MMREG a,MMREG b){return x86(847,a,b);}
int pmulhriw(MMREG a,MEM64 b){return x86(847,a,b);}
int pmulhriw(MMREG a,R_M64 b){return x86(847,a,b);}
int pmulhuw(MMREG a,MMREG b){return x86(848,a,b);}
int pmulhuw(MMREG a,MEM64 b){return x86(848,a,b);}
int pmulhuw(MMREG a,R_M64 b){return x86(848,a,b);}
int pmulhuw(XMMREG a,XMMREG b){return x86(849,a,b);}
int pmulhuw(XMMREG a,MEM128 b){return x86(849,a,b);}
int pmulhuw(XMMREG a,R_M128 b){return x86(849,a,b);}
int pmulhw(MMREG a,MMREG b){return x86(850,a,b);}
int pmulhw(MMREG a,MEM64 b){return x86(850,a,b);}
int pmulhw(MMREG a,R_M64 b){return x86(850,a,b);}
int pmullw(MMREG a,MMREG b){return x86(851,a,b);}
int pmullw(MMREG a,MEM64 b){return x86(851,a,b);}
int pmullw(MMREG a,R_M64 b){return x86(851,a,b);}
int pmulhw(XMMREG a,XMMREG b){return x86(852,a,b);}
int pmulhw(XMMREG a,MEM128 b){return x86(852,a,b);}
int pmulhw(XMMREG a,R_M128 b){return x86(852,a,b);}
int pmullw(XMMREG a,XMMREG b){return x86(853,a,b);}
int pmullw(XMMREG a,MEM128 b){return x86(853,a,b);}
int pmullw(XMMREG a,R_M128 b){return x86(853,a,b);}
int pmuludq(MMREG a,MMREG b){return x86(854,a,b);}
int pmuludq(MMREG a,MEM64 b){return x86(854,a,b);}
int pmuludq(MMREG a,R_M64 b){return x86(854,a,b);}
int pmuludq(XMMREG a,XMMREG b){return x86(855,a,b);}
int pmuludq(XMMREG a,MEM128 b){return x86(855,a,b);}
int pmuludq(XMMREG a,R_M128 b){return x86(855,a,b);}
int pmvzb(MMREG a,MEM64 b){return x86(856,a,b);}
int pmvnzb(MMREG a,MEM64 b){return x86(857,a,b);}
int pmvlzb(MMREG a,MEM64 b){return x86(858,a,b);}
int pmvgezb(MMREG a,MEM64 b){return x86(859,a,b);}
int pop(AX a){return x86(860,a);}
int pop(DX a){return x86(860,a);}
int pop(CX a){return x86(860,a);}
int pop(REG16 a){return x86(860,a);}
int pop(EAX a){return x86(861,a);}
int pop(ECX a){return x86(861,a);}
int pop(REG32 a){return x86(861,a);}
int pop(MEM16 a){return x86(862,a);}
int pop(R_M16 a){return x86(862,a);}
int pop(MEM32 a){return x86(863,a);}
int pop(R_M32 a){return x86(863,a);}
int popa(){return x86(864);}
int popaw(){return x86(865);}
int popad(){return x86(866);}
int popf(){return x86(867);}
int popfw(){return x86(868);}
int popfd(){return x86(869);}
int por(MMREG a,MMREG b){return x86(870,a,b);}
int por(MMREG a,MEM64 b){return x86(870,a,b);}
int por(MMREG a,R_M64 b){return x86(870,a,b);}
int por(XMMREG a,XMMREG b){return x86(871,a,b);}
int por(XMMREG a,MEM128 b){return x86(871,a,b);}
int por(XMMREG a,R_M128 b){return x86(871,a,b);}
int prefetch(MEM8 a){return x86(872,a);}
int prefetch(MEM16 a){return x86(872,a);}
int prefetch(MEM32 a){return x86(872,a);}
int prefetch(MEM64 a){return x86(872,a);}
int prefetch(MEM128 a){return x86(872,a);}
int prefetchw(MEM8 a){return x86(873,a);}
int prefetchw(MEM16 a){return x86(873,a);}
int prefetchw(MEM32 a){return x86(873,a);}
int prefetchw(MEM64 a){return x86(873,a);}
int prefetchw(MEM128 a){return x86(873,a);}
int prefetchnta(MEM8 a){return x86(874,a);}
int prefetchnta(MEM16 a){return x86(874,a);}
int prefetchnta(MEM32 a){return x86(874,a);}
int prefetchnta(MEM64 a){return x86(874,a);}
int prefetchnta(MEM128 a){return x86(874,a);}
int prefetcht0(MEM8 a){return x86(875,a);}
int prefetcht0(MEM16 a){return x86(875,a);}
int prefetcht0(MEM32 a){return x86(875,a);}
int prefetcht0(MEM64 a){return x86(875,a);}
int prefetcht0(MEM128 a){return x86(875,a);}
int prefetcht1(MEM8 a){return x86(876,a);}
int prefetcht1(MEM16 a){return x86(876,a);}
int prefetcht1(MEM32 a){return x86(876,a);}
int prefetcht1(MEM64 a){return x86(876,a);}
int prefetcht1(MEM128 a){return x86(876,a);}
int prefetcht2(MEM8 a){return x86(877,a);}
int prefetcht2(MEM16 a){return x86(877,a);}
int prefetcht2(MEM32 a){return x86(877,a);}
int prefetcht2(MEM64 a){return x86(877,a);}
int prefetcht2(MEM128 a){return x86(877,a);}
int psadbw(MMREG a,MMREG b){return x86(878,a,b);}
int psadbw(MMREG a,MEM64 b){return x86(878,a,b);}
int psadbw(MMREG a,R_M64 b){return x86(878,a,b);}
int psadbw(XMMREG a,XMMREG b){return x86(879,a,b);}
int psadbw(XMMREG a,MEM128 b){return x86(879,a,b);}
int psadbw(XMMREG a,R_M128 b){return x86(879,a,b);}
int pshufd(XMMREG a,XMMREG b,char c){return x86(880,a,b,(IMM)c);}
int pshufd(XMMREG a,MEM128 b,char c){return x86(880,a,b,(IMM)c);}
int pshufd(XMMREG a,R_M128 b,char c){return x86(880,a,b,(IMM)c);}
int pshufhw(XMMREG a,XMMREG b,char c){return x86(881,a,b,(IMM)c);}
int pshufhw(XMMREG a,MEM128 b,char c){return x86(881,a,b,(IMM)c);}
int pshufhw(XMMREG a,R_M128 b,char c){return x86(881,a,b,(IMM)c);}
int pshuflw(XMMREG a,XMMREG b,char c){return x86(882,a,b,(IMM)c);}
int pshuflw(XMMREG a,MEM128 b,char c){return x86(882,a,b,(IMM)c);}
int pshuflw(XMMREG a,R_M128 b,char c){return x86(882,a,b,(IMM)c);}
int pshufw(MMREG a,MMREG b,char c){return x86(883,a,b,(IMM)c);}
int pshufw(MMREG a,MEM64 b,char c){return x86(883,a,b,(IMM)c);}
int pshufw(MMREG a,R_M64 b,char c){return x86(883,a,b,(IMM)c);}
int psllw(MMREG a,MMREG b){return x86(884,a,b);}
int psllw(MMREG a,MEM64 b){return x86(884,a,b);}
int psllw(MMREG a,R_M64 b){return x86(884,a,b);}
int psllw(MMREG a,char b){return x86(885,a,(IMM)b);}
int psllw(XMMREG a,XMMREG b){return x86(886,a,b);}
int psllw(XMMREG a,MEM128 b){return x86(886,a,b);}
int psllw(XMMREG a,R_M128 b){return x86(886,a,b);}
int psllw(XMMREG a,char b){return x86(887,a,(IMM)b);}
int pslld(MMREG a,MMREG b){return x86(888,a,b);}
int pslld(MMREG a,MEM64 b){return x86(888,a,b);}
int pslld(MMREG a,R_M64 b){return x86(888,a,b);}
int pslld(MMREG a,char b){return x86(889,a,(IMM)b);}
int pslld(XMMREG a,XMMREG b){return x86(890,a,b);}
int pslld(XMMREG a,MEM128 b){return x86(890,a,b);}
int pslld(XMMREG a,R_M128 b){return x86(890,a,b);}
int pslld(XMMREG a,char b){return x86(891,a,(IMM)b);}
int psllq(MMREG a,MMREG b){return x86(892,a,b);}
int psllq(MMREG a,MEM64 b){return x86(892,a,b);}
int psllq(MMREG a,R_M64 b){return x86(892,a,b);}
int psllq(MMREG a,char b){return x86(893,a,(IMM)b);}
int psllq(XMMREG a,XMMREG b){return x86(894,a,b);}
int psllq(XMMREG a,MEM128 b){return x86(894,a,b);}
int psllq(XMMREG a,R_M128 b){return x86(894,a,b);}
int psllq(XMMREG a,char b){return x86(895,a,(IMM)b);}
int psraw(MMREG a,MMREG b){return x86(896,a,b);}
int psraw(MMREG a,MEM64 b){return x86(896,a,b);}
int psraw(MMREG a,R_M64 b){return x86(896,a,b);}
int psraw(MMREG a,char b){return x86(897,a,(IMM)b);}
int psraw(XMMREG a,XMMREG b){return x86(898,a,b);}
int psraw(XMMREG a,MEM128 b){return x86(898,a,b);}
int psraw(XMMREG a,R_M128 b){return x86(898,a,b);}
int psraw(XMMREG a,char b){return x86(899,a,(IMM)b);}
int psrad(MMREG a,MMREG b){return x86(900,a,b);}
int psrad(MMREG a,MEM64 b){return x86(900,a,b);}
int psrad(MMREG a,R_M64 b){return x86(900,a,b);}
int psrad(MMREG a,char b){return x86(901,a,(IMM)b);}
int psrad(XMMREG a,XMMREG b){return x86(902,a,b);}
int psrad(XMMREG a,MEM128 b){return x86(902,a,b);}
int psrad(XMMREG a,R_M128 b){return x86(902,a,b);}
int psrad(XMMREG a,char b){return x86(903,a,(IMM)b);}
int psrlw(MMREG a,MMREG b){return x86(904,a,b);}
int psrlw(MMREG a,MEM64 b){return x86(904,a,b);}
int psrlw(MMREG a,R_M64 b){return x86(904,a,b);}
int psrlw(MMREG a,char b){return x86(905,a,(IMM)b);}
int psrlw(XMMREG a,XMMREG b){return x86(906,a,b);}
int psrlw(XMMREG a,MEM128 b){return x86(906,a,b);}
int psrlw(XMMREG a,R_M128 b){return x86(906,a,b);}
int psrlw(XMMREG a,char b){return x86(907,a,(IMM)b);}
int psrld(MMREG a,MMREG b){return x86(908,a,b);}
int psrld(MMREG a,MEM64 b){return x86(908,a,b);}
int psrld(MMREG a,R_M64 b){return x86(908,a,b);}
int psrld(MMREG a,char b){return x86(909,a,(IMM)b);}
int psrld(XMMREG a,XMMREG b){return x86(910,a,b);}
int psrld(XMMREG a,MEM128 b){return x86(910,a,b);}
int psrld(XMMREG a,R_M128 b){return x86(910,a,b);}
int psrld(XMMREG a,char b){return x86(911,a,(IMM)b);}
int psrlq(MMREG a,MMREG b){return x86(912,a,b);}
int psrlq(MMREG a,MEM64 b){return x86(912,a,b);}
int psrlq(MMREG a,R_M64 b){return x86(912,a,b);}
int psrlq(MMREG a,char b){return x86(913,a,(IMM)b);}
int psrlq(XMMREG a,XMMREG b){return x86(914,a,b);}
int psrlq(XMMREG a,MEM128 b){return x86(914,a,b);}
int psrlq(XMMREG a,R_M128 b){return x86(914,a,b);}
int psrlq(XMMREG a,char b){return x86(915,a,(IMM)b);}
int psrldq(XMMREG a,char b){return x86(916,a,(IMM)b);}
int psubb(MMREG a,MMREG b){return x86(917,a,b);}
int psubb(MMREG a,MEM64 b){return x86(917,a,b);}
int psubb(MMREG a,R_M64 b){return x86(917,a,b);}
int psubw(MMREG a,MMREG b){return x86(918,a,b);}
int psubw(MMREG a,MEM64 b){return x86(918,a,b);}
int psubw(MMREG a,R_M64 b){return x86(918,a,b);}
int psubd(MMREG a,MMREG b){return x86(919,a,b);}
int psubd(MMREG a,MEM64 b){return x86(919,a,b);}
int psubd(MMREG a,R_M64 b){return x86(919,a,b);}
int psubq(MMREG a,MMREG b){return x86(920,a,b);}
int psubq(MMREG a,MEM64 b){return x86(920,a,b);}
int psubq(MMREG a,R_M64 b){return x86(920,a,b);}
int psubb(XMMREG a,XMMREG b){return x86(921,a,b);}
int psubb(XMMREG a,MEM128 b){return x86(921,a,b);}
int psubb(XMMREG a,R_M128 b){return x86(921,a,b);}
int psubw(XMMREG a,XMMREG b){return x86(922,a,b);}
int psubw(XMMREG a,MEM128 b){return x86(922,a,b);}
int psubw(XMMREG a,R_M128 b){return x86(922,a,b);}
int psubd(XMMREG a,XMMREG b){return x86(923,a,b);}
int psubd(XMMREG a,MEM128 b){return x86(923,a,b);}
int psubd(XMMREG a,R_M128 b){return x86(923,a,b);}
int psubq(XMMREG a,XMMREG b){return x86(924,a,b);}
int psubq(XMMREG a,MEM128 b){return x86(924,a,b);}
int psubq(XMMREG a,R_M128 b){return x86(924,a,b);}
int psubsb(MMREG a,MMREG b){return x86(925,a,b);}
int psubsb(MMREG a,MEM64 b){return x86(925,a,b);}
int psubsb(MMREG a,R_M64 b){return x86(925,a,b);}
int psubsw(MMREG a,MMREG b){return x86(926,a,b);}
int psubsw(MMREG a,MEM64 b){return x86(926,a,b);}
int psubsw(MMREG a,R_M64 b){return x86(926,a,b);}
int psubsb(XMMREG a,XMMREG b){return x86(927,a,b);}
int psubsb(XMMREG a,MEM128 b){return x86(927,a,b);}
int psubsb(XMMREG a,R_M128 b){return x86(927,a,b);}
int psubsw(XMMREG a,XMMREG b){return x86(928,a,b);}
int psubsw(XMMREG a,MEM128 b){return x86(928,a,b);}
int psubsw(XMMREG a,R_M128 b){return x86(928,a,b);}
int psubusb(MMREG a,MMREG b){return x86(929,a,b);}
int psubusb(MMREG a,MEM64 b){return x86(929,a,b);}
int psubusb(MMREG a,R_M64 b){return x86(929,a,b);}
int psubusw(MMREG a,MMREG b){return x86(930,a,b);}
int psubusw(MMREG a,MEM64 b){return x86(930,a,b);}
int psubusw(MMREG a,R_M64 b){return x86(930,a,b);}
int psubusb(XMMREG a,XMMREG b){return x86(931,a,b);}
int psubusb(XMMREG a,MEM128 b){return x86(931,a,b);}
int psubusb(XMMREG a,R_M128 b){return x86(931,a,b);}
int psubusw(XMMREG a,XMMREG b){return x86(932,a,b);}
int psubusw(XMMREG a,MEM128 b){return x86(932,a,b);}
int psubusw(XMMREG a,R_M128 b){return x86(932,a,b);}
int psubsiw(MMREG a,MMREG b){return x86(933,a,b);}
int psubsiw(MMREG a,MEM64 b){return x86(933,a,b);}
int psubsiw(MMREG a,R_M64 b){return x86(933,a,b);}
int pswapd(MMREG a,MMREG b){return x86(934,a,b);}
int pswapd(MMREG a,MEM64 b){return x86(934,a,b);}
int pswapd(MMREG a,R_M64 b){return x86(934,a,b);}
int punpckhbw(MMREG a,MMREG b){return x86(935,a,b);}
int punpckhbw(MMREG a,MEM64 b){return x86(935,a,b);}
int punpckhbw(MMREG a,R_M64 b){return x86(935,a,b);}
int punpckhwd(MMREG a,MMREG b){return x86(936,a,b);}
int punpckhwd(MMREG a,MEM64 b){return x86(936,a,b);}
int punpckhwd(MMREG a,R_M64 b){return x86(936,a,b);}
int punpckhdq(MMREG a,MMREG b){return x86(937,a,b);}
int punpckhdq(MMREG a,MEM64 b){return x86(937,a,b);}
int punpckhdq(MMREG a,R_M64 b){return x86(937,a,b);}
int punpckhbw(XMMREG a,XMMREG b){return x86(938,a,b);}
int punpckhbw(XMMREG a,MEM128 b){return x86(938,a,b);}
int punpckhbw(XMMREG a,R_M128 b){return x86(938,a,b);}
int punpckhwd(XMMREG a,XMMREG b){return x86(939,a,b);}
int punpckhwd(XMMREG a,MEM128 b){return x86(939,a,b);}
int punpckhwd(XMMREG a,R_M128 b){return x86(939,a,b);}
int punpckhdq(XMMREG a,XMMREG b){return x86(940,a,b);}
int punpckhdq(XMMREG a,MEM128 b){return x86(940,a,b);}
int punpckhdq(XMMREG a,R_M128 b){return x86(940,a,b);}
int punpckhqdq(XMMREG a,XMMREG b){return x86(941,a,b);}
int punpckhqdq(XMMREG a,MEM128 b){return x86(941,a,b);}
int punpckhqdq(XMMREG a,R_M128 b){return x86(941,a,b);}
int punpcklbw(MMREG a,MMREG b){return x86(942,a,b);}
int punpcklbw(MMREG a,MEM64 b){return x86(942,a,b);}
int punpcklbw(MMREG a,R_M64 b){return x86(942,a,b);}
int punpcklwd(MMREG a,MMREG b){return x86(943,a,b);}
int punpcklwd(MMREG a,MEM64 b){return x86(943,a,b);}
int punpcklwd(MMREG a,R_M64 b){return x86(943,a,b);}
int punpckldq(MMREG a,MMREG b){return x86(944,a,b);}
int punpckldq(MMREG a,MEM64 b){return x86(944,a,b);}
int punpckldq(MMREG a,R_M64 b){return x86(944,a,b);}
int punpcklbw(XMMREG a,XMMREG b){return x86(945,a,b);}
int punpcklbw(XMMREG a,MEM128 b){return x86(945,a,b);}
int punpcklbw(XMMREG a,R_M128 b){return x86(945,a,b);}
int punpcklwd(XMMREG a,XMMREG b){return x86(946,a,b);}
int punpcklwd(XMMREG a,MEM128 b){return x86(946,a,b);}
int punpcklwd(XMMREG a,R_M128 b){return x86(946,a,b);}
int punpckldq(XMMREG a,XMMREG b){return x86(947,a,b);}
int punpckldq(XMMREG a,MEM128 b){return x86(947,a,b);}
int punpckldq(XMMREG a,R_M128 b){return x86(947,a,b);}
int punpcklqdq(XMMREG a,XMMREG b){return x86(948,a,b);}
int punpcklqdq(XMMREG a,MEM128 b){return x86(948,a,b);}
int punpcklqdq(XMMREG a,R_M128 b){return x86(948,a,b);}
int push(AX a){return x86(949,a);}
int push(DX a){return x86(949,a);}
int push(CX a){return x86(949,a);}
int push(REG16 a){return x86(949,a);}
int push(EAX a){return x86(950,a);}
int push(ECX a){return x86(950,a);}
int push(REG32 a){return x86(950,a);}
int push(MEM16 a){return x86(951,a);}
int push(R_M16 a){return x86(951,a);}
int push(MEM32 a){return x86(952,a);}
int push(R_M32 a){return x86(952,a);}
int push(char a){return x86(953,(IMM)a);}
int push(short a){return x86(954,(IMM)a);}
int push(int a){return x86(955,(IMM)a);}
int push(REF a){return x86(955,a);}
int pusha(){return x86(956);}
int pushad(){return x86(957);}
int pushaw(){return x86(958);}
int pushf(){return x86(959);}
int pushfd(){return x86(960);}
int pushfw(){return x86(961);}
int pxor(MMREG a,MMREG b){return x86(962,a,b);}
int pxor(MMREG a,MEM64 b){return x86(962,a,b);}
int pxor(MMREG a,R_M64 b){return x86(962,a,b);}
int pxor(XMMREG a,XMMREG b){return x86(963,a,b);}
int pxor(XMMREG a,MEM128 b){return x86(963,a,b);}
int pxor(XMMREG a,R_M128 b){return x86(963,a,b);}
int rcl(AL a,CL b){return x86(965,a,b);}
int rcl(CL a,CL b){return x86(965,a,b);}
int rcl(REG8 a,CL b){return x86(965,a,b);}
int rcl(MEM8 a,CL b){return x86(965,a,b);}
int rcl(R_M8 a,CL b){return x86(965,a,b);}
int rcl(AL a,char b){return x86(966,a,(IMM)b);}
int rcl(CL a,char b){return x86(966,a,(IMM)b);}
int rcl(REG8 a,char b){return x86(966,a,(IMM)b);}
int rcl(MEM8 a,char b){return x86(966,a,(IMM)b);}
int rcl(R_M8 a,char b){return x86(966,a,(IMM)b);}
int rcl(AX a,CL b){return x86(968,a,b);}
int rcl(DX a,CL b){return x86(968,a,b);}
int rcl(CX a,CL b){return x86(968,a,b);}
int rcl(REG16 a,CL b){return x86(968,a,b);}
int rcl(MEM16 a,CL b){return x86(968,a,b);}
int rcl(R_M16 a,CL b){return x86(968,a,b);}
int rcl(EAX a,CL b){return x86(971,a,b);}
int rcl(ECX a,CL b){return x86(971,a,b);}
int rcl(REG32 a,CL b){return x86(971,a,b);}
int rcl(MEM32 a,CL b){return x86(971,a,b);}
int rcl(R_M32 a,CL b){return x86(971,a,b);}
int rcr(AL a,CL b){return x86(974,a,b);}
int rcr(CL a,CL b){return x86(974,a,b);}
int rcr(REG8 a,CL b){return x86(974,a,b);}
int rcr(MEM8 a,CL b){return x86(974,a,b);}
int rcr(R_M8 a,CL b){return x86(974,a,b);}
int rcr(AL a,char b){return x86(975,a,(IMM)b);}
int rcr(CL a,char b){return x86(975,a,(IMM)b);}
int rcr(REG8 a,char b){return x86(975,a,(IMM)b);}
int rcr(MEM8 a,char b){return x86(975,a,(IMM)b);}
int rcr(R_M8 a,char b){return x86(975,a,(IMM)b);}
int rcr(AX a,CL b){return x86(977,a,b);}
int rcr(DX a,CL b){return x86(977,a,b);}
int rcr(CX a,CL b){return x86(977,a,b);}
int rcr(REG16 a,CL b){return x86(977,a,b);}
int rcr(MEM16 a,CL b){return x86(977,a,b);}
int rcr(R_M16 a,CL b){return x86(977,a,b);}
int rcr(EAX a,CL b){return x86(980,a,b);}
int rcr(ECX a,CL b){return x86(980,a,b);}
int rcr(REG32 a,CL b){return x86(980,a,b);}
int rcr(MEM32 a,CL b){return x86(980,a,b);}
int rcr(R_M32 a,CL b){return x86(980,a,b);}
int rcpps(XMMREG a,XMMREG b){return x86(982,a,b);}
int rcpps(XMMREG a,MEM128 b){return x86(982,a,b);}
int rcpps(XMMREG a,R_M128 b){return x86(982,a,b);}
int rcpss(XMMREG a,XMMREG b){return x86(983,a,b);}
int rcpss(XMMREG a,MEM32 b){return x86(983,a,b);}
int rcpss(XMMREG a,XMM32 b){return x86(983,a,b);}
int rdmsr(){return x86(984);}
int rdpmc(){return x86(985);}
int rdtsc(){return x86(986);}
int ret(){return x86(987);}
int ret(char a){return x86(988,(IMM)a);}
int ret(short a){return x86(988,(IMM)a);}
int retf(){return x86(989);}
int retf(char a){return x86(990,(IMM)a);}
int retf(short a){return x86(990,(IMM)a);}
int retn(){return x86(991);}
int retn(char a){return x86(992,(IMM)a);}
int retn(short a){return x86(992,(IMM)a);}
int rol(AL a,CL b){return x86(994,a,b);}
int rol(CL a,CL b){return x86(994,a,b);}
int rol(REG8 a,CL b){return x86(994,a,b);}
int rol(MEM8 a,CL b){return x86(994,a,b);}
int rol(R_M8 a,CL b){return x86(994,a,b);}
int rol(AL a,char b){return x86(995,a,(IMM)b);}
int rol(CL a,char b){return x86(995,a,(IMM)b);}
int rol(REG8 a,char b){return x86(995,a,(IMM)b);}
int rol(MEM8 a,char b){return x86(995,a,(IMM)b);}
int rol(R_M8 a,char b){return x86(995,a,(IMM)b);}
int rol(AX a,CL b){return x86(997,a,b);}
int rol(DX a,CL b){return x86(997,a,b);}
int rol(CX a,CL b){return x86(997,a,b);}
int rol(REG16 a,CL b){return x86(997,a,b);}
int rol(MEM16 a,CL b){return x86(997,a,b);}
int rol(R_M16 a,CL b){return x86(997,a,b);}
int rol(EAX a,CL b){return x86(1000,a,b);}
int rol(ECX a,CL b){return x86(1000,a,b);}
int rol(REG32 a,CL b){return x86(1000,a,b);}
int rol(MEM32 a,CL b){return x86(1000,a,b);}
int rol(R_M32 a,CL b){return x86(1000,a,b);}
int ror(AL a,CL b){return x86(1003,a,b);}
int ror(CL a,CL b){return x86(1003,a,b);}
int ror(REG8 a,CL b){return x86(1003,a,b);}
int ror(MEM8 a,CL b){return x86(1003,a,b);}
int ror(R_M8 a,CL b){return x86(1003,a,b);}
int ror(AL a,char b){return x86(1004,a,(IMM)b);}
int ror(CL a,char b){return x86(1004,a,(IMM)b);}
int ror(REG8 a,char b){return x86(1004,a,(IMM)b);}
int ror(MEM8 a,char b){return x86(1004,a,(IMM)b);}
int ror(R_M8 a,char b){return x86(1004,a,(IMM)b);}
int ror(AX a,CL b){return x86(1006,a,b);}
int ror(DX a,CL b){return x86(1006,a,b);}
int ror(CX a,CL b){return x86(1006,a,b);}
int ror(REG16 a,CL b){return x86(1006,a,b);}
int ror(MEM16 a,CL b){return x86(1006,a,b);}
int ror(R_M16 a,CL b){return x86(1006,a,b);}
int ror(EAX a,CL b){return x86(1009,a,b);}
int ror(ECX a,CL b){return x86(1009,a,b);}
int ror(REG32 a,CL b){return x86(1009,a,b);}
int ror(MEM32 a,CL b){return x86(1009,a,b);}
int ror(R_M32 a,CL b){return x86(1009,a,b);}
int rsm(){return x86(1011);}
int rsqrtps(XMMREG a,XMMREG b){return x86(1012,a,b);}
int rsqrtps(XMMREG a,MEM128 b){return x86(1012,a,b);}
int rsqrtps(XMMREG a,R_M128 b){return x86(1012,a,b);}
int rsqrtss(XMMREG a,XMMREG b){return x86(1013,a,b);}
int rsqrtss(XMMREG a,MEM32 b){return x86(1013,a,b);}
int rsqrtss(XMMREG a,XMM32 b){return x86(1013,a,b);}
int sahf(){return x86(1014);}
int sal(AL a,CL b){return x86(1016,a,b);}
int sal(CL a,CL b){return x86(1016,a,b);}
int sal(REG8 a,CL b){return x86(1016,a,b);}
int sal(MEM8 a,CL b){return x86(1016,a,b);}
int sal(R_M8 a,CL b){return x86(1016,a,b);}
int sal(AL a,char b){return x86(1017,a,(IMM)b);}
int sal(CL a,char b){return x86(1017,a,(IMM)b);}
int sal(REG8 a,char b){return x86(1017,a,(IMM)b);}
int sal(MEM8 a,char b){return x86(1017,a,(IMM)b);}
int sal(R_M8 a,char b){return x86(1017,a,(IMM)b);}
int sal(AX a,CL b){return x86(1019,a,b);}
int sal(DX a,CL b){return x86(1019,a,b);}
int sal(CX a,CL b){return x86(1019,a,b);}
int sal(REG16 a,CL b){return x86(1019,a,b);}
int sal(MEM16 a,CL b){return x86(1019,a,b);}
int sal(R_M16 a,CL b){return x86(1019,a,b);}
int sal(EAX a,CL b){return x86(1022,a,b);}
int sal(ECX a,CL b){return x86(1022,a,b);}
int sal(REG32 a,CL b){return x86(1022,a,b);}
int sal(MEM32 a,CL b){return x86(1022,a,b);}
int sal(R_M32 a,CL b){return x86(1022,a,b);}
int sar(AL a,CL b){return x86(1025,a,b);}
int sar(CL a,CL b){return x86(1025,a,b);}
int sar(REG8 a,CL b){return x86(1025,a,b);}
int sar(MEM8 a,CL b){return x86(1025,a,b);}
int sar(R_M8 a,CL b){return x86(1025,a,b);}
int sar(AL a,char b){return x86(1026,a,(IMM)b);}
int sar(CL a,char b){return x86(1026,a,(IMM)b);}
int sar(REG8 a,char b){return x86(1026,a,(IMM)b);}
int sar(MEM8 a,char b){return x86(1026,a,(IMM)b);}
int sar(R_M8 a,char b){return x86(1026,a,(IMM)b);}
int sar(AX a,CL b){return x86(1028,a,b);}
int sar(DX a,CL b){return x86(1028,a,b);}
int sar(CX a,CL b){return x86(1028,a,b);}
int sar(REG16 a,CL b){return x86(1028,a,b);}
int sar(MEM16 a,CL b){return x86(1028,a,b);}
int sar(R_M16 a,CL b){return x86(1028,a,b);}
int sar(EAX a,CL b){return x86(1031,a,b);}
int sar(ECX a,CL b){return x86(1031,a,b);}
int sar(REG32 a,CL b){return x86(1031,a,b);}
int sar(MEM32 a,CL b){return x86(1031,a,b);}
int sar(R_M32 a,CL b){return x86(1031,a,b);}
int sbb(AL a,AL b){return x86(1033,a,b);}
int sbb(AL a,CL b){return x86(1033,a,b);}
int sbb(AL a,REG8 b){return x86(1033,a,b);}
int sbb(CL a,AL b){return x86(1033,a,b);}
int sbb(CL a,CL b){return x86(1033,a,b);}
int sbb(CL a,REG8 b){return x86(1033,a,b);}
int sbb(REG8 a,AL b){return x86(1033,a,b);}
int sbb(REG8 a,CL b){return x86(1033,a,b);}
int sbb(REG8 a,REG8 b){return x86(1033,a,b);}
int sbb(MEM8 a,AL b){return x86(1033,a,b);}
int sbb(MEM8 a,CL b){return x86(1033,a,b);}
int sbb(MEM8 a,REG8 b){return x86(1033,a,b);}
int sbb(R_M8 a,AL b){return x86(1033,a,b);}
int sbb(R_M8 a,CL b){return x86(1033,a,b);}
int sbb(R_M8 a,REG8 b){return x86(1033,a,b);}
int sbb(AX a,AX b){return x86(1034,a,b);}
int sbb(AX a,DX b){return x86(1034,a,b);}
int sbb(AX a,CX b){return x86(1034,a,b);}
int sbb(AX a,REG16 b){return x86(1034,a,b);}
int sbb(DX a,AX b){return x86(1034,a,b);}
int sbb(DX a,DX b){return x86(1034,a,b);}
int sbb(DX a,CX b){return x86(1034,a,b);}
int sbb(DX a,REG16 b){return x86(1034,a,b);}
int sbb(CX a,AX b){return x86(1034,a,b);}
int sbb(CX a,DX b){return x86(1034,a,b);}
int sbb(CX a,CX b){return x86(1034,a,b);}
int sbb(CX a,REG16 b){return x86(1034,a,b);}
int sbb(REG16 a,AX b){return x86(1034,a,b);}
int sbb(REG16 a,DX b){return x86(1034,a,b);}
int sbb(REG16 a,CX b){return x86(1034,a,b);}
int sbb(REG16 a,REG16 b){return x86(1034,a,b);}
int sbb(MEM16 a,AX b){return x86(1034,a,b);}
int sbb(MEM16 a,DX b){return x86(1034,a,b);}
int sbb(MEM16 a,CX b){return x86(1034,a,b);}
int sbb(MEM16 a,REG16 b){return x86(1034,a,b);}
int sbb(R_M16 a,AX b){return x86(1034,a,b);}
int sbb(R_M16 a,DX b){return x86(1034,a,b);}
int sbb(R_M16 a,CX b){return x86(1034,a,b);}
int sbb(R_M16 a,REG16 b){return x86(1034,a,b);}
int sbb(EAX a,EAX b){return x86(1035,a,b);}
int sbb(EAX a,ECX b){return x86(1035,a,b);}
int sbb(EAX a,REG32 b){return x86(1035,a,b);}
int sbb(ECX a,EAX b){return x86(1035,a,b);}
int sbb(ECX a,ECX b){return x86(1035,a,b);}
int sbb(ECX a,REG32 b){return x86(1035,a,b);}
int sbb(REG32 a,EAX b){return x86(1035,a,b);}
int sbb(REG32 a,ECX b){return x86(1035,a,b);}
int sbb(REG32 a,REG32 b){return x86(1035,a,b);}
int sbb(MEM32 a,EAX b){return x86(1035,a,b);}
int sbb(MEM32 a,ECX b){return x86(1035,a,b);}
int sbb(MEM32 a,REG32 b){return x86(1035,a,b);}
int sbb(R_M32 a,EAX b){return x86(1035,a,b);}
int sbb(R_M32 a,ECX b){return x86(1035,a,b);}
int sbb(R_M32 a,REG32 b){return x86(1035,a,b);}
int lock_sbb(MEM8 a,AL b){return x86(1036,a,b);}
int lock_sbb(MEM8 a,CL b){return x86(1036,a,b);}
int lock_sbb(MEM8 a,REG8 b){return x86(1036,a,b);}
int lock_sbb(MEM16 a,AX b){return x86(1037,a,b);}
int lock_sbb(MEM16 a,DX b){return x86(1037,a,b);}
int lock_sbb(MEM16 a,CX b){return x86(1037,a,b);}
int lock_sbb(MEM16 a,REG16 b){return x86(1037,a,b);}
int lock_sbb(MEM32 a,EAX b){return x86(1038,a,b);}
int lock_sbb(MEM32 a,ECX b){return x86(1038,a,b);}
int lock_sbb(MEM32 a,REG32 b){return x86(1038,a,b);}
int sbb(AL a,MEM8 b){return x86(1039,a,b);}
int sbb(AL a,R_M8 b){return x86(1039,a,b);}
int sbb(CL a,MEM8 b){return x86(1039,a,b);}
int sbb(CL a,R_M8 b){return x86(1039,a,b);}
int sbb(REG8 a,MEM8 b){return x86(1039,a,b);}
int sbb(REG8 a,R_M8 b){return x86(1039,a,b);}
int sbb(AX a,MEM16 b){return x86(1040,a,b);}
int sbb(AX a,R_M16 b){return x86(1040,a,b);}
int sbb(DX a,MEM16 b){return x86(1040,a,b);}
int sbb(DX a,R_M16 b){return x86(1040,a,b);}
int sbb(CX a,MEM16 b){return x86(1040,a,b);}
int sbb(CX a,R_M16 b){return x86(1040,a,b);}
int sbb(REG16 a,MEM16 b){return x86(1040,a,b);}
int sbb(REG16 a,R_M16 b){return x86(1040,a,b);}
int sbb(EAX a,MEM32 b){return x86(1041,a,b);}
int sbb(EAX a,R_M32 b){return x86(1041,a,b);}
int sbb(ECX a,MEM32 b){return x86(1041,a,b);}
int sbb(ECX a,R_M32 b){return x86(1041,a,b);}
int sbb(REG32 a,MEM32 b){return x86(1041,a,b);}
int sbb(REG32 a,R_M32 b){return x86(1041,a,b);}
int sbb(AL a,char b){return x86(1042,a,(IMM)b);}
int sbb(CL a,char b){return x86(1042,a,(IMM)b);}
int sbb(REG8 a,char b){return x86(1042,a,(IMM)b);}
int sbb(MEM8 a,char b){return x86(1042,a,(IMM)b);}
int sbb(R_M8 a,char b){return x86(1042,a,(IMM)b);}
int sbb(AX a,char b){return x86(1043,a,(IMM)b);}
int sbb(AX a,short b){return x86(1043,a,(IMM)b);}
int sbb(DX a,char b){return x86(1043,a,(IMM)b);}
int sbb(DX a,short b){return x86(1043,a,(IMM)b);}
int sbb(CX a,char b){return x86(1043,a,(IMM)b);}
int sbb(CX a,short b){return x86(1043,a,(IMM)b);}
int sbb(REG16 a,char b){return x86(1043,a,(IMM)b);}
int sbb(REG16 a,short b){return x86(1043,a,(IMM)b);}
int sbb(MEM16 a,char b){return x86(1043,a,(IMM)b);}
int sbb(MEM16 a,short b){return x86(1043,a,(IMM)b);}
int sbb(R_M16 a,char b){return x86(1043,a,(IMM)b);}
int sbb(R_M16 a,short b){return x86(1043,a,(IMM)b);}
int sbb(EAX a,int b){return x86(1044,a,(IMM)b);}
int sbb(EAX a,char b){return x86(1044,a,(IMM)b);}
int sbb(EAX a,short b){return x86(1044,a,(IMM)b);}
int sbb(EAX a,REF b){return x86(1044,a,b);}
int sbb(ECX a,int b){return x86(1044,a,(IMM)b);}
int sbb(ECX a,char b){return x86(1044,a,(IMM)b);}
int sbb(ECX a,short b){return x86(1044,a,(IMM)b);}
int sbb(ECX a,REF b){return x86(1044,a,b);}
int sbb(REG32 a,int b){return x86(1044,a,(IMM)b);}
int sbb(REG32 a,char b){return x86(1044,a,(IMM)b);}
int sbb(REG32 a,short b){return x86(1044,a,(IMM)b);}
int sbb(REG32 a,REF b){return x86(1044,a,b);}
int sbb(MEM32 a,int b){return x86(1044,a,(IMM)b);}
int sbb(MEM32 a,char b){return x86(1044,a,(IMM)b);}
int sbb(MEM32 a,short b){return x86(1044,a,(IMM)b);}
int sbb(MEM32 a,REF b){return x86(1044,a,b);}
int sbb(R_M32 a,int b){return x86(1044,a,(IMM)b);}
int sbb(R_M32 a,char b){return x86(1044,a,(IMM)b);}
int sbb(R_M32 a,short b){return x86(1044,a,(IMM)b);}
int sbb(R_M32 a,REF b){return x86(1044,a,b);}
int lock_sbb(MEM8 a,char b){return x86(1047,a,(IMM)b);}
int lock_sbb(MEM16 a,char b){return x86(1048,a,(IMM)b);}
int lock_sbb(MEM16 a,short b){return x86(1048,a,(IMM)b);}
int lock_sbb(MEM32 a,int b){return x86(1049,a,(IMM)b);}
int lock_sbb(MEM32 a,char b){return x86(1049,a,(IMM)b);}
int lock_sbb(MEM32 a,short b){return x86(1049,a,(IMM)b);}
int lock_sbb(MEM32 a,REF b){return x86(1049,a,b);}
int scasb(){return x86(1055);}
int scasw(){return x86(1056);}
int scasd(){return x86(1057);}
int rep_scasb(){return x86(1058);}
int rep_scasw(){return x86(1059);}
int rep_scasd(){return x86(1060);}
int repe_scasb(){return x86(1061);}
int repe_scasw(){return x86(1062);}
int repe_scasd(){return x86(1063);}
int repne_scasb(){return x86(1064);}
int repne_scasw(){return x86(1065);}
int repne_scasd(){return x86(1066);}
int repz_scasb(){return x86(1067);}
int repz_scasw(){return x86(1068);}
int repz_scasd(){return x86(1069);}
int repnz_scasb(){return x86(1070);}
int repnz_scasw(){return x86(1071);}
int repnz_scasd(){return x86(1072);}
int seto(AL a){return x86(1073,a);}
int seto(CL a){return x86(1073,a);}
int seto(REG8 a){return x86(1073,a);}
int seto(MEM8 a){return x86(1073,a);}
int seto(R_M8 a){return x86(1073,a);}
int setno(AL a){return x86(1074,a);}
int setno(CL a){return x86(1074,a);}
int setno(REG8 a){return x86(1074,a);}
int setno(MEM8 a){return x86(1074,a);}
int setno(R_M8 a){return x86(1074,a);}
int setb(AL a){return x86(1075,a);}
int setb(CL a){return x86(1075,a);}
int setb(REG8 a){return x86(1075,a);}
int setb(MEM8 a){return x86(1075,a);}
int setb(R_M8 a){return x86(1075,a);}
int setc(AL a){return x86(1076,a);}
int setc(CL a){return x86(1076,a);}
int setc(REG8 a){return x86(1076,a);}
int setc(MEM8 a){return x86(1076,a);}
int setc(R_M8 a){return x86(1076,a);}
int setnea(AL a){return x86(1077,a);}
int setnea(CL a){return x86(1077,a);}
int setnea(REG8 a){return x86(1077,a);}
int setnea(MEM8 a){return x86(1077,a);}
int setnea(R_M8 a){return x86(1077,a);}
int setae(AL a){return x86(1078,a);}
int setae(CL a){return x86(1078,a);}
int setae(REG8 a){return x86(1078,a);}
int setae(MEM8 a){return x86(1078,a);}
int setae(R_M8 a){return x86(1078,a);}
int setnb(AL a){return x86(1079,a);}
int setnb(CL a){return x86(1079,a);}
int setnb(REG8 a){return x86(1079,a);}
int setnb(MEM8 a){return x86(1079,a);}
int setnb(R_M8 a){return x86(1079,a);}
int setnc(AL a){return x86(1080,a);}
int setnc(CL a){return x86(1080,a);}
int setnc(REG8 a){return x86(1080,a);}
int setnc(MEM8 a){return x86(1080,a);}
int setnc(R_M8 a){return x86(1080,a);}
int sete(AL a){return x86(1081,a);}
int sete(CL a){return x86(1081,a);}
int sete(REG8 a){return x86(1081,a);}
int sete(MEM8 a){return x86(1081,a);}
int sete(R_M8 a){return x86(1081,a);}
int setz(AL a){return x86(1082,a);}
int setz(CL a){return x86(1082,a);}
int setz(REG8 a){return x86(1082,a);}
int setz(MEM8 a){return x86(1082,a);}
int setz(R_M8 a){return x86(1082,a);}
int setne(AL a){return x86(1083,a);}
int setne(CL a){return x86(1083,a);}
int setne(REG8 a){return x86(1083,a);}
int setne(MEM8 a){return x86(1083,a);}
int setne(R_M8 a){return x86(1083,a);}
int setnz(AL a){return x86(1084,a);}
int setnz(CL a){return x86(1084,a);}
int setnz(REG8 a){return x86(1084,a);}
int setnz(MEM8 a){return x86(1084,a);}
int setnz(R_M8 a){return x86(1084,a);}
int setbe(AL a){return x86(1085,a);}
int setbe(CL a){return x86(1085,a);}
int setbe(REG8 a){return x86(1085,a);}
int setbe(MEM8 a){return x86(1085,a);}
int setbe(R_M8 a){return x86(1085,a);}
int setna(AL a){return x86(1086,a);}
int setna(CL a){return x86(1086,a);}
int setna(REG8 a){return x86(1086,a);}
int setna(MEM8 a){return x86(1086,a);}
int setna(R_M8 a){return x86(1086,a);}
int seta(AL a){return x86(1087,a);}
int seta(CL a){return x86(1087,a);}
int seta(REG8 a){return x86(1087,a);}
int seta(MEM8 a){return x86(1087,a);}
int seta(R_M8 a){return x86(1087,a);}
int setnbe(AL a){return x86(1088,a);}
int setnbe(CL a){return x86(1088,a);}
int setnbe(REG8 a){return x86(1088,a);}
int setnbe(MEM8 a){return x86(1088,a);}
int setnbe(R_M8 a){return x86(1088,a);}
int sets(AL a){return x86(1089,a);}
int sets(CL a){return x86(1089,a);}
int sets(REG8 a){return x86(1089,a);}
int sets(MEM8 a){return x86(1089,a);}
int sets(R_M8 a){return x86(1089,a);}
int setns(AL a){return x86(1090,a);}
int setns(CL a){return x86(1090,a);}
int setns(REG8 a){return x86(1090,a);}
int setns(MEM8 a){return x86(1090,a);}
int setns(R_M8 a){return x86(1090,a);}
int setp(AL a){return x86(1091,a);}
int setp(CL a){return x86(1091,a);}
int setp(REG8 a){return x86(1091,a);}
int setp(MEM8 a){return x86(1091,a);}
int setp(R_M8 a){return x86(1091,a);}
int setpe(AL a){return x86(1092,a);}
int setpe(CL a){return x86(1092,a);}
int setpe(REG8 a){return x86(1092,a);}
int setpe(MEM8 a){return x86(1092,a);}
int setpe(R_M8 a){return x86(1092,a);}
int setnp(AL a){return x86(1093,a);}
int setnp(CL a){return x86(1093,a);}
int setnp(REG8 a){return x86(1093,a);}
int setnp(MEM8 a){return x86(1093,a);}
int setnp(R_M8 a){return x86(1093,a);}
int setpo(AL a){return x86(1094,a);}
int setpo(CL a){return x86(1094,a);}
int setpo(REG8 a){return x86(1094,a);}
int setpo(MEM8 a){return x86(1094,a);}
int setpo(R_M8 a){return x86(1094,a);}
int setl(AL a){return x86(1095,a);}
int setl(CL a){return x86(1095,a);}
int setl(REG8 a){return x86(1095,a);}
int setl(MEM8 a){return x86(1095,a);}
int setl(R_M8 a){return x86(1095,a);}
int setnge(AL a){return x86(1096,a);}
int setnge(CL a){return x86(1096,a);}
int setnge(REG8 a){return x86(1096,a);}
int setnge(MEM8 a){return x86(1096,a);}
int setnge(R_M8 a){return x86(1096,a);}
int setge(AL a){return x86(1097,a);}
int setge(CL a){return x86(1097,a);}
int setge(REG8 a){return x86(1097,a);}
int setge(MEM8 a){return x86(1097,a);}
int setge(R_M8 a){return x86(1097,a);}
int setnl(AL a){return x86(1098,a);}
int setnl(CL a){return x86(1098,a);}
int setnl(REG8 a){return x86(1098,a);}
int setnl(MEM8 a){return x86(1098,a);}
int setnl(R_M8 a){return x86(1098,a);}
int setle(AL a){return x86(1099,a);}
int setle(CL a){return x86(1099,a);}
int setle(REG8 a){return x86(1099,a);}
int setle(MEM8 a){return x86(1099,a);}
int setle(R_M8 a){return x86(1099,a);}
int setng(AL a){return x86(1100,a);}
int setng(CL a){return x86(1100,a);}
int setng(REG8 a){return x86(1100,a);}
int setng(MEM8 a){return x86(1100,a);}
int setng(R_M8 a){return x86(1100,a);}
int setg(AL a){return x86(1101,a);}
int setg(CL a){return x86(1101,a);}
int setg(REG8 a){return x86(1101,a);}
int setg(MEM8 a){return x86(1101,a);}
int setg(R_M8 a){return x86(1101,a);}
int setnle(AL a){return x86(1102,a);}
int setnle(CL a){return x86(1102,a);}
int setnle(REG8 a){return x86(1102,a);}
int setnle(MEM8 a){return x86(1102,a);}
int setnle(R_M8 a){return x86(1102,a);}
int sfence(){return x86(1103);}
int shl(AL a,CL b){return x86(1105,a,b);}
int shl(CL a,CL b){return x86(1105,a,b);}
int shl(REG8 a,CL b){return x86(1105,a,b);}
int shl(MEM8 a,CL b){return x86(1105,a,b);}
int shl(R_M8 a,CL b){return x86(1105,a,b);}
int shl(AL a,char b){return x86(1106,a,(IMM)b);}
int shl(CL a,char b){return x86(1106,a,(IMM)b);}
int shl(REG8 a,char b){return x86(1106,a,(IMM)b);}
int shl(MEM8 a,char b){return x86(1106,a,(IMM)b);}
int shl(R_M8 a,char b){return x86(1106,a,(IMM)b);}
int shl(AX a,CL b){return x86(1108,a,b);}
int shl(DX a,CL b){return x86(1108,a,b);}
int shl(CX a,CL b){return x86(1108,a,b);}
int shl(REG16 a,CL b){return x86(1108,a,b);}
int shl(MEM16 a,CL b){return x86(1108,a,b);}
int shl(R_M16 a,CL b){return x86(1108,a,b);}
int shl(EAX a,CL b){return x86(1111,a,b);}
int shl(ECX a,CL b){return x86(1111,a,b);}
int shl(REG32 a,CL b){return x86(1111,a,b);}
int shl(MEM32 a,CL b){return x86(1111,a,b);}
int shl(R_M32 a,CL b){return x86(1111,a,b);}
int shr(AL a,CL b){return x86(1114,a,b);}
int shr(CL a,CL b){return x86(1114,a,b);}
int shr(REG8 a,CL b){return x86(1114,a,b);}
int shr(MEM8 a,CL b){return x86(1114,a,b);}
int shr(R_M8 a,CL b){return x86(1114,a,b);}
int shr(AL a,char b){return x86(1115,a,(IMM)b);}
int shr(CL a,char b){return x86(1115,a,(IMM)b);}
int shr(REG8 a,char b){return x86(1115,a,(IMM)b);}
int shr(MEM8 a,char b){return x86(1115,a,(IMM)b);}
int shr(R_M8 a,char b){return x86(1115,a,(IMM)b);}
int shr(AX a,CL b){return x86(1117,a,b);}
int shr(DX a,CL b){return x86(1117,a,b);}
int shr(CX a,CL b){return x86(1117,a,b);}
int shr(REG16 a,CL b){return x86(1117,a,b);}
int shr(MEM16 a,CL b){return x86(1117,a,b);}
int shr(R_M16 a,CL b){return x86(1117,a,b);}
int shr(EAX a,CL b){return x86(1120,a,b);}
int shr(ECX a,CL b){return x86(1120,a,b);}
int shr(REG32 a,CL b){return x86(1120,a,b);}
int shr(MEM32 a,CL b){return x86(1120,a,b);}
int shr(R_M32 a,CL b){return x86(1120,a,b);}
int shld(AX a,AX b,char c){return x86(1122,a,b,(IMM)c);}
int shld(AX a,DX b,char c){return x86(1122,a,b,(IMM)c);}
int shld(AX a,CX b,char c){return x86(1122,a,b,(IMM)c);}
int shld(AX a,REG16 b,char c){return x86(1122,a,b,(IMM)c);}
int shld(DX a,AX b,char c){return x86(1122,a,b,(IMM)c);}
int shld(DX a,DX b,char c){return x86(1122,a,b,(IMM)c);}
int shld(DX a,CX b,char c){return x86(1122,a,b,(IMM)c);}
int shld(DX a,REG16 b,char c){return x86(1122,a,b,(IMM)c);}
int shld(CX a,AX b,char c){return x86(1122,a,b,(IMM)c);}
int shld(CX a,DX b,char c){return x86(1122,a,b,(IMM)c);}
int shld(CX a,CX b,char c){return x86(1122,a,b,(IMM)c);}
int shld(CX a,REG16 b,char c){return x86(1122,a,b,(IMM)c);}
int shld(REG16 a,AX b,char c){return x86(1122,a,b,(IMM)c);}
int shld(REG16 a,DX b,char c){return x86(1122,a,b,(IMM)c);}
int shld(REG16 a,CX b,char c){return x86(1122,a,b,(IMM)c);}
int shld(REG16 a,REG16 b,char c){return x86(1122,a,b,(IMM)c);}
int shld(MEM16 a,AX b,char c){return x86(1122,a,b,(IMM)c);}
int shld(MEM16 a,DX b,char c){return x86(1122,a,b,(IMM)c);}
int shld(MEM16 a,CX b,char c){return x86(1122,a,b,(IMM)c);}
int shld(MEM16 a,REG16 b,char c){return x86(1122,a,b,(IMM)c);}
int shld(R_M16 a,AX b,char c){return x86(1122,a,b,(IMM)c);}
int shld(R_M16 a,DX b,char c){return x86(1122,a,b,(IMM)c);}
int shld(R_M16 a,CX b,char c){return x86(1122,a,b,(IMM)c);}
int shld(R_M16 a,REG16 b,char c){return x86(1122,a,b,(IMM)c);}
int shld(EAX a,EAX b,char c){return x86(1123,a,b,(IMM)c);}
int shld(EAX a,ECX b,char c){return x86(1123,a,b,(IMM)c);}
int shld(EAX a,REG32 b,char c){return x86(1123,a,b,(IMM)c);}
int shld(ECX a,EAX b,char c){return x86(1123,a,b,(IMM)c);}
int shld(ECX a,ECX b,char c){return x86(1123,a,b,(IMM)c);}
int shld(ECX a,REG32 b,char c){return x86(1123,a,b,(IMM)c);}
int shld(REG32 a,EAX b,char c){return x86(1123,a,b,(IMM)c);}
int shld(REG32 a,ECX b,char c){return x86(1123,a,b,(IMM)c);}
int shld(REG32 a,REG32 b,char c){return x86(1123,a,b,(IMM)c);}
int shld(MEM32 a,EAX b,char c){return x86(1123,a,b,(IMM)c);}
int shld(MEM32 a,ECX b,char c){return x86(1123,a,b,(IMM)c);}
int shld(MEM32 a,REG32 b,char c){return x86(1123,a,b,(IMM)c);}
int shld(R_M32 a,EAX b,char c){return x86(1123,a,b,(IMM)c);}
int shld(R_M32 a,ECX b,char c){return x86(1123,a,b,(IMM)c);}
int shld(R_M32 a,REG32 b,char c){return x86(1123,a,b,(IMM)c);}
int shld(AX a,AX b,CL c){return x86(1124,a,b,c);}
int shld(AX a,DX b,CL c){return x86(1124,a,b,c);}
int shld(AX a,CX b,CL c){return x86(1124,a,b,c);}
int shld(AX a,REG16 b,CL c){return x86(1124,a,b,c);}
int shld(DX a,AX b,CL c){return x86(1124,a,b,c);}
int shld(DX a,DX b,CL c){return x86(1124,a,b,c);}
int shld(DX a,CX b,CL c){return x86(1124,a,b,c);}
int shld(DX a,REG16 b,CL c){return x86(1124,a,b,c);}
int shld(CX a,AX b,CL c){return x86(1124,a,b,c);}
int shld(CX a,DX b,CL c){return x86(1124,a,b,c);}
int shld(CX a,CX b,CL c){return x86(1124,a,b,c);}
int shld(CX a,REG16 b,CL c){return x86(1124,a,b,c);}
int shld(REG16 a,AX b,CL c){return x86(1124,a,b,c);}
int shld(REG16 a,DX b,CL c){return x86(1124,a,b,c);}
int shld(REG16 a,CX b,CL c){return x86(1124,a,b,c);}
int shld(REG16 a,REG16 b,CL c){return x86(1124,a,b,c);}
int shld(MEM16 a,AX b,CL c){return x86(1124,a,b,c);}
int shld(MEM16 a,DX b,CL c){return x86(1124,a,b,c);}
int shld(MEM16 a,CX b,CL c){return x86(1124,a,b,c);}
int shld(MEM16 a,REG16 b,CL c){return x86(1124,a,b,c);}
int shld(R_M16 a,AX b,CL c){return x86(1124,a,b,c);}
int shld(R_M16 a,DX b,CL c){return x86(1124,a,b,c);}
int shld(R_M16 a,CX b,CL c){return x86(1124,a,b,c);}
int shld(R_M16 a,REG16 b,CL c){return x86(1124,a,b,c);}
int shld(EAX a,EAX b,CL c){return x86(1125,a,b,c);}
int shld(EAX a,ECX b,CL c){return x86(1125,a,b,c);}
int shld(EAX a,REG32 b,CL c){return x86(1125,a,b,c);}
int shld(ECX a,EAX b,CL c){return x86(1125,a,b,c);}
int shld(ECX a,ECX b,CL c){return x86(1125,a,b,c);}
int shld(ECX a,REG32 b,CL c){return x86(1125,a,b,c);}
int shld(REG32 a,EAX b,CL c){return x86(1125,a,b,c);}
int shld(REG32 a,ECX b,CL c){return x86(1125,a,b,c);}
int shld(REG32 a,REG32 b,CL c){return x86(1125,a,b,c);}
int shld(MEM32 a,EAX b,CL c){return x86(1125,a,b,c);}
int shld(MEM32 a,ECX b,CL c){return x86(1125,a,b,c);}
int shld(MEM32 a,REG32 b,CL c){return x86(1125,a,b,c);}
int shld(R_M32 a,EAX b,CL c){return x86(1125,a,b,c);}
int shld(R_M32 a,ECX b,CL c){return x86(1125,a,b,c);}
int shld(R_M32 a,REG32 b,CL c){return x86(1125,a,b,c);}
int shrd(AX a,AX b,char c){return x86(1126,a,b,(IMM)c);}
int shrd(AX a,DX b,char c){return x86(1126,a,b,(IMM)c);}
int shrd(AX a,CX b,char c){return x86(1126,a,b,(IMM)c);}
int shrd(AX a,REG16 b,char c){return x86(1126,a,b,(IMM)c);}
int shrd(DX a,AX b,char c){return x86(1126,a,b,(IMM)c);}
int shrd(DX a,DX b,char c){return x86(1126,a,b,(IMM)c);}
int shrd(DX a,CX b,char c){return x86(1126,a,b,(IMM)c);}
int shrd(DX a,REG16 b,char c){return x86(1126,a,b,(IMM)c);}
int shrd(CX a,AX b,char c){return x86(1126,a,b,(IMM)c);}
int shrd(CX a,DX b,char c){return x86(1126,a,b,(IMM)c);}
int shrd(CX a,CX b,char c){return x86(1126,a,b,(IMM)c);}
int shrd(CX a,REG16 b,char c){return x86(1126,a,b,(IMM)c);}
int shrd(REG16 a,AX b,char c){return x86(1126,a,b,(IMM)c);}
int shrd(REG16 a,DX b,char c){return x86(1126,a,b,(IMM)c);}
int shrd(REG16 a,CX b,char c){return x86(1126,a,b,(IMM)c);}
int shrd(REG16 a,REG16 b,char c){return x86(1126,a,b,(IMM)c);}
int shrd(MEM16 a,AX b,char c){return x86(1126,a,b,(IMM)c);}
int shrd(MEM16 a,DX b,char c){return x86(1126,a,b,(IMM)c);}
int shrd(MEM16 a,CX b,char c){return x86(1126,a,b,(IMM)c);}
int shrd(MEM16 a,REG16 b,char c){return x86(1126,a,b,(IMM)c);}
int shrd(R_M16 a,AX b,char c){return x86(1126,a,b,(IMM)c);}
int shrd(R_M16 a,DX b,char c){return x86(1126,a,b,(IMM)c);}
int shrd(R_M16 a,CX b,char c){return x86(1126,a,b,(IMM)c);}
int shrd(R_M16 a,REG16 b,char c){return x86(1126,a,b,(IMM)c);}
int shrd(EAX a,EAX b,char c){return x86(1127,a,b,(IMM)c);}
int shrd(EAX a,ECX b,char c){return x86(1127,a,b,(IMM)c);}
int shrd(EAX a,REG32 b,char c){return x86(1127,a,b,(IMM)c);}
int shrd(ECX a,EAX b,char c){return x86(1127,a,b,(IMM)c);}
int shrd(ECX a,ECX b,char c){return x86(1127,a,b,(IMM)c);}
int shrd(ECX a,REG32 b,char c){return x86(1127,a,b,(IMM)c);}
int shrd(REG32 a,EAX b,char c){return x86(1127,a,b,(IMM)c);}
int shrd(REG32 a,ECX b,char c){return x86(1127,a,b,(IMM)c);}
int shrd(REG32 a,REG32 b,char c){return x86(1127,a,b,(IMM)c);}
int shrd(MEM32 a,EAX b,char c){return x86(1127,a,b,(IMM)c);}
int shrd(MEM32 a,ECX b,char c){return x86(1127,a,b,(IMM)c);}
int shrd(MEM32 a,REG32 b,char c){return x86(1127,a,b,(IMM)c);}
int shrd(R_M32 a,EAX b,char c){return x86(1127,a,b,(IMM)c);}
int shrd(R_M32 a,ECX b,char c){return x86(1127,a,b,(IMM)c);}
int shrd(R_M32 a,REG32 b,char c){return x86(1127,a,b,(IMM)c);}
int shrd(AX a,AX b,CL c){return x86(1128,a,b,c);}
int shrd(AX a,DX b,CL c){return x86(1128,a,b,c);}
int shrd(AX a,CX b,CL c){return x86(1128,a,b,c);}
int shrd(AX a,REG16 b,CL c){return x86(1128,a,b,c);}
int shrd(DX a,AX b,CL c){return x86(1128,a,b,c);}
int shrd(DX a,DX b,CL c){return x86(1128,a,b,c);}
int shrd(DX a,CX b,CL c){return x86(1128,a,b,c);}
int shrd(DX a,REG16 b,CL c){return x86(1128,a,b,c);}
int shrd(CX a,AX b,CL c){return x86(1128,a,b,c);}
int shrd(CX a,DX b,CL c){return x86(1128,a,b,c);}
int shrd(CX a,CX b,CL c){return x86(1128,a,b,c);}
int shrd(CX a,REG16 b,CL c){return x86(1128,a,b,c);}
int shrd(REG16 a,AX b,CL c){return x86(1128,a,b,c);}
int shrd(REG16 a,DX b,CL c){return x86(1128,a,b,c);}
int shrd(REG16 a,CX b,CL c){return x86(1128,a,b,c);}
int shrd(REG16 a,REG16 b,CL c){return x86(1128,a,b,c);}
int shrd(MEM16 a,AX b,CL c){return x86(1128,a,b,c);}
int shrd(MEM16 a,DX b,CL c){return x86(1128,a,b,c);}
int shrd(MEM16 a,CX b,CL c){return x86(1128,a,b,c);}
int shrd(MEM16 a,REG16 b,CL c){return x86(1128,a,b,c);}
int shrd(R_M16 a,AX b,CL c){return x86(1128,a,b,c);}
int shrd(R_M16 a,DX b,CL c){return x86(1128,a,b,c);}
int shrd(R_M16 a,CX b,CL c){return x86(1128,a,b,c);}
int shrd(R_M16 a,REG16 b,CL c){return x86(1128,a,b,c);}
int shrd(EAX a,EAX b,CL c){return x86(1129,a,b,c);}
int shrd(EAX a,ECX b,CL c){return x86(1129,a,b,c);}
int shrd(EAX a,REG32 b,CL c){return x86(1129,a,b,c);}
int shrd(ECX a,EAX b,CL c){return x86(1129,a,b,c);}
int shrd(ECX a,ECX b,CL c){return x86(1129,a,b,c);}
int shrd(ECX a,REG32 b,CL c){return x86(1129,a,b,c);}
int shrd(REG32 a,EAX b,CL c){return x86(1129,a,b,c);}
int shrd(REG32 a,ECX b,CL c){return x86(1129,a,b,c);}
int shrd(REG32 a,REG32 b,CL c){return x86(1129,a,b,c);}
int shrd(MEM32 a,EAX b,CL c){return x86(1129,a,b,c);}
int shrd(MEM32 a,ECX b,CL c){return x86(1129,a,b,c);}
int shrd(MEM32 a,REG32 b,CL c){return x86(1129,a,b,c);}
int shrd(R_M32 a,EAX b,CL c){return x86(1129,a,b,c);}
int shrd(R_M32 a,ECX b,CL c){return x86(1129,a,b,c);}
int shrd(R_M32 a,REG32 b,CL c){return x86(1129,a,b,c);}
int shufpd(XMMREG a,XMMREG b,char c){return x86(1130,a,b,(IMM)c);}
int shufpd(XMMREG a,MEM128 b,char c){return x86(1130,a,b,(IMM)c);}
int shufpd(XMMREG a,R_M128 b,char c){return x86(1130,a,b,(IMM)c);}
int shufps(XMMREG a,XMMREG b,char c){return x86(1131,a,b,(IMM)c);}
int shufps(XMMREG a,MEM128 b,char c){return x86(1131,a,b,(IMM)c);}
int shufps(XMMREG a,R_M128 b,char c){return x86(1131,a,b,(IMM)c);}
int smint(){return x86(1132);}
int smintold(){return x86(1133);}
int sqrtpd(XMMREG a,XMMREG b){return x86(1134,a,b);}
int sqrtpd(XMMREG a,MEM128 b){return x86(1134,a,b);}
int sqrtpd(XMMREG a,R_M128 b){return x86(1134,a,b);}
int sqrtps(XMMREG a,XMMREG b){return x86(1135,a,b);}
int sqrtps(XMMREG a,MEM128 b){return x86(1135,a,b);}
int sqrtps(XMMREG a,R_M128 b){return x86(1135,a,b);}
int sqrtsd(XMMREG a,XMMREG b){return x86(1136,a,b);}
int sqrtsd(XMMREG a,MEM64 b){return x86(1136,a,b);}
int sqrtsd(XMMREG a,XMM64 b){return x86(1136,a,b);}
int sqrtss(XMMREG a,XMMREG b){return x86(1137,a,b);}
int sqrtss(XMMREG a,MEM32 b){return x86(1137,a,b);}
int sqrtss(XMMREG a,XMM32 b){return x86(1137,a,b);}
int stc(){return x86(1138);}
int std(){return x86(1139);}
int sti(){return x86(1140);}
int stmxcsr(MEM32 a){return x86(1141,a);}
int stosb(){return x86(1142);}
int stosw(){return x86(1143);}
int stosd(){return x86(1144);}
int rep_stosb(){return x86(1145);}
int rep_stosw(){return x86(1146);}
int rep_stosd(){return x86(1147);}
int sub(AL a,AL b){return x86(1148,a,b);}
int sub(AL a,CL b){return x86(1148,a,b);}
int sub(AL a,REG8 b){return x86(1148,a,b);}
int sub(CL a,AL b){return x86(1148,a,b);}
int sub(CL a,CL b){return x86(1148,a,b);}
int sub(CL a,REG8 b){return x86(1148,a,b);}
int sub(REG8 a,AL b){return x86(1148,a,b);}
int sub(REG8 a,CL b){return x86(1148,a,b);}
int sub(REG8 a,REG8 b){return x86(1148,a,b);}
int sub(MEM8 a,AL b){return x86(1148,a,b);}
int sub(MEM8 a,CL b){return x86(1148,a,b);}
int sub(MEM8 a,REG8 b){return x86(1148,a,b);}
int sub(R_M8 a,AL b){return x86(1148,a,b);}
int sub(R_M8 a,CL b){return x86(1148,a,b);}
int sub(R_M8 a,REG8 b){return x86(1148,a,b);}
int sub(AX a,AX b){return x86(1149,a,b);}
int sub(AX a,DX b){return x86(1149,a,b);}
int sub(AX a,CX b){return x86(1149,a,b);}
int sub(AX a,REG16 b){return x86(1149,a,b);}
int sub(DX a,AX b){return x86(1149,a,b);}
int sub(DX a,DX b){return x86(1149,a,b);}
int sub(DX a,CX b){return x86(1149,a,b);}
int sub(DX a,REG16 b){return x86(1149,a,b);}
int sub(CX a,AX b){return x86(1149,a,b);}
int sub(CX a,DX b){return x86(1149,a,b);}
int sub(CX a,CX b){return x86(1149,a,b);}
int sub(CX a,REG16 b){return x86(1149,a,b);}
int sub(REG16 a,AX b){return x86(1149,a,b);}
int sub(REG16 a,DX b){return x86(1149,a,b);}
int sub(REG16 a,CX b){return x86(1149,a,b);}
int sub(REG16 a,REG16 b){return x86(1149,a,b);}
int sub(MEM16 a,AX b){return x86(1149,a,b);}
int sub(MEM16 a,DX b){return x86(1149,a,b);}
int sub(MEM16 a,CX b){return x86(1149,a,b);}
int sub(MEM16 a,REG16 b){return x86(1149,a,b);}
int sub(R_M16 a,AX b){return x86(1149,a,b);}
int sub(R_M16 a,DX b){return x86(1149,a,b);}
int sub(R_M16 a,CX b){return x86(1149,a,b);}
int sub(R_M16 a,REG16 b){return x86(1149,a,b);}
int sub(EAX a,EAX b){return x86(1150,a,b);}
int sub(EAX a,ECX b){return x86(1150,a,b);}
int sub(EAX a,REG32 b){return x86(1150,a,b);}
int sub(ECX a,EAX b){return x86(1150,a,b);}
int sub(ECX a,ECX b){return x86(1150,a,b);}
int sub(ECX a,REG32 b){return x86(1150,a,b);}
int sub(REG32 a,EAX b){return x86(1150,a,b);}
int sub(REG32 a,ECX b){return x86(1150,a,b);}
int sub(REG32 a,REG32 b){return x86(1150,a,b);}
int sub(MEM32 a,EAX b){return x86(1150,a,b);}
int sub(MEM32 a,ECX b){return x86(1150,a,b);}
int sub(MEM32 a,REG32 b){return x86(1150,a,b);}
int sub(R_M32 a,EAX b){return x86(1150,a,b);}
int sub(R_M32 a,ECX b){return x86(1150,a,b);}
int sub(R_M32 a,REG32 b){return x86(1150,a,b);}
int lock_sub(MEM8 a,AL b){return x86(1151,a,b);}
int lock_sub(MEM8 a,CL b){return x86(1151,a,b);}
int lock_sub(MEM8 a,REG8 b){return x86(1151,a,b);}
int lock_sub(MEM16 a,AX b){return x86(1152,a,b);}
int lock_sub(MEM16 a,DX b){return x86(1152,a,b);}
int lock_sub(MEM16 a,CX b){return x86(1152,a,b);}
int lock_sub(MEM16 a,REG16 b){return x86(1152,a,b);}
int lock_sub(MEM32 a,EAX b){return x86(1153,a,b);}
int lock_sub(MEM32 a,ECX b){return x86(1153,a,b);}
int lock_sub(MEM32 a,REG32 b){return x86(1153,a,b);}
int sub(AL a,MEM8 b){return x86(1154,a,b);}
int sub(AL a,R_M8 b){return x86(1154,a,b);}
int sub(CL a,MEM8 b){return x86(1154,a,b);}
int sub(CL a,R_M8 b){return x86(1154,a,b);}
int sub(REG8 a,MEM8 b){return x86(1154,a,b);}
int sub(REG8 a,R_M8 b){return x86(1154,a,b);}
int sub(AX a,MEM16 b){return x86(1155,a,b);}
int sub(AX a,R_M16 b){return x86(1155,a,b);}
int sub(DX a,MEM16 b){return x86(1155,a,b);}
int sub(DX a,R_M16 b){return x86(1155,a,b);}
int sub(CX a,MEM16 b){return x86(1155,a,b);}
int sub(CX a,R_M16 b){return x86(1155,a,b);}
int sub(REG16 a,MEM16 b){return x86(1155,a,b);}
int sub(REG16 a,R_M16 b){return x86(1155,a,b);}
int sub(EAX a,MEM32 b){return x86(1156,a,b);}
int sub(EAX a,R_M32 b){return x86(1156,a,b);}
int sub(ECX a,MEM32 b){return x86(1156,a,b);}
int sub(ECX a,R_M32 b){return x86(1156,a,b);}
int sub(REG32 a,MEM32 b){return x86(1156,a,b);}
int sub(REG32 a,R_M32 b){return x86(1156,a,b);}
int sub(AL a,char b){return x86(1157,a,(IMM)b);}
int sub(CL a,char b){return x86(1157,a,(IMM)b);}
int sub(REG8 a,char b){return x86(1157,a,(IMM)b);}
int sub(MEM8 a,char b){return x86(1157,a,(IMM)b);}
int sub(R_M8 a,char b){return x86(1157,a,(IMM)b);}
int sub(AX a,char b){return x86(1158,a,(IMM)b);}
int sub(AX a,short b){return x86(1158,a,(IMM)b);}
int sub(DX a,char b){return x86(1158,a,(IMM)b);}
int sub(DX a,short b){return x86(1158,a,(IMM)b);}
int sub(CX a,char b){return x86(1158,a,(IMM)b);}
int sub(CX a,short b){return x86(1158,a,(IMM)b);}
int sub(REG16 a,char b){return x86(1158,a,(IMM)b);}
int sub(REG16 a,short b){return x86(1158,a,(IMM)b);}
int sub(MEM16 a,char b){return x86(1158,a,(IMM)b);}
int sub(MEM16 a,short b){return x86(1158,a,(IMM)b);}
int sub(R_M16 a,char b){return x86(1158,a,(IMM)b);}
int sub(R_M16 a,short b){return x86(1158,a,(IMM)b);}
int sub(EAX a,int b){return x86(1159,a,(IMM)b);}
int sub(EAX a,char b){return x86(1159,a,(IMM)b);}
int sub(EAX a,short b){return x86(1159,a,(IMM)b);}
int sub(EAX a,REF b){return x86(1159,a,b);}
int sub(ECX a,int b){return x86(1159,a,(IMM)b);}
int sub(ECX a,char b){return x86(1159,a,(IMM)b);}
int sub(ECX a,short b){return x86(1159,a,(IMM)b);}
int sub(ECX a,REF b){return x86(1159,a,b);}
int sub(REG32 a,int b){return x86(1159,a,(IMM)b);}
int sub(REG32 a,char b){return x86(1159,a,(IMM)b);}
int sub(REG32 a,short b){return x86(1159,a,(IMM)b);}
int sub(REG32 a,REF b){return x86(1159,a,b);}
int sub(MEM32 a,int b){return x86(1159,a,(IMM)b);}
int sub(MEM32 a,char b){return x86(1159,a,(IMM)b);}
int sub(MEM32 a,short b){return x86(1159,a,(IMM)b);}
int sub(MEM32 a,REF b){return x86(1159,a,b);}
int sub(R_M32 a,int b){return x86(1159,a,(IMM)b);}
int sub(R_M32 a,char b){return x86(1159,a,(IMM)b);}
int sub(R_M32 a,short b){return x86(1159,a,(IMM)b);}
int sub(R_M32 a,REF b){return x86(1159,a,b);}
int lock_sub(MEM8 a,char b){return x86(1162,a,(IMM)b);}
int lock_sub(MEM16 a,char b){return x86(1163,a,(IMM)b);}
int lock_sub(MEM16 a,short b){return x86(1163,a,(IMM)b);}
int lock_sub(MEM32 a,int b){return x86(1164,a,(IMM)b);}
int lock_sub(MEM32 a,char b){return x86(1164,a,(IMM)b);}
int lock_sub(MEM32 a,short b){return x86(1164,a,(IMM)b);}
int lock_sub(MEM32 a,REF b){return x86(1164,a,b);}
int subpd(XMMREG a,XMMREG b){return x86(1170,a,b);}
int subpd(XMMREG a,MEM128 b){return x86(1170,a,b);}
int subpd(XMMREG a,R_M128 b){return x86(1170,a,b);}
int subps(XMMREG a,XMMREG b){return x86(1171,a,b);}
int subps(XMMREG a,MEM128 b){return x86(1171,a,b);}
int subps(XMMREG a,R_M128 b){return x86(1171,a,b);}
int subsd(XMMREG a,XMMREG b){return x86(1172,a,b);}
int subsd(XMMREG a,MEM64 b){return x86(1172,a,b);}
int subsd(XMMREG a,XMM64 b){return x86(1172,a,b);}
int subss(XMMREG a,XMMREG b){return x86(1173,a,b);}
int subss(XMMREG a,MEM32 b){return x86(1173,a,b);}
int subss(XMMREG a,XMM32 b){return x86(1173,a,b);}
int sysenter(){return x86(1174);}
int test(AL a,AL b){return x86(1175,a,b);}
int test(AL a,CL b){return x86(1175,a,b);}
int test(AL a,REG8 b){return x86(1175,a,b);}
int test(CL a,AL b){return x86(1175,a,b);}
int test(CL a,CL b){return x86(1175,a,b);}
int test(CL a,REG8 b){return x86(1175,a,b);}
int test(REG8 a,AL b){return x86(1175,a,b);}
int test(REG8 a,CL b){return x86(1175,a,b);}
int test(REG8 a,REG8 b){return x86(1175,a,b);}
int test(MEM8 a,AL b){return x86(1175,a,b);}
int test(MEM8 a,CL b){return x86(1175,a,b);}
int test(MEM8 a,REG8 b){return x86(1175,a,b);}
int test(R_M8 a,AL b){return x86(1175,a,b);}
int test(R_M8 a,CL b){return x86(1175,a,b);}
int test(R_M8 a,REG8 b){return x86(1175,a,b);}
int test(AX a,AX b){return x86(1176,a,b);}
int test(AX a,DX b){return x86(1176,a,b);}
int test(AX a,CX b){return x86(1176,a,b);}
int test(AX a,REG16 b){return x86(1176,a,b);}
int test(DX a,AX b){return x86(1176,a,b);}
int test(DX a,DX b){return x86(1176,a,b);}
int test(DX a,CX b){return x86(1176,a,b);}
int test(DX a,REG16 b){return x86(1176,a,b);}
int test(CX a,AX b){return x86(1176,a,b);}
int test(CX a,DX b){return x86(1176,a,b);}
int test(CX a,CX b){return x86(1176,a,b);}
int test(CX a,REG16 b){return x86(1176,a,b);}
int test(REG16 a,AX b){return x86(1176,a,b);}
int test(REG16 a,DX b){return x86(1176,a,b);}
int test(REG16 a,CX b){return x86(1176,a,b);}
int test(REG16 a,REG16 b){return x86(1176,a,b);}
int test(MEM16 a,AX b){return x86(1176,a,b);}
int test(MEM16 a,DX b){return x86(1176,a,b);}
int test(MEM16 a,CX b){return x86(1176,a,b);}
int test(MEM16 a,REG16 b){return x86(1176,a,b);}
int test(R_M16 a,AX b){return x86(1176,a,b);}
int test(R_M16 a,DX b){return x86(1176,a,b);}
int test(R_M16 a,CX b){return x86(1176,a,b);}
int test(R_M16 a,REG16 b){return x86(1176,a,b);}
int test(EAX a,EAX b){return x86(1177,a,b);}
int test(EAX a,ECX b){return x86(1177,a,b);}
int test(EAX a,REG32 b){return x86(1177,a,b);}
int test(ECX a,EAX b){return x86(1177,a,b);}
int test(ECX a,ECX b){return x86(1177,a,b);}
int test(ECX a,REG32 b){return x86(1177,a,b);}
int test(REG32 a,EAX b){return x86(1177,a,b);}
int test(REG32 a,ECX b){return x86(1177,a,b);}
int test(REG32 a,REG32 b){return x86(1177,a,b);}
int test(MEM32 a,EAX b){return x86(1177,a,b);}
int test(MEM32 a,ECX b){return x86(1177,a,b);}
int test(MEM32 a,REG32 b){return x86(1177,a,b);}
int test(R_M32 a,EAX b){return x86(1177,a,b);}
int test(R_M32 a,ECX b){return x86(1177,a,b);}
int test(R_M32 a,REG32 b){return x86(1177,a,b);}
int test(AL a,char b){return x86(1178,a,(IMM)b);}
int test(CL a,char b){return x86(1178,a,(IMM)b);}
int test(REG8 a,char b){return x86(1178,a,(IMM)b);}
int test(MEM8 a,char b){return x86(1178,a,(IMM)b);}
int test(R_M8 a,char b){return x86(1178,a,(IMM)b);}
int test(AX a,char b){return x86(1179,a,(IMM)b);}
int test(AX a,short b){return x86(1179,a,(IMM)b);}
int test(DX a,char b){return x86(1179,a,(IMM)b);}
int test(DX a,short b){return x86(1179,a,(IMM)b);}
int test(CX a,char b){return x86(1179,a,(IMM)b);}
int test(CX a,short b){return x86(1179,a,(IMM)b);}
int test(REG16 a,char b){return x86(1179,a,(IMM)b);}
int test(REG16 a,short b){return x86(1179,a,(IMM)b);}
int test(MEM16 a,char b){return x86(1179,a,(IMM)b);}
int test(MEM16 a,short b){return x86(1179,a,(IMM)b);}
int test(R_M16 a,char b){return x86(1179,a,(IMM)b);}
int test(R_M16 a,short b){return x86(1179,a,(IMM)b);}
int test(EAX a,int b){return x86(1180,a,(IMM)b);}
int test(EAX a,char b){return x86(1180,a,(IMM)b);}
int test(EAX a,short b){return x86(1180,a,(IMM)b);}
int test(EAX a,REF b){return x86(1180,a,b);}
int test(ECX a,int b){return x86(1180,a,(IMM)b);}
int test(ECX a,char b){return x86(1180,a,(IMM)b);}
int test(ECX a,short b){return x86(1180,a,(IMM)b);}
int test(ECX a,REF b){return x86(1180,a,b);}
int test(REG32 a,int b){return x86(1180,a,(IMM)b);}
int test(REG32 a,char b){return x86(1180,a,(IMM)b);}
int test(REG32 a,short b){return x86(1180,a,(IMM)b);}
int test(REG32 a,REF b){return x86(1180,a,b);}
int test(MEM32 a,int b){return x86(1180,a,(IMM)b);}
int test(MEM32 a,char b){return x86(1180,a,(IMM)b);}
int test(MEM32 a,short b){return x86(1180,a,(IMM)b);}
int test(MEM32 a,REF b){return x86(1180,a,b);}
int test(R_M32 a,int b){return x86(1180,a,(IMM)b);}
int test(R_M32 a,char b){return x86(1180,a,(IMM)b);}
int test(R_M32 a,short b){return x86(1180,a,(IMM)b);}
int test(R_M32 a,REF b){return x86(1180,a,b);}
int ucomisd(XMMREG a,XMMREG b){return x86(1184,a,b);}
int ucomisd(XMMREG a,MEM64 b){return x86(1184,a,b);}
int ucomisd(XMMREG a,XMM64 b){return x86(1184,a,b);}
int ucomiss(XMMREG a,XMMREG b){return x86(1185,a,b);}
int ucomiss(XMMREG a,MEM32 b){return x86(1185,a,b);}
int ucomiss(XMMREG a,XMM32 b){return x86(1185,a,b);}
int ud2(){return x86(1186);}
int unpckhpd(XMMREG a,XMMREG b){return x86(1187,a,b);}
int unpckhpd(XMMREG a,MEM128 b){return x86(1187,a,b);}
int unpckhpd(XMMREG a,R_M128 b){return x86(1187,a,b);}
int unpckhps(XMMREG a,XMMREG b){return x86(1188,a,b);}
int unpckhps(XMMREG a,MEM128 b){return x86(1188,a,b);}
int unpckhps(XMMREG a,R_M128 b){return x86(1188,a,b);}
int unpcklpd(XMMREG a,XMMREG b){return x86(1189,a,b);}
int unpcklpd(XMMREG a,MEM128 b){return x86(1189,a,b);}
int unpcklpd(XMMREG a,R_M128 b){return x86(1189,a,b);}
int unpcklps(XMMREG a,XMMREG b){return x86(1190,a,b);}
int unpcklps(XMMREG a,MEM128 b){return x86(1190,a,b);}
int unpcklps(XMMREG a,R_M128 b){return x86(1190,a,b);}
int wait(){return x86(1191);}
int wrmsr(){return x86(1192);}
int xadd(AL a,AL b){return x86(1193,a,b);}
int xadd(AL a,CL b){return x86(1193,a,b);}
int xadd(AL a,REG8 b){return x86(1193,a,b);}
int xadd(CL a,AL b){return x86(1193,a,b);}
int xadd(CL a,CL b){return x86(1193,a,b);}
int xadd(CL a,REG8 b){return x86(1193,a,b);}
int xadd(REG8 a,AL b){return x86(1193,a,b);}
int xadd(REG8 a,CL b){return x86(1193,a,b);}
int xadd(REG8 a,REG8 b){return x86(1193,a,b);}
int xadd(MEM8 a,AL b){return x86(1193,a,b);}
int xadd(MEM8 a,CL b){return x86(1193,a,b);}
int xadd(MEM8 a,REG8 b){return x86(1193,a,b);}
int xadd(R_M8 a,AL b){return x86(1193,a,b);}
int xadd(R_M8 a,CL b){return x86(1193,a,b);}
int xadd(R_M8 a,REG8 b){return x86(1193,a,b);}
int xadd(AX a,AX b){return x86(1194,a,b);}
int xadd(AX a,DX b){return x86(1194,a,b);}
int xadd(AX a,CX b){return x86(1194,a,b);}
int xadd(AX a,REG16 b){return x86(1194,a,b);}
int xadd(DX a,AX b){return x86(1194,a,b);}
int xadd(DX a,DX b){return x86(1194,a,b);}
int xadd(DX a,CX b){return x86(1194,a,b);}
int xadd(DX a,REG16 b){return x86(1194,a,b);}
int xadd(CX a,AX b){return x86(1194,a,b);}
int xadd(CX a,DX b){return x86(1194,a,b);}
int xadd(CX a,CX b){return x86(1194,a,b);}
int xadd(CX a,REG16 b){return x86(1194,a,b);}
int xadd(REG16 a,AX b){return x86(1194,a,b);}
int xadd(REG16 a,DX b){return x86(1194,a,b);}
int xadd(REG16 a,CX b){return x86(1194,a,b);}
int xadd(REG16 a,REG16 b){return x86(1194,a,b);}
int xadd(MEM16 a,AX b){return x86(1194,a,b);}
int xadd(MEM16 a,DX b){return x86(1194,a,b);}
int xadd(MEM16 a,CX b){return x86(1194,a,b);}
int xadd(MEM16 a,REG16 b){return x86(1194,a,b);}
int xadd(R_M16 a,AX b){return x86(1194,a,b);}
int xadd(R_M16 a,DX b){return x86(1194,a,b);}
int xadd(R_M16 a,CX b){return x86(1194,a,b);}
int xadd(R_M16 a,REG16 b){return x86(1194,a,b);}
int xadd(EAX a,EAX b){return x86(1195,a,b);}
int xadd(EAX a,ECX b){return x86(1195,a,b);}
int xadd(EAX a,REG32 b){return x86(1195,a,b);}
int xadd(ECX a,EAX b){return x86(1195,a,b);}
int xadd(ECX a,ECX b){return x86(1195,a,b);}
int xadd(ECX a,REG32 b){return x86(1195,a,b);}
int xadd(REG32 a,EAX b){return x86(1195,a,b);}
int xadd(REG32 a,ECX b){return x86(1195,a,b);}
int xadd(REG32 a,REG32 b){return x86(1195,a,b);}
int xadd(MEM32 a,EAX b){return x86(1195,a,b);}
int xadd(MEM32 a,ECX b){return x86(1195,a,b);}
int xadd(MEM32 a,REG32 b){return x86(1195,a,b);}
int xadd(R_M32 a,EAX b){return x86(1195,a,b);}
int xadd(R_M32 a,ECX b){return x86(1195,a,b);}
int xadd(R_M32 a,REG32 b){return x86(1195,a,b);}
int lock_xadd(MEM8 a,AL b){return x86(1196,a,b);}
int lock_xadd(MEM8 a,CL b){return x86(1196,a,b);}
int lock_xadd(MEM8 a,REG8 b){return x86(1196,a,b);}
int lock_xadd(MEM16 a,AX b){return x86(1197,a,b);}
int lock_xadd(MEM16 a,DX b){return x86(1197,a,b);}
int lock_xadd(MEM16 a,CX b){return x86(1197,a,b);}
int lock_xadd(MEM16 a,REG16 b){return x86(1197,a,b);}
int lock_xadd(MEM32 a,EAX b){return x86(1198,a,b);}
int lock_xadd(MEM32 a,ECX b){return x86(1198,a,b);}
int lock_xadd(MEM32 a,REG32 b){return x86(1198,a,b);}
int xchg(AL a,AL b){return x86(1199,a,b);}
int xchg(AL a,CL b){return x86(1199,a,b);}
int xchg(AL a,REG8 b){return x86(1199,a,b);}
int xchg(AL a,MEM8 b){return x86(1199,a,b);}
int xchg(AL a,R_M8 b){return x86(1199,a,b);}
int xchg(CL a,AL b){return x86(1199,a,b);}
int xchg(CL a,CL b){return x86(1199,a,b);}
int xchg(CL a,REG8 b){return x86(1199,a,b);}
int xchg(CL a,MEM8 b){return x86(1199,a,b);}
int xchg(CL a,R_M8 b){return x86(1199,a,b);}
int xchg(REG8 a,AL b){return x86(1199,a,b);}
int xchg(REG8 a,CL b){return x86(1199,a,b);}
int xchg(REG8 a,REG8 b){return x86(1199,a,b);}
int xchg(REG8 a,MEM8 b){return x86(1199,a,b);}
int xchg(REG8 a,R_M8 b){return x86(1199,a,b);}
int xchg(AX a,AL b){return x86(1200,a,b);}
int xchg(AX a,CL b){return x86(1200,a,b);}
int xchg(AX a,REG8 b){return x86(1200,a,b);}
int xchg(AX a,MEM8 b){return x86(1200,a,b);}
int xchg(AX a,R_M8 b){return x86(1200,a,b);}
int xchg(DX a,AL b){return x86(1200,a,b);}
int xchg(DX a,CL b){return x86(1200,a,b);}
int xchg(DX a,REG8 b){return x86(1200,a,b);}
int xchg(DX a,MEM8 b){return x86(1200,a,b);}
int xchg(DX a,R_M8 b){return x86(1200,a,b);}
int xchg(CX a,AL b){return x86(1200,a,b);}
int xchg(CX a,CL b){return x86(1200,a,b);}
int xchg(CX a,REG8 b){return x86(1200,a,b);}
int xchg(CX a,MEM8 b){return x86(1200,a,b);}
int xchg(CX a,R_M8 b){return x86(1200,a,b);}
int xchg(REG16 a,AL b){return x86(1200,a,b);}
int xchg(REG16 a,CL b){return x86(1200,a,b);}
int xchg(REG16 a,REG8 b){return x86(1200,a,b);}
int xchg(REG16 a,MEM8 b){return x86(1200,a,b);}
int xchg(REG16 a,R_M8 b){return x86(1200,a,b);}
int xchg(EAX a,EAX b){return x86(1201,a,b);}
int xchg(EAX a,ECX b){return x86(1201,a,b);}
int xchg(EAX a,REG32 b){return x86(1201,a,b);}
int xchg(EAX a,MEM32 b){return x86(1201,a,b);}
int xchg(EAX a,R_M32 b){return x86(1201,a,b);}
int xchg(ECX a,EAX b){return x86(1201,a,b);}
int xchg(ECX a,ECX b){return x86(1201,a,b);}
int xchg(ECX a,REG32 b){return x86(1201,a,b);}
int xchg(ECX a,MEM32 b){return x86(1201,a,b);}
int xchg(ECX a,R_M32 b){return x86(1201,a,b);}
int xchg(REG32 a,EAX b){return x86(1201,a,b);}
int xchg(REG32 a,ECX b){return x86(1201,a,b);}
int xchg(REG32 a,REG32 b){return x86(1201,a,b);}
int xchg(REG32 a,MEM32 b){return x86(1201,a,b);}
int xchg(REG32 a,R_M32 b){return x86(1201,a,b);}
int xchg(MEM8 a,AL b){return x86(1202,a,b);}
int xchg(MEM8 a,CL b){return x86(1202,a,b);}
int xchg(MEM8 a,REG8 b){return x86(1202,a,b);}
int xchg(R_M8 a,AL b){return x86(1202,a,b);}
int xchg(R_M8 a,CL b){return x86(1202,a,b);}
int xchg(R_M8 a,REG8 b){return x86(1202,a,b);}
int xchg(AX a,AX b){return x86(1203,a,b);}
int xchg(AX a,DX b){return x86(1203,a,b);}
int xchg(AX a,CX b){return x86(1203,a,b);}
int xchg(AX a,REG16 b){return x86(1203,a,b);}
int xchg(DX a,AX b){return x86(1203,a,b);}
int xchg(DX a,DX b){return x86(1203,a,b);}
int xchg(DX a,CX b){return x86(1203,a,b);}
int xchg(DX a,REG16 b){return x86(1203,a,b);}
int xchg(CX a,AX b){return x86(1203,a,b);}
int xchg(CX a,DX b){return x86(1203,a,b);}
int xchg(CX a,CX b){return x86(1203,a,b);}
int xchg(CX a,REG16 b){return x86(1203,a,b);}
int xchg(REG16 a,AX b){return x86(1203,a,b);}
int xchg(REG16 a,DX b){return x86(1203,a,b);}
int xchg(REG16 a,CX b){return x86(1203,a,b);}
int xchg(REG16 a,REG16 b){return x86(1203,a,b);}
int xchg(MEM16 a,AX b){return x86(1203,a,b);}
int xchg(MEM16 a,DX b){return x86(1203,a,b);}
int xchg(MEM16 a,CX b){return x86(1203,a,b);}
int xchg(MEM16 a,REG16 b){return x86(1203,a,b);}
int xchg(R_M16 a,AX b){return x86(1203,a,b);}
int xchg(R_M16 a,DX b){return x86(1203,a,b);}
int xchg(R_M16 a,CX b){return x86(1203,a,b);}
int xchg(R_M16 a,REG16 b){return x86(1203,a,b);}
int xchg(MEM32 a,EAX b){return x86(1204,a,b);}
int xchg(MEM32 a,ECX b){return x86(1204,a,b);}
int xchg(MEM32 a,REG32 b){return x86(1204,a,b);}
int xchg(R_M32 a,EAX b){return x86(1204,a,b);}
int xchg(R_M32 a,ECX b){return x86(1204,a,b);}
int xchg(R_M32 a,REG32 b){return x86(1204,a,b);}
int lock_xchg(MEM8 a,AL b){return x86(1205,a,b);}
int lock_xchg(MEM8 a,CL b){return x86(1205,a,b);}
int lock_xchg(MEM8 a,REG8 b){return x86(1205,a,b);}
int lock_xchg(MEM16 a,AX b){return x86(1206,a,b);}
int lock_xchg(MEM16 a,DX b){return x86(1206,a,b);}
int lock_xchg(MEM16 a,CX b){return x86(1206,a,b);}
int lock_xchg(MEM16 a,REG16 b){return x86(1206,a,b);}
int lock_xchg(MEM32 a,EAX b){return x86(1207,a,b);}
int lock_xchg(MEM32 a,ECX b){return x86(1207,a,b);}
int lock_xchg(MEM32 a,REG32 b){return x86(1207,a,b);}
int xlatb(){return x86(1212);}
int xor(AL a,AL b){return x86(1213,a,b);}
int xor(AL a,CL b){return x86(1213,a,b);}
int xor(AL a,REG8 b){return x86(1213,a,b);}
int xor(CL a,AL b){return x86(1213,a,b);}
int xor(CL a,CL b){return x86(1213,a,b);}
int xor(CL a,REG8 b){return x86(1213,a,b);}
int xor(REG8 a,AL b){return x86(1213,a,b);}
int xor(REG8 a,CL b){return x86(1213,a,b);}
int xor(REG8 a,REG8 b){return x86(1213,a,b);}
int xor(MEM8 a,AL b){return x86(1213,a,b);}
int xor(MEM8 a,CL b){return x86(1213,a,b);}
int xor(MEM8 a,REG8 b){return x86(1213,a,b);}
int xor(R_M8 a,AL b){return x86(1213,a,b);}
int xor(R_M8 a,CL b){return x86(1213,a,b);}
int xor(R_M8 a,REG8 b){return x86(1213,a,b);}
int xor(AX a,AX b){return x86(1214,a,b);}
int xor(AX a,DX b){return x86(1214,a,b);}
int xor(AX a,CX b){return x86(1214,a,b);}
int xor(AX a,REG16 b){return x86(1214,a,b);}
int xor(DX a,AX b){return x86(1214,a,b);}
int xor(DX a,DX b){return x86(1214,a,b);}
int xor(DX a,CX b){return x86(1214,a,b);}
int xor(DX a,REG16 b){return x86(1214,a,b);}
int xor(CX a,AX b){return x86(1214,a,b);}
int xor(CX a,DX b){return x86(1214,a,b);}
int xor(CX a,CX b){return x86(1214,a,b);}
int xor(CX a,REG16 b){return x86(1214,a,b);}
int xor(REG16 a,AX b){return x86(1214,a,b);}
int xor(REG16 a,DX b){return x86(1214,a,b);}
int xor(REG16 a,CX b){return x86(1214,a,b);}
int xor(REG16 a,REG16 b){return x86(1214,a,b);}
int xor(MEM16 a,AX b){return x86(1214,a,b);}
int xor(MEM16 a,DX b){return x86(1214,a,b);}
int xor(MEM16 a,CX b){return x86(1214,a,b);}
int xor(MEM16 a,REG16 b){return x86(1214,a,b);}
int xor(R_M16 a,AX b){return x86(1214,a,b);}
int xor(R_M16 a,DX b){return x86(1214,a,b);}
int xor(R_M16 a,CX b){return x86(1214,a,b);}
int xor(R_M16 a,REG16 b){return x86(1214,a,b);}
int xor(EAX a,EAX b){return x86(1215,a,b);}
int xor(EAX a,ECX b){return x86(1215,a,b);}
int xor(EAX a,REG32 b){return x86(1215,a,b);}
int xor(ECX a,EAX b){return x86(1215,a,b);}
int xor(ECX a,ECX b){return x86(1215,a,b);}
int xor(ECX a,REG32 b){return x86(1215,a,b);}
int xor(REG32 a,EAX b){return x86(1215,a,b);}
int xor(REG32 a,ECX b){return x86(1215,a,b);}
int xor(REG32 a,REG32 b){return x86(1215,a,b);}
int xor(MEM32 a,EAX b){return x86(1215,a,b);}
int xor(MEM32 a,ECX b){return x86(1215,a,b);}
int xor(MEM32 a,REG32 b){return x86(1215,a,b);}
int xor(R_M32 a,EAX b){return x86(1215,a,b);}
int xor(R_M32 a,ECX b){return x86(1215,a,b);}
int xor(R_M32 a,REG32 b){return x86(1215,a,b);}
int lock_xor(MEM8 a,AL b){return x86(1216,a,b);}
int lock_xor(MEM8 a,CL b){return x86(1216,a,b);}
int lock_xor(MEM8 a,REG8 b){return x86(1216,a,b);}
int lock_xor(MEM16 a,AX b){return x86(1217,a,b);}
int lock_xor(MEM16 a,DX b){return x86(1217,a,b);}
int lock_xor(MEM16 a,CX b){return x86(1217,a,b);}
int lock_xor(MEM16 a,REG16 b){return x86(1217,a,b);}
int lock_xor(MEM32 a,EAX b){return x86(1218,a,b);}
int lock_xor(MEM32 a,ECX b){return x86(1218,a,b);}
int lock_xor(MEM32 a,REG32 b){return x86(1218,a,b);}
int xor(AL a,MEM8 b){return x86(1219,a,b);}
int xor(AL a,R_M8 b){return x86(1219,a,b);}
int xor(CL a,MEM8 b){return x86(1219,a,b);}
int xor(CL a,R_M8 b){return x86(1219,a,b);}
int xor(REG8 a,MEM8 b){return x86(1219,a,b);}
int xor(REG8 a,R_M8 b){return x86(1219,a,b);}
int xor(AX a,MEM16 b){return x86(1220,a,b);}
int xor(AX a,R_M16 b){return x86(1220,a,b);}
int xor(DX a,MEM16 b){return x86(1220,a,b);}
int xor(DX a,R_M16 b){return x86(1220,a,b);}
int xor(CX a,MEM16 b){return x86(1220,a,b);}
int xor(CX a,R_M16 b){return x86(1220,a,b);}
int xor(REG16 a,MEM16 b){return x86(1220,a,b);}
int xor(REG16 a,R_M16 b){return x86(1220,a,b);}
int xor(EAX a,MEM32 b){return x86(1221,a,b);}
int xor(EAX a,R_M32 b){return x86(1221,a,b);}
int xor(ECX a,MEM32 b){return x86(1221,a,b);}
int xor(ECX a,R_M32 b){return x86(1221,a,b);}
int xor(REG32 a,MEM32 b){return x86(1221,a,b);}
int xor(REG32 a,R_M32 b){return x86(1221,a,b);}
int xor(AL a,char b){return x86(1222,a,(IMM)b);}
int xor(CL a,char b){return x86(1222,a,(IMM)b);}
int xor(REG8 a,char b){return x86(1222,a,(IMM)b);}
int xor(MEM8 a,char b){return x86(1222,a,(IMM)b);}
int xor(R_M8 a,char b){return x86(1222,a,(IMM)b);}
int xor(AX a,char b){return x86(1223,a,(IMM)b);}
int xor(AX a,short b){return x86(1223,a,(IMM)b);}
int xor(DX a,char b){return x86(1223,a,(IMM)b);}
int xor(DX a,short b){return x86(1223,a,(IMM)b);}
int xor(CX a,char b){return x86(1223,a,(IMM)b);}
int xor(CX a,short b){return x86(1223,a,(IMM)b);}
int xor(REG16 a,char b){return x86(1223,a,(IMM)b);}
int xor(REG16 a,short b){return x86(1223,a,(IMM)b);}
int xor(MEM16 a,char b){return x86(1223,a,(IMM)b);}
int xor(MEM16 a,short b){return x86(1223,a,(IMM)b);}
int xor(R_M16 a,char b){return x86(1223,a,(IMM)b);}
int xor(R_M16 a,short b){return x86(1223,a,(IMM)b);}
int xor(EAX a,int b){return x86(1224,a,(IMM)b);}
int xor(EAX a,char b){return x86(1224,a,(IMM)b);}
int xor(EAX a,short b){return x86(1224,a,(IMM)b);}
int xor(EAX a,REF b){return x86(1224,a,b);}
int xor(ECX a,int b){return x86(1224,a,(IMM)b);}
int xor(ECX a,char b){return x86(1224,a,(IMM)b);}
int xor(ECX a,short b){return x86(1224,a,(IMM)b);}
int xor(ECX a,REF b){return x86(1224,a,b);}
int xor(REG32 a,int b){return x86(1224,a,(IMM)b);}
int xor(REG32 a,char b){return x86(1224,a,(IMM)b);}
int xor(REG32 a,short b){return x86(1224,a,(IMM)b);}
int xor(REG32 a,REF b){return x86(1224,a,b);}
int xor(MEM32 a,int b){return x86(1224,a,(IMM)b);}
int xor(MEM32 a,char b){return x86(1224,a,(IMM)b);}
int xor(MEM32 a,short b){return x86(1224,a,(IMM)b);}
int xor(MEM32 a,REF b){return x86(1224,a,b);}
int xor(R_M32 a,int b){return x86(1224,a,(IMM)b);}
int xor(R_M32 a,char b){return x86(1224,a,(IMM)b);}
int xor(R_M32 a,short b){return x86(1224,a,(IMM)b);}
int xor(R_M32 a,REF b){return x86(1224,a,b);}
int lock_xor(MEM8 a,char b){return x86(1227,a,(IMM)b);}
int lock_xor(MEM16 a,char b){return x86(1228,a,(IMM)b);}
int lock_xor(MEM16 a,short b){return x86(1228,a,(IMM)b);}
int lock_xor(MEM32 a,int b){return x86(1229,a,(IMM)b);}
int lock_xor(MEM32 a,char b){return x86(1229,a,(IMM)b);}
int lock_xor(MEM32 a,short b){return x86(1229,a,(IMM)b);}
int lock_xor(MEM32 a,REF b){return x86(1229,a,b);}
int xorps(XMMREG a,XMMREG b){return x86(1235,a,b);}
int xorps(XMMREG a,MEM128 b){return x86(1235,a,b);}
int xorps(XMMREG a,R_M128 b){return x86(1235,a,b);}
int db(){return x86(1236);}
int dw(){return x86(1237);}
int dd(){return x86(1238);}
int db(char a){return x86(1239,(IMM)a);}
int dw(char a){return x86(1240,(IMM)a);}
int dw(short a){return x86(1240,(IMM)a);}
int dd(int a){return x86(1241,(IMM)a);}
int dd(char a){return x86(1241,(IMM)a);}
int dd(short a){return x86(1241,(IMM)a);}
int dd(REF a){return x86(1241,a);}
int db(MEM8 a){return x86(1242,a);}
int db(MEM16 a){return x86(1242,a);}
int db(MEM32 a){return x86(1242,a);}
int db(MEM64 a){return x86(1242,a);}
int db(MEM128 a){return x86(1242,a);}
int dw(MEM8 a){return x86(1243,a);}
int dw(MEM16 a){return x86(1243,a);}
int dw(MEM32 a){return x86(1243,a);}
int dw(MEM64 a){return x86(1243,a);}
int dw(MEM128 a){return x86(1243,a);}
int dd(MEM8 a){return x86(1244,a);}
int dd(MEM16 a){return x86(1244,a);}
int dd(MEM32 a){return x86(1244,a);}
int dd(MEM64 a){return x86(1244,a);}
int dd(MEM128 a){return x86(1244,a);}
int db(REF a){return x86(1245,a);}
int db(char* a){return x86(1245,(STR)a);}
int align(int a){return x86(1246,(IMM)a);}
int align(char a){return x86(1246,(IMM)a);}
int align(short a){return x86(1246,(IMM)a);}
int align(REF a){return x86(1246,a);}

#endif   // SOFTWIRE_NO_INTRINSICS
