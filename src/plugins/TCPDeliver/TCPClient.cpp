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


//FIXME: If client application requests audio at the same time as requesting audio, data transfer might fail

TCPClient::TCPClient(const char* _hostname, int _port, IScriptEnvironment* env) : hostname(_hostname), port(_port) {
  LPDWORD ThreadId = 0;
  client = new TCPClientThread(hostname, port, env);
  _RPT0(0, "TCPClient: Client object created.\n");

//  if(!ClientThread) ClientThread = CreateThread(NULL, 0, (unsigned long (__stdcall *)(void *))StartClient, 0, 0 , ThreadId );

}

const VideoInfo& TCPClient::GetVideoInfo() {
  _RPT0(0, "TCPClient: Requesting VideoInfo.\n");
  memset(&vi, 0, sizeof(VideoInfo));
  client->SendRequest(CLIENT_SEND_VIDEOINFO, 0, 0);
  client->GetReply();
  if (client->last_reply_type == SERVER_VIDEOINFO) {
    if (client->last_reply_bytes != sizeof(VideoInfo)) {
      _RPT0(1,"TCPClient: Internal error - VideoInfo was not right size!");
    }
    memcpy(&vi, client->last_reply, sizeof(VideoInfo));
      
  } else {
    _RPT0(1,"TCPClient: Did not recieve expected packet (SERVER_VIDEOINFO)");
  }
  return vi;
}

PVideoFrame __stdcall TCPClient::GetFrame(int n, IScriptEnvironment* env) { 


  int al_b = sizeof(ClientRequestFrame);
  ClientRequestFrame f;
  memset(&f, 0 , sizeof(ClientRequestFrame));
  f.n = n;
  client->SendRequest(CLIENT_REQUEST_FRAME, &f, sizeof(ClientRequestFrame));
  client->GetReply();

  PVideoFrame frame;
  int incoming_pitch;
  unsigned int incoming_bytes;

  if (client->last_reply_type == SERVER_FRAME_INFO) {
    ServerFrameInfo* fi = (ServerFrameInfo *)client->last_reply;
    frame = env->NewVideoFrame(vi);  

    if((unsigned int)frame->GetRowSize() != fi->row_size)
      env->ThrowError("TCPClient: Internal Error - rowsize alignment was not correct.");
    if((unsigned int)frame->GetHeight() != fi->height)
      env->ThrowError("TCPClient: Internal Error - height was not correct.");
    if(fi->framenumber != (unsigned int)n)
      env->ThrowError("TCPClient: Internal Error - framenumber was not correct.");

    incoming_pitch = fi->pitch;
    incoming_bytes = fi->data_size;
    // Todo: Insert compression class.

    switch (fi->compression) {
    case ServerFrameInfo::COMPRESSION_NONE:
      break;
    default:
      env->ThrowError("TCPClient: Unknown compression.");
    }
  } else {
    env->ThrowError("TCPClient: Did not recieve expected packet (SERVER_FRAME_INFO)");
  }
  client->SendRequest(CLIENT_SEND_FRAME, 0, 0);
  client->GetReply();
  _RPT1(0,"TCPClient: Requesting %d bytes (GetFrame)", incoming_bytes);
  if (client->last_reply_type == SERVER_SENDING_FRAME) {
    client->GetDataBlock(incoming_bytes);
      
    env->MakeWritable(&frame);
    BYTE* dstp = frame->GetWritePtr();
    BYTE* srcp = (unsigned char*)client->last_reply;

    env->BitBlt(dstp, frame->GetPitch(), srcp, incoming_pitch, frame->GetRowSize(), frame->GetHeight());
    if (vi.IsYV12()) {
      int uv_pitch = incoming_pitch / 2;
      srcp += frame->GetHeight()*incoming_pitch;
      env->BitBlt(frame->GetWritePtr(PLANAR_U), frame->GetPitch(PLANAR_U), 
        srcp, uv_pitch, frame->GetRowSize(PLANAR_U), frame->GetHeight(PLANAR_U));

      srcp += frame->GetHeight(PLANAR_U)*uv_pitch;
      env->BitBlt(frame->GetWritePtr(PLANAR_V), frame->GetPitch(PLANAR_V), 
        srcp, uv_pitch, frame->GetRowSize(PLANAR_V), frame->GetHeight(PLANAR_V));
    }
  } else {
    env->ThrowError("TCPClient: Did not recieve expected packet (SERVER_SENDING_FRAME)");
  }
  return frame;
}

