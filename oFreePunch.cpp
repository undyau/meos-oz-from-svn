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
    Stigbergsvägen 7, SE-75242 UPPSALA, Sweden
    
************************************************************************/

#include "stdafx.h"
#include <algorithm>
#include <cassert>

#include "oFreePunch.h"
#include "oEvent.h"
#include "Table.h"
#include "meos_util.h"
#include "Localizer.h"
#include "intkeymapimpl.hpp"
#include "socket.h"
#include "meosdb/sqltypes.h"
#include "gdioutput.h"

bool oFreePunch::disableHashing = false;

oFreePunch::oFreePunch(oEvent *poe, int card, int time, int type): oPunch(poe)
{
  Id=oe->getFreePunchId();
  CardNo = card;
	Time = time;
	Type = type;
  itype = 0;
  tRunnerId = 0;
}

oFreePunch::oFreePunch(oEvent *poe, int id): oPunch(poe)
{
  Id=id;
  oe->qFreePunchId = max(id, oe->qFreePunchId);
  itype = 0;
  tRunnerId = 0;
}

oFreePunch::~oFreePunch(void)
{
}

bool oFreePunch::Write(xmlparser &xml)
{
	if(Removed) return true;

	xml.startTag("Punch");
	xml.write("CardNo", CardNo);
	xml.write("Time", Time);
	xml.write("Type", Type);
	xml.write("Id", Id);
	xml.write("Updated", Modified.getStamp());
	xml.endTag();

	return true;
}

void oFreePunch::Set(const xmlobject *xo)
{
	xmlList xl;
  xo->getObjects(xl);

	xmlList::const_iterator it;

	for(it=xl.begin(); it != xl.end(); ++it){
		if(it->is("CardNo")){
			CardNo=it->getInt();			
		}
		if(it->is("Type")){
			Type=it->getInt();			
		}			
		if(it->is("Time")){
			Time=it->getInt();			
		}			
		else if(it->is("Id")){
			Id=it->getInt();			
		}
		else if(it->is("Updated")){
			Modified.setStamp(it->get());
		}
	}
}

bool oFreePunch::setCardNo(int cno, bool databaseUpdate) {
  if (cno != CardNo) {
    pRunner r1 = oe->getRunner(tRunnerId, 0);
    int oldControlId = tMatchControlId;
    oe->punchIndex[itype].remove(CardNo); // Remove from index
    oe->removeFromPunchHash(CardNo, Type, Time);
    rehashPunches(*oe, CardNo, 0);

    CardNo = cno;
    oe->insertIntoPunchHash(CardNo, Type, Time);

    rehashPunches(*oe, CardNo, this);
    pRunner r2 = oe->getRunner(tRunnerId, 0);
   
    if (r1 && oldControlId > 0)
      r1->markClassChanged(oldControlId);
    if (r2 && itype > 0)
      r2->markClassChanged(tMatchControlId);

    if (!databaseUpdate)
      updateChanged();

    return true;
  }
  return false;
}

void oFreePunch::remove() 
{
  if (oe)
    oe->removeFreePunch(Id);
}

bool oFreePunch::canRemove() const 
{
  return true;
}

Table *oEvent::getPunchesTB()//Table mode
{
  if (tables.count("punch") == 0) {
	  Table *table=new Table(this, 20, "Stämplingar", "punches");
	  table->addColumn("Id", 70, true, true);
    table->addColumn("Ändrad", 150, false);
    table->addColumn("Bricka", 70, true);
    table->addColumn("Kontroll", 70, true);
    table->addColumn("Tid", 70, false);
    table->addColumn("Löpare", 170, false);
    table->addColumn("Lag", 170, false);
    tables["punch"] = table;
    table->addOwnership();
  }

  tables["punch"]->update();  
  return tables["punch"];
}

void oEvent::generatePunchTableData(Table &table, oFreePunch *addPunch)
{ 
  if (addPunch) {
    addPunch->addTableRow(table);
    return;
  }

  synchronizeList(oLPunchId);
  oFreePunchList::iterator it;	
  oe->setupCardHash(false);
  table.reserve(punches.size());
  for (it = punches.begin(); it != punches.end(); ++it){		
    if(!it->isRemoved()){
      it->addTableRow(table);
 		}
	}
  oe->setupCardHash(true);
}

