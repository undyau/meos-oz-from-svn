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

// oClub.cpp: implementation of the oClub class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "oClub.h"
#include "meos_util.h"

#include "oEvent.h"
#include "gdioutput.h"
#include "gdifonts.h"
#include "RunnerDB.h"
#include <cassert>
#include "Table.h"
#include "localizer.h"

#include "intkeymapimpl.hpp"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

oClub::oClub(oEvent *poe): oBase(poe)
{
	getDI().initData();
	Id=oe->getFreeClubId();
}

oClub::oClub(oEvent *poe, int id): oBase(poe)
{
	getDI().initData();
	Id=id;
  if (id != cVacantId)
    oe->qFreeClubId = max(id, oe->qFreeClubId);
}


oClub::~oClub()
{

}

bool oClub::write(xmlparser &xml)
{
	if(Removed) return true;

	xml.startTag("Club");
	xml.write("Id", Id);
	xml.write("Updated", Modified.GetStamp());
	xml.write("Name", name);
  for (size_t k=0;k<altNames.size(); k++)
    xml.write("AltName", altNames[k]);

	getDI().write(xml);

	xml.endTag();

	return true;
}

void oClub::set(const xmlobject &xo)
{
	xmlList xl;
  xo.getObjects(xl);

	xmlList::const_iterator it;

	for(it=xl.begin(); it != xl.end(); ++it){
		if(it->is("Id")){
			Id=it->getInt();			
		}
		else if(it->is("Name")){
			internalSetName(it->get());
		}
		else if(it->is("oData")){
			getDI().set(*it);
		}
		else if(it->is("Updated")){
			Modified.SetStamp(it->get());
		}
    else if(it->is("AltName")) {
      altNames.push_back(it->get());
    }
	}
}

void oClub::internalSetName(const string &n)
{
  if (name != n) {
    name = n;
    const char *bf = name.c_str();
    int len = name.length();
    int ix = -1;
    for (int k=0;k <= len-9; k++) {
      if (bf[k] == 'S') {
        if (strcmp(bf+k, "Skid o OK")==0) {
          ix = k;
          break;
        }
        if (strcmp(bf+k, "Skid o OL")==0) {
          ix = k;
          break;
        }
      }
    }
    if (ix >= 0) {
      tPrettyName = name;
      if (strcmp(bf+ix, "Skid o OK")==0) 
        tPrettyName.replace(ix, 9, "SOK", 3);
      else if (strcmp(bf+ix, "Skid o OL")==0) 
        tPrettyName.replace(ix, 9, "SOL", 3);
    }
  }
}

void oClub::setName(const string &n)
{
	if (n != name) {
		internalSetName(n);
    updateChanged();
	}
}
oDataConstInterface oClub::getDCI(void) const
{
	return oe->oClubData->getConstInterface(oData, sizeof(oData), this);
}

oDataInterface oClub::getDI(void)
{
	return oe->oClubData->getInterface(oData, sizeof(oData), this);
}


pClub oEvent::getClub(int Id) const
{
  if(Id<=0) 
    return 0;

  //map<int, pClub>::const_iterator mit=clubIdIndex.find(Id);
  //if (mit!=clubIdIndex.end()) 
    //  return mit->second;

  pClub value;
  if (clubIdIndex.lookup(Id, value))
    return value;
  return 0;
}

pClub oEvent::getClub(const string &pname) const
{
	oClubList::const_iterator it;	

	for (it=Clubs.begin(); it != Clubs.end(); ++it)
		if(it->name==pname)	
      return pClub(&*it);
	
  return 0;
}

