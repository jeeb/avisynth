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

#include "TCPClient.h"
#include "alignplanar.h"

/**************************  TCP Client *****************************
  The TCPCLient is designed as a multithreaded application.

  When TCP-client is invoked it will spawn a request thread, that operates
  independently of the AviSynth request thread.

  This will (in time) allow the client to fetch frames while AviSynth is doing
  further processing, or the client-app is compressing frames or similar.
 ********************************************************************/


TCPClient::TCPClient(const char* _hostname, int _port, const char* compression, IScriptEnvironment* env) : hostname(_hostname), port(_port) {
  LPDWORD ThreadId = 0;

  _RPT0(0, "TCPClient: Creating client object.\n");
  client = new TCPClientThread(hostname, port, compression, env);
  _RPT0(0, "TCPClient: Client object created.\n");

  //  if(!ClientThread) ClientThread = CreateThread(NULL, 0, (unsigned long (__stdcall *)(void *))StartClient, 0, 0 , ThreadId );

}

const VideoInfo& TCPClient::GetVideoInfo() {
  if (client->IsDataPending()) {  // Ignore any pending data
    client->GetReply();  // Kill it.
  }
  _RPT0(0, "TCPClient: Requesting VideoInfo.\n");
  memset(&vi, 0, sizeof(VideoInfo));
  client->SendRequest(CLIENT_SEND_VIDEOINFO, 0, 0);
  client->GetReply();
  if (client->reply->last_reply_type == SERVER_VIDEOINFO) {
    if (client->reply->last_reply_bytes != sizeof(VideoInfo)) {
      _RPT0(1, "TCPClient: Internal error - VideoInfo was not right size!");
    }
    memcpy(&vi, client->reply->last_reply, sizeof(VideoInfo));

  } else {
    _RPT0(1, "TCPClient: Did not recieve expected packet (SERVER_VIDEOINFO)");
  }
  return vi;
}