void oFreePunch::addTableRow(Table &table) const {
  oFreePunch &it = *pFreePunch(this);
  table.addRow(getId(), &it);
  int row = 0;
  table.set(row++, it, TID_ID, itos(getId()), false, cellEdit);
  table.set(row++, it, TID_MODIFIED, getTimeStamp(), false, cellEdit);
  table.set(row++, it, TID_CARD, itos(getCardNo()), true, cellEdit);
  table.set(row++, it, TID_CONTROL, getType(), true, cellEdit);
  table.set(row++, it, TID_TIME, getTime(), true, cellEdit);
  pRunner r = 0;
  if (CardNo > 0)
    r = oe->getRunnerByCard(CardNo, false);

  table.set(row++, it, TID_RUNNER, r ? r->getName() : "?", false, cellEdit);

  if (r && r->getTeam())
    table.set(row++, it, TID_TEAM, r->getTeam()->getName(), false, cellEdit);
  else
    table.set(row++, it, TID_TEAM, "", false, cellEdit);
}

bool oFreePunch::inputData(int id, const string &input, 
                           int inputId, string &output, bool noUpdate)
{
  synchronize(false);    
  switch(id) {
    case TID_CARD:
      setCardNo(atoi(input.c_str()));
      synchronize(true);
      output = itos(CardNo);
      break;

    case TID_TIME:
      setTime(input);
      synchronize(true);
      output = getTime();
      break;

    case TID_CONTROL:
      setType(input);
      synchronize(true);
      output = getType();
  }
  return false;
}

void oFreePunch::fillInput(int id, vector< pair<string, size_t> > &out, size_t &selected)
{ 
}

void oFreePunch::setTimeInt(int t, bool databaseUpdate) {
  if (t != Time) {
    oe->removeFromPunchHash(CardNo, Type, Time);
    Time = t;
    oe->insertIntoPunchHash(CardNo, Type, Time);
    rehashPunches(*oe, CardNo, 0);
    if (!databaseUpdate)
      updateChanged();
  }
}

bool oFreePunch::setType(const string &t, bool databaseUpdate) {
  int type = atoi(t.c_str());
  int ttype = 0;
  if (type>0 && type<10000)
    ttype = type;
  else {
    if (t == lang.tl("Check"))
      ttype = oPunch::PunchCheck;
    else if (t == lang.tl("Mål"))
      ttype = oPunch::PunchFinish;
    if (t == lang.tl("Start"))
      ttype = oPunch::PunchStart;
  }
  if (ttype > 0 && ttype != Type) {
    oe->removeFromPunchHash(CardNo, Type, Time);
    Type = ttype;
    oe->insertIntoPunchHash(CardNo, Type, Time);
    int oldControlId = tMatchControlId;    
    rehashPunches(*oe, CardNo, 0);

    pRunner r = oe->getRunner(tRunnerId, 0);

    if (r) {
      r->markClassChanged(tMatchControlId);
      if (oldControlId > 0)
        r->markClassChanged(oldControlId);
    }

    if (!databaseUpdate)
      updateChanged();

    return true;
  }
  return false;
}

