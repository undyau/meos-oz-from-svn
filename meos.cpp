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
    Stigbergsvägen 7, SE-75242 UPPSALA, Sweden

************************************************************************/

// meos.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "resource.h"
#include <shlobj.h>

#include "oEvent.h"
#include "xmlparser.h"

#include "gdioutput.h"
#include "commctrl.h"
#include "SportIdent.h"
#include "TabBase.h"
#include "TabCompetition.h"
#include "TabAuto.h"
#include "TabClass.h"
#include "TabCourse.h"
#include "TabControl.h"
#include "TabSI.h"
#include "TabList.h"
#include "TabTeam.h"
#include "TabSpeaker.h"
#include "TabMulti.h"
#include "TabRunner.h"
#include "TabClub.h"
#include "progress.h"
#include "inthashmap.h"
#include <cassert>
#include "localizer.h"
#include "intkeymap.hpp"
#include "intkeymapimpl.hpp"
#include "download.h"
#include "meos_util.h"
#include <sys/stat.h>
#include "random.h"

gdioutput *gdi_main=0;
oEvent *gEvent=0;
SportIdent *gSI=0;
Localizer lang;

vector<gdioutput *> gdi_extra;
void resetSaveTimer();
void initMySQLCriticalSection(bool init);

HWND hWndMain;
HWND hWndWorkspace;

#define MAX_LOADSTRING 100

#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

void removeTempFiles();
void Setup(bool overwrite);

// Global Variables:
HINSTANCE hInst; // current instance
TCHAR szTitle[MAX_LOADSTRING]; // The title bar text
TCHAR szWindowClass[MAX_LOADSTRING]; // The title bar text
TCHAR szWorkSpaceClass[MAX_LOADSTRING]; // The title bar text

// Foward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK	WorkSpaceWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK	About(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK GetMsgProc(int nCode, WPARAM wParam, LPARAM lParam); 
void registerToolbar(HINSTANCE hInstance);
extern const char *szToolClass;

HHOOK g_hhk; //- handle to the hook procedure. 

HWND hMainTab=NULL;

list<TabObject> *tabList=0;
void scrollVertical(gdioutput *gdi, int yInc, HWND hWnd);
static int currentFocusIx = 0;


void LoadPage(const string &name)
{
  list<TabObject>::iterator it;

  for (it=tabList->begin(); it!=tabList->end(); ++it) {
    if(it->name==name)
      it->loadPage(*gdi_main);
  }
}

void LoadClassPage(gdioutput &gdi)
{
  LoadPage("Klasser");
}

void LoadPage(gdioutput &gdi, TabType type) {
  TabBase *t = gdi.getTabs().get(type);
  if (t)
    t->loadPage(gdi);
}

// Path to settings file
static char settings[260];
  
int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
	_CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
  
  if (strstr(lpCmdLine, "-s") != 0) {
    Setup(true);
    exit(0);
  }

  getUserFile(settings, "meospref.xml");

  int rInit = (GetTickCount() / 100);
  InitRanom(rInit, rInit/379);
  //intkeymap<int> foo(17);
 /* inthashmap foo(17);
  
  for (int i = 0; i<30; i++)
    foo[i*7] = i;

  for (int i = 0; i<10; i++)
    foo.remove(i*7);

  for (int i = 29; i>=15; i--)
    foo[i*7] = i+100;

  int v;
  for (int i = 0; i<10; i++)
    assert(foo.lookup(i*7,v) == false);
  
  for (int i = 15; i<29; i++) {
    assert(foo.lookup(i*7,v) == true);
    assert(v == 100 +i);
  }

  for (int i = 20; i<30; i++)
    foo.remove(i*7);

  for (int i = 0; i<10; i++)
    foo.insert(i*7, i + 1000);

  for (int i = 20; i<30; i++)
    assert(foo.lookup(i*7,v) == false);
  
  for (int i = 0; i<10; i++) {
    assert(foo.lookup(i*7,v) == true);
    assert(v == 1000 +i);
    assert(foo[i*7] == 1000 +i);
  }


  foo.remove(3);*/
  //vector<string> files;
  //files.push_back("f:\\temp\\result_test.xml");
  //zip("f:\\temp\\foo.zip", 0, files);
 // vector<string> a, b;
 // unzip("c:\\temp\\foo.zip", 0, a);
 // unzip("c:\\temp\\foo.zip", 0, b);

  tabList=new list<TabObject>;

	MSG msg;
	HACCEL hAccelTable;

	gdi_main=new gdioutput(1.0);
  gdi_extra.push_back(gdi_main);

	gEvent = new oEvent(*gdi_main);
  gEvent->loadProperties(settings);
  
  lang.addLangResource("English", "104");
  lang.addLangResource("Svenska", "103");
  lang.addLangResource("Deutsch", "105");

  if (fileExist("extra.lng")) {
    lang.addLangResource("Extraspråk", "extra.lng");
  }
  else {
    char lpath[260];
    getUserFile(lpath, "extra.lng");
    if (fileExist(lpath))
      lang.addLangResource("Extraspråk", lpath);
  }
  
  string defLang = gEvent->getPropertyString("Language", "Svenska");

  // Backward compatibility
  if (defLang=="103")
    defLang = "Svenska";
  else if (defLang=="104")
    defLang = "English";

  gEvent->setProperty("Language", defLang);

  try {
    lang.loadLangResource(defLang);
  }
  catch (std::exception &) {
    lang.loadLangResource("Svenska");
  }

  gEvent->openRunnerDatabase("database");
	strcpy_s(szTitle, "MeOS");
	strcpy_s(szWindowClass, "MeosMainClass");
	strcpy_s(szWorkSpaceClass, "MeosWorkSpace");
	MyRegisterClass(hInstance);  
  registerToolbar(hInstance);

  gdi_main->setFont(gEvent->getPropertyInt("TextSize", 0),
                    gEvent->getPropertyString("TextFont", "Arial"));

	// Perform application initialization:
	if (!InitInstance (hInstance, nCmdShow)) {
		return FALSE;
	}

	RECT rc;
	GetClientRect(hWndMain, &rc);
	SendMessage(hWndMain, WM_SIZE, 0, MAKELONG(rc.right, rc.bottom));

	gdi_main->init(hWndWorkspace, hWndMain, hMainTab);
	gdi_main->getTabs().get(TCmpTab)->loadPage(*gdi_main);
	
  //createExtraWindow(hInst);
  // Install a hook procedure to monitor the message stream for mouse 
  // messages intended for the controls in the dialog box. 
  g_hhk = SetWindowsHookEx(WH_GETMESSAGE, GetMsgProc, 
      (HINSTANCE) NULL, GetCurrentThreadId()); 

	hAccelTable = LoadAccelerators(hInstance, (LPCTSTR)IDC_MEOS);

  initMySQLCriticalSection(true);
	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0)) 
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) 
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
  tabAutoRegister(0);
  tabList->clear();
  delete tabList;
  tabList=0;

  for (size_t k = 0; k<gdi_extra.size(); k++) {
    if (gdi_extra[k]) {
      DestroyWindow(gdi_extra[k]->getHWND());
      if (k < gdi_extra.size()) {
        delete gdi_extra[k];
        gdi_extra[k] = 0;
      }
    }
  }

  gdi_extra.clear();

  if (gEvent)
    gEvent->saveProperties(settings);

	delete gEvent;

  initMySQLCriticalSection(false);

  removeTempFiles();

  #ifdef _DEBUG
    lang.debugDump("untranslated.txt", "translated.txt");
  #endif

	//_CrtDumpMemoryLeaks();

	return msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    This function and its usage is only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