pClub oEvent::getClubCreate(int Id, const string &CreateName) 
{
  if (Id > 0) {
    //map<int, pClub>::iterator mit=clubIdIndex.find(Id);
    //if (mit!=clubIdIndex.end()) {
    pClub value;
    if (clubIdIndex.lookup(Id, value)) {
      if (!trim(CreateName).empty() && _stricmp(value->getName().c_str(), trim(CreateName).c_str())!=0)
        Id = 0; //Bad, used Id.
      if (trim(CreateName).empty() || Id>0)
        return value;
    }
  }
  if (CreateName.empty()) {    
		//Not found. Auto add...	
		return getClubCreate(Id, "Klubblös");
	}
	else	{
    oClubList::iterator it;
    string tname = trim(CreateName);

		//Maybe club exist under different ID
		for (it=Clubs.begin(); it != Clubs.end(); ++it)
      if(_stricmp(it->name.c_str(), tname.c_str())==0)
        return &*it;
	
		//Else, create club.
		return addClub(tname, Id);
	}
}

pClub oEvent::addClub(const string &pname, int createId)
{	
  if (createId>0) {
    pClub pc = getClub(createId);
    if (pc)
      return pc;
  }

  pClub dbClub = oe->runnerDB->getClub(pname);

  if (dbClub) {
    if (dbClub->getName() != pname) {
      pClub pc = getClub(dbClub->getName());
      if (pc)
        return pc;
    }

    if (createId<=0)
      if (getClub(dbClub->Id))
        createId = getFreeClubId(); //We found a db club, but Id is taken.
      else
        createId = dbClub->Id;

    oClub c(this, createId);
    c = *dbClub;
    c.Id = createId;
    Clubs.push_back(c);
  }
  else {
    if (createId==0)
      createId = getFreeClubId();
  
    oClub c(this, createId);
    c.setName(pname);
	  Clubs.push_back(c);
  }
  Clubs.back().synchronize();
  clubIdIndex[Clubs.back().Id]=&Clubs.back();
  return &Clubs.back();
}

pClub oEvent::addClub(const oClub &oc)
{	
  if(clubIdIndex.count(oc.Id)!=0)
    return clubIdIndex[oc.Id];

	Clubs.push_back(oc);	
  if (!oc.existInDB())
    Clubs.back().synchronize();

  clubIdIndex[Clubs.back().Id]=&Clubs.back();
	return &Clubs.back();
}

void oEvent::fillClubs(gdioutput &gdi, const string &id)
{
  vector< pair<string, size_t> > d;
  oe->fillClubs(d);
  gdi.addItem(id, d);
}


const vector< pair<string, size_t> > & oEvent::fillClubs(vector< pair<string, size_t> > &out)
{	
  out.clear();
	//gdi.clearList(name);
	synchronizeList(oLClubId);
  Clubs.sort();

	oClubList::iterator it;	

	for (it=Clubs.begin(); it != Clubs.end(); ++it){
		if(!it->Removed)
      out.push_back(make_pair(it->name, it->Id));
			//gdi.addItem(name, it->name, it->Id);
	}

	return out;
}

void oClub::buildTableCol(oEvent *oe, Table *table) {
   oe->oClubData->buildTableCol(table);
}

#define TB_CLUBS "clubs"
Table *oEvent::getClubsTB()//Table mode
{	
  if (tables.count("club") == 0) {
	  Table *table=new Table(this, 20, "Klubbar", TB_CLUBS);
    
    table->addColumn("Id", 70, true, true);
    table->addColumn("Ändrad", 70, false);

    table->addColumn("Namn", 200, false);
    oe->oClubData->buildTableCol(table);

    table->addColumn("Deltagare", 70, true);
    table->addColumn("Avgift", 70, true);
    table->addColumn("Betalat", 70, true);
    
    tables["club"] = table;
    table->addOwnership();
  }

  tables["club"]->update();
  return tables["club"];

}

void oEvent::generateClubTableData(Table &table, oClub *addClub)
{ 
  oe->setupClubInfoData();
  if (addClub) {
    addClub->addTableRow(table);
    return;
  }
  synchronizeList(oLClubId);
	oClubList::iterator it;	

  for (it=Clubs.begin(); it != Clubs.end(); ++it){		
    if(!it->isRemoved()){
      it->addTableRow(table);
		}
	}
}

