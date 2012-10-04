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
    Stigbergsv�gen 7, SE-75242 UPPSALA, Sweden
    
************************************************************************/

// oControl.cpp: implementation of the oControl class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <algorithm>

#include "oControl.h"
#include "oEvent.h"
#include "gdioutput.h"
#include "meos_util.h"
#include <cassert>
#include "Localizer.h"
#include "Table.h"
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

oControl::oControl(oEvent *poe): oBase(poe)
{
	getDI().initData();
	nNumbers=0;
	Status=StatusOK;
  tMissedTimeMax = 0;
  tMissedTimeTotal = 0;
  tNumVisitors = 0;
  tMissedTimeMedian = 0;
}

oControl::oControl(oEvent *poe, int id): oBase(poe)
{
  Id = id;
	getDI().initData();
	nNumbers=0;
	Status=StatusOK;
  tMissedTimeMax = 0;
  tMissedTimeTotal = 0;
  tNumVisitors = 0;
  tMissedTimeMedian = 0;
}


oControl::~oControl()
{
}

bool oControl::write(xmlparser &xml)
{
	if(Removed) return true;

	xml.startTag("Control");

	xml.write("Id", Id);
	xml.write("Updated", Modified.GetStamp());
	xml.write("Name", Name);
	xml.write("Numbers", codeNumbers());
	xml.write("Status", Status);
	
	getDI().write(xml);
	xml.endTag();

	return true;
}

void oControl::set(int pId, int pNumber, string pName)
{
	Id=pId;
	Numbers[0]=pNumber;
	nNumbers=1;
	Name=pName;
	
	updateChanged();
}


void oControl::setStatus(ControlStatus st){
	if(st!=Status){
		Status=st;
		updateChanged();
	}
}

void oControl::setName(string name)
{
  if(name!=getName()){
		Name=name;
		updateChanged();
	}
}


void oControl::set(const xmlobject *xo)
{
	xmlList xl;
  xo->getObjects(xl);
	nNumbers=0;
	Numbers[0]=0;
	
	xmlList::const_iterator it;

	for(it=xl.begin(); it != xl.end(); ++it){
		if(it->is("Id")){
			Id=it->getInt();			
		}
		else if(it->is("Number")){
			Numbers[0]=it->getInt();			
			nNumbers=1;
		}
		else if(it->is("Numbers")){
			decodeNumbers(it->get());
		}
		else if(it->is("Status")){
			Status=(ControlStatus)it->getInt();			
		}
		else if(it->is("Name")){
			Name=it->get();
		}
		else if(it->is("Updated")){
			Modified.SetStamp(it->get());
		}
    else if(it->is("oData")){
      getDI().set(*it);
		}
	}
}

int oControl::getFirstNumber() const
{
  if (nNumbers > 0)
    return Numbers[0];
  else
    return 0;
}

string oControl::getString()
{
	char bf[32];
	if(Status==StatusOK)
		return codeNumbers('|');
  else if(Status==StatusMultiple)
    return codeNumbers('+');
  else if(Status==StatusRogaining)
    return codeNumbers('|') + ", " + itos(getRogainingPoints()) + "p";
  else
  	sprintf_s(bf, 32, "~%s", codeNumbers().c_str());
	return bf;
}

string oControl::getLongString()
{
	if(Status==StatusOK){
		if(nNumbers==1)
			return codeNumbers('|');
		else
			return string(lang.tl("VALFRI("))+codeNumbers(',')+")";
	}
  else if (Status == StatusMultiple) {
		return string(lang.tl("ALLA("))+codeNumbers(',')+")";
  }
  else if (Status == StatusRogaining) 
    return string(lang.tl("RG("))+codeNumbers(',') + "|" + itos(getRogainingPoints()) +"p)";
	else
    return string(lang.tl("TRASIG("))+codeNumbers(',')+")";
}

bool oControl::hasNumber(int i) 
{
	for(int n=0;n<nNumbers;n++)
    if(Numbers[n]==i) {
      // Mark this number as checked
      checkedNumbers[n]=true;
      return true;
    }
	if(nNumbers>0)
		return false;
	else return true;
}

