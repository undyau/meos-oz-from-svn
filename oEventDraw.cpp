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
    GNU General Public License fro more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    Melin Software HB - software@melin.nu - www.melin.nu
    Stigbergsvägen 7, SE-75242 UPPSALA, Sweden
    
************************************************************************/

#include "stdafx.h"

#include <vector>
#include <set>
#include <cassert>
#include <algorithm>

#include "oEvent.h"
#include "gdioutput.h"
#include "oDataContainer.h"

#include "random.h"

#include "meos.h"
#include "meos_util.h"
#include "localizer.h"

int ClassInfo::sSortOrder=0;

bool ClassInfo::operator <(ClassInfo &ci)
{
	if(sSortOrder==0)
		return nRunners>ci.nRunners;
	else
		return firstStart<ci.firstStart;
}

struct ClassBlockInfo{
	int FirstControl;
	int nRunners;
	int Depth;

	int FirstStart;

	bool operator<(ClassBlockInfo &ci);
};

bool ClassBlockInfo::operator <(ClassBlockInfo &ci)
{
	return Depth<ci.Depth;
}

bool isFree(vector< vector<int> > &StartField, int nFields, 
            int FirstPos, int PosInterval, ClassInfo &cInfo)
{
  int Type = cInfo.unique;
  int nEntries = cInfo.nRunners;

  // Adjust first pos to make room for extra (before first start)
  if (cInfo.nExtra>0) {
    int newFirstPos = FirstPos - cInfo.nExtra * PosInterval;
    while (newFirstPos<0)
      newFirstPos += PosInterval;
    int extra = (FirstPos - newFirstPos) / PosInterval;
    nEntries += extra;
    FirstPos = newFirstPos;
  }

	//Check if free at all...
	for(int k = 0; k < nEntries; k++){
		bool HasFree=false;
		for(int f=0;f<nFields;f++){
			int t = StartField[f][FirstPos+k*PosInterval];
			if(t == 0)
				HasFree=true;
			else if(t == Type)
				return false;//Type of course occupied. Cannot put it here;
		}

		if(!HasFree) return false;//No free start position.
	}

	return true;
}

bool insertStart(vector< vector<int> > &StartField, int nFields, ClassInfo &cInfo)
{
  int Type = cInfo.unique;
  int nEntries = cInfo.nRunners;
  int FirstPos = cInfo.firstStart;
  int PosInterval = cInfo.interval;

  // Adjust first pos to make room for extra (before first start)
  if (cInfo.nExtra>0) {
    int newFirstPos = FirstPos - cInfo.nExtra * PosInterval;
    while (newFirstPos<0)
      newFirstPos += PosInterval;
    int extra = (FirstPos - newFirstPos) / PosInterval;
    nEntries += extra;
    FirstPos = newFirstPos;
  }

	for (int k=0; k<nEntries; k++) {
		bool HasFree=false;	

		for (int f=0;f<nFields && !HasFree;f++) {
			if (StartField[f][FirstPos+k*PosInterval]==0) {
				StartField[f][FirstPos+k*PosInterval]=Type;
				HasFree=true;
			}
		}

		if (!HasFree) 
      return false; //No free start position. Fail.
	}

	return true;
}

