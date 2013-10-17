/********************i****************************************************
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
    Stigbergsv�gen 7, SE-75242 UPPSALA, Sweden
    
************************************************************************/

#include "stdafx.h"
#include "oListInfo.h"
#include "oEvent.h"
#include "gdioutput.h"
#include "meos_util.h"
#include <cassert>
#include <cmath>
#include "Localizer.h"
#include "metalist.h"
#include <algorithm>
#include "gdifonts.h"


oListInfo::oListInfo() {
  listType=EBaseTypeRunner;
  listSubType=EBaseTypeRunner;
  calcResults=false;
  calcTotalResults = false;
  rogainingResults = false;
  listPostFilter.resize(_EFilterMax+1, 0);
  fixedType = false;
  largeSize = false;
  supportClasses = true;
  supportLegs = false;
  supportExtra = false;
  supportLarge = false;
  needPunches = false;
  next = 0;
}

oListInfo::~oListInfo(void)
{
  if (next) 
    delete next;
  next = 0;
}

oPrintPost::oPrintPost()
{
  dx=0;
  dy=0;
  format=0;
  type=lString;
  index=-1;
  fixedWidth=0;
}

oPrintPost::oPrintPost(EPostType type_, const string &text_, 
                       int format_, int dx_, int dy_, int index_)
{
  dx=dx_;
  dy=dy_;
  text=text_;
  format=format_;
  type=type_;
  index=index_;
  fixedWidth=0;
}


void oListInfo::addList(const oListInfo &lst) {
  if (next)
    next->addList(lst);
  else
    next = new oListInfo(lst);  
}

int oListInfo::getMaxCharWidth(const oEvent *oe, EPostType type,
                               const string &format, gdiFonts font,
                               const char *fontFace,
                               bool large, int minSize) 
{
  string out;
  oPrintPost pp;
  pp.text = format;
  pp.type = type;
  int width = minSize;
  oListParam par;
  par.legNumber = 0;
  oCounter c;
 
  string extra;
  switch (type) {
    case lRunnerTotalTimeAfter:
    case lRunnerClassCourseTimeAfter:
    case lRunnerTimeAfterDiff:
    case lRunnerTempTimeAfter:
    case lRunnerTimeAfter:
    case lRunnerMissedTime:
    case lTeamTimeAfter:
    case lTeamLegTimeAfter:
      extra = "+10:00";
      break;
    case lTeamLegTimeStatus:
    case lTeamTimeStatus:
    case lRunnerTempTimeStatus:
    case lRunnerTotalTimeStatus:
    case lRunnerTotalTime:
    case lClassStartTime:
    case lRunnerFinish:
    case lRunnerTime:
    case lRunnerTimeStatus:
      extra = "50:50";
  }
 
  if (type == lClubName || type == lRunnerName || type == lTeamName)
    width = max(10, minSize);

  width = max<int>(extra.size(), minSize);

  for (oClassList::const_iterator it = oe->Classes.begin(); it != oe->Classes.end(); ++it) {
    if (it->isRemoved())
      continue;

    oe->formatListString(pp, out, par, 0, 0, 0, pClass(&*it), c);
    width = max(width, int(out.length()));
  }

  for (oRunnerList::const_iterator it = oe->Runners.begin(); it != oe->Runners.end(); ++it) {
    if (it->isRemoved())
      continue;
    pp.index = it->tLeg;
    int numIter = 1;

    if (type == lPunchNamedTime || type == lPunchTime) {
      width = max(width, 10);

      pRunner r = pRunner(&*it);
      numIter = (r && r->getCard()) ? r->getCard()->getNumPunches() + 1 : 1;
    }

    while (numIter-- > 0) {
      oe->formatListString(pp, out, par, it->tInTeam, pRunner(&*it), it->Club, it->Class, c);
      width = max(width, int(out.length()));
      if (numIter>0)
        c.level3++;
    }
  }
  double w = width;
  double s = 1.0;

  s = oe->gdibase.getRelativeFontScale(font, fontFace);
  /*oe->gdibase.scrollTo(0,0);

  if (font == fontSmall)
    s = 0.7;
  else if (font == italicSmall)
    s = 0.7;
  else if (font == boldSmall)
    s = 0.8;
  else if (font == boldText)
    s = 1.1;
  else if (font == boldLarge)
    s = 1.7;
  else if (font == boldHuge)
    s = 2.5;
  else if (font == fontMedium)
    s = 0.8;
  else if (font == fontMediumPlus)
    s = 1.2;
*/
  if (width>0 && !large)
    return int(s*(w*6.0+20.0));
  else if (width>0 && large)
    return int(s*(w*7.0+10.0));
  else
    return 0;
}

bool oEvent::formatListString(EPostType type, string &out, const pRunner r) const
{
  oPrintPost pp;
  oCounter ctr;
  oListParam par;
  par.legNumber = r->tLeg;
  pp.type = type;
  return formatListString(pp, out, par, r->tInTeam, r, r->Club, r->Class, ctr);
}

