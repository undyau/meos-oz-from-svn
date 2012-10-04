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

#include "stdafx.h"
#include "oFreePunch.h"
#include "oEvent.h"
#include "Table.h"
#include "meos_util.h"
#include "Localizer.h"

oFreePunch::oFreePunch(oEvent *poe, int card, int time, int type): oPunch(poe)
{
  Id=oe->getFreePunchId();
  CardNo = card;
	Time = time;
	Type = type;
  itype = 0;
  tRunnerId = 0;
}

oFreePunch::oFreePunch(oEvent *poe, int id): oPunch(poe)
{
  Id=id;
  oe->qFreePunchId = max(id, oe->qFreePunchId);
  itype = 0;
  tRunnerId = 0;
}

oFreePunch::~oFreePunch(void)
{
}

bool oFreePunch::Write(xmlparser &xml)
{
	if(Removed) return true;

	xml.startTag("Punch");
	xml.write("CardNo", CardNo);
	xml.write("Time", Time);
	xml.write("Type", Type);
	xml.write("Id", Id);
	xml.write("Updated", Modified.GetStamp());
	xml.endTag();

	return true;
}

void oFreePunch::Set(const xmlobject *xo)
{
	xmlList xl;
  xo->getObjects(xl);

	xmlList::const_iterator it;

	for(it=xl.begin(); it != xl.end(); ++it){
		if(it->is("CardNo")){
			CardNo=it->getInt();			
		}
		if(it->is("Type")){
			Type=it->getInt();			
		}			
		if(it->is("Time")){
			Time=it->getInt();			
		}			
		else if(it->is("Id")){
			Id=it->getInt();			
		}
		else if(it->is("Updated")){
			Modified.SetStamp(it->get());
		}
	}
}

void oFreePunch::setCardNo(int cno, bool databaseUpdate) {
  if (cno != CardNo) {
    oe->punchIndex[itype].remove(CardNo); // Remove from index
    CardNo = cno;
    pRunner r1 = oe->getRunner(tRunnerId, 0);
    itype = oe->getControlIdFromPunch(Time, Type, CardNo, true, tRunnerId);
    pRunner r2 = oe->getRunner(tRunnerId, 0);
    
    if (r1)
      r1->markClassChanged();
    if (r2)
      r2->markClassChanged();

    oe->punchIndex[itype][CardNo] = this; // Insert into index
    if (!databaseUpdate)
      updateChanged();
  }
}

/*
void oFreePunch::setData(int card, int time, int type)
{
	CardNo=card;
	Time=time;
	Type=type;
	updateChanged();	
}*/

void oFreePunch::remove() 
{
  if (oe)
    oe->removeFreePunch(Id);
}

bool oFreePunch::canRemove() const 
{
  return true;
}

Table *oEvent::getPunchesTB()//Table mode
{
  if (tables.count("punch") == 0) {
	  Table *table=new Table(this, 20, "Stämplingar", "punches");
	  table->addColumn("Id", 70, true);
    table->addColumn("Ändrad", 150, false);
    table->addColumn("Bricka", 70, true);
    table->addColumn("Kontroll", 70, true);
    table->addColumn("Tid", 70, false);
    table->addColumn("Löpare", 170, false);
    table->addColumn("Lag", 170, false);
    tables["punch"] = table;
    table->addOwnership();
  }

  tables["punch"]->update();  
  return tables["punch"];
}

void oEvent::generatePunchTableData(Table &table, oFreePunch *addPunch)
{ 
  if (addPunch) {
    addPunch->addTableRow(table);
    return;
  }

  synchronizeList(oLPunchId);
  oFreePunchList::iterator it;	
  oe->setupCardHash(false);
  table.reserve(punches.size());
  for (it = punches.begin(); it != punches.end(); ++it){		
    if(!it->isRemoved()){
      it->addTableRow(table);
 		}
	}
  oe->setupCardHash(true);
}

void oFreePunch::addTableRow(Table &table) const {
  oFreePunch &it = *pFreePunch(this);
  table.addRow(getId(), &it);
  int row = 0;
  table.set(row++, it, TID_ID, itos(getId()), false, cellEdit);
  table.set(row++, it, TID_MODIFIED, getTimeStamp(), false, cellEdit);
  table.set(row++, it, TID_CARD, itos(getCardNo()), true, cellEdit);
  table.set(row++, it, TID_CONTROL, getType(), true, cellEdit);
  table.set(row++, it, TID_TIME, getTime(), true, cellEdit);
  pRunner r = 0;
  if (CardNo > 0)
    r = oe->getRunnerByCard(CardNo, false);

  table.set(row++, it, TID_RUNNER, r ? r->getName() : "?", false, cellEdit);

  if (r && r->getTeam())
    table.set(row++, it, TID_TEAM, r->getTeam()->getName(), false, cellEdit);
  else
    table.set(row++, it, TID_TEAM, "", false, cellEdit);
}

bool oFreePunch::inputData(int id, const string &input, 
                           int inputId, string &output, bool noUpdate)
{
  synchronize(false);    
  switch(id) {
    case TID_CARD:
      setCardNo(atoi(input.c_str()));
      synchronize(true);
      output = itos(CardNo);
      break;

    case TID_TIME:
      setTime(input);
      synchronize(true);
      output = getTime();
      break;

    case TID_CONTROL:
      setType(input);
      synchronize(true);
      output = getType();
  }
  return false;
}

void oFreePunch::fillInput(int id, vector< pair<string, size_t> > &out, size_t &selected)
{ 
}

void oFreePunch::setType(const string &t, bool databaseUpdate) {
  int type = atoi(t.c_str());
  int ttype = 0;
  if (type>0 && type<10000)
    ttype = type;
  else {
    if (t == lang.tl("Check"))
      ttype = oPunch::PunchCheck;
    else if (t == lang.tl("Mål"))
      ttype = oPunch::PunchFinish;
    if (t == lang.tl("Start"))
      ttype = oPunch::PunchStart;
  }
  if (ttype > 0 && ttype != Type) {
    oe->punchIndex[itype].remove(CardNo); // Remove from index
    pRunner r1 = oe->getRunner(tRunnerId, 0);
    Type = ttype;
    itype = oe->getControlIdFromPunch(Time, Type, CardNo, true, tRunnerId);
    pRunner r2 = oe->getRunner(tRunnerId, 0);

    if (r1)
      r1->markClassChanged();
    if (r2)
      r2->markClassChanged();

    oe->punchIndex[itype][CardNo] = this; // Insert into index
    if (!databaseUpdate)
      updateChanged();    
  }
}