void oEvent::optimizeStartOrder(gdioutput &gdi, DrawInfo &di, vector<ClassInfo> &cInfo)
{
	if(Classes.size()==0)
		return;

  struct StartParam {
    int nControls;
    int alternator;
    double badness;
    StartParam() : nControls(1), alternator(1), badness(1000) {}
  };

  StartParam opt;
  bool found = false;
  int nCtrl = 1;

  while (!found) {

    for (int alt = 1; alt <= 5 && !found; alt++) {
      vector< vector<int> > startField(di.nFields);
      optimizeStartOrder(startField, di, cInfo, nCtrl, alt);

	    int overShoot = 0;
      int overSum = 0;
      int numOver = 0;
      for (size_t k=0;k<cInfo.size();k++) {
        const ClassInfo &ci = cInfo[k];
        if (ci.overShoot>0) {
          numOver++;
          overShoot = max (overShoot, ci.overShoot);
          overSum += ci.overShoot;
        }
		    //laststart=max(laststart, ci.firstStart+ci.nRunners*ci.interval);
	    }
      double avgShoot = double(overSum)/cInfo.size();
      double badness = overShoot==0 ? 0 : overShoot / avgShoot;

      if (badness<opt.badness) {
        opt.badness = badness;
        opt.alternator = alt;
        opt.nControls = nCtrl;
      }

      if (badness < 5.0) {
        found = true;
      }
    }

    if (!found) {
      nCtrl++;
    }

    if (nCtrl>5) //We need some limit
      found = true;
  }

  vector< vector<int> > startField(di.nFields);
  optimizeStartOrder(startField, di, cInfo, opt.nControls, opt.alternator);

  //Find last starter
  int last = 0;
  for (int k=0;k<di.nFields;k++) {
    for (size_t j=0;j<startField[k].size(); j++)
			if(startField[k][j])
				last = max(last, int(j));
	}

  int nr;
	int T=0;
	int sum=0;
	gdi.addString("", 1, "Antal startande per intervall (inklusive redan lottade):");
	string str="";
	int empty=4;

	while (T <= last) {
		nr=0;
		for(size_t k=0;k<startField.size();k++){
			if(startField[k][T])
				nr++;
		}
		T++;
		sum+=nr;
		if(nr!=0) empty=4;
		else empty--;
	
		char bf[20];
		sprintf_s(bf, 20, "%d ", nr);
		str+=bf;
	}

	gdi.addStringUT(10, str);
	gdi.dropLine();
}



void oEvent::optimizeStartOrder(vector< vector<int> > &StartField, DrawInfo &di, 
                                vector<ClassInfo> &cInfo, int useNControls, int alteration)
{

  if (di.firstStart<=0)
    di.firstStart = 0;

  map<int, ClassInfo> otherClasses;
	cInfo.clear();
	oClassList::iterator c_it;
	for (c_it=Classes.begin(); c_it != Classes.end(); ++c_it) {
    bool drawClass = di.classes.count(c_it->getId())>0;
    ClassInfo *cPtr = 0;
    
    if (!drawClass) {
      otherClasses[c_it->getId()] = ClassInfo(c_it->getId());
      cPtr = &otherClasses[c_it->getId()];
    }
    else
      cPtr = &di.classes[c_it->getId()];

    ClassInfo &ci =  *cPtr;
		pCourse pc = c_it->getCourse();

		if (pc) {
			if(useNControls>0 && pc->nControls>0)
				ci.unique = 1000000 + pc->getIdSum(useNControls);
			else
				ci.unique = 10000 + pc->getId();
			
			ci.courseId = pc->getId();
		}
		else 
      ci.unique = ci.classId;

    if (!drawClass) 
      continue;

    if (ci.nVacant == -1) {
      int nr = c_it->getNumRunners();
      // Auto initialize
      int nVacancies = int(nr * di.vacancyFactor + 0.5);
      nVacancies = max(nVacancies, di.minVacancy);
      nVacancies = min(nVacancies, di.maxVacancy);
      nVacancies = max(nVacancies, 0);
      
      ci.nExtra = max(int(nr * di.extraFactor + 0.5), 1);

      if (di.vacancyFactor == 0)
        nVacancies = 0;

      if (di.extraFactor == 0)
        ci.nExtra = 0;

      ci.nVacant = nVacancies;
		  ci.nRunners = nr + nVacancies;
    }
    else {
      ci.nRunners = c_it->getNumRunners() + ci.nVacant;
    }

		if(ci.nRunners>0)
			cInfo.push_back(ci);
	}

  sort(cInfo.begin(), cInfo.end());

  int maxSize=cInfo.empty() ? 0 : di.minClassInterval*cInfo[0].nRunners;

  // Special case for constant time start
  if (di.baseInterval==0) {
    di.baseInterval = 1;
    di.minClassInterval = 0;
  }


  // Calculate the theoretical best end position to use.
  int bestEndPos = maxSize / di.baseInterval;

  // Calculate an estimated maximal class intervall
  for (size_t k = 0; k < cInfo.size(); k++) {
		int quotient = maxSize/(cInfo[k].nRunners*di.baseInterval);
		
		if(quotient*di.baseInterval > di.maxClassInterval)
			quotient=di.maxClassInterval/di.baseInterval;

		cInfo[k].interval=quotient;
	}

	for(int m=0;m < di.nFields;m++)
		StartField[m].resize(3000, 0);

	int alternator = 0;

  // Fill up with non-drawn classes
  for (oRunnerList::iterator it = Runners.begin(); it!=Runners.end(); ++it) {
    int st = it->getStartTime();
    int relSt = st-di.firstStart;
    int relPos = relSt / di.baseInterval;

    if (st>0 && relSt>=0 && relPos<3000 && (relSt%di.baseInterval) == 0) {
      if (otherClasses.count(it->getClassId())==0)
        continue;

      if (!di.startName.empty() && it->Class && it->Class->getStart()!=di.startName)
        continue;

      ClassInfo &ci = otherClasses[it->getClassId()];
      int k = 0;
      while(true) {
        if (k==StartField.size())
          StartField.push_back(vector<int>(3000));

        if (StartField[k][relPos]==0) {
          StartField[k][relPos] = ci.unique;
          break;
        }
        k++;
      }
    }
  }

  // Fill up classes with fixed starttime
  for (size_t k = 0; k < cInfo.size(); k++) {
    if (cInfo[k].hasFixedTime) {
      insertStart(StartField, di.nFields, cInfo[k]);
    }  
  }

  if (di.minClassInterval == 0) {
    // Set fixed start time
    for (size_t k = 0; k < cInfo.size(); k++) {
      if (cInfo[k].hasFixedTime)
        continue;
      cInfo[k].firstStart = di.firstStart;
      cInfo[k].interval = 0;
    }
  }
  else {
    // Do the distribution
	  for (size_t k = 0; k < cInfo.size(); k++) {
      if (cInfo[k].hasFixedTime)
        continue;

		  int minPos = 1000000;
      int minEndPos = 1000000;
		  int minInterval=cInfo[k].interval;

		  for (int i = di.minClassInterval/di.baseInterval; i<=cInfo[k].interval; i++) {
  			
        int startpos = alternator % max(1, (bestEndPos - cInfo[k].nRunners * i)/3);
        int ipos = startpos;
        int t = 0;

        while( !isFree(StartField, di.nFields, ipos, i, cInfo[k]) ) {
				  t++;

          // Algorithm to randomize start position
          // First startpos -> bestEndTime, then 0 -> startpos, then remaining
          if (t<(bestEndPos-startpos))
            ipos = startpos + t;
          else {
            ipos = t - (bestEndPos-startpos);
            if (ipos>=startpos)
              ipos  = t;
          }
        }

        int endPos = ipos + i*cInfo[k].nRunners;
			  if (endPos < minEndPos || endPos < bestEndPos) {
				  minEndPos = endPos;
          minPos = ipos;
				  minInterval = i;
			  }
		  }

      cInfo[k].firstStart = minPos;
		  cInfo[k].interval = minInterval;
      cInfo[k].overShoot = max(minEndPos - bestEndPos, 0);
		  insertStart(StartField, di.nFields, cInfo[k]);

      alternator += alteration;
	  }
  }

}

