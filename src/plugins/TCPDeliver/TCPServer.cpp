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


#include "TCPServer.h"

HANDLE hThread;  
HWND hDlg;  // Windowhandle

#include "ServerGUICode.h"

TCPServer::TCPServer(PClip _child, int port, IScriptEnvironment* env) : GenericVideoFilter(_child) {
  //LPDWORD ThreadId = 0;
//	DWORD id;
//  hInstance=(HINSTANCE)hModule; 
  s = new TCPServerListener(port, child, env);
//	if(!hThread) hThread=CreateThread(NULL, 10000, (unsigned long (__stdcall *)(void *))startWindow, 0, 0 , &id );
//  startWindow();
}

TCPServer::~TCPServer() {
  DWORD dwExitCode = 0;
  s->KillThread();
}


AVSValue __cdecl Create_TCPServer(AVSValue args, void* user_data, IScriptEnvironment* env) {
  return new TCPServer(args[0].AsClip(), args[1].AsInt(22050), env);
}


/******** Server Thread    *******/

UINT StartServer(LPVOID p) {
  TCPServerListener* t = (TCPServerListener*)p;
  t->Listen();
  return 0;
}


TCPServerListener::TCPServerListener(int port, PClip _child, IScriptEnvironment* _env) : child(_child), env(_env) {

  int iResult = WSAStartup( MAKEWORD(2,2), &wsaData );
  if ( iResult != NO_ERROR )
    env->ThrowError("TCPServer: Could not start up Winsock.");

  m_socket = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );

  if ( m_socket == INVALID_SOCKET ) {
    env->ThrowError("TCPServer: Could not create socket()");
    WSACleanup();
    return;
  }

  hostent* localHost;
  char* localIP;

  // Get the local host information
  localHost = gethostbyname("");
  localIP = inet_ntoa (*(struct in_addr *)*localHost->h_addr_list);


  // Set up the sockaddr structure
  service.sin_family = AF_INET;
  service.sin_addr.s_addr = INADDR_ANY;
  service.sin_port = htons(port);

  if ( bind( m_socket, (SOCKADDR*) &service, sizeof(service) ) == SOCKET_ERROR ) {
    closesocket(m_socket);
    env->ThrowError("TCPServer: bind() failed." );
    return;
  }

  if ( listen( m_socket, 1 ) == SOCKET_ERROR )
    env->ThrowError("TCPServer: Error listening on socket.\n");

  shutdown = false;

  AfxBeginThread(StartServer, this , THREAD_PRIORITY_NORMAL,0,0,NULL);

}


void TCPServerListener::Listen() {
  _RPT0(0,"TCPServer: Starting to Listen\n");
  fd_set test_set;

  timeval t;
  t.tv_sec = 0;
  t.tv_usec = 0; 

  ClientConnection s_list[FD_SETSIZE];
  int i;

  ServerReply s;

  for (i = 0; i< FD_SETSIZE ; i++)
    memset(&s_list[i], 0, sizeof(ClientConnection));

  while (!shutdown) {
    // Attempt to Accept an incoming request

    SOCKET AcceptSocket = SOCKET_ERROR;

    FD_ZERO(&test_set);
    FD_SET(m_socket, &test_set);
    select(0, &test_set, NULL, NULL, &t);

    if (FD_ISSET(m_socket, &test_set)) {
      AcceptSocket = accept( m_socket, NULL, NULL );
      _RPT0(0,"TCPServer: Client Connected.\n");
    }

    if (AcceptSocket != SOCKET_ERROR ) {

      // Find free slot
      int slot = -1;
      for (i = FD_SETSIZE ; i>=0; i--)
        if (!s_list[i].isConnected)
          slot = i;

      if (slot >= 0 ) {
        s_list[slot].s = AcceptSocket;
        s_list[slot].isConnected = true;
      } else {
        _RPT0(0,"TCPServer: All slots full.\n");
        s.allocateBuffer(1);
        s.data[0] = REQUEST_NOMORESOCKETS;
        send(AcceptSocket, (const char*)s.data, s.dataSize, 0);
        closesocket(AcceptSocket);
        s.freeBuffer();
      }
    }

    FD_ZERO(&test_set);

    for (i = 0; i< FD_SETSIZE; i++) 
      if(s_list[i].isConnected) 
        FD_SET(s_list[i].s, &test_set);

    select(0, &test_set, NULL, NULL, &t);
    bool request_handled = false;

    for (i = 0; i< FD_SETSIZE; i++) {
      if(s_list[i].isConnected) {
        if (FD_ISSET(s_list[i].s, &test_set) && (!s_list[i].isDataPending)) {

          request_handled = true;
          TCPRecievePacket* t = new TCPRecievePacket(s_list[i].s);
          
          _RPT1(0, "TCPServer: Bytes Recv: %ld\n", t->dataSize );
          
          if (!t->isDisconnected) {
            s.dataSize = 0;
            s.client = &s_list[i];  // Add client info to serverreply.
            Receive((const char*)t->data, t->dataSize, &s);

            if (s.dataSize > 0) {
              SendPacket(&s_list[i], &s);
            } // end if datasize > 0

          } else { // isDisconnected
            _RPT0(0,  "TCPServer: Connection Closed.\n");
            closesocket(s_list[i].s);
            s_list[i].isConnected = false;
          }
          delete t;
        } // end if fd is set
        if (s_list[i].isDataPending) {
          request_handled = true;
          SendPendingData(&s);
        } // end if isDataPending
      } // end if list != null
    } // end for i
    if (!request_handled) Sleep(1);  // If there has been nothing to do we might as well wait a bit.
  } // while !shutdown
}


