#pragma once
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
    Stigbergsvägen 7, SE-75242 UPPSALA, Sweden
    
************************************************************************/

#include "tabbase.h"
#include "gdioutput.h"
#include <string>
#include "oListInfo.h"

using namespace std;

void tabAutoTimer(gdioutput &gdi);
class TabAuto;
class gdioutput;
class oEvent;

enum AutoSyncType {SyncNone, SyncTimer, SyncDataUp};

class AutoMachine
{
public:
	string name;
	DWORD interval; //Interval seconds
	DWORD timeout; //Timeout (TickCount)
  bool synchronize;
  bool synchronizePunches;
	virtual void process(gdioutput &gdi, oEvent *oe, AutoSyncType ast) = 0;
	virtual void status(gdioutput &gdi) = 0;
	virtual bool stop() {return true;}
  virtual AutoMachine *clone() const = 0;
  AutoMachine(const string &s) : name(s), interval(0), timeout(0), 
                      synchronize(false), synchronizePunches(false) {}
	virtual ~AutoMachine() = 0 {}
};

class PrintResultMachine : 
	public AutoMachine
{
protected:
  string exportFile;
  string exportScript;
  bool doExport;
  bool doPrint;
  bool structuredExport;
  PrinterObject po;
  set<int> classesToPrint;
  bool pageBreak;
  bool showInterResult;
  bool splitAnalysis;
  bool notShown;
  oListInfo listInfo;
  bool readOnly;
  bool lock; // true while printing
public:
  PrintResultMachine *clone() const {
    PrintResultMachine *prm = new PrintResultMachine(*this); 
    prm->lock = false; 
    return prm;
  }
	void status(gdioutput &gdi);
	void process(gdioutput &gdi, oEvent *oe, AutoSyncType ast);
  PrintResultMachine(int v):AutoMachine("Resultatutskrift") {
    interval=v; 
    pageBreak = true; 
    showInterResult = true;
    notShown = true;
    splitAnalysis = true;
    lock = false;
    readOnly = false;
    doExport = false;
    doPrint = true;
    structuredExport = true;
  }
  PrintResultMachine(int v, const oListInfo &li):AutoMachine("Utskrift / export"), listInfo(li) {
    interval=v; 
    pageBreak = true; 
    showInterResult = true;
    notShown = true;
    splitAnalysis = true;
    lock = false;
    readOnly = true;
    doExport = false;
    doPrint = true;
    structuredExport = true;
  }
  friend class TabAuto;
};


class PrewarningMachine : 
	public AutoMachine
{
protected:
  string waveFolder;
  set<int> controls;
  set<int> controlsSI;
public:
  PrewarningMachine *clone() const {return new PrewarningMachine(*this);} 
	void status(gdioutput &gdi);
	void process(gdioutput &gdi, oEvent *oe, AutoSyncType ast);
	PrewarningMachine():AutoMachine("Förvarningsröst") {}
  friend class TabAuto;
};

class MySQLReconnect : 
	public AutoMachine
{
protected:
  int callCount;
  HANDLE hThread;
public:
  MySQLReconnect *clone() const {return new MySQLReconnect(*this);} 
	void status(gdioutput &gdi);
	void process(gdioutput &gdi, oEvent *oe, AutoSyncType ast);
  bool stop();
	MySQLReconnect();
  virtual ~MySQLReconnect();
  friend class TabAuto;
};

bool isThreadReconnecting();

class PunchMachine : 
	public AutoMachine
{
protected:
  int radio;
public:
  PunchMachine *clone() const {return new PunchMachine(*this);} 
	void status(gdioutput &gdi);
	void process(gdioutput &gdi, oEvent *oe, AutoSyncType ast);
	PunchMachine():AutoMachine("Stämplingsautomat"), radio(0) {}
  friend class TabAuto;
};

class SplitsMachine : 
	public AutoMachine
{
protected:
  string file;
  set<int> classes;
  int leg;
public:
  SplitsMachine *clone() const {return new SplitsMachine(*this);} 
	void status(gdioutput &gdi);
	void process(gdioutput &gdi, oEvent *oe, AutoSyncType ast);
	SplitsMachine() : AutoMachine("Sträcktider/WinSplits"), leg(-1) {}
  friend class TabAuto;
};


class TabAuto :
	public TabBase
{
private:
	//DWORD printResultIntervalSec;
	//DWORD printResultTimeOut;
	bool editMode;

  bool synchronize;
  bool synchronizePunches;
  void updateSyncInfo();

	list<AutoMachine *> Machines;
	void setTimer(AutoMachine *am);

	void timerCallback(gdioutput &gdi);
  void syncCallback(gdioutput &gdi);
public:
	
	AutoMachine *getMachine(const string &name);
	bool stopMachine(AutoMachine *am);
  void killMachines();
  void addMachine(const AutoMachine &am)
  {
    Machines.push_back(am.clone());
    setTimer(Machines.back());
  }

	int processButton(gdioutput &gdi, const ButtonInfo &bu);
	int processListBox(gdioutput &gdi, const ListBoxInfo &bu);

	bool loadPage(gdioutput &gdi);
	TabAuto(oEvent *poe);
	~TabAuto(void);

  friend void tabAutoSync(const vector<gdioutput *> &gdi, pEvent oe);
  friend void tabForceSync(gdioutput &gdi, pEvent oe);
	friend void tabAutoTimer(gdioutput &gdi);
};

void tabAutoKillMachines();
void tabAutoRegister(TabAuto *ta);
void tabAutoAddMachinge(const AutoMachine &am);
