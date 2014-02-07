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

#include "stdafx.h"

#include "resource.h"

#include <commctrl.h>
#include <commdlg.h> 

#include "oEvent.h"
#include "xmlparser.h"
#include "gdioutput.h"
#include "csvparser.h"
#include "SportIdent.h"
#include "classconfiginfo.h"

#include "TabAuto.h"
#include "meos_util.h"
#include <shellapi.h>

#include "gdiconstants.h"

static TabAuto *tabAuto = 0;
static long currentRevision = 0;

extern HWND hWndMain;
extern HWND hWndWorkspace;
extern SportIdent *gSI;

TabAuto::TabAuto(oEvent *poe):TabBase(poe)
{
  synchronize=false;
  synchronizePunches=false;
}

TabAuto::~TabAuto(void)
{
	list<AutoMachine *>::iterator it;
	for (it=Machines.begin(); it!=Machines.end(); ++it) {		
		delete *it;
		*it=0;
	}
	tabAuto=0;
}

void tabAutoTimer(gdioutput &gdi)
{
	if(tabAuto)
		tabAuto->timerCallback(gdi);
}

void tabAutoKillMachines()
{
  if(tabAuto)
    tabAuto->killMachines();
}

void tabAutoRegister(TabAuto *ta)
{
  tabAuto=ta;
}

void tabAutoAddMachinge(const AutoMachine &am)
{
  if(tabAuto) {
    tabAuto->addMachine(am);
  }
}

void tabAutoSync(const vector<gdioutput *> &gdi, pEvent oe)
{
	DWORD d=0;
  bool doSync = false;
  bool doSyncPunch = false;
  DWORD SyncPunches=0;
	
  for (size_t k = 0; k<gdi.size(); k++) {
    if (gdi[k] && gdi[k]->getData("DataSync", d)) {
      doSync = true;
    }    
    if (gdi[k] && gdi[k]->getData("PunchSync", SyncPunches)) {
      doSyncPunch = true;
    } 
  }

  if (doSync || (tabAuto && tabAuto->synchronize)) {					
	
    if (tabAuto && tabAuto->synchronizePunches)
      doSyncPunch = true;

    if ( oe->autoSynchronizeLists(doSyncPunch) || oe->getRevision() != currentRevision) {
      if(doSync) {
        for (size_t k = 0; k<gdi.size(); k++) {
          if (gdi[k]) 
            gdi[k]->makeEvent("DataUpdate", "autosync", 0, 0, false);
        }
      }

      if(tabAuto)
        tabAuto->syncCallback(*gdi[0]);
    }			
	}
  currentRevision = oe->getRevision();
}

void tabForceSync(gdioutput &gdi, pEvent oe)
{
  if(tabAuto)
    tabAuto->syncCallback(gdi);
}

int AutomaticCB(gdioutput *gdi, int type, void *data)
{
	if (!tabAuto) 
		throw std::exception("tabAuto undefined.");

	switch(type){
		case GUI_BUTTON: {
			//Make a copy
			ButtonInfo bu=*static_cast<ButtonInfo *>(data);
			return tabAuto->processButton(*gdi, bu);
						 }
		case GUI_LISTBOX:{	
			ListBoxInfo lbi=*static_cast<ListBoxInfo *>(data);
			return tabAuto->processListBox(*gdi, lbi);
						 }
	}
	return 0;
}

void TabAuto::syncCallback(gdioutput &gdi)
{	
  list<AutoMachine *>::iterator it;
	for (it=Machines.begin(); it!=Machines.end(); ++it) {
		AutoMachine *am=*it;
		if(am && am->synchronize)
			am->process(gdi, oe, SyncDataUp);			
	}
}

void TabAuto::updateSyncInfo()
{	
  list<AutoMachine *>::iterator it;
  synchronize=false;
  synchronizePunches=false;

  for (it=Machines.begin(); it!=Machines.end(); ++it) {
		AutoMachine *am=*it;
    if(am){
      am->synchronize= am->synchronize || am->synchronizePunches;
      synchronize=synchronize || am->synchronize;
      synchronizePunches=synchronizePunches || am->synchronizePunches;
    }
	}
}

