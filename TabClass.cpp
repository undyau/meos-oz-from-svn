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
#include <cassert>

#include "resource.h"

#include <commctrl.h>
#include <commdlg.h> 
#include <algorithm>

#include "oEvent.h"
#include "xmlparser.h"
#include "gdioutput.h"
#include "csvparser.h"
#include "SportIdent.h"
#include "meos_util.h"
#include "oListInfo.h"
#include "TabClass.h"
#include "ClassConfigInfo.h"
#include "meosException.h"
#include "gdifonts.h"
#include "oEventDraw.h"

extern pEvent gEvent;

TabClass::TabClass(oEvent *poe):TabBase(poe)
{
  gdiVisualize = 0;
  clear();
}

void TabClass::clear() {
  pSettings.clear();
  currentStage=-1;
  EditChanged=false;
  ClassId=0;
  tableMode = false;
  storedNStage = "3";
  storedStart = "";
  storedPredefined = oEvent::PRelay; 
  cInfoCache.clear();

  if (gdiVisualize) {
    gdiVisualize->closeWindow();
    gdiVisualize = 0;
  }
}

TabClass::~TabClass(void)
{
}

int TimeToRel(const string &s)
{	
	int minute=atoi(s.c_str());

	int second=0;

	int kp=s.find_first_of(':');	
	if(kp>0)
	{		
		string mtext=s.substr(kp+1);
		second=atoi(mtext.c_str());
	}
	return minute*60+second;
}


bool ClassInfoSortStart(ClassInfo &ci1, ClassInfo &ci2)
{
	return ci1.firstStart>ci2.firstStart;
}

void clearClassData(gdioutput &gdi)
{
  if (gdi.getTabs().hasClass()) {
    TabClass &tc = dynamic_cast<TabClass &>(*gdi.getTabs().get(TClassTab));
    tc.clear();      
  }
}

int closeWindowEvent(gdioutput *gdi, int type, void *data) { 
  EventInfo ei = *(EventInfo *)(data);
  if (ei.id ==  "CloseWindow") {
    TabClass *tc = (TabClass *)ei.getExtra();
    if (tc) {
      tc->closeWindow(*gdi);
    }
  }
  return 0;
}

void TabClass::closeWindow(gdioutput &gdi) {
  if (&gdi == gdiVisualize) {
    gdiVisualize = 0;
  }
}

int ClassesCB(gdioutput *gdi, int type, void *data)
{
  TabClass &tc = dynamic_cast<TabClass &>(*gdi->getTabs().get(TClassTab));

  return tc.classCB(*gdi, type, data);
}

int MultiCB(gdioutput *gdi, int type, void *data)
{
  TabClass &tc = dynamic_cast<TabClass &>(*gdi->getTabs().get(TClassTab));
  return tc.multiCB(*gdi, type, data);
}

int TabClass::multiCB(gdioutput &gdi, int type, void *data)
{
	if (type==GUI_BUTTON) {
		ButtonInfo bi=*(ButtonInfo *)data;

    if (bi.id=="ChangeLeg") {
      gdi.dropLine();
      legSetup(gdi);
      gdi.refresh();
      //gdi.disableInput("ChangeLeg");
    }
		else if (bi.id=="SetNStage") {
			if (!checkClassSelected(gdi))
        return false;

			pClass pc=oe->getClass(ClassId);
      
      if (!pc)
        throw std::exception("Klassen finns ej.");

      int total, finished, dns;
      oe->getNumClassRunners(pc->getId(), 0, total, finished, dns);

      ListBoxInfo lbi;
      gdi.getSelectedItem("Predefined", &lbi);
      oEvent::PredefinedTypes newType = oEvent::PredefinedTypes(lbi.data);
      int nstages = gdi.getTextNo("NStage");
      
      if (finished > 0) { 
        if (gdi.ask("warning:has_results") == false) 
          return false;
      }
      else if (total>0) {
        bool ok = false;
        ClassType ct = pc->getClassType();
        if (ct == oClassIndividual) {
          switch (newType) {
            case oEvent::PPatrolOptional:
            case oEvent::PPool:
            case oEvent::PPoolDrawn:
            case oEvent::PNoMulti:
            case oEvent::PForking:
              ok = true;
            break;
            case oEvent::PNoSettings:
              ok = (nstages == 1);
          }
        }

        if (!ok) {
          if (gdi.ask("warning:has_entries") == false)
            return false;
        }
      }
      storedPredefined = newType;

      if (nstages > 0)
        storedNStage = gdi.getText("NStage");
      else {
        storedNStage = "";
        if (lbi.data != oEvent::PNoMulti)
          nstages = 1; //Fixed by type
      }

			if (nstages>0 && nstages<41) {
        string st=gdi.getText("StartTime");
        if (oe->convertAbsoluteTime(st)>0)
          storedStart = st;

        save(gdi, false); //Clears and reloads

				gdi.selectItemByData("Courses", -2);
        gdi.disableInput("Courses");        
        oe->setupRelay(*pc, newType, nstages, st);

        if (gdi.hasField("MAdd")) {
				  gdi.enableInput("MAdd");
				  gdi.enableInput("MCourses");
				  gdi.enableInput("MRemove");
        }

        selectClass(gdi, pc->getId());
			}
			else if (nstages==0){
				pc->setNumStages(0);
        pc->synchronize();
				gdi.restore();
				gdi.enableInput("MultiCourse");
				gdi.enableInput("Courses");
        oe->adjustTeamMultiRunners(pc);
			}
			else {
				gdi.alert("Antalet sträckor måste vara ett heltal mellan 0 och 40.");				
			}
		}
    else if(bi.id.substr(0, 7)=="@Course") {
      int cnr=atoi(bi.id.substr(7).c_str());
      
      gdi.restore("Courses", false);
      gdi.setRestorePoint("Courses");
      char bf[128];
      pClass pc=oe->getClass(ClassId);

      if (!pc) {
        return 0;
        gdi.refresh();
      }
      currentStage=cnr;

      sprintf_s(bf, lang.tl("Banor för %s, sträcka %d").c_str(), pc->getName().c_str(), cnr+1);
	    
      gdi.dropLine();
      gdi.pushX();
      gdi.fillRight();
      gdi.addStringUT(1, bf);
      ButtonInfo &bi1 = gdi.addButton("@Course" + itos(cnr-1), "<< Föregående", MultiCB);
      if (cnr<=0)
        gdi.disableInput(bi1.id.c_str());
      ButtonInfo &bi2 = gdi.addButton("@Course" + itos(cnr+1), "Nästa >>", MultiCB);
      if (unsigned(cnr + 1) >= pc->getNumStages())
        gdi.disableInput(bi2.id.c_str());
      gdi.popX();
      gdi.dropLine(2.5);
      gdi.fillRight();
      int x1=gdi.getCX();
      gdi.addListBox("StageCourses", 180, 200, MultiCB, "Sträckans banor:").ignore(true);
      pc->fillStageCourses(gdi, currentStage, "StageCourses");
      int x2=gdi.getCX();
      gdi.fillDown();
      gdi.addListBox("MCourses", 180, 200, MultiCB, "Banor:").ignore(true);
      oe->fillCourses(gdi, "MCourses", true);
      
      gdi.setCX(x1);
      gdi.fillRight();
	    
      gdi.addButton("MRemove", "Ta bort markerad >>", MultiCB);
	    gdi.setCX(x2);
      gdi.fillDown();
      
      gdi.addButton("MAdd", "<< Lägg till", MultiCB);
      gdi.setCX(x1);
      gdi.refresh();
      if (pc->getNumStages() > 1)
        gdi.scrollTo(gdi.getCX(), gdi.getCY());
    }
		else if(bi.id=="MAdd"){
			DWORD cid=ClassId;
			if (!checkClassSelected(gdi))
        return false;

			pClass pc=oe->getClass(cid);
		
      if (!pc)
        return false;

			if(currentStage>=0){
			  ListBoxInfo lbi;	
				if(gdi.getSelectedItem("MCourses", &lbi)) {
					int courseid=lbi.data;

					pc->addStageCourse(currentStage, courseid);
					pc->fillStageCourses(gdi, currentStage, "StageCourses");

					pc->synchronize();
					oe->checkOrderIdMultipleCourses(cid);
				}
			}			
		}
		else if(bi.id=="MRemove"){
			if (!checkClassSelected(gdi))
        return false;

			DWORD cid=ClassId;

			pClass pc=oe->getClass(cid);

      if (!pc)
        return false;

			if(currentStage>=0){
        ListBoxInfo lbi;
				if(gdi.getSelectedItem("StageCourses", &lbi)) {
					int courseid=lbi.data;

					pc->removeStageCourse(currentStage, courseid, lbi.index);
					pc->synchronize();
					pc->fillStageCourses(gdi, currentStage, "StageCourses");
				}
			}
		}
	}
	else if(type==GUI_LISTBOX) {
		ListBoxInfo bi=*(ListBoxInfo *)data;

    if(bi.id.substr(0, 7)=="LegType") {
      LegTypes lt=LegTypes(bi.data);
      int i=atoi(bi.id.substr(7).c_str());
      pClass pc=oe->getClass(ClassId);
      if (!pc)
        return false;
      pc->setLegType(i, lt);
      char legno[10];
      sprintf_s(legno, "%d", i);
      gdi.setInputStatus(string("StartData")+legno, !pc->startdataIgnored(i));
      gdi.setInputStatus(string("Restart")+legno, !pc->restartIgnored(i));
      gdi.setInputStatus(string("RestartRope")+legno, !pc->restartIgnored(i));

      EditChanged=true;
    }
    else if(bi.id.substr(0, 9)=="StartType") {
      StartTypes st=StartTypes(bi.data);
      int i=atoi(bi.id.substr(9).c_str());
      pClass pc=oe->getClass(ClassId);
      if (!pc)
        return false;
      pc->setStartType(i, st);
      char legno[10];
      sprintf_s(legno, "%d", i);
      gdi.setInputStatus(string("StartData")+legno, !pc->startdataIgnored(i));
      gdi.setInputStatus(string("Restart")+legno, !pc->restartIgnored(i));
      gdi.setInputStatus(string("RestartRope")+legno, !pc->restartIgnored(i));

      EditChanged=true;
    }
    else if (bi.id == "Predefined") {
      bool nleg;
      bool start;
      oe->setupRelayInfo(oEvent::PredefinedTypes(bi.data), nleg, start);
      gdi.setInputStatus("NStage", nleg);
      gdi.setInputStatus("StartTime", start);
      
      string nl = gdi.getText("NStage");
      if (!nleg && nl != "-") {
        storedNStage = nl;
        gdi.setText("NStage", "-");
      }
      else if (nleg && nl == "-") {
        gdi.setText("NStage", storedNStage);
      }

      string st = gdi.getText("StartTime");
      if (!start && st != "-") {
        storedStart = st;
        gdi.setText("StartTime", "-");
      }
      else if (start && st == "-") {
        gdi.setText("StartTime", storedStart);
      }
    }
		else if(bi.id=="Courses")
			EditChanged=true;
	}
	else if(type==GUI_INPUTCHANGE){
		InputInfo ii=*(InputInfo *)data;

    EditChanged=true;
		if(ii.id=="NStage")
			gdi.enableInput("SetNStage");
    //else if(ii.id=="")
	}
	return 0;
}