PVideoFrame __stdcall TCPClient::GetFrame(int n, IScriptEnvironment* env) {

  int al_b = sizeof(ClientRequestFrame);
  ClientRequestFrame f;
  memset(&f, 0 , sizeof(ClientRequestFrame));
  f.n = n;

  bool ready = false;

  if (client->IsDataPending()) {
    client->GetReply();
    if (client->reply->last_reply_type == SERVER_SENDING_FRAME) {
      ServerFrameInfo* fi = (ServerFrameInfo *)client->reply->last_reply;
      if ((int)fi->framenumber == n) {
        ready = true;
        _RPT1(0, "TCPClient: Frame was PreRequested (hit!). Found frame %d.\n", n);
      }
    }
  }
  if (!ready) {
    _RPT1(0, "TCPClient: Frame was not PreRequested (miss). Requesting frame %d.\n", n);
    client->SendRequest(CLIENT_REQUEST_FRAME, &f, sizeof(ClientRequestFrame));
    client->GetReply();
  }

  PVideoFrame frame;
  int incoming_pitch;
  unsigned int incoming_bytes;

  if (client->reply->last_reply_type == SERVER_SENDING_FRAME) {
    ServerFrameInfo* fi = (ServerFrameInfo *)client->reply->last_reply;
    frame = env->NewVideoFrame(vi);

    if ((unsigned int)frame->GetRowSize() != fi->row_size)
      env->ThrowError("TCPClient: Internal Error - rowsize alignment was not correct.");
    if ((unsigned int)frame->GetHeight() != fi->height)
      env->ThrowError("TCPClient: Internal Error - height was not correct.");
    if (fi->framenumber != (unsigned int)n)
      env->ThrowError("TCPClient: Internal Error - framenumber was not correct.");

    incoming_pitch = fi->pitch;
    incoming_bytes = fi->data_size;

    BYTE* dstp = frame->GetWritePtr();
    BYTE* srcp = (unsigned char*)client->reply->last_reply + sizeof(ServerFrameInfo);
    TCPCompression* t = 0;

    switch (fi->compression) {
      case ServerFrameInfo::COMPRESSION_NONE:
          t = (TCPCompression*)new TCPCompression();        
        break;
      case ServerFrameInfo::COMPRESSION_DELTADOWN_LZO: {
          t = (TCPCompression*)new PredictDownLZO();
          break;
        }
      case ServerFrameInfo::COMPRESSION_DELTADOWN_HUFFMAN: {
          t = (TCPCompression*)new PredictDownHuffman();
          break;
                                                           }
      case ServerFrameInfo::COMPRESSION_DELTADOWN_GZIP: {
          t = (TCPCompression*)new PredictDownGZip();
          break;
        }
      default:
        env->ThrowError("TCPClient: Unknown compression.");
    }

    if (!vi.IsPlanar()) {
      t->DeCompressImage(srcp, fi->row_size, fi->height, fi->pitch, fi->compressed_bytes);
      env->BitBlt(dstp, frame->GetPitch(), t->dst, incoming_pitch, frame->GetRowSize(), frame->GetHeight());
      if (!t->inplace) {
        _aligned_free(t->dst);
      }
      delete t;
    } else {
      // Y
      t->DeCompressImage(srcp, fi->row_size, fi->height, fi->pitch, fi->comp_Y_bytes);
      env->BitBlt(dstp, frame->GetPitch(), t->dst, incoming_pitch, frame->GetRowSize(), frame->GetHeight());
      if (!t->inplace) _aligned_free(t->dst);

      int uv_pitch = fi->pitchUV;
      int uv_rowsize  = fi->row_sizeUV;
      int uv_height = fi->heightUV;

      // U
      srcp += fi->comp_Y_bytes;
      t->DeCompressImage(srcp, uv_rowsize, uv_height, uv_pitch, fi->comp_U_bytes);
      env->BitBlt(frame->GetWritePtr(PLANAR_U), frame->GetPitch(PLANAR_U),
                  t->dst, uv_pitch, frame->GetRowSize(PLANAR_U), frame->GetHeight(PLANAR_U));
      if (!t->inplace) _aligned_free(t->dst);

      // V
      srcp += fi->comp_U_bytes;
      t->DeCompressImage(srcp, uv_rowsize, uv_height, uv_pitch, fi->comp_V_bytes);
      env->BitBlt(frame->GetWritePtr(PLANAR_V), frame->GetPitch(PLANAR_V),
                  t->dst, uv_pitch, frame->GetRowSize(PLANAR_V), frame->GetHeight(PLANAR_V));
      if (!t->inplace) _aligned_free(t->dst);
      delete t;
    }
  } else {
    if (client->reply->last_reply_type == INTERNAL_DISCONNECTED)
      env->ThrowError("TCPClient: Disconnected from server");

    env->ThrowError("TCPClient: Did not recieve expected packet (SERVER_SENDING_FRAME)");
  }

  if (true) {  // Request next frame
    f.n = n + 1;
    client->SendRequest(CLIENT_REQUEST_FRAME, &f, sizeof(ClientRequestFrame));
    _RPT1(0, "TCPClient: PreRequesting frame frame %d.\n", f.n);
  }

  return frame;
}



void __stdcall TCPClient::GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env) {
  if (!vi.HasAudio())
    return ;

  ClientRequestAudio a;
  memset(&a, 0 , sizeof(ClientRequestAudio));
  a.start = start;
  a.count = count;
  a.bytes = (int)vi.BytesFromAudioSamples(count);

  client->SendRequest(CLIENT_REQUEST_AUDIO, &a, sizeof(ClientRequestAudio));
  client->GetReply();
  if (client->reply->last_reply_type != SERVER_SENDING_AUDIO) {
    if (client->reply->last_reply_type == INTERNAL_DISCONNECTED)
      env->ThrowError("TCPClient: Disconnected from server");

    env->ThrowError("TCPClient: Did not recieve expected packet (SERVER_SENDING_AUDIO)");
    return ;
  }

  ServerAudioInfo* ai = (ServerAudioInfo *)client->reply->last_reply;
  switch (ai->compression) {
    case ServerAudioInfo::COMPRESSION_NONE:
      break;
    default:
      env->ThrowError("TCPClient: Unknown compression.");
  }

  _RPT2(0, "TCPClient: Got %d of %d bytes of audio (GetAudio)\n", ai->data_size, a.bytes);
  memcpy(buf, client->reply->last_reply + sizeof(ServerAudioInfo), ai->data_size);
}