void oEvent::drawRemaining(bool useSOFTMethod, bool placeAfter)
{
  DrawType drawType = placeAfter ? remainingAfter : remainingBefore;

  for (oClassList::iterator it = Classes.begin(); it !=  Classes.end(); ++it) {
    drawList(it->getId(), 0, 0, 0, 0, useSOFTMethod, false, drawType);
  }
}

void oEvent::drawList(int ClassID, int leg, int FirstStart, 
                      int Interval, int Vacances, 
                      bool useSOFTMethod, bool pairwise, DrawType drawType)
{
	autoSynchronizeLists(false);

	oRunnerList::iterator it;		

	int VacantClubId=getVacantClub();

  pClass pc=getClass(ClassID);

  if(!pc)
    throw std::exception("Klass saknas");

  if (Vacances>0 && pc->getClassType()==oClassRelay)
    throw std::exception("Vakanser stöds ej i stafett");

  if (Vacances>0 && leg>0)
    throw std::exception("Det går endast att sätta in vakanser på sträcka 1");
  
  vector<pRunner> runners;
  runners.reserve(Runners.size());

  if (drawType == drawAll) {
    if (leg==0) {
      //Only remove vacances on leg 0.
      vector<int> toRemove;
      //Remove old vacances
      for (it=Runners.begin(); it != Runners.end(); ++it) {
        if(it->Class && it->Class->Id==ClassID) {
          if(it->getClubId()==VacantClubId) {
            toRemove.push_back(it->getId());
          }
        }
      }

      while (!toRemove.empty()) {
        removeRunner(toRemove.back());
        toRemove.pop_back();
      }

	    while (Vacances>0) {
        oe->addRunnerVacant(ClassID);
		    Vacances--;
	    }
    }

	  for (it=Runners.begin(); it != Runners.end(); ++it)
      if(!it->isRemoved() && it->Class && it->Class->Id==ClassID && it->legToRun()==leg ) {
        runners.push_back(&*it);
      }
  }
  else {
    // Find first/last start in class and interval:
    int first=7*24*3600, last = 0;
    set<int> cinterval;
    int baseInterval = 10*60;

    for (it=Runners.begin(); it != Runners.end(); ++it)
      if (!it->isRemoved() && it->getClassId()==ClassID) {
        int st = it->getStartTime();
        if (st>0) {
          first = min(first, st);
          last = max(last, st);
          cinterval.insert(st);
        }
        else
          runners.push_back(&*it);
      }


    // Find start interval
    int t=0;
    for (set<int>::iterator sit = cinterval.begin(); sit!=cinterval.end();++sit) {
      if ( (*sit-t) > 0)
        baseInterval = min(baseInterval, (*sit-t));
      t = *sit;
    }
 
    if (drawType == remainingBefore)
      FirstStart = first - runners.size()*baseInterval;
    else
      FirstStart = last + baseInterval;

    Interval = baseInterval;

    if (last == 0 || FirstStart<=0 ||  baseInterval == 10*60) {
      // Fallback if incorrect specification.
      FirstStart = 3600;
      Interval = 2*60;
    }

  }

  if(runners.empty()) 
    return;

  vector<int> stimes(runners.size());	
  if (!pairwise) {
  	for(unsigned k=0;k<stimes.size(); k++)
	  	stimes[k] = FirstStart+Interval*k;
  }
  else {
  	for(unsigned k=0;k<stimes.size(); k++)
	  	stimes[k] = FirstStart+Interval*(k/2);
  }

  if (useSOFTMethod)
    drawSOFTMethod(runners);
  else 
	  permute(stimes);

  int minStartNo = Runners.size();
  for(unsigned k=0;k<stimes.size(); k++) {
    runners[k]->setStartTime(stimes[k]);
    minStartNo = min(minStartNo, runners[k]->getStartNo());
  }

  CurrentSortOrder = SortByStartTime;
  sort(runners.begin(), runners.end());
  
  if (minStartNo == 0)
    minStartNo = nextFreeStartNo + 1;

  for(size_t k=0; k<runners.size(); k++) {
    runners[k]->setStartNo(k+minStartNo);
	  runners[k]->synchronize();
  }

  nextFreeStartNo = max<int>(nextFreeStartNo, minStartNo + stimes.size());
}