//    function that was added to Windows 95. It is important to call this function
//    so that the application will get 'well formed' small icons associated
//    with it.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX); 
	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= (WNDPROC)WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, (LPCTSTR)IDI_MEOS);
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= 0;//(LPCSTR)IDC_MEOS;
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, (LPCTSTR)IDI_SMALL);

	RegisterClassEx(&wcex);

	wcex.cbSize = sizeof(WNDCLASSEX); 
  wcex.style			= CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
	wcex.lpfnWndProc	= (WNDPROC)WorkSpaceWndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, (LPCTSTR)IDI_MEOS);;
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= 0;
	wcex.lpszMenuName	= 0;
	wcex.lpszClassName	= szWorkSpaceClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, (LPCTSTR)IDI_SMALL);
	RegisterClassEx(&wcex);

	return true;
}
		

// GetMsgProc - monitors the message stream for mouse messages intended 
//     for a control window in the dialog box. 
// Returns a message-dependent value. 
// nCode - hook code. 
// wParam - message flag (not used). 
// lParam - address of an MSG structure. 
LRESULT CALLBACK GetMsgProc(int nCode, WPARAM wParam, LPARAM lParam) 
{ 
    MSG *lpmsg; 
	
    lpmsg = (MSG *) lParam; 
    if (nCode < 0 || !(IsChild(hWndWorkspace, lpmsg->hwnd))) 
        return (CallNextHookEx(g_hhk, nCode, wParam, lParam)); 
	
    switch (lpmsg->message) { 
	case WM_MOUSEMOVE: 
	case WM_LBUTTONDOWN:  
	case WM_LBUTTONUP: 
	case WM_RBUTTONDOWN: 
	case WM_RBUTTONUP: 
		if (gdi_main->getToolTip() != NULL) { 
			MSG msg; 
			
			msg.lParam = lpmsg->lParam; 
			msg.wParam = lpmsg->wParam; 
			msg.message = lpmsg->message; 
			msg.hwnd = lpmsg->hwnd; 
			SendMessage(gdi_main->getToolTip(), TTM_RELAYEVENT, 0, 
				(LPARAM) (LPMSG) &msg); 
		} 
		break; 
	default: 
		break; 
    } 
    return (CallNextHookEx(g_hhk, nCode, wParam, lParam)); 
} 



