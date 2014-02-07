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
    GNU General Public License fro more details.

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
#include <limits>

#include "oEvent.h"
#include "gdioutput.h"
#include "gdifonts.h"
#include "oDataContainer.h"
#include "MetaList.h"

#include "random.h"
#include "SportIdent.h"

#include "meosException.h"
#include "oFreeImport.h"
#include "TabBase.h"
#include "meos.h"
#include "meos_util.h"
#include "RunnerDB.h"
#include "localizer.h"
#include "progress.h"
#include "intkeymapimpl.hpp"
#include "meosdb/sqltypes.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>
#include "Table.h"

//Version of database
int oEvent::dbVersion = 68;

oEvent::oEvent(gdioutput &gdi):oBase(0), gdibase(gdi)
{
  readOnly = false;
  hMod=0;
	ZeroTime=0;
  vacantId = 0;
  dataRevision = 0;
  sqlCounterRunners=0;
	sqlCounterClasses=0;
	sqlCounterCourses=0;
	sqlCounterControls=0;
	sqlCounterClubs=0;
	sqlCounterCards=0;
	sqlCounterPunches=0;
	sqlCounterTeams=0;

  initProperties();

#ifndef MEOSDB
  listContainer = new MetaListContainer(this);
#else
  throw std::exception();
#endif


  tCurrencyFactor = 1;
  tCurrencySymbol = "kr";
  tCurrencySeparator = ",";
  tCurrencyPreSymbol = false;

  tClubDataRevision = -1;

  nextFreeStartNo = 0;

	SYSTEMTIME st;
	GetLocalTime(&st);

	char bf[64];
	sprintf_s(bf, 64, "%d-%02d-%02d", st.wYear, st.wMonth, st.wDay);
	
	Date=bf;
	ZeroTime=st.wHour*3600;
	oe=this;

  runnerDB = new RunnerDB(this);

  char cp[64];
  DWORD size=64;
  GetComputerName(cp, &size);
  clientName=cp;

	HasDBConnection = false;
  HasPendingDBConnection = false;

#ifdef BUILD_DB_DLL
	msSynchronizeList=0;
	msSynchronizeRead=0;
	msSynchronizeUpdate=0;
	msOpenDatabase=0;
  msRemove=0;
  msMonitor=0;
  msUploadRunnerDB = 0;

  msGetErrorState=0;
  msResetConnection=0;
  msReConnect=0;
#endif

  nextTimeLineEvent = 0;
	//These object must be initialized on creation of any oObject,
	//but we need to create (dummy) objects to get the sizeof their 
	//oData[]-sets...

  // --- REMEMBER TO UPDATE dvVersion when these are changed.

  oEventData=new oDataContainer(sizeof(oData));
	oEventData->initData(oData, sizeof(oData));
  oEventData->addVariableCurrency("CardFee", "Brickhyra");
  oEventData->addVariableCurrency("EliteFee", "Elitavgift");
  oEventData->addVariableCurrency("EntryFee", "Normalavgift");
  oEventData->addVariableCurrency("YouthFee", "Ungdomsavgift");
  oEventData->addVariableInt("YouthAge", oDataContainer::oIS8U, "�ldersgr�ns ungdom");
  oEventData->addVariableInt("SeniorAge", oDataContainer::oIS8U, "�ldersgr�ns �ldre");
  
  oEventData->addVariableString("Account", 30, "Konto");
  oEventData->addVariableDate("PaymentDue", "Sista betalningsdatum");
  oEventData->addVariableDate("OrdinaryEntry", "Ordinarie anm�lningsdatum");
  oEventData->addVariableString("LateEntryFactor", 6, "Avgiftsh�jning (procent)");
  
  oEventData->addVariableString("Organizer", 32, "Arrang�r");
	oEventData->addVariableString("CareOf", 31, "c/o");

  oEventData->addVariableString("Street", 32, "Adress");
  oEventData->addVariableString("Address", 32, "Postadress");
  oEventData->addVariableString("EMail", 64, "E-post");
  oEventData->addVariableString("Homepage", 40, "Hemsida");
	oEventData->addVariableString("Phone", 32, "Telefon");
 
  oEventData->addVariableInt("UseEconomy", oDataContainer::oIS8U, "Ekonomi");
  oEventData->addVariableInt("UseSpeaker", oDataContainer::oIS8U, "Speaker");
  oEventData->addVariableInt("ExtId", oDataContainer::oIS64, "Externt Id");

  oEventData->addVariableInt("MaxTime", oDataContainer::oISTime, "Maxtid");

  oEventData->addVariableString("PreEvent", sizeof(CurrentNameId), "");
  oEventData->addVariableString("PostEvent", sizeof(CurrentNameId), "");

  // Positive number -> stage number, negative number -> no stage number. Zero = unknown
  oEventData->addVariableInt("EventNumber", oDataContainer::oIS8, "");

  oEventData->addVariableInt("CurrencyFactor", oDataContainer::oIS16, "Valutafaktor");
  oEventData->addVariableString("CurrencySymbol", 5, "Valutasymbol");
  oEventData->addVariableString("CurrencySeparator", 2, "Decimalseparator");
  oEventData->addVariableInt("CurrencyPreSymbol", oDataContainer::oIS8, "Symboll�ge");
  oEventData->addVariableString("CurrencyCode", 5, "Valutakod");
  oEventData->addVariableInt("UTC", oDataContainer::oIS8, "UTC");
  
	oDataContainer dummy(0);
	oClubData=oRunnerData=oClassData=oCourseData=oControlData=&dummy;
	oTeamData=&dummy;

	oClub oc(this);
	oClubData=new oDataContainer(sizeof (oc.oData));
  oClubData->addVariableInt("District", oDataContainer::oIS32, "Organisation");
	
	oClubData->addVariableString("ShortName", 8, "Kortnamn");
	oClubData->addVariableString("CareOf", 31, "c/o");
	oClubData->addVariableString("Street", 41, "Gata");
	oClubData->addVariableString("City", 23, "Stad");
  oClubData->addVariableString("State", 23, "Region");
	oClubData->addVariableString("ZIP", 11, "Postkod");
	oClubData->addVariableString("EMail", 64, "E-post");
	oClubData->addVariableString("Phone", 32, "Telefon");
	oClubData->addVariableString("Nationality", 3, "Nationalitet");
  oClubData->addVariableString("Country", 23, "Land");
  oClubData->addVariableString("Type", 20, "Typ");
  oClubData->addVariableInt("ExtId", oDataContainer::oIS64, "Externt Id");

  vector< pair<string,string> > eInvoice;
  eInvoice.push_back(make_pair("E", "Elektronisk"));
  eInvoice.push_back(make_pair("A", "Elektronisk godk�nd"));
  eInvoice.push_back(make_pair("P", "Ej elektronisk"));
  eInvoice.push_back(make_pair("", MakeDash("-")));
  oClubData->addVariableEnum("Invoice", 1, "Faktura", eInvoice);
	

	oRunner or(this);
	oRunnerData=new oDataContainer(sizeof (or.oData));
	oRunnerData->addVariableCurrency("Fee", "Anm. avgift");
	oRunnerData->addVariableCurrency("CardFee", "Brickhyra");
	oRunnerData->addVariableCurrency("Paid", "Betalat");
  oRunnerData->addVariableCurrency("Taxable", "Skattad avgift");

	oRunnerData->addVariableInt("BirthYear", oDataContainer::oIS32, "F�delse�r");	
	oRunnerData->addVariableString("Bib", 8, "Nummerlapp");
	oRunnerData->addVariableInt("Rank", oDataContainer::oIS16U, "Ranking");
	oRunnerData->addVariableInt("VacRank", oDataContainer::oIS16U, "Vak. ranking");

	oRunnerData->addVariableDate("EntryDate", "Anm. datum");
	
  vector< pair<string,string> > sex;
  sex.push_back(make_pair("M", "Man"));
  sex.push_back(make_pair("F", "Kvinna"));
  sex.push_back(make_pair("", MakeDash("-")));

	oRunnerData->addVariableEnum("Sex", 1, "K�n", sex);
	oRunnerData->addVariableString("Nationality", 3, "Nationalitet");
  oRunnerData->addVariableString("Country", 23, "Land");
  oRunnerData->addVariableInt("ExtId", oDataContainer::oIS64, "Externt Id");
  oRunnerData->addVariableInt("Priority", oDataContainer::oIS8U, "Prioritering");
  oRunnerData->addVariableString("Phone", 20, "Telefon");

/*oRunnerData->addVariableCurrency("InputFee", "Avgift (in)");
	oRunnerData->addVariableCurrency("InputCardFee", "Brickhyra (in)");
	oRunnerData->addVariableCurrency("InputPaid", "Betalat (in)");
*/

	oControl ocn(this);
	oControlData=new oDataContainer(sizeof (ocn.oData));
  oControlData->addVariableInt("TimeAdjust", oDataContainer::oIS32, "Tidsjustering");
  oControlData->addVariableInt("MinTime", oDataContainer::oIS32, "Minitid");
  oControlData->addVariableDecimal("xpos", "x", 1);
  oControlData->addVariableDecimal("ypos", "y", 1);
  oControlData->addVariableDecimal("latcrd", "Latitud", 6);
  oControlData->addVariableDecimal("longcrd", "Longitud", 6);

  oControlData->addVariableInt("Rogaining", oDataContainer::oIS32, "Po�ng");
  
	oCourse ocr(this);
	oCourseData=new oDataContainer(sizeof (ocr.oData));
  oCourseData->addVariableInt("NumberMaps", oDataContainer::oIS16, "Kartor");
  oCourseData->addVariableString("StartName", 16, "Start");
	oCourseData->addVariableInt("Climb", oDataContainer::oIS16, "Stigning");
	oCourseData->addVariableInt("StartIndex", oDataContainer::oIS32, "Startindex");
	oCourseData->addVariableInt("FinishIndex", oDataContainer::oIS32, "M�lindex");
	oCourseData->addVariableInt("RPointLimit", oDataContainer::oIS32, "Po�nggr�ns");
	oCourseData->addVariableInt("RTimeLimit", oDataContainer::oIS32, "Tidsgr�ns");
	oCourseData->addVariableInt("RReduction", oDataContainer::oIS32, "Po�ngreduktion");
  oCourseData->addVariableInt("RReductionMethod", oDataContainer::oIS8U, "Reduktionsmetod");
	
  oCourseData->addVariableInt("FirstAsStart", oDataContainer::oIS8U, "Fr�n f�rsta");
  oCourseData->addVariableInt("LastAsFinish", oDataContainer::oIS8U, "Till sista");
  
	oClass ocl(this);
	oClassData=new oDataContainer(sizeof (ocl.oData));

  oClassData->addVariableString("LongName", 32, "L�ngt namn");
	oClassData->addVariableInt("LowAge", oDataContainer::oIS8U, "Undre �lder");
	oClassData->addVariableInt("HighAge", oDataContainer::oIS8U, "�vre �lder");
  oClassData->addVariableInt("HasPool", oDataContainer::oIS8U, "Banpool");
  oClassData->addVariableInt("AllowQuickEntry", oDataContainer::oIS8U, "Direktanm�lan");
  
	oClassData->addVariableString("ClassType", 40, "Klasstyp");

  vector< pair<string,string> > sexClass;
  sexClass.push_back(make_pair("M", "M�n"));
  sexClass.push_back(make_pair("F", "Kvinnor"));
  sexClass.push_back(make_pair("B", "Alla"));
  sexClass.push_back(make_pair("", MakeDash("-")));

  oClassData->addVariableEnum("Sex", 1, "K�n", sexClass);
	oClassData->addVariableString("StartName", 16, "Start");
	oClassData->addVariableInt("StartBlock", oDataContainer::oIS8U, "Block");
  oClassData->addVariableInt("NoTiming", oDataContainer::oIS8U, "Ej tidtagning");
  oClassData->addVariableInt("FreeStart", oDataContainer::oIS8U, "Fri starttid");

  oClassData->addVariableInt("FirstStart", oDataContainer::oIS32, "F�rsta start");
  
  oClassData->addVariableCurrency("ClassFee", "Anm. avgift");
  oClassData->addVariableCurrency("HighClassFee", "Efteranm. avg.");
  oClassData->addVariableCurrency("ClassFeeRed", "Reducerad avg.");
  oClassData->addVariableCurrency("HighClassFeeRed", "Red. avg. efteranm.");
  
  oClassData->addVariableInt("SortIndex", oDataContainer::oIS32, "Sortering");
  oClassData->addVariableInt("MaxTime", oDataContainer::oISTime, "Maxtid");

  vector< pair<string,string> > statusClass;
  statusClass.push_back(make_pair("", "OK"));
  statusClass.push_back(make_pair("IR", "Struken med �terbetalning"));
  statusClass.push_back(make_pair("I", "Struken utan �terbetalning"));

  oClassData->addVariableEnum("Status", 2, "Status", statusClass);

	oTeam ot(this);
	oTeamData = new oDataContainer(sizeof (ot.oData));
	oTeamData->addVariableCurrency("Fee", "Anm. avgift");
	oTeamData->addVariableCurrency("Paid", "Betalat");
  oTeamData->addVariableCurrency("Taxable", "Skattad avgift");
	oTeamData->addVariableDate("EntryDate", "Anm. datum");
	oTeamData->addVariableString("Nationality", 3, "Nationalitet");
  oTeamData->addVariableString("Country", 23, "Land");
	oTeamData->addVariableString("Bib", 8, "Nummerlapp");
	oTeamData->addVariableInt("ExtId", oDataContainer::oIS64, "Externt Id");
  oTeamData->addVariableInt("Priority", oDataContainer::oIS8U, "Prioritering");
  oTeamData->addVariableInt("SortIndex", oDataContainer::oIS16, "Sortering");

  currentClientCS = 0;
  memset(CurrentFile, 0, sizeof(CurrentFile));
}

oEvent::~oEvent()
{
	//Clean up things in the right order.
	clear();
  delete runnerDB;
  runnerDB=0;

  if (hMod) {
    HasDBConnection=false;
    FreeLibrary(hMod);
    hMod=0;
  }

	delete oEventData;
	delete oRunnerData;
	delete oClubData;
	delete oControlData;
	delete oCourseData;
	delete oClassData;
	delete oTeamData;	

  delete listContainer;

  return;
}

void oEvent::initProperties() 
{
  setProperty("TextSize", getPropertyString("TextSize", "0"));
  setProperty("Language", getPropertyString("Language", "103"));

  setProperty("Interactive", getPropertyString("Interactive", "1"));
  setProperty("Database", getPropertyString("Database", "1"));
}

pControl oEvent::addControl(int Id, int Number, const string &Name)
{
	if(Id<=0)
		Id=getFreeControlId();
  else
    qFreeControlId = max (qFreeControlId, Id);

	oControl c(this);
	c.set(Id, Number, Name);
	Controls.push_back(c);

  oe->updateTabs();
	return &Controls.back();
}

int oEvent::getNextControlNumber() const
{
  int c = 31;
  for (oControlList::const_iterator it = Controls.begin(); it!=Controls.end(); ++it)
    c = max(c, it->maxNumber()+1);

  return c;
}

pControl oEvent::addControl(const oControl &oc)
{
  if(oc.Id<=0)
		return 0;

  if (getControl(oc.Id, false))
    return 0;
    
  qFreeControlId = max (qFreeControlId, Id);

	Controls.push_back(oc);
	return &Controls.back();
}

const int legHashConstant = 100000;

int oEvent::getControlIdFromPunch(int time, int type, int card, bool markClassChanged, int &runnerId)
{
  pRunner r=getRunnerByCard(card);

  if (markClassChanged && r && r->Class) {
    r->Class->sqlChanged = true;
    classChanged(r->Class, true);
  }
  if (r!=0) {
    runnerId = r->Id;
    r = r->tParentRunner ? r->tParentRunner : r;
  }
  int race = 0;
  // Try to figure out to which (multi)runner punch belong
  // by looking between which start and finnish the time is
  if (r && r->getNumMulti()) {
    if (markClassChanged && r->Class) {
      r->Class->sqlChanged = true; //Mark class as changed.
      classChanged(r->Class, true);
    }
    pRunner r2 = r;
    while (r2 && r2->Card && race<r->getNumMulti()) {
      if (time >= r2->getStartTime() && time <= r2->getFinishTime())
        break;
      
      race++;
      r2=r->getMultiRunner(race);
    }
  }

  if (type!=oPunch::PunchFinish) {  
    pCourse c = r ? r->getCourse(): 0;

    if (c!=0) {
      for (int k=0; k<c->nControls; k++) {
        pControl ctrl=c->getControl(k);
        if(ctrl && ctrl->hasNumber(type)) {
          pFreePunch p = getPunch(race, ctrl->getId(), card);
          if(!p || (p && abs(p->Time-time)<60)) {
            //Code controlId and runner race number into code
            return ctrl->getId() + race*legHashConstant;
          }          
        }
      }
    }
  }
  
  return type + race*legHashConstant; 
}

void oEvent::getFreeControls(set<int> &controlId) const
{
  controlId.clear();
  for (map<int, intkeymap<pFreePunch> >::const_iterator it = punchIndex.begin(); it != punchIndex.end(); ++it) {
    int id = it->first % legHashConstant;
    controlId.insert(id);
  }
}

pFreePunch oEvent::addFreePunch(int time, int type, int card)
{	
  //int runnerId = 0;
  //int itype = getControlIdFromPunch(time, type, card, true, runnerId);

	oFreePunch ofp(this, card, time, type);

	punches.push_back(ofp);	
	pFreePunch fp=&punches.back();
  oFreePunch::rehashPunches(*this, card, fp);
  //fp->itype = itype;
  //fp->tRunnerId = runnerId;
  //punchIndex[itype][card]=fp;
  
  fp->updateChanged();
  fp->synchronize();
  return fp;
}

pFreePunch oEvent::addFreePunch(oFreePunch &fp)
{		
//  int runnerId = 0;
//  int itype = getControlIdFromPunch(fp.Time, fp.Type, fp.CardNo, true, runnerId);

	punches.push_back(fp);	
	pFreePunch fpz=&punches.back();
  oFreePunch::rehashPunches(*this, fp.CardNo, fpz);
//  fpz->itype = itype;
//  fpz->tRunnerId = runnerId;
//  punchIndex[itype][fp.CardNo]=fpz;
  if (!fpz->existInDB() && HasDBConnection) {
    fpz->changed = true;
    fpz->synchronize();
  }
  return fpz;
}

void oEvent::rebuildPunchIndex()
{
}

void oEvent::removeFreePunch(int Id)
{
	oFreePunchList::iterator it;	

	for (it=punches.begin(); it != punches.end(); ++it) {
    if(it->Id==Id) {
      pRunner r = getRunner(it->tRunnerId, 0);
      if (r)
        r->markClassChanged();

      if(HasDBConnection) 
        msRemove(&*it);
      punchIndex[it->itype].remove(it->CardNo);
      punches.erase(it);
      oFreePunch::rehashPunches(*this, it->CardNo, 0);
      dataRevision++;
			return;
    }
	}
}

pFreePunch oEvent::getPunch(int Id) const
{
	oFreePunchList::const_iterator it;	

	for (it=punches.begin(); it != punches.end(); ++it) {
    if(it->Id==Id) {
      if (it->isRemoved())
        return 0;
			return const_cast<pFreePunch>(&*it);
    }
	}
	return 0;
}

pFreePunch oEvent::getPunch(int runnerRace, int type, int card) const
{
  //Lazy setup
  oFreePunch::rehashPunches(*oe, 0, 0);
  
  map<int, intkeymap<pFreePunch> >::const_iterator it1;
    
  it1=punchIndex.find(type + runnerRace*legHashConstant);

  if (it1!=punchIndex.end()) {
    map<int, pFreePunch>::const_iterator it2;
    const intkeymap<pFreePunch> &cIndex=it1->second;

    //it2=cIndex.find(card);
    pFreePunch value;
    if(cIndex.lookup(card, value) && value && !value->isRemoved())
      return value;
    else return 0;
  }
	return 0;
}

void oEvent::getPunchesForRunner(int runnerId, vector<pFreePunch> &runnerPunches) const {
  runnerPunches.clear();
  pRunner r = getRunner(runnerId, 0);
  if (r == 0)
    return;

  // Get times for when other runners used this card
  vector< pair<int, int> > times;

  for (oRunnerList::const_iterator it = Runners.begin(); it != Runners.end(); ++it) {
    if (it->Id == runnerId)
      continue;
    if (it->Card && it->CardNo == r->CardNo) {
      pair<int, int> t = it->Card->getTimeRange();
      if (it->getStartTime() > 0)
        t.first = min(it->getStartTime(), t.first);

      if (it->getFinishTime() > 0)
        t.second = max(it->getFinishTime(), t.second);

      times.push_back(t);
    }
  }

  for (oFreePunchList::const_iterator it = punches.begin(); it != punches.end(); ++it) {
    if (it->CardNo == r->CardNo) {
      bool other = false;
      int t = it->Time;
      for (size_t k = 0; k<times.size(); k++) {
        if (t >= times[k].first && t <= times[k].second)
          other = true;
      }

      if (!other)
        runnerPunches.push_back(pFreePunch(&*it));
    }
  }
}


pControl oEvent::getControl(int Id, bool create)
{
	oControlList::iterator it;	

	for (it=Controls.begin(); it != Controls.end(); ++it) {
		if(it->Id==Id)
			return &*it;
	}

  if(!create || Id<=0)
    return 0;

	//Not found. Auto add...
	return addControl(Id, Id, "");
}

bool oEvent::existControl(int Id)
{
	oControlList::iterator it;	

	for (it=Controls.begin(); it != Controls.end(); ++it){
		if(it->Id==Id)
			return true;
	}
	return false;
}

