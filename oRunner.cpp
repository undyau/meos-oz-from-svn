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

// oRunner.cpp: implementation of the oRunner class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "oRunner.h"

#include "oEvent.h"
#include "gdioutput.h"
#include "table.h"
#include "meos_util.h"
#include <cassert>
#include "localizer.h"
#include <cmath>
#include "intkeymapimpl.hpp"


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
oAbstractRunner::oAbstractRunner(oEvent *poe):oBase(poe)
{
  Class=0;
  Club=0;
  StartTime=0;
	FinishTime=0;
	Status=StatusUnknown;

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
	setStartTime(oe->getRelativeTime(t));
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
}

bool oRunner::Write(xmlparser &xml) 
{
	if(Removed) return true;

	xml.startTag("Runner");
	xml.write("Id", Id);
	xml.write("Updated", Modified.GetStamp());
	xml.write("Name", Name);
	xml.write("Start", StartTime);
	xml.write("Finish", FinishTime);
	xml.write("Status", Status);
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
			StartTime = it->getInt();			
		}
		else if(it->is("Finish")){
			FinishTime = it->getInt();			
		}
		else if(it->is("Status")){
			Status=RunnerStatus(it->getInt());	
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
			Modified.SetStamp(it->get());
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

void oAbstractRunner::addClassDefaultFee(bool resetFees) {
  if (Class) {
    oDataInterface di = getDI();
    int date = di.getInt("EntryDate");
    if (date == 0)
      di.setDate("EntryDate", getLocalDate());

    int currentFee = di.getInt("Fee");
    if (currentFee == 0 || resetFees) {
      int fee = Class->getEntryFee(di.getDate("EntryDate"));
      di.setInt("Fee", fee);
    }
  }
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
    apply(false);
    if(Class) {
      oe->reCalculateLeaderTimes(Class->getId());
      Class->clearSplitAnalysis();
      Class->tResultInfo.clear();
    }
    if(pc) {
      oe->reCalculateLeaderTimes(pc->getId());
      pc->clearSplitAnalysis();
      pc->tResultInfo.clear();
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
              oe->removeRunner(multiRunner[k]->Id);
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
        t->setStartNo(StartNo);
        t->setRunner(0, this, true);
      }
    }

    apply(false); //We may get old class back from team.

    for (size_t k=0;k<multiRunner.size();k++) {
      if (multiRunner[k] && Class!=multiRunner[k]->Class) {
		    multiRunner[k]->Class=Class;
		    multiRunner[k]->updateChanged();
      }
    }

    if(Class!=pc && !isTemporaryObject) {
      if (Class) {
        oe->reCalculateLeaderTimes(Class->getId());
        Class->clearSplitAnalysis();
        Class->tResultInfo.clear();
      }
      if (pc) {
        oe->reCalculateLeaderTimes(pc->getId());
        pc->clearSplitAnalysis();
        pc->tResultInfo.clear();
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

void oAbstractRunner::setStartTime(int t)
{
	int OST=StartTime;

	if(t>0)
		StartTime=t;
	else StartTime=0;

  if(OST!=StartTime) {
		updateChanged();
    if (Class) {
      Class->clearSplitAnalysis();
      Class->tResultInfo.clear();
    }
  }
  if(OST<StartTime && Class)
    oe->reCalculateLeaderTimes(Class->getId());
}

void oAbstractRunner::setFinishTime(int t)
{
	int OFT=FinishTime;

	if (t>StartTime)
		FinishTime=t;
	else //Beeb
		FinishTime=0;

  if (OFT != FinishTime) {
		updateChanged();
    if (Class) {
      Class->clearSplitAnalysis();
      Class->tResultInfo.clear();
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

  if (Class && t!=FinishTime)
      Class->clearSplitAnalysis();

  tSplitRevision = 0;

  if(update && t!=FinishTime)
    oe->reCalculateLeaderTimes(Class->getId());
}

string oAbstractRunner::getStartTimeS() const
{
	if(StartTime>0)
		return oe->getAbsTime(StartTime);
	else return "-";
}

string oAbstractRunner::getStartTimeCompact() const
{
	if(StartTime>0)
    if(oe->useStartSeconds())
		  return oe->getAbsTime(StartTime);
    else
      return oe->getAbsTimeHM(StartTime);

	else return "-";
}

string oAbstractRunner::getFinishTimeS() const
{
	if(FinishTime>0)
		return oe->getAbsTime(FinishTime);
	else return "-";
}

string oAbstractRunner::getRunningTimeS() const
{
  return formatTime(getRunningTime());
}

const string &oAbstractRunner::getStatusS() const
{
  return oEvent::formatStatus(Status);
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


string oAbstractRunner::getIOFStatusS() const
{
	switch(Status)
	{
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

void oRunner::addPunches(pCard card, vector<int> &MissingPunches)
{
	Card=card;
	
	updateChanged();

	if(card) {
		if(CardNo==0)
			CardNo=card->CardNo;

		//if(card->CardNo!=CardNo)
		//	oe->Notify("Warning: SI Card number doesn't match: ", CardNo);

    assert(card->tOwner==0 || card->tOwner==this);
	}

  if(Card)
    Card->tOwner=this;

	evaluateCard(MissingPunches, 0, true);
}

pCourse oRunner::getCourse() const
{
	if (Course) return Course;

	if (Class) {		
		if (Class->hasMultiCourse()) {
			const_cast<oRunner*>(this)->apply(false);

      if(tInTeam) {
        unsigned leg=legToRun();
			  if (leg<Class->MultiCourse.size()) {
				  vector<pCourse> &courses=Class->MultiCourse[leg];
				  if(courses.size()>0) {
					  int index=StartNo;
            if(index>0)
              index = (index-1) % courses.size();

					  return courses[index];
				  }
			  }
      }
      else {
			  if (unsigned(tDuplicateLeg)<Class->MultiCourse.size()) {
				  vector<pCourse> &courses=Class->MultiCourse[tDuplicateLeg];
				  if(courses.size()>0) {
					  int index=StartNo % courses.size();
					  return courses[index];
				  }
			  }
      }
		}
		return Class->Course;
	}

	return 0;
}

string oRunner::getCourseName() const
{
	pCourse oc=getCourse();
	if(oc) return oc->getName();
	return "[Ingen bana]";
}

#define NOTATIME 0xF0000000
bool oRunner::evaluateCard(vector<int> & MissingPunches, int addpunch, bool sync)
{
  MissingPunches.clear();
  bool inTeam=apply(sync);
  tProblemDescription.clear();

  vector<int> oldTimes;
  swap(splitTimes, oldTimes);

  if(!Card) {
    if (storeTimes() && Class && sync)
      oe->reEvaluateClass(Class->getId(), sync);

    if (oldTimes.size() > 0 && Class)
      Class->clearSplitAnalysis();
		return false;
  }
	//Try to match class?!
	if(!Class)
		return false;

	pCourse course = getCourse();

  if(!course) {
    storeTimes();
		return false;
  }

  // Pairs: <control index, point>
  intkeymap< pair<int, int> > rogaining(course->getNumControls());
  for (int k = 0; k< course->nControls; k++) {
    if (course->Controls[k] && course->Controls[k]->isRogaining()) {
      int pt = course->Controls[k]->getRogainingPoints();
      if (pt > 0) {
        for (int j = 0; j<course->Controls[k]->nNumbers; j++) {
          rogaining.insert(course->Controls[k]->Numbers[j], make_pair(k, pt));
        }
      }
    }
  }
	oPunchList::iterator p_it;

  if(addpunch && Card->Punches.empty()) {		
    Card->addPunch(addpunch, -1, course->Controls[0] ? course->Controls[0]->getId():0);
  }

	if (Card->Punches.empty()) {
    for(int k=0;k<course->nControls;k++) {
      if (course->Controls[k]) {
        course->Controls[k]->startCheckControl();
        course->Controls[k]->addUncheckedPunches(MissingPunches);
      }
    }
    if (storeTimes() && Class && sync)
      oe->reEvaluateClass(Class->getId(), sync);
		
    if (oldTimes.size() > 0 && Class)
      Class->clearSplitAnalysis();

    return false;
	}

  // Reset rogaining
  for (p_it=Card->Punches.begin(); p_it!=Card->Punches.end(); ++p_it) {
    p_it->tRogainingIndex = -1;
  }

  bool clearSplitAnalysis = false;
	p_it=Card->Punches.begin();
	
  //Search for start and update start time.
  while ( p_it!=Card->Punches.end()) {
	  if (p_it->Type==oPunch::PunchStart) {
		  if(tUseStartPunch && p_it->getAdjustedTime() != StartTime) {
        p_it->setTimeAdjust(0);
			  StartTime = p_it->getAdjustedTime();
        clearSplitAnalysis = true;
			  updateChanged();
		  }
	  }
	  ++p_it;
  }
  inthashmap expectedPunchCount(course->nControls);
  inthashmap punchCount(Card->Punches.size());
  for (int k=0; k<course->nControls; k++) {
    pControl ctrl=course->Controls[k];
    if (ctrl && !ctrl->isRogaining()) {
      for (int j = 0; j<ctrl->nNumbers; j++)
        ++expectedPunchCount[ctrl->Numbers[j]];
    }
  }

  for (p_it = Card->Punches.begin(); p_it != Card->Punches.end(); ++p_it) {
    if(p_it->Type>=10 && p_it->Type<=1024)
      ++punchCount[p_it->Type];
  }

  p_it = Card->Punches.begin();	
  splitTimes.resize(course->nControls, NOTATIME);
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
      if (addpunch && ctrl->isRogaining() && ctrl->getFirstNumber() == addpunch) {
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
      while(!ctrl->controlCompleted() && tp_it!=Card->Punches.end()) {
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
          if (ctrl->controlCompleted())
            splitTimes[k]=tp_it->getAdjustedTime();
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
						if (ctrl->controlCompleted())
              splitTimes[k]=-1;						
					}
					else {
            skippedPunches++;
						tp_it->isUsed=false;
						++tp_it;
					}
				}
			}

      if (tp_it==Card->Punches.end() && !ctrl->controlCompleted() 
                    && ctrl->hasNumberUnchecked(addpunch) ) {
        Card->addPunch(addpunch, -1, ctrl->getId());
				if (ctrl->controlCompleted())
          splitTimes[k]=-1;
				Card->Punches.back().isUsed=true;		
        Card->Punches.back().tMatchControlId=ctrl->getId();
        Card->Punches.back().tIndex = k;
			}

      if(ctrl->controlCompleted() && splitTimes[k]==NOTATIME)
        splitTimes[k]=-1;
		}
		else //if(ctrl && ctrl->Status==oControl::StatusBad){
			splitTimes[k]=-1;

    //Add missing punches
    if(ctrl && !ctrl->controlCompleted())
      ctrl->addUncheckedPunches(MissingPunches);
	}

  //Add missing punches for remaining controls
  while (k<course->nControls) {
    if (course->Controls[k]) {
      pControl ctrl = course->Controls[k];
      ctrl->startCheckControl();

      if (ctrl->hasNumberUnchecked(addpunch)) {
        Card->addPunch(addpunch, -1, ctrl->getId());
				Card->updateChanged();
				if (ctrl->controlCompleted())
          splitTimes[k]=-1;						
			}
      ctrl->addUncheckedPunches(MissingPunches);
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

  // Rogaining logic
  if (rogaining.size() > 0) {
    for (p_it=Card->Punches.begin(); p_it != Card->Punches.end(); ++p_it) {
      pair<int, int> pt;
      if (rogaining.lookup(p_it->Type, pt)) {
        if (pt.second > 0) {
          p_it->isUsed = true;
          p_it->tRogainingIndex = pt.first;
          p_it->tMatchControlId = course->Controls[pt.first]->getId();
          tRogaining.push_back(make_pair(course->Controls[pt.first], p_it->getAdjustedTime()));
          tRogainingPoints += pt.second;
          rogaining[p_it->Type].second = 0; // May noy be revisited
        }
      }
    }

    int point_limit = course->getMinimumRogainingPoints();
    if (point_limit>0 && tRogainingPoints<point_limit) {
      tProblemDescription = "X poäng fattas.#" + itos(point_limit-tRogainingPoints);
      OK = false;
    }

    // Check this later
    time_limit = course->getMaximumRogainingTime();
	}

  for (int k = 1; k<course->nControls; k++) {
    if (course->Controls[k] && course->Controls[k]->isRogaining()) {
      splitTimes[k] = splitTimes[k-1];      
    }
  }

	int OldStatus=Status;
	int OldFT=FinishTime;

	if(OK && (Status==0 || Status==StatusDNS || Status==StatusMP || Status==StatusOK || Status==StatusDNF))
		Status=StatusOK;
	else	Status=RunnerStatus(max(int(StatusMP), int(Status)));

  if (Card->Punches.back().Type==oPunch::PunchFinish) {
		FinishTime=Card->Punches.back().getAdjustedTime();
    Card->Punches.back().tMatchControlId=oPunch::PunchFinish;
  }
	else if (FinishTime<=0) {
		Status=RunnerStatus(max(int(StatusDNF), int(Status)));
    tProblemDescription = "Måltid saknas.";
		FinishTime=0;
	}

  if (!MissingPunches.empty()) {
    tProblemDescription  = "Stämplingar saknas: X#" + itos(MissingPunches[0]);
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
        int reduction = (59 + overTime * course->getRogainingPointsPerMinute()) / 60;
        tProblemDescription = "Tidsavdrag: X poäng.#" + itos(reduction); 
        tRogainingPoints = max(0, tRogainingPoints - reduction);
      }
    }
  }

  if(OldStatus!=Status || OldFT!=FinishTime) {
		updateChanged();
    clearSplitAnalysis = true;
  }

  if(inTeam)
    apply(sync); //Post apply. Update start times.

  if (tCachedRunningTime != FinishTime - StartTime) {
    tCachedRunningTime = FinishTime - StartTime;
    clearSplitAnalysis = true;
  }

  if (Class) {
    // Clear split analysis data if necessary
    bool clear = splitTimes.size() != oldTimes.size() || clearSplitAnalysis;
    for (size_t k = 0; !clear && k<oldTimes.size(); k++) {
      if (splitTimes[k] != oldTimes[k])
        clear = true;
    }
    if (clear)
      Class->clearSplitAnalysis();
  }

  storeTimes();
  if (Class && sync) {
    bool update = false;
    if (tInTeam) {
      int t1 = Class->getTotalLegLeaderTime(tLeg);
      int t2 = tInTeam->getLegRunningTime(tLeg);
      if (t2<=t1)
        update = true;
    }

    if (!update) {
      int t1 = Class->getBestLegTime(tLeg);
      int t2 = getRunningTime();
      if (t2<=t1)
        update = true;
    }
    if (update)
      oe->reEvaluateClass(Class->getId(), sync);
  }
	return true;
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
      it->adjustTimeAdjust(adjustment);
      ++it;
    }

    int minTime = ctrl->getMinTime();
    if (minTime>0) {
      int t = 0;
      if (n>0 && splitTimes[n]>0 && splitTimes[n-1]>0) {
        t = splitTimes[n] - splitTimes[n-1];
        //XXX Start
        int maxadjust = max(minTime-t, 0);
        adjustment += maxadjust;
      }
    }

    if (it != Card->Punches.end() && it->tMatchControlId == ctrl->getId()) {
      it->adjustTimeAdjust(adjustment);
      ++it;
    }

    adjustTimes[n] = adjustment;
    if (splitTimes[n]>0)
      splitTimes[n] += adjustment;
  }
  
  // Adjust remaining
  while (it != Card->Punches.end()) {
    it->adjustTimeAdjust(adjustment);
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
      while(firstLeg>=0 && Class->legInfo[firstLeg].isOptional())
        firstLeg--;
      int nleg = Class->legInfo.size();
      while(lastLeg<nleg && Class->legInfo[lastLeg].isOptional())
        lastLeg++;

      for (int leg = firstLeg; leg<lastLeg; leg++) {
        if (Status==StatusOK) {
          int &bt=Class->tLeaderTime[leg].first;
          int rt=getRunningTime(); 
          if (bt==0 || rt<bt) {
            bt=rt;
            updated = true;
          }
        }
        if (tInTeam->getLegStatus(tLeg)==StatusOK) {
          int &bt=Class->tLeaderTime[leg].second;      
          int rt=tInTeam->getLegRunningTime(tLeg);
          if(rt>0 && (bt==0 || rt<bt)) {
            bt=rt;
            updated = true;
          }
        }
      }
    }
  }
  else {
    if (Class && unsigned(tDuplicateLeg)<Class->tLeaderTime.size()) {
      if (Status==StatusOK) {
        int &bt=Class->tLeaderTime[tDuplicateLeg].first;
        int rt=getRunningTime(); 
        if (bt==0 || rt<bt) {
          bt=rt;
          updated = true;
        }
      }
      int &bt=Class->tLeaderTime[tDuplicateLeg].second;
      int rt=getRaceRunningTime(tDuplicateLeg);
      if(rt>0 && (bt==0 || rt<bt)) {
        bt=rt;
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
    if(statusOK())
      return getRunningTime();
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
            return max(r->getFinishTime()-StartTime, dt); // ### Luckor, jaktstart???
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
  else if(a.tStatus != b.tStatus)
	  return a.tStatus<b.tStatus;
  else {
	  if(a.tStatus==StatusOK) {
		  if(a.tRT!=b.tRT)
			  return a.tRT<b.tRT;
	  }
	  return a.Name<b.Name;
  }		
}

bool oRunner::operator <(const oRunner &c)
{
  if(!Class || !c.Class) 
    return Class<c.Class;

	if(oe->CurrentSortOrder==ClassStartTime) {
    if(Class->Id != c.Class->Id)
			return Class->tSortIndex < c.Class->tSortIndex;
    else if (StartTime != c.StartTime) { 
      if (StartTime <= 0 && c.StartTime > 0)
        return false;
      else if (c.StartTime <= 0 && StartTime > 0)
        return true;
      else return StartTime < c.StartTime;
    }
		else if(Name<c.Name)
			return true;
		else return false;
	}
	else if(oe->CurrentSortOrder==ClassResult) {
		if(Class->Id != c.Class->Id)
			return Class->tSortIndex < c.Class->tSortIndex;
    else if (tDuplicateLeg != c.tDuplicateLeg)
      return tDuplicateLeg < c.tDuplicateLeg;
		else if(Status != c.Status)
			return Status < c.Status;
		else {
			if(Status==StatusOK) {
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
		if(Status < c.Status)
			return true;
		else if(Status > c.Status)
			return false;
		else{
			if(Status==StatusOK && FinishTime!=c.FinishTime)
				return FinishTime<c.FinishTime;
			else if(Name<c.Name)
				return true;
			else return false;
		}
	}
	else if(oe->CurrentSortOrder==SortByClassFinishTime){
    if(Class->Id != c.Class->Id)
			return Class->tSortIndex < c.Class->tSortIndex;
		if(Status < c.Status)
			return true;
		else if(Status > c.Status)
			return false;
		else{
			if(Status==StatusOK && FinishTime!=c.FinishTime)
				return FinishTime<c.FinishTime;
			else if(Name<c.Name)
				return true;
			else return false;
		}
	}
	else if(oe->CurrentSortOrder==SortByStartTime){
		if(StartTime < c.StartTime)
			return true;
		else  if(StartTime > c.StartTime)
			return false;
		else if(Name<c.Name)
			return true;
		else return false;
	}
  else if(oe->CurrentSortOrder == SortByPoints){
		if(Class->Id != c.Class->Id)
			return Class->tSortIndex < c.Class->tSortIndex;
    else if (tDuplicateLeg != c.tDuplicateLeg)
      return tDuplicateLeg < c.tDuplicateLeg;
		else if(Status != c.Status)
			return Status < c.Status;
		else {
			if (Status==StatusOK) {
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
    RunnerStatus s1, s2;
		if(Class->Id != c.Class->Id)
			return Class->tSortIndex < c.Class->tSortIndex;
    else if (tDuplicateLeg != c.tDuplicateLeg)
      return tDuplicateLeg < c.tDuplicateLeg;
    else if((s1 = getTotalStatus()) != (s2 = c.getTotalStatus()))
			return s1 < s2;
		else {
			if(Status==StatusOK) {
        if (Class->getNoTiming())
          return Name<c.Name;

        int t = getTotalRunningTime(FinishTime);
				int ct = c.getTotalRunningTime(c.FinishTime);
				if (t!=ct)
					return t < ct;
			}			
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


void oAbstractRunner::setStartNo(int no)
{
	if(no!=StartNo) {
		StartNo=no;
		updateChanged();
	}
}

void oRunner::setStartNo(int no)
{
  if (tParentRunner)
    tParentRunner->setStartNo(no);
  else { 
    oAbstractRunner::setStartNo(no);

    for (size_t k=0;k<multiRunner.size();k++) 
      if(multiRunner[k])
        multiRunner[k]->oAbstractRunner::setStartNo(no);
	}
}

int oRunner::getPlace() const
{
	return tPlace;
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

string oRunner::getGivenName() const
{
	return trim(Name.substr(0, Name.find_first_of(" \xA0")));
}

string oRunner::getFamilyName() const
{
  int k = Name.find_first_of(" \xA0");

  if(k==string::npos)
    return "";
  else return trim(Name.substr(k, string::npos));
}

void oRunner::setCardNo(int cno, bool matchCard)
{
	if(cno!=CardNo){
		CardNo=cno;

    if(matchCard && !Card) {
      pCard c=oe->getCardByNumber(cno);

      if(c && !c->tOwner) {
        vector<int> mp;
        addPunches(c, mp);
      }
    }

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
        otherR->setStatus(StatusUnknown);
        otherR->synchronize(true);
      }
      c->tOwner=this;
      CardNo=c->CardNo;
    }
    Card=c;
    vector<int> mp;
    evaluateCard(mp);
		updateChanged();
    synchronize(true);
	}
  return oldId;
}

void oAbstractRunner::setName(const string &n)
{
  if (trim(n).empty())
    throw std::exception("Tomt namn är inte tillåtet.");
	if(n!=Name){
		Name=trim(n);
		updateChanged();
	}
}

void oRunner::setName(const string &n)
{
  if (trim(n).empty())
    throw std::exception("Tomt namn är inte tillåtet.");

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


void oAbstractRunner::setStatus(RunnerStatus st)
{
	if(Status!=st){
		Status=st;
		updateChanged();

    if (Class) {
      Class->clearSplitAnalysis();
      Class->tResultInfo.clear();
      oe->reCalculateLeaderTimes(Class->getId());
    }
	}
}

int oAbstractRunner::getPrelRunningTime() const
{
  if(FinishTime>0 && Status!=StatusDNS && Status!=StatusDNF) 
		return getRunningTime();
  else if(Status==StatusUnknown)
	  return oe->ComputerTime-StartTime;
	else return 0;
}

string oAbstractRunner::getPrelRunningTimeS() const
{
	int rt=getPrelRunningTime();
	return formatTime(rt);
}

oDataInterface oRunner::getDI(void) 
{
	return oe->oRunnerData->getInterface(oData, sizeof(oData), this);
}

oDataConstInterface oRunner::getDCI(void) const
{
	return oe->oRunnerData->getConstInterface(oData, sizeof(oData), this);
}

void oEvent::getRunners(int classId, vector<pRunner> &r) {
  synchronizeList(oLRunnerId);
  sortRunners(SortByName);

  r.clear();
  if (Classes.size() > 0)
    r.reserve((Runners.size()*min<size_t>(Classes.size(), 4)) / Classes.size());

  for (oRunnerList::iterator it = Runners.begin(); it != Runners.end(); ++it) {
    if (classId == 0 || it->getClassId() == classId)
      r.push_back(&*it);
  }
}

pRunner oEvent::getRunner(int Id, int stage)
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

pRunner oRunner::nextNeedReadout() const
{
  if(!Card || Card->CardNo!=CardNo)
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

pRunner oEvent::getRunnerByCard(int CardNo, bool OnlyWithNoCard) const
{
	oRunnerList::const_iterator it;	

  if (!OnlyWithNoCard && !cardHash.empty()) {
    pRunner r;
    if(cardHash.lookup(CardNo, r) && r) {
      assert(r->CardNo == CardNo);
      return r;
    } 
    else
      return 0;
  }
	//First try runners with no card read or a different card read.
	for (it=Runners.begin(); it != Runners.end(); ++it) {
    if (it->skip())
      continue;

		if(it->CardNo==CardNo && it->nextNeedReadout())	
      return it->nextNeedReadout();
	}

	if (!OnlyWithNoCard) 	{
		//Then try all runners.
		for (it=Runners.begin(); it != Runners.end(); ++it){
      if(!it->isRemoved() && it->CardNo==CardNo) {	
        pRunner r = it->nextNeedReadout();
        return r ? r : pRunner(&*it);
      }
		}
	}

	return 0;
}

pRunner oEvent::getRunnerByStartNo(int startNo) const
{
  // Several runners can have the same bib, try to get the right one.
  // when looking up for manual speaker times.

	oRunnerList::const_iterator it;	

  // First try via teams (get first non-finished runner in team)
  oTeamList::const_iterator tit;	

  for (tit=Teams.begin(); tit!=Teams.end(); ++tit) {
    if (tit->skip())
      continue;

    if (tit->getStartNo()==startNo) {
      const oTeam &t=*tit;

      for (int leg=0; leg<t.getNumRunners(); leg++) {
        if(t.Runners[leg] && t.Runners[leg]->getCardNo() > 0 && t.Runners[leg]->getStatus()==StatusUnknown)
          return t.Runners[leg];
      }
    }
  }

	//Then try normal runners
	for (it=Runners.begin(); it != Runners.end(); ++it) {
    if (it->skip())
      continue;

    if (it->getStartNo()==startNo) {
      if(it->getNumMulti()==0 || it->Status==StatusUnknown)
        return pRunner(&(*it));
      else {
        for(int race=0;race<it->getNumMulti();race++) {
          pRunner r=it->getMultiRunner(race);
          if (r && r->Status==StatusUnknown)
            return r;
        }
        return pRunner(&(*it));
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

void oEvent::fillRunners(gdioutput &gdi, const string &id, bool longName, bool showAll)
{
  vector< pair<string, size_t> > d;
  oe->fillRunners(d, longName, showAll);
  gdi.addItem(id, d);
}


const vector< pair<string, size_t> > &oEvent::fillRunners(vector< pair<string, size_t> > &out, 
                                                          bool longName, bool showAll) 
{	
	synchronizeList(oLRunnerId);
	oRunnerList::iterator it;	
  int lVacId = getVacantClub();
	CurrentSortOrder=SortByName;
	Runners.sort();
  out.clear();
	//gdi.clearList(name);
  char bf[512];

  if(longName) {
    for (it=Runners.begin(); it != Runners.end(); ++it) {
      if(!it->skip() || (showAll && !it->isRemoved())) {
        sprintf_s(bf, "%s\t%s\t%s", it->getNameAndRace().c_str(),
                                          it->getClass().c_str(),
                                          it->getClub().c_str());

        //gdi.addItem(name, bf, it->Id);
        out.push_back(make_pair(bf, it->Id));
      }
	  }
  }
  else {
	  for (it=Runners.begin(); it != Runners.end(); ++it) {
      if(!it->skip() || (showAll && !it->isRemoved())) {
        if( it->getClubId() != lVacId )
          out.push_back(make_pair(it->Name, it->Id));
			    //gdi.addItem(name, it->Name, it->Id);
        else {
          sprintf_s(bf, "%s (%s)", it->Name.c_str(), it->getClass().c_str());
          out.push_back(make_pair(bf, it->Id));
          //gdi.addItem(name, bf, it->Id);
        }
      }
	  }
  }
	return out;
}

void oRunner::resetPersonalData()
{
	getDI().initData();
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
    while(!toRemove.empty()) {
      int id = toRemove.back();
      toRemove.pop_back();
      oe->removeRunner(id);
    }
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

bool oRunner::apply(bool sync) 
{
  createMultiRunner(false, sync);
	tLeg=-1;
  tUseStartPunch=true;
	if(tInTeam)
		tInTeam->apply(sync, this);
  else {
    if (Class && Class->hasMultiCourse()) {
      pClass pc=Class;
      StartTypes st=pc->getStartType(tDuplicateLeg);
      //LegTypes lt=pc->getLegType(tDuplicateLeg);

      if (st==STTime) {
        setStartTime(pc->getStartData(tDuplicateLeg));
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

        setStartTime(lastStart);
        tUseStartPunch=false; 
      }
      else if (st==STHunting) {
        pRunner r=getPredecessor();
        int lastStart=0;

        if (r && r->FinishTime>0 && r->statusOK()) {
          int rt=r->getRaceRunningTime(tDuplicateLeg-1);
          int timeAfter=rt-pc->getTotalLegLeaderTime(r->tDuplicateLeg);
          if(rt>0 && timeAfter>=0)
            lastStart=pc->getStartData(tDuplicateLeg)+timeAfter;
        }
        int restart=pc->getRestartTime(tDuplicateLeg);
        int rope=pc->getRopeTime(tDuplicateLeg);

        if (restart && rope && (lastStart>rope || lastStart==0))
          lastStart=restart; //Runner in restart

        setStartTime(lastStart);
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
    setStartTime(r->getStartTime());

    for (size_t k=0; k < min(multiRunner.size(), r->multiRunner.size()); k++) {
      if (multiRunner[k]!=0 && r->multiRunner[k]!=0) 
        multiRunner[k]->setStartTime(r->multiRunner[k]->getStartTime());
    }
    apply(false);
  }    
}


Table *oEvent::getRunnersTB()//Table mode
{	
  if (tables.count("runner") == 0) {
	  Table *table=new Table(this, 20, "Deltagare", "runners");
	  
    table->addColumn("Id", 70, false);
    table->addColumn("Ändrad", 70, false);

	  table->addColumn("Namn", 200, false);
	  table->addColumn("Klass", 120, false);
    table->addColumn("Bana", 120, false);
	
	  table->addColumn("Klubb", 120, false);
    table->addColumn("Lag", 120, false);
    table->addColumn("Sträcka", 70, true);
  
    table->addColumn("SI", 70, true, false);

    table->addColumn("Start", 70, false, true);
    table->addColumn("Mål", 70, false, true);
    table->addColumn("Status", 70, false);
    table->addColumn("Tid", 70, false, true);
    table->addColumn("Plac.", 70, true, true);

    oe->oRunnerData->buildTableCol(table);

    table->addColumn("Tid in", 70, false, true);
    table->addColumn("Status in", 70, false, true);
    table->addColumn("Poäng in", 70, true);
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
  table.set(row++, it, TID_CLUB, getClub(), true, cellSelection);

  table.set(row++, it, TID_TEAM, tInTeam ? tInTeam->getName() : "", false);
  table.set(row++, it, TID_LEG, tInTeam ? itos(tLeg+1) : "" , false);

  int cno = getCardNo();
  table.set(row++, it, TID_CARD, cno>0 ? itos(cno) : "", true);

  table.set(row++, it, TID_START, getStartTimeS(), true);
  table.set(row++, it, TID_FINISH, getFinishTimeS(), true);
  table.set(row++, it, TID_STATUS, getStatusS(), true, cellSelection);
  table.set(row++, it, TID_RUNNINGTIME, getRunningTimeS(), false);

  table.set(row++, it, TID_PLACE, getPlaceS(), false);

  row = oe->oRunnerData->fillTableCol(oData, it, table);

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
    bool b = oe->oRunnerData->inputData(this, oData, id, input, output, noUpdate);
    if (b) {
      synchronize(true);
      oe->oRunnerData->inputData(this, oData, id, input, output, noUpdate);
    }
    return b;
  }
  switch(id) {
    case TID_CARD:
      setCardNo(atoi(input.c_str()), true);
      synchronizeAll();
      output = itos(getCardNo());
      return true;
    case TID_RUNNER:
      if (trim(input).empty())
        throw std::exception("Tomt namn inte tillåtet.");

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
      evaluateCard(mp);
      s=getStartTime();
      if (s!=t) 
        throw std::exception("Starttiden är definerad genom klassen eller löparens startstämpling.");
      synchronize(true);
      output=getStartTimeS();
      return true;
    break;

    case TID_FINISH:
      setFinishTimeS(input);
      t=getFinishTime();      
      evaluateCard(mp);
      s=getFinishTime();
      if (s!=t) 
        throw std::exception("För att ändra måltiden måste löparens målstämplingstid ändras.");
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
        pClub pc = oe->getClub(inputId);
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
      setStatus(RunnerStatus(inputId));
      int s = getStatus();
      evaluateCard(mp);
      if (s!=getStatus())
        throw std::exception("Status matchar inte data i löparbrickan.");
      synchronize(true);
      output = getStatusS();
    }
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
  if (id==TID_COURSE) {
    oe->fillCourses(out, true);
    //gdi.addItem(controlId, lang.tl("Klassens bana"), 0);
    out.push_back(make_pair(lang.tl("Klassens bana"), 0));
    selected = getCourseId();
    //gdi.selectItemByData(controlId.c_str(), Course ? Course->getId() : 0);
  }
  else if (id==TID_CLASSNAME) {
    oe->fillClasses(out, oEvent::extraNone, oEvent::filterNone);
    //gdi.addItem(controlId, lang.tl("Ingen klass"), 0);
    out.push_back(make_pair(lang.tl("Ingen klass"), 0));
    selected = getClassId();
    //gdi.selectItemByData(controlId.c_str(), getClassId());
  }  
  else if (id==TID_CLUB) {
    oe->fillClubs(out);
    out.push_back(make_pair(lang.tl("Klubblös"), 0));
    selected = getClubId();
    
    //gdi.addItem(controlId, lang.tl("Klubblös"), 0);
    //gdi.selectItemByData(controlId.c_str(), getClubId());
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

int oRunner::getSplitTime(int controlNumber) const
{
  if (controlNumber>0 && controlNumber==splitTimes.size() && FinishTime>0) {
    int t = splitTimes.back();
    if (t >0)      
      return max(FinishTime - t, -1);
  }
  else if ( unsigned(controlNumber)<splitTimes.size() ) {
    if(controlNumber==0)
      return (StartTime>0 && splitTimes[0]>0) ? max(splitTimes[0]-StartTime, -1) : -1;
    else if (splitTimes[controlNumber]>0 && splitTimes[controlNumber-1]>0)
      return max(splitTimes[controlNumber] - splitTimes[controlNumber-1], -1);
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
  pCourse crs=getCourse();
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
      if (splitTimes[controlNumber]>0 && splitTimes[k]>0)
        return max(splitTimes[controlNumber] - splitTimes[k], -1);
      else return -1;
    }
    k--;
  }

  //Measure from start time
  if(splitTimes[controlNumber]>0) 
    return max(splitTimes[controlNumber]-StartTime, -1);

  return -1;
}

string oRunner::getSplitTimeS(int controlNumber) const
{
  return formatTime(getSplitTime(controlNumber));
}

string oRunner::getNamedSplitS(int controlNumber) const
{
  return formatTime(getNamedSplit(controlNumber));
}

int oRunner::getPunchTime(int controlNumber) const
{
  if ( unsigned(controlNumber)<splitTimes.size() ) {
    if(splitTimes[controlNumber]>0)
      return splitTimes[controlNumber]-StartTime;
    else return -1;
  }
  else if ( unsigned(controlNumber)==splitTimes.size() )
    return FinishTime-StartTime;

  return -1;
}

string oRunner::getPunchTimeS(int controlNumber) const
{
  return formatTime(getPunchTime(controlNumber));
}

bool oAbstractRunner::isVacant() const
{
  return getClubId()==oe->getVacantClubIfExist();
}

bool oRunner::needNoCard() const
{
  const_cast<oRunner*>(this)->apply(false);
  return tNeedNoCard;
}

void oRunner::getSplitTime(int controlId, RunnerStatus &stat, int &rt) const
{
  rt = 0;
  stat = StatusUnknown;
  int cardno = tParentRunner ? tParentRunner->CardNo : CardNo;
  if (controlId==oPunch::PunchFinish && 
      FinishTime>0 && Status!=StatusUnknown) {
    stat=Status;
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
    if(controlId==oPunch::PunchFinish && Status!=StatusUnknown)
      stat = Status;
	}
  rt-=StartTime;

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

  int bib=getBib();
  char name[256];
  if(bib>0) {
    sprintf_s(name, "%d %s", bib, getName().c_str());
    spk.name=name;
  }
  else
    spk.name=getName();

  spk.club=getClub();
  spk.finishStatus=getStatus();

  spk.startTimeS=getStartTimeCompact();
  spk.missingStartTime = StartTime<=0;
  
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

pRunner oEvent::findRunner(const string &s, int lastId) const
{
  int len=s.length();
  oRunnerList::const_iterator itstart=Runners.begin();

  if(lastId) 
    for (; itstart != Runners.end(); ++itstart) 
      if(itstart->Id==lastId) {
        ++itstart;
        break;  
      }

  if(!len)
    return 0;

  int sn=atoi(s.c_str());
  oRunnerList::const_iterator it;

  if (sn>0) {
     for (it=itstart; it != Runners.end(); ++it) {  
       if (it->skip())
         continue;
       if(it->StartNo==sn || it->CardNo==sn)
        return pRunner(&*it);
    }
    for (it=Runners.begin(); it != itstart; ++it) { 
      if (it->skip())
         continue;
      if(it->StartNo==sn || it->CardNo==sn)
        return pRunner(&*it);
    }
  }

  char s_lc[1024];
  strcpy_s(s_lc, s.c_str());
  CharLowerBuff(s_lc, len);

  for (it=itstart; it != Runners.end(); ++it) {  
    if (it->skip())
       continue;
    if(filterMatchString(it->Name, s_lc))
      return pRunner(&*it);
  }
  for (it=Runners.begin(); it != itstart; ++it) {  
    if (it->skip())
      continue;
    if(filterMatchString(it->Name, s_lc))
      return pRunner(&*it);
  }
  return 0;
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

  return t-Class->getTotalLegLeaderTime(leg);
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

int oRunner::getBib() const
{
  return getDCI().getInt("Bib");
}

void oRunner::setBib(int bib, bool updateStartNo)
{
  if (tParentRunner)
    tParentRunner->setBib(bib, updateStartNo);
  else {
    if (updateStartNo)
      setStartNo(bib); // Updates multi too.
    getDI().setInt("Bib", bib);
    
    for (size_t k=0;k<multiRunner.size();k++) 
      if(multiRunner[k]) {
        multiRunner[k]->getDI().setInt("Bib", bib);
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

static int findNextControl(const list<pControl> &ctrl, int startIndex, int id, int &offset)
{
  list<pControl>::const_iterator it=ctrl.begin();
  int index=0;
  offset = 1;
  while(startIndex>0 && it!=ctrl.end()) {
    int multi = (*it)->getNumMulti();
    offset += multi-1;
    ++it, --startIndex, ++index;
    if ((*it)->isRogaining())
      index--;
  }

  while(it!=ctrl.end() && (*it) && (*it)->getId()!=id) {
    int multi = (*it)->getNumMulti();
    offset += multi-1;
    ++it, ++index;
    if ((*it)->isRogaining())
      index--;
  }

  if (it==ctrl.end())
    return -1;
  else
    return index;
}

void oRunner::printSplits(gdioutput &gdi) const {
  gdi.setCX(10);
  gdi.fillDown();
  gdi.addStringUT(boldText, oe->getName());
  gdi.addStringUT(fontSmall, oe->getDate());
  gdi.dropLine(0.5);
  
  gdi.addStringUT(boldSmall, getName() + ", " + getClass());
  gdi.addStringUT(fontSmall, getClub());
  gdi.dropLine(0.5);
  gdi.addStringUT(fontSmall, lang.tl("Start: ") + getStartTimeS() + lang.tl(", Mål: ") + getFinishTimeS());
  gdi.addStringUT(fontSmall, lang.tl("Status: ") + getStatusS() + lang.tl(", Tid: ") + getRunningTimeS());
  int cy = gdi.getCY()+4;
  int cx = gdi.getCX();
//  int pnr = 1;
  const int c1=35;
  const int c2=95;
  const int c3=150;
  const int c4=235;
  const int width = 68;
  char bf[256];
  int lastIndex = -1;
  int adjust = 0;
  int offset = 1;

  bool moreThanHour = getRunningTime()>3600;
  list<pControl> ctrl;
  pCourse pc = getCourse();
  if (pc)
    pc->getControls(ctrl);
  
  if (Card) {
    oPunchList &p=Card->Punches;
    for (oPunchList::iterator it=p.begin();it!=p.end();++it) {
      if (it->tRogainingIndex>=0) { 
        const pControl c = pc->getControl(it->tRogainingIndex);
        string point = c ? itos(c->getRogainingPoints()) + "p." : "";

        gdi.addStringUT(cy, cx + c1, fontSmall, point);
        
        sprintf_s(bf, "%d", it->Type);
        gdi.addStringUT(cy, cx, fontSmall, bf);
        int st = Card->getSplitTime(getStartTime(), &*it);
        
        if (st>0)
          gdi.addStringUT(cy, cx + c2, fontSmall, formatTime(st));

        gdi.addStringUT(cy, cx+c3, fontSmall, it->getTime());

        int pt = it->getAdjustedTime();
        st = getStartTime();
        if (st>0 && pt>0 && pt>st) {
          string punchTime = formatTime(pt-st);
          if (!moreThanHour)
            gdi.addStringUT(cy, cx+c4, fontSmall, punchTime);
          else
            gdi.addStringUT(cy, cx+c4+width, fontSmall|textRight, punchTime);
        }

        cy+=int(gdi.getLineHeight()*0.9);
        continue;
      }

      int cid = it->tMatchControlId;
      string punchTime; 
      if (it->isFinish()) {
        gdi.addString("", cy, cx+c1, fontSmall, "Mål");
        int sp = getSplitTime(splitTimes.size());
        if (sp>0) {
          gdi.addStringUT(cy, cx+c2, fontSmall, formatTime(sp));
          punchTime = formatTime(getRunningTime());
        }
        gdi.addStringUT(cy, cx+c3, fontSmall, oe->getAbsTime(it->Time + adjust));

        if (!punchTime.empty()) {
          if (!moreThanHour)
            gdi.addStringUT(cy, cx+c4, fontSmall, punchTime);
          else
            gdi.addStringUT(cy, cx+c4+width, fontSmall|textRight, punchTime);
        }
        cy+=gdi.getLineHeight();
      }
      else if (it->Type>10) { //Filter away check and start
        int index = -1;
        if (cid>0)
          index = findNextControl(ctrl, lastIndex+1, cid, offset);
        if (index>=0) {
          if (index > lastIndex + 1) {
            int xx = cx;
            string str = MakeDash("-");
            int posy = cy-int(gdi.getLineHeight()*0.4);
            const int endx = cx+c4 + (moreThanHour ? width : 50);

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
          int sp = getSplitTime(it->tIndex);
          if (sp>0) {
            punchTime = getPunchTimeS(it->tIndex);
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
        gdi.addStringUT(cy, cx+c3, fontSmall, oe->getAbsTime(it->Time + adjust));

        if (!punchTime.empty()) {
          if (!moreThanHour)
            gdi.addStringUT(cy, cx+c4, fontSmall, punchTime);
          else
            gdi.addStringUT(cy, cx+c4+width, fontSmall|textRight, punchTime);
        }
        cy+=int(gdi.getLineHeight()*0.9);
      }
    }

    vector<string> misses;
    for (size_t k = 0; k<ctrl.size(); k++) {
      int missed = getMissedTime(k);
      if (missed>0) {
        misses.push_back(pc->getControlOrdinal(k) + ". " + formatTime(missed)); 
      }
    }
    if (misses.size()==0) {
      gdi.dropLine();
      gdi.addString("", fontSmall, "Bomfritt lopp / underlag saknas");
    }
    else {
      gdi.dropLine();
      string out = lang.tl("Bommade kontroller: ");
      for (size_t k = 0; k<misses.size(); k++) {
        if (out.length() > 30) {
          gdi.addStringUT(fontSmall, out);
          out.clear();
        }
        out += misses[k];
        if(k < misses.size()-1)
          out += " | ";
      }
      gdi.addStringUT(fontSmall, out);
    }
  }
  gdi.dropLine();
  gdi.addString("", fontSmall, "Av MeOS: www.melin.nu/meos");
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

	for (it=Runners.begin(); it != Runners.end(); ++it) {
    if (!it->isVacant() && !it->isRemoved())
      it->updateFromDB(it->Name, it->getClubId(), it->getClassId(), it->getCardNo(), it->getBirthYear());
  }
}

bool oRunner::updateFromDB(const string &name, int clubId, int classId, 
                           int cardNo, int birthYear) {
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

void oRunner::setSex(const string &sex) 
{
  getDI().setString("Sex", sex);
}

string oRunner::getSex() const 
{
  return getDCI().getString("Sex");
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
  if (Class == 0 || getBib()!=0)
    return getBib()>0;

  int maxbib = 0;
  for(oRunnerList::iterator it = oe->Runners.begin(); it !=oe->Runners.end();++it)
    if (it->Class == Class)
      maxbib = max(it->getBib(), maxbib);

  if (maxbib>0) {
    setBib(maxbib+1, true);
    return true;
  }
  return false;
}

void oRunner::getSplitAnalysis(vector<int> &deltaTimes) const 
{
  deltaTimes.clear();
  vector<int> mp;

  if (splitTimes.empty() || !Class)
    return;

  if (Class->tSplitRevision == tSplitRevision)
    deltaTimes = tMissedTime;

  pCourse pc = getCourse();
  if (!pc)
    return;
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
    res[k] = getSplitTime(k);
    if (res[k] > 0) {
      resSum += res[k];
      baseSum += baseLine[k];
    }
    bestTime += baseLine[k];
  }

  deltaTimes.resize(nc+1);

  // Adjust expected time by removing mistakes
  for (size_t k = 0; k <= nc; k++) {
    if (res[k]  > 0) {
      double part = res[k]*baseSum/(resSum * bestTime);
      double delta = part - baseLine[k] / bestTime;
      int deltaAbs = int(floor(delta * resSum + 0.5));
      if (res[k]-deltaAbs < baseLine[k])
        deltaAbs = int(res[k] - baseLine[k]);

      if (deltaAbs>0)
        resSum -= deltaAbs;
    }
  }

  for (size_t k = 0; k <= nc; k++) {
    if (res[k]  > 0) {
      double part = res[k]*baseSum/(resSum * bestTime);
      double delta = part - baseLine[k] / bestTime;
      
      int deltaAbs = int(floor(delta * resSum + 0.5));

      if (deltaAbs > 0) {
        if ( fabs(delta) > 1.0/100 && (20.0*deltaAbs)>res[k] && deltaAbs>=15)
          deltaTimes[k] = deltaAbs;

        res[k] -= deltaAbs;
        if (res[k] < baseLine[k])
          res[k] = baseLine[k];
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
      double delta = part - baseLine[k] / bestTime;
      int deltaAbs = int(floor(delta * resSum + 0.5));

      if (deltaTimes[k]==0 && fabs(delta) > 1.0/100 && deltaAbs>=15)
        deltaTimes[k] = deltaAbs;
    }
  }
}

void oRunner::getLegPlaces(vector<int> &places) const
{
  places.clear();
  pCourse pc = getCourse();
  if (!pc || !Class || splitTimes.empty())
    return;
  if (Class->tSplitRevision == tSplitRevision)
    places = tPlaceLeg;

  int id = pc->getId();

  if (Class->tSplitAnalysisData.count(id) == 0)
    Class->calculateSplits();
 
  const unsigned nc = pc->getNumControls();

  places.resize(nc+1);
  for (unsigned k = 0; k<=nc; k++) {
    pControl to = 0;
    if (k<nc)
      to = pc->getControl(k);
    pControl from = 0;
    if (k>0)
      from = pc->getControl(k-1);

    int time = getSplitTime(k);

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
  if (Class->tSplitRevision == tSplitRevision)
    times = tAfterLeg;

  pCourse pc = getCourse();
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
    int s = getSplitTime(k);

    if (s>0) {
      times[k] = s - leaders[k];
      if (times[k]<0)
        times[k] = -1;
    }
    else
      times[k] = -1;
  }
}

void oRunner::getLegTimeAfterAcc(vector<int> &times) const 
{
  times.clear();
  if (splitTimes.empty() || !Class || StartTime<=0)
    return;
  if (Class->tSplitRevision == tSplitRevision)
    times = tAfterLegAcc;

  pCourse pc = getCourse();
  if (!pc)
    return;

  int id = pc->getId();

  if (Class->tCourseAccLegLeaderTime.count(id) == 0)
    Class->calculateSplits();
 
  const unsigned nc = pc->getNumControls();

  const vector<int> leaders = Class->tCourseAccLegLeaderTime[id];
  
  if (leaders.size() != nc + 1)
    return;

  times.resize(nc+1);

  for (unsigned k = 0; k<=nc; k++) {
    int s = 0;
    if (k < splitTimes.size())
      s = splitTimes[k];
    else if (k==nc)
      s = FinishTime;

    if (s>0) {
      times[k] = s - StartTime - leaders[k];
      if (times[k]<0)
        times[k] = -1;
    }
    else
      times[k] = -1;
  }
}

void oRunner::getLegPlacesAcc(vector<int> &places) const
{
  places.clear();
  pCourse pc = getCourse();
  if (!pc || !Class)
    return;
  if (splitTimes.empty() || StartTime<=0)
    return;
  if (Class->tSplitRevision == tSplitRevision)
    places = tPlaceLegAcc;

  int id = pc->getId();
  const unsigned nc = pc->getNumControls();

  places.resize(nc+1);
  for (unsigned k = 0; k<=nc; k++) {
    int s = 0;
    if (k < splitTimes.size())
      s = splitTimes[k];
    else if (k==nc)
      s = FinishTime;

    if (s>0) {
      int time = s - StartTime;

      if (time>0)
        places[k] = Class->getAccLegPlace(id, k, time); 
      else
        places[k] = 0;
    }
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
  if (unsigned(ctrlNo) < tLegPlaces.size())
    return tLegPlaces[ctrlNo];
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

    pCourse pc = multi[k]->getCourse();
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
  if (StartTime < 0)
    return 0;  
  if (tInTeam == 0 || tLeg == 0) {
    return time-StartTime + inputTime;
  }
  else {
    if (Class == 0 || unsigned(tLeg) >= Class->legInfo.size())
      return 0;

    if (time == FinishTime) {
      return tInTeam->getLegRunningTime(tLeg) + inputTime; // Use the official running time in this case (which works with parallel legs)
    }

    int baseleg = tLeg-1;
    while (baseleg>0 && (Class->legInfo[baseleg+1].isParallel())) {
      baseleg--;
    }

    int leg = baseleg-1;
    while (leg>0 && (Class->legInfo[leg].legMethod == LTExtra || Class->legInfo[leg].legMethod == LTIgnore)) {
      leg--;
    }

    int pt = leg>=0 ? tInTeam->getLegRunningTime(leg) : 0;
    if (pt>0)
      return pt + time - StartTime + inputTime;
    else if (tInTeam->StartTime > 0)
      return (time - tInTeam->StartTime) + inputTime;
    else
      return 0;
  }
}

  // Get the complete name, including team and club.
string oRunner::getCompleteIdentification() const {
  if (tInTeam == 0 || !Class) {
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

RunnerStatus oRunner::getTotalStatus() const {
  if (Status == StatusUnknown)
    return StatusUnknown;
  else if (inputStatus == StatusUnknown)
    return StatusDNS;

  if (tInTeam == 0 || tLeg == 0) 
    return max(Status, inputStatus);
  else {
    RunnerStatus st = tInTeam->getLegStatus(tLeg-1);

    if (st == StatusOK || st == StatusUnknown)
      return Status;
    else
      return max(max(Status, st), inputStatus);
  }
}

void oRunner::remove() 
{
  if (oe)
    oe->removeRunner(Id);
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
  inputTime = r.getTotalRunningTime(r.FinishTime);
  inputStatus = r.getTotalStatus();
  inputPoints = r.getRogainingPoints() + r.inputPoints;
  inputPlace = r.tTotalPlace < 99000 ? r.tTotalPlace : 0;
}
