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
    Stigbergsvägen 11, SE-75242 UPPSALA, Sweden
    
************************************************************************/

#include "stdafx.h"

#include "resource.h"

#include <commctrl.h>
#include <commdlg.h> 

#include "oEvent.h"
#include "xmlparser.h"
#include "gdioutput.h"
#include "csvparser.h"
#include "SportIdent.h"
#include <cassert>
#include "meos_util.h"
#include "Table.h"

#include "TabClub.h"

TabClub::TabClub(oEvent *poe):TabBase(poe)
{
  baseFee = 0;
  highFee = 0;
}

TabClub::~TabClub(void)
{ 
}

void TabClub::selectClub(gdioutput &gdi,  pClub pc)
{
	if (pc) {
		pc->synchronize();
    ClubId = pc->getId();
  }
	else{
    ClubId = 0;
	}
}

int ClubsCB(gdioutput *gdi, int type, void *data)
{
  TabClub &tc = dynamic_cast<TabClub &>(*gdi->getTabs().get(TClubTab));
  return tc.clubCB(*gdi, type, data);
}

int TabClub::clubCB(gdioutput &gdi, int type, void *data)
{
	if(type==GUI_BUTTON) {
		ButtonInfo bi=*(ButtonInfo *)data;

		if (bi.id=="Save") {
		}
		else if (bi.id=="Invoice") {
      ListBoxInfo lbi;
      gdi.getSelectedItem("Clubs", &lbi);
      pClub pc=oe->getClub(lbi.data);
      if (pc) {
        gdi.clearPage(true);
        oe->calculateResults(false);
        oe->sortRunners(ClassStartTime);
        int pay, paid;
        pc->generateInvoice(gdi, pc->getId(), pay, paid);
        gdi.addButton(gdi.getWidth()+20, 15, gdi.scaleLength(120), 
                      "Cancel", "Återgå", ClubsCB, "", true, false);
        gdi.addButton(gdi.getWidth()+20, 45,  gdi.scaleLength(120), 
                      "Print", "Skriv ut...", ClubsCB, 
                      "Skriv ut fakturan", true, false);
        gdi.refresh();
      }
		}
    else if (bi.id=="AllInvoice") {
      gdi.clearPage(false);
      oe->printInvoices(gdi, false);
      gdi.refresh();
	  gdi.print(oe, 0, false, false);
    }
    else if (bi.id=="UpdateAll") {
      oe->updateClubsFromDB();
      gdi.getTable().update();
      gdi.refresh();
    }
    else if (bi.id=="UpdateAllRunners") {
      oe->updateClubsFromDB();
      oe->updateRunnersFromDB();
      gdi.getTable().update();
      gdi.refresh();
    }
    else if (bi.id=="Update") {
      ListBoxInfo lbi;
      gdi.getSelectedItem("Clubs", &lbi);
      pClub pc=oe->getClub(lbi.data);
      if (pc) {
        pc->updateFromDB();
        pc->synchronize();
        gdi.getTable().update();
        gdi.refresh();
      }
		}    
    else if (bi.id=="Summary") {
      gdi.clearPage(false);
      oe->printInvoices(gdi, true);
      gdi.addButton(gdi.getWidth()+20, 15,  gdi.scaleLength(120), "Cancel", 
                    "Återgå", ClubsCB, "", true, false);
      gdi.addButton(gdi.getWidth()+20, 45,  gdi.scaleLength(120), "Print",
                    "Skriv ut...", ClubsCB, "Skriv ut fakturan", true, false);

      gdi.refresh();
		}
    else if (bi.id=="Merge") {
      ListBoxInfo lbi;
      gdi.getSelectedItem("Clubs", &lbi);
      ClubId = lbi.data;
      pClub pc=oe->getClub(lbi.data);
      if (pc) {
        gdi.clearPage(false);
        gdi.addString("", boldText, "Slå ihop klubb");
        
        char bf[256];
        sprintf_s(bf, lang.tl("help:12352").c_str(), pc->getName().c_str(), pc->getId());

        gdi.addStringUT(10, bf);

        gdi.addSelection("NewClub", 200, 300, 0, "Ny klubb:"); 
        oe->fillClubs(gdi, "NewClub");
        gdi.selectItemByData("NewClub", pc->getId());
        gdi.removeSelected("NewClub");

        gdi.pushX();
        gdi.fillRight();
        gdi.addButton("DoMerge", "Slå ihop", ClubsCB);
        gdi.addButton("Cancel", "Avbryt", ClubsCB);
        gdi.fillDown();
        gdi.popX();
        gdi.dropLine(2);
        gdi.addStringUT(boldText, lang.tl("Klubb att ta bort: ") + pc->getName());
        oe->viewClubMembers(gdi, pc->getId());

        gdi.refresh();
      }
		}    
    else if (bi.id=="DoMerge") {
      pClub pc1 = oe->getClub(ClubId);
      ListBoxInfo lbi;
      gdi.getSelectedItem("NewClub", &lbi);
      pClub pc2 = oe->getClub(lbi.data);

      if (pc1==pc2)
        throw std::exception("En klubb kan inte slås ihop med sig själv.");

      if (pc1 && pc2)
        oe->mergeClub(pc2->getId(), pc1->getId());
      loadPage(gdi);
    }
 		else if (bi.id=="Fees") {
      gdi.clearPage(true);

      gdi.addString("", boldLarge, "Tilldela avgifter");
      
      gdi.dropLine();
      gdi.pushX();
      gdi.fillRight();

      gdi.addSelection("ClassType", 150, 300, 0, "Klasstyp");
      oe->fillClassTypes(gdi, "ClassType");
      gdi.addItem("ClassType", lang.tl("Alla typer"), -5);
      gdi.selectItemByData("ClassType", -5);

      gdi.addInput("BaseFee", oe->formatCurrency(baseFee), 8, 0, "Ordinarie avgift");
      gdi.addInput("HighFee", oe->formatCurrency(highFee), 8, 0, "Förhöjd avgift");

      gdi.popX();
      gdi.fillDown();
      gdi.dropLine(3);

      gdi.addInput("LastDate", entryLimit, 16, 0, "Sista datum för ordinarie anmälan (ÅÅÅÅ-MM-DD):");
    
      gdi.addCheckbox("ResetFees", "Nollställ avgifter", ClubsCB, false);
      
      gdi.pushX();
      gdi.fillRight();
      gdi.addButton("DoFees", "Tilldela", ClubsCB);
      gdi.addButton("Cancel", "Återgå", ClubsCB);
      gdi.popX();
      gdi.fillDown();
      gdi.dropLine(2);
      gdi.refresh();
		}
    else if (bi.id == "ResetFees") {
      bool reset = gdi.isChecked("ResetFees");
      gdi.setInputStatus("BaseFee", !reset);
      gdi.setInputStatus("HighFee", !reset);
      gdi.setInputStatus("LastDate", !reset);
    }
  	else if (bi.id=="DoFees") {
      baseFee = oe->interpretCurrency(gdi.getText("BaseFee"));
      highFee = oe->interpretCurrency(gdi.getText("HighFee"));
      entryLimit = gdi.getText("LastDate");
      ListBoxInfo lbi;
      gdi.getSelectedItem("ClassType", &lbi);
      //string type = gdi.getText("ClassType");
      string typeS;
      if (lbi.data == -5)
        typeS = "*";
      else
        typeS = lbi.text;

      bool reset = gdi.isChecked("ResetFees");

      gdi.dropLine();
      if (!reset)
        oe->generateFees(typeS, entryLimit, baseFee, highFee);
      else
        oe->generateFees(typeS, "", 0, 0);
      
      gdi.addStringUT(1, lang.tl("Utfört: ") + lbi.text).setColor(colorGreen);
      gdi.refresh();    
		}
		else if (bi.id=="Cancel") {
      loadPage(gdi);
		}
		else if (bi.id=="Print")	{
			gdi.print(oe);			
		}

	}
	else if(type==GUI_LISTBOX){
		ListBoxInfo bi=*(ListBoxInfo *)data;

		if(bi.id=="Clubs"){			
			pClub pc=oe->getClub(bi.data);
      if(!pc)
        throw std::exception("Internal error");

			selectClub(gdi, pc);
		}
	}

	return 0;
}


