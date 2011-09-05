// oCard.h: interface for the oCard class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_OCARD_H__674EAB76_A232_4E44_A9B4_C52F6A04D7CF__INCLUDED_)
#define AFX_OCARD_H__674EAB76_A232_4E44_A9B4_C52F6A04D7CF__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

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

#include "oBase.h"
#include "oPunch.h"

#include "xmlparser.h"

typedef list<oPunch> oPunchList;

class gdioutput;
class oCard;
typedef oCard *pCard;

class oCourse;
class oRunner;
typedef oRunner *pRunner;

struct SICard;
class Table;

class oCard : public oBase {
protected:
	oPunchList Punches;
	int CardNo;
	DWORD ReadId; //Identify a specific read-out

  pRunner tOwner;
	oPunch *getPunch(const pPunch punch);
public:

  void remove();
  bool canRemove() const;

  oDataInterface getDI(void);
  oDataConstInterface getDCI(void) const;

  string getInfo() const;

  void addTableRow(Table &table) const;
  
  /// Returns the split time from the last used punch
  /// to the current punch, as indicated by evaluateCard
  int getSplitTime(int startTime, const pPunch punch) const;

  pRunner getOwner() const;
  int getNumPunches() const {return Punches.size();}

  bool setPunchTime(const pPunch punch, string time);
	bool isCardRead(const SICard &card) const;
	void setReadId(const SICard &card);
	void deletePunch(pPunch pp);
	void insertPunchAfter(int pos, int type, int time);
	bool fillPunches(gdioutput &gdi, string name, oCourse *crs);
	void addPunch(int type, int time, int matchControlId);
	oPunch *getPunchByType(int type) const;

  //Get punch by (matched) control punch id.
  oPunch *getPunchById(int id) const;
  oPunch *getPunchByIndex(int ix) const;

  // Return split time to previous matched control
  string getRogainingSplit(int ix, int startTime) const;


  string getCardNo() const;
	void setCardNo(int c);
	void importPunches(string s);
	string getPunchString();

	void Set(const xmlobject &xo);
	bool Write(xmlparser &xml);

	oCard(oEvent *poe);
  oCard(oEvent *poe, int id);
	
	virtual ~oCard();

	friend class oEvent;
	friend class oRunner;
	friend class MeosSQL;
};

#endif // !defined(AFX_OCARD_H__674EAB76_A232_4E44_A9B4_C52F6A04D7CF__INCLUDED_)
