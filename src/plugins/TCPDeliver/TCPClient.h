// Avisynth v2.5.  Copyright 2002 Ben Rudiak-Gould et al.
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


#ifndef TCP_Client_h
#define TCP_Client_h


//#include <afxmt.h>
#include "TCPCommon.h"
#include <winsock2.h>
#include "avisynth.h"

AVSValue __cdecl Create_TCPClient(AVSValue args, void* user_data, IScriptEnvironment* env);

/*
struct ClientRequest {
  BYTE* data;
  unsigned int dataSize;

  void allocateBuffer(int bytes) {
    data = new BYTE[bytes];
    dataSize = bytes;
  }

  void freeBuffer() {
    if (dataSize)
      delete[] data;
    dataSize = 0;
  }

};
*/
/*
class TCPClientSynchronization {
public:
  BYTE* requestdata;
  BYTE* videoData;
  BYTE* audioData;
  VideoInfo vi;
  bool parity;  
};
*/

class TCPClientThread {
public:
  TCPClientThread(const char* pass_server_name, int pass_port, IScriptEnvironment* env);
  void StartRequestLoop();
  // Interfaces for unthreaded communication.
  void SendRequest(char requestId, void* data, unsigned int bytes);
  void TCPClientThread::GetReply();


  HANDLE evtClientReadyForRequest;   // Client is ready to recieve a new request.
  HANDLE evtClientReplyReady;        // Client has finished processing the last request.
  HANDLE evtClientProcesRequest;     // Client should process the passed request
  char* last_reply;
  unsigned int last_reply_bytes;
  char last_reply_type;
  bool disconnect;

private:
  void RecievePacket();
  void RecieveDataBlock();
  char* client_request;
  unsigned int client_request_bytes;
  WSADATA wsaData;
  SOCKET m_socket;
  sockaddr_in service;  
};

UINT StartClient(LPVOID p);

class TCPClient  : public IClip {
public:
  TCPClient(const char* _hostname, int _port, IScriptEnvironment* env);
  ~TCPClient();
  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
  void __stdcall GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env);
  const VideoInfo& __stdcall GetVideoInfo();
  bool __stdcall GetParity(int n);
  void __stdcall SetCacheHints(int cachehints,int frame_range) { } ; 

  TCPClientThread* client;
private:
  const char* hostname;
  int port;
  VideoInfo vi;
  HANDLE ClientThread;

};

#endif