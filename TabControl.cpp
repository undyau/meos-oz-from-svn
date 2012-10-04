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
    Stigbergsv�gen 7, SE-75242 UPPSALA, Sweden
    
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
#include "meos_util.h"
#include "gdifonts.h"

#include <cassert>

#include "TabControl.h"

TabControl::TabControl(oEvent *poe):TabBase(poe)
{
  tableMode = false;
  controlId = 0;
 }

TabControl::~TabControl(void)
{ 
}

void TabControl::selectControl(gdioutput &gdi,  pControl pc)
{
	if (pc) {
		pc->synchronize();
		
    if (pc->getStatus() == oControl::StatusStart ||
          pc->getStatus() == oControl::StatusFinish) {
      gdi.selectItemByData("Controls", pc->getId());
      gdi.selectItemByData("Status", oControl::StatusOK);
		  gdi.setText("ControlID", "-", true);

		  gdi.setText("Code", "");
		  gdi.setText("Name", pc->getName());
      gdi.setText("TimeAdjust", "00:00");
      gdi.setText("MinTime", "-");
      gdi.setText("Point", "");
      controlId = pc->getId();
  	
		  gdi.enableInput("Remove");
		  gdi.enableInput("Save");
      gdi.enableEditControls(false);
      gdi.enableInput("Name");
    }
    else {
      gdi.selectItemByData("Controls", pc->getId());
		  gdi.selectItemByData("Status", pc->getStatus());
      oe->setupMissedControlTime();
      const int numVisit = pc->getNumVisitors();
      string info;
      if (numVisit > 0) {
         info = "Antal bes�kare X, genomsnittlig bomtid Y, st�rsta bomtid Z#" +
          itos(pc->getNumVisitors()) + "#" + getTimeMS(pc->getMissedTimeTotal() / numVisit) + 
          "#" + getTimeMS(pc->getMissedTimeMax());
      }
      gdi.setText("ControlID", itos(pc->getId()), true);

      gdi.setText("Info", lang.tl(info), true);
		  gdi.setText("Code", pc->codeNumbers());
		  gdi.setText("Name", pc->getName());
      gdi.setText("TimeAdjust", pc->getTimeAdjustS());
      gdi.setText("MinTime", pc->getMinTimeS());
      gdi.setText("Point", pc->getRogainingPointsS());
      
      controlId = pc->getId();
  	
		  gdi.enableInput("Remove");
		  gdi.enableInput("Save");
      gdi.enableEditControls(true);

      if (pc->getStatus() == oControl::StatusRogaining) {
        gdi.disableInput("MinTime");
      }
      else 
        gdi.disableInput("Point");
    }
	}
	else{
    gdi.selectItemByData("Controls", -1);
    gdi.selectItemByData("Status", oControl::StatusOK);
		gdi.setText("Code", "");
		gdi.setText("Name", "");
    controlId = 0;
	
		gdi.setText("ControlID", "-", true);
    gdi.setText("TimeAdjust", "00:00");
    gdi.setText("Point", "");
      
		gdi.disableInput("Remove");
		gdi.disableInput("Save");
    gdi.enableEditControls(false);
	}
}

int ControlsCB(gdioutput *gdi, int type, void *data)
{
  TabControl &tc = dynamic_cast<TabControl &>(*gdi->getTabs().get(TControlTab));
  return tc.controlCB(*gdi, type, data);
}

void TabControl::save(gdioutput &gdi)
{
	if (controlId==0)
    return;

  DWORD pcid = controlId;

  pControl pc;
  pc = oe->getControl(pcid, false);

  if(!pc) 
    throw std::exception("Internal error");
  if (pc->getStatus() != oControl::StatusFinish && pc->getStatus() != oControl::StatusStart) {
	  if(!pc->setNumbers(gdi.getText("Code")))
		  gdi.alert("Kodsiffran m�ste vara ett heltal. Flera kodsiffror m�ste separeras med komma.");
  					
	  ListBoxInfo lbi;
	  gdi.getSelectedItem("Status", &lbi);
    pc->setStatus(oControl::ControlStatus(lbi.data));
    pc->setTimeAdjust(gdi.getText("TimeAdjust"));
    if (pc->getStatus() != oControl::StatusRogaining) {
      pc->setMinTime(gdi.getText("MinTime"));
      pc->setRogainingPoints(0);
    }
    else {
      pc->setMinTime(0);
      pc->setRogainingPoints(gdi.getTextNo("Point"));
    }
  }

	pc->setName(gdi.getText("Name"));
  
	pc->synchronize();
	oe->fillControls(gdi, "Controls");

  oe->reEvaluateAll(true);

	selectControl(gdi, pc);
}

