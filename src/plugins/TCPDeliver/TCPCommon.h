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

#define TCPDELIVER_MAJOR 3
#define TCPDELIVER_MINOR 1

#include "stdafx.h"

/*****************************************************************
 General overview:

  This client is designes as a universal way of transfering (un)compressed
 video via TCP LAN. The client is in no way intended to be used over slow
 connections due to the datasizes.

  The system is based on the TCP protocol and implemented using WinSock2
  All data sent is made platform independant, but assume little endian (Intel).

  Data is sent as packaged TCP streams, and each packet is deviced like this:

  0-4: Packet size EXCLUDING these four bytes.[unsigned int32]. 
  5: Packet type.
  6: Packet data.

  Next packet is assumed to start right after the packet data.
  There is no fixed maximum packet type, but large data amounts 
  (like frames and samples) are split up into subpackets.

 *****************************************************************/

enum {
  REQUEST_PING = 1,
  REQUEST_PONG = 2,
  REQUEST_DISCONNECT = 4,       // Request to disconnect.
  REQUEST_NOMORESOCKETS = 5,    // Returned to the client if no more sockets are available.
  REQUEST_CONNECTIONACCEPTED = 6,    // Returned to the client when it has attempted connection and succeeded.
  CLIENT_CHECK_VERSION = 7,

  CLIENT_REQUEST_FRAME = 10,    
    // Client would like to have information about frame 'n', which is prepared to be sent.
    // This will not send the actual data - only information about the data to be sent.
    // Client should send a ClientRequestFrame struct, Server should return a ServerFrameInfo.
  CLIENT_REQUEST_AUDIO = 11,    
  // Same as above
  // Client should send a ClientRequestAudio struct, Server should return a ServerAudioInfo.
  CLIENT_SEND_VIDEOINFO = 22,
  // This will make the server send an AviSynth 2.5 struct containing information about the video stream (VideoInfo).
  CLIENT_SEND_PARITY = 23,

  SERVER_VIDEOINFO = 30,
  // Server send an AviSynth 2.5 VideoInfo struct containing information about the movie.
  SERVER_SENDING_FRAME = 31,
  SERVER_SENDING_AUDIO = 32,
  SERVER_SENDING_PARITY = 35,

  INTERNAL_DISCONNECTED = 51
};

/**********************
  ServerFrameInfo:
  Information returned by the server
  after a client has requested a frame.

  Note that on planar images the information of
  row_size, width, height is the information
  about the first plane.
 **********************/

struct ServerFrameInfo {
  unsigned int framenumber;       // The framenumber to be delivered.
  unsigned int row_size;          // The width of a used pixels of a line in bytes.
  unsigned int row_sizeUV;          // The width of a used pixels of a line in bytes.
  unsigned int height;            // The height of the image in pixels 
  unsigned int heightUV;            // The height of the image in pixels 
  unsigned int pitch;             // The length of a line in bytes.
  unsigned int pitchUV;           // The length of a line in bytes.
  unsigned int compressed_bytes;  // The number of bytes to be transfered after compression.
  unsigned int compression;       // The compression sheme used.
  unsigned int data_size;         // Total size of the uncompressed image in bytes.
  unsigned int comp_Y_bytes;      // Compressed size of Y-plane (in planar mode)
  unsigned int comp_U_bytes;      // U-plane
  unsigned int comp_V_bytes;      // V-plane

  unsigned int reserved1;
  unsigned int reserved2;
  unsigned int reserved3;
  unsigned int reserved4;

  enum {
    COMPRESSION_NONE = 0,         // The image is sent as uncompressed bytes.
    COMPRESSION_DELTADOWN_LZO = 1<<0,    // The image is sent as downwards-delta + lzo compressed data.
    COMPRESSION_DELTADOWN_HUFFMAN = 1<<1, // The image is sent as downwards-delta + huffman compressed.
    COMPRESSION_DELTADOWN_GZIP = 1<<2 // The image is sent as downwards-delta + GZip huffman compressed.
  };
};

/**********************
  ServerAudioInfo:
 Information about a collection of
 samples that has been requested by the
 client.
 **********************/
struct ServerAudioInfo {
  unsigned int compressed_bytes;  // The number of bytes sent after compression
  unsigned int compression;       // Compression sheme used.
  unsigned int data_size;         // Total size of the uncompressed samples in bytes.

  unsigned int reserved1;
  unsigned int reserved2;
  unsigned int reserved3;
  unsigned int reserved4;

  enum {
    COMPRESSION_NONE = 0
  };

};

struct ServerParityReply {
  unsigned int n;       // The number of the frame requested.
  unsigned char parity;  // Parity information
};

/**********************
  ClientRequestAudio:
 A request by the client to send a 
 cumber of samples.
 **********************/
struct ClientRequestAudio {
  __int64 start;        // The offset of the first sample to be returned (in samples, not multiplied by the number of channels).
  __int64 count;        // The number of samples to be fetched.
  unsigned int bytes;   // The expected number of bytes

  unsigned int reserved1;
  unsigned int reserved2;
};

struct ClientRequestFrame {
  unsigned int n;       // The number of the frame requested.

  unsigned int reserved1;
  unsigned int reserved2;
};

struct ClientRequestParity {
  unsigned int n;       // The number of the frame requested.
};

struct ClientCheckVersion {
  int major;
  int minor;
  int compression_supported;
  unsigned int reserved2;
  unsigned int reserved3;
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
 
static HINSTANCE hInstance;

#endif
