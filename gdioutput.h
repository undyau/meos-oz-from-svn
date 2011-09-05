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

// gdioutput.h: interface for the gdioutput class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_GDIOUTPUT_H__396F60F8_679F_498A_B759_DF8F6F346A4A__INCLUDED_)
#define AFX_GDIOUTPUT_H__396F60F8_679F_498A_B759_DF8F6F346A4A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <set>
#include <map>
#include <vector>

enum gdiFonts {  
  normalText=0,
  boldText=1,
  boldLarge=2,
  boldHuge=3,
  boldSmall=5,

  fontLarge=11,
  fontMedium=12,
  fontMediumPlus=14,
  fontSmall=13,

  italicSmall = 15,
  formatIgnore = 1000,
};

enum KeyCommandCode {
  KC_COPY,
  KC_PASTE,
  KC_DELETE,
  KC_INSERT,
  KC_PRINT,
  KC_REFRESH
};

const int pageNewPage=100;
const int pageReserveHeight=101;
const int pagePageInfo=102;

const int textRight=256;
const int textCenter=512;
const int timerCanBeNegative=1024;
const int breakLines=2048;

class Toolbar;

class gdioutput;
typedef int (*GUICALLBACK)(gdioutput *gdi, int type, void *data);

enum {GUI_BUTTON=1, GUI_INPUT=2, GUI_LISTBOX=3, 
GUI_INFOBOX=4, GUI_CLEAR=5, GUI_INPUTCHANGE=6, 
GUI_COMBOCHANGE, GUI_EVENT, GUI_LINK, 
GUI_TIMEOUT, GUI_POSTCLEAR, GUI_FOCUS};

enum GDICOLOR {colorRed = RGB(128,0,0), 
              colorGreen = RGB(0,128,0),
              colorDarkGrey = RGB(40,40,40),
              colorDarkRed = RGB(64,0,0),
              colorGreyBlue = RGB(92,92,128),
              colorDarkBlue = RGB(0,0,92),
              colorDarkGreen = RGB(0,64,0),
              colorYellow = RGB(255, 230, 0),
              colorLightBlue = RGB(240,240,255),
              colorLightRed = RGB(255,230,230),
              colorLightGreen = RGB(180, 255, 180),
              colorLightYellow = RGB(255, 255, 200),
              colorLightCyan = RGB(200, 255, 255),
              colorLightMagenta = RGB(255, 200, 255),
              colorDefault = -1};

class oEvent;
typedef oEvent *pEvent;

class Table;

class FixedTabs;

class BaseInfo
{
protected:
	void *extra;
public:
	BaseInfo():extra(0) {}
	virtual ~BaseInfo() {}
	string id;

	BaseInfo &setExtra(void *e) {extra=e; return *this;}
	void *getExtra() const {return extra;}
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
	int sCX;
	int sCY;
	int sMX;
	int sMY;
  int sOX;
  int sOY;

  int nTooltip;

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
	int getHeight() {return max(int(TextRect.bottom-TextRect.top), reserveHeight);}

	string text;
	
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
            flags(0), originalState(false), isEditControl(false), isCheckbox(false){}

  ButtonInfo &isEdit(bool e) {isEditControl=e; return *this;}

	int xp;
	int yp;
	string text;
	HWND hWnd;
	bool AbsPos;
  bool fixedRightTop;
  int flags;
  bool isCheckbox;
  bool defaultButton() {return (flags&1)==1;}
  bool cancelButton() {return (flags&2)==2;}
  ButtonInfo &setDefault();// {flags|=1;}
  ButtonInfo &setCancel() {flags|=2; return *this;}
  ButtonInfo &fixedCorner() {fixedRightTop = true; return *this;}
	GUICALLBACK CallBack;
  friend class gdioutput;
};

class InputInfo : public BaseInfo
{	
public:
  InputInfo() : hWnd(0), CallBack(0), ignoreCheck(false), isEditControl(true), bgColor(colorDefault) {}
	int xp;
	int yp;
  double width;
  double height;
	string text;
  bool changed() const {return text!=original;}
  void ignore(bool ig) {ignoreCheck=ig;}
  InputInfo &isEdit(bool e) {isEditControl=e; return *this;}
  InputInfo &setBgColor(GDICOLOR c) {bgColor = c; return *this;}
  InputInfo &setPassword(bool pwd);
  HWND hWnd;
	GUICALLBACK CallBack;
private:
  GDICOLOR bgColor;
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
	DWORD data;
};

