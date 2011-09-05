// oCourse.h: interface for the oCourse class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_OCOURSE_H__936E61C9_CDAC_490D_A475_E58190A2910C__INCLUDED_)
#define AFX_OCOURSE_H__936E61C9_CDAC_490D_A475_E58190A2910C__INCLUDED_

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

#include "oControl.h"
#include <map>
class oEvent;
class oCourse;
typedef oCourse * pCourse;

class gdioutput;
class oDataInterface;

struct SICard;

const int NControlsMax = 128;

class oCourse : public oBase
{
protected:
	pControl Controls[NControlsMax];
  
	int nControls;
	string Name;
	int Length;
	BYTE oData[64];
  
  // Length of each leg, Start-1, 1-2,... N-Finish.
  vector<int> legLengths;

  int tMapsRemaining;

  // Get an identity sum based on controls
  int getIdSum(int nControls);

  /// Add an control without update
  pControl doAddControl(int Id);

public:
  void remove();
  bool canRemove() const;

  bool operator<(const oCourse &b) const {return Name<b.Name;}

  void setNumberMaps(int nm);
  int getNumberMaps() const;

  /// Check if course has problems
  string getCourseProblems() const;

  int getNumControls() const {return nControls;}
  void setLegLengths(const vector<int> &legLengths);

  // Get/set the minimal number of rogaining points to pass
  int getMinimumRogainingPoints() const;
  void setMinimumRogainingPoints(int p);

  // Get/set the maximal time allowed for rogaining
  int getMaximumRogainingTime() const;
  void setMaximumRogainingTime(int t);

  // Rogaining: point lost per minute over maximal time 
  int getRogainingPointsPerMinute() const;
  void setRogainingPointsPerMinute(int t);

  // Get the control number as "printed on map". Do not count
  // rogaining controls
  string getControlOrdinal(int controlIndex) const;

  /** Get the part of the course between the start and end. Use start = 0 for the 
      start of the course, and end = 0 for the finish. Returns 0 if fraction 
      cannot be determined */
  double getPartOfCourse(int start, int end) const;

  string getInfo() const;

	oDataInterface getDI();
	oDataConstInterface getDCI() const;

	oControl *getControl(int index);

  /** Return the distance between the course and the card.
      Positive return = extra controls
      Negative return = missing controls
      Zero return = exact match */
	int distance(const SICard &card);

	bool fillCourse(gdioutput &gdi, const string &name);

	void importControls(const string &cstring);
	void importLegLengths(const string &legs);

  pControl addControl(int Id);
	void Set(const xmlobject *xo);

  void getControls(list<pControl> &pc);
	string getControls() const;
	string getLegLengths() const;
	
  string getControlsUI() const;
	vector<string> getCourseReadable(int limit) const;

	const string &getName() const {return Name;}
	int getLength() const {return Length;}
  string getLengthS() const;

	void setName(const string &n);
	void setLength(int l);

  string getStart() const;
  void setStart(const string &start, bool sync);

	bool Write(xmlparser &xml);

  oCourse(oEvent *poe, int id);
	oCourse(oEvent *poe);
	virtual ~oCourse();

	friend class oEvent;
	friend class oClass;
	friend class oRunner;
	friend class MeosSQL;
};

#endif // !defined(AFX_OCOURSE_H__936E61C9_CDAC_490D_A475_E58190A2910C__INCLUDED_)