bool oEvent::existCourse(int Id)
{
	oCourseList::iterator it;	

	for (it=Courses.begin(); it != Courses.end(); ++it){
		if(it->Id==Id)
			return true;
	}
	return false;
}

bool oEvent::existClass(int Id)
{
	oClassList::iterator it;	

	for (it=Classes.begin(); it != Classes.end(); ++it){
		if(it->Id==Id)
			return true;
	}
	return false;
}

bool oEvent::existClub(int Id)
{
	oClubList::iterator it;	

	for (it=Clubs.begin(); it != Clubs.end(); ++it){
		if(it->Id==Id)
			return true;
	}
	return false;
}


bool oEvent::existRunner(int Id)
{
	oRunnerList::iterator it;	

	for (it=Runners.begin(); it != Runners.end(); ++it){
		if(it->Id==Id)
			return true;
	}
	return false;
}

bool oEvent::writeControls(xmlparser &xml)
{
	oControlList::iterator it;	

	xml.startTag("ControlList");

	for (it=Controls.begin(); it != Controls.end(); ++it)
		it->write(xml);

	xml.endTag();
 
	return true;
}

bool oEvent::writeCourses(xmlparser &xml)
{
	oCourseList::iterator it;	

	xml.startTag("CourseList");

	for (it=Courses.begin(); it != Courses.end(); ++it)
		it->Write(xml);

	xml.endTag();
 
	return true;
}

bool oEvent::writeClasses(xmlparser &xml)
{
	oClassList::iterator it;	

	xml.startTag("ClassList");

	for (it=Classes.begin(); it != Classes.end(); ++it)
		it->Write(xml);

	xml.endTag();
 
	return true;
}

bool oEvent::writeClubs(xmlparser &xml)
{
	oClubList::iterator it;	

	xml.startTag("ClubList");

	for (it=Clubs.begin(); it != Clubs.end(); ++it)
		it->write(xml);

	xml.endTag();
 
	return true;
}

bool oEvent::writeRunners(xmlparser &xml, ProgressWindow &pw)
{
	oRunnerList::iterator it;	

	xml.startTag("RunnerList");
  int k=0;
  for (it=Runners.begin(); it != Runners.end(); ++it) {
    if(!it->tDuplicateLeg) //Duplicates is written by the ruling runner.
		  it->Write(xml);
    if (++k%300 == 200)
      pw.setSubProgress( (1000*k)/ Runners.size());
  }
	xml.endTag();
 
	return true;
}


bool oEvent::writePunches(xmlparser &xml, ProgressWindow &pw)
{
	oFreePunchList::iterator it;	

	xml.startTag("PunchList");
  int k = 0;
  for (it=punches.begin(); it != punches.end(); ++it) {
		it->Write(xml);
    if (++k%300 == 200)
      pw.setSubProgress( (1000*k)/ Runners.size());
  }
	xml.endTag();
 
	return true;
}

//Write free cards not owned by a runner.
bool oEvent::writeCards(xmlparser &xml)
{
	oCardList::iterator it;	

	xml.startTag("CardList");

  for (it=Cards.begin(); it != Cards.end(); ++it) {
    if(it->getOwner() == 0)
		  it->Write(xml);
  }

	xml.endTag(); 
	return true;
}

void oEvent::duplicate() {
  char file[260];
	char filename[64];
	char nameid[64];

  SYSTEMTIME st;
	GetLocalTime(&st);

  sprintf_s(filename, 64, "meos_%d%02d%02d_%02d%02d%02d_%X.meos", 
		st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);	
	
	getUserFile(file, filename);
  
  _splitpath_s(filename, NULL, 0, NULL,0, nameid, 64, NULL, 0);
  int i=0;
  while (nameid[i]) {
    if(nameid[i]=='.') { 
      nameid[i]=0;
      break;
    }
    i++;
  }

  char oldFile[260];
  char oldId[64];
  strcpy_s(oldFile, CurrentFile);
  strcpy_s(oldId, CurrentNameId);
  string oldAnno = getAnnotation();

  strcpy_s(CurrentFile, file);
  strcpy_s(CurrentNameId, nameid);

  sprintf_s(filename, "%d/%d %d:%02d", 
		                  st.wDay, st.wMonth, st.wHour, st.wMinute);	

  string anno = lang.tl("Kopia (X)#" + string(filename));
  anno = oldAnno.empty() ? anno : oldAnno + " " + anno;
  setAnnotation(anno);

  try {
    save();
  }
  catch(...) {
    // Restore in case of error
    strcpy_s(CurrentFile, oldFile);
    strcpy_s(CurrentNameId, oldId);
    setAnnotation(oldAnno);
    throw;
  }

  // Restore
  strcpy_s(CurrentFile, oldFile);
  strcpy_s(CurrentNameId, oldId);
  setAnnotation(oldAnno);
}

bool oEvent::save()
{
	if(empty())
		return true;

  autoSynchronizeLists(true);

	if (!CurrentFile[0]) 
    throw std::exception("Felaktigt filnamn");
	
	int f=0;
  _sopen_s(&f, CurrentFile, _O_RDONLY, _SH_DENYNO, _S_IWRITE);
	
	char fn1[260];
	char fn2[260];
  
  //vector<time_t> ages;
  if(f!=-1) {
    
		_close(f);
    time_t currentTime = time(0);
    const int baseAge = 3; // Three minutes
    time_t allowedAge = baseAge*60;
    time_t oldAge = allowedAge + 60;
    const int maxBackup = 8;
    int toDelete = maxBackup;
	
		for(int k = 0; k <= maxBackup; k++) {
			sprintf_s(fn1, MAX_PATH, "%s.bu%d", CurrentFile, k);
      struct stat st;
      int ret = stat(fn1, &st);
      if (ret==0) {
        time_t age = currentTime - st.st_mtime;
        // If file is too young or to old at its
        // position, it is possible to delete.
        // The oldest old file (or youngest young file if none is old) 
        // possible to delete is deleted.
        // If no file is possible to delete, the oldest 
        // file is deleted.
        if ( (age<allowedAge && toDelete==maxBackup) || age>oldAge)
          toDelete = k;
        allowedAge *= 2;
        oldAge*=2;

        if (k==maxBackup-3)
          oldAge = 24*3600; // Allow a few old copies
      }
      else {
        toDelete = k; // File does not exist. No file need be deleted
        break;
      }
    }

		sprintf_s(fn1, MAX_PATH, "%s.bu%d", CurrentFile, toDelete);
    ::remove(fn1);

		for(int k=toDelete;k>0;k--) {
			sprintf_s(fn1, MAX_PATH, "%s.bu%d", CurrentFile, k-1);
			sprintf_s(fn2, MAX_PATH, "%s.bu%d", CurrentFile, k);
			rename(fn1, fn2);
		}

		rename(CurrentFile, fn1);
	}

	return save(CurrentFile);
}

bool oEvent::save(const char *file)
{
	xmlparser xml;
  ProgressWindow pw(gdibase.getHWND());

  if (Runners.size()>200)
    pw.init();

	xml.openOutputT(file, true, "meosdata");
  
	xml.write("Name", Name);
	xml.write("Date", Date);
	xml.write("ZeroTime", ZeroTime);
  xml.write("NameId", CurrentNameId);
	xml.write("Annotation", Annotation);
	xml.write("Id", Id);
	writeExtraXml(xml);
	xml.write("Updated", Modified.getStamp());
	oEventData->write(oData, xml);

  int i = 0;
  vector<int> p;
  p.resize(10);
  p[0] = 2; //= {2, 20, 50, 80, 180, 400,500,700,800,1000};
  p[1] = Controls.size();
  p[2] = Courses.size();
  p[3] = Classes.size();
  p[4] = Clubs.size();
  p[5] = Runners.size() + Cards.size();
  p[6] = Teams.size();
  p[7] = punches.size();
  p[8] = Cards.size();
  p[9] = Runners.size()/2;
  
  int sum = 0;
  for (int k = 0; k<10; k++)
    sum += p[k];

  for (int k = 1; k<10; k++)
    p[k] = p[k-1] + (1000 * p[k]) / sum;
  
  p[9] = 1000;

  pw.setProgress(p[i++]);
	writeControls(xml);
	pw.setProgress(p[i++]);
  writeCourses(xml);
  pw.setProgress(p[i++]);
	writeClasses(xml);
  pw.setProgress(p[i++]);
	writeClubs(xml);
  pw.initSubProgress(p[i], p[i+1]);
  pw.setProgress(p[i++]);
  writeRunners(xml, pw);
  pw.setProgress(p[i++]);
	writeTeams(xml);
  pw.initSubProgress(p[i], p[i+1]);
  pw.setProgress(p[i++]);
	writePunches(xml, pw);
  pw.setProgress(p[i++]);
  writeCards(xml);
  
  xml.startTag("Lists");
  listContainer->save(MetaListContainer::ExternalList, xml);
  xml.endTag();

  xml.closeOut();
  pw.setProgress(p[i++]);
	updateRunnerDatabase();
	pw.setProgress(p[i++]);

  return true;
}

string oEvent::getNameId(int id) const {
  if (id == 0)
    return CurrentNameId;

	list<CompetitionInfo>::const_iterator it;
	for (it=cinfo.begin(); it!=cinfo.end(); ++it) {		
  	if (it->Server.empty()) {
      if(id == it->Id) 
        return it->NameId;
    }
		else if(!it->Server.empty()) {			
      if(id == (10000000+it->Id)) {			
        return it->NameId;
      }
		}
	}
  return _EmptyString;
}

const string &oEvent::getFileNameFromId(int id) const {

	list<CompetitionInfo>::const_iterator it;
	for (it=cinfo.begin(); it!=cinfo.end(); ++it) {		
  	if (it->Server.empty()) {
      if(id == it->Id) 
        return it->FullPath;
    }
		else if(!it->Server.empty()) {			
      if(id == (10000000+it->Id)) {			
        return _EmptyString;
      }
		}
	}
  return _EmptyString;
}


bool oEvent::open(int id)
{
	list<CompetitionInfo>::iterator it;

	for (it=cinfo.begin(); it!=cinfo.end(); ++it) {		
  	if (it->Server.empty()) {
      if(id == it->Id) {
        CompetitionInfo ci=*it; //Take copy
				return open(ci.FullPath.c_str());
      }
    }
		else if(!it->Server.empty()) {			
      if(id == (10000000+it->Id)) {			
        CompetitionInfo ci=*it; //Take copy
				return readSynchronize(ci);
      }
		}
	}

	return false;
}

static DWORD timer;
static string mlog;

static void tic() {
  timer = GetTickCount();
  mlog.clear();
}

static void toc(const string &str) {
  DWORD t = GetTickCount();
  if (!mlog.empty())
    mlog += ",\n";
  else
    mlog = "Tid (hundradels sekunder):\n";

  mlog += str + "=" + itos( (t-timer)/10 );
  timer = t;
}

bool oEvent::open(const string &file, bool Import)
{
	xmlparser xml;
  xml.setProgress(gdibase.getHWND());
  tic();
  string log;
	xml.read(file);

  toc("parse");
	//This generates a new file name
	newCompetition("-");
	
  if (!Import) {
    strcpy_s(CurrentFile, MAX_PATH, file.c_str()); //Keep new file name, if imported
 
    _splitpath_s(CurrentFile, NULL, 0, NULL,0, CurrentNameId, 64, NULL, 0);
    int i=0;
    while (CurrentNameId[i]) {
      if(CurrentNameId[i]=='.') {
        CurrentNameId[i]=0;
        break;
      }
      i++;
    }
  }
  return open(xml);
}

void oEvent::restoreBackup() 
{
  string cfile = string(CurrentFile) + ".meos";
  strcpy_s(CurrentFile, cfile.c_str());
}

bool oEvent::open(const xmlparser &xml) {
 	xmlobject xo;
 
	xo = xml.getObject("Date");
	if(xo) Date=xo.get();

	xo = xml.getObject("Name");
	if(xo) Name=xo.get();
  tOriginalName.clear();
	
  xo = xml.getObject("Annotation");
  if(xo) Annotation = xo.get();
  
  xo=xml.getObject("ZeroTime");
  ZeroTime=0;
	if(xo) ZeroTime=xo.getInt();

	xo=xml.getObject("Id");
	if(xo) Id=xo.getInt();

	readExtraXml(xml);

	xo=xml.getObject("oData");

	if(xo)
		oEventData->set(oData, xo);

  setCurrency(-1, "", ",", false);

  xo = xml.getObject("NameId");
  if (xo) 
    strncpy_s(CurrentNameId, xo.get(), sizeof(CurrentNameId));
  
  toc("event");
	//Get controls
	xo = xml.getObject("ControlList");
	if(xo){
	  xmlList xl;
    xo.getObjects(xl);

		xmlList::const_iterator it;

		for(it=xl.begin(); it != xl.end(); ++it){
			if(it->is("Control")){
				oControl c(this);
				c.set(&*it);

				if(!existControl(c.Id) && c.Id>0)
					Controls.push_back(c);
				//else gdi.alert("Control duplicate");
			}
		}
	}

  toc("controls");

 	//Get courses
	xo=xml.getObject("CourseList");
	if(xo){
	  xmlList xl;
    xo.getObjects(xl);

		xmlList::const_iterator it;

		for(it=xl.begin(); it != xl.end(); ++it){
			if(it->is("Course")){
				oCourse c(this);
				c.Set(&*it);
				if(!existCourse(c.Id) && c.Id>0)
          addCourse(c);
			}
		}
	}

  toc("course");
	
	//Get classes
	xo=xml.getObject("ClassList");
	if(xo){
	  xmlList xl;
    xo.getObjects(xl);

		xmlList::const_iterator it;

		for(it=xl.begin(); it != xl.end(); ++it){
			if(it->is("Class")){
				oClass c(this);
				c.Set(&*it);
				if(!existClass(c.Id) && c.Id>0)
					Classes.push_back(c);
			}
		}
	}

  toc("class");
	
	//Get clubs
	xo=xml.getObject("ClubList");
	if(xo){
	  xmlList xl;
    xo.getObjects(xl);

		xmlList::const_iterator it;

		for(it=xl.begin(); it != xl.end(); ++it){
			if(it->is("Club")){
				oClub c(this);
				c.set(*it);
        if(c.Id>0)
					addClub(c);//Clubs.push_back(c);
			}
		}
	}

  toc("club");
	
	//Get runners
	xo=xml.getObject("RunnerList");
	if(xo){
	  xmlList xl;
    xo.getObjects(xl);

		xmlList::const_iterator it;

		for(it=xl.begin(); it != xl.end(); ++it){
			if(it->is("Runner")){
				oRunner r(this);
        r.Set(*it);
        if (r.Id>0)
          addRunner(r);
        else if (r.Card)
          r.Card->tOwner=0;
			}
		}
	}

  toc("runner");
	
	//Get teams
	xo=xml.getObject("TeamList");
	if(xo){
	  xmlList xl;
    xo.getObjects(xl);

		xmlList::const_iterator it;

		for(it=xl.begin(); it != xl.end(); ++it){
			if(it->is("Team")){
				oTeam t(this);
				t.set(*it);
				if(t.Id>0){
					Teams.push_back(t);
					Teams.back().apply(false, 0);
				}
			}
		}
	}	
	
  toc("team");
	
	xo=xml.getObject("PunchList");
	if(xo){
	  xmlList xl;
    xo.getObjects(xl);

		xmlList::const_iterator it;
    if (xl.size() > 10)
      setupCardHash(false); // This improves performance when there are many cards.
    oFreePunch::disableHashing = true;
    try {
      for(it=xl.begin(); it != xl.end(); ++it){
			  if(it->is("Punch")){
				  oFreePunch p(this, 0, 0, 0);
				  p.Set(&*it);
          addFreePunch(p);
			  }
		  }
    }
    catch(...) {
      oFreePunch::disableHashing = false;
      throw;
    }
    oFreePunch::disableHashing = false;
    oFreePunch::rehashPunches(*this, 0, 0);
    setupCardHash(true); // Clear
	}

  toc("punch");

	xo=xml.getObject("CardList");
	if(xo){
	  xmlList xl;
    xo.getObjects(xl);
		xmlList::const_iterator it;

		for(it=xl.begin(); it != xl.end(); ++it){
			if(it->is("Card")){
				oCard c(this);
				c.Set(*it);
        assert(c.Id>=0);
				Cards.push_back(c);
			}
		}
	}

  toc("card");
	
	xo=xml.getObject("Updated");
	if(xo) Modified.setStamp(xo.get());

  adjustTeamMultiRunners(0);
	updateFreeId();
	reEvaluateAll(true); //True needed to update data for sure

  toc("update");
	
  xmlobject xList = xml.getObject("Lists");
  if (xList)
    listContainer->load(MetaListContainer::ExternalList, xList);

	return true;
}

bool oEvent::openRunnerDatabase(char* filename)
{
	char file[260];
	getUserFile(file, filename);
  
  char fclub[260];
  char frunner[260];

  strcpy_s(fclub, file);
  strcat_s(fclub, ".clubs");

  strcpy_s(frunner, file);
  strcat_s(frunner, ".persons");

  try {
    if (fileExist(fclub) && fileExist(frunner)) {
      runnerDB->loadClubs(fclub);
      runnerDB->loadRunners(frunner);
    }
  }
  catch(std::exception &ex) {
    MessageBox(0, ex.what(), "Error", MB_OK);
  }
	return true;
}

pRunner oEvent::dbLookUpById(__int64 extId) const
{
  oEvent *toe = const_cast<oEvent *>(this);
	static oRunner sRunner = oRunner(toe, 0);
  sRunner = oRunner(toe, 0);
  RunnerDBEntry *dbr = runnerDB->getRunnerById(int(extId));
  if (dbr != 0) {
    sRunner.init(*dbr);
    /*dbr->getName(sRunner.Name);
    sRunner.CardNo = dbr->cardNo;
    sRunner.Club = runnerDB->getClub(dbr->clubNo);
    sRunner.getDI().setString("Nationality", dbr->getNationality());
    sRunner.getDI().setInt("BirthYear", dbr->getBirthYear());
    sRunner.getDI().setString("Sex", dbr->getSex());
    sRunner.setExtIdentifier(dbr->getExtId());*/
	  return &sRunner;
  }
  else
    return 0;
}

pRunner oEvent::dbLookUpByCard(int cardNo) const
{
  oEvent *toe = const_cast<oEvent *>(this);
	static oRunner sRunner = oRunner(toe, 0);
  sRunner = oRunner(toe, 0);
  RunnerDBEntry *dbr = runnerDB->getRunnerByCard(cardNo);
  if (dbr != 0) {
    dbr->getName(sRunner.Name);
    sRunner.init(*dbr);
    sRunner.CardNo = cardNo;
    /*sRunner.Club = runnerDB->getClub(dbr->clubNo);
    sRunner.getDI().setString("Nationality", dbr->getNationality());
    sRunner.getDI().setInt("BirthYear", dbr->getBirthYear());
    sRunner.getDI().setString("Sex", dbr->getSex());
    sRunner.setExtIdentifier(dbr->getExtId());*/
	  return &sRunner;
  }
  else
    return 0;
}

pRunner oEvent::dbLookUpByName(const string &name, int clubId, int classId, int birthYear) const
{
  oEvent *toe = const_cast<oEvent *>(this);

	static oRunner sRunner = oRunner(toe, 0);
  sRunner = oRunner(toe, 0);

  if (birthYear == 0) {
    pClass pc = getClass(classId);
    
    int expectedAge = pc ? pc->getExpectedAge() : 0;

    if (expectedAge>0)
      birthYear = getThisYear() - expectedAge;
  }

  pClub pc = getClub(clubId);

  if (pc && pc->getExtIdentifier()>0)
    clubId = (int)pc->getExtIdentifier();
    
  RunnerDBEntry *dbr = runnerDB->getRunnerByName(name, clubId, birthYear);

  if (dbr) {
    sRunner.init(*dbr);
    /*
    dbr->getName(sRunner.Name);
    sRunner.CardNo = dbr->cardNo;
    sRunner.Club = runnerDB->getClub(dbr->clubNo);
    sRunner.getDI().setString("Nationality", dbr->getNationality());
    sRunner.getDI().setInt("BirthYear", dbr->getBirthYear());
    sRunner.getDI().setString("Sex", dbr->getSex());*/
    sRunner.setExtIdentifier(int(dbr->getExtId()));
	  return &sRunner;
  }

  return 0;
}

bool oEvent::saveRunnerDatabase(char *filename, bool onlyLocal)
{  
  char file[260];
	getUserFile(file, filename);

  char fclub[260];
  char frunner[260];
  strcpy_s(fclub, file);
  strcat_s(fclub, ".clubs");

  strcpy_s(frunner, file);
  strcat_s(frunner, ".persons");

  if (!onlyLocal || !runnerDB->isFromServer()) {
    runnerDB->saveClubs(fclub);
    runnerDB->saveRunners(frunner);
  }
	return true;
}

void oEvent::updateRunnerDatabase()
{
  if(Name=="!TESTT�VLING")
    return;

	oRunnerList::iterator it;	

	for (it=Runners.begin(); it != Runners.end(); ++it){
    if(it->Card && it->Card->CardNo==it->CardNo && 
      it->getDI().getInt("CardFee")==0 && it->Card->getNumPunches()>7)
			updateRunnerDatabase(&*it);
	}
}

void oEvent::updateRunnerDatabase(pRunner r)
{
	if(!r->CardNo)
		return;
  runnerDB->updateAdd(*r);
}

pCourse oEvent::addCourse(const string &pname, int plengh, int id)
{	
	oCourse c(this, id);
	c.Length = plengh;
	c.Name = pname;
  return addCourse(c);
}

