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

/**************************  TCP Server *****************************
  The server is basicly a thread that is spawned.

  The server is optimized for one client, but allows several clients to
  be connected and requesting frames.

  The server is "dumb". It will only respond to requests from clients,
  and not do any "thinking" by itself.
 ********************************************************************/


TCPServer::TCPServer(PClip _child, int port, IScriptEnvironment* env) : GenericVideoFilter(_child) {

  _RPT0(0, "TCPServer: Opening instance\n");
  s = new TCPServerListener(port, child, env);
  //  if(!hThread) hThread=CreateThread(NULL, 10000, (unsigned long (__stdcall *)(void *))startWindow, 0, 0 , &id );
  //  startWindow();
}

TCPServer::~TCPServer() {
  _RPT0(0, "TCPServer: Killing thread.\n");
  DWORD dwExitCode = 0;
  s->KillThread();
  while (s->thread_running) {
    Sleep(10);
  }
  _RPT0(0, "TCPServer: Thread killed.\n");
  delete s;
}


AVSValue __cdecl Create_TCPServer(AVSValue args, void* user_data, IScriptEnvironment* env) {
  return new TCPServer(args[0].AsClip(), args[1].AsInt(22050), env);
}


/********** Server Thread **********/

UINT StartServer(LPVOID p) {
  TCPServerListener* t = (TCPServerListener*)p;
  t->Listen();
  return 0;
}

/*********************************
  class TCPServerListener

  This will open the server socket.
  This function is not part of the server
  thread, and should be done before starting the
  thread.

  Thread is spawned if the server has been
  created successfully.
 *********************************/

TCPServerListener::TCPServerListener(int port, PClip _child, IScriptEnvironment* _env) : child(_child), env(_env) {

  thread_running = false;

  int iResult = WSAStartup( MAKEWORD(2, 2), &wsaData );
  if ( iResult != NO_ERROR )
    env->ThrowError("TCPServer: Could not start up Winsock.");

  m_socket = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );

  if ( m_socket == INVALID_SOCKET ) {
    env->ThrowError("TCPServer: Could not create socket()");
    WSACleanup();
    return ;
  }

  hostent* localHost;
  char* localIP;

  // Get the local host information
  localHost = gethostbyname("");
  localIP = inet_ntoa (*(struct in_addr *) * localHost->h_addr_list);


  // Set up the sockaddr structure
  service.sin_family = AF_INET;
  service.sin_addr.s_addr = INADDR_ANY;
  service.sin_port = htons(port);

  if ( bind( m_socket, (SOCKADDR*) &service, sizeof(service) ) == SOCKET_ERROR ) {
    closesocket(m_socket);
    env->ThrowError("TCPServer: bind() failed." );
    return ;
  }

  if ( listen( m_socket, 1 ) == SOCKET_ERROR )
    env->ThrowError("TCPServer: Error listening on socket.\n");

  shutdown = false;

  AfxBeginThread(StartServer, this , THREAD_PRIORITY_NORMAL, 0, 0, NULL);

  thread_running = true;

}
/*************************
  TCPServerListener::Listen()

  This is the main loop of the server, and this will
  run as long as the server exists.

  The main loop is running and checks each input socket
  for new connections or client requests. One request
  per client is handled per loop.

  The client might have data pending, which is sent
  in minor blocks to avoid blocking all clients
  if one client is on a slower connection.
 *************************/

