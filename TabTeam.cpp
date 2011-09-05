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

#include "stdafx.h"

#include "resource.h"

#include <commctrl.h>
#include <commdlg.h> 

#include "oEvent.h"
#include "xmlparser.h"
#include "gdioutput.h"
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
  teamId=0;
}

TabTeam::~TabTeam(void)
{
}

int TeamCB(gdioutput *gdi, int type, void *data)
{	
  TabTeam &tt = dynamic_cast<TabTeam &>(*gdi->getTabs().get(TTeamTab));

  return tt.teamCB(*gdi, type, data);
}

void TabTeam::selectTeam(gdioutput &gdi, pTeam t)
{
	if(t){
		t->synchronize();

    teamId=t->getId();
		gdi.setText("Name", t->getName());
    int bib = t->getBib();
		gdi.setText("StartNo", bib > 0 ? itos(bib) : "");

		gdi.setText("Club", t->getClub());

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
		gdi.setText("Name", "");
		gdi.setText("StartNo", "");
		gdi.setText("Club", "");

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
		gdi.setText("Start", "-");
		gdi.setText("Finish", "-");
		gdi.setText("Time", "-");
		gdi.selectItemByData("Status", 0);
		return;
	}

	gdi.setText("Start", t->getStartTimeS());
	gdi.setText("Finish",t->getFinishTimeS());
	gdi.setText("Time", t->getRunningTimeS());
	gdi.selectItemByData("Status", t->getStatus());
}