void TabAuto::timerCallback(gdioutput &gdi)
{
	DWORD tc=GetTickCount();

	list<AutoMachine *>::iterator it;
  bool reload=false;

	for (it=Machines.begin(); it!=Machines.end(); ++it) {
		AutoMachine *am=*it;
		if (am && am->interval && tc >= am->timeout) {
			am->process(gdi, oe, SyncTimer);
			setTimer(am);
      reload=true;
		}
	}

  DWORD d=0;
  if(reload && !editMode && gdi.getData("AutoPage", d) && d)
    loadPage(gdi);
	/*if (printResultIntervalSec>0) {

		if (tc>printResultTimeOut) {

			gdioutput gdi;
			//gdi.Init(hWndWorkspace, hWndMain, 0);	
			gdi.ClearPage();
			
			oe->GenerateResultlistPrint(gdi);

			HDC hDC=GetDC(hWndWorkspace);
			RECT rc;
			rc.right=1000;
			rc.left=0;
			rc.bottom=1000;
			rc.top=0;
			gdi.Draw(hDC, rc);

			ReleaseDC(hWndWorkspace, hDC);

			setTimer(printResultTimeOut, printResultIntervalSec);
		}
	}*/
}

void TabAuto::setTimer(AutoMachine *am)
{
	DWORD tc=GetTickCount();

  if(am->interval>0) {
	  DWORD to=am->interval*1000+tc;

	  if(to<tc) { //Warp-around. No reasonable way to handle it
		  to=DWORD(0);
		  am->interval=0;
	  }

	  am->timeout=to;
  }
}

