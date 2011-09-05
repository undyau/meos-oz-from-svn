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

// oCard.cpp: implementation of the oCard class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "oCard.h"
#include "oEvent.h"
#include "gdioutput.h"
#include "table.h"
#include "Localizer.h"
#include "meos_util.h"

#include "SportIdent.h"
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

oCard::oCard(oEvent *poe): oBase(poe)
{
  Id=oe->getFreeCardId();
	CardNo=0;
	ReadId=0;
  tOwner=0;
}

oCard::oCard(oEvent *poe, int id): oBase(poe)
{
  Id=id;
	CardNo=0;
	ReadId=0;
  tOwner=0;
  oe->qFreeCardId = max(id, oe->qFreeCardId);
}


oCard::~oCard()
{
	
}

bool oCard::Write(xmlparser &xml)
{
	if(Removed) return true;

	xml.startTag("Card");
	xml.write("CardNo", CardNo);
	xml.write("Punches", getPunchString());
	xml.write("ReadId", ReadId);
	xml.write("Id", Id);
	xml.write("Updated", Modified.GetStamp());
	xml.endTag();

	return true;
}

void oCard::Set(const xmlobject &xo)
{
	xmlList xl;
  xo.getObjects(xl);
	xmlList::const_iterator it;

	for(it=xl.begin(); it != xl.end(); ++it){
		if(it->is("CardNo")){
			CardNo = it->getInt();			
		}
		else if(it->is("Punches")){
			importPunches(it->get());
		}
		else if(it->is("ReadId")){
			ReadId = it->getInt();			
		}
		else if(it->is("Id")){
			Id = it->getInt();			
		}
		else if(it->is("Updated")){
			Modified.SetStamp(it->get());
		}
	}
}

void oCard::setCardNo(int c)
{
	if(CardNo!=c)
		updateChanged();

	CardNo=c;
}

string oCard::getCardNo() const
{
  char bf[32];
  sprintf_s(bf, "%d", CardNo);
  return bf;
}

void oCard::addPunch(int type, int time, int matchControlId)
{
	oPunch p(oe);
	p.Time = time;
	p.Type = type;
  p.tMatchControlId = matchControlId;
  p.isUsed =  matchControlId!=0;

  if (Punches.empty())
    Punches.push_back(p);
  else {
    oPunch oldBack = Punches.back();
    if (oldBack.isFinish()) { //Make sure finish is last.
      Punches.pop_back();
      Punches.push_back(p);
      Punches.push_back(oldBack);
    }
    else
      Punches.push_back(p);
  }
	updateChanged();
}

string oCard::getPunchString()
{
	oPunchList::iterator it;

	string punches;

	for(it=Punches.begin(); it != Punches.end(); ++it){
		punches+=it->codeString();
	}

	return punches;
}

void oCard::importPunches(string s)
{
	int startpos=0;
	int endpos;

	endpos=s.find_first_of(';', startpos);
	Punches.clear();

	while(endpos!=string::npos) {
		oPunch p(oe);
		p.decodeString(s.substr(startpos, endpos));
		Punches.push_back(p);
		startpos=endpos+1;
		endpos=s.find_first_of(';', startpos);
	}
	return;
}

