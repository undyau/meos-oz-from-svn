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

#include <vector>
#include <io.h>
#include <algorithm>

#include "oEvent.h"
#include "oSpeaker.h"
#include "gdioutput.h"

#include "SportIdent.h"
#include "mmsystem.h"
#include "meos_util.h"
#include "localizer.h"
#include "meos.h"
#include "gdifonts.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////


oTimeLine::oTimeLine(int time_, TimeLineType type_, Priority priority_, int classId_, int ID_, oAbstractRunner *source_) :
  time(time_), type(type_), priority(priority_), classId(classId_), ID(ID_), src(source_)
{

}

__int64 oTimeLine::getTag() const {
  __int64 cs = type != TLTExpected ? time : 0;
  cs = 997 * cs + type;
  cs = 997 * cs + priority;
  cs = 997 * cs + classId;
  cs = 997 * cs + ID;
  if (src)
    cs = 997 * cs + src->getId();
  //int len = msg.length();
  //for (int k = 0; k<len; k++)
   // cs = 31 * cs + msg[k]; 

  return cs;
}

//Order by preliminary times and priotity for speaker list
bool CompareSpkSPList(const oSpeakerObject &a, const oSpeakerObject &b)
{
	if(a.priority!=b.priority)
		return a.priority>b.priority;
	else if(a.status<=1 && b.status<=1){
    int at=a.preliminaryRunningTime;
		int bt=b.preliminaryRunningTime;

    if (at==bt) { //Compare leg times instead
      at=a.preliminaryRunningTimeLeg;
		  bt=b.preliminaryRunningTimeLeg;
    }

		if(at==bt)
			return a.name<b.name;
		else if(at>=0 && bt>=0)
			return at<bt;
		else if(at>=0 && bt<0) 
			return true;
		else if(at<0 && bt>=0)
			return false;
		else return at>bt;
	}
	else if(a.status!=b.status)
		return a.status<b.status;
	else return a.name<b.name;
}

//Order by known time for calculating results
bool CompareSOResult(const oSpeakerObject &a, const oSpeakerObject &b)
{
	if(a.status!=b.status){
		if(a.status==StatusOK)
			return true;
		else if(b.status==StatusOK)
			return false;
		else return a.status<b.status;
	}
	else if(a.status==StatusOK){
    int at=a.runningTime;
		int bt=b.runningTime;
    if(at!=bt)
		  return at<bt;
    else {
      at=a.runningTimeLeg;
		  bt=b.runningTimeLeg;
      if(at!=bt)
		    return at<bt;
      return a.name<b.name;
    }
	}
	else return a.name<b.name;
}


int SpeakerCB (gdioutput *gdi, int type, void *data)
{
	oEvent *oe=0;
	gdi->getData("oEvent", *LPDWORD(&oe));

	if(!oe)
		return false;

	DWORD ClassId=0, ControlId=0, leg=0;
	gdi->getData("ClassId", ClassId);
  gdi->getData("ControlId", ControlId);
  gdi->getData("LegNo", leg);

  if (ClassId>0 && ControlId>0) {
    pClass pc = oe->getClass(ClassId);
    if (pc && pc->wasSQLChanged())
		  oe->speakerList(*gdi, ClassId, leg, ControlId);
  }

	return true;
}

int MovePriorityCB(gdioutput *gdi, int type, void *data)
{
	if(type==GUI_LINK){

		oEvent *oe=0;
		gdi->getData("oEvent", *LPDWORD(&oe));
		if(!oe)	return false;

		TextInfo *ti=(TextInfo *)data;
		//gdi->alert(ti->id);
		if(ti->id.size()>1){
			//int rid=atoi(ti->id.substr(1).c_str());
      oRunner *r=static_cast<oRunner *>(ti->getExtra());

			if(!r) return false;
			
			DWORD ClassId=0, ControlId=0;
			if(gdi->getData("ClassId", ClassId) && gdi->getData("ControlId", ControlId)){			
			
				if(ti->id[0]=='D'){
					r->SetPriority(ControlId, -1);
				}
				else if(ti->id[0]=='U'){
					r->SetPriority(ControlId, 1);
				}
				else if(ti->id[0]=='M'){
					r->SetPriority(ControlId, 0);
				}

        int xo=gdi->GetOffsetX();
        int yo=gdi->GetOffsetY();
        gdi->SetOffsetX(0);
        gdi->SetOffsetY(0);
				gdi->restore("SpeakerList", false);        
				oe->speakerList(*gdi, ClassId, 0, ControlId);
        gdi->setOffset(xo, yo, false);
			}
		}
	}
	return true;
}

