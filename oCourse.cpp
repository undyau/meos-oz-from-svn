/************************************************************************
    MeOS - Orienteering Software
    Copyright (C) 2009-2012 Melin Software HB
    
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

// oCourse.cpp: implementation of the oCourse class.
//
//////////////////////////////////////////////////////////////////////
 
#include "stdafx.h"
#include "oCourse.h"
#include "oEvent.h"
#include "SportIdent.h"
#include <limits>
#include "Localizer.h"
#include "meos_util.h"

#include "gdioutput.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

#include "intkeymapimpl.hpp"

oCourse::oCourse(oEvent *poe) : oBase(poe)
{
	getDI().initData();
	nControls=0;
	Length=0;
	Id=oe->getFreeCourseId();
}

oCourse::oCourse(oEvent *poe, int id) : oBase(poe)
{
	getDI().initData();
	nControls=0;
	Length=0;
  if (id == 0)
    id = oe->getFreeCourseId();

	Id=id;
  oe->qFreeCourseId = max(id, oe->qFreeCourseId);
}

oCourse::~oCourse()
{
}

string oCourse::getInfo() const
{
  return "Bana " + Name;
}

bool oCourse::Write(xmlparser &xml)
{
	if(Removed) return true;

	xml.startTag("Course");

	xml.write("Id", Id);
	xml.write("Updated", Modified.GetStamp());
	xml.write("Name", Name);
	xml.write("Length", Length);
	xml.write("Controls", getControls());
	xml.write("Legs", getLegLengths());

	getDI().write(xml);
	xml.endTag();

	return true;
}

void oCourse::Set(const xmlobject *xo)
{
	xmlList xl;
  xo->getObjects(xl);

	xmlList::const_iterator it;

	for(it=xl.begin(); it != xl.end(); ++it){
		if(it->is("Id")){
			Id=it->getInt();			
		}
		else if(it->is("Length")){
			Length=it->getInt();			
		}
		else if(it->is("Name")){
			Name=it->get();
		}
		else if(it->is("Controls")){
			importControls(it->get());
		}
    else if (it->is("Legs")) {
			importLegLengths(it->get());
    }
    else if(it->is("oData")){
      getDI().set(*it);
		}
		else if(it->is("Updated")){
			Modified.SetStamp(it->get());
		}
	}
}

string oCourse::getLegLengths() const
{
	string str;
	for(size_t m=0; m<legLengths.size(); m++){
		if (m>0)
      str += ";";
    str+=itos(legLengths[m]);
	}
	return str;
}

string oCourse::getControls() const
{
	string str="";
	char bf[16];
	for(int m=0;m<nControls;m++){
		sprintf_s(bf, 16, "%d;", Controls[m]->Id);
		str+=bf;
	}

	return str;
}

string oCourse::getControlsUI() const
{
	string str="";
	char bf[16];
	int m;

	for(m=0;m<nControls-1;m++){
		sprintf_s(bf, 16, "%d, ", Controls[m]->Id);
		str+=bf;
	}

	if(m<nControls){
		sprintf_s(bf, 16, "%d", Controls[m]->Id);
		str+=bf;
	}

	return str;
}

vector<string> oCourse::getCourseReadable(int limit) const
{
  vector<string> res;

	string str="S-";
	int m;
  
  vector<pControl> rg;
  bool needFinish = false;
  bool rogaining = hasRogaining();
	for (m=0; m<nControls; m++) {
    if (Controls[m]->isRogaining(rogaining))
      rg.push_back(Controls[m]);
    else {
		  str += Controls[m]->getLongString() + "-";
      needFinish = true;
    }
    if (str.length() >= size_t(limit)) {
      res.push_back(str);
      str.clear();
    }
	}

  if (needFinish)
    str += "M";

  if (!str.empty()) {
    if (str.length()<5 && !res.empty())
      res.back().append(str);
    else
      res.push_back(str);
    str.clear();
  }

  if (!rg.empty()) {
    str = lang.tl("Rogaining: ");
    for (size_t k = 0; k<rg.size(); k++) {
      if (k>0)
        str += ", ";

      if (str.length() >= size_t(limit)) {
        res.push_back(str);
        str.clear();
      }
      str += rg[k]->getLongString();
	  }
    if (!str.empty()) {
      res.push_back(str);
    }
  }

	return res;
}

pControl oCourse::addControl(int Id)
{
  pControl pc = doAddControl(Id);
	updateChanged();
  return pc;
}

pControl oCourse::doAddControl(int Id)
{
	if (nControls<NControlsMax) {
    pControl c=oe->getControl(Id, true);
    if (c==0)
      throw std::exception("Felaktig kontroll");
		Controls[nControls++]=c;
    return c;
	}
  else throw std::exception("För många kontroller.");
}

void oCourse::importControls(const string &ctrls)
{
	const char *str=ctrls.c_str();

  int oldNC = nControls;
  vector<int> oldC;
  for (int k = 0; k<nControls; k++)
    oldC.push_back(Controls[k] ? Controls[k]->getId() : 0);

	nControls = 0;

	while (*str) {
		int cid=atoi(str);

		while(*str && (*str!=';' && *str!=',' && *str!=' ')) str++;
		while(*str && (*str==';' || *str==',' || *str==' ')) str++;		

		if(cid>0)
			doAddControl(cid);
	}

  bool changed = nControls != oldNC;

  for (int k = 0; !changed && k<nControls; k++)
    changed |= oldC[k] != Controls[k]->getId();
      
  if (changed)
    updateChanged();
}

void oCourse::importLegLengths(const string &legs)
{
  vector<string> splits;
  split(legs, ";", splits);

  legLengths.resize(splits.size());
  for (size_t k = 0; k<legLengths.size(); k++)
    legLengths[k] = atoi(splits[k].c_str());
}

oControl *oCourse::getControl(int index)
{
	if(index>=0 && index<nControls)
		return Controls[index];
	else return 0;
}

bool oCourse::fillCourse(gdioutput &gdi, const string &name)
{
	oPunchList::iterator it;	
  bool rogaining = hasRogaining();
	gdi.clearList(name);
	int offset = 1;
	gdi.addItem(name, lang.tl("Start"), -1);
	for (int k=0;k<nControls;k++) {
		string c = Controls[k]->getString();
		int multi = Controls[k]->getNumMulti();
    int submulti = 0;
    char bf[64];
    if (Controls[k]->isRogaining(rogaining)) {
      sprintf_s(bf, 64, "R\t%s", c.c_str());
      offset--;
    }
    else if (multi == 1) {
		  sprintf_s(bf, 64, "%d\t%s", k+offset, c.c_str());
    }
    else
      sprintf_s(bf, 64, "%d%c\t%s", k+offset, 'A', c.c_str());
    gdi.addItem(name, bf, k);
    while (multi>1) {
      submulti++;
      sprintf_s(bf, 64, "%d%c\t-:-", k+offset, 'A'+submulti);
		  gdi.addItem(name, bf, -1);
      multi--;
    }
    offset += submulti;
	}
	gdi.addItem(name, lang.tl("Mål"), -1);

	return true;
}

void oCourse::getControls(list<pControl> &pc)
{
	pc.clear();
	for(int k=0;k<nControls;k++){
		pc.push_back(Controls[k]);
	}
}

int oCourse::distance(const SICard &card)
{
	int cardindex=0;
	int matches=0;

	for (int k=0;k<nControls;k++) {
		unsigned tindex=cardindex;
		while(tindex<card.nPunch && card.Punch[tindex].Code!=Controls[k]->Numbers[0])
			tindex++;

		if(tindex<card.nPunch){//Control found			
			cardindex=tindex+1;
			matches++;
		}
	}

	if(matches==nControls) {
		//This course is OK. Extra controls?
		return card.nPunch-nControls; //Positive return
	}
	else {
		return matches-nControls; //Negative return;
	}

	return 0;	
}

string oCourse::getLengthS() const
{
  return itos(getLength());
}

void oCourse::setName(const string &n)
{
	if(Name!=n){
		Name=n;
		updateChanged();
	}
}

void oCourse::setLength(int le)
{
  if (le<0 || le > 1000000)
    le = 0;

	if(Length!=le){
		Length=le;
		updateChanged();
	}
}

oDataInterface oCourse::getDI(void)
{
	return oe->oCourseData->getInterface(oData, sizeof(oData), this);
}

oDataConstInterface oCourse::getDCI(void) const
{
	return oe->oCourseData->getConstInterface(oData, sizeof(oData), this);
}

pCourse oEvent::getCourseCreate(int Id)
{
	oCourseList::iterator it;	
	for (it=Courses.begin(); it != Courses.end(); ++it) {
		if(it->Id==Id)
			return &*it;
	}
  if (Id>0) {
    oCourse c(this, Id);
    c.setName(getAutoCourseName());
    return addCourse(c);
  }
  else {
    return addCourse(getAutoCourseName());
  }
}

pCourse oEvent::getCourse(int Id) const
{
  if (Id==0)
    return 0;

  pCourse value;
  if (courseIdIndex.lookup(Id,value))
    return value;

  return 0;
}

pCourse oEvent::getCourse(const string &n) const
{
	oCourseList::const_iterator it;	

	for (it=Courses.begin(); it != Courses.end(); ++it) {
		if(it->Name==n)
			return pCourse(&*it);
	}
	return 0;
}

void oEvent::fillCourses(gdioutput &gdi, const string &id, bool simple)
{
  vector< pair<string, size_t> > d;
  oe->fillCourses(d, simple);
  gdi.addItem(id, d);
}

const vector< pair<string, size_t> > &oEvent::fillCourses(vector< pair<string, size_t> > &out, bool simple)
{	
  out.clear();
	oCourseList::iterator it;	
	synchronizeList(oLCourseId);

  Courses.sort();
	string b;
	for (it=Courses.begin(); it != Courses.end(); ++it) {
		if (!it->Removed){

			if (simple) //gdi.addItem(name, it->Name, it->Id);
        out.push_back(make_pair(it->Name, it->Id));
			else {
				b = it->Name + "\t(" + itos(it->nControls) + ")";
				//sprintf_s(bf, 16, "\t(%d)", it->nControls); 
				//b+=bf;
        if (!it->getCourseProblems().empty())
				  b = "[!] " + b;
        //gdi.addItem(name, b, it->Id);
        out.push_back(make_pair(b, it->Id));
			}
		}
	}
	return out;
}

void oCourse::setNumberMaps(int block)
{
  getDI().setInt("NumberMaps", block);
}

int oCourse::getNumberMaps() const 
{
  return getDCI().getInt("NumberMaps");
}

void oCourse::setStart(const string &start, bool sync)
{
  if (getDI().setString("StartName", start)) {
    if (sync)
      synchronize();
    oClassList::iterator it;
    for (it=oe->Classes.begin();it!=oe->Classes.end();++it) {
      if (it->getCourse()==this) {
        it->setStart(start);
        if (sync)
          it->synchronize();
      }
    }
  }
}

string oCourse::getStart() const 
{
  return getDCI().getString("StartName");
}

void oEvent::calculateNumRemainingMaps()
{
  synchronizeList(oLCourseId);
  synchronizeList(oLRunnerId);

  for (oCourseList::iterator cit=Courses.begin();
    cit!=Courses.end();++cit) {
    int numMaps = cit->getNumberMaps();
    if (numMaps == 0)
      cit->tMapsRemaining = numeric_limits<int>::min();
    else 
      cit->tMapsRemaining = numMaps;
  }
  
	list<oRunner>::const_iterator it;
	for (it=Runners.begin(); it != Runners.end(); ++it)
    if(!it->isRemoved() && it->getStatus()!=StatusDNS){
      pCourse pc = it->getCourse();

      if (pc && pc->tMapsRemaining != numeric_limits<int>::min())
        pc->tMapsRemaining--;	
    }  
}

int oCourse::getIdSum(int nC) {
  
  int id = 0;
  for (int k = 0; k<min(nC, nControls); k++)
    id = 31 * id + (Controls[k] ? Controls[k]->getId() : 0);

  if (id == 0)
    return getId();

  return id;
}

void oCourse::setLegLengths(const vector<int> &legs) {
  if (legs.size() == nControls +1 || legs.empty()) {
    bool diff = legs.size() != legLengths.size();
    if (!diff) {
      for (size_t k = 0; k<legs.size(); k++)
        if (legs[k] != legLengths[k])
          diff = true;
    }
    if (diff) {
      updateChanged();
      legLengths = legs;
    }
  }
  else
    throw std::exception("Invalid parameter value");
}

double oCourse::getPartOfCourse(int start, int end) const 
{
  if (end == 0)
    end = nControls;

  if (legLengths.size() != nControls +1 || start <= end || 
      unsigned(start) >= legLengths.size() || 
      unsigned(end) >= legLengths.size() || Length==0)
    return 0.0;

  int dist = 0;
  for (int k = start; k<end; k++)
    dist += legLengths[k];

  return max(1.0, double(dist) / double(Length));
}


string oCourse::getControlOrdinal(int controlIndex) const
{
  if (controlIndex == nControls)
    return lang.tl("Mål");
  int o = 1;
  bool rogaining = hasRogaining();

  for (int k = 0; k<controlIndex && k<nControls; k++) {
    if (Controls[k] && !Controls[k]->isRogaining(rogaining))
      o++;
  }
  return itos(o);
}

void oCourse::setRogainingPointsPerMinute(int p)
{
  getDI().setInt("RReduction", p);
}

int oCourse::getRogainingPointsPerMinute() const 
{
  return getDCI().getInt("RReduction");
}

void oCourse::setMinimumRogainingPoints(int p)
{
  getDI().setInt("RPointLimit", p);
}

int oCourse::getMinimumRogainingPoints() const 
{
  return getDCI().getInt("RPointLimit");
}

void oCourse::setMaximumRogainingTime(int p)
{
  if (p == NOTIME)
    p = 0;
  getDI().setInt("RTimeLimit", p);
}

int oCourse::getMaximumRogainingTime() const 
{
  return getDCI().getInt("RTimeLimit");
}

bool oCourse::hasRogaining() const {
  return getMaximumRogainingTime() > 0 || getMinimumRogainingPoints() > 0;
}

string oCourse::getCourseProblems() const 
{
  int max_time = getMaximumRogainingTime();
  int min_point = getMinimumRogainingPoints();

  if (max_time > 0) {
    for (int k = 0; k<nControls; k++) {
      if (Controls[k]->isRogaining(true))
        return "";
    }
    return "Banan saknar rogainingkontroller.";
  }
  else if (min_point > 0) {
    int max_p = 0;
    for (int k = 0; k<nControls; k++) {
      if (Controls[k]->isRogaining(true))
        max_p += Controls[k]->getRogainingPoints();
    }
   
    if (max_p < min_point) {
      return "Banans kontroller ger för få poäng för att täcka poängkravet.";
    }
  }
  return "";
}

void oCourse::remove() 
{
  if (oe)
    oe->removeCourse(Id);
}

bool oCourse::canRemove() const 
{
  return !oe->isCourseUsed(Id);
}