void TCPServerListener::Listen() {
  _RPT0(0, "TCPServer: Starting to Listen\n");
  fd_set test_set;

  timeval t;
  t.tv_sec = 0;
  t.tv_usec = 0;

  ClientConnection s_list[FD_SETSIZE];
  int i;
  if (lzo_init() != LZO_E_OK) { 
    env->ThrowError("TCPServer: Could not initialize LZO compression library!");
  }
  ServerReply s;

  for (i = 0; i < FD_SETSIZE ; i++)
    memset(&s_list[i], 0, sizeof(ClientConnection));

  while (!shutdown) {
    // Attempt to Accept an incoming request

    FD_ZERO(&test_set);
    FD_SET(m_socket, &test_set);
    select(0, &test_set, NULL, NULL, &t);

    if (FD_ISSET(m_socket, &test_set)) {
      AcceptClient(accept( m_socket, NULL, NULL ), &s_list[0]);
      _RPT0(0, "TCPServer: Client Connected.\n");
    }


    FD_ZERO(&test_set);

    for (i = 0; i < FD_SETSIZE; i++)
      if (s_list[i].isConnected)
        FD_SET(s_list[i].s, &test_set);

    select(0, &test_set, NULL, NULL, &t);
    bool request_handled = false;

    for (i = 0; i < FD_SETSIZE; i++) {
      s.dataSize = 0;
      if (s_list[i].isConnected) {
        if (FD_ISSET(s_list[i].s, &test_set) && (!s_list[i].isDataPending)) {

          request_handled = true;
          TCPRecievePacket* tr = new TCPRecievePacket(s_list[i].s);

          _RPT1(0, "TCPServer: Bytes Recv: %ld\n", tr->dataSize );

          if (!tr->isDisconnected) {
            s.dataSize = 0;
            s.client = &s_list[i];  // Add client info to serverreply.
            Receive(tr, &s);

            if (s.dataSize > 0) {
              SendPacket(&s_list[i], &s);
            } // end if datasize > 0

          }
          else { // isDisconnected
            _RPT0(0, "TCPServer: Connection Closed.\n");
            closesocket(s_list[i].s);
            s_list[i].reset();
          }
          delete tr;
        } // end if fd is set
      } // end if list != null
    } // end for i


    FD_ZERO(&test_set);

    for (i = 0; i < FD_SETSIZE; i++)
      if (s_list[i].isConnected && s_list[i].isDataPending)
        FD_SET(s_list[i].s, &test_set);

    select(0, NULL, &test_set, NULL, &t);  // Can we send?
    for (i = 0; i < FD_SETSIZE; i++) {
      if (FD_ISSET(s_list[i].s, &test_set) && s_list[i].isDataPending ) {
        request_handled = true;
        SendPendingData(&s_list[i]);
      } // end if isDataPending
    }

    if (!request_handled)
      Sleep(1);  // If there has been nothing to do we might as well wait a bit.
  } // while !shutdown

  closesocket(m_socket);
  WSACleanup();
  thread_running = false;
  _RPT0(0, "TCPServer: Client thread no longer running.\n");
}


void TCPServerListener::AcceptClient(SOCKET AcceptSocket, ClientConnection* s_list) {
  ServerReply s;
  // Find free slot
  int slot = -1;
  for (int i = FD_SETSIZE ; i >= 0; i--)
    if (!s_list[i].isConnected)
      slot = i;

  if (slot >= 0 ) {
    s_list[slot].reset();
    s_list[slot].s = AcceptSocket;
    s_list[slot].isConnected = true;

  } else {
    _RPT0(0, "TCPServer: All slots full.\n");
    s.allocateBuffer(0);
    s.data[0] = REQUEST_NOMORESOCKETS;
    send(AcceptSocket, (const char*)s.data, s.dataSize, 0);
    closesocket(AcceptSocket);
    s.freeBuffer();
  }
}

/*******************************
  TCPServerListener::SendPacket

  This function will send the data constructed in
  a ServerReply to the client.

  This function is always called if there has been
  a request from a client.
 *******************************/

void TCPServerListener::SendPacket(ClientConnection* cc, ServerReply* s) {
  _RPT0(0, "TCPServer: Sending packet.\n");

  unsigned int BytesSent = 0;
  send(cc->s, (const char*)&s->dataSize, 4, 0);
  cc->pendingData = s->internal_data;
  cc->isDataPending = !!s->dataSize;
  cc->totalPendingBytes = s->dataSize;
  cc->pendingBytesSent = 0;

  _RPT0(0, "TCPServer: Packet set to pending.\n");
}

/****************************
  TCPServerListener::Receive

  This function will select a proper
  reply for an incoming client request.
 ****************************/

void TCPServerListener::Receive(TCPRecievePacket* tr, ServerReply* s) {
  const char recvtype = tr->data[0];
  const char* recvbuf = (const char*) & tr->data[1];
  unsigned int bytes = tr->dataSize - 1;

  switch (recvtype) {
  case REQUEST_PING:
    _RPT0(0, "TCPServer: Received Ping? - returning Pong!\n");
    s->allocateBuffer(16);
    memcpy(s->data, recvbuf, min(16, bytes));
    s->setType(REQUEST_PONG);
    break;

  case CLIENT_SEND_VIDEOINFO:
    SendVideoInfo(s);
    break;
  case CLIENT_REQUEST_FRAME:
    SendFrameInfo(s, recvbuf);
    break;
  case CLIENT_REQUEST_AUDIO:
    SendAudioInfo(s, recvbuf);
    break;
  case CLIENT_SEND_PARITY:
    SendParityInfo(s, recvbuf);
    break;
  case REQUEST_DISCONNECT:
    tr->isDisconnected = true;
    break;
  case CLIENT_CHECK_VERSION:
    CheckClientVersion(s, recvbuf);
    break;
  default:
    _RPT1(1, "TCPServer: Could not handle request type %d.", recvbuf[0]);
    break;
  }
}