int TabControl::controlCB(gdioutput &gdi, int type, void *data)
{
	if (type==GUI_BUTTON) {
		ButtonInfo bi=*(ButtonInfo *)data;

		if (bi.id=="Save")
      save(gdi);
		else if (bi.id=="Add") {
      bool rogaining = false;
      if (controlId>0) {
        save(gdi);
        pControl pc = oe->getControl(controlId, false);
        rogaining = pc && pc->getStatus() == oControl::StatusRogaining;
      }
      pControl pc = oe->addControl(0,oe->getNextControlNumber(), ""); 
			
      if (rogaining)
        pc->setStatus(oControl::StatusRogaining);
      oe->fillControls(gdi, "Controls");
			selectControl(gdi, pc);
		}
		else if (bi.id=="Remove") {

			DWORD cid = controlId;
			if (cid==0) {
				gdi.alert("Ingen kontroll vald vald.");
				return 0;
			}

			if(oe->isControlUsed(cid))
				gdi.alert("Kontrollen anv�nds och kan inte tas bort.");
			else
				oe->removeControl(cid);

			oe->fillControls(gdi, "Controls");
			selectControl(gdi, 0);
		}
    else if (bi.id=="SwitchMode") {
      if (!tableMode)
        save(gdi);
      tableMode = !tableMode;
      loadPage(gdi);
    }
	}
	else if(type==GUI_LISTBOX){
		ListBoxInfo bi=*(ListBoxInfo *)data;

		if (bi.id=="Controls") {
      if (gdi.isInputChanged(""))
        save(gdi);

			pControl pc=oe->getControl(bi.data, false);
      if(!pc)
        throw std::exception("Internal error");

			selectControl(gdi, pc);
		}
    else if (bi.id == "Status" ) {
      if (bi.data == oControl::StatusRogaining) {
        gdi.disableInput("MinTime");
        gdi.enableInput("Point");
      }
      else {
        gdi.enableInput("MinTime");
        gdi.disableInput("Point");
      }
    }
	}
  else if(type==GUI_CLEAR) {
    if (controlId>0)
      save(gdi);

    return true;
  }
	return 0;
}


bool TabControl::loadPage(gdioutput &gdi)
{
  oe->checkDB();
	gdi.selectTab(tabId);
	gdi.clearPage(false);
  int xp=gdi.getCX();

  const int button_w=gdi.scaleLength(90);
  string switchMode;
  switchMode=tableMode ? "Formul�rl�ge" : "Tabell�ge";
  gdi.addButton(2, 2, button_w, "SwitchMode", switchMode, 
    ControlsCB, "V�lj vy", false, false).fixedCorner();

  if(tableMode) {
    Table *tbl=oe->getControlTB();
    gdi.addTable(tbl, xp, 30);
    return true;
  }

	gdi.fillDown();
	gdi.addString("", boldLarge, "Kontroller");

	gdi.pushY();
  gdi.addListBox("Controls", 250, 530, ControlsCB).isEdit(false).ignore(true);
	gdi.setTabStops("Controls", 40, 160);
	oe->fillControls(gdi, "Controls");

	gdi.newColumn();
	gdi.fillDown();
	gdi.popY();

  gdi.pushX();
	gdi.fillRight();
  gdi.addString("", 1, "Kontrollens ID-nummer:");
	gdi.fillDown();
  gdi.addString("ControlID", 1, "#-").setColor(colorGreen);
  gdi.popX();

  gdi.fillRight();
	gdi.addInput("Name", "", 16, 0, "Kontrollnamn:");

  gdi.addSelection("Status", 100, 100, ControlsCB, "Status:", "Ange om kontrollen fungerar och hur den ska anv�ndas.");
  oe->fillControlStatus(gdi, "Status");
	
	gdi.addInput("TimeAdjust", "", 6, 0, "Tidsjustering:");
	gdi.fillDown();
  gdi.addInput("MinTime", "", 6, 0, "Minsta str�cktid:");
	
  gdi.popX();
  gdi.dropLine(0.5);
	gdi.addString("", 10, "help:9373");
  
  gdi.fillRight();
  
  gdi.dropLine(0.5);
  gdi.addInput("Code", "", 16, 0, "St�mpelkod(er):");
  gdi.addInput("Point", "", 6, 0, "Rogaining-po�ng:");
  gdi.popX();
	gdi.dropLine(3.5);

  gdi.fillRight();	
	gdi.addButton("Save", "Spara", ControlsCB, "help:save");	
  gdi.disableInput("Save");
	gdi.addButton("Remove", "Radera", ControlsCB);
	gdi.disableInput("Remove");
	gdi.addButton("Add", "Ny kontroll", ControlsCB);

  gdi.dropLine(2.5);
	gdi.popX();
  
  gdi.fillDown();

  gdi.addString("Info", 0, "").setColor(colorGreen);

  gdi.dropLine(1.5);
	gdi.addString("", 10, "help:89064");
  
  selectControl(gdi, oe->getControl(controlId, false));

  gdi.setOnClearCb(ControlsCB);

  oe->setupMissedControlTime();

  gdi.refresh();
  return true;	
}