void oEvent::renderRowSpeakerList(gdioutput &gdi, oSpeakerObject &r, 
                                  oSpeakerObject *next_r, int x, int y, 
                                  int leaderTime, int type)
{
	const int dx_c[7]={0, 40, 300, 530, 590, 650, 660};
  int dx[7];
  for (int k=0;k<7;k++)
    dx[k]=gdi.scaleLength(dx_c[k]);

	int lh=gdi.getLineHeight();

	r.isRendered=true;
	gdi.addStringUT(y, x+dx[0], 0, r.placeS); 

	if(r.finishStatus<=1 || r.finishStatus==r.status)
		gdi.addStringUT(y, x+dx[1], 0, r.name, dx[2]-dx[1]-4);
	else
    gdi.addStringUT(y, x+dx[1], 0, r.name+ " ("+ oEvent::formatStatus(r.finishStatus) +")", dx[2]-dx[1]-4);

	gdi.addStringUT(y, x+dx[2], 0, r.club, dx[4]-dx[2]-4);
	
	if (r.status==StatusOK) {
		gdi.addStringUT(y, x+dx[4], textRight, formatTime(r.runningTime));
    
    if (r.runningTime != r.runningTimeLeg)
      gdi.addStringUT(y, x+dx[3], textRight, formatTime(r.runningTimeLeg));
    
		if(leaderTime!=100000){								
			gdi.addStringUT(y, x+dx[5], textRight, 
                    gdi.getTimerText(r.runningTime-leaderTime, 
                    timerCanBeNegative));
		}
	}
	else if (r.status==StatusUnknown) {		
		DWORD TimeOut=NOTIMEOUT;

    if (r.preliminaryRunningTimeLeg>0 && !r.missingStartTime) {
			
			if(next_r && next_r->status==StatusOK && 
          next_r->preliminaryRunningTime>r.preliminaryRunningTime)
				TimeOut=next_r->preliminaryRunningTime;
			
			RECT rc;
			rc.left=x+dx[1]-4;
			rc.right=x+dx[6]+gdi.scaleLength(60);
			rc.top=y-1;
			rc.bottom=y+lh+1;

      gdi.addRectangle(rc, colorDefault, false);

			gdi.addTimer(y, x+dx[4], textRight, r.preliminaryRunningTime, 0, 0, TimeOut);

      if (r.preliminaryRunningTime != r.preliminaryRunningTimeLeg)
        gdi.addTimer(y, x+dx[3], textRight, r.preliminaryRunningTimeLeg, 0, 0, TimeOut);

			if(leaderTime!=100000)
				gdi.addTimer(y, x+dx[5], timerCanBeNegative|textRight, r.preliminaryRunningTime-leaderTime);
		}
		else{
			gdi.addStringUT(y, x+dx[4], textRight, "["+r.startTimeS+"]");
      if (!r.missingStartTime)
			  gdi.addTimer(y, x+dx[5], timerCanBeNegative|textRight, r.preliminaryRunningTimeLeg, 0, 0, 0);
		}
	}
	else{
    gdi.addStringUT(y, x+dx[4], textRight, oEvent::formatStatus(r.status));
	}
	char bf[16];
  int ownerId = r.owner ? r.owner->getId(): 0;

	if (type==1) {
    sprintf_s(bf, "D%d", ownerId);
		gdi.addString(bf, y, x+dx[6], 0, "[Flytta ner]", 0, MovePriorityCB).setExtra(r.owner);
	}
	else if (type==2) {
    sprintf_s(bf, "U%d", ownerId);
		gdi.addString(bf, y, x+dx[6], 0, "[Bevaka]", 0, MovePriorityCB).setExtra(r.owner);
	}
	else if (type==3) {
		if(r.status<=StatusOK){
			
			if(r.priority<0){ 
				sprintf_s(bf, "M%d", ownerId);
				gdi.addString(bf, y, x+dx[6], 0, "[Återställ]", 0, MovePriorityCB).setExtra(r.owner);
			}
			else{
				sprintf_s(bf, "U%d", ownerId);
        gdi.addString(bf, y, x+dx[6], 0, "[Bevaka]", 0, MovePriorityCB).setExtra(r.owner);
			}
		}
	}

}

void oEvent::calculateResults(list<oSpeakerObject> &rl)
{
  rl.sort(CompareSOResult);	
	list<oSpeakerObject>::iterator it;

	int cPlace=0;
	int vPlace=0;
	int cTime=0;

	for (it=rl.begin(); it != rl.end(); ++it) {
    if (it->status==StatusOK) {
			cPlace++;

      if(it->runningTime>cTime)
				vPlace=cPlace;

      cTime=it->runningTime;

			it->place=vPlace;
		}
		else
			it->place=99000+it->status;
  
    char bf[16]="";
    if(it->place<90000)
      sprintf_s(bf, "%d", it->place);

    it->placeS=bf;
	}
}

