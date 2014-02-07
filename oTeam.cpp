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

#include "stdafx.h"
#include "meos_util.h"
#include "oEvent.h"
#include <assert.h>
#include <algorithm>
#include "meosdb/sqltypes.h"
#include "Table.h"
#include "localizer.h"
#include "meosException.h"
#include "gdioutput.h"

oTeam::oTeam(oEvent *poe): oAbstractRunner(poe)
{
	Id=oe->getFreeTeamId();
	getDI().initData();
  correctionNeeded = false;
  StartNo=0;	
}

oTeam::oTeam(oEvent *poe, int id): oAbstractRunner(poe)
{
	Id=id;
  oe->qFreeTeamId = max(id, oe->qFreeTeamId);
	getDI().initData();
  correctionNeeded = false;
  StartNo=0;	
}


oTeam::~oTeam(void)
{
  for(unsigned i=0; i<Runners.size(); i++) {
		if(Runners[i] && Runners[i]->tInTeam==this) {
			assert(Runners[i]->tInTeam!=this);
			Runners[i]=0;
		}
	}
}

void oTeam::prepareRemove() 
{
  for(unsigned i=0; i<Runners.size(); i++)
    setRunnerInternal(i, 0);
}
	
bool oTeam::write(xmlparser &xml)
{
	if(Removed) return true;

	xml.startTag("Team");
	xml.write("Id", Id);
	xml.write("StartNo", StartNo);
	xml.write("Updated", Modified.getStamp());
	xml.write("Name", Name);
	xml.write("Start", StartTime);
	xml.write("Finish", FinishTime);
	xml.write("Status", Status);
	xml.write("Runners", getRunners());

	if(Club) xml.write("Club", Club->Id);
	if(Class) xml.write("Class", Class->Id);

  xml.write("InputPoint", inputPoints);
  if (inputStatus != StatusOK)
    xml.write("InputStatus", itos(inputStatus)); //Force write of 0
  xml.write("InputTime", inputTime);
  xml.write("InputPlace", inputPlace);


	getDI().write(xml);
	xml.endTag();

	return true;
}

void oTeam::set(const xmlobject &xo)
{
	xmlList xl;
  xo.getObjects(xl);
	xmlList::const_iterator it;

	for(it=xl.begin(); it != xl.end(); ++it){
		if(it->is("Id")){
			Id=it->getInt();			
		}
		else if(it->is("Name")){
			Name=it->get();
		}
		else if(it->is("StartNo")){
			StartNo=it->getInt();			
		}
		else if(it->is("Start")){
			StartTime=it->getInt();			
		}
		else if(it->is("Finish")){
			FinishTime=it->getInt();			
		}
		else if(it->is("Status")){
			Status=RunnerStatus(it->getInt());	
		}
		else if(it->is("Class")){
			Class=oe->getClass(it->getInt());			
		}
		else if(it->is("Club")){
			Club=oe->getClub(it->getInt());			
		}
		else if(it->is("Runners")){
      vector<int> r;
      decodeRunners(it->get(), r);
      importRunners(r);
		}
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
		else if(it->is("Updated")){
			Modified.setStamp(it->get());
		}
		else if (it->is("oData")) {
			getDI().set(*it);
		}			
	}
}

string oTeam::getRunners() const
{
	string str="";
	char bf[16];
	unsigned m=0;

	for(m=0;m<Runners.size();m++){
		if(Runners[m]){
			sprintf_s(bf, 16, "%d;", Runners[m]->Id);
			str+=bf;
		}
		else str+="0;";
	}
	return str;
}

void oTeam::decodeRunners(const string &rns, vector<int> &rid)
{
	const char *str=rns.c_str();
	rid.clear();

	int n=0;

	while (*str) {
		int cid=atoi(str);

		while(*str && (*str!=';' && *str!=',')) str++;
		if(*str==';'  || *str==',') str++;		

    rid.push_back(cid);
		n++;
	}
}

void oTeam::importRunners(const vector<int> &rns)
{
  Runners.resize(rns.size());
  for (size_t n=0;n<rns.size(); n++) {
    if(rns[n]>0) 
      Runners[n] = oe->getRunner(rns[n], 0);
    else
      Runners[n] = 0;
	}
}

void oTeam::importRunners(const vector<pRunner> &rns)
{
  // Unlink old runners
  for (size_t k = rns.size(); k<Runners.size(); k++) {
    if (Runners[k] && Runners[k]->tInTeam == this) {
      Runners[k]->tInTeam = 0;
      Runners[k]->tLeg = 0;
    }
  }

  Runners.resize(rns.size());
  for (size_t n=0;n<rns.size(); n++) {
    if (Runners[n] && rns[n] != Runners[n]) {
      if (Runners[n]->tInTeam == this) {
        Runners[n]->tInTeam = 0;
        Runners[n]->tLeg = 0;
      }
    }    
    Runners[n] = rns[n];
  }
}


void oEvent::removeTeam(int Id)
{
	oTeamList::iterator it;	

	for (it=Teams.begin(); it != Teams.end(); ++it){
		if(it->Id==Id){
			if(HasDBConnection) 
        msRemove(&*it);
      dataRevision++;
      it->prepareRemove();
			Teams.erase(it);
      updateTabs();
			return;
		}
	}	
}