int TabClass::classCB(gdioutput &gdi, int type, void *data)
{
	if (type==GUI_BUTTON) {
		ButtonInfo bi=*(ButtonInfo *)data;

		if (bi.id=="Cancel") {
			loadPage(gdi);
			return 0;
		}
    else if (bi.id == "UseAdvanced") {
      bool checked = gdi.isChecked("UseAdvanced");
      oe->setProperty("AdvancedClassSettings", checked);
      save(gdi, true);
      PostMessage(gdi.getTarget(), WM_USER + 2, TClassTab, 0);
    }
    else if (bi.id=="SwitchMode") {
      if (!tableMode)
        save(gdi, true);
      tableMode=!tableMode;
      loadPage(gdi);
    }
    else if (bi.id=="Restart") {
      save(gdi, true);
      gdi.clearPage(true);
      gdi.addString("", 2, "Omstart i stafettklasser");
      gdi.addString("", 10, "help:31661");
      gdi.addListBox("RestartClasses", 200, 250, 0, "Stafettklasser", "", true);
      oe->fillClasses(gdi, "RestartClasses", oEvent::extraNone, oEvent::filterOnlyMulti);
      gdi.pushX();
      gdi.fillRight();
      oe->updateComputerTime();
      int t=oe->getComputerTime()-(oe->getComputerTime()%60)+60;
      gdi.addInput("Rope", oe->getAbsTime(t), 6, 0, "Repdragningstid");
      gdi.addInput("Restart", oe->getAbsTime(t+600), 6, 0, "Omstartstid");
      gdi.dropLine(0.9);
      gdi.addButton("DoRestart","OK", ClassesCB);
      gdi.addButton("Cancel","Stäng", ClassesCB);
      gdi.fillDown();
      gdi.dropLine(3);
      gdi.popX();
    }
    else if (bi.id=="DoRestart") {
      set<int> cls;
      gdi.getSelection("RestartClasses", cls);
      gdi.fillDown();
      set<int>::iterator it;
      string ropeS=gdi.getText("Rope");
      int rope = oe->getRelativeTime(ropeS);
      string restartS=gdi.getText("Restart");
      int restart = oe->getRelativeTime(restartS);

      if (rope<=0) {
        gdi.alert("Ogiltig repdragningstid.");
        return 0;
      }
      if (restart<=0) {
        gdi.alert("Ogiltig omstartstid.");
        return 0;
      }        
      if (restart<rope) {
        gdi.alert("Repdragningstiden måste ligga före omstartstiden.");
        return 0;
      }
      
      gdi.addString("", 0, "Sätter reptid (X) och omstartstid (Y) för:#" + 
                    oe->getAbsTime(rope) + "#" + oe->getAbsTime(restart));
      
      for (it=cls.begin(); it!=cls.end(); ++it) {
        pClass pc=oe->getClass(*it);
        
        if (pc) {
          gdi.addStringUT(0, pc->getName());

          int ns=pc->getNumStages();

          for (int k=0;k<ns;k++) {
            pc->setRopeTime(k, ropeS);
            pc->setRestartTime(k, restartS);
          }
        }
      }
      gdi.scrollToBottom();
      gdi.refresh();
    }
		else if (bi.id=="DoDrawAll") {
      readClassSettings(gdi);
      ListBoxInfo lbi;
      gdi.getSelectedItem("Method", &lbi);
      bool soft = lbi.data==2;
      bool pairwise = gdi.isChecked("Pairwise");
      
			for(size_t k=0; k<cInfo.size(); k++) {
        const ClassInfo &ci=cInfo[k];
        oe->drawList(ci.classId, 0, drawInfo.firstStart + drawInfo.baseInterval * ci.firstStart,
          drawInfo.baseInterval * ci.interval, ci.nVacant, soft, pairwise, oEvent::drawAll);
			}
			gdi.clearPage(false);
			gdi.addButton("Cancel", "Återgå", ClassesCB);

      oListParam par;
      oListInfo info;
      par.listCode = EStdStartList;
      for (size_t k=0; k<cInfo.size(); k++) 
        par.selection.insert(cInfo[k].classId);

      oe->generateListInfo(par, gdi.getLineHeight(), info);
      oe->generateList(gdi, false, info);
      gdi.refresh();
		}
    else if (bi.id == "RemoveVacant") {
      if (gdi.ask("Vill du radera alla vakanser från tävlingen?")) {
        oe->removeVacanies(0);
      }
    }
    else if (bi.id == "SelectAll") {
      set<int> lst;
      oe->getAllClasses(lst);
      gdi.setSelection("Classes", lst);
    }
    else if (bi.id == "SelectUndrawn") {
      set<int> lst;
      oe->getNotDrawnClasses(lst, false);
      gdi.setSelection("Classes", lst);
    }
    else if (bi.id == "QuickSettings") {
      save(gdi, false);
      prepareForDrawing(gdi);
    }
    else if (bi.id == "DrawMode") {
      save(gdi, false);
      ClassId = 0;
		  
		  EditChanged=false;
		  gdi.clearPage(true);

      gdi.addString("", boldLarge, "Lotta flera klasser");
      gdi.dropLine();

      gdi.pushX();
      gdi.fillRight();
      gdi.addInput("FirstStart", oe->getAbsTime(3600), 10, 0, "Första (ordinarie) start:");
      gdi.addInput("MinInterval", "2:00", 10, 0, "Minsta startintervall:");
      gdi.fillDown();
      gdi.addInput("Vacances", "5%", 10, 0, "Andel vakanser:");
      gdi.popX();
      
      gdi.addSelection("Method", 200, 200, 0, "Metod:");
      gdi.addItem("Method", lang.tl("Lottning"), 1);
      gdi.addItem("Method", lang.tl("SOFT-lottning"), 2);
      gdi.selectItemByData("Method", 1);
      
      gdi.fillDown();
      gdi.addCheckbox("LateBefore", "Efteranmälda före ordinarie");
      gdi.dropLine();

      gdi.popX();
      gdi.fillRight();
      gdi.addButton("AutomaticDraw", "Automatisk lottning", ClassesCB).setDefault();
      gdi.addButton("DrawAll", "Manuell lottning", ClassesCB).setExtra(LPVOID(1));
      
      const bool multiDay = !oe->getDCI().getString("PreEvent").empty();
      
      if (multiDay)
        gdi.addButton("Pursuit", "Hantera jaktstart", ClassesCB);

      gdi.addButton("Cancel", "Återgå", ClassesCB).setCancel();

      gdi.dropLine(3);

      gdi.newColumn();

      gdi.addString("", 10, "help_autodraw");
    }
    else if (bi.id == "Pursuit") {
      pursuitDialog(gdi);
    }
    else if (bi.id == "SelectAllNoneP") {
      bool select = bi.getExtraInt() != 0;
      for (int k = 0; k < oe->getNumClasses(); k++) {
        gdi.check("PLUse" + itos(k), select);
        gdi.setInputStatus("First" + itos(k), select);
      }
    }
    else if (bi.id == "DoPursuit" || bi.id=="CancelPursuit") {
      bool cancel = bi.id=="CancelPursuit";
      int maxAfter = convertAbsoluteTimeMS(gdi.getText("MaxAfter"));
      int deltaRestart = convertAbsoluteTimeMS(gdi.getText("TimeRestart"));
      int interval = convertAbsoluteTimeMS(gdi.getText("Interval"));

      double scale = atof(gdi.getText("ScaleFactor").c_str());
      bool reverse = bi.getExtraInt() == 2;
      bool pairwise = gdi.isChecked("Pairwise");

      oListParam par;

      for (int k = 0; k < oe->getNumClasses(); k++) {
        if (!gdi.hasField("PLUse" + itos(k)))
          continue;
        BaseInfo *bi = gdi.setText("PLUse" + itos(k), "", false);
        if (bi) {
          int id = bi->getExtraInt();
          bool checked = gdi.isChecked("PLUse" + itos(k));
          int first = oe->getRelativeTime(gdi.getText("First" + itos(k)));
          //int max = oe->getRelativeTime(gdi.getText("Max" + itos(k)));
          
          map<int, PursuitSettings>::iterator st = pSettings.find(id);
          if (st != pSettings.end()) {
            st->second.firstTime = first;
            st->second.maxTime = maxAfter;
            st->second.use = checked;
          }

          if (!cancel && checked) {
            oe->drawPersuitList(id, first, first + deltaRestart, maxAfter, 
                                interval, pairwise, reverse, scale);
            par.selection.insert(id);
          }
        }
      }

      if (cancel) {
        loadPage(gdi);
        return 0;
      }

      gdi.restore("Pursuit", false);

			gdi.dropLine();
      gdi.fillDown();

      oListInfo info;
      par.listCode = EStdStartList;
      oe->generateListInfo(par, gdi.getLineHeight(), info);      
      oe->generateList(gdi, false, info);
      gdi.dropLine();
      gdi.addButton("Cancel", "Återgå", ClassesCB);
      gdi.refresh();
    }
    else if (bi.id.substr(0,5) == "PLUse") {
      int k = atoi(bi.id.substr(5).c_str());
      gdi.setInputStatus("First" + itos(k), gdi.isChecked(bi.id));
    }
    else if (bi.id == "AutomaticDraw") {
      string firstStart = gdi.getText("FirstStart");
      string minInterval = gdi.getText("MinInterval");
      string vacances = gdi.getText("Vacances");
      bool lateBefore = false;//gdi.isChecked("LateBefore");
      bool pairwise = gdi.isChecked("Pairwise");
      
      ListBoxInfo lbi;
      gdi.getSelectedItem("Method", &lbi);
      bool useSoft = lbi.data==2;
      gdi.clearPage(true);
      oe->automaticDrawAll(gdi, firstStart, minInterval, vacances, 
                            lateBefore, useSoft, pairwise);

      gdi.scrollToBottom();
      gdi.addButton("Cancel", "Återgå", ClassesCB);
    }
    else if (bi.id == "SelectMisses") {
      set<int> lst;
      oe->getNotDrawnClasses(lst, true);
      gdi.setSelection("Classes", lst);
    }
    else if (bi.id == "SelectNone") {
      gdi.setSelection("Classes", set<int>());
    }
		else if (bi.id == "DrawAll") {
      int origin = int(bi.getExtra());
      string firstStart = oe->getAbsTime(3600);
      string minInterval = "2:00";
      string vacances = "5%";
      int maxNumControl = 1;
      bool pairwise = false;
      int by = 0;
      int bx = gdi.getCX();
      if (origin!=13) {
        if (origin!=1) {
          save(gdi, true);
          ClassId = 0;
			    
			    EditChanged=false;
        }
        else {
          firstStart = gdi.getText("FirstStart");
          minInterval = gdi.getText("MinInterval");
          vacances = gdi.getText("Vacances");
          pairwise = gdi.isChecked("Pairwise");
        }

			  gdi.clearPage(false);

        gdi.addString("", boldLarge, "Lotta flera klasser");
  		
        gdi.pushY();
        gdi.addListBox("Classes", 200, 400, 0, "Klasser:", "", true);
        gdi.setTabStops("Classes", 185);
        gdi.fillRight();
        gdi.pushX();

        gdi.addButton("SelectAll", "Välj allt", ClassesCB,  
                      "Välj alla klasser").isEdit(true);

        gdi.addButton("SelectMisses", "Saknad starttid", ClassesCB,  
          "Välj klasser där någon löpare saknar starttid").isEdit(true);
        
        gdi.dropLine(2.3);
        gdi.popX();

        gdi.addButton("SelectUndrawn", "Ej lottade", ClassesCB, 
          "Välj klasser där alla löpare saknar starttid").isEdit(true);

        gdi.fillDown();
        gdi.addButton("SelectNone", "Välj inget", ClassesCB, 
          "Avmarkera allt").isEdit(true);
        gdi.popX();

        oe->fillClasses(gdi, "Classes", oEvent::extraDrawn, oEvent::filterNone);
        
        by = gdi.getCY()+gdi.getLineHeight();
        bx = gdi.getCX();
        gdi.newColumn();
        gdi.popY();
        gdi.addString("", 1, "Grundinställningar");
        
        gdi.pushX();
        gdi.fillRight();

			  gdi.addInput("FirstStart", firstStart, 10, 0, "Första start:");
        gdi.addInput("nFields", "10", 10, 0, "Max parallellt startande:");

        gdi.popX();
        gdi.dropLine(3);

        gdi.addSelection("MaxCommonControl", 150, 100, 0, 
          "Max antal gemensamma kontroller:");

        vector< pair<string, size_t> > items;
        items.push_back(make_pair(lang.tl("Inga"), 1));
        items.push_back(make_pair(lang.tl("Första kontrollen"), 2));
        for (int k = 2; k<5; k++)
          items.push_back(make_pair(lang.tl("X kontroller#" + itos(k)), k+1));
        
        gdi.addItem("MaxCommonControl", items);
        gdi.selectItemByData("MaxCommonControl", maxNumControl);

        gdi.popX();
        gdi.dropLine(4);
        gdi.addString("", 1, "Startintervall");
        gdi.dropLine(1.4);
        gdi.popX();
        gdi.addInput("BaseInterval", "2:00", 10, 0, "Basintervall (min):");
        gdi.addInput("MinInterval", minInterval, 10, 0, "Minsta intervall i klass:");
        gdi.addInput("MaxInterval", "4:00", 10, 0, "Största intervall i klass:");
			  
        gdi.popX();
        gdi.dropLine(4);
        gdi.addString("", 1, "Vakanser och efteranmälda");
        gdi.dropLine(1.4);
        gdi.popX();
        gdi.addInput("Vacances", vacances, 10, 0, "Andel vakanser:");
        gdi.addInput("VacancesMin", "1", 10, 0, "Min. vakanser (per klass):");
        gdi.addInput("VacancesMax", "10", 10, 0, "Max. vakanser (per klass):");
  		        
        gdi.popX();
        gdi.dropLine(3);

        gdi.addInput("Extra", "15%", 10, 0, "Förväntad andel efteranmälda:");

        gdi.dropLine(4);
        gdi.fillDown();
        gdi.popX();        
        gdi.setRestorePoint("Setup");

      }
      else {
        gdi.restore("Setup");
        by = gdi.getHeight();
        gdi.enableEditControls(true);
      }
			
			gdi.fillRight();
      gdi.pushX();
      gdi.addString("", 1, "Förbered lottning");
      gdi.dropLine(1.5);
      gdi.popX();
      gdi.addButton("PrepareDrawAll", "Fördela starttider...", ClassesCB).isEdit(true).setDefault();
      gdi.addButton("EraseStartAll", "Radera starttider...", ClassesCB).isEdit(true);

      gdi.popX();
      gdi.dropLine(3);
      gdi.addString("", 1, "Efteranmälningar");
      gdi.dropLine(1.5);
      gdi.popX();
      gdi.addButton("DrawAllBefore", "Efteranmälda (före ordinarie)", ClassesCB).isEdit(true);
			gdi.addButton("DrawAllAfter", "Efteranmälda (efter ordinarie)", ClassesCB).isEdit(true);

      gdi.dropLine(4);
      gdi.popX();

      gdi.addButton("Cancel", "Avbryt", ClassesCB).setCancel();
      gdi.addButton("HelpDraw", "Hjälp...", ClassesCB, "");
      gdi.dropLine(3);

      by = max(by, gdi.getCY());
   
      gdi.setCX(bx);
      gdi.setCY(by);
      gdi.fillDown();
			gdi.dropLine();

      gdi.setRestorePoint("ReadyToDistribute");
			gdi.refresh();
		}
    else if (bi.id == "HelpDraw") {
      gdioutput *gdi_new = createExtraWindow(MakeDash("MeOS - " + lang.tl("Hjälp")),  gdi.scaleLength(640));
      gdi_new->addString("", boldLarge, "Lotta flera klasser");  		
      gdi_new->addString("", 10, "help_draw");
      gdi_new->dropLine();
      gdi_new->addButton("CloseWindow", "Stäng", ClassesCB);
    }
    else if (bi.id == "CloseWindow") {
      gdi.closeWindow();
    }
		else if (bi.id=="PrepareDrawAll") {
      set<int> classes;
      gdi.getSelection("Classes", classes);
      if (classes.empty()) {
        gdi.alert("Ingen klass vald.");
        return 0;
      }
			gdi.restore("ReadyToDistribute");
      gdi.enableEditControls(false);
      drawInfo.classes.clear();
      
      for (set<int>::iterator it = classes.begin(); it!=classes.end();++it) {
        map<int, ClassInfo>::iterator res = cInfoCache.find(*it);
        if ( res != cInfoCache.end() ) {
          res->second.hasFixedTime = false;
          drawInfo.classes[*it] = res->second;
        }
        else
          drawInfo.classes[*it] = ClassInfo(oe->getClass(*it));
      }

      ListBoxInfo lbi;
      gdi.getSelectedItem("MaxCommonControl", &lbi);
      drawInfo.maxCommonControl = lbi.data;

			drawInfo.maxVacancy=gdi.getTextNo("VacancesMax");
      drawInfo.minVacancy=gdi.getTextNo("VacancesMin");
      drawInfo.vacancyFactor = 0.01*atof(gdi.getText("Vacances").c_str());
      drawInfo.extraFactor = 0.01*atof(gdi.getText("Extra").c_str());
				
      drawInfo.baseInterval=TimeToRel(gdi.getText("BaseInterval"));
			drawInfo.minClassInterval=TimeToRel(gdi.getText("MinInterval"));
			drawInfo.maxClassInterval=TimeToRel(gdi.getText("MaxInterval"));
			drawInfo.nFields=gdi.getTextNo("nFields");
			drawInfo.firstStart=oe->getRelativeTime(gdi.getText("FirstStart"));

      if (drawInfo.firstStart<=0) {
        drawInfo.baseInterval = 0;
        gdi.addString("", 0, "Raderar starttider...");
      }

			oe->optimizeStartOrder(gdi, drawInfo, cInfo);
			
      showClassSettings(gdi);
		}
    else if (bi.id == "VisualizeDraw") {
      gdioutput *gdi_new = gdiVisualize;
      if (!gdi_new)
        gdi_new = createExtraWindow(MakeDash("MeOS - " + lang.tl("Visualisera startfältet")),  gdi.scaleLength(1000));
      
      gdi_new->clearPage(false);
      gdi_new->addString("", boldLarge, "Visualisera startfältet");
      gdi_new->dropLine();
      gdi_new->addString("", 0, "För muspekaren över en markering för att få mer information.");
      gdi_new->dropLine();
      visualizeField(*gdi_new);
      gdi_new->dropLine();
      gdi_new->addButton("CloseWindow", "Stäng", ClassesCB);
      gdi_new->registerEvent("CloseWindow", closeWindowEvent).setExtra(this);
      gdi_new->refresh();
      gdiVisualize = gdi_new;
    }
    else if (bi.id == "EraseStartAll") {
      set<int> classes;
      gdi.getSelection("Classes", classes);
      if (classes.empty()) {
        gdi.alert("Ingen klass vald.");
        return 0;
      }
      if (classes.size() == 1) {
        pClass pc = oe->getClass(*classes.begin());
        if (!pc || !gdi.ask("Vill du verkligen radera alla starttider i X?#" + pc->getName()))
          return 0;
      }
      else {
        if (!gdi.ask("Vill du verkligen radera starttider i X klasser?#" + itos(classes.size())) ) 
          return 0;
      }

      for (set<int>::const_iterator it = classes.begin(); it != classes.end(); ++it) {
        oe->drawList(*it, 0, 0, 0, 0, false, false, oEvent::drawAll);
      }

      bi.setExtra(1);
      bi.id = "DrawAll";
      classCB(gdi, type, &bi); // Reload draw dialog
    }
    else if (bi.id == "DrawAdjust") {
      readClassSettings(gdi);
      gdi.restore("ReadyToDistribute");
      oe->optimizeStartOrder(gdi, drawInfo, cInfo);
      showClassSettings(gdi);
    }
    else if (bi.id == "DrawAllAdjust") {
      readClassSettings(gdi);
      bi.id = "DrawAll";
      return classCB(gdi, type, &bi);
    }
    else if (bi.id == "DrawAllBefore" || bi.id == "DrawAllAfter") {
      oe->drawRemaining(true, bi.id == "DrawAllAfter");
      loadPage(gdi);
    }
		else if(bi.id=="DoDraw" || bi.id=="DoDrawAfter"  || bi.id=="DoDrawBefore"){			
 			if (!checkClassSelected(gdi))
        return false;

			DWORD cid=ClassId;

      ListBoxInfo lbi;
      gdi.getSelectedItem("Method", &lbi);
			gdi.popX();
		
      int interval = 0;
      if (gdi.hasField("Interval"))
        interval = convertAbsoluteTimeMS(gdi.getText("Interval"));
			
      int vacanses = 0;
      if (gdi.hasField("Vacanses"))
        vacanses = gdi.getTextNo("Vacanses");
      
      int leg = 0;
      if (gdi.hasField("Leg")) {
        ListBoxInfo legInfo;
        gdi.getSelectedItem("Leg", &legInfo);
        leg = legInfo.data;
      }

      int bib = 0;
      bool doBibs = false;

      if (gdi.hasField("Bib")) {
        bib = gdi.getTextNo("Bib");
        doBibs = gdi.isChecked("HandleBibs");
      }

      int t=oe->getRelativeTime(gdi.getText("FirstStart"));

      if (t<=0) 
        throw std::exception("Ogiltig första starttid. Måste vara efter nolltid.");

      oEvent::DrawType type(oEvent::drawAll);
      if (bi.id=="DoDrawAfter")
        type = oEvent::remainingAfter;
      else if (bi.id=="DoDrawBefore")
        type = oEvent::remainingBefore;
      
      bool pairwise = false;
      
      if (gdi.hasField("Pairwise"))
        pairwise = gdi.isChecked("Pairwise");

      int maxTime = 0, restartTime = 0;
      double scaleFactor = 1.0;

      if (gdi.hasField("TimeRestart"))
        restartTime = oe->getRelativeTime(gdi.getText("TimeRestart"));

      if (gdi.hasField("MaxAfter"))
        maxTime = convertAbsoluteTimeMS(gdi.getText("MaxAfter"));
      
      if (gdi.hasField("ScaleFactor"))
        scaleFactor = atof(gdi.getText("ScaleFactor").c_str());
      
      // Clear input
			gdi.restore();
			gdi.addButton("Cancel", "Återgå", ClassesCB);
      TabClass::DrawMethod method = TabClass::DrawMethod(lbi.data);

      if(method == DMRandom || method == DMSOFT)
				oe->drawList(cid, leg, t, interval, vacanses, method == DMSOFT, pairwise, type);
      else if (method == DMClumped)
				oe->drawListClumped(cid, t, interval, vacanses);
      else if (method == DMPursuit || method == DMReversePursuit) {
        oe->drawPersuitList(cid, t, restartTime, maxTime, 
          interval, pairwise, method == DMReversePursuit, scaleFactor);
      }
      else 
        throw std::exception("Not implemented");

      if (doBibs)
			  oe->addBib(cid, leg, bib);

			gdi.dropLine();
    
      oListParam par;
      par.selection.insert(cid);
      oListInfo info;
      par.listCode = EStdStartList;
      par.legNumber = leg;
      oe->generateListInfo(par, gdi.getLineHeight(), info);      
      oe->generateList(gdi, false, info);

			gdi.refresh();
			return 0;
		}
    else if (bi.id=="HandleBibs") {
      gdi.setInputStatus("Bib", gdi.isChecked("HandleBibs"));
    }
    else if (bi.id == "DoDeleteStart") {
      pClass pc=oe->getClass(ClassId);
      if(!pc)
        throw meosException("Class not found");

      if (!gdi.ask("Vill du verkligen radera alla starttider i X?#" + pc->getName()))
        return 0;

      int leg = 0;
      if (gdi.hasField("Leg")) {
        ListBoxInfo legInfo;
        gdi.getSelectedItem("Leg", &legInfo);
        leg = legInfo.data;
      }

      oe->drawList(ClassId, leg, 0, 0, 0, false, false, oEvent::drawAll);
      loadPage(gdi);
    }
		else if (bi.id=="Draw") {
      save(gdi, true);
      if (!checkClassSelected(gdi))
        return false;

			DWORD cid=ClassId;
			
			if(oe->classHasResults(cid)) {
				if(!gdi.ask("warning:drawresult"))
					return 0;
			}

			pClass pc=oe->getClass(cid);

      if(!pc)
        throw std::exception("Class not found");
      if(EditChanged)			
				gdi.sendCtrlMessage("Save");
		
			gdi.clearPage(false);
			
      gdi.addString("", boldLarge, "Lotta klassen X#"+pc->getName());
      gdi.dropLine();		
			gdi.pushX();
			gdi.setRestorePoint();

      gdi.fillDown();
      bool multiDay = !oe->getDCI().getString("PreEvent").empty();
      
      if (multiDay) {
        gdi.addCheckbox("HandleMultiDay", "Använd funktioner för fleretappsklass", ClassesCB, true);
      }

      gdi.addSelection("Method", 200, 200, ClassesCB, "Metod:");
      gdi.dropLine(1.5);
      gdi.popX();

      gdi.setRestorePoint("MultiDayDraw");

      lastDrawMethod = NOMethod;
      drawDialog(gdi, DMRandom, *pc);
		}
    else if (bi.id == "HandleMultiDay") {
      ListBoxInfo lbi;
      gdi.getSelectedItem("Method", &lbi);
      setMultiDayClass(gdi, gdi.isChecked(bi.id), DrawMethod(lbi.data));
    
    }
  	else if (bi.id=="Bibs") {
      save(gdi, true);
      if (!checkClassSelected(gdi))
        return false;

			DWORD cid=ClassId;
	
			pClass pc=oe->getClass(cid);
      if (!pc)
        throw std::exception("Class not found");

			gdi.clearPage(false);			
      gdi.addString("", boldLarge, "Nummerlappar i X#" + pc->getName());
      gdi.dropLine();
      gdi.setRestorePoint("bib");
			gdi.addString("", 0, "Ange första nummerlappsnummer eller lämna blankt för inga nummerlappar.");
      gdi.dropLine(0.5);
      gdi.addInput("Bib", "", 10, 0, "");
      gdi.dropLine();
			gdi.fillRight();
      gdi.addButton("DoBibs", "Tilldela", ClassesCB).setDefault();	
			gdi.fillDown();			
      gdi.addButton("Cancel", "Avbryt", ClassesCB).setCancel();
			gdi.popX();
	
			EditChanged=false;
			gdi.refresh();
		}
		else if (bi.id=="DoBibs") {			
 			if (!checkClassSelected(gdi))
        return false;

			DWORD cid=ClassId;

			int bib=gdi.getTextNo("Bib");
      
      gdi.restore("bib");

      oe->addBib(cid, 0, bib);

//			gdi.addString("", 1, "Utfört");
			gdi.dropLine();
      gdi.addButton("Cancel", "Återgå", ClassesCB).setDefault();


      oListParam par;
      par.selection.insert(cid);
      oListInfo info;
      par.listCode = EStdStartList;
      par.legNumber = 0;
      oe->generateListInfo(par, gdi.getLineHeight(), info);      
      oe->generateList(gdi, false, info);

      gdi.refresh();
			return 0;
		}    
		else if (bi.id=="Split") {
      save(gdi, true);
      if (!checkClassSelected(gdi))
        return false;

			pClass pc=oe->getClass(ClassId);
      if (!pc)
        throw std::exception("Class not found");

			gdi.clearPage(true);			
      gdi.addString("", boldLarge, "Dela klass: X#" + pc->getName());
      gdi.dropLine();
      gdi.addSelection("Type", 200, 100, ClassesCB, "Typ av delning:");
      gdi.addItem("Type", lang.tl("Dela klubbvis"), 1);
      gdi.addItem("Type", lang.tl("Dela efter ranking"), 2);
      gdi.selectItemByData("Type", 1);
      gdi.fillRight();
      gdi.pushX();
      gdi.addString("TypeDesc", 0, "Antal klasser:");
      gdi.setCX(gdi.getCX()+50);
      gdi.addInput("SplitInput", "2", 5);
      gdi.dropLine(3);
      gdi.popX();
      gdi.addButton("DoSplit", "Dela", ClassesCB);
      gdi.addButton("Cancel", "Avbryt", ClassesCB);
      gdi.popX();
    }
    else if (bi.id=="DoSplit") {
      if (!checkClassSelected(gdi))
        return false;

			pClass pc=oe->getClass(ClassId);
      if (!pc)
        throw std::exception("Class not found");

      ListBoxInfo lbi;
      gdi.getSelectedItem("Type", &lbi);

      int number = gdi.getTextNo("SplitInput");

      if (number<=1)
        throw std::exception ("Antal måste vara större än 1.");

      vector<int> outClass;
      if (lbi.data==1)
        oe->splitClass(ClassId, number, outClass);
      else 
        oe->splitClassRank(ClassId, number, outClass);

      gdi.clearPage(true);
      gdi.addButton("Cancel", "Återgå", ClassesCB);
      
      oListParam par;
      par.selection.insert(outClass.begin(), outClass.end());
      oListInfo info;
      par.listCode = EStdStartList;      
      oe->generateListInfo(par, gdi.getLineHeight(), info);      
      oe->generateList(gdi, false, info);
      //oe->generateStartlist(gdi, 0, 0, outClass);
    }
    else if (bi.id=="Merge") {
      save(gdi, true);
      if (!checkClassSelected(gdi))
        return false;

			pClass pc=oe->getClass(ClassId);
      if (!pc)
        throw std::exception("Class not found");

			gdi.clearPage(true);			
      gdi.addString("", boldLarge, "Slå ihop klass: X (denna klass behålls)#" + pc->getName());
      gdi.dropLine();
      gdi.addString("", 10, "help:12138");
      gdi.dropLine(2);
      gdi.addSelection("Class", 150, 300, 0, "Klass att slå ihop:");
      oe->fillClasses(gdi, "Class", oEvent::extraNone, oEvent::filterNone);
      gdi.fillRight();
      gdi.addButton("DoMergeAsk", "Slå ihop", ClassesCB);
      gdi.addButton("Cancel", "Avbryt", ClassesCB);
    }
    else if (bi.id=="DoMergeAsk") {
      if (!checkClassSelected(gdi))
        return false;

			pClass pc=oe->getClass(ClassId);
      if (!pc)
        throw std::exception("Class not found");

      ListBoxInfo lbi;
      gdi.getSelectedItem("Class", &lbi);
      pClass mergeClass = oe->getClass(lbi.data);

      if (!mergeClass)
        throw std::exception("Ingen klass vald.");
      
      if (lbi.data==ClassId)
        throw std::exception("En klass kan inte slås ihop med sig själv.");

      if (gdi.ask("Vill du flytta löpare från X till Y och ta bort Z?#"
        + mergeClass->getName() + "#" + pc->getName() + "#" + mergeClass->getName())) {
        bi.id = "DoMerge";
        return classCB(gdi, type, &bi);
      }
      return false;
    }
    else if (bi.id=="DoMerge") {
      if (!checkClassSelected(gdi))
        return false;

			pClass pc=oe->getClass(ClassId);
      if (!pc)
        throw std::exception("Class not found");

      ListBoxInfo lbi;
      gdi.getSelectedItem("Class", &lbi);

      if (signed(lbi.data)<=0)
        throw std::exception("Ingen klass vald.");
      
      if (lbi.data==ClassId)
        throw std::exception("En klass kan inte slås ihop med sig själv.");

      oe->mergeClass(ClassId, lbi.data);
      gdi.clearPage(true);
      gdi.addButton("Cancel", "Återgå", ClassesCB);

      oListParam par;
      par.selection.insert(ClassId);
      oListInfo info;
      par.listCode = EStdStartList;      
      oe->generateListInfo(par, gdi.getLineHeight(), info);      
      oe->generateList(gdi, false, info);
      gdi.refresh();
    }
    else if (bi.id=="MultiCourse") {
      save(gdi, false);
			multiCourse(gdi, 0);
      gdi.refresh();
		}
		else if (bi.id=="Save")
      save(gdi, false);
		else if (bi.id=="Add") {
      string name = gdi.getText("Name");
      pClass c = oe->getClass(ClassId);
      if (!name.empty() && c && c->getName() != name) {
        if (gdi.ask("Vill du lägga till klassen 'X'?#" + name)) {
          c = oe->addClass(name);
          ClassId = c->getId();
          save(gdi, false);
          return true;
        }
      }


			save(gdi, true);
      pClass pc = oe->addClass(oe->getAutoClassName(), 0);
      if (pc) {
        oe->fillClasses(gdi, "Classes", oEvent::extraDrawn, oEvent::filterNone);
        selectClass(gdi, pc->getId());
        gdi.setInputFocus("Name", true);
      }
		}
		else if (bi.id=="Remove") {
			EditChanged=false;
  		if (!checkClassSelected(gdi))
        return false;

			DWORD cid=ClassId;
	
			if(oe->isClassUsed(cid))
				gdi.alert("Klassen används och kan inte tas bort.");
			else
				oe->removeClass(cid);

			oe->fillClasses(gdi, "Classes", oEvent::extraDrawn, oEvent::filterNone);
      ClassId = 0;
      selectClass(gdi, 0);
		}
	}
	else if (type==GUI_LISTBOX) {
		ListBoxInfo bi=*(ListBoxInfo *)data;

    if(bi.id=="Classes") {
      if (gdi.isInputChanged(""))
        save(gdi, true);

      selectClass(gdi, bi.data);
    }
		else if(bi.id=="Courses")
			EditChanged=true;
    else if (bi.id=="Type") {
      if (bi.data==1) {
        gdi.setText("TypeDesc", "Antal klasser:", true);
        gdi.setText("SplitInput", "2");
      }
      else {
        gdi.setText("TypeDesc", "Löpare per klass:", true);
        gdi.setText("SplitInput", "100");
      }
    }
    else if (bi.id == "Method") {
      pClass pc = oe->getClass(ClassId);
      if (!pc)
        throw std::exception("Nullpointer exception");

      drawDialog(gdi, DrawMethod(bi.data), *pc);
    }
	}
	else if(type==GUI_INPUTCHANGE) {
		//InputInfo ii=*(InputInfo *)data;

		//if(ii.id=="Name")
		//	EditChanged=true;
	}
	else if(type==GUI_CLEAR) {
    if (ClassId>0)
      save(gdi, true);
		if(EditChanged) {
			if(gdi.ask("Spara ändringar?"))
				gdi.sendCtrlMessage("Save");
		}
		return true;
	}
	return 0;
}