void oClub::addTableRow(Table &table) const {
  table.addRow(getId(), pClass(this));

  bool dbClub = table.getInternalName() != TB_CLUBS;
  bool canEdit =  dbClub ? !oe->isClient() : true;

  pClub it = pClub(this);
  int row = 0;
  table.set(row++, *it, TID_ID, itos(getId()), false);
  table.set(row++, *it, TID_MODIFIED, getTimeStamp(), false);		

  table.set(row++, *it, TID_CLUB, getName(), canEdit);
  row = oe->oClubData->fillTableCol(oData, *this, table, canEdit);
  
  if (!dbClub) {
    table.set(row++, *it, TID_NUM, itos(tNumRunners), false);
    table.set(row++, *it, TID_FEE, oe->formatCurrency(tFee), false);
    table.set(row++, *it, TID_PAID, oe->formatCurrency(tPaid), false);
  }
}

bool oClub::inputData(int id, const string &input, 
                        int inputId, string &output, bool noUpdate)
{
  synchronize(false);
    
  if(id>1000) {
    return oe->oClubData->inputData(this, oData, id, input, inputId, output, noUpdate);
  }

  switch(id) {
    case TID_CLUB:
      setName(input);
      synchronize();
      output=getName();
      return true;
    break;
  }

  return false;
}

void oClub::fillInput(int id, vector< pair<string, size_t> > &out, size_t &selected)
{ 
  if(id>1000) {
    oe->oClubData->fillInput(oData, id, 0, out, selected);
    return;
  }
}

void oEvent::mergeClub(int clubIdPri, int clubIdSec)
{
  if (clubIdPri==clubIdSec)
    return;

  pClub pc = getClub(clubIdPri);
  if (!pc)
    return;

  // Update teams
  for (oTeamList::iterator it = Teams.begin(); it!=Teams.end(); ++it) {
    if (it->getClubId() == clubIdSec) {
      it->Club = pc;
      it->updateChanged();      
      it->synchronize(); 
    }
  }

  // Update runners
  for (oRunnerList::iterator it = Runners.begin(); it!=Runners.end(); ++it) {
    if (it->getClubId() == clubIdSec) {
      it->Club = pc;
      it->updateChanged();
      it->synchronize(); 
    }
  }
  oe->removeClub(clubIdSec);
}

void oEvent::getClubs(vector<pClub> &c, bool sort) {
  if (sort) {
    synchronizeList(oLClubId);
    Clubs.sort();
  }
  c.clear();
  c.reserve(Clubs.size());

  for (oClubList::iterator it = Clubs.begin(); it != Clubs.end(); ++it) {
    if (!it->isRemoved())
     c.push_back(&*it);
  }
}


void oEvent::viewClubMembers(gdioutput &gdi, int clubId)
{
  sortRunners(ClassStartTime);
  sortTeams(ClassStartTime, 0);
  
  gdi.fillDown();
  gdi.dropLine();
  int nr = 0;
  int nt = 0;
  // Update teams
  for (oTeamList::iterator it = Teams.begin(); it!=Teams.end(); ++it) {
    if (it->getClubId() == clubId) {
      if (nt==0)
        gdi.addString("", 1, "Lag(flera)");
      gdi.addStringUT(0, it->getName() + ", " + it->getClass() );
      nt++;
    }
  }

  gdi.dropLine();
  // Update runners
  for (oRunnerList::iterator it = Runners.begin(); it!=Runners.end(); ++it) {
    if (it->getClubId() == clubId) {
      if (nr==0)
        gdi.addString("", 1, "Löpare:");
      gdi.addStringUT(0, it->getName() + ", " + it->getClass() );
      nr++; 
    }
  }
}

