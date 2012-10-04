// oBase.h: interface for the oBase class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_OBASE_H__C950D806_EEC7_4C56_B298_132C67FCF719__INCLUDED_)
#define AFX_OBASE_H__C950D806_EEC7_4C56_B298_132C67FCF719__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

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

#include "TimeStamp.h"
#include "stdafx.h"
#include <vector>

class oEvent;
class gdioutput;
class oDataInterface;
class oDataConstInterface;

enum RunnerStatus {StatusOK=1, StatusDNS=20, StatusMP=3, 
                   StatusDNF=4, StatusDQ=5, StatusMAX=6, 
                   StatusUnknown=0};

enum SortOrder {ClassStartTime,
                ClassResult,
                ClassCourseResult,
                ClassTotalResult, 
                ClassFinishTime, 
                ClassStartTimeClub,
                ClassPoints, 
                SortByName, 
                SortByFinishTime, 
                SortByStartTime, 
                CourseResult,
                SortEnumLastItem};

class oBase  
{
private:
  void storeChangeStatus() {reChanged = changed;}
  void resetChangeStatus() {changed &= reChanged;}
  bool reChanged;
	bool changed;

protected:
	int Id;
	TimeStamp Modified;
	string sqlUpdated; //SQL TIMESTAMP
  int counter;

  // True if the object is incorrect and needs correction
  // An example is if id changed as we wrote. Then owner
  // needs to be updated.
  bool correctionNeeded;

	oEvent *oe;
	bool Removed;

	//Used for selections, etc.
	int _objectmarker;
  void updateChanged(){Modified.Update(); changed=true;}

  //Used for handeling GUI combo boxes;
  static void clearCombo(HWND hWnd);
  static void addToCombo(HWND hWnd, const string &str, int data);
public:
  /// Returns textual information on the object
  virtual string getInfo() const = 0;

  //Called (by a table) when user enters data in a cell
  virtual bool inputData(int id, const string &input, int inputId, 
                         string &output, bool noUpdate) {output=""; return false;}
  
  //Called (by a table) to fill a list box with contents in a table
  virtual void fillInput(int id, vector< pair<string, size_t> > &elements, size_t &selected) 
    {throw std::exception("Not implemented");}

  oEvent *getEvent() const {return oe;}
	int getId() const {return Id;}
  /// This is not database safe
  void changeId(int id) {Id=id;}
	bool isChanged() const {return changed;}
	bool isRemoved() const {return Removed;}
	int getAge() const {return Modified.GetAge();}
	bool synchronize(bool writeOnly=false);
  string getTimeStamp() const {return Modified.getStampString();}

  bool existInDB() const { return !sqlUpdated.empty(); }

	virtual oDataInterface getDI() = 0;
  
  virtual oDataConstInterface getDCI() const = 0;

  // Remove object from the competition
  virtual void remove() = 0;

  // Check if object can be remove (is not used by someone else)
  virtual bool canRemove() const = 0;

  /// Set an external identifier (0 if none)
  void setExtIdentifier(__int64 id);
  
  /// Get an external identifier (or 0) if none
  __int64 getExtIdentifier() const;

	oBase(oEvent *poe);
	virtual ~oBase();

  friend class RunnerDB;
	friend class MeosSQL;
	friend class oEvent;
	friend class oDataInterface;
  friend class oDataContainer;
  friend class MetaListContainer;
};

typedef oBase * pBase;

#endif // !defined(AFX_OBASE_H__C950D806_EEC7_4C56_B298_132C67FCF719__INCLUDED_)