void TabClass::readClassSettings(gdioutput &gdi)
{
	for (size_t k=0;k<cInfo.size();k++) {
    ClassInfo &ci = cInfo[k];
    int id = ci.classId;
    const string &start = gdi.getText("S"+itos(id));
    const string &intervall = gdi.getText("I"+itos(id));
    int vacant = atoi(gdi.getText("V"+itos(id)).c_str());
    int reserved = atoi(gdi.getText("R"+itos(id)).c_str());

    int startPos = oe->getRelativeTime(start) - drawInfo.firstStart;

    if (drawInfo.firstStart == 0 && startPos == -1)
      startPos = 0;
    else if (startPos<0 || (startPos % drawInfo.baseInterval)!=0)
      throw std::exception("Ogiltig tid");

    startPos /= drawInfo.baseInterval;

    int intervalPos = TimeToRel(intervall);

    if (intervalPos<0 || (intervalPos % drawInfo.baseInterval)!=0)
      throw std::exception("Ogiltigt startintervall");

    intervalPos /= drawInfo.baseInterval;

    if (ci.nVacant != vacant) {
      ci.nVacantSpecified = true;
      ci.nVacant = vacant;
    }

    if (ci.nExtra != reserved) {
      ci.nExtraSpecified = true;
      ci.nExtra = reserved;
    }
    // If times has been changed, mark this class to be kept fixed
    if (ci.firstStart != startPos || ci.interval!=intervalPos)
      ci.hasFixedTime = true;

    ci.firstStart = startPos;
    ci.interval = intervalPos;

    drawInfo.classes[ci.classId] = ci;

    cInfoCache[ci.classId] = ci;
    cInfoCache[ci.classId].hasFixedTime = false;
	}
}

