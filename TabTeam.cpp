/************************************************************************
    MeOS - Orienteering Software
    Copyright (C) 2009-2017 Melin Software HB

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
    Eksoppsv�gen 16, SE-75646 UPPSALA, Sweden

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

#include "meosexception.h"
#include "TabTeam.h"
#include "TabRunner.h"
#include "MeOSFeatures.h"
#include "RunnerDB.h"

#include "TabSI.h"

TabTeam::TabTeam(oEvent *poe):TabBase(poe)
{
  clearCompetitionData();
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
  if (t){
    t->synchronize();
    t->evaluate(false);

    teamId=t->getId();

    gdi.enableEditControls(true);
    gdi.enableInput("Save");
    gdi.enableInput("Undo");
    gdi.enableInput("Remove");

    oe->fillClasses(gdi, "RClass", oEvent::extraNone, oEvent::filterNone);
    gdi.selectItemByData("RClass", t->getClassId());
    gdi.selectItemByData("Teams", t->getId());

    if (gdi.hasField("StatusIn")) {
      gdi.selectItemByData("StatusIn", t->getInputStatus());
      int ip = t->getInputPlace();
      if (ip > 0)
        gdi.setText("PlaceIn", ip);
      else
        gdi.setText("PlaceIn", MakeDash("-"));

      gdi.setText("TimeIn", t->getInputTimeS());
      if (gdi.hasField("PointIn"))
        gdi.setText("PointIn", t->getInputPoints());
    }

    loadTeamMembers(gdi, 0, 0, t);
  }
  else {
    teamId=0;

    gdi.enableEditControls(false);
    gdi.disableInput("Save");
    gdi.disableInput("Undo");
    gdi.disableInput("Remove");

    ListBoxInfo lbi;
    gdi.getSelectedItem("RClass", lbi);

    gdi.selectItemByData("Teams", -1);

    if (gdi.hasField("StatusIn")) {
      gdi.selectFirstItem("StatusIn");
      gdi.setText("PlaceIn", "");
      gdi.setText("TimeIn", "-");
      if (gdi.hasField("PointIn"))
        gdi.setText("PointIn", "");
    }

    loadTeamMembers(gdi, lbi.data, 0, 0);
  }

  updateTeamStatus(gdi, t);
  gdi.refresh();
}

void TabTeam::updateTeamStatus(gdioutput &gdi, pTeam t)
{
  if (!t) {
    gdi.setText("Name", "");
    if (gdi.hasField("StartNo"))
      gdi.setText("StartNo", "");
    if (gdi.hasField("Club"))
      gdi.setText("Club", "");
    bool hasFee = gdi.hasField("Fee");
    if (hasFee) {
      gdi.setText("Fee", "");
    }
    gdi.setText("Start", "-");
    gdi.setText("Finish", "-");
    gdi.setText("Time", "-");
    gdi.selectItemByData("Status", 0);
    gdi.setText("TimeAdjust", "-");
    gdi.setText("PointAdjust", "-");

    return;
  }

  gdi.setText("Name", t->getName());
  if (gdi.hasField("StartNo"))
    gdi.setText("StartNo", t->getBib());

  if (gdi.hasField("Club"))
    gdi.setText("Club", t->getClub());
  bool hasFee = gdi.hasField("Fee");
  if (hasFee) {
    gdi.setText("Fee", oe->formatCurrency(t->getDI().getInt("Fee")));
  }

  gdi.setText("Start", t->getStartTimeS());
  gdi.setText("Finish",t->getFinishTimeS());
  gdi.setText("Time", t->getRunningTimeS());
  gdi.setText("TimeAdjust", getTimeMS(t->getTimeAdjustment()));
  gdi.setText("PointAdjust", -t->getPointAdjustment());
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
  bool bibModified = false;
  if (t) {
    t->setName(name, true);
    if (gdi.hasField("StartNo")) {
      const string &bib = gdi.getText("StartNo");
      if (bib != t->getBib()) {
        bibModified = true;
        char pat[32];
        int no = oClass::extractBibPattern(bib, pat);
        t->setBib(bib, no, no > 0, false);
      }
    }
    string start = gdi.getText("Start");
    t->setStartTimeS(start);
    if (t->getRunner(0))
      t->getRunner(0)->setStartTimeS(start);

    t->setFinishTimeS(gdi.getText("Finish"));

    if (gdi.hasField("Fee"))
      t->getDI().setInt("Fee", oe->interpretCurrency(gdi.getText("Fee")));

    t->apply(false, 0, false);

    if (gdi.hasField("Club")) {
      ListBoxInfo lbi;
      gdi.getSelectedItem("Club", lbi);

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
    }
    ListBoxInfo lbi;
    gdi.getSelectedItem("Status", lbi);

    RunnerStatus sIn = (RunnerStatus)lbi.data;
    // Must be done AFTER all runners are set. But setting runner can modify status, so decide here.
    bool setDNS = (sIn == StatusDNS) && (t->getStatus()!=StatusDNS);
    bool checkStatus = (sIn != t->getStatus());

    if (sIn == StatusUnknown && t->getStatus() == StatusDNS)
      t->setTeamNoStart(false);
    else if ((RunnerStatus)lbi.data != t->getStatus())
      t->setStatus((RunnerStatus)lbi.data, true, false);

    gdi.getSelectedItem("RClass", lbi);

    int classId = lbi.data;
    bool newClass = t->getClassId() != classId;
    set<int> classes;
    bool globalDep = false;
    if (t->getClassRef())
      globalDep = t->getClassRef()->hasClassGlobalDependance();

    classes.insert(classId);
    classes.insert(t->getClassId());

    bool readStatusIn = true;
    if (newClass && t->getInputStatus() != StatusNotCompetiting && t->hasInputData()) {
      if (gdi.ask("Vill du s�tta resultatet fr�n tidigare etapper till <Deltar ej>?")) {
        t->resetInputData();
        readStatusIn = false;
      }
    }

    if (newClass && !bibModified) {
      pClass pc = oe->getClass(classId);
      if (pc) {
        pair<int, string> snoBib = pc->getNextBib();
        if (snoBib.first > 0) {
          t->setBib(snoBib.second, snoBib.first, true, false);
        }
      }
    }
    
    t->setClassId(classId, true);

    if (gdi.hasField("TimeAdjust")) {
      int time = convertAbsoluteTimeMS(gdi.getText("TimeAdjust"));
      if (time != NOTIME)
        t->setTimeAdjustment(time);
    }
    if (gdi.hasField("PointAdjust")) {
      t->setPointAdjustment(-gdi.getTextNo("PointAdjust"));
    }

    if (gdi.hasField("StatusIn") && readStatusIn) {
      t->setInputStatus(RunnerStatus(gdi.getSelectedItem("StatusIn").first));
      t->setInputPlace(gdi.getTextNo("PlaceIn"));
      t->setInputTime(gdi.getText("TimeIn"));
      if (gdi.hasField("PointIn"))
        t->setInputPoints(gdi.getTextNo("PointIn"));
    }

    pClass pc=oe->getClass(classId);

    if (pc) {
      globalDep |= pc->hasClassGlobalDependance();

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
              int oldId = gdi.getExtraInt(bf);
              // Same runner set
              if (oldId == r->getId()) {
                if (newName) {
                  r->updateFromDB(name, r->getClubId(), r->getClassId(),
                                  cardNo, 0);
                  r->setName(name, true);
                }
                r->setCardNo(cardNo, true);

                if (gdi.isChecked("RENT" + itos(i)))
                  r->getDI().setInt("CardFee", oe->getDI().getInt("CardFee"));
                else
                  r->getDI().setInt("CardFee", 0);

                r->synchronize(true);
                continue;
              }

              if (newName) {
                if (!t->getClub().empty())
                  r->setClub(t->getClub());
                r->resetPersonalData();
                r->updateFromDB(name, r->getClubId(), r->getClassId(),
                                cardNo, 0);
              }
            }
            else
              r=oe->addRunner(name, t->getClubId(), t->getClassId(), cardNo, 0, false);

            r->setName(name, true);
            r->setCardNo(cardNo, true);
            r->synchronize();
            t->setRunner(i, r, true);
          }
        }
      }

    }
    if (setDNS)
      t->setTeamNoStart(true);

    if (t->checkValdParSetup()) {
      gdi.alert("Laguppst�llningen hade fel, som har r�ttats");
    }

    if (t->getRunner(0))
      t->getRunner(0)->setStartTimeS(start);

    t->evaluate(true);

    if (globalDep)
      oe->reEvaluateAll(classes, false);

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
  if (type==GUI_BUTTON) {
    ButtonInfo bi=*(ButtonInfo *)data;

    if (bi.id=="Save") {
      return save(gdi, false);
    }
    if (bi.id == "Cancel") {
       loadPage(gdi);
       return 0;
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
    else if (bi.id=="Undo") {
      pTeam t = oe->getTeam(teamId);
      selectTeam(gdi, t);
      return 0;
    }
    else if (bi.id=="Search") {
      ListBoxInfo lbi;
      gdi.getSelectedItem("Teams", lbi);
      oe->fillTeams(gdi, "Teams");
      stdext::hash_set<int> foo;
      pTeam t=oe->findTeam(gdi.getText("SearchText"), lbi.data, foo);

      if (t) {
        selectTeam(gdi, t);
        gdi.selectItemByData("Teams", t->getId());
      }
      else
        gdi.alert("Laget hittades inte");
    }
    else if (bi.id == "ImportTeams") {
      if (teamId>0)
        save(gdi, true);
      showTeamImport(gdi);
    }
    else if (bi.id == "DoImportTeams") {
      doTeamImport(gdi);
    }
    else if (bi.id == "AddTeamMembers") {
       if (teamId>0)
        save(gdi, true);
      showAddTeamMembers(gdi);
    }
    else if (bi.id == "DoAddTeamMembers") {
      doAddTeamMembers(gdi);
    }
    else if (bi.id == "SaveTeams") {
      saveTeamImport(gdi, bi.getExtraInt() != 0);
    }
    else if (bi.id == "ShowAll") {
      fillTeamList(gdi);
    }
    else if (bi.id == "DirectEntry") {
      gdi.restore("DirectEntry", false);
      gdi.fillDown();
      gdi.dropLine();
      gdi.addString("", boldText, "Direktanm�lan");
      gdi.addString("", 0, "Du kan anv�nda en SI-enhet f�r att l�sa in bricknummer.");
      gdi.dropLine(0.2);
   
      int leg = bi.getExtraInt();
      gdi.fillRight();
      gdi.addInput("DirName", "", 16, TeamCB, "Namn:");
      gdi.addInput("DirCard", "", 8, TeamCB, "Bricka:");

      TabSI &tsi = dynamic_cast<TabSI &>(*gdi.getTabs().get(TSITab));
      tsi.setCardNumberField("DirCard");
      gdi.setPostClearCb(TeamCB);
      bool rent = false;
      gdi.dropLine(1.1);
   
      gdi.addCheckbox("DirRent", "Hyrd", 0, rent);
      gdi.dropLine(-0.2);
  
      gdi.addButton("DirOK", "OK", TeamCB).setDefault().setExtra(leg);
      gdi.addButton("Cancel", "Avbryt", TeamCB).setCancel();
     
      gdi.disableInput("DirOK");
      gdi.refreshFast();
    }
    else if (bi.id == "DirOK") {
      pTeam t = oe->getTeam(teamId);
      if (!t || !t->getClassRef())
        return 0;

      int leg = bi.getExtraInt();
      string name = gdi.getText("DirName");
      int storedId = gdi.getBaseInfo("DirName").getExtraInt();
      
      int card = gdi.getTextNo("DirCard");
      
      
      if (card <= 0 || name.empty())
        throw meosException("Internal error"); //Cannot happen
      
      pRunner r = 0;
      if (storedId > 0) {
        r = oe->getRunner(storedId, 0);
        if (r != 0 && (r->getName() != name || r->getCardNo() != card))
          r = 0; // Ignore match
      }
      
      bool rExists = r != 0;
      
      pRunner old = oe->getRunnerByCardNo(card, 0, true, true);
      if (old && r != old) {
        throw meosException("Brickan anv�nds av X.#" + old->getName() );
      }


      pClub clb = 0;
      if (!rExists) {
        pRunner rOrig = oe->getRunnerByCardNo(card, 0, false, false);
        if (rOrig)
          clb = rOrig->getClubRef();
      }

      bool rent = gdi.isChecked("DirRent");

      if (r == 0) {
        r = oe->addRunner(name, clb ? clb->getId() : t->getClubId(), t->getClassId(), card, 0, false);
      }
      if (rent)
        r->getDI().setInt("CardFee", oe->getDI().getInt("CardFee"));

      t->synchronize();
      pRunner oldR = t->getRunner(leg);
      t->setRunner(leg, 0, false);
      t->synchronize(true);
      if (oldR) {
        if (rExists) {
          switchRunners(t, leg, r, oldR);
        }
        else {
          oldR->setClassId(0, true);
          vector<int> mp;
          oldR->evaluateCard(true, mp, 0, true);
          oldR->synchronize(true);
          t->setRunner(leg, r, true);
          t->checkValdParSetup();
        }
      }
      else {
        t->setRunner(leg, r, true);
        t->checkValdParSetup();
      }

      selectTeam(gdi, t);
    }
    else if (bi.id == "Browse") {
      const char *target = (const char *)bi.getExtra();
      vector< pair<string, string> > ext;
      ext.push_back(make_pair("Laguppst�llning", "*.csv;*.txt"));
      string fileName = gdi.browseForOpen(ext, "csv");
      if (!fileName.empty())
        gdi.setText(target, fileName);
    }
    else if (bi.id == "ChangeKey") {
      pTeam t = oe->getTeam(teamId);
      if (!t || !t->getClassRef())
        return 0;

      pClass  pc = t->getClassRef();
      gdi.restore("ChangeKey", false);
      gdi.fillRight();
      gdi.pushX();
      gdi.dropLine();
      gdi.addSelection("ForkKey", 100, 400, 0, "Gafflingsnyckel:");
      int nf = pc->getNumForks();
      vector< pair<string, size_t> > keys;
      for (int f = 0; f < nf; f++) {
        keys.push_back( make_pair(itos(f+1), f));
      }
      int currentKey = max(t->getStartNo()-1, 0) % nf;
      gdi.addItem("ForkKey", keys);
      gdi.selectItemByData("ForkKey", currentKey);

      gdi.dropLine(0.9);
      gdi.addButton("SaveKey", "�ndra", TeamCB);
      gdi.refreshFast();
    }
    else if (bi.id == "SaveKey") {
      pTeam t = oe->getTeam(teamId);
      if (!t || !t->getClassRef())
        return 0;
      
      pClass  pc = t->getClassRef();
      int nf = pc->getNumForks();
      ListBoxInfo lbi;
      gdi.getSelectedItem("ForkKey", lbi);
      for (int k = 0; k < nf; k++) {
        for (int j = 0; j < 2; j++) {
          int newSno = t->getStartNo();
          if (j == 0)
            newSno += k;
          else 
            newSno -=k;

          if (newSno <= 0)
            continue;

          int currentKey = max(newSno-1, 0) % nf;
          if (currentKey == lbi.data) { 
            t->setStartNo(newSno, false);
            t->apply(false, 0, false);
            t->synchronize(true);
            for (int i = 0; i < t->getNumRunners(); i++) {
              pRunner r = t->getRunner(i);
              if (r) {
                vector<int> mp;
                r->evaluateCard(true, mp);
                r->synchronize(true);
              }
            }
            selectTeam(gdi, t);
            return 0;
          }
        }
      }
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
    else if (bi.id.substr(0,2)=="DR") {
      int leg=atoi(bi.id.substr(2, string::npos).c_str());
      pTeam t = oe->getTeam(teamId);
      if (t == 0)
        return 0;
      pClass pc = t->getClassRef();
      if (pc == 0)
        return 0;

      gdi.restore("SelectR", false);
      gdi.setRestorePoint("SelectR");
      gdi.dropLine();
      gdi.fillDown();

      gdi.addString("", fontMediumPlus, "V�lj l�pare f�r str�cka X#" + pc->getLegNumber(leg));
      gdi.setData("Leg", leg);
      
      gdi.setRestorePoint("DirectEntry");
      gdi.addString("", 0, "help:teamwork");
      gdi.dropLine(0.5);

      gdi.addButton("DirectEntry", "Direktanm�lan...", TeamCB).setExtra(leg);
      set<int> presented;
      gdi.pushX();
      gdi.fillRight();
      int w,h;
      gdi.getTargetDimension(w, h);
      w = max(w, gdi.getWidth());
      int limit = max(w - gdi.scaleLength(150), gdi.getCX() + gdi.scaleLength(200));
      set< pair<string, int> > rToList;
      set<int> usedR;
      string anon = lang.tl("N.N.");
      set<int> clubs;
      
      for (int i = 0; i < t->getNumRunners(); i++) {
        if (pc->getLegRunnerIndex(i) == 0) {
          pRunner r = t->getRunner(i);
          if (!r)
            continue;
          if (r->getClubId() > 0)
            clubs.insert(r->getClubId()); // Combination teams
          if (r && r->getName() != anon) {
            rToList.insert( make_pair(r->getName(), r->getId()));
          }
        }
      }
      showRunners(gdi, "Fr�n laget", rToList, limit, usedR);
      vector<pRunner> clsR;
      oe->getRunners(0, 0, clsR, true);
      rToList.clear();
      if (t->getClubId() > 0) 
        clubs.insert(t->getClubId());
      
      if (!clubs.empty()) {
        for (size_t i = 0; i < clsR.size(); i++) {
          if (clsR[i]->getRaceNo() > 0)
            continue;
          if (clubs.count(clsR[i]->getClubId()) == 0)
            continue;
          if (clsR[i]->getClassId() != t->getClassId())
            continue;
          if (clsR[i]->getName() == anon)
            continue;
          rToList.insert( make_pair(clsR[i]->getName(), clsR[i]->getId()));
        }

        showRunners(gdi, "Fr�n klassen", rToList, limit, usedR);
        rToList.clear();
     
        for (size_t i = 0; i < clsR.size(); i++) {
          if (clsR[i]->getRaceNo() > 0)
            continue;
          if (clubs.count(clsR[i]->getClubId()) == 0)
            continue;
          if (clsR[i]->getName() == anon)
            continue;
          rToList.insert( make_pair(clsR[i]->getName(), clsR[i]->getId()));
        }
        showRunners(gdi, "Fr�n klubben", rToList, limit, usedR);
      }
      
     
      vector< pair<string, size_t> > otherR;

      for (size_t i = 0; i < clsR.size(); i++) {
        if (clsR[i]->getRaceNo() > 0)
          continue;
        if (clsR[i]->getName() == anon)
          continue;
        if (usedR.count(clsR[i]->getId()))
          continue;
        const string &club = clsR[i]->getClub();
        string id = clsR[i]->getName() + ", " + clsR[i]->getClass();
        if (!club.empty())
          id += " (" + club + ")";
        
        otherR.push_back(make_pair(id, clsR[i]->getId()));
      }
      gdi.fillDown();
      if (!otherR.empty()) {
        gdi.addString("", 1, "�vriga");
        gdi.fillRight();
        gdi.addSelection("SelectR", 250, 400, TeamCB);
        gdi.addButton("SelectRunner", "OK", TeamCB).setExtra(leg);
        gdi.fillDown();
        gdi.addButton("Cancel", "Avbryt", TeamCB);
        
        gdi.addItem("SelectR", otherR);
      }
      else { 
        gdi.addButton("Cancel", "Avbryt", TeamCB);
      }
      gdi.refresh();
    }
    else if (bi.id=="SelectRunner") {
      ListBoxInfo lbi;
      gdi.getSelectedItem("SelectR", lbi);
      pRunner r = oe->getRunner(lbi.data, 0);
      if (r == 0) {
        throw meosException("Ingen deltagare vald.");
      }
      int leg = (int)gdi.getData("Leg");
      
      pTeam t = oe->getTeam(teamId);
      processChangeRunner(gdi, t, leg, r);
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

      ListBoxInfo lbi;
      gdi.getSelectedItem("RClass", lbi);

      int clsId;
      if (signed(lbi.data)>0)
        clsId = lbi.data;
      else
        clsId = oe->getFirstClassId(true);

      pClass pc = oe->getClass(clsId);
      if (pc) {
        pair<int, string> snoBib = pc->getNextBib();
        if (snoBib.first > 0) {
          t->setBib(snoBib.second, snoBib.first, true, false);
        }
      }

      t->setClassId(clsId, true);

      fillTeamList(gdi);
      //oe->fillTeams(gdi, "Teams");
      selectTeam(gdi, t);

      //selectTeam(gdi, 0);
      //gdi.selectItemByData("Teams", -1);
      gdi.setInputFocus("Name", true);
    }
    else if (bi.id=="Remove") {
      DWORD tid=teamId;

      if (tid==0)
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
            r->setClassId(0, true);
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
  else if (type == GUI_LINK) {
    TextInfo ti = dynamic_cast<TextInfo &>(*(BaseInfo *)data);
    if (ti.id == "SelectR") {
      int leg = (int)gdi.getData("Leg");
      pTeam t = oe->getTeam(teamId);
      int rid = ti.getExtraInt();
      pRunner r = oe->getRunner(rid, 0);
      processChangeRunner(gdi, t, leg, r);
    }
  }
  else if (type==GUI_LISTBOX) {
    ListBoxInfo bi=*(ListBoxInfo *)data;

    if (bi.id=="Teams") {
      if (gdi.isInputChanged("")) {
        pTeam t = oe->getTeam(teamId);
        bool newName = t && t->getName() != gdi.getText("Name");
        bool newBib = gdi.hasField("StartNo") && t && t->getBib() != gdi.getText("StartNo");
        save(gdi, true);

        if (newName || newBib) {
          fillTeamList(gdi);
        }
      }
      pTeam t=oe->getTeam(bi.data);
      selectTeam(gdi, t);
    }
    else if (bi.id=="RClass") { //New class selected.
      DWORD tid=teamId;
      //gdi.getData("TeamID", tid);

      if (tid){
        pTeam t=oe->getTeam(tid);
        pClass pc=oe->getClass(bi.data);

        if (pc && pc->getNumDistinctRunners() == shownDistinctRunners &&
          pc->getNumStages() == shownRunners) {
            // Keep team setup, i.e. do nothing
        }
        else if (t && pc && (t->getClassId()==bi.data
                || t->getNumRunners()==pc->getNumStages()) )
          loadTeamMembers(gdi, 0,0,t);
        else
          loadTeamMembers(gdi, bi.data, 0, t);
      }
      else loadTeamMembers(gdi, bi.data, 0, 0);
    }
    else {

      ListBoxInfo lbi;
      gdi.getSelectedItem("RClass", lbi);

      if (signed(lbi.data)>0){
        pClass pc=oe->getClass(lbi.data);

        if (pc) {
          vector<pRunner> rCache;
          for(unsigned i=0;i<pc->getNumStages();i++){
            char bf[16];
            sprintf_s(bf, "R%d", i);
            if (bi.id==bf){
              pRunner r=oe->getRunner(bi.data, 0);
              if (r) {
                sprintf_s(bf, "SI%d", i);
                int cno = r->getCardNo();
                gdi.setText(bf, cno > 0 ? itos(cno) : "");
                warnDuplicateCard(gdi, bf, cno, r, rCache);
              }
            }
          }
        }
      }
    }
  }
  else if (type==GUI_INPUTCHANGE) {
    InputInfo &ii=*(InputInfo *)data;
    pClass pc=oe->getClass(classId);
    if (pc){
      for(unsigned i=0;i<pc->getNumStages();i++){
        char bf[16];
        sprintf_s(bf, "R%d", i);
        if (ii.id == bf) {
          for (unsigned k=i+1; k<pc->getNumStages(); k++) {
            if (pc->getLegRunner(k)==i) {
              sprintf_s(bf, "R%d", k);
              gdi.setText(bf, ii.text);
            }
          }
          break;
        }
      }
    }

    if (ii.id == "DirName" || ii.id == "DirCard") {
      gdi.setInputStatus("DirOK", !gdi.getText("DirName").empty() &&
                                  gdi.getTextNo("DirCard") > 0);

    }
  }
  else if (type == GUI_INPUT) {
    InputInfo &ii=*(InputInfo *)data;
    if (ii.id == "DirName" || ii.id == "DirCard") {
      gdi.setInputStatus("DirOK", !gdi.getText("DirName").empty() &&
                                  gdi.getTextNo("DirCard") > 0);

    }
    if (ii.id == "DirCard") {
      int cno = gdi.getTextNo("DirCard");
      if (cno > 0 && gdi.getText("DirName").empty()) {
        bool matched = false;
        pRunner r = oe->getRunnerByCardNo(cno, 0, true, false);
        if (r && (r->getStatus() == StatusUnknown || r->getStatus() == StatusDNS) ) {
          // Switch to exactly this runner. Has not run before
          gdi.setText("DirName", r->getName())->setExtra(r->getId());
          matched = true;
        }
        else {
          r = oe->getRunnerByCardNo(cno, 0, false, false);
          if (r) {
            // Copy only the name.
            gdi.setText("DirName", r->getName())->setExtra(0);
            matched = true;
          }
          else if (oe->getMeOSFeatures().hasFeature(MeOSFeatures::RunnerDb)) {
            const RunnerDBEntry *rdb = oe->getRunnerDatabase().getRunnerByCard(cno);
            if (rdb) {
              string name;
              rdb->getName(name);
              gdi.setText("DirName", name)->setExtra(0);
              matched = true;
            }
          }
        }
        if (r)
          gdi.check("DirRent", r->getDCI().getInt("CardFee") != 0);
        if (matched)
          gdi.setInputStatus("DirOK", true);
      }
    }
    pClass pc=oe->getClass(classId);
    if (pc) {
      for(unsigned i=0;i<pc->getNumStages();i++){
        if (ii.id == "SI" + itos(i)) {
          int cardNo = atoi(ii.text.c_str());
          pTeam t = oe->getTeam(teamId);
          if (t) {
            vector<pRunner> rc;
            warnDuplicateCard(gdi, ii.id, cardNo, t->getRunner(i), rc);
          }
          break;
        }
      }
    }
  
  }
  else if (type==GUI_CLEAR) {
    if (teamId>0)
      save(gdi, true);

    return true;
  }
  else if (type==GUI_POSTCLEAR) {
    // Clear out SI-link
    TabSI &tsi = dynamic_cast<TabSI &>(*gdi.getTabs().get(TSITab));
    tsi.setCardNumberField("");
    return true;
  }
  return 0;
}


void TabTeam::loadTeamMembers(gdioutput &gdi, int ClassId, int ClubId, pTeam t)
{
  if (ClassId==0)
    if (t) ClassId=t->getClassId();

  classId=ClassId;
  gdi.restore("",false);

  pClass pc=oe->getClass(ClassId);
  if (!pc) return;

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
  int dx[6] = {0, 184, 220, 290, 316, 364};
  for (int i = 0; i<6; i++)
    dx[i] = gdi.scaleLength(dx[i]);

  gdi.addString("", yp, xp + dx[0], 0, "Namn:");
  gdi.addString("", yp, xp + dx[2], 0, "Bricka:");
  gdi.addString("", yp, xp + dx[3], 0, "Hyrd:");
  gdi.addString("", yp, xp + dx[5], 0, "Status:");
  gdi.dropLine(0.5);
  vector<pRunner> rCache;

  for (unsigned i=0;i<pc->getNumStages();i++) {
    yp = gdi.getCY();

    sprintf_s(bf, "R%d", i);
    gdi.pushX();
    bool hasSI = false;
    gdi.addStringUT(yp, numberPos, 0, pc->getLegNumber(i)+".");
    if (pc->getLegRunner(i)==i) {

      gdi.addInput(xp + dx[0], yp, bf, "", 18, TeamCB);//Name
      gdi.addButton(xp + dx[1], yp-2, gdi.scaleLength(28), "DR" + itos(i), "<>", TeamCB, "Knyt l�pare till str�ckan.", false, false); // Change
      sprintf_s(bf_si, "SI%d", i);
      hasSI = true;
      gdi.addInput(xp + dx[2], yp, bf_si, "", 5, TeamCB).setExtra(i); //Si

      gdi.addCheckbox(xp + dx[3], yp + gdi.scaleLength(10), "RENT"+itos(i), "", 0, false); //Rentcard
    }
    else {
      //gdi.addInput(bf, "", 24);
      gdi.addInput(xp + dx[0], yp, bf, "", 18, 0);//Name
      gdi.disableInput(bf);
    }
    gdi.addButton(xp + dx[4], yp-2,  gdi.scaleLength(38), "MR" + itos(i), "...", TeamCB, "Redigera deltagaren.", false, false); // Change

    gdi.addString(("STATUS"+itos(i)).c_str(), yp+gdi.scaleLength(5), xp + dx[5], 0, "#MMMMMMMMMMMMMMMM");
    gdi.setText("STATUS"+itos(i), "", false);
    gdi.dropLine(0.5);
    gdi.popX();


    if (t) {
      pRunner r=t->getRunner(i);
      if (r) {
        gdi.setText(bf, r->getNameRaw())->setExtra(r->getId());

        if (hasSI) {
          int cno = r->getCardNo();
          gdi.setText(bf_si, cno > 0 ? itos(cno) : "");
          warnDuplicateCard(gdi, bf_si, cno, r, rCache);
          gdi.check("RENT" + itos(i), r->getDCI().getInt("CardFee") != 0);
        }
        string sid = "STATUS"+itos(i);
        if (r->statusOK()) {
          TextInfo * ti = (TextInfo *)gdi.setText(sid, "OK, " + r->getRunningTimeS(), false);
          if (ti)
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

  gdi.setRestorePoint("SelectR");
  gdi.addString("", 1, "help:7618");
  gdi.dropLine();
  int numF = pc->getNumForks();
  
  if (numF>1 && t) {
    gdi.addString ("", 1, "Gafflingsnyckel X#" + itos(1+(max(t->getStartNo()-1, 0) % numF))).setColor(colorGreen);
    string crsList;
    bool hasCrs = false;
    for (size_t k = 0; k < pc->getNumStages(); k++) {
      pCourse crs = pc->getCourse(k, t->getStartNo());
      string cS; 
      if (crs != 0) {
        cS = crs->getName();
        hasCrs = true;
      }
      else
        cS = MakeDash("-");
    
      if (!crsList.empty())
        crsList += ", ";
      crsList += cS;

      if (hasCrs && crsList.length() > 50) {
        gdi.addStringUT(0, crsList);
        crsList.clear();
      }
    }
    if (hasCrs && !crsList.empty()) {
      gdi.addStringUT(0, crsList);
    }

    if (hasCrs) {
      gdi.dropLine(0.5);
      gdi.setRestorePoint("ChangeKey");
      gdi.addButton("ChangeKey", "�ndra lagets gaffling", TeamCB);
    }
  }
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

  if (currentMode == 1) {
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
  gdi.addButton("ShowAll", "Visa alla", TeamCB).isEdit(false);

  gdi.dropLine(2);
  gdi.popX();
  gdi.fillDown();
  gdi.addListBox("Teams", 250, 440, TeamCB, "", "").isEdit(false).ignore(true);
  gdi.setInputFocus("Teams");
  fillTeamList(gdi);

  int posXForButtons = gdi.getCX();
  int posYForButtons = gdi.getCY();
  
  gdi.newColumn();
  gdi.fillDown();
  gdi.pushX();
  gdi.addInput("Name", "", 24, 0, "Lagnamn:");

  gdi.fillRight();
  bool drop = false;
  if (oe->getMeOSFeatures().hasFeature(MeOSFeatures::Bib)) {
    gdi.addInput("StartNo", "", 4, 0, "Nr:", "Nummerlapp");
    drop = oe->getMeOSFeatures().hasFeature(MeOSFeatures::Economy);
  }

  if (oe->getMeOSFeatures().hasFeature(MeOSFeatures::Clubs)) {
    gdi.addCombo("Club", 180, 300, 0, "Klubb:");
    oe->fillClubs(gdi, "Club");
    drop = true;
  }

  if (drop) {
    gdi.dropLine(3);
    gdi.popX();
  }

  gdi.addSelection("RClass", 170, 300, TeamCB, "Klass:");
  oe->fillClasses(gdi, "RClass", oEvent::extraNone, oEvent::filterNone);
  gdi.addItem("RClass", lang.tl("Ny klass"), 0);

  if (oe->getMeOSFeatures().hasFeature(MeOSFeatures::Economy))
    gdi.addInput("Fee", "", 5, 0, "Avgift:");

  gdi.popX();
  gdi.fillDown();
  gdi.dropLine(3);

  gdi.pushX();
  gdi.fillRight();

  gdi.addInput("Start", "", 8, 0, "Starttid:");
  gdi.addInput("Finish", "", 8, 0, "M�ltid:");

  const bool timeAdjust = oe->getMeOSFeatures().hasFeature(MeOSFeatures::TimeAdjust);
  const bool pointAdjust = oe->getMeOSFeatures().hasFeature(MeOSFeatures::PointAdjust);

  if (timeAdjust || pointAdjust) {
    gdi.dropLine(3);
    gdi.popX();
    if (timeAdjust) {
      gdi.addInput("TimeAdjust", "", 8, 0, "Tidstill�gg:");
    }
    if (pointAdjust) {
      gdi.addInput("PointAdjust", "", 8, 0, "Po�ngavdrag:");
    }
  }

  /*if (oe->getMeOSFeatures().hasFeature(MeOSFeatures::TimeAdjust)) {
    gdi.addInput("TimeAdjust", "", 5, 0, "Tidstill�gg:");
  }
  if (oe->getMeOSFeatures().hasFeature(MeOSFeatures::PointAdjust)) {
    gdi.addInput("PointAdjust", "", 5, 0, "Po�ngavdrag:");
  }*/

  gdi.fillDown();
  gdi.dropLine(3);
  gdi.popX();

  gdi.pushX();
  gdi.fillRight();

  gdi.addInput("Time", "", 6, 0, "Tid:").isEdit(false).ignore(true);
  gdi.disableInput("Time");

  gdi.fillDown();
  gdi.addSelection("Status", 100, 160, 0, "Status:", "tooltip_explain_status");
  oe->fillStatus(gdi, "Status");

  gdi.popX();
  gdi.selectItemByData("Status", 0);

  gdi.dropLine(1.5);
  
  const bool multiDay = oe->hasPrevStage();

  if (multiDay) {
    int xx = gdi.getCX();
    int yy = gdi.getCY();
    gdi.dropLine(0.5);
    gdi.fillDown();
    int dx = int(gdi.getLineHeight()*0.7);
    int ccx = xx + dx;
    gdi.setCX(ccx);
    gdi.addString("", 1, "Resultat fr�n tidigare etapper");
    gdi.dropLine(0.3);
    gdi.fillRight();
 
    gdi.addSelection("StatusIn", 100, 160, 0, "Status:", "tooltip_explain_status");
    oe->fillStatus(gdi, "StatusIn");
    gdi.selectItemByData("Status", 0);
    gdi.addInput("PlaceIn", "", 5, 0, "Placering:");
    int xmax = gdi.getCX() + dx;
    gdi.setCX(ccx);
    gdi.dropLine(3);
    gdi.addInput("TimeIn", "", 5, 0, "Tid:");
    if (oe->hasRogaining()) {
      gdi.addInput("PointIn", "", 5, 0, "Po�ng:");
    }
    gdi.dropLine(3);
    RECT rc;
    rc.right = xx;
    rc.top = yy;
    rc.left = max(xmax, gdi.getWidth()-dx);
    rc.bottom = gdi.getCY();

    gdi.addRectangle(rc, colorLightGreen, true, false);
    gdi.dropLine(1.5);
    gdi.popX();
  }
  
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
  
  RECT rc;
  rc.left = posXForButtons;
  rc.top = posYForButtons;

  gdi.setCY(posYForButtons + gdi.scaleLength(4));
  gdi.setCX(posXForButtons + gdi.getLineHeight());
  gdi.fillDown();
  gdi.addString("", 1, "Verktyg");
  gdi.dropLine(0.3);
  gdi.fillRight();
  gdi.addButton("ImportTeams", "Importera laguppst�llningar", TeamCB);
  gdi.addButton("AddTeamMembers", "Skapa anonyma lagmedlemmar", TeamCB, "Fyll obesatta str�ckor i alla lag med anonyma tillf�lliga lagmedlemmar (N.N.)");
  rc.right = gdi.getCX() + gdi.getLineHeight();
  gdi.dropLine(1.5);
  rc.bottom = gdi.getHeight();
  gdi.addRectangle(rc, colorLightCyan);

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