LRESULT CALLBACK KeyboardProc(int code, WPARAM wParam, LPARAM lParam)
{
  gdioutput *gdi = 0;

  if (size_t(currentFocusIx) < gdi_extra.size())
    gdi = gdi_extra[currentFocusIx];

  if (!gdi)
    gdi = gdi_main;


  HWND hWnd = gdi ? gdi->getHWND() : 0;
  
  bool ctrlPressed = (GetKeyState(VK_CONTROL) & 0x8000) == 0x8000;
  
	//if(code<0) return CallNextHookEx(
	if(wParam==VK_TAB && (lParam& (1<<31)))
	{
		SHORT state=GetKeyState(VK_SHIFT);
    if(gdi) {
		  if(state&(1<<16))
			  gdi->TabFocus(-1);
		  else
			  gdi->TabFocus(1);
    }
	}
  else if(wParam==VK_RETURN && (lParam & (1<<31))) {
		if(gdi)
      gdi->Enter();
  }
  else if(wParam==VK_UP) {
    bool c = false;
		if(gdi  && (lParam & (1<<31)))
      c = gdi->UpDown(1);

    if (!c  && !(lParam & (1<<31)) && !(gdi && gdi->lockUpDown))
      SendMessage(hWnd, WM_VSCROLL, MAKELONG(SB_LINEUP, 0), 0);
  }
  else if (wParam == VK_NEXT && !(lParam & (1<<31)) ) {
    SendMessage(hWnd, WM_VSCROLL, MAKELONG(SB_PAGEDOWN, 0), 0);
  }
  else if (wParam == VK_PRIOR && !(lParam & (1<<31)) ) {
    SendMessage(hWnd, WM_VSCROLL, MAKELONG(SB_PAGEUP, 0), 0);
  }
  else if(wParam==VK_DOWN) {
    bool c = false;
    if(gdi && (lParam & (1<<31)))
      c = gdi->UpDown(-1);

    if (!c && !(lParam & (1<<31)) && !(gdi && gdi->lockUpDown)) 
      SendMessage(hWnd, WM_VSCROLL, MAKELONG(SB_LINEDOWN, 0), 0);
  }
  else if(wParam==VK_LEFT && !(lParam & (1<<31))) {
    if (!gdi || !gdi->hasEditControl())
      SendMessage(hWnd, WM_HSCROLL, MAKELONG(SB_LINEUP, 0), 0);
  }
  else if(wParam==VK_RIGHT && !(lParam & (1<<31))) {
    if (!gdi || !gdi->hasEditControl())
      SendMessage(hWnd, WM_HSCROLL, MAKELONG(SB_LINEDOWN, 0), 0);
  }
  else if(wParam==VK_ESCAPE && (lParam & (1<<31))) {
		if(gdi)
      gdi->Escape();
  }
  else if (wParam==VK_F2) {
    ProgressWindow pw(hWnd);
    
    pw.init();
    for (int k=0;k<=20;k++) {
      pw.setProgress(k*50);
      Sleep(100);
    }
    //pw.draw();
  }
  else if (ctrlPressed && (wParam == VK_ADD || wParam == VK_SUBTRACT ||
           wParam == VK_F5 || wParam == VK_F6)) {
    if (gdi) {
      if (wParam == VK_ADD || wParam == VK_F5)
        gdi->scaleSize(1.1);
      else
        gdi->scaleSize(1.0/1.1);
    }
  }
  else if (wParam == 'C' && ctrlPressed) {
    if (gdi)
      gdi->keyCommand(KC_COPY);
  }
  else if (wParam == 'V'  && ctrlPressed) {
    if (gdi)
      gdi->keyCommand(KC_PASTE);
  }
  else if (wParam == VK_DELETE) {
    if (gdi)
      gdi->keyCommand(KC_DELETE);
  }
  else if (wParam == 'I' &&  ctrlPressed) {
    if (gdi)
      gdi->keyCommand(KC_INSERT);
  }
  else if (wParam == 'P' && ctrlPressed) {
    if (gdi)
      gdi->keyCommand(KC_PRINT);
  }
  else if (wParam == 'I' && ctrlPressed) {
    if (gdi)
      gdi->keyCommand(KC_INSERT);
  }
  else if (wParam == VK_F5 && !ctrlPressed) {
    if (gdi)
      gdi->keyCommand(KC_REFRESH);
  }

		//MessageBeep(-1);
		
	return 0;
}
//
//   FUNCTION: InitInstance(HANDLE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	HWND hWnd;
	
	hInst = hInstance; // Store instance handle in our global variable
	//WS_EX_CONTROLPARENT
  HWND hDskTop=GetDesktopWindow();
  RECT rc;
  GetClientRect(hDskTop, &rc);

  int xp = gEvent->getPropertyInt("xpos", 50);
  int yp = gEvent->getPropertyInt("ypos", 20);

  int xs = gEvent->getPropertyInt("xsize", max(850, min(int(rc.right)-yp, 1124)));
  int ys = gEvent->getPropertyInt("ysize", max(650, min(int(rc.bottom)-yp-40, 800)));

  gEvent->setProperty("ypos", yp + 16);
  gEvent->setProperty("xpos", xp + 32);
  gEvent->saveProperties(settings); // For other instance starting while running
  gEvent->setProperty("ypos", yp);
  gEvent->setProperty("xpos", xp);

  hWnd = CreateWindowEx(0, szWindowClass, szTitle, 
          WS_OVERLAPPEDWINDOW|WS_CLIPCHILDREN|WS_CLIPSIBLINGS,
          xp, yp, max(min(int(rc.right)-yp, xs), 200), 
                  max(min(int(rc.bottom)-yp-40, ys), 100),
          NULL, NULL, hInstance, NULL);
	
	if (!hWnd)
		return FALSE;	
	
	hWndMain = hWnd;
  resetSaveTimer();

  SetWindowsHookEx(WH_KEYBOARD, KeyboardProc, 0, GetCurrentThreadId());
	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);
	
	hWnd = CreateWindowEx(0, szWorkSpaceClass, "WorkSpace", WS_CHILD|WS_CLIPCHILDREN|WS_CLIPSIBLINGS,
		50, 200, 200, 100, hWndMain, NULL, hInstance, NULL);
	
	if (!hWnd)
		return FALSE;
	
	hWndWorkspace=hWnd;
	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);
	
	return TRUE;
}

void destroyExtraWindows() {
  for (size_t k = 1; k<gdi_extra.size(); k++) {
    if (gdi_extra[k]) {
      DestroyWindow(gdi_extra[k]->getHWND());
    }
  }
}