class EventInfo : public BaseInfo
{
public:
	EventInfo() : CallBack(0) {}
	DWORD data;
	GUICALLBACK CallBack;
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

struct PrinterObject {
	//Printing
  HDC hDC;
	HGLOBAL hDevMode;
	HGLOBAL hDevNames;
	
  void freePrinter();

  string Device;
  string Driver;
  DEVMODE DevMode;
  set<__int64> printedPages;
  int nPagesPrinted;
  int nPagesPrintedTotal;
  bool onlyChanged;

  struct DATASET {
	  int pWidth_mm;
	  int pHeight_mm;
	  double pMgBottom;
	  double pMgTop;
	  double pMgRight;
	  double pMgLeft;

	  int MarginX;
	  int MarginY;
	  int PageX;
	  int PageY;
	  double Scale;
	  bool LastPage;
  } ds;

  void operator=(const PrinterObject &po);

  PrinterObject();
  ~PrinterObject();
  PrinterObject(const PrinterObject &po);
};

#define START_YP 30
#define NOTIMEOUT 0x0AAAAAAA

struct ToolInfo {
  TOOLINFO ti;
  string tip;
  int id;
};

typedef list<ToolInfo> ToolList;

class gdioutput  {
protected:
  // Flag set to true when clearPage is called.
  bool hasCleared;
  mutable bool commandLock;
  bool useTables;
  list<TextInfo> transformedPageText;
  __int64 globalCS;
  TextInfo pageInfo;
  bool printHeader;
  bool noPrintMargin;
  void deleteFonts();
  void constructor(double _scale);

  void updateStringPosCache();
  vector<TextInfo *> shownStrings;

  void CalculateCS(TextInfo &text);
  void printPage(PrinterObject &po, int StartY, int &EndY, bool calculate);
  void printPage(PrinterObject &po, int nPage, int nPageMax);	
  bool startDoc(PrinterObject &po);

  bool getSelectedItem(ListBoxInfo &lbi);

	//void printPage(PrinterObject &po, int StartY, int &EndY);
  bool doPrint(PrinterObject &po, pEvent oe);

	PrinterObject po_default;

  void restoreInternal(const RestoreInfo &ri);

	void drawCloseBox(HDC hDC, RECT &Close, bool pressed);

  void setFontCtrl(HWND hWnd);

	list<TextInfo> TL;

  //True if textlist has increasing y-values so 
  //that we can optimize rendering.
  bool renderOptimize; 
  //Stored iterator used to optimize rendering
  //by avoiding to loop through complete TL.
  list<TextInfo>::iterator itTL;

	list<ButtonInfo> BI;
	list<InputInfo> II;
	list<ListBoxInfo> LBI;
	list<DataStore> DataInfo;
	list<EventInfo> Events;
	list<RectangleInfo> Rectangles;
	list<TableInfo> Tables;
  Toolbar *toolbar;
  ToolList toolTips;

  map<string, RestoreInfo> restorePoints;

	GUICALLBACK onClear;
	GUICALLBACK postClear;

	list<InfoBox> IBox;

	list<HWND> FocusList;
	HWND CurrentFocus;


	int lineHeight;
	HWND hWndTarget;
	HWND hWndAppMain;
	HWND hWndToolTip;
	HWND hWndTab;

	HBRUSH Background;
	HFONT Huge;
	HFONT Large;
	HFONT Medium;
	HFONT Small;

	HFONT pfLarge;
	HFONT pfMedium;
	HFONT pfSmall;
	HFONT pfMediumPlus;

  HFONT pfSmallItalic;
	
	int MaxX;
	int MaxY;
	int CurrentX;
	int CurrentY;
	int SX;
	int SY;

	int Direction;

	int OffsetY; //Range 0 -- MaxY
	int OffsetX; //Range 0 -- MaxX