bool __stdcall TCPClient::GetParity(int n) {
  ClientRequestParity c;
  memset(&c, 0, sizeof(ClientRequestParity));
  c.n = n;
  client->SendRequest(CLIENT_SEND_PARITY, &c, sizeof(ClientRequestParity));
  client->GetReply();
  if (client->reply->last_reply_type != SERVER_SENDING_PARITY) {
    _RPT0(1, "TCPClient: Did not recieve expected packet (SERVER_SENDING_PARITY)");
    return false;
  }
  ServerParityReply* p = (ServerParityReply *)client->reply->last_reply;
  _RPT0(0, "TCPClient: Got parity information.\n");
  return !!p->parity;
}


TCPClient::~TCPClient() {
  DWORD dwExitCode = 0;
  _RPT0(0, "TCPClient: Killing thread.\n");
  client->disconnect = true;
  client->SendRequest(REQUEST_DISCONNECT, 0, 0);
  while (client->thread_running) {
    Sleep(10);
  }
  client->CleanUp();
  delete client;
  _RPT0(0, "TCPClient: Thread killed.\n");

}


AVSValue __cdecl Create_TCPClient(AVSValue args, void* user_data, IScriptEnvironment* env) {
  const char* comp = args[2].Defined() ? args[2].AsString("") : 0;
  return new AlignPlanar(new TCPClient(args[0].AsString(), args[1].AsInt(22050), comp, env));
}


/*****************  CLIENT CODE   *******************/

UINT StartClient(LPVOID p) {
  TCPClientThread* t = (TCPClientThread*)p;
  t->StartRequestLoop();
  return 0;
}


