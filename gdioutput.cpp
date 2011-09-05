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

// gdioutput.cpp: implementation of the gdioutput class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "gdioutput.h"
#include "process.h"

#include "meos.h"

#include <commctrl.h>
#include <commdlg.h>
#include <shellapi.h>
#include <objbase.h>
#include <shlobj.h>
#include <cassert>
#include <cmath>

#include "meos_util.h"
#include "Table.h"

#include "Localizer.h"

#include "TabBase.h"
#include "toolbar.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

//Fulhack...
#ifndef IDC_HAND
	#define IDC_HAND MAKEINTRESOURCE(32649)
#endif

#ifndef MEOSDB  

gdioutput::gdioutput(double _scale)
{  
  commandLock = false;
  lockUpDown = false;
  tabs = 0;
  constructor(_scale);
}

gdioutput::gdioutput(double _scale, const PrinterObject &prndef) : po_default(prndef)
{
  commandLock = false;
  lockUpDown = false;
  tabs = 0;
  constructor(_scale);
}

void gdioutput::constructor(double _scale)
{
  Huge = 0;
	Large = 0;
	Medium = 0;
	Small = 0;
  Background = 0;
	pfLarge = 0;
	pfMedium = 0;
	pfMediumPlus = 0;
  pfSmall = 0;
  pfSmallItalic = 0;

  toolbar = 0;
  initCommon(_scale, "Arial");

  OffsetY=0;
	OffsetX=0;

  manualUpdate = false;

  itTL = TL.end();

	hWndTarget = 0;
	hWndToolTip = 0;
  hWndAppMain = 0;
	onClear = 0;
  postClear = 0;
  clearPage(true);
  CurrentFocus = 0;
  hasCleared = false;
}
#endif

void gdioutput::setFont(int size, const string &font)
{
  double s = 1+size*sqrt(double(size))*0.2;
  initCommon(s, font);
}

void gdioutput::setFontCtrl(HWND hWnd) {
  SendMessage(hWnd, WM_SETFONT, (WPARAM) getGUIFont(), MAKELPARAM(TRUE, 0));
}

static void scaleWindow(HWND hWnd, double scale, int &w, int &h) {
  RECT rc;
  GetWindowRect(hWnd, &rc);
  w = rc.right - rc.left;
  h = rc.bottom - rc.top;
  w = int(w * scale + 0.5);
  h = int(h * scale + 0.5);
}

int transformX(int x, double scale) {
  if (x<40)
    return int(x * scale + 0.5);
  else
    return int((x-40) * scale + 0.5) + 40;
}

void gdioutput::scaleSize(double scale_) {
  double ns = scale*scale_;

  if (ns + 1e-6 < 1.0 ) {
    ns = 1.0;
    scale_ = 1.0;
  }
  initCommon(ns, currentFont);

  for (list<TextInfo>::iterator it = TL.begin(); it!=TL.end(); ++it) {
    it->xlimit = int(it->xlimit * scale_ + 0.5);
    it->xp = transformX(it->xp, scale_);
    it->yp = int(it->yp * scale_ + 0.5);
  }
  int w, h;
  OffsetY = int (OffsetY * scale_ + 0.5);
  OffsetX = int (OffsetX * scale_ + 0.5);

  for (list<ButtonInfo>::iterator it = BI.begin(); it!=BI.end(); ++it) {
    if (it->fixedRightTop)
      it->xp = int(scale_ * it->xp + 0.5);
    else
      it->xp = transformX(it->xp, scale_);

    it->yp = int(it->yp * scale_ + 0.5);
    
    if (it->isCheckbox)
      scaleWindow(it->hWnd, 1.0, w, h);
    else
      scaleWindow(it->hWnd, scale_, w, h);
    setFontCtrl(it->hWnd);
    MoveWindow(it->hWnd, it->xp-OffsetX, it->yp-OffsetY, w, h, true);
  }

  for (list<InputInfo>::iterator it = II.begin(); it!=II.end(); ++it) {
    it->xp = transformX(it->xp, scale_);
    it->yp = int(it->yp * scale_ + 0.5);
    it->height *= scale_;
    it->width *= scale_;
    setFontCtrl(it->hWnd);
    MoveWindow(it->hWnd, it->xp-OffsetX, it->yp-OffsetY, int(it->width+0.5), int(it->height+0.5), true);
  }

  for (list<ListBoxInfo>::iterator it = LBI.begin(); it!=LBI.end(); ++it) {
    it->xp = transformX(it->xp, scale_);
    it->yp = int(it->yp * scale_ + 0.5);
    it->height *= scale_;
    it->width *= scale_;
    setFontCtrl(it->hWnd);
    MoveWindow(it->hWnd, it->xp-OffsetX, it->yp-OffsetY, int(it->width+0.5), int(it->height+0.5), true);
  }

  for (list<RectangleInfo>::iterator it = Rectangles.begin(); it!=Rectangles.end(); ++it) {
    it->rc.bottom = int(it->rc.bottom * scale_ + 0.5);
    it->rc.top = int(it->rc.top * scale_ + 0.5);
    it->rc.right = transformX(it->rc.right, scale_);
    it->rc.left = transformX(it->rc.left, scale_);    
  }

  for (list<TableInfo>::iterator it = Tables.begin(); it != Tables.end(); ++it) {
    it->xp = transformX(it->xp, scale_);
    it->yp = int(it->yp * scale_ + 0.5);
  }

  MaxX = transformX(MaxX, scale_);
  MaxY = int (MaxY * scale_ + 0.5);
  CurrentX = transformX(CurrentX, scale_);
  CurrentY = int (CurrentY * scale_ + 0.5);
  SX = transformX(SX, scale_);
  SY = int (SY * scale_ + 0.5);

  for (map<string, RestoreInfo>::iterator it = restorePoints.begin(); it != restorePoints.end(); ++it) {
    RestoreInfo &r = it->second;
    r.sMX = transformX(r.sMX, scale_);
    r.sMY = int (r.sMY * scale_ + 0.5);
    r.sCX = transformX(r.sCX, scale_);
    r.sCY = int (r.sCY * scale_ + 0.5);
    r.sOX = transformX(r.sOX, scale_);
    r.sOY = int (r.sOY * scale_ + 0.5);

  }
  refresh();
}

void gdioutput::initCommon(double _scale, const string &font)
{
  scale = _scale;
  currentFont = font;
  deleteFonts();
  enableTables();
	lineHeight = int(scale*14);

	Huge=CreateFont(int(scale*34), 0, 0, 0, FW_BOLD, false,  false, false, ANSI_CHARSET,
    OUT_TT_ONLY_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY, DEFAULT_PITCH|FF_ROMAN, font.c_str());

	Large=CreateFont(int(scale*24), 0, 0, 0, FW_BOLD, false,  false, false, ANSI_CHARSET,
		OUT_TT_ONLY_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY, DEFAULT_PITCH|FF_ROMAN, font.c_str());
	
	Medium=CreateFont(int(scale*14), 0, 0, 0, FW_BOLD, false,  false, false, ANSI_CHARSET,
		OUT_TT_ONLY_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY, DEFAULT_PITCH|FF_ROMAN, font.c_str());

	Small=CreateFont(int(scale*11), 0, 0, 0, FW_BOLD, false,  false, false, ANSI_CHARSET,
		OUT_TT_ONLY_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY, DEFAULT_PITCH|FF_ROMAN, font.c_str());

	pfLarge=CreateFont(int(scale*24), 0, 0, 0, FW_NORMAL, false,  false, false, ANSI_CHARSET,
		OUT_TT_ONLY_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY, DEFAULT_PITCH|FF_ROMAN, font.c_str());

	pfMedium=CreateFont(int(scale*14), 0, 0, 0, FW_NORMAL, false,  false, false, ANSI_CHARSET,
		OUT_TT_ONLY_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY, DEFAULT_PITCH|FF_ROMAN, font.c_str());

	pfMediumPlus=CreateFont(int(scale*18), 0, 0, 0, FW_NORMAL, false,  false, false, ANSI_CHARSET,
		OUT_TT_ONLY_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY, DEFAULT_PITCH|FF_ROMAN, font.c_str());

	pfSmall=CreateFont(int(scale*11), 0, 0, 0, FW_NORMAL, false,  false, false, ANSI_CHARSET,
		OUT_TT_ONLY_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY, DEFAULT_PITCH|FF_ROMAN, font.c_str());

	pfSmallItalic = CreateFont(int(scale*11), 0, 0, 0, FW_NORMAL, true,  false, false, ANSI_CHARSET,
		OUT_TT_ONLY_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY, DEFAULT_PITCH|FF_ROMAN, font.c_str());


	Background=CreateSolidBrush(GetSysColor(COLOR_WINDOW));
}

void gdioutput::deleteFonts()
{
  if (Huge)
	  DeleteObject(Huge);
  Huge = 0;

  if (Large)
	  DeleteObject(Large);
	Large = 0;

  if (Medium)
    DeleteObject(Medium);
	Medium = 0;

  if (Small)
    DeleteObject(Small);
	Small = 0;

  if (Background)
    DeleteObject(Background);
  Background = 0;

  if (pfLarge)
	  DeleteObject(pfLarge);
	pfLarge = 0;

  if (pfMedium)
    DeleteObject(pfMedium);
	pfMedium = 0;

  if (pfMediumPlus)
    DeleteObject(pfMediumPlus);
	pfMediumPlus = 0;

  if (pfSmall)
    DeleteObject(pfSmall);
  pfSmall = 0;

   if (pfSmallItalic)
    DeleteObject(pfSmallItalic);
  pfSmallItalic = 0;
}

#ifndef MEOSDB  

gdioutput::~gdioutput()
{
  deleteFonts();

  if (toolbar)
    delete toolbar;
  toolbar = 0;
	//delete table;
	while(!Tables.empty()){
    Tables.front().table->releaseOwnership();
		Tables.pop_front();
	}

  if (tabs) {
    delete tabs;
    tabs = 0;
  }
}
#endif


FixedTabs &gdioutput::getTabs() {
#ifndef MEOSDB  
  if (!tabs) 
    tabs = new FixedTabs();
#endif

  return *tabs;
}



void gdioutput::getPrinterSettings(PrinterObject &po) {
  po = po_default;
}


void gdioutput::drawBackground(HDC hDC, RECT &rc)
{
	GRADIENT_RECT gr[1];

	SelectObject(hDC, GetStockObject(NULL_PEN));
	SelectObject(hDC, Background);

	Rectangle(hDC, -1, -1, rc.right-OffsetX+1, 10-OffsetY+1);
	Rectangle(hDC, -1, -1, 11-OffsetX, rc.bottom+1);
	Rectangle(hDC, MaxX+10-OffsetX, 0, rc.right+1, rc.bottom+1);
	Rectangle(hDC, 10-OffsetX, MaxY+13-OffsetY, MaxX+11-OffsetX, rc.bottom+1);


	DWORD c=GetSysColor(COLOR_3DFACE);
	double red=GetRValue(c);
	double green=GetGValue(c);
	double blue=GetBValue(c);

  
  if (blue<100) {
    //Invert
    red = 255-red;
    green = 255-green;
    blue = 255-blue;
  }

	double blue1=min(255., blue*1.1);
	double green1=min(255., green*1.1);
	double red1=min(255., red*1.1);
  

	TRIVERTEX vert[2];
	vert [0] .x      = 10-OffsetX;
	vert [0] .y      = 10-OffsetY;
	vert [0] .Red    = 0xff00&DWORD(red1*256);
	vert [0] .Green  = 0xff00&DWORD(green1*256);
	vert [0] .Blue   = 0xff00&DWORD(blue1*256);
	vert [0] .Alpha  = 0x0000;

	vert [1] .x      = MaxX+10-OffsetX;
	vert [1] .y      = MaxY+13-OffsetY; 
	vert [1] .Red    = 0xff00&DWORD(red*256);
	vert [1] .Green  = 0xff00&DWORD(green*256);
	vert [1] .Blue   = 0xff00&DWORD(blue*256);
	vert [1] .Alpha  = 0x0000;

	gr[0].UpperLeft=0;
	gr[0].LowerRight=1;


	if(MaxY>600)
		GradientFill(hDC,vert, 2, gr, 1,GRADIENT_FILL_RECT_H);
	else
		GradientFill(hDC,vert, 2, gr, 1,GRADIENT_FILL_RECT_V);

	SelectObject(hDC, GetSysColorBrush(COLOR_3DSHADOW));

	Rectangle(hDC, vert[0].x+3, vert[1].y, vert[1].x+1, vert[1].y+3);
	Rectangle(hDC, vert[1].x, vert[0].y+3, vert[1].x+3, vert[1].y+3);

	SelectObject(hDC, GetStockObject(NULL_BRUSH));
	SelectObject(hDC, GetStockObject(BLACK_PEN));
	Rectangle(hDC, vert[0].x, vert[0].y, vert[1].x, vert[1].y);
}


void gdioutput::draw(HDC hDC, RECT &rc)
{ 
	drawBackground(hDC, rc);

	list<RectangleInfo>::iterator rit;
	SelectObject(hDC,GetStockObject(DC_BRUSH));

	for(rit=Rectangles.begin();rit!=Rectangles.end(); ++rit){
		if(rit->drawBorder)
			SelectObject(hDC, GetStockObject(BLACK_PEN));
		else
			SelectObject(hDC, GetStockObject(NULL_PEN));		
		SetDCBrushColor(hDC, rit->color);

		RECT rect_rc=rit->rc;
		OffsetRect(&rect_rc, -OffsetX, -OffsetY);
		Rectangle(hDC, rect_rc.left, rect_rc.top, rect_rc.right, rect_rc.bottom);
	}

  if (useTables)
    for(list<TableInfo>::iterator tit=Tables.begin();tit!=Tables.end(); ++tit){
		  tit->table->draw(*this, hDC, tit->xp, tit->yp, rc);		
	  }

  resetLast();
	TIList::iterator it;

	int BoundYup=OffsetY-100;
	int BoundYdown=OffsetY+rc.bottom+10;

  if(!renderOptimize || itTL == TL.end()) {
    for(it=TL.begin();it!=TL.end(); ++it){
		  TextInfo &ti=*it;
		  if( ti.yp > BoundYup && ti.yp < BoundYdown) 
			  RenderString(*it, hDC);
	  }
  }
  else {
    while( itTL != TL.end() && itTL->yp < BoundYup) 
      ++itTL;

    if (itTL!=TL.end())
      while( itTL != TL.begin() && itTL->yp > BoundYup) 
        --itTL;

    it=itTL;
    while( it != TL.end() && it->yp < BoundYdown) {
		  RenderString(*it, hDC);
      ++it;
	  }
  }

  updateStringPosCache();
	drawBoxes(hDC, rc);
}


void gdioutput::updateStringPosCache()
{
  RECT rc;
  GetClientRect(hWndTarget, &rc);
	int BoundYup = OffsetY-100;
	int BoundYdown = OffsetY+rc.bottom+10;
  shownStrings.clear();
  TIList::iterator it;


  if(!renderOptimize || itTL == TL.end()) {
    for (it=TL.begin();it!=TL.end(); ++it) {
		  TextInfo &ti=*it;
		  if( ti.yp > BoundYup && ti.yp < BoundYdown) 
			  shownStrings.push_back(&ti);
	  }
  }
  else {
    TIList::iterator itC = itTL;

    while( itC != TL.end() && itC->yp < BoundYup) 
      ++itC;

    if (itC!=TL.end())
      while( itC != TL.begin() && itC->yp > BoundYup) 
        --itC;

    it=itC;
    while( it != TL.end() && it->yp < BoundYdown) {
		  shownStrings.push_back(&*it);
      ++it;
	  }
  }
}

void gdioutput::addTimer(int yp, int xp, int format, DWORD ZeroTime, int xlimit, GUICALLBACK cb, int TimeOut)
{
	DWORD zt=GetTickCount()-1000*ZeroTime;
	string text=getTimerText(ZeroTime, format);

  addStringUT(yp, xp, format, text, xlimit, cb);
	TextInfo &ti=TL.back();
	ti.HasTimer=true;
	ti.ZeroTime=zt;

	if(TimeOut!=NOTIMEOUT)
		ti.TimeOut=ti.ZeroTime+(TimeOut)*1000;
}