bool oEvent::formatListString(const oPrintPost &pp, string &out, const oListParam &par,
                              const pTeam t, const pRunner r, const pClub c,
                              const pClass pc, oCounter &counter) const
{
  
  char bf[512];
  const string *sptr=0;
  bf[0]=0;
  bool invalidClass = pc && pc->getClassStatus() != oClass::Normal;

  switch ( pp.type ) {
    case lClassName:
      if (invalidClass)
        sprintf_s(bf, "%s (%s)", pc->Name.c_str(), lang.tl("Struken").c_str());
      else
        sptr=pc ? &pc->Name : 0;
      break;
    case lClassLength:
      if(pc) strcpy_s(bf, pc->getLength(pp.index).c_str());
      break;
    case lClassStartName:
      if(pc) strcpy_s(bf, pc->getDI().getString("StartName").c_str());
      break;
    case lClassStartTime:
      if (pc) {
        int first, last;
        pc->getStartRange(pp.index, first, last);
        if (first == 0)
          strcpy_s(bf, lang.tl("Fri starttid").c_str());
        else if (first == last) {
          strcpy_s(bf, oe->getAbsTimeHM(first).c_str());
        }
      }
      break;
    case lClassResultFraction:
      if(pc && !invalidClass) {
        int total, finished,  dns;
        oe->getNumClassRunners(pc->getId(), par.legNumber, total, finished, dns);
        sprintf_s(bf, "(%d/%d)", finished, total);
      }
      break;
    case lCourseLength:
      if (r) {
        pCourse crs = r->getCourse();
        int len = crs ? crs->getLength() : 0;
        if (len > 0)
          sprintf_s(bf, "%d", len); 
      }
    break;
    case lCourseName:
      if (r) {
        pCourse crs = r->getCourse();
        if (crs)
          sptr = &crs->getName();
      }
    break;
    case lCourseClimb:
      if (r) {
        pCourse crs = r->getCourse();
        int len = crs ? crs->getDCI().getInt("Climb") : 0;
        if (len > 0)
          sprintf_s(bf, "%d", len); 
      }
    break;
    case lCmpName:
      sptr=&this->Name;
      break;
    case lCmpDate:
      strcpy_s(bf, this->getDate().c_str());      
      break;
    case lCurrentTime:
      strcpy_s(bf, getCurrentTimeS().c_str());
      break;
    case lRunnerClub:
      sptr=(r && r->Club) ? &r->Club->getDisplayName() : 0;
      break;
    case lRunnerFinish:
      if(r && !invalidClass) strcpy_s(bf, r->getFinishTimeS().c_str());
      break;
    case lRunnerStart:
      if(r) strcpy_s(bf, r->getStartTimeCompact().c_str());
      break;
    case lRunnerName:
      sptr = r ? &r->Name : 0;
      break;
    case lRunnerGivenName:
      if (r) strcpy_s(bf, r->getGivenName().c_str());
      break;
    case lRunnerFamilyName:
      if (r) strcpy_s(bf, r->getFamilyName().c_str());
      break;
    case lRunnerCompleteName:
      if(r) strcpy_s(bf, r->getCompleteIdentification().c_str());
      break;
    case lPatrolNameNames:
      if(t) {
        pRunner r1 = t->getRunner(0);
        pRunner r2 = t->getRunner(1);
        if (r1 && r2) {
          sprintf_s(bf, "%s / %s", r1->Name.c_str(),r2->Name.c_str());
        }
        else if (r1) {
          sptr = &r1->getName();
        }
        else if (r2) {
          sptr = &r2->getName();
        }
      }
      else {
        sptr = r ? &r->Name : 0;
      }
      break;
    case lPatrolClubNameNames:
      if(t) {
        pRunner r1 = t->getRunner(0);
        pRunner r2 = t->getRunner(1);
        pClub c1 = r1 ? r1->Club : 0;
        pClub c2 = r2 ? r2->Club : 0;
        if (c1 == c2)
          c2 = 0;
        if (c1 && c2) {
          sprintf_s(bf, "%s / %s", c1->getDisplayName().c_str(),c2->getDisplayName().c_str());
        }
        else if (c1) {
          sptr = &c1->getDisplayName();
        }
        else if (c2) {
          sptr = &c2->getDisplayName();
        }
      }
      else {
        sptr = r && r->Club ? &r->Club->getDisplayName() : 0;
      }
      break;
    case lRunnerTime:
      if(r && !invalidClass) strcpy_s(bf, r->getRunningTimeS().c_str());
      break;
    case lRunnerTimeStatus:
      if(r) {
        if (invalidClass)
          sptr = &lang.tl("Struken");
        else if(r->getStatus()==StatusOK && pc && !pc->getNoTiming())
          strcpy_s(bf, r->getRunningTimeS().c_str() );
        else
          strcpy_s(bf, r->getStatusS().c_str() );
      }
      break;
    case lRunnerTimePerKM:
      if(r && !invalidClass && r->statusOK()) {
        const pCourse pc = r->getCourse();
        if (pc) {
          int t = r->getRunningTime();
          int len = pc->getLength();
          if (len > 0 && t > 0) {
            int sperkm = (1000 * t) / len;
            strcpy_s(bf, formatTime(sperkm).c_str());
          }
        }
      }
      break;
    case lRunnerTotalTime:
      if(r && !invalidClass) strcpy_s(bf, r->getTotalRunningTimeS().c_str());
      break;
    case lRunnerTotalTimeStatus:
      if (invalidClass)
        sptr = &lang.tl("Struken");
      else if(r) {
        if(r->getTotalStatus()==StatusOK && pc && !pc->getNoTiming())
          strcpy_s(bf, r->getTotalRunningTimeS().c_str() );
        else
          strcpy_s(bf, r->getTotalStatusS().c_str() );
      }
      break;
    case lRunnerTempTimeStatus:
      if (invalidClass)
          sptr = &lang.tl("Struken");
      else if(r) {
        if(r->tStatus==StatusOK && pc && !pc->getNoTiming())
          strcpy_s(bf, formatTime(r->tRT).c_str() );
        else
          strcpy_s(bf, formatStatus(r->tStatus).c_str() );
      }
      break;
    case lRunnerPlace:
      if (r && !invalidClass) strcpy_s(bf, r->getPrintPlaceS().c_str() );
      break;
    case lRunnerTotalPlace:
      if (r && !invalidClass) strcpy_s(bf, r->getPrintTotalPlaceS().c_str() );
      break;

    case lRunnerClassCoursePlace:
      if (r && !invalidClass) {
        int p = r->getCoursePlace();
      	if (p>0 && p<10000)
		      sprintf_s(bf, "%d.", p);
      }
      break;
    case lRunnerPlaceDiff:
      if (r && !invalidClass) {
        int p = r->getTotalPlace();
        if (r->getTotalStatus() == StatusOK && p > 0 && r->inputPlace>0) {
          int pd = p - r->inputPlace;
          if (pd > 0)
            sprintf_s(bf, "+%d", pd);
          else if (pd < 0)
            sprintf_s(bf, "-%d", -pd);
        }
      }
      break;
    case lRunnerTimeAfterDiff:
      if (r && pc && !invalidClass) {
        int tleg = r->tLeg >= 0 ? r->tLeg:0;
        if (r->getTotalStatus()==StatusOK &&  pc && !pc->getNoTiming()) {
          int after = r->getTotalRunningTime() - pc->getTotalLegLeaderTime(tleg);
          int afterOld = r->inputTime - pc->getBestInputTime(tleg);
          int ad = after - afterOld;
          if (ad > 0)
            sprintf_s(bf, "+%d:%02d", ad/60, ad%60);
          if (ad < 0)
            sprintf_s(bf, "-%d:%02d", (-ad)/60, (-ad)%60);

        }
      }
      break;
    case lRunnerRogainingPoint:
      if (r && !invalidClass) {
        sprintf_s(bf, "%d", r->getRogainingPoints());
      }
      break;
    case lRunnerPenaltyPoint:
      if (r && !invalidClass) {
				sprintf_s(bf, "%d", r->getPenaltyPoints());
      }
      break;
    case lRunnerTimeAfter:
      if (r && pc && !invalidClass) {
        int tleg=r->tLeg>=0 ? r->tLeg:0;
        if (r->Status==StatusOK &&  pc && !pc->getNoTiming() 
              && r->getRunningTime() > pc->getBestLegTime(tleg)) {
          int after=r->getRunningTime()-pc->getBestLegTime(tleg);
          sprintf_s(bf, "+%d:%02d", after/60, after%60);
        }
      }
      break;
    case lRunnerTotalTimeAfter:
      if (r && pc && !invalidClass) {
        int tleg = r->tLeg >= 0 ? r->tLeg:0;
        if (r->getTotalStatus()==StatusOK &&  pc && !pc->getNoTiming()) {
          int after = r->getTotalRunningTime() - pc->getTotalLegLeaderTime(tleg);
          if (after > 0)
            sprintf_s(bf, "+%d:%02d", after/60, after%60);
        }
      }
      break;
    case lRunnerClassCourseTimeAfter:
      if (r && pc && !invalidClass) {
        pCourse crs = r->getCourse();
        if (crs && r->Status==StatusOK && !pc->getNoTiming()) {
          int after = r->getRunningTime() - pc->getBestTimeCourse(crs->getId());
          if (after > 0)
            sprintf_s(bf, "+%d:%02d", after/60, after%60);
        }
      }
      break;
    case lRunnerTimePlaceFixed:
      if (r && !invalidClass) {
        int t = r->getTimeWhenPlaceFixed();
        if (t == 0 || (t>0 && t < getComputerTime())) {
          strcpy_s(bf, lang.tl("klar").c_str());
        }
        else if (t == -1)
          strcpy_s(bf, "-");
        else
          strcpy_s(bf, oe->getAbsTime(t).c_str());
      }
      break;
    case lRunnerLegNumberAlpha:
      if (r && pc) {
        int leg = r->getLegNumber();
        string legStr = pc->getLegNumber(leg);
        strcpy_s(bf, legStr.c_str());
      }
      break;
    case lRunnerMissedTime:
      if (r && r->Status == StatusOK && pc && !pc->getNoTiming() && !invalidClass) {
        strcpy_s(bf, r->getMissedTimeS().c_str());
      }
      break;
    case lRunnerTempTimeAfter:
      if (r && pc) {
        if (r->tStatus==StatusOK &&  pc && !pc->getNoTiming() 
              && r->tRT>pc->tLegLeaderTime) {
          int after=r->tRT-pc->tLegLeaderTime;
          sprintf_s(bf, "+%d:%02d", after/60, after%60);
        }
      }
      break;

    case lRunnerCard:
      if(r && r->CardNo > 0)
        sprintf_s(bf, "%d", r->getCardNo());
      break;
    case lRunnerRank:
      if(r) {
        int rank=r->getDI().getInt("Rank");
        if(rank>0)
          strcpy_s(bf, FormatRank(rank).c_str());          
      }
      break;
    case lRunnerCourse:
      if(r && r->getCourse()) 
        sptr = &r->getCourse()->getName();
      break;
    case lRunnerBib:
      if(r) {
        string bib = r->getBib();
        if (!bib.empty())
          sprintf_s(bf, "%s", bib.c_str());
      }
      break;
    case lRunnerUMMasterPoint:
      if(r) {
        int total, finished, dns;
        oe->getNumClassRunners(pc->getId(), par.legNumber, total, finished, dns);
        int percent = int(floor(0.5+double((100*(total-dns-r->getPlace()))/double(total-dns))));
        if (r->getStatus()==StatusOK)
          sprintf_s(bf, "%d",  percent);
        else
          sprintf_s(bf, "0");
      }
    case lTeamName:
      //sptr = t ? &t->Name : 0;
      if (t) {
        sprintf_s(bf, t->getDisplayName().c_str());
      }
      break;
    case lTeamStart:
      if(t && unsigned(pp.index)<t->Runners.size() && t->Runners[pp.index])
        strcpy_s(bf, t->Runners[pp.index]->getStartTimeCompact().c_str());
      break;
    case lTeamStatus:
      if(t && !invalidClass) strcpy_s(bf, t->getLegStatusS(pp.index).c_str() );
      break;
    case lTeamTime:
      if(t && !invalidClass) strcpy_s(bf, t->getLegRunningTimeS(pp.index).c_str() );
      break;    
    case lTeamTimeStatus:
      if (invalidClass)
          sptr = &lang.tl("Struken");
      else if(t) {
        if(t->getLegStatus(pp.index)==StatusOK)
          strcpy_s(bf, t->getLegRunningTimeS(pp.index).c_str() );
        else
          strcpy_s(bf, t->getLegStatusS(pp.index).c_str() );
      }
      break; 
    case lTeamRogainingPoint:
      if (t && !invalidClass) {
        sprintf_s(bf, "%dp", t->getRogainingPoints());
      }
      break;
    case lTeamTimeAfter:
      //sprintf_s(bf, "+1:01");
      if (t && t->getLegStatus(pp.index)==StatusOK && !invalidClass) {
        int ta=t->getTimeAfter(pp.index);
        if (ta>0)
          sprintf_s(bf, "+%d:%02d", ta/60, ta%60);
      }
      break;
    case lTeamPlace:
      if(t && !invalidClass) strcpy_s(bf, t->getLegPrintPlaceS(pp.index).c_str());
      break;
    case lTeamLegTimeStatus:
      if (invalidClass)
        sptr = &lang.tl("Struken");
      else if(t) {
        int ix = r ? r->getLegNumber() : counter.level3;
        if(t->getLegStatus(ix)==StatusOK)
          strcpy_s(bf, t->getLegRunningTimeS(ix).c_str() );
        else
          strcpy_s(bf, t->getLegStatusS(ix).c_str() );
      }
      break; 
    case lTeamLegTimeAfter:
      if(t) {
        int ix = r ? r->getLegNumber() : counter.level3;
        if (t->getLegStatus(ix)==StatusOK && !invalidClass) {
          int ta=t->getTimeAfter(ix);
        if (ta>0)
          sprintf_s(bf, "+%d:%02d", ta/60, ta%60);
      }
      }
      break;
    case lTeamClub:
      if (t) {
        sprintf_s(bf, t->getDisplayClub().c_str());
      }
      break;
    case lTeamRunner:
      if(t && unsigned(pp.index)<t->Runners.size() && t->Runners[pp.index])
        sptr=&t->Runners[pp.index]->Name;
      break;
    case lTeamRunnerCard:
      if(t && unsigned(pp.index)<t->Runners.size() && t->Runners[pp.index]
      && t->Runners[pp.index]->getCardNo()>0)
        sprintf_s(bf, "%d", t->Runners[pp.index]->getCardNo());
      break;
    case lTeamBib:
      if(t) {
        string bib = t->getBib();
        if(!bib.empty())
          sprintf_s(bf, "%s", bib.c_str());
      }
      break;
    case lTeamStartNo:
      if (t)
        sprintf_s(bf, "%d", t->getStartNo());
      break;
    case lRunnerStartNo:
      if (r)
        sprintf_s(bf, "%d", r->getStartNo());
      break;
    case lPunchNamedTime:
      if(r && r->Card && r->getCourse() && !invalidClass) {
        const pCourse crs = r->getCourse();
        const oControl *ctrl=crs->getControl(counter.level3);
        if (!ctrl || ctrl->isRogaining(crs->hasRogaining()))
          break;
        if (r->getPunchTime(counter.level3)>0 && ctrl->hasName()) {
          sprintf_s(bf, "%s: %s (%s)", ctrl->getName().c_str(),
            r->getNamedSplitS(counter.level3).c_str(),
            r->getPunchTimeS(counter.level3).c_str());
        }
      }
      break;
    case lPunchTime:
      if(r && r->Card && r->getCourse() && !invalidClass) {
        const pCourse crs=r->getCourse();
        int nCtrl = crs->getNumControls();
        if (counter.level3 != nCtrl) { // Always allow finish
        const oControl *ctrl=crs->getControl(counter.level3);
        if (!ctrl || ctrl->isRogaining(crs->hasRogaining()))
          break;
        }

        if (r->getPunchTime(counter.level3)>0) {
          sprintf_s(bf, "%s (%s)", 
            r->getSplitTimeS(counter.level3).c_str(),
            r->getPunchTimeS(counter.level3).c_str());
        }
        else 
          sprintf_s(bf, "- (-)");
      }
      break;
    case lSubSubCounter:
      if (pp.text.empty())
        sprintf_s(bf, "%d.", counter.level3+1);
      else
        sprintf_s(bf, "%d", counter.level3+1);
      break;
    case lSubCounter:
      if (pp.text.empty())
        sprintf_s(bf, "%d.", counter.level2+1);
      else
        sprintf_s(bf, "%d", counter.level2+1);
      break;
    case lTotalCounter:
      if (pp.text.empty())
        sprintf_s(bf, "%d.", counter.level1+1);
      else
        sprintf_s(bf, "%d", counter.level1+1);
      break;
    case lClubName:
      sptr = c != 0 ? &c->getDisplayName() : 0;
      break;
    case lRogainingPunch:
      if(r && r->Card && r->getCourse()) {
        const pCourse crs = r->getCourse();
        const pPunch punch = r->Card->getPunchByIndex(counter.level3);
        if (punch && punch->tRogainingIndex>=0) {
          const pControl ctrl = crs->getControl(punch->tRogainingIndex);
          if (ctrl) {
            sprintf_s(bf, "%d, %dp, %s (%s)", 
                      punch->Type, ctrl->getRogainingPoints(),
                      r->Card->getRogainingSplit(counter.level3, r->StartTime).c_str(),
                      punch->getRunningTime(r->StartTime).c_str());
          }
        }
      }
      break;
  }

  if(pp.type!=lString && (sptr==0 || sptr->empty()) && bf[0]==0) 
    out.clear();
  else if(sptr) {
    if(pp.text.empty())
      out=*sptr;
    else {
      sprintf_s(bf, pp.text.c_str(), sptr->c_str());
      out=bf;
    }
  }
  else {
    if(pp.text.empty())
      out=bf;
    else {
      char bf2[512];
      sprintf_s(bf2, pp.text.c_str(), bf);
      out=bf2;
    }
  }
  return !out.empty();
}

bool oEvent::formatPrintPost(const list<oPrintPost> &ppli, gdioutput &gdi, const oListParam &par,
                              const pTeam t, const pRunner r, const pClub c,
                              const pClass pc, oCounter &counter)
{
  list<oPrintPost>::const_iterator ppit;
  string text;
	int y=gdi.getCY(); 
	int x=gdi.getCX();
  bool updated=false;
  for (ppit=ppli.begin();ppit!=ppli.end();) {
    const oPrintPost &pp=*ppit;
    int limit = 0;
    
    if( ++ppit != ppli.end() && ppit->dy==pp.dy)
      limit = ppit->dx - pp.dx;

    //if(pp.fixedWidth>0)
    limit=max(pp.fixedWidth, limit);

    assert(limit>=0);
    pRunner rr = r;    
    if(!rr && t)
      rr=t->getRunner(pp.index);

    updated |= formatListString(pp, text, par, t, rr, c, pc, counter);

    if(!text.empty()) {
      if (pp.type == lRunnerName && rr)
        gdi.addStringUT(y+gdi.scaleLength(pp.dy), x+gdi.scaleLength(pp.dx), pp.format, text, 
        gdi.scaleLength(limit), par.cb, pp.fontFace.c_str()).setExtra(rr);
      else if (pp.type == lTeamName && t)
        gdi.addStringUT(y+gdi.scaleLength(pp.dy), x+gdi.scaleLength(pp.dx), pp.format, text, 
                        gdi.scaleLength(limit), par.cb, pp.fontFace.c_str()).setExtra(t);
      else
        gdi.addStringUT(y + gdi.scaleLength(pp.dy), x + gdi.scaleLength(pp.dx), 
                        pp.format, text, gdi.scaleLength(limit), 0, pp.fontFace.c_str());

    }
  }  

  return updated; 
}

void oEvent::calculatePrintPostKey(const list<oPrintPost> &ppli, gdioutput &gdi, const oListParam &par,
                                   const pTeam t, const pRunner r, const pClub c,
                                   const pClass pc, oCounter &counter, string &key)
{
  key.clear();
  list<oPrintPost>::const_iterator ppit;
  string text;
  for (ppit=ppli.begin();ppit!=ppli.end(); ++ppit) {
    const oPrintPost &pp=*ppit;
    pRunner rr = r;    
    if(!rr && t)
      rr=t->getRunner(pp.index);

    formatListString(pp, text, par, t, rr, c, pc, counter);
    key += text;
  }   
}

void oEvent::listGeneratePunches(const list<oPrintPost> &ppli, gdioutput &gdi, const oListParam &par,
                                 pTeam t, pRunner r, pClub club, pClass cls) 
{
  if(!r || ppli.empty()) 
    return;

  pCourse crs=r->getCourse();

  bool filterNamed = false;
  int h = gdi.getLineHeight();

  for (list<oPrintPost>::const_iterator it = ppli.begin(); it != ppli.end(); ++it) {
    if (it->type == lPunchNamedTime)
      filterNamed = true;
    else if (it->type == lPunchTime) {
      filterNamed = false;
      break;
    }
    h = max(h, gdi.getLineHeight(it->getFont(), it->fontFace.c_str()));
  }

  if(!crs)
    return;

  int w=gdi.scaleLength(ppli.front().fixedWidth);
  int xlimit=gdi.getCX()+ gdi.scaleLength(600);

  if (w>0) {
    gdi.pushX();
    gdi.fillNone();
  }

  bool neednewline = false;
  bool updated=false;
  oCounter counter;

  int limit = crs->nControls + 1;

  if (r->Card && r->Card->getNumPunches()>limit)
    limit = r->Card->getNumPunches();

  vector<char> skip(limit, false);
  if (filterNamed) {
    for (int k = 0; k < crs->nControls; k++) {
      if (crs->getControl(k) && !crs->getControl(k)->hasName())
        skip[k] = true;
    }
    for (int k = crs->nControls; k < limit; k++) {
      skip[k] = true;
    }
  }

  for (int k=0; k<limit; k++) {
    if(w>0 && updated) {
      updated=false;
      if( gdi.getCX() + w > xlimit) {
        neednewline = false;
        gdi.popX();
        gdi.setCY(gdi.getCY() + h);
      }
      else 
       gdi.setCX(gdi.getCX()+w);
    }
    
    if (!skip[k]) { 
      updated |= formatPrintPost(ppli, gdi, par, t, r, club, cls, counter);
      neednewline |= updated;
    }

    counter.level3++;
  }

  if(w>0) {
    gdi.popX();
    gdi.fillDown();
    if (neednewline)
      gdi.setCY(gdi.getCY() + h);
  }
}