void oFreePunch::rehashPunches(oEvent &oe, int cardNo, pFreePunch newPunch) {
  if (disableHashing || (cardNo == 0 && !oe.punchIndex.empty()) || oe.punches.empty())
    return;
  vector<pFreePunch> fp;

  if (oe.punchIndex.empty()) {
    // Rehash all punches. Ignore cardNo and newPunch (will be included automatically)
    fp.reserve(oe.punches.size());
    for (oFreePunchList::iterator pit = oe.punches.begin(); pit != oe.punches.end(); ++pit) 
      fp.push_back(&(*pit));

    sort(fp.begin(), fp.end(), FreePunchComp());
    disableHashing = true;
    try {
      for (size_t j = 0; j < fp.size(); j++) {
        pFreePunch punch = fp[j];
        punch->itype = oe.getControlIdFromPunch(punch->Time, punch->Type, punch->CardNo, true, 
                                                *punch);
        intkeymap<pFreePunch> &card2Punch = oe.punchIndex[punch->itype];
        if (card2Punch.count(punch->CardNo) == 0)
          card2Punch[punch->CardNo] = punch; // Insert into index
        else {
          // Duplicate, insert at free unused index (do not lose it)
          for (int k = 1; k < 50; k++) {
            intkeymap<pFreePunch> &c2P = oe.punchIndex[-k];
            if (c2P.count(punch->CardNo) == 0) {
              c2P[punch->CardNo] = punch;
              break;
            }
          }
        }
      }
    }
    catch(...) {
      disableHashing = false;
      throw;
    }
    disableHashing = false;
    return;
  }

  map<int, intkeymap<pFreePunch> >::iterator it;
  fp.reserve(oe.punchIndex.size() + 1);

  // Get all punches for the specified card.
  for(it = oe.punchIndex.begin(); it != oe.punchIndex.end(); ++it) {
    pFreePunch value;
    if (it->second.lookup(cardNo, value)) {
      assert(value && value->CardNo == cardNo);
      if (!value->isRemoved()) {
        fp.push_back(value);
        it->second.remove(cardNo);
      }
    }
  }

  if (newPunch)
    fp.push_back(newPunch);

  sort(fp.begin(), fp.end(), FreePunchComp());
  for (size_t j = 0; j < fp.size(); j++) {
    if (j>0 && fp[j-1] == fp[j])
      continue; //Skip duplicates
    pFreePunch punch = fp[j];
    punch->itype = oe.getControlIdFromPunch(punch->Time, punch->Type, cardNo, true, *punch);
    intkeymap<pFreePunch> &card2Punch = oe.punchIndex[punch->itype];
    if (card2Punch.count(cardNo) == 0)
      card2Punch[cardNo] = punch; // Insert into index
    else {
      // Duplicate, insert at free unused index (do not lose it)
      for (int k = 1; k < 50; k++) {
        intkeymap<pFreePunch> &c2P = oe.punchIndex[-k];
        if (c2P.count(cardNo) == 0) {
          c2P[cardNo] = punch;
          break;
        }
      }
    }
  }
}

const int legHashConstant = 100000;

int oEvent::getControlIdFromPunch(int time, int type, int card,
                                  bool markClassChanged, oFreePunch &punch) {
  pRunner r = getRunnerByCard(card);
  punch.tRunnerId = -1;
  punch.tMatchControlId = type;
  if (r!=0) {
    punch.tRunnerId = r->Id;
    r = r->tParentRunner ? r->tParentRunner : r;
  }
  int race = 0;
  // Try to figure out to which (multi)runner punch belong
  // by looking between which start and finnish the time is
  if (r && r->getNumMulti()) {
    pRunner r2 = r;
    while (r2 && r2->Card && race<r->getNumMulti()) {
      if (time >= r2->getStartTime() && time <= r2->getFinishTime()) {
        r = r2;
        punch.tRunnerId = r->Id;
        break;
      }
      race++;
      r2=r->getMultiRunner(race);
    }
  }

  if (type!=oPunch::PunchFinish) {  
    pCourse c = r ? r->getCourse(false): 0;

    if (c!=0) {
      for (int k=0; k<c->nControls; k++) {
        pControl ctrl=c->getControl(k);
        if(ctrl && ctrl->hasNumber(type)) {
          pFreePunch p = getPunch(race, ctrl->getId(), card);
          if(!p || (p && abs(p->Time-time)<60)) {
            ctrl->tHasFreePunchLabel = true;
            punch.tMatchControlId = ctrl->getId();
            int newId = punch.tMatchControlId + race*legHashConstant;
            if (newId != punch.itype && markClassChanged && r) {
              r->markClassChanged(ctrl->getId());
              if (punch.itype > 0)
                r->markClassChanged(punch.itype % legHashConstant);
            }

            //Code controlId and runner race number into code
            return newId;
          }          
        }
      }
    }
  }
  
  int newId = type + race*legHashConstant;

  if (newId != punch.itype && markClassChanged && r) {
    r->markClassChanged(type);
    if (punch.itype > 0)
      r->markClassChanged(punch.itype % legHashConstant);
  }

  return newId;
}

void oEvent::getFreeControls(set<int> &controlId) const
{
  controlId.clear();
  for (map<int, intkeymap<pFreePunch> >::const_iterator it = punchIndex.begin(); it != punchIndex.end(); ++it) {
    int id = it->first % legHashConstant;
    controlId.insert(id);
  }
}

//set< pair<int,int> > readPunchHash;
  