void gdioutput::addTimeout(int TimeOut, GUICALLBACK cb)
{
	addStringUT(0, 0, 0, "", 0, cb);
	TextInfo &ti=TL.back();
	ti.HasTimer=true;
	ti.ZeroTime=GetTickCount();
	if(TimeOut!=NOTIMEOUT)
		ti.TimeOut=ti.ZeroTime+(TimeOut)*1000;
}

TextInfo &gdioutput::addStringUT(int yp, int xp, int format, const string &text, int xlimit, GUICALLBACK cb)
{
	TextInfo TI;
	TI.format=format;
	TI.xp=xp;
	TI.yp=yp;
	TI.text=text;
	TI.xlimit=xlimit;
	TI.CallBack=cb;
  
  if (format != pageNewPage && format != pageReserveHeight) {
	  HDC hDC=GetDC(hWndTarget);

    if (hWndTarget && !manualUpdate)
      RenderString(TI, hDC);    
    else 
      calcStringSize(TI, hDC);

    updatePos(TI.xp,TI.yp, TI.TextRect.right - TI.TextRect.left + scaleLength(10), 
                           TI.TextRect.bottom - TI.TextRect.top + scaleLength(2));

	  ReleaseDC(hWndTarget, hDC);
  	
    if(renderOptimize && !TL.empty()) {
      if(TL.back().yp > TI.yp) 
        renderOptimize=false;
    }
  }
  else {
    TI.TextRect.left = xp;
    TI.TextRect.right = xp;
    TI.TextRect.bottom = yp;
    TI.TextRect.top = yp;
  }

  TL.push_back(TI);
  itTL=TL.begin();

  return TL.back();
}

TextInfo &gdioutput::addString(const char *id, int yp, int xp, int format, const string &text, int xlimit, GUICALLBACK cb)
{
	TextInfo TI;
	TI.format=format;
	TI.xp=xp;
	TI.yp=yp;
	TI.text=lang.tl(text);
	TI.id=id;
	TI.xlimit=xlimit;
	TI.CallBack=cb;

  if (format != pageNewPage && format != pageReserveHeight) {
	  HDC hDC=GetDC(hWndTarget);

    if (hWndTarget && !manualUpdate)
      RenderString(TI, hDC);    
    else 
      calcStringSize(TI, hDC);

    updatePos(TI.xp, TI.yp, TI.TextRect.right - TI.TextRect.left + scaleLength(10), 
                            TI.TextRect.bottom - TI.TextRect.top + scaleLength(2));

	  ReleaseDC(hWndTarget, hDC);
  	
    if(renderOptimize && !TL.empty()) {
      if(TL.back().yp > TI.yp) 
        renderOptimize=false;
    }
  }
  else {
    TI.TextRect.left = xp;
    TI.TextRect.right = xp;
    TI.TextRect.bottom = yp;
    TI.TextRect.top = yp;
  }
  
  TL.push_back(TI);
  itTL=TL.begin();

  return TL.back();
}


TextInfo &gdioutput::addString(const char *id, int format, const string &text, GUICALLBACK cb)
{
  return addString(id, CurrentY, CurrentX, format, text, 0, cb);
}

TextInfo &gdioutput::addStringUT(int format, const string &text, GUICALLBACK cb)
{
  return addStringUT(CurrentY, CurrentX, format, text, 0, cb);
}


ButtonInfo &gdioutput::addButton(const string &id, const string &text, GUICALLBACK cb, 
                                 const string &tooltip)
{
	return addButton(CurrentX,  CurrentY, id, text, cb, tooltip);
}

ButtonInfo &gdioutput::addButton(int x, int y, const string &id, const string &text, GUICALLBACK cb, 
                                 const string &tooltip)
{
  SIZE size;

	HDC hDC=GetDC(hWndTarget);
  SelectObject(hDC, getGUIFont());
  string ttext = lang.tl(text);  
  GetTextExtentPoint32(hDC, ttext.c_str(), ttext.length(), &size);
	ReleaseDC(hWndTarget, hDC);

  ButtonInfo &bi=addButton(x, y, size.cx+30, id, text, cb, tooltip, false, false);

	return bi;
}

ButtonInfo &ButtonInfo::setDefault()
{
  flags|=1;
  //SetWindowLong(hWnd, i, GetWindowLong(hWnd, i)|BS_DEFPUSHBUTTON);
  return *this;
}


int gdioutput::getButtonHeight() const {
  return int(scale * 24)+0;
} 

ButtonInfo &gdioutput::addButton(int x, int y, int w, const string &id, 
                                 const string &text, GUICALLBACK cb, const string &tooltip,
                                 bool AbsPos, bool hasState)
{
  int style = hasState ? BS_CHECKBOX|BS_PUSHLIKE : BS_PUSHBUTTON;
  ButtonInfo bi;
  string ttext = lang.tl(text);
  int height = getButtonHeight();
	if(AbsPos){
	  bi.hWnd=CreateWindow("BUTTON", ttext.c_str(),  WS_TABSTOP|WS_VISIBLE|WS_CHILD|style|BS_NOTIFY, 
		  x-OffsetX, y, w, height, hWndTarget, NULL,  
		  (HINSTANCE)GetWindowLong(hWndTarget, GWL_HINSTANCE), NULL);
  }
  else {
	  bi.hWnd=CreateWindow("BUTTON", ttext.c_str(),  WS_TABSTOP|WS_VISIBLE|WS_CHILD|style|BS_NOTIFY, 
		  x-OffsetX, y-OffsetY-1, w,  height, hWndTarget, NULL,  
		  (HINSTANCE)GetWindowLong(hWndTarget, GWL_HINSTANCE), NULL);
  } 

  if (scale==1)
    SendMessage(bi.hWnd, WM_SETFONT, (WPARAM) GetStockObject(DEFAULT_GUI_FONT), 0);
  else
    SendMessage(bi.hWnd, WM_SETFONT, (WPARAM) pfMedium, 0);
  
	if(!AbsPos)
		updatePos(x, y, w+10, height+5);

	bi.xp=x;
	bi.yp=y;
	bi.text=ttext;
	bi.id=id;
	bi.CallBack=cb;
	bi.AbsPos=AbsPos;

	if(tooltip.length()>0)
		addToolTip(tooltip, bi.hWnd);

	BI.push_back(bi);
	FocusList.push_back(bi.hWnd);
	return BI.back();
}


ButtonInfo &gdioutput::addCheckbox(const string &id, const string &text,
                                   GUICALLBACK cb, bool Checked, const string &tooltip)
{
	return addCheckbox(CurrentX,  CurrentY,  id, text, cb, Checked, tooltip);
}

ButtonInfo &gdioutput::addCheckbox(int x, int y, const string &id, const string &text, 
                                   GUICALLBACK cb, bool Checked, const string &tooltip, bool AbsPos)
{
	ButtonInfo bi;
	SIZE size;

  string ttext = lang.tl(text);
	HDC hDC=GetDC(hWndTarget);
	SelectObject(hDC, GetStockObject(DEFAULT_GUI_FONT));
	GetTextExtentPoint32(hDC, "M", 1, &size);

	int ox=OffsetX;
	int oy=OffsetY;

	if (AbsPos) {
		ox=0;
		oy=0;
	}

  /*
	bi.hWnd=CreateWindowEx(0,"BUTTON", ttext.c_str(),  WS_TABSTOP|WS_VISIBLE|
          WS_CHILD|BS_AUTOCHECKBOX|BS_NOTIFY, 
		      x-ox, y-oy, size.cx+30, size.cy+5, hWndTarget, NULL,  
		      (HINSTANCE)GetWindowLong(hWndTarget, GWL_HINSTANCE), NULL);
  */
	int h = size.cy;
	SelectObject(hDC, getGUIFont());
  GetTextExtentPoint32(hDC, ttext.c_str(), ttext.length(), &size);
	ReleaseDC(hWndTarget, hDC);

  bi.hWnd=CreateWindowEx(0,"BUTTON", "",  WS_TABSTOP|WS_VISIBLE|
          WS_CHILD|BS_AUTOCHECKBOX|BS_NOTIFY, 
		      x-ox, y-oy + (size.cy-h)/2, h, h, hWndTarget, NULL,  
		      (HINSTANCE)GetWindowLong(hWndTarget, GWL_HINSTANCE), NULL);

  addStringUT(y - oy , x - ox + (3*h)/2, 0, ttext, 0);

	SendMessage(bi.hWnd, WM_SETFONT, (WPARAM) getGUIFont(), 0);

  if(Checked)
    SendMessage(bi.hWnd, BM_SETCHECK, BST_CHECKED, 0);

	if(!AbsPos)
		updatePos(x, y, size.cx+int(30*scale), size.cy+int(scale * 12)+3);

	if(tooltip.length()>0)
		addToolTip(tooltip, bi.hWnd);

  bi.isCheckbox = true;
	bi.xp=x;
	bi.yp=y;
	bi.text=ttext;
	bi.id=id;
	bi.CallBack=cb;
	bi.AbsPos=AbsPos;
	bi.originalState = Checked;
  bi.isEdit(true);
	BI.push_back(bi);
	FocusList.push_back(bi.hWnd);
	return BI.back();
}

bool gdioutput::isChecked(const string &id)
{
  list<ButtonInfo>::iterator it;
	for(it=BI.begin(); it != BI.end(); ++it)
	  if(it->id==id)
      return SendMessage(it->hWnd, BM_GETCHECK, 0, 0)==BST_CHECKED;
		
  return false;
}

void gdioutput::check(const string &id, bool State)
{
  list<ButtonInfo>::iterator it;
	for(it=BI.begin(); it != BI.end(); ++it)
    if(it->id==id){
      SendMessage(it->hWnd, BM_SETCHECK, State?BST_CHECKED:BST_UNCHECKED, 0);
      it->originalState = State;
      return;
    }
  return;
}

InputInfo &gdioutput::addInput(const string &id, const string &text, int length, GUICALLBACK cb, const string &Explanation, const string &Help)
{
	return addInput(CurrentX, CurrentY, id, text, length, cb, Explanation, Help);
}

HFONT gdioutput::getGUIFont() const 
{
  if (scale==1)
	  return (HFONT)GetStockObject(DEFAULT_GUI_FONT);
  else
    return pfMedium;
}

InputInfo &gdioutput::addInput(int x, int y, const string &id, const string &text, int length, GUICALLBACK cb, const string &Explanation, const string &Help)
{
	if (Explanation.length()>0) {
		addString("", y, x, 0, Explanation);
		y+=lineHeight;
	}

	InputInfo ii;
	SIZE size;

	HDC hDC=GetDC(hWndTarget);
  SelectObject(hDC, getGUIFont());  
  GetTextExtentPoint32(hDC, "M", 1, &size);
	ReleaseDC(hWndTarget, hDC);

	int ox=OffsetX;
	int oy=OffsetY;

	ii.hWnd=CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", text.c_str(),  
    WS_TABSTOP|WS_VISIBLE | WS_CHILD | ES_AUTOHSCROLL | WS_BORDER, 
		x-ox, y-oy, length*size.cx+scaleLength(8), size.cy+scaleLength(6), 
    hWndTarget, NULL, (HINSTANCE)GetWindowLong(hWndTarget, GWL_HINSTANCE), NULL);

	updatePos(x, y, length*size.cx+scaleLength(12), size.cy+scaleLength(10));

	SendMessage(ii.hWnd, WM_SETFONT, 
              (WPARAM) getGUIFont(), 0);

	ii.xp=x;
	ii.yp=y;
  ii.width = length*size.cx+scaleLength(8);
  ii.height = size.cy+scaleLength(6);
	ii.text = text;
  ii.original = text;
	ii.id=id;
	ii.CallBack=cb;

	II.push_back(ii);

  if (Help.length() > 0)
    addToolTip(Help, ii.hWnd);

	FocusList.push_back(ii.hWnd);
  return II.back();
}

InputInfo &gdioutput::addInputBox(const string &id, int width, int height, const string &text, 
                                  GUICALLBACK cb, const string &Explanation)
{
  return addInputBox(id, CurrentX, CurrentY, width, height, text, cb, Explanation);
}

InputInfo &gdioutput::addInputBox(const string &id, int x, int y, int width, int height,
                                  const string &text, GUICALLBACK cb, const string &Explanation)
{
	if (Explanation.length()>0) {
		addString("", y, x, 0, Explanation);
		y+=lineHeight;
	}

	InputInfo ii;

	int ox=OffsetX;
	int oy=OffsetY;

	ii.hWnd=CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", text.c_str(), WS_HSCROLL|WS_VSCROLL|
    WS_TABSTOP|WS_VISIBLE|WS_CHILD|ES_AUTOHSCROLL|ES_MULTILINE|ES_AUTOVSCROLL|WS_BORDER, 
		x-ox, y-oy, width, height, hWndTarget, NULL,  
		(HINSTANCE)GetWindowLong(hWndTarget, GWL_HINSTANCE), NULL);

	updatePos(x, y, width, height);

	SendMessage(ii.hWnd, WM_SETFONT, (WPARAM) getGUIFont(), 0);

	ii.xp=x;
	ii.yp=y;
  ii.width = width;
  ii.height = height;
	ii.text = text;
  ii.original = text;
  ii.id=id;
	ii.CallBack=cb;

	II.push_back(ii);
	FocusList.push_back(ii.hWnd);
  return II.back();
}


ListBoxInfo &gdioutput::addListBox(const string &id, int width, int height, GUICALLBACK cb, const string &Explanation, const string &Help, bool multiple)
{
	return addListBox(CurrentX, CurrentY, id, width, height, cb, Explanation, Help, multiple);
}

LRESULT CALLBACK GetMsgProc(HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam) {
  ListBoxInfo *lbi = (ListBoxInfo *)(GetWindowLongPtr(hWnd, GWL_USERDATA));
  if (!lbi) {
    throw std::exception("Internal GDI error");
  }

  LPARAM res = CallWindowProc(lbi->originalProc, hWnd, iMsg, wParam, lParam);
  if (iMsg == WM_VSCROLL || iMsg == WM_MOUSEWHEEL || iMsg == WM_KEYDOWN) {
    int topIndex = CallWindowProc(lbi->originalProc, hWnd, LB_GETTOPINDEX, 0, 0);
    if(lbi->lbiSync) {
      ListBoxInfo *other = lbi->lbiSync;
      CallWindowProc(other->originalProc, other->hWnd, LB_SETTOPINDEX, topIndex, 0);
    }    
  }
  return res;
}

void gdioutput::synchronizeListScroll(const string &id1, const string &id2) 
{
  ListBoxInfo *a = 0, *b = 0;
  list<ListBoxInfo>::iterator it;
	for (it = LBI.begin(); it != LBI.end(); ++it) {
    if (it->id == id1)
      a = &*it;
    else if (it->id == id2)
      b = &*it;
  }
  if (!a || !b)
    throw std::exception("Not found");

  a->lbiSync = b;
  b->lbiSync = a;
  SetWindowLongPtr(a->hWnd, GWL_USERDATA, LONG_PTR(a));
  SetWindowLongPtr(b->hWnd, GWL_USERDATA, LONG_PTR(b));

  a->originalProc = WNDPROC(GetWindowLongPtr(a->hWnd, GWL_WNDPROC));
  b->originalProc = WNDPROC(GetWindowLongPtr(b->hWnd, GWL_WNDPROC));

  SetWindowLongPtr(a->hWnd, GWL_WNDPROC, LONG_PTR(GetMsgProc));
  SetWindowLongPtr(b->hWnd, GWL_WNDPROC, LONG_PTR(GetMsgProc));
}

ListBoxInfo &gdioutput::addListBox(int x, int y, const string &id, int width, int height, GUICALLBACK cb, const string &Explanation, const string &Help, bool multiple)
{
	if (Explanation.length()>0) {
		addString("", y, x, 0, Explanation);
		y+=lineHeight;
	}
	ListBoxInfo lbi;
	int ox=OffsetX;
	int oy=OffsetY;

  DWORD style=WS_TABSTOP|WS_VISIBLE|WS_CHILD|WS_BORDER|LBS_USETABSTOPS|LBS_NOTIFY|WS_VSCROLL;

  if (multiple)
    style|=LBS_MULTIPLESEL;

	lbi.hWnd=CreateWindowEx(WS_EX_CLIENTEDGE, "LISTBOX", "",  style, 
		x-ox, y-oy, int(width*scale), int(height*scale), hWndTarget, NULL,  
		(HINSTANCE)GetWindowLong(hWndTarget, GWL_HINSTANCE), NULL);
/*
  if (id == "Punches") {
    winHandle = WNDPROC(GetWindowLong(lbi.hWnd, GWL_WNDPROC));
    SetWindowLong(lbi.hWnd, GWL_WNDPROC, LONG(GetMsgProc));
  }*/

	updatePos(x, y, int(scale*(width+5)), int(scale * (height+2)));
	SendMessage(lbi.hWnd, WM_SETFONT, (WPARAM) getGUIFont(), 0);

	lbi.IsCombo=false;
	lbi.xp=x;
	lbi.yp=y;
  lbi.width = scale*width;
  lbi.height = scale*height;
	lbi.id=id;
	lbi.CallBack=cb;
	LBI.push_back(lbi);

  if (Help.length() > 0)
    addToolTip(Help, lbi.hWnd);

	FocusList.push_back(lbi.hWnd);
  return LBI.back();
}

