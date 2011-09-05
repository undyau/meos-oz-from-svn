/************************************************************************
    MeOS - Orienteering Software
    Copyright (C) 2009-2011 Melin Software HB
    
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

// TimeStamp.cpp: implementation of the TimeStamp class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "meos.h"
#include "TimeStamp.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

TimeStamp::TimeStamp()
{
	Time=0;
	//Update();
}

TimeStamp::~TimeStamp()
{

}

void TimeStamp::Update(TimeStamp &ts)
{
	Time=max(Time, ts.Time);
}

void TimeStamp::Update()
{
	SYSTEMTIME st;
	GetLocalTime(&st);
	
	FILETIME ft;
	SystemTimeToFileTime(&st, &ft);

	__int64 &currenttime=*(__int64*)&ft;

	Time=int((currenttime/10000000)-__int64(370)*365*24*3600);
}

int TimeStamp::GetAge() const
{
	SYSTEMTIME st;
	GetLocalTime(&st);
	FILETIME ft;
	SystemTimeToFileTime(&st, &ft);
	__int64 &currenttime=*(__int64*)&ft;


	int CTime=int((currenttime/10000000)-__int64(370)*365*24*3600);
//	__int64 delta=currenttime-Time;

//	__int64 ds=delta/10000000;

	return CTime-Time;
	//return int((currenttime-Time));
}

string TimeStamp::GetStamp() const
{
	__int64 ft64=(__int64(Time)+__int64(370)*365*24*3600)*10000000;
	FILETIME &ft=*(FILETIME*)&ft64;
	SYSTEMTIME st;
	FileTimeToSystemTime(&ft, &st);

	char bf[32];
	sprintf_s(bf, "%d%02d%02d%02d%02d%02d", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);

	return bf;
}

string TimeStamp::getStampString() const
{
	__int64 ft64=(__int64(Time)+__int64(370)*365*24*3600)*10000000;
	FILETIME &ft=*(FILETIME*)&ft64;
	SYSTEMTIME st;
	FileTimeToSystemTime(&ft, &st);

	char bf[32];
  sprintf_s(bf, "%d-%02d-%02d %02d:%02d:%02d", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);

	return bf;
}

void TimeStamp::SetStamp(string s)
{
  if (s.size()<14)
    return;
	SYSTEMTIME st;
	memset(&st, 0, sizeof(st));

	//const char *ptr=s.c_str();
	//sscanf(s.c_str(), "%4hd%2hd%2hd%2hd%2hd%2hd", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
	st.wYear=atoi(s.substr(0, 4).c_str());
	st.wMonth=atoi(s.substr(4, 2).c_str());
	st.wDay=atoi(s.substr(6, 2).c_str());
	st.wHour=atoi(s.substr(8, 2).c_str());
	st.wMinute=atoi(s.substr(10, 2).c_str());
	st.wSecond=atoi(s.substr(12, 2).c_str());

	FILETIME ft;
	SystemTimeToFileTime(&st, &ft);

	__int64 &currenttime=*(__int64*)&ft;

	Time=int((currenttime/10000000)-__int64(370)*365*24*3600);
}
