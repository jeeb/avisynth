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

typedef void V;

V aaa(){x86(0);}
V aas(){x86(1);}
V aad(){x86(2);}
V aad(int a){x86(3,(IMM)a);}
V aad(char a){x86(3,(IMM)a);}
V aad(short a){x86(3,(IMM)a);}
V aad(REF a){x86(3,a);}
V aam(){x86(4);}
V aam(int a){x86(5,(IMM)a);}
V aam(char a){x86(5,(IMM)a);}
V aam(short a){x86(5,(IMM)a);}
V aam(REF a){x86(5,a);}
V adc(AL a,AL b){x86(6,a,b);}
V adc(AL a,CL b){x86(6,a,b);}
V adc(AL a,REG8 b){x86(6,a,b);}
V adc(CL a,AL b){x86(6,a,b);}
V adc(CL a,CL b){x86(6,a,b);}
V adc(CL a,REG8 b){x86(6,a,b);}
V adc(REG8 a,AL b){x86(6,a,b);}
V adc(REG8 a,CL b){x86(6,a,b);}
V adc(REG8 a,REG8 b){x86(6,a,b);}
V adc(MEM8 a,AL b){x86(6,a,b);}
V adc(MEM8 a,CL b){x86(6,a,b);}
V adc(MEM8 a,REG8 b){x86(6,a,b);}
V adc(R_M8 a,AL b){x86(6,a,b);}
V adc(R_M8 a,CL b){x86(6,a,b);}
V adc(R_M8 a,REG8 b){x86(6,a,b);}
V adc(AX a,AX b){x86(7,a,b);}
V adc(AX a,DX b){x86(7,a,b);}
V adc(AX a,CX b){x86(7,a,b);}
V adc(AX a,REG16 b){x86(7,a,b);}
V adc(DX a,AX b){x86(7,a,b);}
V adc(DX a,DX b){x86(7,a,b);}
V adc(DX a,CX b){x86(7,a,b);}
V adc(DX a,REG16 b){x86(7,a,b);}
V adc(CX a,AX b){x86(7,a,b);}
V adc(CX a,DX b){x86(7,a,b);}
V adc(CX a,CX b){x86(7,a,b);}
V adc(CX a,REG16 b){x86(7,a,b);}
V adc(REG16 a,AX b){x86(7,a,b);}
V adc(REG16 a,DX b){x86(7,a,b);}
V adc(REG16 a,CX b){x86(7,a,b);}
V adc(REG16 a,REG16 b){x86(7,a,b);}
V adc(MEM16 a,AX b){x86(7,a,b);}
V adc(MEM16 a,DX b){x86(7,a,b);}
V adc(MEM16 a,CX b){x86(7,a,b);}
V adc(MEM16 a,REG16 b){x86(7,a,b);}
V adc(R_M16 a,AX b){x86(7,a,b);}
V adc(R_M16 a,DX b){x86(7,a,b);}
V adc(R_M16 a,CX b){x86(7,a,b);}
V adc(R_M16 a,REG16 b){x86(7,a,b);}
V adc(EAX a,EAX b){x86(8,a,b);}
V adc(EAX a,ECX b){x86(8,a,b);}
V adc(EAX a,REG32 b){x86(8,a,b);}
V adc(ECX a,EAX b){x86(8,a,b);}
V adc(ECX a,ECX b){x86(8,a,b);}
V adc(ECX a,REG32 b){x86(8,a,b);}
V adc(REG32 a,EAX b){x86(8,a,b);}
V adc(REG32 a,ECX b){x86(8,a,b);}
V adc(REG32 a,REG32 b){x86(8,a,b);}
V adc(MEM32 a,EAX b){x86(8,a,b);}
V adc(MEM32 a,ECX b){x86(8,a,b);}
V adc(MEM32 a,REG32 b){x86(8,a,b);}
V adc(R_M32 a,EAX b){x86(8,a,b);}
V adc(R_M32 a,ECX b){x86(8,a,b);}
V adc(R_M32 a,REG32 b){x86(8,a,b);}
V lock_adc(MEM8 a,AL b){x86(9,a,b);}
V lock_adc(MEM8 a,CL b){x86(9,a,b);}
V lock_adc(MEM8 a,REG8 b){x86(9,a,b);}
V lock_adc(MEM16 a,AX b){x86(10,a,b);}
V lock_adc(MEM16 a,DX b){x86(10,a,b);}
V lock_adc(MEM16 a,CX b){x86(10,a,b);}
V lock_adc(MEM16 a,REG16 b){x86(10,a,b);}
V lock_adc(MEM32 a,EAX b){x86(11,a,b);}
V lock_adc(MEM32 a,ECX b){x86(11,a,b);}
V lock_adc(MEM32 a,REG32 b){x86(11,a,b);}
V adc(AL a,MEM8 b){x86(12,a,b);}
V adc(AL a,R_M8 b){x86(12,a,b);}
V adc(CL a,MEM8 b){x86(12,a,b);}
V adc(CL a,R_M8 b){x86(12,a,b);}
V adc(REG8 a,MEM8 b){x86(12,a,b);}
V adc(REG8 a,R_M8 b){x86(12,a,b);}
V adc(AX a,MEM16 b){x86(13,a,b);}
V adc(AX a,R_M16 b){x86(13,a,b);}
V adc(DX a,MEM16 b){x86(13,a,b);}
V adc(DX a,R_M16 b){x86(13,a,b);}
V adc(CX a,MEM16 b){x86(13,a,b);}
V adc(CX a,R_M16 b){x86(13,a,b);}
V adc(REG16 a,MEM16 b){x86(13,a,b);}
V adc(REG16 a,R_M16 b){x86(13,a,b);}
V adc(EAX a,MEM32 b){x86(14,a,b);}
V adc(EAX a,R_M32 b){x86(14,a,b);}
V adc(ECX a,MEM32 b){x86(14,a,b);}
V adc(ECX a,R_M32 b){x86(14,a,b);}
V adc(REG32 a,MEM32 b){x86(14,a,b);}
V adc(REG32 a,R_M32 b){x86(14,a,b);}
V adc(AL a,char b){x86(15,a,(IMM)b);}
V adc(CL a,char b){x86(15,a,(IMM)b);}
V adc(REG8 a,char b){x86(15,a,(IMM)b);}
V adc(MEM8 a,char b){x86(15,a,(IMM)b);}
V adc(R_M8 a,char b){x86(15,a,(IMM)b);}
V adc(AX a,char b){x86(16,a,(IMM)b);}
V adc(AX a,short b){x86(16,a,(IMM)b);}
V adc(DX a,char b){x86(16,a,(IMM)b);}
V adc(DX a,short b){x86(16,a,(IMM)b);}
V adc(CX a,char b){x86(16,a,(IMM)b);}
V adc(CX a,short b){x86(16,a,(IMM)b);}
V adc(REG16 a,char b){x86(16,a,(IMM)b);}
V adc(REG16 a,short b){x86(16,a,(IMM)b);}
V adc(MEM16 a,char b){x86(16,a,(IMM)b);}
V adc(MEM16 a,short b){x86(16,a,(IMM)b);}
V adc(R_M16 a,char b){x86(16,a,(IMM)b);}
V adc(R_M16 a,short b){x86(16,a,(IMM)b);}
V adc(EAX a,int b){x86(17,a,(IMM)b);}
V adc(EAX a,char b){x86(17,a,(IMM)b);}
V adc(EAX a,short b){x86(17,a,(IMM)b);}
V adc(EAX a,REF b){x86(17,a,b);}
V adc(ECX a,int b){x86(17,a,(IMM)b);}
V adc(ECX a,char b){x86(17,a,(IMM)b);}
V adc(ECX a,short b){x86(17,a,(IMM)b);}
V adc(ECX a,REF b){x86(17,a,b);}
V adc(REG32 a,int b){x86(17,a,(IMM)b);}
V adc(REG32 a,char b){x86(17,a,(IMM)b);}
V adc(REG32 a,short b){x86(17,a,(IMM)b);}
V adc(REG32 a,REF b){x86(17,a,b);}
V adc(MEM32 a,int b){x86(17,a,(IMM)b);}
V adc(MEM32 a,char b){x86(17,a,(IMM)b);}
V adc(MEM32 a,short b){x86(17,a,(IMM)b);}
V adc(MEM32 a,REF b){x86(17,a,b);}
V adc(R_M32 a,int b){x86(17,a,(IMM)b);}
V adc(R_M32 a,char b){x86(17,a,(IMM)b);}
V adc(R_M32 a,short b){x86(17,a,(IMM)b);}
V adc(R_M32 a,REF b){x86(17,a,b);}
V lock_adc(MEM8 a,char b){x86(20,a,(IMM)b);}
V lock_adc(MEM16 a,char b){x86(21,a,(IMM)b);}
V lock_adc(MEM16 a,short b){x86(21,a,(IMM)b);}
V lock_adc(MEM32 a,int b){x86(22,a,(IMM)b);}
V lock_adc(MEM32 a,char b){x86(22,a,(IMM)b);}
V lock_adc(MEM32 a,short b){x86(22,a,(IMM)b);}
V lock_adc(MEM32 a,REF b){x86(22,a,b);}
V add(AL a,AL b){x86(28,a,b);}
V add(AL a,CL b){x86(28,a,b);}
V add(AL a,REG8 b){x86(28,a,b);}
V add(CL a,AL b){x86(28,a,b);}
V add(CL a,CL b){x86(28,a,b);}
V add(CL a,REG8 b){x86(28,a,b);}
V add(REG8 a,AL b){x86(28,a,b);}
V add(REG8 a,CL b){x86(28,a,b);}
V add(REG8 a,REG8 b){x86(28,a,b);}
V add(MEM8 a,AL b){x86(28,a,b);}
V add(MEM8 a,CL b){x86(28,a,b);}
V add(MEM8 a,REG8 b){x86(28,a,b);}
V add(R_M8 a,AL b){x86(28,a,b);}
V add(R_M8 a,CL b){x86(28,a,b);}
V add(R_M8 a,REG8 b){x86(28,a,b);}
V add(AX a,AX b){x86(29,a,b);}
V add(AX a,DX b){x86(29,a,b);}
V add(AX a,CX b){x86(29,a,b);}
V add(AX a,REG16 b){x86(29,a,b);}
V add(DX a,AX b){x86(29,a,b);}
V add(DX a,DX b){x86(29,a,b);}
V add(DX a,CX b){x86(29,a,b);}
V add(DX a,REG16 b){x86(29,a,b);}
V add(CX a,AX b){x86(29,a,b);}
V add(CX a,DX b){x86(29,a,b);}
V add(CX a,CX b){x86(29,a,b);}
V add(CX a,REG16 b){x86(29,a,b);}
V add(REG16 a,AX b){x86(29,a,b);}
V add(REG16 a,DX b){x86(29,a,b);}
V add(REG16 a,CX b){x86(29,a,b);}
V add(REG16 a,REG16 b){x86(29,a,b);}
V add(MEM16 a,AX b){x86(29,a,b);}
V add(MEM16 a,DX b){x86(29,a,b);}
V add(MEM16 a,CX b){x86(29,a,b);}
V add(MEM16 a,REG16 b){x86(29,a,b);}
V add(R_M16 a,AX b){x86(29,a,b);}
V add(R_M16 a,DX b){x86(29,a,b);}
V add(R_M16 a,CX b){x86(29,a,b);}
V add(R_M16 a,REG16 b){x86(29,a,b);}
V add(EAX a,EAX b){x86(30,a,b);}
V add(EAX a,ECX b){x86(30,a,b);}
V add(EAX a,REG32 b){x86(30,a,b);}
V add(ECX a,EAX b){x86(30,a,b);}
V add(ECX a,ECX b){x86(30,a,b);}
V add(ECX a,REG32 b){x86(30,a,b);}
V add(REG32 a,EAX b){x86(30,a,b);}
V add(REG32 a,ECX b){x86(30,a,b);}
V add(REG32 a,REG32 b){x86(30,a,b);}
V add(MEM32 a,EAX b){x86(30,a,b);}
V add(MEM32 a,ECX b){x86(30,a,b);}
V add(MEM32 a,REG32 b){x86(30,a,b);}
V add(R_M32 a,EAX b){x86(30,a,b);}
V add(R_M32 a,ECX b){x86(30,a,b);}
V add(R_M32 a,REG32 b){x86(30,a,b);}
V lock_add(MEM8 a,AL b){x86(31,a,b);}
V lock_add(MEM8 a,CL b){x86(31,a,b);}
V lock_add(MEM8 a,REG8 b){x86(31,a,b);}
V lock_add(MEM16 a,AX b){x86(32,a,b);}
V lock_add(MEM16 a,DX b){x86(32,a,b);}
V lock_add(MEM16 a,CX b){x86(32,a,b);}
V lock_add(MEM16 a,REG16 b){x86(32,a,b);}
V lock_add(MEM32 a,EAX b){x86(33,a,b);}
V lock_add(MEM32 a,ECX b){x86(33,a,b);}
V lock_add(MEM32 a,REG32 b){x86(33,a,b);}
V add(AL a,MEM8 b){x86(34,a,b);}
V add(AL a,R_M8 b){x86(34,a,b);}
V add(CL a,MEM8 b){x86(34,a,b);}
V add(CL a,R_M8 b){x86(34,a,b);}
V add(REG8 a,MEM8 b){x86(34,a,b);}
V add(REG8 a,R_M8 b){x86(34,a,b);}
V add(AX a,MEM16 b){x86(35,a,b);}
V add(AX a,R_M16 b){x86(35,a,b);}
V add(DX a,MEM16 b){x86(35,a,b);}
V add(DX a,R_M16 b){x86(35,a,b);}
V add(CX a,MEM16 b){x86(35,a,b);}
V add(CX a,R_M16 b){x86(35,a,b);}
V add(REG16 a,MEM16 b){x86(35,a,b);}
V add(REG16 a,R_M16 b){x86(35,a,b);}
V add(EAX a,MEM32 b){x86(36,a,b);}
V add(EAX a,R_M32 b){x86(36,a,b);}
V add(ECX a,MEM32 b){x86(36,a,b);}
V add(ECX a,R_M32 b){x86(36,a,b);}
V add(REG32 a,MEM32 b){x86(36,a,b);}
V add(REG32 a,R_M32 b){x86(36,a,b);}
V add(AL a,char b){x86(37,a,(IMM)b);}
V add(CL a,char b){x86(37,a,(IMM)b);}
V add(REG8 a,char b){x86(37,a,(IMM)b);}
V add(MEM8 a,char b){x86(37,a,(IMM)b);}
V add(R_M8 a,char b){x86(37,a,(IMM)b);}
V add(AX a,char b){x86(38,a,(IMM)b);}
V add(AX a,short b){x86(38,a,(IMM)b);}
V add(DX a,char b){x86(38,a,(IMM)b);}
V add(DX a,short b){x86(38,a,(IMM)b);}
V add(CX a,char b){x86(38,a,(IMM)b);}
V add(CX a,short b){x86(38,a,(IMM)b);}
V add(REG16 a,char b){x86(38,a,(IMM)b);}
V add(REG16 a,short b){x86(38,a,(IMM)b);}
V add(MEM16 a,char b){x86(38,a,(IMM)b);}
V add(MEM16 a,short b){x86(38,a,(IMM)b);}
V add(R_M16 a,char b){x86(38,a,(IMM)b);}
V add(R_M16 a,short b){x86(38,a,(IMM)b);}
V add(EAX a,int b){x86(39,a,(IMM)b);}
V add(EAX a,char b){x86(39,a,(IMM)b);}
V add(EAX a,short b){x86(39,a,(IMM)b);}
V add(EAX a,REF b){x86(39,a,b);}
V add(ECX a,int b){x86(39,a,(IMM)b);}
V add(ECX a,char b){x86(39,a,(IMM)b);}
V add(ECX a,short b){x86(39,a,(IMM)b);}
V add(ECX a,REF b){x86(39,a,b);}
V add(REG32 a,int b){x86(39,a,(IMM)b);}
V add(REG32 a,char b){x86(39,a,(IMM)b);}
V add(REG32 a,short b){x86(39,a,(IMM)b);}
V add(REG32 a,REF b){x86(39,a,b);}
V add(MEM32 a,int b){x86(39,a,(IMM)b);}
V add(MEM32 a,char b){x86(39,a,(IMM)b);}
V add(MEM32 a,short b){x86(39,a,(IMM)b);}
V add(MEM32 a,REF b){x86(39,a,b);}
V add(R_M32 a,int b){x86(39,a,(IMM)b);}
V add(R_M32 a,char b){x86(39,a,(IMM)b);}
V add(R_M32 a,short b){x86(39,a,(IMM)b);}
V add(R_M32 a,REF b){x86(39,a,b);}
V lock_add(MEM8 a,char b){x86(42,a,(IMM)b);}
V lock_add(MEM16 a,char b){x86(43,a,(IMM)b);}
V lock_add(MEM16 a,short b){x86(43,a,(IMM)b);}
V lock_add(MEM32 a,int b){x86(44,a,(IMM)b);}
V lock_add(MEM32 a,char b){x86(44,a,(IMM)b);}
V lock_add(MEM32 a,short b){x86(44,a,(IMM)b);}
V lock_add(MEM32 a,REF b){x86(44,a,b);}
V addpd(XMMREG a,XMMREG b){x86(50,a,b);}
V addpd(XMMREG a,MEM128 b){x86(50,a,b);}
V addpd(XMMREG a,R_M128 b){x86(50,a,b);}
V addps(XMMREG a,XMMREG b){x86(51,a,b);}
V addps(XMMREG a,MEM128 b){x86(51,a,b);}
V addps(XMMREG a,R_M128 b){x86(51,a,b);}
V addsd(XMMREG a,XMMREG b){x86(52,a,b);}
V addsd(XMMREG a,MEM64 b){x86(52,a,b);}
V addsd(XMMREG a,XMM64 b){x86(52,a,b);}
V addss(XMMREG a,XMMREG b){x86(53,a,b);}
V addss(XMMREG a,MEM32 b){x86(53,a,b);}
V addss(XMMREG a,XMM32 b){x86(53,a,b);}
V and(AL a,AL b){x86(54,a,b);}
V and(AL a,CL b){x86(54,a,b);}
V and(AL a,REG8 b){x86(54,a,b);}
V and(CL a,AL b){x86(54,a,b);}
V and(CL a,CL b){x86(54,a,b);}
V and(CL a,REG8 b){x86(54,a,b);}
V and(REG8 a,AL b){x86(54,a,b);}
V and(REG8 a,CL b){x86(54,a,b);}
V and(REG8 a,REG8 b){x86(54,a,b);}
V and(MEM8 a,AL b){x86(54,a,b);}
V and(MEM8 a,CL b){x86(54,a,b);}
V and(MEM8 a,REG8 b){x86(54,a,b);}
V and(R_M8 a,AL b){x86(54,a,b);}
V and(R_M8 a,CL b){x86(54,a,b);}
V and(R_M8 a,REG8 b){x86(54,a,b);}
V and(AX a,AX b){x86(55,a,b);}
V and(AX a,DX b){x86(55,a,b);}
V and(AX a,CX b){x86(55,a,b);}
V and(AX a,REG16 b){x86(55,a,b);}
V and(DX a,AX b){x86(55,a,b);}
V and(DX a,DX b){x86(55,a,b);}
V and(DX a,CX b){x86(55,a,b);}
V and(DX a,REG16 b){x86(55,a,b);}
V and(CX a,AX b){x86(55,a,b);}
V and(CX a,DX b){x86(55,a,b);}
V and(CX a,CX b){x86(55,a,b);}
V and(CX a,REG16 b){x86(55,a,b);}
V and(REG16 a,AX b){x86(55,a,b);}
V and(REG16 a,DX b){x86(55,a,b);}
V and(REG16 a,CX b){x86(55,a,b);}
V and(REG16 a,REG16 b){x86(55,a,b);}
V and(MEM16 a,AX b){x86(55,a,b);}
V and(MEM16 a,DX b){x86(55,a,b);}
V and(MEM16 a,CX b){x86(55,a,b);}
V and(MEM16 a,REG16 b){x86(55,a,b);}
V and(R_M16 a,AX b){x86(55,a,b);}
V and(R_M16 a,DX b){x86(55,a,b);}
V and(R_M16 a,CX b){x86(55,a,b);}
V and(R_M16 a,REG16 b){x86(55,a,b);}
V and(EAX a,EAX b){x86(56,a,b);}
V and(EAX a,ECX b){x86(56,a,b);}
V and(EAX a,REG32 b){x86(56,a,b);}
V and(ECX a,EAX b){x86(56,a,b);}
V and(ECX a,ECX b){x86(56,a,b);}
V and(ECX a,REG32 b){x86(56,a,b);}
V and(REG32 a,EAX b){x86(56,a,b);}
V and(REG32 a,ECX b){x86(56,a,b);}
V and(REG32 a,REG32 b){x86(56,a,b);}
V and(MEM32 a,EAX b){x86(56,a,b);}
V and(MEM32 a,ECX b){x86(56,a,b);}
V and(MEM32 a,REG32 b){x86(56,a,b);}
V and(R_M32 a,EAX b){x86(56,a,b);}
V and(R_M32 a,ECX b){x86(56,a,b);}
V and(R_M32 a,REG32 b){x86(56,a,b);}
V lock_and(MEM8 a,AL b){x86(57,a,b);}
V lock_and(MEM8 a,CL b){x86(57,a,b);}
V lock_and(MEM8 a,REG8 b){x86(57,a,b);}
V lock_and(MEM16 a,AX b){x86(58,a,b);}
V lock_and(MEM16 a,DX b){x86(58,a,b);}
V lock_and(MEM16 a,CX b){x86(58,a,b);}
V lock_and(MEM16 a,REG16 b){x86(58,a,b);}
V lock_and(MEM32 a,EAX b){x86(59,a,b);}
V lock_and(MEM32 a,ECX b){x86(59,a,b);}
V lock_and(MEM32 a,REG32 b){x86(59,a,b);}
V and(AL a,MEM8 b){x86(60,a,b);}
V and(AL a,R_M8 b){x86(60,a,b);}
V and(CL a,MEM8 b){x86(60,a,b);}
V and(CL a,R_M8 b){x86(60,a,b);}
V and(REG8 a,MEM8 b){x86(60,a,b);}
V and(REG8 a,R_M8 b){x86(60,a,b);}
V and(AX a,MEM16 b){x86(61,a,b);}
V and(AX a,R_M16 b){x86(61,a,b);}
V and(DX a,MEM16 b){x86(61,a,b);}
V and(DX a,R_M16 b){x86(61,a,b);}
V and(CX a,MEM16 b){x86(61,a,b);}
V and(CX a,R_M16 b){x86(61,a,b);}
V and(REG16 a,MEM16 b){x86(61,a,b);}
V and(REG16 a,R_M16 b){x86(61,a,b);}
V and(EAX a,MEM32 b){x86(62,a,b);}
V and(EAX a,R_M32 b){x86(62,a,b);}
V and(ECX a,MEM32 b){x86(62,a,b);}
V and(ECX a,R_M32 b){x86(62,a,b);}
V and(REG32 a,MEM32 b){x86(62,a,b);}
V and(REG32 a,R_M32 b){x86(62,a,b);}
V and(AL a,char b){x86(63,a,(IMM)b);}
V and(CL a,char b){x86(63,a,(IMM)b);}
V and(REG8 a,char b){x86(63,a,(IMM)b);}
V and(MEM8 a,char b){x86(63,a,(IMM)b);}
V and(R_M8 a,char b){x86(63,a,(IMM)b);}
V and(AX a,char b){x86(64,a,(IMM)b);}
V and(AX a,short b){x86(64,a,(IMM)b);}
V and(DX a,char b){x86(64,a,(IMM)b);}
V and(DX a,short b){x86(64,a,(IMM)b);}
V and(CX a,char b){x86(64,a,(IMM)b);}
V and(CX a,short b){x86(64,a,(IMM)b);}
V and(REG16 a,char b){x86(64,a,(IMM)b);}
V and(REG16 a,short b){x86(64,a,(IMM)b);}
V and(MEM16 a,char b){x86(64,a,(IMM)b);}
V and(MEM16 a,short b){x86(64,a,(IMM)b);}
V and(R_M16 a,char b){x86(64,a,(IMM)b);}
V and(R_M16 a,short b){x86(64,a,(IMM)b);}
V and(EAX a,int b){x86(65,a,(IMM)b);}
V and(EAX a,char b){x86(65,a,(IMM)b);}
V and(EAX a,short b){x86(65,a,(IMM)b);}
V and(EAX a,REF b){x86(65,a,b);}
V and(ECX a,int b){x86(65,a,(IMM)b);}
V and(ECX a,char b){x86(65,a,(IMM)b);}
V and(ECX a,short b){x86(65,a,(IMM)b);}
V and(ECX a,REF b){x86(65,a,b);}
V and(REG32 a,int b){x86(65,a,(IMM)b);}
V and(REG32 a,char b){x86(65,a,(IMM)b);}
V and(REG32 a,short b){x86(65,a,(IMM)b);}
V and(REG32 a,REF b){x86(65,a,b);}
V and(MEM32 a,int b){x86(65,a,(IMM)b);}
V and(MEM32 a,char b){x86(65,a,(IMM)b);}
V and(MEM32 a,short b){x86(65,a,(IMM)b);}
V and(MEM32 a,REF b){x86(65,a,b);}
V and(R_M32 a,int b){x86(65,a,(IMM)b);}
V and(R_M32 a,char b){x86(65,a,(IMM)b);}
V and(R_M32 a,short b){x86(65,a,(IMM)b);}
V and(R_M32 a,REF b){x86(65,a,b);}
V lock_and(MEM8 a,char b){x86(68,a,(IMM)b);}
V lock_and(MEM16 a,char b){x86(69,a,(IMM)b);}
V lock_and(MEM16 a,short b){x86(69,a,(IMM)b);}
V lock_and(MEM32 a,int b){x86(70,a,(IMM)b);}
V lock_and(MEM32 a,char b){x86(70,a,(IMM)b);}
V lock_and(MEM32 a,short b){x86(70,a,(IMM)b);}
V lock_and(MEM32 a,REF b){x86(70,a,b);}
V andnpd(XMMREG a,XMMREG b){x86(76,a,b);}
V andnpd(XMMREG a,MEM128 b){x86(76,a,b);}
V andnpd(XMMREG a,R_M128 b){x86(76,a,b);}
V andnps(XMMREG a,XMMREG b){x86(77,a,b);}
V andnps(XMMREG a,MEM128 b){x86(77,a,b);}
V andnps(XMMREG a,R_M128 b){x86(77,a,b);}
V andpd(XMMREG a,XMMREG b){x86(78,a,b);}
V andpd(XMMREG a,MEM128 b){x86(78,a,b);}
V andpd(XMMREG a,R_M128 b){x86(78,a,b);}
V andps(XMMREG a,XMMREG b){x86(79,a,b);}
V andps(XMMREG a,MEM128 b){x86(79,a,b);}
V andps(XMMREG a,R_M128 b){x86(79,a,b);}
V bound(AX a,MEM8 b){x86(80,a,b);}
V bound(AX a,MEM16 b){x86(80,a,b);}
V bound(AX a,MEM32 b){x86(80,a,b);}
V bound(AX a,MEM64 b){x86(80,a,b);}
V bound(AX a,MEM128 b){x86(80,a,b);}
V bound(DX a,MEM8 b){x86(80,a,b);}
V bound(DX a,MEM16 b){x86(80,a,b);}
V bound(DX a,MEM32 b){x86(80,a,b);}
V bound(DX a,MEM64 b){x86(80,a,b);}
V bound(DX a,MEM128 b){x86(80,a,b);}
V bound(CX a,MEM8 b){x86(80,a,b);}
V bound(CX a,MEM16 b){x86(80,a,b);}
V bound(CX a,MEM32 b){x86(80,a,b);}
V bound(CX a,MEM64 b){x86(80,a,b);}
V bound(CX a,MEM128 b){x86(80,a,b);}
V bound(REG16 a,MEM8 b){x86(80,a,b);}
V bound(REG16 a,MEM16 b){x86(80,a,b);}
V bound(REG16 a,MEM32 b){x86(80,a,b);}
V bound(REG16 a,MEM64 b){x86(80,a,b);}
V bound(REG16 a,MEM128 b){x86(80,a,b);}
V bound(EAX a,MEM8 b){x86(81,a,b);}
V bound(EAX a,MEM16 b){x86(81,a,b);}
V bound(EAX a,MEM32 b){x86(81,a,b);}
V bound(EAX a,MEM64 b){x86(81,a,b);}
V bound(EAX a,MEM128 b){x86(81,a,b);}
V bound(ECX a,MEM8 b){x86(81,a,b);}
V bound(ECX a,MEM16 b){x86(81,a,b);}
V bound(ECX a,MEM32 b){x86(81,a,b);}
V bound(ECX a,MEM64 b){x86(81,a,b);}
V bound(ECX a,MEM128 b){x86(81,a,b);}
V bound(REG32 a,MEM8 b){x86(81,a,b);}
V bound(REG32 a,MEM16 b){x86(81,a,b);}
V bound(REG32 a,MEM32 b){x86(81,a,b);}
V bound(REG32 a,MEM64 b){x86(81,a,b);}
V bound(REG32 a,MEM128 b){x86(81,a,b);}
V bsf(AX a,AX b){x86(82,a,b);}
V bsf(AX a,DX b){x86(82,a,b);}
V bsf(AX a,CX b){x86(82,a,b);}
V bsf(AX a,REG16 b){x86(82,a,b);}
V bsf(AX a,MEM16 b){x86(82,a,b);}
V bsf(AX a,R_M16 b){x86(82,a,b);}
V bsf(DX a,AX b){x86(82,a,b);}
V bsf(DX a,DX b){x86(82,a,b);}
V bsf(DX a,CX b){x86(82,a,b);}
V bsf(DX a,REG16 b){x86(82,a,b);}
V bsf(DX a,MEM16 b){x86(82,a,b);}
V bsf(DX a,R_M16 b){x86(82,a,b);}
V bsf(CX a,AX b){x86(82,a,b);}
V bsf(CX a,DX b){x86(82,a,b);}
V bsf(CX a,CX b){x86(82,a,b);}
V bsf(CX a,REG16 b){x86(82,a,b);}
V bsf(CX a,MEM16 b){x86(82,a,b);}
V bsf(CX a,R_M16 b){x86(82,a,b);}
V bsf(REG16 a,AX b){x86(82,a,b);}
V bsf(REG16 a,DX b){x86(82,a,b);}
V bsf(REG16 a,CX b){x86(82,a,b);}
V bsf(REG16 a,REG16 b){x86(82,a,b);}
V bsf(REG16 a,MEM16 b){x86(82,a,b);}
V bsf(REG16 a,R_M16 b){x86(82,a,b);}
V bsf(EAX a,EAX b){x86(83,a,b);}
V bsf(EAX a,ECX b){x86(83,a,b);}
V bsf(EAX a,REG32 b){x86(83,a,b);}
V bsf(EAX a,MEM32 b){x86(83,a,b);}
V bsf(EAX a,R_M32 b){x86(83,a,b);}
V bsf(ECX a,EAX b){x86(83,a,b);}
V bsf(ECX a,ECX b){x86(83,a,b);}
V bsf(ECX a,REG32 b){x86(83,a,b);}
V bsf(ECX a,MEM32 b){x86(83,a,b);}
V bsf(ECX a,R_M32 b){x86(83,a,b);}
V bsf(REG32 a,EAX b){x86(83,a,b);}
V bsf(REG32 a,ECX b){x86(83,a,b);}
V bsf(REG32 a,REG32 b){x86(83,a,b);}
V bsf(REG32 a,MEM32 b){x86(83,a,b);}
V bsf(REG32 a,R_M32 b){x86(83,a,b);}
V bsr(AX a,AX b){x86(84,a,b);}
V bsr(AX a,DX b){x86(84,a,b);}
V bsr(AX a,CX b){x86(84,a,b);}
V bsr(AX a,REG16 b){x86(84,a,b);}
V bsr(AX a,MEM16 b){x86(84,a,b);}
V bsr(AX a,R_M16 b){x86(84,a,b);}
V bsr(DX a,AX b){x86(84,a,b);}
V bsr(DX a,DX b){x86(84,a,b);}
V bsr(DX a,CX b){x86(84,a,b);}
V bsr(DX a,REG16 b){x86(84,a,b);}
V bsr(DX a,MEM16 b){x86(84,a,b);}
V bsr(DX a,R_M16 b){x86(84,a,b);}
V bsr(CX a,AX b){x86(84,a,b);}
V bsr(CX a,DX b){x86(84,a,b);}
V bsr(CX a,CX b){x86(84,a,b);}
V bsr(CX a,REG16 b){x86(84,a,b);}
V bsr(CX a,MEM16 b){x86(84,a,b);}
V bsr(CX a,R_M16 b){x86(84,a,b);}
V bsr(REG16 a,AX b){x86(84,a,b);}
V bsr(REG16 a,DX b){x86(84,a,b);}
V bsr(REG16 a,CX b){x86(84,a,b);}
V bsr(REG16 a,REG16 b){x86(84,a,b);}
V bsr(REG16 a,MEM16 b){x86(84,a,b);}
V bsr(REG16 a,R_M16 b){x86(84,a,b);}
V bsr(EAX a,EAX b){x86(85,a,b);}
V bsr(EAX a,ECX b){x86(85,a,b);}
V bsr(EAX a,REG32 b){x86(85,a,b);}
V bsr(EAX a,MEM32 b){x86(85,a,b);}
V bsr(EAX a,R_M32 b){x86(85,a,b);}
V bsr(ECX a,EAX b){x86(85,a,b);}
V bsr(ECX a,ECX b){x86(85,a,b);}
V bsr(ECX a,REG32 b){x86(85,a,b);}
V bsr(ECX a,MEM32 b){x86(85,a,b);}
V bsr(ECX a,R_M32 b){x86(85,a,b);}
V bsr(REG32 a,EAX b){x86(85,a,b);}
V bsr(REG32 a,ECX b){x86(85,a,b);}
V bsr(REG32 a,REG32 b){x86(85,a,b);}
V bsr(REG32 a,MEM32 b){x86(85,a,b);}
V bsr(REG32 a,R_M32 b){x86(85,a,b);}
V bswap(EAX a){x86(86,a);}
V bswap(ECX a){x86(86,a);}
V bswap(REG32 a){x86(86,a);}
V bt(AX a,AX b){x86(87,a,b);}
V bt(AX a,DX b){x86(87,a,b);}
V bt(AX a,CX b){x86(87,a,b);}
V bt(AX a,REG16 b){x86(87,a,b);}
V bt(DX a,AX b){x86(87,a,b);}
V bt(DX a,DX b){x86(87,a,b);}
V bt(DX a,CX b){x86(87,a,b);}
V bt(DX a,REG16 b){x86(87,a,b);}
V bt(CX a,AX b){x86(87,a,b);}
V bt(CX a,DX b){x86(87,a,b);}
V bt(CX a,CX b){x86(87,a,b);}
V bt(CX a,REG16 b){x86(87,a,b);}
V bt(REG16 a,AX b){x86(87,a,b);}
V bt(REG16 a,DX b){x86(87,a,b);}
V bt(REG16 a,CX b){x86(87,a,b);}
V bt(REG16 a,REG16 b){x86(87,a,b);}
V bt(MEM16 a,AX b){x86(87,a,b);}
V bt(MEM16 a,DX b){x86(87,a,b);}
V bt(MEM16 a,CX b){x86(87,a,b);}
V bt(MEM16 a,REG16 b){x86(87,a,b);}
V bt(R_M16 a,AX b){x86(87,a,b);}
V bt(R_M16 a,DX b){x86(87,a,b);}
V bt(R_M16 a,CX b){x86(87,a,b);}
V bt(R_M16 a,REG16 b){x86(87,a,b);}
V bt(EAX a,EAX b){x86(88,a,b);}
V bt(EAX a,ECX b){x86(88,a,b);}
V bt(EAX a,REG32 b){x86(88,a,b);}
V bt(ECX a,EAX b){x86(88,a,b);}
V bt(ECX a,ECX b){x86(88,a,b);}
V bt(ECX a,REG32 b){x86(88,a,b);}
V bt(REG32 a,EAX b){x86(88,a,b);}
V bt(REG32 a,ECX b){x86(88,a,b);}
V bt(REG32 a,REG32 b){x86(88,a,b);}
V bt(MEM32 a,EAX b){x86(88,a,b);}
V bt(MEM32 a,ECX b){x86(88,a,b);}
V bt(MEM32 a,REG32 b){x86(88,a,b);}
V bt(R_M32 a,EAX b){x86(88,a,b);}
V bt(R_M32 a,ECX b){x86(88,a,b);}
V bt(R_M32 a,REG32 b){x86(88,a,b);}
V btc(AX a,AX b){x86(91,a,b);}
V btc(AX a,DX b){x86(91,a,b);}
V btc(AX a,CX b){x86(91,a,b);}
V btc(AX a,REG16 b){x86(91,a,b);}
V btc(DX a,AX b){x86(91,a,b);}
V btc(DX a,DX b){x86(91,a,b);}
V btc(DX a,CX b){x86(91,a,b);}
V btc(DX a,REG16 b){x86(91,a,b);}
V btc(CX a,AX b){x86(91,a,b);}
V btc(CX a,DX b){x86(91,a,b);}
V btc(CX a,CX b){x86(91,a,b);}
V btc(CX a,REG16 b){x86(91,a,b);}
V btc(REG16 a,AX b){x86(91,a,b);}
V btc(REG16 a,DX b){x86(91,a,b);}
V btc(REG16 a,CX b){x86(91,a,b);}
V btc(REG16 a,REG16 b){x86(91,a,b);}
V btc(MEM16 a,AX b){x86(91,a,b);}
V btc(MEM16 a,DX b){x86(91,a,b);}
V btc(MEM16 a,CX b){x86(91,a,b);}
V btc(MEM16 a,REG16 b){x86(91,a,b);}
V btc(R_M16 a,AX b){x86(91,a,b);}
V btc(R_M16 a,DX b){x86(91,a,b);}
V btc(R_M16 a,CX b){x86(91,a,b);}
V btc(R_M16 a,REG16 b){x86(91,a,b);}
V btc(EAX a,EAX b){x86(92,a,b);}
V btc(EAX a,ECX b){x86(92,a,b);}
V btc(EAX a,REG32 b){x86(92,a,b);}
V btc(ECX a,EAX b){x86(92,a,b);}
V btc(ECX a,ECX b){x86(92,a,b);}
V btc(ECX a,REG32 b){x86(92,a,b);}
V btc(REG32 a,EAX b){x86(92,a,b);}
V btc(REG32 a,ECX b){x86(92,a,b);}
V btc(REG32 a,REG32 b){x86(92,a,b);}
V btc(MEM32 a,EAX b){x86(92,a,b);}
V btc(MEM32 a,ECX b){x86(92,a,b);}
V btc(MEM32 a,REG32 b){x86(92,a,b);}
V btc(R_M32 a,EAX b){x86(92,a,b);}
V btc(R_M32 a,ECX b){x86(92,a,b);}
V btc(R_M32 a,REG32 b){x86(92,a,b);}
V btr(AX a,AX b){x86(95,a,b);}
V btr(AX a,DX b){x86(95,a,b);}
V btr(AX a,CX b){x86(95,a,b);}
V btr(AX a,REG16 b){x86(95,a,b);}
V btr(DX a,AX b){x86(95,a,b);}
V btr(DX a,DX b){x86(95,a,b);}
V btr(DX a,CX b){x86(95,a,b);}
V btr(DX a,REG16 b){x86(95,a,b);}
V btr(CX a,AX b){x86(95,a,b);}
V btr(CX a,DX b){x86(95,a,b);}
V btr(CX a,CX b){x86(95,a,b);}
V btr(CX a,REG16 b){x86(95,a,b);}
V btr(REG16 a,AX b){x86(95,a,b);}
V btr(REG16 a,DX b){x86(95,a,b);}
V btr(REG16 a,CX b){x86(95,a,b);}
V btr(REG16 a,REG16 b){x86(95,a,b);}
V btr(MEM16 a,AX b){x86(95,a,b);}
V btr(MEM16 a,DX b){x86(95,a,b);}
V btr(MEM16 a,CX b){x86(95,a,b);}
V btr(MEM16 a,REG16 b){x86(95,a,b);}
V btr(R_M16 a,AX b){x86(95,a,b);}
V btr(R_M16 a,DX b){x86(95,a,b);}
V btr(R_M16 a,CX b){x86(95,a,b);}
V btr(R_M16 a,REG16 b){x86(95,a,b);}
V btr(EAX a,EAX b){x86(96,a,b);}
V btr(EAX a,ECX b){x86(96,a,b);}
V btr(EAX a,REG32 b){x86(96,a,b);}
V btr(ECX a,EAX b){x86(96,a,b);}
V btr(ECX a,ECX b){x86(96,a,b);}
V btr(ECX a,REG32 b){x86(96,a,b);}
V btr(REG32 a,EAX b){x86(96,a,b);}
V btr(REG32 a,ECX b){x86(96,a,b);}
V btr(REG32 a,REG32 b){x86(96,a,b);}
V btr(MEM32 a,EAX b){x86(96,a,b);}
V btr(MEM32 a,ECX b){x86(96,a,b);}
V btr(MEM32 a,REG32 b){x86(96,a,b);}
V btr(R_M32 a,EAX b){x86(96,a,b);}
V btr(R_M32 a,ECX b){x86(96,a,b);}
V btr(R_M32 a,REG32 b){x86(96,a,b);}
V bts(AX a,AX b){x86(99,a,b);}
V bts(AX a,DX b){x86(99,a,b);}
V bts(AX a,CX b){x86(99,a,b);}
V bts(AX a,REG16 b){x86(99,a,b);}
V bts(DX a,AX b){x86(99,a,b);}
V bts(DX a,DX b){x86(99,a,b);}
V bts(DX a,CX b){x86(99,a,b);}
V bts(DX a,REG16 b){x86(99,a,b);}
V bts(CX a,AX b){x86(99,a,b);}
V bts(CX a,DX b){x86(99,a,b);}
V bts(CX a,CX b){x86(99,a,b);}
V bts(CX a,REG16 b){x86(99,a,b);}
V bts(REG16 a,AX b){x86(99,a,b);}
V bts(REG16 a,DX b){x86(99,a,b);}
V bts(REG16 a,CX b){x86(99,a,b);}
V bts(REG16 a,REG16 b){x86(99,a,b);}
V bts(MEM16 a,AX b){x86(99,a,b);}
V bts(MEM16 a,DX b){x86(99,a,b);}
V bts(MEM16 a,CX b){x86(99,a,b);}
V bts(MEM16 a,REG16 b){x86(99,a,b);}
V bts(R_M16 a,AX b){x86(99,a,b);}
V bts(R_M16 a,DX b){x86(99,a,b);}
V bts(R_M16 a,CX b){x86(99,a,b);}
V bts(R_M16 a,REG16 b){x86(99,a,b);}
V bts(EAX a,EAX b){x86(100,a,b);}
V bts(EAX a,ECX b){x86(100,a,b);}
V bts(EAX a,REG32 b){x86(100,a,b);}
V bts(ECX a,EAX b){x86(100,a,b);}
V bts(ECX a,ECX b){x86(100,a,b);}
V bts(ECX a,REG32 b){x86(100,a,b);}
V bts(REG32 a,EAX b){x86(100,a,b);}
V bts(REG32 a,ECX b){x86(100,a,b);}
V bts(REG32 a,REG32 b){x86(100,a,b);}
V bts(MEM32 a,EAX b){x86(100,a,b);}
V bts(MEM32 a,ECX b){x86(100,a,b);}
V bts(MEM32 a,REG32 b){x86(100,a,b);}
V bts(R_M32 a,EAX b){x86(100,a,b);}
V bts(R_M32 a,ECX b){x86(100,a,b);}
V bts(R_M32 a,REG32 b){x86(100,a,b);}
V bts(AX a,int b){x86(101,a,(IMM)b);}
V bts(AX a,char b){x86(101,a,(IMM)b);}
V bts(AX a,short b){x86(101,a,(IMM)b);}
V bts(AX a,REF b){x86(101,a,b);}
V bts(DX a,int b){x86(101,a,(IMM)b);}
V bts(DX a,char b){x86(101,a,(IMM)b);}
V bts(DX a,short b){x86(101,a,(IMM)b);}
V bts(DX a,REF b){x86(101,a,b);}
V bts(CX a,int b){x86(101,a,(IMM)b);}
V bts(CX a,char b){x86(101,a,(IMM)b);}
V bts(CX a,short b){x86(101,a,(IMM)b);}
V bts(CX a,REF b){x86(101,a,b);}
V bts(REG16 a,int b){x86(101,a,(IMM)b);}
V bts(REG16 a,char b){x86(101,a,(IMM)b);}
V bts(REG16 a,short b){x86(101,a,(IMM)b);}
V bts(REG16 a,REF b){x86(101,a,b);}
V bts(MEM16 a,int b){x86(101,a,(IMM)b);}
V bts(MEM16 a,char b){x86(101,a,(IMM)b);}
V bts(MEM16 a,short b){x86(101,a,(IMM)b);}
V bts(MEM16 a,REF b){x86(101,a,b);}
V bts(R_M16 a,int b){x86(101,a,(IMM)b);}
V bts(R_M16 a,char b){x86(101,a,(IMM)b);}
V bts(R_M16 a,short b){x86(101,a,(IMM)b);}
V bts(R_M16 a,REF b){x86(101,a,b);}
V bts(EAX a,int b){x86(102,a,(IMM)b);}
V bts(EAX a,char b){x86(102,a,(IMM)b);}
V bts(EAX a,short b){x86(102,a,(IMM)b);}
V bts(EAX a,REF b){x86(102,a,b);}
V bts(ECX a,int b){x86(102,a,(IMM)b);}
V bts(ECX a,char b){x86(102,a,(IMM)b);}
V bts(ECX a,short b){x86(102,a,(IMM)b);}
V bts(ECX a,REF b){x86(102,a,b);}
V bts(REG32 a,int b){x86(102,a,(IMM)b);}
V bts(REG32 a,char b){x86(102,a,(IMM)b);}
V bts(REG32 a,short b){x86(102,a,(IMM)b);}
V bts(REG32 a,REF b){x86(102,a,b);}
V bts(MEM32 a,int b){x86(102,a,(IMM)b);}
V bts(MEM32 a,char b){x86(102,a,(IMM)b);}
V bts(MEM32 a,short b){x86(102,a,(IMM)b);}
V bts(MEM32 a,REF b){x86(102,a,b);}
V bts(R_M32 a,int b){x86(102,a,(IMM)b);}
V bts(R_M32 a,char b){x86(102,a,(IMM)b);}
V bts(R_M32 a,short b){x86(102,a,(IMM)b);}
V bts(R_M32 a,REF b){x86(102,a,b);}
V lock_btc(MEM16 a,AX b){x86(103,a,b);}
V lock_btc(MEM16 a,DX b){x86(103,a,b);}
V lock_btc(MEM16 a,CX b){x86(103,a,b);}
V lock_btc(MEM16 a,REG16 b){x86(103,a,b);}
V lock_btc(MEM32 a,EAX b){x86(104,a,b);}
V lock_btc(MEM32 a,ECX b){x86(104,a,b);}
V lock_btc(MEM32 a,REG32 b){x86(104,a,b);}
V lock_btr(MEM16 a,AX b){x86(107,a,b);}
V lock_btr(MEM16 a,DX b){x86(107,a,b);}
V lock_btr(MEM16 a,CX b){x86(107,a,b);}
V lock_btr(MEM16 a,REG16 b){x86(107,a,b);}
V lock_btr(MEM32 a,EAX b){x86(108,a,b);}
V lock_btr(MEM32 a,ECX b){x86(108,a,b);}
V lock_btr(MEM32 a,REG32 b){x86(108,a,b);}
V lock_bts(MEM16 a,AX b){x86(111,a,b);}
V lock_bts(MEM16 a,DX b){x86(111,a,b);}
V lock_bts(MEM16 a,CX b){x86(111,a,b);}
V lock_bts(MEM16 a,REG16 b){x86(111,a,b);}
V lock_bts(MEM32 a,EAX b){x86(112,a,b);}
V lock_bts(MEM32 a,ECX b){x86(112,a,b);}
V lock_bts(MEM32 a,REG32 b){x86(112,a,b);}
V lock_bts(MEM16 a,int b){x86(113,a,(IMM)b);}
V lock_bts(MEM16 a,char b){x86(113,a,(IMM)b);}
V lock_bts(MEM16 a,short b){x86(113,a,(IMM)b);}
V lock_bts(MEM16 a,REF b){x86(113,a,b);}
V lock_bts(MEM32 a,int b){x86(114,a,(IMM)b);}
V lock_bts(MEM32 a,char b){x86(114,a,(IMM)b);}
V lock_bts(MEM32 a,short b){x86(114,a,(IMM)b);}
V lock_bts(MEM32 a,REF b){x86(114,a,b);}
V call(int a){x86(115,(IMM)a);}
V call(char a){x86(115,(IMM)a);}
V call(short a){x86(115,(IMM)a);}
V call(REF a){x86(115,a);}
V call(AX a){x86(116,a);}
V call(DX a){x86(116,a);}
V call(CX a){x86(116,a);}
V call(REG16 a){x86(116,a);}
V call(MEM16 a){x86(116,a);}
V call(R_M16 a){x86(116,a);}
V call(EAX a){x86(117,a);}
V call(ECX a){x86(117,a);}
V call(REG32 a){x86(117,a);}
V call(MEM32 a){x86(117,a);}
V call(R_M32 a){x86(117,a);}
V cbw(){x86(118);}
V cwd(){x86(119);}
V cdq(){x86(120);}
V cwde(){x86(121);}
V clc(){x86(122);}
V cld(){x86(123);}
V cli(){x86(124);}
V clflush(MEM8 a){x86(125,a);}
V clflush(MEM16 a){x86(125,a);}
V clflush(MEM32 a){x86(125,a);}
V clflush(MEM64 a){x86(125,a);}
V clflush(MEM128 a){x86(125,a);}
V cmc(){x86(126);}
V cmovo(AX a,AX b){x86(127,a,b);}
V cmovo(AX a,DX b){x86(127,a,b);}
V cmovo(AX a,CX b){x86(127,a,b);}
V cmovo(AX a,REG16 b){x86(127,a,b);}
V cmovo(AX a,MEM16 b){x86(127,a,b);}
V cmovo(AX a,R_M16 b){x86(127,a,b);}
V cmovo(DX a,AX b){x86(127,a,b);}
V cmovo(DX a,DX b){x86(127,a,b);}
V cmovo(DX a,CX b){x86(127,a,b);}
V cmovo(DX a,REG16 b){x86(127,a,b);}
V cmovo(DX a,MEM16 b){x86(127,a,b);}
V cmovo(DX a,R_M16 b){x86(127,a,b);}
V cmovo(CX a,AX b){x86(127,a,b);}
V cmovo(CX a,DX b){x86(127,a,b);}
V cmovo(CX a,CX b){x86(127,a,b);}
V cmovo(CX a,REG16 b){x86(127,a,b);}
V cmovo(CX a,MEM16 b){x86(127,a,b);}
V cmovo(CX a,R_M16 b){x86(127,a,b);}
V cmovo(REG16 a,AX b){x86(127,a,b);}
V cmovo(REG16 a,DX b){x86(127,a,b);}
V cmovo(REG16 a,CX b){x86(127,a,b);}
V cmovo(REG16 a,REG16 b){x86(127,a,b);}
V cmovo(REG16 a,MEM16 b){x86(127,a,b);}
V cmovo(REG16 a,R_M16 b){x86(127,a,b);}
V cmovno(AX a,AX b){x86(128,a,b);}
V cmovno(AX a,DX b){x86(128,a,b);}
V cmovno(AX a,CX b){x86(128,a,b);}
V cmovno(AX a,REG16 b){x86(128,a,b);}
V cmovno(AX a,MEM16 b){x86(128,a,b);}
V cmovno(AX a,R_M16 b){x86(128,a,b);}
V cmovno(DX a,AX b){x86(128,a,b);}
V cmovno(DX a,DX b){x86(128,a,b);}
V cmovno(DX a,CX b){x86(128,a,b);}
V cmovno(DX a,REG16 b){x86(128,a,b);}
V cmovno(DX a,MEM16 b){x86(128,a,b);}
V cmovno(DX a,R_M16 b){x86(128,a,b);}
V cmovno(CX a,AX b){x86(128,a,b);}
V cmovno(CX a,DX b){x86(128,a,b);}
V cmovno(CX a,CX b){x86(128,a,b);}
V cmovno(CX a,REG16 b){x86(128,a,b);}
V cmovno(CX a,MEM16 b){x86(128,a,b);}
V cmovno(CX a,R_M16 b){x86(128,a,b);}
V cmovno(REG16 a,AX b){x86(128,a,b);}
V cmovno(REG16 a,DX b){x86(128,a,b);}
V cmovno(REG16 a,CX b){x86(128,a,b);}
V cmovno(REG16 a,REG16 b){x86(128,a,b);}
V cmovno(REG16 a,MEM16 b){x86(128,a,b);}
V cmovno(REG16 a,R_M16 b){x86(128,a,b);}
V cmovb(AX a,AX b){x86(129,a,b);}
V cmovb(AX a,DX b){x86(129,a,b);}
V cmovb(AX a,CX b){x86(129,a,b);}
V cmovb(AX a,REG16 b){x86(129,a,b);}
V cmovb(AX a,MEM16 b){x86(129,a,b);}
V cmovb(AX a,R_M16 b){x86(129,a,b);}
V cmovb(DX a,AX b){x86(129,a,b);}
V cmovb(DX a,DX b){x86(129,a,b);}
V cmovb(DX a,CX b){x86(129,a,b);}
V cmovb(DX a,REG16 b){x86(129,a,b);}
V cmovb(DX a,MEM16 b){x86(129,a,b);}
V cmovb(DX a,R_M16 b){x86(129,a,b);}
V cmovb(CX a,AX b){x86(129,a,b);}
V cmovb(CX a,DX b){x86(129,a,b);}
V cmovb(CX a,CX b){x86(129,a,b);}
V cmovb(CX a,REG16 b){x86(129,a,b);}
V cmovb(CX a,MEM16 b){x86(129,a,b);}
V cmovb(CX a,R_M16 b){x86(129,a,b);}
V cmovb(REG16 a,AX b){x86(129,a,b);}
V cmovb(REG16 a,DX b){x86(129,a,b);}
V cmovb(REG16 a,CX b){x86(129,a,b);}
V cmovb(REG16 a,REG16 b){x86(129,a,b);}
V cmovb(REG16 a,MEM16 b){x86(129,a,b);}
V cmovb(REG16 a,R_M16 b){x86(129,a,b);}
V cmovc(AX a,AX b){x86(130,a,b);}
V cmovc(AX a,DX b){x86(130,a,b);}
V cmovc(AX a,CX b){x86(130,a,b);}
V cmovc(AX a,REG16 b){x86(130,a,b);}
V cmovc(AX a,MEM16 b){x86(130,a,b);}
V cmovc(AX a,R_M16 b){x86(130,a,b);}
V cmovc(DX a,AX b){x86(130,a,b);}
V cmovc(DX a,DX b){x86(130,a,b);}
V cmovc(DX a,CX b){x86(130,a,b);}
V cmovc(DX a,REG16 b){x86(130,a,b);}
V cmovc(DX a,MEM16 b){x86(130,a,b);}
V cmovc(DX a,R_M16 b){x86(130,a,b);}
V cmovc(CX a,AX b){x86(130,a,b);}
V cmovc(CX a,DX b){x86(130,a,b);}
V cmovc(CX a,CX b){x86(130,a,b);}
V cmovc(CX a,REG16 b){x86(130,a,b);}
V cmovc(CX a,MEM16 b){x86(130,a,b);}
V cmovc(CX a,R_M16 b){x86(130,a,b);}
V cmovc(REG16 a,AX b){x86(130,a,b);}
V cmovc(REG16 a,DX b){x86(130,a,b);}
V cmovc(REG16 a,CX b){x86(130,a,b);}
V cmovc(REG16 a,REG16 b){x86(130,a,b);}
V cmovc(REG16 a,MEM16 b){x86(130,a,b);}
V cmovc(REG16 a,R_M16 b){x86(130,a,b);}
V cmovnea(AX a,AX b){x86(131,a,b);}
V cmovnea(AX a,DX b){x86(131,a,b);}
V cmovnea(AX a,CX b){x86(131,a,b);}
V cmovnea(AX a,REG16 b){x86(131,a,b);}
V cmovnea(AX a,MEM16 b){x86(131,a,b);}
V cmovnea(AX a,R_M16 b){x86(131,a,b);}
V cmovnea(DX a,AX b){x86(131,a,b);}
V cmovnea(DX a,DX b){x86(131,a,b);}
V cmovnea(DX a,CX b){x86(131,a,b);}
V cmovnea(DX a,REG16 b){x86(131,a,b);}
V cmovnea(DX a,MEM16 b){x86(131,a,b);}
V cmovnea(DX a,R_M16 b){x86(131,a,b);}
V cmovnea(CX a,AX b){x86(131,a,b);}
V cmovnea(CX a,DX b){x86(131,a,b);}
V cmovnea(CX a,CX b){x86(131,a,b);}
V cmovnea(CX a,REG16 b){x86(131,a,b);}
V cmovnea(CX a,MEM16 b){x86(131,a,b);}
V cmovnea(CX a,R_M16 b){x86(131,a,b);}
V cmovnea(REG16 a,AX b){x86(131,a,b);}
V cmovnea(REG16 a,DX b){x86(131,a,b);}
V cmovnea(REG16 a,CX b){x86(131,a,b);}
V cmovnea(REG16 a,REG16 b){x86(131,a,b);}
V cmovnea(REG16 a,MEM16 b){x86(131,a,b);}
V cmovnea(REG16 a,R_M16 b){x86(131,a,b);}
V cmovae(AX a,AX b){x86(132,a,b);}
V cmovae(AX a,DX b){x86(132,a,b);}
V cmovae(AX a,CX b){x86(132,a,b);}
V cmovae(AX a,REG16 b){x86(132,a,b);}
V cmovae(AX a,MEM16 b){x86(132,a,b);}
V cmovae(AX a,R_M16 b){x86(132,a,b);}
V cmovae(DX a,AX b){x86(132,a,b);}
V cmovae(DX a,DX b){x86(132,a,b);}
V cmovae(DX a,CX b){x86(132,a,b);}
V cmovae(DX a,REG16 b){x86(132,a,b);}
V cmovae(DX a,MEM16 b){x86(132,a,b);}
V cmovae(DX a,R_M16 b){x86(132,a,b);}
V cmovae(CX a,AX b){x86(132,a,b);}
V cmovae(CX a,DX b){x86(132,a,b);}
V cmovae(CX a,CX b){x86(132,a,b);}
V cmovae(CX a,REG16 b){x86(132,a,b);}
V cmovae(CX a,MEM16 b){x86(132,a,b);}
V cmovae(CX a,R_M16 b){x86(132,a,b);}
V cmovae(REG16 a,AX b){x86(132,a,b);}
V cmovae(REG16 a,DX b){x86(132,a,b);}
V cmovae(REG16 a,CX b){x86(132,a,b);}
V cmovae(REG16 a,REG16 b){x86(132,a,b);}
V cmovae(REG16 a,MEM16 b){x86(132,a,b);}
V cmovae(REG16 a,R_M16 b){x86(132,a,b);}
V cmovnb(AX a,AX b){x86(133,a,b);}
V cmovnb(AX a,DX b){x86(133,a,b);}
V cmovnb(AX a,CX b){x86(133,a,b);}
V cmovnb(AX a,REG16 b){x86(133,a,b);}
V cmovnb(AX a,MEM16 b){x86(133,a,b);}
V cmovnb(AX a,R_M16 b){x86(133,a,b);}
V cmovnb(DX a,AX b){x86(133,a,b);}
V cmovnb(DX a,DX b){x86(133,a,b);}
V cmovnb(DX a,CX b){x86(133,a,b);}
V cmovnb(DX a,REG16 b){x86(133,a,b);}
V cmovnb(DX a,MEM16 b){x86(133,a,b);}
V cmovnb(DX a,R_M16 b){x86(133,a,b);}
V cmovnb(CX a,AX b){x86(133,a,b);}
V cmovnb(CX a,DX b){x86(133,a,b);}
V cmovnb(CX a,CX b){x86(133,a,b);}
V cmovnb(CX a,REG16 b){x86(133,a,b);}
V cmovnb(CX a,MEM16 b){x86(133,a,b);}
V cmovnb(CX a,R_M16 b){x86(133,a,b);}
V cmovnb(REG16 a,AX b){x86(133,a,b);}
V cmovnb(REG16 a,DX b){x86(133,a,b);}
V cmovnb(REG16 a,CX b){x86(133,a,b);}
V cmovnb(REG16 a,REG16 b){x86(133,a,b);}
V cmovnb(REG16 a,MEM16 b){x86(133,a,b);}
V cmovnb(REG16 a,R_M16 b){x86(133,a,b);}
V cmovnc(AX a,AX b){x86(134,a,b);}
V cmovnc(AX a,DX b){x86(134,a,b);}
V cmovnc(AX a,CX b){x86(134,a,b);}
V cmovnc(AX a,REG16 b){x86(134,a,b);}
V cmovnc(AX a,MEM16 b){x86(134,a,b);}
V cmovnc(AX a,R_M16 b){x86(134,a,b);}
V cmovnc(DX a,AX b){x86(134,a,b);}
V cmovnc(DX a,DX b){x86(134,a,b);}
V cmovnc(DX a,CX b){x86(134,a,b);}
V cmovnc(DX a,REG16 b){x86(134,a,b);}
V cmovnc(DX a,MEM16 b){x86(134,a,b);}
V cmovnc(DX a,R_M16 b){x86(134,a,b);}
V cmovnc(CX a,AX b){x86(134,a,b);}
V cmovnc(CX a,DX b){x86(134,a,b);}
V cmovnc(CX a,CX b){x86(134,a,b);}
V cmovnc(CX a,REG16 b){x86(134,a,b);}
V cmovnc(CX a,MEM16 b){x86(134,a,b);}
V cmovnc(CX a,R_M16 b){x86(134,a,b);}
V cmovnc(REG16 a,AX b){x86(134,a,b);}
V cmovnc(REG16 a,DX b){x86(134,a,b);}
V cmovnc(REG16 a,CX b){x86(134,a,b);}
V cmovnc(REG16 a,REG16 b){x86(134,a,b);}
V cmovnc(REG16 a,MEM16 b){x86(134,a,b);}
V cmovnc(REG16 a,R_M16 b){x86(134,a,b);}
V cmove(AX a,AX b){x86(135,a,b);}
V cmove(AX a,DX b){x86(135,a,b);}
V cmove(AX a,CX b){x86(135,a,b);}
V cmove(AX a,REG16 b){x86(135,a,b);}
V cmove(AX a,MEM16 b){x86(135,a,b);}
V cmove(AX a,R_M16 b){x86(135,a,b);}
V cmove(DX a,AX b){x86(135,a,b);}
V cmove(DX a,DX b){x86(135,a,b);}
V cmove(DX a,CX b){x86(135,a,b);}
V cmove(DX a,REG16 b){x86(135,a,b);}
V cmove(DX a,MEM16 b){x86(135,a,b);}
V cmove(DX a,R_M16 b){x86(135,a,b);}
V cmove(CX a,AX b){x86(135,a,b);}
V cmove(CX a,DX b){x86(135,a,b);}
V cmove(CX a,CX b){x86(135,a,b);}
V cmove(CX a,REG16 b){x86(135,a,b);}
V cmove(CX a,MEM16 b){x86(135,a,b);}
V cmove(CX a,R_M16 b){x86(135,a,b);}
V cmove(REG16 a,AX b){x86(135,a,b);}
V cmove(REG16 a,DX b){x86(135,a,b);}
V cmove(REG16 a,CX b){x86(135,a,b);}
V cmove(REG16 a,REG16 b){x86(135,a,b);}
V cmove(REG16 a,MEM16 b){x86(135,a,b);}
V cmove(REG16 a,R_M16 b){x86(135,a,b);}
V cmovz(AX a,AX b){x86(136,a,b);}
V cmovz(AX a,DX b){x86(136,a,b);}
V cmovz(AX a,CX b){x86(136,a,b);}
V cmovz(AX a,REG16 b){x86(136,a,b);}
V cmovz(AX a,MEM16 b){x86(136,a,b);}
V cmovz(AX a,R_M16 b){x86(136,a,b);}
V cmovz(DX a,AX b){x86(136,a,b);}
V cmovz(DX a,DX b){x86(136,a,b);}
V cmovz(DX a,CX b){x86(136,a,b);}
V cmovz(DX a,REG16 b){x86(136,a,b);}
V cmovz(DX a,MEM16 b){x86(136,a,b);}
V cmovz(DX a,R_M16 b){x86(136,a,b);}
V cmovz(CX a,AX b){x86(136,a,b);}
V cmovz(CX a,DX b){x86(136,a,b);}
V cmovz(CX a,CX b){x86(136,a,b);}
V cmovz(CX a,REG16 b){x86(136,a,b);}
V cmovz(CX a,MEM16 b){x86(136,a,b);}
V cmovz(CX a,R_M16 b){x86(136,a,b);}
V cmovz(REG16 a,AX b){x86(136,a,b);}
V cmovz(REG16 a,DX b){x86(136,a,b);}
V cmovz(REG16 a,CX b){x86(136,a,b);}
V cmovz(REG16 a,REG16 b){x86(136,a,b);}
V cmovz(REG16 a,MEM16 b){x86(136,a,b);}
V cmovz(REG16 a,R_M16 b){x86(136,a,b);}
V cmovne(AX a,AX b){x86(137,a,b);}
V cmovne(AX a,DX b){x86(137,a,b);}
V cmovne(AX a,CX b){x86(137,a,b);}
V cmovne(AX a,REG16 b){x86(137,a,b);}
V cmovne(AX a,MEM16 b){x86(137,a,b);}
V cmovne(AX a,R_M16 b){x86(137,a,b);}
V cmovne(DX a,AX b){x86(137,a,b);}
V cmovne(DX a,DX b){x86(137,a,b);}
V cmovne(DX a,CX b){x86(137,a,b);}
V cmovne(DX a,REG16 b){x86(137,a,b);}
V cmovne(DX a,MEM16 b){x86(137,a,b);}
V cmovne(DX a,R_M16 b){x86(137,a,b);}
V cmovne(CX a,AX b){x86(137,a,b);}
V cmovne(CX a,DX b){x86(137,a,b);}
V cmovne(CX a,CX b){x86(137,a,b);}
V cmovne(CX a,REG16 b){x86(137,a,b);}
V cmovne(CX a,MEM16 b){x86(137,a,b);}
V cmovne(CX a,R_M16 b){x86(137,a,b);}
V cmovne(REG16 a,AX b){x86(137,a,b);}
V cmovne(REG16 a,DX b){x86(137,a,b);}
V cmovne(REG16 a,CX b){x86(137,a,b);}
V cmovne(REG16 a,REG16 b){x86(137,a,b);}
V cmovne(REG16 a,MEM16 b){x86(137,a,b);}
V cmovne(REG16 a,R_M16 b){x86(137,a,b);}
V cmovnz(AX a,AX b){x86(138,a,b);}
V cmovnz(AX a,DX b){x86(138,a,b);}
V cmovnz(AX a,CX b){x86(138,a,b);}
V cmovnz(AX a,REG16 b){x86(138,a,b);}
V cmovnz(AX a,MEM16 b){x86(138,a,b);}
V cmovnz(AX a,R_M16 b){x86(138,a,b);}
V cmovnz(DX a,AX b){x86(138,a,b);}
V cmovnz(DX a,DX b){x86(138,a,b);}
V cmovnz(DX a,CX b){x86(138,a,b);}
V cmovnz(DX a,REG16 b){x86(138,a,b);}
V cmovnz(DX a,MEM16 b){x86(138,a,b);}
V cmovnz(DX a,R_M16 b){x86(138,a,b);}
V cmovnz(CX a,AX b){x86(138,a,b);}
V cmovnz(CX a,DX b){x86(138,a,b);}
V cmovnz(CX a,CX b){x86(138,a,b);}
V cmovnz(CX a,REG16 b){x86(138,a,b);}
V cmovnz(CX a,MEM16 b){x86(138,a,b);}
V cmovnz(CX a,R_M16 b){x86(138,a,b);}
V cmovnz(REG16 a,AX b){x86(138,a,b);}
V cmovnz(REG16 a,DX b){x86(138,a,b);}
V cmovnz(REG16 a,CX b){x86(138,a,b);}
V cmovnz(REG16 a,REG16 b){x86(138,a,b);}
V cmovnz(REG16 a,MEM16 b){x86(138,a,b);}
V cmovnz(REG16 a,R_M16 b){x86(138,a,b);}
V cmovbe(AX a,AX b){x86(139,a,b);}
V cmovbe(AX a,DX b){x86(139,a,b);}
V cmovbe(AX a,CX b){x86(139,a,b);}
V cmovbe(AX a,REG16 b){x86(139,a,b);}
V cmovbe(AX a,MEM16 b){x86(139,a,b);}
V cmovbe(AX a,R_M16 b){x86(139,a,b);}
V cmovbe(DX a,AX b){x86(139,a,b);}
V cmovbe(DX a,DX b){x86(139,a,b);}
V cmovbe(DX a,CX b){x86(139,a,b);}
V cmovbe(DX a,REG16 b){x86(139,a,b);}
V cmovbe(DX a,MEM16 b){x86(139,a,b);}
V cmovbe(DX a,R_M16 b){x86(139,a,b);}
V cmovbe(CX a,AX b){x86(139,a,b);}
V cmovbe(CX a,DX b){x86(139,a,b);}
V cmovbe(CX a,CX b){x86(139,a,b);}
V cmovbe(CX a,REG16 b){x86(139,a,b);}
V cmovbe(CX a,MEM16 b){x86(139,a,b);}
V cmovbe(CX a,R_M16 b){x86(139,a,b);}
V cmovbe(REG16 a,AX b){x86(139,a,b);}
V cmovbe(REG16 a,DX b){x86(139,a,b);}
V cmovbe(REG16 a,CX b){x86(139,a,b);}
V cmovbe(REG16 a,REG16 b){x86(139,a,b);}
V cmovbe(REG16 a,MEM16 b){x86(139,a,b);}
V cmovbe(REG16 a,R_M16 b){x86(139,a,b);}
V cmovna(AX a,AX b){x86(140,a,b);}
V cmovna(AX a,DX b){x86(140,a,b);}
V cmovna(AX a,CX b){x86(140,a,b);}
V cmovna(AX a,REG16 b){x86(140,a,b);}
V cmovna(AX a,MEM16 b){x86(140,a,b);}
V cmovna(AX a,R_M16 b){x86(140,a,b);}
V cmovna(DX a,AX b){x86(140,a,b);}
V cmovna(DX a,DX b){x86(140,a,b);}
V cmovna(DX a,CX b){x86(140,a,b);}
V cmovna(DX a,REG16 b){x86(140,a,b);}
V cmovna(DX a,MEM16 b){x86(140,a,b);}
V cmovna(DX a,R_M16 b){x86(140,a,b);}
V cmovna(CX a,AX b){x86(140,a,b);}
V cmovna(CX a,DX b){x86(140,a,b);}
V cmovna(CX a,CX b){x86(140,a,b);}
V cmovna(CX a,REG16 b){x86(140,a,b);}
V cmovna(CX a,MEM16 b){x86(140,a,b);}
V cmovna(CX a,R_M16 b){x86(140,a,b);}
V cmovna(REG16 a,AX b){x86(140,a,b);}
V cmovna(REG16 a,DX b){x86(140,a,b);}
V cmovna(REG16 a,CX b){x86(140,a,b);}
V cmovna(REG16 a,REG16 b){x86(140,a,b);}
V cmovna(REG16 a,MEM16 b){x86(140,a,b);}
V cmovna(REG16 a,R_M16 b){x86(140,a,b);}
V cmova(AX a,AX b){x86(141,a,b);}
V cmova(AX a,DX b){x86(141,a,b);}
V cmova(AX a,CX b){x86(141,a,b);}
V cmova(AX a,REG16 b){x86(141,a,b);}
V cmova(AX a,MEM16 b){x86(141,a,b);}
V cmova(AX a,R_M16 b){x86(141,a,b);}
V cmova(DX a,AX b){x86(141,a,b);}
V cmova(DX a,DX b){x86(141,a,b);}
V cmova(DX a,CX b){x86(141,a,b);}
V cmova(DX a,REG16 b){x86(141,a,b);}
V cmova(DX a,MEM16 b){x86(141,a,b);}
V cmova(DX a,R_M16 b){x86(141,a,b);}
V cmova(CX a,AX b){x86(141,a,b);}
V cmova(CX a,DX b){x86(141,a,b);}
V cmova(CX a,CX b){x86(141,a,b);}
V cmova(CX a,REG16 b){x86(141,a,b);}
V cmova(CX a,MEM16 b){x86(141,a,b);}
V cmova(CX a,R_M16 b){x86(141,a,b);}
V cmova(REG16 a,AX b){x86(141,a,b);}
V cmova(REG16 a,DX b){x86(141,a,b);}
V cmova(REG16 a,CX b){x86(141,a,b);}
V cmova(REG16 a,REG16 b){x86(141,a,b);}
V cmova(REG16 a,MEM16 b){x86(141,a,b);}
V cmova(REG16 a,R_M16 b){x86(141,a,b);}
V cmovnbe(AX a,AX b){x86(142,a,b);}
V cmovnbe(AX a,DX b){x86(142,a,b);}
V cmovnbe(AX a,CX b){x86(142,a,b);}
V cmovnbe(AX a,REG16 b){x86(142,a,b);}
V cmovnbe(AX a,MEM16 b){x86(142,a,b);}
V cmovnbe(AX a,R_M16 b){x86(142,a,b);}
V cmovnbe(DX a,AX b){x86(142,a,b);}
V cmovnbe(DX a,DX b){x86(142,a,b);}
V cmovnbe(DX a,CX b){x86(142,a,b);}
V cmovnbe(DX a,REG16 b){x86(142,a,b);}
V cmovnbe(DX a,MEM16 b){x86(142,a,b);}
V cmovnbe(DX a,R_M16 b){x86(142,a,b);}
V cmovnbe(CX a,AX b){x86(142,a,b);}
V cmovnbe(CX a,DX b){x86(142,a,b);}
V cmovnbe(CX a,CX b){x86(142,a,b);}
V cmovnbe(CX a,REG16 b){x86(142,a,b);}
V cmovnbe(CX a,MEM16 b){x86(142,a,b);}
V cmovnbe(CX a,R_M16 b){x86(142,a,b);}
V cmovnbe(REG16 a,AX b){x86(142,a,b);}
V cmovnbe(REG16 a,DX b){x86(142,a,b);}
V cmovnbe(REG16 a,CX b){x86(142,a,b);}
V cmovnbe(REG16 a,REG16 b){x86(142,a,b);}
V cmovnbe(REG16 a,MEM16 b){x86(142,a,b);}
V cmovnbe(REG16 a,R_M16 b){x86(142,a,b);}
V cmovs(AX a,AX b){x86(143,a,b);}
V cmovs(AX a,DX b){x86(143,a,b);}
V cmovs(AX a,CX b){x86(143,a,b);}
V cmovs(AX a,REG16 b){x86(143,a,b);}
V cmovs(AX a,MEM16 b){x86(143,a,b);}
V cmovs(AX a,R_M16 b){x86(143,a,b);}
V cmovs(DX a,AX b){x86(143,a,b);}
V cmovs(DX a,DX b){x86(143,a,b);}
V cmovs(DX a,CX b){x86(143,a,b);}
V cmovs(DX a,REG16 b){x86(143,a,b);}
V cmovs(DX a,MEM16 b){x86(143,a,b);}
V cmovs(DX a,R_M16 b){x86(143,a,b);}
V cmovs(CX a,AX b){x86(143,a,b);}
V cmovs(CX a,DX b){x86(143,a,b);}
V cmovs(CX a,CX b){x86(143,a,b);}
V cmovs(CX a,REG16 b){x86(143,a,b);}
V cmovs(CX a,MEM16 b){x86(143,a,b);}
V cmovs(CX a,R_M16 b){x86(143,a,b);}
V cmovs(REG16 a,AX b){x86(143,a,b);}
V cmovs(REG16 a,DX b){x86(143,a,b);}
V cmovs(REG16 a,CX b){x86(143,a,b);}
V cmovs(REG16 a,REG16 b){x86(143,a,b);}
V cmovs(REG16 a,MEM16 b){x86(143,a,b);}
V cmovs(REG16 a,R_M16 b){x86(143,a,b);}
V cmovns(AX a,AX b){x86(144,a,b);}
V cmovns(AX a,DX b){x86(144,a,b);}
V cmovns(AX a,CX b){x86(144,a,b);}
V cmovns(AX a,REG16 b){x86(144,a,b);}
V cmovns(AX a,MEM16 b){x86(144,a,b);}
V cmovns(AX a,R_M16 b){x86(144,a,b);}
V cmovns(DX a,AX b){x86(144,a,b);}
V cmovns(DX a,DX b){x86(144,a,b);}
V cmovns(DX a,CX b){x86(144,a,b);}
V cmovns(DX a,REG16 b){x86(144,a,b);}
V cmovns(DX a,MEM16 b){x86(144,a,b);}
V cmovns(DX a,R_M16 b){x86(144,a,b);}
V cmovns(CX a,AX b){x86(144,a,b);}
V cmovns(CX a,DX b){x86(144,a,b);}
V cmovns(CX a,CX b){x86(144,a,b);}
V cmovns(CX a,REG16 b){x86(144,a,b);}
V cmovns(CX a,MEM16 b){x86(144,a,b);}
V cmovns(CX a,R_M16 b){x86(144,a,b);}
V cmovns(REG16 a,AX b){x86(144,a,b);}
V cmovns(REG16 a,DX b){x86(144,a,b);}
V cmovns(REG16 a,CX b){x86(144,a,b);}
V cmovns(REG16 a,REG16 b){x86(144,a,b);}
V cmovns(REG16 a,MEM16 b){x86(144,a,b);}
V cmovns(REG16 a,R_M16 b){x86(144,a,b);}
V cmovp(AX a,AX b){x86(145,a,b);}
V cmovp(AX a,DX b){x86(145,a,b);}
V cmovp(AX a,CX b){x86(145,a,b);}
V cmovp(AX a,REG16 b){x86(145,a,b);}
V cmovp(AX a,MEM16 b){x86(145,a,b);}
V cmovp(AX a,R_M16 b){x86(145,a,b);}
V cmovp(DX a,AX b){x86(145,a,b);}
V cmovp(DX a,DX b){x86(145,a,b);}
V cmovp(DX a,CX b){x86(145,a,b);}
V cmovp(DX a,REG16 b){x86(145,a,b);}
V cmovp(DX a,MEM16 b){x86(145,a,b);}
V cmovp(DX a,R_M16 b){x86(145,a,b);}
V cmovp(CX a,AX b){x86(145,a,b);}
V cmovp(CX a,DX b){x86(145,a,b);}
V cmovp(CX a,CX b){x86(145,a,b);}
V cmovp(CX a,REG16 b){x86(145,a,b);}
V cmovp(CX a,MEM16 b){x86(145,a,b);}
V cmovp(CX a,R_M16 b){x86(145,a,b);}
V cmovp(REG16 a,AX b){x86(145,a,b);}
V cmovp(REG16 a,DX b){x86(145,a,b);}
V cmovp(REG16 a,CX b){x86(145,a,b);}
V cmovp(REG16 a,REG16 b){x86(145,a,b);}
V cmovp(REG16 a,MEM16 b){x86(145,a,b);}
V cmovp(REG16 a,R_M16 b){x86(145,a,b);}
V cmovpe(AX a,AX b){x86(146,a,b);}
V cmovpe(AX a,DX b){x86(146,a,b);}
V cmovpe(AX a,CX b){x86(146,a,b);}
V cmovpe(AX a,REG16 b){x86(146,a,b);}
V cmovpe(AX a,MEM16 b){x86(146,a,b);}
V cmovpe(AX a,R_M16 b){x86(146,a,b);}
V cmovpe(DX a,AX b){x86(146,a,b);}
V cmovpe(DX a,DX b){x86(146,a,b);}
V cmovpe(DX a,CX b){x86(146,a,b);}
V cmovpe(DX a,REG16 b){x86(146,a,b);}
V cmovpe(DX a,MEM16 b){x86(146,a,b);}
V cmovpe(DX a,R_M16 b){x86(146,a,b);}
V cmovpe(CX a,AX b){x86(146,a,b);}
V cmovpe(CX a,DX b){x86(146,a,b);}
V cmovpe(CX a,CX b){x86(146,a,b);}
V cmovpe(CX a,REG16 b){x86(146,a,b);}
V cmovpe(CX a,MEM16 b){x86(146,a,b);}
V cmovpe(CX a,R_M16 b){x86(146,a,b);}
V cmovpe(REG16 a,AX b){x86(146,a,b);}
V cmovpe(REG16 a,DX b){x86(146,a,b);}
V cmovpe(REG16 a,CX b){x86(146,a,b);}
V cmovpe(REG16 a,REG16 b){x86(146,a,b);}
V cmovpe(REG16 a,MEM16 b){x86(146,a,b);}
V cmovpe(REG16 a,R_M16 b){x86(146,a,b);}
V cmovnp(AX a,AX b){x86(147,a,b);}
V cmovnp(AX a,DX b){x86(147,a,b);}
V cmovnp(AX a,CX b){x86(147,a,b);}
V cmovnp(AX a,REG16 b){x86(147,a,b);}
V cmovnp(AX a,MEM16 b){x86(147,a,b);}
V cmovnp(AX a,R_M16 b){x86(147,a,b);}
V cmovnp(DX a,AX b){x86(147,a,b);}
V cmovnp(DX a,DX b){x86(147,a,b);}
V cmovnp(DX a,CX b){x86(147,a,b);}
V cmovnp(DX a,REG16 b){x86(147,a,b);}
V cmovnp(DX a,MEM16 b){x86(147,a,b);}
V cmovnp(DX a,R_M16 b){x86(147,a,b);}
V cmovnp(CX a,AX b){x86(147,a,b);}
V cmovnp(CX a,DX b){x86(147,a,b);}
V cmovnp(CX a,CX b){x86(147,a,b);}
V cmovnp(CX a,REG16 b){x86(147,a,b);}
V cmovnp(CX a,MEM16 b){x86(147,a,b);}
V cmovnp(CX a,R_M16 b){x86(147,a,b);}
V cmovnp(REG16 a,AX b){x86(147,a,b);}
V cmovnp(REG16 a,DX b){x86(147,a,b);}
V cmovnp(REG16 a,CX b){x86(147,a,b);}
V cmovnp(REG16 a,REG16 b){x86(147,a,b);}
V cmovnp(REG16 a,MEM16 b){x86(147,a,b);}
V cmovnp(REG16 a,R_M16 b){x86(147,a,b);}
V cmovpo(AX a,AX b){x86(148,a,b);}
V cmovpo(AX a,DX b){x86(148,a,b);}
V cmovpo(AX a,CX b){x86(148,a,b);}
V cmovpo(AX a,REG16 b){x86(148,a,b);}
V cmovpo(AX a,MEM16 b){x86(148,a,b);}
V cmovpo(AX a,R_M16 b){x86(148,a,b);}
V cmovpo(DX a,AX b){x86(148,a,b);}
V cmovpo(DX a,DX b){x86(148,a,b);}
V cmovpo(DX a,CX b){x86(148,a,b);}
V cmovpo(DX a,REG16 b){x86(148,a,b);}
V cmovpo(DX a,MEM16 b){x86(148,a,b);}
V cmovpo(DX a,R_M16 b){x86(148,a,b);}
V cmovpo(CX a,AX b){x86(148,a,b);}
V cmovpo(CX a,DX b){x86(148,a,b);}
V cmovpo(CX a,CX b){x86(148,a,b);}
V cmovpo(CX a,REG16 b){x86(148,a,b);}
V cmovpo(CX a,MEM16 b){x86(148,a,b);}
V cmovpo(CX a,R_M16 b){x86(148,a,b);}
V cmovpo(REG16 a,AX b){x86(148,a,b);}
V cmovpo(REG16 a,DX b){x86(148,a,b);}
V cmovpo(REG16 a,CX b){x86(148,a,b);}
V cmovpo(REG16 a,REG16 b){x86(148,a,b);}
V cmovpo(REG16 a,MEM16 b){x86(148,a,b);}
V cmovpo(REG16 a,R_M16 b){x86(148,a,b);}
V cmovl(AX a,AX b){x86(149,a,b);}
V cmovl(AX a,DX b){x86(149,a,b);}
V cmovl(AX a,CX b){x86(149,a,b);}
V cmovl(AX a,REG16 b){x86(149,a,b);}
V cmovl(AX a,MEM16 b){x86(149,a,b);}
V cmovl(AX a,R_M16 b){x86(149,a,b);}
V cmovl(DX a,AX b){x86(149,a,b);}
V cmovl(DX a,DX b){x86(149,a,b);}
V cmovl(DX a,CX b){x86(149,a,b);}
V cmovl(DX a,REG16 b){x86(149,a,b);}
V cmovl(DX a,MEM16 b){x86(149,a,b);}
V cmovl(DX a,R_M16 b){x86(149,a,b);}
V cmovl(CX a,AX b){x86(149,a,b);}
V cmovl(CX a,DX b){x86(149,a,b);}
V cmovl(CX a,CX b){x86(149,a,b);}
V cmovl(CX a,REG16 b){x86(149,a,b);}
V cmovl(CX a,MEM16 b){x86(149,a,b);}
V cmovl(CX a,R_M16 b){x86(149,a,b);}
V cmovl(REG16 a,AX b){x86(149,a,b);}
V cmovl(REG16 a,DX b){x86(149,a,b);}
V cmovl(REG16 a,CX b){x86(149,a,b);}
V cmovl(REG16 a,REG16 b){x86(149,a,b);}
V cmovl(REG16 a,MEM16 b){x86(149,a,b);}
V cmovl(REG16 a,R_M16 b){x86(149,a,b);}
V cmovnge(AX a,AX b){x86(150,a,b);}
V cmovnge(AX a,DX b){x86(150,a,b);}
V cmovnge(AX a,CX b){x86(150,a,b);}
V cmovnge(AX a,REG16 b){x86(150,a,b);}
V cmovnge(AX a,MEM16 b){x86(150,a,b);}
V cmovnge(AX a,R_M16 b){x86(150,a,b);}
V cmovnge(DX a,AX b){x86(150,a,b);}
V cmovnge(DX a,DX b){x86(150,a,b);}
V cmovnge(DX a,CX b){x86(150,a,b);}
V cmovnge(DX a,REG16 b){x86(150,a,b);}
V cmovnge(DX a,MEM16 b){x86(150,a,b);}
V cmovnge(DX a,R_M16 b){x86(150,a,b);}
V cmovnge(CX a,AX b){x86(150,a,b);}
V cmovnge(CX a,DX b){x86(150,a,b);}
V cmovnge(CX a,CX b){x86(150,a,b);}
V cmovnge(CX a,REG16 b){x86(150,a,b);}
V cmovnge(CX a,MEM16 b){x86(150,a,b);}
V cmovnge(CX a,R_M16 b){x86(150,a,b);}
V cmovnge(REG16 a,AX b){x86(150,a,b);}
V cmovnge(REG16 a,DX b){x86(150,a,b);}
V cmovnge(REG16 a,CX b){x86(150,a,b);}
V cmovnge(REG16 a,REG16 b){x86(150,a,b);}
V cmovnge(REG16 a,MEM16 b){x86(150,a,b);}
V cmovnge(REG16 a,R_M16 b){x86(150,a,b);}
V cmovge(AX a,AX b){x86(151,a,b);}
V cmovge(AX a,DX b){x86(151,a,b);}
V cmovge(AX a,CX b){x86(151,a,b);}
V cmovge(AX a,REG16 b){x86(151,a,b);}
V cmovge(AX a,MEM16 b){x86(151,a,b);}
V cmovge(AX a,R_M16 b){x86(151,a,b);}
V cmovge(DX a,AX b){x86(151,a,b);}
V cmovge(DX a,DX b){x86(151,a,b);}
V cmovge(DX a,CX b){x86(151,a,b);}
V cmovge(DX a,REG16 b){x86(151,a,b);}
V cmovge(DX a,MEM16 b){x86(151,a,b);}
V cmovge(DX a,R_M16 b){x86(151,a,b);}
V cmovge(CX a,AX b){x86(151,a,b);}
V cmovge(CX a,DX b){x86(151,a,b);}
V cmovge(CX a,CX b){x86(151,a,b);}
V cmovge(CX a,REG16 b){x86(151,a,b);}
V cmovge(CX a,MEM16 b){x86(151,a,b);}
V cmovge(CX a,R_M16 b){x86(151,a,b);}
V cmovge(REG16 a,AX b){x86(151,a,b);}
V cmovge(REG16 a,DX b){x86(151,a,b);}
V cmovge(REG16 a,CX b){x86(151,a,b);}
V cmovge(REG16 a,REG16 b){x86(151,a,b);}
V cmovge(REG16 a,MEM16 b){x86(151,a,b);}
V cmovge(REG16 a,R_M16 b){x86(151,a,b);}
V cmovnl(AX a,AX b){x86(152,a,b);}
V cmovnl(AX a,DX b){x86(152,a,b);}
V cmovnl(AX a,CX b){x86(152,a,b);}
V cmovnl(AX a,REG16 b){x86(152,a,b);}
V cmovnl(AX a,MEM16 b){x86(152,a,b);}
V cmovnl(AX a,R_M16 b){x86(152,a,b);}
V cmovnl(DX a,AX b){x86(152,a,b);}
V cmovnl(DX a,DX b){x86(152,a,b);}
V cmovnl(DX a,CX b){x86(152,a,b);}
V cmovnl(DX a,REG16 b){x86(152,a,b);}
V cmovnl(DX a,MEM16 b){x86(152,a,b);}
V cmovnl(DX a,R_M16 b){x86(152,a,b);}
V cmovnl(CX a,AX b){x86(152,a,b);}
V cmovnl(CX a,DX b){x86(152,a,b);}
V cmovnl(CX a,CX b){x86(152,a,b);}
V cmovnl(CX a,REG16 b){x86(152,a,b);}
V cmovnl(CX a,MEM16 b){x86(152,a,b);}
V cmovnl(CX a,R_M16 b){x86(152,a,b);}
V cmovnl(REG16 a,AX b){x86(152,a,b);}
V cmovnl(REG16 a,DX b){x86(152,a,b);}
V cmovnl(REG16 a,CX b){x86(152,a,b);}
V cmovnl(REG16 a,REG16 b){x86(152,a,b);}
V cmovnl(REG16 a,MEM16 b){x86(152,a,b);}
V cmovnl(REG16 a,R_M16 b){x86(152,a,b);}
V cmovle(AX a,AX b){x86(153,a,b);}
V cmovle(AX a,DX b){x86(153,a,b);}
V cmovle(AX a,CX b){x86(153,a,b);}
V cmovle(AX a,REG16 b){x86(153,a,b);}
V cmovle(AX a,MEM16 b){x86(153,a,b);}
V cmovle(AX a,R_M16 b){x86(153,a,b);}
V cmovle(DX a,AX b){x86(153,a,b);}
V cmovle(DX a,DX b){x86(153,a,b);}
V cmovle(DX a,CX b){x86(153,a,b);}
V cmovle(DX a,REG16 b){x86(153,a,b);}
V cmovle(DX a,MEM16 b){x86(153,a,b);}
V cmovle(DX a,R_M16 b){x86(153,a,b);}
V cmovle(CX a,AX b){x86(153,a,b);}
V cmovle(CX a,DX b){x86(153,a,b);}
V cmovle(CX a,CX b){x86(153,a,b);}
V cmovle(CX a,REG16 b){x86(153,a,b);}
V cmovle(CX a,MEM16 b){x86(153,a,b);}
V cmovle(CX a,R_M16 b){x86(153,a,b);}
V cmovle(REG16 a,AX b){x86(153,a,b);}
V cmovle(REG16 a,DX b){x86(153,a,b);}
V cmovle(REG16 a,CX b){x86(153,a,b);}
V cmovle(REG16 a,REG16 b){x86(153,a,b);}
V cmovle(REG16 a,MEM16 b){x86(153,a,b);}
V cmovle(REG16 a,R_M16 b){x86(153,a,b);}
V cmovng(AX a,AX b){x86(154,a,b);}
V cmovng(AX a,DX b){x86(154,a,b);}
V cmovng(AX a,CX b){x86(154,a,b);}
V cmovng(AX a,REG16 b){x86(154,a,b);}
V cmovng(AX a,MEM16 b){x86(154,a,b);}
V cmovng(AX a,R_M16 b){x86(154,a,b);}
V cmovng(DX a,AX b){x86(154,a,b);}
V cmovng(DX a,DX b){x86(154,a,b);}
V cmovng(DX a,CX b){x86(154,a,b);}
V cmovng(DX a,REG16 b){x86(154,a,b);}
V cmovng(DX a,MEM16 b){x86(154,a,b);}
V cmovng(DX a,R_M16 b){x86(154,a,b);}
V cmovng(CX a,AX b){x86(154,a,b);}
V cmovng(CX a,DX b){x86(154,a,b);}
V cmovng(CX a,CX b){x86(154,a,b);}
V cmovng(CX a,REG16 b){x86(154,a,b);}
V cmovng(CX a,MEM16 b){x86(154,a,b);}
V cmovng(CX a,R_M16 b){x86(154,a,b);}
V cmovng(REG16 a,AX b){x86(154,a,b);}
V cmovng(REG16 a,DX b){x86(154,a,b);}
V cmovng(REG16 a,CX b){x86(154,a,b);}
V cmovng(REG16 a,REG16 b){x86(154,a,b);}
V cmovng(REG16 a,MEM16 b){x86(154,a,b);}
V cmovng(REG16 a,R_M16 b){x86(154,a,b);}
V cmovg(AX a,AX b){x86(155,a,b);}
V cmovg(AX a,DX b){x86(155,a,b);}
V cmovg(AX a,CX b){x86(155,a,b);}
V cmovg(AX a,REG16 b){x86(155,a,b);}
V cmovg(AX a,MEM16 b){x86(155,a,b);}
V cmovg(AX a,R_M16 b){x86(155,a,b);}
V cmovg(DX a,AX b){x86(155,a,b);}
V cmovg(DX a,DX b){x86(155,a,b);}
V cmovg(DX a,CX b){x86(155,a,b);}
V cmovg(DX a,REG16 b){x86(155,a,b);}
V cmovg(DX a,MEM16 b){x86(155,a,b);}
V cmovg(DX a,R_M16 b){x86(155,a,b);}
V cmovg(CX a,AX b){x86(155,a,b);}
V cmovg(CX a,DX b){x86(155,a,b);}
V cmovg(CX a,CX b){x86(155,a,b);}
V cmovg(CX a,REG16 b){x86(155,a,b);}
V cmovg(CX a,MEM16 b){x86(155,a,b);}
V cmovg(CX a,R_M16 b){x86(155,a,b);}
V cmovg(REG16 a,AX b){x86(155,a,b);}
V cmovg(REG16 a,DX b){x86(155,a,b);}
V cmovg(REG16 a,CX b){x86(155,a,b);}
V cmovg(REG16 a,REG16 b){x86(155,a,b);}
V cmovg(REG16 a,MEM16 b){x86(155,a,b);}
V cmovg(REG16 a,R_M16 b){x86(155,a,b);}
V cmovnle(AX a,AX b){x86(156,a,b);}
V cmovnle(AX a,DX b){x86(156,a,b);}
V cmovnle(AX a,CX b){x86(156,a,b);}
V cmovnle(AX a,REG16 b){x86(156,a,b);}
V cmovnle(AX a,MEM16 b){x86(156,a,b);}
V cmovnle(AX a,R_M16 b){x86(156,a,b);}
V cmovnle(DX a,AX b){x86(156,a,b);}
V cmovnle(DX a,DX b){x86(156,a,b);}
V cmovnle(DX a,CX b){x86(156,a,b);}
V cmovnle(DX a,REG16 b){x86(156,a,b);}
V cmovnle(DX a,MEM16 b){x86(156,a,b);}
V cmovnle(DX a,R_M16 b){x86(156,a,b);}
V cmovnle(CX a,AX b){x86(156,a,b);}
V cmovnle(CX a,DX b){x86(156,a,b);}
V cmovnle(CX a,CX b){x86(156,a,b);}
V cmovnle(CX a,REG16 b){x86(156,a,b);}
V cmovnle(CX a,MEM16 b){x86(156,a,b);}
V cmovnle(CX a,R_M16 b){x86(156,a,b);}
V cmovnle(REG16 a,AX b){x86(156,a,b);}
V cmovnle(REG16 a,DX b){x86(156,a,b);}
V cmovnle(REG16 a,CX b){x86(156,a,b);}
V cmovnle(REG16 a,REG16 b){x86(156,a,b);}
V cmovnle(REG16 a,MEM16 b){x86(156,a,b);}
V cmovnle(REG16 a,R_M16 b){x86(156,a,b);}
V cmovo(EAX a,EAX b){x86(157,a,b);}
V cmovo(EAX a,ECX b){x86(157,a,b);}
V cmovo(EAX a,REG32 b){x86(157,a,b);}
V cmovo(EAX a,MEM32 b){x86(157,a,b);}
V cmovo(EAX a,R_M32 b){x86(157,a,b);}
V cmovo(ECX a,EAX b){x86(157,a,b);}
V cmovo(ECX a,ECX b){x86(157,a,b);}
V cmovo(ECX a,REG32 b){x86(157,a,b);}
V cmovo(ECX a,MEM32 b){x86(157,a,b);}
V cmovo(ECX a,R_M32 b){x86(157,a,b);}
V cmovo(REG32 a,EAX b){x86(157,a,b);}
V cmovo(REG32 a,ECX b){x86(157,a,b);}
V cmovo(REG32 a,REG32 b){x86(157,a,b);}
V cmovo(REG32 a,MEM32 b){x86(157,a,b);}
V cmovo(REG32 a,R_M32 b){x86(157,a,b);}
V cmovno(EAX a,EAX b){x86(158,a,b);}
V cmovno(EAX a,ECX b){x86(158,a,b);}
V cmovno(EAX a,REG32 b){x86(158,a,b);}
V cmovno(EAX a,MEM32 b){x86(158,a,b);}
V cmovno(EAX a,R_M32 b){x86(158,a,b);}
V cmovno(ECX a,EAX b){x86(158,a,b);}
V cmovno(ECX a,ECX b){x86(158,a,b);}
V cmovno(ECX a,REG32 b){x86(158,a,b);}
V cmovno(ECX a,MEM32 b){x86(158,a,b);}
V cmovno(ECX a,R_M32 b){x86(158,a,b);}
V cmovno(REG32 a,EAX b){x86(158,a,b);}
V cmovno(REG32 a,ECX b){x86(158,a,b);}
V cmovno(REG32 a,REG32 b){x86(158,a,b);}
V cmovno(REG32 a,MEM32 b){x86(158,a,b);}
V cmovno(REG32 a,R_M32 b){x86(158,a,b);}
V cmovb(EAX a,EAX b){x86(159,a,b);}
V cmovb(EAX a,ECX b){x86(159,a,b);}
V cmovb(EAX a,REG32 b){x86(159,a,b);}
V cmovb(EAX a,MEM32 b){x86(159,a,b);}
V cmovb(EAX a,R_M32 b){x86(159,a,b);}
V cmovb(ECX a,EAX b){x86(159,a,b);}
V cmovb(ECX a,ECX b){x86(159,a,b);}
V cmovb(ECX a,REG32 b){x86(159,a,b);}
V cmovb(ECX a,MEM32 b){x86(159,a,b);}
V cmovb(ECX a,R_M32 b){x86(159,a,b);}
V cmovb(REG32 a,EAX b){x86(159,a,b);}
V cmovb(REG32 a,ECX b){x86(159,a,b);}
V cmovb(REG32 a,REG32 b){x86(159,a,b);}
V cmovb(REG32 a,MEM32 b){x86(159,a,b);}
V cmovb(REG32 a,R_M32 b){x86(159,a,b);}
V cmovc(EAX a,EAX b){x86(160,a,b);}
V cmovc(EAX a,ECX b){x86(160,a,b);}
V cmovc(EAX a,REG32 b){x86(160,a,b);}
V cmovc(EAX a,MEM32 b){x86(160,a,b);}
V cmovc(EAX a,R_M32 b){x86(160,a,b);}
V cmovc(ECX a,EAX b){x86(160,a,b);}
V cmovc(ECX a,ECX b){x86(160,a,b);}
V cmovc(ECX a,REG32 b){x86(160,a,b);}
V cmovc(ECX a,MEM32 b){x86(160,a,b);}
V cmovc(ECX a,R_M32 b){x86(160,a,b);}
V cmovc(REG32 a,EAX b){x86(160,a,b);}
V cmovc(REG32 a,ECX b){x86(160,a,b);}
V cmovc(REG32 a,REG32 b){x86(160,a,b);}
V cmovc(REG32 a,MEM32 b){x86(160,a,b);}
V cmovc(REG32 a,R_M32 b){x86(160,a,b);}
V cmovnea(EAX a,EAX b){x86(161,a,b);}
V cmovnea(EAX a,ECX b){x86(161,a,b);}
V cmovnea(EAX a,REG32 b){x86(161,a,b);}
V cmovnea(EAX a,MEM32 b){x86(161,a,b);}
V cmovnea(EAX a,R_M32 b){x86(161,a,b);}
V cmovnea(ECX a,EAX b){x86(161,a,b);}
V cmovnea(ECX a,ECX b){x86(161,a,b);}
V cmovnea(ECX a,REG32 b){x86(161,a,b);}
V cmovnea(ECX a,MEM32 b){x86(161,a,b);}
V cmovnea(ECX a,R_M32 b){x86(161,a,b);}
V cmovnea(REG32 a,EAX b){x86(161,a,b);}
V cmovnea(REG32 a,ECX b){x86(161,a,b);}
V cmovnea(REG32 a,REG32 b){x86(161,a,b);}
V cmovnea(REG32 a,MEM32 b){x86(161,a,b);}
V cmovnea(REG32 a,R_M32 b){x86(161,a,b);}
V cmovae(EAX a,EAX b){x86(162,a,b);}
V cmovae(EAX a,ECX b){x86(162,a,b);}
V cmovae(EAX a,REG32 b){x86(162,a,b);}
V cmovae(EAX a,MEM32 b){x86(162,a,b);}
V cmovae(EAX a,R_M32 b){x86(162,a,b);}
V cmovae(ECX a,EAX b){x86(162,a,b);}
V cmovae(ECX a,ECX b){x86(162,a,b);}
V cmovae(ECX a,REG32 b){x86(162,a,b);}
V cmovae(ECX a,MEM32 b){x86(162,a,b);}
V cmovae(ECX a,R_M32 b){x86(162,a,b);}
V cmovae(REG32 a,EAX b){x86(162,a,b);}
V cmovae(REG32 a,ECX b){x86(162,a,b);}
V cmovae(REG32 a,REG32 b){x86(162,a,b);}
V cmovae(REG32 a,MEM32 b){x86(162,a,b);}
V cmovae(REG32 a,R_M32 b){x86(162,a,b);}
V cmovnb(EAX a,EAX b){x86(163,a,b);}
V cmovnb(EAX a,ECX b){x86(163,a,b);}
V cmovnb(EAX a,REG32 b){x86(163,a,b);}
V cmovnb(EAX a,MEM32 b){x86(163,a,b);}
V cmovnb(EAX a,R_M32 b){x86(163,a,b);}
V cmovnb(ECX a,EAX b){x86(163,a,b);}
V cmovnb(ECX a,ECX b){x86(163,a,b);}
V cmovnb(ECX a,REG32 b){x86(163,a,b);}
V cmovnb(ECX a,MEM32 b){x86(163,a,b);}
V cmovnb(ECX a,R_M32 b){x86(163,a,b);}
V cmovnb(REG32 a,EAX b){x86(163,a,b);}
V cmovnb(REG32 a,ECX b){x86(163,a,b);}
V cmovnb(REG32 a,REG32 b){x86(163,a,b);}
V cmovnb(REG32 a,MEM32 b){x86(163,a,b);}
V cmovnb(REG32 a,R_M32 b){x86(163,a,b);}
V cmovnc(EAX a,EAX b){x86(164,a,b);}
V cmovnc(EAX a,ECX b){x86(164,a,b);}
V cmovnc(EAX a,REG32 b){x86(164,a,b);}
V cmovnc(EAX a,MEM32 b){x86(164,a,b);}
V cmovnc(EAX a,R_M32 b){x86(164,a,b);}
V cmovnc(ECX a,EAX b){x86(164,a,b);}
V cmovnc(ECX a,ECX b){x86(164,a,b);}
V cmovnc(ECX a,REG32 b){x86(164,a,b);}
V cmovnc(ECX a,MEM32 b){x86(164,a,b);}
V cmovnc(ECX a,R_M32 b){x86(164,a,b);}
V cmovnc(REG32 a,EAX b){x86(164,a,b);}
V cmovnc(REG32 a,ECX b){x86(164,a,b);}
V cmovnc(REG32 a,REG32 b){x86(164,a,b);}
V cmovnc(REG32 a,MEM32 b){x86(164,a,b);}
V cmovnc(REG32 a,R_M32 b){x86(164,a,b);}
V cmove(EAX a,EAX b){x86(165,a,b);}
V cmove(EAX a,ECX b){x86(165,a,b);}
V cmove(EAX a,REG32 b){x86(165,a,b);}
V cmove(EAX a,MEM32 b){x86(165,a,b);}
V cmove(EAX a,R_M32 b){x86(165,a,b);}
V cmove(ECX a,EAX b){x86(165,a,b);}
V cmove(ECX a,ECX b){x86(165,a,b);}
V cmove(ECX a,REG32 b){x86(165,a,b);}
V cmove(ECX a,MEM32 b){x86(165,a,b);}
V cmove(ECX a,R_M32 b){x86(165,a,b);}
V cmove(REG32 a,EAX b){x86(165,a,b);}
V cmove(REG32 a,ECX b){x86(165,a,b);}
V cmove(REG32 a,REG32 b){x86(165,a,b);}
V cmove(REG32 a,MEM32 b){x86(165,a,b);}
V cmove(REG32 a,R_M32 b){x86(165,a,b);}
V cmovz(EAX a,EAX b){x86(166,a,b);}
V cmovz(EAX a,ECX b){x86(166,a,b);}
V cmovz(EAX a,REG32 b){x86(166,a,b);}
V cmovz(EAX a,MEM32 b){x86(166,a,b);}
V cmovz(EAX a,R_M32 b){x86(166,a,b);}
V cmovz(ECX a,EAX b){x86(166,a,b);}
V cmovz(ECX a,ECX b){x86(166,a,b);}
V cmovz(ECX a,REG32 b){x86(166,a,b);}
V cmovz(ECX a,MEM32 b){x86(166,a,b);}
V cmovz(ECX a,R_M32 b){x86(166,a,b);}
V cmovz(REG32 a,EAX b){x86(166,a,b);}
V cmovz(REG32 a,ECX b){x86(166,a,b);}
V cmovz(REG32 a,REG32 b){x86(166,a,b);}
V cmovz(REG32 a,MEM32 b){x86(166,a,b);}
V cmovz(REG32 a,R_M32 b){x86(166,a,b);}
V cmovne(EAX a,EAX b){x86(167,a,b);}
V cmovne(EAX a,ECX b){x86(167,a,b);}
V cmovne(EAX a,REG32 b){x86(167,a,b);}
V cmovne(EAX a,MEM32 b){x86(167,a,b);}
V cmovne(EAX a,R_M32 b){x86(167,a,b);}
V cmovne(ECX a,EAX b){x86(167,a,b);}
V cmovne(ECX a,ECX b){x86(167,a,b);}
V cmovne(ECX a,REG32 b){x86(167,a,b);}
V cmovne(ECX a,MEM32 b){x86(167,a,b);}
V cmovne(ECX a,R_M32 b){x86(167,a,b);}
V cmovne(REG32 a,EAX b){x86(167,a,b);}
V cmovne(REG32 a,ECX b){x86(167,a,b);}
V cmovne(REG32 a,REG32 b){x86(167,a,b);}
V cmovne(REG32 a,MEM32 b){x86(167,a,b);}
V cmovne(REG32 a,R_M32 b){x86(167,a,b);}
V cmovnz(EAX a,EAX b){x86(168,a,b);}
V cmovnz(EAX a,ECX b){x86(168,a,b);}
V cmovnz(EAX a,REG32 b){x86(168,a,b);}
V cmovnz(EAX a,MEM32 b){x86(168,a,b);}
V cmovnz(EAX a,R_M32 b){x86(168,a,b);}
V cmovnz(ECX a,EAX b){x86(168,a,b);}
V cmovnz(ECX a,ECX b){x86(168,a,b);}
V cmovnz(ECX a,REG32 b){x86(168,a,b);}
V cmovnz(ECX a,MEM32 b){x86(168,a,b);}
V cmovnz(ECX a,R_M32 b){x86(168,a,b);}
V cmovnz(REG32 a,EAX b){x86(168,a,b);}
V cmovnz(REG32 a,ECX b){x86(168,a,b);}
V cmovnz(REG32 a,REG32 b){x86(168,a,b);}
V cmovnz(REG32 a,MEM32 b){x86(168,a,b);}
V cmovnz(REG32 a,R_M32 b){x86(168,a,b);}
V cmovbe(EAX a,EAX b){x86(169,a,b);}
V cmovbe(EAX a,ECX b){x86(169,a,b);}
V cmovbe(EAX a,REG32 b){x86(169,a,b);}
V cmovbe(EAX a,MEM32 b){x86(169,a,b);}
V cmovbe(EAX a,R_M32 b){x86(169,a,b);}
V cmovbe(ECX a,EAX b){x86(169,a,b);}
V cmovbe(ECX a,ECX b){x86(169,a,b);}
V cmovbe(ECX a,REG32 b){x86(169,a,b);}
V cmovbe(ECX a,MEM32 b){x86(169,a,b);}
V cmovbe(ECX a,R_M32 b){x86(169,a,b);}
V cmovbe(REG32 a,EAX b){x86(169,a,b);}
V cmovbe(REG32 a,ECX b){x86(169,a,b);}
V cmovbe(REG32 a,REG32 b){x86(169,a,b);}
V cmovbe(REG32 a,MEM32 b){x86(169,a,b);}
V cmovbe(REG32 a,R_M32 b){x86(169,a,b);}
V cmovna(EAX a,EAX b){x86(170,a,b);}
V cmovna(EAX a,ECX b){x86(170,a,b);}
V cmovna(EAX a,REG32 b){x86(170,a,b);}
V cmovna(EAX a,MEM32 b){x86(170,a,b);}
V cmovna(EAX a,R_M32 b){x86(170,a,b);}
V cmovna(ECX a,EAX b){x86(170,a,b);}
V cmovna(ECX a,ECX b){x86(170,a,b);}
V cmovna(ECX a,REG32 b){x86(170,a,b);}
V cmovna(ECX a,MEM32 b){x86(170,a,b);}
V cmovna(ECX a,R_M32 b){x86(170,a,b);}
V cmovna(REG32 a,EAX b){x86(170,a,b);}
V cmovna(REG32 a,ECX b){x86(170,a,b);}
V cmovna(REG32 a,REG32 b){x86(170,a,b);}
V cmovna(REG32 a,MEM32 b){x86(170,a,b);}
V cmovna(REG32 a,R_M32 b){x86(170,a,b);}
V cmova(EAX a,EAX b){x86(171,a,b);}
V cmova(EAX a,ECX b){x86(171,a,b);}
V cmova(EAX a,REG32 b){x86(171,a,b);}
V cmova(EAX a,MEM32 b){x86(171,a,b);}
V cmova(EAX a,R_M32 b){x86(171,a,b);}
V cmova(ECX a,EAX b){x86(171,a,b);}
V cmova(ECX a,ECX b){x86(171,a,b);}
V cmova(ECX a,REG32 b){x86(171,a,b);}
V cmova(ECX a,MEM32 b){x86(171,a,b);}
V cmova(ECX a,R_M32 b){x86(171,a,b);}
V cmova(REG32 a,EAX b){x86(171,a,b);}
V cmova(REG32 a,ECX b){x86(171,a,b);}
V cmova(REG32 a,REG32 b){x86(171,a,b);}
V cmova(REG32 a,MEM32 b){x86(171,a,b);}
V cmova(REG32 a,R_M32 b){x86(171,a,b);}
V cmovnbe(EAX a,EAX b){x86(172,a,b);}
V cmovnbe(EAX a,ECX b){x86(172,a,b);}
V cmovnbe(EAX a,REG32 b){x86(172,a,b);}
V cmovnbe(EAX a,MEM32 b){x86(172,a,b);}
V cmovnbe(EAX a,R_M32 b){x86(172,a,b);}
V cmovnbe(ECX a,EAX b){x86(172,a,b);}
V cmovnbe(ECX a,ECX b){x86(172,a,b);}
V cmovnbe(ECX a,REG32 b){x86(172,a,b);}
V cmovnbe(ECX a,MEM32 b){x86(172,a,b);}
V cmovnbe(ECX a,R_M32 b){x86(172,a,b);}
V cmovnbe(REG32 a,EAX b){x86(172,a,b);}
V cmovnbe(REG32 a,ECX b){x86(172,a,b);}
V cmovnbe(REG32 a,REG32 b){x86(172,a,b);}
V cmovnbe(REG32 a,MEM32 b){x86(172,a,b);}
V cmovnbe(REG32 a,R_M32 b){x86(172,a,b);}
V cmovs(EAX a,EAX b){x86(173,a,b);}
V cmovs(EAX a,ECX b){x86(173,a,b);}
V cmovs(EAX a,REG32 b){x86(173,a,b);}
V cmovs(EAX a,MEM32 b){x86(173,a,b);}
V cmovs(EAX a,R_M32 b){x86(173,a,b);}
V cmovs(ECX a,EAX b){x86(173,a,b);}
V cmovs(ECX a,ECX b){x86(173,a,b);}
V cmovs(ECX a,REG32 b){x86(173,a,b);}
V cmovs(ECX a,MEM32 b){x86(173,a,b);}
V cmovs(ECX a,R_M32 b){x86(173,a,b);}
V cmovs(REG32 a,EAX b){x86(173,a,b);}
V cmovs(REG32 a,ECX b){x86(173,a,b);}
V cmovs(REG32 a,REG32 b){x86(173,a,b);}
V cmovs(REG32 a,MEM32 b){x86(173,a,b);}
V cmovs(REG32 a,R_M32 b){x86(173,a,b);}
V cmovns(EAX a,EAX b){x86(174,a,b);}
V cmovns(EAX a,ECX b){x86(174,a,b);}
V cmovns(EAX a,REG32 b){x86(174,a,b);}
V cmovns(EAX a,MEM32 b){x86(174,a,b);}
V cmovns(EAX a,R_M32 b){x86(174,a,b);}
V cmovns(ECX a,EAX b){x86(174,a,b);}
V cmovns(ECX a,ECX b){x86(174,a,b);}
V cmovns(ECX a,REG32 b){x86(174,a,b);}
V cmovns(ECX a,MEM32 b){x86(174,a,b);}
V cmovns(ECX a,R_M32 b){x86(174,a,b);}
V cmovns(REG32 a,EAX b){x86(174,a,b);}
V cmovns(REG32 a,ECX b){x86(174,a,b);}
V cmovns(REG32 a,REG32 b){x86(174,a,b);}
V cmovns(REG32 a,MEM32 b){x86(174,a,b);}
V cmovns(REG32 a,R_M32 b){x86(174,a,b);}
V cmovp(EAX a,EAX b){x86(175,a,b);}
V cmovp(EAX a,ECX b){x86(175,a,b);}
V cmovp(EAX a,REG32 b){x86(175,a,b);}
V cmovp(EAX a,MEM32 b){x86(175,a,b);}
V cmovp(EAX a,R_M32 b){x86(175,a,b);}
V cmovp(ECX a,EAX b){x86(175,a,b);}
V cmovp(ECX a,ECX b){x86(175,a,b);}
V cmovp(ECX a,REG32 b){x86(175,a,b);}
V cmovp(ECX a,MEM32 b){x86(175,a,b);}
V cmovp(ECX a,R_M32 b){x86(175,a,b);}
V cmovp(REG32 a,EAX b){x86(175,a,b);}
V cmovp(REG32 a,ECX b){x86(175,a,b);}
V cmovp(REG32 a,REG32 b){x86(175,a,b);}
V cmovp(REG32 a,MEM32 b){x86(175,a,b);}
V cmovp(REG32 a,R_M32 b){x86(175,a,b);}
V cmovpe(EAX a,EAX b){x86(176,a,b);}
V cmovpe(EAX a,ECX b){x86(176,a,b);}
V cmovpe(EAX a,REG32 b){x86(176,a,b);}
V cmovpe(EAX a,MEM32 b){x86(176,a,b);}
V cmovpe(EAX a,R_M32 b){x86(176,a,b);}
V cmovpe(ECX a,EAX b){x86(176,a,b);}
V cmovpe(ECX a,ECX b){x86(176,a,b);}
V cmovpe(ECX a,REG32 b){x86(176,a,b);}
V cmovpe(ECX a,MEM32 b){x86(176,a,b);}
V cmovpe(ECX a,R_M32 b){x86(176,a,b);}
V cmovpe(REG32 a,EAX b){x86(176,a,b);}
V cmovpe(REG32 a,ECX b){x86(176,a,b);}
V cmovpe(REG32 a,REG32 b){x86(176,a,b);}
V cmovpe(REG32 a,MEM32 b){x86(176,a,b);}
V cmovpe(REG32 a,R_M32 b){x86(176,a,b);}
V cmovnp(EAX a,EAX b){x86(177,a,b);}
V cmovnp(EAX a,ECX b){x86(177,a,b);}
V cmovnp(EAX a,REG32 b){x86(177,a,b);}
V cmovnp(EAX a,MEM32 b){x86(177,a,b);}
V cmovnp(EAX a,R_M32 b){x86(177,a,b);}
V cmovnp(ECX a,EAX b){x86(177,a,b);}
V cmovnp(ECX a,ECX b){x86(177,a,b);}
V cmovnp(ECX a,REG32 b){x86(177,a,b);}
V cmovnp(ECX a,MEM32 b){x86(177,a,b);}
V cmovnp(ECX a,R_M32 b){x86(177,a,b);}
V cmovnp(REG32 a,EAX b){x86(177,a,b);}
V cmovnp(REG32 a,ECX b){x86(177,a,b);}
V cmovnp(REG32 a,REG32 b){x86(177,a,b);}
V cmovnp(REG32 a,MEM32 b){x86(177,a,b);}
V cmovnp(REG32 a,R_M32 b){x86(177,a,b);}
V cmovpo(EAX a,EAX b){x86(178,a,b);}
V cmovpo(EAX a,ECX b){x86(178,a,b);}
V cmovpo(EAX a,REG32 b){x86(178,a,b);}
V cmovpo(EAX a,MEM32 b){x86(178,a,b);}
V cmovpo(EAX a,R_M32 b){x86(178,a,b);}
V cmovpo(ECX a,EAX b){x86(178,a,b);}
V cmovpo(ECX a,ECX b){x86(178,a,b);}
V cmovpo(ECX a,REG32 b){x86(178,a,b);}
V cmovpo(ECX a,MEM32 b){x86(178,a,b);}
V cmovpo(ECX a,R_M32 b){x86(178,a,b);}
V cmovpo(REG32 a,EAX b){x86(178,a,b);}
V cmovpo(REG32 a,ECX b){x86(178,a,b);}
V cmovpo(REG32 a,REG32 b){x86(178,a,b);}
V cmovpo(REG32 a,MEM32 b){x86(178,a,b);}
V cmovpo(REG32 a,R_M32 b){x86(178,a,b);}
V cmovl(EAX a,EAX b){x86(179,a,b);}
V cmovl(EAX a,ECX b){x86(179,a,b);}
V cmovl(EAX a,REG32 b){x86(179,a,b);}
V cmovl(EAX a,MEM32 b){x86(179,a,b);}
V cmovl(EAX a,R_M32 b){x86(179,a,b);}
V cmovl(ECX a,EAX b){x86(179,a,b);}
V cmovl(ECX a,ECX b){x86(179,a,b);}
V cmovl(ECX a,REG32 b){x86(179,a,b);}
V cmovl(ECX a,MEM32 b){x86(179,a,b);}
V cmovl(ECX a,R_M32 b){x86(179,a,b);}
V cmovl(REG32 a,EAX b){x86(179,a,b);}
V cmovl(REG32 a,ECX b){x86(179,a,b);}
V cmovl(REG32 a,REG32 b){x86(179,a,b);}
V cmovl(REG32 a,MEM32 b){x86(179,a,b);}
V cmovl(REG32 a,R_M32 b){x86(179,a,b);}
V cmovnge(EAX a,EAX b){x86(180,a,b);}
V cmovnge(EAX a,ECX b){x86(180,a,b);}
V cmovnge(EAX a,REG32 b){x86(180,a,b);}
V cmovnge(EAX a,MEM32 b){x86(180,a,b);}
V cmovnge(EAX a,R_M32 b){x86(180,a,b);}
V cmovnge(ECX a,EAX b){x86(180,a,b);}
V cmovnge(ECX a,ECX b){x86(180,a,b);}
V cmovnge(ECX a,REG32 b){x86(180,a,b);}
V cmovnge(ECX a,MEM32 b){x86(180,a,b);}
V cmovnge(ECX a,R_M32 b){x86(180,a,b);}
V cmovnge(REG32 a,EAX b){x86(180,a,b);}
V cmovnge(REG32 a,ECX b){x86(180,a,b);}
V cmovnge(REG32 a,REG32 b){x86(180,a,b);}
V cmovnge(REG32 a,MEM32 b){x86(180,a,b);}
V cmovnge(REG32 a,R_M32 b){x86(180,a,b);}
V cmovge(EAX a,EAX b){x86(181,a,b);}
V cmovge(EAX a,ECX b){x86(181,a,b);}
V cmovge(EAX a,REG32 b){x86(181,a,b);}
V cmovge(EAX a,MEM32 b){x86(181,a,b);}
V cmovge(EAX a,R_M32 b){x86(181,a,b);}
V cmovge(ECX a,EAX b){x86(181,a,b);}
V cmovge(ECX a,ECX b){x86(181,a,b);}
V cmovge(ECX a,REG32 b){x86(181,a,b);}
V cmovge(ECX a,MEM32 b){x86(181,a,b);}
V cmovge(ECX a,R_M32 b){x86(181,a,b);}
V cmovge(REG32 a,EAX b){x86(181,a,b);}
V cmovge(REG32 a,ECX b){x86(181,a,b);}
V cmovge(REG32 a,REG32 b){x86(181,a,b);}
V cmovge(REG32 a,MEM32 b){x86(181,a,b);}
V cmovge(REG32 a,R_M32 b){x86(181,a,b);}
V cmovnl(EAX a,EAX b){x86(182,a,b);}
V cmovnl(EAX a,ECX b){x86(182,a,b);}
V cmovnl(EAX a,REG32 b){x86(182,a,b);}
V cmovnl(EAX a,MEM32 b){x86(182,a,b);}
V cmovnl(EAX a,R_M32 b){x86(182,a,b);}
V cmovnl(ECX a,EAX b){x86(182,a,b);}
V cmovnl(ECX a,ECX b){x86(182,a,b);}
V cmovnl(ECX a,REG32 b){x86(182,a,b);}
V cmovnl(ECX a,MEM32 b){x86(182,a,b);}
V cmovnl(ECX a,R_M32 b){x86(182,a,b);}
V cmovnl(REG32 a,EAX b){x86(182,a,b);}
V cmovnl(REG32 a,ECX b){x86(182,a,b);}
V cmovnl(REG32 a,REG32 b){x86(182,a,b);}
V cmovnl(REG32 a,MEM32 b){x86(182,a,b);}
V cmovnl(REG32 a,R_M32 b){x86(182,a,b);}
V cmovle(EAX a,EAX b){x86(183,a,b);}
V cmovle(EAX a,ECX b){x86(183,a,b);}
V cmovle(EAX a,REG32 b){x86(183,a,b);}
V cmovle(EAX a,MEM32 b){x86(183,a,b);}
V cmovle(EAX a,R_M32 b){x86(183,a,b);}
V cmovle(ECX a,EAX b){x86(183,a,b);}
V cmovle(ECX a,ECX b){x86(183,a,b);}
V cmovle(ECX a,REG32 b){x86(183,a,b);}
V cmovle(ECX a,MEM32 b){x86(183,a,b);}
V cmovle(ECX a,R_M32 b){x86(183,a,b);}
V cmovle(REG32 a,EAX b){x86(183,a,b);}
V cmovle(REG32 a,ECX b){x86(183,a,b);}
V cmovle(REG32 a,REG32 b){x86(183,a,b);}
V cmovle(REG32 a,MEM32 b){x86(183,a,b);}
V cmovle(REG32 a,R_M32 b){x86(183,a,b);}
V cmovng(EAX a,EAX b){x86(184,a,b);}
V cmovng(EAX a,ECX b){x86(184,a,b);}
V cmovng(EAX a,REG32 b){x86(184,a,b);}
V cmovng(EAX a,MEM32 b){x86(184,a,b);}
V cmovng(EAX a,R_M32 b){x86(184,a,b);}
V cmovng(ECX a,EAX b){x86(184,a,b);}
V cmovng(ECX a,ECX b){x86(184,a,b);}
V cmovng(ECX a,REG32 b){x86(184,a,b);}
V cmovng(ECX a,MEM32 b){x86(184,a,b);}
V cmovng(ECX a,R_M32 b){x86(184,a,b);}
V cmovng(REG32 a,EAX b){x86(184,a,b);}
V cmovng(REG32 a,ECX b){x86(184,a,b);}
V cmovng(REG32 a,REG32 b){x86(184,a,b);}
V cmovng(REG32 a,MEM32 b){x86(184,a,b);}
V cmovng(REG32 a,R_M32 b){x86(184,a,b);}
V cmovg(EAX a,EAX b){x86(185,a,b);}
V cmovg(EAX a,ECX b){x86(185,a,b);}
V cmovg(EAX a,REG32 b){x86(185,a,b);}
V cmovg(EAX a,MEM32 b){x86(185,a,b);}
V cmovg(EAX a,R_M32 b){x86(185,a,b);}
V cmovg(ECX a,EAX b){x86(185,a,b);}
V cmovg(ECX a,ECX b){x86(185,a,b);}
V cmovg(ECX a,REG32 b){x86(185,a,b);}
V cmovg(ECX a,MEM32 b){x86(185,a,b);}
V cmovg(ECX a,R_M32 b){x86(185,a,b);}
V cmovg(REG32 a,EAX b){x86(185,a,b);}
V cmovg(REG32 a,ECX b){x86(185,a,b);}
V cmovg(REG32 a,REG32 b){x86(185,a,b);}
V cmovg(REG32 a,MEM32 b){x86(185,a,b);}
V cmovg(REG32 a,R_M32 b){x86(185,a,b);}
V cmovnle(EAX a,EAX b){x86(186,a,b);}
V cmovnle(EAX a,ECX b){x86(186,a,b);}
V cmovnle(EAX a,REG32 b){x86(186,a,b);}
V cmovnle(EAX a,MEM32 b){x86(186,a,b);}
V cmovnle(EAX a,R_M32 b){x86(186,a,b);}
V cmovnle(ECX a,EAX b){x86(186,a,b);}
V cmovnle(ECX a,ECX b){x86(186,a,b);}
V cmovnle(ECX a,REG32 b){x86(186,a,b);}
V cmovnle(ECX a,MEM32 b){x86(186,a,b);}
V cmovnle(ECX a,R_M32 b){x86(186,a,b);}
V cmovnle(REG32 a,EAX b){x86(186,a,b);}
V cmovnle(REG32 a,ECX b){x86(186,a,b);}
V cmovnle(REG32 a,REG32 b){x86(186,a,b);}
V cmovnle(REG32 a,MEM32 b){x86(186,a,b);}
V cmovnle(REG32 a,R_M32 b){x86(186,a,b);}
V cmp(AL a,AL b){x86(187,a,b);}
V cmp(AL a,CL b){x86(187,a,b);}
V cmp(AL a,REG8 b){x86(187,a,b);}
V cmp(CL a,AL b){x86(187,a,b);}
V cmp(CL a,CL b){x86(187,a,b);}
V cmp(CL a,REG8 b){x86(187,a,b);}
V cmp(REG8 a,AL b){x86(187,a,b);}
V cmp(REG8 a,CL b){x86(187,a,b);}
V cmp(REG8 a,REG8 b){x86(187,a,b);}
V cmp(MEM8 a,AL b){x86(187,a,b);}
V cmp(MEM8 a,CL b){x86(187,a,b);}
V cmp(MEM8 a,REG8 b){x86(187,a,b);}
V cmp(R_M8 a,AL b){x86(187,a,b);}
V cmp(R_M8 a,CL b){x86(187,a,b);}
V cmp(R_M8 a,REG8 b){x86(187,a,b);}
V cmp(AX a,AX b){x86(188,a,b);}
V cmp(AX a,DX b){x86(188,a,b);}
V cmp(AX a,CX b){x86(188,a,b);}
V cmp(AX a,REG16 b){x86(188,a,b);}
V cmp(DX a,AX b){x86(188,a,b);}
V cmp(DX a,DX b){x86(188,a,b);}
V cmp(DX a,CX b){x86(188,a,b);}
V cmp(DX a,REG16 b){x86(188,a,b);}
V cmp(CX a,AX b){x86(188,a,b);}
V cmp(CX a,DX b){x86(188,a,b);}
V cmp(CX a,CX b){x86(188,a,b);}
V cmp(CX a,REG16 b){x86(188,a,b);}
V cmp(REG16 a,AX b){x86(188,a,b);}
V cmp(REG16 a,DX b){x86(188,a,b);}
V cmp(REG16 a,CX b){x86(188,a,b);}
V cmp(REG16 a,REG16 b){x86(188,a,b);}
V cmp(MEM16 a,AX b){x86(188,a,b);}
V cmp(MEM16 a,DX b){x86(188,a,b);}
V cmp(MEM16 a,CX b){x86(188,a,b);}
V cmp(MEM16 a,REG16 b){x86(188,a,b);}
V cmp(R_M16 a,AX b){x86(188,a,b);}
V cmp(R_M16 a,DX b){x86(188,a,b);}
V cmp(R_M16 a,CX b){x86(188,a,b);}
V cmp(R_M16 a,REG16 b){x86(188,a,b);}
V cmp(EAX a,EAX b){x86(189,a,b);}
V cmp(EAX a,ECX b){x86(189,a,b);}
V cmp(EAX a,REG32 b){x86(189,a,b);}
V cmp(ECX a,EAX b){x86(189,a,b);}
V cmp(ECX a,ECX b){x86(189,a,b);}
V cmp(ECX a,REG32 b){x86(189,a,b);}
V cmp(REG32 a,EAX b){x86(189,a,b);}
V cmp(REG32 a,ECX b){x86(189,a,b);}
V cmp(REG32 a,REG32 b){x86(189,a,b);}
V cmp(MEM32 a,EAX b){x86(189,a,b);}
V cmp(MEM32 a,ECX b){x86(189,a,b);}
V cmp(MEM32 a,REG32 b){x86(189,a,b);}
V cmp(R_M32 a,EAX b){x86(189,a,b);}
V cmp(R_M32 a,ECX b){x86(189,a,b);}
V cmp(R_M32 a,REG32 b){x86(189,a,b);}
V cmp(AL a,MEM8 b){x86(190,a,b);}
V cmp(AL a,R_M8 b){x86(190,a,b);}
V cmp(CL a,MEM8 b){x86(190,a,b);}
V cmp(CL a,R_M8 b){x86(190,a,b);}
V cmp(REG8 a,MEM8 b){x86(190,a,b);}
V cmp(REG8 a,R_M8 b){x86(190,a,b);}
V cmp(AX a,MEM16 b){x86(191,a,b);}
V cmp(AX a,R_M16 b){x86(191,a,b);}
V cmp(DX a,MEM16 b){x86(191,a,b);}
V cmp(DX a,R_M16 b){x86(191,a,b);}
V cmp(CX a,MEM16 b){x86(191,a,b);}
V cmp(CX a,R_M16 b){x86(191,a,b);}
V cmp(REG16 a,MEM16 b){x86(191,a,b);}
V cmp(REG16 a,R_M16 b){x86(191,a,b);}
V cmp(EAX a,MEM32 b){x86(192,a,b);}
V cmp(EAX a,R_M32 b){x86(192,a,b);}
V cmp(ECX a,MEM32 b){x86(192,a,b);}
V cmp(ECX a,R_M32 b){x86(192,a,b);}
V cmp(REG32 a,MEM32 b){x86(192,a,b);}
V cmp(REG32 a,R_M32 b){x86(192,a,b);}
V cmp(AL a,char b){x86(193,a,(IMM)b);}
V cmp(CL a,char b){x86(193,a,(IMM)b);}
V cmp(REG8 a,char b){x86(193,a,(IMM)b);}
V cmp(MEM8 a,char b){x86(193,a,(IMM)b);}
V cmp(R_M8 a,char b){x86(193,a,(IMM)b);}
V cmp(AX a,char b){x86(194,a,(IMM)b);}
V cmp(AX a,short b){x86(194,a,(IMM)b);}
V cmp(DX a,char b){x86(194,a,(IMM)b);}
V cmp(DX a,short b){x86(194,a,(IMM)b);}
V cmp(CX a,char b){x86(194,a,(IMM)b);}
V cmp(CX a,short b){x86(194,a,(IMM)b);}
V cmp(REG16 a,char b){x86(194,a,(IMM)b);}
V cmp(REG16 a,short b){x86(194,a,(IMM)b);}
V cmp(MEM16 a,char b){x86(194,a,(IMM)b);}
V cmp(MEM16 a,short b){x86(194,a,(IMM)b);}
V cmp(R_M16 a,char b){x86(194,a,(IMM)b);}
V cmp(R_M16 a,short b){x86(194,a,(IMM)b);}
V cmp(EAX a,int b){x86(195,a,(IMM)b);}
V cmp(EAX a,char b){x86(195,a,(IMM)b);}
V cmp(EAX a,short b){x86(195,a,(IMM)b);}
V cmp(EAX a,REF b){x86(195,a,b);}
V cmp(ECX a,int b){x86(195,a,(IMM)b);}
V cmp(ECX a,char b){x86(195,a,(IMM)b);}
V cmp(ECX a,short b){x86(195,a,(IMM)b);}
V cmp(ECX a,REF b){x86(195,a,b);}
V cmp(REG32 a,int b){x86(195,a,(IMM)b);}
V cmp(REG32 a,char b){x86(195,a,(IMM)b);}
V cmp(REG32 a,short b){x86(195,a,(IMM)b);}
V cmp(REG32 a,REF b){x86(195,a,b);}
V cmp(MEM32 a,int b){x86(195,a,(IMM)b);}
V cmp(MEM32 a,char b){x86(195,a,(IMM)b);}
V cmp(MEM32 a,short b){x86(195,a,(IMM)b);}
V cmp(MEM32 a,REF b){x86(195,a,b);}
V cmp(R_M32 a,int b){x86(195,a,(IMM)b);}
V cmp(R_M32 a,char b){x86(195,a,(IMM)b);}
V cmp(R_M32 a,short b){x86(195,a,(IMM)b);}
V cmp(R_M32 a,REF b){x86(195,a,b);}
V cmppd(XMMREG a,XMMREG b,char c){x86(201,a,b,(IMM)c);}
V cmppd(XMMREG a,MEM128 b,char c){x86(201,a,b,(IMM)c);}
V cmppd(XMMREG a,R_M128 b,char c){x86(201,a,b,(IMM)c);}
V cmpeqpd(XMMREG a,XMMREG b){x86(202,a,b);}
V cmpeqpd(XMMREG a,MEM128 b){x86(202,a,b);}
V cmpeqpd(XMMREG a,R_M128 b){x86(202,a,b);}
V cmpltpd(XMMREG a,XMMREG b){x86(203,a,b);}
V cmpltpd(XMMREG a,MEM128 b){x86(203,a,b);}
V cmpltpd(XMMREG a,R_M128 b){x86(203,a,b);}
V cmplepd(XMMREG a,XMMREG b){x86(204,a,b);}
V cmplepd(XMMREG a,MEM128 b){x86(204,a,b);}
V cmplepd(XMMREG a,R_M128 b){x86(204,a,b);}
V cmpunordpd(XMMREG a,XMMREG b){x86(205,a,b);}
V cmpunordpd(XMMREG a,MEM128 b){x86(205,a,b);}
V cmpunordpd(XMMREG a,R_M128 b){x86(205,a,b);}
V cmpneqpd(XMMREG a,XMMREG b){x86(206,a,b);}
V cmpneqpd(XMMREG a,MEM128 b){x86(206,a,b);}
V cmpneqpd(XMMREG a,R_M128 b){x86(206,a,b);}
V cmpnltpd(XMMREG a,XMMREG b){x86(207,a,b);}
V cmpnltpd(XMMREG a,MEM128 b){x86(207,a,b);}
V cmpnltpd(XMMREG a,R_M128 b){x86(207,a,b);}
V cmpnlepd(XMMREG a,XMMREG b){x86(208,a,b);}
V cmpnlepd(XMMREG a,MEM128 b){x86(208,a,b);}
V cmpnlepd(XMMREG a,R_M128 b){x86(208,a,b);}
V cmpordpd(XMMREG a,XMMREG b){x86(209,a,b);}
V cmpordpd(XMMREG a,MEM128 b){x86(209,a,b);}
V cmpordpd(XMMREG a,R_M128 b){x86(209,a,b);}
V cmpps(XMMREG a,XMMREG b,char c){x86(210,a,b,(IMM)c);}
V cmpps(XMMREG a,MEM128 b,char c){x86(210,a,b,(IMM)c);}
V cmpps(XMMREG a,R_M128 b,char c){x86(210,a,b,(IMM)c);}
V cmpeqps(XMMREG a,XMMREG b){x86(211,a,b);}
V cmpeqps(XMMREG a,MEM128 b){x86(211,a,b);}
V cmpeqps(XMMREG a,R_M128 b){x86(211,a,b);}
V cmpleps(XMMREG a,XMMREG b){x86(212,a,b);}
V cmpleps(XMMREG a,MEM128 b){x86(212,a,b);}
V cmpleps(XMMREG a,R_M128 b){x86(212,a,b);}
V cmpltps(XMMREG a,XMMREG b){x86(213,a,b);}
V cmpltps(XMMREG a,MEM128 b){x86(213,a,b);}
V cmpltps(XMMREG a,R_M128 b){x86(213,a,b);}
V cmpneqps(XMMREG a,XMMREG b){x86(214,a,b);}
V cmpneqps(XMMREG a,MEM128 b){x86(214,a,b);}
V cmpneqps(XMMREG a,R_M128 b){x86(214,a,b);}
V cmpnleps(XMMREG a,XMMREG b){x86(215,a,b);}
V cmpnleps(XMMREG a,MEM128 b){x86(215,a,b);}
V cmpnleps(XMMREG a,R_M128 b){x86(215,a,b);}
V cmpnltps(XMMREG a,XMMREG b){x86(216,a,b);}
V cmpnltps(XMMREG a,MEM128 b){x86(216,a,b);}
V cmpnltps(XMMREG a,R_M128 b){x86(216,a,b);}
V cmpordps(XMMREG a,XMMREG b){x86(217,a,b);}
V cmpordps(XMMREG a,MEM128 b){x86(217,a,b);}
V cmpordps(XMMREG a,R_M128 b){x86(217,a,b);}
V cmpunordps(XMMREG a,XMMREG b){x86(218,a,b);}
V cmpunordps(XMMREG a,MEM128 b){x86(218,a,b);}
V cmpunordps(XMMREG a,R_M128 b){x86(218,a,b);}
V cmpsb(){x86(219);}
V cmpsw(){x86(220);}
V cmpsd(){x86(221);}
V repe_cmpsb(){x86(222);}
V repe_cmpsw(){x86(223);}
V repe_cmpsd(){x86(224);}
V repne_cmpsb(){x86(225);}
V repne_cmpsw(){x86(226);}
V repne_cmpsd(){x86(227);}
V repz_cmpsb(){x86(228);}
V repz_cmpsw(){x86(229);}
V repz_cmpsd(){x86(230);}
V repnz_cmpsb(){x86(231);}
V repnz_cmpsw(){x86(232);}
V repnz_cmpsd(){x86(233);}
V cmpsd(XMMREG a,XMMREG b,char c){x86(234,a,b,(IMM)c);}
V cmpsd(XMMREG a,MEM64 b,char c){x86(234,a,b,(IMM)c);}
V cmpsd(XMMREG a,XMM64 b,char c){x86(234,a,b,(IMM)c);}
V cmpeqsd(XMMREG a,XMMREG b){x86(235,a,b);}
V cmpeqsd(XMMREG a,MEM64 b){x86(235,a,b);}
V cmpeqsd(XMMREG a,XMM64 b){x86(235,a,b);}
V cmpltsd(XMMREG a,XMMREG b){x86(236,a,b);}
V cmpltsd(XMMREG a,MEM64 b){x86(236,a,b);}
V cmpltsd(XMMREG a,XMM64 b){x86(236,a,b);}
V cmplesd(XMMREG a,XMMREG b){x86(237,a,b);}
V cmplesd(XMMREG a,MEM64 b){x86(237,a,b);}
V cmplesd(XMMREG a,XMM64 b){x86(237,a,b);}
V cmpunordsd(XMMREG a,XMMREG b){x86(238,a,b);}
V cmpunordsd(XMMREG a,MEM64 b){x86(238,a,b);}
V cmpunordsd(XMMREG a,XMM64 b){x86(238,a,b);}
V cmpneqsd(XMMREG a,XMMREG b){x86(239,a,b);}
V cmpneqsd(XMMREG a,MEM64 b){x86(239,a,b);}
V cmpneqsd(XMMREG a,XMM64 b){x86(239,a,b);}
V cmpnltsd(XMMREG a,XMMREG b){x86(240,a,b);}
V cmpnltsd(XMMREG a,MEM64 b){x86(240,a,b);}
V cmpnltsd(XMMREG a,XMM64 b){x86(240,a,b);}
V cmpnlesd(XMMREG a,XMMREG b){x86(241,a,b);}
V cmpnlesd(XMMREG a,MEM64 b){x86(241,a,b);}
V cmpnlesd(XMMREG a,XMM64 b){x86(241,a,b);}
V cmpordsd(XMMREG a,XMMREG b){x86(242,a,b);}
V cmpordsd(XMMREG a,MEM64 b){x86(242,a,b);}
V cmpordsd(XMMREG a,XMM64 b){x86(242,a,b);}
V cmpss(XMMREG a,XMMREG b,char c){x86(243,a,b,(IMM)c);}
V cmpss(XMMREG a,MEM32 b,char c){x86(243,a,b,(IMM)c);}
V cmpss(XMMREG a,XMM32 b,char c){x86(243,a,b,(IMM)c);}
V cmpeqss(XMMREG a,XMMREG b){x86(244,a,b);}
V cmpeqss(XMMREG a,MEM32 b){x86(244,a,b);}
V cmpeqss(XMMREG a,XMM32 b){x86(244,a,b);}
V cmpless(XMMREG a,XMMREG b){x86(245,a,b);}
V cmpless(XMMREG a,MEM32 b){x86(245,a,b);}
V cmpless(XMMREG a,XMM32 b){x86(245,a,b);}
V cmpltss(XMMREG a,XMMREG b){x86(246,a,b);}
V cmpltss(XMMREG a,MEM32 b){x86(246,a,b);}
V cmpltss(XMMREG a,XMM32 b){x86(246,a,b);}
V cmpneqss(XMMREG a,XMMREG b){x86(247,a,b);}
V cmpneqss(XMMREG a,MEM32 b){x86(247,a,b);}
V cmpneqss(XMMREG a,XMM32 b){x86(247,a,b);}
V cmpnless(XMMREG a,XMMREG b){x86(248,a,b);}
V cmpnless(XMMREG a,MEM32 b){x86(248,a,b);}
V cmpnless(XMMREG a,XMM32 b){x86(248,a,b);}
V cmpnltss(XMMREG a,XMMREG b){x86(249,a,b);}
V cmpnltss(XMMREG a,MEM32 b){x86(249,a,b);}
V cmpnltss(XMMREG a,XMM32 b){x86(249,a,b);}
V cmpordss(XMMREG a,XMMREG b){x86(250,a,b);}
V cmpordss(XMMREG a,MEM32 b){x86(250,a,b);}
V cmpordss(XMMREG a,XMM32 b){x86(250,a,b);}
V cmpunordss(XMMREG a,XMMREG b){x86(251,a,b);}
V cmpunordss(XMMREG a,MEM32 b){x86(251,a,b);}
V cmpunordss(XMMREG a,XMM32 b){x86(251,a,b);}
V cmpxchg(AL a,AL b){x86(252,a,b);}
V cmpxchg(AL a,CL b){x86(252,a,b);}
V cmpxchg(AL a,REG8 b){x86(252,a,b);}
V cmpxchg(CL a,AL b){x86(252,a,b);}
V cmpxchg(CL a,CL b){x86(252,a,b);}
V cmpxchg(CL a,REG8 b){x86(252,a,b);}
V cmpxchg(REG8 a,AL b){x86(252,a,b);}
V cmpxchg(REG8 a,CL b){x86(252,a,b);}
V cmpxchg(REG8 a,REG8 b){x86(252,a,b);}
V cmpxchg(MEM8 a,AL b){x86(252,a,b);}
V cmpxchg(MEM8 a,CL b){x86(252,a,b);}
V cmpxchg(MEM8 a,REG8 b){x86(252,a,b);}
V cmpxchg(R_M8 a,AL b){x86(252,a,b);}
V cmpxchg(R_M8 a,CL b){x86(252,a,b);}
V cmpxchg(R_M8 a,REG8 b){x86(252,a,b);}
V cmpxchg(AX a,AX b){x86(253,a,b);}
V cmpxchg(AX a,DX b){x86(253,a,b);}
V cmpxchg(AX a,CX b){x86(253,a,b);}
V cmpxchg(AX a,REG16 b){x86(253,a,b);}
V cmpxchg(DX a,AX b){x86(253,a,b);}
V cmpxchg(DX a,DX b){x86(253,a,b);}
V cmpxchg(DX a,CX b){x86(253,a,b);}
V cmpxchg(DX a,REG16 b){x86(253,a,b);}
V cmpxchg(CX a,AX b){x86(253,a,b);}
V cmpxchg(CX a,DX b){x86(253,a,b);}
V cmpxchg(CX a,CX b){x86(253,a,b);}
V cmpxchg(CX a,REG16 b){x86(253,a,b);}
V cmpxchg(REG16 a,AX b){x86(253,a,b);}
V cmpxchg(REG16 a,DX b){x86(253,a,b);}
V cmpxchg(REG16 a,CX b){x86(253,a,b);}
V cmpxchg(REG16 a,REG16 b){x86(253,a,b);}
V cmpxchg(MEM16 a,AX b){x86(253,a,b);}
V cmpxchg(MEM16 a,DX b){x86(253,a,b);}
V cmpxchg(MEM16 a,CX b){x86(253,a,b);}
V cmpxchg(MEM16 a,REG16 b){x86(253,a,b);}
V cmpxchg(R_M16 a,AX b){x86(253,a,b);}
V cmpxchg(R_M16 a,DX b){x86(253,a,b);}
V cmpxchg(R_M16 a,CX b){x86(253,a,b);}
V cmpxchg(R_M16 a,REG16 b){x86(253,a,b);}
V cmpxchg(EAX a,EAX b){x86(254,a,b);}
V cmpxchg(EAX a,ECX b){x86(254,a,b);}
V cmpxchg(EAX a,REG32 b){x86(254,a,b);}
V cmpxchg(ECX a,EAX b){x86(254,a,b);}
V cmpxchg(ECX a,ECX b){x86(254,a,b);}
V cmpxchg(ECX a,REG32 b){x86(254,a,b);}
V cmpxchg(REG32 a,EAX b){x86(254,a,b);}
V cmpxchg(REG32 a,ECX b){x86(254,a,b);}
V cmpxchg(REG32 a,REG32 b){x86(254,a,b);}
V cmpxchg(MEM32 a,EAX b){x86(254,a,b);}
V cmpxchg(MEM32 a,ECX b){x86(254,a,b);}
V cmpxchg(MEM32 a,REG32 b){x86(254,a,b);}
V cmpxchg(R_M32 a,EAX b){x86(254,a,b);}
V cmpxchg(R_M32 a,ECX b){x86(254,a,b);}
V cmpxchg(R_M32 a,REG32 b){x86(254,a,b);}
V lock_cmpxchg(MEM8 a,AL b){x86(255,a,b);}
V lock_cmpxchg(MEM8 a,CL b){x86(255,a,b);}
V lock_cmpxchg(MEM8 a,REG8 b){x86(255,a,b);}
V lock_cmpxchg(MEM16 a,AX b){x86(256,a,b);}
V lock_cmpxchg(MEM16 a,DX b){x86(256,a,b);}
V lock_cmpxchg(MEM16 a,CX b){x86(256,a,b);}
V lock_cmpxchg(MEM16 a,REG16 b){x86(256,a,b);}
V lock_cmpxchg(MEM32 a,EAX b){x86(257,a,b);}
V lock_cmpxchg(MEM32 a,ECX b){x86(257,a,b);}
V lock_cmpxchg(MEM32 a,REG32 b){x86(257,a,b);}
V cmpxchg8b(MEM8 a){x86(258,a);}
V cmpxchg8b(MEM16 a){x86(258,a);}
V cmpxchg8b(MEM32 a){x86(258,a);}
V cmpxchg8b(MEM64 a){x86(258,a);}
V cmpxchg8b(MEM128 a){x86(258,a);}
V lock_cmpxchg8b(MEM8 a){x86(259,a);}
V lock_cmpxchg8b(MEM16 a){x86(259,a);}
V lock_cmpxchg8b(MEM32 a){x86(259,a);}
V lock_cmpxchg8b(MEM64 a){x86(259,a);}
V lock_cmpxchg8b(MEM128 a){x86(259,a);}
V comisd(XMMREG a,XMMREG b){x86(260,a,b);}
V comisd(XMMREG a,MEM64 b){x86(260,a,b);}
V comisd(XMMREG a,XMM64 b){x86(260,a,b);}
V comiss(XMMREG a,XMMREG b){x86(261,a,b);}
V comiss(XMMREG a,MEM32 b){x86(261,a,b);}
V comiss(XMMREG a,XMM32 b){x86(261,a,b);}
V cpuid(){x86(262);}
V cvtdq2pd(XMMREG a,XMMREG b){x86(263,a,b);}
V cvtdq2pd(XMMREG a,MEM64 b){x86(263,a,b);}
V cvtdq2pd(XMMREG a,XMM64 b){x86(263,a,b);}
V cvtdq2ps(XMMREG a,XMMREG b){x86(264,a,b);}
V cvtdq2ps(XMMREG a,MEM128 b){x86(264,a,b);}
V cvtdq2ps(XMMREG a,R_M128 b){x86(264,a,b);}
V cvtpd2dq(XMMREG a,XMMREG b){x86(265,a,b);}
V cvtpd2dq(XMMREG a,MEM128 b){x86(265,a,b);}
V cvtpd2dq(XMMREG a,R_M128 b){x86(265,a,b);}
V cvtpd2pi(MMREG a,XMMREG b){x86(266,a,b);}
V cvtpd2pi(MMREG a,MEM128 b){x86(266,a,b);}
V cvtpd2pi(MMREG a,R_M128 b){x86(266,a,b);}
V cvtpd2ps(XMMREG a,XMMREG b){x86(267,a,b);}
V cvtpd2ps(XMMREG a,MEM128 b){x86(267,a,b);}
V cvtpd2ps(XMMREG a,R_M128 b){x86(267,a,b);}
V cvtpi2pd(XMMREG a,MMREG b){x86(268,a,b);}
V cvtpi2pd(XMMREG a,MEM64 b){x86(268,a,b);}
V cvtpi2pd(XMMREG a,R_M64 b){x86(268,a,b);}
V cvtps2dq(XMMREG a,XMMREG b){x86(269,a,b);}
V cvtps2dq(XMMREG a,MEM128 b){x86(269,a,b);}
V cvtps2dq(XMMREG a,R_M128 b){x86(269,a,b);}
V cvtps2pd(XMMREG a,XMMREG b){x86(270,a,b);}
V cvtps2pd(XMMREG a,MEM64 b){x86(270,a,b);}
V cvtps2pd(XMMREG a,XMM64 b){x86(270,a,b);}
V cvtsd2si(EAX a,XMMREG b){x86(271,a,b);}
V cvtsd2si(EAX a,MEM64 b){x86(271,a,b);}
V cvtsd2si(EAX a,XMM64 b){x86(271,a,b);}
V cvtsd2si(ECX a,XMMREG b){x86(271,a,b);}
V cvtsd2si(ECX a,MEM64 b){x86(271,a,b);}
V cvtsd2si(ECX a,XMM64 b){x86(271,a,b);}
V cvtsd2si(REG32 a,XMMREG b){x86(271,a,b);}
V cvtsd2si(REG32 a,MEM64 b){x86(271,a,b);}
V cvtsd2si(REG32 a,XMM64 b){x86(271,a,b);}
V cvtsi2sd(XMMREG a,EAX b){x86(272,a,b);}
V cvtsi2sd(XMMREG a,ECX b){x86(272,a,b);}
V cvtsi2sd(XMMREG a,REG32 b){x86(272,a,b);}
V cvtsi2sd(XMMREG a,MEM32 b){x86(272,a,b);}
V cvtsi2sd(XMMREG a,R_M32 b){x86(272,a,b);}
V cvtss2sd(XMMREG a,XMMREG b){x86(273,a,b);}
V cvtss2sd(XMMREG a,MEM32 b){x86(273,a,b);}
V cvtss2sd(XMMREG a,XMM32 b){x86(273,a,b);}
V cvttpd2dq(XMMREG a,XMMREG b){x86(274,a,b);}
V cvttpd2dq(XMMREG a,MEM128 b){x86(274,a,b);}
V cvttpd2dq(XMMREG a,R_M128 b){x86(274,a,b);}
V cvttpd2pi(MMREG a,XMMREG b){x86(275,a,b);}
V cvttpd2pi(MMREG a,MEM128 b){x86(275,a,b);}
V cvttpd2pi(MMREG a,R_M128 b){x86(275,a,b);}
V cvttps2dq(XMMREG a,XMMREG b){x86(276,a,b);}
V cvttps2dq(XMMREG a,MEM128 b){x86(276,a,b);}
V cvttps2dq(XMMREG a,R_M128 b){x86(276,a,b);}
V cvttsd2si(EAX a,XMMREG b){x86(277,a,b);}
V cvttsd2si(EAX a,MEM64 b){x86(277,a,b);}
V cvttsd2si(EAX a,XMM64 b){x86(277,a,b);}
V cvttsd2si(ECX a,XMMREG b){x86(277,a,b);}
V cvttsd2si(ECX a,MEM64 b){x86(277,a,b);}
V cvttsd2si(ECX a,XMM64 b){x86(277,a,b);}
V cvttsd2si(REG32 a,XMMREG b){x86(277,a,b);}
V cvttsd2si(REG32 a,MEM64 b){x86(277,a,b);}
V cvttsd2si(REG32 a,XMM64 b){x86(277,a,b);}
V cvtpi2ps(XMMREG a,MMREG b){x86(278,a,b);}
V cvtpi2ps(XMMREG a,MEM64 b){x86(278,a,b);}
V cvtpi2ps(XMMREG a,R_M64 b){x86(278,a,b);}
V cvtps2pi(MMREG a,XMMREG b){x86(279,a,b);}
V cvtps2pi(MMREG a,MEM64 b){x86(279,a,b);}
V cvtps2pi(MMREG a,XMM64 b){x86(279,a,b);}
V cvttps2pi(MMREG a,XMMREG b){x86(280,a,b);}
V cvttps2pi(MMREG a,MEM64 b){x86(280,a,b);}
V cvttps2pi(MMREG a,XMM64 b){x86(280,a,b);}
V cvtsi2ss(XMMREG a,EAX b){x86(281,a,b);}
V cvtsi2ss(XMMREG a,ECX b){x86(281,a,b);}
V cvtsi2ss(XMMREG a,REG32 b){x86(281,a,b);}
V cvtsi2ss(XMMREG a,MEM32 b){x86(281,a,b);}
V cvtsi2ss(XMMREG a,R_M32 b){x86(281,a,b);}
V cvtss2si(EAX a,XMMREG b){x86(282,a,b);}
V cvtss2si(EAX a,MEM32 b){x86(282,a,b);}
V cvtss2si(EAX a,XMM32 b){x86(282,a,b);}
V cvtss2si(ECX a,XMMREG b){x86(282,a,b);}
V cvtss2si(ECX a,MEM32 b){x86(282,a,b);}
V cvtss2si(ECX a,XMM32 b){x86(282,a,b);}
V cvtss2si(REG32 a,XMMREG b){x86(282,a,b);}
V cvtss2si(REG32 a,MEM32 b){x86(282,a,b);}
V cvtss2si(REG32 a,XMM32 b){x86(282,a,b);}
V cvttss2si(EAX a,XMMREG b){x86(283,a,b);}
V cvttss2si(EAX a,MEM32 b){x86(283,a,b);}
V cvttss2si(EAX a,XMM32 b){x86(283,a,b);}
V cvttss2si(ECX a,XMMREG b){x86(283,a,b);}
V cvttss2si(ECX a,MEM32 b){x86(283,a,b);}
V cvttss2si(ECX a,XMM32 b){x86(283,a,b);}
V cvttss2si(REG32 a,XMMREG b){x86(283,a,b);}
V cvttss2si(REG32 a,MEM32 b){x86(283,a,b);}
V cvttss2si(REG32 a,XMM32 b){x86(283,a,b);}
V daa(){x86(284);}
V das(){x86(285);}
V dec(AX a){x86(286,a);}
V dec(DX a){x86(286,a);}
V dec(CX a){x86(286,a);}
V dec(REG16 a){x86(286,a);}
V dec(EAX a){x86(287,a);}
V dec(ECX a){x86(287,a);}
V dec(REG32 a){x86(287,a);}
V dec(AL a){x86(288,a);}
V dec(CL a){x86(288,a);}
V dec(REG8 a){x86(288,a);}
V dec(MEM8 a){x86(288,a);}
V dec(R_M8 a){x86(288,a);}
V dec(MEM16 a){x86(289,a);}
V dec(R_M16 a){x86(289,a);}
V dec(MEM32 a){x86(290,a);}
V dec(R_M32 a){x86(290,a);}
V lock_dec(MEM8 a){x86(291,a);}
V lock_dec(MEM16 a){x86(292,a);}
V lock_dec(MEM32 a){x86(293,a);}
V div(AL a){x86(294,a);}
V div(CL a){x86(294,a);}
V div(REG8 a){x86(294,a);}
V div(MEM8 a){x86(294,a);}
V div(R_M8 a){x86(294,a);}
V div(AX a){x86(295,a);}
V div(DX a){x86(295,a);}
V div(CX a){x86(295,a);}
V div(REG16 a){x86(295,a);}
V div(MEM16 a){x86(295,a);}
V div(R_M16 a){x86(295,a);}
V div(EAX a){x86(296,a);}
V div(ECX a){x86(296,a);}
V div(REG32 a){x86(296,a);}
V div(MEM32 a){x86(296,a);}
V div(R_M32 a){x86(296,a);}
V divpd(XMMREG a,XMMREG b){x86(297,a,b);}
V divpd(XMMREG a,MEM128 b){x86(297,a,b);}
V divpd(XMMREG a,R_M128 b){x86(297,a,b);}
V divps(XMMREG a,XMMREG b){x86(298,a,b);}
V divps(XMMREG a,MEM128 b){x86(298,a,b);}
V divps(XMMREG a,R_M128 b){x86(298,a,b);}
V divsd(XMMREG a,XMMREG b){x86(299,a,b);}
V divsd(XMMREG a,MEM64 b){x86(299,a,b);}
V divsd(XMMREG a,XMM64 b){x86(299,a,b);}
V divss(XMMREG a,XMMREG b){x86(300,a,b);}
V divss(XMMREG a,MEM32 b){x86(300,a,b);}
V divss(XMMREG a,XMM32 b){x86(300,a,b);}
V emms(){x86(301);}
V f2xm1(){x86(302);}
V fabs(){x86(303);}
V fadd(MEM32 a){x86(304,a);}
V fadd(MEM64 a){x86(305,a);}
V fadd(ST0 a){x86(306,a);}
V fadd(FPUREG a){x86(306,a);}
V fadd(ST0 a,ST0 b){x86(307,a,b);}
V fadd(ST0 a,FPUREG b){x86(307,a,b);}
V fadd(FPUREG a,ST0 b){x86(308,a,b);}
V faddp(){x86(309);}
V faddp(ST0 a){x86(310,a);}
V faddp(FPUREG a){x86(310,a);}
V faddp(ST0 a,ST0 b){x86(311,a,b);}
V faddp(FPUREG a,ST0 b){x86(311,a,b);}
V fchs(){x86(312);}
V fclex(){x86(313);}
V fnclex(){x86(314);}
V fcmovb(ST0 a){x86(315,a);}
V fcmovb(FPUREG a){x86(315,a);}
V fcmovb(ST0 a,ST0 b){x86(316,a,b);}
V fcmovb(ST0 a,FPUREG b){x86(316,a,b);}
V fcmovbe(ST0 a){x86(317,a);}
V fcmovbe(FPUREG a){x86(317,a);}
V fcmovbe(ST0 a,ST0 b){x86(318,a,b);}
V fcmovbe(ST0 a,FPUREG b){x86(318,a,b);}
V fcmove(ST0 a){x86(319,a);}
V fcmove(FPUREG a){x86(319,a);}
V fcmove(ST0 a,ST0 b){x86(320,a,b);}
V fcmove(ST0 a,FPUREG b){x86(320,a,b);}
V fcmovnb(ST0 a){x86(321,a);}
V fcmovnb(FPUREG a){x86(321,a);}
V fcmovnb(ST0 a,ST0 b){x86(322,a,b);}
V fcmovnb(ST0 a,FPUREG b){x86(322,a,b);}
V fcmovnbe(ST0 a){x86(323,a);}
V fcmovnbe(FPUREG a){x86(323,a);}
V fcmovnbe(ST0 a,ST0 b){x86(324,a,b);}
V fcmovnbe(ST0 a,FPUREG b){x86(324,a,b);}
V fcmovne(ST0 a){x86(325,a);}
V fcmovne(FPUREG a){x86(325,a);}
V fcmovne(ST0 a,ST0 b){x86(326,a,b);}
V fcmovne(ST0 a,FPUREG b){x86(326,a,b);}
V fcmovnu(ST0 a){x86(327,a);}
V fcmovnu(FPUREG a){x86(327,a);}
V fcmovnu(ST0 a,ST0 b){x86(328,a,b);}
V fcmovnu(ST0 a,FPUREG b){x86(328,a,b);}
V fcmovu(ST0 a){x86(329,a);}
V fcmovu(FPUREG a){x86(329,a);}
V fcmovu(ST0 a,ST0 b){x86(330,a,b);}
V fcmovu(ST0 a,FPUREG b){x86(330,a,b);}
V fcom(MEM32 a){x86(331,a);}
V fcom(MEM64 a){x86(332,a);}
V fcom(ST0 a){x86(333,a);}
V fcom(FPUREG a){x86(333,a);}
V fcom(ST0 a,ST0 b){x86(334,a,b);}
V fcom(ST0 a,FPUREG b){x86(334,a,b);}
V fcomp(MEM32 a){x86(335,a);}
V fcomp(MEM64 a){x86(336,a);}
V fcomp(ST0 a){x86(337,a);}
V fcomp(FPUREG a){x86(337,a);}
V fcomp(ST0 a,ST0 b){x86(338,a,b);}
V fcomp(ST0 a,FPUREG b){x86(338,a,b);}
V fcompp(){x86(339);}
V fcomi(ST0 a){x86(340,a);}
V fcomi(FPUREG a){x86(340,a);}
V fcomi(ST0 a,ST0 b){x86(341,a,b);}
V fcomi(ST0 a,FPUREG b){x86(341,a,b);}
V fcomip(ST0 a){x86(342,a);}
V fcomip(FPUREG a){x86(342,a);}
V fcomip(ST0 a,ST0 b){x86(343,a,b);}
V fcomip(ST0 a,FPUREG b){x86(343,a,b);}
V fcos(){x86(344);}
V fdecstp(){x86(345);}
V fdisi(){x86(346);}
V fndisi(){x86(347);}
V feni(){x86(348);}
V fneni(){x86(349);}
V fdiv(MEM32 a){x86(350,a);}
V fdiv(MEM64 a){x86(351,a);}
V fdiv(ST0 a){x86(352,a);}
V fdiv(FPUREG a){x86(352,a);}
V fdiv(ST0 a,ST0 b){x86(353,a,b);}
V fdiv(ST0 a,FPUREG b){x86(353,a,b);}
V fdiv(FPUREG a,ST0 b){x86(354,a,b);}
V fdivr(MEM32 a){x86(355,a);}
V fdivr(MEM64 a){x86(356,a);}
V fdivr(ST0 a){x86(357,a);}
V fdivr(FPUREG a){x86(357,a);}
V fdivr(ST0 a,ST0 b){x86(358,a,b);}
V fdivr(ST0 a,FPUREG b){x86(358,a,b);}
V fdivr(FPUREG a,ST0 b){x86(359,a,b);}
V fdivp(){x86(360);}
V fdivp(ST0 a){x86(361,a);}
V fdivp(FPUREG a){x86(361,a);}
V fdivp(ST0 a,ST0 b){x86(362,a,b);}
V fdivp(FPUREG a,ST0 b){x86(362,a,b);}
V fdivrp(){x86(363);}
V fdivrp(ST0 a){x86(364,a);}
V fdivrp(FPUREG a){x86(364,a);}
V fdivrp(ST0 a,ST0 b){x86(365,a,b);}
V fdivrp(FPUREG a,ST0 b){x86(365,a,b);}
V femms(){x86(366);}
V ffree(ST0 a){x86(367,a);}
V ffree(FPUREG a){x86(367,a);}
V fiadd(MEM16 a){x86(368,a);}
V fiadd(MEM32 a){x86(369,a);}
V ficom(MEM16 a){x86(370,a);}
V ficom(MEM32 a){x86(371,a);}
V ficomp(MEM16 a){x86(372,a);}
V ficomp(MEM32 a){x86(373,a);}
V fidiv(MEM16 a){x86(374,a);}
V fidiv(MEM32 a){x86(375,a);}
V fidivr(MEM16 a){x86(376,a);}
V fidivr(MEM32 a){x86(377,a);}
V fild(MEM16 a){x86(378,a);}
V fild(MEM32 a){x86(379,a);}
V fild(MEM64 a){x86(380,a);}
V fist(MEM16 a){x86(381,a);}
V fist(MEM32 a){x86(382,a);}
V fistp(MEM16 a){x86(383,a);}
V fistp(MEM32 a){x86(384,a);}
V fistp(MEM64 a){x86(385,a);}
V fimul(MEM16 a){x86(386,a);}
V fimul(MEM32 a){x86(387,a);}
V fincstp(){x86(388);}
V finit(){x86(389);}
V fninit(){x86(390);}
V fisub(MEM16 a){x86(391,a);}
V fisub(MEM32 a){x86(392,a);}
V fisubr(MEM16 a){x86(393,a);}
V fisubr(MEM32 a){x86(394,a);}
V fld(MEM32 a){x86(395,a);}
V fld(MEM64 a){x86(396,a);}
V fld(ST0 a){x86(397,a);}
V fld(FPUREG a){x86(397,a);}
V fld1(){x86(398);}
V fldl2e(){x86(399);}
V fldl2t(){x86(400);}
V fldlg2(){x86(401);}
V fldln2(){x86(402);}
V fldpi(){x86(403);}
V fldz(){x86(404);}
V fldcw(MEM16 a){x86(405,a);}
V fldenv(MEM8 a){x86(406,a);}
V fldenv(MEM16 a){x86(406,a);}
V fldenv(MEM32 a){x86(406,a);}
V fldenv(MEM64 a){x86(406,a);}
V fldenv(MEM128 a){x86(406,a);}
V fmul(MEM32 a){x86(407,a);}
V fmul(MEM64 a){x86(408,a);}
V fmul(){x86(409);}
V fmul(ST0 a){x86(410,a);}
V fmul(FPUREG a){x86(410,a);}
V fmul(ST0 a,ST0 b){x86(411,a,b);}
V fmul(ST0 a,FPUREG b){x86(411,a,b);}
V fmul(FPUREG a,ST0 b){x86(412,a,b);}
V fmulp(ST0 a){x86(413,a);}
V fmulp(FPUREG a){x86(413,a);}
V fmulp(ST0 a,ST0 b){x86(414,a,b);}
V fmulp(FPUREG a,ST0 b){x86(414,a,b);}
V fmulp(){x86(415);}
V fnop(){x86(416);}
V fpatan(){x86(417);}
V fptan(){x86(418);}
V fprem(){x86(419);}
V fprem1(){x86(420);}
V frndint(){x86(421);}
V fsave(MEM8 a){x86(422,a);}
V fsave(MEM16 a){x86(422,a);}
V fsave(MEM32 a){x86(422,a);}
V fsave(MEM64 a){x86(422,a);}
V fsave(MEM128 a){x86(422,a);}
V fnsave(MEM8 a){x86(423,a);}
V fnsave(MEM16 a){x86(423,a);}
V fnsave(MEM32 a){x86(423,a);}
V fnsave(MEM64 a){x86(423,a);}
V fnsave(MEM128 a){x86(423,a);}
V frstor(MEM8 a){x86(424,a);}
V frstor(MEM16 a){x86(424,a);}
V frstor(MEM32 a){x86(424,a);}
V frstor(MEM64 a){x86(424,a);}
V frstor(MEM128 a){x86(424,a);}
V fscale(){x86(425);}
V fsetpm(){x86(426);}
V fsin(){x86(427);}
V fsincos(){x86(428);}
V fsqrt(){x86(429);}
V fst(MEM32 a){x86(430,a);}
V fst(MEM64 a){x86(431,a);}
V fst(ST0 a){x86(432,a);}
V fst(FPUREG a){x86(432,a);}
V fstp(MEM32 a){x86(433,a);}
V fstp(MEM64 a){x86(434,a);}
V fstp(ST0 a){x86(435,a);}
V fstp(FPUREG a){x86(435,a);}
V fstcw(MEM16 a){x86(436,a);}
V fnstcw(MEM16 a){x86(437,a);}
V fstenv(MEM8 a){x86(438,a);}
V fstenv(MEM16 a){x86(438,a);}
V fstenv(MEM32 a){x86(438,a);}
V fstenv(MEM64 a){x86(438,a);}
V fstenv(MEM128 a){x86(438,a);}
V fnstenv(MEM8 a){x86(439,a);}
V fnstenv(MEM16 a){x86(439,a);}
V fnstenv(MEM32 a){x86(439,a);}
V fnstenv(MEM64 a){x86(439,a);}
V fnstenv(MEM128 a){x86(439,a);}
V fstsw(MEM16 a){x86(440,a);}
V fstsw(AX a){x86(441,a);}
V fnstsw(MEM16 a){x86(442,a);}
V fnstsw(AX a){x86(443,a);}
V fsub(MEM32 a){x86(444,a);}
V fsub(MEM64 a){x86(445,a);}
V fsub(ST0 a){x86(446,a);}
V fsub(FPUREG a){x86(446,a);}
V fsub(ST0 a,ST0 b){x86(447,a,b);}
V fsub(ST0 a,FPUREG b){x86(447,a,b);}
V fsub(FPUREG a,ST0 b){x86(448,a,b);}
V fsubr(MEM32 a){x86(449,a);}
V fsubr(MEM64 a){x86(450,a);}
V fsubr(ST0 a){x86(451,a);}
V fsubr(FPUREG a){x86(451,a);}
V fsubr(ST0 a,ST0 b){x86(452,a,b);}
V fsubr(ST0 a,FPUREG b){x86(452,a,b);}
V fsubr(FPUREG a,ST0 b){x86(453,a,b);}
V fsubp(){x86(454);}
V fsubp(ST0 a){x86(455,a);}
V fsubp(FPUREG a){x86(455,a);}
V fsubp(ST0 a,ST0 b){x86(456,a,b);}
V fsubp(FPUREG a,ST0 b){x86(456,a,b);}
V fsubrp(){x86(457);}
V fsubrp(ST0 a){x86(458,a);}
V fsubrp(FPUREG a){x86(458,a);}
V fsubrp(ST0 a,ST0 b){x86(459,a,b);}
V fsubrp(FPUREG a,ST0 b){x86(459,a,b);}
V ftst(){x86(460);}
V fucom(ST0 a){x86(461,a);}
V fucom(FPUREG a){x86(461,a);}
V fucom(ST0 a,ST0 b){x86(462,a,b);}
V fucom(ST0 a,FPUREG b){x86(462,a,b);}
V fucomp(ST0 a){x86(463,a);}
V fucomp(FPUREG a){x86(463,a);}
V fucomp(ST0 a,ST0 b){x86(464,a,b);}
V fucomp(ST0 a,FPUREG b){x86(464,a,b);}
V fucompp(){x86(465);}
V fucomi(ST0 a){x86(466,a);}
V fucomi(FPUREG a){x86(466,a);}
V fucomi(ST0 a,ST0 b){x86(467,a,b);}
V fucomi(ST0 a,FPUREG b){x86(467,a,b);}
V fucomip(ST0 a){x86(468,a);}
V fucomip(FPUREG a){x86(468,a);}
V fucomip(ST0 a,ST0 b){x86(469,a,b);}
V fucomip(ST0 a,FPUREG b){x86(469,a,b);}
V fwait(){x86(470);}
V fxam(){x86(471);}
V fxch(){x86(472);}
V fxch(ST0 a){x86(473,a);}
V fxch(FPUREG a){x86(473,a);}
V fxch(ST0 a,ST0 b){x86(474,a,b);}
V fxch(FPUREG a,ST0 b){x86(474,a,b);}
V fxch(ST0 a,FPUREG b){x86(475,a,b);}
V fxtract(){x86(476);}
V fyl2x(){x86(477);}
V fyl2xp1(){x86(478);}
V hlt(){x86(479);}
V idiv(AL a){x86(480,a);}
V idiv(CL a){x86(480,a);}
V idiv(REG8 a){x86(480,a);}
V idiv(MEM8 a){x86(480,a);}
V idiv(R_M8 a){x86(480,a);}
V idiv(AX a){x86(481,a);}
V idiv(DX a){x86(481,a);}
V idiv(CX a){x86(481,a);}
V idiv(REG16 a){x86(481,a);}
V idiv(MEM16 a){x86(481,a);}
V idiv(R_M16 a){x86(481,a);}
V idiv(EAX a){x86(482,a);}
V idiv(ECX a){x86(482,a);}
V idiv(REG32 a){x86(482,a);}
V idiv(MEM32 a){x86(482,a);}
V idiv(R_M32 a){x86(482,a);}
V imul(AL a){x86(483,a);}
V imul(CL a){x86(483,a);}
V imul(REG8 a){x86(483,a);}
V imul(MEM8 a){x86(483,a);}
V imul(R_M8 a){x86(483,a);}
V imul(AX a){x86(484,a);}
V imul(DX a){x86(484,a);}
V imul(CX a){x86(484,a);}
V imul(REG16 a){x86(484,a);}
V imul(MEM16 a){x86(484,a);}
V imul(R_M16 a){x86(484,a);}
V imul(EAX a){x86(485,a);}
V imul(ECX a){x86(485,a);}
V imul(REG32 a){x86(485,a);}
V imul(MEM32 a){x86(485,a);}
V imul(R_M32 a){x86(485,a);}
V imul(AX a,AX b){x86(486,a,b);}
V imul(AX a,DX b){x86(486,a,b);}
V imul(AX a,CX b){x86(486,a,b);}
V imul(AX a,REG16 b){x86(486,a,b);}
V imul(AX a,MEM16 b){x86(486,a,b);}
V imul(AX a,R_M16 b){x86(486,a,b);}
V imul(DX a,AX b){x86(486,a,b);}
V imul(DX a,DX b){x86(486,a,b);}
V imul(DX a,CX b){x86(486,a,b);}
V imul(DX a,REG16 b){x86(486,a,b);}
V imul(DX a,MEM16 b){x86(486,a,b);}
V imul(DX a,R_M16 b){x86(486,a,b);}
V imul(CX a,AX b){x86(486,a,b);}
V imul(CX a,DX b){x86(486,a,b);}
V imul(CX a,CX b){x86(486,a,b);}
V imul(CX a,REG16 b){x86(486,a,b);}
V imul(CX a,MEM16 b){x86(486,a,b);}
V imul(CX a,R_M16 b){x86(486,a,b);}
V imul(REG16 a,AX b){x86(486,a,b);}
V imul(REG16 a,DX b){x86(486,a,b);}
V imul(REG16 a,CX b){x86(486,a,b);}
V imul(REG16 a,REG16 b){x86(486,a,b);}
V imul(REG16 a,MEM16 b){x86(486,a,b);}
V imul(REG16 a,R_M16 b){x86(486,a,b);}
V imul(EAX a,EAX b){x86(487,a,b);}
V imul(EAX a,ECX b){x86(487,a,b);}
V imul(EAX a,REG32 b){x86(487,a,b);}
V imul(EAX a,MEM32 b){x86(487,a,b);}
V imul(EAX a,R_M32 b){x86(487,a,b);}
V imul(ECX a,EAX b){x86(487,a,b);}
V imul(ECX a,ECX b){x86(487,a,b);}
V imul(ECX a,REG32 b){x86(487,a,b);}
V imul(ECX a,MEM32 b){x86(487,a,b);}
V imul(ECX a,R_M32 b){x86(487,a,b);}
V imul(REG32 a,EAX b){x86(487,a,b);}
V imul(REG32 a,ECX b){x86(487,a,b);}
V imul(REG32 a,REG32 b){x86(487,a,b);}
V imul(REG32 a,MEM32 b){x86(487,a,b);}
V imul(REG32 a,R_M32 b){x86(487,a,b);}
V imul(AX a,char b){x86(489,a,(IMM)b);}
V imul(AX a,short b){x86(489,a,(IMM)b);}
V imul(DX a,char b){x86(489,a,(IMM)b);}
V imul(DX a,short b){x86(489,a,(IMM)b);}
V imul(CX a,char b){x86(489,a,(IMM)b);}
V imul(CX a,short b){x86(489,a,(IMM)b);}
V imul(REG16 a,char b){x86(489,a,(IMM)b);}
V imul(REG16 a,short b){x86(489,a,(IMM)b);}
V imul(EAX a,int b){x86(491,a,(IMM)b);}
V imul(EAX a,char b){x86(491,a,(IMM)b);}
V imul(EAX a,short b){x86(491,a,(IMM)b);}
V imul(EAX a,REF b){x86(491,a,b);}
V imul(ECX a,int b){x86(491,a,(IMM)b);}
V imul(ECX a,char b){x86(491,a,(IMM)b);}
V imul(ECX a,short b){x86(491,a,(IMM)b);}
V imul(ECX a,REF b){x86(491,a,b);}
V imul(REG32 a,int b){x86(491,a,(IMM)b);}
V imul(REG32 a,char b){x86(491,a,(IMM)b);}
V imul(REG32 a,short b){x86(491,a,(IMM)b);}
V imul(REG32 a,REF b){x86(491,a,b);}
V imul(AX a,AX b,char c){x86(492,a,b,(IMM)c);}
V imul(AX a,DX b,char c){x86(492,a,b,(IMM)c);}
V imul(AX a,CX b,char c){x86(492,a,b,(IMM)c);}
V imul(AX a,REG16 b,char c){x86(492,a,b,(IMM)c);}
V imul(AX a,MEM16 b,char c){x86(492,a,b,(IMM)c);}
V imul(AX a,R_M16 b,char c){x86(492,a,b,(IMM)c);}
V imul(DX a,AX b,char c){x86(492,a,b,(IMM)c);}
V imul(DX a,DX b,char c){x86(492,a,b,(IMM)c);}
V imul(DX a,CX b,char c){x86(492,a,b,(IMM)c);}
V imul(DX a,REG16 b,char c){x86(492,a,b,(IMM)c);}
V imul(DX a,MEM16 b,char c){x86(492,a,b,(IMM)c);}
V imul(DX a,R_M16 b,char c){x86(492,a,b,(IMM)c);}
V imul(CX a,AX b,char c){x86(492,a,b,(IMM)c);}
V imul(CX a,DX b,char c){x86(492,a,b,(IMM)c);}
V imul(CX a,CX b,char c){x86(492,a,b,(IMM)c);}
V imul(CX a,REG16 b,char c){x86(492,a,b,(IMM)c);}
V imul(CX a,MEM16 b,char c){x86(492,a,b,(IMM)c);}
V imul(CX a,R_M16 b,char c){x86(492,a,b,(IMM)c);}
V imul(REG16 a,AX b,char c){x86(492,a,b,(IMM)c);}
V imul(REG16 a,DX b,char c){x86(492,a,b,(IMM)c);}
V imul(REG16 a,CX b,char c){x86(492,a,b,(IMM)c);}
V imul(REG16 a,REG16 b,char c){x86(492,a,b,(IMM)c);}
V imul(REG16 a,MEM16 b,char c){x86(492,a,b,(IMM)c);}
V imul(REG16 a,R_M16 b,char c){x86(492,a,b,(IMM)c);}
V imul(AX a,AX b,short c){x86(493,a,b,(IMM)c);}
V imul(AX a,DX b,short c){x86(493,a,b,(IMM)c);}
V imul(AX a,CX b,short c){x86(493,a,b,(IMM)c);}
V imul(AX a,REG16 b,short c){x86(493,a,b,(IMM)c);}
V imul(AX a,MEM16 b,short c){x86(493,a,b,(IMM)c);}
V imul(AX a,R_M16 b,short c){x86(493,a,b,(IMM)c);}
V imul(DX a,AX b,short c){x86(493,a,b,(IMM)c);}
V imul(DX a,DX b,short c){x86(493,a,b,(IMM)c);}
V imul(DX a,CX b,short c){x86(493,a,b,(IMM)c);}
V imul(DX a,REG16 b,short c){x86(493,a,b,(IMM)c);}
V imul(DX a,MEM16 b,short c){x86(493,a,b,(IMM)c);}
V imul(DX a,R_M16 b,short c){x86(493,a,b,(IMM)c);}
V imul(CX a,AX b,short c){x86(493,a,b,(IMM)c);}
V imul(CX a,DX b,short c){x86(493,a,b,(IMM)c);}
V imul(CX a,CX b,short c){x86(493,a,b,(IMM)c);}
V imul(CX a,REG16 b,short c){x86(493,a,b,(IMM)c);}
V imul(CX a,MEM16 b,short c){x86(493,a,b,(IMM)c);}
V imul(CX a,R_M16 b,short c){x86(493,a,b,(IMM)c);}
V imul(REG16 a,AX b,short c){x86(493,a,b,(IMM)c);}
V imul(REG16 a,DX b,short c){x86(493,a,b,(IMM)c);}
V imul(REG16 a,CX b,short c){x86(493,a,b,(IMM)c);}
V imul(REG16 a,REG16 b,short c){x86(493,a,b,(IMM)c);}
V imul(REG16 a,MEM16 b,short c){x86(493,a,b,(IMM)c);}
V imul(REG16 a,R_M16 b,short c){x86(493,a,b,(IMM)c);}
V imul(EAX a,EAX b,char c){x86(494,a,b,(IMM)c);}
V imul(EAX a,ECX b,char c){x86(494,a,b,(IMM)c);}
V imul(EAX a,REG32 b,char c){x86(494,a,b,(IMM)c);}
V imul(EAX a,MEM32 b,char c){x86(494,a,b,(IMM)c);}
V imul(EAX a,R_M32 b,char c){x86(494,a,b,(IMM)c);}
V imul(ECX a,EAX b,char c){x86(494,a,b,(IMM)c);}
V imul(ECX a,ECX b,char c){x86(494,a,b,(IMM)c);}
V imul(ECX a,REG32 b,char c){x86(494,a,b,(IMM)c);}
V imul(ECX a,MEM32 b,char c){x86(494,a,b,(IMM)c);}
V imul(ECX a,R_M32 b,char c){x86(494,a,b,(IMM)c);}
V imul(REG32 a,EAX b,char c){x86(494,a,b,(IMM)c);}
V imul(REG32 a,ECX b,char c){x86(494,a,b,(IMM)c);}
V imul(REG32 a,REG32 b,char c){x86(494,a,b,(IMM)c);}
V imul(REG32 a,MEM32 b,char c){x86(494,a,b,(IMM)c);}
V imul(REG32 a,R_M32 b,char c){x86(494,a,b,(IMM)c);}
V imul(EAX a,EAX b,int c){x86(495,a,b,(IMM)c);}
V imul(EAX a,EAX b,short c){x86(495,a,b,(IMM)c);}
V imul(EAX a,EAX b,REF c){x86(495,a,b,c);}
V imul(EAX a,ECX b,int c){x86(495,a,b,(IMM)c);}
V imul(EAX a,ECX b,short c){x86(495,a,b,(IMM)c);}
V imul(EAX a,ECX b,REF c){x86(495,a,b,c);}
V imul(EAX a,REG32 b,int c){x86(495,a,b,(IMM)c);}
V imul(EAX a,REG32 b,short c){x86(495,a,b,(IMM)c);}
V imul(EAX a,REG32 b,REF c){x86(495,a,b,c);}
V imul(EAX a,MEM32 b,int c){x86(495,a,b,(IMM)c);}
V imul(EAX a,MEM32 b,short c){x86(495,a,b,(IMM)c);}
V imul(EAX a,MEM32 b,REF c){x86(495,a,b,c);}
V imul(EAX a,R_M32 b,int c){x86(495,a,b,(IMM)c);}
V imul(EAX a,R_M32 b,short c){x86(495,a,b,(IMM)c);}
V imul(EAX a,R_M32 b,REF c){x86(495,a,b,c);}
V imul(ECX a,EAX b,int c){x86(495,a,b,(IMM)c);}
V imul(ECX a,EAX b,short c){x86(495,a,b,(IMM)c);}
V imul(ECX a,EAX b,REF c){x86(495,a,b,c);}
V imul(ECX a,ECX b,int c){x86(495,a,b,(IMM)c);}
V imul(ECX a,ECX b,short c){x86(495,a,b,(IMM)c);}
V imul(ECX a,ECX b,REF c){x86(495,a,b,c);}
V imul(ECX a,REG32 b,int c){x86(495,a,b,(IMM)c);}
V imul(ECX a,REG32 b,short c){x86(495,a,b,(IMM)c);}
V imul(ECX a,REG32 b,REF c){x86(495,a,b,c);}
V imul(ECX a,MEM32 b,int c){x86(495,a,b,(IMM)c);}
V imul(ECX a,MEM32 b,short c){x86(495,a,b,(IMM)c);}
V imul(ECX a,MEM32 b,REF c){x86(495,a,b,c);}
V imul(ECX a,R_M32 b,int c){x86(495,a,b,(IMM)c);}
V imul(ECX a,R_M32 b,short c){x86(495,a,b,(IMM)c);}
V imul(ECX a,R_M32 b,REF c){x86(495,a,b,c);}
V imul(REG32 a,EAX b,int c){x86(495,a,b,(IMM)c);}
V imul(REG32 a,EAX b,short c){x86(495,a,b,(IMM)c);}
V imul(REG32 a,EAX b,REF c){x86(495,a,b,c);}
V imul(REG32 a,ECX b,int c){x86(495,a,b,(IMM)c);}
V imul(REG32 a,ECX b,short c){x86(495,a,b,(IMM)c);}
V imul(REG32 a,ECX b,REF c){x86(495,a,b,c);}
V imul(REG32 a,REG32 b,int c){x86(495,a,b,(IMM)c);}
V imul(REG32 a,REG32 b,short c){x86(495,a,b,(IMM)c);}
V imul(REG32 a,REG32 b,REF c){x86(495,a,b,c);}
V imul(REG32 a,MEM32 b,int c){x86(495,a,b,(IMM)c);}
V imul(REG32 a,MEM32 b,short c){x86(495,a,b,(IMM)c);}
V imul(REG32 a,MEM32 b,REF c){x86(495,a,b,c);}
V imul(REG32 a,R_M32 b,int c){x86(495,a,b,(IMM)c);}
V imul(REG32 a,R_M32 b,short c){x86(495,a,b,(IMM)c);}
V imul(REG32 a,R_M32 b,REF c){x86(495,a,b,c);}
V in(AL a,char b){x86(496,a,(IMM)b);}
V in(AL a,DX b){x86(499,a,b);}
V in(AX a,DX b){x86(500,a,b);}
V in(EAX a,DX b){x86(501,a,b);}
V inc(AX a){x86(502,a);}
V inc(DX a){x86(502,a);}
V inc(CX a){x86(502,a);}
V inc(REG16 a){x86(502,a);}
V inc(EAX a){x86(503,a);}
V inc(ECX a){x86(503,a);}
V inc(REG32 a){x86(503,a);}
V inc(AL a){x86(504,a);}
V inc(CL a){x86(504,a);}
V inc(REG8 a){x86(504,a);}
V inc(MEM8 a){x86(504,a);}
V inc(R_M8 a){x86(504,a);}
V inc(MEM16 a){x86(505,a);}
V inc(R_M16 a){x86(505,a);}
V inc(MEM32 a){x86(506,a);}
V inc(R_M32 a){x86(506,a);}
V lock_inc(MEM8 a){x86(507,a);}
V lock_inc(MEM16 a){x86(508,a);}
V lock_inc(MEM32 a){x86(509,a);}
V insb(){x86(510);}
V insw(){x86(511);}
V insd(){x86(512);}
V rep_insb(){x86(513);}
V rep_insw(){x86(514);}
V rep_insd(){x86(515);}
V int3(){x86(516);}
V int03(){x86(517);}
V into(){x86(518);}
V jcxz(char a){x86(519,(IMM)a);}
V jecxz(char a){x86(520,(IMM)a);}
V jmp(int a){x86(521,(IMM)a);}
V jmp(char a){x86(521,(IMM)a);}
V jmp(short a){x86(521,(IMM)a);}
V jmp(REF a){x86(521,a);}
V jmp(MEM8 a){x86(523,a);}
V jmp(MEM16 a){x86(523,a);}
V jmp(MEM32 a){x86(523,a);}
V jmp(MEM64 a){x86(523,a);}
V jmp(MEM128 a){x86(523,a);}
V jmp(AX a){x86(524,a);}
V jmp(DX a){x86(524,a);}
V jmp(CX a){x86(524,a);}
V jmp(REG16 a){x86(524,a);}
V jmp(R_M16 a){x86(524,a);}
V jmp(EAX a){x86(525,a);}
V jmp(ECX a){x86(525,a);}
V jmp(REG32 a){x86(525,a);}
V jmp(R_M32 a){x86(525,a);}
V jo(char a){x86(526,(IMM)a);}
V jno(char a){x86(527,(IMM)a);}
V jb(char a){x86(528,(IMM)a);}
V jc(char a){x86(529,(IMM)a);}
V jnae(char a){x86(530,(IMM)a);}
V jae(char a){x86(531,(IMM)a);}
V jnb(char a){x86(532,(IMM)a);}
V jnc(char a){x86(533,(IMM)a);}
V je(char a){x86(534,(IMM)a);}
V jz(char a){x86(535,(IMM)a);}
V jne(char a){x86(536,(IMM)a);}
V jnz(char a){x86(537,(IMM)a);}
V jbe(char a){x86(538,(IMM)a);}
V jna(char a){x86(539,(IMM)a);}
V ja(char a){x86(540,(IMM)a);}
V jnbe(char a){x86(541,(IMM)a);}
V js(char a){x86(542,(IMM)a);}
V jns(char a){x86(543,(IMM)a);}
V jp(char a){x86(544,(IMM)a);}
V jpe(char a){x86(545,(IMM)a);}
V jnp(char a){x86(546,(IMM)a);}
V jpo(char a){x86(547,(IMM)a);}
V jl(char a){x86(548,(IMM)a);}
V jnge(char a){x86(549,(IMM)a);}
V jge(char a){x86(550,(IMM)a);}
V jnl(char a){x86(551,(IMM)a);}
V jle(char a){x86(552,(IMM)a);}
V jng(char a){x86(553,(IMM)a);}
V jg(char a){x86(554,(IMM)a);}
V jnle(char a){x86(555,(IMM)a);}
V jo(int a){x86(556,(IMM)a);}
V jo(short a){x86(556,(IMM)a);}
V jo(REF a){x86(556,a);}
V jno(int a){x86(557,(IMM)a);}
V jno(short a){x86(557,(IMM)a);}
V jno(REF a){x86(557,a);}
V jb(int a){x86(558,(IMM)a);}
V jb(short a){x86(558,(IMM)a);}
V jb(REF a){x86(558,a);}
V jc(int a){x86(559,(IMM)a);}
V jc(short a){x86(559,(IMM)a);}
V jc(REF a){x86(559,a);}
V jnae(int a){x86(560,(IMM)a);}
V jnae(short a){x86(560,(IMM)a);}
V jnae(REF a){x86(560,a);}
V jae(int a){x86(561,(IMM)a);}
V jae(short a){x86(561,(IMM)a);}
V jae(REF a){x86(561,a);}
V jnb(int a){x86(562,(IMM)a);}
V jnb(short a){x86(562,(IMM)a);}
V jnb(REF a){x86(562,a);}
V jnc(int a){x86(563,(IMM)a);}
V jnc(short a){x86(563,(IMM)a);}
V jnc(REF a){x86(563,a);}
V je(int a){x86(564,(IMM)a);}
V je(short a){x86(564,(IMM)a);}
V je(REF a){x86(564,a);}
V jz(int a){x86(565,(IMM)a);}
V jz(short a){x86(565,(IMM)a);}
V jz(REF a){x86(565,a);}
V jne(int a){x86(566,(IMM)a);}
V jne(short a){x86(566,(IMM)a);}
V jne(REF a){x86(566,a);}
V jnz(int a){x86(567,(IMM)a);}
V jnz(short a){x86(567,(IMM)a);}
V jnz(REF a){x86(567,a);}
V jbe(int a){x86(568,(IMM)a);}
V jbe(short a){x86(568,(IMM)a);}
V jbe(REF a){x86(568,a);}
V jna(int a){x86(569,(IMM)a);}
V jna(short a){x86(569,(IMM)a);}
V jna(REF a){x86(569,a);}
V ja(int a){x86(570,(IMM)a);}
V ja(short a){x86(570,(IMM)a);}
V ja(REF a){x86(570,a);}
V jnbe(int a){x86(571,(IMM)a);}
V jnbe(short a){x86(571,(IMM)a);}
V jnbe(REF a){x86(571,a);}
V js(int a){x86(572,(IMM)a);}
V js(short a){x86(572,(IMM)a);}
V js(REF a){x86(572,a);}
V jns(int a){x86(573,(IMM)a);}
V jns(short a){x86(573,(IMM)a);}
V jns(REF a){x86(573,a);}
V jp(int a){x86(574,(IMM)a);}
V jp(short a){x86(574,(IMM)a);}
V jp(REF a){x86(574,a);}
V jpe(int a){x86(575,(IMM)a);}
V jpe(short a){x86(575,(IMM)a);}
V jpe(REF a){x86(575,a);}
V jnp(int a){x86(576,(IMM)a);}
V jnp(short a){x86(576,(IMM)a);}
V jnp(REF a){x86(576,a);}
V jpo(int a){x86(577,(IMM)a);}
V jpo(short a){x86(577,(IMM)a);}
V jpo(REF a){x86(577,a);}
V jl(int a){x86(578,(IMM)a);}
V jl(short a){x86(578,(IMM)a);}
V jl(REF a){x86(578,a);}
V jnge(int a){x86(579,(IMM)a);}
V jnge(short a){x86(579,(IMM)a);}
V jnge(REF a){x86(579,a);}
V jge(int a){x86(580,(IMM)a);}
V jge(short a){x86(580,(IMM)a);}
V jge(REF a){x86(580,a);}
V jnl(int a){x86(581,(IMM)a);}
V jnl(short a){x86(581,(IMM)a);}
V jnl(REF a){x86(581,a);}
V jle(int a){x86(582,(IMM)a);}
V jle(short a){x86(582,(IMM)a);}
V jle(REF a){x86(582,a);}
V jng(int a){x86(583,(IMM)a);}
V jng(short a){x86(583,(IMM)a);}
V jng(REF a){x86(583,a);}
V jg(int a){x86(584,(IMM)a);}
V jg(short a){x86(584,(IMM)a);}
V jg(REF a){x86(584,a);}
V jnle(int a){x86(585,(IMM)a);}
V jnle(short a){x86(585,(IMM)a);}
V jnle(REF a){x86(585,a);}
V lahf(){x86(586);}
V lds(AX a,MEM8 b){x86(587,a,b);}
V lds(AX a,MEM16 b){x86(587,a,b);}
V lds(AX a,MEM32 b){x86(587,a,b);}
V lds(AX a,MEM64 b){x86(587,a,b);}
V lds(AX a,MEM128 b){x86(587,a,b);}
V lds(DX a,MEM8 b){x86(587,a,b);}
V lds(DX a,MEM16 b){x86(587,a,b);}
V lds(DX a,MEM32 b){x86(587,a,b);}
V lds(DX a,MEM64 b){x86(587,a,b);}
V lds(DX a,MEM128 b){x86(587,a,b);}
V lds(CX a,MEM8 b){x86(587,a,b);}
V lds(CX a,MEM16 b){x86(587,a,b);}
V lds(CX a,MEM32 b){x86(587,a,b);}
V lds(CX a,MEM64 b){x86(587,a,b);}
V lds(CX a,MEM128 b){x86(587,a,b);}
V lds(REG16 a,MEM8 b){x86(587,a,b);}
V lds(REG16 a,MEM16 b){x86(587,a,b);}
V lds(REG16 a,MEM32 b){x86(587,a,b);}
V lds(REG16 a,MEM64 b){x86(587,a,b);}
V lds(REG16 a,MEM128 b){x86(587,a,b);}
V lds(EAX a,MEM8 b){x86(588,a,b);}
V lds(EAX a,MEM16 b){x86(588,a,b);}
V lds(EAX a,MEM32 b){x86(588,a,b);}
V lds(EAX a,MEM64 b){x86(588,a,b);}
V lds(EAX a,MEM128 b){x86(588,a,b);}
V lds(ECX a,MEM8 b){x86(588,a,b);}
V lds(ECX a,MEM16 b){x86(588,a,b);}
V lds(ECX a,MEM32 b){x86(588,a,b);}
V lds(ECX a,MEM64 b){x86(588,a,b);}
V lds(ECX a,MEM128 b){x86(588,a,b);}
V lds(REG32 a,MEM8 b){x86(588,a,b);}
V lds(REG32 a,MEM16 b){x86(588,a,b);}
V lds(REG32 a,MEM32 b){x86(588,a,b);}
V lds(REG32 a,MEM64 b){x86(588,a,b);}
V lds(REG32 a,MEM128 b){x86(588,a,b);}
V les(AX a,MEM8 b){x86(589,a,b);}
V les(AX a,MEM16 b){x86(589,a,b);}
V les(AX a,MEM32 b){x86(589,a,b);}
V les(AX a,MEM64 b){x86(589,a,b);}
V les(AX a,MEM128 b){x86(589,a,b);}
V les(DX a,MEM8 b){x86(589,a,b);}
V les(DX a,MEM16 b){x86(589,a,b);}
V les(DX a,MEM32 b){x86(589,a,b);}
V les(DX a,MEM64 b){x86(589,a,b);}
V les(DX a,MEM128 b){x86(589,a,b);}
V les(CX a,MEM8 b){x86(589,a,b);}
V les(CX a,MEM16 b){x86(589,a,b);}
V les(CX a,MEM32 b){x86(589,a,b);}
V les(CX a,MEM64 b){x86(589,a,b);}
V les(CX a,MEM128 b){x86(589,a,b);}
V les(REG16 a,MEM8 b){x86(589,a,b);}
V les(REG16 a,MEM16 b){x86(589,a,b);}
V les(REG16 a,MEM32 b){x86(589,a,b);}
V les(REG16 a,MEM64 b){x86(589,a,b);}
V les(REG16 a,MEM128 b){x86(589,a,b);}
V les(EAX a,MEM8 b){x86(590,a,b);}
V les(EAX a,MEM16 b){x86(590,a,b);}
V les(EAX a,MEM32 b){x86(590,a,b);}
V les(EAX a,MEM64 b){x86(590,a,b);}
V les(EAX a,MEM128 b){x86(590,a,b);}
V les(ECX a,MEM8 b){x86(590,a,b);}
V les(ECX a,MEM16 b){x86(590,a,b);}
V les(ECX a,MEM32 b){x86(590,a,b);}
V les(ECX a,MEM64 b){x86(590,a,b);}
V les(ECX a,MEM128 b){x86(590,a,b);}
V les(REG32 a,MEM8 b){x86(590,a,b);}
V les(REG32 a,MEM16 b){x86(590,a,b);}
V les(REG32 a,MEM32 b){x86(590,a,b);}
V les(REG32 a,MEM64 b){x86(590,a,b);}
V les(REG32 a,MEM128 b){x86(590,a,b);}
V lfs(AX a,MEM8 b){x86(591,a,b);}
V lfs(AX a,MEM16 b){x86(591,a,b);}
V lfs(AX a,MEM32 b){x86(591,a,b);}
V lfs(AX a,MEM64 b){x86(591,a,b);}
V lfs(AX a,MEM128 b){x86(591,a,b);}
V lfs(DX a,MEM8 b){x86(591,a,b);}
V lfs(DX a,MEM16 b){x86(591,a,b);}
V lfs(DX a,MEM32 b){x86(591,a,b);}
V lfs(DX a,MEM64 b){x86(591,a,b);}
V lfs(DX a,MEM128 b){x86(591,a,b);}
V lfs(CX a,MEM8 b){x86(591,a,b);}
V lfs(CX a,MEM16 b){x86(591,a,b);}
V lfs(CX a,MEM32 b){x86(591,a,b);}
V lfs(CX a,MEM64 b){x86(591,a,b);}
V lfs(CX a,MEM128 b){x86(591,a,b);}
V lfs(REG16 a,MEM8 b){x86(591,a,b);}
V lfs(REG16 a,MEM16 b){x86(591,a,b);}
V lfs(REG16 a,MEM32 b){x86(591,a,b);}
V lfs(REG16 a,MEM64 b){x86(591,a,b);}
V lfs(REG16 a,MEM128 b){x86(591,a,b);}
V lfs(EAX a,MEM8 b){x86(592,a,b);}
V lfs(EAX a,MEM16 b){x86(592,a,b);}
V lfs(EAX a,MEM32 b){x86(592,a,b);}
V lfs(EAX a,MEM64 b){x86(592,a,b);}
V lfs(EAX a,MEM128 b){x86(592,a,b);}
V lfs(ECX a,MEM8 b){x86(592,a,b);}
V lfs(ECX a,MEM16 b){x86(592,a,b);}
V lfs(ECX a,MEM32 b){x86(592,a,b);}
V lfs(ECX a,MEM64 b){x86(592,a,b);}
V lfs(ECX a,MEM128 b){x86(592,a,b);}
V lfs(REG32 a,MEM8 b){x86(592,a,b);}
V lfs(REG32 a,MEM16 b){x86(592,a,b);}
V lfs(REG32 a,MEM32 b){x86(592,a,b);}
V lfs(REG32 a,MEM64 b){x86(592,a,b);}
V lfs(REG32 a,MEM128 b){x86(592,a,b);}
V lgs(AX a,MEM8 b){x86(593,a,b);}
V lgs(AX a,MEM16 b){x86(593,a,b);}
V lgs(AX a,MEM32 b){x86(593,a,b);}
V lgs(AX a,MEM64 b){x86(593,a,b);}
V lgs(AX a,MEM128 b){x86(593,a,b);}
V lgs(DX a,MEM8 b){x86(593,a,b);}
V lgs(DX a,MEM16 b){x86(593,a,b);}
V lgs(DX a,MEM32 b){x86(593,a,b);}
V lgs(DX a,MEM64 b){x86(593,a,b);}
V lgs(DX a,MEM128 b){x86(593,a,b);}
V lgs(CX a,MEM8 b){x86(593,a,b);}
V lgs(CX a,MEM16 b){x86(593,a,b);}
V lgs(CX a,MEM32 b){x86(593,a,b);}
V lgs(CX a,MEM64 b){x86(593,a,b);}
V lgs(CX a,MEM128 b){x86(593,a,b);}
V lgs(REG16 a,MEM8 b){x86(593,a,b);}
V lgs(REG16 a,MEM16 b){x86(593,a,b);}
V lgs(REG16 a,MEM32 b){x86(593,a,b);}
V lgs(REG16 a,MEM64 b){x86(593,a,b);}
V lgs(REG16 a,MEM128 b){x86(593,a,b);}
V lgs(EAX a,MEM8 b){x86(594,a,b);}
V lgs(EAX a,MEM16 b){x86(594,a,b);}
V lgs(EAX a,MEM32 b){x86(594,a,b);}
V lgs(EAX a,MEM64 b){x86(594,a,b);}
V lgs(EAX a,MEM128 b){x86(594,a,b);}
V lgs(ECX a,MEM8 b){x86(594,a,b);}
V lgs(ECX a,MEM16 b){x86(594,a,b);}
V lgs(ECX a,MEM32 b){x86(594,a,b);}
V lgs(ECX a,MEM64 b){x86(594,a,b);}
V lgs(ECX a,MEM128 b){x86(594,a,b);}
V lgs(REG32 a,MEM8 b){x86(594,a,b);}
V lgs(REG32 a,MEM16 b){x86(594,a,b);}
V lgs(REG32 a,MEM32 b){x86(594,a,b);}
V lgs(REG32 a,MEM64 b){x86(594,a,b);}
V lgs(REG32 a,MEM128 b){x86(594,a,b);}
V lss(AX a,MEM8 b){x86(595,a,b);}
V lss(AX a,MEM16 b){x86(595,a,b);}
V lss(AX a,MEM32 b){x86(595,a,b);}
V lss(AX a,MEM64 b){x86(595,a,b);}
V lss(AX a,MEM128 b){x86(595,a,b);}
V lss(DX a,MEM8 b){x86(595,a,b);}
V lss(DX a,MEM16 b){x86(595,a,b);}
V lss(DX a,MEM32 b){x86(595,a,b);}
V lss(DX a,MEM64 b){x86(595,a,b);}
V lss(DX a,MEM128 b){x86(595,a,b);}
V lss(CX a,MEM8 b){x86(595,a,b);}
V lss(CX a,MEM16 b){x86(595,a,b);}
V lss(CX a,MEM32 b){x86(595,a,b);}
V lss(CX a,MEM64 b){x86(595,a,b);}
V lss(CX a,MEM128 b){x86(595,a,b);}
V lss(REG16 a,MEM8 b){x86(595,a,b);}
V lss(REG16 a,MEM16 b){x86(595,a,b);}
V lss(REG16 a,MEM32 b){x86(595,a,b);}
V lss(REG16 a,MEM64 b){x86(595,a,b);}
V lss(REG16 a,MEM128 b){x86(595,a,b);}
V lss(EAX a,MEM8 b){x86(596,a,b);}
V lss(EAX a,MEM16 b){x86(596,a,b);}
V lss(EAX a,MEM32 b){x86(596,a,b);}
V lss(EAX a,MEM64 b){x86(596,a,b);}
V lss(EAX a,MEM128 b){x86(596,a,b);}
V lss(ECX a,MEM8 b){x86(596,a,b);}
V lss(ECX a,MEM16 b){x86(596,a,b);}
V lss(ECX a,MEM32 b){x86(596,a,b);}
V lss(ECX a,MEM64 b){x86(596,a,b);}
V lss(ECX a,MEM128 b){x86(596,a,b);}
V lss(REG32 a,MEM8 b){x86(596,a,b);}
V lss(REG32 a,MEM16 b){x86(596,a,b);}
V lss(REG32 a,MEM32 b){x86(596,a,b);}
V lss(REG32 a,MEM64 b){x86(596,a,b);}
V lss(REG32 a,MEM128 b){x86(596,a,b);}
V ldmxcsr(MEM32 a){x86(597,a);}
V lea(AX a,MEM8 b){x86(598,a,b);}
V lea(AX a,MEM16 b){x86(598,a,b);}
V lea(AX a,MEM32 b){x86(598,a,b);}
V lea(AX a,MEM64 b){x86(598,a,b);}
V lea(AX a,MEM128 b){x86(598,a,b);}
V lea(DX a,MEM8 b){x86(598,a,b);}
V lea(DX a,MEM16 b){x86(598,a,b);}
V lea(DX a,MEM32 b){x86(598,a,b);}
V lea(DX a,MEM64 b){x86(598,a,b);}
V lea(DX a,MEM128 b){x86(598,a,b);}
V lea(CX a,MEM8 b){x86(598,a,b);}
V lea(CX a,MEM16 b){x86(598,a,b);}
V lea(CX a,MEM32 b){x86(598,a,b);}
V lea(CX a,MEM64 b){x86(598,a,b);}
V lea(CX a,MEM128 b){x86(598,a,b);}
V lea(REG16 a,MEM8 b){x86(598,a,b);}
V lea(REG16 a,MEM16 b){x86(598,a,b);}
V lea(REG16 a,MEM32 b){x86(598,a,b);}
V lea(REG16 a,MEM64 b){x86(598,a,b);}
V lea(REG16 a,MEM128 b){x86(598,a,b);}
V lea(EAX a,MEM8 b){x86(599,a,b);}
V lea(EAX a,MEM16 b){x86(599,a,b);}
V lea(EAX a,MEM32 b){x86(599,a,b);}
V lea(EAX a,MEM64 b){x86(599,a,b);}
V lea(EAX a,MEM128 b){x86(599,a,b);}
V lea(ECX a,MEM8 b){x86(599,a,b);}
V lea(ECX a,MEM16 b){x86(599,a,b);}
V lea(ECX a,MEM32 b){x86(599,a,b);}
V lea(ECX a,MEM64 b){x86(599,a,b);}
V lea(ECX a,MEM128 b){x86(599,a,b);}
V lea(REG32 a,MEM8 b){x86(599,a,b);}
V lea(REG32 a,MEM16 b){x86(599,a,b);}
V lea(REG32 a,MEM32 b){x86(599,a,b);}
V lea(REG32 a,MEM64 b){x86(599,a,b);}
V lea(REG32 a,MEM128 b){x86(599,a,b);}
V leave(){x86(600);}
V lfence(){x86(601);}
V lodsb(){x86(602);}
V lodsw(){x86(603);}
V lodsd(){x86(604);}
V rep_lodsb(){x86(605);}
V rep_lodsw(){x86(606);}
V rep_lodsd(){x86(607);}
V loop(int a){x86(608,(IMM)a);}
V loop(char a){x86(608,(IMM)a);}
V loop(short a){x86(608,(IMM)a);}
V loop(REF a){x86(608,a);}
V loop(int a,CX b){x86(609,(IMM)a,b);}
V loop(char a,CX b){x86(609,(IMM)a,b);}
V loop(short a,CX b){x86(609,(IMM)a,b);}
V loop(REF a,CX b){x86(609,a,b);}
V loop(int a,ECX b){x86(610,(IMM)a,b);}
V loop(char a,ECX b){x86(610,(IMM)a,b);}
V loop(short a,ECX b){x86(610,(IMM)a,b);}
V loop(REF a,ECX b){x86(610,a,b);}
V loope(int a){x86(611,(IMM)a);}
V loope(char a){x86(611,(IMM)a);}
V loope(short a){x86(611,(IMM)a);}
V loope(REF a){x86(611,a);}
V loope(int a,CX b){x86(612,(IMM)a,b);}
V loope(char a,CX b){x86(612,(IMM)a,b);}
V loope(short a,CX b){x86(612,(IMM)a,b);}
V loope(REF a,CX b){x86(612,a,b);}
V loope(int a,ECX b){x86(613,(IMM)a,b);}
V loope(char a,ECX b){x86(613,(IMM)a,b);}
V loope(short a,ECX b){x86(613,(IMM)a,b);}
V loope(REF a,ECX b){x86(613,a,b);}
V loopz(int a){x86(614,(IMM)a);}
V loopz(char a){x86(614,(IMM)a);}
V loopz(short a){x86(614,(IMM)a);}
V loopz(REF a){x86(614,a);}
V loopz(int a,CX b){x86(615,(IMM)a,b);}
V loopz(char a,CX b){x86(615,(IMM)a,b);}
V loopz(short a,CX b){x86(615,(IMM)a,b);}
V loopz(REF a,CX b){x86(615,a,b);}
V loopz(int a,ECX b){x86(616,(IMM)a,b);}
V loopz(char a,ECX b){x86(616,(IMM)a,b);}
V loopz(short a,ECX b){x86(616,(IMM)a,b);}
V loopz(REF a,ECX b){x86(616,a,b);}
V loopne(int a){x86(617,(IMM)a);}
V loopne(char a){x86(617,(IMM)a);}
V loopne(short a){x86(617,(IMM)a);}
V loopne(REF a){x86(617,a);}
V loopne(int a,CX b){x86(618,(IMM)a,b);}
V loopne(char a,CX b){x86(618,(IMM)a,b);}
V loopne(short a,CX b){x86(618,(IMM)a,b);}
V loopne(REF a,CX b){x86(618,a,b);}
V loopne(int a,ECX b){x86(619,(IMM)a,b);}
V loopne(char a,ECX b){x86(619,(IMM)a,b);}
V loopne(short a,ECX b){x86(619,(IMM)a,b);}
V loopne(REF a,ECX b){x86(619,a,b);}
V loopnz(int a){x86(620,(IMM)a);}
V loopnz(char a){x86(620,(IMM)a);}
V loopnz(short a){x86(620,(IMM)a);}
V loopnz(REF a){x86(620,a);}
V loopnz(int a,CX b){x86(621,(IMM)a,b);}
V loopnz(char a,CX b){x86(621,(IMM)a,b);}
V loopnz(short a,CX b){x86(621,(IMM)a,b);}
V loopnz(REF a,CX b){x86(621,a,b);}
V loopnz(int a,ECX b){x86(622,(IMM)a,b);}
V loopnz(char a,ECX b){x86(622,(IMM)a,b);}
V loopnz(short a,ECX b){x86(622,(IMM)a,b);}
V loopnz(REF a,ECX b){x86(622,a,b);}
V maskmovdqu(XMMREG a,XMMREG b){x86(623,a,b);}
V maskmovq(MMREG a,MMREG b){x86(624,a,b);}
V maxpd(XMMREG a,XMMREG b){x86(625,a,b);}
V maxpd(XMMREG a,MEM128 b){x86(625,a,b);}
V maxpd(XMMREG a,R_M128 b){x86(625,a,b);}
V maxps(XMMREG a,XMMREG b){x86(626,a,b);}
V maxps(XMMREG a,MEM128 b){x86(626,a,b);}
V maxps(XMMREG a,R_M128 b){x86(626,a,b);}
V maxsd(XMMREG a,XMMREG b){x86(627,a,b);}
V maxsd(XMMREG a,MEM64 b){x86(627,a,b);}
V maxsd(XMMREG a,XMM64 b){x86(627,a,b);}
V maxss(XMMREG a,XMMREG b){x86(628,a,b);}
V maxss(XMMREG a,MEM128 b){x86(628,a,b);}
V maxss(XMMREG a,R_M128 b){x86(628,a,b);}
V mfence(){x86(629);}
V minpd(XMMREG a,XMMREG b){x86(630,a,b);}
V minpd(XMMREG a,MEM128 b){x86(630,a,b);}
V minpd(XMMREG a,R_M128 b){x86(630,a,b);}
V minps(XMMREG a,XMMREG b){x86(631,a,b);}
V minps(XMMREG a,MEM128 b){x86(631,a,b);}
V minps(XMMREG a,R_M128 b){x86(631,a,b);}
V minsd(XMMREG a,XMMREG b){x86(632,a,b);}
V minsd(XMMREG a,MEM64 b){x86(632,a,b);}
V minsd(XMMREG a,XMM64 b){x86(632,a,b);}
V minss(XMMREG a,XMMREG b){x86(633,a,b);}
V minss(XMMREG a,MEM32 b){x86(633,a,b);}
V minss(XMMREG a,XMM32 b){x86(633,a,b);}
V mov(AL a,AL b){x86(634,a,b);}
V mov(AL a,CL b){x86(634,a,b);}
V mov(AL a,REG8 b){x86(634,a,b);}
V mov(CL a,AL b){x86(634,a,b);}
V mov(CL a,CL b){x86(634,a,b);}
V mov(CL a,REG8 b){x86(634,a,b);}
V mov(REG8 a,AL b){x86(634,a,b);}
V mov(REG8 a,CL b){x86(634,a,b);}
V mov(REG8 a,REG8 b){x86(634,a,b);}
V mov(MEM8 a,AL b){x86(634,a,b);}
V mov(MEM8 a,CL b){x86(634,a,b);}
V mov(MEM8 a,REG8 b){x86(634,a,b);}
V mov(R_M8 a,AL b){x86(634,a,b);}
V mov(R_M8 a,CL b){x86(634,a,b);}
V mov(R_M8 a,REG8 b){x86(634,a,b);}
V mov(AX a,AX b){x86(635,a,b);}
V mov(AX a,DX b){x86(635,a,b);}
V mov(AX a,CX b){x86(635,a,b);}
V mov(AX a,REG16 b){x86(635,a,b);}
V mov(DX a,AX b){x86(635,a,b);}
V mov(DX a,DX b){x86(635,a,b);}
V mov(DX a,CX b){x86(635,a,b);}
V mov(DX a,REG16 b){x86(635,a,b);}
V mov(CX a,AX b){x86(635,a,b);}
V mov(CX a,DX b){x86(635,a,b);}
V mov(CX a,CX b){x86(635,a,b);}
V mov(CX a,REG16 b){x86(635,a,b);}
V mov(REG16 a,AX b){x86(635,a,b);}
V mov(REG16 a,DX b){x86(635,a,b);}
V mov(REG16 a,CX b){x86(635,a,b);}
V mov(REG16 a,REG16 b){x86(635,a,b);}
V mov(MEM16 a,AX b){x86(635,a,b);}
V mov(MEM16 a,DX b){x86(635,a,b);}
V mov(MEM16 a,CX b){x86(635,a,b);}
V mov(MEM16 a,REG16 b){x86(635,a,b);}
V mov(R_M16 a,AX b){x86(635,a,b);}
V mov(R_M16 a,DX b){x86(635,a,b);}
V mov(R_M16 a,CX b){x86(635,a,b);}
V mov(R_M16 a,REG16 b){x86(635,a,b);}
V mov(EAX a,EAX b){x86(636,a,b);}
V mov(EAX a,ECX b){x86(636,a,b);}
V mov(EAX a,REG32 b){x86(636,a,b);}
V mov(ECX a,EAX b){x86(636,a,b);}
V mov(ECX a,ECX b){x86(636,a,b);}
V mov(ECX a,REG32 b){x86(636,a,b);}
V mov(REG32 a,EAX b){x86(636,a,b);}
V mov(REG32 a,ECX b){x86(636,a,b);}
V mov(REG32 a,REG32 b){x86(636,a,b);}
V mov(MEM32 a,EAX b){x86(636,a,b);}
V mov(MEM32 a,ECX b){x86(636,a,b);}
V mov(MEM32 a,REG32 b){x86(636,a,b);}
V mov(R_M32 a,EAX b){x86(636,a,b);}
V mov(R_M32 a,ECX b){x86(636,a,b);}
V mov(R_M32 a,REG32 b){x86(636,a,b);}
V mov(AL a,MEM8 b){x86(637,a,b);}
V mov(AL a,R_M8 b){x86(637,a,b);}
V mov(CL a,MEM8 b){x86(637,a,b);}
V mov(CL a,R_M8 b){x86(637,a,b);}
V mov(REG8 a,MEM8 b){x86(637,a,b);}
V mov(REG8 a,R_M8 b){x86(637,a,b);}
V mov(AX a,MEM16 b){x86(638,a,b);}
V mov(AX a,R_M16 b){x86(638,a,b);}
V mov(DX a,MEM16 b){x86(638,a,b);}
V mov(DX a,R_M16 b){x86(638,a,b);}
V mov(CX a,MEM16 b){x86(638,a,b);}
V mov(CX a,R_M16 b){x86(638,a,b);}
V mov(REG16 a,MEM16 b){x86(638,a,b);}
V mov(REG16 a,R_M16 b){x86(638,a,b);}
V mov(EAX a,MEM32 b){x86(639,a,b);}
V mov(EAX a,R_M32 b){x86(639,a,b);}
V mov(ECX a,MEM32 b){x86(639,a,b);}
V mov(ECX a,R_M32 b){x86(639,a,b);}
V mov(REG32 a,MEM32 b){x86(639,a,b);}
V mov(REG32 a,R_M32 b){x86(639,a,b);}
V mov(AL a,char b){x86(640,a,(IMM)b);}
V mov(CL a,char b){x86(640,a,(IMM)b);}
V mov(REG8 a,char b){x86(640,a,(IMM)b);}
V mov(AX a,char b){x86(641,a,(IMM)b);}
V mov(AX a,short b){x86(641,a,(IMM)b);}
V mov(DX a,char b){x86(641,a,(IMM)b);}
V mov(DX a,short b){x86(641,a,(IMM)b);}
V mov(CX a,char b){x86(641,a,(IMM)b);}
V mov(CX a,short b){x86(641,a,(IMM)b);}
V mov(REG16 a,char b){x86(641,a,(IMM)b);}
V mov(REG16 a,short b){x86(641,a,(IMM)b);}
V mov(EAX a,int b){x86(642,a,(IMM)b);}
V mov(EAX a,char b){x86(642,a,(IMM)b);}
V mov(EAX a,short b){x86(642,a,(IMM)b);}
V mov(EAX a,REF b){x86(642,a,b);}
V mov(ECX a,int b){x86(642,a,(IMM)b);}
V mov(ECX a,char b){x86(642,a,(IMM)b);}
V mov(ECX a,short b){x86(642,a,(IMM)b);}
V mov(ECX a,REF b){x86(642,a,b);}
V mov(REG32 a,int b){x86(642,a,(IMM)b);}
V mov(REG32 a,char b){x86(642,a,(IMM)b);}
V mov(REG32 a,short b){x86(642,a,(IMM)b);}
V mov(REG32 a,REF b){x86(642,a,b);}
V mov(MEM8 a,char b){x86(643,a,(IMM)b);}
V mov(R_M8 a,char b){x86(643,a,(IMM)b);}
V mov(MEM16 a,char b){x86(644,a,(IMM)b);}
V mov(MEM16 a,short b){x86(644,a,(IMM)b);}
V mov(R_M16 a,char b){x86(644,a,(IMM)b);}
V mov(R_M16 a,short b){x86(644,a,(IMM)b);}
V mov(MEM32 a,int b){x86(645,a,(IMM)b);}
V mov(MEM32 a,char b){x86(645,a,(IMM)b);}
V mov(MEM32 a,short b){x86(645,a,(IMM)b);}
V mov(MEM32 a,REF b){x86(645,a,b);}
V mov(R_M32 a,int b){x86(645,a,(IMM)b);}
V mov(R_M32 a,char b){x86(645,a,(IMM)b);}
V mov(R_M32 a,short b){x86(645,a,(IMM)b);}
V mov(R_M32 a,REF b){x86(645,a,b);}
V movapd(XMMREG a,XMMREG b){x86(646,a,b);}
V movapd(XMMREG a,MEM128 b){x86(646,a,b);}
V movapd(XMMREG a,R_M128 b){x86(646,a,b);}
V movapd(MEM128 a,XMMREG b){x86(647,a,b);}
V movapd(R_M128 a,XMMREG b){x86(647,a,b);}
V movaps(XMMREG a,XMMREG b){x86(648,a,b);}
V movaps(XMMREG a,MEM128 b){x86(648,a,b);}
V movaps(XMMREG a,R_M128 b){x86(648,a,b);}
V movaps(MEM128 a,XMMREG b){x86(649,a,b);}
V movaps(R_M128 a,XMMREG b){x86(649,a,b);}
V movd(MMREG a,EAX b){x86(650,a,b);}
V movd(MMREG a,ECX b){x86(650,a,b);}
V movd(MMREG a,REG32 b){x86(650,a,b);}
V movd(MMREG a,MEM32 b){x86(650,a,b);}
V movd(MMREG a,R_M32 b){x86(650,a,b);}
V movd(EAX a,MMREG b){x86(651,a,b);}
V movd(ECX a,MMREG b){x86(651,a,b);}
V movd(REG32 a,MMREG b){x86(651,a,b);}
V movd(MEM32 a,MMREG b){x86(651,a,b);}
V movd(R_M32 a,MMREG b){x86(651,a,b);}
V movd(XMMREG a,EAX b){x86(652,a,b);}
V movd(XMMREG a,ECX b){x86(652,a,b);}
V movd(XMMREG a,REG32 b){x86(652,a,b);}
V movd(XMMREG a,MEM32 b){x86(652,a,b);}
V movd(XMMREG a,R_M32 b){x86(652,a,b);}
V movd(EAX a,XMMREG b){x86(653,a,b);}
V movd(ECX a,XMMREG b){x86(653,a,b);}
V movd(REG32 a,XMMREG b){x86(653,a,b);}
V movd(MEM32 a,XMMREG b){x86(653,a,b);}
V movd(R_M32 a,XMMREG b){x86(653,a,b);}
V movdq2q(MMREG a,XMMREG b){x86(654,a,b);}
V movdqa(XMMREG a,XMMREG b){x86(655,a,b);}
V movdqa(XMMREG a,MEM128 b){x86(655,a,b);}
V movdqa(XMMREG a,R_M128 b){x86(655,a,b);}
V movdqa(MEM128 a,XMMREG b){x86(656,a,b);}
V movdqa(R_M128 a,XMMREG b){x86(656,a,b);}
V movdqu(XMMREG a,XMMREG b){x86(657,a,b);}
V movdqu(XMMREG a,MEM128 b){x86(657,a,b);}
V movdqu(XMMREG a,R_M128 b){x86(657,a,b);}
V movdqu(MEM128 a,XMMREG b){x86(658,a,b);}
V movdqu(R_M128 a,XMMREG b){x86(658,a,b);}
V movhpd(XMMREG a,MEM64 b){x86(659,a,b);}
V movhpd(MEM64 a,XMMREG b){x86(660,a,b);}
V movhlps(XMMREG a,XMMREG b){x86(661,a,b);}
V movlpd(XMMREG a,MEM64 b){x86(662,a,b);}
V movlpd(MEM64 a,XMMREG b){x86(663,a,b);}
V movhps(XMMREG a,MEM64 b){x86(664,a,b);}
V movhps(MEM64 a,XMMREG b){x86(665,a,b);}
V movhps(XMMREG a,XMMREG b){x86(666,a,b);}
V movlhps(XMMREG a,XMMREG b){x86(667,a,b);}
V movlps(XMMREG a,MEM64 b){x86(668,a,b);}
V movlps(MEM64 a,XMMREG b){x86(669,a,b);}
V movlps(XMMREG a,XMMREG b){x86(670,a,b);}
V movmskpd(EAX a,XMMREG b){x86(671,a,b);}
V movmskpd(ECX a,XMMREG b){x86(671,a,b);}
V movmskpd(REG32 a,XMMREG b){x86(671,a,b);}
V movmskps(EAX a,XMMREG b){x86(672,a,b);}
V movmskps(ECX a,XMMREG b){x86(672,a,b);}
V movmskps(REG32 a,XMMREG b){x86(672,a,b);}
V movntdq(MEM128 a,XMMREG b){x86(673,a,b);}
V movnti(MEM32 a,EAX b){x86(674,a,b);}
V movnti(MEM32 a,ECX b){x86(674,a,b);}
V movnti(MEM32 a,REG32 b){x86(674,a,b);}
V movntpd(MEM128 a,XMMREG b){x86(675,a,b);}
V movntps(MEM128 a,XMMREG b){x86(676,a,b);}
V movntq(MEM64 a,MMREG b){x86(677,a,b);}
V movq(MMREG a,MMREG b){x86(678,a,b);}
V movq(MMREG a,MEM64 b){x86(678,a,b);}
V movq(MMREG a,R_M64 b){x86(678,a,b);}
V movq(MEM64 a,MMREG b){x86(679,a,b);}
V movq(R_M64 a,MMREG b){x86(679,a,b);}
V movq(XMMREG a,XMMREG b){x86(680,a,b);}
V movq(XMMREG a,MEM64 b){x86(680,a,b);}
V movq(XMMREG a,XMM64 b){x86(680,a,b);}
V movq(MEM64 a,XMMREG b){x86(681,a,b);}
V movq(XMM64 a,XMMREG b){x86(681,a,b);}
V movq2dq(XMMREG a,MMREG b){x86(682,a,b);}
V movsb(){x86(683);}
V movsw(){x86(684);}
V movsd(){x86(685);}
V rep_movsb(){x86(686);}
V rep_movsw(){x86(687);}
V rep_movsd(){x86(688);}
V movsd(XMMREG a,XMMREG b){x86(689,a,b);}
V movsd(XMMREG a,MEM64 b){x86(689,a,b);}
V movsd(XMMREG a,XMM64 b){x86(689,a,b);}
V movsd(MEM64 a,XMMREG b){x86(690,a,b);}
V movsd(XMM64 a,XMMREG b){x86(690,a,b);}
V movss(XMMREG a,XMMREG b){x86(691,a,b);}
V movss(XMMREG a,MEM32 b){x86(691,a,b);}
V movss(XMMREG a,XMM32 b){x86(691,a,b);}
V movss(MEM32 a,XMMREG b){x86(692,a,b);}
V movss(XMM32 a,XMMREG b){x86(692,a,b);}
V movsx(AX a,AL b){x86(693,a,b);}
V movsx(AX a,CL b){x86(693,a,b);}
V movsx(AX a,REG8 b){x86(693,a,b);}
V movsx(AX a,MEM8 b){x86(693,a,b);}
V movsx(AX a,R_M8 b){x86(693,a,b);}
V movsx(DX a,AL b){x86(693,a,b);}
V movsx(DX a,CL b){x86(693,a,b);}
V movsx(DX a,REG8 b){x86(693,a,b);}
V movsx(DX a,MEM8 b){x86(693,a,b);}
V movsx(DX a,R_M8 b){x86(693,a,b);}
V movsx(CX a,AL b){x86(693,a,b);}
V movsx(CX a,CL b){x86(693,a,b);}
V movsx(CX a,REG8 b){x86(693,a,b);}
V movsx(CX a,MEM8 b){x86(693,a,b);}
V movsx(CX a,R_M8 b){x86(693,a,b);}
V movsx(REG16 a,AL b){x86(693,a,b);}
V movsx(REG16 a,CL b){x86(693,a,b);}
V movsx(REG16 a,REG8 b){x86(693,a,b);}
V movsx(REG16 a,MEM8 b){x86(693,a,b);}
V movsx(REG16 a,R_M8 b){x86(693,a,b);}
V movsx(EAX a,AL b){x86(694,a,b);}
V movsx(EAX a,CL b){x86(694,a,b);}
V movsx(EAX a,REG8 b){x86(694,a,b);}
V movsx(EAX a,MEM8 b){x86(694,a,b);}
V movsx(EAX a,R_M8 b){x86(694,a,b);}
V movsx(ECX a,AL b){x86(694,a,b);}
V movsx(ECX a,CL b){x86(694,a,b);}
V movsx(ECX a,REG8 b){x86(694,a,b);}
V movsx(ECX a,MEM8 b){x86(694,a,b);}
V movsx(ECX a,R_M8 b){x86(694,a,b);}
V movsx(REG32 a,AL b){x86(694,a,b);}
V movsx(REG32 a,CL b){x86(694,a,b);}
V movsx(REG32 a,REG8 b){x86(694,a,b);}
V movsx(REG32 a,MEM8 b){x86(694,a,b);}
V movsx(REG32 a,R_M8 b){x86(694,a,b);}
V movsx(EAX a,AX b){x86(695,a,b);}
V movsx(EAX a,DX b){x86(695,a,b);}
V movsx(EAX a,CX b){x86(695,a,b);}
V movsx(EAX a,REG16 b){x86(695,a,b);}
V movsx(EAX a,MEM16 b){x86(695,a,b);}
V movsx(EAX a,R_M16 b){x86(695,a,b);}
V movsx(ECX a,AX b){x86(695,a,b);}
V movsx(ECX a,DX b){x86(695,a,b);}
V movsx(ECX a,CX b){x86(695,a,b);}
V movsx(ECX a,REG16 b){x86(695,a,b);}
V movsx(ECX a,MEM16 b){x86(695,a,b);}
V movsx(ECX a,R_M16 b){x86(695,a,b);}
V movsx(REG32 a,AX b){x86(695,a,b);}
V movsx(REG32 a,DX b){x86(695,a,b);}
V movsx(REG32 a,CX b){x86(695,a,b);}
V movsx(REG32 a,REG16 b){x86(695,a,b);}
V movsx(REG32 a,MEM16 b){x86(695,a,b);}
V movsx(REG32 a,R_M16 b){x86(695,a,b);}
V movzx(AX a,AL b){x86(696,a,b);}
V movzx(AX a,CL b){x86(696,a,b);}
V movzx(AX a,REG8 b){x86(696,a,b);}
V movzx(AX a,MEM8 b){x86(696,a,b);}
V movzx(AX a,R_M8 b){x86(696,a,b);}
V movzx(DX a,AL b){x86(696,a,b);}
V movzx(DX a,CL b){x86(696,a,b);}
V movzx(DX a,REG8 b){x86(696,a,b);}
V movzx(DX a,MEM8 b){x86(696,a,b);}
V movzx(DX a,R_M8 b){x86(696,a,b);}
V movzx(CX a,AL b){x86(696,a,b);}
V movzx(CX a,CL b){x86(696,a,b);}
V movzx(CX a,REG8 b){x86(696,a,b);}
V movzx(CX a,MEM8 b){x86(696,a,b);}
V movzx(CX a,R_M8 b){x86(696,a,b);}
V movzx(REG16 a,AL b){x86(696,a,b);}
V movzx(REG16 a,CL b){x86(696,a,b);}
V movzx(REG16 a,REG8 b){x86(696,a,b);}
V movzx(REG16 a,MEM8 b){x86(696,a,b);}
V movzx(REG16 a,R_M8 b){x86(696,a,b);}
V movzx(EAX a,AL b){x86(697,a,b);}
V movzx(EAX a,CL b){x86(697,a,b);}
V movzx(EAX a,REG8 b){x86(697,a,b);}
V movzx(EAX a,MEM8 b){x86(697,a,b);}
V movzx(EAX a,R_M8 b){x86(697,a,b);}
V movzx(ECX a,AL b){x86(697,a,b);}
V movzx(ECX a,CL b){x86(697,a,b);}
V movzx(ECX a,REG8 b){x86(697,a,b);}
V movzx(ECX a,MEM8 b){x86(697,a,b);}
V movzx(ECX a,R_M8 b){x86(697,a,b);}
V movzx(REG32 a,AL b){x86(697,a,b);}
V movzx(REG32 a,CL b){x86(697,a,b);}
V movzx(REG32 a,REG8 b){x86(697,a,b);}
V movzx(REG32 a,MEM8 b){x86(697,a,b);}
V movzx(REG32 a,R_M8 b){x86(697,a,b);}
V movzx(EAX a,AX b){x86(698,a,b);}
V movzx(EAX a,DX b){x86(698,a,b);}
V movzx(EAX a,CX b){x86(698,a,b);}
V movzx(EAX a,REG16 b){x86(698,a,b);}
V movzx(EAX a,MEM16 b){x86(698,a,b);}
V movzx(EAX a,R_M16 b){x86(698,a,b);}
V movzx(ECX a,AX b){x86(698,a,b);}
V movzx(ECX a,DX b){x86(698,a,b);}
V movzx(ECX a,CX b){x86(698,a,b);}
V movzx(ECX a,REG16 b){x86(698,a,b);}
V movzx(ECX a,MEM16 b){x86(698,a,b);}
V movzx(ECX a,R_M16 b){x86(698,a,b);}
V movzx(REG32 a,AX b){x86(698,a,b);}
V movzx(REG32 a,DX b){x86(698,a,b);}
V movzx(REG32 a,CX b){x86(698,a,b);}
V movzx(REG32 a,REG16 b){x86(698,a,b);}
V movzx(REG32 a,MEM16 b){x86(698,a,b);}
V movzx(REG32 a,R_M16 b){x86(698,a,b);}
V movupd(XMMREG a,XMMREG b){x86(699,a,b);}
V movupd(XMMREG a,MEM128 b){x86(699,a,b);}
V movupd(XMMREG a,R_M128 b){x86(699,a,b);}
V movupd(MEM128 a,XMMREG b){x86(700,a,b);}
V movupd(R_M128 a,XMMREG b){x86(700,a,b);}
V movups(XMMREG a,XMMREG b){x86(701,a,b);}
V movups(XMMREG a,MEM128 b){x86(701,a,b);}
V movups(XMMREG a,R_M128 b){x86(701,a,b);}
V movups(MEM128 a,XMMREG b){x86(702,a,b);}
V movups(R_M128 a,XMMREG b){x86(702,a,b);}
V mul(AL a){x86(703,a);}
V mul(CL a){x86(703,a);}
V mul(REG8 a){x86(703,a);}
V mul(MEM8 a){x86(703,a);}
V mul(R_M8 a){x86(703,a);}
V mul(AX a){x86(704,a);}
V mul(DX a){x86(704,a);}
V mul(CX a){x86(704,a);}
V mul(REG16 a){x86(704,a);}
V mul(MEM16 a){x86(704,a);}
V mul(R_M16 a){x86(704,a);}
V mul(EAX a){x86(705,a);}
V mul(ECX a){x86(705,a);}
V mul(REG32 a){x86(705,a);}
V mul(MEM32 a){x86(705,a);}
V mul(R_M32 a){x86(705,a);}
V mulpd(XMMREG a,XMMREG b){x86(706,a,b);}
V mulpd(XMMREG a,MEM128 b){x86(706,a,b);}
V mulpd(XMMREG a,R_M128 b){x86(706,a,b);}
V mulps(XMMREG a,XMMREG b){x86(707,a,b);}
V mulps(XMMREG a,MEM128 b){x86(707,a,b);}
V mulps(XMMREG a,R_M128 b){x86(707,a,b);}
V mulsd(XMMREG a,XMMREG b){x86(708,a,b);}
V mulsd(XMMREG a,MEM64 b){x86(708,a,b);}
V mulsd(XMMREG a,XMM64 b){x86(708,a,b);}
V mulss(XMMREG a,XMMREG b){x86(709,a,b);}
V mulss(XMMREG a,MEM32 b){x86(709,a,b);}
V mulss(XMMREG a,XMM32 b){x86(709,a,b);}
V neg(AL a){x86(710,a);}
V neg(CL a){x86(710,a);}
V neg(REG8 a){x86(710,a);}
V neg(MEM8 a){x86(710,a);}
V neg(R_M8 a){x86(710,a);}
V neg(AX a){x86(711,a);}
V neg(DX a){x86(711,a);}
V neg(CX a){x86(711,a);}
V neg(REG16 a){x86(711,a);}
V neg(MEM16 a){x86(711,a);}
V neg(R_M16 a){x86(711,a);}
V neg(EAX a){x86(712,a);}
V neg(ECX a){x86(712,a);}
V neg(REG32 a){x86(712,a);}
V neg(MEM32 a){x86(712,a);}
V neg(R_M32 a){x86(712,a);}
V lock_neg(MEM8 a){x86(713,a);}
V lock_neg(MEM16 a){x86(714,a);}
V lock_neg(MEM32 a){x86(715,a);}
V not(AL a){x86(716,a);}
V not(CL a){x86(716,a);}
V not(REG8 a){x86(716,a);}
V not(MEM8 a){x86(716,a);}
V not(R_M8 a){x86(716,a);}
V not(AX a){x86(717,a);}
V not(DX a){x86(717,a);}
V not(CX a){x86(717,a);}
V not(REG16 a){x86(717,a);}
V not(MEM16 a){x86(717,a);}
V not(R_M16 a){x86(717,a);}
V not(EAX a){x86(718,a);}
V not(ECX a){x86(718,a);}
V not(REG32 a){x86(718,a);}
V not(MEM32 a){x86(718,a);}
V not(R_M32 a){x86(718,a);}
V lock_not(MEM8 a){x86(719,a);}
V lock_not(MEM16 a){x86(720,a);}
V lock_not(MEM32 a){x86(721,a);}
V nop(){x86(722);}
V or(AL a,AL b){x86(723,a,b);}
V or(AL a,CL b){x86(723,a,b);}
V or(AL a,REG8 b){x86(723,a,b);}
V or(CL a,AL b){x86(723,a,b);}
V or(CL a,CL b){x86(723,a,b);}
V or(CL a,REG8 b){x86(723,a,b);}
V or(REG8 a,AL b){x86(723,a,b);}
V or(REG8 a,CL b){x86(723,a,b);}
V or(REG8 a,REG8 b){x86(723,a,b);}
V or(MEM8 a,AL b){x86(723,a,b);}
V or(MEM8 a,CL b){x86(723,a,b);}
V or(MEM8 a,REG8 b){x86(723,a,b);}
V or(R_M8 a,AL b){x86(723,a,b);}
V or(R_M8 a,CL b){x86(723,a,b);}
V or(R_M8 a,REG8 b){x86(723,a,b);}
V or(AX a,AX b){x86(724,a,b);}
V or(AX a,DX b){x86(724,a,b);}
V or(AX a,CX b){x86(724,a,b);}
V or(AX a,REG16 b){x86(724,a,b);}
V or(DX a,AX b){x86(724,a,b);}
V or(DX a,DX b){x86(724,a,b);}
V or(DX a,CX b){x86(724,a,b);}
V or(DX a,REG16 b){x86(724,a,b);}
V or(CX a,AX b){x86(724,a,b);}
V or(CX a,DX b){x86(724,a,b);}
V or(CX a,CX b){x86(724,a,b);}
V or(CX a,REG16 b){x86(724,a,b);}
V or(REG16 a,AX b){x86(724,a,b);}
V or(REG16 a,DX b){x86(724,a,b);}
V or(REG16 a,CX b){x86(724,a,b);}
V or(REG16 a,REG16 b){x86(724,a,b);}
V or(MEM16 a,AX b){x86(724,a,b);}
V or(MEM16 a,DX b){x86(724,a,b);}
V or(MEM16 a,CX b){x86(724,a,b);}
V or(MEM16 a,REG16 b){x86(724,a,b);}
V or(R_M16 a,AX b){x86(724,a,b);}
V or(R_M16 a,DX b){x86(724,a,b);}
V or(R_M16 a,CX b){x86(724,a,b);}
V or(R_M16 a,REG16 b){x86(724,a,b);}
V or(EAX a,EAX b){x86(725,a,b);}
V or(EAX a,ECX b){x86(725,a,b);}
V or(EAX a,REG32 b){x86(725,a,b);}
V or(ECX a,EAX b){x86(725,a,b);}
V or(ECX a,ECX b){x86(725,a,b);}
V or(ECX a,REG32 b){x86(725,a,b);}
V or(REG32 a,EAX b){x86(725,a,b);}
V or(REG32 a,ECX b){x86(725,a,b);}
V or(REG32 a,REG32 b){x86(725,a,b);}
V or(MEM32 a,EAX b){x86(725,a,b);}
V or(MEM32 a,ECX b){x86(725,a,b);}
V or(MEM32 a,REG32 b){x86(725,a,b);}
V or(R_M32 a,EAX b){x86(725,a,b);}
V or(R_M32 a,ECX b){x86(725,a,b);}
V or(R_M32 a,REG32 b){x86(725,a,b);}
V lock_or(MEM8 a,AL b){x86(726,a,b);}
V lock_or(MEM8 a,CL b){x86(726,a,b);}
V lock_or(MEM8 a,REG8 b){x86(726,a,b);}
V lock_or(MEM16 a,AX b){x86(727,a,b);}
V lock_or(MEM16 a,DX b){x86(727,a,b);}
V lock_or(MEM16 a,CX b){x86(727,a,b);}
V lock_or(MEM16 a,REG16 b){x86(727,a,b);}
V lock_or(MEM32 a,EAX b){x86(728,a,b);}
V lock_or(MEM32 a,ECX b){x86(728,a,b);}
V lock_or(MEM32 a,REG32 b){x86(728,a,b);}
V or(AL a,MEM8 b){x86(729,a,b);}
V or(AL a,R_M8 b){x86(729,a,b);}
V or(CL a,MEM8 b){x86(729,a,b);}
V or(CL a,R_M8 b){x86(729,a,b);}
V or(REG8 a,MEM8 b){x86(729,a,b);}
V or(REG8 a,R_M8 b){x86(729,a,b);}
V or(AX a,MEM16 b){x86(730,a,b);}
V or(AX a,R_M16 b){x86(730,a,b);}
V or(DX a,MEM16 b){x86(730,a,b);}
V or(DX a,R_M16 b){x86(730,a,b);}
V or(CX a,MEM16 b){x86(730,a,b);}
V or(CX a,R_M16 b){x86(730,a,b);}
V or(REG16 a,MEM16 b){x86(730,a,b);}
V or(REG16 a,R_M16 b){x86(730,a,b);}
V or(EAX a,MEM32 b){x86(731,a,b);}
V or(EAX a,R_M32 b){x86(731,a,b);}
V or(ECX a,MEM32 b){x86(731,a,b);}
V or(ECX a,R_M32 b){x86(731,a,b);}
V or(REG32 a,MEM32 b){x86(731,a,b);}
V or(REG32 a,R_M32 b){x86(731,a,b);}
V or(AL a,char b){x86(732,a,(IMM)b);}
V or(CL a,char b){x86(732,a,(IMM)b);}
V or(REG8 a,char b){x86(732,a,(IMM)b);}
V or(MEM8 a,char b){x86(732,a,(IMM)b);}
V or(R_M8 a,char b){x86(732,a,(IMM)b);}
V or(AX a,char b){x86(733,a,(IMM)b);}
V or(AX a,short b){x86(733,a,(IMM)b);}
V or(DX a,char b){x86(733,a,(IMM)b);}
V or(DX a,short b){x86(733,a,(IMM)b);}
V or(CX a,char b){x86(733,a,(IMM)b);}
V or(CX a,short b){x86(733,a,(IMM)b);}
V or(REG16 a,char b){x86(733,a,(IMM)b);}
V or(REG16 a,short b){x86(733,a,(IMM)b);}
V or(MEM16 a,char b){x86(733,a,(IMM)b);}
V or(MEM16 a,short b){x86(733,a,(IMM)b);}
V or(R_M16 a,char b){x86(733,a,(IMM)b);}
V or(R_M16 a,short b){x86(733,a,(IMM)b);}
V or(EAX a,int b){x86(734,a,(IMM)b);}
V or(EAX a,char b){x86(734,a,(IMM)b);}
V or(EAX a,short b){x86(734,a,(IMM)b);}
V or(EAX a,REF b){x86(734,a,b);}
V or(ECX a,int b){x86(734,a,(IMM)b);}
V or(ECX a,char b){x86(734,a,(IMM)b);}
V or(ECX a,short b){x86(734,a,(IMM)b);}
V or(ECX a,REF b){x86(734,a,b);}
V or(REG32 a,int b){x86(734,a,(IMM)b);}
V or(REG32 a,char b){x86(734,a,(IMM)b);}
V or(REG32 a,short b){x86(734,a,(IMM)b);}
V or(REG32 a,REF b){x86(734,a,b);}
V or(MEM32 a,int b){x86(734,a,(IMM)b);}
V or(MEM32 a,char b){x86(734,a,(IMM)b);}
V or(MEM32 a,short b){x86(734,a,(IMM)b);}
V or(MEM32 a,REF b){x86(734,a,b);}
V or(R_M32 a,int b){x86(734,a,(IMM)b);}
V or(R_M32 a,char b){x86(734,a,(IMM)b);}
V or(R_M32 a,short b){x86(734,a,(IMM)b);}
V or(R_M32 a,REF b){x86(734,a,b);}
V lock_or(MEM8 a,char b){x86(737,a,(IMM)b);}
V lock_or(MEM16 a,char b){x86(738,a,(IMM)b);}
V lock_or(MEM16 a,short b){x86(738,a,(IMM)b);}
V lock_or(MEM32 a,int b){x86(739,a,(IMM)b);}
V lock_or(MEM32 a,char b){x86(739,a,(IMM)b);}
V lock_or(MEM32 a,short b){x86(739,a,(IMM)b);}
V lock_or(MEM32 a,REF b){x86(739,a,b);}
V orpd(XMMREG a,XMMREG b){x86(745,a,b);}
V orpd(XMMREG a,MEM128 b){x86(745,a,b);}
V orpd(XMMREG a,R_M128 b){x86(745,a,b);}
V orps(XMMREG a,XMMREG b){x86(746,a,b);}
V orps(XMMREG a,MEM128 b){x86(746,a,b);}
V orps(XMMREG a,R_M128 b){x86(746,a,b);}
V out(char a,AL b){x86(747,(IMM)a,b);}
V out(char a,AX b){x86(748,(IMM)a,b);}
V out(char a,EAX b){x86(749,(IMM)a,b);}
V out(DX a,AL b){x86(750,a,b);}
V out(DX a,AX b){x86(751,a,b);}
V out(DX a,EAX b){x86(752,a,b);}
V outsb(){x86(753);}
V outsw(){x86(754);}
V outsd(){x86(755);}
V rep_outsb(){x86(756);}
V rep_outsw(){x86(757);}
V rep_outsd(){x86(758);}
V packssdw(MMREG a,MMREG b){x86(759,a,b);}
V packssdw(MMREG a,MEM64 b){x86(759,a,b);}
V packssdw(MMREG a,R_M64 b){x86(759,a,b);}
V packsswb(MMREG a,MMREG b){x86(760,a,b);}
V packsswb(MMREG a,MEM64 b){x86(760,a,b);}
V packsswb(MMREG a,R_M64 b){x86(760,a,b);}
V packuswb(MMREG a,MMREG b){x86(761,a,b);}
V packuswb(MMREG a,MEM64 b){x86(761,a,b);}
V packuswb(MMREG a,R_M64 b){x86(761,a,b);}
V packssdw(XMMREG a,XMMREG b){x86(762,a,b);}
V packssdw(XMMREG a,MEM128 b){x86(762,a,b);}
V packssdw(XMMREG a,R_M128 b){x86(762,a,b);}
V packsswb(XMMREG a,XMMREG b){x86(763,a,b);}
V packsswb(XMMREG a,MEM128 b){x86(763,a,b);}
V packsswb(XMMREG a,R_M128 b){x86(763,a,b);}
V packuswb(XMMREG a,XMMREG b){x86(764,a,b);}
V packuswb(XMMREG a,MEM128 b){x86(764,a,b);}
V packuswb(XMMREG a,R_M128 b){x86(764,a,b);}
V paddb(MMREG a,MMREG b){x86(765,a,b);}
V paddb(MMREG a,MEM64 b){x86(765,a,b);}
V paddb(MMREG a,R_M64 b){x86(765,a,b);}
V paddw(MMREG a,MMREG b){x86(766,a,b);}
V paddw(MMREG a,MEM64 b){x86(766,a,b);}
V paddw(MMREG a,R_M64 b){x86(766,a,b);}
V paddd(MMREG a,MMREG b){x86(767,a,b);}
V paddd(MMREG a,MEM64 b){x86(767,a,b);}
V paddd(MMREG a,R_M64 b){x86(767,a,b);}
V paddb(XMMREG a,XMMREG b){x86(768,a,b);}
V paddb(XMMREG a,MEM128 b){x86(768,a,b);}
V paddb(XMMREG a,R_M128 b){x86(768,a,b);}
V paddw(XMMREG a,XMMREG b){x86(769,a,b);}
V paddw(XMMREG a,MEM128 b){x86(769,a,b);}
V paddw(XMMREG a,R_M128 b){x86(769,a,b);}
V paddd(XMMREG a,XMMREG b){x86(770,a,b);}
V paddd(XMMREG a,MEM128 b){x86(770,a,b);}
V paddd(XMMREG a,R_M128 b){x86(770,a,b);}
V paddq(MMREG a,MMREG b){x86(771,a,b);}
V paddq(MMREG a,MEM64 b){x86(771,a,b);}
V paddq(MMREG a,R_M64 b){x86(771,a,b);}
V paddq(XMMREG a,XMMREG b){x86(772,a,b);}
V paddq(XMMREG a,MEM128 b){x86(772,a,b);}
V paddq(XMMREG a,R_M128 b){x86(772,a,b);}
V paddsb(MMREG a,MMREG b){x86(773,a,b);}
V paddsb(MMREG a,MEM64 b){x86(773,a,b);}
V paddsb(MMREG a,R_M64 b){x86(773,a,b);}
V paddsw(MMREG a,MMREG b){x86(774,a,b);}
V paddsw(MMREG a,MEM64 b){x86(774,a,b);}
V paddsw(MMREG a,R_M64 b){x86(774,a,b);}
V paddsb(XMMREG a,XMMREG b){x86(775,a,b);}
V paddsb(XMMREG a,MEM128 b){x86(775,a,b);}
V paddsb(XMMREG a,R_M128 b){x86(775,a,b);}
V paddsw(XMMREG a,XMMREG b){x86(776,a,b);}
V paddsw(XMMREG a,MEM128 b){x86(776,a,b);}
V paddsw(XMMREG a,R_M128 b){x86(776,a,b);}
V paddusb(MMREG a,MMREG b){x86(777,a,b);}
V paddusb(MMREG a,MEM64 b){x86(777,a,b);}
V paddusb(MMREG a,R_M64 b){x86(777,a,b);}
V paddusw(MMREG a,MMREG b){x86(778,a,b);}
V paddusw(MMREG a,MEM64 b){x86(778,a,b);}
V paddusw(MMREG a,R_M64 b){x86(778,a,b);}
V paddusb(XMMREG a,XMMREG b){x86(779,a,b);}
V paddusb(XMMREG a,MEM128 b){x86(779,a,b);}
V paddusb(XMMREG a,R_M128 b){x86(779,a,b);}
V paddusw(XMMREG a,XMMREG b){x86(780,a,b);}
V paddusw(XMMREG a,MEM128 b){x86(780,a,b);}
V paddusw(XMMREG a,R_M128 b){x86(780,a,b);}
V paddsiw(MMREG a,MMREG b){x86(781,a,b);}
V paddsiw(MMREG a,MEM64 b){x86(781,a,b);}
V paddsiw(MMREG a,R_M64 b){x86(781,a,b);}
V pand(MMREG a,MMREG b){x86(782,a,b);}
V pand(MMREG a,MEM64 b){x86(782,a,b);}
V pand(MMREG a,R_M64 b){x86(782,a,b);}
V pandn(MMREG a,MMREG b){x86(783,a,b);}
V pandn(MMREG a,MEM64 b){x86(783,a,b);}
V pandn(MMREG a,R_M64 b){x86(783,a,b);}
V pand(XMMREG a,XMMREG b){x86(784,a,b);}
V pand(XMMREG a,MEM128 b){x86(784,a,b);}
V pand(XMMREG a,R_M128 b){x86(784,a,b);}
V pandn(XMMREG a,XMMREG b){x86(785,a,b);}
V pandn(XMMREG a,MEM128 b){x86(785,a,b);}
V pandn(XMMREG a,R_M128 b){x86(785,a,b);}
V pause(){x86(786);}
V paveb(MMREG a,MMREG b){x86(787,a,b);}
V paveb(MMREG a,MEM64 b){x86(787,a,b);}
V paveb(MMREG a,R_M64 b){x86(787,a,b);}
V pavgb(MMREG a,MMREG b){x86(788,a,b);}
V pavgb(MMREG a,MEM64 b){x86(788,a,b);}
V pavgb(MMREG a,R_M64 b){x86(788,a,b);}
V pavgw(MMREG a,MMREG b){x86(789,a,b);}
V pavgw(MMREG a,MEM64 b){x86(789,a,b);}
V pavgw(MMREG a,R_M64 b){x86(789,a,b);}
V pavgb(XMMREG a,XMMREG b){x86(790,a,b);}
V pavgb(XMMREG a,MEM128 b){x86(790,a,b);}
V pavgb(XMMREG a,R_M128 b){x86(790,a,b);}
V pavgw(XMMREG a,XMMREG b){x86(791,a,b);}
V pavgw(XMMREG a,MEM128 b){x86(791,a,b);}
V pavgw(XMMREG a,R_M128 b){x86(791,a,b);}
V pavgusb(MMREG a,MMREG b){x86(792,a,b);}
V pavgusb(MMREG a,MEM64 b){x86(792,a,b);}
V pavgusb(MMREG a,R_M64 b){x86(792,a,b);}
V pcmpeqb(MMREG a,MMREG b){x86(793,a,b);}
V pcmpeqb(MMREG a,MEM64 b){x86(793,a,b);}
V pcmpeqb(MMREG a,R_M64 b){x86(793,a,b);}
V pcmpeqw(MMREG a,MMREG b){x86(794,a,b);}
V pcmpeqw(MMREG a,MEM64 b){x86(794,a,b);}
V pcmpeqw(MMREG a,R_M64 b){x86(794,a,b);}
V pcmpeqd(MMREG a,MMREG b){x86(795,a,b);}
V pcmpeqd(MMREG a,MEM64 b){x86(795,a,b);}
V pcmpeqd(MMREG a,R_M64 b){x86(795,a,b);}
V pcmpgtb(MMREG a,MMREG b){x86(796,a,b);}
V pcmpgtb(MMREG a,MEM64 b){x86(796,a,b);}
V pcmpgtb(MMREG a,R_M64 b){x86(796,a,b);}
V pcmpgtw(MMREG a,MMREG b){x86(797,a,b);}
V pcmpgtw(MMREG a,MEM64 b){x86(797,a,b);}
V pcmpgtw(MMREG a,R_M64 b){x86(797,a,b);}
V pcmpgtd(MMREG a,MMREG b){x86(798,a,b);}
V pcmpgtd(MMREG a,MEM64 b){x86(798,a,b);}
V pcmpgtd(MMREG a,R_M64 b){x86(798,a,b);}
V pcmpeqb(XMMREG a,XMMREG b){x86(799,a,b);}
V pcmpeqb(XMMREG a,MEM128 b){x86(799,a,b);}
V pcmpeqb(XMMREG a,R_M128 b){x86(799,a,b);}
V pcmpeqw(XMMREG a,XMMREG b){x86(800,a,b);}
V pcmpeqw(XMMREG a,MEM128 b){x86(800,a,b);}
V pcmpeqw(XMMREG a,R_M128 b){x86(800,a,b);}
V pcmpeqd(XMMREG a,XMMREG b){x86(801,a,b);}
V pcmpeqd(XMMREG a,MEM128 b){x86(801,a,b);}
V pcmpeqd(XMMREG a,R_M128 b){x86(801,a,b);}
V pcmpgtb(XMMREG a,XMMREG b){x86(802,a,b);}
V pcmpgtb(XMMREG a,MEM128 b){x86(802,a,b);}
V pcmpgtb(XMMREG a,R_M128 b){x86(802,a,b);}
V pcmpgtw(XMMREG a,XMMREG b){x86(803,a,b);}
V pcmpgtw(XMMREG a,MEM128 b){x86(803,a,b);}
V pcmpgtw(XMMREG a,R_M128 b){x86(803,a,b);}
V pcmpgtd(XMMREG a,XMMREG b){x86(804,a,b);}
V pcmpgtd(XMMREG a,MEM128 b){x86(804,a,b);}
V pcmpgtd(XMMREG a,R_M128 b){x86(804,a,b);}
V pdistib(MMREG a,MEM64 b){x86(805,a,b);}
V pextrw(EAX a,MMREG b,char c){x86(806,a,b,(IMM)c);}
V pextrw(ECX a,MMREG b,char c){x86(806,a,b,(IMM)c);}
V pextrw(REG32 a,MMREG b,char c){x86(806,a,b,(IMM)c);}
V pextrw(EAX a,XMMREG b,char c){x86(807,a,b,(IMM)c);}
V pextrw(ECX a,XMMREG b,char c){x86(807,a,b,(IMM)c);}
V pextrw(REG32 a,XMMREG b,char c){x86(807,a,b,(IMM)c);}
V pf2id(MMREG a,MMREG b){x86(808,a,b);}
V pf2id(MMREG a,MEM64 b){x86(808,a,b);}
V pf2id(MMREG a,R_M64 b){x86(808,a,b);}
V pf2iw(MMREG a,MMREG b){x86(809,a,b);}
V pf2iw(MMREG a,MEM64 b){x86(809,a,b);}
V pf2iw(MMREG a,R_M64 b){x86(809,a,b);}
V pfacc(MMREG a,MMREG b){x86(810,a,b);}
V pfacc(MMREG a,MEM64 b){x86(810,a,b);}
V pfacc(MMREG a,R_M64 b){x86(810,a,b);}
V pfadd(MMREG a,MMREG b){x86(811,a,b);}
V pfadd(MMREG a,MEM64 b){x86(811,a,b);}
V pfadd(MMREG a,R_M64 b){x86(811,a,b);}
V pfcmpeq(MMREG a,MMREG b){x86(812,a,b);}
V pfcmpeq(MMREG a,MEM64 b){x86(812,a,b);}
V pfcmpeq(MMREG a,R_M64 b){x86(812,a,b);}
V pfcmpge(MMREG a,MMREG b){x86(813,a,b);}
V pfcmpge(MMREG a,MEM64 b){x86(813,a,b);}
V pfcmpge(MMREG a,R_M64 b){x86(813,a,b);}
V pfcmpgt(MMREG a,MMREG b){x86(814,a,b);}
V pfcmpgt(MMREG a,MEM64 b){x86(814,a,b);}
V pfcmpgt(MMREG a,R_M64 b){x86(814,a,b);}
V pfmax(MMREG a,MMREG b){x86(815,a,b);}
V pfmax(MMREG a,MEM64 b){x86(815,a,b);}
V pfmax(MMREG a,R_M64 b){x86(815,a,b);}
V pfmin(MMREG a,MMREG b){x86(816,a,b);}
V pfmin(MMREG a,MEM64 b){x86(816,a,b);}
V pfmin(MMREG a,R_M64 b){x86(816,a,b);}
V pfmul(MMREG a,MMREG b){x86(817,a,b);}
V pfmul(MMREG a,MEM64 b){x86(817,a,b);}
V pfmul(MMREG a,R_M64 b){x86(817,a,b);}
V pfnacc(MMREG a,MMREG b){x86(818,a,b);}
V pfnacc(MMREG a,MEM64 b){x86(818,a,b);}
V pfnacc(MMREG a,R_M64 b){x86(818,a,b);}
V pfpnacc(MMREG a,MMREG b){x86(819,a,b);}
V pfpnacc(MMREG a,MEM64 b){x86(819,a,b);}
V pfpnacc(MMREG a,R_M64 b){x86(819,a,b);}
V pfrcp(MMREG a,MMREG b){x86(820,a,b);}
V pfrcp(MMREG a,MEM64 b){x86(820,a,b);}
V pfrcp(MMREG a,R_M64 b){x86(820,a,b);}
V pfrcpit1(MMREG a,MMREG b){x86(821,a,b);}
V pfrcpit1(MMREG a,MEM64 b){x86(821,a,b);}
V pfrcpit1(MMREG a,R_M64 b){x86(821,a,b);}
V pfrcpit2(MMREG a,MMREG b){x86(822,a,b);}
V pfrcpit2(MMREG a,MEM64 b){x86(822,a,b);}
V pfrcpit2(MMREG a,R_M64 b){x86(822,a,b);}
V pfrsqit1(MMREG a,MMREG b){x86(823,a,b);}
V pfrsqit1(MMREG a,MEM64 b){x86(823,a,b);}
V pfrsqit1(MMREG a,R_M64 b){x86(823,a,b);}
V pfrsqrt(MMREG a,MMREG b){x86(824,a,b);}
V pfrsqrt(MMREG a,MEM64 b){x86(824,a,b);}
V pfrsqrt(MMREG a,R_M64 b){x86(824,a,b);}
V pfsub(MMREG a,MMREG b){x86(825,a,b);}
V pfsub(MMREG a,MEM64 b){x86(825,a,b);}
V pfsub(MMREG a,R_M64 b){x86(825,a,b);}
V pfsubr(MMREG a,MMREG b){x86(826,a,b);}
V pfsubr(MMREG a,MEM64 b){x86(826,a,b);}
V pfsubr(MMREG a,R_M64 b){x86(826,a,b);}
V pi2fd(MMREG a,MMREG b){x86(827,a,b);}
V pi2fd(MMREG a,MEM64 b){x86(827,a,b);}
V pi2fd(MMREG a,R_M64 b){x86(827,a,b);}
V pi2fw(MMREG a,MMREG b){x86(828,a,b);}
V pi2fw(MMREG a,MEM64 b){x86(828,a,b);}
V pi2fw(MMREG a,R_M64 b){x86(828,a,b);}
V pinsrw(MMREG a,AX b,char c){x86(829,a,b,(IMM)c);}
V pinsrw(MMREG a,DX b,char c){x86(829,a,b,(IMM)c);}
V pinsrw(MMREG a,CX b,char c){x86(829,a,b,(IMM)c);}
V pinsrw(MMREG a,REG16 b,char c){x86(829,a,b,(IMM)c);}
V pinsrw(MMREG a,MEM16 b,char c){x86(829,a,b,(IMM)c);}
V pinsrw(MMREG a,R_M16 b,char c){x86(829,a,b,(IMM)c);}
V pinsrw(XMMREG a,AX b,char c){x86(830,a,b,(IMM)c);}
V pinsrw(XMMREG a,DX b,char c){x86(830,a,b,(IMM)c);}
V pinsrw(XMMREG a,CX b,char c){x86(830,a,b,(IMM)c);}
V pinsrw(XMMREG a,REG16 b,char c){x86(830,a,b,(IMM)c);}
V pinsrw(XMMREG a,MEM16 b,char c){x86(830,a,b,(IMM)c);}
V pinsrw(XMMREG a,R_M16 b,char c){x86(830,a,b,(IMM)c);}
V pmachriw(MMREG a,MEM64 b){x86(831,a,b);}
V pmaddwd(MMREG a,MMREG b){x86(832,a,b);}
V pmaddwd(MMREG a,MEM64 b){x86(832,a,b);}
V pmaddwd(MMREG a,R_M64 b){x86(832,a,b);}
V pmaddwd(XMMREG a,XMMREG b){x86(833,a,b);}
V pmaddwd(XMMREG a,MEM128 b){x86(833,a,b);}
V pmaddwd(XMMREG a,R_M128 b){x86(833,a,b);}
V pmagw(MMREG a,MMREG b){x86(834,a,b);}
V pmagw(MMREG a,MEM64 b){x86(834,a,b);}
V pmagw(MMREG a,R_M64 b){x86(834,a,b);}
V pmaxsw(XMMREG a,XMMREG b){x86(835,a,b);}
V pmaxsw(XMMREG a,MEM128 b){x86(835,a,b);}
V pmaxsw(XMMREG a,R_M128 b){x86(835,a,b);}
V pmaxsw(MMREG a,MMREG b){x86(836,a,b);}
V pmaxsw(MMREG a,MEM64 b){x86(836,a,b);}
V pmaxsw(MMREG a,R_M64 b){x86(836,a,b);}
V pmaxub(MMREG a,MMREG b){x86(837,a,b);}
V pmaxub(MMREG a,MEM64 b){x86(837,a,b);}
V pmaxub(MMREG a,R_M64 b){x86(837,a,b);}
V pmaxub(XMMREG a,XMMREG b){x86(838,a,b);}
V pmaxub(XMMREG a,MEM128 b){x86(838,a,b);}
V pmaxub(XMMREG a,R_M128 b){x86(838,a,b);}
V pminsw(MMREG a,MMREG b){x86(839,a,b);}
V pminsw(MMREG a,MEM64 b){x86(839,a,b);}
V pminsw(MMREG a,R_M64 b){x86(839,a,b);}
V pminsw(XMMREG a,XMMREG b){x86(840,a,b);}
V pminsw(XMMREG a,MEM128 b){x86(840,a,b);}
V pminsw(XMMREG a,R_M128 b){x86(840,a,b);}
V pminub(MMREG a,MMREG b){x86(841,a,b);}
V pminub(MMREG a,MEM64 b){x86(841,a,b);}
V pminub(MMREG a,R_M64 b){x86(841,a,b);}
V pminub(XMMREG a,XMMREG b){x86(842,a,b);}
V pminub(XMMREG a,MEM128 b){x86(842,a,b);}
V pminub(XMMREG a,R_M128 b){x86(842,a,b);}
V pmovmskb(EAX a,MMREG b){x86(843,a,b);}
V pmovmskb(ECX a,MMREG b){x86(843,a,b);}
V pmovmskb(REG32 a,MMREG b){x86(843,a,b);}
V pmovmskb(EAX a,XMMREG b){x86(844,a,b);}
V pmovmskb(ECX a,XMMREG b){x86(844,a,b);}
V pmovmskb(REG32 a,XMMREG b){x86(844,a,b);}
V pmulhrwa(MMREG a,MMREG b){x86(845,a,b);}
V pmulhrwa(MMREG a,MEM64 b){x86(845,a,b);}
V pmulhrwa(MMREG a,R_M64 b){x86(845,a,b);}
V pmulhrwc(MMREG a,MMREG b){x86(846,a,b);}
V pmulhrwc(MMREG a,MEM64 b){x86(846,a,b);}
V pmulhrwc(MMREG a,R_M64 b){x86(846,a,b);}
V pmulhriw(MMREG a,MMREG b){x86(847,a,b);}
V pmulhriw(MMREG a,MEM64 b){x86(847,a,b);}
V pmulhriw(MMREG a,R_M64 b){x86(847,a,b);}
V pmulhuw(MMREG a,MMREG b){x86(848,a,b);}
V pmulhuw(MMREG a,MEM64 b){x86(848,a,b);}
V pmulhuw(MMREG a,R_M64 b){x86(848,a,b);}
V pmulhuw(XMMREG a,XMMREG b){x86(849,a,b);}
V pmulhuw(XMMREG a,MEM128 b){x86(849,a,b);}
V pmulhuw(XMMREG a,R_M128 b){x86(849,a,b);}
V pmulhw(MMREG a,MMREG b){x86(850,a,b);}
V pmulhw(MMREG a,MEM64 b){x86(850,a,b);}
V pmulhw(MMREG a,R_M64 b){x86(850,a,b);}
V pmullw(MMREG a,MMREG b){x86(851,a,b);}
V pmullw(MMREG a,MEM64 b){x86(851,a,b);}
V pmullw(MMREG a,R_M64 b){x86(851,a,b);}
V pmulhw(XMMREG a,XMMREG b){x86(852,a,b);}
V pmulhw(XMMREG a,MEM128 b){x86(852,a,b);}
V pmulhw(XMMREG a,R_M128 b){x86(852,a,b);}
V pmullw(XMMREG a,XMMREG b){x86(853,a,b);}
V pmullw(XMMREG a,MEM128 b){x86(853,a,b);}
V pmullw(XMMREG a,R_M128 b){x86(853,a,b);}
V pmuludq(MMREG a,MMREG b){x86(854,a,b);}
V pmuludq(MMREG a,MEM64 b){x86(854,a,b);}
V pmuludq(MMREG a,R_M64 b){x86(854,a,b);}
V pmuludq(XMMREG a,XMMREG b){x86(855,a,b);}
V pmuludq(XMMREG a,MEM128 b){x86(855,a,b);}
V pmuludq(XMMREG a,R_M128 b){x86(855,a,b);}
V pmvzb(MMREG a,MEM64 b){x86(856,a,b);}
V pmvnzb(MMREG a,MEM64 b){x86(857,a,b);}
V pmvlzb(MMREG a,MEM64 b){x86(858,a,b);}
V pmvgezb(MMREG a,MEM64 b){x86(859,a,b);}
V pop(AX a){x86(860,a);}
V pop(DX a){x86(860,a);}
V pop(CX a){x86(860,a);}
V pop(REG16 a){x86(860,a);}
V pop(EAX a){x86(861,a);}
V pop(ECX a){x86(861,a);}
V pop(REG32 a){x86(861,a);}
V pop(MEM16 a){x86(862,a);}
V pop(R_M16 a){x86(862,a);}
V pop(MEM32 a){x86(863,a);}
V pop(R_M32 a){x86(863,a);}
V popa(){x86(864);}
V popaw(){x86(865);}
V popad(){x86(866);}
V popf(){x86(867);}
V popfw(){x86(868);}
V popfd(){x86(869);}
V por(MMREG a,MMREG b){x86(870,a,b);}
V por(MMREG a,MEM64 b){x86(870,a,b);}
V por(MMREG a,R_M64 b){x86(870,a,b);}
V por(XMMREG a,XMMREG b){x86(871,a,b);}
V por(XMMREG a,MEM128 b){x86(871,a,b);}
V por(XMMREG a,R_M128 b){x86(871,a,b);}
V prefetch(MEM8 a){x86(872,a);}
V prefetch(MEM16 a){x86(872,a);}
V prefetch(MEM32 a){x86(872,a);}
V prefetch(MEM64 a){x86(872,a);}
V prefetch(MEM128 a){x86(872,a);}
V prefetchw(MEM8 a){x86(873,a);}
V prefetchw(MEM16 a){x86(873,a);}
V prefetchw(MEM32 a){x86(873,a);}
V prefetchw(MEM64 a){x86(873,a);}
V prefetchw(MEM128 a){x86(873,a);}
V prefetchnta(MEM8 a){x86(874,a);}
V prefetchnta(MEM16 a){x86(874,a);}
V prefetchnta(MEM32 a){x86(874,a);}
V prefetchnta(MEM64 a){x86(874,a);}
V prefetchnta(MEM128 a){x86(874,a);}
V prefetcht0(MEM8 a){x86(875,a);}
V prefetcht0(MEM16 a){x86(875,a);}
V prefetcht0(MEM32 a){x86(875,a);}
V prefetcht0(MEM64 a){x86(875,a);}
V prefetcht0(MEM128 a){x86(875,a);}
V prefetcht1(MEM8 a){x86(876,a);}
V prefetcht1(MEM16 a){x86(876,a);}
V prefetcht1(MEM32 a){x86(876,a);}
V prefetcht1(MEM64 a){x86(876,a);}
V prefetcht1(MEM128 a){x86(876,a);}
V prefetcht2(MEM8 a){x86(877,a);}
V prefetcht2(MEM16 a){x86(877,a);}
V prefetcht2(MEM32 a){x86(877,a);}
V prefetcht2(MEM64 a){x86(877,a);}
V prefetcht2(MEM128 a){x86(877,a);}
V psadbw(MMREG a,MMREG b){x86(878,a,b);}
V psadbw(MMREG a,MEM64 b){x86(878,a,b);}
V psadbw(MMREG a,R_M64 b){x86(878,a,b);}
V psadbw(XMMREG a,XMMREG b){x86(879,a,b);}
V psadbw(XMMREG a,MEM128 b){x86(879,a,b);}
V psadbw(XMMREG a,R_M128 b){x86(879,a,b);}
V pshufd(XMMREG a,XMMREG b,char c){x86(880,a,b,(IMM)c);}
V pshufd(XMMREG a,MEM128 b,char c){x86(880,a,b,(IMM)c);}
V pshufd(XMMREG a,R_M128 b,char c){x86(880,a,b,(IMM)c);}
V pshufhw(XMMREG a,XMMREG b,char c){x86(881,a,b,(IMM)c);}
V pshufhw(XMMREG a,MEM128 b,char c){x86(881,a,b,(IMM)c);}
V pshufhw(XMMREG a,R_M128 b,char c){x86(881,a,b,(IMM)c);}
V pshuflw(XMMREG a,XMMREG b,char c){x86(882,a,b,(IMM)c);}
V pshuflw(XMMREG a,MEM128 b,char c){x86(882,a,b,(IMM)c);}
V pshuflw(XMMREG a,R_M128 b,char c){x86(882,a,b,(IMM)c);}
V pshufw(MMREG a,MMREG b,char c){x86(883,a,b,(IMM)c);}
V pshufw(MMREG a,MEM64 b,char c){x86(883,a,b,(IMM)c);}
V pshufw(MMREG a,R_M64 b,char c){x86(883,a,b,(IMM)c);}
V psllw(MMREG a,MMREG b){x86(884,a,b);}
V psllw(MMREG a,MEM64 b){x86(884,a,b);}
V psllw(MMREG a,R_M64 b){x86(884,a,b);}
V psllw(MMREG a,char b){x86(885,a,(IMM)b);}
V psllw(XMMREG a,XMMREG b){x86(886,a,b);}
V psllw(XMMREG a,MEM128 b){x86(886,a,b);}
V psllw(XMMREG a,R_M128 b){x86(886,a,b);}
V psllw(XMMREG a,char b){x86(887,a,(IMM)b);}
V pslld(MMREG a,MMREG b){x86(888,a,b);}
V pslld(MMREG a,MEM64 b){x86(888,a,b);}
V pslld(MMREG a,R_M64 b){x86(888,a,b);}
V pslld(MMREG a,char b){x86(889,a,(IMM)b);}
V pslld(XMMREG a,XMMREG b){x86(890,a,b);}
V pslld(XMMREG a,MEM128 b){x86(890,a,b);}
V pslld(XMMREG a,R_M128 b){x86(890,a,b);}
V pslld(XMMREG a,char b){x86(891,a,(IMM)b);}
V psllq(MMREG a,MMREG b){x86(892,a,b);}
V psllq(MMREG a,MEM64 b){x86(892,a,b);}
V psllq(MMREG a,R_M64 b){x86(892,a,b);}
V psllq(MMREG a,char b){x86(893,a,(IMM)b);}
V psllq(XMMREG a,XMMREG b){x86(894,a,b);}
V psllq(XMMREG a,MEM128 b){x86(894,a,b);}
V psllq(XMMREG a,R_M128 b){x86(894,a,b);}
V psllq(XMMREG a,char b){x86(895,a,(IMM)b);}
V psraw(MMREG a,MMREG b){x86(896,a,b);}
V psraw(MMREG a,MEM64 b){x86(896,a,b);}
V psraw(MMREG a,R_M64 b){x86(896,a,b);}
V psraw(MMREG a,char b){x86(897,a,(IMM)b);}
V psraw(XMMREG a,XMMREG b){x86(898,a,b);}
V psraw(XMMREG a,MEM128 b){x86(898,a,b);}
V psraw(XMMREG a,R_M128 b){x86(898,a,b);}
V psraw(XMMREG a,char b){x86(899,a,(IMM)b);}
V psrad(MMREG a,MMREG b){x86(900,a,b);}
V psrad(MMREG a,MEM64 b){x86(900,a,b);}
V psrad(MMREG a,R_M64 b){x86(900,a,b);}
V psrad(MMREG a,char b){x86(901,a,(IMM)b);}
V psrad(XMMREG a,XMMREG b){x86(902,a,b);}
V psrad(XMMREG a,MEM128 b){x86(902,a,b);}
V psrad(XMMREG a,R_M128 b){x86(902,a,b);}
V psrad(XMMREG a,char b){x86(903,a,(IMM)b);}
V psrlw(MMREG a,MMREG b){x86(904,a,b);}
V psrlw(MMREG a,MEM64 b){x86(904,a,b);}
V psrlw(MMREG a,R_M64 b){x86(904,a,b);}
V psrlw(MMREG a,char b){x86(905,a,(IMM)b);}
V psrlw(XMMREG a,XMMREG b){x86(906,a,b);}
V psrlw(XMMREG a,MEM128 b){x86(906,a,b);}
V psrlw(XMMREG a,R_M128 b){x86(906,a,b);}
V psrlw(XMMREG a,char b){x86(907,a,(IMM)b);}
V psrld(MMREG a,MMREG b){x86(908,a,b);}
V psrld(MMREG a,MEM64 b){x86(908,a,b);}
V psrld(MMREG a,R_M64 b){x86(908,a,b);}
V psrld(MMREG a,char b){x86(909,a,(IMM)b);}
V psrld(XMMREG a,XMMREG b){x86(910,a,b);}
V psrld(XMMREG a,MEM128 b){x86(910,a,b);}
V psrld(XMMREG a,R_M128 b){x86(910,a,b);}
V psrld(XMMREG a,char b){x86(911,a,(IMM)b);}
V psrlq(MMREG a,MMREG b){x86(912,a,b);}
V psrlq(MMREG a,MEM64 b){x86(912,a,b);}
V psrlq(MMREG a,R_M64 b){x86(912,a,b);}
V psrlq(MMREG a,char b){x86(913,a,(IMM)b);}
V psrlq(XMMREG a,XMMREG b){x86(914,a,b);}
V psrlq(XMMREG a,MEM128 b){x86(914,a,b);}
V psrlq(XMMREG a,R_M128 b){x86(914,a,b);}
V psrlq(XMMREG a,char b){x86(915,a,(IMM)b);}
V psrldq(XMMREG a,char b){x86(916,a,(IMM)b);}
V psubb(MMREG a,MMREG b){x86(917,a,b);}
V psubb(MMREG a,MEM64 b){x86(917,a,b);}
V psubb(MMREG a,R_M64 b){x86(917,a,b);}
V psubw(MMREG a,MMREG b){x86(918,a,b);}
V psubw(MMREG a,MEM64 b){x86(918,a,b);}
V psubw(MMREG a,R_M64 b){x86(918,a,b);}
V psubd(MMREG a,MMREG b){x86(919,a,b);}
V psubd(MMREG a,MEM64 b){x86(919,a,b);}
V psubd(MMREG a,R_M64 b){x86(919,a,b);}
V psubq(MMREG a,MMREG b){x86(920,a,b);}
V psubq(MMREG a,MEM64 b){x86(920,a,b);}
V psubq(MMREG a,R_M64 b){x86(920,a,b);}
V psubb(XMMREG a,XMMREG b){x86(921,a,b);}
V psubb(XMMREG a,MEM128 b){x86(921,a,b);}
V psubb(XMMREG a,R_M128 b){x86(921,a,b);}
V psubw(XMMREG a,XMMREG b){x86(922,a,b);}
V psubw(XMMREG a,MEM128 b){x86(922,a,b);}
V psubw(XMMREG a,R_M128 b){x86(922,a,b);}
V psubd(XMMREG a,XMMREG b){x86(923,a,b);}
V psubd(XMMREG a,MEM128 b){x86(923,a,b);}
V psubd(XMMREG a,R_M128 b){x86(923,a,b);}
V psubq(XMMREG a,XMMREG b){x86(924,a,b);}
V psubq(XMMREG a,MEM128 b){x86(924,a,b);}
V psubq(XMMREG a,R_M128 b){x86(924,a,b);}
V psubsb(MMREG a,MMREG b){x86(925,a,b);}
V psubsb(MMREG a,MEM64 b){x86(925,a,b);}
V psubsb(MMREG a,R_M64 b){x86(925,a,b);}
V psubsw(MMREG a,MMREG b){x86(926,a,b);}
V psubsw(MMREG a,MEM64 b){x86(926,a,b);}
V psubsw(MMREG a,R_M64 b){x86(926,a,b);}
V psubsb(XMMREG a,XMMREG b){x86(927,a,b);}
V psubsb(XMMREG a,MEM128 b){x86(927,a,b);}
V psubsb(XMMREG a,R_M128 b){x86(927,a,b);}
V psubsw(XMMREG a,XMMREG b){x86(928,a,b);}
V psubsw(XMMREG a,MEM128 b){x86(928,a,b);}
V psubsw(XMMREG a,R_M128 b){x86(928,a,b);}
V psubusb(MMREG a,MMREG b){x86(929,a,b);}
V psubusb(MMREG a,MEM64 b){x86(929,a,b);}
V psubusb(MMREG a,R_M64 b){x86(929,a,b);}
V psubusw(MMREG a,MMREG b){x86(930,a,b);}
V psubusw(MMREG a,MEM64 b){x86(930,a,b);}
V psubusw(MMREG a,R_M64 b){x86(930,a,b);}
V psubusb(XMMREG a,XMMREG b){x86(931,a,b);}
V psubusb(XMMREG a,MEM128 b){x86(931,a,b);}
V psubusb(XMMREG a,R_M128 b){x86(931,a,b);}
V psubusw(XMMREG a,XMMREG b){x86(932,a,b);}
V psubusw(XMMREG a,MEM128 b){x86(932,a,b);}
V psubusw(XMMREG a,R_M128 b){x86(932,a,b);}
V psubsiw(MMREG a,MMREG b){x86(933,a,b);}
V psubsiw(MMREG a,MEM64 b){x86(933,a,b);}
V psubsiw(MMREG a,R_M64 b){x86(933,a,b);}
V pswapd(MMREG a,MMREG b){x86(934,a,b);}
V pswapd(MMREG a,MEM64 b){x86(934,a,b);}
V pswapd(MMREG a,R_M64 b){x86(934,a,b);}
V punpckhbw(MMREG a,MMREG b){x86(935,a,b);}
V punpckhbw(MMREG a,MEM64 b){x86(935,a,b);}
V punpckhbw(MMREG a,R_M64 b){x86(935,a,b);}
V punpckhwd(MMREG a,MMREG b){x86(936,a,b);}
V punpckhwd(MMREG a,MEM64 b){x86(936,a,b);}
V punpckhwd(MMREG a,R_M64 b){x86(936,a,b);}
V punpckhdq(MMREG a,MMREG b){x86(937,a,b);}
V punpckhdq(MMREG a,MEM64 b){x86(937,a,b);}
V punpckhdq(MMREG a,R_M64 b){x86(937,a,b);}
V punpckhbw(XMMREG a,XMMREG b){x86(938,a,b);}
V punpckhbw(XMMREG a,MEM128 b){x86(938,a,b);}
V punpckhbw(XMMREG a,R_M128 b){x86(938,a,b);}
V punpckhwd(XMMREG a,XMMREG b){x86(939,a,b);}
V punpckhwd(XMMREG a,MEM128 b){x86(939,a,b);}
V punpckhwd(XMMREG a,R_M128 b){x86(939,a,b);}
V punpckhdq(XMMREG a,XMMREG b){x86(940,a,b);}
V punpckhdq(XMMREG a,MEM128 b){x86(940,a,b);}
V punpckhdq(XMMREG a,R_M128 b){x86(940,a,b);}
V punpckhqdq(XMMREG a,XMMREG b){x86(941,a,b);}
V punpckhqdq(XMMREG a,MEM128 b){x86(941,a,b);}
V punpckhqdq(XMMREG a,R_M128 b){x86(941,a,b);}
V punpcklbw(MMREG a,MMREG b){x86(942,a,b);}
V punpcklbw(MMREG a,MEM64 b){x86(942,a,b);}
V punpcklbw(MMREG a,R_M64 b){x86(942,a,b);}
V punpcklwd(MMREG a,MMREG b){x86(943,a,b);}
V punpcklwd(MMREG a,MEM64 b){x86(943,a,b);}
V punpcklwd(MMREG a,R_M64 b){x86(943,a,b);}
V punpckldq(MMREG a,MMREG b){x86(944,a,b);}
V punpckldq(MMREG a,MEM64 b){x86(944,a,b);}
V punpckldq(MMREG a,R_M64 b){x86(944,a,b);}
V punpcklbw(XMMREG a,XMMREG b){x86(945,a,b);}
V punpcklbw(XMMREG a,MEM128 b){x86(945,a,b);}
V punpcklbw(XMMREG a,R_M128 b){x86(945,a,b);}
V punpcklwd(XMMREG a,XMMREG b){x86(946,a,b);}
V punpcklwd(XMMREG a,MEM128 b){x86(946,a,b);}
V punpcklwd(XMMREG a,R_M128 b){x86(946,a,b);}
V punpckldq(XMMREG a,XMMREG b){x86(947,a,b);}
V punpckldq(XMMREG a,MEM128 b){x86(947,a,b);}
V punpckldq(XMMREG a,R_M128 b){x86(947,a,b);}
V punpcklqdq(XMMREG a,XMMREG b){x86(948,a,b);}
V punpcklqdq(XMMREG a,MEM128 b){x86(948,a,b);}
V punpcklqdq(XMMREG a,R_M128 b){x86(948,a,b);}
V push(AX a){x86(949,a);}
V push(DX a){x86(949,a);}
V push(CX a){x86(949,a);}
V push(REG16 a){x86(949,a);}
V push(EAX a){x86(950,a);}
V push(ECX a){x86(950,a);}
V push(REG32 a){x86(950,a);}
V push(MEM16 a){x86(951,a);}
V push(R_M16 a){x86(951,a);}
V push(MEM32 a){x86(952,a);}
V push(R_M32 a){x86(952,a);}
V push(char a){x86(953,(IMM)a);}
V push(short a){x86(954,(IMM)a);}
V push(int a){x86(955,(IMM)a);}
V push(REF a){x86(955,a);}
V pusha(){x86(956);}
V pushad(){x86(957);}
V pushaw(){x86(958);}
V pushf(){x86(959);}
V pushfd(){x86(960);}
V pushfw(){x86(961);}
V pxor(MMREG a,MMREG b){x86(962,a,b);}
V pxor(MMREG a,MEM64 b){x86(962,a,b);}
V pxor(MMREG a,R_M64 b){x86(962,a,b);}
V pxor(XMMREG a,XMMREG b){x86(963,a,b);}
V pxor(XMMREG a,MEM128 b){x86(963,a,b);}
V pxor(XMMREG a,R_M128 b){x86(963,a,b);}
V rcl(AL a,CL b){x86(965,a,b);}
V rcl(CL a,CL b){x86(965,a,b);}
V rcl(REG8 a,CL b){x86(965,a,b);}
V rcl(MEM8 a,CL b){x86(965,a,b);}
V rcl(R_M8 a,CL b){x86(965,a,b);}
V rcl(AL a,char b){x86(966,a,(IMM)b);}
V rcl(CL a,char b){x86(966,a,(IMM)b);}
V rcl(REG8 a,char b){x86(966,a,(IMM)b);}
V rcl(MEM8 a,char b){x86(966,a,(IMM)b);}
V rcl(R_M8 a,char b){x86(966,a,(IMM)b);}
V rcl(AX a,CL b){x86(968,a,b);}
V rcl(DX a,CL b){x86(968,a,b);}
V rcl(CX a,CL b){x86(968,a,b);}
V rcl(REG16 a,CL b){x86(968,a,b);}
V rcl(MEM16 a,CL b){x86(968,a,b);}
V rcl(R_M16 a,CL b){x86(968,a,b);}
V rcl(EAX a,CL b){x86(971,a,b);}
V rcl(ECX a,CL b){x86(971,a,b);}
V rcl(REG32 a,CL b){x86(971,a,b);}
V rcl(MEM32 a,CL b){x86(971,a,b);}
V rcl(R_M32 a,CL b){x86(971,a,b);}
V rcr(AL a,CL b){x86(974,a,b);}
V rcr(CL a,CL b){x86(974,a,b);}
V rcr(REG8 a,CL b){x86(974,a,b);}
V rcr(MEM8 a,CL b){x86(974,a,b);}
V rcr(R_M8 a,CL b){x86(974,a,b);}
V rcr(AL a,char b){x86(975,a,(IMM)b);}
V rcr(CL a,char b){x86(975,a,(IMM)b);}
V rcr(REG8 a,char b){x86(975,a,(IMM)b);}
V rcr(MEM8 a,char b){x86(975,a,(IMM)b);}
V rcr(R_M8 a,char b){x86(975,a,(IMM)b);}
V rcr(AX a,CL b){x86(977,a,b);}
V rcr(DX a,CL b){x86(977,a,b);}
V rcr(CX a,CL b){x86(977,a,b);}
V rcr(REG16 a,CL b){x86(977,a,b);}
V rcr(MEM16 a,CL b){x86(977,a,b);}
V rcr(R_M16 a,CL b){x86(977,a,b);}
V rcr(EAX a,CL b){x86(980,a,b);}
V rcr(ECX a,CL b){x86(980,a,b);}
V rcr(REG32 a,CL b){x86(980,a,b);}
V rcr(MEM32 a,CL b){x86(980,a,b);}
V rcr(R_M32 a,CL b){x86(980,a,b);}
V rcpps(XMMREG a,XMMREG b){x86(982,a,b);}
V rcpps(XMMREG a,MEM128 b){x86(982,a,b);}
V rcpps(XMMREG a,R_M128 b){x86(982,a,b);}
V rcpss(XMMREG a,XMMREG b){x86(983,a,b);}
V rcpss(XMMREG a,MEM32 b){x86(983,a,b);}
V rcpss(XMMREG a,XMM32 b){x86(983,a,b);}
V rdmsr(){x86(984);}
V rdpmc(){x86(985);}
V rdtsc(){x86(986);}
V ret(){x86(987);}
V ret(char a){x86(988,(IMM)a);}
V ret(short a){x86(988,(IMM)a);}
V retf(){x86(989);}
V retf(char a){x86(990,(IMM)a);}
V retf(short a){x86(990,(IMM)a);}
V retn(){x86(991);}
V retn(char a){x86(992,(IMM)a);}
V retn(short a){x86(992,(IMM)a);}
V rol(AL a,CL b){x86(994,a,b);}
V rol(CL a,CL b){x86(994,a,b);}
V rol(REG8 a,CL b){x86(994,a,b);}
V rol(MEM8 a,CL b){x86(994,a,b);}
V rol(R_M8 a,CL b){x86(994,a,b);}
V rol(AL a,char b){x86(995,a,(IMM)b);}
V rol(CL a,char b){x86(995,a,(IMM)b);}
V rol(REG8 a,char b){x86(995,a,(IMM)b);}
V rol(MEM8 a,char b){x86(995,a,(IMM)b);}
V rol(R_M8 a,char b){x86(995,a,(IMM)b);}
V rol(AX a,CL b){x86(997,a,b);}
V rol(DX a,CL b){x86(997,a,b);}
V rol(CX a,CL b){x86(997,a,b);}
V rol(REG16 a,CL b){x86(997,a,b);}
V rol(MEM16 a,CL b){x86(997,a,b);}
V rol(R_M16 a,CL b){x86(997,a,b);}
V rol(EAX a,CL b){x86(1000,a,b);}
V rol(ECX a,CL b){x86(1000,a,b);}
V rol(REG32 a,CL b){x86(1000,a,b);}
V rol(MEM32 a,CL b){x86(1000,a,b);}
V rol(R_M32 a,CL b){x86(1000,a,b);}
V ror(AL a,CL b){x86(1003,a,b);}
V ror(CL a,CL b){x86(1003,a,b);}
V ror(REG8 a,CL b){x86(1003,a,b);}
V ror(MEM8 a,CL b){x86(1003,a,b);}
V ror(R_M8 a,CL b){x86(1003,a,b);}
V ror(AL a,char b){x86(1004,a,(IMM)b);}
V ror(CL a,char b){x86(1004,a,(IMM)b);}
V ror(REG8 a,char b){x86(1004,a,(IMM)b);}
V ror(MEM8 a,char b){x86(1004,a,(IMM)b);}
V ror(R_M8 a,char b){x86(1004,a,(IMM)b);}
V ror(AX a,CL b){x86(1006,a,b);}
V ror(DX a,CL b){x86(1006,a,b);}
V ror(CX a,CL b){x86(1006,a,b);}
V ror(REG16 a,CL b){x86(1006,a,b);}
V ror(MEM16 a,CL b){x86(1006,a,b);}
V ror(R_M16 a,CL b){x86(1006,a,b);}
V ror(EAX a,CL b){x86(1009,a,b);}
V ror(ECX a,CL b){x86(1009,a,b);}
V ror(REG32 a,CL b){x86(1009,a,b);}
V ror(MEM32 a,CL b){x86(1009,a,b);}
V ror(R_M32 a,CL b){x86(1009,a,b);}
V rsm(){x86(1011);}
V rsqrtps(XMMREG a,XMMREG b){x86(1012,a,b);}
V rsqrtps(XMMREG a,MEM128 b){x86(1012,a,b);}
V rsqrtps(XMMREG a,R_M128 b){x86(1012,a,b);}
V rsqrtss(XMMREG a,XMMREG b){x86(1013,a,b);}
V rsqrtss(XMMREG a,MEM32 b){x86(1013,a,b);}
V rsqrtss(XMMREG a,XMM32 b){x86(1013,a,b);}
V sahf(){x86(1014);}
V sal(AL a,CL b){x86(1016,a,b);}
V sal(CL a,CL b){x86(1016,a,b);}
V sal(REG8 a,CL b){x86(1016,a,b);}
V sal(MEM8 a,CL b){x86(1016,a,b);}
V sal(R_M8 a,CL b){x86(1016,a,b);}
V sal(AL a,char b){x86(1017,a,(IMM)b);}
V sal(CL a,char b){x86(1017,a,(IMM)b);}
V sal(REG8 a,char b){x86(1017,a,(IMM)b);}
V sal(MEM8 a,char b){x86(1017,a,(IMM)b);}
V sal(R_M8 a,char b){x86(1017,a,(IMM)b);}
V sal(AX a,CL b){x86(1019,a,b);}
V sal(DX a,CL b){x86(1019,a,b);}
V sal(CX a,CL b){x86(1019,a,b);}
V sal(REG16 a,CL b){x86(1019,a,b);}
V sal(MEM16 a,CL b){x86(1019,a,b);}
V sal(R_M16 a,CL b){x86(1019,a,b);}
V sal(EAX a,CL b){x86(1022,a,b);}
V sal(ECX a,CL b){x86(1022,a,b);}
V sal(REG32 a,CL b){x86(1022,a,b);}
V sal(MEM32 a,CL b){x86(1022,a,b);}
V sal(R_M32 a,CL b){x86(1022,a,b);}
V sar(AL a,CL b){x86(1025,a,b);}
V sar(CL a,CL b){x86(1025,a,b);}
V sar(REG8 a,CL b){x86(1025,a,b);}
V sar(MEM8 a,CL b){x86(1025,a,b);}
V sar(R_M8 a,CL b){x86(1025,a,b);}
V sar(AL a,char b){x86(1026,a,(IMM)b);}
V sar(CL a,char b){x86(1026,a,(IMM)b);}
V sar(REG8 a,char b){x86(1026,a,(IMM)b);}
V sar(MEM8 a,char b){x86(1026,a,(IMM)b);}
V sar(R_M8 a,char b){x86(1026,a,(IMM)b);}
V sar(AX a,CL b){x86(1028,a,b);}
V sar(DX a,CL b){x86(1028,a,b);}
V sar(CX a,CL b){x86(1028,a,b);}
V sar(REG16 a,CL b){x86(1028,a,b);}
V sar(MEM16 a,CL b){x86(1028,a,b);}
V sar(R_M16 a,CL b){x86(1028,a,b);}
V sar(EAX a,CL b){x86(1031,a,b);}
V sar(ECX a,CL b){x86(1031,a,b);}
V sar(REG32 a,CL b){x86(1031,a,b);}
V sar(MEM32 a,CL b){x86(1031,a,b);}
V sar(R_M32 a,CL b){x86(1031,a,b);}
V sbb(AL a,AL b){x86(1033,a,b);}
V sbb(AL a,CL b){x86(1033,a,b);}
V sbb(AL a,REG8 b){x86(1033,a,b);}
V sbb(CL a,AL b){x86(1033,a,b);}
V sbb(CL a,CL b){x86(1033,a,b);}
V sbb(CL a,REG8 b){x86(1033,a,b);}
V sbb(REG8 a,AL b){x86(1033,a,b);}
V sbb(REG8 a,CL b){x86(1033,a,b);}
V sbb(REG8 a,REG8 b){x86(1033,a,b);}
V sbb(MEM8 a,AL b){x86(1033,a,b);}
V sbb(MEM8 a,CL b){x86(1033,a,b);}
V sbb(MEM8 a,REG8 b){x86(1033,a,b);}
V sbb(R_M8 a,AL b){x86(1033,a,b);}
V sbb(R_M8 a,CL b){x86(1033,a,b);}
V sbb(R_M8 a,REG8 b){x86(1033,a,b);}
V sbb(AX a,AX b){x86(1034,a,b);}
V sbb(AX a,DX b){x86(1034,a,b);}
V sbb(AX a,CX b){x86(1034,a,b);}
V sbb(AX a,REG16 b){x86(1034,a,b);}
V sbb(DX a,AX b){x86(1034,a,b);}
V sbb(DX a,DX b){x86(1034,a,b);}
V sbb(DX a,CX b){x86(1034,a,b);}
V sbb(DX a,REG16 b){x86(1034,a,b);}
V sbb(CX a,AX b){x86(1034,a,b);}
V sbb(CX a,DX b){x86(1034,a,b);}
V sbb(CX a,CX b){x86(1034,a,b);}
V sbb(CX a,REG16 b){x86(1034,a,b);}
V sbb(REG16 a,AX b){x86(1034,a,b);}
V sbb(REG16 a,DX b){x86(1034,a,b);}
V sbb(REG16 a,CX b){x86(1034,a,b);}
V sbb(REG16 a,REG16 b){x86(1034,a,b);}
V sbb(MEM16 a,AX b){x86(1034,a,b);}
V sbb(MEM16 a,DX b){x86(1034,a,b);}
V sbb(MEM16 a,CX b){x86(1034,a,b);}
V sbb(MEM16 a,REG16 b){x86(1034,a,b);}
V sbb(R_M16 a,AX b){x86(1034,a,b);}
V sbb(R_M16 a,DX b){x86(1034,a,b);}
V sbb(R_M16 a,CX b){x86(1034,a,b);}
V sbb(R_M16 a,REG16 b){x86(1034,a,b);}
V sbb(EAX a,EAX b){x86(1035,a,b);}
V sbb(EAX a,ECX b){x86(1035,a,b);}
V sbb(EAX a,REG32 b){x86(1035,a,b);}
V sbb(ECX a,EAX b){x86(1035,a,b);}
V sbb(ECX a,ECX b){x86(1035,a,b);}
V sbb(ECX a,REG32 b){x86(1035,a,b);}
V sbb(REG32 a,EAX b){x86(1035,a,b);}
V sbb(REG32 a,ECX b){x86(1035,a,b);}
V sbb(REG32 a,REG32 b){x86(1035,a,b);}
V sbb(MEM32 a,EAX b){x86(1035,a,b);}
V sbb(MEM32 a,ECX b){x86(1035,a,b);}
V sbb(MEM32 a,REG32 b){x86(1035,a,b);}
V sbb(R_M32 a,EAX b){x86(1035,a,b);}
V sbb(R_M32 a,ECX b){x86(1035,a,b);}
V sbb(R_M32 a,REG32 b){x86(1035,a,b);}
V lock_sbb(MEM8 a,AL b){x86(1036,a,b);}
V lock_sbb(MEM8 a,CL b){x86(1036,a,b);}
V lock_sbb(MEM8 a,REG8 b){x86(1036,a,b);}
V lock_sbb(MEM16 a,AX b){x86(1037,a,b);}
V lock_sbb(MEM16 a,DX b){x86(1037,a,b);}
V lock_sbb(MEM16 a,CX b){x86(1037,a,b);}
V lock_sbb(MEM16 a,REG16 b){x86(1037,a,b);}
V lock_sbb(MEM32 a,EAX b){x86(1038,a,b);}
V lock_sbb(MEM32 a,ECX b){x86(1038,a,b);}
V lock_sbb(MEM32 a,REG32 b){x86(1038,a,b);}
V sbb(AL a,MEM8 b){x86(1039,a,b);}
V sbb(AL a,R_M8 b){x86(1039,a,b);}
V sbb(CL a,MEM8 b){x86(1039,a,b);}
V sbb(CL a,R_M8 b){x86(1039,a,b);}
V sbb(REG8 a,MEM8 b){x86(1039,a,b);}
V sbb(REG8 a,R_M8 b){x86(1039,a,b);}
V sbb(AX a,MEM16 b){x86(1040,a,b);}
V sbb(AX a,R_M16 b){x86(1040,a,b);}
V sbb(DX a,MEM16 b){x86(1040,a,b);}
V sbb(DX a,R_M16 b){x86(1040,a,b);}
V sbb(CX a,MEM16 b){x86(1040,a,b);}
V sbb(CX a,R_M16 b){x86(1040,a,b);}
V sbb(REG16 a,MEM16 b){x86(1040,a,b);}
V sbb(REG16 a,R_M16 b){x86(1040,a,b);}
V sbb(EAX a,MEM32 b){x86(1041,a,b);}
V sbb(EAX a,R_M32 b){x86(1041,a,b);}
V sbb(ECX a,MEM32 b){x86(1041,a,b);}
V sbb(ECX a,R_M32 b){x86(1041,a,b);}
V sbb(REG32 a,MEM32 b){x86(1041,a,b);}
V sbb(REG32 a,R_M32 b){x86(1041,a,b);}
V sbb(AL a,char b){x86(1042,a,(IMM)b);}
V sbb(CL a,char b){x86(1042,a,(IMM)b);}
V sbb(REG8 a,char b){x86(1042,a,(IMM)b);}
V sbb(MEM8 a,char b){x86(1042,a,(IMM)b);}
V sbb(R_M8 a,char b){x86(1042,a,(IMM)b);}
V sbb(AX a,char b){x86(1043,a,(IMM)b);}
V sbb(AX a,short b){x86(1043,a,(IMM)b);}
V sbb(DX a,char b){x86(1043,a,(IMM)b);}
V sbb(DX a,short b){x86(1043,a,(IMM)b);}
V sbb(CX a,char b){x86(1043,a,(IMM)b);}
V sbb(CX a,short b){x86(1043,a,(IMM)b);}
V sbb(REG16 a,char b){x86(1043,a,(IMM)b);}
V sbb(REG16 a,short b){x86(1043,a,(IMM)b);}
V sbb(MEM16 a,char b){x86(1043,a,(IMM)b);}
V sbb(MEM16 a,short b){x86(1043,a,(IMM)b);}
V sbb(R_M16 a,char b){x86(1043,a,(IMM)b);}
V sbb(R_M16 a,short b){x86(1043,a,(IMM)b);}
V sbb(EAX a,int b){x86(1044,a,(IMM)b);}
V sbb(EAX a,char b){x86(1044,a,(IMM)b);}
V sbb(EAX a,short b){x86(1044,a,(IMM)b);}
V sbb(EAX a,REF b){x86(1044,a,b);}
V sbb(ECX a,int b){x86(1044,a,(IMM)b);}
V sbb(ECX a,char b){x86(1044,a,(IMM)b);}
V sbb(ECX a,short b){x86(1044,a,(IMM)b);}
V sbb(ECX a,REF b){x86(1044,a,b);}
V sbb(REG32 a,int b){x86(1044,a,(IMM)b);}
V sbb(REG32 a,char b){x86(1044,a,(IMM)b);}
V sbb(REG32 a,short b){x86(1044,a,(IMM)b);}
V sbb(REG32 a,REF b){x86(1044,a,b);}
V sbb(MEM32 a,int b){x86(1044,a,(IMM)b);}
V sbb(MEM32 a,char b){x86(1044,a,(IMM)b);}
V sbb(MEM32 a,short b){x86(1044,a,(IMM)b);}
V sbb(MEM32 a,REF b){x86(1044,a,b);}
V sbb(R_M32 a,int b){x86(1044,a,(IMM)b);}
V sbb(R_M32 a,char b){x86(1044,a,(IMM)b);}
V sbb(R_M32 a,short b){x86(1044,a,(IMM)b);}
V sbb(R_M32 a,REF b){x86(1044,a,b);}
V lock_sbb(MEM8 a,char b){x86(1047,a,(IMM)b);}
V lock_sbb(MEM16 a,char b){x86(1048,a,(IMM)b);}
V lock_sbb(MEM16 a,short b){x86(1048,a,(IMM)b);}
V lock_sbb(MEM32 a,int b){x86(1049,a,(IMM)b);}
V lock_sbb(MEM32 a,char b){x86(1049,a,(IMM)b);}
V lock_sbb(MEM32 a,short b){x86(1049,a,(IMM)b);}
V lock_sbb(MEM32 a,REF b){x86(1049,a,b);}
V scasb(){x86(1055);}
V scasw(){x86(1056);}
V scasd(){x86(1057);}
V rep_scasb(){x86(1058);}
V rep_scasw(){x86(1059);}
V rep_scasd(){x86(1060);}
V repe_scasb(){x86(1061);}
V repe_scasw(){x86(1062);}
V repe_scasd(){x86(1063);}
V repne_scasb(){x86(1064);}
V repne_scasw(){x86(1065);}
V repne_scasd(){x86(1066);}
V repz_scasb(){x86(1067);}
V repz_scasw(){x86(1068);}
V repz_scasd(){x86(1069);}
V repnz_scasb(){x86(1070);}
V repnz_scasw(){x86(1071);}
V repnz_scasd(){x86(1072);}
V seto(AL a){x86(1073,a);}
V seto(CL a){x86(1073,a);}
V seto(REG8 a){x86(1073,a);}
V seto(MEM8 a){x86(1073,a);}
V seto(R_M8 a){x86(1073,a);}
V setno(AL a){x86(1074,a);}
V setno(CL a){x86(1074,a);}
V setno(REG8 a){x86(1074,a);}
V setno(MEM8 a){x86(1074,a);}
V setno(R_M8 a){x86(1074,a);}
V setb(AL a){x86(1075,a);}
V setb(CL a){x86(1075,a);}
V setb(REG8 a){x86(1075,a);}
V setb(MEM8 a){x86(1075,a);}
V setb(R_M8 a){x86(1075,a);}
V setc(AL a){x86(1076,a);}
V setc(CL a){x86(1076,a);}
V setc(REG8 a){x86(1076,a);}
V setc(MEM8 a){x86(1076,a);}
V setc(R_M8 a){x86(1076,a);}
V setnea(AL a){x86(1077,a);}
V setnea(CL a){x86(1077,a);}
V setnea(REG8 a){x86(1077,a);}
V setnea(MEM8 a){x86(1077,a);}
V setnea(R_M8 a){x86(1077,a);}
V setae(AL a){x86(1078,a);}
V setae(CL a){x86(1078,a);}
V setae(REG8 a){x86(1078,a);}
V setae(MEM8 a){x86(1078,a);}
V setae(R_M8 a){x86(1078,a);}
V setnb(AL a){x86(1079,a);}
V setnb(CL a){x86(1079,a);}
V setnb(REG8 a){x86(1079,a);}
V setnb(MEM8 a){x86(1079,a);}
V setnb(R_M8 a){x86(1079,a);}
V setnc(AL a){x86(1080,a);}
V setnc(CL a){x86(1080,a);}
V setnc(REG8 a){x86(1080,a);}
V setnc(MEM8 a){x86(1080,a);}
V setnc(R_M8 a){x86(1080,a);}
V sete(AL a){x86(1081,a);}
V sete(CL a){x86(1081,a);}
V sete(REG8 a){x86(1081,a);}
V sete(MEM8 a){x86(1081,a);}
V sete(R_M8 a){x86(1081,a);}
V setz(AL a){x86(1082,a);}
V setz(CL a){x86(1082,a);}
V setz(REG8 a){x86(1082,a);}
V setz(MEM8 a){x86(1082,a);}
V setz(R_M8 a){x86(1082,a);}
V setne(AL a){x86(1083,a);}
V setne(CL a){x86(1083,a);}
V setne(REG8 a){x86(1083,a);}
V setne(MEM8 a){x86(1083,a);}
V setne(R_M8 a){x86(1083,a);}
V setnz(AL a){x86(1084,a);}
V setnz(CL a){x86(1084,a);}
V setnz(REG8 a){x86(1084,a);}
V setnz(MEM8 a){x86(1084,a);}
V setnz(R_M8 a){x86(1084,a);}
V setbe(AL a){x86(1085,a);}
V setbe(CL a){x86(1085,a);}
V setbe(REG8 a){x86(1085,a);}
V setbe(MEM8 a){x86(1085,a);}
V setbe(R_M8 a){x86(1085,a);}
V setna(AL a){x86(1086,a);}
V setna(CL a){x86(1086,a);}
V setna(REG8 a){x86(1086,a);}
V setna(MEM8 a){x86(1086,a);}
V setna(R_M8 a){x86(1086,a);}
V seta(AL a){x86(1087,a);}
V seta(CL a){x86(1087,a);}
V seta(REG8 a){x86(1087,a);}
V seta(MEM8 a){x86(1087,a);}
V seta(R_M8 a){x86(1087,a);}
V setnbe(AL a){x86(1088,a);}
V setnbe(CL a){x86(1088,a);}
V setnbe(REG8 a){x86(1088,a);}
V setnbe(MEM8 a){x86(1088,a);}
V setnbe(R_M8 a){x86(1088,a);}
V sets(AL a){x86(1089,a);}
V sets(CL a){x86(1089,a);}
V sets(REG8 a){x86(1089,a);}
V sets(MEM8 a){x86(1089,a);}
V sets(R_M8 a){x86(1089,a);}
V setns(AL a){x86(1090,a);}
V setns(CL a){x86(1090,a);}
V setns(REG8 a){x86(1090,a);}
V setns(MEM8 a){x86(1090,a);}
V setns(R_M8 a){x86(1090,a);}
V setp(AL a){x86(1091,a);}
V setp(CL a){x86(1091,a);}
V setp(REG8 a){x86(1091,a);}
V setp(MEM8 a){x86(1091,a);}
V setp(R_M8 a){x86(1091,a);}
V setpe(AL a){x86(1092,a);}
V setpe(CL a){x86(1092,a);}
V setpe(REG8 a){x86(1092,a);}
V setpe(MEM8 a){x86(1092,a);}
V setpe(R_M8 a){x86(1092,a);}
V setnp(AL a){x86(1093,a);}
V setnp(CL a){x86(1093,a);}
V setnp(REG8 a){x86(1093,a);}
V setnp(MEM8 a){x86(1093,a);}
V setnp(R_M8 a){x86(1093,a);}
V setpo(AL a){x86(1094,a);}
V setpo(CL a){x86(1094,a);}
V setpo(REG8 a){x86(1094,a);}
V setpo(MEM8 a){x86(1094,a);}
V setpo(R_M8 a){x86(1094,a);}
V setl(AL a){x86(1095,a);}
V setl(CL a){x86(1095,a);}
V setl(REG8 a){x86(1095,a);}
V setl(MEM8 a){x86(1095,a);}
V setl(R_M8 a){x86(1095,a);}
V setnge(AL a){x86(1096,a);}
V setnge(CL a){x86(1096,a);}
V setnge(REG8 a){x86(1096,a);}
V setnge(MEM8 a){x86(1096,a);}
V setnge(R_M8 a){x86(1096,a);}
V setge(AL a){x86(1097,a);}
V setge(CL a){x86(1097,a);}
V setge(REG8 a){x86(1097,a);}
V setge(MEM8 a){x86(1097,a);}
V setge(R_M8 a){x86(1097,a);}
V setnl(AL a){x86(1098,a);}
V setnl(CL a){x86(1098,a);}
V setnl(REG8 a){x86(1098,a);}
V setnl(MEM8 a){x86(1098,a);}
V setnl(R_M8 a){x86(1098,a);}
V setle(AL a){x86(1099,a);}
V setle(CL a){x86(1099,a);}
V setle(REG8 a){x86(1099,a);}
V setle(MEM8 a){x86(1099,a);}
V setle(R_M8 a){x86(1099,a);}
V setng(AL a){x86(1100,a);}
V setng(CL a){x86(1100,a);}
V setng(REG8 a){x86(1100,a);}
V setng(MEM8 a){x86(1100,a);}
V setng(R_M8 a){x86(1100,a);}
V setg(AL a){x86(1101,a);}
V setg(CL a){x86(1101,a);}
V setg(REG8 a){x86(1101,a);}
V setg(MEM8 a){x86(1101,a);}
V setg(R_M8 a){x86(1101,a);}
V setnle(AL a){x86(1102,a);}
V setnle(CL a){x86(1102,a);}
V setnle(REG8 a){x86(1102,a);}
V setnle(MEM8 a){x86(1102,a);}
V setnle(R_M8 a){x86(1102,a);}
V sfence(){x86(1103);}
V shl(AL a,CL b){x86(1105,a,b);}
V shl(CL a,CL b){x86(1105,a,b);}
V shl(REG8 a,CL b){x86(1105,a,b);}
V shl(MEM8 a,CL b){x86(1105,a,b);}
V shl(R_M8 a,CL b){x86(1105,a,b);}
V shl(AL a,char b){x86(1106,a,(IMM)b);}
V shl(CL a,char b){x86(1106,a,(IMM)b);}
V shl(REG8 a,char b){x86(1106,a,(IMM)b);}
V shl(MEM8 a,char b){x86(1106,a,(IMM)b);}
V shl(R_M8 a,char b){x86(1106,a,(IMM)b);}
V shl(AX a,CL b){x86(1108,a,b);}
V shl(DX a,CL b){x86(1108,a,b);}
V shl(CX a,CL b){x86(1108,a,b);}
V shl(REG16 a,CL b){x86(1108,a,b);}
V shl(MEM16 a,CL b){x86(1108,a,b);}
V shl(R_M16 a,CL b){x86(1108,a,b);}
V shl(EAX a,CL b){x86(1111,a,b);}
V shl(ECX a,CL b){x86(1111,a,b);}
V shl(REG32 a,CL b){x86(1111,a,b);}
V shl(MEM32 a,CL b){x86(1111,a,b);}
V shl(R_M32 a,CL b){x86(1111,a,b);}
V shr(AL a,CL b){x86(1114,a,b);}
V shr(CL a,CL b){x86(1114,a,b);}
V shr(REG8 a,CL b){x86(1114,a,b);}
V shr(MEM8 a,CL b){x86(1114,a,b);}
V shr(R_M8 a,CL b){x86(1114,a,b);}
V shr(AL a,char b){x86(1115,a,(IMM)b);}
V shr(CL a,char b){x86(1115,a,(IMM)b);}
V shr(REG8 a,char b){x86(1115,a,(IMM)b);}
V shr(MEM8 a,char b){x86(1115,a,(IMM)b);}
V shr(R_M8 a,char b){x86(1115,a,(IMM)b);}
V shr(AX a,CL b){x86(1117,a,b);}
V shr(DX a,CL b){x86(1117,a,b);}
V shr(CX a,CL b){x86(1117,a,b);}
V shr(REG16 a,CL b){x86(1117,a,b);}
V shr(MEM16 a,CL b){x86(1117,a,b);}
V shr(R_M16 a,CL b){x86(1117,a,b);}
V shr(EAX a,CL b){x86(1120,a,b);}
V shr(ECX a,CL b){x86(1120,a,b);}
V shr(REG32 a,CL b){x86(1120,a,b);}
V shr(MEM32 a,CL b){x86(1120,a,b);}
V shr(R_M32 a,CL b){x86(1120,a,b);}
V shld(AX a,AX b,char c){x86(1122,a,b,(IMM)c);}
V shld(AX a,DX b,char c){x86(1122,a,b,(IMM)c);}
V shld(AX a,CX b,char c){x86(1122,a,b,(IMM)c);}
V shld(AX a,REG16 b,char c){x86(1122,a,b,(IMM)c);}
V shld(DX a,AX b,char c){x86(1122,a,b,(IMM)c);}
V shld(DX a,DX b,char c){x86(1122,a,b,(IMM)c);}
V shld(DX a,CX b,char c){x86(1122,a,b,(IMM)c);}
V shld(DX a,REG16 b,char c){x86(1122,a,b,(IMM)c);}
V shld(CX a,AX b,char c){x86(1122,a,b,(IMM)c);}
V shld(CX a,DX b,char c){x86(1122,a,b,(IMM)c);}
V shld(CX a,CX b,char c){x86(1122,a,b,(IMM)c);}
V shld(CX a,REG16 b,char c){x86(1122,a,b,(IMM)c);}
V shld(REG16 a,AX b,char c){x86(1122,a,b,(IMM)c);}
V shld(REG16 a,DX b,char c){x86(1122,a,b,(IMM)c);}
V shld(REG16 a,CX b,char c){x86(1122,a,b,(IMM)c);}
V shld(REG16 a,REG16 b,char c){x86(1122,a,b,(IMM)c);}
V shld(MEM16 a,AX b,char c){x86(1122,a,b,(IMM)c);}
V shld(MEM16 a,DX b,char c){x86(1122,a,b,(IMM)c);}
V shld(MEM16 a,CX b,char c){x86(1122,a,b,(IMM)c);}
V shld(MEM16 a,REG16 b,char c){x86(1122,a,b,(IMM)c);}
V shld(R_M16 a,AX b,char c){x86(1122,a,b,(IMM)c);}
V shld(R_M16 a,DX b,char c){x86(1122,a,b,(IMM)c);}
V shld(R_M16 a,CX b,char c){x86(1122,a,b,(IMM)c);}
V shld(R_M16 a,REG16 b,char c){x86(1122,a,b,(IMM)c);}
V shld(EAX a,EAX b,char c){x86(1123,a,b,(IMM)c);}
V shld(EAX a,ECX b,char c){x86(1123,a,b,(IMM)c);}
V shld(EAX a,REG32 b,char c){x86(1123,a,b,(IMM)c);}
V shld(ECX a,EAX b,char c){x86(1123,a,b,(IMM)c);}
V shld(ECX a,ECX b,char c){x86(1123,a,b,(IMM)c);}
V shld(ECX a,REG32 b,char c){x86(1123,a,b,(IMM)c);}
V shld(REG32 a,EAX b,char c){x86(1123,a,b,(IMM)c);}
V shld(REG32 a,ECX b,char c){x86(1123,a,b,(IMM)c);}
V shld(REG32 a,REG32 b,char c){x86(1123,a,b,(IMM)c);}
V shld(MEM32 a,EAX b,char c){x86(1123,a,b,(IMM)c);}
V shld(MEM32 a,ECX b,char c){x86(1123,a,b,(IMM)c);}
V shld(MEM32 a,REG32 b,char c){x86(1123,a,b,(IMM)c);}
V shld(R_M32 a,EAX b,char c){x86(1123,a,b,(IMM)c);}
V shld(R_M32 a,ECX b,char c){x86(1123,a,b,(IMM)c);}
V shld(R_M32 a,REG32 b,char c){x86(1123,a,b,(IMM)c);}
V shld(AX a,AX b,CL c){x86(1124,a,b,c);}
V shld(AX a,DX b,CL c){x86(1124,a,b,c);}
V shld(AX a,CX b,CL c){x86(1124,a,b,c);}
V shld(AX a,REG16 b,CL c){x86(1124,a,b,c);}
V shld(DX a,AX b,CL c){x86(1124,a,b,c);}
V shld(DX a,DX b,CL c){x86(1124,a,b,c);}
V shld(DX a,CX b,CL c){x86(1124,a,b,c);}
V shld(DX a,REG16 b,CL c){x86(1124,a,b,c);}
V shld(CX a,AX b,CL c){x86(1124,a,b,c);}
V shld(CX a,DX b,CL c){x86(1124,a,b,c);}
V shld(CX a,CX b,CL c){x86(1124,a,b,c);}
V shld(CX a,REG16 b,CL c){x86(1124,a,b,c);}
V shld(REG16 a,AX b,CL c){x86(1124,a,b,c);}
V shld(REG16 a,DX b,CL c){x86(1124,a,b,c);}
V shld(REG16 a,CX b,CL c){x86(1124,a,b,c);}
V shld(REG16 a,REG16 b,CL c){x86(1124,a,b,c);}
V shld(MEM16 a,AX b,CL c){x86(1124,a,b,c);}
V shld(MEM16 a,DX b,CL c){x86(1124,a,b,c);}
V shld(MEM16 a,CX b,CL c){x86(1124,a,b,c);}
V shld(MEM16 a,REG16 b,CL c){x86(1124,a,b,c);}
V shld(R_M16 a,AX b,CL c){x86(1124,a,b,c);}
V shld(R_M16 a,DX b,CL c){x86(1124,a,b,c);}
V shld(R_M16 a,CX b,CL c){x86(1124,a,b,c);}
V shld(R_M16 a,REG16 b,CL c){x86(1124,a,b,c);}
V shld(EAX a,EAX b,CL c){x86(1125,a,b,c);}
V shld(EAX a,ECX b,CL c){x86(1125,a,b,c);}
V shld(EAX a,REG32 b,CL c){x86(1125,a,b,c);}
V shld(ECX a,EAX b,CL c){x86(1125,a,b,c);}
V shld(ECX a,ECX b,CL c){x86(1125,a,b,c);}
V shld(ECX a,REG32 b,CL c){x86(1125,a,b,c);}
V shld(REG32 a,EAX b,CL c){x86(1125,a,b,c);}
V shld(REG32 a,ECX b,CL c){x86(1125,a,b,c);}
V shld(REG32 a,REG32 b,CL c){x86(1125,a,b,c);}
V shld(MEM32 a,EAX b,CL c){x86(1125,a,b,c);}
V shld(MEM32 a,ECX b,CL c){x86(1125,a,b,c);}
V shld(MEM32 a,REG32 b,CL c){x86(1125,a,b,c);}
V shld(R_M32 a,EAX b,CL c){x86(1125,a,b,c);}
V shld(R_M32 a,ECX b,CL c){x86(1125,a,b,c);}
V shld(R_M32 a,REG32 b,CL c){x86(1125,a,b,c);}
V shrd(AX a,AX b,char c){x86(1126,a,b,(IMM)c);}
V shrd(AX a,DX b,char c){x86(1126,a,b,(IMM)c);}
V shrd(AX a,CX b,char c){x86(1126,a,b,(IMM)c);}
V shrd(AX a,REG16 b,char c){x86(1126,a,b,(IMM)c);}
V shrd(DX a,AX b,char c){x86(1126,a,b,(IMM)c);}
V shrd(DX a,DX b,char c){x86(1126,a,b,(IMM)c);}
V shrd(DX a,CX b,char c){x86(1126,a,b,(IMM)c);}
V shrd(DX a,REG16 b,char c){x86(1126,a,b,(IMM)c);}
V shrd(CX a,AX b,char c){x86(1126,a,b,(IMM)c);}
V shrd(CX a,DX b,char c){x86(1126,a,b,(IMM)c);}
V shrd(CX a,CX b,char c){x86(1126,a,b,(IMM)c);}
V shrd(CX a,REG16 b,char c){x86(1126,a,b,(IMM)c);}
V shrd(REG16 a,AX b,char c){x86(1126,a,b,(IMM)c);}
V shrd(REG16 a,DX b,char c){x86(1126,a,b,(IMM)c);}
V shrd(REG16 a,CX b,char c){x86(1126,a,b,(IMM)c);}
V shrd(REG16 a,REG16 b,char c){x86(1126,a,b,(IMM)c);}
V shrd(MEM16 a,AX b,char c){x86(1126,a,b,(IMM)c);}
V shrd(MEM16 a,DX b,char c){x86(1126,a,b,(IMM)c);}
V shrd(MEM16 a,CX b,char c){x86(1126,a,b,(IMM)c);}
V shrd(MEM16 a,REG16 b,char c){x86(1126,a,b,(IMM)c);}
V shrd(R_M16 a,AX b,char c){x86(1126,a,b,(IMM)c);}
V shrd(R_M16 a,DX b,char c){x86(1126,a,b,(IMM)c);}
V shrd(R_M16 a,CX b,char c){x86(1126,a,b,(IMM)c);}
V shrd(R_M16 a,REG16 b,char c){x86(1126,a,b,(IMM)c);}
V shrd(EAX a,EAX b,char c){x86(1127,a,b,(IMM)c);}
V shrd(EAX a,ECX b,char c){x86(1127,a,b,(IMM)c);}
V shrd(EAX a,REG32 b,char c){x86(1127,a,b,(IMM)c);}
V shrd(ECX a,EAX b,char c){x86(1127,a,b,(IMM)c);}
V shrd(ECX a,ECX b,char c){x86(1127,a,b,(IMM)c);}
V shrd(ECX a,REG32 b,char c){x86(1127,a,b,(IMM)c);}
V shrd(REG32 a,EAX b,char c){x86(1127,a,b,(IMM)c);}
V shrd(REG32 a,ECX b,char c){x86(1127,a,b,(IMM)c);}
V shrd(REG32 a,REG32 b,char c){x86(1127,a,b,(IMM)c);}
V shrd(MEM32 a,EAX b,char c){x86(1127,a,b,(IMM)c);}
V shrd(MEM32 a,ECX b,char c){x86(1127,a,b,(IMM)c);}
V shrd(MEM32 a,REG32 b,char c){x86(1127,a,b,(IMM)c);}
V shrd(R_M32 a,EAX b,char c){x86(1127,a,b,(IMM)c);}
V shrd(R_M32 a,ECX b,char c){x86(1127,a,b,(IMM)c);}
V shrd(R_M32 a,REG32 b,char c){x86(1127,a,b,(IMM)c);}
V shrd(AX a,AX b,CL c){x86(1128,a,b,c);}
V shrd(AX a,DX b,CL c){x86(1128,a,b,c);}
V shrd(AX a,CX b,CL c){x86(1128,a,b,c);}
V shrd(AX a,REG16 b,CL c){x86(1128,a,b,c);}
V shrd(DX a,AX b,CL c){x86(1128,a,b,c);}
V shrd(DX a,DX b,CL c){x86(1128,a,b,c);}
V shrd(DX a,CX b,CL c){x86(1128,a,b,c);}
V shrd(DX a,REG16 b,CL c){x86(1128,a,b,c);}
V shrd(CX a,AX b,CL c){x86(1128,a,b,c);}
V shrd(CX a,DX b,CL c){x86(1128,a,b,c);}
V shrd(CX a,CX b,CL c){x86(1128,a,b,c);}
V shrd(CX a,REG16 b,CL c){x86(1128,a,b,c);}
V shrd(REG16 a,AX b,CL c){x86(1128,a,b,c);}
V shrd(REG16 a,DX b,CL c){x86(1128,a,b,c);}
V shrd(REG16 a,CX b,CL c){x86(1128,a,b,c);}
V shrd(REG16 a,REG16 b,CL c){x86(1128,a,b,c);}
V shrd(MEM16 a,AX b,CL c){x86(1128,a,b,c);}
V shrd(MEM16 a,DX b,CL c){x86(1128,a,b,c);}
V shrd(MEM16 a,CX b,CL c){x86(1128,a,b,c);}
V shrd(MEM16 a,REG16 b,CL c){x86(1128,a,b,c);}
V shrd(R_M16 a,AX b,CL c){x86(1128,a,b,c);}
V shrd(R_M16 a,DX b,CL c){x86(1128,a,b,c);}
V shrd(R_M16 a,CX b,CL c){x86(1128,a,b,c);}
V shrd(R_M16 a,REG16 b,CL c){x86(1128,a,b,c);}
V shrd(EAX a,EAX b,CL c){x86(1129,a,b,c);}
V shrd(EAX a,ECX b,CL c){x86(1129,a,b,c);}
V shrd(EAX a,REG32 b,CL c){x86(1129,a,b,c);}
V shrd(ECX a,EAX b,CL c){x86(1129,a,b,c);}
V shrd(ECX a,ECX b,CL c){x86(1129,a,b,c);}
V shrd(ECX a,REG32 b,CL c){x86(1129,a,b,c);}
V shrd(REG32 a,EAX b,CL c){x86(1129,a,b,c);}
V shrd(REG32 a,ECX b,CL c){x86(1129,a,b,c);}
V shrd(REG32 a,REG32 b,CL c){x86(1129,a,b,c);}
V shrd(MEM32 a,EAX b,CL c){x86(1129,a,b,c);}
V shrd(MEM32 a,ECX b,CL c){x86(1129,a,b,c);}
V shrd(MEM32 a,REG32 b,CL c){x86(1129,a,b,c);}
V shrd(R_M32 a,EAX b,CL c){x86(1129,a,b,c);}
V shrd(R_M32 a,ECX b,CL c){x86(1129,a,b,c);}
V shrd(R_M32 a,REG32 b,CL c){x86(1129,a,b,c);}
V shufpd(XMMREG a,XMMREG b,char c){x86(1130,a,b,(IMM)c);}
V shufpd(XMMREG a,MEM128 b,char c){x86(1130,a,b,(IMM)c);}
V shufpd(XMMREG a,R_M128 b,char c){x86(1130,a,b,(IMM)c);}
V shufps(XMMREG a,XMMREG b,char c){x86(1131,a,b,(IMM)c);}
V shufps(XMMREG a,MEM128 b,char c){x86(1131,a,b,(IMM)c);}
V shufps(XMMREG a,R_M128 b,char c){x86(1131,a,b,(IMM)c);}
V smint(){x86(1132);}
V smintold(){x86(1133);}
V sqrtpd(XMMREG a,XMMREG b){x86(1134,a,b);}
V sqrtpd(XMMREG a,MEM128 b){x86(1134,a,b);}
V sqrtpd(XMMREG a,R_M128 b){x86(1134,a,b);}
V sqrtps(XMMREG a,XMMREG b){x86(1135,a,b);}
V sqrtps(XMMREG a,MEM128 b){x86(1135,a,b);}
V sqrtps(XMMREG a,R_M128 b){x86(1135,a,b);}
V sqrtsd(XMMREG a,XMMREG b){x86(1136,a,b);}
V sqrtsd(XMMREG a,MEM64 b){x86(1136,a,b);}
V sqrtsd(XMMREG a,XMM64 b){x86(1136,a,b);}
V sqrtss(XMMREG a,XMMREG b){x86(1137,a,b);}
V sqrtss(XMMREG a,MEM32 b){x86(1137,a,b);}
V sqrtss(XMMREG a,XMM32 b){x86(1137,a,b);}
V stc(){x86(1138);}
V std(){x86(1139);}
V sti(){x86(1140);}
V stmxcsr(MEM32 a){x86(1141,a);}
V stosb(){x86(1142);}
V stosw(){x86(1143);}
V stosd(){x86(1144);}
V rep_stosb(){x86(1145);}
V rep_stosw(){x86(1146);}
V rep_stosd(){x86(1147);}
V sub(AL a,AL b){x86(1148,a,b);}
V sub(AL a,CL b){x86(1148,a,b);}
V sub(AL a,REG8 b){x86(1148,a,b);}
V sub(CL a,AL b){x86(1148,a,b);}
V sub(CL a,CL b){x86(1148,a,b);}
V sub(CL a,REG8 b){x86(1148,a,b);}
V sub(REG8 a,AL b){x86(1148,a,b);}
V sub(REG8 a,CL b){x86(1148,a,b);}
V sub(REG8 a,REG8 b){x86(1148,a,b);}
V sub(MEM8 a,AL b){x86(1148,a,b);}
V sub(MEM8 a,CL b){x86(1148,a,b);}
V sub(MEM8 a,REG8 b){x86(1148,a,b);}
V sub(R_M8 a,AL b){x86(1148,a,b);}
V sub(R_M8 a,CL b){x86(1148,a,b);}
V sub(R_M8 a,REG8 b){x86(1148,a,b);}
V sub(AX a,AX b){x86(1149,a,b);}
V sub(AX a,DX b){x86(1149,a,b);}
V sub(AX a,CX b){x86(1149,a,b);}
V sub(AX a,REG16 b){x86(1149,a,b);}
V sub(DX a,AX b){x86(1149,a,b);}
V sub(DX a,DX b){x86(1149,a,b);}
V sub(DX a,CX b){x86(1149,a,b);}
V sub(DX a,REG16 b){x86(1149,a,b);}
V sub(CX a,AX b){x86(1149,a,b);}
V sub(CX a,DX b){x86(1149,a,b);}
V sub(CX a,CX b){x86(1149,a,b);}
V sub(CX a,REG16 b){x86(1149,a,b);}
V sub(REG16 a,AX b){x86(1149,a,b);}
V sub(REG16 a,DX b){x86(1149,a,b);}
V sub(REG16 a,CX b){x86(1149,a,b);}
V sub(REG16 a,REG16 b){x86(1149,a,b);}
V sub(MEM16 a,AX b){x86(1149,a,b);}
V sub(MEM16 a,DX b){x86(1149,a,b);}
V sub(MEM16 a,CX b){x86(1149,a,b);}
V sub(MEM16 a,REG16 b){x86(1149,a,b);}
V sub(R_M16 a,AX b){x86(1149,a,b);}
V sub(R_M16 a,DX b){x86(1149,a,b);}
V sub(R_M16 a,CX b){x86(1149,a,b);}
V sub(R_M16 a,REG16 b){x86(1149,a,b);}
V sub(EAX a,EAX b){x86(1150,a,b);}
V sub(EAX a,ECX b){x86(1150,a,b);}
V sub(EAX a,REG32 b){x86(1150,a,b);}
V sub(ECX a,EAX b){x86(1150,a,b);}
V sub(ECX a,ECX b){x86(1150,a,b);}
V sub(ECX a,REG32 b){x86(1150,a,b);}
V sub(REG32 a,EAX b){x86(1150,a,b);}
V sub(REG32 a,ECX b){x86(1150,a,b);}
V sub(REG32 a,REG32 b){x86(1150,a,b);}
V sub(MEM32 a,EAX b){x86(1150,a,b);}
V sub(MEM32 a,ECX b){x86(1150,a,b);}
V sub(MEM32 a,REG32 b){x86(1150,a,b);}
V sub(R_M32 a,EAX b){x86(1150,a,b);}
V sub(R_M32 a,ECX b){x86(1150,a,b);}
V sub(R_M32 a,REG32 b){x86(1150,a,b);}
V lock_sub(MEM8 a,AL b){x86(1151,a,b);}
V lock_sub(MEM8 a,CL b){x86(1151,a,b);}
V lock_sub(MEM8 a,REG8 b){x86(1151,a,b);}
V lock_sub(MEM16 a,AX b){x86(1152,a,b);}
V lock_sub(MEM16 a,DX b){x86(1152,a,b);}
V lock_sub(MEM16 a,CX b){x86(1152,a,b);}
V lock_sub(MEM16 a,REG16 b){x86(1152,a,b);}
V lock_sub(MEM32 a,EAX b){x86(1153,a,b);}
V lock_sub(MEM32 a,ECX b){x86(1153,a,b);}
V lock_sub(MEM32 a,REG32 b){x86(1153,a,b);}
V sub(AL a,MEM8 b){x86(1154,a,b);}
V sub(AL a,R_M8 b){x86(1154,a,b);}
V sub(CL a,MEM8 b){x86(1154,a,b);}
V sub(CL a,R_M8 b){x86(1154,a,b);}
V sub(REG8 a,MEM8 b){x86(1154,a,b);}
V sub(REG8 a,R_M8 b){x86(1154,a,b);}
V sub(AX a,MEM16 b){x86(1155,a,b);}
V sub(AX a,R_M16 b){x86(1155,a,b);}
V sub(DX a,MEM16 b){x86(1155,a,b);}
V sub(DX a,R_M16 b){x86(1155,a,b);}
V sub(CX a,MEM16 b){x86(1155,a,b);}
V sub(CX a,R_M16 b){x86(1155,a,b);}
V sub(REG16 a,MEM16 b){x86(1155,a,b);}
V sub(REG16 a,R_M16 b){x86(1155,a,b);}
V sub(EAX a,MEM32 b){x86(1156,a,b);}
V sub(EAX a,R_M32 b){x86(1156,a,b);}
V sub(ECX a,MEM32 b){x86(1156,a,b);}
V sub(ECX a,R_M32 b){x86(1156,a,b);}
V sub(REG32 a,MEM32 b){x86(1156,a,b);}
V sub(REG32 a,R_M32 b){x86(1156,a,b);}
V sub(AL a,char b){x86(1157,a,(IMM)b);}
V sub(CL a,char b){x86(1157,a,(IMM)b);}
V sub(REG8 a,char b){x86(1157,a,(IMM)b);}
V sub(MEM8 a,char b){x86(1157,a,(IMM)b);}
V sub(R_M8 a,char b){x86(1157,a,(IMM)b);}
V sub(AX a,char b){x86(1158,a,(IMM)b);}
V sub(AX a,short b){x86(1158,a,(IMM)b);}
V sub(DX a,char b){x86(1158,a,(IMM)b);}
V sub(DX a,short b){x86(1158,a,(IMM)b);}
V sub(CX a,char b){x86(1158,a,(IMM)b);}
V sub(CX a,short b){x86(1158,a,(IMM)b);}
V sub(REG16 a,char b){x86(1158,a,(IMM)b);}
V sub(REG16 a,short b){x86(1158,a,(IMM)b);}
V sub(MEM16 a,char b){x86(1158,a,(IMM)b);}
V sub(MEM16 a,short b){x86(1158,a,(IMM)b);}
V sub(R_M16 a,char b){x86(1158,a,(IMM)b);}
V sub(R_M16 a,short b){x86(1158,a,(IMM)b);}
V sub(EAX a,int b){x86(1159,a,(IMM)b);}
V sub(EAX a,char b){x86(1159,a,(IMM)b);}
V sub(EAX a,short b){x86(1159,a,(IMM)b);}
V sub(EAX a,REF b){x86(1159,a,b);}
V sub(ECX a,int b){x86(1159,a,(IMM)b);}
V sub(ECX a,char b){x86(1159,a,(IMM)b);}
V sub(ECX a,short b){x86(1159,a,(IMM)b);}
V sub(ECX a,REF b){x86(1159,a,b);}
V sub(REG32 a,int b){x86(1159,a,(IMM)b);}
V sub(REG32 a,char b){x86(1159,a,(IMM)b);}
V sub(REG32 a,short b){x86(1159,a,(IMM)b);}
V sub(REG32 a,REF b){x86(1159,a,b);}
V sub(MEM32 a,int b){x86(1159,a,(IMM)b);}
V sub(MEM32 a,char b){x86(1159,a,(IMM)b);}
V sub(MEM32 a,short b){x86(1159,a,(IMM)b);}
V sub(MEM32 a,REF b){x86(1159,a,b);}
V sub(R_M32 a,int b){x86(1159,a,(IMM)b);}
V sub(R_M32 a,char b){x86(1159,a,(IMM)b);}
V sub(R_M32 a,short b){x86(1159,a,(IMM)b);}
V sub(R_M32 a,REF b){x86(1159,a,b);}
V lock_sub(MEM8 a,char b){x86(1162,a,(IMM)b);}
V lock_sub(MEM16 a,char b){x86(1163,a,(IMM)b);}
V lock_sub(MEM16 a,short b){x86(1163,a,(IMM)b);}
V lock_sub(MEM32 a,int b){x86(1164,a,(IMM)b);}
V lock_sub(MEM32 a,char b){x86(1164,a,(IMM)b);}
V lock_sub(MEM32 a,short b){x86(1164,a,(IMM)b);}
V lock_sub(MEM32 a,REF b){x86(1164,a,b);}
V subpd(XMMREG a,XMMREG b){x86(1170,a,b);}
V subpd(XMMREG a,MEM128 b){x86(1170,a,b);}
V subpd(XMMREG a,R_M128 b){x86(1170,a,b);}
V subps(XMMREG a,XMMREG b){x86(1171,a,b);}
V subps(XMMREG a,MEM128 b){x86(1171,a,b);}
V subps(XMMREG a,R_M128 b){x86(1171,a,b);}
V subsd(XMMREG a,XMMREG b){x86(1172,a,b);}
V subsd(XMMREG a,MEM64 b){x86(1172,a,b);}
V subsd(XMMREG a,XMM64 b){x86(1172,a,b);}
V subss(XMMREG a,XMMREG b){x86(1173,a,b);}
V subss(XMMREG a,MEM32 b){x86(1173,a,b);}
V subss(XMMREG a,XMM32 b){x86(1173,a,b);}
V sysenter(){x86(1174);}
V test(AL a,AL b){x86(1175,a,b);}
V test(AL a,CL b){x86(1175,a,b);}
V test(AL a,REG8 b){x86(1175,a,b);}
V test(CL a,AL b){x86(1175,a,b);}
V test(CL a,CL b){x86(1175,a,b);}
V test(CL a,REG8 b){x86(1175,a,b);}
V test(REG8 a,AL b){x86(1175,a,b);}
V test(REG8 a,CL b){x86(1175,a,b);}
V test(REG8 a,REG8 b){x86(1175,a,b);}
V test(MEM8 a,AL b){x86(1175,a,b);}
V test(MEM8 a,CL b){x86(1175,a,b);}
V test(MEM8 a,REG8 b){x86(1175,a,b);}
V test(R_M8 a,AL b){x86(1175,a,b);}
V test(R_M8 a,CL b){x86(1175,a,b);}
V test(R_M8 a,REG8 b){x86(1175,a,b);}
V test(AX a,AX b){x86(1176,a,b);}
V test(AX a,DX b){x86(1176,a,b);}
V test(AX a,CX b){x86(1176,a,b);}
V test(AX a,REG16 b){x86(1176,a,b);}
V test(DX a,AX b){x86(1176,a,b);}
V test(DX a,DX b){x86(1176,a,b);}
V test(DX a,CX b){x86(1176,a,b);}
V test(DX a,REG16 b){x86(1176,a,b);}
V test(CX a,AX b){x86(1176,a,b);}
V test(CX a,DX b){x86(1176,a,b);}
V test(CX a,CX b){x86(1176,a,b);}
V test(CX a,REG16 b){x86(1176,a,b);}
V test(REG16 a,AX b){x86(1176,a,b);}
V test(REG16 a,DX b){x86(1176,a,b);}
V test(REG16 a,CX b){x86(1176,a,b);}
V test(REG16 a,REG16 b){x86(1176,a,b);}
V test(MEM16 a,AX b){x86(1176,a,b);}
V test(MEM16 a,DX b){x86(1176,a,b);}
V test(MEM16 a,CX b){x86(1176,a,b);}
V test(MEM16 a,REG16 b){x86(1176,a,b);}
V test(R_M16 a,AX b){x86(1176,a,b);}
V test(R_M16 a,DX b){x86(1176,a,b);}
V test(R_M16 a,CX b){x86(1176,a,b);}
V test(R_M16 a,REG16 b){x86(1176,a,b);}
V test(EAX a,EAX b){x86(1177,a,b);}
V test(EAX a,ECX b){x86(1177,a,b);}
V test(EAX a,REG32 b){x86(1177,a,b);}
V test(ECX a,EAX b){x86(1177,a,b);}
V test(ECX a,ECX b){x86(1177,a,b);}
V test(ECX a,REG32 b){x86(1177,a,b);}
V test(REG32 a,EAX b){x86(1177,a,b);}
V test(REG32 a,ECX b){x86(1177,a,b);}
V test(REG32 a,REG32 b){x86(1177,a,b);}
V test(MEM32 a,EAX b){x86(1177,a,b);}
V test(MEM32 a,ECX b){x86(1177,a,b);}
V test(MEM32 a,REG32 b){x86(1177,a,b);}
V test(R_M32 a,EAX b){x86(1177,a,b);}
V test(R_M32 a,ECX b){x86(1177,a,b);}
V test(R_M32 a,REG32 b){x86(1177,a,b);}
V test(AL a,char b){x86(1178,a,(IMM)b);}
V test(CL a,char b){x86(1178,a,(IMM)b);}
V test(REG8 a,char b){x86(1178,a,(IMM)b);}
V test(MEM8 a,char b){x86(1178,a,(IMM)b);}
V test(R_M8 a,char b){x86(1178,a,(IMM)b);}
V test(AX a,char b){x86(1179,a,(IMM)b);}
V test(AX a,short b){x86(1179,a,(IMM)b);}
V test(DX a,char b){x86(1179,a,(IMM)b);}
V test(DX a,short b){x86(1179,a,(IMM)b);}
V test(CX a,char b){x86(1179,a,(IMM)b);}
V test(CX a,short b){x86(1179,a,(IMM)b);}
V test(REG16 a,char b){x86(1179,a,(IMM)b);}
V test(REG16 a,short b){x86(1179,a,(IMM)b);}
V test(MEM16 a,char b){x86(1179,a,(IMM)b);}
V test(MEM16 a,short b){x86(1179,a,(IMM)b);}
V test(R_M16 a,char b){x86(1179,a,(IMM)b);}
V test(R_M16 a,short b){x86(1179,a,(IMM)b);}
V test(EAX a,int b){x86(1180,a,(IMM)b);}
V test(EAX a,char b){x86(1180,a,(IMM)b);}
V test(EAX a,short b){x86(1180,a,(IMM)b);}
V test(EAX a,REF b){x86(1180,a,b);}
V test(ECX a,int b){x86(1180,a,(IMM)b);}
V test(ECX a,char b){x86(1180,a,(IMM)b);}
V test(ECX a,short b){x86(1180,a,(IMM)b);}
V test(ECX a,REF b){x86(1180,a,b);}
V test(REG32 a,int b){x86(1180,a,(IMM)b);}
V test(REG32 a,char b){x86(1180,a,(IMM)b);}
V test(REG32 a,short b){x86(1180,a,(IMM)b);}
V test(REG32 a,REF b){x86(1180,a,b);}
V test(MEM32 a,int b){x86(1180,a,(IMM)b);}
V test(MEM32 a,char b){x86(1180,a,(IMM)b);}
V test(MEM32 a,short b){x86(1180,a,(IMM)b);}
V test(MEM32 a,REF b){x86(1180,a,b);}
V test(R_M32 a,int b){x86(1180,a,(IMM)b);}
V test(R_M32 a,char b){x86(1180,a,(IMM)b);}
V test(R_M32 a,short b){x86(1180,a,(IMM)b);}
V test(R_M32 a,REF b){x86(1180,a,b);}
V ucomisd(XMMREG a,XMMREG b){x86(1184,a,b);}
V ucomisd(XMMREG a,MEM64 b){x86(1184,a,b);}
V ucomisd(XMMREG a,XMM64 b){x86(1184,a,b);}
V ucomiss(XMMREG a,XMMREG b){x86(1185,a,b);}
V ucomiss(XMMREG a,MEM32 b){x86(1185,a,b);}
V ucomiss(XMMREG a,XMM32 b){x86(1185,a,b);}
V ud2(){x86(1186);}
V unpckhpd(XMMREG a,XMMREG b){x86(1187,a,b);}
V unpckhpd(XMMREG a,MEM128 b){x86(1187,a,b);}
V unpckhpd(XMMREG a,R_M128 b){x86(1187,a,b);}
V unpckhps(XMMREG a,XMMREG b){x86(1188,a,b);}
V unpckhps(XMMREG a,MEM128 b){x86(1188,a,b);}
V unpckhps(XMMREG a,R_M128 b){x86(1188,a,b);}
V unpcklpd(XMMREG a,XMMREG b){x86(1189,a,b);}
V unpcklpd(XMMREG a,MEM128 b){x86(1189,a,b);}
V unpcklpd(XMMREG a,R_M128 b){x86(1189,a,b);}
V unpcklps(XMMREG a,XMMREG b){x86(1190,a,b);}
V unpcklps(XMMREG a,MEM128 b){x86(1190,a,b);}
V unpcklps(XMMREG a,R_M128 b){x86(1190,a,b);}
V wait(){x86(1191);}
V wrmsr(){x86(1192);}
V xadd(AL a,AL b){x86(1193,a,b);}
V xadd(AL a,CL b){x86(1193,a,b);}
V xadd(AL a,REG8 b){x86(1193,a,b);}
V xadd(CL a,AL b){x86(1193,a,b);}
V xadd(CL a,CL b){x86(1193,a,b);}
V xadd(CL a,REG8 b){x86(1193,a,b);}
V xadd(REG8 a,AL b){x86(1193,a,b);}
V xadd(REG8 a,CL b){x86(1193,a,b);}
V xadd(REG8 a,REG8 b){x86(1193,a,b);}
V xadd(MEM8 a,AL b){x86(1193,a,b);}
V xadd(MEM8 a,CL b){x86(1193,a,b);}
V xadd(MEM8 a,REG8 b){x86(1193,a,b);}
V xadd(R_M8 a,AL b){x86(1193,a,b);}
V xadd(R_M8 a,CL b){x86(1193,a,b);}
V xadd(R_M8 a,REG8 b){x86(1193,a,b);}
V xadd(AX a,AX b){x86(1194,a,b);}
V xadd(AX a,DX b){x86(1194,a,b);}
V xadd(AX a,CX b){x86(1194,a,b);}
V xadd(AX a,REG16 b){x86(1194,a,b);}
V xadd(DX a,AX b){x86(1194,a,b);}
V xadd(DX a,DX b){x86(1194,a,b);}
V xadd(DX a,CX b){x86(1194,a,b);}
V xadd(DX a,REG16 b){x86(1194,a,b);}
V xadd(CX a,AX b){x86(1194,a,b);}
V xadd(CX a,DX b){x86(1194,a,b);}
V xadd(CX a,CX b){x86(1194,a,b);}
V xadd(CX a,REG16 b){x86(1194,a,b);}
V xadd(REG16 a,AX b){x86(1194,a,b);}
V xadd(REG16 a,DX b){x86(1194,a,b);}
V xadd(REG16 a,CX b){x86(1194,a,b);}
V xadd(REG16 a,REG16 b){x86(1194,a,b);}
V xadd(MEM16 a,AX b){x86(1194,a,b);}
V xadd(MEM16 a,DX b){x86(1194,a,b);}
V xadd(MEM16 a,CX b){x86(1194,a,b);}
V xadd(MEM16 a,REG16 b){x86(1194,a,b);}
V xadd(R_M16 a,AX b){x86(1194,a,b);}
V xadd(R_M16 a,DX b){x86(1194,a,b);}
V xadd(R_M16 a,CX b){x86(1194,a,b);}
V xadd(R_M16 a,REG16 b){x86(1194,a,b);}
V xadd(EAX a,EAX b){x86(1195,a,b);}
V xadd(EAX a,ECX b){x86(1195,a,b);}
V xadd(EAX a,REG32 b){x86(1195,a,b);}
V xadd(ECX a,EAX b){x86(1195,a,b);}
V xadd(ECX a,ECX b){x86(1195,a,b);}
V xadd(ECX a,REG32 b){x86(1195,a,b);}
V xadd(REG32 a,EAX b){x86(1195,a,b);}
V xadd(REG32 a,ECX b){x86(1195,a,b);}
V xadd(REG32 a,REG32 b){x86(1195,a,b);}
V xadd(MEM32 a,EAX b){x86(1195,a,b);}
V xadd(MEM32 a,ECX b){x86(1195,a,b);}
V xadd(MEM32 a,REG32 b){x86(1195,a,b);}
V xadd(R_M32 a,EAX b){x86(1195,a,b);}
V xadd(R_M32 a,ECX b){x86(1195,a,b);}
V xadd(R_M32 a,REG32 b){x86(1195,a,b);}
V lock_xadd(MEM8 a,AL b){x86(1196,a,b);}
V lock_xadd(MEM8 a,CL b){x86(1196,a,b);}
V lock_xadd(MEM8 a,REG8 b){x86(1196,a,b);}
V lock_xadd(MEM16 a,AX b){x86(1197,a,b);}
V lock_xadd(MEM16 a,DX b){x86(1197,a,b);}
V lock_xadd(MEM16 a,CX b){x86(1197,a,b);}
V lock_xadd(MEM16 a,REG16 b){x86(1197,a,b);}
V lock_xadd(MEM32 a,EAX b){x86(1198,a,b);}
V lock_xadd(MEM32 a,ECX b){x86(1198,a,b);}
V lock_xadd(MEM32 a,REG32 b){x86(1198,a,b);}
V xchg(AL a,AL b){x86(1199,a,b);}
V xchg(AL a,CL b){x86(1199,a,b);}
V xchg(AL a,REG8 b){x86(1199,a,b);}
V xchg(AL a,MEM8 b){x86(1199,a,b);}
V xchg(AL a,R_M8 b){x86(1199,a,b);}
V xchg(CL a,AL b){x86(1199,a,b);}
V xchg(CL a,CL b){x86(1199,a,b);}
V xchg(CL a,REG8 b){x86(1199,a,b);}
V xchg(CL a,MEM8 b){x86(1199,a,b);}
V xchg(CL a,R_M8 b){x86(1199,a,b);}
V xchg(REG8 a,AL b){x86(1199,a,b);}
V xchg(REG8 a,CL b){x86(1199,a,b);}
V xchg(REG8 a,REG8 b){x86(1199,a,b);}
V xchg(REG8 a,MEM8 b){x86(1199,a,b);}
V xchg(REG8 a,R_M8 b){x86(1199,a,b);}
V xchg(AX a,AL b){x86(1200,a,b);}
V xchg(AX a,CL b){x86(1200,a,b);}
V xchg(AX a,REG8 b){x86(1200,a,b);}
V xchg(AX a,MEM8 b){x86(1200,a,b);}
V xchg(AX a,R_M8 b){x86(1200,a,b);}
V xchg(DX a,AL b){x86(1200,a,b);}
V xchg(DX a,CL b){x86(1200,a,b);}
V xchg(DX a,REG8 b){x86(1200,a,b);}
V xchg(DX a,MEM8 b){x86(1200,a,b);}
V xchg(DX a,R_M8 b){x86(1200,a,b);}
V xchg(CX a,AL b){x86(1200,a,b);}
V xchg(CX a,CL b){x86(1200,a,b);}
V xchg(CX a,REG8 b){x86(1200,a,b);}
V xchg(CX a,MEM8 b){x86(1200,a,b);}
V xchg(CX a,R_M8 b){x86(1200,a,b);}
V xchg(REG16 a,AL b){x86(1200,a,b);}
V xchg(REG16 a,CL b){x86(1200,a,b);}
V xchg(REG16 a,REG8 b){x86(1200,a,b);}
V xchg(REG16 a,MEM8 b){x86(1200,a,b);}
V xchg(REG16 a,R_M8 b){x86(1200,a,b);}
V xchg(EAX a,EAX b){x86(1201,a,b);}
V xchg(EAX a,ECX b){x86(1201,a,b);}
V xchg(EAX a,REG32 b){x86(1201,a,b);}
V xchg(EAX a,MEM32 b){x86(1201,a,b);}
V xchg(EAX a,R_M32 b){x86(1201,a,b);}
V xchg(ECX a,EAX b){x86(1201,a,b);}
V xchg(ECX a,ECX b){x86(1201,a,b);}
V xchg(ECX a,REG32 b){x86(1201,a,b);}
V xchg(ECX a,MEM32 b){x86(1201,a,b);}
V xchg(ECX a,R_M32 b){x86(1201,a,b);}
V xchg(REG32 a,EAX b){x86(1201,a,b);}
V xchg(REG32 a,ECX b){x86(1201,a,b);}
V xchg(REG32 a,REG32 b){x86(1201,a,b);}
V xchg(REG32 a,MEM32 b){x86(1201,a,b);}
V xchg(REG32 a,R_M32 b){x86(1201,a,b);}
V xchg(MEM8 a,AL b){x86(1202,a,b);}
V xchg(MEM8 a,CL b){x86(1202,a,b);}
V xchg(MEM8 a,REG8 b){x86(1202,a,b);}
V xchg(R_M8 a,AL b){x86(1202,a,b);}
V xchg(R_M8 a,CL b){x86(1202,a,b);}
V xchg(R_M8 a,REG8 b){x86(1202,a,b);}
V xchg(AX a,AX b){x86(1203,a,b);}
V xchg(AX a,DX b){x86(1203,a,b);}
V xchg(AX a,CX b){x86(1203,a,b);}
V xchg(AX a,REG16 b){x86(1203,a,b);}
V xchg(DX a,AX b){x86(1203,a,b);}
V xchg(DX a,DX b){x86(1203,a,b);}
V xchg(DX a,CX b){x86(1203,a,b);}
V xchg(DX a,REG16 b){x86(1203,a,b);}
V xchg(CX a,AX b){x86(1203,a,b);}
V xchg(CX a,DX b){x86(1203,a,b);}
V xchg(CX a,CX b){x86(1203,a,b);}
V xchg(CX a,REG16 b){x86(1203,a,b);}
V xchg(REG16 a,AX b){x86(1203,a,b);}
V xchg(REG16 a,DX b){x86(1203,a,b);}
V xchg(REG16 a,CX b){x86(1203,a,b);}
V xchg(REG16 a,REG16 b){x86(1203,a,b);}
V xchg(MEM16 a,AX b){x86(1203,a,b);}
V xchg(MEM16 a,DX b){x86(1203,a,b);}
V xchg(MEM16 a,CX b){x86(1203,a,b);}
V xchg(MEM16 a,REG16 b){x86(1203,a,b);}
V xchg(R_M16 a,AX b){x86(1203,a,b);}
V xchg(R_M16 a,DX b){x86(1203,a,b);}
V xchg(R_M16 a,CX b){x86(1203,a,b);}
V xchg(R_M16 a,REG16 b){x86(1203,a,b);}
V xchg(MEM32 a,EAX b){x86(1204,a,b);}
V xchg(MEM32 a,ECX b){x86(1204,a,b);}
V xchg(MEM32 a,REG32 b){x86(1204,a,b);}
V xchg(R_M32 a,EAX b){x86(1204,a,b);}
V xchg(R_M32 a,ECX b){x86(1204,a,b);}
V xchg(R_M32 a,REG32 b){x86(1204,a,b);}
V lock_xchg(MEM8 a,AL b){x86(1205,a,b);}
V lock_xchg(MEM8 a,CL b){x86(1205,a,b);}
V lock_xchg(MEM8 a,REG8 b){x86(1205,a,b);}
V lock_xchg(MEM16 a,AX b){x86(1206,a,b);}
V lock_xchg(MEM16 a,DX b){x86(1206,a,b);}
V lock_xchg(MEM16 a,CX b){x86(1206,a,b);}
V lock_xchg(MEM16 a,REG16 b){x86(1206,a,b);}
V lock_xchg(MEM32 a,EAX b){x86(1207,a,b);}
V lock_xchg(MEM32 a,ECX b){x86(1207,a,b);}
V lock_xchg(MEM32 a,REG32 b){x86(1207,a,b);}
V xlatb(){x86(1212);}
V xor(AL a,AL b){x86(1213,a,b);}
V xor(AL a,CL b){x86(1213,a,b);}
V xor(AL a,REG8 b){x86(1213,a,b);}
V xor(CL a,AL b){x86(1213,a,b);}
V xor(CL a,CL b){x86(1213,a,b);}
V xor(CL a,REG8 b){x86(1213,a,b);}
V xor(REG8 a,AL b){x86(1213,a,b);}
V xor(REG8 a,CL b){x86(1213,a,b);}
V xor(REG8 a,REG8 b){x86(1213,a,b);}
V xor(MEM8 a,AL b){x86(1213,a,b);}
V xor(MEM8 a,CL b){x86(1213,a,b);}
V xor(MEM8 a,REG8 b){x86(1213,a,b);}
V xor(R_M8 a,AL b){x86(1213,a,b);}
V xor(R_M8 a,CL b){x86(1213,a,b);}
V xor(R_M8 a,REG8 b){x86(1213,a,b);}
V xor(AX a,AX b){x86(1214,a,b);}
V xor(AX a,DX b){x86(1214,a,b);}
V xor(AX a,CX b){x86(1214,a,b);}
V xor(AX a,REG16 b){x86(1214,a,b);}
V xor(DX a,AX b){x86(1214,a,b);}
V xor(DX a,DX b){x86(1214,a,b);}
V xor(DX a,CX b){x86(1214,a,b);}
V xor(DX a,REG16 b){x86(1214,a,b);}
V xor(CX a,AX b){x86(1214,a,b);}
V xor(CX a,DX b){x86(1214,a,b);}
V xor(CX a,CX b){x86(1214,a,b);}
V xor(CX a,REG16 b){x86(1214,a,b);}
V xor(REG16 a,AX b){x86(1214,a,b);}
V xor(REG16 a,DX b){x86(1214,a,b);}
V xor(REG16 a,CX b){x86(1214,a,b);}
V xor(REG16 a,REG16 b){x86(1214,a,b);}
V xor(MEM16 a,AX b){x86(1214,a,b);}
V xor(MEM16 a,DX b){x86(1214,a,b);}
V xor(MEM16 a,CX b){x86(1214,a,b);}
V xor(MEM16 a,REG16 b){x86(1214,a,b);}
V xor(R_M16 a,AX b){x86(1214,a,b);}
V xor(R_M16 a,DX b){x86(1214,a,b);}
V xor(R_M16 a,CX b){x86(1214,a,b);}
V xor(R_M16 a,REG16 b){x86(1214,a,b);}
V xor(EAX a,EAX b){x86(1215,a,b);}
V xor(EAX a,ECX b){x86(1215,a,b);}
V xor(EAX a,REG32 b){x86(1215,a,b);}
V xor(ECX a,EAX b){x86(1215,a,b);}
V xor(ECX a,ECX b){x86(1215,a,b);}
V xor(ECX a,REG32 b){x86(1215,a,b);}
V xor(REG32 a,EAX b){x86(1215,a,b);}
V xor(REG32 a,ECX b){x86(1215,a,b);}
V xor(REG32 a,REG32 b){x86(1215,a,b);}
V xor(MEM32 a,EAX b){x86(1215,a,b);}
V xor(MEM32 a,ECX b){x86(1215,a,b);}
V xor(MEM32 a,REG32 b){x86(1215,a,b);}
V xor(R_M32 a,EAX b){x86(1215,a,b);}
V xor(R_M32 a,ECX b){x86(1215,a,b);}
V xor(R_M32 a,REG32 b){x86(1215,a,b);}
V lock_xor(MEM8 a,AL b){x86(1216,a,b);}
V lock_xor(MEM8 a,CL b){x86(1216,a,b);}
V lock_xor(MEM8 a,REG8 b){x86(1216,a,b);}
V lock_xor(MEM16 a,AX b){x86(1217,a,b);}
V lock_xor(MEM16 a,DX b){x86(1217,a,b);}
V lock_xor(MEM16 a,CX b){x86(1217,a,b);}
V lock_xor(MEM16 a,REG16 b){x86(1217,a,b);}
V lock_xor(MEM32 a,EAX b){x86(1218,a,b);}
V lock_xor(MEM32 a,ECX b){x86(1218,a,b);}
V lock_xor(MEM32 a,REG32 b){x86(1218,a,b);}
V xor(AL a,MEM8 b){x86(1219,a,b);}
V xor(AL a,R_M8 b){x86(1219,a,b);}
V xor(CL a,MEM8 b){x86(1219,a,b);}
V xor(CL a,R_M8 b){x86(1219,a,b);}
V xor(REG8 a,MEM8 b){x86(1219,a,b);}
V xor(REG8 a,R_M8 b){x86(1219,a,b);}
V xor(AX a,MEM16 b){x86(1220,a,b);}
V xor(AX a,R_M16 b){x86(1220,a,b);}
V xor(DX a,MEM16 b){x86(1220,a,b);}
V xor(DX a,R_M16 b){x86(1220,a,b);}
V xor(CX a,MEM16 b){x86(1220,a,b);}
V xor(CX a,R_M16 b){x86(1220,a,b);}
V xor(REG16 a,MEM16 b){x86(1220,a,b);}
V xor(REG16 a,R_M16 b){x86(1220,a,b);}
V xor(EAX a,MEM32 b){x86(1221,a,b);}
V xor(EAX a,R_M32 b){x86(1221,a,b);}
V xor(ECX a,MEM32 b){x86(1221,a,b);}
V xor(ECX a,R_M32 b){x86(1221,a,b);}
V xor(REG32 a,MEM32 b){x86(1221,a,b);}
V xor(REG32 a,R_M32 b){x86(1221,a,b);}
V xor(AL a,char b){x86(1222,a,(IMM)b);}
V xor(CL a,char b){x86(1222,a,(IMM)b);}
V xor(REG8 a,char b){x86(1222,a,(IMM)b);}
V xor(MEM8 a,char b){x86(1222,a,(IMM)b);}
V xor(R_M8 a,char b){x86(1222,a,(IMM)b);}
V xor(AX a,char b){x86(1223,a,(IMM)b);}
V xor(AX a,short b){x86(1223,a,(IMM)b);}
V xor(DX a,char b){x86(1223,a,(IMM)b);}
V xor(DX a,short b){x86(1223,a,(IMM)b);}
V xor(CX a,char b){x86(1223,a,(IMM)b);}
V xor(CX a,short b){x86(1223,a,(IMM)b);}
V xor(REG16 a,char b){x86(1223,a,(IMM)b);}
V xor(REG16 a,short b){x86(1223,a,(IMM)b);}
V xor(MEM16 a,char b){x86(1223,a,(IMM)b);}
V xor(MEM16 a,short b){x86(1223,a,(IMM)b);}
V xor(R_M16 a,char b){x86(1223,a,(IMM)b);}
V xor(R_M16 a,short b){x86(1223,a,(IMM)b);}
V xor(EAX a,int b){x86(1224,a,(IMM)b);}
V xor(EAX a,char b){x86(1224,a,(IMM)b);}
V xor(EAX a,short b){x86(1224,a,(IMM)b);}
V xor(EAX a,REF b){x86(1224,a,b);}
V xor(ECX a,int b){x86(1224,a,(IMM)b);}
V xor(ECX a,char b){x86(1224,a,(IMM)b);}
V xor(ECX a,short b){x86(1224,a,(IMM)b);}
V xor(ECX a,REF b){x86(1224,a,b);}
V xor(REG32 a,int b){x86(1224,a,(IMM)b);}
V xor(REG32 a,char b){x86(1224,a,(IMM)b);}
V xor(REG32 a,short b){x86(1224,a,(IMM)b);}
V xor(REG32 a,REF b){x86(1224,a,b);}
V xor(MEM32 a,int b){x86(1224,a,(IMM)b);}
V xor(MEM32 a,char b){x86(1224,a,(IMM)b);}
V xor(MEM32 a,short b){x86(1224,a,(IMM)b);}
V xor(MEM32 a,REF b){x86(1224,a,b);}
V xor(R_M32 a,int b){x86(1224,a,(IMM)b);}
V xor(R_M32 a,char b){x86(1224,a,(IMM)b);}
V xor(R_M32 a,short b){x86(1224,a,(IMM)b);}
V xor(R_M32 a,REF b){x86(1224,a,b);}
V lock_xor(MEM8 a,char b){x86(1227,a,(IMM)b);}
V lock_xor(MEM16 a,char b){x86(1228,a,(IMM)b);}
V lock_xor(MEM16 a,short b){x86(1228,a,(IMM)b);}
V lock_xor(MEM32 a,int b){x86(1229,a,(IMM)b);}
V lock_xor(MEM32 a,char b){x86(1229,a,(IMM)b);}
V lock_xor(MEM32 a,short b){x86(1229,a,(IMM)b);}
V lock_xor(MEM32 a,REF b){x86(1229,a,b);}
V xorps(XMMREG a,XMMREG b){x86(1235,a,b);}
V xorps(XMMREG a,MEM128 b){x86(1235,a,b);}
V xorps(XMMREG a,R_M128 b){x86(1235,a,b);}
V db(){x86(1236);}
V dw(){x86(1237);}
V dd(){x86(1238);}
V db(char a){x86(1239,(IMM)a);}
V dw(char a){x86(1240,(IMM)a);}
V dw(short a){x86(1240,(IMM)a);}
V dd(int a){x86(1241,(IMM)a);}
V dd(char a){x86(1241,(IMM)a);}
V dd(short a){x86(1241,(IMM)a);}
V dd(REF a){x86(1241,a);}
V db(MEM8 a){x86(1242,a);}
V db(MEM16 a){x86(1242,a);}
V db(MEM32 a){x86(1242,a);}
V db(MEM64 a){x86(1242,a);}
V db(MEM128 a){x86(1242,a);}
V dw(MEM8 a){x86(1243,a);}
V dw(MEM16 a){x86(1243,a);}
V dw(MEM32 a){x86(1243,a);}
V dw(MEM64 a){x86(1243,a);}
V dw(MEM128 a){x86(1243,a);}
V dd(MEM8 a){x86(1244,a);}
V dd(MEM16 a){x86(1244,a);}
V dd(MEM32 a){x86(1244,a);}
V dd(MEM64 a){x86(1244,a);}
V dd(MEM128 a){x86(1244,a);}
V db(REF a){x86(1245,a);}
V db(char* a){x86(1245,(STR)a);}
V align(int a){x86(1246,(IMM)a);}
V align(char a){x86(1246,(IMM)a);}
V align(short a){x86(1246,(IMM)a);}
V align(REF a){x86(1246,a);}

#endif   // SOFTWIRE_NO_INTRINSICS
