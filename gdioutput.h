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
#include <hash_set>
#include <hash_map>

class Toolbar;

class gdioutput;
class oEvent;
typedef oEvent *pEvent;

struct PrinterObject;

class GDIImplFontEnum;
class GDIImplFontSet;

class Table;
class FixedTabs;

typedef int (*GUICALLBACK)(gdioutput *gdi, int type, void *data);

enum GDICOLOR;
enum KeyCommandCode;
enum gdiFonts;
#include "gdistructures.h"


#define START_YP 30
#define NOTIMEOUT 0x0AAAAAAA

typedef list<ToolInfo> ToolList;

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

class gdioutput  {
protected:
  // Flag set to true when clearPage is called.
  bool hasCleared;
  bool useTables;
  list<TextInfo> transformedPageText;
  __int64 globalCS;
  TextInfo *pageInfo;
  
  bool highContrast;

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
  stdext::hash_map<HWND, ButtonInfo *> biByHwnd;

	list<InputInfo> II;
  stdext::hash_map<HWND, InputInfo *> iiByHwnd;

	list<ListBoxInfo> LBI;
  stdext::hash_map<HWND, ListBoxInfo *> lbiByHwnd;

  list<DataStore> DataInfo;
	list<EventInfo> Events;
	list<RectangleInfo> Rectangles;
	list<TableInfo> Tables;
  list<TimerInfo> timers;

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

  map<string, GDIImplFontSet> fonts;
  const GDIImplFontSet &getCurrentFont() const;
  const GDIImplFontSet &getFont(const string &font) const;
  const GDIImplFontSet &loadFont(const string &font);
  mutable const GDIImplFontSet *currentFontSet;

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

  void resetLast() const;
  mutable int lastFormet; 
  mutable bool lastActive; 
  mutable bool lastHighlight;
  mutable DWORD lastColor;
  mutable string lastFont;

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
  vector< GDIImplFontEnum > enumeratedFonts;

  double autoSpeed;
  double autoPos;
  mutable double lastSpeed;
  mutable double autoCounter;

  bool lockRefresh;
  bool fullScreen;

  mutable bool commandLock;
  mutable DWORD commandUnlockTime;

  bool hasCommandLock() const;
  void setCommandLock() const;
  void liftCommandLock() const;

public:

  void getEnumeratedFonts(vector< pair<string, size_t> > &output) const;
  const string &getFontName(int id);
  double getRelativeFontScale(gdiFonts font, const char *fontFace) const;

  void setFullScreen(bool useFullScreen);
  void setAutoScroll(double speed);
  void getAutoScroll(double &speed, double &pos) const;
  void storeAutoPos(double pos);
  int getAutoScrollDir() const {return (autoSpeed > 0 ? 1:-1);}
  int setHighContrastMaxWidth();

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
	string browseForSave(const vector< pair<string, string> > &filter, 
                       const string &defext, int &FilterIndex);
	string browseForOpen(const vector< pair<string, string> > &filter, 
                       const string &defext);

	bool clipOffset(int PageX, int PageY, int &MaxOffsetX, int &MaxOffsetY);
  void addRectangle(RECT &rc, GDICOLOR Color = GDICOLOR(-1), 
                    bool DrawBorder = true, bool addFirst = false);
  DWORD makeEvent(const string &id, const string &origin, 
                  DWORD data, void *extra, bool flushEvent);
	
  void unregisterEvent(const string &id);
  EventInfo &registerEvent(const string &id, GUICALLBACK cb);

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
  void calcStringSize(TextInfo &ti, HDC hDC=0) const;
  void formatString(const TextInfo &ti, HDC hDC) const;

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
  int getLineHeight(gdiFonts font, const char *face) const;

  BaseInfo *setInputFocus(const string &id, bool select=false);  
  InputInfo *getInputFocus();
	
  void enableInput(const char *id, bool acceptMissing = false) {setInputStatus(id, true, acceptMissing);}
  void disableInput(const char *id, bool acceptMissing = false) {setInputStatus(id, false, acceptMissing);}
  void setInputStatus(const char *id, bool status, bool acceptMissing = false);
  void setInputStatus(const string &id, bool status, bool acceptMissing = false)
    {setInputStatus(id.c_str(), status, acceptMissing);}

	void setTabStops(const string &Name, int t1, int t2=-1);
	void setData(const string &id, DWORD data);
	void setData(const string &id, void *data);
	void *getData(const string &id) const; 
	
  bool getData(const string &id, DWORD &data) const; 
	bool hasData(const char *id) const;

  bool selectItemByData(const char *id, int data);
  void removeSelected(const char *id);

  enum AskAnswer {AnswerNo = 0, AnswerYes = 1, AnswerCancel = 2};
  bool ask(const string &s);
	AskAnswer askCancel(const string &s);
	
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
	void adjustDimension(int width, int height);
  
  bool getSelectedItem(string id, ListBoxInfo *lbi);
	bool addItem(const string &id, const string &text, size_t data = 0);
	bool addItem(const string &id, const vector< pair<string, size_t> > &items);
  void filterOnData(const string &id, const stdext::hash_set<int> &filter);

  bool clearList(const string &id);

  bool hasField(const string &id) const;
	const string &getText(const char *id, bool acceptMissing = false) const;
	int getTextNo(const char *id, bool acceptMissing = false) const;
  int getTextNo(const string &id, bool acceptMissing = false) const
    {return getTextNo(id.c_str(), acceptMissing);}
  
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
  TextInfo &addString(const char *id, int yp, int xp, int format, const string &text, 
                      int xlimit=0, GUICALLBACK cb=0, const char *fontFace = 0);
	// Untranslated versions
  TextInfo &addStringUT(int yp, int xp, int format, const string &text, 
                        int xlimit=0, GUICALLBACK cb=0, const char *fontFace = 0);
  TextInfo &addStringUT(int format, const string &text, GUICALLBACK cb=0);

	void addTimer(int yp, int xp, int format, DWORD ZeroTime, int xlimit=0, GUICALLBACK cb=0, int TimeOut=NOTIMEOUT);
  void addTimeout(int TimeOut, GUICALLBACK cb);
  TimerInfo &addTimeoutMilli(int timeOut, const string &id, GUICALLBACK cb);
  void timerProc(TimerInfo &timer, DWORD timeout);

	void draw(HDC hDC, RECT &rc);

  void closeWindow();

	friend int TablesCB(gdioutput *gdi, int type, void *data);
	friend class Table;

	gdioutput(double _scale);
  gdioutput(double _scale, const PrinterObject &defprn);
	virtual ~gdioutput();
};

#endif // !defined(AFX_GDIOUTPUT_H__396F60F8_679F_498A_B759_DF8F6F346A4A__INCLUDED_)