pCourse oEvent::addCourse(const oCourse &oc)
{
  if (oc.Id==0)
    return 0;
  else {
    pCourse pOld=getCourse(oc.getId());
    if (pOld)
      return 0;
  }
	Courses.push_back(oc);
  qFreeCourseId=max(qFreeCourseId, oc.getId());

  pCourse pc = &Courses.back();

  if (!pc->existInDB())
    pc->synchronize();
  courseIdIndex[oc.Id] = pc;
	return pc;
}

void oEvent::autoAddTeam(pRunner pr) 
{
  //Warning: make sure there is no team already in DB that has not yet been applied yet...
  if (pr && pr->Class) {
    pClass pc = pr->Class;
    if (pc->isSingleRunnerMultiStage()) {
      //Auto create corresponding team
      pTeam t = addTeam(pr->getName(), pr->getClubId(), pc->getId());
      if (pr->StartNo == 0)
        pr->StartNo = Teams.size();
      t->setStartNo(pr->StartNo);
      t->setRunner(0, pr, true);
    }
  }
}

void oEvent::autoRemoveTeam(pRunner pr) 
{
  if (pr && pr->Class) {
    pClass pc = pr->Class;
    if (pc->isSingleRunnerMultiStage()) {
      if (pr->tInTeam) {
        // A team may have more than this runner -> do not remove
        bool canRemove = true;
        const vector<pRunner> &runners = pr->tInTeam->Runners;
        for (size_t k = 0; k<runners.size(); k++) {
          if (runners[k] && runners[k]->Name != pr->Name)
            canRemove = false;
        }
        if (canRemove)
          removeTeam(pr->tInTeam->getId());
      }
    }
  }
}

pRunner oEvent::addRunner(const string &name, int clubId, int classId,
                          int cardNo, int birthYear, bool autoAdd)
{	
  if (birthYear != 0)
    birthYear = extendYear(birthYear);

  pRunner db_r = oe->dbLookUpByCard(cardNo);

  if (db_r && !db_r->matchName(name))
    db_r = 0; // "Existing" card, but different runner
    
    
  if (db_r == 0)
    db_r = oe->dbLookUpByName(name, clubId, classId, birthYear);

  if (db_r) {
    // We got name from DB. Other parameters might have changed from DB.
    if (clubId>0)
      db_r->Club = getClub(clubId);
    db_r->Class = getClass(classId);
    if (cardNo>0)
      db_r->CardNo = cardNo;
    if (birthYear>0)
      db_r->setBirthYear(birthYear);
    return addRunnerFromDB(db_r, classId, autoAdd);
  }
  oRunner r(this);
  r.Name = name;
  r.Club = getClub(clubId);
  r.Class = getClass(classId);
  if (cardNo>0)
    r.CardNo = cardNo;
  if (birthYear>0)
    r.setBirthYear(birthYear);
  pRunner pr = addRunner(r);
 
  if (autoAdd)
    autoAddTeam(pr);
  return pr;
}

pRunner oEvent::addRunner(const string &pname, const string &pclub, int classId, 
                          int cardNo, int birthYear, bool autoAdd)
{
  pClub club = getClubCreate(0, pclub);
  return addRunner(pname, club->getId(), classId, cardNo, birthYear, autoAdd);
}

pRunner oEvent::addRunnerFromDB(const pRunner db_r, 
                                int classId, bool autoAdd) 
{
  oRunner r(this);
  r.Name = db_r->Name;
  r.CardNo = db_r->CardNo;

  if (db_r->Club) {
    r.Club = getClub(db_r->getClubId());
    if (!r.Club)
      r.Club = addClub(*db_r->Club);
  }

  r.Class=classId ? getClass(classId) : 0;
  memcpy(r.oData, db_r->oData, sizeof(r.oData));

	pRunner pr = addRunner(r);

  if (autoAdd)
    autoAddTeam(pr);
  return pr;
}

pRunner oEvent::addRunner(const oRunner &r) 
{
  bool needUpdate = Runners.empty();

  Runners.push_back(r);	
	pRunner pr=&Runners.back();

  if (pr->StartNo == 0) {
    pr->StartNo = ++nextFreeStartNo; // Need not be unique
  }
  else {
    nextFreeStartNo = max(nextFreeStartNo, pr->StartNo);
  }

  if (pr->Card)
    pr->Card->tOwner = pr;

  if (HasDBConnection) {
    if (!pr->existInDB())
      pr->synchronize();
  }
  if (needUpdate)
    oe->updateTabs();

  if (pr->Class)
    pr->Class->tResultInfo.clear();

  runnerById[pr->Id] = pr;

  // Notify runner database that runner has entered
  getRunnerDatabase().hasEnteredCompetition(r.getExtIdentifier());
  return pr;
}

pRunner oEvent::addRunnerVacant(int classId) {
  pRunner r=addRunner(lang.tl("Vakant"), getVacantClub(), classId, 0,0, true);
  if (r) {
    r->apply(false);
    r->synchronize(true);
  }
  return r;
}

int oEvent::getFreeCourseId()
{
	qFreeCourseId++;
	return qFreeCourseId;
}

int oEvent::getFreeControlId()
{
	qFreeControlId++;
	return qFreeControlId;
}

string oEvent::getAutoCourseName() const
{
	char bf[32];
  sprintf_s(bf, lang.tl("Bana %d").c_str(), Courses.size()+1);
	return bf;
}

int oEvent::getFreeClassId()
{
	qFreeClassId++;
	return qFreeClassId;
}

int oEvent::getFirstClassId(bool teamClass) const {
  for (oClassList::const_iterator it = Classes.begin(); it != Classes.end(); ++it) {
    if(it->isRemoved())
      continue;
    int ns = it->getNumStages();
    if (teamClass && ns > 0)
      return it->Id;
    else if (!teamClass && ns == 0)
      return it->Id;
  }
  return 0;
}

int oEvent::getFreeCardId()
{
	qFreeCardId++;
	return qFreeCardId;
}

int oEvent::getFreePunchId()
{
	qFreePunchId++;
	return qFreePunchId;
}

string oEvent::getAutoClassName() const
{
	char bf[32];
  sprintf_s(bf, 32, lang.tl("Klass %d").c_str(), Classes.size()+1);
	return bf;
}

string oEvent::getAutoTeamName() const
{
	char bf[32];
  sprintf_s(bf, 32, lang.tl("Lag %d").c_str(), Teams.size()+1);
	return bf;
}

string oEvent::getAutoRunnerName() const
{
	char bf[32];
  sprintf_s(bf, 32, lang.tl("Deltagare %d").c_str(), Runners.size()+1);
	return bf;
}

int oEvent::getFreeClubId()
{
	qFreeClubId++;
	return qFreeClubId;
}

int oEvent::getFreeRunnerId()
{
	qFreeRunnerId++;
	return qFreeRunnerId;
}

void oEvent::updateFreeId(oBase *obj)
{	
	if(typeid(*obj)==typeid(oRunner)){		
		qFreeRunnerId=max(obj->Id, qFreeRunnerId);
	}
	else if(typeid(*obj)==typeid(oClass)){
		qFreeClassId=max(obj->Id, qFreeClassId);
	}
	else if(typeid(*obj)==typeid(oCourse)){
		qFreeCourseId=max(obj->Id, qFreeCourseId);
	}
	else if(typeid(*obj)==typeid(oControl)){
		qFreeControlId=max(obj->Id, qFreeControlId);
	}
	else if(typeid(*obj)==typeid(oClub)){
    if (obj->Id != cVacantId)
		  qFreeClubId=max(obj->Id, qFreeClubId);
	}
	else if(typeid(*obj)==typeid(oCard)){
		qFreeCardId=max(obj->Id, qFreeCardId);
	}
	else if(typeid(*obj)==typeid(oFreePunch)){
		qFreePunchId=max(obj->Id, qFreePunchId);
	}
	else if(typeid(*obj)==typeid(oTeam)){
		qFreeTeamId=max(obj->Id, qFreeTeamId);
	}
	/*else if(typeid(*obj)==typeid(oEvent)){
		qFree
	}*/
}

void oEvent::updateFreeId()
{
	{
		oRunnerList::iterator it;	
		qFreeRunnerId=0;
    nextFreeStartNo = 0;

    for (it=Runners.begin(); it != Runners.end(); ++it) {
			qFreeRunnerId = max(qFreeRunnerId, it->Id);
      nextFreeStartNo = max(nextFreeStartNo, it->StartNo);
    }
	}
	{
		oClassList::iterator it;	
		qFreeClassId=0;
		for (it=Classes.begin(); it != Classes.end(); ++it)
			qFreeClassId=max(qFreeClassId, it->Id);
	}
	{
		oCourseList::iterator it;	
		qFreeCourseId=0;
		for (it=Courses.begin(); it != Courses.end(); ++it)
			qFreeCourseId=max(qFreeCourseId, it->Id);
	}
	{
		oControlList::iterator it;	
		qFreeControlId=0;
		for (it=Controls.begin(); it != Controls.end(); ++it)
			qFreeControlId=max(qFreeControlId, it->Id);
	}
	{
		oClubList::iterator it;	
		qFreeClubId=0;
		for (it=Clubs.begin(); it != Clubs.end(); ++it) {
      if (it->Id != cVacantId)
			  qFreeClubId=max(qFreeClubId, it->Id);
    }
	}
	{
		oCardList::iterator it;	
		qFreeCardId=0;
		for (it=Cards.begin(); it != Cards.end(); ++it)
			qFreeCardId=max(qFreeCardId, it->Id);
	}
	{
		oFreePunchList::iterator it;	
		qFreePunchId=0;
		for (it=punches.begin(); it != punches.end(); ++it)
			qFreePunchId=max(qFreePunchId, it->Id);
	}

	{
		oTeamList::iterator it;	
		qFreeTeamId=0;
		for (it=Teams.begin(); it != Teams.end(); ++it)
			qFreeTeamId=max(qFreeTeamId, it->Id);
	}
}

int oEvent::getVacantClub() 
{
  if (vacantId > 0) 
    return vacantId;

  pClub pc = getClub("Vakant");
  if (pc == 0)
    pc = getClub("Vacant"); //eng
  if (pc == 0)
    pc = getClub(lang.tl("Vakant")); //other lang?

  if (pc == 0)
    pc=getClubCreate(cVacantId, lang.tl("Vakant"));

  vacantId = pc->getId();
  return vacantId;    
}

int oEvent::getVacantClubIfExist() const
{
  if (vacantId > 0) 
    return vacantId;
  if (vacantId == -1)
    return 0;
  pClub pc=getClub("Vakant");
  if (pc == 0)
    pc = getClub("Vacant");
  if (pc == 0)
    pc = getClub(lang.tl("Vakant")); //other lang?

  if (!pc) {
    vacantId = -1;
    return 0;
  }
  vacantId = pc->getId();
  return vacantId;    
}

pCard oEvent::allocateCard(pRunner owner)
{
	oCard c(this);
  c.tOwner = owner;
	Cards.push_back(c);
	pCard newCard = &Cards.back();
	return newCard;
}

bool oEvent::sortRunners(SortOrder so)
{
  reinitializeClasses();
	CurrentSortOrder=so;
	Runners.sort();
	return true;
}

bool oEvent::sortTeams(SortOrder so, int leg)
{
  reinitializeClasses();
  oTeamList::iterator it;

  if(so==ClassResult || so==ClassTotalResult) {
    bool totalResult = so==ClassTotalResult;
    bool hasRunner = (leg == -1);
	  for (it=Teams.begin(); it != Teams.end(); ++it) {
      int lg=leg;
      const int lastIndex = it->Class ? it->Class->getLastStageIndex() : 0;
      lg = min<unsigned>(lg, lastIndex);
      if (lg == leg)
        hasRunner = true;
      //if(lg>=int(it->Class->getNumStages()))
      //  lg=it->Class->getLastStageIndex();

      it->_sortstatus=it->getLegStatus(lg, totalResult);
      it->_sorttime=it->getLegRunningTime(lg, totalResult);

      // Ensure number of restarts has effect on final result
      if (lg == lastIndex)
        it->_sorttime += it->tNumRestarts*24*3600;
	  }

    if (!hasRunner)
      return false;

    Teams.sort(oTeam::compareResult);
  }
  else if(so==ClassStartTime) {
	  for (it=Teams.begin(); it != Teams.end(); ++it) {
		  it->_sortstatus=0;
      it->_sorttime=it->getLegStartTime(leg);
      if (it->_sorttime<=0)
        it->_sortstatus=1;
	  }
    Teams.sort(oTeam::compareResult);
  }
	return true;
}

void oEvent::calculateSplitResults(int controlIdFrom, int controlIdTo)
{
	oRunnerList::iterator it;

  for (it=Runners.begin(); it!=Runners.end(); ++it) {
    int st = 0;
    if (controlIdFrom > 0) {
      RunnerStatus stat;
      it->getSplitTime(controlIdFrom, stat, st);
      if (stat != StatusOK) {
        it->tStatus = stat;
        it->tRT = 0;
        continue;
      }
    }
    if (controlIdTo == 0) {
      it->tRT = max(0, it->FinishTime - (st + it->StartTime) );
      it->tStatus = it->Status;
    }
    else {
      int ft = 0;
      it->getSplitTime(controlIdTo, it->tStatus, ft);
      if(it->tStatus==StatusOK && it->Status > StatusOK)
        it->tStatus=it->Status;

      it->tRT = max(0, ft - st);
    }
  }

  Runners.sort(oRunner::sortSplit);
	int cClassId=-1;
	int cPlace=0;
	int vPlace=0;
	int cTime=0;

	for (it=Runners.begin(); it != Runners.end(); ++it){
		if(it->getClassId()!=cClassId){
			cClassId=it->getClassId();
			cPlace=0;
			vPlace=0;
			cTime=0;
      it->Class->tLegLeaderTime=9999999;
		}
	
		if (it->tStatus==StatusOK) {
			cPlace++;

      if(it->Class)
        it->Class->tLegLeaderTime=min(it->tRT, it->Class->tLegLeaderTime);


			if(it->tRT>cTime)
				vPlace=cPlace;

			cTime=it->tRT;

			it->tPlace=vPlace;
		}
		else
			it->tPlace=99000+it->Status;
	}
}

void oEvent::calculateResults(ResultType resultType)
{
  const bool totalResults = resultType == RTTotalResult;
  const bool courseResults = resultType == RTCourseResult;
  const bool classCourseResults = resultType == RTClassCourseResult;

  if (classCourseResults)
    sortRunners(ClassCourseResult);
  else if (courseResults)
    sortRunners(CourseResult);
  else if (!totalResults)
	  sortRunners(ClassResult);
  else
    sortRunners(ClassTotalResult);

	oRunnerList::iterator it;

	int cClassId=-1;
	int cPlace=0;
	int vPlace=0;
	int cTime=0;
  int cDuplicateLeg=0;
  bool invalidClass = false;
  bool useResults = false;
	for (it=Runners.begin(); it != Runners.end(); ++it) {
    if (it->isRemoved())
      continue;
    // Start new "class"
    if (classCourseResults) {
      const pCourse crs = it->getCourse();
      int crsId = it->getClassId() * 997 + (crs ? crs->getId() : 0);
      if (crsId != cClassId) {
        cClassId = crsId; 
        cPlace=0;
			  vPlace=0;
			  cTime=0;
        useResults = it->Class ? !it->Class->getNoTiming() : false;
			  invalidClass = it->Class ? it->Class->getClassStatus() != oClass::Normal : false;
      }
    }
    else if (courseResults) {
      const pCourse crs = it->getCourse();
      int crsId = crs ? crs->getId() : 0;
      if (crsId != cClassId) {
        cClassId = crsId; 
        useResults = crs != 0;
        cPlace=0;
			  vPlace=0;
			  cTime=0;
      }
    }
    else if (it->getClassId()!=cClassId || it->tDuplicateLeg!=cDuplicateLeg) {
			cClassId=it->getClassId();
      useResults = it->Class ? !it->Class->getNoTiming() : false;
			cPlace=0;
			vPlace=0;
			cTime=0;
      cDuplicateLeg = it->tDuplicateLeg;
      invalidClass = it->Class ? it->Class->getClassStatus() != oClass::Normal : false;
 		}

    // Calculate results
    if (invalidClass) {
      it->tTotalPlace = 0;
      it->tPlace = 0;
    }
    else if (!totalResults) {
      int tPlace = 0;

		  if(it->Status==StatusOK){
			  cPlace++;

			  if(it->getRunningTime()>cTime)
				  vPlace=cPlace;

			  cTime = it->getRunningTime();

        if (useResults && cTime > 0)
			    tPlace = vPlace;
		  }
		  else
			  tPlace = 99000 + it->Status;

      if (!classCourseResults)
        it->tPlace = tPlace;
      else
        it->tCoursePlace = tPlace;
    }
    else {
      int tt = it->getTotalRunningTime(it->FinishTime);

      if (it->getTotalStatus() == StatusOK && tt>0) {
			  cPlace++;
        
        if(tt > cTime)
				  vPlace = cPlace;

			  cTime = tt;

        if (useResults)
          it->tTotalPlace = vPlace;
        else
          it->tTotalPlace = 0;
		  }
		  else
			  it->tTotalPlace = 99000 + it->Status;
    }
	}
}

void oEvent::calculateRogainingResults()
{
	sortRunners(ClassPoints);
	oRunnerList::iterator it;

	int cClassId=-1;
  int cPlace = 0;
	int vPlace = 0;
	int cTime = numeric_limits<int>::min();;
  int cDuplicateLeg=0;
  bool useResults = false;
  bool isRogaining = false;
  bool invalidClass = false;

	for (it=Runners.begin(); it != Runners.end(); ++it) {
    if (it->isRemoved())
      continue;

    if (it->getClassId()!=cClassId || it->tDuplicateLeg!=cDuplicateLeg) {
			cClassId = it->getClassId();
      useResults = it->Class ? !it->Class->getNoTiming() : false;
			cPlace = 0;
			vPlace = 0;
			cTime = numeric_limits<int>::min();
      cDuplicateLeg = it->tDuplicateLeg;
      isRogaining = it->Class ? it->Class->isRogaining() : false;
      invalidClass = it->Class ? it->Class->getClassStatus() != oClass::Normal : false;
		}
	
    if (!isRogaining)
      continue;

    if (invalidClass) {
      it->tTotalPlace = 0;
      it->tPlace = 0;
    }
    else if(it->Status==StatusOK) {
			cPlace++;

      int cmpRes = 3600 * 24 * 7 * it->tRogainingPoints - it->getRunningTime();

      if(cmpRes != cTime)
				vPlace = cPlace;

			cTime = cmpRes;

      if (useResults)
			  it->tPlace = vPlace;
      else
        it->tPlace = 0;
		}
		else
			it->tPlace = 99000 + it->Status;
	}
}

bool oEvent::calculateTeamResults(int leg, bool totalMultiday)
{
	oTeamList::iterator it;

  bool hasRunner;
  if (totalMultiday)
    hasRunner = sortTeams(ClassTotalResult, leg);
  else
    hasRunner = sortTeams(ClassResult, leg);

  if(!hasRunner)
    return false;


	int cClassId=0;
	int cPlace=0;
	int vPlace=0;
	int cTime=0;
  bool invalidClass = false;

	for (it=Teams.begin(); it != Teams.end(); ++it){
    if (it->isRemoved())
      continue;

		if(it->Class && it->Class->Id!=cClassId){
			cClassId=it->Class->Id;
			cPlace=0;
			vPlace=0;
			cTime=0;
      invalidClass = it->Class->getClassStatus() != oClass::Normal;
		}

    int sleg;
    if (leg==-1)
      sleg=it->Runners.size()-1;
    else
      sleg=leg;
    int p;
    if (invalidClass) {
      p = 0;
    }
    else if(it->_sortstatus==StatusOK){
			cPlace++;

			if(it->_sorttime>cTime)
				vPlace=cPlace;

			cTime = it->_sorttime;

			p = vPlace;
		}
    else {
      p = 99000+it->_sortstatus;
    }

    if (totalMultiday)
      it->_places[sleg].totalP = p;
    else
      it->_places[sleg].p = p;
	}
  return true;
}


void oEvent::calculateTeamResults(bool multidayTotal)
{
	for(int i=0;i<maxRunnersTeam;i++)
    if(!calculateTeamResults(i, multidayTotal))
      return;
}


string oEvent::getZeroTime() const
{
  return getAbsTime(0);
}

void oEvent::setZeroTime(string m)
{
  unsigned nZeroTime = convertAbsoluteTime(m);
  if (nZeroTime!=ZeroTime && nZeroTime != -1) {
    updateChanged();
    ZeroTime=nZeroTime;
  }
}

void oEvent::setName(const string &m) 
{
  if (m!=Name) {
    Name=m;
    updateChanged();
  }
}

void oEvent::setAnnotation(const string &m) 
{
  if (m!=Annotation) {
    Annotation=m;
    updateChanged();
  }
}

string oEvent::getTitleName() const
{
  if (empty())
    return "";
  if(isClient())
    return oe->getName() + lang.tl(" (p� server)");
  else
    return oe->getName() + lang.tl(" (lokalt)");
}

void oEvent::setDate(const string &m)
{
 if (m!=Date) {
    int d = convertDateYMS(m);
    if (d <= 0)
      throw meosException("Ange datum p� formen ����-MM-DD, inte som 'X'#" + m);

    Date = m;
    updateChanged();
  }
}

string oEvent::getAbsTime(DWORD time) const
{
	DWORD t=ZeroTime+time;

	if(int(t)<0)
		return "00:00:00";

	char bf[256];
	sprintf_s(bf, 256, "%02d:%02d:%02d", (t/3600)%24, (t/60)%60, t%60);
	return bf;
}

