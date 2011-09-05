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

// oClass.cpp: implementation of the oClass class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#define DODECLARETYPESYMBOLS

#include <cassert>
#include "oClass.h"
#include "oEvent.h"
#include "Table.h"
#include "meos_util.h"
#include <limits>
#include "Localizer.h"
#include <algorithm>
#include "inthashmap.h"
#include "intkeymapimpl.hpp"

#include "gdioutput.h"
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

oClass::oClass(oEvent *poe): oBase(poe)
{
	getDI().initData();
	Course=0;
	Id=oe->getFreeClassId();
  tLeaderTime.resize(1);
  tNoTiming = -1;
  tLegTimeToPlace = 0;
  tLegAccTimeToPlace = 0;
  tSplitRevision = 0;
}

oClass::oClass(oEvent *poe, int id): oBase(poe)
{
	getDI().initData();
	Course=0;
  if (id == 0)
    id = oe->getFreeClassId();
	Id=id;
  oe->qFreeClassId = max(id, oe->qFreeClassId);
  tLeaderTime.resize(1);
  tNoTiming = -1;
  tLegTimeToPlace = 0;
  tLegAccTimeToPlace = 0;
  tSplitRevision = 0;
}

oClass::~oClass()
{
  if (tLegTimeToPlace)
    delete tLegTimeToPlace;
  if (tLegAccTimeToPlace)
    delete tLegAccTimeToPlace;
}

bool oClass::Write(xmlparser &xml)
{
	if(Removed) return true;

	xml.startTag("Class");

	xml.write("Id", Id);
	xml.write("Updated", Modified.GetStamp());
	xml.write("Name", Name);
	
	if(Course)
		xml.write("Course", Course->Id);

	if(MultiCourse.size()>0)
		xml.write("MultiCourse", codeMultiCourse());

  if(legInfo.size()>0)
    xml.write("LegMethod", codeLegMethod());

	getDI().write(xml);
	xml.endTag();

	return true;
}


void oClass::Set(const xmlobject *xo)
{
	xmlList xl;
  xo->getObjects(xl);

	xmlList::const_iterator it;

	for(it=xl.begin(); it != xl.end(); ++it){
		if(it->is("Id")){
			Id = it->getInt();			
		}
		else if(it->is("Name")){
			Name = it->get();
		}
		else if(it->is("Course")){
			Course = oe->getCourse(it->getInt());
		}
		else if(it->is("MultiCourse")){
      set<int> cid;
      vector< vector<int> > multi;
      parseCourses(it->get(), multi, cid);
			importCourses(multi);
		}
    else if(it->is("LegMethod")){
      importLegMethod(it->get());;
		}
		else if(it->is("oData")){
			getDI().set(*it);
		}	
		else if(it->is("Updated")){
			Modified.SetStamp(it->get());
		}
	}

  // Reinit temporary data
  getNoTiming();
}

string oClass::codeMultiCourse() const
{
	vector< vector<pCourse> >::const_iterator stage_it;
	string str;
	char bf[16];

	for (stage_it=MultiCourse.begin();stage_it!=MultiCourse.end(); ++stage_it) {
		vector<pCourse>::const_iterator it;
		for (it=stage_it->begin();it!=stage_it->end(); ++it) {
			if(*it){
				sprintf_s(bf, 16, " %d", (*it)->getId());
				str+=bf;
			}
			else str+=" 0";				
		}
		str+=";";
	}

  if (str.length()>0) {
    return trim(str.substr(0, str.length()-1));
  }
	else return "";
}


void oClass::importCourses(const vector< vector<int> > &multi)
{
  MultiCourse.resize(multi.size());

  for (size_t k=0;k<multi.size();k++) {
    MultiCourse[k].resize(multi[k].size());
    for (size_t j=0; j<multi[k].size(); j++) {
      MultiCourse[k][j] = oe->getCourse(multi[k][j]);
    }
  }
  setNumStages(MultiCourse.size());
}

set<int> &oClass::getMCourseIdSet(set<int> &in) const
{
  in.clear();
  for (size_t k=0;k<MultiCourse.size();k++) {    
    for (size_t j=0; j<MultiCourse[k].size(); j++) {
      if(MultiCourse[k][j])
        in.insert(MultiCourse[k][j]->getId());
    }
  }
  return in;
}

void oClass::parseCourses(const string &courses, 
                          vector< vector<int> > &multi, 
                          set<int> &courseId)
{
  courseId.clear();
  multi.clear();
  if (courses.empty()) 
    return;

	const char *str=courses.c_str();

	vector<int> empty;
	multi.push_back(empty);
	int n_stage=0;

  while (*str && isspace(*str)) 
    str++;
	
	while(*str){
		int cid=atoi(str);

    if (cid) {
			multi[n_stage].push_back(cid);
      courseId.insert(cid);
    }

		while (*str && (*str!=';' && *str!=' ')) str++;
	
		if (*str==';') {
			str++;
			while (*str && *str==' ') str++;	
			n_stage++;
			multi.push_back(empty);
		}		
		else {
			if(*str) str++;
		}
	}
}

string oLegInfo::codeLegMethod() const
{
  char bf[256];
  sprintf_s(bf, "(%s:%s:%d:%d:%d:%d)", StartTypeNames[startMethod],
                             LegTypeNames[legMethod],
                             legStartData, legRestartTime,
                             legRopeTime, duplicateRunner);
  return bf;
}

void oLegInfo::importLegMethod(const string &leg)
{
  //Defaults
  startMethod=STTime;
  legMethod=LTNormal;
  legStartData = 0;
  legRestartTime = 0;

  size_t begin=leg.find_first_of('(');

  if(begin==leg.npos)
    return;
  begin++;

  string coreLeg=leg.substr(begin, leg.find_first_of(')')-begin);

  vector< string > legsplit;
  split(coreLeg, ":", legsplit);

  if(legsplit.size()>=1)  
    for( int st = 0 ; st < nStartTypes ; ++st )
     if( legsplit[0]==StartTypeNames[st] )
       startMethod=(StartTypes)st;

  if(legsplit.size()>=2)  
    for( int t = 0 ; t < nLegTypes ; ++t )
     if( legsplit[1]==LegTypeNames[t] )
       legMethod=(LegTypes)t;

  if(legsplit.size()>=3)
    legStartData = atoi(legsplit[2].c_str());

  if(legsplit.size()>=4)
    legRestartTime = atoi(legsplit[3].c_str());

  if(legsplit.size()>=5)
    legRopeTime = atoi(legsplit[4].c_str());

  if(legsplit.size()>=6)
    duplicateRunner = atoi(legsplit[5].c_str());
}

string oClass::codeLegMethod() const
{
  string code;
  for(size_t k=0;k<legInfo.size();k++) {
    if(k>0) code+="*";
    code+=legInfo[k].codeLegMethod();
  }
  return code;
}

string oClass::getInfo() const
{
  return "Klass " + Name;
}

void oClass::importLegMethod(const string &legMethods)
{
  vector< string > legsplit;
  split(legMethods, "*", legsplit);

  legInfo.clear();
  for (size_t k=0;k<legsplit.size();k++) {
    oLegInfo oli;
    oli.importLegMethod(legsplit[k]);
    legInfo.push_back(oli);
  }

  // Ensure we got valid data
  for (size_t k=0;k<legsplit.size();k++) {
    if(legInfo[k].duplicateRunner!=-1) {
      if( unsigned(legInfo[k].duplicateRunner)<legInfo.size() )
        legInfo[legInfo[k].duplicateRunner].duplicateRunner=-1;
      else
        legInfo[k].duplicateRunner=-1;
    }
  }
  setNumStages(legInfo.size());
  apply();
}

int oClass::getNumRunners() const
{
	int nRunners=0;
	oRunnerList::iterator it;		

	for (it=oe->Runners.begin(); it != oe->Runners.end(); ++it)
		if(it->getClassId()==Id) nRunners++;

	return nRunners;
}


void oClass::setCourse(pCourse c)
{
	if(Course!=c){
		Course=c;
		updateChanged();
	}
  // Update start from course
  if (Course && !Course->getStart().empty()) {
    setStart(Course->getStart());
  }
}

void oClass::setName(const string &name)
{
	if(Name!=name){
		Name=name;	
		//Modified.Update();
		updateChanged();
	}
}

oDataInterface oClass::getDI(void)
{
	return oe->oClassData->getInterface(oData, sizeof(oData), this);
}