void oEvent::speakerList(gdioutput &gdi, int ClassId, int leg, int ControlId)
{
#ifdef _DEBUG
  OutputDebugString("SpeakerListUpdate\n");
#endif

  DWORD clsIds = 0, ctrlIds = 0, cLegs = 0; 
  gdi.getData("ClassId", clsIds);
	gdi.getData("ControlId", ctrlIds);
  gdi.getData("LegNo", cLegs);
  //bool refresh = clsIds == ClassId &&  ctrlIds == ControlId && leg == cLegs;

  //if (refresh)
  //  gdi.takeShownStringsSnapshot();

  gdi.setData("ClassId", ClassId);
	gdi.setData("ControlId", ControlId);
  gdi.setData("LegNo", leg);
	gdi.setData("oEvent", DWORD(this));


  gdi.restoreNoUpdate("SpeakerList");
	gdi.setRestorePoint("SpeakerList");

	gdi.setData("ClassId", ClassId);
	gdi.setData("ControlId", ControlId);
  gdi.setData("LegNo", leg);
	gdi.setData("oEvent", DWORD(this));

	gdi.registerEvent("DataUpdate", SpeakerCB);
	gdi.setData("DataSync", 1);
	gdi.setData("PunchSync", 1);

	list<oSpeakerObject> speakerList;

  //For preliminary times
  updateComputerTime();
  oSpeakerObject so;

  speakerList.clear();
  
  if(classHasTeams(ClassId)) {
    oTeamList::iterator it=Teams.begin();
    while(it!=Teams.end()){
		  if(it->getClassId()==ClassId && !it->skip()){		
	      it->fillSpeakerObject(leg, ControlId, so);
        if (so.owner)
			    speakerList.push_back(so);
		  }
		  ++it;
	  }	
  }
  else {
    oRunnerList::iterator it=Runners.begin();
    while(it!=Runners.end()){
      if(it->getClassId()==ClassId && !it->skip()){		
	      it->fillSpeakerObject(leg, ControlId, so);
        if (so.owner)
			    speakerList.push_back(so);
		  }
		  ++it;
	  }	
  }
	if(speakerList.empty()){
		gdi.addString("",0, "Inga deltagare");
		return;
	}

  list<oSpeakerObject>::iterator sit;
	for (sit=speakerList.begin(); sit != speakerList.end(); ++sit) {
		if(sit->status==StatusOK && sit->priority>=0)
			sit->priority=1;
		else if(sit->status > StatusOK  && sit->priority<=0)
			sit->priority=-1;
	}

	//Calculate place...
	calculateResults(speakerList);

	//Calculate preliminary times and sort by this and prio.
	speakerList.sort(CompareSpkSPList);	

  char bf[256];
  pClass pCls=oe->getClass(ClassId);

  if(!pCls)
    return;

  char bf2[64]="";
  if(pCls->getNumStages()>1)
    sprintf_s(bf2, "str. %d, ", leg+1);

  if(ControlId!=oPunch::PunchFinish) {
    pControl pCtrl=oe->getControl(ControlId, false);

    if(!pCtrl)
      return;

    string control=string(bf2)+lang.tl("kontroll")+" "+pCtrl->getName();
    sprintf_s(bf, "%s, %s", pCls->getName().c_str(), control.c_str());
  }
  else {
    string control=string(bf2)+lang.tl("mål");
    sprintf_s(bf, "%s, %s", pCls->getName().c_str(), control.c_str());
  }

  int y=gdi.getCY()+5;
	int x=30;

  gdi.addStringUT(y, x, 1, bf).setColor(colorGreyBlue);
	int lh=gdi.getLineHeight();
 
  y+=lh*2;
	int LeaderTime=100000;

	//Calculate leader-time
	for(sit=speakerList.begin(); sit != speakerList.end(); ++sit){
		if(sit->status==StatusOK)	
      LeaderTime=min(LeaderTime, sit->runningTime);
	} 

  bool rendered=false;
	sit=speakerList.begin();
	while(sit != speakerList.end()){		
		if(sit->priority>0 || (sit->status==StatusOK && sit->priority>=0)){
			
      if(rendered==false) {
        gdi.addString("", y, x, boldSmall, "Resultat");
    	  y+=lh, rendered=true;
      }

      oSpeakerObject &so=*sit;
			oSpeakerObject *next_so=0;

			++sit;
			if(sit!=speakerList.end())
				next_so=&*sit;

			renderRowSpeakerList(gdi, so, next_so, x, y, LeaderTime, 1);
			y+=lh;
		}
		else ++sit;
	}
	
  rendered=false;
	sit=speakerList.begin();	
	while (sit != speakerList.end()) {		
		if (sit->status==StatusUnknown && sit->priority==0) {

      if(rendered==false) {
      	gdi.addString("", y+5, x, boldSmall, "Inkommande");
      	y+=lh+5, rendered=true;
      }

 			oSpeakerObject &so=*sit;
			oSpeakerObject *next_so=0;

			++sit;
			if(sit!=speakerList.end())
				next_so=&*sit;

			renderRowSpeakerList(gdi, so, next_so, x, y, LeaderTime, 2);
			y+=lh;
		}
		else ++sit;
	}

	sit=speakerList.begin();	
  rendered=false;
	while(sit != speakerList.end()){		
		if (!sit->isRendered) {
      if(rendered==false) {
        gdi.addString("", y+5, x, boldSmall, "Övriga");
	      y+=lh+5, rendered=true;
      }
 			oSpeakerObject &so=*sit;
			oSpeakerObject *next_so=0;

			++sit;
			if(sit!=speakerList.end())
				next_so=&*sit;

			renderRowSpeakerList(gdi, so, next_so, x, y, LeaderTime, 3);
      y+=lh;
		}
		else ++sit;
	}
//  if (refresh)
//    SmartFromSnapshot();
//  else
    gdi.refresh();
}

void oEvent::updateComputerTime()
{
	SYSTEMTIME st;	
	GetLocalTime(&st);

	ComputerTime=((24+2+st.wHour)*3600+st.wMinute*60+st.wSecond-ZeroTime)%(24*3600)-2*3600;
}

void oEvent::clearPrewarningSounds()
{
	oFreePunchList::reverse_iterator it;
	for (it=punches.rbegin(); it!=punches.rend(); ++it) 
		it->hasBeenPlayed=true;
}

void oEvent::tryPrewarningSounds(const string &basedir, int number)
{
	char wave[20];
	sprintf_s(wave, "%d.wav", number);

	string file=basedir+"\\"+wave;
	
	if(_access(file.c_str(), 0)==-1)
    gdibase.alert("Fel: hittar inte filen X.#" + file);

	PlaySound(file.c_str(), 0, SND_SYNC|SND_FILENAME );			
}

void oEvent::playPrewarningSounds(const string &basedir, set<int> &controls)
{
	oFreePunchList::reverse_iterator it;
	for (it=punches.rbegin(); it!=punches.rend() && !it->hasBeenPlayed; ++it) {

    if (controls.count(it->Type)==1 || controls.empty()) {
		  pRunner r=getRunnerByCard(it->CardNo);

		  if(r){			
			  char wave[20];
			  sprintf_s(wave, "%d.wav", r->getStartNo());

			  string file=basedir+"\\"+r->getDI().getString("Nationality")+"\\"+wave;

			  if(_access(file.c_str(), 0)==-1)
				  file=basedir+"\\"+wave;
  			
			  PlaySound(file.c_str(), 0, SND_SYNC|SND_FILENAME );			
			  it->hasBeenPlayed=true;
		  }
    }
	}
}