  //Set to true if we should not update window during "addText" operations
  bool manualUpdate;
  
  LRESULT ProcessMsgWrp(UINT iMessage, LPARAM lParam, WPARAM wParam);
	void getWindowText(HWND hWnd, string &text);
  double scale;
  HFONT getGUIFont() const;

  void resetLast();
  int lastFormet; 
  bool lastActive; 
  bool lastHighlight;
  DWORD lastColor;
  void initCommon(double scale, const string &font);

  void processButtonMessage(ButtonInfo &bi, DWORD wParam);
  void processEditMessage(InputInfo &bi, DWORD wParam);
  void processComboMessage(ListBoxInfo &bi, DWORD wParam);
  void processListMessage(ListBoxInfo &bi, DWORD wParam);

  void doEnter();
  void doEscape();
  bool doUpDown(int direction);
	
  FixedTabs *tabs;

  string currentFont;
public:
  HWND getToolbarWindow() const;
  bool hasToolbar() const;
  void activateToolbar(bool active);

  void processToolbarMessage(const string &id, void *data);

  void synchronizeListScroll(const string &id1, const string &id2);

  FixedTabs &getTabs();

  // True if up/down is locked, i.e, don't move page
  bool lockUpDown;
      

  double getScale() const {return scale;}
  void enableEditControls(bool enable);

  bool hasEditControl() const;

  void setFont(int size, const string &font);

  int getButtonHeight() const;   
  int scaleLength(int input) const {return int(scale*input + 0.5);}
  void getPrinterSettings(PrinterObject &po);

  void tableCB(ButtonInfo &bu, Table *t); 

  void *getExtra(const char *id) const;

  void enableTables(); 
  void disableTables();

  void pasteText(const char *id);

  bool writeHTML(const string &file, const string &title) const;
  bool writeTableHTML(const string &file, const string &title) const;

  void print(pEvent oe, Table *t=0, bool printMeOSHeader=true, bool noMargin=false);
  void print(PrinterObject &po, pEvent oe, bool printMeOSHeader=true, bool noMargin=false);
  void printSetup(PrinterObject &po);
  void destroyPrinterDC(PrinterObject &po);

  void setSelection(const string &id, const set<int> &selection);
  void getSelection(const string &id, set<int> &selection);

	HWND getTarget(){return hWndTarget;}
	HWND getMain(){return hWndAppMain;}

	string browseForFolder(const string &FolderStart);
	void scrollToBottom();
  void scrollTo(int x, int y);
	void setOffset(int x, int y, bool update);

	void selectTab(int Id);

	void addTable(Table *table, int x, int y);
  Table &getTable() const; //Get the (last) table. If needed, add support for named tables... 

	void addToolTip(const string &Tip, HWND hWnd, RECT *rc=0);
	HWND getToolTip(){return hWndToolTip;}
	
  void init(HWND hWnd, HWND hMainApp, HWND hTab);
	bool openDoc(const char *doc);
	string browseForSave(const string &Filter, const string &defext, int &FilterIndex);
	string browseForOpen(const string &Filter, const string &defext);

	bool clipOffset(int PageX, int PageY, int &MaxOffsetX, int &MaxOffsetY);
  void addRectangle(RECT &rc, GDICOLOR Color = colorDefault, bool DrawBorder=true);
	DWORD makeEvent(const string &id, DWORD data, void *extra=0);
	
  void unregisterEvent(const string &id);
  void registerEvent(const string &id, GUICALLBACK cb);

	int sendCtrlMessage(const string &id);
	bool canClear();
	void setOnClearCb(GUICALLBACK cb);
	void setPostClearCb(GUICALLBACK cb);

  void restore(const string &id="", bool DoRefresh=true);
  
  /// Restore, but do not update client area size,
  /// position, zoom, scrollbars, and do not refresh
  void restoreNoUpdate(const string &id);

  void setRestorePoint();
  void setRestorePoint(const string &id);

	bool removeControl(const string &id);
	bool hideControl(const string &id);
	
