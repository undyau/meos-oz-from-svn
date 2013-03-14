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

class spkClassSelection {
  // The currently selected leg 
  int selectedLeg;
  // Given a leg, get the corresponding control to watch
  map<int, int> legToControl;
public:
  spkClassSelection() : selectedLeg(0) {}
  void setLeg(int leg) {selectedLeg=leg;}
  int getLeg() const {return selectedLeg;}

  void setControl(int controlId) { 
    legToControl[selectedLeg]=controlId;
  }  
  int getControl()  { 
    if(legToControl.count(selectedLeg)==1)
      return legToControl[selectedLeg];
    else return -1;
  }
};

class TabSpeaker :
	public TabBase {
private:
  set<int> controlsToWatch;
  set<int> classesToWatch;
  set<int> controlsToWatchSI;
  
  int lastControlToWatch;
  int lastClassToWatch;

  set<__int64> shownEvents;
  vector<oTimeLine> events;
  bool watchEvents;
  oTimeLine::Priority watchLevel;
  int watchNumber;

  void generateControlList(gdioutput &gdi, int classId);

  string lastControl;
  
  void manualTimePage(gdioutput &gdi) const;
  void storeManualTime(gdioutput &gdi);
  
  //Curren class-
  int classId;
  //Map CourseNo -> selected Control.
  //map<int, int> selectedControl;
  map<int, spkClassSelection> selectedControl;

  bool ownWindow;

  void drawTimeLine(gdioutput &gdi);
  void splitAnalysis(gdioutput &gdi, int xp, int yp, pRunner r);

  // Runner Id:s to set priority for
  vector<int> runnersToSet;

public:

  bool onClear(gdioutput &gdi);
  void loadPriorityClass(gdioutput &gdi, int classId);
  void savePriorityClass(gdioutput &gdi);

  void updateTimeLine(gdioutput &gdi);

  //Clear selection data
  void clear();

 	int processButton(gdioutput &gdi, const ButtonInfo &bu);
	int processListBox(gdioutput &gdi, const ListBoxInfo &bu);
  int handleEvent(gdioutput &gdi, const EventInfo &ei);
	
  bool loadPage(gdioutput &gdi);
	TabSpeaker(oEvent *oe);
	~TabSpeaker(void);
};