void TabTeam::showTeamImport(gdioutput &gdi) {
  gdi.clearPage(false);
  gdi.addString("", boldLarge, "Importera laguppst�llningar");

  gdi.addString("", 10, "help:teamlineup");
  gdi.dropLine();
  gdi.setRestorePoint("TeamLineup");
  gdi.pushX();

  gdi.fillRight();
  gdi.addInput("FileName", "", 40, 0, "Filnamn:");
  gdi.dropLine(0.9);
  gdi.addButton("Browse", "Bl�ddra", TeamCB).setExtra("FileName");
  gdi.dropLine(3);
  gdi.popX();
  gdi.fillDown();
  gdi.addCheckbox("OnlyExisting", "Anv�nd befintliga deltagare", 0, false,
    "Knyt redan anm�lda deltagare till laget (identifiera genom namn och/eller bricka)");
  gdi.fillRight();
  gdi.addButton("DoImportTeams", "Importera", TeamCB).setDefault();
  gdi.addButton("Cancel", "Avbryt", TeamCB).setCancel();

  gdi.refresh();
}

void TabTeam::doTeamImport(gdioutput &gdi) {
  string file = gdi.getText("FileName");
  bool useExisting = gdi.isChecked("OnlyExisting");


  csvparser csv;
  map<string, int> classNameToNumber;
  vector<pClass> cls;
  oe->getClasses(cls, true);
  for (size_t k = 0; k < cls.size();k++) {
    classNameToNumber[cls[k]->getName()] = cls[k]->getNumStages();
  }
  gdi.fillDown();
  csv.importTeamLineup(file, classNameToNumber, teamLineup);

  gdi.restore("TeamLineup", false);

  gdi.dropLine();
  for (size_t k = 0; k < teamLineup.size(); k++) {
    string tdesc = teamLineup[k].teamClass +", " + teamLineup[k].teamName;
    if (!teamLineup[k].teamClub.empty())
      tdesc += ", " + teamLineup[k].teamClub;
    gdi.addStringUT(1, tdesc);
    for (size_t j = 0; j < teamLineup[k].members.size(); j++) {
      TeamLineup::TeamMember &member = teamLineup[k].members[j];
      if (member.name.empty())
        continue;

      string mdesc = " " + itos(j+1) + ". ";
      bool warn = false;
      
      if (useExisting) {
        pRunner r = findRunner(member.name, member.cardNo);
        if (r != 0)
          mdesc += r->getCompleteIdentification();
        else {
          mdesc += member.name + lang.tl(" (ej funnen)");
          warn = true;
        }
      }
      else {
        mdesc += member.name + " (" + itos(member.cardNo) + ") " + member.club;
      }

      if (!member.course.empty()) {
        if (oe->getCourse(member.course))
          mdesc += " : " + member.course;
        else {
          mdesc += " : " + lang.tl("Banan saknas");
          warn = true;
        }
      }

      if (!member.cls.empty()) {
        if (oe->getClass(member.cls))
          mdesc += " [" + member.cls + "]";
        else {
          mdesc += " " + lang.tl("Klassen saknas");
          warn = true;
        }
      }

      TextInfo &ti = gdi.addStringUT(0, mdesc);
      if (warn)
        ti.setColor(colorRed);
    }
    gdi.dropLine();
  }
  gdi.fillRight();
  gdi.addButton("ImportTeams", "<< Bak�t", TeamCB);
  gdi.addButton("SaveTeams", "Spara laguppst�llningar", TeamCB).setDefault().setExtra(useExisting);
  gdi.addButton("Cancel", "Avbryt", TeamCB).setCancel();
  gdi.refresh();
}

