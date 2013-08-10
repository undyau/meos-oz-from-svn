/************************************************************************
    MeOS - Orienteering Software
    Copyright (C) 2009-2013 Melin Software HB
    
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

#include "stdafx.h"
#include "Download.h"
#include <Wininet.h>
#include "Localizer.h"
#include "meos_util.h"
#include "progress.h"

#include <sys/stat.h>
#include <io.h>
#include <fcntl.h>

#include <process.h>

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

Download::Download()
{
	hThread = 0;
	doExit = false;
//	hProgress = NULL;

  fileno = 0;
	hInternet = NULL;
	hURL = NULL;

	bytesLoaded = 0;
	bytesToLoad = 1024;

	success = false;
}

Download::~Download()
{
	shutDown();
	endDownload();

	if(hInternet)
		InternetCloseHandle(hInternet);
}

void __cdecl SUThread(void *ptr)
{
	Download *dwl=(Download *)ptr;
	dwl->initThread();	
}

bool Download::createDownloadThread() {	
	doExit=false;
	hThread=_beginthread(SUThread, 0, this);

	if(hThread==-1)	{
		hThread=0;
		return false;
	}

	return true;
}

void Download::shutDown()
{
	if(hThread)	{
		doExit=true;

		int m=0;
		while(m<100 && hThread) {
			Sleep(0);
			Sleep(10);
			m++;
		}
		//If unsuccessful ending thread, do it violently
		if(hThread) {
			OutputDebugString("Terminate thread...\n");
			TerminateThread(HANDLE(hThread), 0);
			CloseHandle(HANDLE(hThread));					   			
		}
		hThread=0;
	}

}

void Download::initThread()
{
	int status = true;
	while(!doExit && status)	{
		status = doDownload();
	}
	hThread=0;
}

void Download::initInternet() {
	hInternet = InternetOpen("MeOS", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);

	if(hInternet==NULL)	{
		DWORD ec = GetLastError();
    string error = lang.tl("Error: X#" + getErrorMessage(ec));
    throw std::exception(error.c_str());
	}
}

void Download::downloadFile(const string &url, const string &file, const vector< pair<string, string> > &headers)
{		 
	if(hURL || !hInternet) 
    throw std::exception("Not inititialized");

	success = false;

  string hdr;
  for (size_t k = 0; k<headers.size(); k++)
    hdr += headers[k].first + ": " + headers[k].second; 

  string url2 = url;
  hURL = InternetOpenUrl(hInternet, url2.c_str(), hdr.empty() ? 0 : hdr.c_str(), hdr.length(), INTERNET_FLAG_DONT_CACHE, 0);

	if (!hURL) {		
    int err = GetLastError();
    string msg2 = getErrorMessage(err);
		DWORD em = 0, blen = 256;
    char bf2[256];
		InternetGetLastResponseInfo(&em, bf2, &blen);
    string msg = "Failed to connect to: " + url;
    msg += " " + msg2;
    if (bf2[0] != 0)
      msg += " (" + string(bf2) + ")";
    throw std::exception(msg.c_str());
	}


	DWORD dwContentLen = 0;
  DWORD dwBufLen = sizeof(dwContentLen);
  BOOL success = HttpQueryInfo(hURL, 
                          HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER, 
                          (LPVOID)&dwContentLen, &dwBufLen, 0);

  if (success)
    setBytesToDownload(dwContentLen);
  else
    setBytesToDownload(0);

  DWORD dwStatus = 0;
  dwBufLen = sizeof(dwStatus);
  success = HttpQueryInfo(hURL, HTTP_QUERY_STATUS_CODE|HTTP_QUERY_FLAG_NUMBER,
                          (LPVOID)&dwStatus, &dwBufLen, 0);

  if (success) {
    if (dwStatus >= 400) {
      char bf[256];
      switch (dwStatus) {
        case HTTP_STATUS_BAD_REQUEST:
          sprintf_s(bf, "HTTP Error 400: The request could not be processed by the server due to invalid syntax.");
          break;
        case HTTP_STATUS_DENIED:
          sprintf_s(bf, "HTTP Error 401: The requested resource requires user authentication.");
          break;
        case HTTP_STATUS_FORBIDDEN:
          sprintf_s(bf, "HTTP Error 403: �tkomst nekad (access is denied).");
          break;
        case HTTP_STATUS_NOT_FOUND:
          sprintf_s(bf, "HTTP Error 404: Resursen kunde ej hittas (not found).");
          break;
        case HTTP_STATUS_NOT_SUPPORTED:
          sprintf_s(bf, "HTTP Error 501: F�rfr�gan st�ds ej (not supported).");
          break;
        case HTTP_STATUS_SERVER_ERROR:
          sprintf_s(bf, "HTTP Error 500: Internt serverfel (server error).");
          break;
        default: 
          sprintf_s(bf, "HTTP Status Error %d", dwStatus);
      }      
      throw dwException(bf, dwStatus);
    }
  }

  fileno=_open(file.c_str(), O_BINARY|O_CREAT|O_WRONLY|O_TRUNC, S_IREAD|S_IWRITE);

	if(fileno==-1)	{
		fileno=0;
		endDownload();
		char bf[256];
    sprintf_s(bf, "Error opening '%s' for writing", file);
    throw std::exception(bf);
	}

	bytesLoaded = 0;
	return;
}

void Download::endDownload()
{
	if(hURL && hInternet)	{
		InternetCloseHandle(hURL);
		hURL=NULL;
	}

	if (fileno)	{
		_close(fileno);
		fileno=0;
	}
}

bool Download::doDownload()
{
	if(hURL && hInternet)	{
		char buffer[512];

		DWORD bRead;
		if(InternetReadFile(hURL, buffer, 512, &bRead)) {
			//Success!
			if(bRead==0) {					
				//EOF
				success=true;
				endDownload();
			}
			else{
				if(_write(fileno, buffer, bRead) != int(bRead)) {
					endDownload();
          return false;
				}
				bytesLoaded+=bRead;
				return true;
			}	
		}	
	}	
	return false;
}

void Download::setBytesToDownload(DWORD btd)
{
	bytesToLoad = btd;
}

bool Download::isWorking()
{
	return hThread!=0;
}

bool Download::successful()
{
	return success;
}

void Download::postFile(const string &url, const string &file, const string &fileOut,
                        const vector< pair<string, string> > &headers, ProgressWindow &pw) {
  SetLastError(0);
  DWORD_PTR dw = 0;
  URL_COMPONENTS uc;
  memset(&uc, 0, sizeof(uc));
  uc.dwStructSize = sizeof(uc);
  char host[128];
  char path[128];
  char extra[256];
  uc.lpszExtraInfo = extra;
  uc.dwExtraInfoLength = sizeof(extra);
  uc.lpszHostName = host;
  uc.dwHostNameLength = sizeof(host);
  uc.lpszUrlPath = path;
  uc.dwUrlPathLength = sizeof(path);

  InternetCrackUrl(url.c_str(), url.length(), ICU_ESCAPE, &uc);
  int port = INTERNET_DEFAULT_HTTP_PORT;
  if (uc.nScheme == INTERNET_SCHEME_HTTPS)
    port = INTERNET_DEFAULT_HTTPS_PORT;
  else if (uc.nPort>0)
    port = uc.nPort;
  HINTERNET hConnect = InternetConnect(hInternet, host, port,
                                       NULL, NULL, INTERNET_SERVICE_HTTP, 0, dw);
  bool success = false;
  try {
    success = httpSendReqEx(hConnect, path, headers, file, fileOut, pw);
  }
  catch (std::exception &) {
    InternetCloseHandle(hConnect);
    throw;
  }
  InternetCloseHandle(hConnect);

  if (!success) {
 		DWORD ec = GetLastError();

    string error = ec != 0 ? getErrorMessage(ec) : "";
    if (error.empty())
      "Ett ok�nt fel intr�ffade.";
    throw std::exception(error.c_str());
  }
 }

bool Download::httpSendReqEx(HINTERNET hConnect, const string &dest, 
                             const vector< pair<string, string> > &headers,
                             const string &upFile, const string &outFile, ProgressWindow &pw) const {
  INTERNET_BUFFERS BufferIn;
  memset(&BufferIn, 0, sizeof(BufferIn));
  BufferIn.dwStructSize = sizeof( INTERNET_BUFFERS );

  HINTERNET hRequest = HttpOpenRequest (hConnect, "POST", dest.c_str(), NULL, NULL, NULL, INTERNET_FLAG_NO_CACHE_WRITE, 0);

  DWORD dwBytesRead;
  DWORD dwBytesWritten;
  BYTE pBuffer[4*1024]; // Read from file in 4K chunks

  string hdr;
  for (size_t k = 0; k<headers.size(); k++)
    hdr += headers[k].first + ": " + headers[k].second; 
  /*
  HttpSendRequest(hRequest, hdr.c_str(), hdr.length(), NULL, 0);
  
  // at this point normal authentication logic can be used. If
  // credentials are supplied in InternetConnect, then Wininet will
  // resubmit credentials itself.  See HttpDump Internet Client SDK sample
  // for more information. 

  // Read all returned data with InternetReadFile () 
  do {
    InternetReadFile (hRequest, pBuffer, sizeof(pBuffer), &dwBytesRead);
  }
  while (dwBytesRead != 0);
  */
  int retry = 5;
  while (retry>0) {

    HANDLE hFile = CreateFile(upFile.c_str(), GENERIC_READ, FILE_SHARE_READ, 
                              NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hFile == HANDLE(-1))
      return false;


    BufferIn.dwBufferTotal = GetFileSize (hFile, NULL);
    BufferIn.dwHeadersLength = hdr.length();
    BufferIn.lpcszHeader = hdr.c_str();

    double totSize = BufferIn.dwBufferTotal;

    if(!HttpSendRequestEx( hRequest, &BufferIn, NULL, 0, 0)) {
      CloseHandle(hFile);
      InternetCloseHandle(hRequest);
      return false;
    }

    DWORD sum = 0;
    do {
      if (!ReadFile (hFile, pBuffer, sizeof(pBuffer), &dwBytesRead, NULL)) {
        CloseHandle(hFile);
        InternetCloseHandle(hRequest);
        return false;
      }

      if (dwBytesRead > 0) {
        if (!InternetWriteFile(hRequest, pBuffer, dwBytesRead, &dwBytesWritten)) {
          CloseHandle(hFile);
          InternetCloseHandle(hRequest);
          return false;
        }
      }
      sum += dwBytesWritten;

      try {
        pw.setProgress(int(1000 * double(sum) / totSize));
      }
      catch (std::exception &) {
        CloseHandle(hFile);
        InternetCloseHandle(hRequest);
        throw;
      }
    }
    while (dwBytesRead == sizeof(pBuffer)) ;

    CloseHandle(hFile);
    
    if (!HttpEndRequest(hRequest, NULL, 0, 0)) {
      DWORD error = GetLastError();
      if (error == ERROR_INTERNET_FORCE_RETRY)      
        retry--;
      else if (error == ERROR_INTERNET_TIMEOUT) {
        throw std::exception("Fick inget svar i tid (ERROR_INTERNET_TIMEOUT)");
      }
      else {
        InternetCloseHandle(hRequest);
        return false;
      }
    }
    else
      retry = 0; // Done
  }

  DWORD dwStatus = 0;
  DWORD dwBufLen = sizeof(dwStatus);
  int success = HttpQueryInfo(hRequest, HTTP_QUERY_STATUS_CODE|HTTP_QUERY_FLAG_NUMBER,
                                        (LPVOID)&dwStatus, &dwBufLen, 0);

  if (success) {
    if (dwStatus >= 400) {
      char bf[256];
      switch (dwStatus) {
        case HTTP_STATUS_BAD_REQUEST:
          sprintf_s(bf, "HTTP Error 400: The request could not be processed by the server due to invalid syntax.");
          break;
        case HTTP_STATUS_DENIED:
          sprintf_s(bf, "HTTP Error 401: The requested resource requires user authentication.");
          break;
        case HTTP_STATUS_FORBIDDEN:
          sprintf_s(bf, "HTTP Error 403: �tkomst nekad (access is denied).");
          break;
        case HTTP_STATUS_NOT_FOUND:
          sprintf_s(bf, "HTTP Error 404: Resursen kunde ej hittas (not found).");
          break;
        case HTTP_STATUS_NOT_SUPPORTED:
          sprintf_s(bf, "HTTP Error 501: F�rfr�gan st�ds ej (not supported).");
          break;
        case HTTP_STATUS_SERVER_ERROR:
          sprintf_s(bf, "HTTP Error 500: Internt serverfel (server error).");
          break;
        default: 
          sprintf_s(bf, "HTTP Status Error %d", dwStatus);
      }      
      throw dwException(bf, dwStatus);
    }
  }
  
  int fileno = _open(outFile.c_str(), O_BINARY|O_CREAT|O_WRONLY|O_TRUNC, S_IREAD|S_IWRITE);

	do {	
    dwBytesRead=0;
		if(InternetReadFile(hRequest, pBuffer, sizeof(pBuffer)-1, &dwBytesRead)) {
      _write(fileno, pBuffer, dwBytesRead);
	  }
  } while(dwBytesRead>0);

  _close(fileno);

  InternetCloseHandle(hRequest);
  return true;
}