const string &oEvent::getTimeZoneString() const {
  if (!date2LocalTZ.count(Date))
    date2LocalTZ[Date] = ::getTimeZoneString(Date);
  return date2LocalTZ[Date];
}

string oEvent::getAbsDateTimeISO(DWORD time, bool includeDate, bool useGMT) const
{
	DWORD t = ZeroTime + time;
  string dateS, timeS;
  if (int(t)<0) {
    dateS = "2000-01-01";
    if (useGMT)
      timeS = "00:00:00Z";
    else
      timeS = "00:00:00" + getTimeZoneString();
  }
  else {
    int extraDay;

    if (useGMT) {
      int offset = ::getTimeZoneInfo(Date);
      t += offset;
      if (t < 0) {
        extraDay = -1;
        t += 3600 * 24;
      }
      else {
        extraDay = t / (3600*24);
      }
      char bf[64];
      sprintf_s(bf, "%02d:%02d:%02dZ", (t/3600)%24, (t/60)%60, t%60);
      timeS = bf;
    }
    else {
      char bf[64];
      extraDay = t / (3600*24);
  	  sprintf_s(bf, "%02d:%02d:%02d", (t/3600)%24, (t/60)%60, t%60);
      timeS = bf + getTimeZoneString();
    }

    if (extraDay == 0 ) {
      dateS = Date;
    }
    else {
      SYSTEMTIME st;
      convertDateYMS(Date, st);
      __int64 sec = SystemTimeToInt64Second(st);
      sec = sec + (extraDay * 3600 * 24);
      st = Int64SecondToSystemTime(sec);
      dateS = convertSystemDate(st);
    }
  }

  if (includeDate)
    return dateS + "T" + timeS;
  else
    return timeS;
}

string oEvent::getAbsTimeHM(DWORD time) const
{
	DWORD t=ZeroTime+time;

	if(int(t)<0)
		return "00:00";

	char bf[256];
	sprintf_s(bf, 256, "%02d:%02d", (t/3600)%24, (t/60)%60);
	return bf;
}

//Absolute time string to absolute time int (used by cvs-parser)
int oEvent::convertAbsoluteTime(const string &m)
{
	if(m.empty() || m[0]=='-')
		return -1;

  int len=m.length();
  bool firstComma = false;
  for (int k=0;k<len;k++) {
    BYTE b=m[k];
    if ( !(b==' ' || (b>='0' && b<='9')) ) {
      if (b==':' && firstComma == false)
        continue;
      else if ((b==',' || b=='.') && firstComma == false) {
        firstComma = true;
        continue;
      }
      return -1;
    }
  }

	int hour=atoi(m.c_str());

	if(hour<0 || hour>23)
		return -1;

	int minute=0;
	int second=0;

	int kp=m.find_first_of(':');
	
	if(kp>0)
	{
		string mtext=m.substr(kp+1);
		minute=atoi(mtext.c_str());

		if(minute<0 || minute>60)
			minute=0;

		kp=mtext.find_last_of(':');

		if(kp>0) {
			second=atoi(mtext.substr(kp+1).c_str());

			if(second<0 || second>60)
				second=0;
		}
	}
	int t=hour*3600+minute*60+second;

	if(t<0)	return 0;

	return t;
}

int oEvent::getRelativeTime(const string &date, const string &absoluteTime, const string &timeZone) const {

  int atime=convertAbsoluteTime(absoluteTime);
 
  if (timeZone == "Z" || timeZone == "z") {
    SYSTEMTIME st;
    convertDateYMS(date, st); 
    
    st.wHour = atime / 3600;
    st.wMinute = (atime / 60) % 60;
    st.wSecond = atime % 60;

    SYSTEMTIME localTime;
    memset(&localTime, 0, sizeof(SYSTEMTIME));
    SystemTimeToTzSpecificLocalTime(0, &st, &localTime);

    atime = localTime.wHour*3600 + localTime.wMinute * 60 + localTime.wSecond;
  }

	if(atime>=0 && atime<3600*24){
		int rtime=atime-ZeroTime;
		
		if(rtime<=0) 
      rtime+=3600*24;

    //Don't allow times just before zero time.
    if(rtime>3600*22)
      return -1;

		return rtime;
	}
	else return -1;
}


int oEvent::getRelativeTime(const string &m) const
{
	int atime=convertAbsoluteTime(m);

	if(atime>=0 && atime<3600*24){
		int rtime=atime-ZeroTime;
		
		if(rtime<=0) 
      rtime+=3600*24;

    //Don't allow times just before zero time.
    if(rtime>3600*20)
      return -1;

		return rtime;
	}
	else return -1;
}

int oEvent::getRelativeTimeFrom12Hour(const string &m) const
{
	int atime=convertAbsoluteTime(m);

	if (atime>=0 && atime<3600*24) {
		int rtime=atime-(ZeroTime % (3600*12));
		
		if(rtime<=0) 
      rtime+=3600*12;

    //Don't allow times just before zero time.
    if(rtime>3600*20)
      return -1;

		return rtime;
	}
	else return -1;
}

void oEvent::removeRunner(const vector<int> &ids)
{
	oRunnerList::iterator it;	

  set<int> toRemove;
  for (size_t k = 0; k < ids.size(); k++) {
    int Id = ids[k];
    pRunner r=getRunner(Id, 0);

    if (r==0)
      continue;
    
    r = r->tParentRunner ? r->tParentRunner : r;

    if (toRemove.count(r->getId()))
      continue; //Already found.

    //Remove a singe runner team
    autoRemoveTeam(r);

    for (size_t k=0;k<r->multiRunner.size();k++)
      if (r->multiRunner[k])
        toRemove.insert(r->multiRunner[k]->getId());

    toRemove.insert(Id);
  }
  
  if (toRemove.empty())
    return;

  dataRevision++;

	for (it=Runners.begin(); it != Runners.end();){
    oRunner &cr = *it;
    if(toRemove.count(cr.getId())> 0){
			if(HasDBConnection) 
        msRemove(&cr);
      toRemove.erase(cr.getId());
      runnerById.erase(cr.getId());
      if (cr.Card) {
        assert( cr.Card->tOwner == &cr );
        cr.Card->tOwner = 0;
      }
      // Reset team runner (this should not happen)
      if (it->tInTeam) {
        if (it->tInTeam->Runners[it->tLeg]==&*it)
          it->tInTeam->Runners[it->tLeg] = 0;
      }
      
      oRunnerList::iterator next = it;
      ++next;

			Runners.erase(it);
      if (toRemove.empty()) {
        oe->updateTabs();
			  return;
      }
      else
      it = next;
		}
    else
      ++it;
	}
  oe->updateTabs();
}

void oEvent::removeCourse(int Id)
{
	oCourseList::iterator it;	

	for (it=Courses.begin(); it != Courses.end(); ++it){
		if(it->Id==Id){
      if(HasDBConnection) 
        msRemove(&*it);
      dataRevision++;
			Courses.erase(it);
      courseIdIndex.erase(Id);
			return;
		}
	}	
}

void oEvent::removeClass(int Id)
{
	oClassList::iterator it;	

	for (it=Classes.begin(); it != Classes.end(); ++it){
		if(it->Id==Id){
			if(HasDBConnection) 
        msRemove(&*it);
			Classes.erase(it);
      dataRevision++;
      updateTabs();
			return;
    }
	}	
}

void oEvent::removeControl(int Id)
{
	oControlList::iterator it;	

	for (it=Controls.begin(); it != Controls.end(); ++it){
		if(it->Id==Id){
			if(HasDBConnection)
        msRemove(&*it);
			Controls.erase(it);
      dataRevision++;
			return;
		}
	}	
}

void oEvent::removeClub(int Id)
{
	oClubList::iterator it;	

	for (it=Clubs.begin(); it != Clubs.end(); ++it){
		if (it->Id==Id) {
			if(HasDBConnection)
        msRemove(&*it);
			Clubs.erase(it);
      clubIdIndex.erase(Id);
      dataRevision++;
			return;
		}
	}
  if (vacantId == Id)
    vacantId = 0; // Clear vacant id
}

void oEvent::removeCard(int Id)
{
	oCardList::iterator it;	

  for (it=Cards.begin(); it != Cards.end(); ++it) {
    if(it->getOwner() == 0 && it->Id == Id) {
      if (it->tOwner) {
        if (it->tOwner->Card == &*it)
          it->tOwner->Card = 0;
      }
			if (HasDBConnection) 
        msRemove(&*it);
			Cards.erase(it);
      dataRevision++;
      return;
    }
  }
}

bool oEvent::isCourseUsed(int Id) const
{
	oClassList::const_iterator it;	

	for (it=Classes.begin(); it != Classes.end(); ++it){
		if(it->isCourseUsed(Id))
			return true;
	}	

	oRunnerList::const_iterator rit;

	for (rit=Runners.begin(); rit != Runners.end(); ++rit){
		pCourse pc=rit->getCourse();
		if(pc && pc->Id==Id)	
			return true;
	}
	return false;
}

bool oEvent::isClassUsed(int Id) const
{
	//Search runners
  oRunnerList::const_iterator it;
	for (it=Runners.begin(); it != Runners.end(); ++it){		
		if(it->getClassId()==Id)	
			return true;
	}	

	//Search teams
	oTeamList::const_iterator tit;
	for (tit=Teams.begin(); tit != Teams.end(); ++tit){		
		if(tit->getClassId()==Id)	
			return true;
	}
	return false;
}

bool oEvent::isClubUsed(int Id) const
{
	//Search runners
  oRunnerList::const_iterator it;
	for (it=Runners.begin(); it != Runners.end(); ++it){		
		if(it->getClubId()==Id)	
			return true;
	}	

	//Search teams
	oTeamList::const_iterator tit;
	for (tit=Teams.begin(); tit != Teams.end(); ++tit){		
		if(tit->getClubId()==Id)	
			return true;
	}

	return false;
}

bool oEvent::isRunnerUsed(int Id) const
{
	//Search teams
  oTeamList::const_iterator tit;
	for (tit=Teams.begin(); tit != Teams.end(); ++tit){		
    if(tit->isRunnerUsed(Id)) {	
      if (tit->Class && tit->Class->isSingleRunnerMultiStage())
        //Don't report single-runner-teams as blocking
        continue;
      return true; 
    }
	}

	return false;
}

bool oEvent::isControlUsed(int Id) const
{
	oCourseList::const_iterator it;	

	for (it=Courses.begin(); it != Courses.end(); ++it){
		
		for(int i=0;i<it->nControls;i++)
			if(it->Controls[i] && it->Controls[i]->Id==Id)
				return true;
	}	
	return false;
}

bool oEvent::classHasResults(int Id) const
{
	oRunnerList::const_iterator it;

  for (it=Runners.begin(); it != Runners.end(); ++it) {
    if (it->isRemoved())
      continue;
		if( (Id == 0 || it->getClassId() == Id) && (it->getCard() || it->FinishTime))	
      return true;
  }
	
	return false;
}

bool oEvent::classHasTeams(int Id) const
{
  oTeamList::const_iterator it;

  for (it=Teams.begin(); it != Teams.end(); ++it)
    if(it->getClassId()==Id)
      return true;
  
  return false;
}

void oEvent::generateVacancyList(gdioutput &gdi, GUICALLBACK cb)
{
	sortRunners(ClassStartTime);
	oRunnerList::iterator it;

	// BIB, START, NAME, CLUB, SI
	int dx[5]={0, 0, 70, 150};

	bool withbib=hasBib(true, false);
	int i;

	if (withbib) for (i=1;i<4;i++) dx[i]+=40;
		
	int y=gdi.getCY(); 
	int x=gdi.getCX();
	int lh=gdi.getLineHeight();
  
  const int yStart = y;
  int nVac = 0;
  for (it=Runners.begin(); it != Runners.end(); ++it) {	
    if (it->skip() || !it->isVacant())
      continue;
    nVac++;
  }

  int nCol = 1 + min(3, nVac/10);
  int RunnersPerCol = nVac / nCol;
  
  char bf[256];
  int nRunner = 0;
	y+=lh;

	int Id=0;
	for(it=Runners.begin(); it != Runners.end(); ++it){	
    if (it->skip() || !it->isVacant())
      continue;
		
		if (it->getClassId() != Id) {
			Id=it->getClassId();
			y+=lh/2;

      if (nRunner>=RunnersPerCol) {
        y = yStart;
        x += dx[3]+5;
        nRunner = 0;
      }


			gdi.addStringUT(y, x+dx[0], 1, it->getClass());
			y+=lh+lh/3;	
		}

		oDataInterface DI=it->getDI();

		if (withbib) {
      string bib=it->getBib();

			if (!bib.empty()) {
				gdi.addStringUT(y, x+dx[0], 0, bib);
			}
		}
		gdi.addStringUT(y, x+dx[1], 0, it->getStartTimeS(), 0,  cb).setExtra(&*it);

		_itoa_s(it->Id, bf, 256, 10);
    gdi.addStringUT(y, x+dx[2], 0, it->Name, dx[3]-dx[2]-4, cb).setExtra(&*it);
		//gdi.addStringUT(y, x+dx[3], 0, it->getClub());
		
		y+=lh;
    nRunner++;
  }
  if (nVac==0)
    gdi.addString("", y, x, 0, "Inga vakanser tillg�ngliga. Vakanser skapas vanligen vid lottning.");
	gdi.updateScrollbars();
}

void oEvent::generateInForestList(gdioutput &gdi, GUICALLBACK cb, GUICALLBACK cb_nostart)
{
  //Lazy setup: tie runners and persons
  oFreePunch::rehashPunches(*oe, 0, 0);
  
  // Map cardNo -> punch
  multimap<int, pFreePunch> punchHash;

  typedef multimap<int, pFreePunch>::const_iterator TPunchIter; 
        
  for (oFreePunchList::iterator it = punches.begin(); it != punches.end(); ++it) {
    punchHash.insert(make_pair(it->getCardNo(), &*it));
  }


  sortTeams(ClassStartTime, 0);
	int y=gdi.getCY();
	int x=gdi.getCX();
	int lh=gdi.getLineHeight();

  oTeamList::iterator it;
	gdi.addStringUT(2, lang.tl("Kvar-i-skogen") + MakeDash(" - ") + getName());
	y+=lh/2;

	gdi.addStringUT(1, getDate());

  gdi.dropLine();

  y+=3*lh;
  int id=0;
  int nr=0;

  for(it=Teams.begin(); it!=Teams.end(); ++it) {
    if(it->Status==StatusUnknown) {

      if (id != it->getClassId()) {
        if(nr>0) {
          gdi.addString("", y, x, 0, "Antal: X#"+itos(nr));
          y+=lh;
          nr=0;
        }
        y += lh;
        id = it->getClassId();
        gdi.addStringUT(y, x, 1, it->getClass());
        y += lh;
      }
      gdi.addStringUT(y, x, 0, it->getClass());
      nr++;
      gdi.addStringUT(y, x+100, 0, it->Name, 0, cb).setExtra(&*it);
      y+=lh;
    }
  } 

  if(nr>0) {
    gdi.addString("", y, x, 0, "Antal: X#"+itos(nr));
    y+=lh;
  }

  {  
    int tnr = 0;
    id=0;
    nr=0;
	  sortRunners(ClassStartTime);

	  oRunnerList::iterator it;

	  int dx[4]={0, 70, 350, 470};
	  int y=gdi.getCY();
	  int x=gdi.getCX();
	  int lh=gdi.getLineHeight();

    y+=lh;
	  char bf[256];

	  y=gdi.getCY();

	  for(it=Runners.begin(); it != Runners.end(); ++it){	
      if (it->skip() || it->needNoCard())
        continue;

		  if(it->Status==StatusUnknown) {

        if (id != it->getClassId()) {
          if(nr>0) {
            gdi.addString("", y, x, 0, "Antal: X#"+itos(nr));
            y+=lh;
            nr=0;
          }
          y += lh;
          id = it->getClassId();
          gdi.addStringUT(y, x, 1, it->getClass());
          y += lh;
        }

        bool hasPunch = false;
        string punches;
        pair<TPunchIter, TPunchIter> range = punchHash.equal_range(it->getCardNo());
        for (TPunchIter pit = range.first; pit != range.second; ++pit) {
          if (pit->second->tRunnerId == it->getId()) {
            if (hasPunch)
              punches.append(", ");
            else
              hasPunch = true;
            
            punches.append(pit->second->getSimpleString());
          }
        }

			  gdi.addStringUT(y, x+dx[0], 0, it->getStartTimeS());
        string club = it->getClub();
        if (!club.empty())
          club = " (" + club + ")";

			  gdi.addStringUT(y, x+dx[1], 0, it->Name+club, dx[2]-dx[1]-4, cb).setExtra(&*it);
			  _itoa_s(it->Id, bf, 256, 10);
        nr++;
        tnr++;

        if (hasPunch) {
          RECT rc = gdi.addString("", y, x+dx[2], 0, "(har st�mplat)", dx[3]-dx[2]-4).TextRect;
          capitalize(punches);
          gdi.addToolTip(punches, 0, &rc);
          punches = "�rlig";
          capitalize(punches);
          punches = "�rlig";
          capitalize(punches);
          
        }
			  //gdi.addString(bf, y, x+dx[2], 0, "Startade inte", dx[3]-dx[2]-4, cb_nostart);

			  gdi.addStringUT(y, x+dx[3], 0, it->getClass());
			  y+=lh;
		  }
	  }
    if(nr>0) {
      gdi.addString("", y, x, 0, "Antal: X#"+itos(nr));
      y+=lh;
    }

    if (tnr == 0 && Runners.size()>0) {
      gdi.addString("", 10, "inforestwarning");
    }
  }
	gdi.updateScrollbars();
}

void oEvent::generateMinuteStartlist(gdioutput &gdi)
{
	sortRunners(SortByStartTime);
	oRunnerList::iterator it;

	int dx[5]={0, 70, 300, 410, 510};
	int y=gdi.getCY();
	int x=gdi.getCX();	
	int lh=gdi.getLineHeight();

  vector<int> blocks;
  vector<string> starts;
  getStartBlocks(blocks, starts);
  
  char bf[256];
  for (size_t k=0;k<blocks.size();k++) {
    gdi.dropLine();
    if (k>0)
      gdi.addStringUT(gdi.getCY()-1, 0, pageNewPage, "");          

    gdi.addStringUT(boldLarge, lang.tl("Minutstartlista") +  MakeDash(" - ") + getName());
    if (!starts[k].empty()) {
      sprintf_s(bf, lang.tl("%s, block: %d").c_str(), starts[k].c_str(), blocks[k]);
      gdi.addStringUT(fontMedium, bf);
    }
    else if (blocks[k]!=0) {
      sprintf_s(bf, lang.tl("Startblock: %d").c_str(),  blocks[k]);
      gdi.addStringUT(fontMedium, bf);
    }
   
    y = gdi.getCY();
  	
	  int LastStartTime=-1;
	  for (it=Runners.begin(); it != Runners.end(); ++it) {		
      if (it->Class && it->Class->getBlock()!=blocks[k])
        continue;
      if (it->Class && it->Class->getStart() != starts[k] ) 
        continue;
      if (!it->Class && blocks[k]!=0)
        continue;
		  if (LastStartTime!=it->StartTime) {
			  y+=lh/2;
			  LastStartTime=it->StartTime;
        gdi.addStringUT(y, x+dx[0], boldText, it->getStartTimeS());
  		  y+=lh;
		  }

      if (it->getCardNo()>0)
        gdi.addStringUT(y, x+dx[0], fontMedium, itos(it->getCardNo()));
		
      string name;
      if (it->getBib().empty())
        name = it->getName();
      else
        name = it->getBib() + ", " + it->getName();
      gdi.addStringUT(y, x+dx[1], fontMedium, name, dx[2]-dx[1]-4);
      gdi.addStringUT(y, x+dx[2], fontMedium, it->getClub(), dx[3]-dx[2]-4);
		  gdi.addStringUT(y, x+dx[3], fontMedium, it->getClass(), dx[4]-dx[3]-4);			
      gdi.addStringUT(y, x+dx[4], fontMedium, it->getCourse() ? it->getCourse()->getName() : "No course");
		  y+=lh;
	  }
  }
	gdi.refresh();
}