oDataConstInterface oClass::getDCI(void) const
{
	return oe->oClassData->getConstInterface(oData, sizeof(oData), this);
}

pClass oEvent::getClassCreate(int Id, const string &CreateName) 
{
	oClassList::iterator it;	
	
  if (Id>0) 
	  for (it=Classes.begin(); it != Classes.end(); ++it) {
      if (it->Id==Id) {

        if (compareClassName(CreateName, it->getName())) {
			    if(it!=Classes.begin())
				    Classes.splice(Classes.begin(), Classes, it, Classes.end());
    				
			    return &Classes.front();
        }
        else {
          Id=0; //Bad Id
          break;
        }
		  }
	  }

  if(CreateName.empty() && Id>0) {
    oClass c(this, Id);
    c.setName(getAutoClassName());
    return addClass(c);
	}
	else {
		//Check if class exist under different id
		for (it=Classes.begin(); it != Classes.end(); ++it) {
			if (compareClassName(it->Name, CreateName))
				return &*it;
		}

    if(Id<=0)
			Id=getFreeClassId();

    oClass c(this, Id);
    c.Name = CreateName;

    //No! Create class with this Id
		pClass pc=addClass(c);

		//Not found. Auto add...	
		return pc;
	}
}

pClass oEvent::getClass(const string &cname) const
{
  for (oClassList::const_iterator it=Classes.begin(); it != Classes.end(); ++it) {
    if (compareClassName(cname, it->Name) && !it->isRemoved())
			return pClass(&*it);
	}
  return 0;
}

pClass oEvent::getClass(int Id) const
{
	if(Id<=0) 
    return 0;

	oClassList::const_iterator it;	
	
  for (it=Classes.begin(); it != Classes.end(); ++it){
    if (it->Id==Id && !it->isRemoved()) {
		  return pClass(&*it);
	  }
  }
  return 0;
}

pClass oEvent::addClass(const string &pname, int CourseId, int classId)
{
  if (classId > 0){
    pClass pOld=getClass(classId);
    if (pOld)
      return 0;
  }

	oClass c(this, classId);
	c.Name=pname;

	if(CourseId>0)
		c.Course=getCourse(CourseId);

	Classes.push_back(c);
  Classes.back().synchronize();
  updateTabs();
	return &Classes.back();
}

pClass oEvent::addClass(oClass &c)
{
  if (c.Id==0)
    return 0;
  else {
    pClass pOld=getClass(c.getId());
    if (pOld)
      return 0;
  }

	Classes.push_back(c);	

  if (!Classes.back().existInDB()) {
    Classes.back().changed = true;
    Classes.back().synchronize();
  }
	return &Classes.back();
}

bool oClass::fillStages(gdioutput &gdi, const string &name) const
{	
	gdi.clearList(name);
	
	vector<vector<pCourse>>::const_iterator stage_it;
	string out;
	string str="";
	char bf[32];
	int m=0;

	for (stage_it=MultiCourse.begin();stage_it!=MultiCourse.end(); ++stage_it) {
    sprintf_s(bf, lang.tl("Sträcka %d").c_str(), m+1);
		gdi.addItem(name, bf, m++);
	}
	return true;
}

bool oClass::fillStageCourses(gdioutput &gdi, int stage, 
                              const string &name) const
{	
	if(unsigned(stage)>=MultiCourse.size())
		return false;

	gdi.clearList(name);	
	const vector<pCourse> &Stage=MultiCourse[stage];
	vector<pCourse>::const_iterator it;
	string out;
	string str="";
	char bf[128];
	int m=0;

	for (it=Stage.begin(); it!=Stage.end(); ++it) {
		sprintf_s(bf, "%d: %s", ++m, (*it)->getName().c_str());
		gdi.addItem(name, bf, (*it)->getId());
	}

	return true;
}

bool oClass::addStageCourse(int iStage, int CourseId)
{	
	if(unsigned(iStage)>=MultiCourse.size())
		return false;

	vector<pCourse> &Stage=MultiCourse[iStage];
	pCourse pc=oe->getCourse(CourseId);

  if(pc) {
		Stage.push_back(pc);
    updateChanged();
    return true;
  }
	return false;
}

bool oClass::removeStageCourse(int iStage, int CourseId, int position)
{
	if(unsigned(iStage)>=MultiCourse.size())
		return false;

	vector<pCourse> &Stage=MultiCourse[iStage];

  if( !(DWORD(position)<Stage.size()))
    return false;

	if(Stage[position]->getId()==CourseId){
		Stage.erase(Stage.begin()+position);
    updateChanged();
		return true;
	}

	return false;
}

void oClass::setNumStages(int no)
{
  if (no>=0) {
    if (MultiCourse.size() != no)
      updateChanged();
	  MultiCourse.resize(no);
    legInfo.resize(no);
    tLeaderTime.resize(max(no, 1));
  }
  oe->updateTabs();
}

void oClass::getTrueStages(vector< pair<int, int> > &stages) const
{
  stages.clear();
  if (!legInfo.empty()) {
    for (size_t k = 0; k+1 < legInfo.size(); k++) {
      if (legInfo[k].trueLeg != legInfo[k+1].trueLeg)
        stages.push_back(pair<int,int>(k, legInfo[k].trueLeg));
    }
    stages.push_back(pair<int, int>(legInfo.size()-1, legInfo.back().trueLeg));
  }
  else
    stages.push_back(make_pair(-1,1));
}

bool oClass::startdataIgnored(int i) const
{
  StartTypes st=getStartType(i);
  LegTypes lt=getLegType(i);

  if(lt==LTIgnore || lt==LTExtra || lt==LTParallel || lt == LTParallelOptional)
    return true;

  if(st==STChange || st==STDrawn)
    return true;

  return false;
}

bool oClass::restartIgnored(int i) const
{
  StartTypes st=getStartType(i);
  LegTypes lt=getLegType(i);

  if(lt==LTIgnore || lt==LTExtra || lt==LTParallel || lt == LTParallelOptional)
    return true;

  if(st==STTime || st==STDrawn)
    return true;

  return false;
}

void oClass::fillStartTypes(gdioutput &gdi, const string &name)
{
  gdi.clearList(name);

  gdi.addItem(name, lang.tl("Starttid"), STTime);
  gdi.addItem(name, lang.tl("Växling"), STChange);
  gdi.addItem(name, lang.tl("Lottad"), STDrawn);
  gdi.addItem(name, lang.tl("Jaktstart"), STHunting);
}

StartTypes oClass::getStartType(int leg) const
{
  if(unsigned(leg)<legInfo.size())
    return legInfo[leg].startMethod;
  else return STTime;
}

LegTypes oClass::getLegType(int leg) const
{
  if(unsigned(leg)<legInfo.size())
    return legInfo[leg].legMethod;
  else return LTNormal;
}

int oClass::getStartData(int leg) const
{
  if(unsigned(leg)<legInfo.size())
    return legInfo[leg].legStartData;
  else return 0;
}

int oClass::getRestartTime(int leg) const
{
  if(unsigned(leg)<legInfo.size())
    return legInfo[leg].legRestartTime;
  else return 0;
}


int oClass::getRopeTime(int leg) const
{
  if(unsigned(leg)<legInfo.size())
    return legInfo[leg].legRopeTime;
  else return 0;
}


string oClass::getStartDataS(int leg) const
{
  int s=getStartData(leg);
  StartTypes t=getStartType(leg);

  if(t==STTime || t==STHunting) {
    if(s>0)
      return oe->getAbsTime(s);
    else return "-";
  }
  else if(t==STChange || t==STDrawn)
    return "-";

  return "?";
}

string oClass::getRestartTimeS(int leg) const
{
  int s=getRestartTime(leg);
  StartTypes t=getStartType(leg);

  if(t==STChange || t==STHunting) {
    if(s>0)
      return oe->getAbsTime(s);
    else return "-";
  }
  else if(t==STTime || t==STDrawn)
    return "-";

  return "?";
}

string oClass::getRopeTimeS(int leg) const
{
  int s=getRopeTime(leg);
  StartTypes t=getStartType(leg);

  if(t==STChange || t==STHunting) {
    if(s>0)
      return oe->getAbsTime(s);
    else return "-";
  }
  else if(t==STTime || t==STDrawn)
    return "-";

  return "?";
}

int oClass::getLegRunner(int leg) const
{
  if (unsigned(leg)<legInfo.size()) 
    if(legInfo[leg].duplicateRunner==-1)
      return leg;
    else
      return legInfo[leg].duplicateRunner;

  return leg;
}