void gdioutput::setSelection(const string &id, const set<int> &selection)
{
  list<ListBoxInfo>::iterator it;
	for(it=LBI.begin(); it != LBI.end(); ++it){
		if (it->id==id && !it->IsCombo) {
      list<int>::const_iterator cit;

      if (selection.count(-1)==1)
        SendMessage(it->hWnd, LB_SETSEL, 1, -1);
      else {
        int count=SendMessage(it->hWnd, LB_GETCOUNT, 0,0);
        SendMessage(it->hWnd, LB_SETSEL, 0, -1);
        for(int i=0;i<count;i++){
          int d=SendMessage(it->hWnd, LB_GETITEMDATA, i, 0);
        
          if(selection.count(d)==1)
            SendMessage(it->hWnd, LB_SETSEL, 1, i);
        }
        return;
      }
		}
  }
}

void gdioutput::getSelection(const string &id, set<int> &selection)
{
  list<ListBoxInfo>::iterator it;
	for(it=LBI.begin(); it != LBI.end(); ++it){
		if (it->id==id && !it->IsCombo) {
      selection.clear();
      int count=SendMessage(it->hWnd, LB_GETCOUNT, 0,0);
      for(int i=0;i<count;i++){
        int s=SendMessage(it->hWnd, LB_GETSEL, i, 0);
        if (s) {
          int d=SendMessage(it->hWnd, LB_GETITEMDATA, i, 0);
          selection.insert(d);
        }
      }
      return;
		}
  }
} 

ListBoxInfo &gdioutput::addSelection(const string &id, int width, int height, GUICALLBACK cb, const string &Explanation, const string &Help)
{	
	return addSelection(CurrentX, CurrentY, id, width, height, cb, Explanation, Help);
}

ListBoxInfo &gdioutput::addSelection(int x, int y, const string &id, int width, int height, GUICALLBACK cb, const string &Explanation, const string &Help)
{
	if(Explanation.length()>0) {
		addString("", y, x, 0, Explanation);
		y+=lineHeight;
	}

	ListBoxInfo lbi;

	int ox = OffsetX;
	int oy = OffsetY;

	lbi.hWnd=CreateWindowEx(WS_EX_CLIENTEDGE, "COMBOBOX", "",  WS_TABSTOP|WS_VISIBLE | WS_CHILD |WS_BORDER|CBS_DROPDOWNLIST|WS_VSCROLL , 
		x-ox, y-oy, int(scale*width), int(scale*height), hWndTarget, NULL,  
		(HINSTANCE)GetWindowLong(hWndTarget, GWL_HINSTANCE), NULL);

	updatePos(x, y, int(scale*(width+5)), int(scale*30));

	SendMessage(lbi.hWnd, WM_SETFONT, (WPARAM) getGUIFont(), 0);
	
	lbi.IsCombo=true;
	lbi.xp=x;
	lbi.yp=y;
  lbi.width = scale*width;
  lbi.height = scale*height;
	lbi.id=id;
	lbi.CallBack=cb;

	LBI.push_back(lbi);

  if (Help.length() > 0)
    addToolTip(Help, lbi.hWnd);

	FocusList.push_back(lbi.hWnd);
  return LBI.back();
}



ListBoxInfo &gdioutput::addCombo(const string &id, int width, int height, GUICALLBACK cb, const string &Explanation, const string &Help)
{
	return addCombo(CurrentX, CurrentY, id, width, height, cb, Explanation, Help);
}

ListBoxInfo &gdioutput::addCombo(int x, int y, const string &id, int width, int height, GUICALLBACK cb, const string &Explanation, const string &Help)
{
	if(Explanation.length()>0) {
		addString("", y, x, 0, Explanation);
		y+=lineHeight;
	}

	ListBoxInfo lbi;
	int ox=OffsetX;
	int oy=OffsetY;

	lbi.hWnd=CreateWindowEx(WS_EX_CLIENTEDGE, "COMBOBOX", "",  WS_TABSTOP|WS_VISIBLE | WS_CHILD |WS_BORDER|CBS_DROPDOWN |CBS_AUTOHSCROLL|CBS_SORT, 
		x-ox, y-oy, int(scale*width), int(scale*height), hWndTarget, NULL,  
		(HINSTANCE)GetWindowLong(hWndTarget, GWL_HINSTANCE), NULL);

	updatePos(x, y, int(scale * (width+5)), getButtonHeight()+scaleLength(5));

	SendMessage(lbi.hWnd, WM_SETFONT, (WPARAM) getGUIFont(), 0);
	
	lbi.IsCombo=true;
	lbi.xp=x;
	lbi.yp=y;
  lbi.width = scale*width;
  lbi.height = scale*height;
	lbi.id=id;
	lbi.CallBack=cb;

	LBI.push_back(lbi);
  
  if (Help.length() > 0)
    addToolTip(Help, lbi.hWnd);

	FocusList.push_back(lbi.hWnd);
  return LBI.back();
}

bool gdioutput::addItem(const string &id, const string &text, size_t data)
{
	list<ListBoxInfo>::iterator it;
	for (it=LBI.begin(); it != LBI.end(); ++it) {
		if (it->id==id) {
			if(it->IsCombo) {
				int index=SendMessage(it->hWnd, CB_ADDSTRING, 0, LPARAM(text.c_str()));
				SendMessage(it->hWnd, CB_SETITEMDATA, index, data);
			}
			else {
				int index=SendMessage(it->hWnd, LB_INSERTSTRING, -1, LPARAM(text.c_str()));
				SendMessage(it->hWnd, LB_SETITEMDATA, index, data);
			}
			return true;
		}
	}
	return false;
}

bool gdioutput::addItem(const string &id, const vector< pair<string, size_t> > &items)
{
	list<ListBoxInfo>::iterator it;
	for (it=LBI.begin(); it != LBI.end(); ++it) {
		if (it->id==id) {
			if(it->IsCombo) {
        SendMessage(it->hWnd, CB_RESETCONTENT, 0, 0);
        for (size_t k = 0; k<items.size(); k++) {
				  int index=SendMessage(it->hWnd, CB_ADDSTRING, 0, LPARAM(items[k].first.c_str()));
          SendMessage(it->hWnd, CB_SETITEMDATA, index, items[k].second);
        }
			}
			else {
        SendMessage(it->hWnd, LB_RESETCONTENT, 0, 0);
        for (size_t k = 0; k<items.size(); k++) {
          int index=SendMessage(it->hWnd, LB_INSERTSTRING, -1, LPARAM(items[k].first.c_str()));
          SendMessage(it->hWnd, LB_SETITEMDATA, index, items[k].second);
        }
			}
			return true;
		}
	}
	return false;
}

bool gdioutput::clearList(const string &id)
{
	list<ListBoxInfo>::iterator it;
	for(it=LBI.begin(); it != LBI.end(); ++it){
		if (it->id==id) {
      it->original = "";
      it->originalIdx = -1;
			if(it->IsCombo)
				SendMessage(it->hWnd, CB_RESETCONTENT , 0, 0);			
			else
				SendMessage(it->hWnd, LB_RESETCONTENT , 0, 0);			
			return true;
		}
	}

	return false;
}

bool gdioutput::getSelectedItem(string id, ListBoxInfo *lbi)
{
  lbi->data=0;
  lbi->CallBack=0;
  lbi->setExtra(0);
  lbi->hWnd=0;
  lbi->id.clear();
  lbi->text.clear();
  lbi->xp=0;
  lbi->yp=0;
	list<ListBoxInfo>::iterator it;
	for (it=LBI.begin(); it != LBI.end(); ++it) {
		if (it->id==id) {
      bool ret = getSelectedItem(*it);
      *lbi=*it;
      return ret;
		}
	}
	return false;
}

bool gdioutput::getSelectedItem(ListBoxInfo &lbi) {
  if (lbi.IsCombo)	{
	  int index=SendMessage(lbi.hWnd, CB_GETCURSEL, 0, 0);

	  if (index == CB_ERR) {
		  char bf[256];
		  GetWindowText(lbi.hWnd, bf, 256);
		  lbi.text=bf;
		  lbi.data=-1;
      lbi.index=index;
		  return false;
	  }
	  lbi.data=SendMessage(lbi.hWnd, CB_GETITEMDATA, index, 0);
	  char bf[1024];
	  if(SendMessage(lbi.hWnd, CB_GETLBTEXT, index, LPARAM(bf)) != CB_ERR)
		  lbi.text=bf;
  }
  else {
	  int index=SendMessage(lbi.hWnd, LB_GETCURSEL, 0, 0);

	  if(index==LB_ERR)
		  return false;

	  lbi.data=SendMessage(lbi.hWnd, LB_GETITEMDATA, index, 0);
    lbi.index=index;
  	
	  char bf[1024];
	  if(SendMessage(lbi.hWnd, LB_GETTEXT, index, LPARAM(bf))!=LB_ERR)
		  lbi.text=bf;
  }
  return true;
}

bool gdioutput::selectItemByData(const char *id, int data)
{
	list<ListBoxInfo>::iterator it;
	for(it=LBI.begin(); it != LBI.end(); ++it){
		if (it->id==id) {
			if(it->IsCombo) {
				int m=0;
				int ret=0;
				
				if (data==-1) {
					SendMessage(it->hWnd, CB_SETCURSEL, -1, 0);					
					it->data = 0;
					it->text = "";
          it->original = "";
          it->originalIdx = -1;
					return true;
				}
				else while(ret!=CB_ERR) {
					ret = SendMessage(it->hWnd, CB_GETITEMDATA, m, 0);

					if (ret==data) {
						SendMessage(it->hWnd, CB_SETCURSEL, m, 0);
            it->data = data;
            it->originalIdx = data;
				    char bf[1024];
            if (SendMessage(it->hWnd, CB_GETLBTEXT, m, LPARAM(bf))!=CB_ERR) {
					    it->text = bf;
              it->original = bf;
            }
						return true;
					}
					else m++;
				}
        return false;
			}
			else {
				int m=0;
				int ret=0;
				
				if (data==-1) {
					SendMessage(it->hWnd, LB_SETCURSEL, -1, 0);					
					it->data=0;
					it->text = "";
          it->original = "";
          it->originalIdx = -1;
					return true;
				}
				else while (ret!=LB_ERR) {
					ret=SendMessage(it->hWnd, LB_GETITEMDATA, m, 0);

          if(ret==data) {
						SendMessage(it->hWnd, LB_SETCURSEL, m, 0);
			      it->data = data;
            it->originalIdx = data;
			      char bf[1024];
            if(SendMessage(it->hWnd, LB_GETTEXT, m, LPARAM(bf))!=LB_ERR) {
				      it->text = bf;
              it->original = bf;
            }
            return true;
          }
					else m++;
				}
        return false;
			}
		}
	}
	return false;
}

void gdioutput::removeSelected(const char *id)
{
}

LRESULT gdioutput::ProcessMsg(UINT iMessage, LPARAM lParam, WPARAM wParam)
{
  string msg;
  try {
    return ProcessMsgWrp(iMessage, lParam, wParam);
  }
  catch(std::exception &ex) {
    msg=ex.what();
  }
  catch(...) {
    msg="Ett ok�nt fel intr�ffade.";
  }

  if(!msg.empty()) {
    alert(msg);
    setWaitCursor(false);
  }
  return 0;
}

void gdioutput::processButtonMessage(ButtonInfo &bi, DWORD wParam)
{
  WORD hwParam = HIWORD(wParam);
  
  switch (hwParam) {
    case BN_CLICKED:
      if (bi.CallBack) {
        setWaitCursor(true);
        bi.CallBack(this, GUI_BUTTON, &bi); //it may be destroyed here...
				setWaitCursor(false);
      }
      break;
    case BN_SETFOCUS:
      CurrentFocus = bi.hWnd;
      break;
		
  }
}

void gdioutput::processEditMessage(InputInfo &bi, DWORD wParam)
{
  WORD hwParam = HIWORD(wParam);
  
  switch (hwParam) {
    case EN_CHANGE:
      getWindowText(bi.hWnd, bi.text);
		  if (bi.CallBack)
        bi.CallBack(this, GUI_INPUTCHANGE, &bi); //it may be destroyed here...
      break;

    case EN_KILLFOCUS:
      getWindowText(bi.hWnd, bi.text);
      if(bi.CallBack) 
        bi.CallBack(this, GUI_INPUT, &bi);
			break;

    case EN_SETFOCUS:
      CurrentFocus = bi.hWnd;
      getWindowText(bi.hWnd, bi.text);
      if(bi.CallBack) 
        bi.CallBack(this, GUI_FOCUS, &bi);
			break;
  }
}

void gdioutput::processComboMessage(ListBoxInfo &bi, DWORD wParam)
{
  WORD hwParam = HIWORD(wParam);
  int index; 
  switch (hwParam) {
    case CBN_SETFOCUS: 
		  CurrentFocus = bi.hWnd;
      lockUpDown = true;
			break;
    case CBN_KILLFOCUS:
      lockUpDown = false;
	    if (bi.CallBack) {
        char bf[1024];
        index=SendMessage(bi.hWnd, CB_GETCURSEL, 0, 0);
        
        if (index != CB_ERR) {					
          if (SendMessage(bi.hWnd, CB_GETLBTEXT, index, LPARAM(bf)) != CB_ERR)
            if (bi.text != bf) {  
				      bi.text = bf;
              bi.data=SendMessage(bi.hWnd, CB_GETITEMDATA, index, 0);
              bi.CallBack(this, GUI_COMBOCHANGE, &bi); //it may be destroyed here...
          
            }
        }
      }
			break;
    case CBN_SELCHANGE:
     	index=SendMessage(bi.hWnd, CB_GETCURSEL, 0, 0);

			if (index != CB_ERR) {					
        bi.data=SendMessage(bi.hWnd, CB_GETITEMDATA, index, 0);

				char bf[1024];
				if (SendMessage(bi.hWnd, CB_GETLBTEXT, index, LPARAM(bf)) != CB_ERR)
				  bi.text=bf;					

				if (bi.CallBack) {
          setWaitCursor(true);
          hasCleared = false;
          try {
            bi.writeLock = true;
					  bi.CallBack(this, GUI_LISTBOX, &bi); //it may be destroyed here... Then hasCleared is set.
          }
          catch(...) {
            if (!hasCleared)
              bi.writeLock = false;
            setWaitCursor(false);
            throw;
          }       
          if (!hasCleared)
            bi.writeLock = false;
          setWaitCursor(false);
      	}
      }
			break;
  }
}

#ifndef MEOSDB

void gdioutput::keyCommand(KeyCommandCode code) {
  if (commandLock)
    return;
  string msg;
  try {
    list<TableInfo>::iterator tit;
    if (useTables) {
      for (tit=Tables.begin(); tit!=Tables.end(); ++tit)
        if (tit->table->keyCommand(*this, code))
          return;
    };
  }
  catch(std::exception &ex) {
    msg = ex.what();
  }
  catch(...) {
    msg = "Ett ok�nt fel intr�ffade.";
  }
  
  if(!msg.empty())
    alert(msg);
}

#endif

void gdioutput::processListMessage(ListBoxInfo &bi, DWORD wParam)
{
  WORD hwParam = HIWORD(wParam);
  int index;

  switch (hwParam) {
    case LBN_SETFOCUS: 
		  CurrentFocus = bi.hWnd;
      lockUpDown = true;
			break;
    case LBN_KILLFOCUS:
      lockUpDown = false;
      break;
    case LBN_SELCHANGE:
		  index=SendMessage(bi.hWnd, LB_GETCURSEL, 0, 0);

      if (index!=LB_ERR) {					
			  bi.data = SendMessage(bi.hWnd, LB_GETITEMDATA, index, 0);

				char bf[1024];
				if(SendMessage(bi.hWnd, LB_GETTEXT, index, LPARAM(bf)) != LB_ERR)
				  bi.text = bf;					

        if (bi.CallBack) {
          setWaitCursor(true);
          bi.CallBack(this, GUI_LISTBOX, &bi); //it may be destroyed here...
          setWaitCursor(false);
        }         
      }
      break;
  }
}


