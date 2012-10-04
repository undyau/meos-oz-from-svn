/************************************************************************
    MeOS - Orienteering Software
    Copyright (C) 2009-2012 Melin Software HB
    
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

#include "resource.h"

#include <commctrl.h>
#include <commdlg.h> 

#include "oEvent.h"
#include "xmlparser.h"
#include "gdioutput.h"
#include "gdifonts.h"

#include "csvparser.h"
#include "SportIdent.h"
#include "meos_util.h"
#include "oListInfo.h"

#include "TabSpeaker.h"
#include <cassert>

//Base position for speaker buttons
#define SPEAKER_BASE_X 40

TabSpeaker::TabSpeaker(oEvent *poe):TabBase(poe)
{
  classId=0;
  ownWindow = false;

  lastControlToWatch = 0;
  lastClassToWatch = 0;
  watchEvents = false;
  watchLevel = oTimeLine::PMedium;
  watchNumber = 5;
}

TabSpeaker::~TabSpeaker()
{
}

void clearSpeakerData(gdioutput &gdi)
{
  if (gdi.getTabs().hasSpeaker()) {
    TabSpeaker &ts = dynamic_cast<TabSpeaker &>(*gdi.getTabs().get(TSpeakerTab));
    ts.clear();
  }
}

int tabSpeakerCB(gdioutput *gdi, int type, void *data)
{
  TabSpeaker &ts = dynamic_cast<TabSpeaker &>(*gdi->getTabs().get(TSpeakerTab));

	switch(type){
		case GUI_BUTTON: {
			//Make a copy
			ButtonInfo bu=*static_cast<ButtonInfo *>(data);
			return ts.processButton(*gdi, bu);
  	}
		case GUI_LISTBOX:{	
			ListBoxInfo lbi=*static_cast<ListBoxInfo *>(data);
			return ts.processListBox(*gdi, lbi);
		}
    case GUI_EVENT: {
      EventInfo ei=*static_cast<EventInfo *>(data);
      return ts.handleEvent(*gdi, ei);
    }
    case GUI_CLEAR:
      return ts.onClear(*gdi);
    case GUI_TIMEOUT:
      ts.updateTimeLine(*gdi);
    break;
	}
	return 0;
}

int TabSpeaker::handleEvent(gdioutput &gdi, const EventInfo &ei)
{
  updateTimeLine(gdi);
  return 0;
}	

int TabSpeaker::processButton(gdioutput &gdi, const ButtonInfo &bu)
{
	if (bu.id=="Settings") {
    gdi.restore("settings");
    gdi.unregisterEvent("DataUpdate");
    gdi.fillDown();
    gdi.addString("", 0, "help:speaker_setup");
    gdi.pushX();
    gdi.fillRight();
    gdi.addListBox("Classes", 150, 300,0,"Klasser","", true);
    
    oe->fillClasses(gdi, "Classes", oEvent::extraNone, oEvent::filterNone);
    gdi.setSelection("Classes", classesToWatch);
   
    gdi.addListBox("Controls", 200, 300,0, "Kontroller","", true);
    gdi.pushX();
    gdi.fillDown();
    oe->fillControls(gdi, "Controls");
    gdi.setSelection("Controls", controlsToWatch);

    gdi.addButton("OK", "OK", tabSpeakerCB);
  }
  else if (bu.id=="ZoomIn") {
    gdi.scaleSize(1.05);
  }
  else if (bu.id=="ZoomOut") {
    gdi.scaleSize(1.0/1.05);
  }
  else if (bu.id=="Manual") {
    gdi.unregisterEvent("DataUpdate");
    gdi.restore("settings");
    gdi.fillDown();
    gdi.addString("", 1, "Inmatning av mellantider");
    gdi.dropLine(0.5);
    manualTimePage(gdi);
  }
  else if (bu.id == "PunchTable") {
    gdi.clearPage(false);
    gdi.addButton("Cancel", "Stäng", tabSpeakerCB);
    gdi.dropLine();
    gdi.addTable(oe->getPunchesTB(), gdi.getCX(), gdi.getCY());
    gdi.refresh();
  }
  else if (bu.id == "Priority") {
    gdi.clearPage(false);
    gdi.addString("", boldLarge, "Bevakningsprioritering");
    gdi.addString("", 10, "help:speakerprio");
    gdi.dropLine();
    gdi.fillRight();
    gdi.pushX();
    gdi.addString("", 0, "Klass:");
    gdi.addSelection("Class", 200, 200, tabSpeakerCB, "", "Välj klass");
    oe->fillClasses(gdi, "Class", oEvent::extraNone, oEvent::filterNone);
    gdi.addButton("ClosePri", "Stäng", tabSpeakerCB);
    gdi.dropLine(2);
    gdi.popX();
    gdi.refresh();
  }
  else if (bu.id == "ClosePri") {
    savePriorityClass(gdi);
    loadPage(gdi);
  }
  else if (bu.id == "Events") {
    gdi.restore("classes");  

    shownEvents.clear();
    events.clear();

	  gdi.registerEvent("DataUpdate", tabSpeakerCB);
	  gdi.setData("DataSync", 1);
	  gdi.setData("PunchSync", 1);
  	
    drawTimeLine(gdi);
    watchEvents = false;
  }
  else if (bu.id == "Window") {
    oe->setupTimeLineEvents(0);

    gdioutput *gdi_new = createExtraWindow(MakeDash("MeOS - Speakerstöd"), gdi.getWidth() + 64 + gdi.scaleLength(120));
    if (gdi_new) {
      TabSpeaker &tl = dynamic_cast<TabSpeaker &>(*gdi_new->getTabs().get(TSpeakerTab));
      tl.ownWindow = true;
      tl.loadPage(*gdi_new);
      //oe->renderTimeLineEvents(*gdi_new);
    }
  }
  else if (bu.id=="StoreTime") {
    storeManualTime(gdi);
  }
  else if (bu.id=="Cancel") {
    loadPage(gdi);
  }
  else if (bu.id=="OK") {
    gdi.getSelection("Classes", classesToWatch);  
    gdi.getSelection("Controls", controlsToWatch);  

    controlsToWatchSI.clear();
    for (set<int>::iterator it=controlsToWatch.begin();it!=controlsToWatch.end();++it) {
      pControl pc=oe->getControl(*it, false);

      if (pc) 
        controlsToWatchSI.insert(pc->Numbers, pc->Numbers+pc->nNumbers);
    }

    loadPage(gdi);
  }
  else if (bu.id.substr(0, 3)=="cid" ) {
    classId=atoi(bu.id.substr(3, string::npos).c_str());
    generateControlList(gdi, classId);
		gdi.setRestorePoint("speaker");
  	gdi.setRestorePoint("SpeakerList");

    if (selectedControl.count(classId)==1)
      oe->speakerList(gdi, classId, selectedControl[classId].getLeg(), 
                                    selectedControl[classId].getControl()); 
  }
  else if (bu.id.substr(0, 4)=="ctrl") {
    int ctrl=atoi(bu.id.substr(4, string::npos).c_str());    
    selectedControl[classId].setControl(ctrl);
    gdi.restore("speaker");
    oe->speakerList(gdi, classId, selectedControl[classId].getLeg(), ctrl);    
  }
  
  return 0;
}

void TabSpeaker::drawTimeLine(gdioutput &gdi) {
  gdi.restoreNoUpdate("SpeakerList");
	gdi.setRestorePoint("SpeakerList");

  gdi.fillRight();
  gdi.pushX();
  gdi.dropLine(0.3);
  gdi.addString("", 0, "Filtrering:");
  gdi.dropLine(-0.2);
  gdi.addSelection("DetailLevel", 160, 100, tabSpeakerCB);
  gdi.addItem("DetailLevel", lang.tl("Alla händelser"), oTimeLine::PLow);
  gdi.addItem("DetailLevel", lang.tl("Viktiga händelser"), oTimeLine::PMedium);
  gdi.addItem("DetailLevel", lang.tl("Avgörande händelser"), oTimeLine::PHigh);
  gdi.selectItemByData("DetailLevel", watchLevel);

  gdi.dropLine(0.2);
  gdi.addString("", 0, "Antal:");
  gdi.dropLine(-0.2);
  gdi.addSelection("WatchNumber", 160, 200, tabSpeakerCB);
  gdi.addItem("WatchNumber", lang.tl("X senaste#5"), 5);
  gdi.addItem("WatchNumber", lang.tl("X senaste#10"), 10);
  gdi.addItem("WatchNumber", lang.tl("X senaste#20"), 20);
  gdi.addItem("WatchNumber", lang.tl("X senaste#50"), 50);
  gdi.addItem("WatchNumber", "Alla", 0);
  gdi.selectItemByData("WatchNumber", watchNumber);
  gdi.dropLine(2);
  gdi.popX();

  string cls;
  for (set<int>::iterator it = classesToWatch.begin(); it != classesToWatch.end(); ++it) {
    pClass pc = oe->getClass(*it);
    if (pc) {
      if (!cls.empty())
        cls += ", ";
      cls += oe->getClass(*it)->getName();
    }
  }
  gdi.fillDown();  
  gdi.addString("", 1, "Bevakar händelser i X#" + cls);
  gdi.dropLine();

  gdi.setRestorePoint("TimeLine");
  updateTimeLine(gdi);
}


void TabSpeaker::updateTimeLine(gdioutput &gdi) {
  gdi.restore("TimeLine", false);

  int nextEvent = oe->getTimeLineEvents(classesToWatch, events, shownEvents, 0);
  int timeOut = nextEvent - oe->getComputerTime();
  if (timeOut > 0)
  gdi.addTimeout(timeOut, tabSpeakerCB);
  bool showClass = classesToWatch.size()>1;

  oListInfo li;  
  const int classWidth = gdi.scaleLength(li.getMaxCharWidth(oe, lClassName, "", normalText, false));
  const int timeWidth = gdi.scaleLength(60);
  const int totWidth = gdi.scaleLength(450);
  const int extraWidth = gdi.scaleLength(200);
  string dash = MakeDash("- ");
  const int extra = 5;

  int limit = watchNumber > 0 ? watchNumber : events.size();
  for (size_t k = events.size()-1; signed(k) >= 0; k--) {
    oTimeLine &e = events[k];

    if (e.getPriority() < watchLevel)
      continue;

    if (--limit < 0)
      break;

    int t = e.getTime();
    string msg = t>0 ? oe->getAbsTime(t) : MakeDash("--:--:--"); 
    pRunner r = pRunner(e.getSource());

    RECT rc;
    rc.top = gdi.getCY();
    rc.left = gdi.getCX();

    int xp = rc.left + extra;
    int yp = rc.top + extra;

    bool largeFormat = e.getPriority() >= oTimeLine::PHigh && k + 15 >= events.size();

    if (largeFormat) {
      gdi.addStringUT(yp, xp, 0, msg);  

      int dx = 0;
      if (showClass) {
        pClass pc = oe->getClass(e.getClassId());
        if (pc) {
          gdi.addStringUT(yp, xp + timeWidth, fontMediumPlus, pc->getName());
          dx = classWidth;
        }
      }

      if (r) {
        string bib = r->getBib();
        if (!bib.empty()) 
          msg = bib + ", ";
        else
          msg = "";
        msg += r->getCompleteIdentification();
        int xlimit = totWidth + extraWidth - (timeWidth + dx);
        gdi.addStringUT(yp, xp + timeWidth + dx, fontMediumPlus, msg, xlimit);
      }

      yp += int(gdi.getLineHeight() * 1.5);
      msg = dash + lang.tl(e.getMessage());
      gdi.addStringUT(yp, xp + 10, breakLines, msg, totWidth);

      const string &detail = e.getDetail();

      if (!detail.empty()) {
        gdi.addString("", gdi.getCY(), xp + 20, 0, detail).setColor(colorDarkGrey);
      }

      if (r && e.getType() == oTimeLine::TLTFinish && r->getStatus() == StatusOK) {
        splitAnalysis(gdi, xp + 20, gdi.getCY(), r);
      }

      GDICOLOR color = colorLightRed;

      switch (e.getType()) {
        case oTimeLine::TLTFinish:
          if (r && r->statusOK())
            color = colorLightGreen;
          else
            color = colorLightRed;
          break;
        case oTimeLine::TLTStart:
          color = colorLightYellow;
          break;
        case oTimeLine::TLTRadio:
          color = colorLightCyan;
          break;
        case oTimeLine::TLTExpected:
          color = colorLightBlue;
          break;
      }
      
      rc.bottom = gdi.getCY() + extra;
      rc.right = rc.left + totWidth + extraWidth + 2 * extra;      
      gdi.addRectangle(rc, color, true);
      gdi.dropLine(0.5);
    }
    else {
      if (showClass) {
        pClass pc = oe->getClass(e.getClassId());
        if (pc )
          msg += " (" + pc->getName() + ") ";
        else
          msg += " ";
      }
      else
        msg += " ";

      if (r) {      
        msg += r->getCompleteIdentification() + " ";
      }
      msg += lang.tl(e.getMessage());
      gdi.addStringUT(gdi.getCY(), gdi.getCX(), breakLines, msg, totWidth);

      const string &detail = e.getDetail();

      if (!detail.empty()) {
        gdi.addString("", gdi.getCY(), gdi.getCX()+50, 0, detail).setColor(colorDarkGrey);
      }

      if (r && e.getType() == oTimeLine::TLTFinish && r->getStatus() == StatusOK) {
        splitAnalysis(gdi, xp, gdi.getCY(), r);
      }
      gdi.dropLine(0.5);
    }
  }

  gdi.refresh();
}

void TabSpeaker::splitAnalysis(gdioutput &gdi, int xp, int yp, pRunner r)
{
  if (!r) 
    return;

  vector<int> delta;
  r->getSplitAnalysis(delta);
  string timeloss = lang.tl("Bommade kontroller: ");
  pCourse pc = 0;
  bool first = true;
  const int charlimit = 90;
  for (size_t j = 0; j<delta.size(); ++j) {
    if (delta[j] > 0) {
      if (pc == 0) {
        pc = r->getCourse();
        if (pc == 0)
          break;
      }

      if (!first)
        timeloss += " | ";
      else
        first = false;

      timeloss += pc->getControlOrdinal(j) + ". " + formatTime(delta[j]);
    }
    if (timeloss.length() > charlimit || (!timeloss.empty() && !first && j+1 == delta.size())) {
      gdi.addStringUT(yp, xp, 0, timeloss).setColor(colorDarkRed);
      yp += gdi.getLineHeight();
      timeloss = "";
    }
  }
  if (first) {
    gdi.addString("", yp, xp, 0, "Bomfritt lopp / underlag saknas").setColor(colorDarkGreen);
  }
}

void TabSpeaker::generateControlList(gdioutput &gdi, int classId)
{
  pClass pc=oe->getClass(classId);  

  if(!pc)
    return;

  int leg = selectedControl[pc->getId()].getLeg();

  list<pControl> controls;
  
  gdi.restore("classes", true);
  pCourse course=0;      

  if(pc->hasMultiCourse())
    course=pc->getCourse(leg);
  else
    course=pc->getCourse();

  if(course)						
	  course->getControls(controls);						
  
  list<pControl>::iterator it;
	gdi.fillDown();

  int h,w;
  gdi.getTargetDimension(w, h);

  int bw=gdi.scaleLength(100);
  int nbtn=max((w-80)/bw, 1);
  bw=(w-80)/nbtn;
  int basex=SPEAKER_BASE_X;
  int basey=gdi.getCY()+4;

  int cx=basex;
  int cy=basey;
  int cb=1;

  vector< pair<int, int> > stages;
  pc->getTrueStages(stages);

  if (stages.size()>1) {
    gdi.addSelection(cx, cy+2, "Leg", int(bw/gdi.getScale())-5, 100, tabSpeakerCB);

    for (size_t k=0; k<stages.size(); k++) {      
      gdi.addItem("Leg", lang.tl("Sträcka X#" + itos(stages[k].second)), stages[k].first);
    }
    gdi.selectItemByData("Leg", leg);
    cb+=1;
    cx+=1*bw;
  }
  else if (stages.size() == 1) {
    selectedControl[classId].setLeg(stages[0].first);
  }

	for (it=controls.begin(); it!=controls.end(); ++it) {		
    if (controlsToWatch.count( (*it)->getId() ) ) {

      if (selectedControl[classId].getControl() == -1) {
        // Default control
        selectedControl[classId].setControl((*it)->getId());
      }

		  char bf[16];
      sprintf_s(bf, "ctrl%d", (*it)->getId());
      gdi.addButton(cx, cy, bw, bf, "#" + (*it)->getName(), tabSpeakerCB, "", false, false);
      cx+=bw;
      
      cb++;
      if (cb>nbtn) {
        cb=1;
        cy+=gdi.getButtonHeight()+4;
        cx=basex;
      }
    }
  }
	gdi.fillDown();
  char bf[16];
  sprintf_s(bf, "ctrl%d", oPunch::PunchFinish);
  gdi.addButton(cx, cy, bw, bf, "Mål", tabSpeakerCB, "", false, false);

  if (selectedControl[classId].getControl() == -1) {
    // Default control
    selectedControl[classId].setControl(oPunch::PunchFinish);
  }

  gdi.popX(); 
}

int TabSpeaker::processListBox(gdioutput &gdi, const ListBoxInfo &bu)
{
  if(bu.id=="Leg") {
    if (classId>0) {
      selectedControl[classId].setLeg(bu.data);
      generateControlList(gdi, classId);
      oe->speakerList(gdi, classId, selectedControl[classId].getLeg(),
                      selectedControl[classId].getControl());    
    }
  }
  else if (bu.id == "DetailLevel") {
    watchLevel = oTimeLine::Priority(bu.data);
    shownEvents.clear();
    events.clear();

    updateTimeLine(gdi);
  }
  else if (bu.id == "WatchNumber") {
    watchNumber = bu.data;
    updateTimeLine(gdi);
  }
  else if (bu.id == "Class") {
    savePriorityClass(gdi);
    int classId = int(bu.data);
    loadPriorityClass(gdi, classId);
  }
  return 0;
}

bool TabSpeaker::loadPage(gdioutput &gdi)
{
  oe->checkDB();

	gdi.clearPage(true);
  gdi.pushX();
  gdi.setRestorePoint("settings");

  gdi.pushX();
  gdi.fillDown();

  int h,w;
  gdi.getTargetDimension(w, h);

  int bw=gdi.scaleLength(100);
  int nbtn=max((w-80)/bw, 1);
  bw=(w-80)/nbtn;
  int basex = SPEAKER_BASE_X;
  int basey=gdi.getCY();

  int cx=basex;
  int cy=basey;
  int cb=1;
  for (set<int>::iterator it=classesToWatch.begin();it!=classesToWatch.end();++it) {
    char classid[32];
    sprintf_s(classid, "cid%d", *it);
    pClass pc=oe->getClass(*it);
    
    if (pc) {
      gdi.addButton(cx, cy, bw, classid, "#" + pc->getName(), tabSpeakerCB, "", false, false);
      cx+=bw;
      cb++;

      if (cb>nbtn) {
        cb=1;
        cx=basex;
        cy+=gdi.getButtonHeight()+4;
      }
    }
  }

  int db = 0;
  if (classesToWatch.empty()) {
    gdi.addString("", boldLarge, "Speakerstöd"); 
    gdi.dropLine();
    cy=gdi.getCY();
    cx=gdi.getCX();
  }
  else {
    gdi.addButton(cx+db, cy, bw-2, "Events", "Händelser", tabSpeakerCB, "Löpande information om viktiga händelser i tävlingen", false, false);
    if (++cb>nbtn) {
      cb = 1, cx = basex, db = 0;
      cy += gdi.getButtonHeight()+4;
    } else db += bw;
    gdi.addButton(cx+db, cy, bw/5, "ZoomIn", "+", tabSpeakerCB, "Zooma in (Ctrl + '+')", false, false);
    db += bw/5+2;
    gdi.addButton(cx+db, cy, bw/5, "ZoomOut", MakeDash("-"), tabSpeakerCB, "Zooma ut (Ctrl + '-')", false, false);
    db += bw/5+2;
  }
  gdi.addButton(cx+db, cy, bw-2, "Settings", "Inställningar...", tabSpeakerCB, "Välj vilka klasser och kontroller som bevakas", false, false);
  if (++cb>nbtn) {
    cb = 1, cx = basex, db = 0;
    cy += gdi.getButtonHeight()+4;
  } else db += bw;
  gdi.addButton(cx+db, cy, bw-2, "Manual", "Tidsinmatning", tabSpeakerCB, "Mata in radiotider manuellt", false, false);
  if (++cb>nbtn) {
    cb = 1, cx = basex, db = 0;
    cy += gdi.getButtonHeight()+4;
  } else db += bw;

  gdi.addButton(cx+db, cy, bw-2, "PunchTable", "Stämplingar", tabSpeakerCB, "Visa en tabell över alla stämplingar", false, false);
  if (++cb>nbtn) {
    cb = 1, cx = basex, db = 0;
    cy += gdi.getButtonHeight()+4;
  } else db += bw;

  if (!ownWindow) {
    gdi.addButton(cx+db, cy, bw-2, "Priority", "Prioritering", tabSpeakerCB, "Välj löpare att prioritera bevakning för", false, false);
    if (++cb>nbtn) {
      cb = 1, cx = basex, db = 0;
      cy += gdi.getButtonHeight()+4;
    } else db += bw;


    gdi.addButton(cx+db, cy, bw-2, "Window", "Nytt fönster", tabSpeakerCB, "", false, false);
    if (++cb>nbtn) {
      cb = 1, cx = basex, db = 0;
      cy += gdi.getButtonHeight()+4;
    } else db += bw;
  }
  gdi.setRestorePoint("classes");

	return true;
}

void TabSpeaker::clear()
{
  controlsToWatch.clear();
  classesToWatch.clear();
  controlsToWatchSI.clear();
  selectedControl.clear();
  classId=0;
  lastControl.clear();
  
  lastControlToWatch = 0;
  lastClassToWatch = 0;

  shownEvents.clear();
  events.clear();
  watchEvents = false;
}

void TabSpeaker::manualTimePage(gdioutput &gdi) const
{
  gdi.setRestorePoint("manual");

  gdi.fillRight();
  gdi.pushX();
  gdi.addInput("Control", lastControl, 5, 0, "Kontroll");
  gdi.addInput("Runner", "", 6, 0, "Löpare");
  gdi.addInput("Time", "", 8, 0, "Tid");
  gdi.dropLine();
  gdi.addButton("StoreTime", "Spara", tabSpeakerCB).setDefault();
  gdi.addButton("Cancel", "Avbryt", tabSpeakerCB).setDefault();
  gdi.fillDown();
  gdi.popX();
  gdi.dropLine(3);
  gdi.addString("", 10, "help:14692");
  
  gdi.setInputFocus("Runner");
  gdi.refresh();
}

void TabSpeaker::storeManualTime(gdioutput &gdi) 
{
  char bf[256];

  int punch=gdi.getTextNo("Control");
  
  if (punch<=0) 
    throw std::exception("Kontrollnummer måste anges.");

  lastControl=gdi.getText("Control");
  int r_no=gdi.getTextNo("Runner");
  string time=gdi.getText("Time");
  pRunner r=oe->getRunnerByStartNo(r_no);

  if(!r)
    r=oe->getRunnerByCard(r_no);

  string Name;
  int sino=r_no;
  if(r) {
    Name=r->getName();
    sino=r->getCardNo();
  }
  else
    Name="Okänd";

  if (time.empty())
    time=getLocalTimeOnly();
  
  int itime=oe->getRelativeTime(time);

  if (itime <= 0)
    throw std::exception("Ogiltig tid.");

  if (sino <= 0) {
    sprintf_s(bf, "Ogiltigt bricknummer.#%d", sino);
    throw std::exception(bf);
  }

  pFreePunch fp=oe->addFreePunch(itime, punch, sino);
  fp->synchronize();
           
  gdi.restore("manual", false);
  gdi.addString("", 0, "Löpare: X, kontroll: Y, kl Z#" + Name + "#" + itos(punch) + "#" +  oe->getAbsTime(itime));

  manualTimePage(gdi);
}

void TabSpeaker::loadPriorityClass(gdioutput &gdi, int classId) {

  gdi.restore("PrioList");
  gdi.setRestorePoint("PrioList");
  gdi.setOnClearCb(tabSpeakerCB);
  runnersToSet.clear();
  vector<pRunner> r;
  oe->getRunners(classId, r);

  int x = gdi.getCX();
  int y = gdi.getCY()+2*gdi.getLineHeight();
  int dist = gdi.scaleLength(25);
  int dy = int(gdi.getLineHeight()*1.3); 
  for (size_t k = 0; k<r.size(); k++) {
    if (r[k]->skip() /*|| r[k]->getLeg>0*/)
      continue;
    int pri = r[k]->getSpeakerPriority();
    int id = r[k]->getId();
    gdi.addCheckbox(x,y,"A" + itos(id), "", 0, pri>0);
    gdi.addCheckbox(x+dist,y,"B" + itos(id), "", 0, pri>1);
    gdi.addStringUT(y-dist/3, x+dist*2, 0, r[k]->getCompleteIdentification());
    runnersToSet.push_back(id);
    y += dy;
  }
  gdi.refresh();
}

void TabSpeaker::savePriorityClass(gdioutput &gdi) {
  oe->synchronizeList(oLRunnerId);
  oe->synchronizeList(oLTeamId);

  for (size_t k = 0; k<runnersToSet.size(); k++) {
    pRunner r = oe->getRunner(runnersToSet[k], 0);
    if (r) {
      int id = runnersToSet[k];
      if (!gdi.hasField("A" + itos(id))) {
        runnersToSet.clear(); //Page not loaded. Abort.
        return;
      }

      bool a = gdi.isChecked("A" + itos(id));      
      bool b = gdi.isChecked("B" + itos(id));      
      int pri = (a?1:0) + (b?1:0);
      pTeam t = r->getTeam();
      if (t) {
        t->setSpeakerPriority(pri);
        t->synchronize(true);
      }
      else {
        r->setSpeakerPriority(pri);
        r->synchronize(true);
      }
    }
  }
}

bool TabSpeaker::onClear(gdioutput &gdi) {
  if (!runnersToSet.empty())
    savePriorityClass(gdi);

  return true;
}