int oClass::getLegRunnerIndex(int leg) const
{
  if (unsigned(leg)<legInfo.size()) 
    if(legInfo[leg].duplicateRunner==-1)
      return 0;
    else {
      int base=legInfo[leg].duplicateRunner;
      int index=1;
      for (int k=base+1;k<leg;k++)
        if(legInfo[k].duplicateRunner==base)
          index++;
      return index;
    }

  return leg;
}


void oClass::setLegRunner(int leg, int runnerNo)
{
  bool changed=false;
  if (leg==runnerNo)
    runnerNo=-1; //Default
  else {
    if (runnerNo<leg) {
      setLegRunner(runnerNo, runnerNo);  
    }
    else {
      setLegRunner(runnerNo, leg);
      runnerNo=-1;
    }
  }

  if (unsigned(leg)<legInfo.size()) 
    changed=legInfo[leg].duplicateRunner!=runnerNo;      
  else if (leg>=0) {
    changed=true;
    legInfo.resize(leg+1);
  }

  legInfo[leg].duplicateRunner=runnerNo;
  
  if(changed)
    updateChanged();
}

void oClass::setStartType(int leg, StartTypes st)
{
  bool changed=false;

  if (unsigned(leg)<legInfo.size()) 
    changed=legInfo[leg].startMethod!=st;      
  else if (leg>=0) {
    changed=true;
    legInfo.resize(leg+1);
  }

  legInfo[leg].startMethod=st;
  
  if(changed)
    updateChanged();
}

void oClass::setLegType(int leg, LegTypes lt)
{
  bool changed=false;

  if (unsigned(leg)<legInfo.size())
    changed=legInfo[leg].legMethod!=lt;      
  else if(leg>=0) {
    changed=true;
    legInfo.resize(leg+1);    
  }

  legInfo[leg].legMethod=lt;

  if(changed) {
    apply();
    updateChanged();
  }
}

void oClass::setStartData(int leg, const string &s)
{
  int rt;
  StartTypes styp=getStartType(leg);
  if(styp==STTime || styp==STHunting)
    rt=oe->getRelativeTime(s);
  else
    rt=atoi(s.c_str());

  bool changed=false;

  if (unsigned(leg)<legInfo.size()) 
    changed=legInfo[leg].legStartData!=rt;
  else if(leg>=0) {
    changed=true;
    legInfo.resize(leg+1);    
  }
  legInfo[leg].legStartData=rt;

  if(changed)
    updateChanged();
}

void oClass::setRestartTime(int leg, const string &t)
{
  int rt=oe->getRelativeTime(t);
  bool changed=false;

  if (unsigned(leg)<legInfo.size()) 
    changed=legInfo[leg].legRestartTime!=rt;
  else if(leg>=0) {
    changed=true;
    legInfo.resize(leg+1);    
  }
  legInfo[leg].legRestartTime=rt;

  if(changed)
    updateChanged();
}

void oClass::setRopeTime(int leg, const string &t)
{
  int rt=oe->getRelativeTime(t);
  bool changed=false;

  if (unsigned(leg)<legInfo.size()) 
    changed=legInfo[leg].legRopeTime!=rt;
  else if(leg>=0) {
    changed=true;
    legInfo.resize(leg+1);    
  }
  legInfo[leg].legRopeTime=rt;

  if(changed)
    updateChanged();
}


void oClass::fillLegTypes(gdioutput &gdi, const string &name)
{
  gdi.clearList(name);

  gdi.addItem(name, lang.tl("Normal"), LTNormal);
  gdi.addItem(name, lang.tl("Parallell"), LTParallel);
  gdi.addItem(name, lang.tl("Valbar"), LTParallelOptional);
  gdi.addItem(name, lang.tl("Extra"), LTExtra);
  gdi.addItem(name, lang.tl("Summera"), LTSum);
  gdi.addItem(name, lang.tl("Medlöpare"), LTIgnore);
}

void oEvent::fillClasses(gdioutput &gdi, const string &id, ClassExtra extended, ClassFilter filter)
{
  vector< pair<string, size_t> > d;
  oe->fillClasses(d, extended, filter);
  gdi.addItem(id, d);
}
	
const vector< pair<string, size_t> > &oEvent::fillClasses(vector< pair<string, size_t> > &out, 
                                                          ClassExtra extended, ClassFilter filter)
{	
  set<int> undrawn;
  set<int> hasRunner;
  out.clear();
  if (extended == extraDrawn) {
    oRunnerList::iterator rit;
    
	  for (rit=Runners.begin(); rit != Runners.end(); ++rit) {
      if(rit->StartTime==0)
        undrawn.insert(rit->getClassId());
      hasRunner.insert(rit->getClassId());
    }
  }
  else if (extended == extraNumMaps)
    calculateNumRemainingMaps();

	oClassList::iterator it;	
	synchronizeList(oLClassId);

	
  reinitializeClasses();
	Classes.sort();//Sort by Id

	for (it=Classes.begin(); it != Classes.end(); ++it){		
    if(!it->Removed) {

      if (filter==filterOnlyMulti && it->getNumStages()<=1)
        continue;
      else if (filter==filterOnlySingle && it->getNumStages()>1)
        continue;
      else if (filter==filterOnlyDirect && !it->getAllowQuickEntry())
        continue;

      if (extended == extraNone)
        out.push_back(make_pair(it->Name, it->Id));
        //gdi.addItem(name, it->Name, it->Id);
      else if (extended == extraDrawn) {
        char bf[256];
        
        if(undrawn.count(it->getId()) || !hasRunner.count(it->getId()))        
          sprintf_s(bf, "%s", it->Name.c_str());
        else
          sprintf_s(bf, "%s\t[S]", it->Name.c_str());
        
        out.push_back(make_pair(string(bf), it->Id));
        //gdi.addItem(name, bf, it->Id);
      }
      else if (extended == extraNumMaps) {
        char bf[256];
        int nmaps = it->getNumRemainingMaps(false);
        if (nmaps != numeric_limits<int>::min())
          sprintf_s(bf, "%s (%d %s)", it->Name.c_str(), nmaps, lang.tl("kartor").c_str());
        else
          sprintf_s(bf, "%s ( - %s)", it->Name.c_str(), lang.tl("kartor").c_str());

        out.push_back(make_pair(string(bf), it->Id));
        //gdi.addItem(name, bf, it->Id);
      }
    }
	}
	return out;
}

void oEvent::getNotDrawnClasses(set<int> &classes, bool someMissing)
{	
  set<int> drawn;
  classes.clear();

  oRunnerList::iterator rit;

  synchronizeList(oLRunnerId);
  for (rit=Runners.begin(); rit != Runners.end(); ++rit) {
    if (rit->StartTime>0)
      drawn.insert(rit->getClassId());
    else if (someMissing)
      classes.insert(rit->getClassId());
  }

  // Return all classe where some runner has no start time
  if (someMissing)
    return;
  
	oClassList::iterator it;	
	synchronizeList(oLClassId);
	
  // Return classes where no runner has a start time
	for (it=Classes.begin(); it != Classes.end(); ++it) {
    if (drawn.count(it->getId())==0)
      classes.insert(it->getId());
	}
}


void oEvent::getAllClasses(set<int> &classes) 
{
  classes.clear();

  oClassList::const_iterator it;	
	synchronizeList(oLClassId);
	
	for (it=Classes.begin(); it != Classes.end(); ++it){		
		if(!it->Removed){
      classes.insert(it->getId());
    }
  }
}

bool oEvent::fillClassesTB(gdioutput &gdi)//Table mode
{	
	oClassList::iterator it;	
	synchronizeList(oLClassId);
  
  reinitializeClasses();
	Classes.sort();//Sort by Id

	int dx[4]={100, 100, 50, 100};
	int y=gdi.getCY();
	int x=gdi.getCX();
	int lh=gdi.getLineHeight();

	y+=lh/2;

	int xp=x;
	gdi.addString("", y, xp, 0, "Klass"); xp+=dx[0];
	gdi.addString("", y, xp, 0, "Bana"); xp+=dx[1];
	gdi.addString("", y, xp, 0, "Deltagare"); xp+=dx[2];
	y+=(3*lh)/2;

	for (it = Classes.begin(); it != Classes.end(); ++it){		
		if(!it->Removed){
			int xp=x;
	
			gdi.addString("", y, xp, 0, it->getName(), dx[0]); xp+=dx[0];

			pCourse pc=it->getCourse();
			if(pc) gdi.addString("", y, xp, 0, pc->getName(), dx[1]); 
			else gdi.addString("", y, xp, 0, "-", dx[1]); 
			xp+=dx[1];

			char num[10];
			_itoa_s(it->getNumRunners(), num, 10);

			gdi.addString("", y, xp, 0, num, dx[2]); 
			xp+=dx[2];
			
			y+=lh;
		}
	}
	return true;
}

