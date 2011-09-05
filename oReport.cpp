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
    Stigbergsv�gen 7, SE-75242 UPPSALA, Sweden
    
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
#include "SportIdent.h"

#include "oFreeImport.h"

#include "meos.h"
#include "meos_util.h"
#include "RunnerDB.h"

#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "Localizer.h"


void oEvent::generateCompetitionReport(gdioutput &gdi)
{
  gdi.fillDown();
  gdi.addString("", boldLarge, "T�vlingsstatistik");

  
  int lh = gdi.getLineHeight();

  oClassList::iterator it;
  
  reinitializeClasses();
  Classes.sort();

  gdi.dropLine();
 //  int xp = gdi.getCX();
//  int yp = gdi.getCY();
  int entries=0;
  int started=0;
  int fee=0;
//  int dx[]={0, 150, 200, 250, 350, 450};

  int entries_sum=0;
  int started_sum=0;
  int fee_sum=0;

  int entries_sum_y=0;
  int started_sum_y=0;
  int fee_sum_y=0;

  int cfee;
  gdi.addString("", boldText, "Elitklasser");
  vector<ClassMetaType> types;
  types.push_back(ctElite);
  cfee = getDCI().getInt("EliteFee");
  generateStatisticsPart(gdi, types, 0, cfee, 90, entries, started, fee);
  entries_sum += entries;
  started_sum +=  started;
  fee_sum += fee;

  gdi.addString("", boldText, "Vuxenklasser");
  types.clear();
  types.push_back(ctNormal);
  types.push_back(ctExercise);
  cfee = getDCI().getInt("EntryFee");
  generateStatisticsPart(gdi, types, 0, cfee, 90, entries, started, fee);
  entries_sum += entries;
  started_sum +=  started;
  fee_sum += fee;

  gdi.addString("", boldText, "Ungdomsklasser");
  types.clear();
  types.push_back(ctYouth);
  cfee = getDCI().getInt("YouthFee");
  generateStatisticsPart(gdi, types, 0, cfee, 50, entries, started, fee);
  entries_sum_y += entries;
  started_sum_y +=  started;
  fee_sum_y += fee;

  types.clear();
  types.push_back(ctOpen);
  
  gdi.addString("", boldText, "�ppna klasser, vuxna");
  cfee = getDCI().getInt("EntryFee");
  generateStatisticsPart(gdi, types, cfee, cfee, 90, entries, started, fee);
  entries_sum += entries;
  started_sum +=  started;
  fee_sum += fee;

  gdi.addString("", boldText, "�ppna klasser, ungdom");
  cfee = getDCI().getInt("YouthFee");
  generateStatisticsPart(gdi, types, cfee, cfee, 50, entries, started, fee);
  entries_sum_y += entries;
  started_sum_y +=  started;
  fee_sum_y += fee;

  gdi.addString("", boldText, "Sammanst�llning");
  gdi.dropLine();
  int xp = gdi.getCX();
  int yp = gdi.getCY();

  gdi.addString("", yp, xp+200, textRight|fontMedium, "Vuxna");
  gdi.addString("", yp, xp+300, textRight|fontMedium, "Ungdom");
  gdi.addString("", yp, xp+400, textRight|fontMedium, "Totalt");

  yp+=lh;
  gdi.addString("", yp, xp+0, fontMedium, "Anm�lda");
  gdi.addStringUT(yp, xp+200, textRight|fontMedium, itos(entries_sum));
  gdi.addStringUT(yp, xp+300, textRight|fontMedium, itos(entries_sum_y));
  gdi.addStringUT(yp, xp+400, textRight|boldText, itos(entries_sum+entries_sum_y));

  yp+=lh;
  gdi.addString("", yp, xp+0, fontMedium, "Startande");
  gdi.addStringUT(yp, xp+200, textRight|fontMedium, itos(started_sum));
  gdi.addStringUT(yp, xp+300, textRight|fontMedium, itos(started_sum_y));
  gdi.addStringUT( yp, xp+400, textRight|boldText, itos(started_sum+started_sum_y));

  yp+=lh;
  gdi.addString("", yp, xp+0, fontMedium, "Grundavgift");
  gdi.addStringUT(yp, xp+200, textRight|fontMedium, itos(fee_sum));
  gdi.addStringUT(yp, xp+300, textRight|fontMedium, itos(fee_sum_y));
  gdi.addStringUT(yp, xp+400, textRight|boldText, itos(fee_sum+fee_sum_y));

  yp+=lh;
  gdi.addString("", yp, xp+0, fontMedium, "SOFT-avgift");
  gdi.addStringUT(yp, xp+200, textRight|fontMedium, itos(entries_sum*15));
  gdi.addStringUT(yp, xp+300, textRight|fontMedium, itos(entries_sum_y*5));
  gdi.addStringUT(yp, xp+400, textRight|boldText, itos(entries_sum*15+entries_sum_y*5));

  yp+=lh*2;
  gdi.addString("", yp, xp+0,fontMedium, "Underlag f�r t�vlingsavgift:");
  int baseFee =  (fee_sum+fee_sum_y) - (entries_sum*15+5*entries_sum_y);
  gdi.addStringUT(yp, xp+200,fontMedium, itos(baseFee));

  yp+=lh;
  gdi.addString("", yp, xp+0,fontMedium, "Total t�vlingsavgift:");
  int cmpFee =  int((baseFee * .34 * (baseFee - 5800)) / (200000));  
  gdi.addStringUT(yp, xp+200, fontMedium, itos(cmpFee));

  yp+=lh;
  gdi.addString("", yp, xp, fontMedium, "Avrundad t�vlingsavgift:");
  gdi.addStringUT(yp, xp+200, boldText, itos(((cmpFee+50))/100)+"00");

  gdi.dropLine();
  gdi.addString("", boldText, "Geografisk f�rdelning");
  gdi.addString("", fontSmall, "Anm�lda per distrikt");
  gdi.dropLine(0.2);
  yp = gdi.getCY();
  vector<int> runners;
  vector<string> districts;
  getRunnersPerDistrict(runners);
  getDistricts(districts);
  int nd = min(runners.size(), districts.size());
  
  int ybase = yp;
  for (int k=1;k<=nd;k++) {
    gdi.addStringUT(yp, xp, fontMedium, itos(k) + " " + districts[k%nd]);
    gdi.addStringUT(yp, xp+200, textRight|fontMedium, itos(runners[k%nd]));
    yp+=lh;
    if (k%8==0){
      yp = ybase;
      xp += 250;
    }
  } 
}