void TabTeam::saveTeamImport(gdioutput &gdi, bool useExisting) {
  for (size_t k = 0; k < teamLineup.size(); k++) {
    pClub club = !teamLineup[k].teamClub.empty() ? oe->getClubCreate(0, teamLineup[k].teamClub) : 0;
    pTeam t = oe->addTeam(teamLineup[k].teamName, club ? club->getId() : 0, oe->getClass(teamLineup[k].teamClass)->getId());

    for (size_t j = 0; j < teamLineup[k].members.size(); j++) {
      TeamLineup::TeamMember &member = teamLineup[k].members[j];
      if (member.name.empty())
        continue;

      pRunner r = 0;
      if (useExisting) {
        r = findRunner(member.name, member.cardNo);
        if (r && !member.course.empty()) {
          pCourse pc = oe->getCourse(member.course);
          r->setCourseId(pc ? pc->getId() : 0);
        }
        if (r && !member.cls.empty()) {
          pClass rcls = oe->getClass(member.cls);
          r->setClassId(rcls ? rcls->getId() : 0, true);
        }
      }
      else {
        r = oe->addRunner(member.name, member.club, 0, member.cardNo, 0, false);

        if (r && !member.course.empty()) {
          pCourse pc = oe->getCourse(member.course);
          r->setCourseId(pc ? pc->getId() : 0);
        }

        if (r && !member.cls.empty()) {
          pClass rcls = oe->getClass(member.cls);
          r->setClassId(rcls ? rcls->getId() : 0, true);
        }
      }

      t->setRunner(j, r, false);
      if (r)
        r->synchronize(true);
    }

    t->synchronize();
    gdi.dropLine();
  }
  loadPage(gdi);
}