static bool compareFinishTime(pRunner a, pRunner b) {
  return a->getFinishTime() < b->getFinishTime();
}

struct TimeRunner {
  TimeRunner(int t, pRunner r) : time(t), runner(r) {}
  int time;
  pRunner runner;
  bool operator<(const TimeRunner &b) const {return time<b.time;}
};

string getOrder(int k) {
  string str;
  if (k==1)
    str = "första";
  else if (k==2)
    str = "andra";
  else if (k==3)
    str = "tredje";
  else if (k==4)
    str = "fjärde";
  else if (k==5)
    str = "femte";
  else if (k==6)
    str = "sjätte";
  else if (k==7)
    str = "sjunde";
  else if (k==8)
    str = "åttonde";
  else if (k==9)
    str = "nionde";
  else if (k==10)
    str = "tionde";
  else if (k==11)
    str = "elfte";
  else if (k==12)
    str = "tolfte";
  else
    return lang.tl("X:e#" + itos(k));

  return lang.tl(str);
}

string getNumber(int k) {
  string str;
  if (k==1)
    str = "etta";
  else if (k==2)
    str = "tvåa";
  else if (k==3)
    str = "trea";
  else if (k==4)
    str = "fyra";
  else if (k==5)
    str = "femma";
  else if (k==6)
    str = "sexa";
  else if (k==7)
    str = "sjua";
  else if (k==8)
    str = "åtta";
  else if (k==9)
    str = "nia";
  else if (k==10)
    str = "tia";
  else if (k==11)
    str = "elva";
  else if (k==12)
    str = "tolva";
  else
    return lang.tl("X:e#" + itos(k));

  return lang.tl(str);
}



struct BestTime {
  static const int nt = 4;  
  static const int maxtime = 3600*24*7;
  int times[nt];

  BestTime() {
    for (int k = 0; k<nt; k++)
      times[k] = maxtime;
  }

  void addTime(int t) {
    for (int k = 0; k<nt; k++) {
      if (t<times[k]) {
        int a = times[k];
        times[k] = t;
        t = a;
      }
    }
  }

  // Get the best time, but filter away unrealistic good times
  int getBestTime() {
    int avg = 0;
    int navg = 0;
    for (int k = 0; k<nt; k++) {
      if (times[k] < maxtime) {
        avg += times[k];
        navg++;
      }
    }

    if (navg == 0)
      return 0;
    else if (navg == 1)
      return avg;
    int limit = int(0.8 * double(avg) / double(navg));
    for (int k = 0; k<nt; k++) {
      if (limit < times[k] && times[k] < maxtime) {
        return times[k];
      }
    }
    return 0; // Cannot happen
  }

  // Get the second best time
  int getSecondBestTime() {
    int t = getBestTime();
    if (t > 0) {
      for (int k = 0; k < nt-1; k++) {
        if (t == times[k] && times[k+1] < maxtime)
          return times[k+1];
        else if (t < times[k] && times[k] < maxtime)
          return times[k]; // Will not happen with current implementation of besttime
      }
    }
    return t;
  }
};

struct PlaceRunner {
  PlaceRunner(int p, pRunner r) : place(p), runner(r) {}
  int place;
  pRunner runner;
};

// ClassId -> (Time -> Place, Runner)
typedef multimap<int, PlaceRunner> TempResultMap;

void insertResult(TempResultMap &rm, oRunner &r, int time, 
                  int &place, bool &sharedPlace, vector<pRunner> &preRunners) {
  TempResultMap::iterator it = rm.insert(make_pair(time, PlaceRunner(0, &r)));
  place = 1;
  sharedPlace = false;
  preRunners.clear();
  
  if (it != rm.begin()) {
    TempResultMap::iterator p_it = it;
    --p_it;
    if (p_it->first < it->first)
      place = p_it->second.place + 1;
    else {
      place = p_it->second.place;
      sharedPlace = true;
    }
    int pretime = p_it->first;
    while(p_it->first == pretime) {
      preRunners.push_back(p_it->second.runner);
      if(p_it != rm.begin()) 
        --p_it;
      else 
        break;
    }
  }

  TempResultMap::iterator p_it = it;
  if ( (++p_it) != rm.end() && p_it->first == time)
    sharedPlace = true;


  int lastplace = place;
  int cplace = place;

  // Update remaining
  while(it != rm.end()) {
    if (time == it->first) 
      it->second.place = lastplace;
    else {
      it->second.place = cplace;
      lastplace = cplace;
      time = it->first;
    }
    ++cplace;
    ++it;
  }
}

int oEvent::setupTimeLineEvents(int currentTime)
{
  if (currentTime == 0) {
    updateComputerTime();
    currentTime = ComputerTime;
  }

  int nextKnownEvent = 3600*48;
  vector<pRunner> started;
  started.reserve(Runners.size());
  timeLineEvents.clear();

  for(set<int>::iterator it = timelineClasses.begin(); it != timelineClasses.end(); ++it) {
    int ne = setupTimeLineEvents(*it, currentTime);
    nextKnownEvent = min(ne, nextKnownEvent);
    modifiedClasses.erase(*it);
  }
  return nextKnownEvent;
}