void __stdcall TCPClient::GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env) {
  ClientRequestAudio a;
  memset(&a, 0 , sizeof(ClientRequestAudio));
  a.start = start;
  a.count = count;
  a.bytes = vi.BytesFromAudioSamples(count);
  client->SendRequest(CLIENT_REQUEST_AUDIO, &a, sizeof(ClientRequestAudio));
  client->GetReply();
  if (client->last_reply_type != SERVER_AUDIO_INFO) {
    _RPT0(1,"TCPClient: Did not recieve expected packet (SERVER_AUDIOINFO)");
    return;
  }

  ServerAudioInfo* ai = (ServerAudioInfo *)client->last_reply;
  switch (ai->compression) {
    case ServerAudioInfo::COMPRESSION_NONE:
      break;
    default:
      env->ThrowError("TCPClient: Unknown compression.");
  }

  client->SendRequest(CLIENT_SEND_AUDIO, 0, 0);
  client->GetReply();
  _RPT1(0,"TCPClient: Requesting %d bytes (GetFrame)", a.bytes);
  if (client->last_reply_type == SERVER_SENDING_AUDIO) {
    client->GetDataBlock(a.bytes);
    memcpy(buf, client->last_reply, client->last_reply_bytes);
  } else {
    env->ThrowError("TCPClient: Did not recieve expected packet (SERVER_SENDING_AUDIO)");
  }
  
}

bool __stdcall TCPClient::GetParity(int n) {
  return false;
}


TCPClient::~TCPClient() {
  DWORD dwExitCode = 0;
  client->disconnect = true;
}


AVSValue __cdecl Create_TCPClient(AVSValue args, void* user_data, IScriptEnvironment* env) {
  return new AlignPlanar(new TCPClient(args[0].AsString(), args[1].AsInt(22050), env));
}


/*****************  CLIENT CODE   *******************/

UINT StartClient(LPVOID p) {
  TCPClientThread* t = (TCPClientThread*)p;
  t->StartRequestLoop();
  return 0;
}


TCPClientThread::TCPClientThread(const char* hostname, int port, IScriptEnvironment* env) {
  disconnect = false;

  evtClientReadyForRequest = ::CreateEvent (NULL,	FALSE, FALSE, NULL);
  evtClientReplyReady = ::CreateEvent (NULL,	FALSE, FALSE, NULL);
  evtClientProcesRequest = ::CreateEvent (NULL,	FALSE, FALSE, NULL);

  int iResult = WSAStartup( MAKEWORD(2,2), &wsaData );
  if ( iResult != NO_ERROR )
    env->ThrowError("TCPClient: Could not start up Winsock.");

  m_socket = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );

  if ( m_socket == INVALID_SOCKET ) {
    env->ThrowError("TCPClient: Could not create socket()");
    WSACleanup();
    return;
  }

  // Set up the sockaddr structure
  service.sin_family = AF_INET;
  service.sin_addr.s_addr = inet_addr(hostname);
  service.sin_port = htons(port);

  if ( connect( m_socket, (SOCKADDR*) &service, sizeof(service) ) == SOCKET_ERROR) {
    env->ThrowError("TCPClient: Could not connect to server." );
    WSACleanup();
    return;
  }
  _RPT0(0, "TCPClient: Connected to server!  Spawning thread.\n");

  AfxBeginThread(StartClient, this , THREAD_PRIORITY_NORMAL,0,0,NULL);

}

// To be called from external interface
void TCPClientThread::SendRequest(char requestId, void* data, unsigned int bytes) {
    _RPT0(0, "TCPClient: Waiting for ready for request.\n");    
    HRESULT wait_result = WAIT_TIMEOUT;
    while (wait_result == WAIT_TIMEOUT) {
      wait_result = WaitForSingleObject(evtClientReadyForRequest, 1000);
    }

    _RPT0(0, "TCPClient: Sending Request.\n");    

    client_request_bytes = bytes+1;  // bytecount + id + data
    client_request = new char[client_request_bytes+4];
    memcpy(client_request, &client_request_bytes, 4);
    client_request[4] = requestId;
    memcpy(&client_request[5], data, bytes);
    client_request_bytes += 4;  // Compensate for byte count

    SetEvent(evtClientProcesRequest);
}