void oEvent::generateResultlistFinishTime(gdioutput &gdi, bool PerClass, GUICALLBACK cb)
{
  calculateResults(RTClassResult);

	if(PerClass)
		sortRunners(ClassFinishTime);
	else
		sortRunners(SortByFinishTime);

	oRunnerList::iterator it;

	int dx[5]={0, 40, 300, 490, 550};
	int y=gdi.getCY();
	int x=gdi.getCX();
	int lh=gdi.getLineHeight();

	gdi.addStringUT(2, lang.tl("Resultat") + MakeDash(" - ") + getName());
	y+=lh/2;

	
	gdi.addStringUT(1, getDate());
	y+=lh;
	y+=(3*lh)/2;

	int FirstStart=getFirstStart();
	
	int Id=0;
	int place=0;
	int order=1;
	int lasttime=0;
	char bf[256];

	for(it=Runners.begin(); it != Runners.end(); ++it){		
		if(it->Status!=0)	{
			if(PerClass && it->getClassId()!=Id){
				//Next class
				Id=it->getClassId();
				y+=lh/2;

				gdi.addStringUT(y, x+dx[0], 1, it->getClass());

				FirstStart=getFirstStart(Id);
				
				order=1;
				lasttime=0;
				y+=lh+lh/3;	
			}

			if(it->FinishTime>lasttime) {
				place=order;
				lasttime=it->FinishTime;
			}
			order++;

			if(it->getAge()<100) {
				RECT rc;
				rc.left=x;
				rc.right=x+dx[3];
				rc.top=y;
				rc.bottom=y+lh;
				gdi.addRectangle(rc);
			}	
			gdi.addStringUT(y, x+dx[1], 0, it->Name, dx[2]-dx[1]-4, cb);
			gdi.addStringUT(y, x+dx[2], 0, it->getClub(), dx[3]-dx[2]-4);
		
			
			if(it->getStatus()==StatusOK){
				_itoa_s(place, bf, 256, 10);
				gdi.addStringUT(y, x+dx[0], 0, string(bf)+".");

				char tstr[32];
				int rt=it->FinishTime-FirstStart;

				if(rt>=3600)
					sprintf_s(tstr, 32, "%d:%02d:%02d", rt/3600,(rt/60)%60, rt%60);
				else
					sprintf_s(tstr, 32, "%d:%02d", (rt/60), rt%60);

				gdi.addStringUT(y, x+dx[3], 0, string(tstr));

				if(PerClass)
					gdi.addStringUT(y, x+dx[4], 0, string("(")+it->getPlaceS()+", "+it->getRunningTimeS()+")");
				else
					gdi.addStringUT(y, x+dx[4], 0, string("(")+it->getClass()+": "+it->getPlaceS()+", "+it->getRunningTimeS()+")");

			}
			else
				gdi.addStringUT(y, x+dx[3], 0, it->getStatusS());

			y+=lh;
		}
	}
	//gdi.UpdatePos(x+700,y+40,0,0);
	gdi.updateScrollbars();
}

void oEvent::generateResultlistFinishTime(const string &file, bool PerClass)
{
	calculateResults(RTClassResult);

	if(PerClass)
		sortRunners(ClassFinishTime);
	else
		sortRunners(SortByFinishTime);


	oRunnerList::iterator it;

	ofstream fout(file.c_str());

	fout << "<!DOCTYPE html PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\"\n" <<
		"\"http://www.w3.org/TR/html4/loose.dtd\">\n\n";
	
	fout << "<html>\n<head>\n<meta http-equiv=\"Content-Type\" content=\"text/html; charset=iso-8859-1\">\n";

	fout << "<title>" << "Resultat &ndash; " + getName() << "</title>\n</head>\n\n";

	fout << "<body bgcolor=\"#FFFFFF\" text=\"#000000\" link=\"#FF0000\" vlink=\"#AA0000\" alink=\"#0000DD\">\n";


	fout << "<h2>" << "Resultat &ndash; " + getName() << "</h2>\n";

	char bf1[256];
	char bf2[256];
    GetTimeFormat(LOCALE_USER_DEFAULT, 0, NULL, NULL, bf2, 256);
	GetDateFormat(LOCALE_USER_DEFAULT, 0, NULL, NULL, bf1, 256);

	fout << "Skapad av <i>MeOS</i>: " << bf1 << " "<< bf2 << "\n";

	int FirstStart=getFirstStart();

	int Id=0;
	int place=0;
	int order=1;
	int lasttime=0;
	char bf[256];

	fout << "<table>\n";
	
	fout << "<tr>";
	fout << "<td width=\"20\">&nbsp;</td>";
	fout << "<td width=\"200\">&nbsp;</td>";
	fout << "<td width=\"200\">&nbsp;</td>";
	fout << "<td width=\"80\">&nbsp;</td>";
	fout << "</tr>";

	for(it=Runners.begin(); it != Runners.end(); ++it){		
		if(it->Status!=0){
			if(PerClass && it->getClassId()!=Id){

				//Next class
				Id=it->getClassId();
				
				if(Id)
					fout << "<tr><td colspan=\"4\">&nbsp;</td></tr>\n";

				Id=it->getClassId();
		
				fout << "<tr><td colspan=\"4\"><b>" << it->getClass() << "</b></td></tr>\n";
	
				FirstStart=getFirstStart(Id);
				
				order=1;
				lasttime=0;			
			}

			if(it->FinishTime>lasttime)
			{
				place=order;
				lasttime=it->FinishTime;
			}
			order++;
	

			fout << "<tr>\n";

			if(it->getStatus()==StatusOK){
				_itoa_s(place, bf, 256, 10);
				fout << "<td>" <<  string(bf)+"." << "</td>\n";
			}
			else
				fout << "<td>" << "&nbsp;" << "</td>\n";

			fout << "<td>" <<  it->Name << "</td>\n";
			fout << "<td>" <<  it->getClub() << "</td>\n";

	
			if(it->getStatus()==StatusOK)	{
				
				char tstr[32];
				int rt=it->FinishTime-FirstStart;

				if(rt>=3600)
					sprintf_s(tstr, 32, "%d:%02d:%02d", rt/3600,(rt/60)%60, rt%60);
				else
					sprintf_s(tstr, 32, "%d:%02d", (rt/60), rt%60);

				fout << "<td>" <<  tstr << "</td>\n";

				
				
				if(PerClass)
					fout << "<td>" << string("(")+"<b>"+it->getPlaceS()+"</b>, "+it->getRunningTimeS()+")</td>\n";
				else
					fout << "<td>" << string("(")+it->getClass()+": <b>"+it->getPlaceS()+"</b>, "+it->getRunningTimeS()+")</td>\n";



			}			else
			{
				fout << "<td>" <<  it->getStatusS() << "</td>\n";
				fout << "<td>" <<  string("(")+it->getClass()+")" << "</td>\n";
			}
			fout << "</tr>\n";
		
		}
	}
	fout << "</table>\n";

	fout << "</body></html>\n";
}


bool oEvent::empty() const
{
  return Name.empty();
}

void oEvent::clearListedCmp()
{
  cinfo.clear();
}

bool oEvent::enumerateCompetitions(const char *file, const char *filetype)
{
	WIN32_FIND_DATA fd;
	
	char dir[MAX_PATH];
	//char buff[MAX_PATH];
	char FullPath[MAX_PATH];
	
	strcpy_s(dir, MAX_PATH, file);

	if(dir[strlen(file)-1]!='\\')
		strcat_s(dir, MAX_PATH, "\\");
	
	strcpy_s(FullPath, MAX_PATH, dir);
	
	strcat_s(dir, MAX_PATH, filetype);
	
	HANDLE h=FindFirstFile(dir, &fd);
	
	if(h==INVALID_HANDLE_VALUE)
		return false;
	
	bool more=true;//=FindNextFile(h, &fd);
	
	int id=1;


	/*
  list<CompetitionInfo> saved;
  list<CompetitionInfo>::iterator it;

	for (it=cinfo.begin(); it!=cinfo.end(); ++it) {
		if(it->Server.length()>0)
			saved.push_back(*it);
	}

	cinfo=saved;
	*/
  cinfo.clear();

	while (more) {
		if(fd.cFileName[0]!='.') //Avoid .. and .
		{
			char FullPathFile[MAX_PATH];
			strcpy_s(FullPathFile, MAX_PATH, FullPath);
			strcat_s(FullPathFile, MAX_PATH, fd.cFileName);

			CompetitionInfo ci;

			ci.FullPath=FullPathFile;
			ci.Name="";
			ci.Date="2007-01-01";
			ci.Id=id++;

			SYSTEMTIME st;
			FileTimeToSystemTime(&fd.ftLastWriteTime, &st);
			ci.Modified=convertSystemTime(st);
			xmlparser xp;

      try {
			  xp.read(FullPathFile, 8);
  			
        const xmlobject date=xp.getObject("Date");

			  if(date) ci.Date=date.get();

			  const xmlobject name=xp.getObject("Name");

			  if(name) ci.Name=name.get();

        const xmlobject annotation=xp.getObject("Annotation");

			  if(annotation)
          ci.Annotation=annotation.get();

			  const xmlobject nameid = xp.getObject("NameId");
			  if(nameid) 
          ci.NameId = nameid.get();

			  cinfo.push_front(ci);
      }
      catch (std::exception &) {
        // XXX Do what??
      }
		}
		more=FindNextFile(h, &fd)!=0;
	}
	
	FindClose(h);

  if (!getServerName().empty())
    msListCompetitions(this);

	return true;
}

bool oEvent::enumerateBackups(const char *file) 
{
  backupInfo.clear();

  enumerateBackups(file, "*.meos.bu?", 1);
  enumerateBackups(file, "*.removed", 1);
  enumerateBackups(file, "*.dbmeos*", 2);
  backupInfo.sort();
  return true;
}

bool oEvent::listBackups(gdioutput &gdi, GUICALLBACK cb) 
{
  int y = gdi.getCY();
  int x = gdi.getCX();

  list<BackupInfo>::iterator it = backupInfo.begin();

  while (it != backupInfo.end()) {
    string type = lang.tl(it->type==1 ? "backup" : "serverbackup"); 
    gdi.addStringUT(y,x,boldText, it->Name + " (" + it->Date + ") " + type, 400);
    string date = it->Modified;
    string file = it->fileName + it->Name;
    while(file == it->fileName + it->Name) {
      y += gdi.getLineHeight();

      gdi.addStringUT(y,x+30, 0, it->Modified, 400, cb).setExtra(&*it);
      ++it;
      if (it == backupInfo.end())
        break;
    }
    y+=gdi.getLineHeight()*2;
  }

  return true;
}

bool BackupInfo::operator<(const BackupInfo &ci)
{
  if (Date!=ci.Date)
    return Date>ci.Date;

  if (fileName!=ci.fileName)
    return fileName<ci.fileName;

  return Modified>ci.Modified;
}


bool oEvent::enumerateBackups(const char *file, const char *filetype, int type)
{
	WIN32_FIND_DATA fd;
	char dir[MAX_PATH];
	char FullPath[MAX_PATH];
	
	strcpy_s(dir, MAX_PATH, file);

	if(dir[strlen(file)-1]!='\\')
		strcat_s(dir, MAX_PATH, "\\");
	
	strcpy_s(FullPath, MAX_PATH, dir);
	strcat_s(dir, MAX_PATH, filetype);
	HANDLE h=FindFirstFile(dir, &fd);
	
	if(h==INVALID_HANDLE_VALUE)
		return false;
	
	bool more=true;
  while (more) {
    if (fd.cFileName[0]!='.') {//Avoid .. and .
			char FullPathFile[MAX_PATH];
			strcpy_s(FullPathFile, MAX_PATH, FullPath);
			strcat_s(FullPathFile, MAX_PATH, fd.cFileName);

			BackupInfo ci;
      
      ci.type = type;
			ci.FullPath=FullPathFile;
			ci.Name="";
			ci.Date="2007-01-01";
      ci.fileName = fd.cFileName;

      size_t pIndex = ci.fileName.find_first_of(".");
      if (pIndex>0 && pIndex<ci.fileName.size())
        ci.fileName = ci.fileName.substr(0, pIndex);

			SYSTEMTIME st;
      FILETIME localTime;
			FileTimeToLocalFileTime(&fd.ftLastWriteTime, &localTime);
      FileTimeToSystemTime(&localTime, &st);
     
			ci.Modified=convertSystemTime(st);
			xmlparser xp;

      try {
			  xp.read(FullPathFile, 5);
			  //xmlobject *xo=xp.getObject("meosdata");
			  const xmlobject date=xp.getObject("Date");

			  if(date) ci.Date=date.get();

			  const xmlobject name=xp.getObject("Name");

			  if(name) ci.Name=name.get();

			  backupInfo.push_front(ci);
      }
      catch (std::exception &) {
        //XXX Do what?
      }
		}
		more=FindNextFile(h, &fd)!=0;
	}
	
	FindClose(h);

	return true;
}

bool oEvent::fillCompetitions(gdioutput &gdi, 
                              const string &name, int type,
                              const string &select) {	
	cinfo.sort();
	cinfo.reverse();
  list<CompetitionInfo>::iterator it;

	gdi.clearList(name);
	string b;
  int idSel = -1;
	//char bf[128];
	for (it=cinfo.begin(); it!=cinfo.end(); ++it) {
    string annotation;
    if (!it->Annotation.empty())
      annotation = " (" + it->Annotation + ")";
		if (it->Server.length()==0) {
      if (type==0 || type==1) {
        if (it->NameId == select && !select.empty())
          idSel = it->Id;
        string bf = "[" + it->Date + "] " + it->Name;
        gdi.addItem(name, bf + annotation, it->Id);
      }
		}
		else if (type==0 || type==2) {
      if (it->NameId == select && !select.empty())
        idSel = it->Id;
      string bf;

      if (type==0)
        bf = lang.tl("Server: [X] Y#" + it->Date + "#" + it->Name);
      else
         bf = "[" + it->Date + "] " + it->Name;
      
			gdi.addItem(name, bf + annotation, 10000000+it->Id);
		}
	}

  if (idSel != -1)
    gdi.selectItemByData(name.c_str(), idSel);
	return true;
}

void tabAutoKillMachines();
void clearClassData(gdioutput &gdi);

void oEvent::checkDB()
{
  if(HasDBConnection) {
    vector<string> err;
    int k=checkChanged(err);

#ifdef _DEBUG
    if(k>0) {
      char bf[256];
      sprintf_s(bf, "Databasen inneh�ller %d osynkroniserade �ndringar.", k);
      string msg(bf);
      for(int i=0;i < min<int>(err.size(), 10);i++)
        msg+=string("\n")+err[i];

      MessageBox(0, msg.c_str(), "Varning/Fel", MB_OK);
    }
#endif   
  }
  updateTabs();
  gdibase.setWindowTitle(getTitleName());
}

void clearSpeakerData(gdioutput &gdi);
void destroyExtraWindows();


void oEvent::clear()
{
  checkDB();

  if(HasDBConnection)
    msMonitor(0);

	HasDBConnection=false;
  HasPendingDBConnection = false;

#ifndef MEOSDB
  destroyExtraWindows();

  while (!tables.empty()) {
    tables.begin()->second->releaseOwnership();
    tables.erase(tables.begin());
  }

  getRunnerDatabase().releaseTables();
#endif

  tOriginalName.clear();
  Id=0;
  dataRevision = 0;
  tClubDataRevision = -1;

	ZeroTime=0;
	Name.clear();
  Annotation.clear();

  //Make sure no daemon is hunting us.
  tabAutoKillMachines();

	//Order of destruction is extreamly important...
  runnerById.clear();
  Runners.clear();
	Teams.clear();

	Classes.clear();
	Courses.clear();
	courseIdIndex.clear();

  Controls.clear();
	
	Cards.clear();
	Clubs.clear();
  clubIdIndex.clear();
  
  punchIndex.clear();
	punches.clear();
	
	updateFreeId();
	
	strcpy_s(CurrentNameId, "");
	strcpy_s(CurrentFile, "");

  sqlCounterRunners=0;
	sqlCounterClasses=0;
	sqlCounterCourses=0;
	sqlCounterControls=0;
	sqlCounterClubs=0;
	sqlCounterCards=0;
	sqlCounterPunches=0;
	sqlCounterTeams=0;

  sqlUpdateControls.clear();
  sqlUpdateCards.clear();
  sqlUpdateClubs.clear();
  sqlUpdateClasses.clear();
  sqlUpdateCourses.clear();
  sqlUpdateRunners.clear();
  sqlUpdatePunches.clear();
  sqlUpdateTeams.clear();
  
  vacantId = 0;

  timelineClasses.clear();
  timeLineEvents.clear();
  nextTimeLineEvent = 0;

  tCurrencyFactor = 1;
  tCurrencySymbol = "kr";
  tCurrencySeparator = ",";
  tCurrencyPreSymbol = false;

  //Reset speaker data structures.
#ifndef MEOSDB
  listContainer->clearExternal();
  clearSpeakerData(gdibase);
  clearClassData(gdibase);
#else
  throw std::exception("Bad call");
#endif
}

bool oEvent::deleteCompetition()
{
	if (!empty() && !HasDBConnection) {
    string removed = string(CurrentFile)+".removed";
    ::remove(removed.c_str()); //Delete old removed file
    ::rename(CurrentFile, removed.c_str());
		return true;
	}
	else return false;
}

void oEvent::newCompetition(const string &name)
{
	clear();

	SYSTEMTIME st;
	GetLocalTime(&st);

  Date = convertSystemDate(st);
	ZeroTime = st.wHour*3600;

	Name = name;
  tOriginalName.clear();
	oEventData->initData(oData, sizeof(oData));

  getDI().setString("Organizer", getPropertyString("Organizer", ""));
  getDI().setString("Street", getPropertyString("Street", ""));
  getDI().setString("Address", getPropertyString("Address", ""));
  getDI().setString("EMail", getPropertyString("EMail", ""));
	getDI().setString("Homepage", getPropertyString("Homepage", ""));
	
  getDI().setInt("CardFee", getPropertyInt("CardFee", 25));
  getDI().setInt("EliteFee", getPropertyInt("EliteFee", 130));
  getDI().setInt("EntryFee", getPropertyInt("EntryFee", 90));
  getDI().setInt("YouthFee", getPropertyInt("YouthFee", 50));
  
  getDI().setInt("SeniorAge", getPropertyInt("SeniorAge", 0));
  getDI().setInt("YouthAge", getPropertyInt("YouthAge", 16));
  
  getDI().setString("Account", getPropertyString("Account", ""));
  getDI().setString("LateEntryFactor", getPropertyString("LateEntryFactor", "50 %"));

  getDI().setString("CurrencySymbol", getPropertyString("CurrencySymbol", "$"));
  getDI().setString("CurrencySeparator", getPropertyString("CurrencySeparator", "."));
  getDI().setInt("CurrencyFactor", getPropertyInt("CurrencyFactor", 1));
  getDI().setInt("CurrencyPreSymbol", getPropertyInt("CurrencyPreSymbol", 0));

  setCurrency(-1, "", "", 0);
/*  FILETIME ft;
  SystemTimeToFileTime(&st, &ft); 
  __int64 &ft64 = *(__int64 *)&ft;
  ft64 -= (__int64)1000 * 60 *60 * 24 *7 * 1000 * 10;
  
  SYSTEMTIME st_week;
  FileTimeToSystemTime(&ft, &st_week);
  char entryDate[32];
  sprintf_s(entryDate, "%d-%02d-%02d", st_week.wYear, 
                                       st_week.wMonth, 
                                       st_week.wDay);
  
  getDI().setDate("OrdinaryEntry", entryDate); 
*/
  char file[260];
	char filename[64];
	sprintf_s(filename, 64, "meos_%d%02d%02d_%02d%02d%02d_%X.meos", 
		st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);	
	
	//strcpy_s(CurrentNameId, filename);
	getUserFile(file, filename);

	strcpy_s(CurrentFile, MAX_PATH, file);
	_splitpath_s(CurrentFile, NULL, 0, NULL,0, CurrentNameId, 64, NULL, 0);
  int i=0;
  while (CurrentNameId[i]) {
    if(CurrentNameId[i]=='.') { 
      CurrentNameId[i]=0;
      break;
    }
    i++;
  }

  oe->updateTabs();
}

void oEvent::reEvaluateClass(int ClassId, bool DoSync)
{
	if(DoSync)
		autoSynchronizeLists(false);

	oRunnerList::iterator it;

  pClass pc=getClass(ClassId);
  if (pc) {
    pc->resetLeaderTime();
    pc->reinitialize();
  }
	vector<int> mp;
  bool needupdate = true;
  int leg = 0;
  while (needupdate) {
    needupdate = false;
	  for(it=Runners.begin(); it != Runners.end(); ++it){		
      if(!it->isRemoved() && it->Class && it->Class->getId() == ClassId) {
        if (it->tLeg == leg) {
          it->evaluateCard(mp); // Must not sync!
			    if(DoSync)
				    it->synchronize(true);
			    mp.clear();
        }
        else if (it->tLeg > leg)
          needupdate = true;
		  }
    }
    leg++;
  }
  
  // Update team start times etc.
  for(oTeamList::iterator tit=Teams.begin();tit!=Teams.end();++tit) {
    if(!tit->isRemoved() && tit->Class && tit->Class->getId() == ClassId) {
      tit->apply(DoSync);
    }
  }
}

void oEvent::reEvaluateCourse(int CourseId, bool DoSync)
{
	oRunnerList::iterator it;
	
	if(DoSync)
		autoSynchronizeLists(false);

	vector<int> mp;
  set<int> classes;
	for(it=Runners.begin(); it != Runners.end(); ++it){		
		if(it->getCourse() && it->getCourse()->getId()==CourseId){
      classes.insert(it->getClassId());
		}
	}

  for(set<int>::iterator sit=classes.begin();sit!=classes.end(); ++sit)
    reEvaluateClass(*sit, DoSync);
}

void oEvent::reEvaluateAll(bool doSync)
{	
	if(doSync)
		autoSynchronizeLists(false);

  for(oClassList::iterator it=Classes.begin();it!=Classes.end();++it) {
    it->resetLeaderTime();
    it->reinitialize();
  }
  oRunnerList::iterator it;
	vector<int> mp;
	bool needupdate = true;
  int leg = 0;
  while (needupdate) {
    needupdate = false;
	  for (it=Runners.begin(); it != Runners.end(); ++it) {		
		  if (!it->isRemoved()) {
        if (it->tLeg == leg) {
          it->evaluateCard(mp); // Must not sync!
			    if(doSync)
				    it->synchronize(true);
			    mp.clear();
        }
        else if (it->tLeg>leg)
          needupdate = true;
		  }
	  }
    leg++;
  }

  // Update team start times etc.
  for(oTeamList::iterator tit=Teams.begin();tit!=Teams.end();++tit) {
    if(!tit->isRemoved()) {
      tit->apply(doSync);
    }
  }
}