bool oCard::fillPunches(gdioutput &gdi, string name, pCourse crs)
{
	oPunchList::iterator it;	
	synchronize(true);

	gdi.clearList(name);
	
	bool HasStart=false;
	bool HasFinish=false;
	bool Extra=false;
	int k=0;

  pControl ctrl=0;
 
  int matchPunch=0;
  int punchRemain=1;
  if(crs)
    ctrl=crs->getControl(matchPunch);

  if(ctrl)
    punchRemain=ctrl->getNumMulti();

  map<int, pPunch> rogainingIndex;
  for (it=Punches.begin(); it != Punches.end(); ++it) {			
    if (it->tRogainingIndex >= 0)
      rogainingIndex[it->tRogainingIndex] = &*it;
  }

	for (it=Punches.begin(); it != Punches.end(); ++it){			
		if(!HasStart && !it->isStart()){
			if(it->isUsed){
				gdi.addItem(name, lang.tl("Start")+"\t-", 0);
				HasStart=true;
			}
		}

    if (it->tRogainingIndex != -1)
      continue;    

    {
		  if(it->isStart())  
			  HasStart=true;
      else if(it->isUsed && !it->isFinish() &&  !it->isCheck()) {
        while(ctrl && it->tMatchControlId!=ctrl->getId()) {
          if (ctrl->getStatus() == oControl::StatusRogaining) {
            if (rogainingIndex.count(matchPunch) == 1)
              gdi.addItem(name, rogainingIndex[matchPunch]->getString(), 
                          int(rogainingIndex[matchPunch]));
            else
              gdi.addItem(name, "-\t-", 0);
          }
          else {
            while(0<punchRemain--) {
              gdi.addItem(name, "-\t-", 0);
            }
          }
          // Next control
          ctrl=crs ? crs->getControl(++matchPunch):0;          
          punchRemain=ctrl ? ctrl->getNumMulti() : 1;
        }
      }
      
      if(it->isUsed || it->isFinish() || it->isStart()) {
        gdi.addItem(name, it->getString(), int(&*it));
       
        if(!(it->isFinish() || it->isStart())) {
          punchRemain--;
          if(punchRemain<=0) {
            // Next contol
            ctrl = crs ? crs->getControl(++matchPunch):0;

            // Match rogaining here
            while (ctrl && ctrl->getStatus() == oControl::StatusRogaining) {
              if (rogainingIndex.count(matchPunch) == 1)
                gdi.addItem(name, rogainingIndex[matchPunch]->getString(), 
                            int(rogainingIndex[matchPunch]));
              else
                gdi.addItem(name, "-\t-", 0);
              ctrl = crs->getControl(++matchPunch);  
            }
            punchRemain = ctrl ? ctrl->getNumMulti() : 1;
          }
        }
      }
		  else Extra=true;
  		
      k++;

		  if(it->isFinish())
			  HasFinish=true;
    }
	}

	if(!HasStart)
		gdi.addItem(name, lang.tl("Start")+"\t-", 0);
	
	if(!HasFinish)
		gdi.addItem(name, lang.tl("Mål")+"\t-", 0);
	

	if(Extra)	{
		//Show punches that are not used.
		k=0;
		gdi.addItem(name, "", 0);
		gdi.addItem(name, lang.tl("Extra stämplingar"), 0);
		for (it=Punches.begin(); it != Punches.end(); ++it)
		{			
			if(!it->isUsed && !it->isFinish() && !it->isStart())
				gdi.addItem(name, it->getString(), int(&*it));

			//k++;

		}
	}
	return true;
}

void oCard::insertPunchAfter(int pos, int type, int time)
{
	if(pos==1023)
		return;

	oPunchList::iterator it;	

	oPunch punch(oe);
	punch.Time=time;
	punch.Type=type;

	int k=-1;
	for (it=Punches.begin(); it != Punches.end(); ++it)	{	
		if(k==pos) {
			updateChanged();
			Punches.insert(it, punch);
			return;
		}
		k++;		
	}

	updateChanged();
	//Insert last
	Punches.push_back(punch);
}

void oCard::deletePunch(pPunch pp)
{
	int k=0;
	oPunchList::iterator it;
	
	for (it=Punches.begin(); it != Punches.end(); ++it)	{	
		if(&*it == pp) {
			Punches.erase(it);
			updateChanged();
			return;
		}
		k++;		
	}
}

string oCard::getInfo() const
{
  char bf[128];
  sprintf_s(bf, lang.tl("Löparbricka %d").c_str(), CardNo);
  return bf;
}

oPunch *oCard::getPunch(const pPunch punch)
{
	int k=0;
	oPunchList::iterator it;

	for (it=Punches.begin(); it != Punches.end(); ++it) {	
		if(&*it == punch)	return &*it;		
		k++;		
	}
	return 0;
}


oPunch *oCard::getPunchByType(int Type) const
{
	oPunchList::const_iterator it;

	for (it=Punches.begin(); it != Punches.end(); ++it)
		if(it->Type==Type) 
			return pPunch(&*it);		

	return 0;
}

oPunch *oCard::getPunchById(int Id) const
{
	oPunchList::const_iterator it;

	for (it=Punches.begin(); it != Punches.end(); ++it)
    if(it->tMatchControlId==Id) 
			return pPunch(&*it);		

	return 0;
}


oPunch *oCard::getPunchByIndex(int ix) const
{
	oPunchList::const_iterator it;
  for (it=Punches.begin(); it != Punches.end(); ++it) {
    if (0 == ix--)
      return pPunch(&*it);
  }
	return 0;
}


void oCard::setReadId(const SICard &card)
{
	updateChanged();
	ReadId=card.nPunch*100000+card.FinishPunch.Time;
}

bool oCard::isCardRead(const SICard &card) const
{
	if(ReadId==(card.nPunch*100000+card.FinishPunch.Time))
		return true;
	else return false;
}

pRunner oCard::getOwner() const {
  return tOwner && !tOwner->isRemoved() ? tOwner : 0;
}

