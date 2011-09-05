#pragma once
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

#include <set>
#include <vector>
#include "oBase.h"

typedef oEvent *pEvent;

enum EPostType
{
  lAlignNext,
  lNone,
  lString,
  lCmpName,
  lCmpDate,
  lCurrentTime,
  lClubName,
  lClassName,
  lClassStartName,
  lClassStart,
  lClassLength,
  lClassResultFraction,
  lRunnerName,  
  lRunnerCompleteName,  
  lPatrolNameNames,
  lRunnerFinish,
  lRunnerTime,
  lRunnerTimeStatus,
  lRunnerTempTimeStatus,
  lRunnerTimeAfter,
  lRunnerMissedTime,
  lRunnerTempTimeAfter,
  lRunnerPlace,
  lRunnerStart,
  lRunnerClub,
  lRunnerCard,
  lRunnerBib,
  lRunnerStartNo,
  lRunnerRank,
  lRunnerCourse,
  lRunnerRogainingPoint,
  lRunnerUMMasterPoint,
  lRunnerTimePlaceFixed,
  lTeamName,
  lTeamStart,
  lTeamTimeStatus,
  lTeamTimeAfter,
  lTeamPlace,
  lTeamLegTimeStatus,
  lTeamLegTimeAfter,
  lTeamRogainingPoint,
  lTeamTime,
  lTeamStatus,
  lTeamClub,
  lTeamRunner,
  lTeamRunnerCard,
  lTeamBib,
  lTeamStartNo,
  lPunchNamedTime,
  lPunchTime,
  lRogainingPunch,
  lTotalCounter,
  lSubCounter,
  lSubSubCounter,
};

enum EStdListType
{
  EStdNone=-1,
  EStdStartList=1,
  EStdResultList,
  EGeneralResultList,
  ERogainingInd,
  EStdTeamResultListAll,
  EStdTeamResultListLeg,
  EStdTeamResultList,
  EStdTeamStartList,
  EStdTeamStartListLeg,
  EStdIndMultiStartListLeg,
  EStdIndMultiResultListLeg,
  EStdIndMultiResultListAll,
  EStdPatrolStartList,
  EStdPatrolResultList,
  EStdRentedCard,
  EStdResultListLARGE,
  EStdTeamResultListLegLARGE,
  EStdPatrolResultListLARGE,
  EStdIndMultiResultListLegLARGE,
  EStdRaidResultListLARGE, //Obsolete
  ETeamCourseList,
  EIndCourseList,
  EStdClubStartList,
  EStdClubResultList,

  EIndPriceList,
  EStdUM_Master,

  EFixedPreReport,
  EFixedReport,
  EFixedInForest,
  EFixedInvoices,
  EFixedEconomy,
  EFixedResultFinishPerClass,
  EFixedResultFinish,
  EFixedMinuteStartlist,
};

enum EFilterList
{
  EFilterHasResult,
  EFilterRentCard,
  EFilterHasCard,
  EFilterExcludeDNS,
  EFilterVacant,
  EFilterHasNoCard,
  _EFilterMax=EFilterHasNoCard
};

struct oPrintPost
{
  oPrintPost();
  oPrintPost(EPostType type_, const string &format_, 
                       int style_, int dx_, int dy_, int index_=0);

  EPostType type;
  string text;
  int format;  
  int dx;
  int dy;
  int index;

  int fixedWidth;
};

class gdioutput;
typedef int (*GUICALLBACK)(gdioutput *gdi, int type, void *data);

struct oListParam {
  oListParam();
  EStdListType listCode;
  GUICALLBACK cb;
  set<int> selection;
  int legNumber;
  int useControlIdResultTo;
  int useControlIdResultFrom;
  int filterMaxPer;
  bool pageBreak;
  bool showInterTimes;
  bool showSplitTimes;
  bool splitAnalysis;
  string title; 
  // Generate a large-size list (supported as input when supportLarge is true)
  bool useLargeSize;

  void setCustomTitle(const string &t) {title = t;}
  void getCustomTitle(char *t) const; // 256 size buffer required. Get title if set
  const string &getCustomTitle(const string &t) const; 


};

class oListInfo {
public:
  enum EBaseType {EBaseTypeRunner, EBaseTypeTeam, 
                EBaseTypeClub, EBaseTypePunches, EBaseTypeNone};
 
  const string &getName() const {return Name;}
protected:
  string Name;
  EBaseType listType;
  EBaseType listSubType;
  SortOrder sortOrder;
  bool calcResults;
  bool rogainingResults;
  oListParam lp;

  list<oPrintPost> Head;
  list<oPrintPost> subHead;
  list<oPrintPost> listPost;
  vector<char> listPostFilter;
  list<oPrintPost> subListPost;
  bool fixedType;
  bool needPunches;

  oListInfo *next;
public:

  void addList(const oListInfo &lst);

  bool supportClasses;
  bool supportLegs;
  bool supportExtra;
  // True if large (and non-large) is supported
  bool supportLarge; 
  // True if a large-size list only
  bool largeSize;

  bool needPunchCheck() const {return needPunches;}
  void setCallback(GUICALLBACK cb) {lp.cb=cb;}
  int getLegNumber() const {return lp.legNumber;}
  EStdListType getListCode() const {return lp.listCode;}
  void addHead(const oPrintPost &pp) {Head.push_back(pp);}
  void addSubHead(const oPrintPost &pp) {subHead.push_back(pp);}
  void addListPost(const oPrintPost &pp) {listPost.push_back(pp);}
  void addSubListPost(const oPrintPost &pp) {subListPost.push_back(pp);}
  inline bool filter(EFilterList i) const {return listPostFilter[i]!=0;}
  void setFilter(EFilterList i) {listPostFilter[i]=1;}

  oListInfo(void);
  ~oListInfo(void);

  friend class oEvent;

  int getMaxCharWidth(const oEvent *oe, EPostType type, 
                      const string &format, bool large = false,
                      int minSize = 0);
};