gdioutput *createExtraWindow(const string &title, int max_x, int max_y)
{
	HWND hWnd;

  HWND hDskTop=GetDesktopWindow();
  RECT rc;
  GetClientRect(hDskTop, &rc);

  int xp = gEvent->getPropertyInt("xpos", 50) + 16;
  int yp = gEvent->getPropertyInt("ypos", 20) + 32;

  for (size_t k = 0; k<gdi_extra.size(); k++) {
    if (gdi_extra[k]) {
      HWND hWnd = gdi_extra[k]->getHWND();
      RECT rc;
      if (GetWindowRect(hWnd, &rc)) {
        xp = max<int>(rc.left + 16, xp);
        yp = max<int>(rc.top + 32, yp);
      }
    }
  }

  int xs = gEvent->getPropertyInt("xsize", max(850, min(int(rc.right)-yp, 1124)));
  int ys = gEvent->getPropertyInt("ysize", max(650, min(int(rc.bottom)-yp-40, 800)));

  if (max_x>0)
    xs = min(max_x, xs);
  if (max_y>0)
    ys = min(max_y, ys);
  
  hWnd = CreateWindowEx(0, szWorkSpaceClass, title.c_str(), 
    WS_OVERLAPPEDWINDOW|WS_CLIPCHILDREN|WS_CLIPSIBLINGS,
		xp, yp, max(xs, 200), max(ys, 100), 0, NULL, hInst, NULL);
	
	if (!hWnd)
		return 0;
	
	ShowWindow(hWnd, SW_SHOWNORMAL);
	UpdateWindow(hWnd);
  gdioutput *gdi = new gdioutput(1.0);
  gdi->setFont(gEvent->getPropertyInt("TextSize", 0),
               gEvent->getPropertyString("TextFont", "Arial"));

  gdi->init(hWnd, hWnd, 0);

  SetWindowLong(hWnd, GWL_USERDATA, gdi_extra.size());
  currentFocusIx = gdi_extra.size();
  gdi_extra.push_back(gdi);

	return gdi;
}


//
//  FUNCTION: WndProc(HWND, unsigned, WORD, LONG)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
/*
void CallBack(gdioutput *gdi, int type, void *data)
{
	ButtonInfo bi=*(ButtonInfo* )data;

	string t=gdi->getText("input");
	gdi->ClearPage();

	gdi->addButton(bi.text+" *"+t, bi.xp+5, bi.yp+30, "", CallBack);
}

void CallBackLB(gdioutput *gdi, int type, void *data)
{
	ListBoxInfo lbi=*(ListBoxInfo* )data;

	gdi->setText("input", lbi.text);
}

void CallBackINPUT(gdioutput *gdi, int type, void *data)
{
	InputInfo lbi=*(InputInfo* )data;

	MessageBox(NULL, "MB_OK", 0, MB_OK);
	//gdi->setText("input", lbi.text);
}*/

void InsertSICard(gdioutput &gdi, SICard &sic);

//static int xPos=0, yPos=0;
void createTabs(bool force, bool onlyMain, bool skipTeam, bool skipSpeaker, 
                bool skipEconomy, bool skipLists, bool skipRunners, bool skipControls)
{
  static bool onlyMainP = false;
  static bool skipTeamP = false;
  static bool skipSpeakerP = false;
  static bool skipEconomyP = false;
  static bool skipListsP = false;
  static bool skipRunnersP = false;
  static bool skipControlsP = false;

  if (!force && onlyMain==onlyMainP && skipTeam==skipTeamP && skipSpeaker==skipSpeakerP &&
      skipEconomy==skipEconomyP && skipLists==skipListsP && 
      skipRunners==skipRunnersP && skipControls==skipControlsP)
    return;

  onlyMainP = onlyMain;
  skipTeamP = skipTeam;
  skipSpeakerP = skipSpeaker;
  skipEconomyP = skipEconomy;
  skipListsP = skipLists;
  skipRunnersP = skipRunners;
  skipControlsP = skipControls;

	int oldid=TabCtrl_GetCurSel(hMainTab);
  TabObject *to = 0;
	for (list<TabObject>::iterator it=tabList->begin();it!=tabList->end();++it) {
	  if (it->id==oldid) {
      to = &*it;
  	}
  }

	SendMessage(hMainTab, WM_SETFONT, (WPARAM) GetStockObject(DEFAULT_GUI_FONT), 0);
	int id=0;
  TabCtrl_DeleteAllItems(hMainTab);
	for (list<TabObject>::iterator it=tabList->begin();it!=tabList->end();++it) {
    it->setId(-1);

    if (onlyMain && it->getType() != typeid(TabCompetition))
      continue;

    if (skipTeam && it->getType() == typeid(TabTeam))
      continue;

    if (skipSpeaker && it->getType() == typeid(TabSpeaker))
      continue;

    if (skipEconomy && it->getType() == typeid(TabClub))
      continue;
    
    if (skipRunners && it->getType() == typeid(TabRunner))
      continue;

    if (skipControls && it->getType() == typeid(TabControl))
      continue;

    if (skipLists && (it->getType() == typeid(TabList) || it->getType() == typeid(TabAuto)))
      continue;

		TCITEM ti;
		char bf[256];
		strcpy_s(bf, lang.tl(it->name).c_str());
		ti.pszText=bf;
		ti.mask=TCIF_TEXT;			
		it->setId(id++);
		TabCtrl_InsertItem(hMainTab, it->id, &ti);
	}

  if (to && (to->id)>=0)
    TabCtrl_SetCurSel(hMainTab, to->id);
}