void oTeam::setRunner(unsigned i, pRunner r, bool sync)
{
  if (i>=Runners.size()) {
    if (i>=0 && i<100)
      Runners.resize(i+1);
    else
      throw std::exception("Bad runner index");
  }

  if (Runners[i]==r) 
    return;

  setRunnerInternal(i, r);
	
  if (r) {
    if (Status == StatusDNS)
      setStatus(StatusUnknown);
    r->tInTeam=this;
    r->tLeg=i;
    r->createMultiRunner(true, sync);
  }
  
  if (Class) {
    int index=1; //Set multi runners
    for (unsigned k=i+1;k<Class->getNumStages(); k++) {
      if (Class->getLegRunner(k)==i) {
        if (r!=0) {
          pRunner mr=r->getMultiRunner(index++);

          if(mr) {
            mr->setName(r->getName());
            mr->synchronize();
            setRunnerInternal(k, mr);
          }
        }
        else setRunnerInternal(k, 0);
      }
    }
  }
}

void oTeam::setRunnerInternal(int k, pRunner r) 
{
  if (r==Runners[k]) {
    if (r) { 
      r->tInTeam = this;
      r->tLeg = k;
    }
    return;
  }

  if (Runners[k]) {
    assert(Runners[k]->tInTeam == 0 || Runners[k]->tInTeam == this);
    Runners[k]->tInTeam = 0;
    Runners[k]->tLeg = 0;
  }

  // Reset old team
  if (r && r->tInTeam) {
    if (r->tInTeam->Runners[r->tLeg] != 0) {
      r->tInTeam->Runners[r->tLeg] = 0;
      r->tInTeam->updateChanged();
      if (r->tInTeam != this)
        r->tInTeam->synchronize(true);
    }    
  }

  Runners[k]=r;

  if (Runners[k]) {
    Runners[k]->tInTeam = this;
    Runners[k]->tLeg = k;
    Runners[k]->Class = Class;
  }
  updateChanged();
}

string oTeam::getLegFinishTimeS(int leg) const
{
	if (leg==-1)
		leg=Runners.size()-1;
		
	if (unsigned(leg)<Runners.size() && Runners[leg])
		return Runners[leg]->getFinishTimeS();
	else return "-";
}

int oTeam::getLegToUse(int leg) const {
  if (Runners.empty())
    return 0;
	if (leg==-1)
    leg=Runners.size()-1;
  int oleg = leg;
	if (Class && !Runners[leg]) {
    LegTypes lt = Class->getLegType(leg);
    while (leg>=0 && lt == LTParallelOptional && !Runners[leg]) {
      if (leg == 0)
        return oleg; //Suitable leg not found
      leg--;
      lt = Class->getLegType(leg);
    }
  }
  return leg;
}

int oTeam::getLegFinishTime(int leg) const
{
  leg = getLegToUse(leg);
	//if (leg==-1)
	//	leg=Runners.size()-1;
		
  if (Class) {
    pClass pc = Class;
    LegTypes lt = pc->getLegType(leg);
    while (leg> 0  && (lt == LTIgnore || 
              (lt == LTExtra && (!Runners[leg] || Runners[leg]->getFinishTime() <= 0)) ) ) {
      leg--;
      lt = pc->getLegType(leg);
    }
  }

	if (unsigned(leg)<Runners.size() && Runners[leg])
		return Runners[leg]->getFinishTime();
	else return 0;
}

int oTeam::getRunningTime() const {
  return getLegRunningTime(-1, false);
}

int oTeam::getLegRunningTime(int leg, bool multidayTotal) const
{
  leg = getLegToUse(leg);

  int addon = 0;
  if (multidayTotal)
    addon = inputTime;

	if (unsigned(leg)<Runners.size() && Runners[leg]) {
    if (Class) {
      pClass pc=Class;

      LegTypes lt = pc->getLegType(leg);
      LegTypes ltNext = pc->getLegType(leg+1);
      if (ltNext == LTParallel || ltNext == LTParallelOptional) // If the following leg is parallel, then so is this.
        lt = ltNext;

      switch(lt) {
        case LTNormal:
          if(Runners[leg]->statusOK()) {
            int dt = leg>0 ? getLegRunningTime(leg-1, false)+Runners[leg]->getRunningTime():0;     
            return addon + max(Runners[leg]->getFinishTime()-StartTime, dt);
          }
          else return 0;
        break;

        case LTParallelOptional:
        case LTParallel: //Take the longest time of this runner and the previous
          if(Runners[leg]->statusOK()) {
            int pt=leg>0 ? getLegRunningTime(leg-1, false) : 0;
            return addon + max(Runners[leg]->getFinishTime()-StartTime, pt);
          }
          else return 0;
        break;

        case LTExtra: //Take the best time of this runner and the previous
          if (leg==0) 
            return addon + max(Runners[leg]->getFinishTime()-StartTime, 0);
          else {
            int pt = getLegRunningTime(leg-1, multidayTotal);

            if (Runners[leg]->getFinishTime()>StartTime)
              return min(addon + Runners[leg]->getFinishTime()-StartTime, pt);
            else return pt;
          }  
        break;

        case LTSum:
          if(Runners[leg]->statusOK()) {
            if (leg==0) 
              return addon + Runners[leg]->getRunningTime();
            else {
              int prev = getLegRunningTime(leg-1, multidayTotal);
              if (prev == 0)
                return 0;
              else
                return Runners[leg]->getRunningTime() + prev;
            }
          }
          else return 0;

        case LTIgnore:
          if (leg==0) 
            return 0;
          else
            return getLegRunningTime(leg-1, multidayTotal);

        break;
      }
    }
    else {
		  int dt=addon + max(Runners[leg]->getFinishTime()-StartTime, 0);
		  int dt2=0;

		  if(leg>0)
        dt2=getLegRunningTime(leg-1, multidayTotal)+Runners[leg]->getRunningTime();
      
		  return max(dt, dt2);
    }
	}
	return 0;
}