LRESULT gdioutput::ProcessMsgWrp(UINT iMessage, LPARAM lParam, WPARAM wParam)
{
	if (iMessage==WM_COMMAND) {
    WORD hwParam = HIWORD(wParam);
    HWND hWnd=(HWND)lParam;
    if (hwParam==EN_CHANGE) {
		  list<TableInfo>::iterator tit;
      if (useTables)
        for (tit=Tables.begin(); tit!=Tables.end(); ++tit)
          if(tit->table->inputChange(*this, hWnd))
            return 0;
		}

    {
      list<ButtonInfo>::iterator it;
  	  for (it=BI.begin(); it != BI.end(); ++it) {
		    if (it->hWnd==hWnd) {
				  processButtonMessage(*it, wParam);
          return 0;
			  }
		  }
    }

    {
		  list<InputInfo>::iterator it;
		  for (it=II.begin(); it != II.end(); ++it) {				
			  if(it->hWnd==hWnd) {
     		  processEditMessage(*it, wParam);
    		  return 0;
			  }
		  }
    }

    {
      list<ListBoxInfo>::iterator it;
			for(it=LBI.begin(); it != LBI.end(); ++it) {				
				if(it->hWnd==hWnd) {
          if (it->IsCombo)
            processComboMessage(*it, wParam);
          else
            processListMessage(*it, wParam);
					return 0;
				}
			}
    }
	}
	else if (iMessage==WM_MOUSEMOVE) {
 		POINT pt;
		pt.x=(signed short)LOWORD(lParam);
		pt.y=(signed short)HIWORD(lParam);

    list<TableInfo>::iterator tit;

    bool GotCapture=false;

    if (useTables)
      for (tit=Tables.begin(); tit!=Tables.end(); ++tit)
        GotCapture = tit->table->mouseMove(*this, pt.x, pt.y) || GotCapture;

    if(GotCapture)
      return 0;

		list<InfoBox>::iterator it=IBox.begin();

		
		while (it != IBox.end()) {
			if (PtInRect(&it->TextRect, pt) && it->CallBack) {
				SetCursor(LoadCursor(NULL, IDC_HAND));

				HDC hDC=GetDC(hWndTarget);
				drawBoxText(hDC, it->TextRect, *it, true);
				ReleaseDC(hWndTarget, hDC);
				SetCapture(hWndTarget);
				GotCapture=true;
				it->HasTCapture=true;
			}
			else {
				if (it->HasTCapture) {
					HDC hDC=GetDC(hWndTarget);
					drawBoxText(hDC, it->TextRect, *it, false);
					ReleaseDC(hWndTarget, hDC);
					if(!GotCapture) 	ReleaseCapture();
					it->HasTCapture=false;
				}
			}
			
			if(it->HasCapture) {
				if (GetCapture()!=hWndTarget) {
					HDC hDC=GetDC(hWndTarget);
					drawCloseBox(hDC, it->Close, false);
					ReleaseDC(hWndTarget, hDC);
					if(!GotCapture) ReleaseCapture();
					it->HasCapture=false;
				}
				else if(!PtInRect(&it->Close, pt)) {
					HDC hDC=GetDC(hWndTarget);
					drawCloseBox(hDC, it->Close, false);
					ReleaseDC(hWndTarget, hDC);
				}		
				else {
					HDC hDC=GetDC(hWndTarget);
					drawCloseBox(hDC, it->Close, true);
					ReleaseDC(hWndTarget, hDC);
				}
			}
			++it;
		}

    for (size_t k=0;k<shownStrings.size();k++) {
      TextInfo &ti = *shownStrings[k];
      if (!ti.CallBack)
        continue;

      if (PtInRect(&ti.TextRect, pt)) {
				if(!ti.Highlight){
					ti.Highlight=true;
          InvalidateRect(hWndTarget, &ti.TextRect, true);
				}

				SetCapture(hWndTarget);
				GotCapture=true;
				ti.HasCapture=true;
        SetCursor(LoadCursor(NULL, IDC_HAND));
			}
			else {
				if (ti.Highlight) {
					ti.Highlight=false;
          InvalidateRect(hWndTarget, &ti.TextRect, true);
				}

				if (ti.HasCapture) {						
					if (!GotCapture)
						ReleaseCapture();

					ti.HasCapture=false;
				}
			}
    }

		//Handle linked text:
/*	{
			list<TextInfo>::iterator it=TL.begin();

			while(it!=TL.end()) {
	
				if(!it->CallBack) {
					++it;
					continue;
				}
				
				if(PtInRect(&it->TextRect, pt)){
					//Sleep(1000);
					if(!it->Highlight){
						it->Highlight=true;
						//RenderString(*it);
            InvalidateRect(hWndTarget, &it->TextRect, true);
					}

					SetCapture(hWndTarget);
					GotCapture=true;
					it->HasCapture=true;
          SetCursor(LoadCursor(NULL, IDC_HAND));
				}
				else{
					if(it->Highlight){
						it->Highlight=false;
						//RenderString(*it)
            InvalidateRect(hWndTarget, &it->TextRect, true);
					}

					if(it->HasCapture){						
						if(!GotCapture)
							ReleaseCapture();
						it->HasCapture=false;
					}
				}
				++it;
			}
		}*/
	}
	else if(iMessage==WM_LBUTTONDOWN) {
		list<InfoBox>::iterator it=IBox.begin();

		POINT pt;
		pt.x=(signed short)LOWORD(lParam);
		pt.y=(signed short)HIWORD(lParam);

    list<TableInfo>::iterator tit;

    if (useTables)
      for (tit=Tables.begin(); tit!=Tables.end(); ++tit)
        if(tit->table->mouseLeftDown(*this, pt.x, pt.y))
          return 0;

		while(it!=IBox.end()) {
			if(PtInRect(&it->Close, pt)) {				
				HDC hDC=GetDC(hWndTarget);
				drawCloseBox(hDC, it->Close, true);
				ReleaseDC(hWndTarget, hDC);
				SetCapture(hWndTarget);
				it->HasCapture=true;
			}
			++it;
		}


		//Handle links
    for (size_t k=0;k<shownStrings.size();k++) {
      TextInfo &ti = *shownStrings[k];
      if (!ti.CallBack)
        continue;

      if (ti.HasCapture) {
			  HDC hDC=GetDC(hWndTarget);
				if (PtInRect(&ti.TextRect, pt)) {					
				  ti.Active=true;
					RenderString(ti, hDC);
				}
				ReleaseDC(hWndTarget, hDC);
			}		
		}
		/*	
		{
			list<TextInfo>::iterator it=TL.begin();

			while (it!=TL.end()) {

				if(!it->CallBack) {
					++it;
					continue;
				}

				if (it->HasCapture) {
					HDC hDC=GetDC(hWndTarget);
					if(PtInRect(&it->TextRect, pt)){					
						it->Active=true;
						RenderString(*it, hDC);
						return;
					}
					ReleaseDC(hWndTarget, hDC);
				}		
				++it;
			}
		}
    */
    
	}
	else if (iMessage==WM_LBUTTONUP) {
    list<TableInfo>::iterator tit;
    
    list<InfoBox>::iterator it=IBox.begin();

		POINT pt;
		pt.x=(signed short)LOWORD(lParam);
		pt.y=(signed short)HIWORD(lParam);

    if (useTables)
      for (tit=Tables.begin(); tit!=Tables.end(); ++tit)
        if (tit->table->mouseLeftUp(*this, pt.x, pt.y))
          return 0;

		while (it!=IBox.end()) {
			if (it->HasCapture) {
				HDC hDC=GetDC(hWndTarget);
				drawCloseBox(hDC, it->Close, false);
				ReleaseDC(hWndTarget, hDC);
				ReleaseCapture();
				it->HasCapture=false;

				if (PtInRect(&it->Close, pt)) {					
					IBox.erase(it);
					refresh();
					return 0;
				}
			}
			else if (it->HasTCapture) {
				ReleaseCapture();
				it->HasTCapture=false;

				if (PtInRect(&it->TextRect, pt)) {
					if(it->CallBack)
						it->CallBack(this, GUI_INFOBOX, &*it); //it may be destroyed here...
					return 0;
				}
			}
			++it;
		}

		//Handle links
    for (size_t k=0;k<shownStrings.size();k++) {
      TextInfo &ti = *shownStrings[k];
      if (!ti.CallBack)
        continue;
			
      if(ti.HasCapture){		
			  ReleaseCapture();
				ti.HasCapture = false;

				if (PtInRect(&ti.TextRect, pt)) {					
					if (ti.Active) {
						ti.Active=false;							
						RenderString(ti);
						ti.CallBack(this, GUI_LINK, &ti);
						return 0;
					}
				}
			}		
			else if(ti.Active){
				ti.Active=false;
				RenderString(ti);
			}
    }
		/*
    {
			list<TextInfo>::iterator it=TL.begin();

			while(it!=TL.end()){
				
				if(!it->CallBack)
				{
					++it;
					continue;
				}

				if(it->HasCapture){		
					ReleaseCapture();
					it->HasCapture=false;

					if(PtInRect(&it->TextRect, pt)){			
						
						if(it->Active)
						{
							it->Active=false;							
							RenderString(*it);
						//	MessageBox(hWndTarget, "ACTION", 0, MB_OK);
							it->CallBack(this, GUI_LINK, &*it);
							return;
						}
					}
				}		
				else if(it->Active){
					it->Active=false;
					RenderString(*it);
				}
				++it;
			}
		}*/
	}
  else if (iMessage==WM_LBUTTONDBLCLK) {
    list<TableInfo>::iterator tit;
		POINT pt;
		pt.x=(signed short)LOWORD(lParam);
		pt.y=(signed short)HIWORD(lParam);

    if (useTables)
      for (tit=Tables.begin(); tit!=Tables.end(); ++tit)
        if (tit->table->mouseLeftDblClick(*this, pt.x, pt.y))
          return 0;

  }
  else if (iMessage == WM_CHAR) {
    /*list<TableInfo>::iterator tit;
    if (useTables)
      for (tit=Tables.begin(); tit!=Tables.end(); ++tit)
        if (tit->table->character(*this, int(wParam), lParam & 0xFFFF))
          return 0;*/
  }
  else if (iMessage == WM_CTLCOLOREDIT) {
    for (list<InputInfo>::const_iterator it = II.begin(); it != II.end(); ++it) {
      if (it->hWnd == HWND(lParam)) {
        if (it->bgColor != colorDefault) {
          SetDCBrushColor(HDC(wParam), it->bgColor);
          SetBkColor(HDC(wParam), it->bgColor);
          return LRESULT(GetStockObject(DC_BRUSH));
        }
      }
    }
    return 0;
  }

  return 0;
}

void gdioutput::TabFocus(int direction)
{
  list<TableInfo>::iterator tit;

  if (useTables)
    for (tit=Tables.begin(); tit!=Tables.end(); ++tit)
      if (tit->table->tabFocus(*this, direction))
        return;

	if(FocusList.empty())
		return;
		
	list<HWND>::iterator it=FocusList.begin();

	while(it!=FocusList.end() && *it!=CurrentFocus)
		++it;

	//if(*it==CurrentFocus)
	if(it!=FocusList.end())	{
		if(direction==1){
			++it;
			if(it==FocusList.end()) it=FocusList.begin();
			while(!IsWindowEnabled(*it) && *it!=CurrentFocus){
				++it;
				if(it==FocusList.end()) it=FocusList.begin();
			}
		}
		else{
			if(it==FocusList.begin()) it=FocusList.end();		
			
			it--;				
			while(!IsWindowEnabled(*it) && *it!=CurrentFocus){
				if(it==FocusList.begin()) it=FocusList.end();		
				it--;
			}

		}

		CurrentFocus=*it;
		SetFocus(*it);
	}
	else{
		CurrentFocus=*FocusList.begin();
		SetFocus(CurrentFocus);
	}
}

bool gdioutput::isInputChanged(const string &exclude)
{
  for(list<InputInfo>::iterator it=II.begin(); it != II.end(); ++it) {
    if(it->id!=exclude) {
      if (it->changed()  && !it->ignoreCheck)
        return true;
    }
  }

  for (list<ListBoxInfo>::iterator it = LBI.begin(); it != LBI.end(); ++it) {
    getSelectedItem(*it);
    if (it->changed() && !it->ignoreCheck)
      return true;
  }

  for (list<ButtonInfo>::iterator it = BI.begin(); it != BI.end(); ++it) {
    bool checked = SendMessage(it->hWnd, BM_GETCHECK, 0, 0)==BST_CHECKED;
    if (it->originalState != checked)
      return true;
  }

  return false;
}

BaseInfo *gdioutput::setInputFocus(const string &id, bool select)
{
	for(list<InputInfo>::iterator it=II.begin(); it != II.end(); ++it)
    if(it->id==id) {
      scrollTo(it->xp, it->yp);
      BaseInfo *bi = SetFocus(it->hWnd)!=NULL ? &*it: 0;
      if (bi) {
        if (select)
          PostMessage(it->hWnd, EM_SETSEL, it->text.length(), 0);
      }
      return bi;
    }
  
  for(list<ListBoxInfo>::iterator it=LBI.begin(); it!=LBI.end();++it) 
    if(it->id==id) {
      scrollTo(it->xp, it->yp);
      return SetFocus(it->hWnd)!=NULL ? &*it: 0;
  }

  for(list<ButtonInfo>::iterator it=BI.begin(); it!=BI.end();++it) 
    if(it->id==id) {
      scrollTo(it->xp, it->yp);
      return SetFocus(it->hWnd)!=NULL ? &*it: 0;
  }

	return 0;
}

InputInfo *gdioutput::getInputFocus()
{
  HWND hF=GetFocus();

  if (hF) {
	  list<InputInfo>::iterator it;

	  for(it=II.begin(); it != II.end(); ++it)
      if(it->hWnd==hF)	
			  return &*it;
  }
	return 0;
}

void gdioutput::Enter()
{
  string msg;
  try {
    doEnter();
  }
  catch(std::exception &ex) {
    msg = ex.what();
  }
  catch(...) {
    msg = "Ett ok�nt fel intr�ffade.";
  }
  
  if(!msg.empty())
    alert(msg);
}

void gdioutput::doEnter()
{
  list<TableInfo>::iterator tit;

  if (useTables)
    for (tit=Tables.begin(); tit!=Tables.end(); ++tit)
      if (tit->table->enter(*this))
        return;

	HWND hWnd=GetFocus();

  for (list<ButtonInfo>::iterator it=BI.begin(); it!=BI.end(); ++it)
    if (it->defaultButton() && it->CallBack) {
      it->CallBack(this, GUI_BUTTON, &*it);
      return;
    }

	list<InputInfo>::iterator it;

	for(it=II.begin(); it != II.end(); ++it)
		if(it->hWnd==hWnd && it->CallBack){
			char bf[1024];
			GetWindowText(hWnd, bf, 1024);
			it->text=bf;
			it->CallBack(this, GUI_INPUT, &*it);
			return;
		}			
}

bool gdioutput::UpDown(int direction)
{
  string msg;
  try {
    return doUpDown(direction);
  }
  catch(std::exception &ex) {
    msg = ex.what();
  }
  catch(...) {
    msg = "Ett ok�nt fel intr�ffade.";
  }

  if(!msg.empty())
    alert(msg);
  return false;
}


bool gdioutput::doUpDown(int direction)
{
  list<TableInfo>::iterator tit;

  if (useTables)
    for (tit=Tables.begin(); tit!=Tables.end(); ++tit)
      if (tit->table->UpDown(*this, direction))
        return true;

  return false;
}

void gdioutput::Escape()
{
  string msg;
  try {
    doEscape();
  }
  catch(std::exception &ex) {
    msg = ex.what();
  }
  catch(...) {
    msg = "Ett ok�nt fel intr�ffade.";
  }

  if(!msg.empty())
    alert(msg);
}