void resetSaveTimer() {
  KillTimer(hWndMain, 1);
  SetTimer(hWndMain, 1, (3*60+15)*1000, 0); //Autosave
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message) 
	{
		case WM_CREATE:	
			
      tabList->push_back(TabObject(gdi_main->getTabs().get(TCmpTab), "Tävling"));
      tabList->push_back(TabObject(gdi_main->getTabs().get(TRunnerTab), "Deltagare"));
      tabList->push_back(TabObject(gdi_main->getTabs().get(TTeamTab), "Lag"));
      tabList->push_back(TabObject(gdi_main->getTabs().get(TListTab), "Listor"));
      {
        TabAuto *ta = (TabAuto *)gdi_main->getTabs().get(TAutoTab);
        tabList->push_back(TabObject(ta, "Automater"));
        tabAutoRegister(ta);
      }
      tabList->push_back(TabObject(gdi_main->getTabs().get(TSpeakerTab), "Speaker"));
      tabList->push_back(TabObject(gdi_main->getTabs().get(TClassTab), "Klasser"));
      tabList->push_back(TabObject(gdi_main->getTabs().get(TCourseTab), "Banor"));
      tabList->push_back(TabObject(gdi_main->getTabs().get(TControlTab), "Kontroller"));
      tabList->push_back(TabObject(gdi_main->getTabs().get(TClubTab), "Klubbar"));

      tabList->push_back(TabObject(gdi_main->getTabs().get(TSITab), "SportIdent"));
			
			INITCOMMONCONTROLSEX ic;

			ic.dwSize=sizeof(ic);
			ic.dwICC=ICC_TAB_CLASSES ;
			InitCommonControlsEx(&ic);
			hMainTab=CreateWindowEx(0, WC_TABCONTROL, "tabs", WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS, 0, 0, 300, 20, hWnd, 0, hInst, 0);
      createTabs(true, true, false, false, false, false, false, false);

			//SetTimer(hWnd, 1, 5*60*1000, 0); //Autosave
			SetTimer(hWnd, 2, 1000, 0); //Interface timeout
			SetTimer(hWnd, 3, 2500, 0); //DataSync
      SetTimer(hWnd, 4, 5000, 0); //Connection check
			break;

    case WM_MOUSEWHEEL: {
      int dz = GET_WHEEL_DELTA_WPARAM(wParam);
      scrollVertical(gdi_main, -dz, hWndWorkspace);
      }
      break;

		case WM_SIZE:
			MoveWindow(hMainTab, 0,0, LOWORD(lParam), 30, 1);
			MoveWindow(hWndWorkspace, 0, 30, LOWORD(lParam), HIWORD(lParam)-30, 1);

			RECT rc;
			GetClientRect(hWndWorkspace, &rc);
			PostMessage(hWndWorkspace, WM_SIZE, wParam, MAKELONG(rc.right, rc.bottom));
			break;

    case WM_WINDOWPOSCHANGED:
      if (gEvent) {
        LPWINDOWPOS wp = (LPWINDOWPOS) lParam; // points to size and position data 
 
        if (wp->x>=0 && wp->y>=0 && wp->cx>300 && wp->cy>200) {
          gEvent->setProperty("xpos", wp->x);
          gEvent->setProperty("ypos", wp->y);
          gEvent->setProperty("xsize", wp->cx);
          gEvent->setProperty("ysize", wp->cy);
        }
        else {
          Sleep(0);
        }
      }
      return DefWindowProc(hWnd, message, wParam, lParam);
		case WM_TIMER:

			if(!gdi_main) return 0;

			if (wParam==1) {
				if(gEvent && !gEvent->empty()){
					RECT rc;
					GetClientRect(hWnd, &rc);					
					gEvent->save();					
					gdi_main->addInfoBox("", "Tävlingsdata har sparats.", 10);
				}
			}
			else if (wParam==2) {
        for (size_t k = 0; k<gdi_extra.size(); k++) {
          if (gdi_extra[k])
				    gdi_extra[k]->CheckInterfaceTimeouts(GetTickCount());
        }
				tabAutoTimer(*gdi_main);
			}
			else if (wParam==3) {
				//SQL-synch only if in list-show mode.
        tabAutoSync(gdi_extra, gEvent);
			}
      else if (wParam==4) {
        // Verify database link
        if(gEvent)
          gEvent->verifyConnection();
        
        if(gdi_main) {
          if (gEvent->hasClientChanged()) {
            gdi_main->makeEvent("Connections", 0);
            gEvent->validateClients();
          }
        }
      }
			break;

    case WM_ACTIVATE: 
      if (LOWORD(wParam) != WA_INACTIVE)
        currentFocusIx = 0;
      return DefWindowProc(hWnd, message, wParam, lParam);

    case WM_NCACTIVATE:
      if (gdi_main && gdi_main->hasToolbar())
        gdi_main->activateToolbar(wParam != 0);        
      return DefWindowProc(hWnd, message, wParam, lParam);

		case WM_NOTIFY:
		{
			LPNMHDR pnmh = (LPNMHDR) lParam; 

			if(pnmh->hwndFrom==hMainTab && gdi_main && gEvent)
			{
				if(pnmh->code==TCN_SELCHANGE)
				{
					int id=TabCtrl_GetCurSel(hMainTab);

					for (list<TabObject>::iterator it=tabList->begin();it!=tabList->end();++it) {
						if (it->id==id) {
              try {
							  it->loadPage(*gdi_main);
              }
              catch(std::exception &ex) {
                MessageBox(hWnd, ex.what(), "Ett fel inträffade", MB_OK);
              }
						}
					}
				}
				else if (pnmh->code==TCN_SELCHANGING) {
					if(gEvent->empty())	{
						MessageBeep(-1);
						return true;
					}
					else {
						if(!gdi_main->canClear())
							return true;

						return false;
					}
				}
			}
			break;
		}

		case WM_USER://MessageBox(hWnd, "SI REC", NULL, MB_OK);

			//The card has been read and posted to a synchronized 
			//queue by different thread. Read and process this card.
      {
			  SICard sic;
			  while (gSI && gSI->GetCard(sic)) 
				  InsertSICard(*gdi_main, sic);
			  break;
      }
		case WM_USER+1:
			MessageBox(hWnd, "Kommunikationen med en SI-enhet avbröts.", "SportIdent", MB_OK);
      break;
            
		case WM_COMMAND:
			wmId    = LOWORD(wParam); 
			wmEvent = HIWORD(wParam); 
			// Parse the menu selections:
			switch (wmId)
			{
				case IDM_EXIT:
				   //DestroyWindow(hWnd);
					PostMessage(hWnd, WM_CLOSE, 0,0);
				   break;
				default:
				   return DefWindowProc(hWnd, message, wParam, lParam);
			}
			break;
		case WM_PAINT:
			hdc = BeginPaint(hWnd, &ps);
			// TODO: Add any drawing code here...

	
			EndPaint(hWnd, &ps);
			break;

		case WM_CLOSE:
			if(!gEvent || gEvent->empty() || gdi_main->ask("Vill du verkligen stänga MeOS?"))
					DestroyWindow(hWnd);
			break;

		case WM_DESTROY:
			delete gSI;
			gSI=0;

			if (gEvent) {
        try {
  				gEvent->save();
        }
        catch(std::exception &ex) {
          MessageBox(hWnd, ex.what(), "Fel när tävlingen skulle sparas", MB_OK);
        }

        try {
          gEvent->saveRunnerDatabase("database", true);
        }
        catch(std::exception &ex) {
          MessageBox(hWnd, ex.what(), "Fel när löpardatabas skulle sparas", MB_OK);
        }

        if (gEvent)
          gEvent->saveProperties(settings);

				delete gEvent;
				gEvent=0;
			}

			PostQuitMessage(0);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
   }
   return 0;
}

