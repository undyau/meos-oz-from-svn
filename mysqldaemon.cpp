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
    Stigbergsvägen 7, SE-75242 UPPSALA, Sweden

************************************************************************/

#include "stdafx.h"

#include "oEvent.h"
#include "TabAuto.h"
#include "meos_util.h"
#include "gdiconstants.h"
#include "meosdb/sqltypes.h"

MySQLReconnect::MySQLReconnect() : AutoMachine("MySQL-daemon") 
{
  callCount=0;
  hThread=0;
}

MySQLReconnect::~MySQLReconnect()
{
  CloseHandle(hThread);
  hThread=0;
}

bool MySQLReconnect::stop()
{
  if(interval==0)
    return true;

  return MessageBox(0, "If this daemon is stopped, then MeOS will not reconnect to the network. Continue?",
    "Warning", MB_YESNO|MB_ICONWARNING)==IDYES;
}

static CRITICAL_SECTION CS_MySQL;
static volatile DWORD mysqlConnecting=0;
static volatile DWORD mysqlStatus=0;

void initMySQLCriticalSection(bool init) {
  if (init)
    InitializeCriticalSection(&CS_MySQL);
  else
    DeleteCriticalSection(&CS_MySQL);
}

bool isThreadReconnecting()
{
  EnterCriticalSection(&CS_MySQL);
  bool res = (mysqlConnecting != 0);
  LeaveCriticalSection(&CS_MySQL);
  return res;
}

int ReconnectThread(void *v)
{
  if(isThreadReconnecting()) 
    return 0;
  
  OPENDB_FCN msReconnect=static_cast<OPENDB_FCN>(v);

  if(!msReconnect)
    return 0;

  EnterCriticalSection(&CS_MySQL);
    mysqlConnecting=1;
    mysqlStatus=0;
  LeaveCriticalSection(&CS_MySQL);

  bool res =  msReconnect();

  EnterCriticalSection(&CS_MySQL);
    if(res) 
      mysqlStatus=1;     
    else
      mysqlStatus=-1;

    mysqlConnecting=0;
  LeaveCriticalSection(&CS_MySQL);
  
  return 0;
}

void MySQLReconnect::settings(gdioutput &gdi, oEvent &oe, bool created) {
}

void MySQLReconnect::process(gdioutput &gdi, oEvent *oe, AutoSyncType ast)
{
  if( isThreadReconnecting())
    return;

  if (mysqlStatus==1) {
    if(hThread){
        CloseHandle(hThread);
        hThread=0;
    }
    mysqlStatus=0;
    char bf[256];
    if (!oe->reConnect(bf)) {
      gdi.addInfoBox("", "warning:dbproblem#" + string(bf), 9000);    
    }
    else {
      gdi.addInfoBox("", "Återansluten mot databasen, tävlingen synkroniserad.", 5000);
      interval=0;
    }
  }
  else if (mysqlStatus==-1) {
    if(hThread){
        CloseHandle(hThread);
        hThread=0;
    }
    mysqlStatus=0;
    callCount=10; //Wait ten seconds for next attempt

    char bf[256];
    if (oe->HasDBConnection) {
      msGetErrorState(bf);
      gdi.addInfoBox("", "warning:dbproblem#" + string(bf), 9000);    
    }
    return;
  }
  else if(callCount<=0) {    
    hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)&ReconnectThread,
                            msReConnect, 0, NULL);
    Sleep(100);
  }
  else callCount--;
}

void MySQLReconnect::status(gdioutput &gdi)
{
  gdi.dropLine(0.5);
	gdi.addString("", 1, name);
  gdi.fillRight();
	gdi.pushX();
	if(interval>0){		
		gdi.addString("", 0, "Nästa försök:");
		gdi.addTimer(gdi.getCY(),  gdi.getCX()+10, timerCanBeNegative, (GetTickCount()-timeout)/1000);
	}
  gdi.popX();
  gdi.fillDown();
  gdi.dropLine();
}