void gdioutput::doEscape()
{
  list<TableInfo>::iterator tit;

  if (useTables)
    for (tit=Tables.begin(); tit!=Tables.end(); ++tit)
      tit->table->escape(*this);

  for (list<ButtonInfo>::iterator it=BI.begin(); it!=BI.end(); ++it)
    if (it->cancelButton() && it->CallBack) {
      it->CallBack(this, GUI_BUTTON, &*it);
      return;
    }
}

void gdioutput::clearPage(bool autoRefresh, bool keepToolbar)
{
  lockUpDown = false;
  enableTables();
  #ifndef MEOSDB
    if (toolbar && !keepToolbar)
      toolbar->hide();
  #endif
  restorePoints.clear();
  shownStrings.clear();
	onClear=0;
	FocusList.clear();
  CurrentFocus = 0;
	TL.clear();
  itTL=TL.end();

	if(hWndTarget && autoRefresh)
		InvalidateRect(hWndTarget, NULL, true);

	fillDown();
  
  hasCleared = true;

  for (ToolList::iterator it = toolTips.begin(); it != toolTips.end(); ++it) {
    if (hWndToolTip) {
      SendMessage(hWndToolTip, TTM_DELTOOL, 0, (LPARAM) &it->ti);
    }
  }
  toolTips.clear();

	{
		list<ButtonInfo>::iterator it;
		for(it=BI.begin(); it != BI.end(); ++it)
			DestroyWindow(it->hWnd);
		BI.clear();
	}
	{	
		list<InputInfo>::iterator it;
		for(it=II.begin(); it != II.end(); ++it)
			DestroyWindow(it->hWnd);
		II.clear();
	}

	{
		list<ListBoxInfo>::iterator it;
    for(it=LBI.begin(); it != LBI.end(); ++it) {
			DestroyWindow(it->hWnd);
      if (it->writeLock)
        hasCleared = true;
    }
		LBI.clear();
	}

	//delete table;
	//table=0;	
	while(!Tables.empty()){
    Table *t=Tables.front().table;
    Tables.front().table=0;		
		Tables.pop_front();
    t->hide(*this);
    t->releaseOwnership();
	}

	DataInfo.clear();
	FocusList.clear();
	Events.clear();

	Rectangles.clear();

	MaxX=40;
	MaxY=100;

	CurrentX=40;
	CurrentY=START_YP;

	OffsetX=0;
	OffsetY=0;
	
  renderOptimize=true;

	setRestorePoint();

  if(autoRefresh)
	  updateScrollbars();

  try {
    if (postClear)
	    postClear(this, GUI_POSTCLEAR, 0);
  }
  catch(const std::exception &ex) {            
    string msg(ex.what());  
    alert(msg);
  }
  postClear = 0;

  manualUpdate=!autoRefresh;
}

void gdioutput::getWindowText(HWND hWnd, string &text)
{
  char bf[1024];
  char *bptr=bf;
  
  int len=GetWindowTextLength(hWnd);

  if(len>1023)
    bptr=new char[len+1];

  GetWindowText(hWnd, bptr, len+1);
  text=bptr;

  if(len>1023)
    delete[] bptr;
}

const string &gdioutput::getText(const char *id, bool acceptMissing) const
{
	char bf[1024];
  char *bptr=bf;

	for(list<InputInfo>::const_iterator it=II.begin(); 
                                  it != II.end(); ++it){
		if(it->id==id){
      int len=GetWindowTextLength(it->hWnd);
      
      if(len>1023)
        bptr=new char[len+1];
			
      GetWindowText(it->hWnd, bptr, len+1);
			const_cast<string&>(it->text)=bptr;

      if(len>1023)
        delete[] bptr;
			
			return it->text;
		}
	}

	for(list<ListBoxInfo>::const_iterator it=LBI.begin(); 
                                  it != LBI.end(); ++it){
		if(it->id==id && it->IsCombo){
      if (!it->writeLock) {
			  GetWindowText(it->hWnd, bf, 1024);
			  const_cast<string&>(it->text)=bf;
      }
      return it->text;
		}
	}

	for(list<TextInfo>::const_iterator it=TL.begin(); 
                                  it != TL.end(); ++it){
		if (it->id==id) {      
      return it->text;
		}
	}

#ifdef _DEBUG
  if (!acceptMissing) {
    string err = string("Internal Error, identifier not found: X#") + id;
    throw std::exception(err.c_str());
  }
#endif
	return _EmptyString;
}

bool gdioutput::hasField(const string &id) const
{
	for(list<InputInfo>::const_iterator it=II.begin(); 
                                  it != II.end(); ++it){
		if(it->id==id)
      return true;
	}

	for(list<ListBoxInfo>::const_iterator it=LBI.begin(); 
                                  it != LBI.end(); ++it){
		if(it->id==id)
      return true;
	}

  for(list<ButtonInfo>::const_iterator it=BI.begin(); 
                                  it != BI.end(); ++it){
		if(it->id==id)
      return true;
	}

	return false;
}

int gdioutput::getTextNo(const char *id, bool acceptMissing) const
{
  const string &t = getText(id, acceptMissing);
	return atoi(t.c_str());
}

BaseInfo *gdioutput::setText(const char *id, int number, bool Update)
{
	char bf[16];
	sprintf_s(bf, 16, "%d", number);
	return setText(id, bf, Update);	
}

BaseInfo *gdioutput::setTextZeroBlank(const char *id, int number, bool Update)
{
  if (number!=0)	
	  return setText(id, number, Update);	
  else
    return setText(id, "", Update);
}

BaseInfo *gdioutput::setText(const char *id, const string &text, bool Update)
{	
	for (list<InputInfo>::iterator it=II.begin(); 
                         it != II.end(); ++it) {
		if (it->id==id) {
			SetWindowText(it->hWnd, text.c_str());
			it->text = text;
      it->original = text;
			return &*it;
		}
	}

	for (list<ListBoxInfo>::iterator it=LBI.begin();
                        it != LBI.end(); ++it) {
		if (it->id==id && it->IsCombo) {
			SetWindowText(it->hWnd, text.c_str());
			it->text = text;
      it->original = text;
			return &*it;
		}
	}
	
	for (list<ButtonInfo>::iterator it=BI.begin(); 
                        it != BI.end(); ++it) {
		if (it->id==id) {
			SetWindowText(it->hWnd, text.c_str());
			it->text=text;			
			return &*it;
		}
	}
	
	for(list<TextInfo>::iterator it=TL.begin(); 
      it != TL.end(); ++it){
		if (it->id==id) {				
			if (Update) {					
				RECT rc=it->TextRect;

				it->text=text;
				calcStringSize(*it);

				rc.right=max(it->TextRect.right, rc.right);
				rc.bottom=max(it->TextRect.bottom, rc.bottom);

				if(hWndTarget)
					InvalidateRect(hWndTarget, &rc, true);
			}
			else it->text=text;	

			return &*it;
		}
	}
	return 0;
}

bool gdioutput::insertText(const string &id, const string &text)
{	
	for (list<InputInfo>::iterator it=II.begin(); 
                         it != II.end(); ++it) {
		if (it->id==id) {
			SetWindowText(it->hWnd, text.c_str());
			it->text = text;
      
      if(it->CallBack) 
        it->CallBack(this, GUI_INPUT, &*it);
			
			return true;
		}
	}
  return false;
}

void gdioutput::setData(const string &id, DWORD data)
{
	list<DataStore>::iterator it;
	for(it=DataInfo.begin(); it != DataInfo.end(); ++it){
		if(it->id==id){
			it->data=data;
			return;
		}
	}

	DataStore ds;
	ds.id=id;
	ds.data=data;

	DataInfo.push_front(ds);	
	return;
}

bool gdioutput::getData(const string &id, DWORD &data)
{
	list<DataStore>::iterator it;
	for(it=DataInfo.begin(); it != DataInfo.end(); ++it){
		if(it->id==id){
			data=it->data;
			return true;
		}
	}

	data=0;
	return false;
}

void gdioutput::updatePos(int x, int y, int width, int height)
{
	int ox=MaxX;
	int oy=MaxY;

	MaxX=max(x+width, MaxX);
	MaxY=max(y+height, MaxY);

  if  ((ox!=MaxX || oy!=MaxY) && hWndTarget && !manualUpdate) {
		RECT rc;
    if (ox == MaxX) {
      rc.top = oy - CurrentY - 5;
      rc.bottom = MaxY - CurrentY + scaleLength(50);
      rc.right = 10000;
      rc.left = 0;
      InvalidateRect(hWndTarget, &rc, false);
    }
    else {
      InvalidateRect(hWndTarget, 0, false);    
    } 
    GetClientRect(hWndTarget, &rc);

    if(MaxX>rc.right || MaxY>rc.bottom) //Update scrollbars
      SendMessage(hWndTarget, WM_SIZE, 0, MAKELONG(rc.right, rc.bottom)); 

  }

	if(Direction==1) {
		CurrentY=max(y+height, CurrentY);
	}
	else if(Direction==0) {
		CurrentX=max(x+width, CurrentX);
	}
}

void gdioutput::alert(const string &msg) const
{
  commandLock = true;
  HWND hFlt = getToolbarWindow();
  if (hasToolbar()) {
    EnableWindow(hFlt, false);
  }
  refreshFast();
  SetForegroundWindow(hWndAppMain);
	MessageBox(hWndAppMain, lang.tl(msg).c_str(), "MeOS", MB_OK|MB_ICONINFORMATION);
  if (hasToolbar()) {
    EnableWindow(hFlt, true);
  }
  commandLock = false;
}

bool gdioutput::ask(const string &s)
{
  commandLock = true;
  SetForegroundWindow(hWndAppMain);
	bool yes = MessageBox(hWndAppMain, lang.tl(s).c_str(), "MeOS", MB_YESNO|MB_ICONQUESTION)==IDYES;
  commandLock = false;
  return yes;
}

void gdioutput::setTabStops(const string &Name, int t1, int t2)
{
	DWORD array[2];
	int n=1;
	LONG bu=GetDialogBaseUnits();
	int baseunitX=LOWORD(bu);	
	array[0]=(t1 * 4) / baseunitX ;
	array[1]=(t2 * 4) / baseunitX ;
	
	if(t2>0) n=2;

	list<ListBoxInfo>::iterator it;
	for(it=LBI.begin(); it != LBI.end(); ++it){
		if(it->id==Name){
			if(!it->IsCombo)
				SendMessage(it->hWnd, LB_SETTABSTOPS, n, LPARAM(array));
			return;
		}
	}
}

void gdioutput::setInputStatus(const char *id, bool status)
{
	for(list<InputInfo>::iterator it=II.begin(); it != II.end(); ++it)
		if(it->id==id)
			EnableWindow(it->hWnd, status);

	for(list<ListBoxInfo>::iterator it=LBI.begin(); it != LBI.end(); ++it)
		if(it->id==id)
			EnableWindow(it->hWnd, status);

	for(list<ButtonInfo>::iterator it=BI.begin(); it != BI.end(); ++it)
    if(it->id==id) {
			EnableWindow(it->hWnd, status);
      if (status==false)
        it->flags = 0; //Remove default status etc.
    }
}

void gdioutput::refresh() const
{
	if (hWndTarget) {
		updateScrollbars();
		InvalidateRect(hWndTarget, NULL, true);
		UpdateWindow(hWndTarget);
	}
}

void gdioutput::refreshFast() const
{
	if (hWndTarget) {
		InvalidateRect(hWndTarget, NULL, false);
		UpdateWindow(hWndTarget);
	}
}

void gdioutput::removeString(string id)
{
	list<TextInfo>::iterator it;
	for (it=TL.begin(); it != TL.end(); ++it) {
		if (it->id==id) {	
			HDC hDC=GetDC(hWndTarget);
			//TextOut(
			RECT rc;
			rc.left=it->xp;
			rc.top=it->yp;

			DrawText(hDC, it->text.c_str(), -1, &rc, DT_CALCRECT|DT_NOPREFIX);
			SelectObject(hDC, GetStockObject(NULL_PEN));
			SelectObject(hDC, Background);
			Rectangle(hDC, rc.left, rc.top, rc.right, rc.bottom);
			ReleaseDC(hWndTarget, hDC);
			TL.erase(it);
      shownStrings.clear();
			return;
		}
	}
}

bool gdioutput::selectFirstItem(const string &id)
{
	list<ListBoxInfo>::iterator it;
	for(it=LBI.begin(); it != LBI.end(); ++it)
		if(it->id==id) {
      bool ret;
      if(it->IsCombo) 
				ret = SendMessage(it->hWnd, CB_SETCURSEL, 0,0)>=0;
			else
				ret = SendMessage(it->hWnd, LB_SETCURSEL, 0,0)>=0;
      getSelectedItem(*it);
      it->original = it->text;
      it->originalIdx = it->data;
		}

	return false;
}

void gdioutput::setWindowTitle(const string &title)
{
	if (title.length()>0) {
		string Title=MakeDash("MeOS - [") + title + "]";		
		SetWindowText(hWndAppMain, Title.c_str());
	}
	else SetWindowText(hWndAppMain, "MeOS");
}

void gdioutput::setWaitCursor(bool wait)
{
  if(wait)
	  SetCursor(LoadCursor(NULL, IDC_WAIT));
  else
	  SetCursor(LoadCursor(NULL, IDC_ARROW));
}

struct FadeInfo
{
	TextInfo ti;
	DWORD Start;
	DWORD End;
	HWND hWnd;
	COLORREF StartC;
	COLORREF EndC;
};

void TextFader(void *f)
{
	FadeInfo *fi=(FadeInfo *)f;
	HDC hDC=GetDC(fi->hWnd);

	SelectObject(hDC, GetStockObject(DEFAULT_GUI_FONT));
	SetBkMode(hDC, TRANSPARENT);
	
	double p=0;

	double r1=GetRValue(fi->StartC);
	double g1=GetGValue(fi->StartC);
	double b1=GetBValue(fi->StartC);

	double r2=GetRValue(fi->EndC);
	double g2=GetGValue(fi->EndC);
	double b2=GetBValue(fi->EndC);

	while(p<1)
	{		
		p=double(GetTickCount()-fi->Start)/double(fi->End-fi->Start);

		if(p>1) p=1;

		p=1-(p-1)*(p-1);

		int red=int((1-p)*r1+(p)*r2);
		int green=int((1-p)*g1+(p)*g2);
		int blue=int((1-p)*b1+(p)*b2);
		//int green=int((p-1)*GetGValue(fi->StartC)+(p)*GetGValue(fi->EndC));
		//int blue=int((p-1)*GetBValue(fi->StartC)+(p)*GetBValue(fi->EndC));

		SetTextColor(hDC, RGB(red, green, blue));
		TextOut(hDC, fi->ti.xp, fi->ti.yp, fi->ti.text.c_str(), fi->ti.text.length());
		Sleep(30);
		//char bf[10];
		//fi->ti.text=fi->ti.text+itoa(red, bf, 16);
	}

	ReleaseDC(fi->hWnd, hDC);
	delete fi;
}

void gdioutput::fadeOut(string Id, int ms)
{
	list<TextInfo>::iterator it;
	for(it=TL.begin(); it != TL.end(); ++it){
		if(it->id==Id){					
			FadeInfo *fi=new FadeInfo;
			fi->Start=GetTickCount();
			fi->End=fi->Start+ms;
			fi->ti=*it;
			fi->StartC=RGB(0, 0, 0);
			fi->EndC=GetSysColor(COLOR_WINDOW);
			fi->hWnd=hWndTarget;
			_beginthread(TextFader, 0, fi);
			TL.erase(it);
			return;
		}
	}
}