pRunner TabTeam::findRunner(const string &name, int cardNo) const {
  string n = canonizeName(name.c_str());

  if (cardNo != 0) {
    vector<pRunner> pr;
    oe->getRunnersByCard(cardNo, pr);
    for (size_t k = 0; k < pr.size(); k++) {
      string a = canonizeName(pr[k]->getName().c_str());
      if (a == n)
        return pr[k];
    }
  }
  else {
    vector<pRunner> pr;
    oe->getRunners(0, 0, pr, false);
    for (size_t k = 0; k < pr.size(); k++) {
      string a = canonizeName(pr[k]->getName().c_str());
      if (a == n)
        return pr[k];
    }
  }
  return 0;
}

void TabTeam::showAddTeamMembers(gdioutput &gdi) {
  gdi.clearPage(false);
  gdi.addString("", boldLarge, "Tills�tt tillf�lliga anonyma lagmedlemmar");

  gdi.addString("", 10, "help:anonymous_team");
  gdi.dropLine();
  gdi.pushX();

  gdi.fillDown();
  gdi.addInput("Name", lang.tl("N.N."), 24, 0, "Anonymt namn:");
  gdi.fillDown();
  gdi.addCheckbox("OnlyRequired", "Endast p� obligatoriska str�ckor", 0, true);
  gdi.addCheckbox("WithFee", "Med anm�lningsavgift (lagets klubb)", 0, true);
  
  gdi.fillRight();
  gdi.addButton("DoAddTeamMembers", "Tills�tt", TeamCB).setDefault();
  gdi.addButton("Cancel", "Avbryt", TeamCB).setCancel();

  gdi.refresh();
}