void oEvent::generateStatisticsPart(gdioutput &gdi, const vector<ClassMetaType> &type, int feeLock, 
                                    int actualFee, int baseFee, 
                                    int &entries_sum, int &started_sum, int &fee_sum) const
{
  entries_sum=0;
  started_sum=0;
  fee_sum=0;
  int xp = gdi.getCX();
  int yp = gdi.getCY();
  int entries;
  int started;
  int dx[]={0, 150, 210, 270, 350, 450};
  int lh = gdi.getLineHeight();
  oClassList::const_iterator it;

  gdi.addString("", yp, xp+dx[0], fontSmall, "Klass");
  gdi.addString("", yp, xp+dx[1], textRight|fontSmall, "Anm. avg.");
  gdi.addString("", yp, xp+dx[2], textRight|fontSmall, "Grund avg.");
  gdi.addString("", yp, xp+dx[3], textRight|fontSmall, "Anm�lda");
  gdi.addString("", yp, xp+dx[4], textRight|fontSmall, "Avgift");
  gdi.addString("", yp, xp+dx[5], textRight|fontSmall, "Startande");
  yp+=lh;
  for(it=Classes.begin(); it!=Classes.end(); ++it) {
    if (it->isRemoved())
      continue;
    if (count(type.begin(), type.end(), it->interpretClassType())==1) {
      it->getStatistics(feeLock, entries, started);
      gdi.addStringUT(yp, xp+dx[0], fontMedium, it->getName());
      gdi.addStringUT(yp, xp+dx[1], textRight|fontMedium, itos(actualFee));
      gdi.addStringUT(yp, xp+dx[2], textRight|fontMedium, itos(baseFee));
      gdi.addStringUT(yp, xp+dx[3], textRight|fontMedium, itos(entries));
      gdi.addStringUT(yp, xp+dx[4], textRight|fontMedium, itos(baseFee*entries));
      gdi.addStringUT(yp, xp+dx[5], textRight|fontMedium, itos(started));
      entries_sum += entries;
      started_sum += started;
      fee_sum += entries*baseFee;
      yp+=lh;
    }
  }
  yp+=lh/2;
  gdi.addStringUT(yp, xp+dx[3], textRight|boldText, itos(entries_sum));
  gdi.addStringUT(yp, xp+dx[4], textRight|boldText, itos(fee_sum));
  gdi.addStringUT(yp, xp+dx[5], textRight|boldText, itos(started_sum));
  gdi.dropLine();
}