void oEvent::reEvaluateChanged()
{	
  for(oClassList::iterator it=Classes.begin();it!=Classes.end();++it)  
    it->resetLeaderTime();

  oRunnerList::iterator it;
	vector<int> mp;
  bool needupdate = true;
  int leg = 0;
  while (needupdate) {
    needupdate = false;
	  for (it=Runners.begin(); it != Runners.end(); ++it) {		
		  if (!it->isRemoved()) {
        if (it->tLeg == leg) {
          if (it->wasSQLChanged())
			      it->evaluateCard(mp);
          else
            it->storeTimes();

  		    mp.clear();
        }
        else if (it->tLeg>leg)
          needupdate = true;
		  }
	  }
    leg++;
  }
}

void oEvent::reCalculateLeaderTimes(int classId)
{
  for (oClassList::iterator it=Classes.begin(); it != Classes.end(); ++it) {	
    if (!it->isRemoved() && (classId==it->getId() || classId==0))
      it->resetLeaderTime();
  }
	bool needupdate = true;
  int leg = 0;
  while (needupdate) {
    needupdate = false;
    for (oRunnerList::iterator it=Runners.begin(); it != Runners.end(); ++it) {		
		  if (!it->isRemoved() && (classId==0 || classId==it->getClassId())) {
        if (it->tLeg == leg)
          it->storeTimes();
        else if (it->tLeg>leg)
          needupdate = true;
		  }
	  }  
    leg++;
  }
}

string oEvent::getCurrentTimeS() const
{
	SYSTEMTIME st;
	GetLocalTime(&st);

	char bf[64];
	sprintf_s(bf, 64, "%02d:%02d:%02d", st.wHour, st.wMinute, st.wSecond);
	return bf;
}

int oEvent::findBestClass(const SICard &card, vector<pClass> &classes) const
{
  classes.clear();
	int Distance=-1000;
	oClassList::const_iterator it;	

	for (it=Classes.begin(); it != Classes.end(); ++it) {
		vector<pCourse> courses;
    it->getCourses(0, courses);
    bool insertClass = false; // Make sure a class is only included once

    for (size_t k = 0; k<courses.size(); k++) {
      pCourse pc = courses[k];
		  if (pc) {
			  int d=pc->distance(card);

			  if(d>=0) {
				  if(Distance<0) Distance=1000;

				  if(d<Distance) {
					  Distance=d;
            classes.clear();
            insertClass = true;
            classes.push_back(pClass(&*it));
				  }
          else if(d == Distance) {
            if (!insertClass) {
              insertClass = true;
              classes.push_back(pClass(&*it));
            }
          }
			  }
			  else {
				  if(Distance<0 && d>Distance) {
					  Distance = d;
            classes.clear();
            insertClass = true;
            classes.push_back(pClass(&*it));
				  }
          else if (Distance == d) {
            if (!insertClass) {
              insertClass = true;
              classes.push_back(pClass(&*it));
            }
          }
			  }
		  }
    }
	}
  return Distance;
}

void oEvent::convertTimes(SICard &sic) const
{
  if (sic.convertedTime)
    return;

  sic.convertedTime = true;

	if(sic.CheckPunch.Code!=-1){
		if(sic.CheckPunch.Time<ZeroTime)
			sic.CheckPunch.Time+=(24*3600);

		sic.CheckPunch.Time-=ZeroTime;
	}

	if(sic.StartPunch.Code!=-1){
		if(sic.StartPunch.Time<ZeroTime)
			sic.StartPunch.Time+=(24*3600);
		
		sic.StartPunch.Time-=ZeroTime;
	}

	for(unsigned k=0;k<sic.nPunch;k++){
		if(sic.Punch[k].Code!=-1){
			if(sic.Punch[k].Time<ZeroTime)
				sic.Punch[k].Time+=(24*3600);
			
			sic.Punch[k].Time-=ZeroTime;
		}
	}
	
	
	if(sic.FinishPunch.Code!=-1){
		if(sic.FinishPunch.Time<ZeroTime)
			sic.FinishPunch.Time+=(24*3600);
	
		sic.FinishPunch.Time-=ZeroTime;
	}	
}

int oEvent::getFirstStart(int ClassId)
{
	oRunnerList::iterator it=Runners.begin();
	int MinTime=3600*24;

	while(it!=Runners.end()){
		if(ClassId==0 || it->getClassId()==ClassId)
			if(it->StartTime<MinTime && it->Status==StatusOK && it->StartTime!=0)
				MinTime=it->StartTime;

		++it;
	}
	
	if(MinTime==3600*24)
		MinTime=0;

	return MinTime;
}

bool oEvent::exportONattResults(gdioutput &gdi, const string &file)
{
	ofstream fout(file.c_str());

	calculateResults(RTClassResult);

	sortRunners(ClassFinishTime);

	oRunnerList::iterator it;

	int Id=0;

	int ClassNr=1;
	bool warn=false;
	char bf[256];

	int FirstStart=0;

	for(it=Runners.begin(); it != Runners.end(); ++it){		
		if(it->Status!=0 && it->Status!=StatusDNS){
			if(it->getClassId()!=Id){
				//Next class
				Id=it->getClassId();			
				FirstStart=getFirstStart(Id);
				
				if(it->getClass()=="L�ng")
					ClassNr=1;
				else if(it->getClass()=="Mellan")
					ClassNr=2;
				else if(it->getClass()=="Kort")
					ClassNr=3;
				else{
					if(!warn)
						gdi.alert("Varning: Funktionen anpassad f�r klassnamn L�ng, Mellan och Kort");
					warn=true;
					ClassNr=Id;
				}
			}

			fout << ClassNr << ";";

			int st=it->StartTime-FirstStart;
			sprintf_s(bf, 64, "%d.%02d;", st/60, st%60);
			fout << bf;

			if(it->getStatus()==StatusOK){
				int ft=it->FinishTime-FirstStart;
				sprintf_s(bf, 64, "%d.%02d;", ft/60, ft%60);
				fout << bf;
			}
			else fout << ";";

			fout << it->getName() << ";";
			fout << it->getClub() << endl;

		}
	}

	return true;
}

bool oEvent::hasRank() const
{	
	oRunnerList::const_iterator it;	

	for (it=Runners.begin(); it != Runners.end(); ++it){
		if(it->getDCI().getInt("Rank")>0)
			return true;
	}	
	return false;
}

void oEvent::setMaximalTime(const string &t)
{
  getDI().setInt("MaxTime", convertAbsoluteTime(t));
}

int oEvent::getMaximalTime() const 
{
  return getDCI().getInt("MaxTime");
}

string oEvent::getMaximalTimeS() const 
{
  return formatTime(getMaximalTime());
}


bool oEvent::hasBib(bool runnerBib, bool teamBib) const
{
  if (runnerBib) {
	  oRunnerList::const_iterator it;	
	  for (it=Runners.begin(); it != Runners.end(); ++it){
      if(!it->getBib().empty())
			  return true;
	  }
  }
  if (teamBib) {
	  oTeamList::const_iterator it;	
	  for (it=Teams.begin(); it != Teams.end(); ++it){
      if(!it->getBib().empty())
			  return true;
	  }	
  }
	return false;
}

bool oEvent::hasTeam() const
{
	return Teams.size() > 0;
}

void oEvent::addBib(int ClassId, int leg, int FirstNumber)
{
  if( !classHasTeams(ClassId) ) {
	  sortRunners(ClassStartTimeClub);
	  oRunnerList::iterator it;

	  if(FirstNumber>0) {
		  for (it=Runners.begin(); it != Runners.end(); ++it) {	
        if (it->isRemoved())
          continue;
        if( (ClassId==0 || it->getClassId()==ClassId) && it->legToRun()==leg) {
          it->setBib(itos(FirstNumber++), true);
          it->synchronize();
			  }
		  }
	  }
	  else {
		  for(it=Runners.begin(); it != Runners.end(); ++it){	
        if (it->isRemoved())
          continue;
        if(ClassId==0 || it->getClassId()==ClassId) {
				  it->getDI().setString("Bib", "");//Update only bib
          it->synchronize();
        }
		  }
	  }
  }
  else {
    oTeamList::iterator it;
    for (it=Teams.begin(); it != Teams.end(); ++it) 
      it->apply(false, 0);
    

    if(FirstNumber>0) {
      // Clear out start number temporarily, to not use it for sorting
		  for (it=Teams.begin(); it != Teams.end(); ++it) {	
        if(ClassId==0 || it->getClassId()==ClassId) {
          it->setStartNo(0);
        }
		  }
	  }

    sortTeams(ClassStartTime, 0); // Sort on first leg starttime and sortindex

    if(FirstNumber>0) {
		  for (it=Teams.begin(); it != Teams.end(); ++it) {	
        if(ClassId==0 || it->getClassId()==ClassId) {
          it->setBib(itos(FirstNumber++), true);
          it->apply(true, 0);
        }
		  }
	  }
	  else {
		  for(it=Teams.begin(); it != Teams.end(); ++it){	
        if(ClassId==0 || it->getClassId()==ClassId) {
				  it->getDI().setString("Bib", ""); //Update only bib
          it->apply(true, 0);
        }
		  }
	  }
  }
}

void oEvent::checkOrderIdMultipleCourses(int ClassId)
{
	sortRunners(ClassStartTime);
	int order=1;
	oRunnerList::iterator it;

	//Find first free order
	for(it=Runners.begin(); it != Runners.end(); ++it){	
		if(ClassId==0 || it->getClassId()==ClassId){
			it->synchronize();//Ensure we are up-to-date
			order=max(order, it->StartNo);
		}
	}

	//Assign orders
	for(it=Runners.begin(); it != Runners.end(); ++it){	
		if(ClassId==0 || it->getClassId()==ClassId)
			if(it->StartNo==0){
				it->StartNo=++order;
				it->updateChanged(); //Mark as changed.
				it->synchronize(); //Sync!
			}
	}
}

void oEvent::fillStatus(gdioutput &gdi, const string& id)
{
  vector< pair<string, size_t> > d;
  oe->fillStatus(d);
  gdi.addItem(id, d);
}

const vector< pair<string, size_t> > &oEvent::fillStatus(vector< pair<string, size_t> > &out)
{
  out.clear();
  out.push_back(make_pair(lang.tl("-"), StatusUnknown));
  out.push_back(make_pair(lang.tl("OK"), StatusOK));
  out.push_back(make_pair(lang.tl("Ej start"), StatusDNS));
  out.push_back(make_pair(lang.tl("Felst."), StatusMP));
  out.push_back(make_pair(lang.tl("Utg."), StatusDNF));
  out.push_back(make_pair(lang.tl("Disk."), StatusDQ));
  out.push_back(make_pair(lang.tl("Maxtid"), StatusMAX));
  out.push_back(make_pair(lang.tl("Deltar ej"), StatusNotCompetiting));
  return out;
}

int oEvent::getPropertyInt(const char *name, int def)
{
  if(eventProperties.count(name)==1)
    return atoi(eventProperties[name].c_str()); 
  else {
    setProperty(name, def);
    return def;
  }
}

const string &oEvent::getPropertyString(const char *name, const string &def)
{
  if(eventProperties.count(name)==1)
    return eventProperties[name];
  else {
    eventProperties[name] = def;
    return eventProperties[name];
  }
}

string oEvent::getPropertyStringDecrypt(const char *name, const string &def)
{
  char bf[MAX_COMPUTERNAME_LENGTH + 1];
  DWORD len = MAX_COMPUTERNAME_LENGTH + 1;
  GetComputerName(bf, &len);
  string prop = getPropertyString(name, def);
  string prop2;
  int code = 0;
  const int s = 337;
  
  for (size_t j = 0; j<prop.length(); j+=2) {
    for (size_t k = 0; k<len; k++)
      code = code * 31 + bf[k];
    unsigned int b1 = ((unsigned char *)prop.c_str())[j] - 33;
    unsigned int b2 = ((unsigned char *)prop.c_str())[j+1] - 33;
    unsigned int b = b1 | (b2<<4);
    unsigned kk = abs(code) % s;
    b = (b + s - kk) % s;
    code += b%5;
    prop2.push_back((unsigned char)b);
  }
  return prop2;
}

void oEvent::setPropertyEncrypt(const char *name, const string &prop)
{
  char bf[MAX_COMPUTERNAME_LENGTH + 1];
  DWORD len = MAX_COMPUTERNAME_LENGTH + 1;
  GetComputerName(bf, &len);
  string prop2;
  int code = 0;
  const int s = 337;
  
  for (size_t j = 0; j<prop.length(); j++) {
    for (size_t k = 0; k<len; k++)
      code = code * 31 + bf[k];
    unsigned int b = ((unsigned char *)prop.c_str())[j];
    unsigned kk = abs(code) % s;
    code += b%5;
    b = (b + kk) % s;
    unsigned b1 = (b & 0x0F) + 33;
    unsigned b2 = (b>>4) + 33;
    prop2.push_back((unsigned char)b1);
    prop2.push_back((unsigned char)b2);
  }
  
  setProperty(name, prop2);
}

void oEvent::setProperty(const char *name, int prop)
{
  eventProperties[name]=itos(prop);
}

void oEvent::setProperty(const char *name, const string &prop)
{
  eventProperties[name]=prop;
}

void oEvent::saveProperties(const char *file) {
  map<string, string>::const_iterator it;
  xmlparser xml;
  xml.openOutputT(file, false, "MeOSPreference");
  
  for (it = eventProperties.begin(); it != eventProperties.end(); ++it) {
    xml.write(it->first.c_str(), it->second);
  }
  
  xml.closeOut();
}

void oEvent::loadProperties(const char *file) {
  eventProperties.clear();
  initProperties();
  try {
    xmlparser xml;
    xml.read(file);
    xmlobject xo = xml.getObject("MeOSPreference");
    if (xo) {
      xmlList list;
      xo.getObjects(list);
      for (size_t k = 0; k<list.size(); k++) {
        eventProperties[list[k].getName()] = list[k].get();
      }
    }
  }
  catch (std::exception &) {
    // Failed to read. Continue. 
  }
}

bool compareClubClassTeamName(const oRunner &a, const oRunner &b)
{
  if(a.Club==b.Club) {
    if(a.Class==b.Class) {
      if(a.tInTeam==b.tInTeam)
        return a.Name<b.Name;
      else if(a.tInTeam) {
        if(b.tInTeam)
          return a.tInTeam->getStartNo() < b.tInTeam->getStartNo();
        else return false;
      }
      return b.tInTeam!=0;
    }
    else 
      return a.getClass()<b.getClass();
  }
  else
    return a.getClub()<b.getClub();
  
}

void oEvent::assignCardInteractive(gdioutput &gdi, GUICALLBACK cb)
{
  gdi.fillDown();
  gdi.dropLine(1);
  gdi.addString("", 2, "Tilldelning av hyrbrickor");

  Runners.sort(compareClubClassTeamName);

  oRunnerList::iterator it;
  pClub lastClub=0;

  int k=0;
	for (it=Runners.begin(); it != Runners.end(); ++it) {	  

    if(it->skip() || it->getCardNo() || it->isVacant() || it->needNoCard())
      continue;

    if (it->getStatus() == StatusDNS || it->getStatus() == StatusNotCompetiting)
      continue;

    if(it->Club!=lastClub) {
      lastClub=it->Club;
      gdi.dropLine(0.5);
      gdi.addString("", 1, it->getClub());
    }

    string r;
    if(it->Class)
      r+=it->getClass()+", ";

    if(it->tInTeam) {
      char bf[1024];
      sprintf_s(bf, "%d %s, ", it->tInTeam->getStartNo(), 
                    it->tInTeam->getName().c_str());
      r+=bf;
    }

    r+=it->Name+":";
    gdi.fillRight();
    gdi.pushX();
    gdi.addStringUT(0, r);
    char id[24];
    sprintf_s(id, "*%d", k++);

    gdi.addInput(max(gdi.getCX(), 450), gdi.getCY()-4, 
                 id, "", 10, cb).setExtra(LPVOID(it->getId()));

    gdi.popX();
    gdi.dropLine(1.6);
    gdi.fillDown();
	}

  if (k==0) 
    gdi.addString("", 0, "Ingen l�pare saknar bricka");
}

void oEvent::calcUseStartSeconds()
{
  tUseStartSeconds=false;
  oRunnerList::iterator it;	
	for (it=Runners.begin(); it != Runners.end(); ++it)
    if( it->getStartTime()>0 && 
        (it->getStartTime()+ZeroTime)%60!=0 ) {
      tUseStartSeconds=true;
      return;
    } 
}

const string &oEvent::formatStatus(RunnerStatus status) 
{
  const static string stats[8]={"?", "OK", "Ej start", "Felst.", "Utg.", "Disk.", "Maxtid", "Deltar ej"};
	switch(status)	{
	case StatusOK:
		return lang.tl(stats[1]);
	case StatusDNS:
		return lang.tl(stats[2]);
	case StatusMP:
		return lang.tl(stats[3]);
	case StatusDNF:
		return lang.tl(stats[4]);
	case StatusDQ:
		return lang.tl(stats[5]);
	case StatusMAX:
		return lang.tl(stats[6]);
  case StatusNotCompetiting:
    return lang.tl(stats[7]);
  default:
    return stats[0];
	}
}

#ifndef MEOSDB  

void oEvent::analyzeClassResultStatus() const
{
  map<int, ClassResultInfo> res;
  for (oRunnerList::const_iterator it = Runners.begin(); it != Runners.end(); ++it) {
    if (it->isRemoved() || !it->Class)
      continue;

    int id = it->Class->Id * 31 + it->tLeg;
    ClassResultInfo &cri = res[id];

    if (it->getStatus() == StatusUnknown) {
      cri.nUnknown++;
      if (it->StartTime > 0) {
        if (!it->isVacant()) {
          if (cri.lastStartTime>=0)
            cri.lastStartTime = max(cri.lastStartTime, it->StartTime);
        }
      }
      else
        cri.lastStartTime = -1; // Cannot determine
    }
    else
      cri.nFinished++;

  }

  for (oClassList::const_iterator it = Classes.begin(); it != Classes.end(); ++it) {
    if (it->isRemoved())
      continue;

    if (!it->legInfo.empty()) {
      it->tResultInfo.resize(it->legInfo.size());
      for (size_t k = 0; k<it->legInfo.size(); k++) {
        int id = it->Id * 31 + k;
        it->tResultInfo[k] = res[id];
      }
    }
    else {
      it->tResultInfo.resize(1);
      it->tResultInfo[0] = res[it->Id * 31];
    }
  }
}

void oEvent::generateTestCard(SICard &sic) const
{
  sic.clear(0);

  if(Runners.empty())
    return;

  analyzeClassResultStatus(); 

  oRunnerList::const_iterator it;
  
  int rNo = rand()%Runners.size();
  
  it=Runners.begin();

  while(rNo-->0)
    ++it;
  
  oRunner *r = 0;
  int cardNo = 0;
  while(r==0 && it!=Runners.end()) {
    cardNo = it->CardNo;
    if (cardNo == 0 && it->tParentRunner)
      cardNo = it->tParentRunner->CardNo;

    if (it->Class && it->tLeg>0) {
      StartTypes st = it->Class->getStartType(it->tLeg);
      if (st == STHunting) {
        if (it->Class->tResultInfo[it->tLeg-1].nUnknown > 0)
          cardNo = 0; // Wait with this leg
      }
    }

    // Make sure teams start in right order
    if (it->tInTeam && it->tLeg>0) {
      if (it->Class) {
        StartTypes st = it->Class->getStartType(it->tLeg); 
        if (st != STDrawn && st != STTime) {
          pRunner prev = it->tInTeam->Runners[it->tLeg - 1];
          if (prev && prev->getStatus() == StatusUnknown)
            cardNo = 0; // Wait with this runner
        }
      }
    }

    if(cardNo && !it->Card) {
      // For team runners, we require start time to get right order
      if(!it->tInTeam || it->StartTime>0)
        r=pRunner(&*it);
    }
    ++it;
  }
  --it;
  while(r==0 && it!=Runners.begin()) {
    cardNo = it->CardNo;
    if (cardNo == 0 && it->tParentRunner)
      cardNo = it->tParentRunner->CardNo;

    if (it->Class && it->tLeg>0) {
      StartTypes st = it->Class->getStartType(it->tLeg);
      if (st == STHunting) {
        if (it->Class->tResultInfo[it->tLeg-1].nUnknown > 0)
          cardNo = 0; // Wait with this leg
      }
    }

    // Make sure teams start in right order
    if (it->tInTeam && it->tLeg>0) {
      if (it->Class) {
        StartTypes st = it->Class->getStartType(it->tLeg); 
        if (st != STDrawn && st != STTime) {
          pRunner prev = it->tInTeam->Runners[it->tLeg - 1];
          if (prev && prev->getStatus() == StatusUnknown)
            cardNo = 0; // Wait with this runner
        }
      }
    }

    if(cardNo && !it->Card) {
      // For team runners, we require start time to get right order
      if(!it->tInTeam || it->StartTime>0) {
        r=pRunner(&*it);
      }
    }
    --it;
  }

  if (r) {
    r->synchronize();
    pCourse pc=r->getCourse();

    if (!pc) {
      pClass cls = r->Class;
      if (cls) {
        pc = const_cast<oEvent *>(this)->generateTestCourse(rand()%15+7);
        pc->synchronize();
        cls->setCourse(pc);
        cls->synchronize();
      }
    }

    if(pc) {
      sic.CardNumber = cardNo;
      int s = sic.StartPunch.Time = r->StartTime>0 ? r->StartTime+ZeroTime : ZeroTime+3600+rand()%(3600*3);
      int tomiss = rand()%(60*10);
      if (tomiss>60*9)
        tomiss = rand()%30;
      else if (rand()%20 == 3)
        tomiss *= rand()%3;

      int f = sic.FinishPunch.Time = s+(30+pc->getLength()/200)*60+ rand()%(60*10) + tomiss;

      if(rand()%40==0 || r->StartTime>0)
        sic.StartPunch.Code=-1;
      
      if(rand()%50==31)
        sic.FinishPunch.Code=-1;

      if(rand()%70==31)
        sic.CardNumber++;

      sic.nPunch=0;
      double dt=1./double(pc->nControls+1);

      int missed = 0;

      for(int k=0;k<pc->nControls;k++) {      
        if (rand()%130!=50) {
          sic.Punch[sic.nPunch].Code=pc->getControl(k)->Numbers[0];
          double cc=(k+1)*dt;

          
          if (missed < tomiss) {
            int left = pc->nControls - k;
            if (rand() % left == 1)
              missed += ( (tomiss - missed) * (rand()%4 + 1))/6;
            else if (left == 1)
              missed = tomiss;
          }

		      sic.Punch[sic.nPunch].Time=int((f-tomiss)*cc+s*(1.-cc)) + missed;	
          sic.nPunch++;
        }
      }
    }
  }
}