string oTeam::getLegRunningTimeS(int leg, bool multidayTotal) const
{
  if (leg==-1)
    leg = Runners.size()-1;

  int rt=getLegRunningTime(leg, multidayTotal);

	if(rt>0) {
		char bf[16];
		if(rt>=3600)
			sprintf_s(bf, 16, "%d:%02d:%02d", rt/3600,(rt/60)%60, rt%60);
		else
			sprintf_s(bf, 16, "%d:%02d", (rt/60), rt%60);

    if (unsigned(leg)<Runners.size() && Runners[leg] && 
        Class && Runners[leg]->getStartTime()==Class->getRestartTime(leg))
		  return string(bf) + "*";
    else
      return bf;
	}
	return "-";
}


RunnerStatus oTeam::getLegStatus(int leg, bool multidayTotal) const
{
  if (leg==-1) 
    leg=Runners.size()-1;
	
  if (unsigned(leg)>=Runners.size())
    return StatusUnknown;

  if (multidayTotal) {
    if (inputStatus == StatusUnknown)
      return StatusUnknown;
    RunnerStatus s = getLegStatus(leg, false);
    return max(inputStatus, s);
  }

  //Let the user specify a global team status disqualified/maxtime.
  if((leg==Runners.size()-1) && (Status==StatusDQ || Status==StatusMAX))
    return Status;

  leg = getLegToUse(leg); // Ignore optional runners

	int s=0;
	
  if (!Class)
    return StatusUnknown;
	
  while(leg>0 && Class->getLegType(leg)==LTIgnore)
    leg--;

	for (int i=0;i<=leg;i++) {
    // Ignore runners to be ignored
    while(i<leg && Class->getLegType(i)==LTIgnore)
      i++;

    int st=Runners[i] ? Runners[i]->getStatus(): StatusDNS;
    int bestTime=Runners[i] ? Runners[i]->getFinishTime():0;

    //When Type Extra is used, the runner with the best time
    //is used for change. Then the status of this runner
    //should be carried forward.
    if(Class) while( (i+1) < int(Runners.size()) && Class->getLegType(i+1)==LTExtra) {
      i++;
      
      if (Runners[i]) {
        if(bestTime==0 || (Runners[i]->getFinishTime()>0 && 
                         Runners[i]->getFinishTime()<bestTime) ) {
          st=Runners[i]->getStatus();        
          bestTime = Runners[i]->getFinishTime();
        }
      }      
    }
		
    if(st==0) 
      return RunnerStatus(s==StatusOK ? 0 : s);

		s=max(s, st);		
	}

  // Allow global status DNS
  if(s==StatusUnknown && Status==StatusDNS)
    return Status;

	return RunnerStatus(s);
}

const string &oTeam::getLegStatusS(int leg, bool multidayTotal) const
{
  return oe->formatStatus(getLegStatus(leg, multidayTotal));
}

int oTeam::getLegPlace(int leg, bool multidayTotal) const {
  if (!multidayTotal)
    return leg>=0 && leg<maxRunnersTeam ? _places[leg].p:_places[Runners.size()-1].p;
  else
    return leg>=0 && leg<maxRunnersTeam ? _places[leg].totalP:_places[Runners.size()-1].totalP;
}


string oTeam::getLegPlaceS(int leg, bool multidayTotal) const
{
	int p=getLegPlace(leg, multidayTotal);
	char bf[16];
	if(p>0 && p<10000){
		sprintf_s(bf, "%d", p);
		return bf;
	}	
  return _EmptyString;
}

string oTeam::getLegPrintPlaceS(int leg, bool multidayTotal) const
{
  int p=getLegPlace(leg, multidayTotal);
	char bf[16];
	if(p>0 && p<10000){
		sprintf_s(bf, "%d.", p);
		return bf;
	}	
  return _EmptyString;
}

bool oTeam::compareResult(const oTeam &a, const oTeam &b)
{
	if (a.Class != b.Class) {
		if(a.Class) {
      if(b.Class) return a.Class->tSortIndex < b.Class->tSortIndex;
			else return true;
		}
		else return false;
	}
	else if (a._sortstatus!=b._sortstatus)
		return a._sortstatus<b._sortstatus;
  else if (a._sorttime!=b._sorttime)
    return a._sorttime<b._sorttime;
  else if(a.StartNo!=b.StartNo)
    return a.StartNo<b.StartNo;

  int aix = a.getDCI().getInt("SortIndex");
  int bix = b.getDCI().getInt("SortIndex");
  if (aix != bix)
    return aix < bix;
  else
    return a.Name<b.Name;
}

bool oTeam::compareStartTime(const oTeam &a, const oTeam &b)
{
  if(a.Class != b.Class) {
		if(a.Class) {
      if(b.Class) return a.Class->tSortIndex<b.Class->tSortIndex;
			else return true;
		}
  }
  else if (a.StartTime != b.StartTime)
    return a.StartTime < b.StartTime;
  else if (a.StartNo != b.StartNo)
    return a.StartNo < b.StartNo;
  
  int aix = a.getDCI().getInt("SortIndex");
  int bix = b.getDCI().getInt("SortIndex");
  if (aix != bix)
    return aix < bix;
  else
    return a.Name<b.Name;
}

bool oTeam::compareSNO(const oTeam &a, const oTeam &b) {
  if (a.StartNo != b.StartNo)
    return a.StartNo<b.StartNo;
  else if(a.Class != b.Class) {
		if(a.Class) {
      if(b.Class) return a.Class->tSortIndex<b.Class->tSortIndex;
			else return true;
		}
  }
  return a.Name<b.Name;
}
	