void TabClass::visualizeField(gdioutput &gdi) {
  ClassInfo::sSortOrder = 3;			
  sort(cInfo.begin(), cInfo.end());
	ClassInfo::sSortOrder = 0;
	
	vector<int> field;
  vector<int> index(cInfo.size(), -1);

  for (size_t k = 0;k < cInfo.size(); k++) {
    const ClassInfo &ci = cInfo[k];
	  int laststart = ci.firstStart + (ci.nRunners-1) * ci.interval;
	
    for (size_t j = 0; j < field.size(); j++) {
      if (field[j] < ci.firstStart) {
        index[k] = j;
        field[j] = laststart;
        break;
      }
    }
    if (index[k] == -1) {
      index[k] = field.size();
      field.push_back(laststart);
    }
/*
		string first=oe->getAbsTime(ci.firstStart*drawInfo.baseInterval+drawInfo.firstStart);
		string last=oe->getAbsTime((laststart)*drawInfo.baseInterval+drawInfo.firstStart);
    pClass pc=oe->getClass(ci.classId);*/
  }

  map<int, int> groupNumber;
  map<int, string> groups;
  int freeNumber = 1;
  for (size_t k = 0;k < cInfo.size(); k++) {
    const ClassInfo &ci = cInfo[k];
    if (!groupNumber.count(ci.unique))
      groupNumber[ci.unique] = freeNumber++;

    pClass pc = oe->getClass(ci.classId);
    if (pc) {
      if (groups[ci.unique].empty())
        groups[ci.unique] = pc->getName();
      else if (groups[ci.unique].size() < 64)
        groups[ci.unique] += ", " + pc->getName();
      else
        groups[ci.unique] += "...";
    }
  }

  int marg = gdi.scaleLength(20);
  int xp = gdi.getCX() + marg;
  int yp = gdi.getCY() + marg;
  int h = gdi.scaleLength(12);
  int w = gdi.scaleLength(6);
  int maxx = xp, maxy = yp;

  RECT rc;
  for (size_t k = 0;k < cInfo.size(); k++) {
    const ClassInfo &ci = cInfo[k];
    rc.top = yp + index[k] * h;
    rc.bottom = rc.top + h - 1;
    int g = ci.unique;
    GDICOLOR color = GDICOLOR(RGB(((g * 30)&0xFF), ((g * 50)&0xFF), ((g * 70)&0xFF)));
    for (int j = 0; j<ci.nRunners; j++) {
      rc.left = xp + (ci.firstStart + j * ci.interval) * w;
      rc.right = rc.left + w-1;
      gdi.addRectangle(rc, color);
    }
    pClass pc = oe->getClass(ci.classId);
    if (pc) {
      string tip = "X (Y deltagare, grupp Z, W)#" + pc->getName() + "#" + itos(ci.nRunners) + "#" + itos(groupNumber[ci.unique])
                    + "#" + groups[ci.unique];
      rc.left = xp + ci.firstStart * w;
      int laststart = ci.firstStart + (ci.nRunners-1) * ci.interval;
      rc.right = xp + (laststart + 1) * w;
      gdi.addToolTip(tip, 0, &rc);
      maxx = max<int>(maxx, rc.right);
      maxy = max<int>(maxy, rc.bottom);
    }
  }
  rc.left = xp - marg;
  rc.top = yp - marg;
  rc.bottom = maxy + marg;
  rc.right = maxx + marg;
  gdi.addRectangle(rc, colorLightYellow, true, true);

}