int TabAuto::processButton(gdioutput &gdi, const ButtonInfo &bu)
{
  
  if (bu.id=="GenerateCMP") {
#ifndef MEOSDB  
    int nClass=gdi.getTextNo("nClass");
    int nRunner=gdi.getTextNo("nRunner");

    if(nRunner>0 && 
      gdi.ask("Vill du dumpa aktuellt tävling och skapa en testtävling?")) {
      oe->generateTestCompetition(nClass, nRunner, 
        gdi.isChecked("UseRelay"));
    }
#endif    
  }
	if (bu.id=="Result") {
#ifndef MEOSDB  
		gdi.restore();
    editMode=true;
    gdi.dropLine(1.5);
		gdi.addString("", 1, "Resultatutskrift / export");
		gdi.pushX();
		gdi.fillRight();

	  PrintResultMachine *prm=dynamic_cast<PrintResultMachine*>((AutoMachine*)bu.getExtra());
    bool createNew=(prm==0);

		if (!prm) {
			prm=new PrintResultMachine(0);			
      oe->getAllClasses(prm->classesToPrint);
			Machines.push_back(prm);
		}
    gdi.dropLine(1.5);
    gdi.popX();
    gdi.addCheckbox("DoPrint", "Skriv ut", AutomaticCB, prm->doPrint);
    gdi.dropLine(-0.5);
    gdi.addButton("PrinterSetup", "Skrivare...", AutomaticCB, "Välj skrivare...").setExtra(prm);
    
    gdi.dropLine(4);
    gdi.popX();
    gdi.addCheckbox("DoExport", "Exportera", AutomaticCB, prm->doExport);
    gdi.dropLine(-1);
    int cx = gdi.getCX();
    gdi.addInput("ExportFile", prm->exportFile, 32, 0, "Fil att exportera till:");
    gdi.dropLine(0.7);
    gdi.addButton("BrowseFile", "Bläddra...", AutomaticCB);
    gdi.setCX(cx);
    gdi.dropLine(2.3);                     
    gdi.addCheckbox("StructuredExport", "Strukturerat exportformat", 0, prm->structuredExport);
    gdi.dropLine(1.2);
    gdi.setCX(cx);
    gdi.addInput("ExportScript", prm->exportScript, 32, 0, "Skript att köra efter export:");
    gdi.dropLine(0.7);
    gdi.addButton("BrowseScript", "Bläddra...", AutomaticCB);
    gdi.dropLine(3);
    gdi.popX();

    gdi.setInputStatus("ExportFile", prm->doExport);
    gdi.setInputStatus("ExportScript", prm->doExport);
    gdi.setInputStatus("BrowseFile", prm->doExport);
    gdi.setInputStatus("BrowseScript", prm->doExport);    
    gdi.setInputStatus("StructuredExport", prm->doExport); 
    gdi.setInputStatus("PrinterSetup", prm->doPrint);

    string time=createNew ? "10:00" : getTimeMS(prm->interval);		
		gdi.addInput("Interval", time, 7, 0, "Tidsintervall (MM:SS)");
    gdi.dropLine(1);
    gdi.addButton("StartResult", "Starta automaten", AutomaticCB).setExtra(prm);
    gdi.addButton(createNew ? "Stop":"Cancel", "Avbryt", AutomaticCB).setExtra(prm);
    gdi.dropLine(2.5);
    gdi.popX();

    if (!prm->readOnly) {
      gdi.fillDown();
      gdi.addString("", 1, "Listval");  
      gdi.dropLine();
      gdi.fillRight();
      gdi.addListBox("Classes", 150,300,0,"","", true);
      gdi.pushX();
      gdi.fillDown();
      //oe->fillClasses(gdi, "Classes", oEvent::extraNone, oEvent::filterNone);
      vector< pair<string, size_t> > d;
      gdi.addItem("Classes", oe->fillClasses(d, oEvent::extraNone, oEvent::filterNone));
      gdi.setSelection("Classes", prm->classesToPrint);

      gdi.addSelection("ListType", 200, 100, 0, "Lista");
      oe->fillListTypes(gdi, "ListType", 1);
      if (prm->notShown) {
        prm->notShown = false;
        ClassConfigInfo cnf;
        oe->getClassConfigurationInfo(cnf);
        int type = EStdResultListLARGE;
        if (cnf.hasRelay())
          type = EStdTeamResultListLegLARGE;
        else if (cnf.hasPatrol())
          type = EStdRaidResultListLARGE;

        gdi.selectItemByData("ListType", type);
     
      }
      else 
        gdi.selectItemByData("ListType", prm->listInfo.getListCode());
     
      gdi.addSelection("LegNumber", 140, 300, 0, "Sträcka:");
      oe->fillLegNumbers(gdi, "LegNumber");
      int leg = prm->listInfo.getLegNumber();
      gdi.selectItemByData("LegNumber", leg==-1 ? 1000 : leg);

      gdi.addCheckbox("PageBreak", "Sidbrytning mellan klasser", 0, prm->pageBreak);
      gdi.addCheckbox("ShowInterResults", "Visa mellantider", 0, prm->showInterResult, 
                                          "Mellantider visas för namngivna kontroller.");
      gdi.addCheckbox("SplitAnalysis", "Med sträcktidsanalys", 0, prm->splitAnalysis);
    
      gdi.addCheckbox("OnlyChanged", "Skriv endast ut ändade sidor", 0, prm->po.onlyChanged);
          
      gdi.popX();
      gdi.addButton("SelectAll", "Välj allt", AutomaticCB, "").setExtra("Classes");
      gdi.popX();
      gdi.addButton("SelectNone", "Välj inget", AutomaticCB, "").setExtra("Classes");
    }
    else {
      gdi.fillDown();
      gdi.addString("", 1, "Lista av typ 'X'#" + prm->listInfo.getName());
      gdi.addCheckbox("OnlyChanged", "Skriv endast ut ändade sidor", 0, prm->po.onlyChanged);
    }
    gdi.refresh();
#endif
	}
  else if (bu.id == "BrowseFile") {
    static int index = 0;
    vector< pair<string, string> > ext;
    ext.push_back(make_pair("Webbdokument", "*.html;*.htm"));

    string file = gdi.browseForSave(ext, "html", index);
    if (!file.empty())
      gdi.setText("ExportFile", file);
  }
  else if (bu.id == "BrowseScript") {
    vector< pair<string, string> > ext;
    ext.push_back(make_pair("Skript", "*.bat;*.exe;*.js"));

    string file = gdi.browseForOpen(ext, "bat");
    if (!file.empty())
      gdi.setText("ExportScript", file);
  }
  else if (bu.id == "DoExport") {
    bool stat = gdi.isChecked(bu.id);
    gdi.setInputStatus("ExportFile", stat);
    gdi.setInputStatus("ExportScript", stat);
    gdi.setInputStatus("BrowseFile", stat);
    gdi.setInputStatus("BrowseScript", stat);    
    gdi.setInputStatus("StructuredExport", stat); 
  }
  else if (bu.id == "DoPrint") {
    bool stat = gdi.isChecked(bu.id);
    gdi.setInputStatus("PrinterSetup", stat);
  }
  else if (bu.id=="Splits") {
		gdi.restore();
    editMode=true;
    gdi.dropLine();
		gdi.addString("", 1, "Sträcktider / WinSplits");
		gdi.pushX();
		gdi.fillDown();

	  SplitsMachine *sm=dynamic_cast<SplitsMachine*>((AutoMachine*)bu.getExtra());
			
    bool createNew=(sm==0);

		if (!sm) {
			sm=new SplitsMachine;			
 			Machines.push_back(sm);
		}
    gdi.addString("", 10, "help:winsplits_auto");

    char bf[16]="";
    if (sm->interval>0)
      sprintf_s(bf, "%d", sm->interval);
    string time=createNew ? "30" : bf;		
    gdi.dropLine();	
    gdi.addInput("Interval", time, 10, 0, "Intervall (sekunder). Lämna blankt för att uppdatera när "
                                          "tävlingsdata ändras.");
   
    gdi.fillRight();
    gdi.addInput("FileName", sm->file, 30, 0, "Filnamn:");
    gdi.dropLine(0.9);
    gdi.addButton("BrowseSplits", "Bläddra...", AutomaticCB);
    gdi.dropLine(2.5);
    gdi.popX();
    gdi.fillRight();
		gdi.addButton("StartSplits", "OK", AutomaticCB).setExtra(sm);		
    gdi.addButton(createNew ? "Stop":"Cancel", "Avbryt", AutomaticCB).setExtra(sm);
    gdi.dropLine(2.5);
    gdi.refresh();
    gdi.fillDown();
	}
  else if (bu.id=="Prewarning") {
		gdi.restore();
    editMode=true;

		gdi.addString("", 1, "Förvarningsröst");

    gdi.addString("", 10, "help:computer_voice");


    PrewarningMachine *pwm=dynamic_cast<PrewarningMachine*>((AutoMachine*)bu.getExtra());

    bool createNew=(pwm==0);

		if (!pwm) {
			pwm=new PrewarningMachine();			      
			Machines.push_back(pwm);
		}

		gdi.pushX();
		gdi.fillRight();
		gdi.addInput("WaveFolder", pwm->waveFolder,	48, 0, "Ljudfiler, baskatalog.");

		gdi.fillDown();
		gdi.dropLine();
    gdi.addButton("WaveBrowse", "Bläddra...", AutomaticCB);			
		gdi.popX();
		
		gdi.pushX();
		gdi.fillRight();

		gdi.addButton("StartPrewarning", "OK", AutomaticCB).setExtra(pwm);		
    gdi.addButton( (createNew ? "Stop":"Cancel"), "Avbryt", AutomaticCB).setExtra(pwm);
    gdi.dropLine(2.5);
    gdi.popX();
    gdi.addListBox("Controls", 100, 200, 0, "", "", true);
    gdi.pushX();
    gdi.fillDown();
    vector< pair<string, size_t> > d;
    oe->fillControls(d);
    gdi.addItem("Controls", d);
    gdi.setSelection("Controls", pwm->controls);
    gdi.popX();
    gdi.addButton("SelectAll", "Välj alla", AutomaticCB, "").setExtra("Controls");
    gdi.popX(); 
    //gdi.addButton("SelectNone", "Välj inga", AutomaticCB, "","").setExtra("Controls");
	}
  else if (bu.id=="Punches") {
		gdi.restore();
    editMode=true;

		gdi.addString("", 1, "Test av stämplingsinläsningar");

    gdi.addString("", 10, "help:simulate");

    PunchMachine *pm=dynamic_cast<PunchMachine*>((PunchMachine*)bu.getExtra());

    bool createNew=(pm==0);

		if (!pm) {
			pm=new PunchMachine();			      
			Machines.push_back(pm);
		}
		
		gdi.pushX();
		gdi.fillRight();

    gdi.addString("", 0, "Radiotider, kontroll:");
    gdi.dropLine(-0.2);
    gdi.addInput("Radio", "", 6, 0);
    gdi.popX();
    gdi.dropLine(2);
    string time=createNew ? "0:10" : getTimeMS(pm->interval);		
		gdi.addInput("Interval", time, 10, 0, "Stämplingsintervall (MM:SS)");
    gdi.dropLine();
		gdi.addButton("StartPunch", "OK", AutomaticCB).setExtra(pm);		
    gdi.addButton( (createNew ? "Stop":"Cancel"), "Avbryt", AutomaticCB).setExtra(pm);

    gdi.fillDown();
    gdi.dropLine(3);
    gdi.popX();
    gdi.dropLine(1);
    gdi.addString("", 1, "Generera testtävling");
    gdi.fillRight();
    gdi.addInput("nRunner", "100", 10, 0, "Antal löpare");
    gdi.addInput("nClass", "10", 10, 0, "Antal klasser");
    gdi.dropLine();
    gdi.addCheckbox("UseRelay", "Med stafettklasser");
    gdi.dropLine(-1);
    gdi.addButton("GenerateCMP", "Generera testtävling", AutomaticCB);		
    gdi.fillDown();
    gdi.popX();
    gdi.refresh();
	}
	else if (bu.id=="StartResult") {
#ifndef MEOSDB
		string minute=gdi.getText("Interval");
		int t=convertAbsoluteTimeMS(minute);

		if (t<5 || t>7200) {
			gdi.alert("Intervallet måste anges på formen MM:SS.");
		}
		else {
			PrintResultMachine *prm=dynamic_cast<PrintResultMachine*>((AutoMachine*)bu.getExtra());
			
      if(prm) {
        prm->interval = t;
        prm->doExport = gdi.isChecked("DoExport");
        prm->doPrint = gdi.isChecked("DoPrint");
        prm->exportFile = gdi.getText("ExportFile");
        prm->exportScript = gdi.getText("ExportScript");
        prm->structuredExport = gdi.isChecked("StructuredExport");

        if (!prm->readOnly) {
          gdi.getSelection("Classes", prm->classesToPrint);

          ListBoxInfo lbi;
          if (gdi.getSelectedItem("ListType", &lbi)) {
            oListParam par;
            par.selection=prm->classesToPrint;
            par.listCode = EStdListType(lbi.data);
            par.pageBreak = gdi.isChecked("PageBreak");
            par.showInterTimes = gdi.isChecked("ShowInterResults");
            par.splitAnalysis = gdi.isChecked("SplitAnalysis");
            gdi.getSelectedItem("LegNumber", &lbi);
            if (signed(lbi.data)>=0)
              par.legNumber = lbi.data == 1000 ? -1: lbi.data;
            else
              par.legNumber = 0;

            oe->generateListInfo(par, gdi.getLineHeight(), prm->listInfo);      
          }
        }
        prm->po.onlyChanged = gdi.isChecked("OnlyChanged");
        prm->pageBreak = gdi.isChecked("PageBreak");
        prm->showInterResult = gdi.isChecked("ShowInterResults");
        prm->splitAnalysis = gdi.isChecked("SplitAnalysis");
        prm->synchronize=true; //To force continuos data sync.
			  setTimer(prm);
      }
      updateSyncInfo();
			loadPage(gdi);
		}
#endif
	}
  else if (bu.id=="StartSplits") {
		int iv=gdi.getTextNo("Interval");
		const string &file=gdi.getText("FileName");

    if (file.empty()) {
      gdi.alert("Filnamnet får inte vara tomt");
      return false;
    }

    //Try exporting.
#ifndef MEOSDB
    oe->exportIOFSplits(oEvent::IOF20, file.c_str(), true, false, set<int>());
#endif

		SplitsMachine *sm=dynamic_cast<SplitsMachine*>((AutoMachine*)bu.getExtra());
			
    if(sm) {
      sm->interval=iv;

      sm->file=file;
      sm->synchronize=true;
		  setTimer(sm);
    }
    updateSyncInfo();
		loadPage(gdi);
	}	
 	else if (bu.id=="StartPrewarning") {
		PrewarningMachine *pwm=dynamic_cast<PrewarningMachine*>((AutoMachine*)bu.getExtra());
			
    if (pwm) {  
      pwm->waveFolder=gdi.getText("WaveFolder");
      gdi.getSelection("Controls", pwm->controls);    

			oe->synchronizeList(oLPunchId);
			oe->clearPrewarningSounds();

      pwm->synchronizePunches=true;
      pwm->controlsSI.clear();
      for (set<int>::iterator it=pwm->controls.begin();it!=pwm->controls.end();++it) {
        pControl pc=oe->getControl(*it, false);

        if (pc) 
          pwm->controlsSI.insert(pc->Numbers, pc->Numbers+pc->nNumbers);
      }
    }
		updateSyncInfo();
    loadPage(gdi);
	}
  else if (bu.id=="StartPunch") {
			
		string minute=gdi.getText("Interval");
		int t=convertAbsoluteTimeMS(minute);

		if (t<1 || t>7200) {
			gdi.alert("Intervallet måste anges på formen MM:SS");
		}
		else {
		  PunchMachine *pm=dynamic_cast<PunchMachine*>((AutoMachine*)bu.getExtra());
			
      if(pm) {
        pm->interval=t;
        pm->radio=gdi.getTextNo("Radio");
        setTimer(pm);
      }      
    }
		updateSyncInfo();
    loadPage(gdi);
	}
	else if (bu.id == "Cancel") {
		loadPage(gdi);
	}
  else if (bu.id == "Stop") {
		if (bu.getExtra()) 
			stopMachine(static_cast<AutoMachine*>(bu.getExtra()));		

    updateSyncInfo();
		loadPage(gdi);
	}
  else if (bu.id == "PrinterSetup") {
    PrintResultMachine *prm =
          dynamic_cast<PrintResultMachine*>((AutoMachine*)bu.getExtra());

    if (prm) {
      gdi.printSetup(prm->po);
    }
  }
  else if (bu.id == "PrintNow") {
     PrintResultMachine *prm =
          dynamic_cast<PrintResultMachine*>((AutoMachine*)bu.getExtra());

     if(prm) {
       prm->process(gdi, oe, SyncNone);
       setTimer(prm);
       loadPage(gdi);
     }
  }
  else if (bu.id == "SelectAll") {
    const char *ctrl=
      static_cast<const char *>(bu.getExtra());
    set<int> lst;
    lst.insert(-1);
    gdi.setSelection(ctrl, lst);
  }
  else if (bu.id == "SelectNone") {
    const char *ctrl=
      static_cast<const char *>(bu.getExtra());
    set<int> lst;
    gdi.setSelection(ctrl, lst);
  }
  else if (bu.id == "TestVoice") {
    PrewarningMachine *pwm=dynamic_cast<PrewarningMachine*>((AutoMachine*)bu.getExtra());

    if(pwm)
      oe->tryPrewarningSounds(pwm->waveFolder, rand()%400+1);
  }
  else if( bu.id == "WaveBrowse") {
	  string wf=gdi.browseForFolder(gdi.getText("WaveFolder"));

		if(wf.length()>0)
		  gdi.setText("WaveFolder", wf);
	}
  else if( bu.id == "BrowseSplits") {
    int index=0;
    vector< pair<string, string> > ext;
    ext.push_back(make_pair("Sträcktider", "*.xml"));

    string wf = gdi.browseForSave(ext, "xml", index);
      
		if(!wf.empty())
		  gdi.setText("FileName", wf);
	}

	return 0;
}