/*
Table *oEvent::GetClassesTB()//Table mode
{	
	oClassList::iterator it;	
	SynchronizeList(oLClassId);

	Table *table=new Table;

	table->addColumn("Id", 40, true);
	table->addColumn("Klass", 120, false);
	table->addColumn("Bana", 90, false);
	table->addColumn("Deltagare", 120, true);
	table->addColumn("TS", 120, true);
	//table

	
	for (it=Classes.begin(); it != Classes.end(); ++it){		
		if(!it->Removed){


			table->addRow();
			
			
			char num[16];
			_itoa_s(it->getId(), num, 10);
			table->set(0, num);

			table->set(1, it->GetName());
			
			pCourse pc=it->getCourse();
			if(pc) table->set(1, pc->GetName());
			else table->set(2, "-");
			
			//char num[10];
			_itoa_s(it->GetNumRunners(), num, 10);
			table->set(3, num);

			table->set(4, it->Modified.GetStamp());
		}
	}
	return table;
}
*/
bool oClass::isCourseUsed(int Id) const
{
	if(Course && Course->getId()==Id)	
		return true;

	if(hasMultiCourse()){
		for(unsigned i=0;i<getNumStages(); i++) {
			const vector<pCourse> &pv=MultiCourse[i];
			for(unsigned j=0; j<pv.size(); j++)
				if (pv[j]->getId()==Id)	return true;
		}
	}

	return false;
}
/*
int oClass::GetClassStartTime() 
{
	return getDI().getInt("StartTime");
}

void oClass::SetClassStartTime(const string &t)
{
	getDI().setInt("StartTime", oe->GetRelativeTime(t));
}

string oClass::GetClassStartTimeS()
{
	int StartTime=GetClassStartTime();
	if(StartTime>0)
		return oe->GetAbsTime(StartTime);
	else return "-";
}*/

string oClass::getClassResultStatus() const
{
	list<oRunner>::iterator it;

	int nrunner=0;
	int nrunner_finish=0;

	for (it=oe->Runners.begin(); it!=oe->Runners.end();++it) {
		if (it->getClassId()==Id){
			nrunner++;

			if(it->getStatus())
				nrunner_finish++;
		}
	}

	char bf[128];
	sprintf_s(bf, "%d/%d", nrunner_finish, nrunner); 

	return bf;
}

string oClass::getLength(int leg) const
{
  char bf[64];
	if(hasMultiCourse()){
    int minlen=1000000;
    int maxlen=0;

		for(unsigned i=0;i<getNumStages(); i++) {
      if (i == leg || leg == -1) {
			  const vector<pCourse> &pv=MultiCourse[i];
        for(unsigned j=0; j<pv.size(); j++) {
          int l=pv[j]->getLength();
				  minlen=min(l, minlen);
          maxlen=max(l, maxlen);
        }
      }
		}

    if(maxlen==0)
      return string();
    else if(minlen==0)
      minlen=maxlen;

    if( (maxlen-minlen)<100 )
      sprintf_s(bf, "%d", maxlen);
    else
      sprintf_s(bf, "%d - %d", minlen, maxlen);

    return MakeDash(bf);
	}
  else if(Course && Course->getLength()>0) {    
    return Course->getLengthS();
  }
  return string();
}

pCourse oClass::getCourse(int leg, unsigned fork) const
{
  if(unsigned(leg)<MultiCourse.size()){
    if(!MultiCourse[leg].empty())
      return MultiCourse[leg][fork % MultiCourse[leg].size()];
  }
  return 0;
}

ClassType oClass::getClassType() const
{
  if(legInfo.size()==2 && (legInfo[1].isParallel() ||
                           legInfo[1].legMethod==LTIgnore) )
    return oClassPatrol;
  else if(legInfo.size()>=2) {

    for(size_t k=1;k<legInfo.size();k++)
      if(legInfo[k].duplicateRunner!=0)
        return oClassRelay;

    return oClassIndividRelay;
  }
  else 
    return oClassIndividual;
}

void oEvent::getNumClassRunners(int id, int leg, int &total, int &finished, int &dns) const
{
  total=0;
  finished=0;
  dns = 0;
	//Search runners
  
  const oClass *pc = getClass(id);

  if(!pc) 
    return;

  ClassType ct=pc->getClassType();
  if(ct==oClassIndividual || ct==oClassIndividRelay) {
    oRunnerList::const_iterator it;
    int maxleg = pc->getLastStageIndex();

	  for (it=Runners.begin(); it != Runners.end(); ++it){		
      if(!it->skip() && it->getClassId()==id) {	
        if (leg==0) {
   			  total++;

          if(it->Status!=StatusUnknown)
            finished++;
          else if(it->Status==StatusDNS)
            dns++;
        }
        else {
          int tleg = leg>0 ? leg : maxleg;
          const pRunner r=it->getMultiRunner(tleg);
          if (r) {
    			  total++;
            if (r->Status!=StatusUnknown)
              finished++;
            else if(it->Status==StatusDNS)
              dns++;
          }
        }
      }
	  }	
  }
  else {
    oTeamList::const_iterator it;

	  for (it=Teams.begin(); it != Teams.end(); ++it) {
      if(it->getClassId()==id) {	
			  total++;

        if(it->Status!=StatusUnknown || 
          it->getLegStatus(leg)!=StatusUnknown)
          finished++;
      }
	  }	
  }
}

int oClass::getNumMultiRunners(int leg) const
{
  int ndup=0;
  for (size_t k=0;k<legInfo.size();k++) {
    if(leg==legInfo[k].duplicateRunner || (legInfo[k].duplicateRunner==-1 && k==leg))
      ndup++;
  }
  if(legInfo.empty())
    ndup++; //If no multi-course, we run at least one race.

  return ndup;
}

int oClass::getNumParallel(int leg) const
{
  int nleg = legInfo.size();
  if (leg>=nleg)
    return 1;

  int nP = 1;
  int i = leg;
  while (++i<nleg && legInfo[i].isParallel())
    nP++;

  i = leg;
  while (i>=0 && legInfo[i--].isParallel())
    nP++;
  return nP;
}

int oClass::getNumDistinctRunners() const
{
  if (legInfo.empty())
    return 1;
  
  int ndist=0;
  for (size_t k=0;k<legInfo.size();k++) {
    if(legInfo[k].duplicateRunner==-1)
      ndist++;
  }
  return ndist;
}

int oClass::getNumDistinctRunnersMinimal() const
{
  if (legInfo.empty())
    return 1;
  
  int ndist=0;
  for (size_t k=0;k<legInfo.size();k++) {
    LegTypes lt = legInfo[k].legMethod;
    if(legInfo[k].duplicateRunner==-1 && (lt != LTExtra && lt != LTIgnore && lt != LTParallelOptional) )
      ndist++;
  }
  return max(ndist, 1);
}

void oClass::resetLeaderTime()
{
  vector< pair<int, int> >::iterator it;

  for(it=tLeaderTime.begin();it!=tLeaderTime.end();++it)
    it->first=0,it->second=0;
}

bool oClass::hasCoursePool() const
{
  return getDCI().getInt("HasPool")!=0;
}

void oClass::setCoursePool(bool p)
{
  getDI().setInt("HasPool", p);
}

pCourse oClass::selectCourseFromPool(int leg, const SICard &card) const
{
	int Distance=-1000;

	const oCourse *rc=0; //Best match course

  if(MultiCourse.size()==0)
    return Course;
  
  if(unsigned(leg)>=MultiCourse.size())
    return Course;

  for (size_t k=0;k<MultiCourse[leg].size(); k++) {
    pCourse pc=MultiCourse[leg][k];

		if(pc) {
			int d=pc->distance(card);

			if(d>=0) {
				if(Distance<0) Distance=1000;

				if(d<Distance) {
					Distance=d;
					rc=pc;
				}
			}
			else {
				if(Distance<0 && d>Distance) {
					Distance=d;
					rc=pc;
				}
			}
		}
	}

	return const_cast<pCourse>(rc);
}