void TabTeam::doAddTeamMembers(gdioutput &gdi) {
  vector<pTeam> t;
  oe->getTeams(0, t, true);
  bool onlyReq = gdi.isChecked("OnlyRequired");
  bool withFee = gdi.isChecked("WithFee");
  string nn = gdi.getText("Name");

  for (size_t k = 0; k < t.size(); k++) {
    pTeam mt = t[k];
    pClass cls = mt->getClassRef(); 
    if (cls == 0)
      continue;
    bool ch = false;
    for (int j = 0; j < mt->getNumRunners(); j++) {
      if (mt->getRunner(j) == 0) {
        LegTypes lt = cls->getLegType(j);
        if (onlyReq && lt == LTExtra || lt == LTIgnore || lt == LTParallelOptional)
           continue;
        pRunner r = 0;
        if (withFee) {
          r = oe->addRunner(nn, mt->getClubId(), 0, 0, 0, false);  
          r->synchronize();
          mt->setRunner(j, r, false);
          r->addClassDefaultFee(true);
        }
        else {
          r = oe->addRunnerVacant(0);
          r->setName(nn, false);
            //oe->addRunner(nn, oe->getVacantClub(), 0, 0, 0, false);
          r->synchronize();
          mt->setRunner(j, r, false);
        }
        ch = true;
      }
    }
    if (ch) {
      mt->apply(false, 0, false);
      mt->synchronize();
      for (int j = 0; j < mt->getNumRunners(); j++) {
        if (mt->getRunner(j) != 0) {
          mt->getRunner(j)->synchronize();
        }
      }
    }
  }

  loadPage(gdi);
}