int TabAuto::processListBox(gdioutput &gdi, const ListBoxInfo &bu)
{
	return 0;
}

bool TabAuto::stopMachine(AutoMachine *am)
{
	list<AutoMachine *>::iterator it;
	for (it=Machines.begin(); it!=Machines.end(); ++it) 	
		if (am==*it)  {
			if (am->stop()) {
        delete am;
				Machines.erase(it);
				return true;
			}
		}
	return false;
}


void TabAuto::killMachines()
{
  while(!Machines.empty()) {
    Machines.back()->stop();
    delete Machines.back();		
    Machines.pop_back();
  }
}

bool TabAuto::loadPage(gdioutput &gdi)
{
  oe->checkDB();
	tabAuto=this;
  editMode=false;

	gdi.clearPage(true);
  gdi.setData("AutoPage", 1);
	gdi.addString("", boldLarge, "Automater");
  gdi.addString("", 10, "help:10000");
	
	gdi.setRestorePoint();

  gdi.dropLine();
  gdi.addString("", boldText, "Tillgängliga automater");
	gdi.dropLine();
  gdi.fillRight();
  gdi.pushX();
  gdi.addButton("Result", "Resultatutskrift / export", AutomaticCB, "tooltip:resultprint"); 
  gdi.addButton("Prewarning", "Förvarningsröst", AutomaticCB, "tooltip:voice"); 
  gdi.addButton("Splits", "Sträcktider (WinSplits)", AutomaticCB, "Spara sträcktider till en fil för automatisk synkronisering med WinSplits"); 
  
  gdi.addButton("Punches", "Stämplingstest", AutomaticCB, "Simulera inläsning av stämplar"); 
  
  gdi.fillDown();
	gdi.dropLine(3);
  gdi.popX();

	if (!Machines.empty()) {
		gdi.addString("", boldText, "Startade automater");	
		list<AutoMachine *>::iterator it;

		for (it=Machines.begin(); it!=Machines.end(); ++it) {
			AutoMachine *am=*it;
			if (am) {
				//gdi.addString("", 0, am->name);
				am->status(gdi); 
			}
		}		
		gdi.dropLine();
	}
	return true;
}


