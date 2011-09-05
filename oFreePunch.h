#pragma once

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

class oFreePunch;
typedef oFreePunch* pFreePunch;
class Table;

class oFreePunch : public oPunch
{
protected:
	int CardNo;
  int itype; //Index type used for lookup
  int tRunnerId; // Id of runner the punch is classified to.
public:
  
  void addTableRow(Table &table) const;
  void fillInput(int id, vector< pair<string, size_t> > &out, size_t &selected);
  bool inputData(int id, const string &input, int inputId, string &output, bool noUpdate);
  
  void remove();
  bool canRemove() const;

	int getCardNo() const {return CardNo;}
  void setCardNo(int cardNo, bool databaseUpdate = false);

  void setType(const string &t, bool databaseUpdate = false);

	//void setData(int card, int time, int type);
	oFreePunch(oEvent *poe, int card, int time, int type);
	oFreePunch(oEvent *poe, int id);
  virtual ~oFreePunch(void);

	void Set(const xmlobject *xo);
	bool Write(xmlparser &xml);

	friend class oEvent;
	friend class oRunner;
	friend class MeosSQL;
};