void scrollVertical(gdioutput *gdi, int yInc, HWND hWnd) {
	SCROLLINFO si;
	si.cbSize=sizeof(si);
	si.fMask=SIF_ALL;			
	GetScrollInfo(hWnd, SB_VERT, &si);
  if (si.nPage==0)
    yInc = 0;

  int yPos=gdi->GetOffsetY();
	int a=si.nMax-signed(si.nPage-1) - yPos;

	if ( (yInc = max( -yPos, min(yInc, a)))!=0 ) { 
		yPos += yInc; 
		RECT ScrollArea, ClipArea;
    GetClientRect(hWnd, &ScrollArea);
		ClipArea=ScrollArea;

    ScrollArea.top=-gdi->getHeight()-100;
    ScrollArea.bottom+=gdi->getHeight();
    ScrollArea.right=gdi->getWidth()-gdi->GetOffsetX()+15;
    ScrollArea.left = -2000;
		gdi->SetOffsetY(yPos);				
	
		ScrollWindowEx (hWnd, 0,  -yInc, 
			&ScrollArea, &ClipArea, 
			(HRGN) NULL, (LPRECT) NULL, SW_INVALIDATE|SW_SCROLLCHILDREN); 
    
   //	gdi->UpdateObjectPositions();

		si.cbSize = sizeof(si); 
		si.fMask  = SIF_POS; 
		si.nPos   = yPos; 
	
		SetScrollInfo(hWnd, SB_VERT, &si, TRUE); 				
	  UpdateWindow (hWnd);		
  }
}