bool TabClub::loadPage(gdioutput &gdi)
{
  oe->checkDB();
	gdi.selectTab(tabId);

  if (baseFee == 0) {
    if (oe->getDCI().getInt("OrdinaryEntry") > 0)
      entryLimit = oe->getDCI().getDate("OrdinaryEntry");
    baseFee = oe->getDCI().getInt("EntryFee");
    highFee = (int)(baseFee * 0.01*double(100 + atof(oe->getDCI().getString("LateEntryFactor").c_str())));
  }
	gdi.clearPage(false);
	gdi.fillDown();
	gdi.addString("", boldLarge, "Klubbar");
  gdi.dropLine(0.5);
  gdi.pushX();
  gdi.fillRight();
  gdi.addSelection("Clubs", 200, 300, ClubsCB);
  oe->fillClubs(gdi, "Clubs");
  gdi.selectItemByData("Clubs", ClubId);
  gdi.addButton("Merge", "Ta bort / slå ihop...", ClubsCB);
  gdi.addButton("Invoice", "Faktura", ClubsCB);
  gdi.addButton("Update", "Uppdatera", ClubsCB, "Uppdatera klubbens uppgifter med data från löpardatabasen/distriktsregistret");

  gdi.popX();
  gdi.dropLine(2);

  gdi.addButton("Fees", "Avgifter...", ClubsCB);
  gdi.addButton("UpdateAll", "Uppdatera alla klubbar", ClubsCB, "Uppdatera klubbarnas uppgifter med data från löpardatabasen/distriktsregistret");
  gdi.addButton("UpdateAllRunners", "Uppdatera klubbar && löpare", ClubsCB, "Uppdatera klubbarnas och löparnas uppgifter med data från löpardatabasen/distriktsregistret");
  
  gdi.addButton("AllInvoice", "Alla fakturor...", ClubsCB);
  gdi.addButton("Summary", "Sammanställning", ClubsCB);
  gdi.popX();
  gdi.fillDown();
  gdi.dropLine(2);
  gdi.addString("", 10, "help:29758");
  gdi.dropLine(1);
  Table *tbl=oe->getClubsTB();
  gdi.addTable(tbl, gdi.getCX(), gdi.getCY());
  gdi.refresh();
  return true;	
}