void TabClass::showClassSettings(gdioutput &gdi) 
{
  ClassInfo::sSortOrder = 2;			
  sort(cInfo.begin(), cInfo.end());
	ClassInfo::sSortOrder=0;
	
	int laststart=0;
  for (size_t k=0;k<cInfo.size();k++) {
    const ClassInfo &ci = cInfo[k];
		laststart=max(laststart, ci.firstStart+ci.nRunners*ci.interval);
	}

  int y = 0;
  int xp = gdi.getCX();
  const int width = 80;

  if (!cInfo.empty()) {
    gdi.dropLine();
    
    y = gdi.getCY();
    gdi.addString("", y, xp, 1, "Sammanställning, klasser:");
    gdi.addString("", y, xp+300, 0, "Första start:");
    gdi.addString("", y, xp+300+width, 0, "Intervall:");
    gdi.addString("", y, xp+300+width*2, 0, "Vakanser:");
    gdi.addString("", y, xp+300+width*3, 0, "Reserverade:");
  }

  gdi.pushX();
	for (size_t k=0;k<cInfo.size();k++) {
    const ClassInfo &ci = cInfo[k];
		char bf[256];
		int laststart = ci.firstStart + (ci.nRunners-1) * ci.interval;
		string first=oe->getAbsTime(ci.firstStart*drawInfo.baseInterval+drawInfo.firstStart);
		string last=oe->getAbsTime((laststart)*drawInfo.baseInterval+drawInfo.firstStart);
    pClass pc=oe->getClass(ci.classId);
		sprintf_s(bf, 256, "%s, %d platser. Start %d-[%d]-%d (%s-%s)",
      pc ? pc->getName().c_str():"", ci.nRunners,
			ci.firstStart, ci.interval, laststart, first.c_str(), last.c_str());
		
    gdi.fillRight();
		gdi.addStringUT(0, bf);
    y = gdi.getCY();
    int id = ci.classId;
    gdi.addInput(xp+300, y, "S"+itos(id), first, 7);
    gdi.addInput(xp+300+width, y, "I"+itos(id), formatTime(ci.interval*drawInfo.baseInterval), 7);
    gdi.addInput(xp+300+width*2, y, "V"+itos(id), itos(ci.nVacant), 7);
    gdi.addInput(xp+300+width*3, y, "R"+itos(id), itos(ci.nExtra), 7);

    if (k%5 == 4)
      gdi.dropLine(1);

    gdi.dropLine(1.6);
    gdi.fillDown();
    gdi.popX();
	}

	gdi.dropLine();
	gdi.addButton("VisualizeDraw", "Visualisera startfältet...", ClassesCB);
  gdi.dropLine();
	
  gdi.fillRight();

  if (!cInfo.empty()) {
    gdi.pushX();

    gdi.addSelection("Method", 200, 200, 0, "Metod:");
    gdi.addItem("Method", lang.tl("Lottning"), 1);
    gdi.addItem("Method", lang.tl("SOFT-lottning"), 2);
      
    gdi.selectItemByData("Method", 1);
    gdi.dropLine(0.9);

    gdi.addButton("DoDrawAll", "Utför lottning", ClassesCB);
    gdi.addButton("DrawAdjust", "Uppdatera fördelning", ClassesCB, 
        "Uppdatera fördelningen av starttider med hänsyn till manuella ändringar ovan");
  }
  gdi.addButton("DrawAllAdjust", "Ändra inställningar", ClassesCB,
      "Ändra grundläggande inställningar och gör en ny fördelning").setExtra(13);

  gdi.addButton("Cancel", "Avbryt", ClassesCB);

  gdi.fillDown();
	gdi.dropLine(2);
  gdi.popX();
  if (!cInfo.empty()) 
    gdi.addCheckbox("Pairwise", "Tillämpa parstart", 0, false);
    
  gdi.scrollToBottom();
	gdi.updateScrollbars();
	gdi.refresh();
}