void PrintResultMachine::process(gdioutput &gdi, oEvent *oe, AutoSyncType ast)
{
  #ifndef MEOSDB  

  if (lock)
    return;

  if(ast!=SyncDataUp) {
    lock = true;
    try {
      gdioutput gdiPrint(gdi.getScale());	
	    gdiPrint.clearPage(false);	
      oe->generateList(gdiPrint, true, listInfo);
      if (doPrint) {
        gdiPrint.refresh();
        gdiPrint.print(po, oe);
      }
      if (doExport) {
        if (!exportFile.empty()) {
          if (structuredExport)
            gdiPrint.writeTableHTML(exportFile, oe->getName());
          else
            gdiPrint.writeHTML(exportFile, oe->getName());

          if (!exportScript.empty()) {
            ShellExecute(NULL, NULL, exportScript.c_str(), exportFile.c_str(), NULL, SW_HIDE);
          }
        }
      }
    }
    catch (...) {
      lock = false;
      throw;
    }
    lock = false;
  }
  #else
    throw new std::exception("Bad method call");
  #endif
}

void PrintResultMachine::status(gdioutput &gdi)
{
  gdi.dropLine(0.5);
  gdi.fillRight();
  gdi.pushX();
  gdi.addString("", 0, name).setColor(colorGreen);
  gdi.addString("", 0, listInfo.getName());
  gdi.dropLine();
  if (doExport) {
    gdi.popX();
    gdi.addString("", 0, "Målfil: ");
    gdi.addStringUT(0, exportFile).setColor(colorRed);
    gdi.dropLine();
  }
  gdi.fillRight();
	gdi.popX();
	if(interval>0){		
		gdi.addString("", 0, "Automatisk utskrift / export: ");
		gdi.addTimer(gdi.getCY(),  gdi.getCX(), timerCanBeNegative, (GetTickCount()-timeout)/1000);
	}
	else {

	}
	gdi.popX();
	gdi.dropLine(2);
  gdi.addButton("Stop", "Stoppa automaten", AutomaticCB).setExtra(this);
  gdi.addButton("PrintNow", "Exportera nu", AutomaticCB).setExtra(this);
	gdi.fillDown();
	gdi.addButton("Result", "Inställningar...", AutomaticCB).setExtra(this);
	gdi.popX();		
}