void oClub::generateInvoice(gdioutput &gdi, int number, int &toPay, int &hasPaid) const
{
  string account = oe->getDI().getString("Account");
  string pdate = oe->getDI().getDate("PaymentDue");
  int pdateI = oe->getDI().getInt("PaymentDue");
  string organizer = oe->getDI().getString("Organizer");
  gdi.fillDown();

  if (account.empty())
    gdi.addString("", 0, "Varning: Inget kontonummer angivet (Se tävlingsinställningar).").setColor(colorRed);

  if (pdateI == 0)
    gdi.addString("", 0, "Varning: Inget sista betalningsdatum angivet (Se tävlingsinställningar).").setColor(colorRed);

  if (organizer.empty())
    gdi.addString("", 0, "Varning: Ingen organisatör/avsändare av fakturan angiven (Se tävlingsinställningar).").setColor(colorRed);

  vector<pRunner> runners;
  oe->getClubRunners(getId(), runners);

  toPay = 0;
  hasPaid = 0;

  if (runners.empty())
    return;

  int xs = 20;
  int ys = gdi.getCY();
  int lh = gdi.getLineHeight();

  gdi.addString("", ys, xs+350, boldHuge, "FAKTURA");
  if (number>0)
    gdi.addStringUT(ys+lh*3, xs+350, fontMedium, lang.tl("Faktura nr")+ ": " + itos(number));
  int yp = ys+lh;
  string ostreet = oe->getDI().getString("Street");
  string oaddress = oe->getDI().getString("Address");
  
  if (!organizer.empty())
    gdi.addStringUT(yp, xs, fontMedium, organizer), yp+=lh;
  if (!ostreet.empty())
    gdi.addStringUT(yp, xs, fontMedium, ostreet), yp+=lh;
  if (!oaddress.empty())
    gdi.addStringUT(yp, xs, fontMedium, oaddress), yp+=lh;

  yp+=lh;

  gdi.addStringUT(yp, xs, fontLarge, oe->getName());
  gdi.addStringUT(yp+lh*2, xs, fontMedium, oe->getDate());

  string co =  getDCI().getString("CareOf");
  string address =  getDCI().getString("Street");
  string city =  getDCI().getString("ZIP") + " " + getDCI().getString("City");
  
  int ayp = ys + 122;  
  gdi.addStringUT(ayp, xs+350, fontMedium, getName());
  ayp+=lh;

  if (!co.empty())
    gdi.addStringUT(ayp, xs+350, fontMedium, co), ayp+=lh;

  if (!address.empty())
    gdi.addStringUT(ayp, xs+350, fontMedium, address), ayp+=lh;

  if (!city.empty())
    gdi.addStringUT(ayp, xs+350, fontMedium, city), ayp+=lh;

  yp = ayp+30;


  int total_fee_amount = 0;
  int total_rent_amount = 0;
  int total_paid_amount = 0;

  gdi.addString("", yp, xs, fontSmall, "Deltagare");
  gdi.addString("", yp, xs+300, fontSmall, "Klass");
  
  gdi.addString("", yp, xs+430, fontSmall, "Resultat");
  
  gdi.addString("", yp, xs+580, fontSmall|textRight, "Avgift");
  gdi.addString("", yp, xs+630, fontSmall|textRight, "Brickhyra");
  gdi.addString("", yp, xs+680, fontSmall|textRight, "Betalat");
  yp += lh;

  for (size_t k=0;k<runners.size(); k++) {
    gdi.addStringUT(yp, xs, fontMedium, runners[k]->getName());
    gdi.addStringUT(yp, xs+300, fontMedium, runners[k]->getClass());
    string ts;
    
    if (runners[k]->getStatus()==StatusUnknown)
      ts = "-";      
    else if (runners[k]->getStatus()==StatusOK) {
      ts =  runners[k]->getPrintPlaceS()+ " (" + runners[k]->getRunningTimeS() + ")";
    } 
    else
      ts =  runners[k]->getStatusS();

    gdi.addStringUT(yp, xs+430, fontMedium, ts);

    int fee = runners[k]->getDCI().getInt("Fee");
    int card = runners[k]->getDCI().getInt("CardFee");
    int paid = runners[k]->getDCI().getInt("Paid");

    if (runners[k]->getClassRef() && runners[k]->getClassRef()->getClassStatus() == oClass::InvalidRefund) {
      fee = 0;
      card = 0;
    }

    gdi.addStringUT(yp, xs+580, fontMedium|textRight, oe->formatCurrency(fee));
    gdi.addStringUT(yp, xs+630, fontMedium|textRight, oe->formatCurrency(card));
    gdi.addStringUT(yp, xs+680, fontMedium|textRight, oe->formatCurrency(paid));
    total_fee_amount += fee;
    total_rent_amount += card;
    total_paid_amount += paid;
    yp += lh;
  }
  yp += lh;
  gdi.addStringUT(yp, xs+580, boldText|textRight, oe->formatCurrency(total_fee_amount));
  gdi.addStringUT(yp, xs+630, boldText|textRight, oe->formatCurrency(total_rent_amount));
  gdi.addStringUT(yp, xs+680, boldText|textRight, oe->formatCurrency(total_paid_amount));

  yp+=lh*2;
  toPay = total_fee_amount+total_rent_amount-total_paid_amount;
  hasPaid = total_paid_amount;

  gdi.addString("", yp, xs+600, boldText, "Att betala: X#" + oe->formatCurrency(toPay));
  gdi.updatePos(700,0,0,0);

  yp+=lh*2;

  gdi.addStringUT(yp, xs,boldText, lang.tl("Vänligen betala senast") + 
            " " + pdate + " " + lang.tl("till") + " " + account + ".");
  gdi.dropLine(2);  
  //gdi.addStringUT(gdi.getCY()-1, 1, pageNewPage, blank, 0, 0);
  gdi.addStringUT(gdi.getCY()-1, xs, pageNewPage, "", 0, 0);
}

