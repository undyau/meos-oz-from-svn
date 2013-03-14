// oClub.h: interface for the oClub class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_OCLUB_H__8B2917E2_6A48_4E7F_82AD_4F8C64167439__INCLUDED_)
#define AFX_OCLUB_H__8B2917E2_6A48_4E7F_82AD_4F8C64167439__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

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

#include "xmlparser.h"
#include "oBase.h"
class oEvent;

class oClub;
class oRunner;
typedef oRunner* pRunner;
class oTeam;
typedef oTeam* pTeam;

typedef oClub* pClub;
class oDataInterface;
class oDataConstInterface;
class Table;

class oClub : public oBase
{
protected:
	string name;
  vector<string> altNames;
  string tPrettyName;
	BYTE oData[384];

  int tNumRunners;
  int tFee;
  int tPaid;

  bool inputData(int id, const string &input, int inputId, 
                        string &output, bool noUpdate);

  void fillInput(int id, vector< pair<string, size_t> > &elements, size_t &selected);

  void exportIOFClub(xmlparser &xml, bool compact) const; 

  void exportClubOrId(xmlparser &xml) const;

  // Set name internally, and update pretty name
  void internalSetName(const string &n);

  void changeId(int newId);

  struct InvoiceData {
    int yp;
    int xs;
    int nameIndent;
    int adrPos;
    int clsPos;
    bool multiDay;
    int cardPos;
    int feePos;
    int paidPos;
    int resPos;
    int total_fee_amount;
    int total_rent_amount;
    int total_paid_amount;
    int lh;//lineheight
    InvoiceData(int lh_) {
      memset(this, 0, sizeof(InvoiceData));
      lh = lh_;
    }
  };

  void addRunnerInvoiceLine(gdioutput &gdi, const pRunner r, InvoiceData &data, bool inTeam) const;
  void addTeamInvoiceLine(gdioutput &gdi, const pTeam r, InvoiceData &data) const;
public:
  static void buildTableCol(oEvent *oe, Table *t);
  void addTableRow(Table &table) const;

  void remove();
  bool canRemove() const;

  void updateFromDB();

  bool operator<(const oClub &c) {return name<c.name;}
  void generateInvoice(gdioutput &gdi, int number, int &toPay, int &hasPaid) const;

  string getInfo() const {return "Klubb " + name;}
  bool sameClub(const oClub &c); 

	oDataInterface getDI();
  oDataConstInterface getDCI() const;

  const string &getName() const {return name;}

  const string &getDisplayName() const {return tPrettyName.empty() ?  name : tPrettyName;}
  
  void setName(const string &n);
	
  void set(const xmlobject &xo);
	bool write(xmlparser &xml);

  bool isVacant() const;
	oClub(oEvent *poe);
	oClub(oEvent *poe, int id);
  virtual ~oClub();
	
  friend class oAbstractRunner;
	friend class oEvent;
	friend class oRunner;
	friend class oTeam;
  friend class oClass;
	friend class MeosSQL;
};

#endif // !defined(AFX_OCLUB_H__8B2917E2_6A48_4E7F_82AD_4F8C64167439__INCLUDED_)