void gdioutput::RenderString(TextInfo &ti, HDC hDC)
{
	if(ti.format==pageNewPage || 
          ti.format==pageReserveHeight)
		return;

  if (ti.HasTimer && ti.xp == 0)
    return;

	HDC hThis=0;

	if(!hDC){
		assert(hWndTarget!=0);
		hDC=hThis=GetDC(hWndTarget);
	}
	RECT rc;
	rc.left=ti.xp-OffsetX;
	rc.top=ti.yp-OffsetY;
  rc.right = rc.left;
  rc.bottom = rc.top;

  formatString(ti, hDC);
  int format=ti.format&0xFF;

  if(format != 10 && (breakLines&ti.format) == 0){
		if(ti.xlimit==0){
      if (ti.format&textRight) {
			  DrawText(hDC, ti.text.c_str(), ti.text.length(), &rc, DT_CALCRECT|DT_NOPREFIX);
        int dx=rc.right-rc.left;
        rc.right-=dx;
        rc.left-=dx;
			  ti.TextRect=rc;        
			  DrawText(hDC, ti.text.c_str(), ti.text.length(), &rc, DT_RIGHT|DT_NOCLIP|DT_NOPREFIX);
      }
      else if (ti.format&textCenter) {
			  DrawText(hDC, ti.text.c_str(), ti.text.length(), &rc, DT_CENTER|DT_CALCRECT|DT_NOPREFIX);
        int dx=rc.right-rc.left;
        rc.right-=dx/2;
        rc.left-=dx/2;
			  ti.TextRect=rc;
			  DrawText(hDC, ti.text.c_str(), ti.text.length(), &rc, DT_CENTER|DT_NOCLIP|DT_NOPREFIX);
      }
      else{
			  DrawText(hDC, ti.text.c_str(), ti.text.length(), &rc, DT_LEFT|DT_CALCRECT|DT_NOPREFIX);
			  ti.TextRect=rc;
        DrawText(hDC, ti.text.c_str(), ti.text.length(), &rc, DT_LEFT|DT_NOCLIP|DT_NOPREFIX);
      }
		}
		else{
			DrawText(hDC, ti.text.c_str(), -1, &rc, DT_LEFT|DT_CALCRECT|DT_NOPREFIX);
			rc.right=rc.left+ti.xlimit;//min(rc.right, rc.left+ti.xlimit);
			DrawText(hDC, ti.text.c_str(), -1, &rc, DT_LEFT|DT_NOPREFIX);
			ti.TextRect=rc;
		}
	}
	else {
		RECT rc;
		memset(&rc, 0, sizeof(rc));
    int width =  scaleLength( (breakLines&ti.format) ? ti.xlimit : 450 );
    rc.right = width;
    int dx = (breakLines&ti.format) ? 0 : scaleLength(20);
		DrawText(hDC, ti.text.c_str(), ti.text.length(), &rc, DT_CALCRECT|DT_LEFT|DT_NOPREFIX|DT_WORDBREAK);
    ti.TextRect=rc;
    ti.TextRect.right+=ti.xp+dx;
    ti.TextRect.left+=ti.xp;
    ti.TextRect.top+=ti.yp;
    ti.TextRect.bottom+=ti.yp+dx;
	
    if (ti.format == 10) {
		  DWORD c=GetSysColor(COLOR_INFOBK);
		  double red=GetRValue(c);
		  double green=GetGValue(c);
		  double blue=GetBValue(c);

		  double blue1=min(255., blue*1.05);
		  double green1=min(255., green*1.05);
		  double red1=min(255., red*1.05);

		  TRIVERTEX vert[2];
		  vert [0] .x      = ti.xp-OffsetX;
		  vert [0] .y      = ti.yp-OffsetY;
		  vert [0] .Red    = 0xff00&DWORD(red1*256);
		  vert [0] .Green  = 0xff00&DWORD(green1*256);
		  vert [0] .Blue   = 0xff00&DWORD(blue1*256);
		  vert [0] .Alpha  = 0x0000;

		  vert [1] .x      = ti.xp+rc.right+dx-OffsetX;
		  vert [1] .y      = ti.yp+rc.bottom+dx-OffsetY; 
		  vert [1] .Red    = 0xff00&DWORD(red*256);
		  vert [1] .Green  = 0xff00&DWORD(green*256);
		  vert [1] .Blue   = 0xff00&DWORD(blue*256);
		  vert [1] .Alpha  = 0x0000;

		  GRADIENT_RECT gr[1];
		  gr[0].UpperLeft=0;
		  gr[0].LowerRight=1;

		  GradientFill(hDC,vert, 2, gr, 1,GRADIENT_FILL_RECT_H);
		  SelectObject(hDC, GetStockObject(NULL_BRUSH));
		  SelectObject(hDC, GetStockObject(BLACK_PEN));
		  Rectangle(hDC, vert[0].x, vert[0].y, vert[1].x, vert[1].y);
    }
    dx/=2;
		rc.top=ti.yp+dx-OffsetY;
		rc.left=ti.xp+dx-OffsetX;
		rc.bottom+=ti.yp+dx-OffsetY;
		rc.right=ti.xp+dx+width-OffsetX;
		
    SetTextColor(hDC, GetSysColor(COLOR_INFOTEXT));
		DrawText(hDC, ti.text.c_str(), ti.text.length(), &rc, DT_LEFT|DT_NOPREFIX|DT_WORDBREAK);
	}

	if(hThis)
		ReleaseDC(hWndTarget, hDC);
}

void gdioutput::RenderString(TextInfo &ti, const string &text, HDC hDC)
{
  if((ti.format&0xFF)==pageNewPage || 
      (ti.format&0xFF)==pageReserveHeight)
		return;

	RECT rc;
	rc.left=ti.xp-OffsetX;
	rc.top=ti.yp-OffsetY;
  rc.right = rc.left;
  rc.bottom = rc.top;

  int format=ti.format&0xFF;
	assert(format!=10);
  formatString(ti, hDC);

  if(ti.xlimit==0){
    if (ti.format&textRight) {
	    DrawText(hDC, text.c_str(), text.length(), &rc, DT_CALCRECT|DT_NOPREFIX);
      int dx=rc.right-rc.left;
      rc.right-=dx;
      rc.left-=dx;
	    ti.TextRect=rc;        
	    DrawText(hDC, text.c_str(), text.length(), &rc, DT_RIGHT|DT_NOCLIP|DT_NOPREFIX);
    }
    else if (ti.format&textCenter) {
	    DrawText(hDC, text.c_str(), text.length(), &rc, DT_CENTER|DT_CALCRECT|DT_NOPREFIX);
      int dx=rc.right-rc.left;
      rc.right-=dx/2;
      rc.left-=dx/2;
	    ti.TextRect=rc;
	    DrawText(hDC, text.c_str(), text.length(), &rc, DT_CENTER|DT_NOCLIP|DT_NOPREFIX);
    }
    else{
	    DrawText(hDC, text.c_str(), text.length(), &rc, DT_LEFT|DT_CALCRECT|DT_NOPREFIX);
	    ti.TextRect=rc;
	    DrawText(hDC, text.c_str(), text.length(), &rc, DT_LEFT|DT_NOCLIP|DT_NOPREFIX);
    }
  }
  else{
    if (ti.format&textRight) {
      DrawText(hDC, text.c_str(), text.length(), &rc, DT_LEFT|DT_CALCRECT|DT_NOPREFIX);
      rc.right = rc.left + ti.xlimit;      
	    ti.TextRect = rc;
	    DrawText(hDC, text.c_str(), text.length(), &rc, DT_RIGHT|DT_NOPREFIX);
    }
    else {
      DrawText(hDC, text.c_str(), text.length(), &rc, DT_LEFT|DT_CALCRECT|DT_NOPREFIX);
      rc.right=rc.left+ti.xlimit;
      DrawText(hDC, text.c_str(), text.length(), &rc, DT_LEFT|DT_NOPREFIX);
      ti.TextRect=rc;
    }
  }
}

void gdioutput::resetLast() {
  lastFormet = -1; 
  lastActive = false; 
  lastHighlight = false;
  lastColor = -1;
}

void gdioutput::formatString(const TextInfo &ti, HDC hDC)
{
  int format=ti.format&0xFF;

  if (lastFormet == format && 
      lastActive == ti.Active && 
      lastHighlight == ti.Highlight && 
      lastColor == ti.color)
    return;

  if (format==0 || format==10) {
    SelectObject(hDC, pfMedium);
  }
  else if(format==fontMedium){
		SelectObject(hDC, pfMedium);	
	}
	else if(format==1){
		SelectObject(hDC, Medium);	
	}	
	else if(format==boldLarge){
		SelectObject(hDC, Large);	
	}	
	else if(format==boldHuge){
		SelectObject(hDC, Huge);	
	}
	else if(format==boldSmall){
		SelectObject(hDC, Small);	
	}
	else if(format==fontLarge){
		SelectObject(hDC, pfLarge);	
	}
  else if(format==fontMediumPlus){
		SelectObject(hDC, pfMediumPlus);	
	}
	else if(format==fontSmall){
		SelectObject(hDC, pfSmall);	
	}
  else if(format==italicSmall){
		SelectObject(hDC, pfSmallItalic);	
	}
	else {
		SelectObject(hDC, GetStockObject(DEFAULT_GUI_FONT));
  }	

	SetBkMode(hDC, TRANSPARENT);

	if(ti.Active) 
		SetTextColor(hDC, RGB(255,0,0));
	else if(ti.Highlight) 
		SetTextColor(hDC, RGB(64,64,128));	
	else
		SetTextColor(hDC, ti.color);
}

void gdioutput::calcStringSize(TextInfo &ti, HDC hDC_in)
{
  HDC hDC=hDC_in;

  if(!hDC) {
	  assert(hWndTarget!=0);
	  hDC=GetDC(hWndTarget);
  }
  RECT rc;
  rc.left=ti.xp-OffsetX;
	rc.top=ti.yp-OffsetY;
  rc.right = rc.left;
  rc.bottom = rc.top;

  resetLast();
  formatString(ti, hDC);
  int format=ti.format&0xFF;

  if (format != 10 && (breakLines&ti.format) == 0) {
    if(ti.xlimit==0){
      if (ti.format&textRight) {
		    DrawText(hDC, ti.text.c_str(), ti.text.length(), &rc, DT_CALCRECT|DT_NOPREFIX);
        int dx=rc.right-rc.left;
        rc.right-=dx;
        rc.left-=dx;
		    ti.TextRect=rc;        
      }
      else if (ti.format&textCenter) {
		    DrawText(hDC, ti.text.c_str(), ti.text.length(), &rc, DT_CENTER|DT_CALCRECT|DT_NOPREFIX);
        int dx=rc.right-rc.left;
        rc.right-=dx/2;
        rc.left-=dx/2;
		    ti.TextRect=rc;
      }
      else{
        DrawText(hDC, ti.text.c_str(), ti.text.length(), &rc, DT_LEFT|DT_CALCRECT|DT_NOPREFIX);
		    ti.TextRect=rc;
      }
	  }
	  else {
		  DrawText(hDC, ti.text.c_str(), ti.text.length(), &rc, DT_LEFT|DT_CALCRECT|DT_NOPREFIX);
		  rc.right=rc.left+ti.xlimit;//min(rc.right, rc.left+ti.xlimit);
		  ti.TextRect=rc;
	  }
  }
  else {
		RECT rc;
		memset(&rc, 0, sizeof(rc));
		rc.right = scaleLength( (breakLines&ti.format) ? ti.xlimit : 450 );
    int dx = (breakLines&ti.format) ? 0 : scaleLength(20);
		DrawText(hDC, ti.text.c_str(), ti.text.length(), &rc, DT_CALCRECT|DT_LEFT|DT_NOPREFIX|DT_WORDBREAK);
    ti.TextRect=rc;
    ti.TextRect.right+=ti.xp+dx;
    ti.TextRect.left+=ti.xp;
    ti.TextRect.top+=ti.yp;
    ti.TextRect.bottom+=ti.yp+dx;
  }


  if(!hDC_in)
	  ReleaseDC(hWndTarget, hDC);
}


void gdioutput::updateScrollbars() const
{
	RECT rc;
	GetClientRect(hWndTarget, &rc);
	SendMessage(hWndTarget, WM_SIZE, 0, MAKELONG(rc.right, rc.bottom)); 
}


void gdioutput::updateObjectPositions()
{
	{
		list<ButtonInfo>::iterator it;
		for(it=BI.begin(); it != BI.end(); ++it)
		{
			//MoveWindow(it->hWnd, it->
			if(!it->AbsPos)
				SetWindowPos(it->hWnd, 0, it->xp-OffsetX, it->yp-OffsetY, 0,0, SWP_NOSIZE|SWP_NOZORDER|SWP_NOCOPYBITS);
		}	
	}
	{	
		list<InputInfo>::iterator it;
		for(it=II.begin(); it != II.end(); ++it)
			SetWindowPos(it->hWnd, 0, it->xp-OffsetX, it->yp-OffsetY, 0,0, SWP_NOSIZE|SWP_NOZORDER);	
	}

	{
		list<ListBoxInfo>::iterator it;
		for(it=LBI.begin(); it != LBI.end(); ++it)
			SetWindowPos(it->hWnd, 0, it->xp-OffsetX, it->yp-OffsetY, 0,0, SWP_NOSIZE|SWP_NOZORDER);	

	}
}

void gdioutput::addInfoBox(string id, string text, int TimeOut, GUICALLBACK cb)
{
  InfoBox Box;

  Box.id=id;
  Box.CallBack=cb;
  Box.text=lang.tl(text);

  if(TimeOut>0)
    Box.TimeOut=GetTickCount()+TimeOut;

	IBox.push_back(Box);		 
	refresh();
}

void gdioutput::drawBox(HDC hDC, InfoBox &Box, RECT &pos)
{
	SelectObject(hDC, GetStockObject(DEFAULT_GUI_FONT));	
	SetBkMode(hDC, TRANSPARENT);

	//Calculate size.
	RECT testrect;

	memset(&testrect, 0, sizeof(RECT));
	DrawText(hDC, Box.text.c_str(), Box.text.length(), &testrect, DT_CALCRECT|DT_LEFT|DT_NOPREFIX|DT_SINGLELINE);

	if(testrect.right>250 || Box.text.find_first_of('\n')!=string::npos)
	{
		testrect.right=250;
		DrawText(hDC, Box.text.c_str(), Box.text.length(), &testrect, DT_CALCRECT|DT_LEFT|DT_NOPREFIX|DT_WORDBREAK);
	}
	else if(testrect.right<80)
		testrect.right=80;
	
	pos.left=pos.right-(testrect.right+22);
	pos.top=pos.bottom-(testrect.bottom+20);

	DWORD c=GetSysColor(COLOR_INFOBK);
	double red=GetRValue(c);
	double green=GetGValue(c);
	double blue=GetBValue(c);

	double blue1=min(255., blue*1.1);
	double green1=min(255., green*1.1);
	double red1=min(255., red*1.1);

	TRIVERTEX vert[2];
	vert [0] .x      = pos.left;
	vert [0] .y      = pos.top;
	vert [0] .Red    = 0xff00&DWORD(red*256);
	vert [0] .Green  = 0xff00&DWORD(green*256);
	vert [0] .Blue   = 0xff00&DWORD(blue*256);
	vert [0] .Alpha  = 0x0000;

	vert [1] .x      = pos.right;
	vert [1] .y      = pos.bottom; 
	vert [1] .Red    = 0xff00&DWORD(red1*256);
	vert [1] .Green  = 0xff00&DWORD(green1*256);
	vert [1] .Blue   = 0xff00&DWORD(blue1*256);
	vert [1] .Alpha  = 0x0000;

	GRADIENT_RECT gr[1];

	gr[0].UpperLeft=0;
	gr[0].LowerRight=1;

	//if(MaxY>500)
	GradientFill(hDC,vert, 2, gr, 1,GRADIENT_FILL_RECT_V);

	SelectObject(hDC, GetStockObject(NULL_BRUSH));
	SelectObject(hDC, GetStockObject(BLACK_PEN));
	Rectangle(hDC, pos.left, pos.top, pos.right, pos.bottom);
  Box.BoundingBox=pos;
	//Close Box
	RECT Close;
	Close.top=pos.top+3;
	Close.bottom=Close.top+11;
	Close.right=pos.right-3;
	Close.left=Close.right-11;

	Box.Close=Close;
	drawCloseBox(hDC, Close, false);
	
	RECT tr=pos;

	tr.left+=10;
	tr.right-=10;
	tr.top+=15;
	tr.bottom-=5;

	drawBoxText(hDC, tr, Box, false);

	Box.TextRect=tr;
	
}

void gdioutput::drawBoxes(HDC hDC, RECT &rc)
{
	RECT pos;
	pos.right=rc.right;
	pos.bottom=rc.bottom;

	list<InfoBox>::iterator it=IBox.begin();

	while(it!=IBox.end())	{
		drawBox(hDC, *it, pos);
		pos.bottom=pos.top;
		++it;
	}
}