bool oCard::setPunchTime(const pPunch punch, string time)
{
	oPunch *op=getPunch(punch);
	if(!op) return false;

	DWORD ot=op->Time;
	op->setTime(time);
	
	if(ot!=op->Time)
		updateChanged();
	
	return true;
}


pCard oEvent::getCard(int Id) const
{
	oCardList::const_iterator it;
		
	for (it=Cards.begin(); it != Cards.end(); ++it){
		if(it->Id==Id)	
      return const_cast<pCard>(&*it);
	}	
	return 0;
}

pCard oEvent::addCard(const oCard &oc)
{
  if (oc.Id<=0)
    return 0;

  Cards.push_back(oc);

  return &Cards.back();
}


pCard oEvent::getCardByNumber(int cno) const
{
	oCardList::const_iterator it;
		
	for (it=Cards.begin(); it != Cards.end(); ++it){
		if(it->CardNo==cno)	
      return const_cast<pCard>(&*it);
	}	
	return 0;
}

bool oEvent::isCardRead(const SICard &card) const
{
  oCardList::const_iterator it;

	for(it=Cards.begin(); it!=Cards.end(); ++it) {
		if(it->CardNo==card.CardNumber && it->isCardRead(card))
			return true;
	}

	return false;
}


Table *oEvent::getCardsTB() //Table mode
{	
 	oCardList::iterator it;	

  Table *table=new Table(this, 20, "Brickor", "cards");

  table->addColumn("Id", 70, false);
  table->addColumn("Ändrad", 70, false);

	table->addColumn("Bricka", 120, true);
	table->addColumn("Deltagare", 200, false);

  table->addColumn("Starttid", 70, false);
  table->addColumn("Måltid", 70, false);
  table->addColumn("Stämplingar", 70, false);
  
  table->setTableProp(Table::CAN_DELETE);
  table->update();

	return table;
}

void oEvent::generateCardTableData(Table &table, oCard *addCard)
{	
  if (addCard) {
    addCard->addTableRow(table);
    return;
  }

 	oCardList::iterator it;	
	synchronizeList(oLCardId);
  synchronizeList(oLRunnerId);

  for (it=Cards.begin(); it!=Cards.end(); ++it) {
    if(!it->isRemoved()) {
      it->addTableRow(table);
    }
  }
}

void oCard::addTableRow(Table &table) const {
			
  string runner(lang.tl("Oparad"));
  if(getOwner()) 
    runner = tOwner->getNameAndRace();

  oCard &it = *pCard(this);
  table.addRow(getId(), &it);

  int row = 0;
  table.set(row++, it, TID_ID, itos(getId()), false);
  table.set(row++, it, TID_MODIFIED, getTimeStamp(), false);

  table.set(row++, it, TID_CARD, getCardNo(), true, cellAction);

  table.set(row++, it, TID_RUNNER, runner, true, cellAction);

  oPunch *p=getPunchByType(oPunch::PunchStart);
  string time = "-";
  if (p) 
    time = p->getTime();
  table.set(row++, it, TID_START, time, false, cellEdit);	

  p = getPunchByType(oPunch::PunchFinish);
  time = "-";
  if (p) 
    time = p->getTime();
  table.set(row++, it, TID_FINISH, time, false, cellEdit);	

  char npunch[16];
  sprintf_s(npunch, "%d", getNumPunches());
  table.set(row++, it, TID_COURSE, npunch, false, cellEdit);
}

oDataInterface oCard::getDI() 
{
  throw std::exception("Unsupported");
}

oDataConstInterface oCard::getDCI() const
{
  throw std::exception("Unsupported");
}

int oCard::getSplitTime(int startTime, const pPunch punch) const {

  for (oPunchList::const_iterator it = Punches.begin(); it != Punches.end(); ++it) {
    if (&*it == punch) {
      int t = it->getAdjustedTime();
      if (t<=0)
        return -1;

      if (startTime > 0)
        return t - startTime;
      else
        return -1;
    }
    else if (it->isUsed)
      startTime = it->getAdjustedTime();
  }
  return -1;
}


string oCard::getRogainingSplit(int ix, int startTime) const 
{
	oPunchList::const_iterator it;
  for (it = Punches.begin(); it != Punches.end(); ++it) {
    int t = it->getAdjustedTime();
    if (0 == ix--) {
      if (t > 0 && t > startTime)
        return formatTime(t - startTime);
    }
    if (it->isUsed)
      startTime = t;
  }
 return "-";
}

void oCard::remove() 
{
  if (oe)
    oe->removeCard(Id);
}

bool oCard::canRemove() const 
{
  return getOwner() == 0;
}