bool oTeam::isRunnerUsed(int Id) const
{
	for(unsigned i=0;i<Runners.size(); i++) {		
		if(Runners[i] && Runners[i]->Id==Id)
			return true;
	}
	return false;
}

void oTeam::setTeamNoStart(bool dns)
{
  if (dns) {
    setStatus(StatusDNS);
    for(unsigned i=0;i<Runners.size(); i++) {
      if (Runners[i] && Runners[i]->getStatus()==StatusUnknown) {
        Runners[i]->setStatus(StatusDNS);
      }
    }
  }
  else {
    setStatus(StatusUnknown);
    for(unsigned i=0;i<Runners.size(); i++) {
      if (Runners[i] && Runners[i]->getStatus()==StatusDNS) {
        Runners[i]->setStatus(StatusUnknown);
      }
    }
  }
  // Do not sync here
}

static void compressStartTimes(vector<int> &availableStartTimes, int finishTime)
{
  for (size_t j=0; j<availableStartTimes.size(); j++)
    finishTime = max(finishTime, availableStartTimes[j]);

  availableStartTimes.resize(1);
  availableStartTimes[0] = finishTime;
}

static void addStartTime(vector<int> &availableStartTimes, int finishTime)
{
  for (size_t k=0; k<availableStartTimes.size(); k++)
    if (finishTime >= availableStartTimes[k]) {
      availableStartTimes.insert(availableStartTimes.begin()+k, finishTime);
      return;
    }

  availableStartTimes.push_back(finishTime);
}

static int getBestStartTime(vector<int> &availableStartTimes)
{
  if (availableStartTimes.empty())
    return 0;
  int t = availableStartTimes.back();
  availableStartTimes.pop_back();
  return t;
}

bool oTeam::apply(bool sync) 
{
  return apply(sync, 0);
}