int oEvent::setupTimeLineEvents(int classId, int currentTime)
{
  // leg -> started on leg
  vector< vector<pRunner> > started;
  started.reserve(32);
  int nextKnownEvent = 3600*48;
  int classSize = 0;

  pClass pc = getClass(classId);
  if (!pc)
    return nextKnownEvent;

  vector<char> skipLegs;
  skipLegs.resize(max<int>(1, pc->getNumStages()));

  for (size_t k = 1; k < skipLegs.size(); k++) {
    LegTypes lt = pc->getLegType(k);
    if (lt == LTIgnore) {
      skipLegs[k] = true;
    }
    else if (lt == LTParallel || lt == LTParallelOptional) {
      StartTypes st = pc->getStartType(k-1);
      if (st != STChange) {
        // Check that there is no forking
        vector<pCourse> &cc1 = pc->MultiCourse[k-1];
        vector<pCourse> &cc2 = pc->MultiCourse[k];
        bool equal = cc1.size() == cc2.size();
        for (size_t j = 0; j<cc1.size() && equal; j++)
          equal = cc1[j] == cc2[j];

        if (equal) { // Don't watch both runners if "patrol" style
          skipLegs[k-1] = true;
        }
      }
    }
  }
  // Count the number of starters at the same time
  inthashmap startTimes;

  for (oRunnerList::iterator it = Runners.begin(); it != Runners.end(); ++it) {
    oRunner &r = *it;
    if (r.isRemoved() || r.isVacant())
      continue;
    if (!r.Class ||r.Class->Id != classId)
      continue;
    if (r.tStatus == StatusDNS || r.tStatus == StatusNotCompetiting)
      continue;
    if (r.CardNo == 0)
      continue;
    if (r.tLeg == 0)
      classSize++;
    if (size_t(r.tLeg) < skipLegs.size() && skipLegs[r.tLeg])
      continue;
    if (r.tStartTime > 0 && r.tStartTime < currentTime) {
      if (started.size() <= size_t(r.tLeg)) {
        started.resize(r.tLeg+1);
        started.reserve(Runners.size() / (r.tLeg + 1));
      }
      r.tTimeAfter = 0; //Reset time after
      r.tInitialTimeAfter = 0;
      started[r.tLeg].push_back(&r);
      int id = r.tLeg + 100 * r.tStartTime;
      ++startTimes[id];
    }
    else if (r.tStartTime > currentTime) {
      nextKnownEvent = min(r.tStartTime, nextKnownEvent);
    }
  }

  if (started.empty())
    return nextKnownEvent;
 
  size_t firstNonEmpty = 0;
  while (started[firstNonEmpty].empty()) // Note -- started cannot be empty.
    firstNonEmpty++;

  int sLimit = min(4, classSize);

  if (startTimes.size() == 1) {
    oRunner &r = *started[firstNonEmpty][0];
    
    oTimeLine tl(r.tStartTime, oTimeLine::TLTStart, oTimeLine::PHigh, r.getClassId(), 0, 0);
    TimeLineIterator it = timeLineEvents.insert(pair<int, oTimeLine>(r.tStartTime, tl));
    it->second.setMessage("X har startat.#" + r.getClass());
  }
  else {
    for (size_t j = 0; j<started.size(); j++) {
      bool startedClass = false;    
      for (size_t k = 0; k<started[j].size(); k++) {
        oRunner &r = *started[j][k];
        int id = r.tLeg + 100 * r.tStartTime;
        if (startTimes[id] < sLimit) {
          oTimeLine::Priority prio = oTimeLine::PLow;
          int p = r.getSpeakerPriority();
          if (p > 1)
            prio = oTimeLine::PHigh;
          else if (p == 1)
            prio = oTimeLine::PMedium;
          oTimeLine tl(r.tStartTime, oTimeLine::TLTStart, prio, r.getClassId(), r.getId(), &r);
          TimeLineIterator it = timeLineEvents.insert(pair<int, oTimeLine>(r.tStartTime + 1, tl));
          it->second.setMessage("har startat.");
        }
        else if (!startedClass) {
          // The entire class started
          oTimeLine tl(r.tStartTime, oTimeLine::TLTStart, oTimeLine::PHigh, r.getClassId(), 0, 0);
          TimeLineIterator it = timeLineEvents.insert(pair<int, oTimeLine>(r.tStartTime, tl));
          it->second.setMessage("X har startat.#" + r.getClass());
          startedClass = true;
        }
      }
    }
  }
  set<int> controlId;
  getFreeControls(controlId);

  // Radio controls for each leg
  map<int, vector<pControl> > radioControls;

  for (size_t leg = 0; leg<started.size(); leg++) {
    for (size_t k = 0; k < started[leg].size(); k++) {
      if (radioControls.count(leg) == 0) {
        pCourse pc = started[leg][k]->getCourse(false);
        if (pc) {
          vector<pControl> &rc = radioControls[leg];
          for (int j = 0; j < pc->nControls; j++) {
            int id = pc->Controls[j]->Id;
            if (controlId.count(id) > 0)
              rc.push_back(pc->Controls[j]);
          }
        }
      }
    }
  }
  for (size_t leg = 0; leg<started.size(); leg++) {
    const vector<pControl> &rc = radioControls[leg];
    int nv = setupTimeLineEvents(started[leg], rc, currentTime, leg + 1 == started.size());
    nextKnownEvent = min(nv, nextKnownEvent);
  }
  return nextKnownEvent;
}

void getTimeAfterDetail(string &detail, int timeAfter, int deltaTime, bool wasAfter) 
{
  if (timeAfter > 0) {
    string aTimeS = timeAfter >= 60 ? formatTime(timeAfter) : itos(timeAfter) + "s.";
    if (!wasAfter || deltaTime == 0)
      detail = "Tid efter: X#" + aTimeS;
    else {
      int ad = abs(deltaTime);
      string deltaS = ad >= 60 ? getTimeMS(ad) : itos(ad) + "s.";
      if (deltaTime > 0)
        detail = "Tid efter: X; har tappat Y#" + 
                  aTimeS + "#" + deltaS;
      else
        detail = "Tid efter: X; har tagit in Y#" + 
                  aTimeS + "#" + deltaS;
    }
  }
}