// The data is only valid until SendRequest is called.
void TCPClientThread::GetReply() {
    _RPT0(0, "TCPClient: Waiting for reply.\n");    
    HRESULT wait_result = WAIT_TIMEOUT;
    while (wait_result == WAIT_TIMEOUT) {
      wait_result = WaitForSingleObject(evtClientReplyReady, 1000);
    }
    if (client_request)
      delete[] client_request;  // The request data can now be freed.
}

void TCPClientThread::GetDataBlock(unsigned int bytes) {
  SendRequest(INTERNAL_GETDATABLOCK, 0,0);
  GetReply();
  char* d = new char[bytes];
  unsigned int write_offset = 0;
  if (last_reply_type == SERVER_SPLIT_BLOCK) {
    while (last_reply_type ==SERVER_SPLIT_BLOCK) {
      memcpy(&d[write_offset], last_reply, last_reply_bytes);
      write_offset+= last_reply_bytes;
      SendRequest(INTERNAL_GETDATABLOCK, 0,0);
      GetReply();
      if (write_offset>bytes) {
        _RPT0(1, "TCPServer: Recieved too many bytes!");
        break;
      }
    }
  } else {
    _RPT0(1, "TCPClient: Did not recieve a split block, as expected.");
  }
  delete[] last_reply;
  last_reply = d;
  last_reply_bytes = write_offset;
  last_reply_type = INTERNAL_GETDATABLOCK;
}

// Main thread for sending and recieving data

void TCPClientThread::StartRequestLoop() {
  _RPT0(0, "TCPClient: Thread starting.\n");
  last_reply = new char[1];
  while (!disconnect) {

    HRESULT wait_result = WAIT_TIMEOUT;
    SetEvent(evtClientReadyForRequest);

    while (wait_result == WAIT_TIMEOUT) {
      wait_result = WaitForSingleObject(evtClientProcesRequest, 1000);
    }

    _RPT0(0, "TCPClient: Processing request.\n");    

    delete[] last_reply;
    last_reply_bytes = 0;
    last_reply_type = 0;

    if (client_request[4] != INTERNAL_GETDATABLOCK) {
      int bytesSent = send(m_socket, client_request, client_request_bytes, 0);

      _RPT0(0, "TCPClient: Request sent.\n");    

      if (bytesSent == WSAECONNRESET || bytesSent == 0) {
        _RPT0(1, "TCPClient: Client was disconnected!");
        disconnect = true;
      } else {
        // Wait for reply.
        RecievePacket();
      }
    } else {
      RecievePacket();
    }
    _RPT0(0, "TCPClient: Returning reply.\n");    
    if (disconnect) {
      last_reply_type = INTERNAL_DISCONNECTED;
    }
    SetEvent(evtClientReplyReady);  // FIXME: Could give deadlocks, if client is waiting for reply.
  } //end while
  CloseHandle(evtClientProcesRequest);
  CloseHandle(evtClientReplyReady);
  CloseHandle(evtClientReadyForRequest);
}

void TCPClientThread::RecievePacket() {
  unsigned int dataSize;
  unsigned int recieved = 0;
  while (recieved < 4) {
    int bytesRecv = recv(m_socket, (char*)&dataSize+recieved, 4-recieved, 0 );
    if (bytesRecv == WSAECONNRESET || bytesRecv == 0) {
      _RPT0(0, "TCPClient: Could not retrieve packet size!\n");
      disconnect = true;
      return;
    }
    recieved += bytesRecv;
  }

  _RPT1(0,"TCPClient: Got packet, of %d bytes\n", dataSize);

//  if (dataSize> 65536)
//    _RPT1(1,"TCPClient: Excessively large package recieved: %d bytes.", dataSize);

  char* data = new char[dataSize];
  recieved = 0;

  while (recieved < dataSize) {
    int bytesRecv = recv(m_socket, (char*)&data[recieved], dataSize-recieved, 0 );
    if (bytesRecv == WSAECONNRESET || bytesRecv == 0) {
      _RPT0(0, "TCPClient: Could not retrieve packet data!\n");
      disconnect = true;
      return;
    }
    recieved += bytesRecv;
  }
  last_reply = new char[dataSize - 1];  // Freed in StartRequestLoop
  last_reply_bytes = dataSize - 1;
  last_reply_type = data[0];
  memcpy(last_reply, &data[1], last_reply_bytes);
  delete[] data;
}