void getLargestClub(map<int, vector<pRunner> > &clubRunner, vector<pRunner> &largest)
{
  size_t maxClub=0;
  for (map<int, vector<pRunner> >::iterator it = 
               clubRunner.begin(); it!=clubRunner.end(); ++it) {
    maxClub = max(maxClub, it->second.size());
  }

  for (map<int, vector<pRunner> >::iterator it = 
               clubRunner.begin(); it!=clubRunner.end(); ++it) {
    if (it->second.size()==maxClub) {
      swap(largest, it->second);
      clubRunner.erase(it);
      return;
    }
  }
}

void getRange(int size, vector<int> &p)
{
  p.resize(size);
  for (size_t k=0;k<p.size();k++)
    p[k] = k;
}

void oEvent::drawSOFTMethod(vector<pRunner> &runners, bool handleBlanks)
{
  if (runners.empty())
    return;

  //Group runners per club
  map<int, vector<pRunner> > clubRunner;

  for (size_t k=0;k<runners.size();k++) {
    int clubId = runners[k] ? runners[k]->getClubId() : -1;
    clubRunner[clubId].push_back(runners[k]);
  }
  
  vector< vector<pRunner> > runnerGroups(1);

  // Find largest club
  getLargestClub(clubRunner, runnerGroups[0]);

  int largeSize = runnerGroups[0].size();
  int ngroups = (runners.size()+largeSize-1) / largeSize;
  runnerGroups.resize(ngroups);

  while (!clubRunner.empty()) {
    // Find the smallest available group
    unsigned small = runners.size()+1;
    int cgroup = -1;
    for (size_t k=1;k<runnerGroups.size();k++)
      if (runnerGroups[k].size()<small) {
        cgroup = k;
        small = runnerGroups[k].size();
      }

    // Add the largest remaining group to the smallest.
    vector<pRunner> largest;
    getLargestClub(clubRunner, largest);
    runnerGroups[cgroup].insert(runnerGroups[cgroup].end(), largest.begin(), largest.end());
  }

  unsigned maxGroup=runnerGroups[0].size();

  //Permute the first group
  vector<int> pg(maxGroup);
  getRange(pg.size(), pg);
  permute(pg);
  vector<pRunner> pr(maxGroup);
  for (unsigned k=0;k<maxGroup;k++)
    pr[k] = runnerGroups[0][pg[k]];
  runnerGroups[0] = pr;

  //Find the largest group
  for (size_t k=1;k<runnerGroups.size();k++) 
    maxGroup = max(maxGroup, runnerGroups[k].size()); 

  if (handleBlanks) {
    //Give all groups same size (fill with 0)
    for (size_t k=1;k<runnerGroups.size();k++) 
      runnerGroups[k].resize(maxGroup); 
  }

  // Apply algorithm recursivly to groups with several clubs
  for (size_t k=1;k<runnerGroups.size();k++) 
    drawSOFTMethod(runnerGroups[k]);

  // Permute the order of groups
  vector<int> p(runnerGroups.size());
  getRange(p.size(), p);
  permute(p);

  // Write back result
  int index = 0;
  for (unsigned level = 0; level<maxGroup; level++) {
    for (size_t k=0;k<runnerGroups.size();k++) {
      int gi = p[k];
      if (level<runnerGroups[gi].size() && (runnerGroups[gi][level]!=0  || !handleBlanks)) 
        runners[index++] = runnerGroups[gi][level];
    }
  }

  if (handleBlanks)
    runners.resize(index);
}