bool oControl::uncheckNumber(int i) 
{
	for(int n=0;n<nNumbers;n++)
    if(Numbers[n]==i) {
      // Mark this number as checked
      checkedNumbers[n]=false;
      return true;
    }
  return false;
}

bool oControl::hasNumberUnchecked(int i) 
{
	for(int n=0;n<nNumbers;n++)
    if(Numbers[n]==i && checkedNumbers[n]==0) {
      // Mark this number as checked
      checkedNumbers[n]=true;
      return true;
    }
	if(nNumbers>0)
		return false;
	else return true;
}


int oControl::getNumMulti() 
{
  if(Status==StatusMultiple)
    return nNumbers;
  else
    return 1;
}


string oControl::codeNumbers(char sep) const
{
	string n;
	char bf[16];

	for(int i=0;i<nNumbers;i++){
		_itoa_s(Numbers[i], bf, 16, 10);
		n+=bf;
		if(i+1<nNumbers)
			n+=sep;
	}
	return n;
}

bool oControl::decodeNumbers(string s)
{
	const char *str=s.c_str();

	nNumbers=0;

	while(*str){
		int cid=atoi(str);

		while(*str && (*str!=';' && *str!=',' && *str!=' ')) str++;
		while(*str && (*str==';' || *str==',' || *str==' ')) str++;		

		if(cid>0 && cid<1024 && nNumbers<32)
			Numbers[nNumbers++]=cid;
	}

	if(Numbers==0){
		Numbers[0]=0;
		nNumbers=1;
		return false;
	}
	else return true;
}

bool oControl::setNumbers(const string &numbers)
{	
	int nn=nNumbers;
	int bf[32];

	if(unsigned(nNumbers)<32)
		memcpy(bf, Numbers, sizeof(int)*nNumbers);

	bool success=decodeNumbers(numbers);

  if (!success) {
    memcpy(Numbers, bf, sizeof(int)*nn);
    nNumbers = nn;
  }

	if(nNumbers!=nn || memcmp(bf, Numbers, sizeof(int)*nNumbers)!=0)
		updateChanged();

	return success;	
}

string oControl::getName() const
{
	if(!Name.empty())
		return Name;
	else {
		char bf[16];
		sprintf_s(bf, "[%d]", Id);
		return bf;
	}
}

oDataInterface oControl::getDI(void)
{
	return oe->oControlData->getInterface(oData, sizeof(oData), this);
}

oDataConstInterface oControl::getDCI(void) const
{
	return oe->oControlData->getConstInterface(oData, sizeof(oData), this);
}

void oEvent::fillControls(gdioutput &gdi, const string &id, int type)
{
  vector< pair<string, size_t> > d;
  oe->fillControls(d, type);
  gdi.addItem(id, d);
}


const vector< pair<string, size_t> > &oEvent::fillControls(vector< pair<string, size_t> > &out, int type) 
{	
  out.clear();
	oControlList::iterator it;	
	synchronizeList(oLControlId);
  Controls.sort();
	string b;
	char bf[256];
	for (it=Controls.begin(); it != Controls.end(); ++it) {
   	if(!it->Removed){
			b.clear();

      if(type==0) {
        if (it->Status == oControl::StatusFinish || it->Status == oControl::StatusStart) {
          b += it->Name;
        }
        else {
			    if(it->Status==oControl::StatusOK)
				    b+="[OK]\t";
          else if (it->Status==oControl::StatusMultiple)
            b+="[M]\t";
          else if (it->Status==oControl::StatusRogaining)
            b+="[R]\t";
          else if (it->Status==oControl::StatusBad)
            b += MakeDash("[-]\t");
			    else b+="[ ]\t";

			    sprintf_s(bf, " %s", it->codeNumbers(' ').c_str());
			    b+=bf;
          
          if (it->Status==oControl::StatusRogaining)
            b+="\t(" + itos(it->getRogainingPoints()) + "p)";
          else if (it->Name.length()>0) {
				    b+="\t("+it->Name+")";
          }
        }
      }
      else if(type==1) {
        if (it->Status == oControl::StatusFinish || it->Status == oControl::StatusStart) 
          continue;
        
        sprintf_s(bf, lang.tl("Kontroll %s").c_str(), it->codeNumbers(' ').c_str());
			  b=bf;
        
			  if(!it->Name.empty())
				  b+=" ("+it->Name+")";
      }
      out.push_back(make_pair(b, it->Id));
			//gdi.addItem(name, b, it->Id);
		}
	}
	return out;
}