void gdioutput::drawCloseBox(HDC hDC, RECT &Close, bool pressed)
{
	if (!pressed) {
		SelectObject(hDC, GetStockObject(WHITE_BRUSH));
		SelectObject(hDC, GetStockObject(BLACK_PEN));
	}
	else {
		SelectObject(hDC, GetStockObject(LTGRAY_BRUSH));
		SelectObject(hDC, GetStockObject(BLACK_PEN));
	}
	//Close Box
	Rectangle(hDC, Close.left, Close.top, Close.right, Close.bottom);

	MoveToEx(hDC, Close.left+2, Close.top+2, 0);
	LineTo(hDC, Close.right-2, Close.bottom-2);
	
	MoveToEx(hDC, Close.right-2, Close.top+2, 0);
	LineTo(hDC, Close.left+2, Close.bottom-2);
}

void gdioutput::drawBoxText(HDC hDC, RECT &tr, InfoBox &Box, bool highligh)
{
	SelectObject(hDC, GetStockObject(DEFAULT_GUI_FONT));	
	SetBkMode(hDC, TRANSPARENT);

	if (highligh) {
		SetTextColor(hDC, 0x005050FF);
	}
	else {
		SetTextColor(hDC, GetSysColor(COLOR_INFOTEXT));
	}

	DrawText(hDC, Box.text.c_str(), Box.text.length(), &tr, DT_LEFT|DT_NOPREFIX|DT_WORDBREAK);
}

bool gdioutput::RemoveFirstInfoBox(const string &id)
{
	list<InfoBox>::iterator it=IBox.begin();

	while(it!=IBox.end())	{
		if(it->id==id) {
			IBox.erase(it);
			return true;
		}
		++it;
	}
	return false;
}


string gdioutput::getTimerText(int ZeroTime, int format)
{
	TextInfo temp;
	temp.ZeroTime=0;
	//memset(&temp, 0, sizeof(TextInfo));
	temp.format=format;
	return getTimerText(&temp, 1000*ZeroTime);	
}

string gdioutput::getTimerText(TextInfo *tit, DWORD T)
{
	int rt=(int(T)-int(tit->ZeroTime))/1000;
	string text;

	int t=abs(rt);
	char bf[16];
	if(rt>=3600)
		sprintf_s(bf, 16, "%d:%02d:%02d", t/3600, (t/60)%60, t%60);
	else
		sprintf_s(bf, 16, "%d:%02d", (t/60), t%60);

	if(rt>0)
		if(tit->format&timerCanBeNegative) text=string("+")+bf;
		else				text=bf;
	else if(rt<0)
		if(tit->format&timerCanBeNegative) text=string("-")+bf;
		else				text="-";	

	return text;
}

void gdioutput::CheckInterfaceTimeouts(DWORD T)
{
	list<InfoBox>::iterator it=IBox.begin();

	while (it!=IBox.end())	{
		if (it->TimeOut && it->TimeOut<T) {
			if(it->HasCapture || it->HasTCapture)
				ReleaseCapture();

      InvalidateRect(hWndTarget, &(it->BoundingBox), true);
			IBox.erase(it);
      it=IBox.begin();
		}
    else ++it;
	}

	list<TextInfo>::iterator tit = TL.begin();
	TextInfo *timeout=0;
	while(tit!=TL.end()){
		if(tit->HasTimer){
      string text = tit->xp > 0 ? getTimerText(&*tit, T) : "";
			if(tit->TimeOut && T>DWORD(tit->TimeOut)){
				tit->TimeOut=0;
				timeout=&*tit;
			}
			if (text != tit->text) {
				RECT rc=tit->TextRect;
				tit->text=text;
				calcStringSize(*tit);

				rc.right=max(tit->TextRect.right, rc.right);
				rc.bottom=max(tit->TextRect.bottom, rc.bottom);

				InvalidateRect(hWndTarget, &rc, true);
			}
		}
		++tit;
	}

	if(timeout && timeout->CallBack)
		timeout->CallBack(this, GUI_TIMEOUT, timeout);
}

bool gdioutput::removeControl(const string &id)
{
	list<ButtonInfo>::iterator it=BI.begin();

	while (it!=BI.end())	{
		if (it->id==id) {
			DestroyWindow(it->hWnd);
			BI.erase(it);
			return true;
		}
		++it;
	}

  list<ListBoxInfo>::iterator lit=LBI.begin();

	while (lit!=LBI.end())	{
		if (lit->id==id) {
			DestroyWindow(lit->hWnd);
			if (lit->writeLock)
        hasCleared = true;
      LBI.erase(lit);
      return true;
		}
		++lit;
	}

  list<InputInfo>::iterator iit=II.begin();

	while (iit!=II.end())	{
		if (iit->id==id) {
			DestroyWindow(iit->hWnd);
			II.erase(iit);
			return true;
		}
		++iit;
	}
	return false;
}

bool gdioutput::hideControl(const string &id)
{
	list<ButtonInfo>::iterator it=BI.begin();

	while (it!=BI.end())	{
		if (it->id==id) {
      ShowWindow(it->hWnd, SW_HIDE);
			return true;
		}
		++it;
	}

  list<ListBoxInfo>::iterator lit=LBI.begin();

	while (lit!=LBI.end())	{
		if (lit->id==id) {
			ShowWindow(lit->hWnd, SW_HIDE);
			return true;
		}
		++lit;
	}

  list<InputInfo>::iterator iit=II.begin();

	while (iit!=II.end())	{
		if (iit->id==id) {
			ShowWindow(iit->hWnd, SW_HIDE);
			return true;
		}
		++iit;
	}
	return false;
}


void gdioutput::setRestorePoint()
{
  setRestorePoint("");
}


void gdioutput::setRestorePoint(const string &id)
{
  RestoreInfo ri;

  ri.id=id;

	ri.nLBI=LBI.size();
	ri.nBI=BI.size();
	ri.nII=II.size();
	ri.nTL=TL.size();
	ri.nRect=Rectangles.size();
	ri.nTooltip = toolTips.size();
	
	ri.nHWND=FocusList.size();
  
	ri.sCX=CurrentX;
	ri.sCY=CurrentY;
	ri.sMX=MaxX;
	ri.sMY=MaxY;
  ri.sOX=OffsetX;
  ri.sOY=OffsetY;
  
  ri.onClear = onClear;
  ri.postClear = postClear;
  restorePoints[id]=ri;
}

void gdioutput::restoreInternal(const RestoreInfo &ri)
{
  int toolRemove=toolTips.size()-ri.nTooltip;
	while (toolRemove>0 && toolTips.size()>0) {
    ToolInfo &info=toolTips.back();
    if (hWndToolTip) {
      SendMessage(hWndToolTip, TTM_DELTOOL, 0, (LPARAM) &info.ti);
    }
		toolTips.pop_back();
		toolRemove--;
	}

	int lbiRemove=LBI.size()-ri.nLBI;
	while (lbiRemove>0 && LBI.size()>0) {
		ListBoxInfo &lbi=LBI.back();
		DestroyWindow(lbi.hWnd);
    if (lbi.writeLock)
      hasCleared = true;
		LBI.pop_back();
		lbiRemove--;
	}
	int tlRemove=TL.size()-ri.nTL;

	while (tlRemove>0 && TL.size()>0) {
		TL.pop_back();
		tlRemove--;
	}
  itTL=TL.begin();
  // Clear cache of shown strings
  shownStrings.clear();

	int biRemove=BI.size()-ri.nBI;
	while (biRemove>0 && BI.size()>0) {
		ButtonInfo &bi=BI.back();

		DestroyWindow(bi.hWnd);
		BI.pop_back();
		biRemove--;
	}

	int iiRemove=II.size()-ri.nII;

	while (iiRemove>0 && II.size()>0) {
		InputInfo &ii=II.back();

		DestroyWindow(ii.hWnd);
		II.pop_back();
		iiRemove--;
	}

	int rectRemove=Rectangles.size()-ri.nRect;

	while (rectRemove>0 && Rectangles.size()>0) {
		Rectangles.pop_back();
		rectRemove--;
	}
	int hwndRemove=FocusList.size()-ri.nHWND;

	while(hwndRemove>0 && FocusList.size()>0)
		FocusList.pop_back();

  CurrentX=ri.sCX;
	CurrentY=ri.sCY;
  onClear = ri.onClear;
  postClear = ri.postClear;
}

void gdioutput::restore(const string &id, bool DoRefresh)
{
  if(restorePoints.count(id)==0)
    return;

  const RestoreInfo &ri=restorePoints[id];

  restoreInternal(ri);

	MaxX=ri.sMX;
	MaxY=ri.sMY;

	if(DoRefresh)
		refresh();

  setOffset(ri.sOY, ri.sOY, false);
}

void gdioutput::restoreNoUpdate(const string &id)
{
  if(restorePoints.count(id)==0)
    return;

  const RestoreInfo &ri=restorePoints[id];
  
  restoreInternal(ri);
}

void gdioutput::setPostClearCb(GUICALLBACK cb)
{
	postClear=cb;
}


void gdioutput::setOnClearCb(GUICALLBACK cb)
{
	onClear=cb;
}

bool gdioutput::canClear()
{
	if(!onClear)
		return true;

  try {
	  return onClear(this, GUI_CLEAR, 0)!=0;
  }
  catch(const std::exception &ex) {            
    string msg(ex.what());  
    alert(msg);
    return true;
  }
}

int gdioutput::sendCtrlMessage(const string &id)
{
	list<ButtonInfo>::iterator it;
	for (it=BI.begin(); it != BI.end(); ++it) {		
		if (id==it->id && it->CallBack) {
			return it->CallBack(this, GUI_BUTTON, &*it); //it may be destroyed here...			
		}
	}
	{
		list<EventInfo>::iterator it;
		for(it=Events.begin(); it != Events.end(); ++it){		
			if (id==it->id && it->CallBack) {
				return it->CallBack(this, GUI_EVENT, &*it); //it may be destroyed here...			
			}
		}
	}
	return 0;
}

void gdioutput::unregisterEvent(const string &id)
{
	list<EventInfo>::iterator it;
  for (it = Events.begin(); it != Events.end(); ++it) {
    if ( id == it->id) {
      Events.erase(it);
      return;
    }
  }
}

void gdioutput::registerEvent(const string &id, GUICALLBACK cb)
{
	list<EventInfo>::iterator it;
  for (it = Events.begin(); it != Events.end(); ++it) {
    if ( id == it->id) {
      Events.erase(it);
      break;
    }
  }

	EventInfo ei;
	ei.id=id;
	ei.CallBack=cb;

	Events.push_front(ei);
}

DWORD gdioutput::makeEvent(const string &id, DWORD data, void *extra)
{
	list<EventInfo>::iterator it;
	
	for(it=Events.begin(); it != Events.end(); ++it){		
		if(id==it->id && it->CallBack){
			it->data=data;
      it->setExtra(extra);
			return it->CallBack(this, GUI_EVENT, &*it); //it may be destroyed here...			
		}
	}
	return -1;
}

void gdioutput::addRectangle(RECT &rc, GDICOLOR Color, bool DrawBorder)
{
	RectangleInfo ri;

	ri.rc=rc;
	if(Color==colorDefault)
		ri.color=GetSysColor(COLOR_INFOBK);
	else ri.color=Color;

	ri.drawBorder=DrawBorder;

  if(hWndTarget && !manualUpdate) {
    HDC hDC=GetDC(hWndTarget);

		if(ri.drawBorder)
			SelectObject(hDC, GetStockObject(BLACK_PEN));
		else
			SelectObject(hDC, GetStockObject(NULL_PEN));		
		SetDCBrushColor(hDC, ri.color);

		RECT rect_rc=rc;
		OffsetRect(&rect_rc, -OffsetX, -OffsetY);
		Rectangle(hDC, rect_rc.left, rect_rc.top, rect_rc.right, rect_rc.bottom);

    ReleaseDC(hWndTarget, hDC);
  }

  updatePos(rc.left, rc.top, rc.right-rc.left+5, rc.bottom-rc.top+5);
	Rectangles.push_back(ri);
}


void gdioutput::setOffset(int x, int y, bool update)
{	
  int h,w;
  getTargetDimension(w, h);

  int cdy = 0;
  int cdx = 0;

  if(y!=OffsetY) {    
    int oldY = OffsetY;
    OffsetY = y;
    if (OffsetY < 0)
      OffsetY = 0;
    else if (OffsetY > MaxY)
      OffsetY = MaxY;
    //cdy=(oldY!=OffsetY);
    cdy = oldY - OffsetY;
  }

  if(x!=OffsetX) {
    int oldX = OffsetX;
    OffsetX = x;
    if (OffsetX < 0)
      OffsetX = 0;
    else if (OffsetX > MaxX)
      OffsetX = MaxX;

    //cdx=(oldX!=OffsetX);
    cdx = oldX - OffsetX;
  }

  if(cdx || cdy) {
    updateScrollbars();	
    updateObjectPositions();
    if(cdy) {
      SCROLLINFO si;
      memset(&si, 0, sizeof(si));
      
      si.nPos=OffsetY;
      si.fMask=SIF_POS;
      SetScrollInfo(hWndTarget, SB_VERT, &si, true);
    }

    if(cdx) {
      SCROLLINFO si;
      memset(&si, 0, sizeof(si));
      
      si.nPos=OffsetX;
      si.fMask=SIF_POS;
      SetScrollInfo(hWndTarget, SB_HORZ, &si, true);
    }

    if (update) {
		  //RECT ScrollArea, ClipArea;
      //GetClientRect(hWndTarget, &ScrollArea);
		  //ClipArea = ScrollArea;

    /*  ScrollArea.top=-gdi->getHeight()-100;
      ScrollArea.bottom+=gdi->getHeight();
      ScrollArea.right=gdi->getWidth()-gdi->GetOffsetX()+15;
      ScrollArea.left = -2000;
	*/
      ScrollWindowEx(hWndTarget, -cdx,  cdy, 
			               NULL, NULL,  
                    (HRGN) NULL, (LPRECT) NULL, 0/*SW_INVALIDATE|SW_SMOOTHSCROLL|(1000*65536 )*/); 
      UpdateWindow(hWndTarget);

    }
  }
}

void gdioutput::scrollTo(int x, int y)
{
	int cx=x-OffsetX;
  int cy=y-OffsetY;

  int h,w;
  getTargetDimension(w, h);

  bool cdy=false;
  bool cdx=false;

  if(cy<=(h/15) || cy>=(h-h/10)) {
    int oldY=OffsetY;
    OffsetY=y-h/2;
    if(OffsetY<0)
      OffsetY=0;
    else if(OffsetY>MaxY)
      OffsetY=MaxY;

    cdy=(oldY!=OffsetY);
  }

  if(cx<=(w/15) || cx>=(w-w/8)) {
    int oldX=OffsetX;
    OffsetX=x-w/2;
    if(OffsetX<0)
      OffsetX=0;
    else if(OffsetX>MaxX)
      OffsetX=MaxX;

    cdx=(oldX!=OffsetX);
  }

  if(cdx || cdy) {
    updateScrollbars();	
    updateObjectPositions();
    if(cdy) {
      SCROLLINFO si;
      memset(&si, 0, sizeof(si));
      
      si.nPos=OffsetY;
      si.fMask=SIF_POS;
      SetScrollInfo(hWndTarget, SB_VERT, &si, true);
    }

    if(cdx) {
      SCROLLINFO si;
      memset(&si, 0, sizeof(si));
      
      si.nPos=OffsetX;
      si.fMask=SIF_POS;
      SetScrollInfo(hWndTarget, SB_HORZ, &si, true);
    }
  }
}

void gdioutput::scrollToBottom()
{
	OffsetY=MaxY;
  SCROLLINFO si;
  memset(&si, 0, sizeof(si));

  updateScrollbars();
  updateObjectPositions();
  si.nPos=OffsetY;
  si.fMask=SIF_POS;
  SetScrollInfo(hWndTarget, SB_VERT, &si, true);
}

bool gdioutput::clipOffset(int PageX, int PageY, int &MaxOffsetX, int &MaxOffsetY)
{
	int oy=OffsetY;
	int ox=OffsetX;

  MaxOffsetY=max(GetPageY()-PageY, 0);
  MaxOffsetX=max(GetPageX()-PageX, 0);
 
/*	if(PageY){
    if((GetPageY()-OffsetY)<PageY)
			OffsetY=GetPageY()-PageY;
	}
	else
		OffsetY=0;
*/
	if(OffsetY<0) OffsetY=0;
  else if(OffsetY>MaxOffsetY)
		OffsetY=MaxOffsetY;

  if(OffsetX<0) OffsetX=0;
  else if(OffsetX>MaxOffsetX)
		OffsetX=MaxOffsetX;

	/*if(PageX){ 
		if ((MaxX-OffsetX)<PageX)
			OffsetX=MaxX-PageX;
	}
	else
		OffsetX=0;

	if(OffsetX<0) OffsetX=0;
	else if(OffsetX>MaxX)
		OffsetX=MaxX;
*/
	if(ox!=OffsetX || oy!=OffsetY){
		updateObjectPositions();
		return true;
	}
	return false;

}	