TCPClientThread::TCPClientThread(const char* hostname, int port, const char* compression, IScriptEnvironment* env) {
  disconnect = false;
  data_waiting = false;
  thread_running = false;
  client_request = 0;
  reply = new ServerToClientReply();

  evtClientReadyForRequest = ::CreateEvent (NULL, FALSE, FALSE, NULL);
  evtClientReplyReady = ::CreateEvent (NULL, FALSE, FALSE, NULL);
  evtClientProcesRequest = ::CreateEvent (NULL, FALSE, FALSE, NULL);

  int iResult = WSAStartup( MAKEWORD(2, 2), &wsaData );
  if ( iResult != NO_ERROR )
    env->ThrowError("TCPClient: Could not start up Winsock.");

  m_socket = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );

  if ( m_socket == INVALID_SOCKET ) {
    env->ThrowError("TCPClient: Could not create socket()");
    WSACleanup();
    return ;
  }

  hostent* host_info = gethostbyname(hostname);
  char* host_ip = *(host_info->h_addr_list);
  unsigned long addr = *(unsigned long*)(host_ip);

  const static int sendbufsize = 1024; // Small send size
  const static int rcvbufsize = 262144;   // Maximum rcv size

  setsockopt(m_socket, SOL_SOCKET, SO_RCVBUF, (char *) &rcvbufsize, sizeof(rcvbufsize));
  setsockopt(m_socket, SOL_SOCKET, SO_SNDBUF, (char *) &sendbufsize, sizeof(sendbufsize));

  // Set up the sockaddr structure
  service.sin_family = AF_INET;
  service.sin_addr.s_addr = addr;
  service.sin_port = htons(port);

  if ( connect( m_socket, (SOCKADDR*) &service, sizeof(service) ) == SOCKET_ERROR) {
    env->ThrowError("TCPClient: Could not connect to server." );
    WSACleanup();
    return ;
  }
  _RPT0(0, "TCPClient: Connected to server!  Spawning thread.\n");

  int one = 1;         // for 4.3 BSD style setsockopt()
  setsockopt(m_socket, IPPROTO_TCP, TCP_NODELAY, (PCHAR )&one, sizeof(one));

  AfxBeginThread(StartClient, this , THREAD_PRIORITY_ABOVE_NORMAL, 0, 0, NULL);

  thread_running = true;

  ClientCheckVersion ccv;
  ccv.major = TCPDELIVER_MAJOR;
  ccv.minor = TCPDELIVER_MINOR;
  ccv.compression_supported = ServerFrameInfo::COMPRESSION_DELTADOWN_LZO |
    ServerFrameInfo::COMPRESSION_DELTADOWN_HUFFMAN |
    ServerFrameInfo::COMPRESSION_DELTADOWN_GZIP;

  if (compression) {  // Override compression if specified.
    if (!lstrcmpi(compression, "none")) {
      ccv.compression_supported = ServerFrameInfo::COMPRESSION_NONE;
    } else if (!lstrcmpi(compression, "lzo")) {
      ccv.compression_supported = ServerFrameInfo::COMPRESSION_DELTADOWN_LZO;
    } else if (!lstrcmpi(compression, "huffman")) {
      ccv.compression_supported = ServerFrameInfo::COMPRESSION_DELTADOWN_HUFFMAN;
    } else if (!lstrcmpi(compression, "gzip")) {
      ccv.compression_supported = ServerFrameInfo::COMPRESSION_DELTADOWN_GZIP;
    } else {
      env->ThrowError("TCPSource: Unknown Compression type specified.");
    }
  }

  SendRequest(CLIENT_CHECK_VERSION, &ccv, sizeof(ccv));
  GetReply();

  if (reply->last_reply_type != REQUEST_CONNECTIONACCEPTED) {
    env->ThrowError("TCPSource: Version Check failed! (Ensure Client and Server are same version)");
  }

}

// To be called from external interface
void TCPClientThread::SendRequest(char requestId, void* data, unsigned int bytes) {
  if (data_waiting) {
    PushReply();
  }

  _RPT0(0, "TCPClient: Waiting for ready for request.\n");

  data_waiting = true;
  HRESULT wait_result = WAIT_TIMEOUT;
  while (wait_result == WAIT_TIMEOUT) {
    wait_result = WaitForSingleObject(evtClientReadyForRequest, 1000);
  }

  _RPT0(0, "TCPClient: Sending Request.\n");

  client_request_bytes = bytes + 1;  // bytecount + id + data
  client_request = new char[client_request_bytes + 4];
  memcpy(client_request, &client_request_bytes, 4);
  client_request[4] = requestId;
  memcpy(&client_request[5], data, bytes);
  client_request_bytes += 4;  // Compensate for byte count

  SetEvent(evtClientProcesRequest);
}

// The data is only valid until SendRequest is called.
void TCPClientThread::GetReply() {
  if (!data_waiting) {
    if (reply->pushed_reply)
      PopReply();

    data_waiting = false;
    return ;
  }

  _RPT0(0, "TCPClient: Waiting for reply.\n");
  HRESULT wait_result = WAIT_TIMEOUT;

  while (wait_result == WAIT_TIMEOUT) {
    wait_result = WaitForSingleObject(evtClientReplyReady, 1000);
  }

  if (client_request) {
    delete[] client_request;  // The request data can now be freed.
    client_request = 0;
  }

  data_waiting = false;
}

bool TCPClientThread::IsDataPending() {
  if (data_waiting)
    return true;
  if (reply->pushed_reply)
    return true;
  return false;
}

void TCPClientThread::PushReply() {
  if (!data_waiting)
    return ;  // Nothing to push

  GetReply();  // Wait for request to finish.

  reply = new ServerToClientReply(reply); // Create new and push old.
  data_waiting = false;
  _RPT0(0, "TCPClient: Pushed reply.\n");
}