void TabClass::selectClass(gdioutput &gdi, int cid)
{
  oe->fillCourses(gdi, "Courses", true);
  gdi.addItem("Courses", lang.tl("Ingen bana"), -2);

  if (cid==0) {
		gdi.restore("", true);
		gdi.disableInput("MultiCourse");
   	gdi.enableInput("Courses");
    gdi.enableEditControls(false);
    gdi.setText("Name", "");  	
	  gdi.selectItemByData("Courses", -2);
//    gdi.check("CoursePool", false);
    gdi.check("AllowQuickEntry", true);
    gdi.check("FreeStart", false);
    
    gdi.check("NoTiming", false);
 
    ClassId=cid;
    EditChanged=false;
    
    gdi.disableInput("Remove");
    gdi.disableInput("Save");
    return;
  }

	pClass pc = oe->getClass(cid);

  if (!pc) {
    selectClass(gdi, 0);
    return;
  }
  gdi.enableEditControls(true);
  gdi.enableInput("Remove");
  gdi.enableInput("Save");

	pc->synchronize();
	gdi.setText("Name", pc->getName());
  
  gdi.setText("ClassType", pc->getType());
  gdi.setText("StartName", pc->getStart());
  if (pc->getBlock()>0)
    gdi.selectItemByData("StartBlock", pc->getBlock());
  else
    gdi.selectItemByData("StartBlock", -1);
  
  if (gdi.hasField("Status")) {
    vector< pair<string, size_t> > out;
    size_t selected = 0;
    pc->getDCI().fillInput("Status", out, selected);
    gdi.addItem("Status", out);
    gdi.selectItemByData("Status", selected);
  }

  gdi.check("AllowQuickEntry", pc->getAllowQuickEntry());
  gdi.check("NoTiming", pc->getNoTiming());
  gdi.check("FreeStart", pc->hasFreeStart());
  
  ClassId=cid;

	if (pc->hasMultiCourse()) {				
		gdi.restore("", false);

    multiCourse(gdi, pc->getNumStages());
    gdi.refresh();
		//gdi.setText("StartTime", pc->GetClassStartTimeS());

		int nstage=pc->getNumStages();
    gdi.setText("NStage", nstage);  

    for (int k=0;k<nstage;k++) {
      char legno[10];
      sprintf_s(legno, "%d", k);

      gdi.selectItemByData((string("LegType")+legno).c_str(), pc->getLegType(k));
      gdi.selectItemByData((string("StartType")+legno).c_str(), pc->getStartType(k));
      gdi.setText(string("StartData")+legno, pc->getStartDataS(k));
      gdi.setInputStatus(string("StartData")+legno, !pc->startdataIgnored(k));
      gdi.setInputStatus(string("Restart")+legno, !pc->restartIgnored(k));
      gdi.setInputStatus(string("RestartRope")+legno, !pc->restartIgnored(k));

      gdi.setText(string("Restart")+legno, pc->getRestartTimeS(k));
      gdi.setText(string("RestartRope")+legno, pc->getRopeTimeS(k));
      gdi.selectItemByData((string("MultiR")+legno).c_str(), pc->getLegRunner(k));
    }

    gdi.addItem("Courses", lang.tl("Flera banor"), -3); 
		gdi.selectItemByData("Courses", -3);
		gdi.disableInput("Courses");			

    gdi.check("CoursePool", pc->hasCoursePool());
	}
	else {
 		gdi.restore("", true);
		gdi.enableInput("MultiCourse");
   	gdi.enableInput("Courses");
   	pCourse pcourse=pc->getCourse();
	  gdi.selectItemByData("Courses", pcourse ? pcourse->getId():-2);
	}

  gdi.selectItemByData("Classes", cid);

  ClassId=cid;
	EditChanged=false;
}

void TabClass::legSetup(gdioutput &gdi) 
{
  gdi.restore("RelaySetup");
  gdi.pushX();
  gdi.fillDown();

  gdi.addString("", 10, "help:relaysetup");
  gdi.dropLine();
  gdi.addSelection("Predefined", 150, 200, MultiCB, "Fördefinierade tävlingsformer:").ignore(true);
  oe->fillPredefinedCmp(gdi, "Predefined");
  gdi.selectItemByData("Predefined", storedPredefined);
  
  gdi.fillRight();
  gdi.addInput("NStage", storedNStage, 4, MultiCB, "Antal sträckor:").ignore(true);
  gdi.addInput("StartTime",  storedStart, 6, MultiCB, "Starttid (HH:MM:SS):");
  gdi.popX();
  
  bool nleg;
  bool start;
  oe->setupRelayInfo(storedPredefined, nleg, start);
  gdi.setInputStatus("NStage", nleg);
  gdi.setInputStatus("StartTime", start);
 
  gdi.fillRight();
  gdi.dropLine(3);
  gdi.addButton("SetNStage", "Verkställ", MultiCB);
  gdi.fillDown();
  gdi.addButton("Cancel", "Avbryt", ClassesCB);
  
  gdi.popX();
}


