#include "StdAfx.h"
#include "track_sockets.h"
#include "requests.h"
#include "test_state.h"

const DWORD LOCALHOST = 0x0100007F; // 127.0.0.1

/*-----------------------------------------------------------------------------
-----------------------------------------------------------------------------*/
TrackSockets::TrackSockets(Requests& requests, TestState& test_state):
  _nextSocketId(1)
  , _requests(requests)
  , _test_state(test_state) {
  InitializeCriticalSection(&cs);
  _openSockets.InitHashTable(257);
  _socketInfo.InitHashTable(257);
}

/*-----------------------------------------------------------------------------
-----------------------------------------------------------------------------*/
TrackSockets::~TrackSockets(void){
  Reset();
  DeleteCriticalSection(&cs);
}

/*-----------------------------------------------------------------------------
-----------------------------------------------------------------------------*/
void TrackSockets::Create(SOCKET s){
}

/*-----------------------------------------------------------------------------
-----------------------------------------------------------------------------*/
void TrackSockets::Close(SOCKET s){
  EnterCriticalSection(&cs);
  DWORD socket_id = 0;
  if (_openSockets.Lookup(s, socket_id) && socket_id)
    _requests.SocketClosed(socket_id);
  _openSockets.RemoveKey(s);
  LeaveCriticalSection(&cs);
}

/*-----------------------------------------------------------------------------
-----------------------------------------------------------------------------*/
void TrackSockets::Connect(SOCKET s, const struct sockaddr FAR * name, 
                            int namelen){
  ATLTRACE(_T("[wpthook] - TrackSockets::Connect(%d)\n"), s);

  // we only care about IP sockets at this point
  if (namelen >= sizeof(struct sockaddr_in) && name->sa_family == AF_INET)
  {
    struct sockaddr_in * ipName = (struct sockaddr_in *)name;

      // only add it to the list if it's not connecting to localhost
    EnterCriticalSection(&cs);
    SocketInfo * info = new SocketInfo;
    info->_id = _nextSocketId;
    info->_during_test = _test_state._active;
    memcpy(&info->_addr, ipName, sizeof(info->_addr));
    QueryPerformanceCounter(&info->_connect_start);
    _socketInfo.SetAt(info->_id, info);
    _openSockets.SetAt(s, info->_id);
    _nextSocketId++;
    LeaveCriticalSection(&cs);
  }
}

/*-----------------------------------------------------------------------------
-----------------------------------------------------------------------------*/
void TrackSockets::Connected(SOCKET s) {
  ATLTRACE(_T("[wpthook] - TrackSockets::Connected(%d)\n"), s);

  EnterCriticalSection(&cs);
  DWORD socket_id = 0;
  _openSockets.Lookup(s, socket_id);
  if (socket_id) {
    SocketInfo * info = NULL;
    if (_socketInfo.Lookup(socket_id, info) && info )
      QueryPerformanceCounter(&info->_connect_end);
  }
  LeaveCriticalSection(&cs);
}

/*-----------------------------------------------------------------------------
  Look up the socket ID (or create one if it doesn't already exist)
  and pass the data on to the request tracker
-----------------------------------------------------------------------------*/
void TrackSockets::DataIn(SOCKET s, const char * data, unsigned long data_len){
  ATLTRACE(_T("[wpthook] - TrackSockets::DataIn() %d bytes on socket %d"),
            data_len, s);
  bool localhost = false;
  EnterCriticalSection(&cs);
  DWORD socket_id = 0;
  _openSockets.Lookup(s, socket_id);
  SocketInfo * info = NULL;
  if (!socket_id) {
    socket_id = _nextSocketId;
    _openSockets.SetAt(s, socket_id);
    _nextSocketId++;
  } else {
    _socketInfo.Lookup(socket_id, info);
  }
  if (!info) {
    info = new SocketInfo;
    info->_id = socket_id;
    info->_during_test = _test_state._active;
    int len = sizeof(info->_addr);
    getpeername(s, (sockaddr *)&info->_addr, &len);
    _socketInfo.SetAt(info->_id, info);
  }
  if (info->_addr.sin_addr.S_un.S_addr == LOCALHOST)
    localhost = true;
  LeaveCriticalSection(&cs);

  if (!localhost )
    _requests.DataIn(socket_id, data, data_len);
}

/*-----------------------------------------------------------------------------
  Look up the socket ID (or create one if it doesn't already exist)
  and pass the data on to the request tracker
-----------------------------------------------------------------------------*/
void TrackSockets::DataOut(SOCKET s, const char * data, 
                            unsigned long data_len){
  ATLTRACE(_T("[wpthook] - TrackSockets::DataOut() %d bytes on socket %d"),
            data_len, s);
  bool localhost = false;
  EnterCriticalSection(&cs);
  DWORD socket_id = 0;
  _openSockets.Lookup(s, socket_id);
  SocketInfo * info = NULL;
  if (!socket_id) {
    socket_id = _nextSocketId;
    _openSockets.SetAt(s, socket_id);
    _nextSocketId++;
  } else {
    if (_socketInfo.Lookup(socket_id, info) && info) {
      if (info->_connect_start.QuadPart && !info->_connect_end.QuadPart)
        QueryPerformanceCounter(&info->_connect_end);
    }
  }
  if (!info) {
    info = new SocketInfo;
    info->_id = socket_id;
    info->_during_test = _test_state._active;
    int len = sizeof(info->_addr);
    getpeername(s, (sockaddr *)&info->_addr, &len);
    _socketInfo.SetAt(info->_id, info);
  }
  if (info->_addr.sin_addr.S_un.S_addr == LOCALHOST)
    localhost = true;
  LeaveCriticalSection(&cs);

  if (!localhost )
    _requests.DataOut(socket_id, data, data_len);
}

/*-----------------------------------------------------------------------------
-----------------------------------------------------------------------------*/
void TrackSockets::Reset() {
  EnterCriticalSection(&cs);
  POSITION pos = _socketInfo.GetStartPosition();
  while (pos) {
    SocketInfo * info = NULL;
    DWORD id = 0;
    _socketInfo.GetNextAssoc(pos, id, info);
    if (info)
      delete info;
  }
  _socketInfo.RemoveAll();
  LeaveCriticalSection(&cs);
}

/*-----------------------------------------------------------------------------
  Claim ownership of a connection (associate it with a request)
-----------------------------------------------------------------------------*/
bool TrackSockets::ClaimConnect(DWORD socket_id, LONGLONG before, 
                                LONGLONG& start, LONGLONG& end) {
  bool claimed = false;
  EnterCriticalSection(&cs);
  SocketInfo * info = NULL;
  if (_socketInfo.Lookup(socket_id, info) && info) {
    if (!info->_accounted_for &&
        info->_connect_start.QuadPart <= before && 
        info->_connect_end.QuadPart <= before) {
      claimed = true;
      info->_accounted_for = true;
      start = info->_connect_start.QuadPart;
      end = info->_connect_end.QuadPart;
    }
  }
  LeaveCriticalSection(&cs);
  return claimed;
}

/*-----------------------------------------------------------------------------
-----------------------------------------------------------------------------*/
ULONG TrackSockets::GetPeerAddress(DWORD socket_id) {
  ULONG ret = 0;
  EnterCriticalSection(&cs);
  SocketInfo * info = NULL;
  if (_socketInfo.Lookup(socket_id, info) && info) {
    ret = info->_addr.sin_addr.S_un.S_addr;
  }
  LeaveCriticalSection(&cs);
  return ret;
}