void oEvent::getClubRunners(int clubId, vector<pRunner> &runners) const
{
  oRunnerList::const_iterator rit;
  runners.clear();

	for (rit=Runners.begin(); rit != Runners.end(); ++rit) {
    if (!rit->skip() && rit->getClubId() == clubId)
      runners.push_back(pRunner(&*rit));
	}  
}

void oEvent::printInvoices(gdioutput &gdi, InvoicePrintType type, 
                           const string &basePath, bool onlySummary) {
  Clubs.sort();
  oClubList::const_iterator it;
  oe->calculateResults(RTClassResult);
  oe->sortRunners(ClassStartTime);
  int pay, paid;
  vector<int> fees, vpaid;
  set<int> clubId;
  fees.reserve(Clubs.size());
  int k=0;

  bool toFile = type > 10;

  string path = basePath;
  if (basePath.size() > 0 && *basePath.rbegin() != '\\' && *basePath.rbegin() != '/')
    path.push_back('\\');

  if (toFile) {
    ofstream fout;
    
    if (type == IPTElectronincHTML) 
      fout.open((path + "invoices.txt").c_str());


	  for (it=Clubs.begin(); it != Clubs.end(); ++it) {
      if (!it->isRemoved()) {
        gdi.clearPage(false);
        int nr = 1000+(k++);

        string filename = "invoice" + itos(nr*197) + ".html";
        string email = it->getDCI().getString("EMail");
        bool hasEmail = !(email.empty() || email.find_first_of('@') == email.npos);

        if (type == IPTElectronincHTML) {
          if (!hasEmail)
            continue;

        }
        
        it->generateInvoice(gdi, nr, pay, paid);

        if (type == IPTElectronincHTML && pay > 0) {


          fout << it->getId() << ";" << it->getName() << ";" << 
                nr << ";" << filename << ";" << email << ";"
                << formatCurrency(pay)  <<endl;
        }
        

        gdi.writeHTML(path + filename, "Faktura");
        clubId.insert(it->getId());
        fees.push_back(pay);
        vpaid.push_back(paid);
      }
      gdi.clearPage(true); 
	  }
  }
  else {
	  for (it=Clubs.begin(); it != Clubs.end(); ++it) {
      if (!it->isRemoved()) {

        string email = it->getDCI().getString("EMail");
        bool hasEmail = !(email.empty() || email.find_first_of('@') == email.npos);
        if (type == IPTNoMailPrint && hasEmail)
          continue;

        it->generateInvoice(gdi, 1000+(k++), pay, paid);
        clubId.insert(it->getId());
        fees.push_back(pay);
        vpaid.push_back(paid);
      }
	  }
  }

  if (onlySummary)
    gdi.clearPage(true);
  k=0;
  gdi.dropLine(1);
  gdi.addString("", boldLarge, "Sammanställning, ekonomi");
  int yp = gdi.getCY() + 10;

  gdi.addString("", yp, 50, boldText, "Faktura nr");
  gdi.addString("", yp, 240, boldText|textRight, "KlubbId"); 
  
  gdi.addString("", yp, 250, boldText, "Klubb"); 
  gdi.addString("", yp, 550, boldText|textRight, "Faktura"); 
  gdi.addString("", yp, 620, boldText|textRight, "Kontant"); 

  yp+=gdi.getLineHeight()+3;

  int sum = 0, psum = 0;
  for (it=Clubs.begin(); it != Clubs.end(); ++it) {
    if (!it->isRemoved() && clubId.count(it->getId()) > 0) {
      
      gdi.addStringUT(yp, 50, fontMedium, itos(1000+k));
      gdi.addStringUT(yp, 240, textRight|fontMedium, itos(it->getId())); 
      gdi.addStringUT(yp, 250, fontMedium, it->getName()); 
      gdi.addStringUT(yp, 550, fontMedium|textRight, oe->formatCurrency(fees[k])); 
      gdi.addStringUT(yp, 620, fontMedium|textRight, oe->formatCurrency(vpaid[k])); 
      
      sum+=fees[k];
      psum+=vpaid[k];
      k++;
      yp+=gdi.getLineHeight();
    }
	}

  gdi.addStringUT(yp, 550, boldText|textRight, lang.tl("Totalt faktureras: ") + oe->formatCurrency(sum));
  gdi.addStringUT(yp+gdi.getLineHeight(), 550, boldText|textRight, lang.tl("Totalt kontant: ") + oe->formatCurrency(psum));
}