void oEvent::generateList(gdioutput &gdi, bool reEvaluate, const oListInfo &li)
{ 
  if (reEvaluate)
    reEvaluateAll(false);

  oe->calcUseStartSeconds();
  oe->updateComputerTime();

  if (!li.Head.empty()) {
    oCounter counter;
    string name;
    formatListString(li.Head.front(), name, li.lp, 0, 0, 0, 0, counter);
    li.lp.updateDefaultName(name);
  }

  generateListInternal(gdi, li, true);
  oListInfo *next = li.next;
  while(next) {
    generateListInternal(gdi, *next, false);
    next = next->next;
  }
}

void oEvent::generateListInternal(gdioutput &gdi, const oListInfo &li, bool formatHead)
{

  oCounter counter;
  //Render header
  
  if (formatHead)
    formatPrintPost(li.Head, gdi, li.lp, 0,0,0,0, counter);
  
  if (li.fixedType) {
    generateFixedList(gdi, li);
    return;
  }

  //int Id=0;
  string oldKey;
  if( li.listType==li.EBaseTypeRunner ) {

    if (li.calcCourseClassResults)
      calculateResults(RTClassCourseResult);

    if (li.calcTotalResults) {
      if (li.calcResults)
        calculateResults(RTClassResult);
       
      calculateResults(RTTotalResult);

      if (li.sortOrder != ClassTotalResult)
        sortRunners(li.sortOrder);
    }
    else if (li.calcResults) {
      if (li.rogainingResults) {
        if (li.sortOrder == CoursePoints)
          calculateCourseRogainingResults();
        else {
          calculateRogainingResults();
          if (li.sortOrder != ClassPoints)
            sortRunners(li.sortOrder);
        }
      }
      else if (li.lp.useControlIdResultTo || li.lp.useControlIdResultFrom) 
        calculateSplitResults(li.lp.useControlIdResultFrom, li.lp.useControlIdResultTo);
      else if (li.sortOrder == CourseResult) {
        calculateResults(RTCourseResult);
      }
      else {
        calculateResults(RTClassResult);
        if (li.sortOrder != ClassResult)
          sortRunners(li.sortOrder);
      }
    }
    else sortRunners(li.sortOrder);

    oRunnerList::iterator it;
    //bool lastLeg = li.lp.legNumber < 0;

	  for (it=Runners.begin(); it != Runners.end(); ++it) {	
      if (it->isRemoved() || it->Status == StatusNotCompetiting)
        continue;

      if (it->legToRun() != li.lp.legNumber && li.lp.legNumber!=-1)
        continue;

      if(!li.lp.selection.empty() && li.lp.selection.count(it->getClassId())==0)
			  continue;
      
      if(li.filter(EFilterExcludeDNS))
        if (it->Status==StatusDNS)
          continue;

      if (li.filter(EFilterVacant)) {
        if (it->isVacant())
          continue;
      }
      if (li.filter(EFilterOnlyVacant)) {
        if (!it->isVacant())
          continue;
      }

      if(li.filter(EFilterHasResult))
        if(li.lp.useControlIdResultTo==0 && it->Status==StatusUnknown)
          continue;
        else if( (li.lp.useControlIdResultTo>0 || li.lp.useControlIdResultFrom>0) && it->tStatus!=StatusOK)
          continue;

      if(li.filter(EFilterRentCard) && it->getDI().getInt("CardFee")==0)
        continue;

      if(li.filter(EFilterHasCard) && it->getCardNo()==0)
        continue;

      if(li.filter(EFilterHasNoCard) && it->getCardNo()>0)
        continue;

      string newKey;
      calculatePrintPostKey(li.subHead, gdi, li.lp, it->tInTeam, &*it, it->Club, it->Class, counter, newKey);
	  
      if (newKey != oldKey) {
        if (li.lp.pageBreak) {
          if (!oldKey.empty()) 
            gdi.addStringUT(gdi.getCY()-1, 0, pageNewPage, "");
          
          gdi.addStringUT(pagePageInfo, it->getClass());
        }
        oldKey.swap(newKey);
        counter.level2=0;
        counter.level3=0;
        formatPrintPost(li.subHead, gdi, li.lp, it->tInTeam, &*it, it->Club, it->Class, counter);
	    }
      if (li.lp.filterMaxPer==0 || counter.level2<li.lp.filterMaxPer) {
        formatPrintPost(li.listPost, gdi, li.lp, it->tInTeam, &*it, it->Club, it->Class, counter);
        
        if(li.listSubType==li.EBaseTypePunches) {
          listGeneratePunches(li.subListPost, gdi, li.lp, it->tInTeam, &*it, it->Club, it->Class); 
        }
      }
      ++counter;
	  }
  }
  else if( li.listType==li.EBaseTypeTeam ) {
    if (li.calcResults)
      calculateTeamResults();
    if (li.rogainingResults)
      throw std::exception("Not implemented");
    if (li.calcCourseClassResults)
      calculateResults(RTClassCourseResult);

    sortTeams(li.sortOrder, li.lp.legNumber);

    oTeamList::iterator it;
	  for (it=Teams.begin(); it != Teams.end(); ++it) {
      if (it->isRemoved() || it->Status == StatusNotCompetiting)
        continue;

      if (!li.lp.selection.empty() && li.lp.selection.count(it->getClassId())==0)
			  continue;

      if(li.filter(EFilterExcludeDNS))
        if (it->Status==StatusDNS)
          continue;

      if (li.filter(EFilterVacant))
        if (it->isVacant())
          continue;

      if (li.filter(EFilterOnlyVacant)) {
        if (!it->isVacant())
          continue;
      }

      if(li.filter(EFilterHasResult) && it->getLegStatus(li.lp.legNumber)==StatusUnknown)
        continue;

//		  if (it->getClassId()!=Id) {
      string newKey;
      calculatePrintPostKey(li.subHead, gdi, li.lp, &*it, 0, it->Club, it->Class, counter, newKey);
      if (newKey != oldKey) {
        if(li.lp.pageBreak) {
          if(!oldKey.empty()) 
            gdi.addStringUT(gdi.getCY()-1, 0, pageNewPage, "");
          
          gdi.addStringUT(pagePageInfo, it->getClass());
        }
        oldKey.swap(newKey);
        //Id=it->getClassId();
        //classCounter=0;
        counter.level2=0;
        counter.level3=0;
        formatPrintPost(li.subHead, gdi, li.lp, &*it, 0, it->Club, it->Class, counter);
		  }
      ++counter;
      if (li.lp.filterMaxPer==0 || counter.level2<=li.lp.filterMaxPer) {
        counter.level3=0;
        formatPrintPost(li.listPost, gdi, li.lp, &*it, 0, it->Club, it->Class, counter);
  
        if(li.subListPost.empty())
          continue;

        if(li.listSubType==li.EBaseTypeRunner) {
          for (int k=0;k<int(it->Runners.size());k++) {
            if(it->Runners[k]) {
              if(it->Runners[k]->Status!=StatusUnknown || !li.filter(EFilterHasResult)) {
                formatPrintPost(li.subListPost, gdi, li.lp, &*it, it->Runners[k], 
                              it->Club, it->Class, counter);
              }
              counter.level3++;
            }
          }
        }
        else if(li.listSubType==li.EBaseTypePunches) {
          pRunner r=it->Runners.empty() ? 0:it->Runners[0];
          if(!r) continue;

          listGeneratePunches(li.subListPost, gdi, li.lp, &*it, r, it->Club, it->Class); 
        }
      }
	  }
  }
  else if( li.listType==li.EBaseTypeClub ) {
    if (li.calcResults)
      calculateTeamResults();
    if (li.calcCourseClassResults)
      calculateResults(RTClassCourseResult);
    
    sortTeams(li.sortOrder, li.lp.legNumber);
    if( li.calcResults ) {
      if (li.lp.useControlIdResultTo || li.lp.useControlIdResultFrom) 
        calculateSplitResults(li.lp.useControlIdResultFrom, li.lp.useControlIdResultTo);
      else 
        calculateResults(RTClassResult);
    }
    else sortRunners(li.sortOrder);

    Clubs.sort();
    oClubList::iterator it;
    
    oRunnerList::iterator rit;
    bool first = true;
	  for (it = Clubs.begin(); it != Clubs.end(); ++it) {	
      if (it->isRemoved())
        continue;

      if (li.filter(EFilterVacant)) {
        if (it->getId() == getVacantClub())
          continue;
      }

      if (li.filter(EFilterOnlyVacant)) {
        if (it->getId() != getVacantClub())
          continue;
      }

      bool startClub = false;
      pRunner pLeader = 0;
      for (rit = Runners.begin(); rit != Runners.end(); ++rit) {
        if (rit->skip() || rit->Status == StatusNotCompetiting)
          continue;

        if (rit->legToRun() != li.lp.legNumber && li.lp.legNumber!=-1)
          continue;

        if(!li.lp.selection.empty() && li.lp.selection.count(rit->getClassId())==0)
			    continue;
      
        if(li.filter(EFilterExcludeDNS))
          if (rit->Status==StatusDNS)
            continue;

        if(li.filter(EFilterHasResult)) {
          if(li.lp.useControlIdResultTo==0 && rit->Status==StatusUnknown)
            continue;
          else if((li.lp.useControlIdResultTo>0 || li.lp.useControlIdResultFrom>0) && rit->tStatus!=StatusOK)
            continue;
        }

        if (!pLeader || pLeader->Class != rit->Class) 
          pLeader = &*rit;
        if (rit->Club == &*it) {
          if (!startClub) {
            if(li.lp.pageBreak) {
              if (!first)
                gdi.addStringUT(gdi.getCY()-1, 0, pageNewPage, "");
              else
                first = false;
              gdi.addStringUT(pagePageInfo, it->getName());
            }
            counter.level2=0;
            counter.level3=0;
            formatPrintPost(li.subHead, gdi, li.lp, 0, 0, &*it, 0, counter);
            startClub = true;
          }
          ++counter;
          if (li.lp.filterMaxPer==0 || counter.level2<=li.lp.filterMaxPer) {
            counter.level3=0;
            formatPrintPost(li.listPost, gdi, li.lp, 0, &*rit, &*it, rit->Class, counter);
            if(li.subListPost.empty())
              continue;
          }
        }
      }//Runners
    }//Clubs
  }
	gdi.updateScrollbars();
}

void oEvent::fillListTypes(gdioutput &gdi, const string &name, int filter)
{
  map<EStdListType, oListInfo> listMap;
  getListTypes(listMap, filter);

  gdi.clearList(name);
  map<EStdListType, oListInfo>::iterator it;
	
  for (it=listMap.begin(); it!=listMap.end(); ++it) {
    gdi.addItem(name, it->second.Name, it->first);
  }
}

void oEvent::getListType(EStdListType type, oListInfo &li) 
{
  map<EStdListType, oListInfo> listMap;
  getListTypes(listMap, false);
  li = listMap[type];
}
  