void oEvent::getRunnersPerDistrict(vector<int> &runners) const
{
  runners.clear();
  runners.resize(24);

  oRunnerList::const_iterator it;

  for (it = Runners.begin(); it != Runners.end(); ++it) {
    if(!it->skip()) {
      int code = 0;
      if (it->Club) 
        code = it->Club->getDCI().getInt("District");

      if (code>0 && code<24)
        ++runners[code];
      else
        ++runners[0];
    }
  }
}

void oEvent::getDistricts(vector<string> &districts) 
{
  districts.resize(24);
  int i=0;
  districts[i++]="�vriga";
  districts[i++]="Blekinge";
  districts[i++]="Bohusl�n-Dal";
  districts[i++]="Dalarna";
  districts[i++]="Gotland";
  districts[i++]="G�strikland";
  districts[i++]="G�teborg";
  districts[i++]="Halland";
  districts[i++]="H�lsingland";
  districts[i++]="J�mtland-H�rjedalan";
  districts[i++]="Medelpad";
  districts[i++]="Norrbotten";
  districts[i++]="�rebro l�n";
  districts[i++]="Sk�ne";
  districts[i++]="Sm�land";
  districts[i++]="Stockholm";
  districts[i++]="S�dermanland";
  districts[i++]="Uppland";
  districts[i++]="V�rmland";
  districts[i++]="V�sterbotten";
  districts[i++]="V�sterg�tland";
  districts[i++]="V�stmanland";
  districts[i++]="�ngermanland";
  districts[i++]="�sterg�tland";
}