void oClub::updateFromDB()
{
  pClub pc = oe->runnerDB->getClub(Id);

  if (pc && !pc->sameClub(*this))
    pc = 0;

  if (pc==0)
    pc = oe->runnerDB->getClub(name);

  if (pc) {
    memcpy(oData, pc->oData, sizeof (oData));
    updateChanged();
  }
}

void oEvent::updateClubsFromDB()
{
  oClubList::iterator it;

  for (it=Clubs.begin();it!=Clubs.end();++it) {
    it->updateFromDB();
    it->synchronize();   
  }
}

bool oClub::sameClub(const oClub &c)
{
  return _stricmp(name.c_str(), c.name.c_str())==0;
}

void oClub::remove() 
{
  if (oe)
    oe->removeClub(Id);
}

bool oClub::canRemove() const 
{
  return !oe->isClubUsed(Id);
}

void oEvent::setupClubInfoData() {
  if (tClubDataRevision == dataRevision || Clubs.empty())
    return;

  inthashmap fee(Clubs.size());
  inthashmap paid(Clubs.size());
  inthashmap runners(Clubs.size());

  for (oRunnerList::iterator it = Runners.begin(); it != Runners.end(); ++it) {
    oRunner &r = *it;
    if (r.Club) {
      int id = r.Club->Id;
      ++runners[id];
      oDataConstInterface di = r.getDCI();
      bool skip = r.Class && r.Class->getClassStatus() == oClass::InvalidRefund;

      if (!skip)
        fee[id] += di.getInt("Fee") + di.getInt("CardFee");
      paid[id] += di.getInt("Paid");
    }
  }

  for (oClubList::iterator it = Clubs.begin(); it != Clubs.end(); ++it) {
    int id = it->Id;
    it->tFee = fee[id];
    it->tPaid = paid[id];
    it->tNumRunners = runners[id];
  } 

  tClubDataRevision = dataRevision;
}


bool oClub::isVacant() const {
  return getId() == oe->getVacantClubIfExist();
}
