// Avisynth v2.5.
// http://www.avisynth.org

// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA, or visit
// http://www.gnu.org/copyleft/gpl.html .
//
// Linking Avisynth statically or dynamically with other modules is making a
// combined work based on Avisynth.  Thus, the terms and conditions of the GNU
// General Public License cover the whole combination.
//
// As a special exception, the copyright holders of Avisynth give you
// permission to link Avisynth with independent modules that communicate with
// Avisynth solely through the interfaces defined in avisynth.h, regardless of the license
// terms of these independent modules, and to copy and distribute the
// resulting combined work under terms of your choice, provided that
// every copy of the combined work is accompanied by a complete copy of
// the source code of Avisynth (the version of Avisynth used to produce the
// combined work), being distributed under the terms of the GNU General
// Public License plus this exception.  An independent module is a module
// which is not derived from or based on Avisynth, such as 3rd-party filters,
// import and export plugins, or graphical user interfaces.

// TCPDeliver (c) 2004 by Klaus Post

#include "TCPCommon.h"
#include "avisynth.h"


#define TCPD_DO1(buf,i)  {s1 += buf[i]; s2 += s1;}
#define TCPD_DO2(buf,i)  TCPD_DO1(buf,i); TCPD_DO1(buf,i+1);
#define TCPD_DO4(buf,i)  TCPD_DO2(buf,i); TCPD_DO2(buf,i+2);
#define TCPD_DO8(buf,i)  TCPD_DO4(buf,i); TCPD_DO4(buf,i+4);
#define TCPD_DO16(buf,i) TCPD_DO8(buf,i); TCPD_DO8(buf,i+8);

unsigned int adler32(unsigned int adler, const char* buf, unsigned int len)
{
	unsigned int s1 = adler & 0xffff;
	unsigned int s2 = (adler >> 16) & 0xffff;
	int k;

	if (buf == NULL)
		return 1;

	while (len > 0)
	{
		k = len < TCPD_NMAX ? (int) len : TCPD_NMAX;
		len -= k;
		if (k >= 16) do
		{
			TCPD_DO16(buf,0);
			buf += 16;
			k -= 16;
		} while (k >= 16);
		if (k != 0) do
		{
			s1 += *buf++;
			s2 += s1;
		} while (--k > 0);
		s1 %= TCPD_BASE;
		s2 %= TCPD_BASE;
	}
	return (s2 << 16) | s1;
}
 
/*
BOOL APIENTRY DllMain(HANDLE hModule, ULONG ulReason, LPVOID lpReserved) {
	switch(ulReason) {
	case DLL_PROCESS_ATTACH:
		hInstance=(HINSTANCE)hModule; 
		_RPT0(0,"Process attach\n");
		break;

	case DLL_PROCESS_DETACH:
		_RPT0(0,"Process detach\n");
		break;
	}
    return TRUE;
}
*/