int oClass::getBestLegTime(int leg) const
{
  if (unsigned(leg)>=tLeaderTime.size())
    return 0;
  else return 
    tLeaderTime[leg].first;
}

int oClass::getTotalLegLeaderTime(int leg) const
{
  if (unsigned(leg)>=tLeaderTime.size())
    return 0;
  else return 
    tLeaderTime[leg].second;
}


/*
bool oClass::inputData(int id, const string &input, string &output, bool noUpdate)
{
  switch (id) {
    case TID_CLASSNAME:
      setName(input);
      synchronize();
      output=getName();
      return true;
  }
  return false;
}*/


void oEvent::mergeClass(int classIdPri, int classIdSec)
{
  pClass pc = getClass(classIdPri);
  if (!pc)
    return;

  // Update teams
  for (oTeamList::iterator it = Teams.begin(); it!=Teams.end(); ++it) {
    if (it->getClassId() == classIdSec) {
      it->Class = pc;
      it->updateChanged();
      for (size_t k=0;k<it->Runners.size();k++)
        if (it->Runners[k]) {
          it->Runners[k]->Class = pc;
          it->Runners[k]->updateChanged();
        }
      it->synchronize(); //Synchronizes runners also
    }
  }

  // Update runners
  for (oRunnerList::iterator it = Runners.begin(); it!=Runners.end(); ++it) {
    if (it->getClassId() == classIdSec) {
      it->Class = pc;
      it->updateChanged();
      it->synchronize(); 
    }
  }
  oe->removeClass(classIdSec);
}

class clubCounter {
private:
  map<int, int> clubSize;
  map<int, int> clubSplit;
public:
  void addId(int clubId);
  void split(int parts);
  int getClassNo(int clubId); 
};

int clubCounter::getClassNo(int clubId) 
{
  return clubSplit[clubId];
}

void clubCounter::addId(int clubId) {
  ++clubSize[clubId];
}

void clubCounter::split(int parts)
{
  vector<int> classSize(parts);
  while ( !clubSize.empty() ) {
    // Find largest club
    int club=0;
    int size=0;
    for (map<int, int>::iterator it=clubSize.begin(); it!=clubSize.end(); ++it)
      if (it->second>size) {
        club = it->first;
        size = it->second;
      }
    clubSize.erase(club);
    // Find smallest class
    int nrunner = 1000000;
    int cid = 0;

    for(int k=0;k<parts;k++)
      if (classSize[k]<nrunner) {
        nrunner = classSize[k];
        cid = k;
      }

    //Store result
    clubSplit[club] = cid;
    classSize[cid] += size;
  }
}

void oEvent::splitClass(int classId, int parts, vector<int> &outClassId)
{
  if (parts<=1)
    return;
  
  pClass pc = getClass(classId);
  if (pc==0)
    return;

  vector<pClass> pcv(parts);
  outClassId.resize(parts);
  pcv[0] = pc;
  outClassId[0] = pc->getId();

  for (int k=1; k<parts; k++) {
    char bf[16];
    sprintf_s(bf, "-%d", k+1);
    pcv[k] = addClass(pc->getName() + MakeDash(bf), pc->getCourseId());
    if (pcv[k]) {
      // Find suitable sort index
      int si = pcv[k]->getSortIndex(pc->getDCI().getInt("SortIndex")+1);
      pcv[k]->getDI().setInt("SortIndex", si);
   
      pcv[k]->synchronize();
    }
    outClassId[k] = pcv[k]->getId();
  }

  pc->setName(pc->getName() + MakeDash("-1"));
  pc->synchronize();

  if ( classHasTeams(classId) ) {
    clubCounter cc;
    // Split teams.
    for (oTeamList::iterator it = Teams.begin(); it!=Teams.end(); ++it) {
      if (it->getClassId() == classId) 
        cc.addId(it->getClubId());
    }
    cc.split(parts);
  
    for (oTeamList::iterator it = Teams.begin(); it!=Teams.end(); ++it) {
      if (it->getClassId() == classId) {
        it->Class = pcv[cc.getClassNo(it->getClubId())];
        it->updateChanged();
        for (size_t k=0;k<it->Runners.size();k++)
          if (it->Runners[k]) {
            it->Runners[k]->Class = it->Class;
            it->Runners[k]->updateChanged();
          }
        it->synchronize(); //Synchronizes runners also
      }       
    }
  }
  else {
    clubCounter cc;
    for (oRunnerList::iterator it = Runners.begin(); it!=Runners.end(); ++it) {
      if (it->getClassId() == classId) 
        cc.addId(it->getClubId());
    }
    cc.split(parts);
    for (oRunnerList::iterator it = Runners.begin(); it!=Runners.end(); ++it) {
      if (it->getClassId() == classId) {
        it->Class = pcv[cc.getClassNo(it->getClubId())];
        it->updateChanged();
        it->synchronize(); //Synchronizes runners also
      }       
    }
  }
}

void oEvent::splitClassRank(int classId, int runners, vector<int> &outClassId) 
{
  throw std::exception("Not implemented yet");
}

void oClass::getAgeLimit(int &low, int &high) const
{
  low = getDCI().getInt("LowAge");
  high = getDCI().getInt("HighAge");
}

void oClass::setAgeLimit(int low, int high)
{
  getDI().setInt("LowAge", low);
  getDI().setInt("HighAge", high);
}

int oClass::getExpectedAge() const
{
  int low, high;
  getAgeLimit(low, high);

  if (low>0 && high>0)
    return (low+high)/2;

  if (low==0 && high>0)
    return high-3;

  if (low>0 && high==0)
    return low + 1;


  // Try to guess age from class name
  for (size_t k=0; k<Name.length(); k++) {
    if (Name[k]>='0' && Name[k]<='9') {
      int age = atoi(&Name[k]);
      if (age>=10 && age<100) {
        if (age>=10 && age<=20)
          return age - 1;
        else if (age==21)
          return 28;
        else if (age>=35)
          return age + 2;
      }
    }
  }

  return 0;
}

void oClass::setSex(const string &sex) 
{
  getDI().setString("Sex", sex);
}

string oClass::getSex() const 
{
  return getDCI().getString("Sex");
}

void oClass::setStart(const string &start)
{
  getDI().setString("StartName", start);
}

string oClass::getStart() const 
{
  return getDCI().getString("StartName");
}

void oClass::setBlock(int block)
{
  getDI().setInt("StartBlock", block);
}

int oClass::getBlock() const 
{
  return getDCI().getInt("StartBlock");
}

void oClass::setAllowQuickEntry(bool quick)
{
  getDI().setInt("AllowQuickEntry", quick);
}

bool oClass::getAllowQuickEntry() const 
{
  return getDCI().getInt("AllowQuickEntry")!=0;
}

void oClass::setNoTiming(bool quick)
{
  tNoTiming = quick ? 1 : 0;
  getDI().setInt("NoTiming", quick);
}

bool oClass::getNoTiming() const 
{
  if (tNoTiming!=0 && tNoTiming!=1)
    tNoTiming = getDCI().getInt("NoTiming")!=0 ? 1 : 0;
  return tNoTiming!=0;
}


void oClass::setType(const string &start)
{
  getDI().setString("ClassType", start);
}

string oClass::getType() const 
{
  return getDCI().getString("ClassType");
}

void oEvent::fillStarts(gdioutput &gdi, const string &id)
{
  vector< pair<string, size_t> > d;
  oe->fillStarts(d);
  gdi.addItem(id, d);
}

const vector< pair<string, size_t> > &oEvent::fillStarts(vector< pair<string, size_t> > &out)
{
  out.clear();
  set<string> starts;
  for (oClassList::iterator it = Classes.begin(); it!=Classes.end(); ++it) {
    if (!it->getStart().empty())
      starts.insert(it->getStart());
  }

  if (starts.empty())
    starts.insert("Start 1");

  for (set<string>::iterator it = starts.begin(); it!=starts.end(); ++it) {
    //gdi.addItem(id, *it);
    out.push_back(make_pair(*it, 0));
  }
  return out;
}

void oEvent::fillClassTypes(gdioutput &gdi, const string &id)
{
  vector< pair<string, size_t> > d;
  oe->fillClassTypes(d);
  gdi.addItem(id, d);
}