void TCPClientThread::PopReply() {
  if (reply->pushed_reply) {
    delete[] reply->last_reply;
    ServerToClientReply* STCRpushed = reply->pushed_reply;
    delete reply;
    reply = STCRpushed;
    data_waiting = true;
    _RPT0(0, "TCPClient: Popped reply.\n");
  }
}

// Main thread for sending and recieving data

void TCPClientThread::StartRequestLoop() {
  _RPT0(0, "TCPClient: Thread starting.\n");

  while (!disconnect) {

    HRESULT wait_result = WAIT_TIMEOUT;
    SetEvent(evtClientReadyForRequest);

    while (wait_result == WAIT_TIMEOUT) {
      wait_result = WaitForSingleObject(evtClientProcesRequest, 1000);
    }

    _RPT0(0, "TCPClient: Processing request.\n");

    delete[] reply->last_reply;
    reply->last_reply = 0;
    reply->last_reply_bytes = 0;
    reply->last_reply_type = 0;

    int bytesSent = send(m_socket, client_request, client_request_bytes, 0);

    _RPT0(0, "TCPClient: Request sent.\n");

    if (disconnect || bytesSent == WSAECONNRESET || bytesSent == 0) {
      _RPT0(0, "TCPClient: Client was disconnected!");
      disconnect = true;
    } else {
      // Wait for reply.
      RecievePacket();
    }
    _RPT0(0, "TCPClient: Returning reply.\n");

    if (disconnect) {
      reply->last_reply_type = INTERNAL_DISCONNECTED;
    }
    SetEvent(evtClientReplyReady);  // FIXME: Could give deadlocks, if client is waiting for reply.
  } //end while


  CloseHandle(evtClientProcesRequest);
  CloseHandle(evtClientReplyReady);
  CloseHandle(evtClientReadyForRequest);

  closesocket(m_socket);
  WSACleanup();

  thread_running = false;
  _RPT0(0, "TCPClient: Client thread no longer running.\n");
}

void TCPClientThread::CleanUp() {
  if (client_request) {
    delete[] client_request;  // The request data can now be freed.
    client_request = 0;
  }

  if (reply->pushed_reply) {
    if (reply->pushed_reply->last_reply_bytes)
      delete[] reply->pushed_reply->last_reply;
  }

  if (reply->last_reply_bytes)
    delete[] reply->last_reply;

  delete reply;
}

void TCPClientThread::RecievePacket() {
  unsigned int dataSize;
  unsigned int recieved = 0;
  while (recieved < 4) {
    int bytesRecv = recv(m_socket, (char*) & dataSize + recieved, 4 - recieved, 0 );
    if (bytesRecv == WSAECONNRESET || bytesRecv <= 0) {
      _RPT0(0, "TCPClient: Could not retrieve packet size!\n");
      disconnect = true;
      return ;
    }
    recieved += bytesRecv;
  }

  _RPT1(0, "TCPClient: Got packet, of %d bytes\n", dataSize);

  char* data = new char[dataSize];
  recieved = 0;

  fd_set test_set;
  timeval t;
  t.tv_sec = 0;
  t.tv_usec = 10000;  // 10 ms

  while (recieved < dataSize) {
    FD_ZERO(&test_set);
    FD_SET(m_socket, &test_set);
    select(0, &test_set, NULL, NULL, &t);
    if (FD_ISSET(m_socket, &test_set)) {
      int bytesRecv = recv(m_socket, (char*) & data[recieved], dataSize - recieved, 0 );
      if (bytesRecv == WSAECONNRESET || bytesRecv <= 0) {
        _RPT0(0, "TCPClient: Could not retrieve packet data!\n");
        disconnect = true;
        return ;
      }
      recieved += bytesRecv;
    }
  }
  reply->last_reply = new char[dataSize - 1];  // Freed in StartRequestLoop
  reply->last_reply_bytes = dataSize - 1;
  reply->last_reply_type = data[0];
  memcpy(reply->last_reply, &data[1], reply->last_reply_bytes);
  delete[] data;
}