LRESULT CALLBACK WorkSpaceWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;

  LONG ix = GetWindowLong(hWnd, GWL_USERDATA);
  gdioutput *gdi = 0;
  if (ix < LONG(gdi_extra.size()))
    gdi = gdi_extra[ix];

  if(gdi) {
		LRESULT res = gdi->ProcessMsg(message, lParam, wParam);
    if (res)
      return res;
  }
	switch (message) 
	{
		case WM_CREATE:	
			break;

		case WM_SIZE:
  		SCROLLINFO si;
			si.cbSize=sizeof(si);
			si.fMask=SIF_PAGE|SIF_RANGE;
			
			int nHeight;
      nHeight = HIWORD(lParam); 
      int nWidth;
      nWidth = LOWORD(lParam);

      int maxx, maxy;
      gdi->clipOffset(nWidth, nHeight, maxx, maxy);

      si.nMin=0;

      if(maxy>0) {
        si.nMax=maxy+nHeight;
        si.nPos=gdi->GetOffsetY();
        si.nPage=nHeight; 
      }
      else {
        si.nMax=0;
        si.nPos=0;
        si.nPage=0;
      }
			SetScrollInfo(hWnd, SB_VERT, &si, true);
 
      si.nMin=0;
      if(maxx>0) {        
        si.nMax=maxx+nWidth;
        si.nPos=gdi->GetOffsetX();
        si.nPage=nWidth; 
			}
      else {
        si.nMax=0;
        si.nPos=0;
        si.nPage=0;
      }
      SetScrollInfo(hWnd, SB_HORZ, &si, true);

      InvalidateRect(hWnd, NULL, true);
			//gdi.refresh();__NO NO NO recursive
			//gdi.Up
			break;

		case WM_VSCROLL:
		{
			int	nScrollCode = (int) LOWORD(wParam); // scroll bar value 
			//int hwndScrollBar = (HWND) lParam;      // handle to scroll bar 

			int yInc;
      int yPos=gdi->GetOffsetY();

			switch(nScrollCode)
			{
				// User clicked shaft left of the scroll box. 
				case SB_PAGEUP: 
					 yInc = -80; 
					 break; 
 
				// User clicked shaft right of the scroll box. 
				case SB_PAGEDOWN: 
					 yInc = 80; 
					 break; 
 
				// User clicked the left arrow. 
				case SB_LINEUP: 
					 yInc = -10; 
					 break; 
 
				// User clicked the right arrow. 
				case SB_LINEDOWN: 
					 yInc = 10; 
					 break; 
 
				// User dragged the scroll box. 
				case SB_THUMBTRACK: 
          yInc = HIWORD(wParam) - yPos; 
          break; 
 
				default: 
				yInc = 0;  
			} 

      scrollVertical(gdi, yInc, hWnd);

 			break;
		}		

		case WM_HSCROLL:
		{
			int	nScrollCode = (int) LOWORD(wParam); // scroll bar value 
			//int hwndScrollBar = (HWND) lParam;      // handle to scroll bar 

			int xInc;
      int xPos=gdi->GetOffsetX();

			switch(nScrollCode)
			{
				// User clicked shaft left of the scroll box. 
				case SB_PAGEUP: 
					 xInc = -80; 
					 break; 
 
				// User clicked shaft right of the scroll box. 
				case SB_PAGEDOWN: 
					 xInc = 80; 
					 break; 
 
				// User clicked the left arrow. 
				case SB_LINEUP: 
					 xInc = -10; 
					 break; 
 
				// User clicked the right arrow. 
				case SB_LINEDOWN: 
					 xInc = 10; 
					 break; 
 
				// User dragged the scroll box. 
				case SB_THUMBTRACK: 
          xInc = HIWORD(wParam) - xPos; 
          break; 
 
				default: 
				  xInc = 0;  
			} 

			SCROLLINFO si;
			si.cbSize=sizeof(si);
			si.fMask=SIF_ALL;			
			GetScrollInfo(hWnd, SB_HORZ, &si);
		
      if (si.nPage==0)
        xInc = 0;

			int a=si.nMax-signed(si.nPage-1) - xPos;

			if ((xInc = max( -xPos, min(xInc, a)))!=0) { 
				xPos += xInc; 
				RECT ScrollArea, ClipArea;
        GetClientRect(hWnd, &ScrollArea);
				ClipArea=ScrollArea;

				gdi->SetOffsetX(xPos);				
			
				ScrollWindowEx (hWnd, -xInc,  0, 
					0, &ClipArea, 
					(HRGN) NULL, (LPRECT) NULL, SW_INVALIDATE|SW_SCROLLCHILDREN); 
        
				si.cbSize = sizeof(si); 
				si.fMask  = SIF_POS; 
				si.nPos   = xPos; 
			
				SetScrollInfo(hWnd, SB_HORZ, &si, TRUE); 				
			  UpdateWindow (hWnd);				 
			} 
 			break;
		}		

    case WM_MOUSEWHEEL: {
      int dz = GET_WHEEL_DELTA_WPARAM(wParam);
      scrollVertical(gdi, -dz, hWnd);
      }
      break;

		case WM_TIMER:
			/*if (wParam==1)	{
				gdi_main->removeString("AutoBUInfo");

				if (!gEvent->empty()) {
					RECT rc;
					GetClientRect(hWnd, &rc);
					gEvent->save();
					gdi_main->addInfoBox("", "Tävlingsdata har sparats.", 10);
				}
			}
			else if (wParam==2) {
				gdi_main->CheckInterfaceTimeouts(GetTickCount());
			}*/
      MessageBox(hWnd, "Runtime exception", 0, MB_OK);
			break;

    case WM_ACTIVATE: {
      int fActive = LOWORD(wParam); 
      if (fActive != WA_INACTIVE)
        currentFocusIx = ix;

      return DefWindowProc(hWnd, message, wParam, lParam);
    }

    case WM_USER + 2:
      if (gdi)
       LoadPage(*gdi, TabType(wParam));
      break;

		case WM_COMMAND:
			wmId    = LOWORD(wParam); 
			wmEvent = HIWORD(wParam); 
			// Parse the menu selections:
			switch (wmId)
			{
			case 0: break;
				default:
				   return DefWindowProc(hWnd, message, wParam, lParam);
			}
			break;

		case WM_PAINT:
			hdc = BeginPaint(hWnd, &ps);
			RECT rt;
			GetClientRect(hWnd, &rt);
			if(gdi)
				gdi->draw(hdc, rt);
			EndPaint(hWnd, &ps);
			break;

		case WM_ERASEBKGND:
			return 0;
			break;

    case WM_DESTROY:
      if (ix > 0) {
        gdi_extra[ix] = 0;
        delete gdi;

        while(!gdi_extra.empty() && gdi_extra.back() == 0)
          gdi_extra.pop_back();
      }
      break;

		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
   }
   return 0;
}


// Message handler for about box.
LRESULT CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG:
				return TRUE;

		case WM_COMMAND:
			if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) 
			{
				EndDialog(hDlg, LOWORD(wParam));
				return TRUE;
			}
			break;
	}
    return FALSE;
}