void TabTeam::showRunners(gdioutput &gdi, const char *title, 
                          const set< pair<string, int> > &rToList, 
                          int limitX, set<int> &usedR) {

  if (rToList.empty())
    return;

  bool any = false;
  for(set< pair<string, int> >::const_iterator it = rToList.begin(); it != rToList.end(); ++it) {
    if (usedR.count(it->second))
      continue;
    usedR.insert(it->second);
    
    if (!any) {
      gdi.addString("", boldText, title);
      gdi.dropLine(1.2);
      gdi.popX();
      any = true;
    }

    if (gdi.getCX() > limitX) {
      gdi.dropLine(1.5);
      gdi.popX();
    }

    gdi.addString("SelectR", 0, "#" + it->first, TeamCB).setExtra(it->second);
  }

  if (any) {
    gdi.dropLine(2);
    gdi.popX();
  }
}

void TabTeam::processChangeRunner(gdioutput &gdi, pTeam t, int leg, pRunner r) {
  if (r && t && leg < t->getNumRunners()) {
    pRunner oldR = t->getRunner(leg);
    gdioutput::AskAnswer ans = gdioutput::AnswerNo;
    if (r == oldR) {
      gdi.restore("SelectR");
      return;
    }
    else if (oldR) {
      if (r->getTeam()) {
        ans = gdi.askCancel("Vill du att X och Y byter str�cka?#" +
                              r->getName() + "#" + oldR->getName());
      }
      else {
        ans = gdi.askCancel("Vill du att X tar str�ckan ist�llet f�r Y?#" +
                              r->getName() + "#" + oldR->getName());
      }
    }
    else {
      ans = gdi.askCancel("Vill du att X g�r in i laget?#" + r->getName());
    }

    if (ans == gdioutput::AnswerNo)
      return;
    else if (ans == gdioutput::AnswerCancel) {
      gdi.restore("SelectR");
      return;
    }

    save(gdi, true);
    vector<int> mp;
    switchRunners(t, leg, r, oldR);
    /*if (r->getTeam()) {
      pTeam otherTeam = r->getTeam();
      int otherLeg = r->getLegNumber();
      otherTeam->setRunner(otherLeg, oldR, true);
      if (oldR)
        oldR->evaluateCard(true, mp, 0, true);
      otherTeam->checkValdParSetup();
      otherTeam->apply(true, 0, false);
      otherTeam->synchronize(true);
    }
    else if (oldR) {
      t->setRunner(leg, 0, false);
      t->synchronize(true);
      oldR->setClassId(r->getClassId(), true);
      oldR->evaluateCard(true, mp, 0, true);
      oldR->synchronize(true);
    }

    t->setRunner(leg, r, true);
    r->evaluateCard(true, mp, 0, true);
    t->checkValdParSetup();
    t->apply(true, 0, false);
    t->synchronize(true);*/
    loadPage(gdi);
  }
}