ClassMetaType oClass::interpretClassType() const {
  int lowAge;
  int highAge;
  getAgeLimit(lowAge, highAge);

  if (highAge>0 && highAge <= 16)
    return ctYouth;

  map<string, ClassMetaType> types;
  oe->getPredefinedClassTypes(types);

  string type = getType();

  for (map<string, ClassMetaType>::iterator it = types.begin(); it != types.end(); ++it) {
    if (type == it->first || type == lang.tl(it->first))
      return it->second;
  }

  if (oe->classTypeNameToType.empty()) {
    // Lazy readout of baseclasstypes
    char path[_MAX_PATH];
    getUserFile(path, "baseclass.xml");
    xmlparser xml;
    if (xml.read(path)) {
      xmlobject cType = xml.getObject("BaseClassTypes");
      xmlList xtypes;
      cType.getObjects("Type", xtypes);
      for (size_t k = 0; k<xtypes.size(); k++) {
        string name = xtypes[k].getAttrib("name").get();
        string typeS = xtypes[k].getAttrib("class").get();
        ClassMetaType mtype = ctUnknown;
        if (stringMatch(typeS, "normal"))
          mtype = ctNormal;
        else if (stringMatch(typeS, "elite"))
          mtype = ctElite;
        else if (stringMatch(typeS, "youth"))
          mtype = ctYouth;
        else if (stringMatch(typeS, "open"))
          mtype = ctOpen;
        else if (stringMatch(typeS, "exercise"))
          mtype = ctExercise;
        else if (stringMatch(typeS, "training"))
          mtype = ctTraining;
        else {
          string err = "Unknown type X#" + typeS;
          throw std::exception(err.c_str());
        }
        oe->classTypeNameToType[name] = mtype;
      }
    }
  }

  if (oe->classTypeNameToType.count(type) == 1)
    return oe->classTypeNameToType[type];

  return ctUnknown;
}

void oClass::assignTypeFromName(){
  string type = getType();
  if (type.empty()) {
    string prefix, suffix;
    int age = extractAnyNumber(Name, prefix, suffix);

    ClassMetaType mt = ctUnknown;
    if (age>=18) {
      if (stringMatch(suffix, "Elit") || strchr(suffix.c_str(), 'E'))
        mt = ctElite;
      else if (stringMatch(suffix, "Motion") || strchr(suffix.c_str(), 'M'))
        mt = ctExercise;
      else
        mt = ctNormal;
    }
    else if (age>=10 && age<=16) {
      mt = ctYouth;
    }
    else if (age<10) {
      if (stringMatch(prefix, "Ungdom") || strchr(prefix.c_str(), 'U'))
        mt = ctYouth;
      else if (stringMatch(suffix, "Motion") || strchr(suffix.c_str(), 'M'))
        mt = ctExercise;
      else
        mt = ctOpen;
    }

    map<string, ClassMetaType> types;
    oe->getPredefinedClassTypes(types);

    for (map<string, ClassMetaType>::iterator it = types.begin(); it != types.end(); ++it) {
      if (it->second == mt) {
        setType(it->first);
        return;
      }
    }


  }
}

void oEvent::getPredefinedClassTypes(map<string, ClassMetaType> &types) const {
  types.clear();
  types["Elit"] = ctElite;
  types["Vuxen"] = ctNormal;
  types["Ungdom"] = ctYouth;
  types["Motion"] = ctExercise;
  types["Öppen"] = ctOpen;
  types["Träning"] = ctTraining;
}

const vector< pair<string, size_t> > &oEvent::fillClassTypes(vector< pair<string, size_t> > &out)
{
  out.clear();
  set<string> cls;
  bool allHasType = !Classes.empty();
  for (oClassList::iterator it = Classes.begin(); it!=Classes.end(); ++it) {
    if (it->isRemoved())
      continue;
    
    if (!it->getType().empty())
      cls.insert(it->getType());
    else
      allHasType = false;
  }

  if (!allHasType) {
    map<string, ClassMetaType> types;
    getPredefinedClassTypes(types);

    for (map<string, ClassMetaType>::iterator it = types.begin(); it != types.end(); ++it)
      cls.insert(lang.tl(it->first));
  }

  for (set<string>::iterator it = cls.begin(); it!=cls.end(); ++it) {
    //gdi.addItem(id, *it);
    out.push_back(make_pair(*it, 0));
  }
  return out;
}


int oClass::getNumRemainingMaps(bool recalculate) const
{
  if (recalculate)
    oe->calculateNumRemainingMaps();

  if (Course)
    return Course->tMapsRemaining;
  else
    return numeric_limits<int>::min();
}

void oEvent::getStartBlocks(vector<int> &blocks, vector<string> &starts) const
{
  oClassList::const_iterator it;
  map<int, string> bs;
  for (it = Classes.begin(); it != Classes.end(); ++it) {
    if (it->isRemoved())
      continue;
    map<int, string>::iterator v = bs.find(it->getBlock());

    if (v!=bs.end() && v->first!=0 && v->second!=it->getStart()) {
      string msg = "Ett startblock spänner över flera starter: " + it->getStart() + " " + v->second;
      throw std::exception(msg.c_str());
    }
    bs[it->getBlock()] = it->getStart();
  }

  blocks.clear();
  starts.clear();

  if (bs.size() > 1) {
    for (map<int, string>::iterator v = bs.begin(); v != bs.end(); ++v) {
      blocks.push_back(v->first);
      starts.push_back(v->second);
    }
  }
  else if (bs.size() == 1) {
    set<string> s;
    for (it = Classes.begin(); it != Classes.end(); ++it) {
      if (it->isRemoved())
        continue;
      s.insert(it->getStart());
    }
    for (set<string>::iterator v = s.begin(); v != s.end(); ++v) {
      blocks.push_back(bs.begin()->first);
      starts.push_back(*v);
    }

  }


}

Table *oEvent::getClassTB()//Table mode
{	
  if (tables.count("class") == 0) {
	  Table *table=new Table(this, 20, "Klasser", "classes");

    table->addColumn("Id", 70, false);
    table->addColumn("Ändrad", 70, false);
	  
    table->addColumn("Namn", 200, false);
    oe->oClassData->buildTableCol(table);
    tables["class"] = table;
    table->addOwnership();
  }

  tables["class"]->update();
  return tables["class"];
}

void oEvent::generateClassTableData(Table &table, oClass *addClass)
{ 
  if (addClass) {
    addClass->addTableRow(table);
    return;
  }

  synchronizeList(oLClassId);
	oClassList::iterator it;	

  for (it=Classes.begin(); it != Classes.end(); ++it){		
    if(!it->isRemoved())
      it->addTableRow(table);
	}
}

void oClass::addTableRow(Table &table) const {
  pClass it = pClass(this);
  table.addRow(getId(), it);

  int row = 0;
  table.set(row++, *it, TID_ID, itos(getId()), false);
  table.set(row++, *it, TID_MODIFIED, getTimeStamp(), false);

  table.set(row++, *it, TID_CLASSNAME, getName(), true);
  oe->oClassData->fillTableCol(oData, *this, table);
}



bool oClass::inputData(int id, const string &input, 
                       int inputId, string &output, bool noUpdate)
{
  synchronize(false);
    
  if(id>1000) {
    bool b = oe->oClassData->inputData(this, oData, id, input, output, noUpdate);
    if (b) {
      synchronize(true);
      oe->oClassData->inputData(this, oData, id, input, output, noUpdate);
    }
    return b;
  }
  switch(id) {
    case TID_CLASSNAME:
      setName(input);
      synchronize();
      output=getName();
      return true;
    break;
  }

  return false;
}

void oClass::fillInput(int id, vector< pair<string, size_t> > &out, size_t &selected)
{ 
  if (id==TID_COURSE) {
    out.clear();
    oe->fillCourses(out, true);
    out.push_back(make_pair(lang.tl("Ingen bana"), 0));
    //gdi.selectItemByData(controlId.c_str(), Course ? Course->getId() : 0);
    selected = Course ? Course->getId() : 0;
  }  
}


void oClass::getStatistics(int feeLock, int &entries, int &started) const
{
  oRunnerList::const_iterator it;
  entries = 0;
  started = 0;
  for (it = oe->Runners.begin(); it != oe->Runners.end(); ++it) {
    if (it->skip() || it->isVacant())
      continue;

    if (it->getClassId()==Id) {
      if (feeLock == 0 || feeLock==it->getDCI().getInt("Fee")) {
        entries++;
        if (it->getStatus()!= StatusUnknown && it->getStatus()!= StatusDNS)
          started++;
      }
    }
  }
}

bool oClass::isSingleRunnerMultiStage() const
{
  return getNumStages()>1 && getNumDistinctRunnersMinimal()==1;
}