bool TabTeam::save(gdioutput &gdi) {
  if (teamId==0)
    return 0;

	DWORD tid=teamId;
	string name=gdi.getText("Name");

	if (name.empty()) {
		gdi.alert("Alla lag måste ha ett namn.");
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
    int bib = gdi.getTextNo("StartNo");
		t->setBib(bib, bib>0); 
		t->setStartTimeS(gdi.getText("Start"));
		t->setFinishTimeS(gdi.getText("Finish"));

		ListBoxInfo lbi;
		gdi.getSelectedItem("Club", &lbi);
		
    if (!lbi.text.empty()) {
			pClub pc=oe->getClub(lbi.text);
      if (!pc)
        pc = oe->addClub(lbi.text);
			pc->synchronize();										
		}

		t->setClub(lbi.text);
		oe->fillClubs(gdi, "Club");
		gdi.setText("Club", lbi.text);

		gdi.getSelectedItem("Status", &lbi);
		t->setStatus((RunnerStatus)lbi.data);
		        
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
					  pRunner p_old=t->getRunner(i);
					  t->setRunner(i, 0, true);

            //Remove multi runners
            for (unsigned k=i+1;k<pc->getNumStages(); k++) 
              if (pc->getLegRunner(k)==i) 
                t->setRunner(k, 0, true);

            //No need to delete multi runners. Disappears when parent is gone.
					  if(p_old && !oe->isRunnerUsed(p_old->getId())){
              if(gdi.ask("Ska X raderas från tävlingen?#" + p_old->getName())){
							  oe->removeRunner(p_old->getId());
                t = oe->getTeam(teamId);
                if (t == 0) {
                  // Team was removed
                  selectTeam(gdi, 0);
                  return false;
                }
						  }
					  }
				  }
				  else {
					  pRunner r=t->getRunner(i);
					  char bf2[16];
            sprintf_s(bf2, "SI%d", i);					
					  int cardNo = gdi.getTextNo(bf2);
            
            if(r) {
              // Same runner set
              if (int(gdi.getExtra(bf))==r->getId()) {
                r->updateFromDB(name, r->getClubId(), r->getClassId(), 
                                cardNo, 0);
                r->setName(name);
					      r->setCardNo(cardNo, true);
                
                if(gdi.isChecked("RENT" + itos(i)))
                  r->getDI().setInt("CardFee", oe->getDI().getInt("CardFee"));
                else
                  r->getDI().setInt("CardFee", 0);

                r->synchronize();
                continue;
              }

						  if(!t->getClub().empty())
							  r->setClub(t->getClub());
						  r->resetPersonalData();
              r->updateFromDB(name, r->getClubId(), r->getClassId(), 
                              cardNo, 0);
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
    if(t->getStatus()==StatusDNS)
      t->setTeamNoStart();

    t->evaluate(true);

		oe->fillTeams(gdi, "Teams");
		gdi.selectItemByData("Teams", t->getId());

		gdi.setText("Time", t->getRunningTimeS());
		gdi.selectItemByData("Status", t->getStatus());
	}

	if (create) {
		selectTeam(gdi, 0);
		gdi.setInputFocus("Name", true);
	}
  return true;
}

int TabTeam::teamCB(gdioutput &gdi, int type, void *data)
{
	if(type==GUI_BUTTON) {
		ButtonInfo bi=*(ButtonInfo *)data;

		if (bi.id=="Save") {
			return save(gdi);
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
      pTeam t=oe->findTeam(gdi.getText("SearchText"), lbi.data);

      if(t) {
        selectTeam(gdi, t);
        gdi.selectItemByData("Teams", t->getId());
      }
      else
        gdi.alert("Laget hittades inte");
    }
    else if (bi.id.substr(0,2)=="MR") { 
      int leg = atoi(bi.id.substr(2, string::npos).c_str());

      if (teamId != 0) {
        save(gdi);
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

      sprintf_s(bf2, "Välj löpare för sträcka X#%d", leg+1);
      gdi.addString("", boldText, bf2);
      gdi.dropLine(0.5);
      gdi.fillRight();
      gdi.addSelection("SelectR", 180, 300, TeamCB);
      gdi.fillDown();
      gdi.addButton("SelectRunner", "OK", TeamCB).setExtra(LPVOID(leg));
      oe->fillRunners(gdi, "SelectR", false);
      pTeam t = oe->getTeam(teamId);
      pRunner r = t ? t->getRunner(leg) : 0;
      gdi.selectItemByData("SelectR", r ? r->getId() : -1);
      gdi.refresh();
    }
    else if (bi.id=="SelectRunner") { // NOT USED ANY MORE
      ListBoxInfo lbi;
      gdi.getSelectedItem("SelectR", &lbi);
      pRunner r=oe->getRunner(lbi.data, 0);

      if (teamId == 0)
        save(gdi);

      pTeam t = oe->getTeam(teamId);
      
      if (t != 0) {
        t->setRunner(int(bi.getExtra()), r, true);
        t->evaluate(true);
        selectTeam(gdi, t);
      }
    }
		else if (bi.id=="Add") {
      if (teamId>0)
        save(gdi);

      pTeam t = oe->addTeam(oe->getAutoTeamName());
      t->setStartNo(oe->getFreeStartNo());
      
      ListBoxInfo lbi;
      gdi.getSelectedItem("RClass", &lbi);

      if (signed(lbi.data)>0)
        t->setClassId(lbi.data);
      else  
        t->setClassId(oe->getFirstClassId(true));
      
      oe->fillTeams(gdi, "Teams");
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
        
        for (size_t k = 0; k<runners.size(); k++) {
          oe->removeRunner(runners[k]);
        }
        for (size_t k = 0; k<noRemove.size(); k++) {
          pRunner r = oe->getRunner(noRemove[k], 0);
          if (r) {
            r->setClassId(0);
            r->synchronize();
          }
        }
				oe->fillTeams(gdi, "Teams");
				selectTeam(gdi, 0);
				gdi.selectItemByData("Teams", -1);
			}
		}
	}
	else if (type==GUI_LISTBOX) {
		ListBoxInfo bi=*(ListBoxInfo *)data;

		if(bi.id=="Teams") {
      if (gdi.isInputChanged(""))
        save(gdi);

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
					loadTeamMembers(gdi, bi.data, 0, 0);
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
							  gdi.setText(bf, r->getCardNo());
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
      save(gdi);

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
 
  //int dx[5] = {0, 190, 260, 290, 334};
  int dx[6] = {0, 188, 220, 290, 316, 364};
  for (int i = 0; i<6; i++)
    dx[i] = gdi.scaleLength(dx[i]);

  gdi.addString("", yp, xp + dx[0], 0, "Namn:");
  gdi.addString("", yp, xp + dx[2], 0, "Bricka:");
  gdi.addString("", yp, xp + dx[3], 0, "Hyrd:");
  gdi.addString("", yp, xp + dx[5], 0, "Status:");
  //gdi.addString("", yp, xp + dx[3], 0, "Ändra:");
  gdi.dropLine(0.5);
	for (unsigned i=0;i<pc->getNumStages();i++) {
    yp = gdi.getCY();
 
    sprintf_s(bf, "R%d", i);
		gdi.pushX();
		bool hasSI=false;

    if(pc->getLegRunner(i)==i) {
		
      gdi.addInput(xp + dx[0], yp, bf, "", 16, TeamCB);//Name  
      gdi.addButton(xp + dx[1], yp-2, gdi.scaleLength(28), "DR" + itos(i), "<>", TeamCB, "Knyt löpare till sträckan.", false, false); // Change
      sprintf_s(bf_si, "SI%d", i);
      hasSI=true;
      gdi.addInput(xp + dx[2], yp, bf_si, "", 5).width; //Si

      gdi.addCheckbox(xp + dx[3], yp + gdi.scaleLength(10), "RENT"+itos(i), "", 0, false); //Rentcard
      gdi.addButton(xp + dx[4], yp-2,  gdi.scaleLength(38), "MR" + itos(i), "...", TeamCB, "Redigera deltagaren.", false, false); // Change
  
      gdi.addString(("STATUS"+itos(i)).c_str(), yp+gdi.scaleLength(5), xp + dx[5], 0, "#MMMMMMMMMMMMMMMM");
      gdi.setText("STATUS"+itos(i), "", false);
      gdi.dropLine(0.5); 
		  gdi.popX();
    }
    else {
      gdi.addInput(bf, "", 24);
      gdi.disableInput(bf);
    }
    
		if (t) {
			pRunner r=t->getRunner(i);
			if (r) {
        gdi.setText(bf, r->getName())->setExtra((void *)r->getId());
        
        if(hasSI)
				  gdi.setText(bf_si, r->getCardNo());

         gdi.check("RENT" + itos(i), r->getDCI().getInt("CardFee") > 0);
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
  oe->reEvaluateAll(true);

	gdi.selectTab(tabId);
	gdi.clearPage(false);
	gdi.fillDown();
	gdi.addString("", boldLarge, "Lag");

  gdi.pushX();
  gdi.fillRight();
  gdi.addInput("SearchText", "", 17, 0, "", "Sök på namn, bricka eller startnummer.").isEdit(false).setBgColor(colorLightCyan).ignore(true);
  gdi.dropLine(-0.2);
  gdi.addButton("Search", "Sök", TeamCB, "Sök på namn, bricka eller startnummer.").isEdit(false);
  gdi.dropLine(2);
  gdi.popX();
	gdi.fillDown();
	gdi.addListBox("Teams", 250, 400, TeamCB, "", "").isEdit(false).ignore(true);
	oe->fillTeams(gdi, "Teams");

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
  gdi.fillDown();
  gdi.popX();
	//gdi.addInput("Class", "", 16, 0, "Klass:");
	gdi.addSelection("RClass", 180, 300, TeamCB, "Klass:");
	oe->fillClasses(gdi, "RClass", oEvent::extraNone, oEvent::filterNone);	
	gdi.addItem("RClass", lang.tl("Ny klass"), 0);

	gdi.pushX();
	gdi.fillRight();

	gdi.addInput("Start", "", 6, 0, "Starttid:");
	
	gdi.fillDown();
	gdi.addInput("Finish", "", 6, 0, "Måltid:");
	
	gdi.popX();
	
	gdi.pushX();
	gdi.fillRight();

	gdi.addInput("Time", "", 6, 0, "Löptid:");
	gdi.disableInput("Time");

	gdi.fillDown();
	gdi.addSelection("Status", 90, 160, 0, "Status:");
	oe->fillStatus(gdi, "Status");

	gdi.popX();
	gdi.selectItemByData("Status", 0);

	gdi.dropLine(1.5);
  gdi.fillRight();
	gdi.addButton("Save", "Spara", TeamCB, "help:save");
  gdi.disableInput("Save");
  gdi.addButton("Undo", "Ångra", TeamCB);
  gdi.disableInput("Undo");

  gdi.popX();
  gdi.dropLine(2.5);
	gdi.addButton("Remove", "Radera", TeamCB);
  gdi.disableInput("Remove");
  gdi.addButton("Add", "Nytt lag", TeamCB);

  gdi.setOnClearCb(TeamCB);

	gdi.setRestorePoint();

  selectTeam(gdi, oe->getTeam(teamId));

	gdi.refresh();
  return true;
}
