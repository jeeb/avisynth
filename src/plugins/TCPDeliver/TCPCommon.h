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

#ifndef TCP_Common_h
#define TCP_Common_h

#define TCPDELIVER_MAJOR 1
#define TCPDELIVER_MINOR 1


enum {
  REQUEST_PING = 1,
  REQUEST_PONG = 2,
  REQUEST_VERSION = 3,
  REQUEST_DISCONNECT = 4,
  REQUEST_NOMORESOCKETS = 5,

  CLIENT_SENDFRAME = 20,
  CLIENT_SENDAUDIO = 21,
  CLIENT_VIDEOINFO = 22,
  CLIENT_PREPAREFRAME = 23,

  SERVER_VIDEOINFO = 30,
  SERVER_SENDING_FRAME = 31,
  SERVER_SENDING_AUDIO = 32,
  SERVER_DATA_BLOCK = 33,

  SERVER_NEXT_PLANE = 34
};

struct ServerFrameInfo {
  unsigned int framenumber;
  unsigned int row_size;
  unsigned int height;
  unsigned int compressed_bytes;
  unsigned int compression;
  unsigned int crc;
  unsigned int reserved1;
  unsigned int reserved2;
  unsigned int reserved3;
  unsigned int reserved4;
  unsigned int reserved5;
  unsigned int reserved6;
  unsigned int reserved7;
  unsigned int reserved8;

  enum {
    COMPRESSION_NONE = 0
  };

};

struct ServerAudioInfo {
  unsigned int comporessed_bytes;
  unsigned int compression;
  unsigned int reserved1;
  unsigned int reserved2;
  unsigned int reserved3;
  unsigned int reserved4;
  unsigned int reserved5;
  unsigned int reserved6;
  unsigned int reserved7;
  unsigned int reserved8;

  enum {
    COMPRESSION_NONE = 0
  };

};

struct ClientRequestAudio {
  __int64 start;
  __int64 count;  
  unsigned int bytes;
  unsigned int reserved1;
  unsigned int reserved2;
  unsigned int reserved3;
  unsigned int reserved4;
  unsigned int reserved5;
  unsigned int reserved6;
  unsigned int reserved7;
  unsigned int reserved8;
};

struct ClientRequestFrame {
  unsigned int n;
  unsigned int reserved1;
  unsigned int reserved2;
  unsigned int reserved3;
  unsigned int reserved4;
  unsigned int reserved5;
  unsigned int reserved6;
  unsigned int reserved7;
  unsigned int reserved8;
};


/***********************************************************************
// adler32 checksum
// adapted from free code by Mark Adler <madler@alumni.caltech.edu>
// see http://www.cdrom.com/pub/infozip/zlib/
************************************************************************/

#define TCPD_BASE 65521u /* largest prime smaller than 65536 */
#define TCPD_NMAX 5552
/* NMAX is the largest n such that 255n(n+1)/2 + (n+1)(BASE-1) <= 2^32-1 */


unsigned __int32 adler32(unsigned int adler, const unsigned char* buf, unsigned int len);
 

#endif