void TCPServerListener::SendPacket(ClientConnection* cc, ServerReply* s) {
   _RPT0(0, "TCPServer: Sending packet.\n");    

  unsigned int BytesSent = 0;
  int r = 1;
  send(cc->s, (const char*)&s->dataSize, 4, 0);
  while (r > 0) {
    r = send(cc->s, (const char*)(&s->internal_data[BytesSent]), s->dataSize-BytesSent, 0);
    if (r == SOCKET_ERROR) {
      _RPT0(0, "TCPServer: Could not send packet (SOCKET_ERROR). Connection closed\n", );
      closesocket(cc->s);
      cc->isConnected = false;
    }
    BytesSent += r;                
  } // end while
  if (BytesSent != s->dataSize) {
    _RPT2(0, "TCPServer: Failed in sending %d bytes - could only send %d bytes!\n", s->dataSize, BytesSent);
    closesocket(cc->s);
    cc->isConnected = false;
  }
  s->freeBuffer();
  _RPT0(0, "TCPServer: Packet sent succesfully.\n");    
}


void TCPServerListener::Receive(const char* recvbuf, int bytes, ServerReply* s) {
  switch (recvbuf[0]) {
    case REQUEST_PING:
      _RPT0(0, "TCPServer: Received Ping? - returning Pong!\n");
      s->allocateBuffer(bytes);
      memcpy(s->data, recvbuf, bytes);
      s->data[0] = REQUEST_PONG;
      break;

    case CLIENT_SEND_VIDEOINFO:
      SendVideoInfo(s);
      break;
    case CLIENT_REQUEST_FRAME:
      SendFrameInfo(s, &recvbuf[1]);
      break;
    case CLIENT_REQUEST_AUDIO:
      SendAudioInfo(s, &recvbuf[1]);
      break;
    case CLIENT_SEND_FRAME:
      SendVideoFrame(s);
      break;
    case CLIENT_SEND_AUDIO:
      SendAudioData(s);
      break;
    default:
      _RPT1(1,"TCPServer: Could not handle request type %d.", recvbuf[0]);
      break;
  }
}

void TCPServerListener::SendPendingData(ServerReply* s) {
  ClientConnection* cc = s->client;
  if (!cc->isDataPending) {
    return;
  }
  int bytes_left = cc->totalPendingBytes - cc->pendingBytesSent;
  int send_bytes = max(0, min(bytes_left, 32*1024));
  s->allocateBuffer(send_bytes);
  s->setType(SERVER_SPLIT_BLOCK);
  memcpy(s->data, &cc->pendingData[cc->pendingBytesSent], send_bytes);
  SendPacket(cc, s);
  cc->pendingBytesSent += send_bytes;

  if (cc->pendingBytesSent >= cc->totalPendingBytes) {
    cc->isDataPending = false;
    delete[] cc->pendingData;
    s->allocateBuffer(0);
    s->setType(SERVER_END_SPLIT_BLOCK);
    SendPacket(cc, s);
  }
}

void TCPServerListener::SendAudioData(ServerReply* s) {
  s->client->isDataPending = true;
  s->allocateBuffer(0);
  s->setType(SERVER_SENDING_AUDIO);
}

void TCPServerListener::SendVideoFrame(ServerReply* s) {
  s->client->isDataPending = true;
  s->allocateBuffer(0);
  s->setType(SERVER_SENDING_FRAME);
}

void TCPServerListener::SendVideoInfo(ServerReply* s) {
  _RPT0(0, "TCPServer: Sending VideoInfo!\n");

  s->allocateBuffer(sizeof(VideoInfo));
  s->setType(SERVER_VIDEOINFO);
  memcpy(s->data, &child->GetVideoInfo(), sizeof(VideoInfo));
}