void oEvent::timeLinePrognose(TempResultMap &results, TimeRunner &tr, int prelT, 
                              int radioNumber, const string &rname, int radioId) {
  TempResultMap::iterator place_it = results.lower_bound(prelT);
  int p = place_it != results.end() ? place_it->second.place : results.size() + 1;
  int prio = tr.runner->getSpeakerPriority();

  if ((radioNumber == 0 && prio > 0) || (radioNumber > 0 && p <= 10) || (radioNumber > 0 && prio > 0 && p <= 20)) {
    string msg;
    if (radioNumber > 0) {
      if (p == 1)
        msg = "väntas till X om någon minut, och kan i så fall ta ledningen.#" + rname;
      else
        msg = "väntas till X om någon minut, och kan i så fall ta en Y plats.#" + rname + getOrder(p);
    }
    else
      msg = "väntas till X om någon minut.#" + rname;

    oTimeLine::Priority mp = oTimeLine::PMedium;
    if (p <= (3 + prio * 3))
      mp = oTimeLine::PHigh;
    else if (p>6 + prio * 5)
      mp = oTimeLine::PLow;

    oTimeLine tl(tr.time, oTimeLine::TLTExpected, mp, tr.runner->getClassId(), radioId, tr.runner);
    TimeLineIterator tlit = timeLineEvents.insert(pair<int, oTimeLine>(tl.getTime(), tl));
    tlit->second.setMessage(msg);
  }
}