bool oTeam::apply(bool sync, pRunner source) 
{
  int lastStartTime = 0;
  RunnerStatus lastStatus = StatusUnknown;
  if(Class && Runners.size()!=size_t(Class->getNumStages()))
    Runners.resize(Class->getNumStages());
  const string bib = getBib();
  tNumRestarts = 0;
  vector<int> availableStartTimes;
	for (size_t i=0;i<Runners.size(); i++) {
    if (!sync && i>0 && source!=0 && Runners[i-1] == source)
      return true;
    if (!Runners[i] && Class) {
       unsigned lr = Class->getLegRunner(i);
        
       if(lr<i && Runners[lr]) {
         Runners[lr]->createMultiRunner(false, sync);
         int dup=Class->getLegRunnerIndex(i); 
         Runners[i]=Runners[lr]->getMultiRunner(dup);
       }
    }

    if (sync && Runners[i] && Class) {
      unsigned lr = Class->getLegRunner(i);
      if (lr == i && Runners[i]->tParentRunner) {
        pRunner parent = Runners[i]->tParentRunner;
        for (size_t kk = 0; kk<parent->multiRunner.size(); ++kk) {
          if (parent->multiRunner[kk] == Runners[i]) {
            parent->multiRunner.erase(parent->multiRunner.begin() + kk);
            Runners[i]->tParentRunner = 0;
            Runners[i]->tDuplicateLeg = 0;
            parent->markForCorrection();
            parent->updateChanged();
            Runners[i]->markForCorrection();
            Runners[i]->updateChanged();
            break;
          }
        }
      }
    }
    // Avoid duplicates, same runner running more 
    // than one leg 
    //(note: quadric complexity, assume total runner count is low)
    if (Runners[i]) {
      for (size_t k=0;k<i; k++)
        if (Runners[i] == Runners[k])
          Runners[i] = 0;
    }

		if (Runners[i]) {
      if (Runners[i]->tInTeam && Runners[i]->tInTeam!=this) {
        Runners[i]->tInTeam->correctRemove(Runners[i]);
      }
      //assert(Runners[i]->tInTeam==0 || Runners[i]->tInTeam==this);
			Runners[i]->tInTeam=this;
			Runners[i]->tLeg=i;
      Runners[i]->setStartNo(StartNo);
      if (!bib.empty() && Runners[i]->isChanged())
        Runners[i]->setBib(bib, false);

      if(Runners[i]->Class!=Class) {
        Runners[i]->Class=Class;
        Runners[i]->updateChanged();//Changed=true;
      }

      Runners[i]->tNeedNoCard=false;
      if (Class) {
        pClass pc=Class;

        //Ignored runners need no SI-card (used by SI assign function)
        if(pc->getLegType(i)==LTIgnore) {
          Runners[i]->tNeedNoCard=true;
          if (lastStatus != StatusUnknown) {
            Runners[i]->setStatus(max(Runners[i]->Status, lastStatus));
          }
        }
        else
          lastStatus = Runners[i]->getStatus();
  
        StartTypes st = pc->getStartType(i);
        LegTypes lt = pc->getLegType(i);

        if ((lt==LTParallel || lt==LTParallelOptional) && i==0) {
          pc->setLegType(0, LTNormal);
          throw std::exception("Första sträckan kan inte vara parallell.");
        }
        if(lt==LTIgnore || lt==LTExtra) {
          if (st != STDrawn)
            Runners[i]->setStartTime(lastStartTime);
          Runners[i]->tUseStartPunch = (st == STDrawn);
        }
        else { //Calculate start time.
          int probeIndex = 1;
          switch (st) {
            case STDrawn: //Do nothing
              if (lt==LTParallel || lt==LTParallelOptional) {
                Runners[i]->setStartTime(lastStartTime);
                Runners[i]->tUseStartPunch=false;
              }
              else
                lastStartTime = Runners[i]->getStartTime();

              break;

            case STTime:
              if(lt==LTNormal || lt==LTSum)
                lastStartTime=pc->getStartData(i);

              Runners[i]->setStartTime(lastStartTime);
              Runners[i]->tUseStartPunch=false;
            break;

            case STChange:
              // Allow for empty slots when ignore/extra
              while ((i-probeIndex)>=0 && !Runners[i-probeIndex]) {
                LegTypes tlt = pc->getLegType(i-probeIndex);
                if (tlt == LTIgnore || tlt==LTExtra)
                  probeIndex++;
                else 
                  break;
              }

              if ( (i-probeIndex)>=0 && Runners[i-probeIndex]) {
                int z = i-probeIndex;
                LegTypes tlt = pc->getLegType(z);
                int ft = 0;
                if (availableStartTimes.empty()) {
                  //We are not involved in parallel legs
                  ft = (tlt != LTIgnore) ? Runners[z]->getFinishTime() : 0;

                  // Take the best time for extra runners
                  while (z>0 && (tlt==LTExtra || tlt==LTIgnore)) { 
                    tlt = pc->getLegType(--z);
                    if (Runners[z] && Runners[z]->getStatus() == StatusOK) {
                      int tft = Runners[z]->getFinishTime();
                      if (tft>0 && tlt != LTIgnore) 
                        ft = ft>0 ? min(tft, ft) : tft;
                    }
                  }              
                }
                else 
                  ft = getBestStartTime(availableStartTimes);
                
                if(ft<=0)
                  ft=0;

                int restart=pc->getRestartTime(i);
                int rope=pc->getRopeTime(i);

                if ((restart>0 && rope>0 && (ft==0 || ft>rope)) || (ft == 0 && restart>0)) {
                  ft=restart; //Runner in restart
                  tNumRestarts++;
                }

                if (ft > 0)
                  Runners[i]->setStartTime(ft);
                Runners[i]->tUseStartPunch=false;
                lastStartTime=ft;
              }
              else {//The else below should only be run by mistake (for an incomplete team)
                Runners[i]->setStartTime(Class->getRestartTime(i));
                Runners[i]->tUseStartPunch=false;
                //Runners[i]->setStatus(StatusDNS);
              }
            break;

            case STHunting: {
              bool setStart = false;
              if (i>0 && Runners[i-1]) {
                int rt = this->getLegRunningTime(i-1, false);
                if (rt>0)
                  setStart = true;
                int leaderTime = pc->getTotalLegLeaderTime(i-1, false);
                int timeAfter = leaderTime > 0 ? rt - leaderTime : 0;

                if(rt>0 && timeAfter>=0)
                  lastStartTime=pc->getStartData(i)+timeAfter;
                
                int restart=pc->getRestartTime(i);
                int rope=pc->getRopeTime(i);

                RunnerStatus hst = getLegStatus(i-1, false);
                if (hst != StatusUnknown && hst != StatusOK) {
                  setStart = true;
                  lastStartTime = restart;
                }

                if (restart && rope && (lastStartTime>rope)) {
                  lastStartTime=restart; //Runner in restart  
                  tNumRestarts++;
                }
                if (Runners[i]->getFinishTime()>0)
                  setStart = true;                
                if (!setStart)
                  lastStartTime=0;
              }
              else 
                lastStartTime=0;

              Runners[i]->tUseStartPunch=false;
              if (sync)
                Runners[i]->setStartTime(lastStartTime);
            }
            break;
          }
        }

        //Add available start times for parallel
        if (i+1 < Runners.size()) {
          st=pc->getStartType(i+1);
          int finishTime = Runners[i]->getFinishTime();

          if (st==STDrawn || st==STTime)
            availableStartTimes.clear();
          else if (finishTime>0) {
            int nRCurrent = pc->getNumParallel(i);
            int nRNext = pc->getNumParallel(i+1);
            if (nRCurrent>1 || nRNext>1) {
              if (nRCurrent<nRNext) {
                for (int j=0; j<nRNext/nRCurrent; j++)
                  availableStartTimes.push_back(finishTime);
              }
              else if (nRNext==1)
                compressStartTimes(availableStartTimes, finishTime);
              else
                addStartTime(availableStartTimes, finishTime);
            }
            else
              availableStartTimes.clear();
          }
        }      
      }
      if(sync)
        Runners[i]->synchronize(true);
    }
	}

  if(!Runners.empty() && Runners[0]) 
    setStartTime(Runners[0]->getStartTime());
  else if(Class)
    setStartTime(Class->getStartData(0));

  setFinishTime(getLegFinishTime(-1));
  setStatus(getLegStatus(-1, false));

  if(sync)
    synchronize(true);
	return true;
}

void oTeam::evaluate(bool sync)
{
  apply(false, 0);
  vector<int> mp;
  for(unsigned i=0;i<Runners.size(); i++) 
    if(Runners[i])
      Runners[i]->evaluateCard(mp);
	apply(sync, 0);
}

void oTeam::correctRemove(pRunner r) {
  for(unsigned i=0;i<Runners.size(); i++) 
    if(r!=0 && Runners[i]==r) {
      Runners[i] = 0;
      r->tInTeam = 0;
      r->tLeg = 0;
      correctionNeeded = true;
      r->correctionNeeded = true;
    }
}