void oEvent::fillControlTypes(gdioutput &gdi, const string &id)
{
  vector< pair<string, size_t> > d;
  oe->fillControlTypes(d);
  gdi.addItem(id, d);
}

const vector< pair<string, size_t> > &oEvent::fillControlTypes(vector< pair<string, size_t> > &out)
{	
	oControlList::iterator it;	
	synchronizeList(oLControlId);
  out.clear();
	//gdi.clearList(name);
	out.clear();
  set<int> sicodes;

	for (it=Controls.begin(); it != Controls.end(); ++it){
    if(!it->Removed) {
      for (int k=0;k<it->nNumbers;k++)
        sicodes.insert(it->Numbers[k]);
    }
	}

  set<int>::iterator sit;
  char bf[32];
  /*gdi.addItem(name, lang.tl("Check"), oPunch::PunchCheck);
  gdi.addItem(name, lang.tl("Start"), oPunch::PunchStart);
  gdi.addItem(name, lang.tl("M�l"), oPunch::PunchFinish);*/
  out.push_back(make_pair(lang.tl("Check"), oPunch::PunchCheck));
  out.push_back(make_pair(lang.tl("Start"), oPunch::PunchStart));
  out.push_back(make_pair(lang.tl("M�l"), oPunch::PunchFinish));

  for (sit = sicodes.begin(); sit!=sicodes.end(); ++sit) {
    sprintf_s(bf, lang.tl("Kontroll %s").c_str(), itos(*sit).c_str());
    //gdi.addItem(name, bf, *sit);
    out.push_back(make_pair(bf, *sit));
  }
	return out;
}

int oControl::getTimeAdjust() const
{
  return getDCI().getInt("TimeAdjust");
}

string oControl::getTimeAdjustS() const
{
  return getTimeMS(getTimeAdjust());
}

int oControl::getMinTime() const
{
  return getDCI().getInt("MinTime");
}

string oControl::getMinTimeS() const
{
  if (getMinTime()>0)
    return getTimeMS(getMinTime());
  else
    return "-";
}

int oControl::getRogainingPoints() const
{
  return getDCI().getInt("Rogaining");
}

string oControl::getRogainingPointsS() const
{
  int pt = getRogainingPoints();
  return pt > 0 ? itos(pt) : "";
}

void oControl::setTimeAdjust(int v)
{
  getDI().setInt("TimeAdjust", v);
}

void oControl::setTimeAdjust(const string &s)
{
  setTimeAdjust(convertAbsoluteTimeMS(s));
}

void oControl::setMinTime(int v)
{
  if (v<0)
    v = 0;
  getDI().setInt("MinTime", v);
}

void oControl::setMinTime(const string &s)
{
  setMinTime(convertAbsoluteTimeMS(s));
}

void oControl::setRogainingPoints(int v)
{
  if (v<0)
    v = 0;
  getDI().setInt("Rogaining", v);
}

void oControl::setRogainingPoints(const string &s)
{
  setRogainingPoints(atoi(s.c_str()));
}

void oControl::startCheckControl()
{
  //Mark all numbers as unchecked.
  for (int k=0;k<nNumbers;k++)
    checkedNumbers[k]=false;
}
 
string oControl::getInfo() const
{
  return getName();
}

void oControl::addUncheckedPunches(vector<int> &mp, bool supportRogaining) const
{
  if(controlCompleted(supportRogaining))
    return;

  for (int k=0;k<nNumbers;k++)
    if(!checkedNumbers[k]) {
      mp.push_back(Numbers[k]);

      if(Status!=StatusMultiple)
        return;
    }
}