//bool ::GetSaveFile(string &file, char *filter)
string gdioutput::browseForSave(const string &Filter, const string &defext, int &FilterIndex)
{
	InitCommonControls();
	
	char FileName[260];
	FileName[0]=0;
	char sbuff[256];
	memset(sbuff,0, 256);
	OPENFILENAME of;
//	LoadString(hInst,1016,sbuff,256);
	sprintf_s(sbuff, 256, "%s", Filter.c_str());

	int sl=strlen(sbuff);
	for(int m=0;m<sl;m++) if(sbuff[m]=='�') sbuff[m]=0;

	of.lStructSize      =sizeof(of);
	of.hwndOwner        =hWndTarget;
	of.hInstance        =(HINSTANCE)GetWindowLong(hWndTarget, GWL_HINSTANCE);
	of.lpstrFilter      =sbuff;
	of.lpstrCustomFilter=NULL;
	of.nMaxCustFilter   =0;
	of.nFilterIndex     =1;
	of.lpstrFile        = FileName;
	of.nMaxFile         =260;
	of.lpstrFileTitle   =NULL;
	of.nMaxFileTitle    =0;
	of.lpstrInitialDir  =NULL;
	of.lpstrTitle       =NULL;
	of.Flags            =OFN_OVERWRITEPROMPT|OFN_HIDEREADONLY;
	of.lpstrDefExt   	  = defext.c_str();
	of.lpfnHook		     =NULL;
	
	
	if(GetSaveFileName(&of)==false)
		return "";

	FilterIndex=of.nFilterIndex;

	return FileName;
}


string gdioutput::browseForOpen(const string &Filter, const string &defext)
{
	InitCommonControls();
	
	char FileName[260];
	FileName[0]=0;
	char sbuff[256];
	memset(sbuff,0, 256);
	OPENFILENAME of;
//	LoadString(hInst,1016,sbuff,256);
	sprintf_s(sbuff, 256, "%s", Filter.c_str());

	int sl=strlen(sbuff);
	for(int m=0;m<sl;m++) if(sbuff[m]=='�') sbuff[m]=0;
	
	of.lStructSize      =sizeof(of);
	of.hwndOwner        =hWndTarget;
	of.hInstance        =(HINSTANCE)GetWindowLong(hWndTarget, GWL_HINSTANCE);
	of.lpstrFilter      =sbuff;
	of.lpstrCustomFilter=NULL;
	of.nMaxCustFilter   =0;
	of.nFilterIndex     =1;
	of.lpstrFile        = FileName;
	of.nMaxFile         =260;
	of.lpstrFileTitle   =NULL;
	of.nMaxFileTitle    =0;
	of.lpstrInitialDir  =NULL;
	of.lpstrTitle       =NULL;
	of.Flags            =OFN_PATHMUSTEXIST|OFN_FILEMUSTEXIST|OFN_HIDEREADONLY;
	of.lpstrDefExt   	  = defext.c_str();
	of.lpfnHook		     =NULL;
	
	
	if(GetOpenFileName(&of)==false)
		return "";
	
	return FileName;
}

string gdioutput::browseForFolder(const string &FolderStart)
{
	CoInitializeEx(0, COINIT_APARTMENTTHREADED);
	BROWSEINFO bi;
	
	char InstPath[260];
	strcpy_s(InstPath, FolderStart.c_str());

	memset(&bi, 0, sizeof(bi) );
				
	bi.hwndOwner=hWndAppMain;
	bi.pszDisplayName=InstPath;
	bi.lpszTitle="V�lj katalog";
	bi.ulFlags=BIF_RETURNONLYFSDIRS|BIF_EDITBOX|BIF_NEWDIALOGSTYLE|BIF_EDITBOX;
	
	LPITEMIDLIST  pidl_new=SHBrowseForFolder(&bi);
				
	if (pidl_new==NULL)
		return "";
				
	// Convert the item ID list's binary
	// representation into a file system path
	//char szPath[_MAX_PATH];
	SHGetPathFromIDList(pidl_new, InstPath);
				
	// Allocate a pointer to an IMalloc interface
	LPMALLOC pMalloc;
	
	// Get the address of our task allocator's IMalloc interface
	SHGetMalloc(&pMalloc);
	
	// Free the item ID list allocated by SHGetSpecialFolderLocation
	pMalloc->Free(pidl_new);
	
	// Free our task allocator
	pMalloc->Release();

	return InstPath;
}


bool gdioutput::openDoc(const char *doc)
{
	return (int)ShellExecute(hWndTarget, "open", doc, NULL, "", SW_SHOWNORMAL ) >32; 	
}

void gdioutput::init(HWND hWnd, HWND hMain, HWND hTab)
{
	SetWindow(hWnd);
	hWndAppMain=hMain;
	hWndTab=hTab;

  InitCommonControls(); 

  hWndToolTip = CreateWindow(TOOLTIPS_CLASS, (LPSTR) NULL, TTS_ALWAYSTIP, 
      CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 
      NULL, (HMENU) NULL, (HINSTANCE)GetWindowLong(hWndTarget, GWL_HINSTANCE), NULL); 
}

void gdioutput::addToolTip(const string &Tip, HWND hWnd, RECT *rc)
{
	if(!hWndToolTip)
		return;
 
  toolTips.push_back(ToolInfo());
  ToolInfo &info = toolTips.back();
  TOOLINFO &ti = info.ti; 
  info.tip = lang.tl(Tip);

  memset(&ti, 0, sizeof(ti));
  ti.cbSize = sizeof(TOOLINFO); 

  if (hWnd != 0) {
    ti.uFlags = TTF_IDISHWND; 
    info.id = int(hWnd);
    ti.uId = (UINT) hWnd; 
  }
  else {
    ti.uFlags = TTF_SUBCLASS; 
    info.id = toolTips.size();
    ti.uId = info.id;
  }

  ti.hwnd = hWndTarget; 
  ti.hinst = (HINSTANCE)GetWindowLong(hWndTarget, GWL_HINSTANCE); 
  
  ti.lpszText = (char *)toolTips.back().tip.c_str(); 

  if (rc != 0)
    ti.rect = *rc;
    
	SendMessage(hWndToolTip, TTM_ADDTOOL, 0, (LPARAM) &ti);
}


void gdioutput::selectTab(int Id)
{
	if(hWndTab)
		TabCtrl_SetCurSel(hWndTab, Id);
}

void gdioutput::getTargetDimension(int &x, int &y) const
{
	if(hWndTarget){
		RECT rc;
		GetClientRect(hWndTarget, &rc);
		x=rc.right;
    y=rc.bottom;
	}
  else {
    x=0;
    y=0;
  }
}

Table &gdioutput::getTable() const {
  if(Tables.empty())
    throw std::exception("No table defined");

  return *const_cast<Table *>(Tables.back().table);
}

static int gdiTableCB(gdioutput *gdi, int type, void *data) 
{
  if (type == GUI_BUTTON) {
    ButtonInfo bi = *static_cast<ButtonInfo *>(data);
    gdi->tableCB(bi, static_cast<Table *>(bi.getExtra()));
  }
  return 0;
}

void gdioutput::tableCB(ButtonInfo &bu, Table *t) 
{
  #ifndef MEOSDB  
  
  if (bu.id=="tblPrint") {
    t->keyCommand(*this, KC_PRINT);
  }
  else if (bu.id=="tblColumns") {
    disableTables();
    if (Tables.empty())
      return;

    restore("tblRestore");
    int ybase =  Tables.back().yp;
    addString("", ybase, 20, boldLarge, "V�lj kolumner");
    ybase += scaleLength(30);
    addString("", ybase, 20, 0, "V�lj kolumner f�r tabellen X.#"+ t->getTableName());
    ybase += getLineHeight()*2;

    addListBox(20, ybase, "tblColSel", 180, 450, 0, "", "", true);
    const int btnHeight = getButtonHeight()+scaleLength(5);
    vector<Table::ColSelection> cols = t->getColumns();
    set<int> sel;

    for (size_t k=0; k<cols.size(); k++) {
      addItem("tblColSel", cols[k].name, cols[k].index);
      if (cols[k].selected)
        sel.insert(cols[k].index);
    }
    setSelection("tblColSel", sel);
    int xp = scaleLength(220);
    addButton(xp, ybase+btnHeight*0, "tblAll", "V�lj allt", gdiTableCB);
    addButton(xp, ybase+btnHeight*1, "tblNone", "V�lj inget", gdiTableCB);
    addButton(xp, ybase+btnHeight*2, "tblAuto", "V�lj automatiskt", gdiTableCB).setExtra(t);
    
    addButton(xp, ybase+btnHeight*4, "tblOK", "OK", gdiTableCB).setExtra(t);
    addButton(xp, ybase+btnHeight*5, "tblCancel", "Avbryt", gdiTableCB);
    
    if (toolbar)
      toolbar->hide();

    refresh();
  }
  else if (bu.id=="tblAll") {
    set<int> sel;
    sel.insert(-1);
    setSelection("tblColSel", sel);
  }
  else if (bu.id=="tblNone") {
    set<int> sel;
    setSelection("tblColSel", sel);
  }
  else if (bu.id=="tblAuto") {
    restore("tblRestore", false); 
    t->autoSelectColumns();
    t->autoAdjust(*this);
    enableTables();
    refresh();
  }
  else if (bu.id=="tblOK") {
    set<int> sel;
    getSelection("tblColSel", sel);
    restore("tblRestore", false);
    t->selectColumns(sel);
    t->autoAdjust(*this);
    enableTables();
    refresh();
  }
  else if (bu.id=="tblReset") {
    t->resetColumns();
    t->autoAdjust(*this);
    t->updateDimension(*this);
    refresh();
  }
  else if (bu.id=="tblUpdate") {
    t->keyCommand(*this, KC_REFRESH);
  }
  else if (bu.id=="tblCancel") {
    restore("tblRestore", true);
    enableTables();
    refresh();
  }
  else if (bu.id == "tblCopy") {
    t->keyCommand(*this, KC_COPY);
  }
  else if (bu.id == "tblPaste") {
    t->keyCommand(*this, KC_PASTE);
  }
  else if (bu.id == "tblRemove") {    
    t->keyCommand(*this, KC_DELETE);
  }
  else if (bu.id == "tblInsert") {
    t->keyCommand(*this, KC_INSERT);
  }

  #endif
}

void gdioutput::enableTables()
{ 
  useTables=true; 
#ifndef MEOSDB  
  if (!Tables.empty()) {
    Table *t = Tables.front().table;
    if (toolbar == 0)  
      toolbar = new Toolbar(*this);
/*    RECT rc;
    rc.top = 10;
    rc.bottom = 100;
    rc.left = 100;
    rc.right = 200;
    addToolTip("Hej hopp!", 0, &rc);
*/
    toolbar->setData(t);

    string tname = string("table") + itos(t->canDelete()) + itos(t->canInsert()) + itos(t->canPaste());
    if (!toolbar->isLoaded(tname)) {
      toolbar->reset();
      toolbar->addButton("tblColumns", 1, 2, "V�lj vilka kolumner du vill visa");
      toolbar->addButton("tblPrint", 0, STD_PRINT, "Skriv ut tabellen (X)#Ctrl+P");
      toolbar->addButton("tblUpdate", 1, 0, "Uppdatera alla v�rden i tabellen (X)#F5");
      toolbar->addButton("tblReset", 1, 4, "�terst�ll tabeldesignen och visa allt");
      toolbar->addButton("tblCopy", 0, STD_COPY, "Kopiera selektionen till urklipp (X)#Ctrl+C");
      if (t->canPaste())
        toolbar->addButton("tblPaste", 0, STD_PASTE, "Klistra in data fr�n urklipp (X)#Ctrl+V");
      if (t->canDelete())     
       toolbar->addButton("tblRemove", 1, 1, "Ta bort valda rader fr�n tabellen (X)#Del");
      if (t->canInsert())
       toolbar->addButton("tblInsert", 1, 3, "L�gg till en ny rad i tabellen (X)#Ctrl+I");
      toolbar->createToolbar(tname, "Tabellverktyg");
    }
    else {
      toolbar->show();
    }
  }
#endif
}

void gdioutput::processToolbarMessage(const string &id, void *data) {
  if (commandLock)
    return;
  string msg;
  try {
    ButtonInfo bi;
    bi.id = id;  
    tableCB(bi, (Table *)data);    
  }
  catch(std::exception &ex) {
    msg = ex.what();
  }
  catch(...) {
    msg = "Ett ok�nt fel intr�ffade.";
  }
  
  if(!msg.empty())
    alert(msg);
}

#ifndef MEOSDB

HWND gdioutput::getToolbarWindow() const {
  if (!toolbar)
    return 0;
  return toolbar->getFloater();
}
  
bool gdioutput::hasToolbar() const {
  if (!toolbar)
    return false;
  return toolbar->isVisible();
}

void gdioutput::activateToolbar(bool active) {
  if (!toolbar)
    return;
  toolbar->activate(active);
}
#else
  HWND gdioutput::getToolbarWindow() const {
    return 0;
  }
  
  bool gdioutput::hasToolbar() const {
    return false;
  }
#endif



void gdioutput::disableTables()
{ 
  useTables=false; 
  
  for(list<ButtonInfo>::iterator bit=BI.begin(); bit != BI.end();) {
    if(bit->id.substr(0, 3)=="tbl" && bit->getExtra()!=0) {
      string id = bit->id;
      ++bit;
      removeControl(id);
    }
    else
      ++bit;
  }

}

void gdioutput::addTable(Table *t, int x, int y)
{
	TableInfo ti;
	ti.table = t;
	ti.xp = x;
  ti.yp = y;
  t->setPosition(x,y);
	//int dx, dy;

  if (t->hasAutoSelect())
    t->autoSelectColumns();
  t->autoAdjust(*this);
  //t->getDimension(*this, dx, dy, false);
  ti.table->addOwnership();
	Tables.push_back(ti);

	//updatePos(x, y, dx + TableXMargin, dy + TableYMargin);
	setRestorePoint("tblRestore");
  
  enableTables();
  updateScrollbars();	
}

void gdioutput::pasteText(const char *id)
{
	list<InputInfo>::iterator it;
	for (it=II.begin(); it != II.end(); ++it) {
		if (it->id==id) {
      SendMessage(it->hWnd, WM_PASTE, 0,0);
			return;
		}
	}
}

void *gdioutput::getExtra(const char *id) const
{
	list<InputInfo>::const_iterator it;
	for (it=II.begin(); it != II.end(); ++it) {
		if (it->id==id) 
      return it->getExtra();
	}
  return 0;
}


bool gdioutput::hasEditControl() const
{
  return !II.empty() || (Tables.size()>0 && Tables.front().table->hasEditControl());
}

void gdioutput::enableEditControls(bool enable)
{
  for (list<ButtonInfo>::iterator it=BI.begin(); it != BI.end(); ++it) {
    if (it->isEditControl)
      EnableWindow(it->hWnd, enable);
  }
 		  
  for (list<InputInfo>::iterator it=II.begin(); it != II.end(); ++it) {
    if (it->isEditControl)    
      EnableWindow(it->hWnd, enable);
  }

	for(  list<ListBoxInfo>::iterator it=LBI.begin(); it != LBI.end(); ++it) {
    if (it->isEditControl)    
      EnableWindow(it->hWnd, enable);
	}
}

void gdioutput::closeWindow() 
{
  PostMessage(hWndTarget, WM_CLOSE, 0, 0);
}

InputInfo &InputInfo::setPassword(bool pwd) {
  LONG style = GetWindowLong(hWnd, GWL_STYLE);
  if (pwd)
    style |= ES_PASSWORD;
  else
    style &= ~ES_PASSWORD;
  SetWindowLong(hWnd, GWL_STYLE, style);
  SendMessage(hWnd, EM_SETPASSWORDCHAR, 183, 0);
  return *this;
}