pCourse oEvent::generateTestCourse(int nCtrl)
{
  char bf[64];
  static int sk=0;
  sprintf_s(bf, lang.tl("Bana %d").c_str(), ++sk);
  pCourse pc=addCourse(bf, 4000+(rand()%1000)*10);

  int i=0;
  for (;i<nCtrl/3;i++) 
    pc->addControl(rand()%(99-32)+32);
  
  i++;
  pc->addControl(50)->setName("Radio 1");

  for (;i<(2*nCtrl)/3;i++) 
    pc->addControl(rand()%(99-32)+32);

  i++;
  pc->addControl(150)->setName("Radio 2");

  for (;i<nCtrl-1;i++) 
    pc->addControl(rand()%(99-32)+32);
  pc->addControl(100)->setName("F�rvarning");

  return pc;
}


pClass oEvent::generateTestClass(int nlegs, int nrunners, 
                               char *name, const string &start)
{   
  pClass cls=addClass(name);

  if (nlegs==1 && nrunners==1) {
    int nCtrl=rand()%15+5;
    if(rand()%10==1)
      nCtrl+=rand()%40;
    cls->setCourse(generateTestCourse(nCtrl));
  }
  else if (nlegs==1 && nrunners==2) {
    setupRelay(*cls, PPatrol, 2, start);
    int nCtrl=rand()%15+10;
    pCourse pc=generateTestCourse(nCtrl);
    cls->addStageCourse(0, pc->getId());
    cls->addStageCourse(1, pc->getId());
  }
  else if (nlegs>1 && nrunners==2) {
    setupRelay(*cls, PTwinRelay, nlegs, start);
    int nCtrl=rand()%8+10;
    int cid[64];
    for (int k=0;k<nlegs;k++)
      cid[k]=generateTestCourse(nCtrl)->getId();

    for (int k=0;k<nlegs;k++)
      for (int j=0;j<nlegs;j++)
        cls->addStageCourse(k, cid[(k+j)%nlegs]);
  }
  else if (nlegs>1 && nrunners==nlegs) {
    setupRelay(*cls, PRelay, nlegs, start);
    int nCtrl=rand()%8+10;
    int cid[64];
    for (int k=0;k<nlegs;k++)
      cid[k]=generateTestCourse(nCtrl)->getId();

    for (int k=0;k<nlegs;k++)
      for (int j=0;j<nlegs;j++)
        cls->addStageCourse(k, cid[(k+j)%nlegs]);
  }
  else if (nlegs>1 && nrunners==1) {
    setupRelay(*cls, PHunting, 2, start);
    cls->addStageCourse(0, generateTestCourse(rand()%8+10)->getId());
    cls->addStageCourse(1, generateTestCourse(rand()%8+10)->getId());
  }
  return cls;
}


void oEvent::generateTestCompetition(int nClasses, int nRunners, 
                                     bool generateTeams)
{ 
  if (nClasses > 0) {
    oe->newCompetition("!TESTT�VLING");
    oe->setZeroTime("05:00:00");
  }
  bool useSOFTMethod = true;
  vector<string> gname;
  //gname.reserve(RunnerDatabase.size());  
  vector<string> fname;
  //fname.reserve(RunnerDatabase.size());

  runnerDB->getAllNames(gname, fname);

  if (fname.empty())
    fname.push_back("Foo");

  if (gname.empty())
    gname.push_back("Bar");

/*  oRunnerList::iterator it;
  for(it=RunnerDatabase.begin(); it!=RunnerDatabase.end(); ++it){
    if (!it->getGivenName().empty())
      gname.push_back(it->getGivenName());

    if (!it->getFamilyName().empty())
      fname.push_back(it->getFamilyName());
  }
*/
  int nClubs=30;
  char bf[128];
  int startno=1;
  const vector<oClub> &oc = runnerDB->getClubDB();
  for(int k=0;k<nClubs;k++) {
    if (oc.empty()) {
      sprintf_s(bf, "Klubb %d", k);
      addClub(bf, k+1);
    }
    else {
      addClub(oc[(k*13)%oc.size()].getName(), k+1);
    }
  }

  int now=getRelativeTime(getCurrentTimeS());
  string start=getAbsTime(now+60*3-(now%60));

  for (int k=0;k<nClasses;k++) {
    pClass cls=0;

    if (!generateTeams) {
      int age=0;
      if(k<7)
        age=k+10;
      else if(k==7)
        age=18;
      else if(k==8)
        age=20;
      else if(k==9)
        age=21;
      else 
        age=30+(k-9)*5;
        
      sprintf_s(bf, "HD %d", age);
      cls=generateTestClass(1,1, bf, "");
    }
    else {
      sprintf_s(bf, "Klass %d", k);
      int nleg=k%5+1;
      int nrunner=k%3+1;
      nrunner = nrunner == 3 ? nleg:nrunner;

      nleg=3;
      nrunner=3;
      cls=generateTestClass(nleg, nrunner, bf, start);
    }
  }

  nClasses = Classes.size();
  int k = 0;

  for (oClassList::iterator it = Classes.begin(); it != Classes.end(); ++it, ++k) {
    pClass cls = &*it;

    int classesLeft=(nClasses-k);
    int nRInClass=nRunners/classesLeft;

    if(classesLeft>2 && nRInClass>3)
      nRInClass+=int(nRInClass*0.7)-rand()%int(nRInClass*1.5);
    
    if (cls->getNumDistinctRunners()==1) {
      for (int i=0;i<nRInClass;i++) {
        pRunner r=addRunner(gname[rand()%gname.size()]+" "+fname[rand()%fname.size()],
          rand()%nClubs+1, cls->getId(), 0, 0, true);

        r->setStartNo(startno++);
        r->setCardNo(500001+Runners.size()*97+rand()%97, false);
        r->apply(false);
      }
      nRunners-=nRInClass;
      if (k%5!=5)
        drawList(cls->getId(), 0, getRelativeTime(start), 10, 3, useSOFTMethod, false, drawAll); 
      else
        cls->Name += " �ppen";
    }
    else {
      int dr=cls->getNumDistinctRunners();
      for (int i=0;i<nRInClass;i++) {
        pTeam t=addTeam("Lag " + fname[rand()%fname.size()], rand()%nClubs+1, cls->getId());
        t->setStartNo(startno++);

        for (int j=0;j<dr;j++) {
          pRunner r=addRunner(gname[rand()%gname.size()]+" "+fname[rand()%fname.size()], 0, 0, 0, 0, true);
          r->setCardNo(500001+Runners.size()*97+rand()%97, false);
          t->setRunner(j, r, false);
        }
      }
      nRunners-=nRInClass;

      if( cls->getStartType(0)==STDrawn )
        drawList(cls->getId(), 0, getRelativeTime(start), 20, 3, useSOFTMethod, false, drawAll); 
    }
  }
}

#endif

void oEvent::getFreeImporter(oFreeImport &fi)
{
  if(!fi.isLoaded())
    fi.load();

  fi.init(Runners, Clubs, Classes);
}


void oEvent::fillFees(gdioutput &gdi, const string &name) const {	
	gdi.clearList(name);

  set<int> fees;

  int f;
  for (oClassList::const_iterator it = Classes.begin(); it != Classes.end(); ++it) {
    if (it->isRemoved())
      continue;

    f = it->getDCI().getInt("ClassFee");
    if (f > 0)
      fees.insert(f);

    f = it->getDCI().getInt("ClassFeeRed");
    if (f > 0)
      fees.insert(f);
  }

  f = getDCI().getInt("EliteFee");
  if (f > 0)
    fees.insert(f);

  f = getDCI().getInt("EntryFee");
  if (f > 0)
    fees.insert(f);

  f = getDCI().getInt("YouthFee");
  if (f > 0)
    fees.insert(f);

  vector< pair<string, size_t> > ff;
  for (set<int>::iterator it = fees.begin(); it != fees.end(); ++it)
    ff.push_back(make_pair(formatCurrency(*it), *it));

  gdi.addItem(name, ff);
}

void oEvent::fillLegNumbers(gdioutput &gdi, const string &name)
{	
	oClassList::iterator it;	
  synchronizeList(oLClassId);

	gdi.clearList(name);
	string b;
	char bf[64];
  int maxmulti=0;
	for (it=Classes.begin(); it != Classes.end(); ++it) 
    if (!it->Removed) 
      maxmulti=max<unsigned>(maxmulti, it->getNumStages());
 
  if (maxmulti==0)
    gdi.disableInput(name.c_str());
  else {
    
    for (int k=0;k<maxmulti;k++) {
      sprintf_s(bf, lang.tl("Str�cka %d").c_str(), k+1);
      gdi.addItem(name, bf, k);
    }
    gdi.addItem(name, lang.tl("Sista str�ckan"), 1000);

    gdi.selectFirstItem(name);
  }
}

void oEvent::generateTableData(const string &tname, Table &table, TableUpdateInfo &tui) 
{
  if (tname == "runners") {
    pRunner r = tui.doAdd ? addRunner(getAutoRunnerName(),0,0,0,0,false) : pRunner(tui.object);
    generateRunnerTableData(table, r);
    return;
  }
  else if (tname == "classes") {
    pClass c = tui.doAdd ? addClass(getAutoClassName()) : pClass(tui.object);
    generateClassTableData(table, c);
    return;
  }
  else if (tname == "clubs") {
    pClub c = tui.doAdd ? addClub("Club", 0) : pClub(tui.object);
    generateClubTableData(table, c);
    return;
  }
  else if (tname == "teams") {
    pTeam t = tui.doAdd ? addTeam(getAutoTeamName()) : pTeam(tui.object);
    generateTeamTableData(table, t);
    return;
  }
  else if (tname == "cards") {    
    generateCardTableData(table, pCard(tui.object));
    return;
  }
  else if (tname == "controls") {
    generateControlTableData(table, pControl(tui.object));
    return;
  }
  else if (tname == "punches") {
    pFreePunch c = tui.doAdd ? addFreePunch(0,0,0) : pFreePunch(tui.object);
    generatePunchTableData(table, c);
    return;
  }
  else if (tname == "runnerdb") {
    oDBRunnerEntry *entry = (oDBRunnerEntry *)(tui.object);
    getRunnerDatabase().generateRunnerTableData(table, entry);
    return;
  }
  else if (tname == "clubdb") {
    pClub c = pClub(tui.object);
    getRunnerDatabase().generateClubTableData(table, c);
    return;
  }
  throw std::exception("Wrong table name");
}

void oEvent::applyEventFees(bool updateClassFromEvent, 
                            bool updateFees, bool updateCardFees,
                            const set<int> &classFilter) {
  synchronizeList(oLClassId);
  synchronizeList(oLRunnerId);
  bool allClass = classFilter.empty();

  if (updateClassFromEvent) {
    for (oClassList::iterator it = Classes.begin(); it != Classes.end(); ++it) {
      if (it->isRemoved())
        continue;
      if (allClass || classFilter.count(it->getId())) {
        it->addClassDefaultFee(true);
        it->synchronize(true);
      }
    }
  }

  if (updateFees) {
    for (oRunnerList::iterator it = Runners.begin(); it != Runners.end(); ++it) {
      if (it->skip())
        continue;

      if (allClass || classFilter.count(it->getClassId())) {
        it->addClassDefaultFee(true); 
        it->synchronize(true);
      }
    }
  }

  if (updateCardFees) {
    int cf = getDCI().getInt("CardFee");
    for (oRunnerList::iterator it = Runners.begin(); it != Runners.end(); ++it) {
      if (it->skip())
        continue;

      if (it->getDI().getInt("CardFee") != 0) {
        it->getDI().setInt("CardFee", cf);
        it->synchronize(true);
      }
    }
  }
}

#ifndef MEOSDB 
void hideTabs();
void createTabs(bool force, bool onlyMain, bool skipTeam, bool skipSpeaker, 
                bool skipEconomy, bool skipLists, bool skipRunners, bool skipControls);

void oEvent::updateTabs(bool force) const
{
  bool hasTeam = !Teams.empty();

  for (oClassList::const_iterator it = Classes.begin(); 
                  !hasTeam && it!=Classes.end(); ++it) {
    if (it->getNumStages()>1)
      hasTeam = true;
  }

  bool hasRunner = !Runners.empty() || !Classes.empty();
  bool hasLists = !empty();

  if (isReadOnly())
    hideTabs();
  else
    createTabs(force, empty(), !hasTeam, getDCI().getInt("UseSpeaker")==0, 
                !useEconomy(), !hasLists, !hasRunner, Controls.empty());
}

#else
void oEvent::updateTabs(bool force) const 
{
}
#endif

bool oEvent::useEconomy() const
{
  return getDCI().getInt("UseEconomy")!=0;
}

bool oEvent::hasMultiRunner() const
{
  for (oClassList::const_iterator it = Classes.begin(); it!=Classes.end(); ++it) {
    if(it->hasMultiCourse() && it->getNumDistinctRunners() != it->getNumStages())
      return true;
  }

  return false;
}

/**Return false if card is not used*/
bool oEvent::checkCardUsed(gdioutput &gdi, int CardNo)
{
  oe->synchronizeList(oLRunnerId);
  pRunner pold=oe->getRunnerByCard(CardNo, false);
  char bf[1024];

  if(pold) {
    sprintf_s(bf, ("#" + lang.tl("Bricka %d anv�nds redan av %s och kan inte tilldelas.")).c_str(),
                  CardNo, pold->getName().c_str());
    gdi.alert(bf);
    return true;
  }
  return false;
}

void oEvent::removeVacanies(int classId) {
  oRunnerList::iterator it;
  vector<int> toRemove;

	for (it=Runners.begin(); it != Runners.end(); ++it) {
    if (it->skip() || !it->isVacant())
      continue;

    if (classId!=0 && it->getClassId()!=classId)
      continue;

    if (!isRunnerUsed(it->Id))
      toRemove.push_back(it->Id);
  }

  removeRunner(toRemove);
}

void oEvent::sanityCheck(gdioutput &gdi, bool expectResult) {
  bool hasResult = false;
  bool warnNoName = false;
  bool warnNoClass = false;
  bool warnNoTeam = false;
  bool warnNoPatrol = false;
  bool warnIndividualTeam = false;

  for (oRunnerList::iterator it = Runners.begin(); it!=Runners.end(); ++it) {
    if (it->isRemoved())
      continue;
    if (it->Name.empty()) {
      if (!warnNoName) {
        warnNoName = true;
        gdi.alert("Varning: deltagare med blankt namn p�tr�ffad. MeOS "
                  "kr�ver att alla deltagare har ett namn, och tilldelar namnet 'N.N.'");
      }
      it->setName("N.N.");
      it->synchronize();
    }

    if (!it->Class) {
      if (!warnNoClass) {
        gdi.alert("Deltagaren 'X' saknar klass.#" + it->Name);
        warnNoClass = true;
      }
      continue;
    }

    if (!it->tInTeam) {
      ClassType type = it->Class->getClassType();
      if (type == oClassIndividRelay) {
        it->setClassId(it->Class->getId());
        it->synchronize();
      }
      else if (type == oClassRelay) {
        if (!warnNoTeam) {
          gdi.alert("Deltagaren 'X' deltar i stafettklassen 'Y' men saknar lag. Klassens start- "
                    "och resultatlistor kan d�rmed bli felaktiga.#" + it->Name + 
                     "#" + it->getClass());
          warnNoTeam = true;
        }
      }
      else if (type == oClassPatrol) {
        if (!warnNoPatrol) {
          gdi.alert("Deltagaren 'X' deltar i patrullklassen 'Y' men saknar patrull. Klassens start- "
                    "och resultatlistor kan d�rmed bli felaktiga.#" + it->Name +
                     + "#" + it->getClass());
          warnNoPatrol = true;
        }
      }
    }

    if (it->getFinishTime()>0)
      hasResult = true;
  }

  for (oTeamList::iterator it = Teams.begin(); it != Teams.end(); ++it) {
    if (it->isRemoved())
      continue;

    if (it->Name.empty()) {
      if (!warnNoName) {
        warnNoName = true;
        gdi.alert("Varning: lag utan namn p�tr�ffat. "
                  "MeOS kr�ver att alla lag har ett namn, och tilldelar namnet 'N.N.'");
      }
      it->setName("N.N.");
      it->synchronize();
    }

    if (!it->Class) {
      if (!warnNoClass) {
        gdi.alert("Laget 'X' saknar klass.#" + it->Name);
        warnNoClass = true;
      }
      continue;
    }

    ClassType type = it->Class->getClassType();
    if (type == oClassIndividual) {       
      if (!warnIndividualTeam) {
        gdi.alert("Laget 'X' deltar i individuella klassen 'Y'. Klassens start- och resultatlistor "
          "kan d�rmed bli felaktiga.#" + it->Name + "#" + it->getClass());
        warnIndividualTeam = true;
      }
    }
  }
 

  if (expectResult && !hasResult)
    gdi.alert("T�vlingen inneh�ller inga resultat.");


  bool warnBadStart = false;

  for (oClassList::iterator it = Classes.begin(); it != Classes.end(); ++it) {
    if (it->isRemoved())
      continue;
    if (it->getClassStatus() != oClass::Normal)
      continue;
    if (it->hasMultiCourse()) {
      for (unsigned k=0;k<it->getNumStages(); k++) {
        StartTypes st = it->getStartType(k);
        LegTypes lt = it->getLegType(k);
        if (k==0 && (st == STChange || st == STHunting) && !warnBadStart) {
          warnBadStart = true;
          gdi.alert("Klassen 'X' har jaktstart/v�xling p� f�rsta str�ckan.#" + it->getName());
        }
        if (st == STTime && it->getStartData(k)<=0 && !warnBadStart && 
              (lt == LTNormal || lt == LTSum)) {
          warnBadStart = true;
          gdi.alert("Ogiltig starttid i 'X' p� str�cka Y.#" + it->getName() + "#" + itos(k+1));
        }
      }
    }
  }
}

HWND oEvent::hWnd() const
{
  return gdibase.getHWND();
}

oTimeLine::~oTimeLine() {

}

void oEvent::remove() {
  if (isClient())
   dropDatabase();
  else 
   deleteCompetition();

  clearListedCmp();
  newCompetition("");
}

bool oEvent::canRemove() const {
  return true;
}

string oEvent::formatCurrency(int c, bool includeSymbol) const {
  if (tCurrencyFactor == 1)
    if (!includeSymbol)
      return itos(c);
    else if (tCurrencyPreSymbol)
      return tCurrencySymbol + itos(c);
    else
      return itos(c) + tCurrencySymbol;
  else {
    char bf[32];
    if (includeSymbol) {
      sprintf_s(bf, 32, "%d%s%02d", c/tCurrencyFactor, 
                tCurrencySeparator.c_str(), c%tCurrencyFactor);
      
      if (tCurrencyPreSymbol)
        return tCurrencySymbol + bf;
      else
        return bf + tCurrencySymbol;
    }
    else {
      sprintf_s(bf, 32, "%d.%02d", c/tCurrencyFactor, c%tCurrencyFactor);
      return bf;
    }
  }
}

int oEvent::interpretCurrency(const string &c) const {
  if (tCurrencyFactor == 1 && tCurrencyPreSymbol == false)
    return atoi(c.c_str());

  size_t s = 0;
  while (s < c.length() && (c[s]<'0' || c[s]>'9'))
    s++;

  string cc = c.substr(s);

  for (size_t k = 0; k<cc.length(); k++) {
    if (cc[k] == ',' || cc[k] == tCurrencySeparator[0])
      cc[k] = '.';
  }

  return int(atof(cc.c_str())*tCurrencyFactor);
}

int oEvent::interpretCurrency(double val, const string &cur)  {
  if (_stricmp("sek", cur.c_str()) == 0)
    setCurrency(1, "kr", ",", false);
  else if(_stricmp("eur", cur.c_str()) == 0)
    setCurrency(100, "�", ".", false);

  return int(floor(val * tCurrencyFactor+0.5));
}

