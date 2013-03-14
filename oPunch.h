// oPunch.h: interface for the oPunch class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_OPUNCH_H__67B23AF5_5783_4A6A_BB2E_E522B9283A42__INCLUDED_)
#define AFX_OPUNCH_H__67B23AF5_5783_4A6A_BB2E_E522B9283A42__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include "oBase.h"

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
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    Melin Software HB - software@melin.nu - www.melin.nu
    Stigbergsvägen 7, SE-75242 UPPSALA, Sweden
    
************************************************************************/

class oEvent;

class oPunch : public oBase
{
protected:

	int Type;
	int Time;
	bool isUsed; //Is used in the course...

  int tRogainingIndex;

  //Adjustment of this punch, loaded from control 
  int tTimeAdjust;

  int tIndex; // Control match index in course
  int tMatchControlId;
	bool hasBeenPlayed;
public:
  void remove();
  bool canRemove() const;

  string getInfo() const;

	bool isStart() const {return Type==PunchStart;}
	bool isFinish() const {return Type==PunchFinish;}
  bool isCheck() const {return Type==PunchCheck;}
  int getControlNumber() const {return Type>=30 ? Type : 0;}
  string getType() const;
  
	string getString() const ;
	string getSimpleString() const;

	string getTime() const;
  int getAdjustedTime() const;
	void setTime(string t);
  virtual void setTimeInt(int newTime, bool databaseUpdate);

  void setTimeAdjust(int t) {tTimeAdjust=t;}
  void adjustTimeAdjust(int t) {tTimeAdjust+=t;}

  string getRunningTime(int startTime) const;

	enum{PunchStart=1, PunchFinish=2, PunchCheck=3};
	void decodeString(const string &s);
	string codeString() const;
	oPunch(oEvent *poe);
	virtual ~oPunch();

	oDataInterface getDI(void);
  oDataConstInterface getDCI(void) const;

	friend class oCard;
	friend class oRunner;
	friend class oEvent;
};

typedef oPunch * pPunch;

#endif // !defined(AFX_OPUNCH_H__67B23AF5_5783_4A6A_BB2E_E522B9283A42__INCLUDED_)