bool ExpandDirectory(const char *file, const char *filetype)
{
	WIN32_FIND_DATA fd;
	
	char dir[MAX_PATH];
	//char buff[MAX_PATH];
	char FullPath[MAX_PATH];
	
	strcpy_s(dir, MAX_PATH, file);

	if(dir[strlen(file)-1]!='\\')
		strcat_s(dir, MAX_PATH, "\\");
	
	strcpy_s(FullPath, MAX_PATH, dir);
	
	strcat_s(dir, MAX_PATH, filetype);
	
	HANDLE h=FindFirstFile(dir, &fd);
	
	if(h==INVALID_HANDLE_VALUE)
		return false;
	
	bool more=true;//=FindNextFile(h, &fd);
	
	while(more)
	{
		if(fd.cFileName[0]!='.') //Avoid .. and .
		{
			char FullPathFile[MAX_PATH];
			strcpy_s(FullPathFile, MAX_PATH, FullPath);
			strcat_s(FullPathFile, MAX_PATH, fd.cFileName);

		}
		more=FindNextFile(h, &fd)!=0;
	}
	
	FindClose(h);
	
	return true;
}

namespace setup {
const int nFiles=10;
const string fileList[nFiles]={"baseclass.xml",
                               "family.mwd", 
                               "given.mwd", 
                               "club.mwd", 
                               "class.mwd",
                               "database.clubs",
                               "database.persons",
                               "itest.meos",
                               "stest.meos",
                               "pctest.meos"};
}

void Setup(bool overwrite)
{
  static bool isSetup=false;
  if(isSetup && overwrite==false)
    return;
  isSetup=true; //Run at most once.

  char bf[260];
  for(int k=0;k<setup::nFiles;k++) {
    getUserFile(bf, (setup::fileList[k]).c_str());
    CopyFile(setup::fileList[k].c_str(), bf, !overwrite);
  }
}

void exportSetup()
{
  char bf[260];
  for(int k=0;k<setup::nFiles;k++) {
    getUserFile(bf, (setup::fileList[k]).c_str());
    CopyFile(bf, setup::fileList[k].c_str(), false);
  }
}


bool getUserFile(char *FileNamePath, const char *FileName) 
{
	char Path[MAX_PATH];
	char AppPath[MAX_PATH];

	if (SHGetSpecialFolderPath(hWndMain, Path, CSIDL_APPDATA, 1)!=NOERROR) {		
		int i=strlen(Path);
		if(Path[i-1]!='\\')
			strcat_s(Path, MAX_PATH, "\\");

		strcpy_s(AppPath, MAX_PATH, Path);
		strcat_s(AppPath, MAX_PATH, "Meos\\");

		CreateDirectory(AppPath, NULL);

    Setup(false);

		strcpy_s(FileNamePath, MAX_PATH, AppPath);
		strcat_s(FileNamePath, MAX_PATH, FileName);

		//return true;
	}
	else strcpy_s(FileNamePath, MAX_PATH, FileName);

	return true;
}


bool getDesktopFile(char *FileNamePath, const char *FileName) 
{
	char Path[MAX_PATH];
	char AppPath[MAX_PATH];

	if(SHGetSpecialFolderPath(hWndMain, Path, CSIDL_DESKTOPDIRECTORY, 1)!=NOERROR) {		
		int i=strlen(Path);
		if(Path[i-1]!='\\')
			strcat_s(Path, MAX_PATH, "\\");

		strcpy_s(AppPath, MAX_PATH, Path);
		strcat_s(AppPath, MAX_PATH, "Meos\\");

		CreateDirectory(AppPath, NULL);

		strcpy_s(FileNamePath, MAX_PATH, AppPath);
		strcat_s(FileNamePath, MAX_PATH, FileName);
	}
	else strcpy_s(FileNamePath, MAX_PATH, FileName);

	return true;
}

static set<string> tempFiles;
static string tempPath;

string getTempPath() {
  char tempFile[MAX_PATH];    
  if (tempPath.empty()) {
    char path[MAX_PATH];
    GetTempPath(MAX_PATH, path);
    GetTempFileName(path, "meos", 0, tempFile);
    DeleteFile(tempFile);
    if (CreateDirectory(tempFile, NULL)) 
      tempPath = tempFile;
    else
      throw std::exception("Failed to create temporary file.");
  }
  return tempPath;
}

void registerTempFile(const string &tempFile) {
  tempFiles.insert(tempFile);
}

string getTempFile() {
  getTempPath();

  char tempFile[MAX_PATH];    
  if (GetTempFileName(tempPath.c_str(), "ix", 0, tempFile)) {
    tempFiles.insert(tempFile);
    return tempFile;
  }
  else
    throw std::exception("Failed to create temporary file.");
}

void removeTempFile(const string &file) {
  DeleteFile(file.c_str());
  tempFiles.erase(file);
}

void removeTempFiles() {
  vector<string> dir;
  for (set<string>::iterator it = tempFiles.begin(); it!= tempFiles.end(); ++it) {
    char c = *it->rbegin();
    if (c == '/' || c=='\\')
      dir.push_back(*it);
    else
      DeleteFile(it->c_str());
  }
  tempFiles.clear();
  bool removed = true;
  while (removed) {
    removed = false;
    for (size_t k = 0; k<dir.size(); k++) {
      if (!dir[k].empty() && RemoveDirectory(dir[k].c_str()) != 0) {
        removed = true;
        dir[k].clear();
      }
    }
  }
  
  if (!tempPath.empty()) {
    RemoveDirectory(tempPath.c_str());
    tempPath.clear();
  }
}
