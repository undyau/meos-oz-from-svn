/************************************************************************
    MeOS - Orienteering Software
    Copyright (C) 2009-2014 Melin Software HB
    
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    Melin Software HB - software@melin.nu - www.melin.nu
    Stigbergsv�gen 7, SE-75242 UPPSALA, Sweden
    
************************************************************************/

#include <list>
#include <vector>
#include <winsock2.h>

struct SocketPunchInfo {
  int runnerId;
  int controlId;
  int status;
  int time;
  SocketPunchInfo() {
    runnerId = -1;
    controlId = -1;
    status = -1;
    time = -1;
  }
};

class DirectSocket {
private:
  
  struct ExtPunchInfo {
    int cmpId;
    SocketPunchInfo punch;
  };

  int competitionId;
  list<SocketPunchInfo> messageQueue;
  HWND hDestinationWindow;
  CRITICAL_SECTION syncObj;
  volatile bool shutDown;
  void listenDirectSocket();
  void addPunchInfo(const SocketPunchInfo &pi);  
  bool startedUDPThread;

  SOCKET sendSocket;
  bool clearQueue;
public:

  void startUDPSocketThread(HWND targetWindow);
  void sendPunchInfo(SocketPunchInfo &pi);
  void getPunchQueue(vector<SocketPunchInfo> &queue);

  void sendPunch(SocketPunchInfo &pi);

  DirectSocket(int cmpId);
  ~DirectSocket();

  friend void startListeningDirectSocket(void *p);
};