void TabClass::multiCourse(gdioutput &gdi, int nLeg)
{
  currentStage=-1;
	gdi.disableInput("MultiCourse");
	gdi.setRestorePoint();
  gdi.fillDown();
 	gdi.newColumn();
	
	//gdi.dropLine();
	gdi.addString("", 2, "Flera banor / stafett / patrull / banpool");
  gdi.addString("", 0, "Låt klassen ha mer än en bana eller sträcka");
  gdi.dropLine();
  gdi.setRestorePoint("RelaySetup");

  if (nLeg==0) 
    legSetup(gdi);
  else {
    gdi.pushX();
    gdi.fillRight();
    gdi.addButton("ChangeLeg", "Ändra klassinställningar...", MultiCB, "Starta en guide som hjälper dig göra klassinställningar");

    gdi.fillDown();
    gdi.popX();
    gdi.dropLine(2);
    
    gdi.dropLine(0.5);  
    int headYPos=gdi.getCY();
    gdi.dropLine(1.2);

    vector< pair<string, size_t> > legs;
    legs.reserve(nLeg);
    for (int j=0;j<nLeg;j++) {
      char bf[16];
      sprintf_s(bf, lang.tl("Str. %d").c_str(), j+1);
      legs.push_back( make_pair(bf, j) );
      //gdi.addItem(multir, bf, j);
    }

    for (int k=0;k<nLeg;k++) {
      char legno[10];
      int headXPos[10];
      sprintf_s(legno, "%d", k);
      gdi.fillRight();

      headXPos[0]=gdi.getCX();
      gdi.addInput(legno, "", 2);
      gdi.setText(legno, k+1);
      gdi.disableInput(legno);

      headXPos[1]=gdi.getCX();
      string legType(string("LegType")+legno);
      gdi.addSelection(legType, 100, 200, MultiCB);
      oClass::fillLegTypes(gdi, legType);

      headXPos[2]=gdi.getCX();
      string startType(string("StartType")+legno);    
      gdi.addSelection(startType, 90, 200, MultiCB);
      oClass::fillStartTypes(gdi, startType);

      headXPos[3]=gdi.getCX();
      gdi.addInput(string("StartData")+legno, "", 5, MultiCB);

      string multir(string("MultiR")+legno);
      headXPos[4]=gdi.getCX();
      gdi.addSelection(multir, 60, 200, MultiCB);
      gdi.addItem(multir, legs);
      /*for (int j=0;j<nLeg;j++) {
        char bf[16];
        sprintf_s(bf, lang.tl("Str. %d").c_str(), j+1);
        gdi.addItem(multir, bf, j);
      }*/

      headXPos[5]=gdi.getCX();
      gdi.addInput(string("RestartRope")+legno, "", 5, MultiCB);

      headXPos[6]=gdi.getCX();
      gdi.addInput(string("Restart")+legno, "", 5, MultiCB);

      gdi.dropLine(-0.1);
      gdi.addButton(string("@Course")+legno, "Banor...", MultiCB);

      gdi.fillDown();
      gdi.popX();
      gdi.dropLine(2.1);

      if (k==0) { //Add headers
        gdi.addString("", headYPos, headXPos[0], 0, "Str.");
        gdi.addString("", headYPos, headXPos[1], 0, "Sträcktyp");
        gdi.addString("", headYPos, headXPos[2], 0, "Starttyp");
        gdi.addString("", headYPos, headXPos[3], 0, "Starttid");
        gdi.addString("", headYPos, headXPos[4], 0, "Löpare");
        gdi.addString("", headYPos, headXPos[5], 0, "Rep");
        gdi.addString("", headYPos, headXPos[6], 0, "Omstart");
      }
    }

    if(nLeg==1)
      gdi.sendCtrlMessage("@Course0");

    gdi.addCheckbox("CoursePool", "Använd banpool", 0, false, 
                      "Knyt löparna till banor från en pool vid målgång.");
  }
  gdi.refresh();
}

bool TabClass::checkClassSelected(const gdioutput &gdi) const
{
  if (ClassId<=0) {
		gdi.alert("Ingen klass vald.");
		return false;
	}
  else return true;
}

void TabClass::save(gdioutput &gdi, bool skipReload)
{
  bool checkValid = EditChanged || gdi.isInputChanged("");
	DWORD cid=ClassId;

  pClass pc;			
	string name = gdi.getText("Name");
	
  if (cid==0 && name.empty()) 
    return;
  
  if (name.empty()) 
    throw std::exception("Klassen måste ha ett namn.");
	
	bool create=false;

	if (cid>0)
		pc=oe->getClass(cid);
	else {
		pc=oe->addClass(name);
		create=true;
	}

  if (!pc)
    throw std::exception("Class not found.");

	ClassId=pc->getId();

	pc->setName(name);
  if (gdi.hasField("StartName"))
    pc->setStart(gdi.getText("StartName"));
  
  if (gdi.hasField("ClassType"))
    pc->setType(gdi.getText("ClassType"));
  
  if (gdi.hasField("StartBlock"))
    pc->setBlock(gdi.getTextNo("StartBlock"));

  ListBoxInfo lbi;
  if (gdi.hasField("Status")) {
    gdi.getSelectedItem("Status", &lbi);
    pc->getDI().setEnum("Status", lbi.data);
  }

  pc->setCoursePool(gdi.isChecked("CoursePool"));

  pc->setAllowQuickEntry(gdi.isChecked("AllowQuickEntry"));
  pc->setNoTiming(gdi.isChecked("NoTiming"));
  pc->setFreeStart(gdi.isChecked("FreeStart"));
  
	gdi.getSelectedItem("Courses", &lbi);

	if (lbi.data==0)	{
		//Skapa ny bana...
		pCourse pcourse=oe->addCourse("Bana "+name);
		pc->setCourse(pcourse);				
		pc->synchronize();
		return;
	}
	else if(lbi.data==-2)
		pc->setCourse(0);
	else if(signed(lbi.data)>0)
		pc->setCourse(oe->getCourse(lbi.data));

  if(pc->hasMultiCourse()) {
		int nstage=pc->getNumStages();

    for (int k=0;k<nstage;k++) {
      char legno[10];
      sprintf_s(legno, "%d", k);

      if (!gdi.hasField(string("LegType")+legno))
        continue;

      ListBoxInfo lbi;
      gdi.getSelectedItem(string("LegType")+legno, &lbi);
      pc->setLegType(k, LegTypes(lbi.data));

      gdi.getSelectedItem(string("StartType")+legno, &lbi);
      pc->setStartType(k, StartTypes(lbi.data));

      pc->setStartData(k, gdi.getText(string("StartData")+legno));      

      pc->setRestartTime(k, gdi.getText(string("Restart")+legno));      
      pc->setRopeTime(k, gdi.getText(string("RestartRope")+legno));      

      gdi.getSelectedItem(string("MultiR")+legno, &lbi);
      pc->setLegRunner(k, lbi.data);    
    }
  }

  pc->addClassDefaultFee(false);
  pc->updateChangedCoursePool();
	pc->synchronize();
  oe->reCalculateLeaderTimes(pc->getId());
	oe->reEvaluateClass(pc->getId(), true);					
	
  oe->fillClasses(gdi, "Classes", oEvent::extraDrawn, oEvent::filterNone);
	EditChanged=false;
  if (!skipReload) {
    ClassId = 0;
    selectClass(gdi, pc->getId());
  }

  if (checkValid) {
    // Check/warn that starts blocks are set up correctly
    vector<int> b;
    vector<string> s;
    oe->getStartBlocks(b, s);
    oe->sanityCheck(gdi, false);
  }
}

bool TabClass::loadPage(gdioutput &gdi)
{
  oe->checkDB();
	gdi.selectTab(tabId);
	gdi.clearPage(false);
  int xp=gdi.getCX();
	
  const int button_w=gdi.scaleLength(90);
  string switchMode;
  switchMode=tableMode ? "Formulärläge" : "Tabelläge";
  gdi.addButton(2, 2, button_w, "SwitchMode", switchMode, 
    ClassesCB, "Välj vy", false, false).fixedCorner();

  if(tableMode) {
    Table *tbl=oe->getClassTB();
    gdi.addTable(tbl, xp, 30);
    return true;
  }

  ClassConfigInfo cnf;
  oe->getClassConfigurationInfo(cnf);
      
  bool showAdvanced = oe->getPropertyInt("AdvancedClassSettings", 0) != 0;
  gdi.addString("", boldLarge, "Klasser");

	gdi.fillDown();
 	gdi.addListBox("Classes", 200, 420, ClassesCB, "").isEdit(false).ignore(true);
  gdi.setTabStops("Classes", 185);
	oe->fillClasses(gdi, "Classes", oEvent::extraDrawn, oEvent::filterNone);
	
	gdi.newColumn();
	gdi.dropLine(2);		

	gdi.fillRight();
  gdi.pushX();
  gdi.addInput("Name", "", 14, ClassesCB, "Klassnamn:");
  
  if (showAdvanced) {
    gdi.addCombo("ClassType", 100, 300, 0, "Typ:");
    oe->fillClassTypes(gdi, "ClassType");
  }

  gdi.dropLine(3);
  gdi.popX();
  gdi.addSelection("Courses", 120, 400, ClassesCB, "Bana:");
	oe->fillCourses(gdi, "Courses", true);
	gdi.addItem("Courses", lang.tl("Ingen bana"), -2);
	
  gdi.dropLine(0.9);
	gdi.addButton("MultiCourse", "Flera banor/stafett...", ClassesCB);
  gdi.disableInput("MultiCourse");
  gdi.popX();
  if (showAdvanced) {
    gdi.dropLine(3);

    gdi.addCombo("StartName", 120, 300, 0, "Startnamn:");
    oe->fillStarts(gdi, "StartName");

    gdi.addSelection("StartBlock", 80, 300, 0, "Startblock:");

    for (int k=1;k<=100;k++) {
      char bf[16];
      sprintf_s(bf, "%d", k);
      gdi.addItem("StartBlock", bf, k);
    }

    gdi.popX();
	  gdi.dropLine(3);
    gdi.addSelection("Status", 200, 300, 0, "Status:");
  }
  
  gdi.popX();
	gdi.dropLine(3.5);
  gdi.addCheckbox("AllowQuickEntry", "Tillåt direktanmälan", 0);
  gdi.addCheckbox("NoTiming", "Utan tidtagning", 0);

  gdi.dropLine(2);
  gdi.popX();
  gdi.addCheckbox("FreeStart", "Fri starttid", 0, false, "Klassen lottas inte, startstämpling");

  gdi.dropLine(2);
  gdi.popX();

  gdi.fillDown();
  gdi.addString("", 1, "Funktioner");
  gdi.dropLine(0.3);
  gdi.pushX();
  gdi.fillRight();
	gdi.addButton("Draw", "Lotta / starttider...", ClassesCB).isEdit(true);
	gdi.addButton("Bibs", "Nummerlappar...", ClassesCB).isEdit(true);

  gdi.popX();
  gdi.dropLine(2.5);
  bool drop = false;
  if (cnf.hasTeamClass()) {
    drop = true;
    gdi.addButton("Restart", "Omstart...", ClassesCB).isEdit(true);
  }
  if (showAdvanced) {
    drop = true;
    gdi.addButton("Merge", "Slå ihop klasser...",  ClassesCB).isEdit(true);
	  gdi.addButton("Split", "Dela klassen...", ClassesCB).isEdit(true);
  }

  if (drop) {
    gdi.popX();
	  gdi.dropLine(2.5);
  }

	gdi.addButton("DrawMode", "Lotta flera klasser", ClassesCB);

  gdi.fillDown();
  gdi.addButton("QuickSettings", "Snabbinställningar", ClassesCB);
  
  if (showAdvanced) {
    gdi.popX();
    gdi.addButton("RemoveVacant", "Radera vakanser", ClassesCB);
  }

  gdi.popX();
  gdi.dropLine(3);
	gdi.fillRight();	
  gdi.addButton("Save", "Spara", ClassesCB).setDefault();
	gdi.disableInput("Save");
  gdi.addButton("Remove", "Radera", ClassesCB);
  gdi.disableInput("Remove");
  gdi.addButton("Add", "Ny klass", ClassesCB);

  gdi.popX();
  gdi.fillDown();
  gdi.dropLine(3);
  gdi.addCheckbox("UseAdvanced", "Visa avancerade funktioner", ClassesCB, showAdvanced).isEdit(false);

	gdi.setOnClearCb(ClassesCB);
	gdi.setRestorePoint(); 

  gdi.setCX(xp);
  gdi.setCY(gdi.getHeight());

  gdi.addString("", 10, "help:26963");
                 
  selectClass(gdi, ClassId);

	EditChanged=false;
	gdi.refresh();

  return true;
}