void oTeam::fillSpeakerObject(int leg, int controlId, oSpeakerObject &spk) const
{
  if(leg==-1)
    leg = Runners.size()-1;
  
  spk.club = getName();
  spk.isRendered = false;

  spk.preliminaryRunningTimeLeg = 0;
  spk.runningTimeLeg = 0;
  spk.runningTime = 0;
  spk.status = StatusUnknown;
    
  spk.missingStartTime = true;
  //Defaults (if early return)
  spk.status = spk.finishStatus = StatusUnknown;
  spk.priority = 0;
  spk.preliminaryRunningTime = 0;
  spk.owner = 0;
  spk.name.clear();
  if(!Class || unsigned(leg) >= Runners.size()) 
    return;

  if(!Runners[leg])
    return;

  // Get many names for paralell legs
  int firstLeg = leg;
  int requestedLeg = leg;
  LegTypes lt=Class->getLegType(firstLeg--);
  while(firstLeg>=0 && (lt==LTIgnore || lt==LTParallel || lt==LTParallelOptional || lt==LTExtra) ) 
    lt=Class->getLegType(firstLeg--);

  spk.name.clear();
  for(int k=firstLeg+1;k<=leg;k++) {
    if(Runners[k]) {
      const string &n=Runners[k]->getName();
      if(spk.name.empty())
        spk.name=n;
      else
        spk.name+="/"+n;
    }
  }
  // Add start number
  char bf[512];
  sprintf_s(bf, "%03d: %s", StartNo, spk.name.c_str());
  spk.name=bf;

  leg=firstLeg+1; //This does not work well with parallel or extra...

  Runners[leg]->getSplitTime(controlId, spk.status, spk.runningTimeLeg);

  spk.owner=Runners[leg];
  spk.finishStatus=getLegStatus(leg, false);
  int st=Runners[leg]->getStartTime();
  spk.startTimeS = Runners[leg]->getStartTimeCompact();
  spk.missingStartTime = st <= 0;

	map<int,int>::iterator mapit=Runners[leg]->Priority.find(controlId);
	if(mapit!=Runners[leg]->Priority.end())
		spk.priority=mapit->second;
  else 
    spk.priority=0;

  spk.preliminaryRunningTimeLeg=Runners[leg]->getPrelRunningTime();

  int timeOffset=0;
  RunnerStatus inheritStatus = StatusUnknown;
  if (firstLeg>=0) {
    timeOffset = getLegRunningTime(firstLeg, false);
    inheritStatus = getLegStatus(firstLeg, false);
  }

  if (inheritStatus>StatusOK)
    spk.status=inheritStatus;

  if(spk.status==StatusOK) {
    if (controlId != 2)
      spk.runningTime = spk.runningTimeLeg+timeOffset;
    else
      spk.runningTime = getLegRunningTime(requestedLeg, false); // Get official time

    spk.preliminaryRunningTime = spk.runningTime;
    spk.preliminaryRunningTimeLeg = spk.runningTimeLeg;
  }
  else if(spk.status==StatusUnknown && spk.finishStatus==StatusUnknown) {
    spk.runningTime = spk.preliminaryRunningTimeLeg + timeOffset;
    spk.preliminaryRunningTime = spk.preliminaryRunningTimeLeg + timeOffset;
    spk.runningTimeLeg = spk.preliminaryRunningTimeLeg;
  }
  else if(spk.status==StatusUnknown)
    spk.status=StatusMP;
}

int oTeam::getTimeAfter(int leg) const
{
  if(leg==-1)
    leg=Runners.size()-1;

  if(!Class || Class->tLeaderTime.size()<=unsigned(leg))
    return -1;

  int t=getLegRunningTime(leg, false);

  if (t<=0)
    return -1;

  return t-Class->getTotalLegLeaderTime(leg, false);
}

int oTeam::getLegStartTime(int leg) const
{
  if(leg==0)
    return StartTime;
  else if(unsigned(leg)<Runners.size() && Runners[leg])
    return Runners[leg]->getStartTime();
  else return 0;
}

string oTeam::getLegStartTimeS(int leg) const
{
  int s=getLegStartTime(leg);
	if(s>0)
		return oe->getAbsTime(s);
	else return "-";
}

string oTeam::getLegStartTimeCompact(int leg) const
{
  int s=getLegStartTime(leg);
	if(s>0)
    if(oe->useStartSeconds())
		  return oe->getAbsTime(s);
    else
      return oe->getAbsTimeHM(s);

	else return "-";
}

string oTeam::getBib() const
{
  return getDCI().getString("Bib");
}

void oTeam::setBib(const string &bib, bool updateStartNo)
{
  getDI().setString("Bib", bib);
  if (updateStartNo) {
    setStartNo(atoi(bib.c_str()));
  }
}

oDataInterface oTeam::getDI(void) 
{
	return oe->oTeamData->getInterface(oData, sizeof(oData), this);
}

oDataConstInterface oTeam::getDCI(void) const
{
	return oe->oTeamData->getConstInterface(oData, sizeof(oData), this);
}

pRunner oTeam::getRunner(unsigned leg) const
{
  if(leg==-1)
    leg=Runners.size()-1;

  return leg<Runners.size() ? Runners[leg]: 0;
}

int oTeam::getRogainingPoints() const
{
  int pt = 0;
  for (size_t k = 0; k < Runners.size(); k++) {
    if (Runners[k])
      pt += Runners[k]->tRogainingPoints;
  }
  return pt;
}

void oTeam::remove() 
{
  if (oe)
    oe->removeTeam(Id);
}

bool oTeam::canRemove() const 
{
  return true;
}