void oEvent::insertIntoPunchHash(int card, int code, int time) {
  if (time > 0) {
    int p1 = time * 4096 + code;
    int p2 = card;
    readPunchHash.insert(make_pair(p1, p2));
  }
}

void oEvent::removeFromPunchHash(int card, int code, int time) {
  int p1 = time * 4096 + code;
  int p2 = card;
  readPunchHash.erase(make_pair(p1, p2));
}

bool oEvent::isInPunchHash(int card, int code, int time) {
  int p1 = time * 4096 + code;
  int p2 = card;
  return readPunchHash.count(make_pair(p1, p2)) > 0;
}


pFreePunch oEvent::addFreePunch(int time, int type, int card, bool updateStartFinish) {	
	if (time > 0 && isInPunchHash(card, type, time))
    return 0;
  oFreePunch ofp(this, card, time, type);

	punches.push_back(ofp);	
	pFreePunch fp=&punches.back();
  oFreePunch::rehashPunches(*this, card, fp);
  insertIntoPunchHash(card, type, time);

  if (fp->getTiedRunner() && oe->isClient() && oe->getPropertyInt("UseDirectSocket", true)!=0) {
    SocketPunchInfo pi;
    pi.runnerId = fp->getTiedRunner()->getId();
    pi.time = fp->getAdjustedTime();
    pi.status = fp->getTiedRunner()->getStatus();
    if (fp->getTypeCode() > 10)
      pi.controlId = fp->getControlId();
    else
      pi.controlId = fp->getTypeCode();

    getDirectSocket().sendPunch(pi);
  }

  fp->updateChanged();
  fp->synchronize();

  // Update start/finish time
  if (updateStartFinish && type == oPunch::PunchStart || type == oPunch::PunchFinish) {
    pRunner tr = fp->getTiedRunner();
    if (tr && tr->getStatus() == StatusUnknown && time > 0) {
      tr->synchronize();
      if (type == oPunch::PunchStart)
          tr->setStartTime(time, true, false);
      else 
        tr->setFinishTime(time);

      // Direct result
      if (type == oPunch::PunchFinish && tr->getClassRef() && tr->getClassRef()->hasDirectResult()) {
        if (tr->getCourse(false) == 0 && tr->getCard() == 0) {
          tr->setStatus(StatusOK, true, false, true);
        }
        else if (tr->getCourse(false) != 0 && tr->getCard() == 0) {
          pCard card = allocateCard(tr);
          card->setupFromRadioPunches(*tr);
          vector<int> mp;
          card->synchronize();
          tr->addPunches(card, mp);
        }
      }

      tr->synchronize(true);
    }
  }
  if (fp->getTiedRunner())
    pushDirectChange();
  return fp;
}

pFreePunch oEvent::addFreePunch(oFreePunch &fp) { 
  insertIntoPunchHash(fp.CardNo, fp.Type, fp.Time);
	punches.push_back(fp);	
	pFreePunch fpz=&punches.back();
  oFreePunch::rehashPunches(*this, fp.CardNo, fpz);

  if (!fpz->existInDB() && HasDBConnection) {
    fpz->changed = true;
    fpz->synchronize();
  }
  return fpz;
}

void oEvent::removeFreePunch(int Id) {
	oFreePunchList::iterator it;	

	for (it=punches.begin(); it != punches.end(); ++it) {
    if(it->Id==Id) {
      pRunner r = getRunner(it->tRunnerId, 0);
      if (r && r->Class) {
        r->markClassChanged(it->tMatchControlId);
        classChanged(r->Class, true);
      }
      if(HasDBConnection) 
        msRemove(&*it);
      punchIndex[it->itype].remove(it->CardNo);
      int cardNo = it->CardNo;
      removeFromPunchHash(cardNo, it->Type, it->Time);
      punches.erase(it);
      oFreePunch::rehashPunches(*this, cardNo, 0);
      dataRevision++;
			return;
    }
	}
}

pFreePunch oEvent::getPunch(int Id) const
{
	oFreePunchList::const_iterator it;	

	for (it=punches.begin(); it != punches.end(); ++it) {
    if(it->Id==Id) {
      if (it->isRemoved())
        return 0;
			return const_cast<pFreePunch>(&*it);
    }
	}
	return 0;
}