int oClass::getEntryFee(const string &date) const 
{
  string oentry = oe->getDCI().getDate("OrdinaryEntry");
  bool late = date >= oentry;
  if (late)
    return getDCI().getInt("HighClassFee");
  else
    return getDCI().getInt("ClassFee");
}
  
void oClass::addClassDefaultFee(bool resetFee) {
  int fee = getDCI().getInt("ClassFee");

  if (fee == 0 || resetFee) {
    assignTypeFromName();
    ClassMetaType type = interpretClassType();
   // if (type.empty())
    switch (type) {
      case ctElite:
        fee = oe->getDCI().getInt("EliteFee");
      break;
      case ctYouth:
        fee = oe->getDCI().getInt("YouthFee");
      break;
      default:
        fee = oe->getDCI().getInt("EntryFee");
    }
    double factor = 1.0 + 0.01 * atof(oe->getDCI().getString("LateEntryFactor").c_str());
    int lateFee = fee;
    if (factor > 1)
      lateFee = int(fee*factor + 0.5);

    getDI().setInt("ClassFee", fee);
    getDI().setInt("HighClassFee", lateFee);
  }
}

void oClass::reinitialize() 
{
  int ix = getDI().getInt("SortIndex");
  if (ix == 0) {
    ix = getSortIndex(getId()*10);
    getDI().setInt("SortIndex", ix);
  }
  tSortIndex = ix;
}

void oEvent::reinitializeClasses() 
{
  for (oClassList::iterator it = Classes.begin(); it != Classes.end(); ++it)
    it->reinitialize();
}

int oClass::getSortIndex(int candidate) 
{
  int major = numeric_limits<int>::max();
  int minor = 0;

  for (oClassList::iterator it = oe->Classes.begin(); it != oe->Classes.end(); ++it) {
    int ix = it->getDCI().getInt("SortIndex");
    if (ix>0) {
      if (ix>candidate && ix<major)
        major = ix;

      if (ix<candidate && ix>minor)
        minor = ix;
    }
  }
 
  // If the gap is less than 10 (which is the default), optimize
  if (major < numeric_limits<int>::max() && minor>0 && ((major-candidate)<10 || (candidate-minor)<10))
    return (major+minor)/2;
  else
    return candidate;
}

void oEvent::getClassSettingsTable(gdioutput &gdi, GUICALLBACK cb) 
{
  synchronizeList(oLCourseId);
  reinitializeClasses();
  Classes.sort();

  oClassList::const_iterator it;

  int yp = gdi.getCY();
  int a = gdi.scaleLength(160);
  int b = gdi.scaleLength(250);
  int c = gdi.scaleLength(300);
  int d = gdi.scaleLength(350);
  int e = gdi.scaleLength(520);
  int f = gdi.scaleLength(30);

  int ek1 = 0, ek2 = 0, ekextra = 0;
  bool useEco = oe->useEconomy();

  if (useEco) {
    ek1 = gdi.scaleLength(60);
    ek2 = gdi.scaleLength(60);
    ekextra = gdi.scaleLength(20);
    d += ek1 + ek2 + ekextra;
    e += ek1 + ek2 + ekextra;
    gdi.addString("", yp, c+ek1, 1, "Avgift");
    gdi.addString("", yp, c+ek2+ek1, 1, "Hög avgift");
  }
  
  gdi.addString("", yp, gdi.getCX(), 1, "Klass");
  gdi.addString("", yp, a, 1, "Start");
  gdi.addString("", yp, b, 1, "Block");
  gdi.addString("", yp, c, 1, "Index");
  gdi.addString("", yp, d, 1, "Bana");
  gdi.addString("", yp, e, 1, "Direktanmälan");

  vector< pair<string,size_t> > arg;
  fillCourses(arg, true);

  for (it = Classes.begin(); it != Classes.end(); ++it) {
    if (it->isRemoved())
      continue;
    int yp = gdi.getCY();
    string id = itos(it->getId());
    gdi.addStringUT(0, it->getName(), 0);
    gdi.addInput(a, yp, "Strt"+id, it->getStart(), 7, cb).setExtra(LPVOID(&*it));
    string blk = it->getBlock()>0 ? itos(it->getBlock()) : "";
    gdi.addInput(b, yp, "Blck"+id, blk, 4);
    gdi.addInput(c, yp, "Sort"+id, itos(it->getDCI().getInt("SortIndex")), 4);
  
    if (useEco) {
      gdi.addInput(c + ek1, yp, "Fee"+id, oe->formatCurrency(it->getDCI().getInt("ClassFee")), 5);
      gdi.addInput(c + ek1 + ek2, yp, "LateFee"+id, oe->formatCurrency(it->getDCI().getInt("HighClassFee")), 5);
    }

    string crs = "Cors"+id;
    gdi.addSelection(d, yp, crs, 150, 400);
    gdi.addItem(crs, arg);
    gdi.selectItemByData(crs.c_str(), it->getCourseId());
    gdi.addCheckbox(e+f, yp, "Dirc"+id, "   ", 0, it->getAllowQuickEntry());
    gdi.dropLine(-0.3);
  }
}

void oEvent::saveClassSettingsTable(gdioutput &gdi)
{
  synchronizeList(oLCourseId);
  oClassList::iterator it;

  for (it = Classes.begin(); it != Classes.end(); ++it) {
    if (it->isRemoved())
      continue;

    string id = itos(it->getId());
    string start = gdi.getText("Strt"+id);
    int block = gdi.getTextNo(("Blck"+id).c_str());
    int sort = gdi.getTextNo(("Sort"+id).c_str());

    if (gdi.hasField("Fee" + id)) {
      int fee = gdi.getTextNo(("Fee"+id).c_str());
      int latefee = gdi.getTextNo(("LateFee"+id).c_str());
      it->getDI().setInt("ClassFee", fee);
      it->getDI().setInt("HighClassFee", latefee);
    }

    int courseId = 0;
    ListBoxInfo lbi;
    if(gdi.getSelectedItem("Cors"+id, &lbi))
      courseId = lbi.data;
    bool direct = gdi.isChecked("Dirc"+id);

    it->setStart(start);
    it->setBlock(block);
    it->setCourse(getCourse(courseId));
    it->getDI().setInt("SortIndex", sort);
    it->setAllowQuickEntry(direct);
  }
}

void oClass::apply() {
  int trueLeg = 0;
  int trueSubLeg = 0;

  for (size_t k = 0; k<legInfo.size(); k++) {
    oLegInfo &li = legInfo[k];
    LegTypes lt = li.legMethod;
    if (lt == LTNormal || lt == LTSum) {
      trueLeg++;
      trueSubLeg = 0;
    }
    else 
      trueSubLeg++;

    if (trueSubLeg == 0 && (k+1) < legInfo.size()) {
      LegTypes nt = legInfo[k+1].legMethod;
      if (nt == LTParallel || nt == LTParallelOptional || nt == LTExtra || nt == LTIgnore)
        trueSubLeg = 1;
    }
    li.trueLeg = trueLeg;
    li.trueSubLeg = trueSubLeg;
    if (trueSubLeg == 0)
      li.displayLeg = itos(trueLeg);
    else
      li.displayLeg = itos(trueLeg) + "." + itos(trueSubLeg);
  }
}

class LegResult 
{
private:
  inthashmap rmap;
public:
  void addTime(int from, int to, int time);
  int getTime(int from, int to) const;
};

void LegResult::addTime(int from, int to, int time) 
{
  int key = from + (to<<15);
  int value;
  if (rmap.lookup(key, value))
    time = min(value, time);
  rmap[key] = time;
}

int LegResult::getTime(int from, int to) const 
{
  int key = from + (to<<15);
  int value;
  if (rmap.lookup(key, value))
    return value;
  else
    return 0;
}

void oClass::clearSplitAnalysis() 
{
  tFirstStart.clear();
  tLastStart.clear();

  tSplitAnalysisData.clear();
  tCourseLegLeaderTime.clear();
  tCourseAccLegLeaderTime.clear();

  if (tLegLeaderTime)
    delete tLegTimeToPlace;
  tLegTimeToPlace = 0;

  if (tLegAccTimeToPlace)
    delete tLegAccTimeToPlace;
  tLegAccTimeToPlace = 0;
  
  tSplitRevision++;

  oe->classChanged(this, false);
}