void oEvent::drawListClumped(int ClassID, int FirstStart, int Interval, int Vacances)
{
  pClass pc=getClass(ClassID);

  if(!pc)
    throw std::exception("Klass saknas");

  if (Vacances>0 && pc->getClassType()!=oClassIndividual)
    throw std::exception("Lottningsmetoden stöds ej i den här klassen.");

	oRunnerList::iterator it;		
	int nRunners=0;

	autoSynchronizeLists(false);

	while (Vacances>0)	{
		addRunnerVacant(ClassID);
		Vacances--;
	}

	for (it=Runners.begin(); it != Runners.end(); ++it)
		if(it->Class && it->Class->Id==ClassID) nRunners++;

	if(nRunners==0) return;

	int *stimes=new int[nRunners];	


	//Number of start groups
	//int ngroups=(nRunners/5)
	int ginterval;

	if(nRunners>=Interval)
		ginterval=10;
	else if(Interval/nRunners>60){
		ginterval=40;
	}
	else if(Interval/nRunners>30){
		ginterval=20;
	}
	else if(Interval/nRunners>20){
		ginterval=15;	
	}
	else if(Interval/nRunners>10){
		ginterval=1;	
	}
	else ginterval=10;

	int nGroups=Interval/ginterval+1; //15 s. per interval.
	int k;

	if(nGroups>0){

		int MaxRunnersGroup=max((2*nRunners)/nGroups, 4)+GetRandomNumber(2);
		int *sgroups=new int[nGroups];	

		for(k=0;k<nGroups; k++)
			sgroups[k]=FirstStart+((ginterval*k+2)/5)*5;

		if(nGroups>5){
			//Remove second group...
			sgroups[1]=sgroups[nGroups-1];
			nGroups--;
		}
		
		if(nGroups>9 && ginterval<60 && (GetRandomBit() || GetRandomBit() || GetRandomBit())){
			//Remove third group...
			sgroups[2]=sgroups[nGroups-1];
			nGroups--;

			if(nGroups>13 &&  ginterval<30 && (GetRandomBit() || GetRandomBit() || GetRandomBit())){
				//Remove third group...
				sgroups[3]=sgroups[nGroups-1];
				nGroups--;


				int ng=4; //Max two minutes pause
				while(nGroups>10 && (nRunners/nGroups)<MaxRunnersGroup && ng<8 && (GetRandomBit() || GetRandomBit())){
					//Remove several groups...
					sgroups[ng]=sgroups[nGroups-1];
					nGroups--;
					ng++;
				}
			}
		}

		//Permute some of the groups (not first and last group)
		if(nGroups>5){
			permute(sgroups+2, nGroups-2);

			//Remove some random groups (except first and last).
			for(k=2;k<nGroups; k++){
				if((nRunners/nGroups)<MaxRunnersGroup && nGroups>5){					
					sgroups[k]=sgroups[nGroups-1];
					nGroups--;
				}				
			}
		}

		//Premute all groups;
		permute(sgroups, nGroups);

		int *counters=new int[nGroups];
		memset(counters, 0, sizeof(int)*nGroups);
		
		stimes[0]=FirstStart;
		stimes[1]=FirstStart+Interval;

		for(k=2;k<nRunners; k++){
			int g=GetRandomNumber(nGroups);

			if(counters[g]<=2 && GetRandomBit()){
				//Prefer already large groups
				g=(g+1)%nGroups;
			}

			if(counters[g]>MaxRunnersGroup){
				g=(g+3)%nGroups;
			}

			if(sgroups[g]==FirstStart){
				//Avoid first start
				if(GetRandomBit() || GetRandomBit())
					g=(g+1)%nGroups;
			}
			
			if(counters[g]>MaxRunnersGroup){
				g=(g+2)%nGroups;
			}
			
			if(counters[g]>MaxRunnersGroup){
				g=(g+2)%nGroups;
			}

			stimes[k]=sgroups[g];
			counters[g]++;
		}

		delete[] sgroups;
		delete[] counters;

	}
	else{
		for(k=0;k<nRunners; k++) stimes[k]=FirstStart;
	}

	
	permute(stimes, nRunners);

	k=0;

	for (it=Runners.begin(); it != Runners.end(); ++it)
		if(it->Class && it->Class->Id==ClassID){
			it->setStartTime(stimes[k++]);
			it->StartNo=k;
			it->synchronize();
		}

	delete[] stimes;
}

