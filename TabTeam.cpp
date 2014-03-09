/************************************************************************
    MeOS - Orienteering Software
    Copyright (C) 2009-2014 Melin Software HB
    
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

#include "resource.h"

#include <commctrl.h>
#include <commdlg.h> 

#include "oEvent.h"
#include "xmlparser.h"
#include "gdioutput.h"
#include "gdiconstants.h"

#include "csvparser.h"
#include "SportIdent.h"
#include "meos_util.h"
#include <cassert>

#include "TabTeam.h"
#include "TabRunner.h"

TabTeam::TabTeam(oEvent *poe):TabBase(poe)
{
  shownRunners = 0;
  shownDistinctRunners = 0;
  teamId = 0;
  inputId = 0;
  timeToFill = 0;
  currentMode = 0;
}

TabTeam::~TabTeam(void)
{
}

int TeamCB(gdioutput *gdi, int type, void *data)
{	
  TabTeam &tt = dynamic_cast<TabTeam &>(*gdi->getTabs().get(TTeamTab));

  return tt.teamCB(*gdi, type, data);
}

int teamSearchCB(gdioutput *gdi, int type, void *data)
{
  TabTeam &tc = dynamic_cast<TabTeam &>(*gdi->getTabs().get(TTeamTab));

  return tc.searchCB(*gdi, type, data);
}

int TabTeam::searchCB(gdioutput &gdi, int type, void *data) {
  static DWORD editTick = 0;
  string expr;
  bool showNow = false;
  bool filterMore = false;

  if (type == GUI_INPUTCHANGE) {
    inputId++;
    InputInfo &ii = *(InputInfo *)(data);
    expr = trim(ii.text);
    filterMore = expr.length() > lastSearchExpr.length() &&
                  expr.substr(0, lastSearchExpr.length()) == lastSearchExpr;
    editTick = GetTickCount();
    if (expr != lastSearchExpr) {
      int nr = oe->getNumRunners();
      if (timeToFill < 50 || (filterMore && (timeToFill * lastFilter.size())/nr < 50))
        showNow = true;
      else {// Delay filter 
        gdi.addTimeoutMilli(500, "Search: " + expr, teamSearchCB).setExtra((void *)inputId);
      }
    }
  }
  else if (type == GUI_TIMER) {
    
    TimerInfo &ti = *(TimerInfo *)(data);
    
    if (inputId != int(ti.getExtra()))
      return 0;

    expr = ti.id.substr(8);
    filterMore = expr.length() > lastSearchExpr.length() &&
              expr.substr(0, lastSearchExpr.length()) == lastSearchExpr;
    showNow = true;
  }
  else if (type == GUI_EVENT) {
    EventInfo &ev = *(EventInfo *)(data);
    if (ev.getKeyCommand() == KC_FIND) {
      gdi.setInputFocus("SearchText", true);
    }
    else if (ev.getKeyCommand() == KC_FINDBACK) {
      gdi.setInputFocus("SearchText", false);
    }
  }
  else if (type == GUI_FOCUS) {
    InputInfo &ii = *(InputInfo *)(data);

    if (ii.text == getSearchString()) {
      ((InputInfo *)gdi.setText("SearchText", ""))->setFgColor(colorDefault);
    }
  }

  if (showNow) {
    stdext::hash_set<int> filter;
    
    if (type == GUI_TIMER)
      gdi.setWaitCursor(true);

    if (filterMore) {
      
      oe->findTeam(expr, 0, filter);
      lastSearchExpr = expr;
      // Filter more
      if (filter.empty()) {
        vector< pair<string, size_t> > runners;
        runners.push_back(make_pair(lang.tl("Ingen matchar 'X'#" + expr), -1));
        gdi.addItem("Teams", runners);
      }
      else
        gdi.filterOnData("Teams", filter);
    }
    else {
      oe->findTeam(expr, 0, filter);
      lastSearchExpr = expr;
     
      vector< pair<string, size_t> > runners;
      oe->fillTeams(runners);
      
      if (filter.size() == runners.size()){
      }
      else if (filter.empty()) {
        runners.clear();
        runners.push_back(make_pair(lang.tl("Ingen matchar 'X'#" + expr), -1));
      }
      else {
        vector< pair<string, size_t> > runners2;
        
        for (size_t k = 0; k<runners.size(); k++) {
          if (filter.count(runners[k].second) == 1) {
            runners2.push_back(make_pair(string(), runners[k].second));
            runners2.back().first.swap(runners[k].first);
          }
        }
        runners.swap(runners2);
      }

      gdi.addItem("Teams", runners);
    }

    if (type == GUI_TIMER)
      gdi.setWaitCursor(false);
  }

  return 0;
}


void TabTeam::selectTeam(gdioutput &gdi, pTeam t)
{
	if(t){
		t->synchronize();

    teamId=t->getId();

    gdi.enableEditControls(true);
    gdi.enableInput("Save");
    gdi.enableInput("Undo");
    gdi.enableInput("Remove");

		oe->fillClasses(gdi, "RClass", oEvent::extraNone, oEvent::filterNone);	
		gdi.selectItemByData("RClass", t->getClassId());
		gdi.selectItemByData("Teams", t->getId());
		
    loadTeamMembers(gdi, 0, 0, t);
	}
	else {
    teamId=0;
	

    gdi.enableEditControls(false);
    gdi.disableInput("Save");
    gdi.disableInput("Undo");
    gdi.disableInput("Remove");

		ListBoxInfo lbi;
		gdi.getSelectedItem("RClass", &lbi);
    
    gdi.selectItemByData("Teams", -1);
		
		loadTeamMembers(gdi, lbi.data, 0, 0);
	}
	  
	updateTeamStatus(gdi, t);	
  gdi.refresh();
}

void TabTeam::updateTeamStatus(gdioutput &gdi, pTeam t)
{
	if (!t) {
	  gdi.setText("Name", "");
		gdi.setText("StartNo", "");
		gdi.setText("Club", "");
		bool hasFee = gdi.hasField("Fee");
    if (hasFee) {
      gdi.setText("Fee", "");
    }
		gdi.setText("Start", "-");
		gdi.setText("Finish", "-");
		gdi.setText("Time", "-");
		gdi.selectItemByData("Status", 0);
		return;
	}

  gdi.setText("Name", t->getName());
	gdi.setText("StartNo", t->getBib());

	gdi.setText("Club", t->getClub());
  bool hasFee = gdi.hasField("Fee");
  if (hasFee) {
    gdi.setText("Fee", oe->formatCurrency(t->getDI().getInt("Fee")));	
  }

	gdi.setText("Start", t->getStartTimeS());
	gdi.setText("Finish",t->getFinishTimeS());
	gdi.setText("Time", t->getRunningTimeS());
	gdi.selectItemByData("Status", t->getStatus());
}

bool TabTeam::save(gdioutput &gdi, bool dontReloadTeams) {
  if (teamId==0)
    return 0;

	DWORD tid=teamId;
	string name=gdi.getText("Name");

	if (name.empty()) {
		gdi.alert("Alla lag m�ste ha ett namn.");
		return 0;
	}

	bool create=false;

	pTeam t;
	if (tid==0) {
		t=oe->addTeam(name);
		create=true;
	}
	else t=oe->getTeam(tid);

  teamId=t->getId();

	if (t) {				
		t->setName(name);
    const string &bib = gdi.getText("StartNo");
    t->setBib(bib, atoi(bib.c_str()) > 0, false); 
		t->setStartTimeS(gdi.getText("Start"));
		t->setFinishTimeS(gdi.getText("Finish"));

    if (oe->useEconomy())
      t->getDI().setInt("Fee", oe->interpretCurrency(gdi.getText("Fee")));
    
    t->apply(false, 0, false);

		ListBoxInfo lbi;
		gdi.getSelectedItem("Club", &lbi);
		
    if (!lbi.text.empty()) {
			pClub pc=oe->getClub(lbi.text);
      if (!pc)
        pc = oe->addClub(lbi.text);
			pc->synchronize();										
		}

		t->setClub(lbi.text);
    if (!dontReloadTeams) 
		  oe->fillClubs(gdi, "Club");
		gdi.setText("Club", lbi.text);

		gdi.getSelectedItem("Status", &lbi);

    RunnerStatus sIn = (RunnerStatus)lbi.data;
    // Must be done AFTER all runners are set. But setting runner can modify status, so decide here.
    bool setDNS = (sIn == StatusDNS) && (t->getStatus()!=StatusDNS);
	  bool checkStatus = (sIn != t->getStatus());
	  
    if (sIn == StatusUnknown && t->getStatus() == StatusDNS)
      t->setTeamNoStart(false);
    else if ((RunnerStatus)lbi.data != t->getStatus())
		  t->setStatus((RunnerStatus)lbi.data, true, false);
   	        
		gdi.getSelectedItem("RClass", &lbi);
		t->setClassId(lbi.data);

		pClass pc=oe->getClass(lbi.data);
		
    if (pc) {
		  for (unsigned i=0;i<pc->getNumStages(); i++) {
			  char bf[16];
			  sprintf_s(bf, "R%d", i);
        if (!gdi.hasField("SI" + itos(i))) // Skip if field not loaded in page
          continue;

        if (pc->getLegRunner(i)==i) {
  		
          const string name=gdi.getText(bf);
				  if (name.empty()) { //Remove
            t->removeRunner(gdi, true, i);
				  }
				  else {
					  pRunner r=t->getRunner(i);
					  char bf2[16];
            sprintf_s(bf2, "SI%d", i);					
					  int cardNo = gdi.getTextNo(bf2);
            
            if (r) {
              bool newName = name != r->getName();
              int oldId = int(gdi.getExtra(bf));
              // Same runner set
              if (oldId == r->getId()) {
                if (newName) {
                  r->updateFromDB(name, r->getClubId(), r->getClassId(), 
                                  cardNo, 0);
                  r->setName(name);
                }
					      r->setCardNo(cardNo, true);
                
                if(gdi.isChecked("RENT" + itos(i)))
                  r->getDI().setInt("CardFee", oe->getDI().getInt("CardFee"));
                else
                  r->getDI().setInt("CardFee", 0);

                r->synchronize(true);
                continue;
              }

              if (newName) {
						    if(!t->getClub().empty())
							    r->setClub(t->getClub());
						    r->resetPersonalData();
                r->updateFromDB(name, r->getClubId(), r->getClassId(), 
                                cardNo, 0);
              }
					  }
					  else
						  r=oe->addRunner(name, t->getClubId(), t->getClassId(), cardNo, 0, false);

					  r->setName(name);
					  r->setCardNo(cardNo, true);
					  r->synchronize();
					  t->setRunner(i, r, true);
				  }
			  }
		  }

    }
    if(setDNS)
      t->setTeamNoStart(true);

    t->evaluate(true);

    if (!dontReloadTeams) {
      fillTeamList(gdi);
      //updateTeamStatus(gdi, t);
    }
    if (checkStatus && sIn != t->getStatus()) {
      gdi.alert("Status matchar inte deltagarnas status.");
    }
	}

	if (create) {
		selectTeam(gdi, 0);
		gdi.setInputFocus("Name", true);
	}
  else if (!dontReloadTeams) {
    selectTeam(gdi, t);
  }
  return true;
}

int TabTeam::teamCB(gdioutput &gdi, int type, void *data)
{
	if(type==GUI_BUTTON) {
		ButtonInfo bi=*(ButtonInfo *)data;

		if (bi.id=="Save") {
			return save(gdi, false);
		}
    else if (bi.id=="TableMode") {
      if (currentMode == 0 && teamId>0)
        save(gdi, true);

      currentMode = 1;
      loadPage(gdi);
    }
    else if (bi.id=="FormMode") {
      if (currentMode != 0) {
        currentMode = 0;
        gdi.enableTables();
        loadPage(gdi);
      }
    }
    if (bi.id=="Undo") {
      pTeam t = oe->getTeam(teamId);     
      selectTeam(gdi, t);
      return 0;
		}
    else if (bi.id=="Search") {
      ListBoxInfo lbi;
      gdi.getSelectedItem("Teams", &lbi);
      oe->fillTeams(gdi, "Teams");
      stdext::hash_set<int> foo;
      pTeam t=oe->findTeam(gdi.getText("SearchText"), lbi.data, foo);

      if(t) {
        selectTeam(gdi, t);
        gdi.selectItemByData("Teams", t->getId());
      }
      else
        gdi.alert("Laget hittades inte");
    }
    else if (bi.id == "ShowAll") {
      fillTeamList(gdi);
    }
    else if (bi.id.substr(0,2)=="MR") { 
      int leg = atoi(bi.id.substr(2, string::npos).c_str());

      if (teamId != 0) {
        save(gdi, false);
        pTeam t = oe->getTeam(teamId);
        if (t != 0) {
          pRunner r = t->getRunner(leg);
          if (r) {
            TabRunner *tr = (TabRunner *)gdi.getTabs().get(TRunnerTab);
            tr->loadPage(gdi, r->getId());
          }
        }
      }
    }
    else if (bi.id.substr(0,2)=="DR") { // NOT USED ANY MORE
      int leg=atoi(bi.id.substr(2, string::npos).c_str());

      char bf[16], bf2[128];
      sprintf_s(bf, "SR%d", leg);
      gdi.restore("SelectR");
      gdi.setRestorePoint("SelectR");
      gdi.dropLine();
      gdi.fillDown();

      sprintf_s(bf2, "V�lj l�pare f�r str�cka X#%d", leg+1);
      gdi.addString("", boldText, bf2);
      gdi.dropLine(0.5);
      gdi.fillRight();
      gdi.addSelection("SelectR", 180, 300, TeamCB);
      gdi.fillDown();
      gdi.addButton("SelectRunner", "OK", TeamCB).setExtra(leg);
      oe->fillRunners(gdi, "SelectR", false, 0);
      pTeam t = oe->getTeam(teamId);
      pRunner r = t ? t->getRunner(leg) : 0;
      gdi.selectItemByData("SelectR", r ? r->getId() : -1);
      gdi.refresh();
    }
    else if (bi.id=="SelectRunner") {
      ListBoxInfo lbi;
      gdi.getSelectedItem("SelectR", &lbi);
      pRunner r=oe->getRunner(lbi.data, 0);

      if (teamId == 0)
        save(gdi, false);

      pTeam t = oe->getTeam(teamId);
      
      if (t != 0) {
        t->setRunner(int(bi.getExtra()), r, true);
        t->evaluate(true);
        selectTeam(gdi, t);
      }
    }
		else if (bi.id=="Add") {
      if (teamId>0) {

        string name = gdi.getText("Name");
        pTeam t = oe->getTeam(teamId);
        if (!name.empty() && t && t->getName() != name) {
          if (gdi.ask("Vill du l�gga till laget 'X'?#" + name)) {
            t = oe->addTeam(name);
            teamId = t->getId();
          }
          save(gdi, false);
          return true;
        }

        save(gdi, false);
      }

      pTeam t = oe->addTeam(oe->getAutoTeamName());
      t->setStartNo(oe->getFreeStartNo(), false);
      
      ListBoxInfo lbi;
      gdi.getSelectedItem("RClass", &lbi);

      if (signed(lbi.data)>0)
        t->setClassId(lbi.data);
      else  
        t->setClassId(oe->getFirstClassId(true));
      
      fillTeamList(gdi);
      //oe->fillTeams(gdi, "Teams");
      selectTeam(gdi, t);

			//selectTeam(gdi, 0);
			//gdi.selectItemByData("Teams", -1);
			gdi.setInputFocus("Name", true);
		}
		else if (bi.id=="Remove") {
			DWORD tid=teamId;
     
			if(tid==0)
        throw std::exception("Inget lag valt.");
		
      pTeam t = oe->getTeam(tid);
      
      if (!t || t->isRemoved()) {
        selectTeam(gdi, 0);
      }
      else if (gdi.ask("Vill du verkligen ta bort laget?")) {
        vector<int> runners;
        vector<int> noRemove;
        for (int k = 0; k < t->getNumRunners(); k++) {
          pRunner r = t->getRunner(k);
          if (r && r->getRaceNo() == 0) {
            if (r->getCard() == 0)
              runners.push_back(r->getId());
            else 
              noRemove.push_back(r->getId());
          }
        }
				oe->removeTeam(tid);
        oe->removeRunner(runners);
        
        for (size_t k = 0; k<noRemove.size(); k++) {
          pRunner r = oe->getRunner(noRemove[k], 0);
          if (r) {
            r->setClassId(0);
            r->synchronize();
          }
        }
        
        fillTeamList(gdi);
				//oe->fillTeams(gdi, "Teams");
				selectTeam(gdi, 0);
				gdi.selectItemByData("Teams", -1);
			}
		}
	}
	else if (type==GUI_LISTBOX) {
		ListBoxInfo bi=*(ListBoxInfo *)data;

		if(bi.id=="Teams") {
      if (gdi.isInputChanged(""))
        save(gdi, true);

			pTeam t=oe->getTeam(bi.data);
			selectTeam(gdi, t);
		}
		else if(bi.id=="RClass") { //New class selected.
			DWORD tid=teamId;
			//gdi.getData("TeamID", tid);

			if(tid){
				pTeam t=oe->getTeam(tid);
        pClass pc=oe->getClass(bi.data);

        if (pc && pc->getNumDistinctRunners() == shownDistinctRunners && 
          pc->getNumStages() == shownRunners) {
            // Keep team setup, i.e. do nothing
        }
        else if(t && pc && (t->getClassId()==bi.data 
                || t->getNumRunners()==pc->getNumStages()) )
					loadTeamMembers(gdi, 0,0,t);
				else
					loadTeamMembers(gdi, bi.data, 0, t);
			}
			else loadTeamMembers(gdi, bi.data, 0, 0);
		}
		else {

			ListBoxInfo lbi;
			gdi.getSelectedItem("RClass", &lbi);

			if(signed(lbi.data)>0){
				pClass pc=oe->getClass(lbi.data);

				if(pc){
					for(unsigned i=0;i<pc->getNumStages();i++){
						char bf[16];
						sprintf_s(bf, "R%d", i);
						if(bi.id==bf){
							pRunner r=oe->getRunner(bi.data, 0);
              if (r) {
							  sprintf_s(bf, "SI%d", i);
                int cno = r->getCardNo();
                gdi.setText(bf, cno > 0 ? itos(cno) : "");
              }
						}
					}
				}
			}
		}
	}
  else if (type==GUI_INPUTCHANGE) {
    InputInfo &bi=*(InputInfo *)data;
    pClass pc=oe->getClass(classId);
		if(pc){
			for(unsigned i=0;i<pc->getNumStages();i++){
				char bf[16];
				sprintf_s(bf, "R%d", i);
				if (bi.id==bf) {
          for (unsigned k=i+1; k<pc->getNumStages(); k++) {
            if(pc->getLegRunner(k)==i) {
              sprintf_s(bf, "R%d", k);
              gdi.setText(bf, bi.text);
            }
          }
          break;
				}
			}
		}
  }
  else if(type==GUI_CLEAR) {
    if (teamId>0)
      save(gdi, true);

    return true;
  }
	return 0;
}


void TabTeam::loadTeamMembers(gdioutput &gdi, int ClassId, int ClubId, pTeam t)
{	
	if(ClassId==0)
		if(t) ClassId=t->getClassId();

  classId=ClassId;
	gdi.restore("",false);

  pClass pc=oe->getClass(ClassId);
	if(!pc) return;

  shownRunners = pc->getNumStages();
  shownDistinctRunners = pc->getNumDistinctRunners();

  gdi.setRestorePoint();
 	gdi.newColumn();

  gdi.fillDown();
	char bf[16];	
	char bf_si[16];
  int xp = gdi.getCX();
  int yp = gdi.getCY();
  int numberPos = xp;
  xp += gdi.scaleLength(25);
  int dx[6] = {0, 188, 220, 290, 316, 364};
  for (int i = 0; i<6; i++)
    dx[i] = gdi.scaleLength(dx[i]);

  gdi.addString("", yp, xp + dx[0], 0, "Namn:");
  gdi.addString("", yp, xp + dx[2], 0, "Bricka:");
  gdi.addString("", yp, xp + dx[3], 0, "Hyrd:");
  gdi.addString("", yp, xp + dx[5], 0, "Status:");
  gdi.dropLine(0.5);

	for (unsigned i=0;i<pc->getNumStages();i++) {
    yp = gdi.getCY();
 
    sprintf_s(bf, "R%d", i);
		gdi.pushX();
		bool hasSI = false;
    gdi.addStringUT(yp, numberPos, 0, pc->getLegNumber(i)+".");
    if(pc->getLegRunner(i)==i) {
      		
      gdi.addInput(xp + dx[0], yp, bf, "", 18, TeamCB);//Name  
      gdi.addButton(xp + dx[1], yp-2, gdi.scaleLength(28), "DR" + itos(i), "<>", TeamCB, "Knyt l�pare till str�ckan.", false, false); // Change
      sprintf_s(bf_si, "SI%d", i);
      hasSI = true;
      gdi.addInput(xp + dx[2], yp, bf_si, "", 5).width; //Si

      gdi.addCheckbox(xp + dx[3], yp + gdi.scaleLength(10), "RENT"+itos(i), "", 0, false); //Rentcard
      gdi.addButton(xp + dx[4], yp-2,  gdi.scaleLength(38), "MR" + itos(i), "...", TeamCB, "Redigera deltagaren.", false, false); // Change
  
      gdi.addString(("STATUS"+itos(i)).c_str(), yp+gdi.scaleLength(5), xp + dx[5], 0, "#MMMMMMMMMMMMMMMM");
      gdi.setText("STATUS"+itos(i), "", false);
      gdi.dropLine(0.5); 
		  gdi.popX();
    }
    else {
      //gdi.addInput(bf, "", 24);
      gdi.addInput(xp + dx[0], yp, bf, "", 18, 0);//Name  
      gdi.disableInput(bf);
    }
    
		if (t) {
			pRunner r=t->getRunner(i);
			if (r) {
        gdi.setText(bf, r->getName())->setExtra((void *)r->getId());
        
        if(hasSI) {
				  gdi.setText(bf_si, r->getCardNo());
          gdi.check("RENT" + itos(i), r->getDCI().getInt("CardFee") != 0);
        }
        string sid = "STATUS"+itos(i);
        if (r->statusOK()) {
          TextInfo * ti = (TextInfo *)gdi.setText(sid, "OK, " + r->getRunningTimeS(), false);
          if(ti)
            ti->setColor(colorGreen);
        }
        else if (r->getStatus() != StatusUnknown) {
          TextInfo * ti = (TextInfo *)gdi.setText(sid, r->getStatusS() + ", " + r->getRunningTimeS(), false);
          if (ti)
            ti->setColor(colorRed);
        }
			}
		}
	}

	gdi.addString("", 10, "help:7618");
  gdi.refresh();
}

bool TabTeam::loadPage(gdioutput &gdi, int id) {
  teamId = id;
  return loadPage(gdi);
}

bool TabTeam::loadPage(gdioutput &gdi)
{
  shownRunners = 0;
  shownDistinctRunners = 0;

  oe->checkDB();
  oe->reEvaluateAll(set<int>(), true);

	gdi.selectTab(tabId);
	gdi.clearPage(false);

  if(currentMode == 1) {
    Table *tbl=oe->getTeamsTB();
    addToolbar(gdi);
    gdi.dropLine(1);
    gdi.addTable(tbl, gdi.getCX(), gdi.getCY());
    return true;
  }


	gdi.fillDown();
	gdi.addString("", boldLarge, "Lag(flera)");

  gdi.pushX();
  gdi.fillRight();

  gdi.registerEvent("SearchRunner", teamSearchCB).setKeyCommand(KC_FIND);
  gdi.registerEvent("SearchRunnerBack", teamSearchCB).setKeyCommand(KC_FINDBACK);
  gdi.addInput("SearchText", "", 17, teamSearchCB, "", "S�k p� namn, bricka eller startnummer.").isEdit(false).setBgColor(colorLightCyan).ignore(true);
  gdi.dropLine(-0.2);
//  gdi.addButton("Search", "S�k", TeamCB, "S�k p� namn, bricka eller startnummer.").isEdit(false);
  //gdi.dropLine(-0.2);
  //gdi.addButton("Search", "S�k", RunnerCB, "S�k p� namn, bricka eller startnummer.");
  gdi.addButton("ShowAll", "Visa alla", TeamCB).isEdit(false);
  
  gdi.dropLine(2);
  gdi.popX();
	gdi.fillDown();
	gdi.addListBox("Teams", 250, 400, TeamCB, "", "").isEdit(false).ignore(true);
  gdi.setInputFocus("Teams");
  fillTeamList(gdi);
	
  gdi.fillRight();	

	gdi.newColumn();
	gdi.fillDown();
  gdi.pushX();
	gdi.addInput("Name", "", 24, 0, "Lagnamn:");

  gdi.fillRight();
	gdi.addInput("StartNo", "", 4, 0, "Nr", "Nummerlapp");

	gdi.addCombo("Club", 180, 300, 0, "Klubb:");
	oe->fillClubs(gdi, "Club");

  gdi.dropLine(3);
  gdi.popX();


  gdi.addSelection("RClass", 170, 300, TeamCB, "Klass:");
	oe->fillClasses(gdi, "RClass", oEvent::extraNone, oEvent::filterNone);	
	gdi.addItem("RClass", lang.tl("Ny klass"), 0);
  
  if (oe->useEconomy())
    gdi.addInput("Fee", "", 5, 0, "Avgift:");

  gdi.popX();
  gdi.fillDown();
  gdi.dropLine(3);
  
	gdi.pushX();
	gdi.fillRight();

	gdi.addInput("Start", "", 6, 0, "Starttid:");
	
	gdi.fillDown();
	gdi.addInput("Finish", "", 6, 0, "M�ltid:");
	
	gdi.popX();
	
	gdi.pushX();
	gdi.fillRight();

  gdi.addInput("Time", "", 6, 0, "Tid:").isEdit(false).ignore(true);
	gdi.disableInput("Time");

	gdi.fillDown();
	gdi.addSelection("Status", 100, 160, 0, "Status:");
	oe->fillStatus(gdi, "Status");

	gdi.popX();
	gdi.selectItemByData("Status", 0);

	gdi.dropLine(1.5);
  gdi.fillRight();
	gdi.addButton("Save", "Spara", TeamCB, "help:save");
  gdi.disableInput("Save");
  gdi.addButton("Undo", "�ngra", TeamCB);
  gdi.disableInput("Undo");

  gdi.popX();
  gdi.dropLine(2.5);
	gdi.addButton("Remove", "Radera", TeamCB);
  gdi.disableInput("Remove");
  gdi.addButton("Add", "Nytt lag", TeamCB);

  gdi.setOnClearCb(TeamCB);

  addToolbar(gdi);
	gdi.setRestorePoint();

  selectTeam(gdi, oe->getTeam(teamId));

	gdi.refresh();
  return true;
}

void TabTeam::fillTeamList(gdioutput &gdi) {
  timeToFill = GetTickCount();
  oe->fillTeams(gdi, "Teams");
  timeToFill = GetTickCount() - timeToFill;
  lastSearchExpr = "";
  ((InputInfo *)gdi.setText("SearchText", getSearchString()))->setFgColor(colorGreyBlue);
    lastFilter.clear();
}


const string &TabTeam::getSearchString() const {
  return lang.tl("S�k (X)#Ctrl+F");
}

void TabTeam::addToolbar(gdioutput &gdi) const {

  const int button_w=gdi.scaleLength(130);

  gdi.addButton(2+0*button_w, 2, button_w, "FormMode", 
    "Formul�rl�ge", TeamCB, "", false, true).fixedCorner();
  gdi.check("FormMode", currentMode==0);

  gdi.addButton(2+1*button_w, 2, button_w, "TableMode", 
            "Tabell�ge", TeamCB, "", false, true).fixedCorner();
  gdi.check("TableMode", currentMode==1);
  
}