  void CheckInterfaceTimeouts(DWORD T);
	bool RemoveFirstInfoBox(const string &id);
	void drawBoxText(HDC hDC, RECT &tr, InfoBox &Box, bool highligh);
	void drawBoxes(HDC hDC, RECT &rc);
	void drawBox(HDC hDC, InfoBox &Box, RECT &pos);
	void addInfoBox(string id, string text, int TimeOut=0, GUICALLBACK cb=0);
	HWND getHWND() const {return hWndTarget;}
	void updateObjectPositions();
	void drawBackground(HDC hDC, RECT &rc);
	void updateScrollbars() const;

	void SetOffsetY(int oy){OffsetY=oy;}
  void SetOffsetX(int ox){OffsetX=ox;}
	int GetPageY(){return max(MaxY, 100)+60;}
  int GetPageX(){return max(MaxX, 100)+100;}
  int GetOffsetY(){return OffsetY;}
  int GetOffsetX(){return OffsetX;}

  void RenderString(TextInfo &ti, const string &text, HDC hDC);
	void RenderString(TextInfo &ti, HDC hDC=0);
  void calcStringSize(TextInfo &ti, HDC hDC=0);
  void formatString(const TextInfo &ti, HDC hDC);

	string getTimerText(TextInfo *tit, DWORD T);
	string getTimerText(int ZeroTime, int format);
	
  
	void fadeOut(string Id, int ms);
	void setWaitCursor(bool wait);
	void setWindowTitle(const string &title);
	bool selectFirstItem(const string &name);
	void removeString(string Id);
	void refresh() const;
  void refreshFast() const;

	void dropLine(double lines=1){CurrentY+=int(lineHeight*lines); MaxY=max(MaxY, CurrentY);}
	int getCX() const {return CurrentX;}
	int getCY() const {return CurrentY;}
	int getWidth() const {return MaxX;}
	int getHeight() const {return MaxY;}
	void getTargetDimension(int &x, int &y) const;

	void setCX(int cx){CurrentX=cx;}
	void setCY(int cy){CurrentY=cy;}
	int getLineHeight() const {return lineHeight;}

  BaseInfo *setInputFocus(const string &id, bool select=false);  
  InputInfo *getInputFocus();
	
  void enableInput(const char *id) {setInputStatus(id, true);}
  void disableInput(const char *id) {setInputStatus(id, false);}
  void setInputStatus(const char *id, bool status);
  void setInputStatus(const string &id, bool status)
    {setInputStatus(id.c_str(), status);}

	void setTabStops(const string &Name, int t1, int t2=-1);
	void setData(const string &id, DWORD data);
	bool getData(const string &id, DWORD &data); 
	bool selectItemByData(const char *id, int data);
  void removeSelected(const char *id);

  bool ask(const string &s);
	void alert(const string &msg) const;
	void fillDown(){Direction=1;}
	void fillRight(){Direction=0;}
  void fillNone(){Direction=-1;}
	void newColumn(){CurrentY=START_YP; CurrentX=MaxX+10;}
	void newRow(){CurrentY=MaxY; CurrentX=10;}

	void pushX(){SX=CurrentX;}
	void pushY(){SY=CurrentY;}
	void popX(){CurrentX=SX;}
	void popY(){CurrentY=SY;}

	void updatePos(int x, int y, int width, int height);
	bool getSelectedItem(string id, ListBoxInfo *lbi);
	bool addItem(const string &id, const string &text, size_t data = 0);
	bool addItem(const string &id, const vector< pair<string, size_t> > &items);
  bool clearList(const string &id);

  bool hasField(const string &id) const;
	const string &getText(const char *id, bool acceptMissing = false) const;
	int getTextNo(const char *id, bool acceptMissing = false) const;
  const string &getText(const string &id, bool acceptMissing = false) const
  {return getText(id.c_str(), acceptMissing);}

  // Insert text and notify "focusList"
	bool insertText(const string &id, const string &text);

	BaseInfo *setText(const char *id, const string &text, bool update=false);
	BaseInfo *setText(const char *id, int number, bool update=false);
  BaseInfo *setTextZeroBlank(const char *id, int number, bool update=false);
	BaseInfo *setText(const string &id, const string &text, bool update=false)
    {return setText(id.c_str(), text, update);}
	BaseInfo *setText(const string &id, int number, bool update=false)
	  {return setText(id.c_str(), number, update);}
  