int oEvent::setupTimeLineEvents(vector<pRunner> &started, const vector<pControl> &rc, int currentTime, bool finish)
{
  int nextKnownEvent = 48*3600;
  vector< vector<TimeRunner> > radioResults(rc.size());
  vector<BestTime> bestLegTime(rc.size() + 1);
  vector<BestTime> bestTotalTime(rc.size() + 1);
  vector<BestTime> bestRaceTime(rc.size());

  for (size_t j = 0; j < rc.size(); j++) {
    int id = rc[j]->Id;
    vector<TimeRunner> &radio = radioResults[j];
    radio.reserve(started.size());
    for (size_t k = 0; k < started.size(); k++) {
      int rt;
      oRunner &r = *started[k];
      RunnerStatus rs;
      r.getSplitTime(id, rs, rt);
      if (rs == StatusOK)
        radio.push_back(TimeRunner(rt + r.tStartTime, &r));
      else
        radio.push_back(TimeRunner(0, &r));

      if (rt > 0) {
        bestTotalTime[j].addTime(r.getTotalRunningTime(rt + r.tStartTime));
        bestRaceTime[j].addTime(rt);
        // Calculate leg time since last radio (or start)
        int lt = 0;
        if (j == 0)
          lt = rt;
        else if (radioResults[j-1][k].time>0)
          lt = rt + r.tStartTime - radioResults[j-1][k].time;

        if (lt>0)
          bestLegTime[j].addTime(lt);

        if (j == rc.size()-1 && r.FinishTime>0 && r.tStatus == StatusOK) {
          // Get best total time
          bestTotalTime[j+1].addTime(r.getTotalRunningTime(r.FinishTime));

          // Calculate best time from last radio to finish
          int ft = r.FinishTime - (rt + r.tStartTime);
          if (ft > 0)
            bestLegTime[j+1].addTime(ft);
        }
      }
    }
  }

  // Relative speed of a runner
  vector<double> relSpeed(started.size(), 1.0);

  // Calculate relative speeds
  for (size_t k = 0; k < started.size(); k++) {
    size_t j = 0;
    int j_radio = -1;
    int time = 0;
    while(j < rc.size() && (radioResults[j][k].time > 0 || j_radio == -1)) {
      if (radioResults[j][k].time > 0) {
        j_radio = j;
        time = radioResults[j][k].time - radioResults[j][k].runner->tStartTime;
      }
      j++;
    }

    if (j_radio >= 0) {
      int reltime = bestRaceTime[j_radio].getBestTime();
      if (reltime == time)
        reltime = bestRaceTime[j_radio].getSecondBestTime();
      relSpeed[k] = double(time) / double(reltime);
    }
  }

  vector< vector<TimeRunner> > expectedAtNext(rc.size() + 1);
  
  // Time before expection to "prewarn"
  const int pwTime = 60;

  // First radio
  int bestLeg = bestLegTime[0].getBestTime();
  if (bestLeg > 0 && bestLeg != BestTime::maxtime){
    vector<TimeRunner> &radio = radioResults[0];
    vector<TimeRunner> &expectedRadio = expectedAtNext[0];
    expectedRadio.reserve(radio.size());
    for (size_t k = 0; k < radio.size(); k++) {
      oRunner &r = *radio[k].runner;
      int expected = r.tStartTime + bestLeg;
      int actual = radio[k].time;
      if ( (actual == 0 && (expected - pwTime) < currentTime) || (actual > (expected - pwTime)) ) {
        expectedRadio.push_back(TimeRunner(expected-pwTime, &r));
      }
    }
  }

  // Remaining radios and finish
  for (size_t j = 0; j < rc.size(); j++) {
    bestLeg = bestLegTime[j+1].getBestTime();
    if (bestLeg == 0 || bestLeg == BestTime::maxtime)
      continue;

    vector<TimeRunner> &radio = radioResults[j];
    vector<TimeRunner> &expectedRadio = expectedAtNext[j+1];
    expectedRadio.reserve(radio.size());
    for (size_t k = 0; k < radio.size(); k++) {
      oRunner &r = *radio[k].runner;
      if (radio[k].time > 0) {
        int expected = radio[k].time + int(bestLeg * relSpeed[k]);
        int actual = 0;
        if (j + 1 < rc.size())
          actual = radioResults[j+1][k].time;
        else
          actual = r.FinishTime;

        if ( (actual == 0 && (expected - pwTime) <= currentTime) || (actual > (expected - pwTime)) ) {
          expectedRadio.push_back(TimeRunner(expected-pwTime, &r));
        }
        else if (actual == 0 && (expected - pwTime) > currentTime) {
          nextKnownEvent = min(nextKnownEvent, expected - pwTime);
        }
      }
    }
  }

  vector<TempResultMap> timeAtRadio(rc.size());
  
  for (size_t j = 0; j < rc.size(); j++) {

    TempResultMap &results = timeAtRadio[j];
    vector<TimeRunner> &radio = radioResults[j];
    vector<TimeRunner> &expectedRadio = expectedAtNext[j];
  
    string rname;
    if (rc[j]->Name.empty())
      rname = lang.tl("radio X#" + itos(j+1)) + "#";
    else
      rname = rc[j]->Name + "#";

    // Sort according to pass time
    sort(radio.begin(), radio.end());
    sort(expectedRadio.begin(), expectedRadio.end());
    size_t expIndex = 0;

    for (size_t k = 0; k < radio.size(); k++) {
      RunnerStatus ts = radio[k].runner->getTotalStatus();
      if (ts == StatusMP || ts == StatusDQ)
        continue;
      if (radio[k].time > 0) {
        while (expIndex < expectedRadio.size() && expectedRadio[expIndex].time < radio[k].time) {
          TimeRunner &tr = expectedRadio[expIndex];
          int prelT = tr.runner->getTotalRunningTime(tr.time + pwTime);

          timeLinePrognose(results, tr, prelT, j, rname, rc[j]->getId());
          expIndex++;
        }

        oRunner &r = *radio[k].runner;
        int place = 1;
        bool sharedPlace = false;
        vector<pRunner> preRunners;
        int time = radio[k].time - r.tStartTime;
        int totTime = r.getTotalRunningTime(radio[k].time);
        insertResult(results, r, totTime, place, sharedPlace, preRunners);
        int leaderTime = results.begin()->first;
        int timeAfter = totTime - leaderTime;
        int deltaTime = timeAfter - r.tTimeAfter;

        string timeS = formatTime(time);
        string msg, detail;
        getTimeAfterDetail(detail, timeAfter, deltaTime, r.tTimeAfter > 0);
        r.tTimeAfter = timeAfter;

        if (place == 1) {
          if (results.size() == 1)
            msg = "är först vid X med tiden Y.#" + rname + timeS;
          else if (!sharedPlace)
            msg = "tar ledningen vid X med tiden Y.#" + rname + timeS;
          else
            msg = "går upp i delad ledning vid X med tiden Y.#" + rname + timeS;
        }
        else {
          if (!sharedPlace) {
            msg = "stämplar vid X som Y, på tiden Z.#" +
                          rname + getNumber(place) +
                          "#" + timeS;
            
          }
          else {
            msg = "stämplar vid X som delad Y med tiden Z.#" + rname + getNumber(place) +
                                                         "#" + timeS;
          }
        }

        oTimeLine::Priority mp = oTimeLine::PLow;
        if (place <= 2)
          mp = oTimeLine::PTop;
        else if (place <= 5)
          mp = oTimeLine::PHigh;
        else if (place <= 10)
          mp = oTimeLine::PMedium;

        oTimeLine tl(radio[k].time, oTimeLine::TLTRadio, mp, r.getClassId(),  rc[j]->getId(), &r);
        TimeLineIterator tlit = timeLineEvents.insert(pair<int, oTimeLine>(tl.getTime(), tl));
        tlit->second.setMessage(msg).setDetail(detail);
      }
    }
  }

  TempResultMap results;
  sort(started.begin(), started.end(), compareFinishTime);

  string location, locverb, thelocation;
  if (finish) {
    location = "i mål";
    locverb = "går i mål";
    thelocation = lang.tl("målet") + "#";
  }
  else {
    location = "vid växeln";
    locverb = "växlar";
    thelocation = lang.tl("växeln") + "#";
  }

  vector<TimeRunner> &expectedFinish = expectedAtNext.back();
    // Sort according to pass time
  sort(expectedFinish.begin(), expectedFinish.end());
  size_t expIndex = 0;


  for (size_t k = 0; k<started.size(); k++) {
    oRunner &r = *started[k];
    if (r.getTotalStatus() == StatusOK) {

      while (expIndex < expectedFinish.size() && expectedFinish[expIndex].time < r.FinishTime) {
        TimeRunner &tr = expectedFinish[expIndex];
        int prelT = tr.runner->getTotalRunningTime(tr.time + pwTime);
        timeLinePrognose(results, tr, prelT, 1, thelocation, 0);
        expIndex++;
      }

      int place = 1;
      bool sharedPlace = false;
      vector<pRunner> preRunners;
      int time = r.getTotalRunningTime(r.FinishTime);

      insertResult(results, r, time, place, sharedPlace, preRunners);
  
      int leaderTime = results.begin()->first;
      int timeAfter = time - leaderTime;
      int deltaTime = timeAfter - r.tInitialTimeAfter;

      string timeS = formatTime(time);

      string msg, detail;
      getTimeAfterDetail(detail, timeAfter, deltaTime, r.tInitialTimeAfter > 0);
      
      // Transfer time after to next runner
      if (r.tInTeam) {
        pRunner next = r.tInTeam->getRunner(r.tLeg + 1);
        if (next) {
          next->tTimeAfter = timeAfter;
          next->tInitialTimeAfter = timeAfter;
        }
      }

      if (place == 1) {
        if (results.size() == 1)
          msg = "är först " + location + " med tiden X.#" + timeS;
        else if (!sharedPlace)
          msg = "tar ledningen med tiden X.#" + timeS;
        else
          msg = "går upp i delad ledning med tiden X.#" + timeS;
      }
      else {
        if (!sharedPlace) {
          if (preRunners.size() == 1 && place < 10)
            msg = locverb + " på X plats, efter Y, på tiden Z.#" + 
                        getOrder(place) +
                        "#" + preRunners[0]->getCompleteIdentification() +
                        "#" + timeS;
          else
             msg = locverb + " på X plats med tiden Y.#" + 
                        getOrder(place) + "#" + timeS;
 
        }
        else {
          msg = locverb + " på delad X plats med tiden Y.#" + getOrder(place) +
                                                       "#" + timeS;
        }
      }

      oTimeLine::Priority mp = oTimeLine::PLow;
      if (place <= 5)
        mp = oTimeLine::PTop;
      else if (place <= 10)
        mp = oTimeLine::PHigh;
      else if (place <= 20)
        mp = oTimeLine::PMedium;

      oTimeLine tl(r.FinishTime, oTimeLine::TLTFinish, mp, r.getClassId(), r.getId(), &r);
      TimeLineIterator tlit = timeLineEvents.insert(pair<int, oTimeLine>(tl.getTime(), tl));
      tlit->second.setMessage(msg).setDetail(detail);
    }
    else if (r.getStatus() != StatusUnknown && r.getStatus() != StatusOK) {
      int t = r.FinishTime;
      if ( t == 0)
        t = r.tStartTime;

      int place = 1000;
      if (r.FinishTime > 0) {
        place = results.size() + 1;
        int rt = r.getTotalRunningTime(r.FinishTime);
        if (rt > 0) {
          TempResultMap::iterator place_it = results.lower_bound(rt);
          place = place_it != results.end() ? place_it->second.place : results.size() + 1;
        }
      }
      oTimeLine::Priority mp = r.getSpeakerPriority() > 0 ? oTimeLine::PMedium : oTimeLine::PLow;
      if (place <= 3)
        mp = oTimeLine::PTop;
      else if (place <= 10)
        mp = oTimeLine::PHigh;
      else if (place <= 20)
        mp = oTimeLine::PMedium;

      oTimeLine tl(r.FinishTime, oTimeLine::TLTFinish, mp, r.getClassId(), r.getId(), &r);
      TimeLineIterator tlit = timeLineEvents.insert(pair<int, oTimeLine>(t, tl));
      string msg;
      if (r.getStatus() != StatusDQ)
        msg = "är inte godkänd.";
      else
        msg = "är diskvalificerad.";

      tlit->second.setMessage(msg);
    }
  }
  return nextKnownEvent;
}