void TabTeam::switchRunners(pTeam t, int leg, pRunner r, pRunner oldR) {
  vector<int> mp;
    
  if (r->getTeam()) {
    pTeam otherTeam = r->getTeam();
    int otherLeg = r->getLegNumber();
    otherTeam->setRunner(otherLeg, oldR, true);
    if (oldR)
      oldR->evaluateCard(true, mp, 0, true);
    otherTeam->checkValdParSetup();
    otherTeam->apply(true, 0, false);
    otherTeam->synchronize(true);
  }
  else if (oldR) {
    t->setRunner(leg, 0, false);
    t->synchronize(true);
    oldR->setClassId(r->getClassId(), true);
    oldR->evaluateCard(true, mp, 0, true);
    oldR->synchronize(true);
  }

  t->setRunner(leg, r, true);
  r->evaluateCard(true, mp, 0, true);
  t->checkValdParSetup();
  t->apply(true, 0, false);
  t->synchronize(true);
}

void TabTeam::clearCompetitionData() {
  shownRunners = 0;
  shownDistinctRunners = 0;
  teamId = 0;
  inputId = 0;
  timeToFill = 0;
  currentMode = 0;
}

bool TabTeam::warnDuplicateCard(gdioutput &gdi, string id, int cno, pRunner r, vector<pRunner> &allRCache) {
  pRunner warnCardDupl = 0;

  if (r && !r->getCard()) {
    if (allRCache.empty()) // Fill cache if not initialized
      oe->getRunners(0, 0, allRCache, false);

    for (size_t k = 0; k < allRCache.size(); k++) {
      if (!r->canShareCard(allRCache[k], cno)) {
        warnCardDupl = allRCache[k];
        break;
      }
    }
  }

  InputInfo &cardNo = dynamic_cast<InputInfo &>(gdi.getBaseInfo(id.c_str()));
  if (warnCardDupl) {
    cardNo.setBgColor(colorLightRed);
    gdi.updateToolTip(id, "Brickan anv�nds av X.#" + warnCardDupl->getCompleteIdentification());
    cardNo.refresh();
    return warnCardDupl->getTeam() == r->getTeam();
  }
  else {
    if (cardNo.getBgColor() != colorDefault) {
      cardNo.setBgColor(colorDefault);
      gdi.updateToolTip(id, "");
      cardNo.refresh();
    }
    return false;
  }
}