/*****************************
  TCPServerListener::SendPendingData

  This function will send (a part of) any
  pending data to a client.

  A maximum blocksize is hidden somewhere
  in the code below :)
 *****************************/

void TCPServerListener::SendPendingData(ClientConnection* cc) {
  if (!cc->isDataPending) {
    return ;
  }
  int bytes_left = cc->totalPendingBytes - cc->pendingBytesSent;
  int send_bytes = max(0, min(bytes_left, 64 * 1024));
  //  int send_bytes = bytes_left;

  int r = send(cc->s, (const char*)(&cc->pendingData[cc->pendingBytesSent]), send_bytes, 0);
  if (r == SOCKET_ERROR || r < 0) {
    _RPT0(0, "TCPServer: Could not send packet (SOCKET_ERROR). Connection closed\n", );
    closesocket(cc->s);
    cc->isConnected = false;
    cc->totalPendingBytes = 0;
    return;
  }

  cc->pendingBytesSent += r;

  if (cc->pendingBytesSent >= cc->totalPendingBytes) {
    cc->isDataPending = false;
    delete[] cc->pendingData;
    cc->totalPendingBytes = 0;
  }

}

void TCPServerListener::CheckClientVersion(ServerReply* s, const char* request) {
  ClientCheckVersion* ccv = (ClientCheckVersion*)request;
  s->allocateBuffer(0);
  if (ccv->major != TCPDELIVER_MAJOR) {
    s->setType(REQUEST_DISCONNECT);
  } else {
    s->setType(REQUEST_CONNECTIONACCEPTED);
  }
  if (ccv->compression_supported & ServerFrameInfo::COMPRESSION_DELTADOWN_HUFFMAN) {
    delete s->client->compression;
    s->client->compression = new PredictDownHuffman();
  } else if (ccv->compression_supported & ServerFrameInfo::COMPRESSION_DELTADOWN_LZO) {
    delete s->client->compression;
    s->client->compression = new PredictDownLZO();
  }
}

void TCPServerListener::SendVideoInfo(ServerReply* s) {
  _RPT0(0, "TCPServer: Sending VideoInfo!\n");

  s->allocateBuffer(sizeof(VideoInfo));
  s->setType(SERVER_VIDEOINFO);
  memcpy(s->data, &child->GetVideoInfo(), sizeof(VideoInfo));
}


void TCPServerListener::SendParityInfo(ServerReply* s, const char* request) {
  ClientRequestParity* p = (ClientRequestParity *) request;
  ServerParityReply r;
  memset(&r, 0, sizeof(ServerParityReply));
  r.n = p->n;
  r.parity = child->GetParity(p->n);
  s->allocateBuffer(sizeof(ServerParityReply));
  s->setType(SERVER_SENDING_PARITY);
  memcpy(s->data, &r, sizeof(ServerParityReply));
}