void oEvent::renderTimeLineEvents(gdioutput &gdi) const
{
  gdi.fillDown();
  for (TimeLineMap::const_iterator it = timeLineEvents.begin(); it != timeLineEvents.end(); ++it) {
    int t = it->second.getTime();
    string msg = t>0 ? getAbsTime(t) : MakeDash("--:--:--"); 
    msg += " (" + it->second.getSource()->getClass() + ") ";
    msg += it->second.getSource()->getName() + ", " + it->second.getSource()->getClub();
    msg += ", " + lang.tl(it->second.getMessage());
    gdi.addStringUT(0, msg);
  }
}

int oEvent::getTimeLineEvents(const set<int> &classes, vector<oTimeLine> &events, 
                              set<__int64> &stored, int currentTime) {  
  if (currentTime == 0) {
    updateComputerTime();
    currentTime = ComputerTime;
  }

  const int timeWindowSize = 10*60;
  int eval = nextTimeLineEvent <= ComputerTime;
  for (set<int>::const_iterator it = classes.begin(); it != classes.end(); ++it) {
    if (timelineClasses.count(*it) == 0) {
      timelineClasses.insert(*it);
      eval = true;
    }
    if (modifiedClasses.count(*it) != 0)
      eval = true;
  }
  if (eval) {    
    nextTimeLineEvent = setupTimeLineEvents(currentTime);
  }
  
  int time = 0;
  for (int k = events.size()-1; k>=0; k--) {
    int t = events[k].getTime();
    if (t > 0) {
      time = t;
      break;
    }
  }
  
  TimeLineMap::const_iterator it_start = timeLineEvents.lower_bound(time - timeWindowSize);

  bool filter = !events.empty();

  if (filter) {
    for (TimeLineMap::const_iterator it = it_start; it != timeLineEvents.end(); ++it) {
  

    }
  }

  for (TimeLineMap::const_iterator it = it_start; it != timeLineEvents.end(); ++it) {
    if (it->first <= time && it->second.getType() == oTimeLine::TLTExpected)
      continue; // Never get old expectations
    if (classes.count(it->second.getClassId()) == 0)
      continue;

    __int64 tag = it->second.getTag();
    if (stored.count(tag) == 0) {
      events.push_back(it->second);
      stored.insert(tag);
    }
  } 

  return nextTimeLineEvent;
}

void oEvent::classChanged(pClass cls, bool punchOnly) {
  if (timelineClasses.count(cls->getId()) == 1) {
    modifiedClasses.insert(cls->getId());
    timeLineEvents.clear();
  }
}

