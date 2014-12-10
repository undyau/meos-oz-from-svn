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

// oRunner.cpp: implementation of the oRunner class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "oRunner.h"

#include "oEvent.h"
#include "gdioutput.h"
#include "gdifonts.h"
#include "table.h"
#include "meos_util.h"
#include "oFreeImport.h"
#include <cassert>
#include "localizer.h"
#include <cmath>
#include "intkeymapimpl.hpp"
#include "runnerdb.h"
#include "meosexception.h"
#include <algorithm>
#include "oExtendedEvent.h"
#include "socket.h"

oRunner::RaceIdFormatter oRunner::raceIdFormatter;

const string &oRunner::RaceIdFormatter::formatData(oBase *ob) const {
  return itos(dynamic_cast<oRunner *>(ob)->getRaceIdentifier());
}

string oRunner::RaceIdFormatter::setData(oBase *ob, const string &input) const {
  int rid = atoi(input.c_str());
  if (input == "0")
    ob->getDI().setInt("RaceId", 0);
  else if(rid>0 && rid != dynamic_cast<oRunner *>(ob)->getRaceIdentifier())
    ob->getDI().setInt("RaceId", rid);
  return formatData(ob);
}

int oRunner::RaceIdFormatter::addTableColumn(Table *table, const string &description, int minWidth) const {
  return table->addColumn(description, max(minWidth, 90), true, true);
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
oAbstractRunner::oAbstractRunner(oEvent *poe):oBase(poe)
{
  Class=0;
  Club=0;
  startTime = 0;
  tStartTime = 0;
  
  FinishTime = 0;
	tStatus = status = StatusUnknown;

  inputPoints = 0;
  inputStatus = StatusOK;
  inputTime = 0;
  inputPlace = 0;
}

string oAbstractRunner::getInfo() const
{
  return getName();
}

void oAbstractRunner::setFinishTimeS(const string &t)
{
	setFinishTime(oe->getRelativeTime(t));
}

void oAbstractRunner::setStartTimeS(const string &t)
{
	setStartTime(oe->getRelativeTime(t), true, false);
}

oRunner::oRunner(oEvent *poe):oAbstractRunner(poe)
{
  isTemporaryObject = false;
	Id=oe->getFreeRunnerId();
	Course=0;
	StartNo=0;
	CardNo=0;

	tInTeam=0;
	tLeg=0;
	tNeedNoCard=false;
  tUseStartPunch=true;
	getDI().initData();
  correctionNeeded = false;

  tDuplicateLeg=0;
  tParentRunner=0;

	Card=0;
	cPriority=0;

  tCachedRunningTime = 0;
  tSplitRevision = -1;

  tRogainingPoints = 0;
  tAdaptedCourse = 0;
  tPenaltyPoints = 0;
  tAdaptedCourseRevision = -1;
}

oRunner::oRunner(oEvent *poe, int id):oAbstractRunner(poe)
{
  isTemporaryObject = false;
	Id=id;
  oe->qFreeRunnerId = max(id, oe->qFreeRunnerId);
	Course=0;
	StartNo=0;
	CardNo=0;

	tInTeam=0;
	tLeg=0;
	tNeedNoCard=false;
  tUseStartPunch=true;
	getDI().initData();
  correctionNeeded = false;

  tDuplicateLeg=0;
  tParentRunner=0;

	Card=0;
	cPriority=0;
  tCachedRunningTime = 0;
  tSplitRevision = -1;
  
  tRogainingPoints = 0;
  tAdaptedCourse = 0;
  tAdaptedCourseRevision = -1;
}

oRunner::~oRunner()
{
	if(tInTeam){
    for(unsigned i=0;i<tInTeam->Runners.size(); i++)
			if(tInTeam->Runners[i] && tInTeam->Runners[i]==this)
				tInTeam->Runners[i]=0;

		tInTeam=0;
	}

  for (size_t k=0;k<multiRunner.size(); k++) {
    if (multiRunner[k])
      multiRunner[k]->tParentRunner=0;  
  }

  if (tParentRunner) {    
    for (size_t k=0;k<tParentRunner->multiRunner.size(); k++) 
      if (tParentRunner->multiRunner[k] == this)
        tParentRunner->multiRunner[k]=0;
  }

  delete tAdaptedCourse;
  tAdaptedCourse = 0;
}

bool oRunner::Write(xmlparser &xml) 
{
	if(Removed) return true;

	xml.startTag("Runner");
	xml.write("Id", Id);
	xml.write("Updated", Modified.getStamp());
	xml.write("Name", Name);
	xml.write("Start", startTime);
	xml.write("Finish", FinishTime);
	xml.write("Status", status);
	xml.write("CardNo", CardNo);
	xml.write("StartNo", StartNo);

  xml.write("InputPoint", inputPoints);
  if (inputStatus != StatusOK)
    xml.write("InputStatus", itos(inputStatus)); //Force write of 0
  xml.write("InputTime", inputTime);
  xml.write("InputPlace", inputPlace);

	if(Club) xml.write("Club", Club->Id);
	if(Class) xml.write("Class", Class->Id);
	if(Course) xml.write("Course", Course->Id);

  if (multiRunner.size()>0)
    xml.write("MultiR", codeMultiR());

  if(Card) {
    assert(Card->tOwner==this);
    Card->Write(xml);
  }
	getDI().write(xml);

	xml.endTag();

  for (size_t k=0;k<multiRunner.size();k++)
    if(multiRunner[k])
      multiRunner[k]->Write(xml);

	return true;
}

void oRunner::Set(const xmlobject &xo)
{
  xmlList xl;
  xo.getObjects(xl);
	xmlList::const_iterator it;

	for(it=xl.begin(); it != xl.end(); ++it){
		if(it->is("Id")){
			Id = it->getInt();			
		}
		else if(it->is("Name")){
			Name = it->get();
		}
		else if(it->is("Start")){
      tStartTime = startTime = it->getInt();			
		}
		else if(it->is("Finish")){
			FinishTime = it->getInt();			
		}
		else if(it->is("Status")){
			tStatus = status = RunnerStatus(it->getInt());	
		}
		else if(it->is("CardNo")){
			CardNo=it->getInt();	
		}
		else if(it->is("StartNo") || it->is("OrderId"))
			StartNo=it->getInt();
		else if(it->is("Club"))
			Club=oe->getClub(it->getInt());
		else if(it->is("Class"))
			Class=oe->getClass(it->getInt());
		else if(it->is("Course"))
			Course=oe->getCourse(it->getInt());
		else if(it->is("Card")){
			Card=oe->allocateCard(this);
			Card->Set(*it);
      assert(Card->getId()!=0);
		}
		else if(it->is("oData"))
			getDI().set(*it);
		else if(it->is("Updated"))
			Modified.setStamp(it->get());
    else if(it->is("MultiR")) 
      decodeMultiR(it->get());
    else if (it->is("InputTime")) {
      inputTime = it->getInt();
    }
    else if (it->is("InputStatus")) {
      inputStatus = RunnerStatus(it->getInt());
    }
    else if (it->is("InputPoint")) {
      inputPoints = it->getInt();
    }
    else if (it->is("InputPlace")) {
      inputPlace = it->getInt();
    }
  }
}


int oAbstractRunner::getBirthAge() const {
  return 0;
}

int oRunner::getBirthAge() const {
  int y = getBirthYear();
  if (y > 0)
    return getThisYear() - y;
  return 0;
}

void oAbstractRunner::addClassDefaultFee(bool resetFees) {
  if (Class) {
    oDataInterface di = getDI();

    if (isVacant()) {
      di.setInt("Fee", 0);
      di.setInt("EntryDate", 0);
      di.setInt("Paid", 0);
      if (typeid(*this)==typeid(oRunner))
        di.setInt("CardFee", 0);
      return;
    }
    string date = getEntryDate();

    int currentFee = di.getInt("Fee");

    pTeam t = getTeam();
    if (t && t != this) {
      // Thus us a runner in a team 
      // Check if the team has a fee.
      // Don't assign personal fee if so.
      if (t->getDCI().getInt("Fee") > 0)
        return;
    }

    if (currentFee == 0 || resetFees) {
      int age = getBirthAge();
      int fee = Class->getEntryFee(di.getDate("EntryDate"), age);
      di.setInt("Fee", fee);
    }
  }
}

// Get entry date of runner (or its team)
string oRunner::getEntryDate(bool useTeamEntryDate) const {
  if (useTeamEntryDate && tInTeam) {
    string date = tInTeam->getEntryDate(false);
    if (!date.empty())
      return date;
  }
  oDataConstInterface dci = getDCI();
  int date = dci.getInt("EntryDate");
  if (date == 0) {
    (const_cast<oRunner *>(this)->getDI()).setDate("EntryDate", getLocalDate());
  }
  return dci.getDate("EntryDate");
}

string oRunner::codeMultiR() const
{
  char bf[32];
  string r;

  for (size_t k=0;k<multiRunner.size() && multiRunner[k];k++) {
    if(!r.empty())
      r+=":";
    sprintf_s(bf, "%d", multiRunner[k]->getId());
    r+=bf;
  }
  return r;
}

void oRunner::decodeMultiR(const string &r)
{
  vector<string> sv;
  split(r, ":", sv);  
  multiRunnerId.clear();

  for (size_t k=0;k<sv.size();k++) {
    int d = atoi(sv[k].c_str());
    if (d>0) 
      multiRunnerId.push_back(d);
  }
  multiRunnerId.push_back(0); // Mark as containing something
}

void oAbstractRunner::setClassId(int id)
{
	pClass pc=Class;
  Class=id ? oe->getClass(id):0;
	
  if (Class!=pc) {
    apply(false, 0, false);
    if(Class) {
      Class->clearCache(true);
    }
    if(pc) {
      pc->clearCache(true);
    }
		updateChanged();
  }
}

// Update all classes (for multirunner)
void oRunner::setClassId(int id)
{
  if (tParentRunner)
    tParentRunner->setClassId(id);
  else { 
	  pClass pc = Class;
    pClass nPc = id>0 ? oe->getClass(id):0;

    if (pc && pc->isSingleRunnerMultiStage() && nPc!=pc && tInTeam) {
      if (!isTemporaryObject) {
        oe->autoRemoveTeam(this);

        if (nPc) {
          int newNR = max(nPc->getNumMultiRunners(0), 1);
          for (size_t k = newNR - 1; k<multiRunner.size(); k++) {
            if (multiRunner[k]) {
              assert(multiRunner[k]->tParentRunner == this);
              multiRunner[k]->tParentRunner = 0;
              vector<int> toRemove;
              toRemove.push_back(multiRunner[k]->Id);
              oe->removeRunner(toRemove);
            }
          }
          multiRunner.resize(newNR-1);
        }
      }
    }

    Class = nPc;

    if (Class != 0 && Class != pc && tInTeam==0 && 
                      Class->isSingleRunnerMultiStage()) {
      if (!isTemporaryObject) {
        pTeam t = oe->addTeam(getName(), getClubId(), getClassId());
        t->setStartNo(StartNo, false);
        t->setRunner(0, this, true);
      }
    }

    apply(false, 0, false); //We may get old class back from team.

    for (size_t k=0;k<multiRunner.size();k++) {
      if (multiRunner[k] && Class!=multiRunner[k]->Class) {
		    multiRunner[k]->Class=Class;
		    multiRunner[k]->updateChanged();
      }
    }

    if(Class!=pc && !isTemporaryObject) {
      if (Class) {
        Class->clearCache(true);
      }
      if (pc) {
        pc->clearCache(true);
      }
      tSplitRevision = 0;
		  updateChanged();
    }
	}  
}

void oRunner::setCourseId(int id)
{
	pCourse pc=Course;

	if(id>0)
		Course=oe->getCourse(id);
	else 
		Course=0;

  if(Course!=pc) {
		updateChanged();
    if (Class)
      Class->clearSplitAnalysis();
    tSplitRevision = 0;
  }
}

bool oAbstractRunner::setStartTime(int t, bool updateSource, bool tmpOnly, bool recalculate) {
  assert(!(updateSource && tmpOnly));
  if (tmpOnly) {
    tmpStore.startTime = t;
    return false;
  }

	int tOST=tStartTime;
  if(t>0)
		tStartTime=t;
	else tStartTime=0;

  if (updateSource) {
    int OST=startTime;
    startTime = tStartTime;

    if(OST!=startTime) {
		  updateChanged();   
    }
  }  

  if (tOST != tStartTime) {
    changedObject();
    if (Class) {
      Class->clearCache(false);
    }
  }

  if(tOST<tStartTime && Class && recalculate)
    oe->reCalculateLeaderTimes(Class->getId());

  return tOST != tStartTime;
}

void oAbstractRunner::setFinishTime(int t)
{
	int OFT=FinishTime;

	if (t>tStartTime)
		FinishTime=t;
	else //Beeb
		FinishTime=0;

  if (OFT != FinishTime) {
		updateChanged();
    if (Class) {
      Class->clearCache(false);
    }
  }

  if(OFT>FinishTime && Class)
    oe->reCalculateLeaderTimes(Class->getId());
}

void oRunner::setFinishTime(int t)
{
  bool update=false;
  if(Class && (getTimeAfter(tDuplicateLeg)==0 || getTimeAfter()==0))
    update=true;
  
  oAbstractRunner::setFinishTime(t);

  tSplitRevision = 0;

  if(update && t!=FinishTime)
    oe->reCalculateLeaderTimes(Class->getId());
}

string oAbstractRunner::getStartTimeS() const
{
	if(tStartTime>0)
		return oe->getAbsTime(tStartTime);
	else return "-";
}

string oAbstractRunner::getStartTimeCompact() const
{
	if(tStartTime>0)
    if(oe->useStartSeconds())
		  return oe->getAbsTime(tStartTime);
    else
      return oe->getAbsTimeHM(tStartTime);

	else return "-";
}

string oAbstractRunner::getFinishTimeS() const
{
	if(FinishTime>0)
		return oe->getAbsTime(FinishTime);
	else return "-";
}

const string &oAbstractRunner::getRunningTimeS() const
{
  return formatTime(getRunningTime());
}

string oAbstractRunner::getTotalRunningTimeS() const
{
  return formatTime(getTotalRunningTime());
}

int oAbstractRunner::getTotalRunningTime() const {
  int t = getRunningTime();
  if (t > 0 && inputTime>=0)
    return t + inputTime;
  else
    return 0;
}

int oRunner::getTotalRunningTime() const {
  return getTotalRunningTime(getFinishTime());
}


const string &oAbstractRunner::getStatusS() const
{
  return oEvent::formatStatus(tStatus);
}

const string &oAbstractRunner::getTotalStatusS() const
{
  return oEvent::formatStatus(getTotalStatus());
}

/*
 - Inactive		: Has not yet started
	 - DidNotStart	: Did Not Start (in this race)
	 - Active		: Currently on course
	 - Finished		: Finished but not validated
	 - OK			: Finished and validated
	 - MisPunch		: Missing Punch
	 - DidNotFinish	: Did Not Finish
	 - Disqualified	: Disqualified
	 - NotCompeting	: Not Competing (running outside the competition)
	 - SportWithdr	: Sporting Withdrawal (e.g. helping injured)
	 - OverTime 	: Overtime, i.e. did not finish within max time
	 - Moved		: Moved to another class
	 - MovedUp		: Moved to a "better" class, in case of entry
	 				  restrictions
	 - Cancelled	
*/
const char *formatIOFStatus(RunnerStatus s) {
	switch(s)	{
	case StatusOK:
		return "OK";
	case StatusDNS:
		return "DidNotStart";
	case StatusMP:
		return "MisPunch";
	case StatusDNF:
		return "DidNotFinish";
	case StatusDQ:
		return "Disqualified";
	case StatusMAX:
		return "OverTime";
	}
	return "Inactive";

}

string oAbstractRunner::getIOFStatusS() const
{
  return formatIOFStatus(tStatus);
}

string oAbstractRunner::getIOFTotalStatusS() const
{
  return formatIOFStatus(getTotalStatus());
}


void oRunner::addPunches(pCard card, vector<int> &MissingPunches)
{

  RunnerStatus oldStatus = getStatus();
  int oldFinishTime = getFinishTime();
  pCard oldCard = Card;

  if (Card && card != Card) {
    Card->tOwner = 0;
  }

	Card = card;
	
	updateChanged();

	if(card) {
		if(CardNo==0)
			CardNo=card->CardNo;

    assert(card->tOwner==0 || card->tOwner==this);
	}

  if(Card)
    Card->tOwner=this;

	evaluateCard(true, MissingPunches, 0, true);

  if (oe->isClient() && oe->getPropertyInt("UseDirectSocket", true)!=0) {
    if (oldStatus != getStatus() || oldFinishTime != getFinishTime()) {
      SocketPunchInfo pi;
      pi.runnerId = getId();
      pi.time = getFinishTime();
      pi.status = getStatus();
      pi.controlId = oPunch::PunchFinish;
      oe->getDirectSocket().sendPunch(pi);
    }
  }
  
  oe->pushDirectChange();
  if (oldCard && Card && oldCard != Card && oldCard->isConstructedFromPunches())
    oldCard->remove(); // Remove card constructed from punches
}

pCourse oRunner::getCourse(bool useAdaptedCourse) const {
  pCourse tCrs = 0;
	if (Course) 
    tCrs = Course;
  else if (Class) {		
		if (Class->hasMultiCourse()) {
      if (tInTeam) {
        if (size_t(tLeg) >= tInTeam->Runners.size() || tInTeam->Runners[tLeg] != this) {
          tInTeam->quickApply();
        }
      }
      
      if(tInTeam) {
        unsigned leg=legToRun();
			  if (leg<Class->MultiCourse.size()) {
				  vector<pCourse> &courses=Class->MultiCourse[leg];
				  if(courses.size()>0) {
					  int index=StartNo;
            if(index>0)
              index = (index-1) % courses.size();

					  tCrs = courses[index];
				  }
			  }
      }
      else {
			  if (unsigned(tDuplicateLeg)<Class->MultiCourse.size()) {
				  vector<pCourse> &courses=Class->MultiCourse[tDuplicateLeg];
				  if(courses.size()>0) {
					  int index=StartNo % courses.size();
					  tCrs = courses[index];
				  }
			  }
      }
		}
		else
      tCrs = Class->Course;
	}

  if (tCrs && useAdaptedCourse && Card && tCrs->getCommonControl() != 0) {
    if (tAdaptedCourse && tAdaptedCourseRevision == oe->dataRevision) {
      return tAdaptedCourse;
    }
    if (!tAdaptedCourse)
      tAdaptedCourse = new oCourse(oe, -1);
    
    tCrs = tCrs->getAdapetedCourse(*Card, *tAdaptedCourse);
    tAdaptedCourseRevision = oe->dataRevision;
    return tCrs;
  }

	return tCrs;
}

string oRunner::getCourseName() const
{
	pCourse oc=getCourse(false);
	if(oc) return oc->getName();
	return MakeDash("-");
}

#define NOTATIME 0xF0000000
void oAbstractRunner::resetTmpStore() {
  tmpStore.startTime = startTime;
  tmpStore.status = status;
  tmpStore.startNo = StartNo;
  tmpStore.bib = getBib();
}

bool oAbstractRunner::setTmpStore() {
  bool res = false;
  setStartNo(tmpStore.startNo, false);
  res |= setStartTime(tmpStore.startTime, false, false, false);
  res |= setStatus(tmpStore.status, false, false, false);
  setBib(tmpStore.bib, false, false);
  return res;
}

bool oRunner::evaluateCard(bool doApply, vector<int> & MissingPunches, 
                              int addpunch, bool sync) {
  MissingPunches.clear();
  int oldFT = FinishTime;
  int oldStartTime;
  RunnerStatus oldStatus;
  int *refStartTime;
  RunnerStatus *refStatus;
  
  if (doApply) {
    oldStartTime = tStartTime;
    tStartTime = startTime;
    oldStatus = tStatus;
    tStatus = status;
    refStartTime = &tStartTime;
    refStatus = &tStatus;

    resetTmpStore();
    apply(sync, 0, true);
  }
  else {
    // tmp initialized from outside. Do not change tStatus, tStartTime. Work with tmpStore instead!
    oldStartTime = tStartTime;
    oldStatus = tStatus;
    refStartTime = &tmpStore.startTime;
    refStatus = &tmpStore.status;
  
    createMultiRunner(false, sync);
  }

  bool inTeam = tInTeam != 0;
  tProblemDescription.clear();

  vector<SplitData> oldTimes;
  swap(splitTimes, oldTimes);

  if(!Card) {
    if((inTeam || !tUseStartPunch) && doApply)
      apply(sync, 0, false); //Post apply. Update start times.

    if (storeTimes() && Class && sync) {
      set<int> cls;
      cls.insert(Class->getId());
      oe->reEvaluateAll(cls, sync);
    }
    normalizedSplitTimes.clear();
    if (oldTimes.size() > 0 && Class)
      Class->clearSplitAnalysis();
		return false;
  }
	//Try to match class?!
	if(!Class)
		return false;

	const pCourse course = getCourse(true);
	oPunchList::iterator p_it;

  if (!course) {
    // Reset rogaining. Store start/finish
    for (p_it=Card->Punches.begin(); p_it!=Card->Punches.end(); ++p_it) {
      p_it->tRogainingIndex = -1;
      p_it->isUsed = false;
      p_it->tIndex = -1;
      p_it->tMatchControlId = -1;
      p_it->tTimeAdjust = 0;
      if (p_it->isStart() && tUseStartPunch)
        *refStartTime = p_it->Time;
      else if (p_it->isFinish())
        setFinishTime(p_it->Time);
    }
    if((inTeam || !tUseStartPunch) && doApply)
      apply(sync, 0, false); //Post apply. Update start times.

    storeTimes();
		return false;
  }

  int startPunchCode = course->getStartPunchType();
  int finishPunchCode = course->getFinishPunchType();

  bool hasRogaining = course->hasRogaining();

  // Pairs: <control index, point>
  intkeymap< pair<int, int> > rogaining(course->getNumControls());
  for (int k = 0; k< course->nControls; k++) {
    if (course->Controls[k] && course->Controls[k]->isRogaining(hasRogaining)) {
      int pt = course->Controls[k]->getRogainingPoints();
      if (pt > 0) {
        for (int j = 0; j<course->Controls[k]->nNumbers; j++) {
          rogaining.insert(course->Controls[k]->Numbers[j], make_pair(k, pt));
        }
      }
    }
  }

  if(addpunch && Card->Punches.empty()) {		
    Card->addPunch(addpunch, -1, course->Controls[0] ? course->Controls[0]->getId():0);
  }

	if (Card->Punches.empty()) {
    for(int k=0;k<course->nControls;k++) {
      if (course->Controls[k]) {
        course->Controls[k]->startCheckControl();
        course->Controls[k]->addUncheckedPunches(MissingPunches, hasRogaining);
      }
    }
    if((inTeam || !tUseStartPunch) && doApply)
      apply(sync, 0, false); //Post apply. Update start times.

    if (storeTimes() && Class && sync) {
      set<int> cls;
      cls.insert(Class->getId());
      oe->reEvaluateAll(cls, sync);
    }

    normalizedSplitTimes.clear();
    if (oldTimes.size() > 0 && Class)
      Class->clearSplitAnalysis();

    return false;
	}

  // Reset rogaining
  for (p_it=Card->Punches.begin(); p_it!=Card->Punches.end(); ++p_it) {
    p_it->tRogainingIndex = -1;
  }

  bool clearSplitAnalysis = false;
	
  	
  //Search for start and update start time.
	p_it=Card->Punches.begin();
  while ( p_it!=Card->Punches.end()) {
    if (p_it->Type == startPunchCode) {
		  if(tUseStartPunch && p_it->getAdjustedTime() != *refStartTime) {
        p_it->setTimeAdjust(0);
			  *refStartTime = p_it->getAdjustedTime();
        if (*refStartTime != oldStartTime)
          clearSplitAnalysis = true;
			  //updateChanged();
		  }
      break;
	  }
	  ++p_it;
  }
  inthashmap expectedPunchCount(course->nControls);
  inthashmap punchCount(Card->Punches.size());
  for (int k=0; k<course->nControls; k++) {
    pControl ctrl=course->Controls[k];
    if (ctrl && !ctrl->isRogaining(hasRogaining)) {
      for (int j = 0; j<ctrl->nNumbers; j++)
        ++expectedPunchCount[ctrl->Numbers[j]];
    }
  }

  for (p_it = Card->Punches.begin(); p_it != Card->Punches.end(); ++p_it) {
    if(p_it->Type>=10 && p_it->Type<=1024)
      ++punchCount[p_it->Type];
  }

  p_it = Card->Punches.begin();	
  splitTimes.resize(course->nControls, SplitData(NOTATIME, SplitData::Missing));
  int k=0;


  for (k=0;k<course->nControls;k++) {
    //Skip start finish check
    while(p_it!=Card->Punches.end() && 
          (p_it->isCheck() || p_it->isFinish() || p_it->isStart())) {
      p_it->setTimeAdjust(0);
      ++p_it;
    }

    if(p_it==Card->Punches.end())
      break;

    oPunchList::iterator tp_it=p_it;
		pControl ctrl=course->Controls[k];
    int skippedPunches = 0;

		if (ctrl) {
      int timeAdjust=ctrl->getTimeAdjust();
      ctrl->startCheckControl();

      // Add rogaining punches
      if (addpunch && ctrl->isRogaining(hasRogaining) && ctrl->getFirstNumber() == addpunch) {
        if ( Card->getPunchByType(addpunch) == 0) {
          oPunch op(oe);
				  op.Type=addpunch;
				  op.Time=-1;
				  op.isUsed=true;
          op.tIndex = k;
          op.tMatchControlId=ctrl->getId();
				  Card->Punches.insert(tp_it, op);
          Card->updateChanged();
        }
      }

      if (ctrl->getStatus() == oControl::StatusBad) {
        // The control is marked "bad" but we found it anyway in the card. Mark it as used.
        if (tp_it!=Card->Punches.end() && ctrl->hasNumberUnchecked(tp_it->Type)) {
				  tp_it->isUsed=true; //Show that this is used when splittimes are calculated.
                            // Adjust if the time of this control was incorrectly set.
          tp_it->setTimeAdjust(timeAdjust);
          tp_it->tMatchControlId=ctrl->getId();
          tp_it->tIndex = k;
          splitTimes[k].setPunchTime(tp_it->getAdjustedTime());
					++tp_it;
					p_it=tp_it;
        }
      }
      else {
        while(!ctrl->controlCompleted(hasRogaining) && tp_it!=Card->Punches.end()) {
				  if (ctrl->hasNumberUnchecked(tp_it->Type)) {

            if (skippedPunches>0) {
              if (ctrl->Status == oControl::StatusOK) {
                int code = tp_it->Type;
                if (expectedPunchCount[code]>1 && punchCount[code] < expectedPunchCount[code]) {
                  tp_it==Card->Punches.end();
                  ctrl->uncheckNumber(code);
                  break;
                }
              }
            }
					  tp_it->isUsed=true; //Show that this is used when splittimes are calculated.
            // Adjust if the time of this control was incorrectly set.
            tp_it->setTimeAdjust(timeAdjust);
            tp_it->tMatchControlId=ctrl->getId();
            tp_it->tIndex = k;
            if (ctrl->controlCompleted(hasRogaining))
              splitTimes[k].setPunchTime(tp_it->getAdjustedTime());
					  ++tp_it;
					  p_it=tp_it;
				  }
				  else {					
					  if(ctrl->hasNumberUnchecked(addpunch)){
						  //Add this punch. 
						  oPunch op(oe);
						  op.Type=addpunch;
						  op.Time=-1;
						  op.isUsed=true;

              op.tMatchControlId=ctrl->getId();
						  op.tIndex = k;
              Card->Punches.insert(tp_it, op);
						  Card->updateChanged();
						  if (ctrl->controlCompleted(hasRogaining))
                splitTimes[k].setPunched();						
					  }
					  else {
              skippedPunches++;
						  tp_it->isUsed=false;
						  ++tp_it;
					  }
				  }
			  }
      }

      if (tp_it==Card->Punches.end() && !ctrl->controlCompleted(hasRogaining) 
                    && ctrl->hasNumberUnchecked(addpunch) ) {
        Card->addPunch(addpunch, -1, ctrl->getId());
				if (ctrl->controlCompleted(hasRogaining))
          splitTimes[k].setPunched();
				Card->Punches.back().isUsed=true;		
        Card->Punches.back().tMatchControlId=ctrl->getId();
        Card->Punches.back().tIndex = k;
			}

      if(ctrl->controlCompleted(hasRogaining) && splitTimes[k].time == NOTATIME)
        splitTimes[k].setPunched();
		}
		else //if(ctrl && ctrl->Status==oControl::StatusBad){
			splitTimes[k].setNotPunched();

    //Add missing punches
    if(ctrl && !ctrl->controlCompleted(hasRogaining))
      ctrl->addUncheckedPunches(MissingPunches, hasRogaining);
	}

  //Add missing punches for remaining controls
  while (k<course->nControls) {
    if (course->Controls[k]) {
      pControl ctrl = course->Controls[k];
      ctrl->startCheckControl();

      if (ctrl->hasNumberUnchecked(addpunch)) {
        Card->addPunch(addpunch, -1, ctrl->getId());
				Card->updateChanged();
				if (ctrl->controlCompleted(hasRogaining))
          splitTimes[k].setNotPunched();						
			}
      ctrl->addUncheckedPunches(MissingPunches, hasRogaining);
    }
	  k++;		
  }

	//Set the rest (if exist -- probably not) to "not used"
	while(p_it!=Card->Punches.end()){
		p_it->isUsed=false;
    p_it->tIndex = -1;
    p_it->setTimeAdjust(0);
		++p_it;
	}
  
  int OK = MissingPunches.empty();
  
  tRogaining.clear();
  tRogainingPoints = 0;
  int time_limit = 0;

  // Rogaining logicu
  if (rogaining.size() > 0) {
    set<int> visitedControls;
    for (p_it=Card->Punches.begin(); p_it != Card->Punches.end(); ++p_it) {
      pair<int, int> pt;
      if (rogaining.lookup(p_it->Type, pt)) {
        if (visitedControls.count(pt.first) == 0) {
          visitedControls.insert(pt.first); // May noy be revisited
          p_it->isUsed = true;
          p_it->tRogainingIndex = pt.first;
          p_it->tMatchControlId = course->Controls[pt.first]->getId();
          tRogaining.push_back(make_pair(course->Controls[pt.first], p_it->getAdjustedTime()));
          tRogainingPoints += pt.second;
        }
      }
    }

    int point_limit = course->getMinimumRogainingPoints();
    if (point_limit>0 && tRogainingPoints<point_limit) {
      tProblemDescription = "X po�ng fattas.#" + itos(point_limit-tRogainingPoints);
      OK = false;
    }

    // Check this later
    time_limit = course->getMaximumRogainingTime();
	}

  for (int k = 1; k<course->nControls; k++) {
    if (course->Controls[k] && course->Controls[k]->isRogaining(hasRogaining)) {
      splitTimes[k] = splitTimes[k-1];      
    }
  }

  int maxTimeStatus = 0;
  if (Class && FinishTime>0) {
    int mt = Class->getMaximumRunnerTime();
    if (mt>0) {
      if (getRunningTime() > mt)
        maxTimeStatus = 1;
      else
        maxTimeStatus = 2;
    }
    else
      maxTimeStatus = 2;
  }

  if (*refStatus == StatusMAX && maxTimeStatus == 2)
    *refStatus = StatusUnknown;

	if(OK && (*refStatus==0 || *refStatus==StatusDNS || *refStatus==StatusMP || *refStatus==StatusOK || *refStatus==StatusDNF))
		*refStatus = StatusOK;
	else	*refStatus = RunnerStatus(max(int(StatusMP), int(*refStatus)));

  oPunchList::reverse_iterator backIter = Card->Punches.rbegin();

  if (finishPunchCode != oPunch::PunchFinish) {
    while (backIter != Card->Punches.rend()) {
      if (backIter->Type == finishPunchCode)
        break;
      ++backIter;
    }
  }
  
  if (backIter != Card->Punches.rend() && backIter->Type == finishPunchCode) {
		FinishTime = backIter->getAdjustedTime();
    if (finishPunchCode == oPunch::PunchFinish)
      backIter->tMatchControlId=oPunch::PunchFinish;
  }
	else if (FinishTime<=0) {
		*refStatus=RunnerStatus(max(int(StatusDNF), int(tStatus)));
    tProblemDescription = "M�ltid saknas.";
		FinishTime=0;
	}

  if (*refStatus == StatusOK && maxTimeStatus == 1)
    *refStatus = StatusMAX; //Maxtime

  if (!MissingPunches.empty()) {
    tProblemDescription  = "St�mplingar saknas: X#" + itos(MissingPunches[0]);
    for (unsigned j = 1; j<3; j++) {
      if (MissingPunches.size()>j)
        tProblemDescription += ", " + itos(MissingPunches[j]);
    }
    if (MissingPunches.size()>3)
      tProblemDescription += "...";
    else
      tProblemDescription += ".";
  }

  // Adjust times on course, including finish time
  doAdjustTimes(course);

	if (time_limit > 0) {
    int rt = getRunningTime();
    if (rt > 0) {
      int overTime = rt - time_limit;
      if (overTime > 0) {
        tPenaltyPoints = course->calculateReduction(overTime);
        tProblemDescription = "Tidsavdrag: X po�ng.#" + itos(tPenaltyPoints); 
        tRogainingPoints = max(0, tRogainingPoints - tPenaltyPoints);
      }
			else
				tPenaltyPoints = 0;  // should be initialised to this - being corrupted somewhere ??
    }
  }

  if(oldStatus!=*refStatus || oldFT!=FinishTime) {
    clearSplitAnalysis = true;
  }

  if((inTeam || !tUseStartPunch) && doApply)
    apply(sync, 0, false); //Post apply. Update start times.

  if (tCachedRunningTime != FinishTime - *refStartTime) {
    tCachedRunningTime = FinishTime - *refStartTime;
    clearSplitAnalysis = true;
  }

  // Clear split analysis data if necessary
  bool clear = splitTimes.size() != oldTimes.size() || clearSplitAnalysis;
  for (size_t k = 0; !clear && k<oldTimes.size(); k++) {
    if (splitTimes[k].time != oldTimes[k].time)
      clear = true;
  }

  if (clear) {
    normalizedSplitTimes.clear();
    if (Class)
      Class->clearSplitAnalysis();
  }
  
  if (doApply)
    storeTimes();
  if (Class && sync) {
    bool update = false;
    if (tInTeam) {
      int t1 = Class->getTotalLegLeaderTime(tLeg, false);
      int t2 = tInTeam->getLegRunningTime(tLeg, false);
      if (t2<=t1 && t2>0)
        update = true;

      int t3 = Class->getTotalLegLeaderTime(tLeg, true);
      int t4 = tInTeam->getLegRunningTime(tLeg, true);
      if (t4<=t3 && t4>0)
        update = true;
    }

    if (!update) {
      int t1 = Class->getBestLegTime(tLeg);
      int t2 = getRunningTime();
      if (t2<=t1 && t2>0)
        update = true;
    }
    if (update) {
      set<int> cls;
      cls.insert(Class->getId());
      oe->reEvaluateAll(cls, sync);
    }
  }
	return true;
}

void oRunner::clearOnChangedRunningTime() {
  if (tCachedRunningTime != FinishTime - tStartTime) {
    tCachedRunningTime = FinishTime - tStartTime;
    normalizedSplitTimes.clear();
    if (Class)
      Class->clearSplitAnalysis();
  }
}

void oRunner::doAdjustTimes(pCourse course) {
  if (!Card)
    return;

  assert(course->nControls == splitTimes.size());
  int adjustment = 0;
  oPunchList::iterator it = Card->Punches.begin();

  adjustTimes.resize(splitTimes.size());
  for (int n = 0; n < course->nControls; n++) {
    pControl ctrl = course->Controls[n];
    if (!ctrl)
      continue;

    while (it != Card->Punches.end() && !it->isUsed) {
      it->setTimeAdjust(adjustment);
      ++it;
    }

    int minTime = ctrl->getMinTime();
    if (ctrl->getStatus() == oControl::StatusNoTiming) {
      int t = 0;
      if (n>0 && splitTimes[n].time>0 && splitTimes[n-1].time>0) {
        t = splitTimes[n].time + adjustment - splitTimes[n-1].time;
      }
      else if (n == 0 && splitTimes[n].time>0) {
        t = splitTimes[n].time - tStartTime;
      }
      adjustment -= t;
    }
    else if (minTime>0) {
      int t = 0;
      if (n>0 && splitTimes[n].time>0 && splitTimes[n-1].time>0) {
        t = splitTimes[n].time + adjustment - splitTimes[n-1].time;
      }
      else if (n == 0 && splitTimes[n].time>0) {
        t = splitTimes[n].time - tStartTime;
      }
      int maxadjust = max(minTime-t, 0);
      adjustment += maxadjust;
    }

    if (it != Card->Punches.end() && it->tMatchControlId == ctrl->getId()) {
      it->adjustTimeAdjust(adjustment);
      ++it;
    }

    adjustTimes[n] = adjustment;
    if (splitTimes[n].time>0)
      splitTimes[n].time += adjustment;
  }
  
  // Adjust remaining
  while (it != Card->Punches.end()) {
    it->setTimeAdjust(adjustment);
    ++it;
  }

  FinishTime += adjustment;
}

bool oRunner::storeTimes()
{
  bool updated = false;
  //Store best time in class
  if (tInTeam) {
    if (Class && unsigned(tLeg)<Class->tLeaderTime.size()) {
      // Update for extra/optional legs
      int firstLeg = tLeg;
      int lastLeg = tLeg + 1;
      while(firstLeg>0 && Class->legInfo[firstLeg].isOptional())
        firstLeg--;
      int nleg = Class->legInfo.size();
      while(lastLeg<nleg && Class->legInfo[lastLeg].isOptional())
        lastLeg++;

      for (int leg = firstLeg; leg<lastLeg; leg++) {
        if (tStatus==StatusOK) {
          int &bt=Class->tLeaderTime[leg].bestTimeOnLeg;
          int rt=getRunningTime(); 
          if (rt > 0 && (bt == 0 || rt < bt)) {
            bt=rt;
            updated = true;
          }
        }
      }

      bool updateTotal = true;
      bool updateTotalInput = true;
      
      int basePLeg = firstLeg;
      while (basePLeg > 0 && Class->legInfo[basePLeg].isParallel())
        basePLeg--;

      int ix = basePLeg;
      while (ix < nleg && (ix == basePLeg || Class->legInfo[ix].isParallel()) ) {
        updateTotal = updateTotal && tInTeam->getLegStatus(ix, false)==StatusOK;
        updateTotalInput = updateTotalInput && tInTeam->getLegStatus(ix, true)==StatusOK;
        ix++;
      }

      if (updateTotal) {
        int rt = 0;
        int ix = basePLeg;
        while (ix < nleg && (ix == basePLeg || Class->legInfo[ix].isParallel()) ) {
          rt = max(rt, tInTeam->getLegRunningTime(ix, false));
          ix++;
        }

        for (int leg = firstLeg; leg<lastLeg; leg++) {
          int &bt=Class->tLeaderTime[leg].totalLeaderTime; 
          if(rt > 0 && (bt == 0 || rt < bt)) {
            bt=rt;
            updated = true;
          }
        }
      }
      if (updateTotalInput) {
        //int rt=tInTeam->getLegRunningTime(tLeg, true);
        int rt = 0;
        int ix = basePLeg;
        while (ix < nleg && (ix == basePLeg || Class->legInfo[ix].isParallel()) ) {
          rt = max(rt, tInTeam->getLegRunningTime(ix, true));
          ix++;
        }
        for (int leg = firstLeg; leg<lastLeg; leg++) {
          int &bt=Class->tLeaderTime[leg].totalLeaderTimeInput;      
          if(rt > 0 && (bt == 0 || rt < bt)) {
            bt=rt;
            updated = true;
          }
        }
      }
    
      // Normalize total times for parallell legs (take maximal value)
      /*int maxTotal = 0, maxTotalInput = 0;
      int ix = basePLeg;
      while (ix < nleg && (ix == basePLeg || Class->legInfo[ix].isParallel()) ) {
        maxTotal = max(maxTotal, Class->tLeaderTime[ix].totalLeaderTime);
        maxTotalInput = max(maxTotalInput, Class->tLeaderTime[ix].totalLeaderTimeInput);
        ix++;
      }
      ix = basePLeg;
      while (ix < nleg && (ix == basePLeg || Class->legInfo[ix].isParallel()) ) {
        Class->tLeaderTime[ix].totalLeaderTime = maxTotal;
        Class->tLeaderTime[ix].totalLeaderTimeInput = maxTotalInput;
        ix++;
      }*/
    }
  }
  else {
    if (Class && unsigned(tDuplicateLeg)<Class->tLeaderTime.size()) {
      if (tStatus==StatusOK) {
        int &bt=Class->tLeaderTime[tDuplicateLeg].bestTimeOnLeg;
        int rt=getRunningTime(); 
        if (rt > 0 && (bt==0 || rt<bt)) {
          bt=rt;
          updated = true;
        }
      }
      int &bt=Class->tLeaderTime[tDuplicateLeg].totalLeaderTime;
      int rt=getRaceRunningTime(tDuplicateLeg);
      if(rt>0 && (bt==0 || rt<bt)) {
        bt = rt;
        updated = true;
        Class->tLeaderTime[tDuplicateLeg].totalLeaderTimeInput = rt;
      }
    }
  }

  // Best input time
  if (Class && unsigned(tLeg)<Class->tLeaderTime.size()) {
    int &it = Class->tLeaderTime[tLeg].inputTime;
    if (inputTime > 0 && inputStatus == StatusOK && (it == 0 || inputTime < it) ) {
      it = inputTime;
      updated = true;
    }
  }
 
  if (Class && tStatus==StatusOK) {
    int rt = getRunningTime(); 
    pCourse pCrs = getCourse(false);
    if (pCrs && rt > 0) {
      map<int, int>::iterator res = Class->tBestTimePerCourse.find(pCrs->getId());
      if (res == Class->tBestTimePerCourse.end()) {
        Class->tBestTimePerCourse[pCrs->getId()] = rt;
        updated = true;
      }
      else if (rt < res->second) {
        res->second = rt;
        updated = true;
      }
    }
  }
  return updated;
}

int oRunner::getRaceRunningTime(int leg) const
{
  if(tParentRunner)
    return tParentRunner->getRaceRunningTime(leg);

	if (leg==-1)
    leg=multiRunner.size()-1;
		
  if(leg==0) {
    if(getTotalStatus() == StatusOK)
      return getRunningTime() + inputTime;
    else return 0;
  }
  leg--;

  if (unsigned(leg) < multiRunner.size() && multiRunner[leg]) {
    if (Class) {
      pClass pc=Class;
      LegTypes lt=pc->getLegType(leg);
      pRunner r=multiRunner[leg];

      switch(lt) {
        case LTNormal:
          if(r->statusOK()) {            
            int dt=leg>0 ? r->getRaceRunningTime(leg)+r->getRunningTime():0;     
            return max(r->getFinishTime()-tStartTime, dt); // ### Luckor, jaktstart???
          }
          else return 0;
        break;

        case LTSum:
          if(r->statusOK())
            return r->getRunningTime()+getRaceRunningTime(leg);
          else return 0;
        
        default:
          return 0;
      }
    }
    else return getRunningTime();
	}
	return 0;
}

bool oRunner::sortSplit(const oRunner &a, const oRunner &b)
{
  int acid=a.getClassId();
  int bcid=b.getClassId();
  if(acid!=bcid)
	  return acid<bcid;
  else if(a.tempStatus != b.tempStatus)
	  return a.tempStatus<b.tempStatus;
  else {
	  if(a.tempStatus==StatusOK) {
		  if(a.tempRT!=b.tempRT)
			  return a.tempRT<b.tempRT;
	  }
	  return a.Name<b.Name;
  }		
}

bool oRunner::operator <(const oRunner &c)
{
  if(!Class || !c.Class) 
    return size_t(Class)<size_t(c.Class);
  else if (Class == c.Class && Class->getClassStatus() != oClass::Normal)
    return Name < c.Name;

	if(oe->CurrentSortOrder==ClassStartTime) {
    if(Class->Id != c.Class->Id)
			return Class->tSortIndex < c.Class->tSortIndex;
    else if (tStartTime != c.tStartTime) { 
      if (tStartTime <= 0 && c.tStartTime > 0)
        return false;
      else if (c.tStartTime <= 0 && tStartTime > 0)
        return true;
      else return tStartTime < c.tStartTime;
    }
    else {
      if (StartNo != c.StartNo && !(getBib().empty() && c.getBib().empty()))
        return StartNo < c.StartNo;
		  else 
        return Name < c.Name;
    }
	}
	else if(oe->CurrentSortOrder==ClassResult) {
		if(Class != c.Class)
			return Class->tSortIndex < c.Class->tSortIndex;
    else if (tDuplicateLeg != c.tDuplicateLeg)
      return tDuplicateLeg < c.tDuplicateLeg;
		else if(tStatus != c.tStatus)
			return unsigned(tStatus-1) < unsigned(c.tStatus-1);
		else {
			if(tStatus==StatusOK) {
        if (Class->getNoTiming())
          return Name<c.Name;

				int t=getRunningTime();
        if (t<=0)
          t = 3600*100;
				int ct=c.getRunningTime();
        if (ct<=0)
          ct = 3600*100;

				if(t!=ct)
					return t<ct;
			}			
			return Name<c.Name;
		}		
	}
  else if(oe->CurrentSortOrder == ClassCourseResult) {
		if(Class != c.Class)
			return Class->tSortIndex < c.Class->tSortIndex;

    const pCourse crs1 = getCourse(false);
    const pCourse crs2 = c.getCourse(false);
    if (crs1 != crs2) {
      int id1 = crs1 ? crs1->getId() : 0;
      int id2 = crs2 ? crs2->getId() : 0;
      return id1 < id2;
    }
    else if (tDuplicateLeg != c.tDuplicateLeg)
      return tDuplicateLeg < c.tDuplicateLeg;
		else if(tStatus != c.tStatus)
			return unsigned(tStatus-1) < unsigned(c.tStatus-1);
		else {
			if(tStatus==StatusOK) {
        if (Class->getNoTiming())
          return Name<c.Name;

				int t=getRunningTime();
				int ct=c.getRunningTime();
				if(t!=ct)
					return t<ct;
			}			
			return Name<c.Name;
		}		
	}
	else if(oe->CurrentSortOrder==SortByName)
		return Name<c.Name;
	else if(oe->CurrentSortOrder==SortByFinishTime){
		if(tStatus != c.tStatus)
			return unsigned(tStatus-1) < unsigned(c.tStatus-1);
		else{
			if(tStatus==StatusOK && FinishTime!=c.FinishTime)
				return FinishTime<c.FinishTime;
			else if(Name<c.Name)
				return true;
			else return false;
		}
	}
	else if(oe->CurrentSortOrder == ClassFinishTime){
    if(Class != c.Class)
			return Class->tSortIndex < c.Class->tSortIndex;
		if(tStatus != c.tStatus)
			return unsigned(tStatus-1) < unsigned(c.tStatus-1);
		else{
			if(tStatus==StatusOK && FinishTime!=c.FinishTime)
				return FinishTime<c.FinishTime;
			else if(Name<c.Name)
				return true;
			else return false;
		}
	}
	else if(oe->CurrentSortOrder==SortByStartTime){
		if(tStartTime < c.tStartTime)
			return true;
		else  if(tStartTime > c.tStartTime)
			return false;
		else if(Name<c.Name)
			return true;
		else return false;
	}
  else if(oe->CurrentSortOrder == ClassPoints){
		if(Class != c.Class)
			return Class->tSortIndex < c.Class->tSortIndex;
    else if (tDuplicateLeg != c.tDuplicateLeg)
      return tDuplicateLeg < c.tDuplicateLeg;
		else if(tStatus != c.tStatus)
			return unsigned(tStatus-1) < unsigned(c.tStatus-1);
		else {
			if (tStatus==StatusOK) {
        if(tRogainingPoints != c.tRogainingPoints)
			    return tRogainingPoints > c.tRogainingPoints;
				int t=getRunningTime();
				int ct=c.getRunningTime();
				if (t != ct)
					return t < ct;
			}			
			return Name  <c.Name;
		}
	}
  else if(oe->CurrentSortOrder==ClassTotalResult) {
		if(Class != c.Class)
			return Class->tSortIndex < c.Class->tSortIndex;
    else if (tDuplicateLeg != c.tDuplicateLeg)
      return tDuplicateLeg < c.tDuplicateLeg;
    else {
      RunnerStatus s1, s2;
      s1 = getTotalStatus();
      s2 = c.getTotalStatus();
      if(s1 != s2)
			  return s1 < s2;
		  else if(s1 == StatusOK) {
        if (Class->getNoTiming())
          return Name<c.Name;

        int t = getTotalRunningTime(FinishTime);
			  int ct = c.getTotalRunningTime(c.FinishTime);
			  if (t!=ct)
				  return t < ct;
      }
    } 
		return Name < c.Name;
	}
  else if(oe->CurrentSortOrder == CoursePoints){
    const pCourse crs1 = getCourse(false);
    const pCourse crs2 = c.getCourse(false);
    if (crs1 != crs2) {
      int id1 = crs1 ? crs1->getId() : 0;
      int id2 = crs2 ? crs2->getId() : 0;
      return id1 < id2;
    }
		if(status != c.status)
			return unsigned(status-1) < unsigned(c.status-1);
		else {
			if (status==StatusOK) {
        if(tRogainingPoints != c.tRogainingPoints)
			    return tRogainingPoints > c.tRogainingPoints;
				int t=getRunningTime();
				int ct=c.getRunningTime();
				if (t != ct)
					return t < ct;
			}			
			return Name  <c.Name;
		}
	}
  else if (oe->CurrentSortOrder == CourseResult) {
    const pCourse crs1 = getCourse(false);
    const pCourse crs2 = c.getCourse(false);
    if (crs1 != crs2) {
      int id1 = crs1 ? crs1->getId() : 0;
      int id2 = crs2 ? crs2->getId() : 0;
      return id1 < id2;
    }
		else if(tStatus != c.tStatus)
			return unsigned(tStatus-1) < unsigned(c.tStatus-1);
		else {
			if (tStatus==StatusOK) {
				int t=getRunningTime();
				int ct=c.getRunningTime();
        if (t != ct) {
					return t<ct;
        }
			}			
		}
    return Name<c.Name;
  }
  else if(oe->CurrentSortOrder==ClassStartTimeClub) {
    if(Class != c.Class)
			return Class->tSortIndex < c.Class->tSortIndex;
    else if (tStartTime != c.tStartTime) { 
      if (tStartTime <= 0 && c.tStartTime > 0)
        return false;
      else if (c.tStartTime <= 0 && tStartTime > 0)
        return true;
      else return tStartTime < c.tStartTime;
    }
    else if (Club != c.Club) {
      return getClub() < c.getClub();
    }
		else if(Name<c.Name)
			return true;
		else return false;
	}
  else if(oe->CurrentSortOrder==ClassTeamLeg) {
    if(Class->Id != c.Class->Id)
			return Class->tSortIndex < c.Class->tSortIndex;
    else if (tInTeam != c.tInTeam) {
      if (tInTeam == 0 || c.tInTeam == 0)
        return tInTeam < c.tInTeam;
      if (tInTeam->StartNo != c.tInTeam->StartNo)
        return tInTeam->StartNo < c.tInTeam->StartNo;
      else
        return tInTeam->Name < c.tInTeam->Name;
    }
    else if (tLeg != c.tLeg)
      return tLeg < c.tLeg;
    else if (tStartTime != c.tStartTime) { 
      if (tStartTime <= 0 && c.tStartTime > 0)
        return false;
      else if (c.tStartTime <= 0 && tStartTime > 0)
        return true;
      else return tStartTime < c.tStartTime;
    }
    else {
      if (StartNo != c.StartNo && !getBib().empty())
        return StartNo < c.StartNo;
		  else 
        return Name < c.Name;
    }
  }
  else throw std::exception("Bad sort order");
}

void oAbstractRunner::setClub(const string &Name)
{
	pClub pc=Club;
  Club = Name.empty() ? 0 : oe->getClubCreate(0, Name);
  if (pc != Club) {
    updateChanged();
    if (Class) {
      // Vacant clubs have special logic
      Class->tResultInfo.clear();
    }
  }
}

pClub oAbstractRunner::setClubId(int clubId)
{
	pClub pc=Club;
  Club = oe->getClub(clubId);
  if (pc != Club) {
    updateChanged();
    if (Class) {
      // Vacant clubs have special logic
      Class->tResultInfo.clear();
    }
  }
  return Club;
}

void oRunner::setClub(const string &Name)
{
  if (tParentRunner)
    tParentRunner->setClub(Name);
  else { 
    oAbstractRunner::setClub(Name);

    for (size_t k=0;k<multiRunner.size();k++) 
      if (multiRunner[k] && multiRunner[k]->Club!=Club) {
		    multiRunner[k]->Club = Club;
		    multiRunner[k]->updateChanged();
      }  
	}
}

pClub oRunner::setClubId(int clubId)
{
  if (tParentRunner)
    tParentRunner->setClubId(clubId);
  else { 
    oAbstractRunner::setClubId(clubId);

    for (size_t k=0;k<multiRunner.size();k++) 
      if (multiRunner[k] && multiRunner[k]->Club!=Club) {
		    multiRunner[k]->Club = Club;
		    multiRunner[k]->updateChanged();
      }  
	}
  return Club;
}


void oAbstractRunner::setStartNo(int no, bool tmpOnly) {
  if (tmpOnly) {
    tmpStore.startNo = no;
    return;
  }

	if(no!=StartNo) {
    if (oe)
      oe->bibStartNoToRunnerTeam.clear();
		StartNo=no;
		updateChanged();
	}
}

void oRunner::setStartNo(int no, bool tmpOnly)
{
  if (tParentRunner)
    tParentRunner->setStartNo(no, tmpOnly);
  else { 
    oAbstractRunner::setStartNo(no, tmpOnly);

    for (size_t k=0;k<multiRunner.size();k++) 
      if(multiRunner[k])
        multiRunner[k]->oAbstractRunner::setStartNo(no, tmpOnly);
	}
}

int oRunner::getPlace() const
{
	return tPlace;
}

int oRunner::getCoursePlace() const
{
	return tCoursePlace;
}


int oRunner::getTotalPlace() const
{
  if (tInTeam)
    return tInTeam->getLegPlace(tLeg, true);
  else
	  return tTotalPlace;
}

string oAbstractRunner::getPlaceS() const
{
	char bf[16];
  int p=getPlace();
	if(p>0 && p<10000){
		_itoa_s(p, bf, 16, 10);
		return bf;
	}
	else return _EmptyString;
}

string oAbstractRunner::getPrintPlaceS() const
{
	char bf[16];
  int p=getPlace();
	if(p>0 && p<10000){
		_itoa_s(p, bf, 16, 10);
		return string(bf)+".";
	}
  else return _EmptyString;
}

string oAbstractRunner::getTotalPlaceS() const
{
	char bf[16];
  int p=getTotalPlace();
	if(p>0 && p<10000){
		_itoa_s(p, bf, 16, 10);
		return bf;
	}
	else return _EmptyString;
}

string oAbstractRunner::getPrintTotalPlaceS() const
{
	char bf[16];
  int p=getTotalPlace();
	if(p>0 && p<10000){
		_itoa_s(p, bf, 16, 10);
		return string(bf)+".";
	}
  else return _EmptyString;
}

/// Split a name into first name and last name
int getNameSplitPoint(const string &name) {
  int split[10];
  int nSplit = 0;

  for (unsigned k = 1; k + 1<name.size(); k++) {
    if (name[k] == ' ' || name[k] == 0xA0) {
      split[nSplit++] = k;
      if ( nSplit>=9 )
        break;
    }
  }

  if (nSplit == 1)
    return split[0] + 1;
  else if (nSplit == 0)
    return -1;
  else {
    const oWordList &givenDB = lang.get().getGivenNames();
    int sp_ix = 0;
    for (int k = 1; k<nSplit; k++) {
      string sn = name.substr(split[k-1]+1, split[k] - split[k-1]-1);
      if (!givenDB.lookup(sn.c_str()))
        break;
      sp_ix = k;
    }
    return split[sp_ix]+1;
  }
}

string oRunner::getGivenName() const
{
  int sp = getNameSplitPoint(Name);
  if (sp == -1)
    return trim(Name);
  else
    return trim(Name.substr(0, sp)); 

	//return trim(Name.substr(0, Name.find_first_of(" \xA0")));
}

string oRunner::getFamilyName() const
{
//  int k = Name.find_first_of(" \xA0");
  int sp = getNameSplitPoint(Name);
  if (sp == -1)
    return _EmptyString;
  else
    return trim(Name.substr(sp)); 
/*
  if(k==string::npos)
    return "";
  else return trim(Name.substr(k, string::npos));*/
}

void oRunner::setCardNo(int cno, bool matchCard, bool updateFromDatabase)
{
	if(cno!=CardNo){
    int oldNo = CardNo;
		CardNo=cno;

		if (static_cast<oExtendedEvent*>(oe)->isRentedCard(cno))
			getDI().setInt("CardFee", oe->getDI().getInt("CardFee"));

    oFreePunch::rehashPunches(*oe, oldNo, 0);
    oFreePunch::rehashPunches(*oe, CardNo, 0);

    if(matchCard && !Card) {
      pCard c=oe->getCardByNumber(cno);

      if(c && !c->tOwner) {
        vector<int> mp;
        addPunches(c, mp);
      }
    }

    if (!updateFromDatabase)

		updateChanged();
	}
}

int oRunner::setCard(int cardId)
{
  pCard c=cardId ? oe->getCard(cardId) : 0;
  int oldId=0;

  if (Card!=c) {
    if (Card) { 
      oldId=Card->getId();
      Card->tOwner=0;
    }
    if (c) {
      if(c->tOwner) {
        pRunner otherR=c->tOwner;        
        assert(otherR!=this);
        otherR->Card=0;
        otherR->updateChanged();
        otherR->setStatus(StatusUnknown, true, false);
        otherR->synchronize(true);
      }
      c->tOwner=this;
      CardNo=c->CardNo;
    }
    Card=c;
    vector<int> mp;
    evaluateCard(true, mp);
		updateChanged();
    synchronize(true);
	}
  return oldId;
}

void oAbstractRunner::setName(const string &n)
{
  if (trim(n).empty())
    throw std::exception("Tomt namn �r inte till�tet.");
	if(n!=Name){
		Name=trim(n);
		updateChanged();
	}
}

void oRunner::setName(const string &n)
{
  if (trim(n).empty())
    throw std::exception("Tomt namn �r inte till�tet.");

  if (tParentRunner)
    tParentRunner->setName(n);
  else { 
    string oldName = Name;
    if (n!=Name) {
		  Name=trim(n);
      for (size_t k = 0; k<Name.length(); k++) {
        if (BYTE(Name[k]) == BYTE(160))
          Name[k] = ' ';
      }
		  updateChanged();
    }

    for (size_t k=0;k<multiRunner.size();k++) {
      if (multiRunner[k] && n!=multiRunner[k]->Name) {
		    multiRunner[k]->Name=trim(n);
		    multiRunner[k]->updateChanged();
      }
    }
    if (tInTeam && Class && Class->isSingleRunnerMultiStage()) {
      if (tInTeam->Name == oldName)     
        tInTeam->setName(Name);
    }
	}
}

bool oAbstractRunner::setStatus(RunnerStatus st, bool updateSource, bool tmpOnly, bool recalculate) {
  assert(!(updateSource && tmpOnly));
  if (tmpOnly) {
    tmpStore.status = st;
    return false;
  }
  bool ch = false;
	if(tStatus!=st) {
    ch = true;
		tStatus=st;

    if (Class) {
      Class->clearCache(recalculate);
    }
	}

  if (st != status) {
    status = st;
    if (updateSource)
      updateChanged();
    else
      changedObject();
  }

  return ch;
}

int oAbstractRunner::getPrelRunningTime() const
{
  if(FinishTime>0 && tStatus!=StatusDNS && tStatus!=StatusDNF && tStatus!=StatusNotCompetiting) 
		return getRunningTime();
  else if(tStatus==StatusUnknown)
    return oe->getComputerTime()-tStartTime;
	else return 0;
}

string oAbstractRunner::getPrelRunningTimeS() const
{
	int rt=getPrelRunningTime();
	return formatTime(rt);
}

oDataContainer &oRunner::getDataBuffers(pvoid &data, pvoid &olddata, pvectorstr &strData) const {
  data = (pvoid)oData;
  olddata = (pvoid)oDataOld;
  strData = 0;
  return *oe->oRunnerData;
}

void oEvent::getRunners(int classId, int courseId, vector<pRunner> &r, bool sort) {
  if (sort) {
    synchronizeList(oLRunnerId);
    sortRunners(SortByName);
  }
  r.clear();
  if (Classes.size() > 0)
    r.reserve((Runners.size()*min<size_t>(Classes.size(), 4)) / Classes.size());

  for (oRunnerList::iterator it = Runners.begin(); it != Runners.end(); ++it) {
    if (it->isRemoved())
      continue;
    if (courseId != 0) {
      pCourse pc = it->getCourse(false);
      if (pc == 0 || pc->getId() != courseId)
        continue;
    }

    if (classId == 0 || it->getClassId() == classId)
      r.push_back(&*it);
  }
}

void oEvent::getRunnersByCard(int cardNo, vector<pRunner> &r) {
  synchronizeList(oLRunnerId);
  sortRunners(SortByName);
  r.clear();

  for (oRunnerList::iterator it = Runners.begin(); it != Runners.end(); ++it) {
    if (it->getCardNo() == cardNo)
      r.push_back(&*it);
  }
}


pRunner oEvent::getRunner(int Id, int stage) const
{
  pRunner value;

  if (runnerById.lookup(Id, value) && value) {
    if (value->isRemoved())
      return 0;
    assert(value->Id == Id);
    if (stage==0)
      return value;
    else if (unsigned(stage)<=value->multiRunner.size())
      return value->multiRunner[stage-1];
  }
	return 0;
}

pRunner oRunner::nextNeedReadout() const {
  if (tInTeam) {
    // For a runner in a team, first the team for the card
    for (size_t k = 0; k < tInTeam->Runners.size(); k++) {
      pRunner tr = tInTeam->Runners[k];
      if (tr && tr->CardNo == CardNo && !tr->Card && !tr->statusOK())
        return tr;
    }
  }
  
  if(!Card || Card->CardNo!=CardNo || Card->isConstructedFromPunches()) //-1 means card constructed from punches
    return pRunner(this);
  
  for (size_t k=0;k<multiRunner.size();k++) {
    if (multiRunner[k] && (!multiRunner[k]->Card || 
           multiRunner[k]->Card->CardNo!=CardNo))
      return multiRunner[k];
  }
  return 0;
}

void oEvent::setupCardHash(bool clear) {
  if (clear) {
    cardHash.clear();
  }
  else {
    assert(cardHash.empty());

    cardHash.resize(Runners.size());
    for (oRunnerList::iterator it = Runners.begin(); it != Runners.end(); ++it) {
      if (it->skip())
        continue;
      pRunner r = it->nextNeedReadout();
      if (r) {
        cardHash.insert(r->CardNo, r);
      }
		}

    pRunner r;
  	for (oRunnerList::iterator it=Runners.begin(); it != Runners.end(); ++it) {
      if(!it->isRemoved() && it->CardNo>0 && cardHash.lookup(it->CardNo, r) == false) {	
        r = it->nextNeedReadout();
        int cno = it->CardNo;
        r = r ? r : pRunner(&*it);
        cardHash.insert(cno, r);
      }
		}
  }
}

pRunner oEvent::getRunnerByCard(int CardNo, bool OnlyWithNoCard, bool ignoreRunnersWithNoStart) const
{
	oRunnerList::const_iterator it;	

  if (!OnlyWithNoCard && !cardHash.empty()) {
    pRunner r;
    if(cardHash.lookup(CardNo, r) && r) {
      assert(r->getCardNo() == CardNo);
      return r;
    } 
    else
      return 0;
  }
	//First try runners with no card read or a different card read.
	for (it=Runners.begin(); it != Runners.end(); ++it) {
    if (it->skip())
      continue;
    if (ignoreRunnersWithNoStart && it->getStatus() == StatusDNS)
      continue;
    if (it->getStatus() == StatusNotCompetiting)
      continue;

    pRunner ret;
		if(it->CardNo==CardNo && (ret = it->nextNeedReadout()) != 0)	
      return ret;
	}

	if (!OnlyWithNoCard) 	{
		//Then try all runners.
		for (it=Runners.begin(); it != Runners.end(); ++it){
      if (ignoreRunnersWithNoStart && it->getStatus() == StatusDNS)
        continue;
      if (it->getStatus() == StatusNotCompetiting)
        continue;
      if(!it->isRemoved() && it->CardNo==CardNo) {	
        pRunner r = it->nextNeedReadout();
        return r ? r : pRunner(&*it);
      }
		}
	}

	return 0;
}

int oRunner::getRaceIdentifier() const {
  if (tParentRunner)
    return tParentRunner->getRaceIdentifier();// A unique person has a unique race identifier, even if the race is "split" into several
  
  int stored = getDCI().getInt("RaceId");
  if (stored != 0)
    return stored;

  if (!tInTeam)
    return 1000000 + Id * 2;//Even
  else
    return 1000000 * (tLeg+1) + tInTeam->Id * 2 + 1;//Odd
}

static int getEncodedBib(const string &bib) {
  int enc = 0;
  for (size_t j = 0; j < bib.length(); j++) {
    int x = toupper(bib[j])-32;
    if (x<0)
      return 0; // Not a valid bib
    enc = enc * 97 - x;
  }
  return enc;
}

int oAbstractRunner::getEncodedBib() const {
  return ::getEncodedBib(getBib());
}


typedef multimap<int, oAbstractRunner*>::iterator BSRTIterator;

pRunner oEvent::getRunnerByBibOrStartNo(const string &bib, bool findWithoutCardNo) const {
  if (bib.empty() || bib == "0")
    return 0;

  if (bibStartNoToRunnerTeam.empty()) {
    for (oTeamList::const_iterator tit = Teams.begin(); tit != Teams.end(); ++tit) {
      const oTeam &t=*tit;
      if (t.skip())
        continue;
      
      int sno = t.getStartNo();
      if (sno != 0)
        bibStartNoToRunnerTeam.insert(make_pair(sno, (oAbstractRunner *)&t));
      int enc = t.getEncodedBib();
      if (enc != 0)
        bibStartNoToRunnerTeam.insert(make_pair(enc, (oAbstractRunner *)&t));
    }

	  for (oRunnerList::const_iterator it=Runners.begin(); it != Runners.end(); ++it) {
      if (it->skip())
        continue;
       const oRunner &t=*it;
     
      int sno = t.getStartNo();
      if (sno != 0)
        bibStartNoToRunnerTeam.insert(make_pair(sno, (oAbstractRunner *)&t));
      int enc = t.getEncodedBib();
      if (enc != 0)
        bibStartNoToRunnerTeam.insert(make_pair(enc, (oAbstractRunner *)&t));
    }
  }

  int sno = atoi(bib.c_str());

  pair<BSRTIterator, BSRTIterator> res;
  if (sno > 0) {
    // Require that a bib starts with numbers
    int bibenc = getEncodedBib(bib);
    res = bibStartNoToRunnerTeam.equal_range(bibenc);
    if (res.first == res.second)
      res = bibStartNoToRunnerTeam.equal_range(sno); // Try startno instead

    for(BSRTIterator it = res.first; it != res.second; ++it) {
      oAbstractRunner *pa = it->second;
      if (pa->isRemoved())
        continue;

      if (typeid(*pa)==typeid(oRunner)) {
	      oRunner &r = dynamic_cast<oRunner &>(*pa);
        if (r.getStartNo()==sno || stringMatch(r.getBib(), bib)) {
          if (findWithoutCardNo) {
            if (r.getCardNo() == 0 && r.needNoCard() == false)
              return &r;
          }
          else {
            if(r.getNumMulti()==0 || r.tStatus == StatusUnknown)
              return &r;
            else {
              for(int race = 0; race < r.getNumMulti(); race++) {
                pRunner r2 = r.getMultiRunner(race);
                if (r2 && r2->tStatus == StatusUnknown)
                  return r2;
              }
              return &r;
            }
          }
        }        
      }
      else {
        oTeam &t = dynamic_cast<oTeam &>(*pa);
        if (t.getStartNo()==sno || stringMatch(t.getBib(), bib)) {
          if (!findWithoutCardNo) {
            for (int leg=0; leg<t.getNumRunners(); leg++) {
              if(t.Runners[leg] && t.Runners[leg]->getCardNo() > 0 && t.Runners[leg]->getStatus()==StatusUnknown)
                return t.Runners[leg];
            }
          }
          else {
            for (int leg=0; leg<t.getNumRunners(); leg++) {
              if(t.Runners[leg] && t.Runners[leg]->getCardNo() == 0 && t.Runners[leg]->needNoCard() == false)
                return t.Runners[leg];
            }
          }
        }
      }
    }
  }
	return 0;
}

pRunner oEvent::getRunnerByName(const string &pname, const string &pclub) const
{
	oRunnerList::const_iterator it;	
  vector<pRunner> cnd;

  for (it=Runners.begin(); it != Runners.end(); ++it) {
		if (!it->skip() && it->Name==pname) {
      if(pclub.empty() || pclub==it->getClub())
        cnd.push_back(pRunner(&*it));
		}
	}
	
  if (cnd.size() == 1)
    return cnd[0]; // Only return if uniquely defined.

	return 0;
}

void oEvent::fillRunners(gdioutput &gdi, const string &id, bool longName, int filter)
{
  vector< pair<string, size_t> > d;
  oe->fillRunners(d, longName, filter, stdext::hash_set<int>());
  gdi.addItem(id, d);
}

const vector< pair<string, size_t> > &oEvent::fillRunners(vector< pair<string, size_t> > &out, 
                                                          bool longName, int filter, 
                                                          const stdext::hash_set<int> &personFilter) 
{	
  const bool showAll = (filter & RunnerFilterShowAll) == RunnerFilterShowAll;
  const bool noResult = (filter & RunnerFilterOnlyNoResult) ==  RunnerFilterOnlyNoResult;
	const bool withResult = (filter & RunnerFilterWithResult) ==  RunnerFilterWithResult;
	const bool compact = (filter & RunnerCompactMode) == RunnerCompactMode;

  synchronizeList(oLRunnerId);
	oRunnerList::iterator it;	
  int lVacId = getVacantClub();
	CurrentSortOrder=SortByName;
	Runners.sort();
  out.clear();
  if (personFilter.empty())
    out.reserve(Runners.size());
  else
    out.reserve(personFilter.size());

  char bf[512];
  const bool usePersonFilter = !personFilter.empty();

  if(longName) {
    for (it=Runners.begin(); it != Runners.end(); ++it) {
      if (noResult && (it->Card || it->FinishTime>0))
        continue;
      if (withResult && !it->Card && it->FinishTime == 0)
        continue;
      if (usePersonFilter && personFilter.count(it->Id) == 0)
        continue;
      if(!it->skip() || (showAll && !it->isRemoved())) {
        if (compact) {
          sprintf_s(bf, "%s, %s (%s)", it->getNameAndRace().c_str(),
                                      it->getClub().c_str(),
                                      it->getClass().c_str());
                               
        } else {
          sprintf_s(bf, "%s\t%s\t%s", it->getNameAndRace().c_str(),
                                      it->getClass().c_str(),
                                      it->getClub().c_str());
        }
        out.push_back(make_pair(bf, it->Id));
      }
	  }
  }
  else {
	  for (it=Runners.begin(); it != Runners.end(); ++it) {
      if (noResult && (it->Card || it->FinishTime>0))
        continue;
      if (withResult && !it->Card && it->FinishTime == 0)
        continue;
      if (usePersonFilter && personFilter.count(it->Id) == 0)
        continue;

      if(!it->skip() || (showAll && !it->isRemoved())) {
        if( it->getClubId() != lVacId )
          out.push_back(make_pair(it->Name, it->Id));
        else {
          sprintf_s(bf, "%s (%s)", it->Name.c_str(), it->getClass().c_str());
          out.push_back(make_pair(bf, it->Id));
        }
      }
	  }
  }
	return out;
}

void oRunner::resetPersonalData()
{
  oDataInterface di = getDI();
  di.setInt("BirthYear", 0);
  di.setString("Nationality", "");
  di.setString("Country", "");
  di.setInt64("ExtId", 0);
	//getDI().initData();
}

string oRunner::getNameAndRace() const
{
  if (tDuplicateLeg>0 || multiRunner.size()>0) {
    char bf[16];
    sprintf_s(bf, " (%d)", getRaceNo()+1);
    return Name+bf;
  } 
  else return Name;
}

pRunner oRunner::getMultiRunner(int race) const
{
  if (race==0) {
    if(!tParentRunner)
      return pRunner(this);
    else return tParentRunner;
  }
  
  const vector<pRunner> &mr = tParentRunner ? tParentRunner->multiRunner : multiRunner;

  if (unsigned(race-1)>=mr.size()) {
    assert(tParentRunner);
    return 0;
  }

  return mr[race-1];
}

void oRunner::createMultiRunner(bool createMaster, bool sync)
{
  if(tDuplicateLeg)
    return; //Never allow chains.

  if (multiRunnerId.size()>0) {
    multiRunner.resize(multiRunnerId.size() - 1);
    for (size_t k=0;k<multiRunner.size();k++) {
      multiRunner[k]=oe->getRunner(multiRunnerId[k], 0);
      if (multiRunner[k]) {
        if (multiRunner[k]->multiRunnerId.size() > 1 || !multiRunner[k]->multiRunner.empty())
          multiRunner[k]->markForCorrection();

        multiRunner[k]->multiRunner.clear(); //Do not allow chains
        multiRunner[k]->multiRunnerId.clear();
        multiRunner[k]->tDuplicateLeg = k+1;
        multiRunner[k]->tParentRunner = this;
        multiRunner[k]->CardNo=0;

        if (multiRunner[k]->Id != multiRunnerId[k])
          markForCorrection();
      }
      else if (multiRunnerId[k]>0)
        markForCorrection();

      assert(multiRunner[k]);
    }
    multiRunnerId.clear();
  }

  if (!Class || !createMaster)
    return;

  int ndup=0;

  if (!tInTeam) 
    ndup=Class->getNumMultiRunners(0);
  else
    ndup=Class->getNumMultiRunners(tLeg);
  
  bool update = false;

  vector<int> toRemove;

  for (size_t k = ndup-1; k<multiRunner.size();k++) {
    if (multiRunner[k] && multiRunner[k]->getStatus()==StatusUnknown) {
      toRemove.push_back(multiRunner[k]->getId());
      multiRunner[k]->tParentRunner = 0;
      if (multiRunner[k]->tInTeam && size_t(multiRunner[k]->tLeg)<multiRunner[k]->tInTeam->Runners.size()) {
        if (multiRunner[k]->tInTeam->Runners[multiRunner[k]->tLeg]==multiRunner[k])
          multiRunner[k]->tInTeam->Runners[multiRunner[k]->tLeg] = 0;
      }
    }
  }

  multiRunner.resize(ndup-1);
  for(int k=1;k<ndup; k++) {
    if (!multiRunner[k-1]) {
      update = true;
      multiRunner[k-1]=oe->addRunner(Name, getClubId(), 
                                     getClassId(), 0, 0, false);
      multiRunner[k-1]->tDuplicateLeg=k;
      multiRunner[k-1]->tParentRunner=this;

      if (sync)
        multiRunner[k-1]->synchronize();
    }
  }
  if (update) 
    updateChanged();

  if (sync) {
    synchronize(true);
    oe->removeRunner(toRemove); 
    }
}

pRunner oRunner::getPredecessor() const
{
  if(!tParentRunner || unsigned(tDuplicateLeg-1)>=16)
    return 0;
  
  if(tDuplicateLeg==1)
    return tParentRunner;
  else
    return tParentRunner->multiRunner[tDuplicateLeg-2];
}



bool oRunner::apply(bool sync, pRunner src, bool setTmpOnly) {
  createMultiRunner(false, sync);
	tLeg=-1;
  tUseStartPunch=true;
	if(tInTeam)
		tInTeam->apply(sync, this, setTmpOnly);
  else {
    if (Class && Class->hasMultiCourse()) {
      pClass pc=Class;
      StartTypes st=pc->getStartType(tDuplicateLeg);
    
      if (st==STTime) {
        setStartTime(pc->getStartData(tDuplicateLeg), false, setTmpOnly);
        tUseStartPunch = false;
      }
      else if (st==STChange) {
        pRunner r=getPredecessor();
        int lastStart=0;
        if(r && r->FinishTime>0) 
          lastStart = r->FinishTime;

        int restart=pc->getRestartTime(tDuplicateLeg);
        int rope=pc->getRopeTime(tDuplicateLeg);

        if (restart && rope && (lastStart>rope || lastStart==0))
          lastStart=restart; //Runner in restart

        setStartTime(lastStart, false, setTmpOnly);
        tUseStartPunch=false; 
      }
      else if (st==STHunting) {
        pRunner r=getPredecessor();
        int lastStart=0;

        if (r && r->FinishTime>0 && r->statusOK()) {
          int rt=r->getRaceRunningTime(tDuplicateLeg-1);
          int timeAfter=rt-pc->getTotalLegLeaderTime(r->tDuplicateLeg, true);
          if(rt>0 && timeAfter>=0)
            lastStart=pc->getStartData(tDuplicateLeg)+timeAfter;
        }
        int restart=pc->getRestartTime(tDuplicateLeg);
        int rope=pc->getRopeTime(tDuplicateLeg);

        if (restart && rope && (lastStart>rope || lastStart==0))
          lastStart=restart; //Runner in restart

        setStartTime(lastStart, false, setTmpOnly);
        tUseStartPunch=false;  
      }
    }
  }

	if (tLeg==-1) {
		tLeg=0;
		tInTeam=0;
    return false;
	}
  else return true;
}

void oRunner::cloneStartTime(const pRunner r) {
  if (tParentRunner)
    tParentRunner->cloneStartTime(r);
  else { 
    setStartTime(r->getStartTime(), true, false);

    for (size_t k=0; k < min(multiRunner.size(), r->multiRunner.size()); k++) {
      if (multiRunner[k]!=0 && r->multiRunner[k]!=0) 
        multiRunner[k]->setStartTime(r->multiRunner[k]->getStartTime(), true, false);
    }
    apply(false, 0, false);
  }    
}

void oRunner::cloneData(const pRunner r) {
  if (tParentRunner)
    tParentRunner->cloneData(r);
  else {
    size_t t = sizeof(oData);
    memcpy(oData, r->oData, t);
  }
}


Table *oEvent::getRunnersTB()//Table mode
{	
  if (tables.count("runner") == 0) {
	  Table *table=new Table(this, 20, "Deltagare", "runners");
	  
    table->addColumn("Id", 70, true, true);
    table->addColumn("�ndrad", 70, false);

	  table->addColumn("Namn", 200, false);
	  table->addColumn("Klass", 120, false);
    table->addColumn("Bana", 120, false);
	
	  table->addColumn("Klubb", 120, false);
    table->addColumn("Lag", 120, false);
    table->addColumn("Str�cka", 70, true);
  
    table->addColumn("SI", 90, true, false);

    table->addColumn("Start", 70, false, true);
    table->addColumn("M�l", 70, false, true);
    table->addColumn("Status", 70, false);
    table->addColumn("Tid", 70, false, true);
    table->addColumn("Plac.", 70, true, true);
    table->addColumn("Start nr.", 70, true, false);

    oe->oRunnerData->buildTableCol(table);

    table->addColumn("Tid in", 70, false, true);
    table->addColumn("Status in", 70, false, true);
    table->addColumn("Po�ng in", 70, true);
    table->addColumn("Placering in", 70, true);

    tables["runner"] = table;
    table->addOwnership();
  }
  tables["runner"]->update();
  return tables["runner"];
}

void oEvent::generateRunnerTableData(Table &table, oRunner *addRunner)
{ 
  if (addRunner) {
    addRunner->addTableRow(table);
    return;
  }

  synchronizeList(oLRunnerId);
	oRunnerList::iterator it;	
  table.reserve(Runners.size());
  for (it=Runners.begin(); it != Runners.end(); ++it){		
		if(!it->skip()){
      it->addTableRow(table);
		}
	}
}

void oRunner::addTableRow(Table &table) const
{
  oRunner &it = *pRunner(this);
  table.addRow(getId(), &it);	

  int row = 0;
  table.set(row++, it, TID_ID, itos(getId()), false);
  table.set(row++, it, TID_MODIFIED, getTimeStamp(), false);			

  table.set(row++, it, TID_RUNNER, getName(), true);
  table.set(row++, it, TID_CLASSNAME, getClass(), true, cellSelection);			
  table.set(row++, it, TID_COURSE, getCourseName(), true, cellSelection);			
  table.set(row++, it, TID_CLUB, getClub(), true, cellCombo);

  table.set(row++, it, TID_TEAM, tInTeam ? tInTeam->getName() : "", false);
  table.set(row++, it, TID_LEG, tInTeam ? itos(tLeg+1) : "" , false);

  int cno = getCardNo();
  table.set(row++, it, TID_CARD, cno>0 ? itos(cno) : "", true);

  table.set(row++, it, TID_START, getStartTimeS(), true);
  table.set(row++, it, TID_FINISH, getFinishTimeS(), true);
  table.set(row++, it, TID_STATUS, getStatusS(), true, cellSelection);
  table.set(row++, it, TID_RUNNINGTIME, getRunningTimeS(), false);

  table.set(row++, it, TID_PLACE, getPlaceS(), false);
  table.set(row++, it, TID_STARTNO, itos(getStartNo()), true);
  
  row = oe->oRunnerData->fillTableCol(it, table, true);

  table.set(row++, it, TID_INPUTTIME, getInputTimeS(), true);
  table.set(row++, it, TID_INPUTSTATUS, getInputStatusS(), true, cellSelection);
  table.set(row++, it, TID_INPUTPOINTS, itos(inputPoints), true);
  table.set(row++, it, TID_INPUTPLACE, itos(inputPlace), true);
}

bool oRunner::inputData(int id, const string &input, 
                        int inputId, string &output, bool noUpdate)
{
  int t,s;
  vector<int> mp;
  synchronize(false);
 
  if(id>1000) {
    return oe->oRunnerData->inputData(this, id, input, 
                                        inputId, output, noUpdate);
  }
  switch(id) {
    case TID_CARD:
      setCardNo(atoi(input.c_str()), true);
      synchronizeAll();
      output = itos(getCardNo());
      return true;
    case TID_RUNNER:
      if (trim(input).empty())
        throw std::exception("Tomt namn inte till�tet.");

      if (Name != input) {
        updateFromDB(input, getClubId(), getClassId(), getCardNo(), getBirthYear());
        setName(input);
        synchronizeAll();
      }
      output=getName();
      return true;
    break;

    case TID_START:
      setStartTimeS(input);
      t=getStartTime();      
      evaluateCard(true, mp);
      s=getStartTime();
      if (s!=t) 
        throw std::exception("Starttiden �r definerad genom klassen eller l�parens startst�mpling.");
      synchronize(true);
      output=getStartTimeS();
      return true;
    break;

    case TID_FINISH:
      setFinishTimeS(input);
      t=getFinishTime();      
      evaluateCard(true, mp);
      s=getFinishTime();
      if (s!=t) 
        throw std::exception("F�r att �ndra m�ltiden m�ste l�parens m�lst�mplingstid �ndras.");
      synchronize(true);
      output=getStartTimeS();
      return true;
    break;

    case TID_COURSE:
      setCourseId(inputId);
      synchronize(true);
      output = getCourseName();
      break;

    case TID_CLUB: 
      {
        pClub pc = 0;
        if (inputId > 0)
          pc = oe->getClub(inputId);
        else
          pc = oe->getClubCreate(0, input);

        updateFromDB(getName(), pc ? pc->getId():0, getClassId(), getCardNo(), getBirthYear());

        setClub(pc ? pc->getName() : "");
        synchronize(true);
        output = getClub();
      }
      break;

    case TID_CLASSNAME:
      setClassId(inputId);
      synchronize(true);
      output = getClass();
      break;

    case TID_STATUS: {
      setStatus(RunnerStatus(inputId), true, false);
      int s = getStatus();
      evaluateCard(true, mp);
      if (s!=getStatus())
        throw std::exception("Status matchar inte data i l�parbrickan.");
      synchronize(true);
      output = getStatusS();
    }
    break;

    case TID_STARTNO:
      setStartNo(atoi(input.c_str()), false);
      synchronize(true);
      output = itos(getStartNo());
      break;

    case TID_INPUTSTATUS:
      setInputStatus(RunnerStatus(inputId));
      synchronize(true);
      output = getInputStatusS();
      break;

    case TID_INPUTTIME:
      setInputTime(input);
      synchronize(true);
      output = getInputTimeS();
      break;

    case TID_INPUTPOINTS:
      setInputPoints(atoi(input.c_str()));
      synchronize(true);
      output = itos(getInputPoints());
      break;

    case TID_INPUTPLACE:
      setInputPlace(atoi(input.c_str()));
      synchronize(true);
      output = itos(getInputPlace());
      break;
  }

  return false;
}

void oRunner::fillInput(int id, vector< pair<string, size_t> > &out, size_t &selected)
{ 
  if(id>1000) {
    oe->oRunnerData->fillInput(oData, id, 0, out, selected);
    return;
  }

  if (id==TID_COURSE) {
    oe->fillCourses(out, true);
    out.push_back(make_pair(lang.tl("Klassens bana"), 0));
    selected = getCourseId();
  }
  else if (id==TID_CLASSNAME) {
    oe->fillClasses(out, oEvent::extraNone, oEvent::filterNone);
    out.push_back(make_pair(lang.tl("Ingen klass"), 0));
    selected = getClassId();
  }  
  else if (id==TID_CLUB) {
    oe->fillClubs(out);
    out.push_back(make_pair(lang.tl("Klubbl�s"), 0));
    selected = getClubId();
  }
  else if (id==TID_STATUS) {
    oe->fillStatus(out);
    selected = getStatus();
  }
  else if (id==TID_INPUTSTATUS) {
    oe->fillStatus(out);
    selected = inputStatus;
  }
}

int oRunner::getSplitTime(int controlNumber, bool normalized) const
{
  const vector<SplitData> &st = getSplitTimes(normalized);
  if (controlNumber>0 && controlNumber == st.size() && FinishTime>0) {
    int t = st.back().time;
    if (t >0)      
      return max(FinishTime - t, -1);
  }
  else if ( unsigned(controlNumber)<st.size() ) {
    if(controlNumber==0)
      return (tStartTime>0 && st[0].time>0) ? max(st[0].time-tStartTime, -1) : -1;
    else if (st[controlNumber].time>0 && st[controlNumber-1].time>0)
      return max(st[controlNumber].time - st[controlNumber-1].time, -1);
    else return -1;
  }
  return -1;
}

int oRunner::getTimeAdjust(int controlNumber) const
{
  if ( unsigned(controlNumber)<adjustTimes.size() ) {
    return adjustTimes[controlNumber];
  }
  return 0;
}

int oRunner::getNamedSplit(int controlNumber) const
{
  pCourse crs=getCourse(true);
  if(!crs || unsigned(controlNumber)>=unsigned(crs->nControls)
        || unsigned(controlNumber)>=splitTimes.size())
    return -1;

  pControl ctrl=crs->Controls[controlNumber];
  if(!ctrl || !ctrl->hasName()) 
    return -1;

  int k=controlNumber-1;

  //Measure from previous named control
  while(k>=0) {
    pControl c=crs->Controls[k];
  
    if(c && c->hasName()) {
      if (splitTimes[controlNumber].time>0 && splitTimes[k].time>0)
        return max(splitTimes[controlNumber].time - splitTimes[k].time, -1);
      else return -1;
    }
    k--;
  }

  //Measure from start time
  if(splitTimes[controlNumber].time>0) 
    return max(splitTimes[controlNumber].time - tStartTime, -1);

  return -1;
}

string oRunner::getSplitTimeS(int controlNumber, bool normalized) const
{
  return formatTime(getSplitTime(controlNumber, normalized));
}

string oRunner::getNamedSplitS(int controlNumber) const
{
  return formatTime(getNamedSplit(controlNumber));
}

int oRunner::getPunchTime(int controlNumber, bool normalized) const
{
  const vector<SplitData> &st = getSplitTimes(normalized);

  if ( unsigned(controlNumber)<st.size() ) {
    if(st[controlNumber].time>0)
      return st[controlNumber].time-tStartTime;
    else return -1;
  }
  else if ( unsigned(controlNumber)==st.size() )
    return FinishTime-tStartTime;

  return -1;
}

string oRunner::getPunchTimeS(int controlNumber, bool normalized) const
{
  return formatTime(getPunchTime(controlNumber, normalized));
}

bool oAbstractRunner::isVacant() const
{
  return getClubId()==oe->getVacantClubIfExist();
}

bool oRunner::needNoCard() const {
  const_cast<oRunner*>(this)->apply(false, 0, false);
  return tNeedNoCard;
}

void oRunner::getSplitTime(int controlId, RunnerStatus &stat, int &rt) const
{
  rt = 0;
  stat = StatusUnknown;
  int cardno = tParentRunner ? tParentRunner->CardNo : CardNo;
  if (controlId==oPunch::PunchFinish && 
      FinishTime>0 && tStatus!=StatusUnknown) {
    stat=tStatus;
    rt=FinishTime;
  }
  else if (Card) {
		oPunch *p=Card->getPunchById(controlId);
		if (p && p->Time>0) {
      rt=p->getAdjustedTime();
      stat = StatusOK;
		}
    else 
      stat = controlId==oPunch::PunchFinish ? StatusDNF: StatusMP;
	}
  else if (cardno)	{
		oFreePunch *fp=oe->getPunch(getRaceNo(), controlId, cardno);

		if (fp) {
      rt=fp->getAdjustedTime();
      stat=StatusOK;
		}
    if(controlId==oPunch::PunchFinish && tStatus!=StatusUnknown)
      stat = tStatus;
	}
  rt-=tStartTime;

  if(rt<0)
    rt=0;
}

void oRunner::fillSpeakerObject(int leg, int controlId, oSpeakerObject &spk) const
{
  spk.runningTime=0;
  spk.status=StatusUnknown;
  spk.owner=const_cast<oRunner *>(this);

  spk.preliminaryRunningTimeLeg = 0;
  spk.runningTimeLeg = 0;

  getSplitTime(controlId, spk.status, spk.runningTime);
    
  if (controlId == oPunch::PunchFinish)
    spk.timeSinceChange = oe->getComputerTime() - FinishTime;
  else
    spk.timeSinceChange = oe->getComputerTime() - (spk.runningTime + tStartTime);

  const string &bib = getBib();

  if(!bib.empty()) {
    spk.name=bib + " " + getName();
  }
  else
    spk.name=getName();

  spk.club=getClub();
  spk.finishStatus=getStatus();

  spk.startTimeS=getStartTimeCompact();
  spk.missingStartTime = tStartTime<=0;
  
  spk.isRendered=false;
  
	map<int,int>::const_iterator mapit=Priority.find(controlId);
	if(mapit!=Priority.end())
		spk.priority=mapit->second;
  else 
    spk.priority=0;

  spk.preliminaryRunningTime=getPrelRunningTime();
  
  if(spk.status==StatusOK) {
    spk.runningTimeLeg=spk.runningTime;
    spk.preliminaryRunningTime=spk.runningTime;
    spk.preliminaryRunningTimeLeg=spk.runningTime;
  }
  else {
    spk.runningTimeLeg = spk.preliminaryRunningTime;
    spk.preliminaryRunningTimeLeg = spk.preliminaryRunningTime;
  }
}

pRunner oEvent::findRunner(const string &s, int lastId, const stdext::hash_set<int> &inputFilter,
                           stdext::hash_set<int> &matchFilter) const
{
  matchFilter.clear();
  string trm = trim(s);
  int len = trm.length();
  int sn = atoi(trm.c_str());
  char s_lc[1024];
  strcpy_s(s_lc, s.c_str());
  CharLowerBuff(s_lc, len);

  pRunner res = 0;

  if (!inputFilter.empty() && inputFilter.size() < Runners.size() / 2) {
    for (stdext::hash_set<int>::const_iterator it = inputFilter.begin(); it!= inputFilter.end(); ++it) {
      int id = *it;
      pRunner r = getRunner(id, 0);
      if (!r)
        continue;

      if (sn>0) {
        if (matchNumber(r->StartNo, s_lc) || matchNumber(r->CardNo, s_lc)) {
          matchFilter.insert(id);
          if (res == 0)
            res = r;
        }
      }
      else {
        if (filterMatchString(r->Name, s_lc)) {
          matchFilter.insert(id);
          if (res == 0)
            res = r;
        }
      }
    }
    return res;
  }

  oRunnerList::const_iterator itstart = Runners.begin();

  if(lastId) {
    for (; itstart != Runners.end(); ++itstart) {
      if(itstart->Id==lastId) {
        ++itstart;
        break;  
      }
    }
  }
  
  oRunnerList::const_iterator it;
  for (it=itstart; it != Runners.end(); ++it) {
    pRunner r = pRunner(&(*it));
    if (r->skip())
       continue;

    if (sn>0) {
      if (matchNumber(r->StartNo, s_lc) || matchNumber(r->CardNo, s_lc)) {
        matchFilter.insert(r->Id);
        if (res == 0)
          res = r;
      }
    }
    else {
      if (filterMatchString(r->Name, s_lc)) {
        matchFilter.insert(r->Id);
        if (res == 0)
          res = r;
      }
    }
  }
  for (it=Runners.begin(); it != itstart; ++it) { 
    pRunner r = pRunner(&(*it));
    if (r->skip())
       continue;

    if (sn>0) {
      if (matchNumber(r->StartNo, s_lc) || matchNumber(r->CardNo, s_lc)) {
        matchFilter.insert(r->Id);
        if (res == 0)
          res = r;
      }
    }
    else {
      if (filterMatchString(r->Name, s_lc)) {
        matchFilter.insert(r->Id);
        if (res == 0)
          res = r;
      }
    }
  }

  return res;
}

int oRunner::getTimeAfter(int leg) const
{
  if(leg==-1)
    leg=tDuplicateLeg;

  if(!Class || Class->tLeaderTime.size()<=unsigned(leg))
    return -1;

  int t=getRaceRunningTime(leg);

  if (t<=0)
    return -1;

  return t-Class->getTotalLegLeaderTime(leg, true);
}

int oRunner::getTimeAfter() const
{
  int leg=0;
  if(tInTeam)
    leg=tLeg;
  else 
    leg=tDuplicateLeg;

  if(!Class || Class->tLeaderTime.size()<=unsigned(leg))
    return -1;

  int t=getRunningTime();

  if (t<=0)
    return -1;

  return t-Class->getBestLegTime(leg);
}

int oRunner::getTimeAfterCourse() const
{
  if(!Class)
    return -1;

  const pCourse crs = getCourse(false);
  if (!crs)
    return -1;

  int t = getRunningTime();

  if (t<=0)
    return -1;

  int bt = Class->getBestTimeCourse(crs->getId());
  
  if (bt <= 0)
    return -1;

  return t - bt;
}

bool oRunner::synchronizeAll()
{
  if (tParentRunner)
    tParentRunner->synchronizeAll();
  else { 
    synchronize();
    for (size_t k=0;k<multiRunner.size();k++) {
      if (multiRunner[k]) 
        multiRunner[k]->synchronize();
    }
    if (Class && Class->isSingleRunnerMultiStage())
      if (tInTeam)
        tInTeam->synchronize(false);
	}
  return true;
}

const string &oAbstractRunner::getBib() const
{
  return getDCI().getString("Bib");
}

void oRunner::setBib(const string &bib, bool updateStartNo, bool tmpOnly) {
  if (tParentRunner)
    tParentRunner->setBib(bib, updateStartNo, tmpOnly);
  else {
    if (updateStartNo)
      setStartNo(atoi(bib.c_str()), tmpOnly); // Updates multi too.
    
    if (tmpOnly) {
      tmpStore.bib = bib;
      return;
    }
    if (getDI().setString("Bib", bib)) {
      if (oe)
        oe->bibStartNoToRunnerTeam.clear();
    }
    
    for (size_t k=0;k<multiRunner.size();k++) {
      if(multiRunner[k]) {
        multiRunner[k]->getDI().setString("Bib", bib);
      }
    }
	}
}


void oEvent::analyseDNS(vector<pRunner> &unknown_dns, vector<pRunner> &known_dns, 
                        vector<pRunner> &known, vector<pRunner> &unknown)
{
  autoSynchronizeLists(true);
  
  vector<pRunner> stUnknown;
  vector<pRunner> stDNS;

  for (oRunnerList::iterator it = Runners.begin(); it!=Runners.end();++it) {
    if (!it->isRemoved() && !it->needNoCard()) {
      if (it->getStatus() == StatusUnknown)
        stUnknown.push_back(&*it);
      else if(it->getStatus() == StatusDNS)
        stDNS.push_back(&*it);
    }
  }

  set<int> knownPunches;
  for (oFreePunchList::iterator it = punches.begin(); it!=punches.end(); ++it) {
    knownPunches.insert(it->getCardNo());
  }

  set<int> knownCards;
  for (oCardList::iterator it = Cards.begin(); it!=Cards.end(); ++it) {
    if (it->tOwner == 0)
      knownCards.insert(it->CardNo);
  }

  unknown.clear();
  known.clear();

  for (size_t k=0;k<stUnknown.size();k++) {
    int card = stUnknown[k]->CardNo;
    if (card == 0)
      unknown.push_back(stUnknown[k]);
    else if (knownCards.count(card)==1 || knownPunches.count(card)==1)
      known.push_back(stUnknown[k]);
    else
      unknown.push_back(stUnknown[k]); //These can be given "dns"
  }

  unknown_dns.clear();
  known_dns.clear();

  for (size_t k=0;k<stDNS.size(); k++) {
    int card = stDNS[k]->CardNo;
    if (card == 0)
      unknown_dns.push_back(stDNS[k]);
    else if (knownCards.count(card)==1 || knownPunches.count(card)==1)
      known_dns.push_back(stDNS[k]);
    else
      unknown_dns.push_back(stDNS[k]); 
  }
}

static int findNextControl(const vector<pControl> &ctrl, int startIndex, int id, int &offset, bool supportRogaining)
{
  vector<pControl>::const_iterator it=ctrl.begin();
  int index=0;
  offset = 1;
  while(startIndex>0 && it!=ctrl.end()) {
    int multi = (*it)->getNumMulti();
    offset += multi-1;
    ++it, --startIndex, ++index;
    if (it!=ctrl.end() && (*it)->isRogaining(supportRogaining))
      index--;
  }

  while(it!=ctrl.end() && (*it) && (*it)->getId()!=id) {
    int multi = (*it)->getNumMulti();
    offset += multi-1;
    ++it, ++index;
    if (it!=ctrl.end() && (*it)->isRogaining(supportRogaining))
      index--;
  }

  if (it==ctrl.end())
    return -1;
  else
    return index;
}

void oRunner::printSplits(gdioutput &gdi) const {
  bool withAnalysis = (oe->getDI().getInt("Analysis") & 1) == 0;
  bool withSpeed = (oe->getDI().getInt("Analysis") & 2) == 0;
  
  
  if (getCourse(false) && getCourse(false)->hasRogaining()) {
	  vector<pControl> ctrl;
    bool rogaining(true);
    getCourse(false)->getControls(ctrl);
    for (vector<pControl>::const_iterator it=ctrl.begin(); it!=ctrl.end(); ++it)
      if (!(*it)->isRogaining(true))
        rogaining = false;
    if (rogaining) {  
      printRogainingSplits( gdi);
      return;
    }
  }

  gdi.setCX(10);
  gdi.fillDown();
  gdi.addStringUT(boldText, oe->getName());
  gdi.addStringUT(fontSmall, oe->getDate());
  gdi.dropLine(0.5);
  pCourse pc = getCourse(true);

  gdi.addStringUT(boldSmall, getName() + ", " + getClass());
  gdi.addStringUT(fontSmall, getClub());
  gdi.dropLine(0.5);
  gdi.addStringUT(fontSmall, lang.tl("Start: ") + getStartTimeS() + lang.tl(", M�l: ") + getFinishTimeS());
  string statInfo = lang.tl("Status: ") + getStatusS() + lang.tl(", Tid: ") + getRunningTimeS();
  if (withSpeed && pc && pc->getLength() > 0) {
    int kmt = (getRunningTime() * 1000) / pc->getLength();
    statInfo += " (" + formatTime(kmt) + lang.tl(" min/km") + ")";
  }
  if (pc && withSpeed) {
    if (pc->legLengths.empty() || *max_element(pc->legLengths.begin(), pc->legLengths.end()) <= 0)
      withSpeed = false; // No leg lenghts available
  }
  gdi.addStringUT(fontSmall, statInfo);
  oe->calculateResults(oEvent::RTClassResult);
	if (getPlaceS() != _EmptyString)
    gdi.addStringUT(fontSmall, lang.tl("Aktuell klassposition") + " in " + getClass() + ": " + getPlaceS());
  

  int cy = gdi.getCY()+4;
  int cx = gdi.getCX();

  int spMax = 0;
  int totMax = 0;
  if (pc) {
    for (int n = 0; n < pc->nControls; n++) {
      spMax = max(spMax, getSplitTime(n, false));
      totMax = max(totMax, getPunchTime(n, false));
    }

  }
  bool moreThanHour = max(totMax, getRunningTime()) >= 3600;
  bool moreThanHourSplit = spMax >= 3600;

  const int c1=35;
  const int c2=95 + (moreThanHourSplit ? 65 : 55);
  const int c3 = c2 + 10;
  const int c4 = moreThanHour ? c3+153 : c3+133;
  const int c5 = withSpeed ? c4 + 80 : c4;

  char bf[256];
  int lastIndex = -1;
  int adjust = 0;
  int offset = 1;

  vector<pControl> ctrl;

  int finishType = -1;
  int startType = -1, startOffset = 0;
  if (pc) {
    pc->getControls(ctrl);
    finishType = pc->getFinishPunchType();

    if (pc->useFirstAsStart()) {
      startType = pc->getStartPunchType();
      startOffset = -1;
    }
  }
  
  if (Card && pc) {
    bool hasRogaining = pc->hasRogaining();

    gdi.addString("", cy, cx, italicSmall, "Kontroll");
    gdi.addString("", cy, cx+c2-55, italicSmall, "Tid");
    //gdi.addString("", cy, cx+c3, boldSmall, "Tidpunkt");
    //gdi.addString("", cy, cx+c4, boldSmall|textRight, "Summa");
    if (withSpeed)
      gdi.addString("", cy, cx+c5, italicSmall|textRight, "min/km");
    cy += int(gdi.getLineHeight()*0.9);

    oPunchList &p=Card->Punches;
    for (oPunchList::iterator it=p.begin();it!=p.end();++it) {
      bool any = false;
      if (it->tRogainingIndex>=0) { 
        const pControl c = pc->getControl(it->tRogainingIndex);
        string point = c ? itos(c->getRogainingPoints()) + "p." : "";

        gdi.addStringUT(cy, cx + c1 + 10, fontSmall, point);
        any = true;

        sprintf_s(bf, "%d", it->Type);
        gdi.addStringUT(cy, cx, fontSmall, bf);
        int st = Card->getSplitTime(getStartTime(), &*it);
        
        if (st>0)
          gdi.addStringUT(cy, cx + c2, fontSmall|textRight, formatTime(st));

        gdi.addStringUT(cy, cx+c3, fontSmall, it->getTime());

        int pt = it->getAdjustedTime();
        st = getStartTime();
        if (st>0 && pt>0 && pt>st) {
          string punchTime = formatTime(pt-st);
          gdi.addStringUT(cy, cx+c4, fontSmall|textRight, punchTime);
        }

        cy+=int(gdi.getLineHeight()*0.9);
        continue;
      }

      int cid = it->tMatchControlId;
      string punchTime; 
      int sp;
      int controlLegIndex = -1;
      if (it->isFinish(finishType)) {
        gdi.addString("", cy, cx, fontSmall, "M�l");
        sp = getSplitTime(splitTimes.size(), false);
        if (sp>0) {
          gdi.addStringUT(cy, cx+c2, fontSmall|textRight, formatTime(sp));
          punchTime = formatTime(getRunningTime());
        }
        gdi.addStringUT(cy, cx+c3, fontSmall, oe->getAbsTime(it->Time + adjust));
        any = true;
        if (!punchTime.empty()) {
          gdi.addStringUT(cy, cx+c4, fontSmall|textRight, punchTime);
        }
        controlLegIndex = pc->getNumControls();
      }
      else if (it->Type>10) { //Filter away check and start
        int index = -1;
        if (cid>0)
          index = findNextControl(ctrl, lastIndex+1, cid, offset, hasRogaining);
        if (index>=0) {
          if (index > lastIndex + 1) {
            int xx = cx;
            string str = MakeDash("-");
            int posy = cy-int(gdi.getLineHeight()*0.4);
            const int endx = cx+c5 + 5;

            while (xx < endx) {
              gdi.addStringUT(posy, xx, fontSmall, str);
              xx += 20;
            }
            
            cy+=int(gdi.getLineHeight()*0.3);
          }
          lastIndex = index;

          if (it->Type == startType && (index+offset) == 1) 
            continue; // Skip start control

          sprintf_s(bf, "%d.", index+offset+startOffset);
          gdi.addStringUT(cy, cx, fontSmall, bf);
          sprintf_s(bf, "(%d)", it->Type);
          gdi.addStringUT(cy, cx+c1, fontSmall, bf);
          
          controlLegIndex = it->tIndex;
          adjust = getTimeAdjust(controlLegIndex);
          sp = getSplitTime(controlLegIndex, false);
          if (sp>0) {
            punchTime = getPunchTimeS(controlLegIndex, false);
						gdi.addStringUT(cy, cx+c2, getLegPlace(it->tIndex) == 1 ? boldSmall|textRight : fontSmall|textRight, formatTime(sp));
          }
        }
        else {
          if (!it->isUsed) {
            gdi.addStringUT(cy, cx, fontSmall, MakeDash("-"));
          }
          sprintf_s(bf, "(%d)", it->Type);
          gdi.addStringUT(cy, cx+c1, fontSmall, bf);
        }
        if (it->Time > 0)
          gdi.addStringUT(cy, cx+c3, fontSmall, oe->getAbsTime(it->Time + adjust));
        else {
          string str = MakeDash("-");
          gdi.addStringUT(cy, cx+c3, fontSmall, str);
        }

        if (!punchTime.empty()) {
          gdi.addStringUT(cy, cx+c4, getLegPlaceAcc(it->tIndex) == 1 ? boldSmall|textRight : fontSmall|textRight, punchTime);
        }
        any = true;
      }

      if (withSpeed && controlLegIndex>=0 && size_t(controlLegIndex) < pc->legLengths.size()) {
			  int length = pc->legLengths[controlLegIndex];
			  if(length > 0) {
				  int tempo=(sp*1000)/length;
          gdi.addStringUT(cy, cx+c5, fontSmall|textRight, formatTime(tempo));
			  }
      }

      if (any)
        cy+=int(gdi.getLineHeight()*0.9);
    }
    gdi.dropLine();
    
    if (withAnalysis) {
      vector<string> misses;
      int last = ctrl.size();
      if (pc->useLastAsFinish())
        last--;

      for (int k = pc->useFirstAsStart() ? 1 : 0; k < last; k++) {
        int missed = getMissedTime(k);
        if (missed>0) {
          misses.push_back(pc->getControlOrdinal(k) + "-" + formatTime(missed)); 
        }
      }
      if (misses.size()==0) {
        vector<pRunner> rOut;
        oe->getRunners(0, pc->getId(), rOut, false);
        int count = 0;
        for (size_t k = 0; k < rOut.size(); k++) {
          if (rOut[k]->getCard())
            count++;
        }

        if (count < 3)
          gdi.addString("", fontSmall, "Underlag saknas f�r bomanalys.");
        else
          gdi.addString("", fontSmall, "Inga bommar registrerade.");
      }
      else {
        string out = lang.tl("Tidsf�rluster (kontroll-tid): ");
        for (size_t k = 0; k<misses.size(); k++) {
          if (out.length() > (withSpeed ? 40u : 35u)) {
            gdi.addStringUT(fontSmall, out);
            out.clear();
          }
          out += misses[k];
          if(k < misses.size()-1)
            out += ", ";
          else 
            out += ".";
        }
        gdi.addStringUT(fontSmall, out);
      }
    }
  }
  gdi.dropLine();

  vector< pair<string, int> > lines;
  oe->getExtraLines("SPExtra", lines);

  for (size_t k = 0; k < lines.size(); k++) {
    gdi.addStringUT(lines[k].second, lines[k].first);
  }
  if (lines.size()>0)
    gdi.dropLine(0.5);

  gdi.addString("", fontSmall, "Av MeOS: www.melin.nu/meos");
}

void oRunner::printRogainingSplits(gdioutput &gdi) const {
  const int ct1=160;

  gdi.setCX(10);
  gdi.fillDown();
  gdi.addStringUT(boldText, oe->getName());
  gdi.addStringUT(fontSmall, oe->getDate());
  gdi.dropLine(0.5);
  
  gdi.addStringUT(boldSmall, getName() + " " + getClub());
  int cy = gdi.getCY();
  gdi.addStringUT(boldSmall, lang.tl("Po�ng: ") + itos(getRogainingPoints()));    
  gdi.addStringUT(cy, gdi.getCX() + ct1, boldSmall, lang.tl("Tid: ") + getRunningTimeS());
	if (getCard())
		gdi.addStringUT(fontSmall, lang.tl("SportIdent: ") + getCard()->getCardNo());
  gdi.dropLine(0.5);
  cy = gdi.getCY();
  gdi.addStringUT(fontSmall, lang.tl("Bana: ") + getCourseName());
  gdi.addStringUT(cy, gdi.getCX() + ct1, fontSmall, lang.tl("Klass: ") + getClass());
  cy = gdi.getCY();
  gdi.addStringUT(fontSmall, lang.tl("Gross: ") + itos(getRogainingPoints() + getPenaltyPoints()));
  gdi.addStringUT(cy, gdi.getCX() + ct1, fontSmall, lang.tl("Penalty: ") + itos(getPenaltyPoints()));
  cy = gdi.getCY();
  gdi.addStringUT(fontSmall, lang.tl("Start: ") + getStartTimeS());
  gdi.addStringUT(cy, gdi.getCX() + ct1, fontSmall, lang.tl("M�l: ") + getFinishTimeS());

  cy = gdi.getCY()+4;
  int cx = gdi.getCX();
  const int c1=90;
  const int c2=130;
  const int c3=170;
  const int c4=200;
  const int c5=240;
  const int width = 68;
  char bf[256];
  int lastIndex = -1;
  int adjust = 0;
  int offset = 1;
  int runningTotal = 0;

  bool moreThanHour = getRunningTime()>3600;
  vector<pControl> ctrl;
  pCourse pc = getCourse(false);
  if (pc)
    pc->getControls(ctrl);

  if (Card) {
    oPunchList &p=Card->Punches;
		/*gdi.addStringUT(cy, cx, fontSmall, lang.tl("Kontroll"));
		gdi.addStringUT(cy, cx + c3, fontSmall, lang.tl("Po�ng"));
		gdi.addStringUT(cy, cx + c4, fontSmall, lang.tl("L�ptid"));*/
		cy+=int(gdi.getLineHeight()*0.9);
    for (oPunchList::iterator it=p.begin();it!=p.end();++it) {
      if (it->tRogainingIndex>=0) { 
        const pControl c = pc->getControl(it->tRogainingIndex);
        string point = c ? c->getRogainingPointsS() : "";
        runningTotal += c ? c->getRogainingPoints() : 0;   
        
        if (c->getName().length() > 0) 
          gdi.addStringUT(cy, cx, fontSmall, c->getName() + " (" + itos(it->getControlNumber()) + ")");
        else
          gdi.addStringUT(cy, cx, fontSmall, itos(it->getControlNumber()));
        gdi.addStringUT(cy, cx + c1, fontSmall, point);

        int st = Card->getSplitTime(getStartTime(), &*it);
        
        if (st>0) {
          gdi.addStringUT(cy, cx + c2, fontSmall, formatTime(st));
					if (c->getRogainingPoints() > 0) {
						float pps = ((float)c->getRogainingPoints() * 60.0f)/st;
						sprintf_s(bf, "%01.1f", pps);
						//gdi.addStringUT(cy, cx+c2, fontSmall, bf);
					}
				}

        int pt = it->getAdjustedTime();
        st = getStartTime();
        if (st>0 && pt>0 && pt>st) {
          string punchTime = formatTime(pt-st);
          if (!moreThanHour)
            gdi.addStringUT(cy, cx+c5, fontSmall, punchTime);
          else
            gdi.addStringUT(cy, cx+c5+width, fontSmall|textRight, punchTime);
        }

        gdi.addStringUT(cy, cx+c4, fontSmall, itos(runningTotal));

        cy+=int(gdi.getLineHeight()*0.9);
        continue;
      }

      int cid = it->tMatchControlId;
      string punchTime; 
      if (it->isFinish()) {
        gdi.addString("", cy, cx, fontSmall, "M�l");
        int sp = getSplitTime(splitTimes.size(), false);
        if (sp>0) {
          gdi.addStringUT(cy, cx+c2, fontSmall, formatTime(sp));
          punchTime = formatTime(getRunningTime());
        }
        gdi.addStringUT(cy, cx+c3, fontSmall, oe->getAbsTime(it->Time + adjust));

        if (!punchTime.empty()) {
          if (!moreThanHour)
            gdi.addStringUT(cy, cx+c5, fontSmall, punchTime);
          else
            gdi.addStringUT(cy, cx+c5+width, fontSmall|textRight, punchTime);
        }
        cy+=gdi.getLineHeight();
      }
      else if (it->Type>10) { //Filter away check and start
        int index = -1;
        if (cid>0)
          index = findNextControl(ctrl, lastIndex+1, cid, offset, true);
        if (index>=0) {
          if (index > lastIndex + 1) {
            int xx = cx;
            string str = MakeDash("-");
            int posy = cy-int(gdi.getLineHeight()*0.4);
            const int endx = cx+c5 + (moreThanHour ? width : 50);

            while (xx < endx) {
              gdi.addStringUT(posy, xx, fontSmall, str);
              xx += 20;
            }
            
            cy+=int(gdi.getLineHeight()*0.3);
          }
          
          lastIndex = index;
          sprintf_s(bf, "%d.", index+offset);
          gdi.addStringUT(cy, cx, fontSmall, bf);
          sprintf_s(bf, "(%d)", it->Type);
          gdi.addStringUT(cy, cx+c1, fontSmall, bf);
          
          adjust = getTimeAdjust(it->tIndex);
          int sp = getSplitTime(it->tIndex, false);
          if (sp>0) {
            punchTime = getPunchTimeS(it->tIndex, false);
            gdi.addStringUT(cy, cx+c2, fontSmall, formatTime(sp));
          }
        }
        else {
          if (!it->isUsed) {
            gdi.addStringUT(cy, cx, fontSmall, MakeDash("-"));
          }
          sprintf_s(bf, "(%d)", it->Type);
          gdi.addStringUT(cy, cx+c1, fontSmall, bf);
        }
        if (it->Time > 0)
          gdi.addStringUT(cy, cx+c3, fontSmall, oe->getAbsTime(it->Time + adjust));
        else {
          string str = MakeDash("-");
          gdi.addStringUT(cy, cx+c3, fontSmall, str);
        }

        if (!punchTime.empty()) {
          if (!moreThanHour)
            gdi.addStringUT(cy, cx+c5, fontSmall, punchTime);
          else
            gdi.addStringUT(cy, cx+c5+width, fontSmall|textRight, punchTime);
        }
        cy+=int(gdi.getLineHeight()*0.9);
      }
    }


		if (getProblemDescription().size() > 0)
			gdi.addStringUT(fontSmall, lang.tl(getProblemDescription()));
  }

  gdi.dropLine();

  vector< pair<string, int> > lines;
  oe->getExtraLines("SPExtra", lines);

  for (size_t k = 0; k < lines.size(); k++) {
    gdi.addStringUT(lines[k].second, lines[k].first);
  }
  if (lines.size()>0)
    gdi.dropLine(0.5);

  gdi.addString("", fontSmall, "Av MeOS: www.melin.nu/meos");
}

void oRunner::printLabel(gdioutput &gdi) const {
  
  bool rogaining(false);
  if (getCourse(false) && getCourse(false)->hasRogaining()) {
		rogaining = true;
	  vector<pControl> ctrl;
    getCourse(false)->getControls(ctrl);
    for (vector<pControl>::const_iterator it=ctrl.begin(); it!=ctrl.end(); ++it)
      if (!(*it)->isRogaining(true))
        rogaining = false;
  }

  const int c2=400;
 	gdi.setCX(0);
	gdi.setCY(0);
	int cx = gdi.getCX();
	int cy = gdi.getCY();
  gdi.fillDown();

	gdi.addStringUT(getName().length() < 20 ? boldHuge : boldLarge, getName());
	gdi.dropLine(getName().length() < 20 ? 0.2 : 0.4);
	cy = gdi.getCY();
	gdi.addStringUT(cy, cx, boldLarge, getClass());
	if (getStatus()==StatusOK)
			{
			if (rogaining)
				gdi.addStringUT(cy, cx+c2, boldHuge, itos(getRogainingPoints()));    
			else
				gdi.addStringUT(cy, cx+c2, boldHuge, getRunningTimeS());
			}
		else
			gdi.addStringUT(cy, cx+c2, boldHuge,  getStatusS());
  gdi.dropLine(-0.1);
	cy = gdi.getCY();
	gdi.addStringUT(cy, cx, fontMedium, getClub());
	if (getCourse(false))
		gdi.addStringUT(cy, cx + c2, fontMedium, getCourse(false)->getName());

}

vector<pRunner> oRunner::getRunnersOrdered() const {
  if (tParentRunner)
    return tParentRunner->getRunnersOrdered();

  vector<pRunner> r(multiRunner.size()+1);
  r[0] = (pRunner)this;
  for (size_t k=0;k<multiRunner.size();k++)
    r[k+1] = (pRunner)multiRunner[k];

  return r;
}

int oRunner::getMultiIndex() {
  if (!tParentRunner)
    return 0;

  const vector<pRunner> &r = tParentRunner->multiRunner;

  for (size_t k=0;k<r.size(); k++)
    if (r[k]==this)
      return k+1;

  // Error
  tParentRunner = 0;
  markForCorrection();
  return -1;
}

void oRunner::correctRemove(pRunner r) {
  for(unsigned i=0;i<multiRunner.size(); i++) 
    if(r!=0 && multiRunner[i]==r) {
      multiRunner[i] = 0;
      r->tParentRunner = 0;
      r->tLeg = 0;
      if (i+1==multiRunner.size())
        multiRunner.pop_back();

      correctionNeeded = true;
      r->correctionNeeded = true;
    }
}

void oEvent::updateRunnersFromDB() 
{
	oRunnerList::iterator it;	
  if (!oe->useRunnerDb())
    return;

	for (it=Runners.begin(); it != Runners.end(); ++it) {
    if (!it->isVacant() && !it->isRemoved())
      it->updateFromDB(it->Name, it->getClubId(), it->getClassId(), it->getCardNo(), it->getBirthYear());
  }
}

bool oRunner::updateFromDB(const string &name, int clubId, int classId, 
                           int cardNo, int birthYear) {
  if (!oe->useRunnerDb())
    return false;
  
  pRunner db_r = 0;
  if (cardNo>0) {
    db_r = oe->dbLookUpByCard(cardNo);
    
    if (db_r && db_r->matchName(name)) {
      //setName(db_r->getName());
      //setClub(db_r->getClub()); Don't...
      setExtIdentifier(db_r->getExtIdentifier());
      setBirthYear(db_r->getBirthYear());
      setSex(db_r->getSex());
      setNationality(db_r->getNationality());
      return true;
    }
  }

  db_r = oe->dbLookUpByName(name, clubId, classId, birthYear);

  if (db_r) {
    setExtIdentifier(db_r->getExtIdentifier());
    setBirthYear(db_r->getBirthYear());
    setSex(db_r->getSex());
    setNationality(db_r->getNationality());
    return true;
  }
  else if (getExtIdentifier()>0) {
    db_r = oe->dbLookUpById(getExtIdentifier());
    if (db_r && db_r->matchName(name)) {
      setBirthYear(db_r->getBirthYear());
      setSex(db_r->getSex());
      setNationality(db_r->getNationality());
      return true;
    }
    // Reset external identifier
    setExtIdentifier(0);
    setBirthYear(0);
    // Do not reset nationality and sex, 
    // since they are likely correct.
  }

  return false;
}

void oRunner::setSex(PersonSex sex) 
{
  getDI().setString("Sex", encodeSex(sex));
}

PersonSex oRunner::getSex() const 
{
  return interpretSex(getDCI().getString("Sex"));
}

void oRunner::setBirthYear(int year) 
{
  getDI().setInt("BirthYear", year);
}

int oRunner::getBirthYear() const 
{
  return getDCI().getInt("BirthYear");
}

void oAbstractRunner::setSpeakerPriority(int year) 
{
  if (Class) {
    oe->classChanged(Class, false);
  }
  getDI().setInt("Priority", year);
}

int oAbstractRunner::getSpeakerPriority() const 
{
  return getDCI().getInt("Priority");
}

int oRunner::getSpeakerPriority() const {
  int p = oAbstractRunner::getSpeakerPriority();

  if (tParentRunner)
    p = max(p, tParentRunner->getSpeakerPriority());
  else if (tInTeam) {
    p = max(p, tInTeam->getSpeakerPriority());
  }

  return p;
}

void oRunner::setNationality(const string &nat) 
{
  getDI().setString("Nationality", nat);
}

string oRunner::getNationality() const 
{
  return getDCI().getString("Nationality");
}

bool oRunner::matchName(const string &pname) const
{
  if (pname == Name)
    return true;

  vector<string> myNames, inNames;

  split(Name, " ", myNames);
  split(pname, " ", inNames);

  for (size_t k = 0; k < myNames.size(); k++) 
    myNames[k] = canonizeName(myNames[k].c_str());
  
  int nMatched = 0;
  for (size_t j = 0; j < inNames.size(); j++) {
    string inName = canonizeName(inNames[j].c_str());
    for (size_t k = 0; k < myNames.size(); k++) {
      if (myNames[k] == inName) {
        nMatched++;

        // Suppert changed last name in the most common case
        if (j == 0 && k == 0 && inNames.size() == 2 && myNames.size() == 2) {
          return true;
        }
        break;
      }
    }
  }

  return nMatched >= min<int>(myNames.size(), 2);
}

bool oRunner::autoAssignBib() {
  if (Class == 0 || !getBib().empty())
    return !getBib().empty();

  int maxbib = 0;
  for(oRunnerList::iterator it = oe->Runners.begin(); it !=oe->Runners.end();++it) {
    if (it->Class == Class)
      maxbib = max(atoi(it->getBib().c_str()), maxbib);
  }

  if (maxbib>0) {
    setBib(itos(maxbib+1), true, false);
    return true;
  }
  return false;
}

void oRunner::getSplitAnalysis(vector<int> &deltaTimes) const {
  deltaTimes.clear();
  vector<int> mp;

  if (splitTimes.empty() || !Class)
    return;

  if (Class->tSplitRevision == tSplitRevision)
    deltaTimes = tMissedTime;

  pCourse pc = getCourse(true);
  if (!pc)
    return;
  vector<int> reorder;
  if (pc->isAdapted())
    reorder = pc->getMapToOriginalOrder();
  else {
    reorder.reserve(pc->nControls+1);
    for (int k = 0; k <= pc->nControls; k++)
      reorder.push_back(k);
  }

  int id = pc->getId();

  if (Class->tSplitAnalysisData.count(id) == 0)
    Class->calculateSplits();
 
  const vector<int> &baseLine = Class->tSplitAnalysisData[id];
  const unsigned nc = pc->getNumControls();

  if (baseLine.size() != nc+1)
    return;

  vector<double> res(nc+1);
        
  double resSum = 0;
  double baseSum = 0;
  double bestTime = 0;
  for (size_t k = 0; k <= nc; k++) {
    res[k] = getSplitTime(k, false);
    if (res[k] > 0) {
      resSum += res[k];
      baseSum += baseLine[reorder[k]];
    }
    bestTime += baseLine[reorder[k]];
  }

  deltaTimes.resize(nc+1);

  // Adjust expected time by removing mistakes
  for (size_t k = 0; k <= nc; k++) {
    if (res[k]  > 0) {
      double part = res[k]*baseSum/(resSum * bestTime);
      double delta = part - baseLine[reorder[k]] / bestTime;
      int deltaAbs = int(floor(delta * resSum + 0.5));
      if (res[k]-deltaAbs < baseLine[reorder[k]])
        deltaAbs = int(res[k] - baseLine[reorder[k]]);

      if (deltaAbs>0)
        resSum -= deltaAbs;
    }
  }

  for (size_t k = 0; k <= nc; k++) {
    if (res[k]  > 0) {
      double part = res[k]*baseSum/(resSum * bestTime);
      double delta = part - baseLine[reorder[k]] / bestTime;
      
      int deltaAbs = int(floor(delta * resSum + 0.5));

      if (deltaAbs > 0) {
        if ( fabs(delta) > 1.0/100 && (20.0*deltaAbs)>res[k] && deltaAbs>=15)
          deltaTimes[k] = deltaAbs;

        res[k] -= deltaAbs;
        if (res[k] < baseLine[reorder[k]])
          res[k] = baseLine[reorder[k]];
      }
    }
  }

  resSum = 0;
  for (size_t k = 0; k <= nc; k++) {
    if (res[k] > 0) {
      resSum += res[k];
    }
  }

  for (size_t k = 0; k <= nc; k++) {
    if (res[k]  > 0) {
      double part = res[k]*baseSum/(resSum * bestTime);
      double delta = part - baseLine[reorder[k]] / bestTime;
      int deltaAbs = int(floor(delta * resSum + 0.5));

      if (deltaTimes[k]==0 && fabs(delta) > 1.0/100 && deltaAbs>=15)
        deltaTimes[k] = deltaAbs;
    }
  }
}

void oRunner::getLegPlaces(vector<int> &places) const
{
  places.clear();
  pCourse pc = getCourse(true);
  if (!pc || !Class || splitTimes.empty())
    return;
  if (Class->tSplitRevision == tSplitRevision)
    places = tPlaceLeg;

  int id = pc->getId();

  if (Class->tSplitAnalysisData.count(id) == 0)
    Class->calculateSplits();
 
  const unsigned nc = pc->getNumControls();

  places.resize(nc+1);
  int cc = pc->getCommonControl();
  for (unsigned k = 0; k<=nc; k++) {
    int to = cc;
    if (k<nc)
      to = pc->getControl(k) ? pc->getControl(k)->getId() : 0;
    int from = cc;
    if (k>0)
      from = pc->getControl(k-1) ? pc->getControl(k-1)->getId() : 0;

    int time = getSplitTime(k, false);

    if (time>0)
      places[k] = Class->getLegPlace(from, to, time);
    else
      places[k] = 0;
  }
}

void oRunner::getLegTimeAfter(vector<int> &times) const 
{
  times.clear();
  if (splitTimes.empty() || !Class)
    return;
  if (Class->tSplitRevision == tSplitRevision) {
    times = tAfterLeg;
    return;
  }

  pCourse pc = getCourse(false);
  if (!pc)
    return;

  int id = pc->getId();

  if (Class->tCourseLegLeaderTime.count(id) == 0)
    Class->calculateSplits();
 
  const unsigned nc = pc->getNumControls();

  const vector<int> leaders = Class->tCourseLegLeaderTime[id];
  
  if (leaders.size() != nc + 1) 
    return;

  times.resize(nc+1);

  for (unsigned k = 0; k<=nc; k++) {
    int s = getSplitTime(k, true);

    if (s>0) {
      times[k] = s - leaders[k];
      if (times[k]<0)
        times[k] = -1;
    }
    else
      times[k] = -1;
  }
  // Normalized order
  const vector<int> &reorder = getCourse(true)->getMapToOriginalOrder();
  if (!reorder.empty()) {
    vector<int> orderedTimes(times.size());
    for (size_t k = 0; k < times.size(); k++) {
      orderedTimes[k] = times[reorder[k]];
    }
    times.swap(orderedTimes);
  }
}

void oRunner::getLegTimeAfterAcc(vector<int> &times) const 
{
  times.clear();
  if (splitTimes.empty() || !Class || tStartTime<=0)
    return;
  if (Class->tSplitRevision == tSplitRevision)
    times = tAfterLegAcc;

  pCourse pc = getCourse(false); //XXX Does not work for loop courses
  if (!pc)
    return;

  int id = pc->getId();

  if (Class->tCourseAccLegLeaderTime.count(id) == 0)
    Class->calculateSplits();
 
  const unsigned nc = pc->getNumControls();

  const vector<int> leaders = Class->tCourseAccLegLeaderTime[id];
  const vector<SplitData> &sp = getSplitTimes(true);
  if (leaders.size() != nc + 1)
    return;
  //xxx reorder output
  times.resize(nc+1);

  for (unsigned k = 0; k<=nc; k++) {
    int s = 0;
    if (k < sp.size())
      s = sp[k].time;
    else if (k==nc)
      s = FinishTime;

    if (s>0) {
      times[k] = s - tStartTime - leaders[k];
      if (times[k]<0)
        times[k] = -1;
    }
    else
      times[k] = -1;
  }

   // Normalized order
  const vector<int> &reorder = getCourse(true)->getMapToOriginalOrder();
  if (!reorder.empty()) {
    vector<int> orderedTimes(times.size());
    for (size_t k = 0; k < times.size(); k++) {
      orderedTimes[k] = times[reorder[k]];
    }
    times.swap(orderedTimes);
  }
}

void oRunner::getLegPlacesAcc(vector<int> &places) const
{
  places.clear();
  pCourse pc = getCourse(false);
  if (!pc || !Class)
    return;
  if (splitTimes.empty() || tStartTime<=0)
    return;
  if (Class->tSplitRevision == tSplitRevision) {
    places = tPlaceLegAcc;
    return;
  }

  int id = pc->getId();
  const unsigned nc = pc->getNumControls();
  const vector<SplitData> &sp = getSplitTimes(true);
  places.resize(nc+1);
  for (unsigned k = 0; k<=nc; k++) {
    int s = 0;
    if (k < sp.size())
      s = sp[k].time;
    else if (k==nc)
      s = FinishTime;

    if (s>0) {
      int time = s - tStartTime;

      if (time>0)
        places[k] = Class->getAccLegPlace(id, k, time);
      else
        places[k] = 0;
    }
  }

  // Normalized order
  const vector<int> &reorder = getCourse(true)->getMapToOriginalOrder();
  if (!reorder.empty()) {
    vector<int> orderedPlaces(places.size());
    for (size_t k = 0; k < places.size(); k++) {
      orderedPlaces[k] = places[reorder[k]];
    }
    places.swap(orderedPlaces);
  }
}

void oRunner::setupRunnerStatistics() const
{
  if (!Class)
    return;
  if (Class->tSplitRevision == tSplitRevision)
    return;

  getSplitAnalysis(tMissedTime);
  getLegPlaces(tPlaceLeg);
  getLegTimeAfter(tAfterLeg);
  getLegPlacesAcc(tPlaceLegAcc);
  getLegPlacesAcc(tAfterLegAcc);
  tSplitRevision = Class->tSplitRevision; 
}

int oRunner::getMissedTime(int ctrlNo) const {
  setupRunnerStatistics();
  if (unsigned(ctrlNo) < tMissedTime.size())
    return tMissedTime[ctrlNo];
  else
    return -1;
}

string oRunner::getMissedTimeS() const
{
  setupRunnerStatistics();
  int t = 0;
  for (size_t k = 0; k<tMissedTime.size(); k++)
    if (tMissedTime[k]>0)
      t += tMissedTime[k];

  return getTimeMS(t);
}

string oRunner::getMissedTimeS(int ctrlNo) const
{
  int t = getMissedTime(ctrlNo);
  if (t>0)
    return getTimeMS(t);
  else
    return "";
}

int oRunner::getLegPlace(int ctrlNo) const {
  setupRunnerStatistics();
  if (unsigned(ctrlNo) < tPlaceLeg.size())
    return tPlaceLeg[ctrlNo];
  else
    return 0;
}

int oRunner::getLegTimeAfter(int ctrlNo) const {
  setupRunnerStatistics();
  if (unsigned(ctrlNo) < tAfterLeg.size())
    return tAfterLeg[ctrlNo];
  else
    return -1;
}

int oRunner::getLegPlaceAcc(int ctrlNo) const {
  setupRunnerStatistics();
  if (unsigned(ctrlNo) < tPlaceLegAcc.size())
    return tPlaceLegAcc[ctrlNo];
  else
    return 0;
}

int oRunner::getLegTimeAfterAcc(int ctrlNo) const {
  setupRunnerStatistics();
  if (unsigned(ctrlNo) < tAfterLegAcc.size())
    return tAfterLegAcc[ctrlNo];
  else
    return -1;
}

int oRunner::getTimeWhenPlaceFixed() const {
  if (!Class || !statusOK())
    return -1;

#ifndef MEOSDB 
  if (unsigned(tLeg) >= Class->tResultInfo.size()) {
    oe->analyzeClassResultStatus();
    if (unsigned(tLeg) >= Class->tResultInfo.size())
      return -1;
  }
#endif

  int lst =  Class->tResultInfo[tLeg].lastStartTime;
  return lst > 0 ? lst + getRunningTime() : lst;
}


pRunner oRunner::getMatchedRunner(const SICard &sic) const {
  if (multiRunner.size() == 0 && tParentRunner == 0)
    return pRunner(this);
  if (!Class)
    return pRunner(this);
  if (Class->getLegType(tLeg) != LTExtra)
    return pRunner(this);

  vector<pRunner> multi = tParentRunner ? tParentRunner->multiRunner : multiRunner;
  multi.push_back( tParentRunner ? tParentRunner : pRunner(this));
  
  int Distance=-1000;
  pRunner r = 0; //Best runner

  for (size_t k = 0; k<multi.size(); k++) {
    if (!multi[k] || multi[k]->Card || multi[k]->getStatus() != StatusUnknown)
      continue;

    if (Class->getLegType(multi[k]->tLeg) != LTExtra)
      return pRunner(this);

    pCourse pc = multi[k]->getCourse(false);
    if (pc) {
			int d = pc->distance(sic);

			if(d>=0) {
				if(Distance<0) Distance=1000;
				if(d<Distance) {
					Distance=d;
					r = multi[k];
				}
			}
			else {
				if(Distance<0 && d>Distance) {
					Distance=d;
			  	r = multi[k];
				}
			}
		}
  }

  if (r)
    return r;
  else
    return pRunner(this);
}

int oRunner::getTotalRunningTime(int time) const {
  if (tStartTime < 0)
    return 0;  
  if (tInTeam == 0 || tLeg == 0) {
    return time-tStartTime + inputTime;
  }
  else {
    if (Class == 0 || unsigned(tLeg) >= Class->legInfo.size())
      return 0;

    if (time == FinishTime) {
      return tInTeam->getLegRunningTime(tLeg, true); // Use the official running time in this case (which works with parallel legs)
    }

    int baseleg = tLeg-1;
    while (baseleg>0 && (Class->legInfo[baseleg+1].isParallel())) {
      baseleg--;
    }

    int leg = baseleg-1;
    while (leg>0 && (Class->legInfo[leg].legMethod == LTExtra || Class->legInfo[leg].legMethod == LTIgnore)) {
      leg--;
    }

    int pt = leg>=0 ? tInTeam->getLegRunningTime(leg, true) : 0;
    if (pt>0)
      return pt + time - tStartTime;
    else if (tInTeam->tStartTime > 0)
      return (time - tInTeam->tStartTime) + tInTeam->inputTime;
    else
      return 0;
  }
}

  // Get the complete name, including team and club.
string oRunner::getCompleteIdentification() const {
  if (tInTeam == 0 || !Class || tInTeam->getName() == Name) {
    if (Club)
      return Name + " (" + Club->name + ")";
    else
      return Name;
  }
  else {
    string names;

    // Get many names for paralell legs
    int firstLeg = tLeg;
    LegTypes lt=Class->getLegType(firstLeg--);
    while(firstLeg>=0 && (lt==LTIgnore || lt==LTParallel || lt==LTParallelOptional || lt==LTExtra) ) 
      lt=Class->getLegType(firstLeg--);

    for (size_t k = firstLeg+1; k < Class->legInfo.size(); k++) {
      pRunner r = tInTeam->getRunner(k);
      if (r) {
        if (names.empty())
          names = r->Name;
        else
          names += "/" + r->Name;
      }
      lt = Class->getLegType(k + 1);
      if ( !(lt==LTIgnore || lt==LTParallel || lt == LTParallelOptional || lt==LTExtra))
        break;
    }

    if (Class->legInfo.size() <= 2) 
      return names + " (" + tInTeam->Name + ")";
    else
      return tInTeam->Name + " (" + names + ")";
  }
}

RunnerStatus oAbstractRunner::getTotalStatus() const {
  if (tStatus == StatusUnknown && inputStatus != StatusNotCompetiting)
    return StatusUnknown;
  else if (inputStatus == StatusUnknown)
    return StatusDNS;

  return max(tStatus, inputStatus);
}

RunnerStatus oRunner::getTotalStatus() const {
  if (tStatus == StatusUnknown && inputStatus != StatusNotCompetiting)
    return StatusUnknown;
  else if (inputStatus == StatusUnknown)
    return StatusDNS;

  if (tInTeam == 0 || tLeg == 0) 
    return max(tStatus, inputStatus);
  else {
    RunnerStatus st = tInTeam->getLegStatus(tLeg-1, true);

    if (st == StatusOK || st == StatusUnknown)
      return tStatus;
    else
      return max(max(tStatus, st), inputStatus);
  }
}

void oRunner::remove() 
{
  if (oe) {
    vector<int> me;
    me.push_back(Id);
    oe->removeRunner(me);
  }
}

bool oRunner::canRemove() const 
{
  return !oe->isRunnerUsed(Id);
}

void oAbstractRunner::setInputTime(const string &time) {
  int t = convertAbsoluteTimeMS(time);
  if (t != inputTime) {
    inputTime = t;
    updateChanged();
  }
}

string oAbstractRunner::getInputTimeS() const {
  if (inputTime > 0)
    return formatTime(inputTime);
  else
    return "-";
}

void oAbstractRunner::setInputStatus(RunnerStatus s) {
  if (inputStatus != s) {
    inputStatus = s;
    updateChanged();
  }
}

string oAbstractRunner::getInputStatusS() const {
  return oe->formatStatus(inputStatus);
}

void oAbstractRunner::setInputPoints(int p)
{
  if (p != inputPoints) {
    inputPoints = p;
    updateChanged();
  }
}

void oAbstractRunner::setInputPlace(int p)
{
  if (p != inputPlace) {
    inputPlace = p;
    updateChanged();
  }
}

void oRunner::setInputData(const oRunner &r) {
  if (!r.multiRunner.empty() && r.multiRunner.back() && r.multiRunner.back() != &r)
    setInputData(*r.multiRunner.back());
  else {
    if (r.tStatus != StatusNotCompetiting) {
  inputTime = r.getTotalRunningTime(r.FinishTime);
  inputStatus = r.getTotalStatus();
      if (r.tInTeam) { // If a team has not status ok, transfer this status to all team members.
        if (r.tInTeam->getTotalStatus() > StatusOK)
          inputStatus = r.tInTeam->getTotalStatus();
      }
  inputPoints = r.getRogainingPoints() + r.inputPoints;
  inputPlace = r.tTotalPlace < 99000 ? r.tTotalPlace : 0;
    }
    else {
      // Copy input
      inputTime = r.inputTime;
      inputStatus = r.inputStatus;
      inputPoints = r.inputPoints;
      inputPlace = r.inputPlace;
    }
  }
}

void oEvent::getDBRunnersInEvent(intkeymap<pClass> &runners) const {
  runners.clear();
  for (oRunnerList::const_iterator it = Runners.begin(); it != Runners.end(); ++it) {
    if (it->isRemoved())
      continue;
    __int64 id = it->getExtIdentifier();
    if (id>0 && id < unsigned(-1))
      runners.insert(int(id), it->Class);
  }
}

void oRunner::init(const RunnerDBEntry &dbr) {
  setTemporary();
  dbr.getName(Name);
  CardNo = dbr.cardNo;
  Club = oe->getRunnerDatabase().getClub(dbr.clubNo);
  getDI().setString("Nationality", dbr.getNationality());
  getDI().setInt("BirthYear", dbr.getBirthYear());
  getDI().setString("Sex", dbr.getSex());
  setExtIdentifier(dbr.getExtId());      
}


void oEvent::selectRunners(const string &classType, int lowAge, 
                           int highAge, const string &firstDate,
                           const string &lastDate, bool includeWithFee,
                           vector<pRunner> &output) const {
  oRunnerList::const_iterator it;
  int cid = 0;
  if (classType.length() > 2 && classType.substr(0,2) == "::")
    cid = atoi(classType.c_str() + 2);

  output.clear();

  int firstD = 0, lastD = 0;
  if (!firstDate.empty()) {
    firstD = convertDateYMS(firstDate);
    if (firstD == -1)
      throw meosException("Felaktigt datumfromat 'X' (Anv�nd ����-MM-DD).#" + firstDate);
  }

  if (!lastDate.empty()) {
    lastD = convertDateYMS(lastDate);
    if (lastD == -1)
      throw meosException("Felaktigt datumfromat 'X' (Anv�nd ����-MM-DD).#" + lastDate);
  }


  bool allClass = classType == "*";
  for (it=Runners.begin(); it != Runners.end(); ++it) {
    if (it->skip()) 
      continue;

    const pClass pc = it->Class;
    if (cid > 0 && (pc == 0 || pc->getId() != cid))
      continue;
    
    if (cid == 0 && !allClass) {
      if ((pc && pc->getType()!=classType) || (pc==0 && !classType.empty())) 
        continue;
    }

    int age = it->getBirthAge();
    if (age > 0 && (lowAge > 0 || highAge > 0)) {
      if (lowAge > highAge)
        throw meosException("Undre �ldersgr�nsen �r h�gre �n den �vre.");

      if (age < lowAge || age > highAge)
        continue;
      /*
      bool ageOK = false;
      if (lowAge > 0 && age <= lowAge)
        ageOK = true;
      else if (highAge > 0 && age >= highAge)
        ageOK = true;

      if (!ageOK)
        continue;*/
    }

    int date = it->getDCI().getInt("EntryDate");
    if (date > 0) {
      if (firstD > 0 && date < firstD)
        continue;
      if (lastD > 0 && date > lastD)
        continue;

    }

    if (!includeWithFee) {
      int fee = it->getDCI().getInt("Fee");
      if (fee != 0)
        continue;
    }
    //    string date = di.getDate("EntryDate");

    output.push_back(pRunner(&*it));
  }
}

void oRunner::changeId(int newId) {
  pRunner old = oe->runnerById[Id];
  if (old == this)
    oe->runnerById.remove(Id);

  oBase::changeId(newId);
  
  oe->runnerById[newId] = this;
}

const vector<SplitData> &oRunner::getSplitTimes(bool normalized) const {
  if (!normalized)
    return splitTimes;
  else {
    pCourse pc = getCourse(true);
    if (pc && pc->isAdapted() && splitTimes.size() == pc->nControls) {
      if (!normalizedSplitTimes.empty())
        return normalizedSplitTimes;
      const vector<int> &mapToOriginal = pc->getMapToOriginalOrder();
      normalizedSplitTimes.resize(splitTimes.size()); // nControls
      vector<int> orderedSplits(splitTimes.size() + 1, -1);

      for (int k = 0; k < pc->nControls; k++) {
        if (splitTimes[k].hasTime()) {
          int t = -1;
          int j = k - 1;
          while (j >= -1 && t == -1) {
            if (j == -1)
              t = getStartTime();
            else if (splitTimes[j].hasTime())
              t = splitTimes[j].time;
            j--;
          }
          orderedSplits[mapToOriginal[k]] = splitTimes[k].time - t;
        }
      }

      // Last to finish
      {
        int t = -1;
        int j = pc->nControls - 1;
        while (j >= -1 && t == -1) {
          if (j == -1)
            t = getStartTime();
          else if (splitTimes[j].hasTime())
            t = splitTimes[j].time;
          j--;
        }
        orderedSplits[mapToOriginal[pc->nControls]] = FinishTime - t;
      }

      int accumulatedTime = getStartTime();
      for (int k = 0; k < pc->nControls; k++) {
        if (orderedSplits[k] > 0) {
          accumulatedTime += orderedSplits[k];
          normalizedSplitTimes[k].setPunchTime(accumulatedTime);
        }
        else
          normalizedSplitTimes[k].setNotPunched();
      }

      return normalizedSplitTimes;
    }
    return splitTimes;
  }
}

void oRunner::markClassChanged(int controlId) {
  if (Class) 
    Class->markSQLChanged(tLeg, controlId);
  else if(oe)
    oe->globalModification = true;
}

void oAbstractRunner::changedObject() {
  markClassChanged(-1);
}