pFreePunch oEvent::getPunch(int runnerRace, int type, int card) const
{
  //Lazy setup
  oFreePunch::rehashPunches(*oe, 0, 0);
  
  map<int, intkeymap<pFreePunch> >::const_iterator it1;
    
  int itype = type + runnerRace*legHashConstant;
  it1=punchIndex.find(itype);

  if (it1!=punchIndex.end()) {
    map<int, pFreePunch>::const_iterator it2;
    const intkeymap<pFreePunch> &cIndex=it1->second;

    //it2=cIndex.find(card);
    pFreePunch value;
    if(cIndex.lookup(card, value) && value && !value->isRemoved())
      return value;
  }

  map<pair<int, int>, oFreePunch>::const_iterator res = advanceInformationPunches.find(make_pair(itype, card));
  if (res != advanceInformationPunches.end())
    return (pFreePunch)&res->second;

	return 0;
}

void oEvent::getPunchesForRunner(int runnerId, vector<pFreePunch> &runnerPunches) const {
  runnerPunches.clear();
  pRunner r = getRunner(runnerId, 0);
  if (r == 0)
    return;

  // Get times for when other runners used this card
  vector< pair<int, int> > times;

  for (oRunnerList::const_iterator it = Runners.begin(); it != Runners.end(); ++it) {
    if (it->Id == runnerId)
      continue;
    if (it->Card && it->CardNo == r->CardNo) {
      pair<int, int> t = it->Card->getTimeRange();
      if (it->getStartTime() > 0)
        t.first = min(it->getStartTime(), t.first);

      if (it->getFinishTime() > 0)
        t.second = max(it->getFinishTime(), t.second);

      times.push_back(t);
    }
  }

  for (oFreePunchList::const_iterator it = punches.begin(); it != punches.end(); ++it) {
    if (it->CardNo == r->CardNo) {
      bool other = false;
      int t = it->Time;
      for (size_t k = 0; k<times.size(); k++) {
        if (t >= times[k].first && t <= times[k].second)
          other = true;
      }

      if (!other)
        runnerPunches.push_back(pFreePunch(&*it));
    }
  }

  // XXX Advance punches...
}


bool oEvent::advancePunchInformation(const vector<gdioutput *> &gdi, vector<SocketPunchInfo> &pi,
                                     bool fetchPunch, bool fetchFinish) {
  if (pi.empty())
    return false;

  bool m = false;
  for (size_t k = 0; k < pi.size(); k++) {
    pRunner r = getRunner(pi[k].runnerId, 0);
    if (!r)
      continue;
    if (pi[k].controlId == oPunch::PunchFinish && fetchFinish) {      
      if (r->getStatus() == StatusUnknown && r->getFinishTime() <= 0 && !r->isChanged()) {
        r->FinishTime = pi[k].time;
        r->tStatus = RunnerStatus(pi[k].status);
        r->status = RunnerStatus(pi[k].status); // Will be overwritten (do not set isChanged flag)
        if (r->Class) {
          r->markClassChanged(pi[k].controlId);
          classChanged(r->Class, false);
        }
        m = true;
      }
    }
    else if (fetchPunch) {

      if (getPunch(0, pi[k].controlId, r->getCardNo()) == 0) {
        oFreePunch fp(this, 0, pi[k].time, pi[k].controlId);
        fp.tRunnerId = pi[k].runnerId;
        fp.itype = pi[k].controlId;
        fp.tMatchControlId = fp.itype % legHashConstant;
        fp.changed = false;
        advanceInformationPunches.insert(make_pair(make_pair<int,int>(pi[k].controlId, r->getCardNo()),fp));
        if (r->Class) {
          r->markClassChanged(pi[k].controlId);
          classChanged(r->Class, true);
        }
        m = true;
      }
    }
  }
  if (m) {
    dataRevision++;
    for (size_t k = 0; k<gdi.size(); k++) {
      if (gdi[k]) 
        gdi[k]->makeEvent("DataUpdate", "autosync", 0, 0, false);
    }
  }
  return m;
}

pRunner oFreePunch::getTiedRunner() const {
  return oe->getRunner(tRunnerId, 0);
}

void oFreePunch::changedObject() {
  pRunner r = getTiedRunner();
  if (r && tMatchControlId>0)
    r->markClassChanged(tMatchControlId);
}