string oTeam::getDisplayName() const {
  if (!Class)
    return Name;

  ClassType ct = Class->getClassType();
  if (ct == oClassIndividRelay || ct == oClassPatrol) {
    if (Club) {
      string cname = getDisplayClub();
      if (!cname.empty())
        return cname;
    }
  }
  return Name;
}

string oTeam::getDisplayClub() const {
  vector<pClub> clubs;
  if (Club)
    clubs.push_back(Club);
  for (size_t k = 0; k<Runners.size(); k++) {
    if (Runners[k] && Runners[k]->Club) {
      if (count(clubs.begin(), clubs.end(), Runners[k]->Club) == 0)
        clubs.push_back(Runners[k]->Club);
    }
  }
  if (clubs.size() == 1)
    return clubs[0]->getDisplayName();
  
  string res;
  for (size_t k = 0; k<clubs.size(); k++) {
    if (k == 0)
      res = clubs[k]->getDisplayName();
    else
      res += " / " + clubs[k]->getDisplayName();
  }
  return res;
}

void oTeam::setInputData(const oTeam &t) {
  inputTime = t.getRunningTime() + t.inputTime;
  inputStatus = t.getTotalStatus();
  inputPoints = t.getRogainingPoints() + t.inputPoints;
  int tp = t.getTotalPlace();
  inputPlace = tp < 99000 ? tp : 0;
}

RunnerStatus oTeam::getTotalStatus() const {
  if (Status == StatusUnknown)
    return StatusUnknown;
  else if (inputStatus == StatusUnknown)
    return StatusDNS;

  return max(Status, inputStatus);
}

void oEvent::getTeams(int classId, vector<pTeam> &t, bool sort) {
  if (sort) {
    synchronizeList(oLTeamId);
    //sortTeams(SortByName);
  }
  t.clear();
  if (Classes.size() > 0)
    t.reserve((Teams.size()*min<size_t>(Classes.size(), 4)) / Classes.size());

  for (oTeamList::iterator it = Teams.begin(); it != Teams.end(); ++it) {
    if (it->isRemoved())
      continue;

    if (classId == 0 || it->getClassId() == classId)
      t.push_back(&*it);
  }
}

string oTeam::getEntryDate(bool dummy) const {
  oDataConstInterface dci = getDCI();
  int date = dci.getInt("EntryDate");
  if (date == 0) {
    (const_cast<oTeam *>(this)->getDI()).setDate("EntryDate", getLocalDate());
  }
  return dci.getDate("EntryDate");
}

unsigned static nRunnerMaxStored = -1;

Table *oEvent::getTeamsTB() //Table mode
{	
  
  vector<pClass> cls;
  oe->getClasses(cls);
  unsigned nRunnerMax = 0;
  for (size_t k = 0; k < cls.size(); k++) {
    nRunnerMax = max(nRunnerMax, cls[k]->getNumStages());
  }

  bool forceUpdate = nRunnerMax != nRunnerMaxStored;

  if (forceUpdate && tables.count("team")) {
    tables["team"]->releaseOwnership();
    tables.erase("team");
  }

  if (tables.count("team") == 0) {

 	  oCardList::iterator it;	

    Table *table=new Table(this, 20, "Lag(flera)", "teams");

    table->addColumn("Id", 70, true, true);
    table->addColumn("Ändrad", 70, false);

	  table->addColumn("Namn", 200, false);
    table->addColumn("Klass", 120, false);
	  table->addColumn("Klubb", 120, false);

    table->addColumn("Start", 70, false, true);
    table->addColumn("Mål", 70, false, true);
    table->addColumn("Status", 70, false);
    table->addColumn("Tid", 70, false, true);

    table->addColumn("Plac.", 70, true, true);
    table->addColumn("Start nr.", 70, true, false);
     
    for (unsigned k = 0; k < nRunnerMax; k++) {
      table->addColumn("Sträcka X#" + itos(k+1), 200, false, false);
      table->addColumn("Bricka X#" + itos(k+1), 70, true, true);
    }
    nRunnerMaxStored = nRunnerMax;

    oe->oTeamData->buildTableCol(table);

    table->addColumn("Tid in", 70, false, true);
    table->addColumn("Status in", 70, false, true);
    table->addColumn("Poäng in", 70, true);
    table->addColumn("Placering in", 70, true);

    tables["team"] = table;
    table->addOwnership();
  }
  
  tables["team"]->update();
  return tables["team"];
}


void oEvent::generateTeamTableData(Table &table, oTeam *addTeam)
{ 
  vector<pClass> cls;
  oe->getClasses(cls);
  unsigned nRunnerMax = 0;

  for (size_t k = 0; k < cls.size(); k++) {
    nRunnerMax = max(nRunnerMax, cls[k]->getNumStages());
  }

  if (nRunnerMax != nRunnerMaxStored)
    throw meosException("Internal table error: Restart MeOS");

  if (addTeam) {
    addTeam->addTableRow(table);
    return;
  }

  synchronizeList(oLTeamId);
	oTeamList::iterator it;	
  table.reserve(Teams.size());
  for (it=Teams.begin(); it != Teams.end(); ++it){		
		if(!it->skip()){
      it->addTableRow(table);
		}
	}
}