static int classSettingsCB(gdioutput *gdi, int type, void *data)
{
  static string lastStart = "Start 1";
  if (type==GUI_INPUT) {
		InputInfo ii=*(InputInfo *)data;

    if (ii.id.substr(0,4) == "Strt") {
      lastStart = ii.text;
    }
  }
  else if (type == GUI_FOCUS) {
    InputInfo ii=*(InputInfo *)data;
    if (ii.id.substr(0,4) == "Strt") {
      if (ii.text.empty()) {
        gdi->setText(ii.id, lastStart);
        gdi->setInputFocus(ii.id, true);
      }
    }
  }
  else if (type == GUI_BUTTON) {
    ButtonInfo bi = *(ButtonInfo*)data;
    if (bi.id == "SaveCS") {
      pEvent oe = pEvent(bi.getExtra());
      set<int> modifiedFee;
      oe->saveClassSettingsTable(*gdi, modifiedFee);

      if (!modifiedFee.empty() && oe->getNumRunners() > 0) {
        bool updateFee = gdi->ask("ask:changedclassfee");

        if (updateFee)
          oe->applyEventFees(false, true, false, modifiedFee);
      }

      gdi->sendCtrlMessage("Cancel");
    }
  }
  return 0;
}

void TabClass::prepareForDrawing(gdioutput &gdi)
{
  gdi.clearPage(false);
  gdi.addString("", 2, "Klassinställningar");
  gdi.addString("", 10, "help:59395");
  
  gdi.dropLine();
  oe->getClassSettingsTable(gdi, classSettingsCB);

  gdi.dropLine();
  gdi.fillRight();
  gdi.addButton("SaveCS", "Spara", classSettingsCB).setExtra(LPVOID(oe));
  gdi.addButton("Cancel", "Avbryt", ClassesCB);

  gdi.refresh();
}

void TabClass::drawDialog(gdioutput &gdi, TabClass::DrawMethod method, const oClass &pc) {
  if (lastDrawMethod == method)
    return;

  int firstStart = 3600, 
      interval = 120,
      vac = 0;

  bool pairWise = false;

  if (gdi.hasField("FirstStart"))
    firstStart = oe->getRelativeTime(gdi.getText("FirstStart"));

  if (gdi.hasField("Interval"))
    interval = convertAbsoluteTimeMS(gdi.getText("Interval"));
  
  if (gdi.hasField("Pairwise"))
    pairWise = gdi.isChecked("Pairwise");

  gdi.restore("MultiDayDraw", false);

  if (int(method) < 10) {
    gdi.fillRight();
    gdi.addCheckbox("HandleBibs", "Tilldela nummerlappar:", ClassesCB, false);
    gdi.dropLine(-0.2);
    gdi.addInput("Bib", "", 10, 0, "", "Mata in första nummerlappsnummer, eller blankt för att ta bort nummerlappar");
    gdi.disableInput("Bib");
    gdi.fillDown();
    gdi.dropLine();
    gdi.popX();
  }
  
  const bool multiDay = !oe->getDCI().getString("PreEvent").empty();

	gdi.popX();
	gdi.dropLine(2.5);
  
  if (method == DMRandom || method == DMSOFT || method == DMPursuit || method == DMReversePursuit)
    gdi.addCheckbox("Pairwise", "Tillämpa parstart", 0, pairWise);
  
  gdi.fillRight();

  gdi.addInput("FirstStart", oe->getAbsTime(firstStart), 10, 0, "Första start:");

  if (method == DMPursuit || method == DMReversePursuit) {
    gdi.addInput("MaxAfter", "60:00", 10, 0, "Maxtid efter:", "Maximal tid efter ledaren för att delta i jaktstart");
    gdi.addInput("TimeRestart", oe->getAbsTime(firstStart + 3600),  8, 0, "Första omstartstid:"); 
    gdi.addInput("ScaleFactor", "1.0",  8, 0, "Tidsskalning:"); 
  }

  gdi.addInput("Interval", formatTime(interval), 10, 0, "Startintervall (min):");

  if (method == DMRandom || method == DMSOFT || method == DMClumped) 
    gdi.addInput("Vacanses", itos(vac), 10, 0, "Antal vakanser:");
  
  if ((method == DMRandom || method == DMSOFT) && pc.getNumStages() > 1 && pc.getClassType() != oClassPatrol) { 
    gdi.addSelection("Leg", 90, 100, 0, "Sträcka:", "Sträcka att lotta");
    for (unsigned k = 0; k<pc.getNumStages(); k++)
      gdi.addItem("Leg", lang.tl("Sträcka X#" + itos(k+1)), k);

    gdi.selectFirstItem("Leg");
  }

  gdi.popX();
  gdi.dropLine(3.5);
  
  gdi.popX();

  gdi.addButton("DoDraw", "Lotta klassen", ClassesCB, "Lotta om hela klassen");
	
  if (method == DMRandom || method == DMSOFT) {
    gdi.addButton("DoDrawBefore", "Ej lottade, före", ClassesCB, "Lotta löpare som saknar starttid");
    gdi.addButton("DoDrawAfter", "Ej lottade, efter", ClassesCB, "Lotta löpare som saknar starttid");
  }

  gdi.fillDown();
  gdi.addButton("DoDeleteStart", "Radera starttider", ClassesCB);
  
	gdi.popX();
	gdi.dropLine();
  gdi.addButton("Cancel", "Avbryt", ClassesCB).setCancel();

  gdi.dropLine();
  
  gdi.addString("", 10, "help:41641");

  setMultiDayClass(gdi, multiDay, method);

	EditChanged=false;
	gdi.refresh();
  
  lastDrawMethod = method;
}

void TabClass::setMultiDayClass(gdioutput &gdi, bool hasMulti, TabClass::DrawMethod defaultMethod) {

  gdi.clearList("Method");
  gdi.addItem("Method", lang.tl("Lottning"), DMRandom);
  gdi.addItem("Method", lang.tl("SOFT-lottning"), DMSOFT);
  gdi.addItem("Method", lang.tl("Klungstart"), DMClumped);
  
  if (hasMulti) {
    gdi.addItem("Method", lang.tl("Jaktstart"), DMPursuit);
    gdi.addItem("Method", lang.tl("Omvänd jaktstart"), DMReversePursuit);
    //gdi.addItem("Method", lang.tl("Seedad lottning"), DMSeeded);
  }
  else if (defaultMethod > 10)
    defaultMethod = DMRandom;
  
  gdi.selectItemByData("Method", defaultMethod);

  gdi.setInputStatus("Vacanses", !hasMulti);
  gdi.setInputStatus("HandleBibs", !hasMulti);
  gdi.setInputStatus("DoDrawBefore", !hasMulti);
  gdi.setInputStatus("DoDrawAfter", !hasMulti);
}

void TabClass::pursuitDialog(gdioutput &gdi) {
  gdi.clearPage(false);
  gdi.addString("", boldLarge, "Jaktstart");
  gdi.dropLine();
  vector<pClass> cls;
  oe->getClasses(cls);

  gdi.setRestorePoint("Pursuit");

  gdi.pushX();

  gdi.fillRight();

  gdi.addInput("MaxAfter", "60:00", 10, 0, "Maxtid efter:", "Maximal tid efter ledaren för att delta i jaktstart");
  gdi.addInput("TimeRestart", "+60:00",  8, 0, "Första omstartstid:",  "Ange tiden relativt klassens första start"); 
  gdi.addInput("Interval", "2:00",  8, 0, "Startintervall:",  "Ange startintervall för minutstart"); 
  gdi.addInput("ScaleFactor", "1.0",  8, 0, "Tidsskalning:"); 


  gdi.dropLine(4);
  gdi.popX();
  gdi.fillDown();

  gdi.addCheckbox("Pairwise", "Tillämpa parstart", 0, false);

  int cx = gdi.getCX();
  int cy = gdi.getCY();

  const int len5 = gdi.scaleLength(5);
  const int len40 = gdi.scaleLength(30);
  const int len200 = gdi.scaleLength(200);

  gdi.addString("", cy, cx, 1, "Välj klasser");
  gdi.addString("", cy, cx + len200 + len40, 1, "Första starttid");
  cy += gdi.getLineHeight()*2;

  for (size_t k = 0; k<cls.size(); k++) {
    map<int, PursuitSettings>::iterator st = pSettings.find(cls[k]->getId());
    
    if (st == pSettings.end()) {
      pSettings.insert(make_pair(cls[k]->getId(), PursuitSettings(*cls[k])));
      st = pSettings.find(cls[k]->getId());
    }

    PursuitSettings &ps = st->second;

    ButtonInfo &bi = gdi.addCheckbox(cx, cy + len5, "PLUse" + itos(k), "", ClassesCB, ps.use);
    bi.setExtra(cls[k]->getId());
    gdi.addStringUT(cy, cx + len40, 0, cls[k]->getName(), len200);
    
    gdi.addInput(cx + len200 + len40, cy, "First" + itos(k), oe->getAbsTime(ps.firstTime), 8);

    if (!ps.use)
      gdi.disableInput(("First" + itos(k)).c_str());

    cy += int(gdi.getLineHeight()*1.8);
  }

  gdi.dropLine();
  gdi.fillRight();
  gdi.addButton("SelectAllNoneP", "Välj alla", ClassesCB).setExtra(1);
  gdi.addButton("SelectAllNoneP", "Välj ingen", ClassesCB).setExtra(0);
  gdi.popX();
  gdi.dropLine(3);
  gdi.addButton("DoPursuit", "Jaktstart", ClassesCB).setDefault().setExtra(1);
  gdi.addButton("DoPursuit", "Omvänd jaktstart", ClassesCB).setExtra(2);
  
  gdi.addButton("CancelPursuit", "Återgå", ClassesCB).setCancel();
  gdi.refresh();
}