int oControl::getMissingNumber() const
{
  for (int k=0;k<nNumbers;k++)
    if (!checkedNumbers[k])
      return Numbers[k];

  assert(false);
  return Numbers[0];//This should not happen
}

bool oControl::controlCompleted(bool supportRogaining) const
{
  if (Status==StatusOK || ((Status == StatusRogaining) && !supportRogaining)) {
    //Check if any number is used.
    for (int k=0;k<nNumbers;k++)
      if(checkedNumbers[k])
        return true;

    //Return true only if there is no control
    return nNumbers==0;
  }
  else if (Status==StatusMultiple) {
    //Check if al numbers are used.
    for (int k=0;k<nNumbers;k++)
      if(!checkedNumbers[k])
        return false;

    return true;
  }
  else return true;
}

void oEvent::setupMissedControlTime() {
  // Reset all times
  for (oControlList::iterator it = Controls.begin(); it != Controls.end(); ++it) {
    it->tMissedTimeMax = 0;
    it->tMissedTimeTotal = 0;
    it->tNumVisitors = 0;
    it->tMissedTimeMedian = 0;
  }

  map<int, vector<int> > lostPerControl;
  vector<int> delta;
  for (oRunnerList::iterator it = Runners.begin(); it != Runners.end(); ++it) {
    if (it->isRemoved())
      continue;
    pCourse pc = it->getCourse();
    if (!pc)
      continue;
    it->getSplitAnalysis(delta);
    int nc = pc->getNumControls();
    if (delta.size()>unsigned(nc)) {
      for (int i = 0; i<nc; i++) {
        pControl ctrl = pc->getControl(i);
        if (ctrl && delta[i]>0) {
          if (delta[i] < 10 * 60)
            ctrl->tMissedTimeTotal += delta[i];
          else
            ctrl->tMissedTimeTotal += 10*60; // Use max 10 minutes

          ctrl->tMissedTimeMax = max(ctrl->tMissedTimeMax, delta[i]);
        }

        if (delta[i] >= 0)
          lostPerControl[ctrl->getId()].push_back(delta[i]);

        ctrl->tNumVisitors++;
      }
    }
  }

  for (oControlList::iterator it = Controls.begin(); it != Controls.end(); ++it) {
    if (!it->isRemoved()) {
      int id = it->getId();

      map<int, vector<int> >::iterator res = lostPerControl.find(id);
      if (res != lostPerControl.end()) {
        sort(res->second.begin(), res->second.end());
        int avg = res->second[res->second.size() / 2];
        it->tMissedTimeMedian = avg;
      }
    }
  }
}

bool oEvent::hasRogaining() const
{
	oControlList::const_iterator it;	
	for (it=Controls.begin(); it != Controls.end(); ++it) {
    if(!it->Removed && it->isRogaining(true))
		  return true;
  }
  return false;
}

const string oControl::getStatusS() const {
  //enum ControlStatus {StatusOK=0, StatusBad=1, StatusMultiple=2, 
  //                    StatusStart = 4, StatusFinish = 5, StatusRogaining = 6};

  switch (getStatus()) {
    case StatusOK:
      return lang.tl("OK");
    case StatusBad:
      return lang.tl("Trasig");
    case StatusMultiple:
      return lang.tl("Multipel");
    case StatusRogaining:
      return lang.tl("Rogaining");
    case StatusStart:
      return lang.tl("Start");
    case StatusFinish:
      return lang.tl("M�l");
    default:
      return lang.tl("Ok�nd");
  }
}

void oEvent::fillControlStatus(gdioutput &gdi, const string& id) const
{
  vector< pair<string, size_t> > d;
  oe->fillControlStatus(d);
  gdi.addItem(id, d);
}