void oClass::insertLegPlace(int from, int to, int time, int place)
{
  if (tLegTimeToPlace) {
    int key = time + (to + from*256)*8013;
    tLegTimeToPlace->insert(key, place); 
  }
}

int oClass::getLegPlace(pControl from, pControl to, int time) const 
{
  if (tLegTimeToPlace) {
    int ito = to ? to->getId() : 0;
    int ifrom = from ? from->getId() : 0;
    int key = time + (ito + ifrom*256)*8013;
    int place;
    if (tLegTimeToPlace->lookup(key, place))
      return place;
  }
  return 0;
}

void oClass::insertAccLegPlace(int courseId, int controlNo, int time, int place)
{
  if (tLegAccTimeToPlace) {
    int key = time + (controlNo + courseId*128)*16013;
    tLegAccTimeToPlace->insert(key, place); 
  }
}

void oClass::getStartRange(int leg, int &firstStart, int &lastStart) const {
  if (tFirstStart.empty()) {
    size_t s = getLastStageIndex() + 1;
    assert(s>0);
    tFirstStart.resize(s, 3600 * 24 * 365);
    tLastStart.resize(s, 0);
    for (oRunnerList::iterator it = oe->Runners.begin(); it != oe->Runners.end(); ++it) {
      if (it->isRemoved() || it->Class != this) 
        continue;
      if (it->needNoCard())
        continue;
      size_t tleg = it->tLeg;
      if (tleg < s) {
        tFirstStart[tleg] = min<unsigned>(tFirstStart[tleg], it->StartTime);
        tLastStart[tleg] = max<signed>(tLastStart[tleg], it->StartTime);
      }
    }
  }
  if (unsigned(leg) >= tFirstStart.size())
    leg = 0;

  firstStart = tFirstStart[leg];
  lastStart = tLastStart[leg];
}

int oClass::getAccLegPlace(int courseId, int controlNo, int time) const 
{
  if (tLegAccTimeToPlace) {
    int key = time + (controlNo + courseId*128)*16013;
    int place;
    if (tLegAccTimeToPlace->lookup(key, place))
      return place;
  }
  return 0;
}


void oClass::calculateSplits() 
{
  clearSplitAnalysis();
  set<pCourse> cSet;
  map<int, vector<int> > legToTime;

  for (size_t k=0;k<MultiCourse.size();k++) {    
    for (size_t j=0; j<MultiCourse[k].size(); j++) {
      if(MultiCourse[k][j])
        cSet.insert(MultiCourse[k][j]);
    }
  }
  if (getCourse())
    cSet.insert(getCourse());

  LegResult legRes;
  LegResult legBestTime;

  for (set<pCourse>::iterator cit = cSet.begin(); cit!= cSet.end(); ++cit)  { 
    pCourse pc = *cit;
    // Store all split times in a matrix
    const unsigned nc = pc->getNumControls();
    if (nc == 0)
      return;

    vector< vector<int> > splits(nc+1);
    vector< vector<int> > splitsAcc(nc+1);
  
    for (oRunnerList::iterator it = oe->Runners.begin(); it != oe->Runners.end(); ++it) {
      if (it->isRemoved() || it->Class != this)
        continue;
      
      pCourse tpc = it->getCourse();

      if (tpc != pc)
        continue;

      const vector<int> &sp = it->getSplitTimes();
      const int s = min<int>(nc, sp.size());

      vector<int> &tLegTimes = it->tLegTimes;
      tLegTimes.resize(nc + 1);
      bool ok = true;

      for (int k = 0; k < s; k++) {
        if (sp[k] > 0) {
          if (ok) {
            // Store accumulated times
            int t = sp[k] - it->StartTime;
            if (it->StartTime>0 && t>0) 
              splitsAcc[k].push_back(t);
          }

          if (k == 0) { // start -> first
            int t = sp[0] - it->StartTime;
            if (it->StartTime>0 && t>0) {
              splits[k].push_back(t);
              tLegTimes[k] = t;
            }
            else
              tLegTimes[k] = 0;
          }
          else { // control -> control
            int t = sp[k] - sp[k-1];
            if (sp[k-1]>0 && t>0) {
              splits[k].push_back(t);
              tLegTimes[k] = t;
            }
            else
              tLegTimes[k] = 0;
          }
        }
        else
          ok = false;
      }

      // last -> finish
      if (sp.size() == nc && sp[nc-1]>0 && it->FinishTime > 0) {
        int t = it->FinishTime - sp[nc-1];
        if (t>0) {
          splits[nc].push_back(t);
          tLegTimes[nc] = t;
          if (it->statusOK() && (it->FinishTime - it->StartTime) > 0) {
            splitsAcc[nc].push_back(it->FinishTime - it->StartTime);
          }
        }
        else
          tLegTimes[nc] = 0;
      }
    }
    
    if (splits.size()>0 && tLegTimeToPlace == 0) {
      tLegTimeToPlace = new inthashmap(splits.size() * splits[0].size());
      tLegAccTimeToPlace = new inthashmap(splits.size() * splits[0].size());
    }

    vector<int> &accLeaderTime = tCourseAccLegLeaderTime[pc->getId()];

    for (size_t k = 0; k < splits.size(); k++) {
      
      // Calculate accumulated best times and places
      if (!splitsAcc[k].empty()) {
        sort(splitsAcc[k].begin(), splitsAcc[k].end());
        accLeaderTime.push_back(splitsAcc[k].front()); // Store best time

        int place = 1;
        for (size_t j = 0; j < splitsAcc[k].size(); j++) {
          if (j>0 && splitsAcc[k][j-1]<splitsAcc[k][j])
            place = j+1;
          insertAccLegPlace(pc->getId(), k, splitsAcc[k][j], place);
        }
      }
      
      sort(splits[k].begin(), splits[k].end());
      const size_t ntimes = splits[k].size();
      if (ntimes == 0)
        continue;

      int from = 0, to = 0; // Represents start/finish
      if (k < nc && pc->getControl(k))
        to = pc->getControl(k)->getId();
      if (k>0 && pc->getControl(k-1))
        from = pc->getControl(k-1)->getId();

      for (size_t j = 0; j < ntimes; j++)
        legToTime[256*from + to].push_back(splits[k][j]);

      int time = 0;
      if (ntimes < 5)
        time = splits[k][0]; // Best time
      else if (ntimes < 12)
        time = (splits[k][0]+splits[k][1]) / 2; //Average best time
      else {
        int nval = ntimes/6;
        for (int r = 1; r <= nval; r++)// "Best fraction", skip winner
          time += splits[k][r];
        time /= nval;
      }

      legRes.addTime(from, to, time);
      legBestTime.addTime(from, to, splits[k][0]); // Add leader time
    }
  }

  // Loop and sort times for each leg run in this class
  for (map<int, vector<int> >::iterator cit = legToTime.begin(); cit != legToTime.end(); ++cit) {
    int key = cit->first;
    vector<int> &times = cit->second;
    sort(times.begin(), times.end());
    int jsiz = times.size();
    for (int j = 0; j<jsiz; ++j) {
      if (j==0 || times[j-1]<times[j])
        insertLegPlace(0, key, times[j], j+1);
    }
  }

  for (set<pCourse>::iterator cit = cSet.begin(); cit != cSet.end(); ++cit)  { 
    pCourse pc = *cit;
    const unsigned nc = pc->getNumControls();
    vector<int> normRes(nc+1);
    vector<int> bestRes(nc+1);
        
    for (size_t k = 0; k <= nc; k++) {
      int from = 0, to = 0; // Represents start/finish
      if (k < nc && pc->getControl(k))
        to = pc->getControl(k)->getId();
      if (k>0 && pc->getControl(k-1))
        from = pc->getControl(k-1)->getId();
      normRes[k] = legRes.getTime(from, to);
      bestRes[k] = legBestTime.getTime(from, to);
    }

    swap(tSplitAnalysisData[pc->getId()], normRes);
    swap(tCourseLegLeaderTime[pc->getId()], bestRes);
  }
}

bool oClass::isRogaining() const {
  if (Course) 
    return Course->getMaximumRogainingTime() > 0;

  for (size_t k = 0;k<MultiCourse.size(); k++) 
    for (size_t j = 0;k<MultiCourse[j].size(); j++)
      if (MultiCourse[k][j])
        return MultiCourse[k][j]->getMaximumRogainingTime() > 0;
        
  return false;
}

void oClass::remove() 
{
  if (oe)
    oe->removeClass(Id);
}

bool oClass::canRemove() const 
{
  return !oe->isClassUsed(Id);
}
