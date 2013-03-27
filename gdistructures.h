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

#ifndef GDI_STRUCTURES
#define GDI_STRUCTURES

enum {GUI_BUTTON=1, GUI_INPUT=2, GUI_LISTBOX=3, 
GUI_INFOBOX=4, GUI_CLEAR=5, GUI_INPUTCHANGE=6, 
GUI_COMBOCHANGE, GUI_EVENT, GUI_LINK, 
GUI_TIMEOUT, GUI_POSTCLEAR, GUI_FOCUS, GUI_TIMER};

class BaseInfo
{
protected:
	void *extra;
public:
	BaseInfo():extra(0) {}
	virtual ~BaseInfo() {}
	string id;

	BaseInfo &setExtra(void *e) {extra=e; return *this;}
  BaseInfo &setExtra(const void *e) {extra=(void *)e; return *this;}
  
  BaseInfo &setExtra(int e) {extra = (void *)(e); return *this;}
  BaseInfo &setExtra(size_t e) {extra = (void *)(e); return *this;}
  
  void *getExtra() const {return extra;}
  int getExtraInt() const {return int(extra);}
  size_t getExtraSize() const {return size_t(extra);}
};

class RestoreInfo : public BaseInfo
{
public:
	int nLBI;
	int nBI;
	int nII;
	int nTL;
	int nRect;		
	int nHWND;
  int nData;

	int sCX;
	int sCY;
	int sMX;
	int sMY;
  int sOX;
  int sOY;

  int nTooltip;
  int nTables;

  GUICALLBACK onClear;
	GUICALLBACK postClear;
};

class RectangleInfo : public BaseInfo
{
private:
  DWORD color;  
	bool drawBorder;
	RECT rc;

public:
	RectangleInfo(): color(0), drawBorder(false) {memset(&rc, 0, sizeof(RECT));}
  RectangleInfo &setColor(GDICOLOR c) {color = c; return *this;}	
  friend class gdioutput;
};


class TableInfo : public BaseInfo
{
public:
	TableInfo():xp(0), yp(0), table(0) {}
	int xp;
	int yp;
	Table *table;	
};


class TextInfo : public BaseInfo
{
public:
  
	TextInfo():format(0), color(0), xlimit(0), HasTimer(false),
		HasCapture(false), CallBack(0), Highlight(false), 
		Active(false), reserveHeight(0) {}

  TextInfo &setColor(GDICOLOR c) {color = c; return *this;}
	TextInfo &changeFont(const string &fnt) {font = fnt; return *this;} //Note: size not updated
	
  int getHeight() {return max(int(TextRect.bottom-TextRect.top), reserveHeight);}
  gdiFonts getGdiFont() const {return gdiFonts(format & 0xFF);}
	string text;
	string font;

	int xp;
	int yp;

	int format;	
	DWORD color;
	int xlimit;

	bool HasTimer;
	DWORD ZeroTime;
	DWORD TimeOut;

	bool HasCapture;
	GUICALLBACK CallBack;
	RECT TextRect;
	bool Highlight;
	bool Active;
  int reserveHeight;
};

class ButtonInfo : public BaseInfo
{	
private:
  bool originalState;
  bool isEditControl;
public:
	ButtonInfo(): CallBack(0), hWnd(0), AbsPos(false), fixedRightTop(false),
            flags(0), storedFlags(0), originalState(false), isEditControl(false), isCheckbox(false){}

  ButtonInfo &isEdit(bool e) {isEditControl=e; return *this;}

	int xp;
	int yp;
	string text;
	HWND hWnd;
	bool AbsPos;
  bool fixedRightTop;
  int flags;
  int storedFlags;
  bool isCheckbox;
  bool isDefaultButton() const {return (flags&1)==1;}
  bool isCancelButton() const {return (flags&2)==2;}

  void moveButton(gdioutput &gdi, int xp, int yp);
  void getDimension(gdioutput &gdi, int &w, int &h);

  ButtonInfo &setDefault();
  ButtonInfo &setCancel() {flags|=2, storedFlags|=2; return *this;}
  ButtonInfo &fixedCorner() {fixedRightTop = true; return *this;}
	GUICALLBACK CallBack;
  friend class gdioutput;
};

class InputInfo : public BaseInfo
{	
public:
  InputInfo();
	int xp;
	int yp;
  double width;
  double height;
	string text;
  bool changed() const {return text!=original;}
  void ignore(bool ig) {ignoreCheck=ig;}
  InputInfo &isEdit(bool e) {isEditControl=e; return *this;}
  InputInfo &setBgColor(GDICOLOR c) {bgColor = c; return *this;}
  InputInfo &setFgColor(GDICOLOR c) {fgColor = c; return *this;}
  InputInfo &setPassword(bool pwd);
  HWND hWnd;
	GUICALLBACK CallBack;
private:
  GDICOLOR bgColor;
  GDICOLOR fgColor;
  bool isEditControl;
  bool writeLock;
  string original;
  bool ignoreCheck; // True if changed-state should be ignored
  friend class gdioutput;
};

class ListBoxInfo : public BaseInfo
{
public:
	ListBoxInfo() : hWnd(0), CallBack(0), IsCombo(false), index(-1), 
              writeLock(false), ignoreCheck(false), isEditControl(true),
              originalProc(0), lbiSync(0) {}
	int xp;
	int yp;
  double width;
  double height;
	HWND hWnd;
	string text;
  bool changed() const {return text!=original;}
	size_t data;
	bool IsCombo;
  int index;
	GUICALLBACK CallBack;
  void ignore(bool ig) {ignoreCheck=ig;}
  ListBoxInfo &isEdit(bool e) {isEditControl=e; return *this;}

private:
  bool isEditControl;
  bool writeLock;
  string original;
  int originalIdx;
  bool ignoreCheck; // True if changed-state should be ignored

  // Synchronize with other list box
  WNDPROC originalProc;
  ListBoxInfo *lbiSync;

  friend LRESULT CALLBACK GetMsgProc(HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam);
  friend class gdioutput;
};

class DataStore
{
public:
	string id;
	void *data;
};

class EventInfo : public BaseInfo
{
private:
  string origin;
  DWORD data;
  KeyCommandCode keyEvent;
public:
  KeyCommandCode getKeyCommand() const {return keyEvent;}
  DWORD getData() const {return data;}
  void setKeyCommand(KeyCommandCode kc) {keyEvent = kc;}
	void setData(const string &origin_, DWORD d) {origin = origin_, data = d;}
  const string &getOrigin() {return origin;}
  EventInfo();
	GUICALLBACK CallBack;
};

class TimerInfo : public BaseInfo
{
private:
  DWORD data;
  gdioutput *parent;
  TimerInfo(gdioutput *gdi, GUICALLBACK cb) : parent(gdi), CallBack(cb) {}
  
public:
 	GUICALLBACK CallBack;
  friend class gdioutput;
  friend void CALLBACK gdiTimerProc(HWND hWnd, UINT a, UINT_PTR ptr, DWORD b);
};


class InfoBox : public BaseInfo
{
public:
	InfoBox() : CallBack(0), HasCapture(0), HasTCapture(0), TimeOut(0) {}
	string text;	
	GUICALLBACK CallBack;

	RECT TextRect;
	RECT Close;
  RECT BoundingBox;

	bool HasCapture;
	bool HasTCapture;

	DWORD TimeOut;
}; 

typedef list<TextInfo> TIList;

struct ToolInfo {
  TOOLINFO ti;
  string tip;
  int id;
};


#endif