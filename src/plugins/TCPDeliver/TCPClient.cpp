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

//static IScriptEnvironment* pass_client_env;
//static int pass_server_port;
//static const char pass_server_name;


//static void StartClientThread() {
//  TCPClientThread* s = new TCPClientThread(pass_server_name, pass_server_port, pass_client_env);

//}


TCPClient::TCPClient(const char* _hostname, int _port, IScriptEnvironment* env) : hostname(_hostname), port(_port) {
  LPDWORD ThreadId = 0;
  client = new TCPClientThread(hostname, port, env);

  if(!ClientThread) ClientThread = CreateThread(NULL, 0, (unsigned long (__stdcall *)(void *))client->StartClient(0), 0, 0 , ThreadId );

}

const VideoInfo& TCPClient::GetVideoInfo() {
  return vi;
}

PVideoFrame __stdcall TCPClient::GetFrame(int n, IScriptEnvironment* env) { 
  return NULL; 
}

void __stdcall TCPClient::GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env) {
   
}

bool __stdcall TCPClient::GetParity(int n) {
  return false;
}


TCPClient::~TCPClient() {
  DWORD dwExitCode = 0;
	if(ClientThread)  {
    TerminateThread(ClientThread, dwExitCode);
		ClientThread=NULL; 
  }
}


AVSValue __cdecl Create_TCPClient(AVSValue args, void* user_data, IScriptEnvironment* env) {
  return new TCPClient(args[0].AsString(), args[1].AsInt(22050), env);
}


/*****************  CLIENT CODE   *******************/

TCPClientThread::TCPClientThread(const char* hostname, int port, IScriptEnvironment* env) {
  disconnect = false;

  evtClientReadyForRequest = CreateEvent (NULL,	FALSE, FALSE, NULL);
  evtClientReplyReady = CreateEvent (NULL,	FALSE, FALSE, NULL);
  evtClientProcesRequest = CreateEvent (NULL,	FALSE, FALSE, NULL);

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
}



UINT TCPClientThread::StartClient(LPVOID) {
  while (!disconnect) {

    HRESULT wait_result;
    SetEvent(evtClientReadyForRequest);

    while (wait_result == WAIT_TIMEOUT) {
      wait_result = WaitForSingleObject(evtClientProcesRequest, 1000);
    }
    
  }
  return 0;
}