void oEvent::generatePreReport(gdioutput &gdi)
{
	CurrentSortOrder=SortByName;
	Runners.sort();
	
  int lVacId = getVacantClub();

	oRunnerList::iterator r_it;
	oTeamList::iterator t_it;

	//BIB, START, NAME, CLUB, RANK, SI
	int dx[6]={0, 0, 70, 300, 470, 470};

	bool withrank = hasRank();
	bool withbib = hasBib(true, true);
	int i;

	if(withrank) dx[5]+=50;
	if(withbib) for(i=1;i<6;i++) dx[i]+=40;
		
	int y=gdi.getCY(); 
	int x=gdi.getCX();
	int lh=gdi.getLineHeight();
 
	gdi.addStringUT(2, lang.tl("Rapport inf�r") +": " + getName());
	
	gdi.addStringUT(1, getDate());
	gdi.dropLine();
	char bf[256];

	list<pRunner> no_card;
	list<pRunner> no_start;
	list<pRunner> no_class;
	list<pRunner> no_course;
	list<pRunner> no_club;

	for (r_it=Runners.begin(); r_it != Runners.end(); ++r_it){	
    if (r_it->isRemoved())
      continue;

    bool needStartTime = true;
    bool needCourse = true;

    pClass pc = r_it->Class;
    if (pc) {
      LegTypes lt = pc->getLegType(r_it->tLeg);
      if (lt == LTIgnore) {
        needStartTime = false;
        needCourse = false;
      }

      StartTypes st = pc->getStartType(r_it->tLeg);
      
      if (st != STTime && st != STDrawn)
        needStartTime = false;
    }
		if( r_it->getClubId() != lVacId) {
			if(needCourse && r_it->getCardNo()==0)
				no_card.push_back(&*r_it);
			if(needStartTime && r_it->getStartTime()==0)
				no_start.push_back(&*r_it);
			if(r_it->getClubId()==0)
				no_club.push_back(&*r_it);
		}

		if(r_it->getClassId()==0)
			no_class.push_back(&*r_it);
		else if(needCourse && r_it->getCourse()==0)
			no_course.push_back(&*r_it);
	}
	
	list<pRunner> si_duplicate;

	if(Runners.size()>1){
		Runners.sort(oRunner::CompareSINumber);
		
		r_it=Runners.begin();		
		while (++r_it != Runners.end()){	
			oRunnerList::iterator r_it2=r_it;
			r_it2--;

			if(r_it2->getCardNo() && r_it2->getCardNo()==r_it->getCardNo()){
				
				if(si_duplicate.size()==0 || si_duplicate.back()->getId()!=r_it2->getId())
					si_duplicate.push_back(&*r_it2);

				si_duplicate.push_back(&*r_it);
			}
		}
	}

	const string Ellipsis="[ ... ]";

 	sprintf_s(bf, lang.tl("L�pare utan klass: %d.").c_str(), no_class.size());
	gdi.addStringUT(1, bf);
	i=0;

	while(!no_class.empty() && ++i<20){
		pRunner r=no_class.front();
		no_class.pop_front();
		sprintf_s(bf, "%s (%s)", r->getName().c_str(), r->getClub().c_str());
		gdi.addStringUT(0, bf);
	}
	if(!no_class.empty()) gdi.addStringUT(1, Ellipsis);

	gdi.dropLine();
  sprintf_s(bf, lang.tl("L�pare utan bana: %d.").c_str(), no_course.size());
	gdi.addStringUT(1, bf);
	i=0;

	while(!no_course.empty() && ++i<20){
		pRunner r=no_course.front();
		no_course.pop_front();
		sprintf_s(bf, "%s: %s (%s)", r->getClass().c_str(), r->getName().c_str(), r->getClub().c_str());
		gdi.addStringUT(0, bf);
	}
	if(!no_course.empty()) gdi.addStringUT(1, Ellipsis);


	gdi.dropLine();
	sprintf_s(bf, lang.tl("L�pare utan klubb: %d.").c_str(), no_club.size());
	gdi.addStringUT(1, bf);
	i=0;

	while(!no_club.empty() && ++i<20){
		pRunner r=no_club.front();
		no_club.pop_front();
		sprintf_s(bf, "%s: %s", r->getClass().c_str(), r->getName().c_str());
		gdi.addStringUT(0, bf);
	}
	if(!no_club.empty()) gdi.addStringUT(1, Ellipsis);

	gdi.dropLine();
	sprintf_s(bf, lang.tl("L�pare utan starttid: %d.").c_str(), no_start.size());
	gdi.addStringUT(1, bf);
	i=0;

	while(!no_start.empty() && ++i<20){
		pRunner r=no_start.front();
		no_start.pop_front();
		sprintf_s(bf, "%s: %s (%s)", r->getClass().c_str(), r->getName().c_str(), r->getClub().c_str());
		gdi.addStringUT(0, bf);
	}
	if(!no_start.empty()) gdi.addStringUT(1, Ellipsis);

	gdi.dropLine();
	sprintf_s(bf, lang.tl("L�pare utan SI-bricka: %d.").c_str(), no_card.size());
	gdi.addStringUT(1, bf);
	i=0;

	while(!no_card.empty() && ++i<20){
		pRunner r=no_card.front();
		no_card.pop_front();
		sprintf_s(bf, "%s: %s (%s)", r->getClass().c_str(), r->getName().c_str(), r->getClub().c_str());
		gdi.addStringUT(0, bf);
	}
	if(!no_card.empty()) gdi.addStringUT(1, Ellipsis);


	gdi.dropLine();
	sprintf_s(bf, lang.tl("SI-dubbletter: %d.").c_str(), si_duplicate.size());
	gdi.addStringUT(1, bf);
	i=0;

	while(!si_duplicate.empty() && ++i<20){
		pRunner r=si_duplicate.front();
		si_duplicate.pop_front();
		sprintf_s(bf, "%s: %s (%s) SI=%d", r->getClass().c_str(), 
              r->getName().c_str(), r->getClub().c_str(), r->getCardNo());
		gdi.addStringUT(0, bf);
	}
	if(!si_duplicate.empty()) gdi.addStringUT(1, Ellipsis);

	
	//List all competitors not in a team.
  if (oe->hasTeam()) {
	  for (r_it=Runners.begin(); r_it != Runners.end(); ++r_it)
		  r_it->_objectmarker=0; 
  	
	  for (t_it=Teams.begin(); t_it != Teams.end(); ++t_it){		
		  pClass pc=getClass(t_it->getClassId());
  		 
		  if(pc){
			  for(unsigned i=0;i<pc->getNumStages();i++){
				  pRunner r=t_it->getRunner(i);
				  if(r) r->_objectmarker++;
			  }
		  }		
	  }

	  gdi.dropLine();
    gdi.addString("", 1, "L�pare som f�rekommer i mer �n ett lag:");
    bool any = false;
	  for (r_it=Runners.begin(); r_it != Runners.end(); ++r_it){
		  if(r_it->_objectmarker>1){
			  sprintf_s(bf, "%s: %s (%s)", r_it->getClass().c_str(), r_it->getName().c_str(), r_it->getClub().c_str());
			  gdi.addStringUT(0, bf);
        any = true;
		  }
	  }
    if (!any)
      gdi.addStringUT(1, "0");
  }
	sortRunners(ClassStartTime);	
	
	gdi.dropLine();
	gdi.addString("", 1, "Individuella deltagare");
	
	y=gdi.getCY(); 
	int tab[5]={0, 100, 350, 420, 550};
	for (r_it=Runners.begin(); r_it != Runners.end(); ++r_it){
		if(r_it->_objectmarker==0){ //Only consider runners not in a team.
			gdi.addStringUT(y, x+tab[0], 0, r_it->getClass(), tab[1]-tab[0]);
			gdi.addStringUT(y, x+tab[1], 0, r_it->getName()+" ("+r_it->getClub()+")", tab[2]-tab[1]);
			gdi.addStringUT(y, x+tab[2], 0, itos(r_it->getCardNo()), tab[3]-tab[2]);
			gdi.addStringUT(y, x+tab[3], 0, r_it->getCourseName(), tab[4]-tab[3]);
			y+=lh;	
			pCourse pc=r_it->getCourse();
			
			if(pc){
        vector<string> res = pc->getCourseReadable(101);
        for (size_t k = 0; k<res.size(); k++) {
				  gdi.addStringUT(y, x+tab[1], 0, res[k]);
				  y+=lh;
        }
			}
		}
	}

	gdi.dropLine();
	gdi.addStringUT(1, "Lag");
	
	for (t_it=Teams.begin(); t_it != Teams.end(); ++t_it){		
		pClass pc=getClass(t_it->getClassId());
	
		gdi.addStringUT(0, t_it->getClass() + ": " + t_it->getName() + "  " +t_it->getStartTimeS());

		if(pc){
			for(unsigned i=0;i<pc->getNumStages();i++){
				pRunner r=t_it->getRunner(i);
				if(r){
					gdi.addStringUT(0, r->getName()+ " SI: " +itos(r->getCardNo()));

					pCourse pcourse=r->getCourse();
			
					if(pcourse){
            y = gdi.getCY();
            vector<string> res = pcourse->getCourseReadable(101);
            for (size_t k = 0; k<res.size(); k++) {
              gdi.addStringUT(y, x+tab[1], 0, res[k]);
  						y+=lh;
            }
					}
				}
				else
					gdi.addString("", 0, "L�pare saknas");
			}
		}	
		gdi.dropLine();
	}

	gdi.updateScrollbars();
}