// Requests should optimally be handled by a separate thread to avoid blocking other clients while requesting the frame.
void TCPServerListener::SendFrameInfo(ServerReply* s, const char* request) {
  _RPT0(0, "TCPServer: Sending Frame Info!\n");
  ClientRequestFrame f;
  memcpy(&f, request, sizeof(ClientRequestFrame));
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

  // Prepare data
  const BYTE* srcp = src->GetReadPtr();
  int src_pitch = src->GetPitch();
  int src_height = src->GetHeight();
  int src_rowsize = src->GetRowSize();

  if (child->GetVideoInfo().IsYV12()) {
    data_size = data_size + data_size / 2;
  }

  BYTE* dstp;
  sfi.data_size = data_size;

  if (!child->GetVideoInfo().IsPlanar()) {
    env->MakeWritable(&src);
    
    sfi.compression = s->client->compression->compression_type;
    sfi.compressed_bytes = s->client->compression->CompressImage(src->GetWritePtr(), src_rowsize, src_height, src_pitch);

    s->allocateBuffer(sizeof(ServerFrameInfo) + sfi.compressed_bytes);
    dstp = s->data + sizeof(ServerFrameInfo);
    env->BitBlt(dstp, 0, s->client->compression->dst, 0, sfi.compressed_bytes, 1);

    if (!s->client->compression->inplace) {
      _aligned_free(s->client->compression->dst);
    }
   
  } else {
    env->MakeWritable(&src);
    BYTE *dst1, *dst2, *dst3;

    sfi.comp_Y_bytes = s->client->compression->CompressImage(src->GetWritePtr(PLANAR_Y), src_rowsize, src_height, src_pitch);
    dst1 = s->client->compression->dst;

    sfi.comp_U_bytes = s->client->compression->CompressImage(src->GetWritePtr(PLANAR_U), src_rowsize/2, src_height/2, src_pitch/2);  //FIXME: YV12 only!
    dst2 = s->client->compression->dst;

    sfi.comp_V_bytes = s->client->compression->CompressImage(src->GetWritePtr(PLANAR_V), src_rowsize/2, src_height/2, src_pitch/2);  //FIXME: YV12 only!
    dst3 = s->client->compression->dst;

    sfi.compressed_bytes = sfi.comp_Y_bytes + sfi.comp_U_bytes + sfi.comp_V_bytes;
    s->allocateBuffer(sizeof(ServerFrameInfo) + sfi.compressed_bytes);

    dstp = s->data + sizeof(ServerFrameInfo);
    sfi.compression = s->client->compression->compression_type;

    env->BitBlt(dstp, 0, dst1, 0, sfi.comp_Y_bytes, 1);
    env->BitBlt(&dstp[sfi.comp_Y_bytes], 0, dst2, 0, sfi.comp_U_bytes, 1);
    env->BitBlt(&dstp[sfi.comp_Y_bytes+sfi.comp_U_bytes], 0, dst3, 0, sfi.comp_V_bytes, 1);

    if (!s->client->compression->inplace) {
      _aligned_free(dst1);
      _aligned_free(dst2);
      _aligned_free(dst3);
    }
  }

  s->setType(SERVER_SENDING_FRAME);

  // Send Reply
  memcpy(s->data, &sfi, sizeof(ServerFrameInfo));
}

// 28 98 88 12

void TCPServerListener::SendAudioInfo(ServerReply* s, const char* request) {
  _RPT0(0, "TCPServer: Sending Audio Info!\n");
  ClientRequestAudio a;
  memcpy(&a, request, sizeof(ClientRequestAudio));
  s->allocateBuffer(sizeof(ServerAudioInfo) + a.bytes);
  s->setType(SERVER_SENDING_AUDIO);

  if (a.bytes != child->GetVideoInfo().BytesFromAudioSamples(a.count))
    _RPT0(1, "TCPServer: Did not recieve proper bytecount.\n");

  ServerAudioInfo sfi;
  sfi.compression = ServerAudioInfo::COMPRESSION_NONE;
  sfi.compressed_bytes = a.bytes;

  memcpy(s->data, &sfi, sizeof(ServerAudioInfo));
  child->GetAudio(s->data + sizeof(ServerAudioInfo), a.start, a.count, env);
}

void TCPServerListener::KillThread() {
  shutdown = true;
}

/***************************
  class TCPRecievePacket

  This helper class will recieve and
  decode a client request.
 ***************************/

TCPRecievePacket::TCPRecievePacket(SOCKET _s) : s(_s) {
  isDisconnected = false;
  int recieved = 0;

  while (recieved < 4) {
    int bytesRecv = recv(s, (char*) & dataSize + recieved, 4 - recieved, 0 );
    if (bytesRecv == WSAECONNRESET || bytesRecv <= 0) {
      _RPT0(0, "TCPServer: Could not retrieve packet size!\n");
      isDisconnected = true;
      return ;
    }
    recieved += bytesRecv;
  }

  if (dataSize <= 0) {
    _RPT0(0, "TCPServer: Packet size less than 0!");
    isDisconnected = true;
    dataSize = 0;
    return ;
  }

  data = (BYTE*)malloc(dataSize);
  recieved = 0;
  while (recieved < dataSize) {
    int bytesRecv = recv(s, (char*) & data[recieved], dataSize - recieved, 0 );
    if (bytesRecv == WSAECONNRESET || bytesRecv <= 0) {
      _RPT0(0, "TCPServer: Could not retrieve packet data!");
      isDisconnected = true;
      return ;
    }
    recieved += bytesRecv;
  }
}

TCPRecievePacket::~TCPRecievePacket() {
  if (dataSize > 0)
    free(data);
}