void oTeam::addTableRow(Table &table) const {
  oRunner &it = *pRunner(this);
  table.addRow(getId(), &it);	

  int row = 0;
  table.set(row++, it, TID_ID, itos(getId()), false);
  table.set(row++, it, TID_MODIFIED, getTimeStamp(), false);			

  table.set(row++, it, TID_NAME, getName(), true);
  table.set(row++, it, TID_CLASSNAME, getClass(), true, cellSelection);			
  table.set(row++, it, TID_CLUB, getClub(), true, cellCombo);

  table.set(row++, it, TID_START, getStartTimeS(), true);
  table.set(row++, it, TID_FINISH, getFinishTimeS(), true);
  table.set(row++, it, TID_STATUS, getStatusS(), true, cellSelection);
  table.set(row++, it, TID_RUNNINGTIME, getRunningTimeS(), false);

  table.set(row++, it, TID_PLACE, getPlaceS(), false);
  table.set(row++, it, TID_STARTNO, itos(getStartNo()), true);

  for (unsigned k = 0; k < nRunnerMaxStored; k++) {
    pRunner r = getRunner(k);
    if (r) {
      table.set(row++, it, 100+2*k, r->getName(), r->getRaceNo() == 0);
      table.set(row++, it, 101+2*k, itos(r->getCardNo()), true);
    }
    else {
      table.set(row++, it, 100+2*k, "", Class && Class->getLegRunner(k) == k);
      table.set(row++, it, 101+2*k, "", false);
    }
  }

  row = oe->oTeamData->fillTableCol(oData, it, table, true);

  table.set(row++, it, TID_INPUTTIME, getInputTimeS(), true);
  table.set(row++, it, TID_INPUTSTATUS, getInputStatusS(), true, cellSelection);
  table.set(row++, it, TID_INPUTPOINTS, itos(inputPoints), true);
  table.set(row++, it, TID_INPUTPLACE, itos(inputPlace), true);
}

bool oTeam::inputData(int id, const string &input, 
                        int inputId, string &output, bool noUpdate)
{
  int s,t;
  synchronize(false);
 
  if(id>1000) {
    return oe->oTeamData->inputData(this, oData, id, input, 
                                        inputId, output, noUpdate);
  }
  else if (id >= 100) {

    bool isName = (id&1) == 0;
    size_t ix = (id-100)/2;
    if (ix < Runners.size()) {
      if (Runners[ix]) {
        if (isName) {
          if (input.empty()) {
            removeRunner(oe->gdibase, false, ix);
          }
          else {
            Runners[ix]->setName(input);
            Runners[ix]->synchronize(true);
            output = Runners[ix]->getName();
          }
        }
        else {
          Runners[ix]->setCardNo(atoi(input.c_str()), true);
          Runners[ix]->synchronize(true);
          output = itos(Runners[ix]->getCardNo());
        }
      }
      else {
        if (isName && !input.empty() && Class) {
          pRunner r = oe->addRunner(input, getClubId(), getClassId(), 0, 0, false);
          setRunner(ix, r, true);
          output = r->getName();
        }

      }
    }
  }

  switch(id) {

    case TID_NAME:
      setName(input);
      synchronize(true);
      output = getName();
      break;

    case TID_START:
      setStartTimeS(input);
      t=getStartTime();      
      s=getStartTime();
      if (s!=t) 
        throw std::exception("Starttiden är definerad genom klassen eller löparens startstämpling.");
      synchronize(true);
      output=getStartTimeS();
      return true;
    break;

    case TID_CLUB: 
      {
        pClub pc = 0;
        if (inputId > 0)
          pc = oe->getClub(inputId);
        else
          pc = oe->getClubCreate(0, input);

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
      RunnerStatus sIn = RunnerStatus(inputId);
      if (sIn != StatusDNS) {
        if (getStatus() == StatusDNS && sIn == StatusUnknown)
          setTeamNoStart(false);
        else
          setStatus(sIn);
      }
      else if (getStatus() != StatusDNS)
        setTeamNoStart(true);

      apply(true, 0);
      RunnerStatus sOut = getStatus();
      if (sOut != sIn)
        throw meosException("Status matchar inte deltagarnas status.");
      output = getStatusS();
    }
    break;

    case TID_STARTNO:
      setStartNo(atoi(input.c_str()));
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

void oTeam::fillInput(int id, vector< pair<string, size_t> > &out, size_t &selected)
{ 
  if(id>1000) {
    oe->oRunnerData->fillInput(oData, id, 0, out, selected);
    return;
  }
  else if (id==TID_CLASSNAME) {
    oe->fillClasses(out, oEvent::extraNone, oEvent::filterOnlyMulti);
    out.push_back(make_pair(lang.tl("Ingen klass"), 0));
    selected = getClassId();
  }  
  else if (id==TID_CLUB) {
    oe->fillClubs(out);
    out.push_back(make_pair(lang.tl("Klubblös"), 0));
    selected = getClubId();
  }
  else if (id==TID_STATUS || id==TID_INPUTSTATUS) {
    oe->fillStatus(out);
    selected = getStatus();
  }
}

void oTeam::removeRunner(gdioutput &gdi, bool askRemoveRunner, int i) {
  pRunner p_old = getRunner(i);
  setRunner(i, 0, true);

  //Remove multi runners
  if (Class) {
    for (unsigned k = i+1;k < Class->getNumStages(); k++) { 
      if (Class->getLegRunner(k)==i) 
        setRunner(k, 0, true);
    }
  }

  //No need to delete multi runners. Disappears when parent is gone.
  if(p_old && !oe->isRunnerUsed(p_old->getId())){
    if(!askRemoveRunner || gdi.ask("Ska X raderas från tävlingen?#" + p_old->getName())){
	    vector<int> oldR;
      oldR.push_back(p_old->getId());
      oe->removeRunner(oldR);
    }
  }
}

int oTeam::getTeamFee() const {
  int f = getDCI().getInt("Fee");
  for (size_t k = 0; k < Runners.size(); k++) {
    if (Runners[k])
      f += Runners[k]->getDCI().getInt("Fee");
  }
  return f;
}