void PrewarningMachine::process(gdioutput &gdi, oEvent *oe, AutoSyncType ast)
{
  oe->playPrewarningSounds(waveFolder, controlsSI);
}

void PrewarningMachine::status(gdioutput &gdi)
{
 	gdi.dropLine(0.5);
  gdi.addString("", 1, name);
    
  string info="Förvarning på (SI-kod): ";
  bool first=true;

  if(controls.empty())
    info+="alla stämplingar";
  else {
    for (set<int>::iterator it=controlsSI.begin();it!=controlsSI.end();++it) {
      char bf[32];
      _itoa_s(*it, bf, 10);  
      
      if(!first) info+=", ";
      else       first=false;

      info+=bf;
    }
  }
  gdi.addString("", 0, info);
  gdi.fillRight();
	gdi.pushX();

	gdi.popX();
	gdi.dropLine(0.3);
  gdi.addButton("Stop", "Stoppa automaten", AutomaticCB).setExtra(this);
  gdi.addButton("TestVoice", "Testa rösten", AutomaticCB).setExtra(this);
	gdi.fillDown();
  gdi.addButton("Prewarning", "Inställningar...", AutomaticCB).setExtra(this);
  gdi.popX();
}

void PunchMachine::status(gdioutput &gdi)
{
  gdi.dropLine(0.5);
	gdi.addString("", 1, name);
  gdi.fillRight();
	gdi.pushX();
	if(interval>0){		
		gdi.addString("", 0, "Stämplar om: ");
		gdi.addTimer(gdi.getCY(),  gdi.getCX(), timerCanBeNegative, (GetTickCount()-timeout)/1000);
	}
	else {

	}
	gdi.popX();
	gdi.dropLine(2);
  gdi.addButton("Stop", "Stoppa automaten", AutomaticCB).setExtra(this);
  gdi.fillDown();
	gdi.addButton("Punches", "Inställningar...", AutomaticCB).setExtra(this);
	gdi.popX();		
}