void oEvent::setCurrency(int factor, const string &symbol, const string &separator, bool preSymbol) {
  if (factor == -1) {
    // Load from data
    int cf = getDCI().getInt("CurrencyFactor");
    if (cf != 0)
      tCurrencyFactor = cf;

    string cs = getDCI().getString("CurrencySymbol");
    if (!cs.empty())
      tCurrencySymbol = cs;

    cs = getDCI().getString("CurrencySeparator");
    if (!cs.empty())
      tCurrencySeparator = cs;

    int ps = getDCI().getInt("CurrencyPreSymbol");
    tCurrencyPreSymbol = (ps != 0);

    if (tCurrencySymbol.size() > 0) {
      if (tCurrencyPreSymbol) {
        char end = *tCurrencySymbol.rbegin();
        if ((end>='a' && end <='z') || end>='A' && end <='Z')
          tCurrencySymbol += " ";
      }
      else {
        char end = *tCurrencySymbol.begin();
        if ((end>='a' && end <='z') || end>='A' && end <='Z')
          tCurrencySymbol = " " + tCurrencySymbol;
      }
    }
  }
  else {
    tCurrencyFactor = factor;
    tCurrencySymbol = symbol;
    tCurrencySeparator = separator;
    tCurrencyPreSymbol = preSymbol;
    getDI().setString("CurrencySymbol", symbol);
    getDI().setInt("CurrencyFactor", factor);
    getDI().setString("CurrencySeparator", separator);
    getDI().setInt("CurrencyPreSymbol", preSymbol ? 1 : 0);
  }
}
string oEvent::cloneCompetition(bool cloneRunners, bool cloneTimes, 
                                bool cloneCourses, bool cloneResult, bool addToDate) {

  if (cloneResult) {
    cloneTimes = true;
    cloneCourses = true;
  }
  if (cloneTimes)
    cloneRunners = true;

  oEvent ce(gdibase);
  ce.newCompetition(Name);
  ce.ZeroTime = ZeroTime;
  ce.Date = Date;

  if (addToDate) {
    SYSTEMTIME st;
    convertDateYMS(Date, st);
    __int64 absD = SystemTimeToInt64Second(st);
    absD += 3600*24;
    ce.Date = convertSystemDate(Int64SecondToSystemTime(absD));
  }
  int len = Name.length();
  if (len > 2 && isdigit(Name[len-1]) && !isdigit(Name[len-2])) {
    ++ce.Name[len-1]; // E1 -> E2
  }
  else
    ce.Name += " E2";

  memcpy(ce.oData, oData, sizeof(oData));

  for (oClubList::iterator it = Clubs.begin(); it != Clubs.end(); ++it) {
    if (it->isRemoved())
      continue;
    pClub pc = ce.addClub(it->name, it->Id);
    memcpy(pc->oData, it->oData, sizeof(pc->oData));
  }

  if (cloneCourses) {
    for (oControlList::iterator it = Controls.begin(); it != Controls.end(); ++it) {
      if (it->isRemoved())
        continue;
      pControl pc = ce.addControl(it->Id, 100, it->Name);
      pc->setNumbers(it->codeNumbers());
      pc->Status = it->Status;
      memcpy(pc->oData, it->oData, sizeof(pc->oData));
    }

    for (oCourseList::iterator it = Courses.begin(); it != Courses.end(); ++it) {
      if (it->isRemoved())
        continue;
      pCourse pc = ce.addCourse(it->Name, it->Length, it->Id);
      pc->importControls(it->getControls());
      pc->legLengths = it->legLengths;
      memcpy(pc->oData, it->oData, sizeof(pc->oData));
    }
  }

  for (oClassList::iterator it = Classes.begin(); it != Classes.end(); ++it) {
    if (it->isRemoved())
      continue;
    pClass pc = ce.addClass(it->Name, 0, it->Id);
    memcpy(pc->oData, it->oData, sizeof(pc->oData));
    pc->setNumStages(it->getNumStages());
    pc->legInfo = it->legInfo;

    if (cloneCourses) {
      pc->Course = ce.getCourse(it->getCourseId());
      pc->MultiCourse = it->MultiCourse; // Points to wrong competition, but valid for now...
    }
  }

  if (cloneRunners) {
    for (oRunnerList::iterator it = Runners.begin(); it != Runners.end(); ++it) {
      if (it->isRemoved())
        continue;

      oRunner r(&ce, it->Id);
      r.Name = it->Name;
      r.StartNo = it->StartNo;
      r.CardNo = it->CardNo;
      r.Club = ce.getClub(it->getClubId());
      r.Class = ce.getClass(it->getClassId());

      if (cloneCourses)
        r.Course = ce.getCourse(it->getCourseId());

      pRunner pr = ce.addRunner(r);

      pr->decodeMultiR(it->codeMultiR());
      memcpy(pr->oData, it->oData, sizeof(pr->oData));
      
      if (cloneTimes) {
        pr->StartTime = it->StartTime;
      }

      if (cloneResult) {
        if (it->Card) {
          pr->Card = ce.addCard(*it->Card);
          pr->Card->tOwner = pr;
        }
        pr->FinishTime = it->FinishTime;
        pr->Status = it->Status;
      }
    }

    for (oTeamList::iterator it = Teams.begin(); it != Teams.end(); ++it) {
      if (it->skip())
        continue;
      
      oTeam t(&ce, it->Id);

      t.Name = it->Name;
      t.StartNo = it->StartNo;
      t.Club = ce.getClub(it->getClubId());
      t.Class = ce.getClass(it->getClassId());
      
      if (cloneTimes) 
        t.StartTime = it->StartTime;

      pTeam pt = ce.addTeam(t);
      memcpy(pt->oData, it->oData, sizeof(pt->oData));
      
      pt->Runners.resize(it->Runners.size());
      for (size_t k = 0; k<it->Runners.size(); k++) {
        int id = it->Runners[k] ? it->Runners[k]->Id : 0;
        if (id)
          pt->Runners[k] = ce.getRunner(id, 0);
      }

      t.apply(false, 0);
    }

    for (oRunnerList::iterator it = ce.Runners.begin(); it != ce.Runners.end(); ++it) {
      it->createMultiRunner(false, false);
    }
  }

  vector<pRunner> changedClass, assignedVacant, newEntries, notTransfered, noAssign;
  set<int> dummy;
  transferResult(ce, dummy, false, changedClass, assignedVacant, newEntries, notTransfered, noAssign);

  vector<pTeam> newEntriesT, notTransferedT, noAssignT;
  transferResult(ce, newEntriesT, notTransferedT, noAssignT);


  int eventNumberCurrent = getDCI().getInt("EventNumber");
  if (eventNumberCurrent <= 0) {
    eventNumberCurrent = 1;
    getDI().setInt("EventNumber", eventNumberCurrent);
  }

  ce.getDI().setString("PreEvent", CurrentNameId);
  ce.getDI().setInt("EventNumber", eventNumberCurrent + 1);
  getDI().setString("PostEvent", ce.CurrentNameId);
  ce.save();

  return ce.CurrentFile;
}

void checkTargetClass(pRunner target, pRunner source, 
                      const oClassList &Classes, 
                      const vector<pRunner> &targetVacant,
                      vector<pRunner> &changedClass) {
  if (!compareClassName(target->getClass(), source->getClass())) {
    // Store all vacant positions in the right class
    int targetClass = -1;

    for (oClassList::const_iterator cit = Classes.begin(); cit != Classes.end(); ++cit) {
      if (cit->isRemoved())
        continue;
      if (compareClassName(cit->getName(), source->getClass())) {
        targetClass = cit->getId();

        if (targetClass == source->getClassId() || cit->getName() == source->getClass())
          break; // Assume exact match
      }
    }
    
    if (targetClass != -1) {
      set<int> vacantIx;
      for (size_t j = 0; j < targetVacant.size(); j++) {
        if (!targetVacant[j])
          continue;
        if (targetVacant[j]->getClassId() == targetClass)
          vacantIx.insert(j);
      }
      int posToUse = -1;
      if (vacantIx.size() == 1)
        posToUse = *vacantIx.begin();
      else if (vacantIx.size() > 1) {
        string srcBib = source->getBib();
        if (srcBib.length() > 0) {
          for (set<int>::iterator tit = vacantIx.begin(); tit != vacantIx.end(); ++tit) {
            if (targetVacant[*tit]->getBib() == srcBib) {
              posToUse = *tit;
              break;
            }
          }
        }

        if (posToUse == -1)
          posToUse = *vacantIx.begin();
      }

      if (posToUse != -1) {
        changedClass.push_back(target);

        int oldStart = target->getStartTime();
        string oldBib = target->getBib();
        int oldSN = target->getStartNo();
        int oldClass = target->getClassId();
        pRunner tgt = targetVacant[posToUse];
        target->cloneStartTime(tgt);
        target->setBib(tgt->getBib(), false);
        target->setStartNo(tgt->getStartNo());
        target->setClassId(tgt->getClassId());

        tgt->setStartTime(oldStart);
        tgt->setBib(oldBib, false);
        tgt->setStartNo(oldSN);
        tgt->setClassId(oldClass);
      }
    }
  }
}

void oEvent::transferResult(oEvent &ce,
                            const set<int> &allowNewEntries,
                            bool transferAllNoCompete,
                            vector<pRunner> &changedClass,
                            vector<pRunner> &assignedVacant,
                            vector<pRunner> &newEntries,
                            vector<pRunner> &notTransfered,
                            vector<pRunner> &noAssignmentTarget) {

  inthashmap processed(ce.Runners.size());
  inthashmap used(Runners.size());

  changedClass.clear();
  assignedVacant.clear();
  newEntries.clear();
  notTransfered.clear();
  noAssignmentTarget.clear();

  vector<pRunner> targetRunners;
  vector<pRunner> targetVacant;

  targetRunners.reserve(ce.Runners.size());
  for (oRunnerList::iterator it = ce.Runners.begin(); it != ce.Runners.end(); ++it) {
    if (!it->skip()) {
      if (!it->isVacant())
        targetRunners.push_back(&*it);
      else 
        targetVacant.push_back(&*it);
    }
  }

  calculateResults(RTTotalResult);
  // Lookup by id
  for (size_t k = 0; k < targetRunners.size(); k++) {
    pRunner it = targetRunners[k];
    pRunner r = getRunner(it->Id, 0);
    if (!r)
      continue;

    __int64 id1 = r->getExtIdentifier();
    __int64 id2 = it->getExtIdentifier();

    if (id1>0 && id2>0 && id1 != id2)
      continue;
    
    if ((id1>0 && id1==id2) || (r->CardNo>0 && r->CardNo == it->CardNo) || (it->Name == r->Name && it->getClub() == r->getClub())) {
      processed.insert(it->Id, 1);      
      used.insert(r->Id, 1);
      it->setInputData(*r);
      checkTargetClass(it, r, ce.Classes, targetVacant, changedClass);
    }
  }

  if (processed.size() < int(targetRunners.size())) {
    // Lookup by card
    int v;
    setupCardHash(false);
    for (size_t k = 0; k < targetRunners.size(); k++) {
      pRunner it = targetRunners[k];
      if (processed.lookup(it->Id, v))
        continue;
      if (it->CardNo > 0) {
        pRunner r = getRunnerByCard(it->CardNo);

        if (!r || used.lookup(r->Id, v))
          continue;

        __int64 id1 = r->getExtIdentifier();
        __int64 id2 = it->getExtIdentifier();

        if (id1>0 && id2>0 && id1 != id2)
          continue;
        
        if ((id1>0 && id1==id2) || (it->Name == r->Name && it->getClub() == r->getClub())) {
          processed.insert(it->Id, 1);      
          used.insert(r->Id, 1);
          it->setInputData(*r);
          checkTargetClass(it, r, ce.Classes, targetVacant, changedClass);
        }
      }
    }
    setupCardHash(true); // Clear cache
  }

  int v = -1;

  // Store remaining runners
  vector<pRunner> remainingRunners;
  for (oRunnerList::iterator it2 = Runners.begin(); it2 != Runners.end(); ++it2) {
    if (it2->skip() || used.lookup(it2->Id, v))
      continue;
    if (it2->isVacant())
      continue; // Ignore vacancies on source side

    remainingRunners.push_back(&*it2);
  }

  if (processed.size() < int(targetRunners.size()) && !remainingRunners.empty()) {
    // Lookup by name / ext id
    vector<int> cnd;
    for (size_t k = 0; k < targetRunners.size(); k++) {
      pRunner it = targetRunners[k];
      if (processed.lookup(it->Id, v))
        continue;

      __int64 id1 = it->getExtIdentifier();

      cnd.clear();
      for (size_t j = 0; j < remainingRunners.size(); j++) {
        pRunner src = remainingRunners[j];
        if (!src)
          continue;
        
        if (id1 > 0) {
          __int64 id2 = src->getExtIdentifier();
          if (id2 == id1) {
            cnd.clear();
            cnd.push_back(j);
            break; //This is the one, if they have the same Id there will be a unique match below
          }
        }

        if (it->Name == src->Name && it->getClub() == src->getClub())
          cnd.push_back(j);
      }

      if (cnd.size() == 1) {
        pRunner &src = remainingRunners[cnd[0]];
        processed.insert(it->Id, 1);      
        used.insert(src->Id, 1);
        it->setInputData(*src);
        checkTargetClass(it, src, ce.Classes, targetVacant, changedClass);
        src = 0;
      }
      else if (cnd.size() > 0) { // More than one candidate
        int winnerIx = -1;
        int point = -1;
        for (size_t j = 0; j < cnd.size(); j++) {
          pRunner src = remainingRunners[cnd[j]];
          int p = 0;
          if (src->getClass() == it->getClass())
            p += 1;
          if (src->getBirthYear() == it->getBirthYear())
            p += 2;
          if (p > point) {
            winnerIx = cnd[j];
            point = p;
          }
        }

        if (winnerIx != -1) {
          processed.insert(it->Id, 1);
          pRunner winner = remainingRunners[winnerIx];
          remainingRunners[winnerIx] = 0;

          used.insert(winner->Id, 1);
          it->setInputData(*winner);
          checkTargetClass(it, winner, ce.Classes, targetVacant, changedClass);
        }
      }
    }
  }

  // Transfer vacancies
  for (size_t k = 0; k < remainingRunners.size(); k++) {
    pRunner src = remainingRunners[k];
    if (!src || used.lookup(src->Id, v))
      continue;

    pRunner targetVacant = ce.getRunner(src->getId(), 0);
    if (targetVacant && targetVacant->isVacant() && compareClassName(targetVacant->getClass(), src->getClass()) ) {
      targetVacant->setName(src->getName());
      targetVacant->setClub(src->getClub());
      targetVacant->setCardNo(src->getCardNo(), false);
      targetVacant->cloneData(src);
      assignedVacant.push_back(targetVacant);
    }
    else {
      pClass dstClass = ce.getClass(src->getClassId());
      if (dstClass && compareClassName(dstClass->getName(), src->getClass())) {
        if (allowNewEntries.count(src->getClassId())) {
          if (src->getClubId() > 0)
            ce.getClubCreate(src->getClubId(), src->getClub());
          pRunner dst = ce.addRunner(src->getName(), src->getClubId(), src->getClassId(),
                                     src->getCardNo(), src->getBirthYear(), true);
          dst->cloneData(src);
          dst->setInputData(*src);
          newEntries.push_back(dst);
        }
        else if (transferAllNoCompete) {
          if (src->getClubId() > 0)
            ce.getClubCreate(src->getClubId(), src->getClub());
          pRunner dst = ce.addRunner(src->getName(), src->getClubId(), src->getClassId(),
                                     0, src->getBirthYear(), true);
          dst->cloneData(src);
          dst->setInputData(*src);
          dst->setStatus(StatusNotCompetiting);
          notTransfered.push_back(dst);
        }
        else
          notTransfered.push_back(src);
      }
    }
  }

  // Runners on target side not assigned a result
  for (size_t k = 0; k < targetRunners.size(); k++) {
    if (targetRunners[k] && !processed.count(targetRunners[k]->Id)) {
      noAssignmentTarget.push_back(targetRunners[k]);
      if (targetRunners[k]->inputStatus == StatusUnknown || 
           (targetRunners[k]->inputStatus == StatusOK && targetRunners[k]->inputTime == 0)) {
        targetRunners[k]->inputStatus = StatusNotCompetiting;
      }
    }
  }
}

void oEvent::transferResult(oEvent &ce,
                            vector<pTeam> &newEntries,
                            vector<pTeam> &notTransfered,
                            vector<pTeam> &noAssignmentTarget) {

  inthashmap processed(ce.Teams.size());
  inthashmap used(Teams.size());

  newEntries.clear();
  notTransfered.clear();
  noAssignmentTarget.clear();

  vector<pTeam> targetTeams;

  targetTeams.reserve(ce.Teams.size());
  for (oTeamList::iterator it = ce.Teams.begin(); it != ce.Teams.end(); ++it) {
    if (!it->skip()) {
      targetTeams.push_back(&*it);
    }
  }

  calculateTeamResults(true);
  // Lookup by id
  for (size_t k = 0; k < targetTeams.size(); k++) {
    pTeam it = targetTeams[k];
    pTeam t = getTeam(it->Id);
    if (!t)
      continue;

    __int64 id1 = t->getExtIdentifier();
    __int64 id2 = it->getExtIdentifier();

    if (id1>0 && id2>0 && id1 != id2)
      continue;
    
    if ((id1>0 && id1==id2) || (it->Name == t->Name && it->getClub() == t->getClub())) {
      processed.insert(it->Id, 1);      
      used.insert(t->Id, 1);
      it->setInputData(*t);
      //checkTargetClass(it, r, ce.Classes, targetVacant, changedClass);
    }
  }

  int v = -1;

  // Store remaining runners
  vector<pTeam> remainingTeams;
  for (oTeamList::iterator it2 = Teams.begin(); it2 != Teams.end(); ++it2) {
    if (it2->skip() || used.lookup(it2->Id, v))
      continue;
    if (it2->isVacant())
      continue; // Ignore vacancies on source side

    remainingTeams.push_back(&*it2);
  }

  if (processed.size() < int(targetTeams.size()) && !remainingTeams.empty()) {
    // Lookup by name / ext id
    vector<int> cnd;
    for (size_t k = 0; k < targetTeams.size(); k++) {
      pTeam it = targetTeams[k];
      if (processed.lookup(it->Id, v))
        continue;

      __int64 id1 = it->getExtIdentifier();

      cnd.clear();
      for (size_t j = 0; j < remainingTeams.size(); j++) {
        pTeam src = remainingTeams[j];
        if (!src)
          continue;
        
        if (id1 > 0) {
          __int64 id2 = src->getExtIdentifier();
          if (id2 == id1) {
            cnd.clear();
            cnd.push_back(j);
            break; //This is the one, if they have the same Id there will be a unique match below
          }
        }

        if (it->Name == src->Name && it->getClub() == src->getClub())
          cnd.push_back(j);
      }

      if (cnd.size() == 1) {
        pTeam &src = remainingTeams[cnd[0]];
        processed.insert(it->Id, 1);      
        used.insert(src->Id, 1);
        it->setInputData(*src);
        //checkTargetClass(it, src, ce.Classes, targetVacant, changedClass);
        src = 0;
      }
      else if (cnd.size() > 0) { // More than one candidate
        int winnerIx = -1;
        int point = -1;
        for (size_t j = 0; j < cnd.size(); j++) {
          pTeam src = remainingTeams[cnd[j]];
          int p = 0;
          if (src->getClass() == it->getClass())
            p += 1;
          if (p > point) {
            winnerIx = cnd[j];
            point = p;
          }
        }

        if (winnerIx != -1) {
          processed.insert(it->Id, 1);
          pTeam winner = remainingTeams[winnerIx];
          remainingTeams[winnerIx] = 0;

          used.insert(winner->Id, 1);
          it->setInputData(*winner);
          //checkTargetClass(it, winner, ce.Classes, targetVacant, changedClass);
        }
      }
    }
  }
/*
  // Transfer vacancies
  for (size_t k = 0; k < remainingRunners.size(); k++) {
    pRunner src = remainingRunners[k];
    if (!src || used.lookup(src->Id, v))
      continue;

    pRunner targetVacant = ce.getRunner(src->getId(), 0);
    if (targetVacant && targetVacant->isVacant() && compareClassName(targetVacant->getClass(), src->getClass()) ) {
      targetVacant->setName(src->getName());
      targetVacant->setClub(src->getClub());
      targetVacant->setCardNo(src->getCardNo(), false);
      targetVacant->cloneData(src);
      assignedVacant.push_back(targetVacant);
    }
    else {
      pClass dstClass = ce.getClass(src->getClassId());
      if (dstClass && compareClassName(dstClass->getName(), src->getClass())) {
        if (allowNewEntries.count(src->getClassId())) {
          if (src->getClubId() > 0)
            ce.getClubCreate(src->getClubId(), src->getClub());
          pRunner dst = ce.addRunner(src->getName(), src->getClubId(), src->getClassId(),
                                     src->getCardNo(), src->getBirthYear(), true);
          dst->cloneData(src);
          dst->setInputData(*src);
          newEntries.push_back(dst);
        }
        else if (transferAllNoCompete) {
          if (src->getClubId() > 0)
            ce.getClubCreate(src->getClubId(), src->getClub());
          pRunner dst = ce.addRunner(src->getName(), src->getClubId(), src->getClassId(),
                                     0, src->getBirthYear(), true);
          dst->cloneData(src);
          dst->setInputData(*src);
          dst->setStatus(StatusNotCompetiting);
          notTransfered.push_back(dst);
        }
        else
          notTransfered.push_back(src);
      }
    }
  }

  // Runners on target side not assigned a result
  for (size_t k = 0; k < targetRunners.size(); k++) {
    if (targetRunners[k] && !processed.count(targetRunners[k]->Id)) {
      noAssignmentTarget.push_back(targetRunners[k]);
      if (targetRunners[k]->inputStatus == StatusUnknown || 
           (targetRunners[k]->inputStatus == StatusOK && targetRunners[k]->inputTime == 0)) {
        targetRunners[k]->inputStatus = StatusNotCompetiting;
      }
    }

  }*/
}

MetaListContainer &oEvent::getListContainer() const {
  if (!listContainer)
    throw std::exception("Nullpointer exception");
  return *listContainer;
}