const vector< pair<string, size_t> > &oEvent::fillControlStatus(vector< pair<string, size_t> > &out) const
{
  out.clear();
  out.push_back(make_pair(lang.tl("OK"), oControl::StatusOK));
  out.push_back(make_pair(lang.tl("Multipel"), oControl::StatusMultiple));
  out.push_back(make_pair(lang.tl("Rogaining"), oControl::StatusRogaining));
  out.push_back(make_pair(lang.tl("Trasig"), oControl::StatusBad));
  return out;
}

Table *oEvent::getControlTB()//Table mode
{
  if (tables.count("control") == 0) {
	  Table *table=new Table(this, 20, "Kontroller", "controls");

    table->addColumn("Id", 70, true);
    table->addColumn("�ndrad", 70, false);

	  table->addColumn("Namn", 150, false);
    table->addColumn("Status", 70, false);
    table->addColumn("St�mpelkoder", 100, true);
    table->addColumn("Antal l�pare", 70, true, true);
    table->addColumn("Bomtid (max)", 70, true, true);
    table->addColumn("Bomtid (medel)", 70, true, true);
    table->addColumn("Bomtid (median)", 70, true, true);
    
    oe->oControlData->buildTableCol(table);
    tables["control"] = table;
    table->addOwnership();

    table->setTableProp(Table::CAN_DELETE);
  }

  tables["control"]->update();  
  return tables["control"];
}

void oEvent::generateControlTableData(Table &table, oControl *addControl)
{ 
  if (addControl) {
    addControl->addTableRow(table);
    return;
  }

  synchronizeList(oLControlId);
  setupMissedControlTime();
	oControlList::iterator it;	

  for (it=Controls.begin(); it != Controls.end(); ++it){		
    if(!it->isRemoved()){
      it->addTableRow(table);
 		}
	}
}

void oControl::addTableRow(Table &table) const {
  oControl &it = *pControl(this);
  table.addRow(getId(), &it);		

  int row = 0;
  table.set(row++, it, TID_ID, itos(getId()), false);
  table.set(row++, it, TID_MODIFIED, getTimeStamp(), false);

  table.set(row++, it, TID_CONTROL, getName(), true);
  bool canEdit = getStatus() != oControl::StatusFinish && getStatus() != oControl::StatusStart;
  table.set(row++, it, TID_STATUS, getStatusS(), canEdit, cellSelection);
  table.set(row++, it, TID_CODES, codeNumbers(), true);

  int nv = getNumVisitors();
  table.set(row++, it, 50, itos(getNumVisitors()), false);
  table.set(row++, it, 51, nv > 0 ? formatTime(getMissedTimeMax()) : "-", false);
  table.set(row++, it, 52, nv > 0 ? formatTime(getMissedTimeTotal()/nv) : "-", false);
  table.set(row++, it, 53, nv > 0 ? formatTime(getMissedTimeMedian()) : "-", false);
  	
  oe->oControlData->fillTableCol(oData, it, table, true);
}

bool oControl::inputData(int id, const string &input, 
                       int inputId, string &output, bool noUpdate)
{
  synchronize(false);
    
  if(id>1000) {
    return oe->oControlData->inputData(this, oData, id, input, inputId, output, noUpdate);
  }
  switch(id) {
    case TID_CONTROL:
      setName(input);
      synchronize();
      output=getName();
      return true;
    case TID_STATUS:
      setStatus(ControlStatus(inputId));
      synchronize(true);
      output = getStatusS();
      return true;
    case TID_CODES:
      bool stat = setNumbers(input);
      synchronize(true);
      output = codeNumbers();
      return stat;
    break;
  }

  return false;
}

void oControl::fillInput(int id, vector< pair<string, size_t> > &out, size_t &selected)
{ 
  if(id>1000) {
    oe->oControlData->fillInput(oData, id, 0, out, selected);
    return;
  }

  if (id==TID_STATUS) {
    oe->fillControlStatus(out);
    selected = getStatus();
  }  
}

void oControl::remove() 
{
  if (oe)
    oe->removeControl(Id);
}

bool oControl::canRemove() const 
{
  return !oe->isControlUsed(Id);
}