void Download::postData(const string &url, const string &data, ProgressWindow &pw) {
  SetLastError(0);
  DWORD_PTR dw = 0;
  URL_COMPONENTS uc;
  memset(&uc, 0, sizeof(uc));
  uc.dwStructSize = sizeof(uc);
  char host[128];
  char path[128];
  char extra[256];
  uc.lpszExtraInfo = extra;
  uc.dwExtraInfoLength = sizeof(extra);
  uc.lpszHostName = host;
  uc.dwHostNameLength = sizeof(host);
  uc.lpszUrlPath = path;
  uc.dwUrlPathLength = sizeof(path);

  InternetCrackUrl(url.c_str(), url.length(), ICU_ESCAPE, &uc);
  int port = INTERNET_DEFAULT_HTTP_PORT;
  if (uc.nScheme == INTERNET_SCHEME_HTTPS)
    port = INTERNET_DEFAULT_HTTPS_PORT;
  else if (uc.nPort>0)
    port = uc.nPort;
  HINTERNET hConnect = InternetConnect(hInternet, host, port,
                                       NULL, NULL, INTERNET_SERVICE_HTTP, 0, dw);
  static TCHAR hdrs[] = _T("Content-Type: application/x-www-form-urlencoded");
	static string formdata = data;
	PCTSTR accept[] = {_T("*/*"), NULL};

	HINTERNET hRequest(NULL);
	bool success (true);

	if (!hConnect)
		success = false;

	if (success)
		hRequest = HttpOpenRequest(hConnect, "POST", uc.lpszUrlPath, NULL, NULL, accept, 0, 1);
	if (!hRequest)
		success = false;

	if (success && !HttpSendRequest(hRequest, hdrs, strlen(hdrs), (TCHAR*)formdata.c_str(), formdata.size()))
		success = false;
	else {
		DWORD dwStatus = 0;
		DWORD dwBufLen = sizeof(dwStatus);
	  success = !!HttpQueryInfo(hRequest, HTTP_QUERY_STATUS_CODE|HTTP_QUERY_FLAG_NUMBER,
                          (LPVOID)&dwStatus, &dwBufLen, 0);
		if (success && dwStatus >= 400) {
      char bf[256];
      switch (dwStatus) {
        case HTTP_STATUS_BAD_REQUEST:
          sprintf_s(bf, "HTTP Error 400: The request could not be processed by the server due to invalid syntax.");
          break;
        case HTTP_STATUS_DENIED:
          sprintf_s(bf, "HTTP Error 401: The requested resource requires user authentication.");
          break;
        case HTTP_STATUS_FORBIDDEN:
          sprintf_s(bf, "HTTP Error 403: �tkomst nekad (access is denied).");
          break;
        case HTTP_STATUS_NOT_FOUND:
          sprintf_s(bf, "HTTP Error 404: Resursen kunde ej hittas (not found).");
          break;
        case HTTP_STATUS_NOT_SUPPORTED:
          sprintf_s(bf, "HTTP Error 501: F�rfr�gan st�ds ej (not supported).");
          break;
        case HTTP_STATUS_SERVER_ERROR:
          sprintf_s(bf, "HTTP Error 500: Internt serverfel (server error).");
          break;
        default: 
          sprintf_s(bf, "HTTP Status Error %d", dwStatus);
      }      
      throw dwException(bf, dwStatus);
    }
	}

  if (hConnect)
		InternetCloseHandle(hConnect);

  if (!success) {
 		DWORD ec = GetLastError();

    string error = ec != 0 ? getErrorMessage(ec) : "";
    if (error.empty())
      lang.tl("Ett ok�nt fel intr�ffade.");
    throw std::exception(error.c_str());
  }

 }