void oEvent::getListTypes(map<EStdListType, oListInfo> &listMap, int filterResults)
{
  listMap.clear();
  oListInfo li;

  if(!filterResults) {
    li.Name=lang.tl("Startlista, individuell");
    li.listType=li.EBaseTypeRunner;
    li.supportClasses = true;
    li.supportLegs = true;
    li.supportExtra = false;
    listMap[EStdStartList]=li;
  }

  li.Name=lang.tl("Resultat, individuell");
  li.listType=li.EBaseTypeRunner;
  li.supportClasses = true;
  li.supportLegs = true;
  li.supportExtra = true;
  listMap[EStdResultList]=li;

  li.Name=lang.tl("Resultat, course");  // results by course
  li.listType=li.EBaseTypeRunner;
  li.supportClasses = true;
  li.supportLegs = true;
  li.supportExtra = true;
  listMap[EStdCourseResultList]=li;

  li.Name=lang.tl("Resultat, generell");
  li.listType=li.EBaseTypeRunner;
  li.supportClasses = true;
  li.supportLegs = true;
  li.supportExtra = true;
  li.supportLarge = true;
  listMap[EGeneralResultList]=li;

  li.Name=lang.tl("Rogaining, individuell");
  li.listType=li.EBaseTypeRunner;
  li.supportClasses = true;
  li.supportLegs = true;
  li.supportExtra = true;
  listMap[ERogainingInd]=li;

	li.Name=lang.tl("Rogaining, course");  // results by course
  li.listType=li.EBaseTypeRunner;
  li.supportClasses = true;
  li.supportLegs = true;
  li.supportExtra = true;
  listMap[ECourseRogainingInd]=li;

  li.Name=lang.tl("Avgjorda klasser (prisutdelningslista)");
  li.listType=li.EBaseTypeRunner;
  li.supportClasses = true;
  li.supportLegs = true;
  li.supportExtra = true;
  listMap[EIndPriceList]=li;

  if(!filterResults) {
    li.Name=lang.tl("Startlista, patrull");
    li.listType=li.EBaseTypeTeam;
    li.supportClasses = true;
    li.supportLegs = false;
    li.supportExtra = false;

    listMap[EStdPatrolStartList]=li;
  }

  li.Name=lang.tl("Resultat, patrull");
  li.listType=li.EBaseTypeTeam;
  li.supportClasses = true;
  li.supportLegs = false;
  li.supportExtra = false;

  listMap[EStdPatrolResultList]=li;

/*  li.Name="Patrullresultat, lagnamn (STOR)";
  li.listType=li.EBaseTypeTeam;
  li.supportClasses = true;
  li.supportLegs = false;
  li.supportExtra = false;
  li.largeSize = true;
  listMap[EStdRaidResultListLARGE]=li;
*/
  li.Name="Patrullresultat (STOR)";
  li.listType=li.EBaseTypeTeam;
  li.supportClasses = true;
  li.supportLegs = false;
  li.supportExtra = false;
  li.largeSize = true;
  listMap[EStdPatrolResultListLARGE]=li;


  li.Name=lang.tl("Resultat (STOR)");
  li.supportClasses = true;
  li.supportLegs = true;
  li.supportExtra = true;
  li.largeSize = true;
  li.listType=li.EBaseTypeRunner;
  listMap[EStdResultListLARGE]=li;

  li.Name=lang.tl("Stafettresultat, str�cka (STOR)");
  li.supportClasses = true;
  li.supportLegs = true;
  li.supportExtra = false;
  li.largeSize = true;
  li.listType=li.EBaseTypeTeam;
  listMap[EStdTeamResultListLegLARGE]=li;
  
  if(!filterResults) {
    li.Name=lang.tl("Hyrbricksrapport");
    li.listType=li.EBaseTypeRunner;
    li.supportClasses = false;
    li.supportLegs = false;
    li.supportExtra = false;
    listMap[EStdRentedCard]=li;
  }

  li.Name=lang.tl("Stafettresultat, delstr�ckor");
  li.listType=li.EBaseTypeTeam;
  li.supportClasses = true;
  li.supportLegs = false;
  li.supportExtra = false;
  listMap[EStdTeamResultListAll]=li;

  li.Name=lang.tl("Stafettresultat, lag");
  li.listType=li.EBaseTypeTeam;
  li.supportClasses = true;
  li.supportLegs = false;
  li.supportExtra = false;
  listMap[EStdTeamResultList]=li;

  li.Name=lang.tl("Stafettresultat, str�cka");
  li.listType=li.EBaseTypeTeam;
  li.supportClasses = true;
  li.supportLegs = true;
  li.supportExtra = false;
  listMap[EStdTeamResultListLeg]=li;

  if(!filterResults) {
    li.Name=lang.tl("Startlista, stafett (lag)");
    li.listType=li.EBaseTypeTeam;
    li.supportClasses = true;
    li.supportLegs = false;
    li.supportExtra = false;
    listMap[EStdTeamStartList]=li;

    li.Name=lang.tl("Startlista, stafett (str�cka)");
    li.listType=li.EBaseTypeTeam;
    li.supportClasses = true;
    li.supportLegs = true;
    li.supportExtra = false;
    listMap[EStdTeamStartListLeg]=li;

    li.Name=lang.tl("Bantilldelning, stafett");
    li.listType=li.EBaseTypeTeam;
    li.supportClasses = true;
    li.supportLegs = false;
    li.supportExtra = false;
    listMap[ETeamCourseList]=li;

    li.Name=lang.tl("Bantilldelning, individuell");
    li.listType=li.EBaseTypeRunner;
    li.supportClasses = true;
    li.supportLegs = false;
    li.supportExtra = false;
    listMap[EIndCourseList]=li;

    li.Name=lang.tl("Individuell startlista, visst lopp");
    li.listType=li.EBaseTypeTeam;
    li.supportClasses = true;
    li.supportLegs = true;
    li.supportExtra = false;
    listMap[EStdIndMultiStartListLeg]=li;
  }

  li.Name=lang.tl("Individuell resultatlista, visst lopp");
  li.listType=li.EBaseTypeTeam;
  li.supportClasses = true;
  li.supportLegs = true;
  li.supportExtra = true;
  listMap[EStdIndMultiResultListLeg]=li;

  li.Name=lang.tl("Individuell resultatlista, visst lopp (STOR)");
  li.listType=li.EBaseTypeTeam;
  li.supportClasses = true;
  li.supportLegs = true;
  li.supportExtra = true;
  li.largeSize = true;
  listMap[EStdIndMultiResultListLegLARGE]=li;

  li.Name = lang.tl("Individuell resultatlista, alla lopp");
  li.listType = li.EBaseTypeTeam;
  li.supportClasses = true;
  li.supportLegs = false;
  li.supportExtra = false;
  listMap[EStdIndMultiResultListAll]=li;
  
  if(!filterResults) {
    li.Name = lang.tl("Klubbstartlista");
    li.listType = li.EBaseTypeClub;
    li.supportClasses = true;
    li.supportLegs = false;
    li.supportExtra = false;
    listMap[EStdClubStartList]=li;
  }

  li.Name = lang.tl("Klubbresultatlista");
  li.listType = li.EBaseTypeClub;
  li.supportClasses = true;
  li.supportLegs = false;
  li.supportExtra = false;
  listMap[EStdClubResultList]=li;

  
  if(!filterResults) {
    li.Name="UM, m�starpo�ng";
    li.listType=li.EBaseTypeRunner;
    li.supportClasses = false;
    li.supportLegs = false;
    li.supportExtra = false;
    listMap[EStdUM_Master]=li;
    
    li.Name=lang.tl("T�vlingsrapport");
    li.supportClasses = false;
    li.supportLegs = false;
    li.supportExtra = false;
    li.listType=li.EBaseTypeNone;
    listMap[EFixedReport]=li;

    li.Name=lang.tl("Kontroll inf�r t�vlingen");
    li.supportClasses = false;
    li.supportLegs = false;
    li.supportExtra = false;
    li.listType=li.EBaseTypeNone;
    listMap[EFixedPreReport]=li;

    li.Name=lang.tl("Kvar-i-skogen");
    li.supportClasses = false;
    li.supportLegs = false;
    li.supportExtra = false;
    li.listType=li.EBaseTypeNone;
    listMap[EFixedInForest]=li;
    
    li.Name=lang.tl("Fakturor");
    li.supportClasses = false;
    li.supportLegs = false;
    li.supportExtra = false;
    li.listType=li.EBaseTypeNone;
    listMap[EFixedInvoices]=li;
    
    li.Name=lang.tl("Ekonomisk sammanst�llning");
    li.supportClasses = false;
    li.supportLegs = false;
    li.supportExtra = false;
    li.listType=li.EBaseTypeNone;
    listMap[EFixedEconomy]=li;
  }

  li.Name=lang.tl("F�rst-i-m�l, klassvis");
  li.supportClasses = false;
  li.supportLegs = false;
  li.supportExtra = false;
  li.listType=li.EBaseTypeNone;
  listMap[EFixedResultFinishPerClass]=li;
  
  li.Name=lang.tl("F�rst-i-m�l, gemensam");
  li.supportClasses = false;
  li.supportLegs = false;
  li.supportExtra = false;
  li.listType=li.EBaseTypeNone;
  listMap[EFixedResultFinish]=li;
  
  if(!filterResults) {
    li.Name=lang.tl("Minutstartlista");
    li.supportClasses = false;
    li.supportLegs = false;
    li.supportExtra = false;
    li.listType=li.EBaseTypeNone;
    listMap[EFixedMinuteStartlist]=li;
  }

  li.Name=lang.tl("H�ndelser - tidslinje");
  li.supportClasses = true;
  li.supportLegs = false;
  li.supportExtra = false;
  li.listType=li.EBaseTypeNone;
  listMap[EFixedTimeLine]=li;

  getListContainer().setupListInfo(EFirstLoadedList, listMap, filterResults != 0);
}


void oEvent::generateListInfo(EStdListType lt, const gdioutput &gdi, int classId, oListInfo &li)
{
  oListParam par;

  if(classId!=0)
    par.selection.insert(classId);

  par.listCode=lt;
 
  generateListInfo(par, gdi.getLineHeight(), li);
}

int openRunnerTeamCB(gdioutput *gdi, int type, void *data);

void oEvent::generateFixedList(gdioutput &gdi, const oListInfo &li)
{
  string dmy;
  switch (li.lp.listCode) {
    case EFixedPreReport:
      generatePreReport(gdi);
    break;
    
    case EFixedReport:
      generateCompetitionReport(gdi);
    break;

    case EFixedInForest:
      generateInForestList(gdi, openRunnerTeamCB, 0);
    break;

    case EFixedEconomy:
      printInvoices(gdi, IPTAllPrint, dmy, true);
    break;

    case EFixedInvoices:
      printInvoices(gdi, IPTAllPrint, dmy, false);
    break;

    case EFixedResultFinishPerClass:
      generateResultlistFinishTime(gdi, true, li.lp.cb);
    break;
    
    case EFixedResultFinish:
      generateResultlistFinishTime(gdi, false, li.lp.cb);
    break;

    case EFixedMinuteStartlist:
      generateMinuteStartlist(gdi);
    break;

    case EFixedTimeLine:
      gdi.clearPage(false);
      gdi.addString("", boldLarge, MakeDash("Tidslinje - X#" + getName()));

      gdi.dropLine();
      set<__int64> stored;
      vector<oTimeLine> events;
      
      map<int, string> cName;
      for (oClassList::const_iterator it = Classes.begin(); it != Classes.end(); ++it) {
        if (!it->isRemoved())
          cName[it->getId()] = it->getName();
      }

      oe->getTimeLineEvents(li.lp.selection, events, stored, 3600*24*7);
      gdi.fillDown();
      int yp = gdi.getCY();
      int xp = gdi.getCX();

      int w1 = gdi.scaleLength(60);
      int w = gdi.scaleLength(110);
      int w2 = w1+w;
      
      for (size_t k = 0; k<events.size(); k++) {
        const oTimeLine &ev = events[k];

        pRunner r = dynamic_cast<pRunner>(ev.getSource());
        
        if (ev.getType() == oTimeLine::TLTFinish && r->getStatus() != StatusOK)
          continue;

        string name = "";
        if (r)
          name = r->getCompleteIdentification() + " ";

        gdi.addStringUT(yp, xp, 0, oe->getAbsTime(ev.getTime()));
        gdi.addStringUT(yp, xp + w1, 0, cName[ev.getClassId()], w-10);
        gdi.addStringUT(yp, xp + w2, 0, name + lang.tl(ev.getMessage()));

        yp += gdi.getLineHeight();

        
        /*string detail = ev.getDetail();
        
        if (detail.size() > 0) {
          gdi.addStringUT(yp, xp + w, 0, detail);
          yp += gdi.getLineHeight();
        }*/
          
      }
      gdi.refresh();
    
    break;
  }
}

void generateNBestHead(const oListParam &par, oListInfo &li, int ypos) {
  if (par.filterMaxPer > 0)
    li.addHead(oPrintPost(lString, lang.tl("Visar de X b�sta#" + itos(par.filterMaxPer)), normalText, 0, ypos));
}

void oEvent::getResultTitle(const oListInfo &li, char *title) {
  if (li.lp.useControlIdResultTo==0 && li.lp.useControlIdResultFrom==0)
    sprintf_s(title, 256, "%s", lang.tl("Resultat - %s").c_str());
  else if (li.lp.useControlIdResultTo>0 && li.lp.useControlIdResultFrom==0){
    pControl ctrl=getControl(li.lp.useControlIdResultTo, false);
    if(ctrl && !ctrl->Name.empty())
      sprintf_s(title, 256, "%s, %s", lang.tl("Resultat - %s").c_str(), ctrl->Name.c_str());
    else if(ctrl)
      sprintf_s(title, 256, lang.tl("%s, vid kontroll %d").c_str(), 
      lang.tl("Resultat - %s").c_str(), ctrl->Numbers[0]);
    else
      sprintf_s(title, 256, lang.tl("Resultat vid ok�nd kontroll").c_str());
  }
  else {
    string fromS = lang.tl("Start"), toS = lang.tl("M�l");
    if (li.lp.useControlIdResultTo) {
      pControl to = getControl(li.lp.useControlIdResultTo, false);
      if (to && !to->Name.empty())
        toS = to->Name;
      else
        toS = itos(li.lp.useControlIdResultTo);
    }
    if (li.lp.useControlIdResultFrom) {
      pControl from = getControl(li.lp.useControlIdResultFrom, false);
      if (from && !from->Name.empty())
        fromS = from->Name;
      else
        fromS = itos(li.lp.useControlIdResultFrom);
    }
    sprintf_s(title, 256, lang.tl("Resultat mellan X och Y#" + fromS + "#" + toS).c_str());          
  }
}