// Silly way of handling requests. A real mess.
// Requests should optimally be handled by a separate thread to avoid blocking other clients while requesting the frame.
void TCPServerListener::SendFrameInfo(ServerReply* s, const char* request) {
  _RPT0(0, "TCPServer: Sending Frame Info!\n");
  ClientRequestFrame f;
  memcpy(&f, request, sizeof(ClientRequestFrame));
  s->allocateBuffer(sizeof(ServerFrameInfo));
  s->setType(SERVER_FRAME_INFO);
  PVideoFrame src = child->GetFrame(f.n, env);
  
  ServerFrameInfo sfi;
  memset(&sfi, 0, sizeof(ServerFrameInfo));
  sfi.framenumber = f.n;
  sfi.compression = ServerFrameInfo::COMPRESSION_NONE;
  sfi.height = src->GetHeight();
  sfi.row_size = src->GetRowSize();
  sfi.pitch = src->GetPitch();

  int data_size = sfi.height * sfi.pitch;
  if (child->GetVideoInfo().IsYV12()) {
    data_size = data_size + data_size / 2;
  }
  sfi.data_size = data_size;

  // Prepare data
  const BYTE* srcp = src->GetReadPtr();
  int src_pitch = src->GetPitch();
  int src_height = src->GetHeight();
  int src_rowsize = src->GetRowSize();

  if (child->GetVideoInfo().IsYV12()) {
    data_size = data_size + data_size / 2;
  }
  s->client->totalPendingBytes = data_size;
  s->client->pendingBytesSent = 0;

  BYTE* dstp = s->client->pendingData = new BYTE[data_size];
  env->BitBlt(dstp, src_pitch, srcp, src_pitch, src_rowsize, src_height);

  if (child->GetVideoInfo().IsYV12()) {
    dstp += src_height * src_pitch;
    srcp = src->GetReadPtr(PLANAR_U);
    env->BitBlt(dstp, src_pitch/2, srcp, src_pitch/2, src_rowsize/2, src_height/2);

    dstp += src_height/2 * src_pitch/2;
    srcp = src->GetReadPtr(PLANAR_U);
    env->BitBlt(dstp, src_pitch/2, srcp, src_pitch/2, src_rowsize/2, src_height/2);
  }
  
  // Send Reply
  memcpy(s->data, &sfi, sizeof(ServerFrameInfo));
}

void TCPServerListener::SendAudioInfo(ServerReply* s, const char* request) {
  _RPT0(0, "TCPServer: Sending Audio Info!\n");
  ClientRequestAudio a;
  memcpy(&a, request, sizeof(ClientRequestAudio));
  s->allocateBuffer(sizeof(ServerAudioInfo));
  s->setType(SERVER_AUDIO_INFO);

  if (a.bytes != child->GetVideoInfo().BytesFromAudioSamples(a.count))
    _RPT0(1, "TCPServer: Did not recieve proper bytecount.\n");

  BYTE* buf = s->client->pendingData = new BYTE[a.bytes];
  
  child->GetAudio(buf, a.start, a.count, env);

  ServerAudioInfo sfi;
  sfi.compression = ServerAudioInfo::COMPRESSION_NONE;
  sfi.comporessed_bytes = a.bytes;

  s->client->pendingData = buf;
  s->client->totalPendingBytes = a.bytes;
  s->client->pendingBytesSent = 0;

  memcpy(s->data, &sfi, sizeof(ServerAudioInfo));
}

void TCPServerListener::KillThread() {
  shutdown = true;
}


TCPRecievePacket::TCPRecievePacket(SOCKET _s) : s(_s) {
  isDisconnected = false;
  int recieved = 0;

  while (recieved < 4) {
    int bytesRecv = recv(s, (char*)&dataSize+recieved, 4-recieved, 0 );
    if (bytesRecv == WSAECONNRESET || bytesRecv <= 0) {
      _RPT0(0, "TCPServer: Could not retrieve packet size!");
      isDisconnected = true;
      return;
    }
    recieved += bytesRecv;
  }

  if (dataSize <= 0) {
    _RPT0(1, "TCPServer: Packet size less than 0!");
    isDisconnected = true;
    dataSize = 0;
    return;
  }

  data = (BYTE*)malloc(dataSize);
  recieved = 0;
  while (recieved < dataSize) {
    int bytesRecv = recv(s, (char*)&data[recieved], dataSize-recieved, 0 );
    if (bytesRecv == WSAECONNRESET || bytesRecv <= 0) {
      _RPT0(0, "TCPServer: Could not retrieve packet data!");
      isDisconnected = true;
      return;
    }
    recieved += bytesRecv;
  }
}

TCPRecievePacket::~TCPRecievePacket() {
  if (dataSize >0)
    free(data);
}