void PunchMachine::process(gdioutput &gdi, oEvent *oe, AutoSyncType ast)
{
#ifndef MEOSDB
  SICard sic;
  oe->generateTestCard(sic);
  if(gSI && !sic.empty()) {
    if(!radio) gSI->AddCard(sic);
  }
  else gdi.addInfoBox("", "Failed to generate card.", interval*2);

  if(radio && !sic.empty()) {
    pRunner r=oe->getRunnerByCard(sic.CardNumber, false);
    //??? if(r) r=oe->getRunner(r->getId()+1, 0);
    if(r && r->getCardNo()) {
      sic.CardNumber=r->getCardNo();
      sic.PunchOnly=true;
      sic.nPunch=1;
      sic.Punch[0].Code=radio;
      gSI->AddCard(sic);
    }
  }
#endif
}

void SplitsMachine::status(gdioutput &gdi)
{
  gdi.dropLine(0.5);
	gdi.addString("", 1, name);
  if (!file.empty()) {
    gdi.fillRight();
	  gdi.pushX();
    gdi.addString("", 0, "Fil: X#" + file);
      
	  if(interval>0){		
      gdi.popX();
      gdi.dropLine(1);  
		  gdi.addString("", 0, "Skriver sträcktider om: ");
		  gdi.addTimer(gdi.getCY(),  gdi.getCX(), timerCanBeNegative, (GetTickCount()-timeout)/1000);
      gdi.addString("", 0, "(sekunder)");
	  }
	  else {
      gdi.dropLine(1);  
		  gdi.addString("", 0, "Skriver sträcktider när tävlingsdata ändras.");
	  }

    gdi.popX();
  }
	gdi.dropLine(2);
  gdi.addButton("Stop", "Stoppa automaten", AutomaticCB).setExtra(this);
  gdi.fillDown();
	gdi.addButton("Splits", "Inställningar...", AutomaticCB).setExtra(this);
	gdi.popX();		
}

void SplitsMachine::process(gdioutput &gdi, oEvent *oe, AutoSyncType ast)
{
#ifndef MEOSDB
  if ((interval>0 && ast==SyncTimer) || (interval==0 && ast==SyncDataUp)) {
    if(!file.empty())
      oe->exportIOFSplits(oEvent::IOF20, file.c_str(), true, false, classes, leg);
  }
#endif
}