  void clearPage(bool autoRefresh, bool keepToolbar = false);

	void TabFocus(int direction=1);
	void Enter();
  void Escape();
  bool UpDown(int direction);
	void keyCommand(KeyCommandCode code);

	LRESULT ProcessMsg(UINT iMessage, LPARAM lParam, WPARAM wParam);
	void SetWindow(HWND hWnd){hWndTarget=hWnd;}
	
  void scaleSize(double scale);

	ButtonInfo &addButton(const string &id, const string &text, GUICALLBACK cb, const string &tooltip="");

	ButtonInfo &addButton(int x, int y, const string &id, const string &text, 
                        GUICALLBACK cb, const string &tooltop="");
	ButtonInfo &addButton(int x, int y, int w, const string &id, const string &text, 
                        GUICALLBACK cb, const string &tooltop, bool AbsPos, bool hasState);

  ButtonInfo &addCheckbox(const string &id, const string &text, GUICALLBACK cb=0, bool Checked=true, const string &Help="");
  ButtonInfo &addCheckbox(int x, int y, const string &id, const string &text, GUICALLBACK cb=0, bool Checked=true, const string &Help="", bool AbsPos=false);
  bool isChecked(const string &id);
  void check(const string &id, bool State);

  bool isInputChanged(const string &exclude);

  InputInfo &addInput(const string &id, const string &text="", int length=16, GUICALLBACK cb=0, const string &Explanation="", const string &tooltip="");
	InputInfo &addInput(int x, int y, const string &id, const string &text, int length, GUICALLBACK cb=0, const string &Explanation="", const string &tooltip="");
	
  InputInfo &addInputBox(const string &id, int width, int height, const string &text, 
                         GUICALLBACK cb, const string &Explanation);
	
  InputInfo &addInputBox(const string &id, int x, int y, int width, int height,
                         const string &text, GUICALLBACK cb, const string &Explanation);

  ListBoxInfo &addListBox(const string &id, int width, int height, GUICALLBACK cb=0, const string &Explanation="", const string &tooltip="", bool multiple=false);
  ListBoxInfo &addListBox(int x, int y, const string &id, int width, int height, GUICALLBACK cb=0, const string &Explanation="", const string &tooltip="", bool multiple=false);
	
	ListBoxInfo &addSelection(const string &id, int width, int height, GUICALLBACK cb=0, const string &Explanation="", const string &tooltip="");
	ListBoxInfo &addSelection(int x, int y, const string &id, int width, int height, GUICALLBACK cb=0, const string &Explanation="", const string &tooltip="");

	ListBoxInfo &addCombo(const string &id, int width, int height, GUICALLBACK cb=0, const string &Explanation="", const string &tooltip="");
	ListBoxInfo &addCombo(int x, int y, const string &id, int width, int height, GUICALLBACK cb=0, const string &Explanation="", const string &tooltip="");

	TextInfo &addString(const char *id, int format, const string &text, GUICALLBACK cb=0);
  TextInfo &addString(const char *id, int yp, int xp, int format, const string &text, int xlimit=0, GUICALLBACK cb=0);
	// Untranslated versions
  TextInfo &addStringUT(int yp, int xp, int format, const string &text, int xlimit=0, GUICALLBACK cb=0);
  TextInfo &addStringUT(int format, const string &text, GUICALLBACK cb=0);

	void addTimer(int yp, int xp, int format, DWORD ZeroTime, int xlimit=0, GUICALLBACK cb=0, int TimeOut=NOTIMEOUT);
  void addTimeout(int TimeOut, GUICALLBACK cb);

	void draw(HDC hDC, RECT &rc);

  void closeWindow();

	friend int TablesCB(gdioutput *gdi, int type, void *data);
	friend class Table;

	gdioutput(double _scale);
  gdioutput(double _scale, const PrinterObject &defprn);
	virtual ~gdioutput();
};

#endif // !defined(AFX_GDIOUTPUT_H__396F60F8_679F_498A_B759_DF8F6F346A4A__INCLUDED_)