void oEvent::automaticDrawAll(gdioutput &gdi, const string &firstStart, 
                               const string &minIntervall, const string &vacances, 
                               bool lateBefore, bool softMethod, bool pairwise) 
{
  gdi.refresh();
  const int leg = 0;
  const double extraFactor = 0.15;
  int drawn = 0;

  int baseInterval = convertAbsoluteTimeMS(minIntervall)/2;

  if (baseInterval == 0) {
    gdi.fillDown();
    int iFirstStart = getRelativeTime(firstStart);
   
    if (iFirstStart>0)
      gdi.addString("", 1, "Gemensam start");
    else {
      gdi.addString("", 1, "Nollställer starttider");
      iFirstStart = 0;
    }
    gdi.refreshFast();
    gdi.dropLine();
    for (oClassList::iterator it = Classes.begin(); it!=Classes.end(); ++it) {
      oe->drawList(it->getId(), 0, iFirstStart, 0, 0, false, false, drawAll);
    }
    return;
  }


  if (baseInterval<1 || baseInterval>60*60)
    throw std::exception("Felaktigt tidsformat för intervall");

  int iFirstStart = getRelativeTime(firstStart);

  if (iFirstStart<=0)
    throw std::exception("Felaktigt tidsformat för första start");

  double vacancy = atof(vacances.c_str())/100;

  gdi.fillDown();
  gdi.addString("", 1, "Automatisk lottning").setColor(colorGreen);
  gdi.addString("", 0, "Inspekterar klasser...");
  gdi.refreshFast();

  set<int> notDrawn;
  getNotDrawnClasses(notDrawn, false);

  set<int> needsCompletion;
  getNotDrawnClasses(needsCompletion, true);

  for(set<int>::iterator it = notDrawn.begin(); it!=notDrawn.end(); ++it)
    needsCompletion.erase(*it);

  //Start with not drawn classes
  map<string, int> starts;
  map<pClass, int> runnersPerClass; 
    
  // Count number of runners per start
  for (oRunnerList::iterator it = Runners.begin(); it!=Runners.end(); ++it) {
    if (it->skip())
      continue;
    if (it->tLeg != leg)
      continue;
    if (it->isVacant() && notDrawn.count(it->getClassId())==1)
      continue;
    pClass pc = it->Class;
    if (pc)
      ++starts[pc->getStart()];

    ++runnersPerClass[pc];
  }

  while ( !starts.empty() ) {
    // Select smallest start
    int runnersStart = Runners.size()+1;
    string start;
    for ( map<string, int>::iterator it = starts.begin(); it != starts.end(); ++it) {
      if (runnersStart > it->second) {
        start = it->first;
        runnersStart = it->second;
      }
    }
    starts.erase(start);

    // Estimate parameters for start
    DrawInfo di;
    int maxRunners = 0;

    // Find largest class in start;
    for (oClassList::iterator it = Classes.begin(); it!=Classes.end(); ++it) {
      if (it->getStart() != start)
        continue;

      maxRunners = max(maxRunners, runnersPerClass[&*it]);
    }

    if (maxRunners==0)
      continue;

    int maxParallell = 15;

    if (runnersStart < 100)
      maxParallell = 4;
    else if (runnersStart < 300)
      maxParallell = 6;
    else if (runnersStart < 700)
      maxParallell = 10;
    else if (runnersStart < 1000)
      maxParallell = 12;
    else
      maxParallell = 15;

    int optimalParallel = runnersStart / (maxRunners*2); // Min is every second interval

    di.nFields = max(3, min (optimalParallel + 2, 15));

    di.baseInterval = baseInterval;
    di.extraFactor = extraFactor;
    di.firstStart = iFirstStart;
    di.minClassInterval = baseInterval * 2;
    di.maxClassInterval = di.minClassInterval * 4;

    di.minVacancy = 1;
    di.maxVacancy = 100;
    di.vacancyFactor = vacancy;

    di.startName = start;

    for (oClassList::iterator it = Classes.begin(); it!=Classes.end(); ++it) {
      if (it->getStart() != start)
        continue;
      if (notDrawn.count(it->getId())==0)
        continue; // Only not drawn classes

      di.classes[it->getId()] = ClassInfo(it->getId());
    }

    if (di.classes.size()==0)
      continue;

    gdi.dropLine();
    gdi.addStringUT(1, lang.tl("Optimerar startfördelning") + " " + start);
    gdi.refreshFast();
    gdi.dropLine();
    vector<ClassInfo> cInfo;
    optimizeStartOrder(gdi, di, cInfo);

	  int laststart=0;
    for (size_t k=0;k<cInfo.size();k++) {
      const ClassInfo &ci = cInfo[k];
		  laststart=max(laststart, ci.firstStart+ci.nRunners*ci.interval);
	  }

	  gdi.addStringUT(1, lang.tl("Sista start (nu tilldelad)") + ": " +
                    getAbsTime((laststart)*di.baseInterval+di.firstStart));
	  gdi.dropLine();
    gdi.refreshFast();

    for (size_t k=0;k<cInfo.size();k++) {
      const ClassInfo &ci = cInfo[k];
      gdi.addString("", 0, "Lottar: X#" + getClass(ci.classId)->getName());
      
      drawList(ci.classId, leg, di.firstStart + di.baseInterval * ci.firstStart,
               di.baseInterval * ci.interval, ci.nVacant, softMethod, pairwise, oEvent::drawAll);

      gdi.scrollToBottom();
      gdi.refreshFast();
      drawn++;
	  }
  }

  // Classes that need completion
  for (oClassList::iterator it = Classes.begin(); it!=Classes.end(); ++it) {
    if (needsCompletion.count(it->getId())==0)
      continue;

    gdi.addStringUT(0, lang.tl("Lottar efteranmälda") + ": " + it->getName());

    drawList(it->getId(), leg, 0, 0, softMethod, 0, false,
              lateBefore ? remainingBefore : remainingAfter);
    
    gdi.scrollToBottom();
    gdi.refreshFast();
    drawn++;
  }

  gdi.dropLine();

  if (drawn==0)
    gdi.addString("", 1, "Klart: inga klasser behövde lottas.").setColor(colorGreen);
  else
    gdi.addString("", 1, "Klart: alla klasser lottade.").setColor(colorGreen);
  // Relay classes?
  gdi.dropLine();
  gdi.refreshFast();
}