void oEvent::generateListInfo(oListParam &par, int lineHeight, oListInfo &li)
{
  lineHeight = 14;
  par.cb = 0;
  const int lh=lineHeight;
  const int vspace=lh/2;
  EStdListType lt=par.listCode;
  li=oListInfo();
  li.lp = par;
  int bib, ln;
  string ttl;
  Position pos;
  const double scale = 1.8;
 
  map<EStdListType, oListInfo> listMap;
  getListTypes(listMap, false);
  li.Name = listMap[lt].Name;
  li.lp.defaultName = li.Name;

  switch (lt) {
    case EStdStartList: {
      li.addHead(oPrintPost(lCmpName, MakeDash(lang.tl("Startlista - %s")), boldLarge, 0,0));
      li.addHead(oPrintPost(lCmpDate, "", normalText, 0, 25));
      
      int bib = 0;
      int rank = 0;
      if (hasBib(true, false)) {
        li.addListPost(oPrintPost(lRunnerBib, "", normalText, 0, 0));
        bib=40;
      }
      li.addListPost(oPrintPost(lRunnerStart, "", normalText, 0+bib, 0));
      li.addListPost(oPrintPost(lPatrolNameNames, "", normalText, 70+bib, 0));
      li.addListPost(oPrintPost(lPatrolClubNameNames, "", normalText, 300+bib, 0));
      if (hasRank()) {
        li.addListPost(oPrintPost(lRunnerRank, "", normalText, 470+bib, 0));
        rank = 50;
      }
      li.addListPost(oPrintPost(lRunnerCard, "", normalText, 470+bib+rank, 0));

      li.addSubHead(oPrintPost(lClassName, "", boldText, 0, 12));
      li.addSubHead(oPrintPost(lClassLength, lang.tl("%s meter"), boldText, 300+bib, 12));
      li.addSubHead(oPrintPost(lClassStartName, "", boldText, 470+bib+rank, 12));
      li.addSubHead(oPrintPost(lString, "", boldText, 470+bib, 16));

      li.listType=li.EBaseTypeRunner;
      li.sortOrder=ClassStartTime;
      li.setFilter(EFilterExcludeDNS);
      break;
    }

    case EStdClubStartList: {
      li.addHead(oPrintPost(lCmpName, MakeDash(lang.tl("Klubbstartlista - %s")), boldLarge, 0,0));
      li.addHead(oPrintPost(lCmpDate, "", normalText, 0, 25));
      
      if (hasBib(true, true)) {
        pos.add("bib", 40);
        li.addListPost(oPrintPost(lRunnerBib, "", normalText, 0, 0));
      }
  
      pos.add("start", li.getMaxCharWidth(this, lRunnerStart, "", normalText));
      li.addListPost(oPrintPost(lRunnerStart, "", normalText, pos.get("start"), 0));
 
      if (hasRank()) {
        pos.add("rank", 50);
        li.addListPost(oPrintPost(lRunnerRank, "", normalText, pos.get("rank"), 0));
      }
      pos.add("name", li.getMaxCharWidth(this, lRunnerName, "", normalText));
      li.addListPost(oPrintPost(lPatrolNameNames, "", normalText, pos.get("name"), 0));
      pos.add("class", li.getMaxCharWidth(this, lClassName, "", normalText));
      li.addListPost(oPrintPost(lClassName, "", normalText, pos.get("class"), 0));
      pos.add("length", li.getMaxCharWidth(this, lClassLength, "%s m", normalText));
      li.addListPost(oPrintPost(lClassLength, lang.tl("%s m"), normalText, pos.get("length"), 0));
      pos.add("sname", li.getMaxCharWidth(this, lClassStartName, "", normalText));
      li.addListPost(oPrintPost(lClassStartName, "", normalText, pos.get("sname"), 0));
      
      pos.add("card", 70);
      li.addListPost(oPrintPost(lRunnerCard, "", normalText, pos.get("card"), 0));

      li.addSubHead(oPrintPost(lClubName, "", boldText, 0, 12));
      li.addSubHead(oPrintPost(lString, "", boldText, 100, 16));

      li.listType=li.EBaseTypeClub;
      li.sortOrder=ClassTeamLeg;
      li.setFilter(EFilterExcludeDNS);
      li.setFilter(EFilterVacant);
      break;
    }   

    case EStdClubResultList: {
      li.addHead(oPrintPost(lCmpName, MakeDash(lang.tl("Klubbresultatlista - %s")), boldLarge, 0,0));
      li.addHead(oPrintPost(lCmpDate, "", normalText, 0, 25));

      pos.add("class", li.getMaxCharWidth(this, lClassName, "", normalText));
      li.addListPost(oPrintPost(lClassName, "", normalText, pos.get("class"), 0));

      pos.add("place", 40);
      li.addListPost(oPrintPost(lRunnerPlace, "", normalText, pos.get("place"), 0));
 
      pos.add("name", li.getMaxCharWidth(this, lPatrolNameNames, "", normalText));
      li.addListPost(oPrintPost(lRunnerName, "", normalText, pos.get("name"), 0));
 
      pos.add("time", li.getMaxCharWidth(this, lRunnerTimeStatus, "", normalText));
      li.addListPost(oPrintPost(lRunnerTimeStatus, "", normalText, pos.get("time"), 0));
 
      pos.add("after", li.getMaxCharWidth(this, lRunnerTimeAfter, "", normalText));
      li.addListPost(oPrintPost(lRunnerTimeAfter, "", normalText, pos.get("after"), 0));
 
      li.addSubHead(oPrintPost(lClubName, "", boldText, 0, 12));
      li.addSubHead(oPrintPost(lString, "", boldText, 100, 16));

      li.listType=li.EBaseTypeClub;
      li.sortOrder=ClassResult;
      li.calcResults = true;
      li.setFilter(EFilterVacant);
      break;
    }   

    case EStdRentedCard: 
    {
      li.addHead(oPrintPost(lCmpName, MakeDash(lang.tl("Hyrbricksrapport - %s")), boldLarge, 0,0));
      li.addHead(oPrintPost(lCmpDate, "", normalText, 0, 25));
     
      li.addListPost(oPrintPost(lTotalCounter, "%s", normalText, 0, 0));     
      li.addListPost(oPrintPost(lRunnerCard, "", normalText, 30, 0));
      li.addListPost(oPrintPost(lRunnerName, "", normalText, 130, 0));      
      li.addSubHead(oPrintPost(lClassName, "", boldText, 0, 10));
      
      li.setFilter(EFilterHasCard);
      li.setFilter(EFilterRentCard);
      li.setFilter(EFilterExcludeDNS);
      li.listType=li.EBaseTypeRunner;
      li.sortOrder=ClassStartTime;
      break;
    }   

    case EStdResultList:
      char title[256];
      getResultTitle(li, title);
      par.getCustomTitle(title);

      li.addHead(oPrintPost(lCmpName, MakeDash(title), boldLarge, 0,0));
      li.addHead(oPrintPost(lCmpDate, "", normalText, 0, 25));
      generateNBestHead(par, li, 25+lh);

      pos.add("place", 25);
      pos.add("name", li.getMaxCharWidth(this, lPatrolNameNames, "", normalText));
      pos.add("club", li.getMaxCharWidth(this, lPatrolClubNameNames, "", normalText));
      pos.add("status", 50);
      pos.add("after", 50);
      pos.add("missed", 50);

      li.addSubHead(oPrintPost(lClassName, "", boldText, 0, 10));

      li.addListPost(oPrintPost(lRunnerPlace, "", normalText, pos.get("place"), 0));
      li.addListPost(oPrintPost(lPatrolNameNames, "", normalText, pos.get("name"), 0));
      li.addListPost(oPrintPost(lPatrolClubNameNames, "", normalText, pos.get("club"), 0));

      if (li.lp.useControlIdResultTo==0 && li.lp.useControlIdResultFrom==0) {
        li.addSubHead(oPrintPost(lClassResultFraction, "", boldText, pos.get("club"), 10));
 
        li.addListPost(oPrintPost(lRunnerTimeStatus, "", normalText, pos.get("status"), 0));
        li.addListPost(oPrintPost(lRunnerTimeAfter, "", normalText, pos.get("after"), 0));
        if(li.lp.showInterTimes) {
          li.addSubListPost(oPrintPost(lPunchNamedTime, "", italicSmall, pos.get("name"), 0, 1));
          li.subListPost.back().fixedWidth = 160;
          li.listSubType = li.EBaseTypePunches;
        }
        else if (li.lp.showSplitTimes) {
          li.addSubListPost(oPrintPost(lPunchTime, "", italicSmall, pos.get("name"), 0, 1));
          li.subListPost.back().fixedWidth = 95;
          li.listSubType = li.EBaseTypePunches;
        }
      }
      else {
        li.needPunches = true;
        li.addListPost(oPrintPost(lRunnerTempTimeStatus, "", normalText, pos.get("status"), 0));
        li.addListPost(oPrintPost(lRunnerTempTimeAfter, "", normalText, pos.get("after"), 0));
      }
      li.addSubHead(oPrintPost(lString, lang.tl("Tid"), boldText, pos.get("status"), 10));
      li.addSubHead(oPrintPost(lString, lang.tl("Efter"), boldText, pos.get("after"), 10));

      if (li.lp.splitAnalysis)  {
        li.addListPost(oPrintPost(lRunnerMissedTime, "", normalText, pos.get("missed"), 0));
        li.addSubHead(oPrintPost(lString, lang.tl("Bomtid"), boldText, pos.get("missed"), 10));
      }

      li.calcResults = true;
      li.listType=li.EBaseTypeRunner;
      li.sortOrder=ClassResult;
      li.setFilter(EFilterHasResult);
      break;

    case EStdCourseResultList:
      getResultTitle(li, title);
      par.getCustomTitle(title);

      li.addHead(oPrintPost(lCmpName, MakeDash(title), boldLarge, 0,0));
      li.addHead(oPrintPost(lCmpDate, "", normalText, 0, 25));
      generateNBestHead(par, li, 25+lh);

      pos.add("place", 25);
      pos.add("name", li.getMaxCharWidth(this, lPatrolNameNames, "", normalText));
      pos.add("club", li.getMaxCharWidth(this, lPatrolClubNameNames, "", normalText));
      pos.add("status", 50);
      pos.add("after", 50);
      pos.add("missed", 50);

      li.addSubHead(oPrintPost(lCourseName, "", boldText, 0, 10));

      li.addListPost(oPrintPost(lRunnerPlace, "", normalText, pos.get("place"), 0));
      li.addListPost(oPrintPost(lPatrolNameNames, "", normalText, pos.get("name"), 0));
      li.addListPost(oPrintPost(lPatrolClubNameNames, "", normalText, pos.get("club"), 0));

      if (li.lp.useControlIdResultTo==0 && li.lp.useControlIdResultFrom==0) {
        li.addListPost(oPrintPost(lRunnerTimeStatus, "", normalText, pos.get("status"), 0));
        li.addListPost(oPrintPost(lRunnerTimeAfter, "", normalText, pos.get("after"), 0));
        if(li.lp.showInterTimes) {
          li.addSubListPost(oPrintPost(lPunchNamedTime, "", italicSmall, pos.get("name"), 0, 1));
          li.subListPost.back().fixedWidth = 160;
          li.listSubType = li.EBaseTypePunches;
        }
        else if (li.lp.showSplitTimes) {
          li.addSubListPost(oPrintPost(lPunchTime, "", italicSmall, pos.get("name"), 0, 1));
          li.subListPost.back().fixedWidth = 95;
          li.listSubType = li.EBaseTypePunches;
        }
      }
      else {
        li.needPunches = true;
        li.addListPost(oPrintPost(lRunnerTempTimeStatus, "", normalText, pos.get("status"), 0));
        li.addListPost(oPrintPost(lRunnerTempTimeAfter, "", normalText, pos.get("after"), 0));
      }
      li.addSubHead(oPrintPost(lString, lang.tl("Tid"), boldText, pos.get("status"), 10));
      li.addSubHead(oPrintPost(lString, lang.tl("Efter"), boldText, pos.get("after"), 10));

      if (li.lp.splitAnalysis)  {
        li.addListPost(oPrintPost(lRunnerMissedTime, "", normalText, pos.get("missed"), 0));
        li.addSubHead(oPrintPost(lString, lang.tl("Bomtid"), boldText, pos.get("missed"), 10));
      }

      li.calcResults = true;
      li.listType=li.EBaseTypeRunner;
      li.sortOrder=CourseResult;
      li.setFilter(EFilterHasResult);
      break;

    case EGeneralResultList:
      getResultTitle(li, title);
      par.getCustomTitle(title);

      gdiFonts normal, header, small;
      double s;

      if (par.useLargeSize) {
        s = scale;
        normal = fontLarge;
        header = boldLarge;
        small = normalText;
      }
      else {
        s = 1.0;
        normal = normalText;
        header = boldText;
        small = italicSmall;
      }

      li.addHead(oPrintPost(lCmpName, MakeDash(title), boldLarge, 0,0));
      li.addHead(oPrintPost(lCmpDate, "", normalText, 0, 25));
      generateNBestHead(par, li, 25+lh);

      pos.add("place", 25);
      pos.add("name", li.getMaxCharWidth(this, lRunnerCompleteName, "", normalText));
      pos.add("status", 50);
      pos.add("after", 50);
      pos.add("missed", 50);

      li.addSubHead(oPrintPost(lClassName, "", header, 0, 10));

      li.addListPost(oPrintPost(lRunnerPlace, "", normal, pos.get("place", s), 0));
      li.addListPost(oPrintPost(lRunnerCompleteName, "", normal, pos.get("name", s), 0));

      if (li.lp.useControlIdResultTo==0 && li.lp.useControlIdResultFrom==0) {
        li.addListPost(oPrintPost(lRunnerTimeStatus, "", normal, pos.get("status", s), 0));
        li.addListPost(oPrintPost(lRunnerTimeAfter, "", normal, pos.get("after", s), 0));
        if(li.lp.showInterTimes) {
          li.addSubListPost(oPrintPost(lPunchNamedTime, "", small, pos.get("name", s), 0, 1));
          li.subListPost.back().fixedWidth = 160;
          li.listSubType = li.EBaseTypePunches;
        }
        else if (li.lp.showSplitTimes) {
          li.addSubListPost(oPrintPost(lPunchTime, "", small, pos.get("name", s), 0, 1));
          li.subListPost.back().fixedWidth = 95;
          li.listSubType = li.EBaseTypePunches;
        }
      }
      else {
        li.needPunches = true;
        li.addListPost(oPrintPost(lRunnerTempTimeStatus, "", normal, pos.get("status", s), 0));
        li.addListPost(oPrintPost(lRunnerTempTimeAfter, "", normal, pos.get("after", s), 0));
      }
      li.addSubHead(oPrintPost(lString, lang.tl("Tid"), header, pos.get("status", s), 10));
      li.addSubHead(oPrintPost(lString, lang.tl("Efter"), header, pos.get("after", s), 10));

      li.calcResults = true;
      li.listType=li.EBaseTypeRunner;
      li.sortOrder=ClassResult;
      li.setFilter(EFilterHasResult);
      break;

    case EIndPriceList:
      
      li.addHead(oPrintPost(lCmpName, MakeDash(lang.tl("Avgjorda placeringar - %s")), boldLarge, 0,0));
      li.addHead(oPrintPost(lCurrentTime, lang.tl("Genererad: ") + "%s", normalText, 0, 25));
      generateNBestHead(par, li, 25+lh);

      pos.add("place", 25);
      pos.add("name", li.getMaxCharWidth(this, lPatrolNameNames, "", normalText));
      pos.add("club", li.getMaxCharWidth(this, lPatrolClubNameNames, "", normalText));
      pos.add("status", 80);
      pos.add("info", 80);

      li.addSubHead(oPrintPost(lClassName, "", boldText, 0, 10));
      li.addSubHead(oPrintPost(lClassResultFraction, "", boldText, pos.get("club"), 10));
      li.addSubHead(oPrintPost(lString, lang.tl("Tid"), boldText, pos.get("status"), 10));
      li.addSubHead(oPrintPost(lString, lang.tl("Avg�rs kl"), boldText, pos.get("info"), 10));

      li.addListPost(oPrintPost(lRunnerPlace, "", normalText, pos.get("place"), 0));
      li.addListPost(oPrintPost(lPatrolNameNames, "", normalText, pos.get("name"), 0));
      li.addListPost(oPrintPost(lPatrolClubNameNames, "", normalText, pos.get("club"), 0));
      li.addListPost(oPrintPost(lRunnerTimeStatus, "", normalText, pos.get("status"), 0));
      li.addListPost(oPrintPost(lRunnerTimePlaceFixed, "", normalText, pos.get("info"), 0));
      
      li.calcResults = true;
      li.listType = li.EBaseTypeRunner;
      li.sortOrder = ClassResult;
      li.setFilter(EFilterHasResult);
      break;

    case EStdTeamResultList:
      li.addHead(oPrintPost(lCmpName, MakeDash(lang.tl("Resultatsammanst�llning - %s")), boldLarge, 0,0));
      li.addHead(oPrintPost(lCmpDate, "", normalText, 0, 25));
      generateNBestHead(par, li, 25+lh);

      li.addSubHead(oPrintPost(lClassName, "", boldText, 0, 14));
      li.addSubHead(oPrintPost(lClassResultFraction, "", normalText, 280, 14));

      //Use last leg for every team (index=-1)
      li.addListPost(oPrintPost(lTeamPlace, "", normalText, 0, 5, -1));
      li.addListPost(oPrintPost(lTeamName, "", normalText, 25, 5, -1));
      li.addListPost(oPrintPost(lTeamTimeStatus, "", normalText, 280, 5, -1));
      li.addListPost(oPrintPost(lTeamTimeAfter, "", normalText, 340, 5, -1));
      
      li.lp.legNumber=-1;
      li.calcResults=true;
      li.listType=li.EBaseTypeTeam;
      li.listSubType=li.EBaseTypeRunner;
      li.sortOrder=ClassResult;
      li.setFilter(EFilterHasResult); 
      break;

    case EStdTeamResultListAll:
      li.addHead(oPrintPost(lCmpName, MakeDash(lang.tl("Resultat - %s")), boldLarge, 0,0));
      li.addHead(oPrintPost(lCmpDate, "", normalText, 0, 25));
      generateNBestHead(par, li, 25+lh);

      li.addSubHead(oPrintPost(lClassName, "", boldText, 0, 14));
      li.addSubHead(oPrintPost(lClassResultFraction, "", boldText, 280, 14));
      li.addSubHead(oPrintPost(lString, lang.tl("Tid"), boldText, 400, 14));
      li.addSubHead(oPrintPost(lString, lang.tl("Efter"), boldText, 460, 14));

      //Use last leg for every team (index=-1)
      li.addListPost(oPrintPost(lTeamPlace, "", normalText, 0, 5, -1));
      li.addListPost(oPrintPost(lTeamName, "", normalText, 25, 5, -1));
      li.addListPost(oPrintPost(lTeamTimeStatus, "", normalText, 400, 5, -1));
      li.addListPost(oPrintPost(lTeamTimeAfter, "", normalText, 460, 5, -1));
      
      li.addSubListPost(oPrintPost(lRunnerLegNumberAlpha, "%s.", normalText, 25, 0, 0));
      li.addSubListPost(oPrintPost(lRunnerName, "", normalText, 45, 0, 0));
      li.addSubListPost(oPrintPost(lRunnerTimeStatus, "", normalText, 280, 0, 0));
      li.addSubListPost(oPrintPost(lTeamLegTimeStatus, "", normalText, 400, 0, 0));
      li.addSubListPost(oPrintPost(lTeamLegTimeAfter, "", normalText, 460, 0, 0));

      if (li.lp.splitAnalysis)  {
        li.addSubListPost(oPrintPost(lRunnerMissedTime, "", normalText, 510, 0));
        li.addSubHead(oPrintPost(lString, lang.tl("Bomtid"), boldText, 510, 14));
      }

      li.lp.legNumber=-1;
      li.calcResults=true;
      li.listType=li.EBaseTypeTeam;
      li.listSubType=li.EBaseTypeRunner;
      li.sortOrder=ClassResult;
      li.setFilter(EFilterHasResult); 
      break;

    case EStdTeamResultListLeg:
      if (li.lp.legNumber>=0)
        sprintf_s(title, (lang.tl("Resultat efter str�cka %d")+" - %%s").c_str(), li.lp.legNumber+1);
      else
        sprintf_s(title, (lang.tl("Resultat efter str�ckan")+" - %%s").c_str());

      pos.add("place", 25);
      pos.add("team", li.getMaxCharWidth(this, lTeamName, "", normalText));
      pos.add("name", li.getMaxCharWidth(this, lRunnerName, "", normalText));
      pos.add("status", 50);
      pos.add("teamstatus", 50);
      pos.add("after", 50);
      pos.add("missed", 50);
      
      li.addHead(oPrintPost(lCmpName, MakeDash(title), boldLarge, 0,0));
      li.addHead(oPrintPost(lCmpDate, "", normalText, 0, 25));
      generateNBestHead(par, li, 25+lh);

      li.addSubHead(oPrintPost(lClassName, "", boldText, pos.get("place"), 14));
      li.addSubHead(oPrintPost(lClassResultFraction, "", boldText, pos.get("name"), 14));
      li.addSubHead(oPrintPost(lString, lang.tl("Tid"), boldText, pos.get("status"), 14));
      li.addSubHead(oPrintPost(lString, lang.tl("Totalt"), boldText, pos.get("teamstatus"), 14));
      li.addSubHead(oPrintPost(lString, lang.tl("Efter"), boldText, pos.get("after"), 14));

      ln=li.lp.legNumber;
      li.addListPost(oPrintPost(lTeamPlace, "", normalText, pos.get("place"), 2, ln));
      li.addListPost(oPrintPost(lTeamName, "", normalText, pos.get("team"), 2, ln));
      li.addListPost(oPrintPost(lRunnerName, "", normalText, pos.get("name"), 2, ln));
      li.addListPost(oPrintPost(lRunnerTimeStatus, "", normalText, pos.get("status"), 2, ln));
      li.addListPost(oPrintPost(lTeamTimeStatus, "", normalText, pos.get("teamstatus"), 2, ln));
      li.addListPost(oPrintPost(lTeamTimeAfter, "", normalText, pos.get("after"), 2, ln));
      
      if (li.lp.splitAnalysis)  {
        li.addListPost(oPrintPost(lRunnerMissedTime, "", normalText, pos.get("missed"), 2, ln));
        li.addSubHead(oPrintPost(lString, lang.tl("Bomtid"), boldText, pos.get("missed"), 14));
      }

      li.calcResults=true;
      li.listType=li.EBaseTypeTeam;
      li.sortOrder=ClassResult;
      li.setFilter(EFilterHasResult); 
      break;

    case EStdTeamResultListLegLARGE:
      if (li.lp.legNumber>=0)
        sprintf_s(title, ("%%s - "+lang.tl("str�cka %d")).c_str(), li.lp.legNumber+1);
      else
        sprintf_s(title, ("%%s - "+lang.tl("slutstr�ckan")).c_str());
      
      pos.add("place", 25);
      pos.add("team", min(120, li.getMaxCharWidth(this, lTeamName, "", normalText)));
      pos.add("name", min(120, li.getMaxCharWidth(this, lRunnerName, "", normalText)));
      pos.add("status", 50);
      pos.add("teamstatus", 50);
      pos.add("after", 50);
      pos.add("missed", 50);
      
      li.addSubHead(oPrintPost(lClassName, MakeDash(title), boldLarge, pos.get("place", scale), 14));
      li.addSubHead(oPrintPost(lClassResultFraction, "", boldLarge, pos.get("name", scale), 14));

      li.addSubHead(oPrintPost(lString, lang.tl("Tid"), boldLarge, pos.get("status", scale), 14));
      li.addSubHead(oPrintPost(lString, lang.tl("Totalt"), boldLarge, pos.get("teamstatus", scale), 14));
      li.addSubHead(oPrintPost(lString, lang.tl("Efter"), boldLarge, pos.get("after", scale), 14));

      ln=li.lp.legNumber;
      li.addListPost(oPrintPost(lTeamPlace, "", fontLarge, pos.get("place", scale), 5, ln));
      li.addListPost(oPrintPost(lTeamName, "", fontLarge, pos.get("team", scale), 5, ln)); 
      li.addListPost(oPrintPost(lRunnerName, "", fontLarge, pos.get("name", scale), 5, ln));
      li.addListPost(oPrintPost(lRunnerTimeStatus, "", fontLarge, pos.get("status", scale), 5, ln));
      li.addListPost(oPrintPost(lTeamTimeStatus, "", fontLarge, pos.get("teamstatus", scale), 5, ln));
      li.addListPost(oPrintPost(lTeamTimeAfter, "", fontLarge, pos.get("after", scale), 5, ln));

      if (li.lp.splitAnalysis)  {
        li.addListPost(oPrintPost(lRunnerMissedTime, "", fontLarge, pos.get("missed", scale), 5, ln));
        li.addSubHead(oPrintPost(lString, lang.tl("Bomtid"), boldLarge, pos.get("missed", scale), 14));
      }

      li.calcResults=true;
      li.listType=li.EBaseTypeTeam;
      li.sortOrder=ClassResult;
      li.setFilter(EFilterHasResult); 
      break;

    case EStdTeamStartList:
      {
        MetaList mList;
        mList.setListName("Startlista, stafett");
        mList.addToHead(lCmpName).setText("Startlista - X");;
        mList.newHead();
        mList.addToHead(lCmpDate);

        mList.addToSubHead(lClassName);
        mList.addToSubHead(MetaListPost(lClassStartTime, lNone));
        mList.addToSubHead(MetaListPost(lClassLength, lNone)).setText("X meter");
        mList.addToSubHead(MetaListPost(lClassStartName, lNone));

        mList.addToList(lTeamBib);
        mList.addToList(lTeamStart);
        mList.addToList(lTeamName);
      
        mList.addToSubList(lRunnerLegNumberAlpha).align(lTeamName, false).setText("X.");
        mList.addToSubList(lRunnerName);
        mList.addToSubList(lRunnerCard).align(lClassStartName);

        mList.interpret(this, gdibase, par, lh, li);
      }
/*      li.addHead(oPrintPost(lCmpName, MakeDash(lang.tl("Startlista - %s")), boldLarge, 0,0));
      li.addHead(oPrintPost(lCmpDate, "", normalText, 0, 25));

      li.listType=li.EBaseTypeTeam;
      bib=0;
      if (hasBib(false, true)) {
        li.addListPost(oPrintPost(lTeamBib, "", normalText, 0, 0));
        bib=40;
      }
      li.addListPost(oPrintPost(lTeamStart, "", normalText, 0+bib, 0));
      li.addListPost(oPrintPost(lTeamName, "", normalText, 70+bib, 0));
      li.addListPost(oPrintPost(lTeamClub, "", normalText, 300+bib, 0));
      
      li.listSubType=li.EBaseTypeRunner;
      li.addSubListPost(oPrintPost(lSubSubCounter, "", normalText, 0+bib, 0, 0));
      li.addSubListPost(oPrintPost(lRunnerName, "", normalText, 70+bib, 0, 0));
      li.addSubListPost(oPrintPost(lRunnerCard, "", normalText, 300+bib, 0, 0));
    
      li.addSubHead(oPrintPost(lClassName, "", boldText, 0, 10));
      li.addSubHead(oPrintPost(lClassStartName, "", normalText, 470+bib, 10));
*/
      li.listType=li.EBaseTypeTeam;
      li.listSubType=li.EBaseTypeRunner;
      li.setFilter(EFilterExcludeDNS);
      li.sortOrder = ClassStartTime;
      break;

    case ETeamCourseList:
      li.addHead(oPrintPost(lCmpName, MakeDash(lang.tl("Bantilldelningslista - %s")), boldLarge, 0,0));
      li.addHead(oPrintPost(lCmpDate, "", normalText, 0, 25));
      li.listType=li.EBaseTypeTeam;

      bib=0;
      li.addListPost(oPrintPost(lTeamBib, "", normalText, 0+bib, 4));
      li.addListPost(oPrintPost(lTeamName, "", normalText, 50+bib, 4));
      li.addListPost(oPrintPost(lTeamClub, "", normalText, 250+bib, 4));
      
      li.listSubType=li.EBaseTypeRunner;
      li.addSubListPost(oPrintPost(lRunnerLegNumberAlpha, "%s.", normalText, 25+bib, 0, 0));
      li.addSubListPost(oPrintPost(lRunnerName, "", normalText, 50+bib, 0, 0));
      li.addSubListPost(oPrintPost(lRunnerCard, "", normalText, 250+bib, 0, 0));
      li.addSubListPost(oPrintPost(lRunnerCourse, "", normalText, 370+bib, 0, 0));
    
      li.addSubHead(oPrintPost(lClassName, "", boldText, 0, 10));
      li.addSubHead(oPrintPost(lString, lang.tl("Bricka"), boldText, 250+bib, 10));
      li.addSubHead(oPrintPost(lString, lang.tl("Bana"),   boldText, 370+bib, 10));
      
      li.sortOrder = ClassStartTime;
      break;

    case EIndCourseList:
      li.addHead(oPrintPost(lCmpName, MakeDash(lang.tl("Bantilldelningslista - %s")), boldLarge, 0,0));
      li.addHead(oPrintPost(lCmpDate, "", normalText, 0, 25));
      li.listType=li.EBaseTypeRunner;

      bib=0;
      li.addListPost(oPrintPost(lRunnerBib, "", normalText, 0+bib, 0));
      li.addListPost(oPrintPost(lRunnerName, "", normalText, 50+bib, 0));
      li.addListPost(oPrintPost(lRunnerClub, "", normalText, 250+bib, 0));
      li.addListPost(oPrintPost(lRunnerCard, "", normalText, 350+bib, 0));
      li.addListPost(oPrintPost(lRunnerCourse, "", normalText, 420+bib, 0));
      
      li.addSubHead(oPrintPost(lClassName, "", boldText, 0, 10));
      li.addSubHead(oPrintPost(lString, lang.tl("Bricka"), boldText, 350+bib, 10));
      li.addSubHead(oPrintPost(lString, lang.tl("Bana"),   boldText, 420+bib, 10));
      
      li.sortOrder = ClassStartTime;
      break;


    case EStdTeamStartListLeg:
      if (li.lp.legNumber==-1)
        throw std::exception("Ogiltigt val av str�cka");

      sprintf_s(title, lang.tl("Startlista %%s - str�cka %d").c_str(), li.lp.legNumber+1);

      li.addHead(oPrintPost(lCmpName, MakeDash(title), boldLarge, 0,0));
      li.addHead(oPrintPost(lCmpDate, "", normalText, 0, 25));

      ln=li.lp.legNumber;
      li.listType=li.EBaseTypeTeam;
      bib=0;
      if (hasBib(false, true)) {
        li.addListPost(oPrintPost(lTeamBib, "", normalText, 0, 0));
        bib=40;
      }
      li.addListPost(oPrintPost(lTeamStart, "", normalText, 0+bib, 0, ln));
      li.addListPost(oPrintPost(lTeamName, "", normalText, 70+bib, 0, ln));
      li.addListPost(oPrintPost(lTeamRunner, "", normalText, 300+bib, 0, ln));
      li.addListPost(oPrintPost(lTeamRunnerCard, "", normalText, 520+bib, 0, ln));
      
      li.addSubHead(oPrintPost(lClassName, "", boldText, 0, 10));
      li.addSubHead(oPrintPost(lClassStartName, "", normalText, 300+bib, 10));

      li.sortOrder=ClassStartTime;
      li.setFilter(EFilterExcludeDNS);
      break;

    case EStdIndMultiStartListLeg:
      if (li.lp.legNumber==-1)
        throw std::exception("Ogiltigt val av str�cka");

      //sprintf_s(title, lang.tl("Startlista lopp %d - %%s").c_str(), li.lp.legNumber+1);
      ln=li.lp.legNumber;
     
      ttl = MakeDash(lang.tl("Startlista lopp X - Y#" + itos(ln+1) + "#%s"));
      li.addHead(oPrintPost(lCmpName, ttl, boldLarge, 0,0));
      li.addHead(oPrintPost(lCmpDate, "", normalText, 0, 25));

      li.listType=li.EBaseTypeTeam;
      bib=0;
      if (hasBib(false, true)) {
        li.addListPost(oPrintPost(lTeamBib, "", normalText, 0, 0));
        bib=40;
      }
      li.addListPost(oPrintPost(lTeamStart, "", normalText, 0+bib, 0, ln));
      li.addListPost(oPrintPost(lTeamRunner, "", normalText, 70+bib, 0, ln));
      li.addListPost(oPrintPost(lTeamClub, "", normalText, 300+bib, 0, ln));
      li.addListPost(oPrintPost(lTeamRunnerCard, "", normalText, 500+bib, 0, ln));
      
      li.addSubHead(oPrintPost(lClassName, "", boldText, 0, 10));
      li.addSubHead(oPrintPost(lClassLength, lang.tl("%s meter"), boldText, 300+bib, 10, ln));
      li.addSubHead(oPrintPost(lClassStartName, "", boldText, 500+bib, 10, ln));
      
      li.sortOrder=ClassStartTime;
      li.setFilter(EFilterExcludeDNS);
      break;

    case EStdIndMultiResultListLeg:
      ln=li.lp.legNumber;

      if (li.lp.legNumber>=0)
        ttl = lang.tl("Resultat lopp X - Y#" + itos(ln+1) + "#%s");
      else
        ttl = lang.tl("Resultat - X#%s");
      
      li.addHead(oPrintPost(lCmpName, MakeDash(ttl), boldLarge, 0,0));
      li.addHead(oPrintPost(lCmpDate, "", normalText, 0, 25));
      generateNBestHead(par, li, 25+lh);

      li.addSubHead(oPrintPost(lClassName, "", boldText, 0, 14));
      li.addSubHead(oPrintPost(lClassResultFraction, "", boldText, 260, 14));
      
      li.addSubHead(oPrintPost(lString, lang.tl("Tid"), boldText, 460, 14));
      li.addSubHead(oPrintPost(lString, lang.tl("Totalt"), boldText, 510, 14));
      li.addSubHead(oPrintPost(lString, lang.tl("Efter"), boldText, 560, 14));
      
      li.addListPost(oPrintPost(lTeamPlace, "", normalText, 0, 0, ln));
      li.addListPost(oPrintPost(lRunnerName, "", normalText, 40, 0, ln));
      li.addListPost(oPrintPost(lRunnerClub, "", normalText, 260, 0, ln));
      
      li.addListPost(oPrintPost(lRunnerTimeStatus, "", normalText, 460, 0, ln));
      li.addListPost(oPrintPost(lTeamTimeStatus, "", normalText, 510, 0, ln));
      li.addListPost(oPrintPost(lTeamTimeAfter, "", normalText, 560, 0, ln));
      
      if (li.lp.splitAnalysis)  {
        li.addListPost(oPrintPost(lRunnerMissedTime, "", normalText, 620, 0, ln));
        li.addSubHead(oPrintPost(lString, lang.tl("Bomtid"), boldText, 620, 14));
      }

      li.calcResults=true;
      li.listType=li.EBaseTypeTeam;
      li.sortOrder=ClassResult;
      li.setFilter(EFilterHasResult); 
      break;

    case EStdIndMultiResultListLegLARGE:
      if (li.lp.legNumber==-1)
        throw std::exception("Ogiltigt val av str�cka");

      ln=li.lp.legNumber;

      pos.add("place", 25);
      pos.add("name", li.getMaxCharWidth(this, lRunnerName, "", normalText));
      pos.add("club", li.getMaxCharWidth(this, lRunnerClub, "", normalText));
      
      pos.add("status", 50);
      pos.add("teamstatus", 50);
      pos.add("after", 50);

      ttl = "%s - " + lang.tl("Lopp ") + itos(ln+1);
      li.addSubHead(oPrintPost(lClassName, MakeDash(ttl), boldLarge, pos.get("place", scale), 14));
      li.addSubHead(oPrintPost(lClassResultFraction, "", boldLarge, pos.get("club", scale), 14));
      
      li.addSubHead(oPrintPost(lString, lang.tl("Tid"), boldLarge, pos.get("status", scale), 14));
      li.addSubHead(oPrintPost(lString, lang.tl("Totalt"), boldLarge, pos.get("teamstatus", scale), 14));
      li.addSubHead(oPrintPost(lString, lang.tl("Efter"), boldLarge, pos.get("after", scale), 14));
      
      li.addListPost(oPrintPost(lTeamPlace, "", fontLarge, pos.get("place", scale), 0, ln));
      li.addListPost(oPrintPost(lRunnerName, "", fontLarge, pos.get("name", scale), 0, ln));
      li.addListPost(oPrintPost(lRunnerClub, "", fontLarge, pos.get("club", scale), 0, ln));
      
      li.addListPost(oPrintPost(lRunnerTimeStatus, "", fontLarge, pos.get("status", scale), 0, ln));
      li.addListPost(oPrintPost(lTeamTimeStatus, "", fontLarge, pos.get("teamstatus", scale), 0, ln));
      li.addListPost(oPrintPost(lTeamTimeAfter, "", fontLarge, pos.get("after", scale), 0, ln));
      
      li.calcResults=true;
      li.listType=li.EBaseTypeTeam;
      li.sortOrder=ClassResult;
      li.setFilter(EFilterHasResult); 
      break;

    case EStdIndMultiResultListAll:
      li.addHead(oPrintPost(lCmpName, MakeDash(lang.tl("Resultat - %s")), boldLarge, 0,0));
      li.addHead(oPrintPost(lCmpDate, "", normalText, 0, 25));
      generateNBestHead(par, li, 25+lh);

      li.addSubHead(oPrintPost(lClassName, "", boldText, 0, 14));
      li.addSubHead(oPrintPost(lClassResultFraction, "", boldText, 280, 14));
      li.addSubHead(oPrintPost(lClassResultFraction, lang.tl("Tid"), boldText, 480, 14));
      li.addSubHead(oPrintPost(lClassResultFraction, lang.tl("Efter"), boldText, 540, 14));

      //Use last leg for every team (index=-1)
      li.addListPost(oPrintPost(lTeamPlace, "", normalText, 0, 5, -1));
      li.addListPost(oPrintPost(lRunnerName, "", normalText, 25, 5, -1));
      li.addListPost(oPrintPost(lRunnerClub, "", normalText, 280, 5, -1));
      
      li.addListPost(oPrintPost(lTeamTimeStatus, "", normalText, 480, 5, -1));
      li.addListPost(oPrintPost(lTeamTimeAfter, "", normalText, 540, 5, -1));
      
      li.addSubListPost(oPrintPost(lSubSubCounter, lang.tl("Lopp %s"), normalText, 25, 0, 0));
      li.addSubListPost(oPrintPost(lRunnerTimeStatus, "", normalText, 90, 0, 0));
      li.addSubListPost(oPrintPost(lTeamLegTimeStatus, "", normalText, 150, 0, 0));
      li.addSubListPost(oPrintPost(lTeamLegTimeAfter, "", normalText, 210, 0, 0));

      li.lp.legNumber=-1;
      li.calcResults=true;
      li.listType=li.EBaseTypeTeam;
      li.listSubType=li.EBaseTypeRunner;
      li.sortOrder=ClassResult;
      li.setFilter(EFilterHasResult); 
      break;
    case EStdPatrolStartList:
    {
      MetaList mList;
      mList.setListName("Startlista, patrull");

      mList.addToHead(lCmpName).setText("Startlista - X").align(false);
      mList.newHead();
      mList.addToHead(lCmpDate).align(false);

      mList.addToSubHead(lClassName);
      mList.addToSubHead(lClassStartTime);
      mList.addToSubHead(lClassLength).setText("X meter");
      mList.addToSubHead(lClassStartName);

      mList.addToList(lTeamBib);
      mList.addToList(lTeamStart).setLeg(0);
      mList.addToList(lTeamName);
    
      mList.newListRow();

      mList.addToList(MetaListPost(lTeamRunner, lTeamName, 0));
      mList.addToList(MetaListPost(lTeamRunnerCard, lAlignNext, 0));
      mList.addToList(MetaListPost(lTeamRunner, lAlignNext, 1));
      mList.addToList(MetaListPost(lTeamRunnerCard, lAlignNext, 1));

      mList.setListType(li.EBaseTypeTeam);
      mList.setSortOrder(ClassStartTime);
      mList.addFilter(EFilterExcludeDNS);
      mList.save("foo.xml");

/*      xmlparser xfoo, xbar;
      xfoo.openMemoryOutput(true);

      mList.save(xfoo);

      string res;
      xfoo.getMemoryOutput(res);
      xbar.readMemory(res, 0);

      MetaList mList2;
      mList2.load(xbar.getObject("MeOSListDefinition"));
*/
      mList.interpret(this, gdibase, par, lh, li);
      break;
    }
    case EStdPatrolResultList:
      li.addHead(oPrintPost(lCmpName, MakeDash(lang.tl("Resultatlista - %s")), boldLarge, 0,0));
      li.addHead(oPrintPost(lCmpDate, "", normalText, 0, 25));
      generateNBestHead(par, li, 25+lh);
      
      li.addListPost(oPrintPost(lTeamPlace, "", normalText, 0, vspace, 1));
      li.addListPost(oPrintPost(lTeamName, "", normalText, 70, vspace));
      //li.addListPost(oPrintPost(lTeamClub, "", normalText, 250, vspace));
      li.addListPost(oPrintPost(lTeamTimeStatus, "", normalText, 400, vspace, 1));
      li.addListPost(oPrintPost(lTeamTimeAfter, "", normalText, 460, vspace, 1));
      
      li.addListPost(oPrintPost(lTeamRunner, "", normalText, 70, lh+vspace, 0));
      li.addListPost(oPrintPost(lTeamRunner, "", normalText, 250, lh+vspace, 1));
      li.setFilter(EFilterHasResult); 

      li.addSubHead(oPrintPost(lClassName, "", boldText, 0, 10));
      li.addSubHead(oPrintPost(lString, lang.tl("Tid"), boldText, 400, 10));
      li.addSubHead(oPrintPost(lString, lang.tl("Efter"), boldText, 460, 10));

      if(li.lp.showInterTimes) {
        li.addSubListPost(oPrintPost(lPunchNamedTime, "", normalText, 10, 0, 1));
        li.subListPost.back().fixedWidth=160;
        li.listSubType=li.EBaseTypePunches;
      }

      if (li.lp.splitAnalysis)  {
        li.addListPost(oPrintPost(lRunnerMissedTime, "", normalText, 520, vspace, 0));
        li.addSubHead(oPrintPost(lString, lang.tl("Bomtid"), boldText, 520, 10));
      }
      
      li.listType=li.EBaseTypeTeam;
      li.sortOrder=ClassResult;
      li.lp.legNumber = -1;
      li.calcResults=true;
      break;

    case EStdPatrolResultListLARGE:
      pos.add("place", 25);
      pos.add("team", int(0.7*max(li.getMaxCharWidth(this, lTeamName, "", normalText), 
                        li.getMaxCharWidth(this, lPatrolNameNames, "", normalText))));
      
      pos.add("status", 50);
      pos.add("after", 50);
      pos.add("missed", 50);

      li.addListPost(oPrintPost(lTeamPlace, "", fontLarge, pos.get("place", scale), vspace, 1));
      li.addListPost(oPrintPost(lTeamName, "", fontLarge, pos.get("team", scale), vspace));
      li.addListPost(oPrintPost(lTeamTimeStatus, "", fontLarge, pos.get("status", scale), vspace, 1));
      li.addListPost(oPrintPost(lTeamTimeAfter, "", fontLarge, pos.get("after", scale), vspace, 1));
      
      li.addListPost(oPrintPost(lPatrolNameNames, "", fontLarge, pos.get("team", scale), 25+vspace, 0));
      //li.addListPost(oPrintPost(lTeamRunner, "", fontLarge, pos.get("status", scale), 25+vspace, 1));

      li.addSubHead(oPrintPost(lClassName, "", boldLarge, 0, 10));
      li.addSubHead(oPrintPost(lString, lang.tl("Tid"), boldLarge, pos.get("status", scale), 10));
      li.addSubHead(oPrintPost(lString, lang.tl("Efter"), boldLarge, pos.get("after", scale), 10));

      if(li.lp.showInterTimes) {
        li.addSubListPost(oPrintPost(lPunchNamedTime, "", normalText, 10, 0, 1));
        li.subListPost.back().fixedWidth=200;
        li.listSubType=li.EBaseTypePunches;
      }

      if (li.lp.splitAnalysis)  {
        li.addListPost(oPrintPost(lRunnerMissedTime, "", fontLarge, pos.get("missed", scale), vspace, 0));
        li.addSubHead(oPrintPost(lString, lang.tl("Bomtid"), boldLarge, pos.get("missed", scale), 10));
      }

      li.setFilter(EFilterHasResult); 
      li.listType=li.EBaseTypeTeam;
      li.sortOrder=ClassResult;
      li.lp.legNumber = -1;
      li.calcResults=true;
      break;

    case EStdRaidResultListLARGE:
      li.addListPost(oPrintPost(lTeamPlace, "", fontLarge, 0, vspace));
      li.addListPost(oPrintPost(lTeamName, "", fontLarge, 40, vspace));
      li.addListPost(oPrintPost(lTeamTimeStatus, "", fontLarge, 490, vspace));
      
      li.addListPost(oPrintPost(lTeamRunner, "", fontLarge, 40, 25+vspace, 0));
      li.addListPost(oPrintPost(lTeamRunner, "", fontLarge, 300, 25+vspace, 1));
      
      li.addSubListPost(oPrintPost(lPunchNamedTime, "", fontMedium, 0, 2, 1));
      li.subListPost.back().fixedWidth=200;
      li.setFilter(EFilterHasResult); 
 
      li.addSubHead(oPrintPost(lClassName, "", boldLarge, 0, 10));

      li.listType=li.EBaseTypeTeam;
      li.listSubType=li.EBaseTypePunches;
      li.sortOrder=ClassResult;
      li.calcResults=true;
      break;

   case EStdResultListLARGE:
      pos.add("place", 25);
      pos.add("name", li.getMaxCharWidth(this, lPatrolNameNames, "", normalText, 0, true));
      pos.add("club", li.getMaxCharWidth(this, lPatrolClubNameNames, "", normalText, 0, true));
      pos.add("status", 50);
      pos.add("missed", 50);

      li.addListPost(oPrintPost(lRunnerPlace, "", fontLarge, pos.get("place", scale), vspace));
      li.addListPost(oPrintPost(lPatrolNameNames, "", fontLarge, pos.get("name", scale), vspace));
      li.addListPost(oPrintPost(lPatrolClubNameNames, "", fontLarge, pos.get("club", scale), vspace));

      li.addSubHead(oPrintPost(lClassName, "", boldLarge, pos.get("place", scale), 10));
  
      if (li.lp.useControlIdResultTo==0 && li.lp.useControlIdResultFrom==0) {
        li.addSubHead(oPrintPost(lClassResultFraction, "", boldLarge, pos.get("club", scale), 10));
        li.addListPost(oPrintPost(lRunnerTimeStatus, "", fontLarge, pos.get("status", scale), vspace));
      }
      else {
        li.needPunches = true;
        li.addListPost(oPrintPost(lRunnerTempTimeStatus, "", normalText, pos.get("status", scale), vspace));
      }
      if (li.lp.splitAnalysis) {
        li.addSubHead(oPrintPost(lString, lang.tl("Bomtid"), boldLarge, pos.get("missed", scale), 10));
        li.addListPost(oPrintPost(lRunnerMissedTime, "", fontLarge, pos.get("missed", scale), vspace));
      }

      if(li.lp.showInterTimes) {
        li.addSubListPost(oPrintPost(lPunchNamedTime, "", normalText, 0, 0, 1));
        li.subListPost.back().fixedWidth = 160;
        li.listSubType = li.EBaseTypePunches;
      }
      else if (li.lp.showSplitTimes) {
        li.addSubListPost(oPrintPost(lPunchTime, "", normalText, 0, 0, 1));
        li.subListPost.back().fixedWidth = 95;
        li.listSubType = li.EBaseTypePunches;
      }
   
      li.setFilter(EFilterHasResult); 
      li.lp.legNumber = 0;
      li.listType=li.EBaseTypeRunner;
      li.sortOrder=ClassResult;
      li.calcResults=true;

      /*
            char title[256];
      if (li.lp.useControlIdResult==0)
        sprintf_s(title, "%s", lang.tl("Resultat - %s").c_str());
      else {
        pControl ctrl=getControl(li.lp.useControlIdResult, false);
        if(ctrl && !ctrl->Name.empty())
          sprintf_s(title, "%s, %s", lang.tl("Resultat - %s").c_str(), ctrl->Name.c_str());
        else if(ctrl)
          sprintf_s(title, lang.tl("%s, vid kontroll %d").c_str(), 
          lang.tl("Resultat - %s").c_str(), ctrl->Numbers[0]);
        else
          sprintf_s(title, lang.tl("Resultat vid ok�nd kontroll").c_str());
      }
      li.addHead(oPrintPost(lCmpName, MakeDash(title), boldLarge, 0,0));
      li.addHead(oPrintPost(lCmpDate, "", normalText, 0, 25));

      pos.add("place", 40);
      pos.add("name", li.getMaxCharWidth(this, lRunnerName, "", par.selection));
      pos.add("club", li.getMaxCharWidth(this, lRunnerClub, "", par.selection));
      pos.add("status", 50);
      pos.add("after", 50);
      
      li.addSubHead(oPrintPost(lClassName, "", boldText, 0, 10));

      li.addListPost(oPrintPost(lRunnerPlace, "", normalText, pos.get("place"), 0));
      li.addListPost(oPrintPost(lRunnerName, "", normalText, pos.get("name"), 0));
      li.addListPost(oPrintPost(lRunnerClub, "", normalText, pos.get("club"), 0));

      if (li.lp.useControlIdResult==0) {
        li.addSubHead(oPrintPost(lClassResultFraction, "", boldText, pos.get("club"), 10));
 
        li.addListPost(oPrintPost(lRunnerTimeStatus, "", normalText, pos.get("status"), 0));
        li.addListPost(oPrintPost(lRunnerTimeAfter, "", normalText, pos.get("after"), 0));
        if(li.lp.showInterTimes) {
          li.addSubListPost(oPrintPost(lPunchNamedTime, "", normalText, 10, 0, 1));
          li.subListPost.back().fixedWidth = 160;
          li.listSubType = li.EBaseTypePunches;
        }
        else if (li.lp.showSplitTimes) {
          li.addSubListPost(oPrintPost(lPunchTime, "", fontSmall, 10, 0, 1));
          li.subListPost.back().fixedWidth = 95;
          li.listSubType = li.EBaseTypePunches;
        }
      }
      else {
        li.addListPost(oPrintPost(lRunnerTempTimeStatus, "", normalText, pos.get("status"), 0));
        li.addListPost(oPrintPost(lRunnerTempTimeAfter, "", normalText, pos.get("after"), 0));
      }

      li.addSubHead(oPrintPost(lString, lang.tl("Tid"), boldText, pos.get("status"), 10));
      li.addSubHead(oPrintPost(lString, lang.tl("Efter"), boldText, pos.get("after"), 10));
     
      li.calcResults = true;
      li.listType=li.EBaseTypeRunner;
      li.sortOrder=ClassResult;
      li.setFilter(EFilterHasResult);
      break;

      */
      break;

   case EStdUM_Master:
      li.addListPost(oPrintPost(lRunnerPlace, "", fontMedium, 0, 0));
      li.addListPost(oPrintPost(lRunnerName, "", fontMedium, 40, 0));
      li.addListPost(oPrintPost(lRunnerClub, "", fontMedium, 250, 0));
      li.addListPost(oPrintPost(lClassName, "", fontMedium, 490, 0));
      li.addListPost(oPrintPost(lRunnerUMMasterPoint, "", fontMedium, 580, 0));
            
      li.setFilter(EFilterHasResult); 
    
      li.lp.legNumber = 0;
      li.listType=li.EBaseTypeRunner;
      li.sortOrder=ClassResult;
      li.calcResults=true;
      break;

   case ERogainingInd:
      pos.add("place", 25);
      pos.add("name", li.getMaxCharWidth(this, lRunnerCompleteName, "", normalText, 0, true));
      pos.add("points", 50);
      pos.add("status", 50);
      li.addHead(oPrintPost(lCmpName, MakeDash(par.getCustomTitle(lang.tl("Rogainingresultat - %s"))), boldLarge, 0,0));
      li.addHead(oPrintPost(lCmpDate, "", normalText, 0, 25));
      generateNBestHead(par, li, 25+lh);
      
      li.addListPost(oPrintPost(lRunnerPlace, "", normalText, pos.get("place"), vspace));
      li.addListPost(oPrintPost(lRunnerCompleteName, "", normalText, pos.get("name"), vspace));
      li.addListPost(oPrintPost(lRunnerRogainingPoint, "", normalText, pos.get("points"), vspace));
      li.addListPost(oPrintPost(lRunnerTimeStatus, "", normalText, pos.get("status"), vspace));
      
      li.setFilter(EFilterHasResult); 

      if(li.lp.splitAnalysis || li.lp.showInterTimes) {
        li.addSubListPost(oPrintPost(lRogainingPunch, "", normalText, 10, 0, 1));
        li.subListPost.back().fixedWidth=130;
        li.listSubType=li.EBaseTypePunches;
      }

      li.addSubHead(oPrintPost(lClassName, "", boldText, pos.get("place"), 10));
      li.addSubHead(oPrintPost(lString, lang.tl("Po�ng"), boldText, pos.get("points"), 10));
      li.addSubHead(oPrintPost(lString, lang.tl("Tid"), boldText, pos.get("status"), 10));

      li.listType=li.EBaseTypeRunner;
      li.sortOrder = ClassPoints;
      li.lp.legNumber = -1;
      li.calcResults = true;
      li.rogainingResults = true;
    break;


   case ECourseRogainingInd:
      pos.add("place", 45);
      pos.add("name", li.getMaxCharWidth(this, lRunnerCompleteName, "", fontLarge, 0, true));
      pos.add("class", 80);
      pos.add("status", 80);
      pos.add("penalty", 80);
      pos.add("points", 80);

      li.addHead(oPrintPost(lCmpName, MakeDash(par.getCustomTitle(lang.tl("Rogainingresultat - %s"))), boldLarge, 0,0));
      li.addHead(oPrintPost(lCmpDate, "", fontLarge, 0, 25));
      generateNBestHead(par, li, 25+lh);
      li.largeSize = true;
      
      li.addListPost(oPrintPost(lRunnerPlace, "", fontLarge, pos.get("place"), vspace));
      li.addListPost(oPrintPost(lRunnerCompleteName, "", fontLarge, pos.get("name"), vspace));
      li.addListPost(oPrintPost(lClassName, "", fontLarge, pos.get("class"), vspace));
      li.addListPost(oPrintPost(lRunnerTimeStatus, "", fontLarge, pos.get("status"), vspace));
      li.addListPost(oPrintPost(lRunnerPenaltyPoint, "", fontLarge, pos.get("penalty"), vspace));
      li.addListPost(oPrintPost(lRunnerRogainingPoint, "", fontLarge, pos.get("points"), vspace));

      
      li.setFilter(EFilterHasResult); 

      if(li.lp.splitAnalysis || li.lp.showInterTimes) {
        li.addSubListPost(oPrintPost(lRogainingPunch, "", fontLarge, 10, 0, 1));
        li.subListPost.back().fixedWidth=130;
        li.listSubType=li.EBaseTypePunches;
      }

      li.addSubHead(oPrintPost(lCourseName, "", boldLarge, pos.get("place"), 10));
      li.addSubHead(oPrintPost(lString, lang.tl("Tid"), boldLarge, pos.get("status"), 10));
      li.addSubHead(oPrintPost(lString, lang.tl("Penalty"), boldLarge, pos.get("penalty"), 10));
      li.addSubHead(oPrintPost(lString, lang.tl("Po�ng"), boldLarge, pos.get("points"), 10));


      li.listType=li.EBaseTypeRunner;
      li.sortOrder = CoursePoints;
      li.lp.legNumber = -1;
      li.calcResults = true;
      li.rogainingResults = true;
    break;


    case EFixedPreReport:
    case EFixedReport:
    case EFixedInForest:
    case EFixedEconomy:
    case EFixedInvoices:
    case EFixedResultFinishPerClass:
    case EFixedResultFinish:
    case EFixedMinuteStartlist:
    case EFixedTimeLine:
      li.fixedType = true;
    break;

    default:
      if (!getListContainer().interpret(this, gdibase, par, lineHeight, li))
        throw std::exception("Not implemented");
  }  
}

string oPrintPost::encodeFont(const string &face, int factor) {
  string out(face);
  if (factor > 0 && factor != 100) {
    out += ";" + itos(factor/100) + "." + itos(factor%100);
  }
  return out;    
}
