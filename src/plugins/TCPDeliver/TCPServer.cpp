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
/*	if(ServerThread)  {
    TerminateThread(ServerThread, dwExitCode);
		ServerThread=NULL; 
  }*/
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


TCPServerListener::TCPServerListener(int port, PClip _child, IScriptEnvironment* env) : child(_child) {

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
  t.tv_usec = 1;

  SOCKET s_list[FD_SETSIZE];
  int i;

  char *recvbuf = new char[1025];
  ServerReply s;

  for (i = 0; i< FD_SETSIZE ; i++)
    s_list[i] = NULL;

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
        if (s_list[i] == NULL)
          slot = i;

      if (slot >= 0 ) {
        s_list[slot] = AcceptSocket;
      } else {
        _RPT0(0,"TCPServer: All slots full.\n");
        s.allocateBuffer(1);
        s.data[0] = REQUEST_NOMORESOCKETS;
        send(AcceptSocket, (const char*)s.data, s.dataSize, 0);
        closesocket(AcceptSocket);
      }
    }

    FD_ZERO(&test_set);

    for (i = 0; i< FD_SETSIZE; i++) 
      if(s_list[i] != NULL) 
        FD_SET(s_list[i], &test_set);

    select(0, &test_set, NULL, NULL, &t);
    
    for (i = 0; i< FD_SETSIZE; i++) {
      if(s_list[i] != NULL) {
        if (FD_ISSET(s_list[i], &test_set)) {

          TCPRecievePacket* t = new TCPRecievePacket(s_list[i]);
          // FIXME: Possibly test if all bytes has been received.
          
          _RPT1(0, "TCPServer: Bytes Recv: %ld\n", t->dataSize );
          
          if (!t->isDisconnected) {
            s.dataSize = 0;
            Receive((const char*)t->data, t->dataSize, &s);

            if (s.dataSize>0) {
              unsigned int BytesSent = 0;
              int r = 1;
              send(s_list[i], (const char*)&s.dataSize, 4, 0);
              while (r > 0) {
                r = send(s_list[i], (const char*)(&s.data[BytesSent]), s.dataSize-BytesSent, 0);
                BytesSent += r;                
              } // end while
              if (BytesSent != s.dataSize) {
                _RPT2(1, "TCPServer: Failed in sending %d bytes - could only send %d bytes!\n", s.dataSize, BytesSent);
                closesocket(s_list[i]);
                s_list[i] = NULL;
              }
              s.freeBuffer();
            } // end if datasize > 0
          } else { // isDiconnected
            _RPT0(0,  "TCPServer: Connection Closed.\n");
            closesocket(s_list[i]);
            s_list[i] = NULL;
          }          
        } // end if fd is set
      } // end if list != null
    } // end for i

  } // while !shutdown
}



void TCPServerListener::Receive(const char* recvbuf, int bytes, ServerReply* s) {
  switch (recvbuf[0]) {
    case 'a':
      _RPT0(0,"WOW - jeg fik et a\n");
      s->allocateBuffer(32);
      memcpy(s->data, "Hejsa - min ven\r\n", 20);
      break;
    case REQUEST_PING:
      _RPT0(0, "TCPServer: Received Ping? - returning Pong!\n");
      s->allocateBuffer(bytes);
      memcpy(s->data, recvbuf, bytes);
      s->data[0] = REQUEST_PONG;
      break;

    case CLIENT_SEND_VIDEOINFO:
      SendVideoInfo(s);
      break;

    default:
      // Brok dig!
      break;
  }
}

void TCPServerListener::SendVideoInfo(ServerReply* s) {
  _RPT0(0, "TCPServer: Sending VideoInfo!\n");

  s->allocateBuffer(sizeof(VideoInfo)+1);
  s->data[0] = SERVER_VIDEOINFO;
  memcpy(&s->data[1], &child->GetVideoInfo(), sizeof(VideoInfo));
}

void TCPServerListener::KillThread() {
  shutdown = true;
}


TCPRecievePacket::TCPRecievePacket(SOCKET _s) : s(_s) {
  isDisconnected = false;
  int recieved = 0;

  while (recieved < 4) {
    int bytesRecv = recv(s, (char*)&dataSize+recieved, 4-recieved, 0 );
    if (bytesRecv == WSAECONNRESET) {
      _RPT0(1, "TCPServer: Could not retrieve packet size!");
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
    if (bytesRecv == WSAECONNRESET) {
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

