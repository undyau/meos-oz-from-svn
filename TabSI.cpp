/************************************************************************
    MeOS - Orienteering Software
    Copyright (C) 2009-2014 Melin Software HB
    
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
#include "gdifonts.h"
#include "gdiconstants.h"

#include "csvparser.h"

#include "TabSI.h"
#include "TabAuto.h"
#include "TabList.h"
#include "meos_util.h"
#include <cassert>
#include "TabRunner.h"
#include "meosexception.h"

TabSI::TabSI(oEvent *poe):TabBase(poe) {
  interactiveReadout=poe->getPropertyInt("Interactive", 1)!=0;
  useDatabase=poe->useRunnerDb() && poe->getPropertyInt("Database", 1)!=0;
  printSplits=false;
  manualInput = poe->getPropertyInt("ManualInput", 0) == 1;

  mode=ModeReadOut;
  currentAssignIndex=0;
  
  lastClubId=0;
  lastClassId=0;
  lastFee=0;
  logger = 0;

  minRunnerId = 0;
  inputId = 0;
}

TabSI::~TabSI(void)
{
  if (logger!=0)
    delete logger;
  logger = 0;
}


static void entryTips(gdioutput &gdi) {
  gdi.fillDown();
  gdi.addString("", 10, "help:21576");
  gdi.dropLine(1);
  gdi.setRestorePoint("EntryLine");
}


void TabSI::logCard(const SICard &card)
{
  if (logger == 0) {
    logger = new csvparser;
    string readlog = "sireadlog_" + getLocalTimeFileName() + ".csv";
    char file[260];
    string subfolder = makeValidFileName(oe->getName(), true);
    const char *sf = subfolder.empty() ? 0 : subfolder.c_str();
    getDesktopFile(file, readlog.c_str(), sf);
    logger->openOutput(file);
    vector<string> head = SICard::logHeader();
    logger->OutputRow(head);
    logcounter = 0;
  }
  
  vector<string> log = card.codeLogData(++logcounter);
  logger->OutputRow(log);
}

extern SportIdent *gSI;
extern pEvent gEvent;

void LoadRunnerPage(gdioutput &gdi);


int SportIdentCB(gdioutput *gdi, int type, void *data)
{
  TabSI &tsi = dynamic_cast<TabSI &>(*gdi->getTabs().get(TSITab));

  return tsi.siCB(*gdi, type, data);
}

int TabSI::siCB(gdioutput &gdi, int type, void *data)
{
	if (type==GUI_BUTTON) {
		ButtonInfo bi=*(ButtonInfo *)data;
    
    if (bi.id=="SIPassive") {
      string port=gdi.getText("ComPortName");
      bool debugLogic = true;
      if (debugLogic && gSI->OpenComListen(port.c_str(), gdi.getTextNo("BaudRate"))) {
				gSI->StartMonitorThread(port.c_str());
				loadPage(gdi);
        gdi.addString("", 1, "Lyssnar p� X.#"+port).setColor(colorDarkGreen);
			}
			else
        gdi.addString("", 1, "FEL: Porten kunde inte �ppnas").setColor(colorRed);
      gdi.dropLine();
      gdi.refresh();
		}
    else if (bi.id=="CancelTCP")
      gdi.restore("TCP");
    else if (bi.id=="StartTCP") {          
      gSI->tcpAddPort(gdi.getTextNo("tcpPortNo"), 0);
      gdi.restore("TCP");
      gSI->StartMonitorThread("TCP");

			gdi.addStringUT(0, gSI->getInfoString("TCP"));
      gdi.dropLine(0.5);
      refillComPorts(gdi);
      gdi.refresh();
    }
		else if (bi.id=="StartSI")	{
			char bf[64];
      bool debugLogic = true;
			ListBoxInfo lbi;
			if(gdi.getSelectedItem("ComPort", &lbi)) {

				sprintf_s(bf, 64, "COM%d", lbi.data);
				string port=bf;

        if(lbi.text.substr(0, 3)=="TCP")
          port="TCP";
				
				if (gSI->IsPortOpen(port)) {
          gSI->CloseCom(port.c_str());
					gdi.addStringUT(0, lang.tl("Kopplar ifr�n SportIdent p� ") + port + lang.tl("... OK"));
          refillComPorts(gdi);
				}
				else {
          gdi.fillDown();
          if (!debugLogic && port=="TCP") {
            gdi.setRestorePoint("TCP");
            gdi.dropLine();
            gdi.pushX();
            gdi.fillRight();
            gdi.addInput("tcpPortNo", "10000", 8,0, "Port f�r TCP:");
            gdi.dropLine();
            gdi.addButton("StartTCP", "Starta", SportIdentCB);    
            gdi.addButton("CancelTCP", "Avbryt", SportIdentCB);  
            gdi.dropLine(2);
            gdi.popX();
            gdi.fillDown();
            gdi.addString("", 10, "help:14070");
            gdi.scrollToBottom();
            gdi.refresh();
            return 0;
          }

					gdi.addStringUT(0, lang.tl("Startar SI p�")+" "+ port + "...");
					gdi.refresh();
          if(gSI->OpenCom(port.c_str())){
						gSI->StartMonitorThread(port.c_str());
						gdi.addStringUT(0, lang.tl("SI p�")+" "+ port + ": "+lang.tl("OK"));
						gdi.addStringUT(0, gSI->getInfoString(port));
            SI_StationInfo *si = gSI->findStation(port);
            if (si && !si->Extended)
              gdi.addString("", boldText, "warn:notextended").setColor(colorDarkRed);
					}
					else{
						//Retry...
						Sleep(300);
						if (gSI->OpenCom(port.c_str())) {
							gSI->StartMonitorThread(port.c_str());
							gdi.addStringUT(0, lang.tl("SI p�") + " " + port + ": " + lang.tl("OK"));
							gdi.addStringUT(0, gSI->getInfoString(port.c_str()));
              SI_StationInfo *si = gSI->findStation(port);
              if (si && !si->Extended)
                gdi.addString("", boldText, "warn:notextended").setColor(colorDarkRed);
						}
						else {
							gdi.setRestorePoint();
							gdi.addStringUT(1, lang.tl("SI p�") +" "+ port + ": " + lang.tl("FEL, inget svar.")).setColor(colorRed);
              gdi.dropLine();
							gdi.refresh();

							if(gdi.ask("help:9615")) {
 
								gdi.pushX();
								gdi.fillRight();
								gdi.addInput("ComPortName", port, 10, 0, "COM-Port:");
								//gdi.addInput("BaudRate", "4800", 10, 0, "help:baudrate");
                gdi.fillDown();
                gdi.addCombo("BaudRate", 130, 100, 0, "help:baudrate");
                gdi.popX();
                gdi.addItem("BaudRate", "4800", 4800);
                gdi.addItem("BaudRate", "38400", 38400);
                gdi.selectItemByData("BaudRate", 38400);


                gdi.fillRight();
                gdi.addButton("SIPassive", "Lyssna...", SportIdentCB).setDefault();
								gdi.fillDown();
                gdi.addButton("Cancel", "Avbryt", SportIdentCB).setCancel();
								gdi.popX();
							}
						}
					}
					refillComPorts(gdi);
				}
				gdi.refresh();
			}
		}
		else if (bi.id=="SIInfo") {
			char bf[64];
			ListBoxInfo lbi;
			if(gdi.getSelectedItem("ComPort", &lbi))
			{
        if(lbi.text.substr(0,3)=="TCP")
          sprintf_s(bf, 64, "TCP");
        else
				  sprintf_s(bf, 64, "COM%d", lbi.data);

				gdi.addStringUT(0, lang.tl("H�mtar information om")+" "+string(bf)+".");
				gdi.addStringUT(0, gSI->getInfoString(bf));
        gdi.refresh();
			}
		}
		else if(bi.id=="AutoDetect")
		{
			gdi.addString("", 0, "S�ker efter SI-enheter... ");
			gdi.refresh();
			list<int> ports;
			if (!gSI->AutoDetect(ports)) {
				gdi.addString("SIInfo", 0, "help:5422");
				gdi.refresh();
				return 0;
			}
			char bf[128];
			gSI->CloseCom(0);

			while(!ports.empty()) {
				int p=ports.front();
				sprintf_s(bf, 128, "COM%d", p);

        gdi.addString((string("SIInfo")+bf).c_str(), 0, "#" + lang.tl("Startar SI p�") + " " + string(bf) + "...");
				gdi.refresh();
				if (gSI->OpenCom(bf)) {
					gSI->StartMonitorThread(bf);
					gdi.addStringUT(0, lang.tl("SI p�") + " " + string(bf) + ": " + lang.tl("OK"));
					gdi.addStringUT(0, gSI->getInfoString(bf));
          SI_StationInfo *si = gSI->findStation(bf);
          if (si && !si->Extended)
             gdi.addString("", boldText, "warn:notextended").setColor(colorDarkRed);
				}
				else if (gSI->OpenCom(bf)) {
					gSI->StartMonitorThread(bf);
					gdi.addStringUT(0, lang.tl("SI p�") + " " + string(bf) + ": " + lang.tl("OK"));
					gdi.addStringUT(0, gSI->getInfoString(bf));
          SI_StationInfo *si = gSI->findStation(bf);
          if (si && !si->Extended)
            gdi.addString("", boldText, "warn:notextended").setColor(colorDarkRed);
				}
				else gdi.addStringUT(0, lang.tl("SI p�") + " " + string(bf) + ": " +lang.tl("FEL, inget svar"));
				
				gdi.refresh();
				ports.pop_front();
			}
		}    
    else if (bi.id == "PrinterSetup") {
      //printerSetup(gdi);
      printSplits = true;
      TabList::splitPrintSettings(*oe, gdi, TSITab);
    }
    else if (bi.id == "AutoTie") {
      gEvent->setProperty("AutoTie", gdi.isChecked("AutoTie"));
    }
    else if (bi.id == "RentCard") {
      gEvent->setProperty("RentCard", gdi.isChecked("RentCard"));
    }
    else if (bi.id == "TieOK") {
      tieCard(gdi);
    }
    else if (bi.id=="Interactive") {
      interactiveReadout=gdi.isChecked(bi.id);
      gEvent->setProperty("Interactive", interactiveReadout);

      if (mode == ModeAssignCards) {
        gdi.restore("ManualTie", false);
        showAssignCard(gdi, false);
      }
    }
    else if (bi.id=="Database") {
      useDatabase=gdi.isChecked(bi.id);
      gEvent->setProperty("Database", useDatabase);
    }
    else if (bi.id=="PrintSplits") {
      printSplits=gdi.isChecked(bi.id);
    }
    else if (bi.id == "UseManualInput") {
      manualInput = gdi.isChecked("UseManualInput");
      oe->setProperty("ManualInput", manualInput ? 1 : 0);
      gdi.restore("ManualInput");
      if (manualInput)
        showManualInput(gdi);
    }
    else if (bi.id=="Import") {
      int origin = (int)bi.getExtra();

      vector< pair<string, string> > ext;
      ext.push_back(make_pair("Semikolonseparerad (csv)", "*.csv"));

      string file = gdi.browseForOpen(ext, "csv");
      if (!file.empty()) {
        gdi.restore("Help");
        csvparser csv;
        csv.importCards(*oe, file.c_str(), cards);
        if (cards.empty()) {
          csv.importPunches(*oe, file.c_str(), punches);
          if (!punches.empty()) {
            gdi.dropLine(2);
            gdi.addString("", 1, "Inl�sta st�mplar");
            set<string> dates;
            showReadPunches(gdi, punches, dates);
            
            filterDate.clear();
            filterDate.push_back(lang.tl("Inget filter"));
            for (set<string>::iterator it = dates.begin(); it!=dates.end(); ++it)
              filterDate.push_back(*it);

            gdi.dropLine(2);
            gdi.scrollToBottom();
            gdi.fillRight();
            gdi.pushX();
            gdi.addSelection("ControlType", 150, 300, 0, "Enhetstyp");
            oe->fillControlTypes(gdi, "ControlType");
            gdi.selectItemByData("ControlType", oPunch::PunchCheck);
            
            gdi.addSelection("Filter", 150, 300, 0, lang.tl("Datumfilter"));
            for (size_t k = 0; k<filterDate.size(); k++) {
              gdi.addItem("Filter", filterDate[k], k);
            }
            gdi.selectItemByData("Filter", 0);   
            gdi.dropLine(1);
            gdi.addButton("SavePunches", "Spara", SportIdentCB).setExtra((void *)origin);
            gdi.addButton("Cancel", "Avbryt", SportIdentCB).setExtra((void *)origin);
            gdi.fillDown();
            gdi.popX();
          }
          else {
            loadPage(gdi);
            throw std::exception("Felaktigt filformat");
          }
        }
        else {
          gdi.pushX();
          gdi.dropLine(3);
          
          gdi.addString("", 1, "Inl�sta brickor");
          showReadCards(gdi, cards);
          gdi.dropLine();
          gdi.fillDown();
          if (interactiveReadout)
            gdi.addString("", 0, "V�lj Spara f�r att lagra brickorna. Interaktiv inl�sning �r aktiverad.");
          else
            gdi.addString("", 0, "V�lj Spara f�r att lagra brickorna. Interaktiv inl�sning �r INTE aktiverad.");
          
          gdi.fillRight();
          gdi.pushX();
          gdi.addButton("SaveCards", "Spara", SportIdentCB).setExtra((void *)origin);
          gdi.addButton("Cancel", "Avbryt", SportIdentCB).setExtra((void *)origin);
          gdi.fillDown();
          gdi.scrollToBottom();
        }
      }
    }
    else if (bi.id=="SavePunches") {
      int origin = (int)bi.getExtra();
      ListBoxInfo lbi;
      gdi.getSelectedItem("ControlType", &lbi);
      int type = lbi.data;
      gdi.getSelectedItem("Filter", &lbi);
      bool dofilter = signed(lbi.data)>0;
      string filter = lbi.data < filterDate.size() ? filterDate[lbi.data] : "";
      
      gdi.restore("Help");
      for (size_t k=0;k<punches.size();k++) {
        if (dofilter && filter != punches[k].date)
          continue;
        oe->addFreePunch(punches[k].time, type, punches[k].card, true);
      }
      punches.clear();
      if (origin==1) {
        TabRunner &tc = dynamic_cast<TabRunner &>(*gdi.getTabs().get(TRunnerTab));
        tc.showInForestList(gdi);
      }
    }
    else if (bi.id=="SaveCards") {
      int origin = (int)bi.getExtra();
      gdi.restore("Help");
      oe->synchronizeList(oLCardId, true, false);
      oe->synchronizeList(oLRunnerId, false, true);
      for (size_t k=0;k<cards.size();k++)
        insertSICard(gdi, cards[k]);

      oe->reEvaluateAll(set<int>(), true);
      cards.clear();
      if (origin==1) {
        TabRunner &tc = dynamic_cast<TabRunner &>(*gdi.getTabs().get(TRunnerTab));
        tc.showInForestList(gdi);
      }
    }
		else if (bi.id=="Save") {
			SICard sic;
      sic.clear(0);
      sic.CheckPunch.Code = -1;
			sic.CardNumber=gdi.getTextNo("SI");
			int f=sic.FinishPunch.Time=convertAbsoluteTimeHMS(gdi.getText("Finish"));
			int s=sic.StartPunch.Time=convertAbsoluteTimeHMS(gdi.getText("Start"));
      
      double t = 0.1;
      for (sic.nPunch = 0; sic.nPunch<8; sic.nPunch++) {
        sic.Punch[sic.nPunch].Code=gdi.getTextNo("C" + itos(sic.nPunch+1));
			  sic.Punch[sic.nPunch].Time=(int)(f*t+s*(1.0-t));
        t += ((1.0-t) * (sic.nPunch + 1) / 10.0) * ((rand() % 100) + 400.0)/500.0;
        if (sic.nPunch == 1 || 5 == sic.nPunch)
          t += min(0.2, 0.9-t);
      }

			gSI->AddCard(sic);
		}
    else if (bi.id=="SaveP") {
			SICard sic;
      sic.clear(0);
      sic.FinishPunch.Code = -1;
      sic.CheckPunch.Code = -1;
      sic.StartPunch.Code = -1;

			sic.CardNumber=gdi.getTextNo("SI");
			int f=convertAbsoluteTimeHMS(gdi.getText("Finish"));
      if (f > 0) {
        sic.FinishPunch.Time = f;
        sic.FinishPunch.Code = 1;
        sic.PunchOnly = true;
        gSI->AddCard(sic);
        return 0;
      }


			int s=convertAbsoluteTimeHMS(gdi.getText("Start"));
      if (s > 0) {
        sic.StartPunch.Time = s;
        sic.StartPunch.Code = 1;
        sic.PunchOnly = true;
        gSI->AddCard(sic);
        return 0;
      }

      sic.Punch[sic.nPunch].Code=gdi.getTextNo("C1");
			sic.Punch[sic.nPunch].Time=convertAbsoluteTimeHMS(gdi.getText("C2"));
      sic.nPunch = 1;
      sic.PunchOnly = true;
			gSI->AddCard(sic);
		}
		else if(bi.id=="Cancel") {
      int origin = (int)bi.getExtra();
      activeSIC.clear(0);
      punches.clear();
      if (origin==1) {
        TabRunner &tc = dynamic_cast<TabRunner &>(*gdi.getTabs().get(TRunnerTab));
        tc.showInForestList(gdi);
			  return 0;
      }
      loadPage(gdi);

      checkMoreCardsInQueue(gdi);
			return 0;
		}
		else if(bi.id=="OK1") {
			string name=gdi.getText("Runners");
			string club=gdi.getText("Club");

			if(name.length()==0){
				gdi.alert("Alla deltagare m�ste ha ett namn.");
				return 0;
			}

			pRunner r=0;
			DWORD rid;
      bool lookup = true;

			if(gdi.getData("RunnerId", rid) && rid>0) {
				r = gEvent->getRunner(rid, 0);

        if (r && r->getCard()) {
          if (!askOverwriteCard(gdi, r)) {
            r = 0;
            lookup = false;
          }
        }

				if (r && stringMatch(r->getName(), name)) {
					gdi.restore();
					//We have a match!
          SICard copy = activeSIC;
          activeSIC.clear(&activeSIC);
          processCard(gdi, r, copy);          
					return 0;
				}
			}
		  
      if (lookup) {
			  r = gEvent->getRunnerByName(name, club);
        if (r && r->getCard()) {
          if (!askOverwriteCard(gdi, r))
            r = 0;
        }
      }

			if (r)	{
        //We have a match!
				gdi.setData("RunnerId", r->getId());
				
				gdi.restore();				
        SICard copy = activeSIC;
				activeSIC.clear(&activeSIC);
        processCard(gdi, r, copy);        
				return 0;
			}
			
			//We have a new runner in our system
			gdi.fillRight();
			gdi.pushX();
	
			SICard si_copy=activeSIC;
			gEvent->convertTimes(si_copy);

      //Find matching class...
      vector<pClass> classes;
			int dist = gEvent->findBestClass(activeSIC, classes);

      if (classes.size()==1 && dist == 0 && si_copy.StartPunch.Time>0 && classes[0]->getType()!="tmp") {
				//We have a match!
        string club = gdi.getText("Club");

			  if(club.length()==0)
				  club=lang.tl("Klubbl�s");
        int year = 0;
        pRunner r=gEvent->addRunner(gdi.getText("Runners"), club, 
                                    classes[0]->getId(), activeSIC.CardNumber, year, true);

				gdi.setData("RunnerId", r->getId());
				
				gdi.restore();				
        SICard copy = activeSIC;
				activeSIC.clear(&activeSIC);
        processCard(gdi, r, copy);
        r->synchronize();
				return 0;
      }
        
      
      gdi.restore("restOK1", false);
      gdi.popX();
			gdi.dropLine(2);

      gdi.addInput("StartTime", gEvent->getAbsTime(si_copy.StartPunch.Time), 8, 0, "Starttid:");
			
			gdi.addSelection("Classes", 200, 300, 0, "Klass:");
			gEvent->fillClasses(gdi, "Classes", oEvent::extraNone, oEvent::filterNone);
			gdi.setInputFocus("Classes");
			
      if(classes.size()>0)	
        gdi.selectItemByData("Classes", classes[0]->getId());

			gdi.dropLine();

      gdi.setRestorePoint("restOK2");
      
			gdi.addButton("Cancel", "Avbryt", SportIdentCB).setCancel();
		  if (oe->getNumClasses() > 0)
			  gdi.addButton("OK2", "OK", SportIdentCB).setDefault();
			gdi.fillDown();

			gdi.addButton("NewClass", "Skapa ny klass", SportIdentCB);

			gdi.popX();
      if(classes.size()>0)	
        gdi.addString("FindMatch", 0, "Press enter to continue").setColor(colorGreen);
      gdi.dropLine();

			gdi.refresh();
			return 0;
		}
		else if(bi.id=="OK2")
		{
			//New runner in existing class...

			ListBoxInfo lbi;
			gdi.getSelectedItem("Classes", &lbi);
			
			if(lbi.data==0 || lbi.data==-1) {
				gdi.alert("Du m�ste v�lja en klass");
				return 0;
			}
      pClass pc = oe->getClass(lbi.data);
      if (pc && pc->getType()== "tmp")
        pc->setType("");

			string club = gdi.getText("Club");

			if(club.length()==0)
				club=lang.tl("Klubbl�s");

      int year = 0;
			pRunner r=gEvent->addRunner(gdi.getText("Runners"), club, 
                                  lbi.data, activeSIC.CardNumber, year, true);

			r->setStartTimeS(gdi.getText("StartTime"));
			r->setCardNo(activeSIC.CardNumber, false);
			
			gdi.restore();				
      SICard copy = activeSIC;
			activeSIC.clear(&activeSIC);
      processCard(gdi, r, copy);      
		}
		else if(bi.id=="NewClass") {
      gdi.restore("restOK2", false);
      gdi.popX();
      gdi.dropLine(2);
			//gdi.disableInput("OK2");
			//gdi.disableInput("NewClass");
			gdi.fillRight();
			gdi.pushX();
	
			gdi.addInput("ClassName", gEvent->getAutoClassName(), 10,0, "Klassnamn:");

			gdi.dropLine();
		  gdi.addButton("Cancel", "Avbryt", SportIdentCB).setCancel();	
			gdi.fillDown();
      gdi.addButton("OK3", "OK", SportIdentCB).setDefault();
      gdi.setInputFocus("ClassName", true);
			gdi.refresh();
			gdi.popX();
		}
		else if(bi.id=="OK3") {
      pCourse pc = 0;
      pClass pclass = 0;

      if (oe->getNumClasses() == 1 && oe->getClass(1) != 0 &&
              oe->getClass(1)->getType()=="tmp" && oe->getClass(1)->getNumRunners(false)==0) {
        pclass = oe->getClass(1);
        pclass->setType("");
        pclass->setName(gdi.getText("ClassName"));
        pc = pclass->getCourse();
        if (pc)
          pc->setName(gdi.getText("ClassName"));
      }
      
      if (pc == 0) {
			  pc=gEvent->addCourse(gdi.getText("ClassName"));
			  for(unsigned i=0;i<activeSIC.nPunch; i++)
				  pc->addControl(activeSIC.Punch[i].Code);
      }
      if (pclass == 0) {
			  pclass=gEvent->addClass(gdi.getText("ClassName"), pc->getId());
      }
      else
        pclass->setCourse(pc);
      int year = 0;
			pRunner r=gEvent->addRunner(gdi.getText("Runners"), gdi.getText("Club"), 
                                  pclass->getId(), activeSIC.CardNumber, year, true);

			r->setStartTimeS(gdi.getText("StartTime"));
			r->setCardNo(activeSIC.CardNumber, false);
			gdi.restore();
      SICard copy_sic = activeSIC;
			activeSIC.clear(&activeSIC);
      processCard(gdi, r, copy_sic);
		}
		else if (bi.id=="OK4")	{
			//Existing runner in existing class...

			ListBoxInfo lbi;
			gdi.getSelectedItem("Classes", &lbi);
			
			if(lbi.data==0 || lbi.data==-1)
			{
				gdi.alert("Du m�ste v�lja en klass");
				return 0;
			}
			
			DWORD rid;
			pRunner r;

			if(gdi.getData("RunnerId", rid) && rid>0)
				r = gEvent->getRunner(rid, 0);
			else r = gEvent->addRunner(lang.tl("Oparad bricka"), lang.tl("Ok�nd"), 0, 0, 0, false);

			r->setClassId(lbi.data);

			gdi.restore();				
      SICard copy = activeSIC;
			activeSIC.clear(&activeSIC);
      processCard(gdi, r, copy);      
		}
    else if (bi.id=="EntryOK") {
      oe->synchronizeList(oLRunnerId, true, false);
      oe->synchronizeList(oLCardId, false, true);
  
      string name=gdi.getText("Name");
      if(name.empty()) {
        gdi.alert("Alla deltagare m�ste ha ett namn.");
        return 0;
      }
      pRunner r = pRunner(bi.getExtra());
      int cardNo = gdi.getTextNo("CardNo");
      
      pRunner cardRunner = oe->getRunnerByCard(cardNo, true);
      if (cardNo>0 && cardRunner!=0 && cardRunner!=r) {
        gdi.alert("Bricknummret �r upptaget (X).#" + cardRunner->getName() + ", " + cardRunner->getClass());
        return 0;
      }
  
      ListBoxInfo lbi;
	    gdi.getSelectedItem("Class", &lbi);

      if(signed(lbi.data)<=0) {
        pClass pc = oe->getClassCreate(0, lang.tl("�ppen klass"));
        lbi.data = pc->getId();        
        pc->setAllowQuickEntry(true);
        pc->synchronize();
      }
      bool updated = false;
      int year = 0;

      if (r==0)
        r = oe->addRunner(name, gdi.getText("Club"), lbi.data, cardNo, year, true);
      else {
        pClub club = oe->getClubCreate(0, gdi.getText("Club"));
        int birthYear = 0;
        r->updateFromDB(name, club->getId(), lbi.data, cardNo, birthYear);
        r->setName(name);
        r->setClubId(club->getId());
        r->setClassId(lbi.data);
        updated = true;
      }

      lastClubId=r->getClubId();
      lastClassId=r->getClassId();
      lastFee=oe->interpretCurrency(gdi.getText("Fee"));
      
	    r->setCardNo(cardNo, true);
      
      oDataInterface di=r->getDI();

      di.setInt("Fee", lastFee);

      di.setInt("CardFee", gdi.isChecked("RentCard") ? oe->getDI().getInt("CardFee") : 0);
      di.setInt("Paid", gdi.isChecked("Paid") ? di.getInt("Fee")+di.getInt("CardFee") : 0);
      di.setString("Phone", gdi.getText("Phone"));

  	  r->setStartTimeS(gdi.getText("StartTime"));
      
      string bib = "";

      if (r->autoAssignBib())
        bib = ", " + lang.tl("Nummerlapp: ") + r->getBib();
      
      r->synchronize();

      gdi.restore("EntryLine");

      char bf[256];
      sprintf_s(bf, "(SI: %d), %s, %s", r->getCardNo(), r->getClub().c_str(), 
                  r->getClass().c_str());

      string info(bf);
      if(r->getDI().getInt("CardFee") != 0)
        info+=lang.tl(", Hyrbricka");

      if(r->getDI().getInt("Paid")>0)
        info+=lang.tl(", Betalat");
      
      if (bib.length()>0)
        info+=bib;

      if (updated)
        info += lang.tl(" [Uppdaterad anm�lan]");

      gdi.pushX();
      gdi.fillRight();
      gdi.addString("ChRunner", 0, r->getName(), SportIdentCB).setColor(colorGreen).setExtra(r);
      gdi.fillDown();      
      gdi.addString("", 0, info, 0);

      //gdi.addButton("ChangeRunner", "�ndra...", SportIdentCB, "");
      gdi.popX();
      //gdi.dropLine(3);
      gdi.setRestorePoint("EntryLine");
      generateEntryLine(gdi, 0);
    }
    else if(bi.id=="EntryCancel") {
      gdi.restore("EntryLine");
      generateEntryLine(gdi, 0);
    }
    else if (bi.id=="RentCard" || bi.id=="Paid") {
      updateEntryInfo(gdi);
    }
    else if (bi.id == "ManualOK") {
      if (runnerMatchedId == -1) 
        throw meosException("L�paren hittades inte");

      bool useNow = int(gdi.getExtra("FinishTime")) == 1;
      string time = useNow ? getLocalTimeOnly() : gdi.getText("FinishTime");

      int relTime = oe->getRelativeTime(time);
      if (relTime <= 0) {
        throw meosException("Ogiltig tid.");
      }
      bool ok = gdi.isChecked("StatusOK");
      bool dnf = gdi.isChecked("StatusDNF");

      pRunner r = oe->getRunner(runnerMatchedId, 0);
      if (r==0)
        throw meosException("L�paren hittades inte");

      if (r->getStatus() != StatusUnknown) {
        if (!gdi.ask("X har redan ett resultat. Vi du forts�tta?#" + r->getCompleteIdentification()))
          return 0;
      }

      gdi.restore("ManualInput", false);

      SICard sic;
      sic.runnerId = runnerMatchedId;
      sic.relativeFinishTime = relTime;
      sic.statusOK = ok;
      sic.statusDNF = dnf;

      gSI->AddCard(sic);
    }
    else if (bi.id == "StatusOK") {
      bool ok = gdi.isChecked(bi.id);
      if (ok) {
       gdi.check("StatusDNF", false);
      }
    }
    else if (bi.id == "StatusDNF") {
      bool dnf = gdi.isChecked(bi.id);
      gdi.setInputStatus("StatusOK", !dnf);
      gdi.check("StatusOK", !dnf);
    }
	}
	else if (type==GUI_LISTBOX) {
		ListBoxInfo bi=*(ListBoxInfo *)data;

		if (bi.id=="Runners") {
			pRunner r = gEvent->getRunner(bi.data, 0);
      if (r) {
        gdi.setData("RunnerId", bi.data);
				gdi.setText("Club", r->getClub());
        gdi.setText("FindMatch", lang.tl("Press Enter to continue"), true);
      }
		}
    else if (bi.id=="ComPort") {
			char bf[64];

      if(bi.text.substr(0,3)!="TCP")
			  sprintf_s(bf, 64, "COM%d", bi.data);
      else
        strcpy_s(bf, "TCP");
			
			if(gSI->IsPortOpen(bf))			
				gdi.setText("StartSI", lang.tl("Koppla ifr�n"));
			else
				gdi.setText("StartSI", lang.tl("Aktivera"));
		}
    else if (bi.id=="ReadType") {
      gdi.restore("SIPageLoaded");
      mode=SIMode(bi.data);
      if(mode==ModeAssignCards || mode==ModeEntry) {
        if(mode==ModeAssignCards) {
          gdi.dropLine(1);
          showAssignCard(gdi, true);
        }
        else {
          entryTips(gdi);
          generateEntryLine(gdi, 0);
        }
        gdi.setInputStatus("Interactive", mode == ModeAssignCards);
        gdi.setInputStatus("Database", mode != ModeAssignCards, true);
        gdi.disableInput("PrintSplits");
        gdi.disableInput("UseManualInput");
      }
      else if(mode==ModeReadOut) {
        gdi.enableInput("Interactive");
        gdi.enableInput("Database", true);
        gdi.enableInput("PrintSplits");
        gdi.enableInput("UseManualInput");
        gdi.fillDown();
        gdi.addButton("Import", "Importera fr�n fil...", SportIdentCB);

        if (gdi.isChecked("UseManualInput"))
          showManualInput(gdi);
      }
      gdi.refresh();
    }
    else if (bi.id=="Fee") {
      updateEntryInfo(gdi);    
    }
	}
  else if (type == GUI_LINK) {
    TextInfo ti = *(TextInfo *)data;
    if (ti.id == "ChRunner") {
      pRunner r = pRunner(ti.getExtra());
      generateEntryLine(gdi, r);
    }
    else if (ti.id == "EditAssign") {
      int id = ti.getExtraInt();
      pRunner r = oe->getRunner(id, 0);
      if (r) {
        gdi.setText("CardNo", r->getCardNo());
        gdi.setText("RunnerId", r->getRaceIdentifier());
        gdi.setText("FindMatch", r->getCompleteIdentification(), true);
        runnerMatchedId = r->getId();
      }
    }
  }
  else if (type == GUI_COMBO) {
    ListBoxInfo bi=*(ListBoxInfo *)data;

    if (bi.id=="Fee") {
      updateEntryInfo(gdi);    
    }
    else if (bi.id == "Runners") {
      DWORD rid;
      if((gdi.getData("RunnerId", rid) && rid>0) || !gdi.getText("Club").empty())
        return 0; // Selected from list

      if (!bi.text.empty() && useDatabase) {
        pRunner db_r = oe->dbLookUpByName(bi.text, 0, 0, 0);
        if (!db_r && lastClubId)
          db_r = oe->dbLookUpByName(bi.text, lastClubId, 0, 0);

        if (db_r) {
          gdi.setText("Club", db_r->getClub());
        }
      }
      gdi.setText("FindMatch", lang.tl("Press Enter to continue"), true);
       
    }
  }
  else if (type == GUI_COMBOCHANGE) {
    ListBoxInfo bi=*(ListBoxInfo *)data;
    if (bi.id == "Runners") {
      inputId++;
      gdi.addTimeoutMilli(300, "AddRunnerInteractive", SportIdentCB).setExtra(inputId);
    }
  }
  else if (type == GUI_EVENT) {
    EventInfo ev = *(EventInfo *)data;
    if (ev.id == "AutoComplete") {
      pRunner r = oe->getRunner(runnerMatchedId, 0);
      if (r) {
        gdi.setInputFocus("OK1");
        gdi.setText("Runners", r->getName());
        gdi.setData("RunnerId", runnerMatchedId);
        gdi.setText("Club", r->getClub());
        inputId = -1;
        gdi.setText("FindMatch", lang.tl("Press Enter to continue"), true);
        
      }
    }
  }
  else if (type == GUI_FOCUS) {
    InputInfo &ii=*(InputInfo *)data;
    
    if (ii.id == "FinishTime") {
      if (ii.getExtraInt() == 1) {
        ii.setExtra(0);
        ii.setFgColor(colorDefault);
        //gdi.refreshFast();
        gdi.setText(ii.id, "", true);
      }
    }
  }
  else if (type == GUI_TIMER) {
    TimerInfo &ti = *(TimerInfo *)(data);
    
    if (ti.id == "TieCard") {
      runnerMatchedId = ti.getExtraInt();
      tieCard(gdi);
      return 0;
    }

    if (inputId != ti.getExtraInt())
      return 0;

    if (ti.id == "RunnerId") {
      const string &text = gdi.getText(ti.id);
      int nr = atoi(text.c_str());

      pRunner r = 0;
      if (nr > 0) {
        r = getRunnerByIdentifier(nr);
        if (r == 0) {
          r = oe->getRunnerByBibOrStartNo(text, true);
          if (r == 0) {
            // Seek where a card is already defined
            r = oe->getRunnerByBibOrStartNo(text, false);
          }
        }
      }

      if (nr == 0 && text.size() > 2) {
        stdext::hash_set<int> f1, f2;
        r = oe->findRunner(text, 0, f1, f2);
      }
      if (r != 0) {
        gdi.setText("FindMatch", r->getCompleteIdentification(), true);
        runnerMatchedId = r->getId();
      }
      else {
        gdi.setText("FindMatch", "", true);
        runnerMatchedId = -1;
      }

      gdi.setInputStatus("TieOK", runnerMatchedId != -1);

      if (runnerMatchedId != -1 && gdi.getTextNo("CardNo") > 0 && gdi.isChecked("AutoTie"))
        tieCard(gdi);
    }
    else if (ti.id == "Manual") {
      const string &text = gdi.getText(ti.id);
      int nr = atoi(text.c_str());

      pRunner r = 0;
      if (nr > 0) {
        r = oe->getRunnerByBibOrStartNo(text, false);
        if (r == 0)
          r = oe->getRunnerByCard(nr, true, true);
      }

      if (nr == 0 && text.size() > 2) {
        stdext::hash_set<int> f1, f2;
        r = oe->findRunner(text, 0, f1, f2);
      }
      if (r != 0) {
        gdi.setText("FindMatch", r->getCompleteIdentification(), true);
        runnerMatchedId = r->getId();
      }
      else {
        gdi.setText("FindMatch", "", true);
        runnerMatchedId = -1;
      }
    }
    else if (ti.id == "AddRunnerInteractive") {
      const string &text = gdi.getText("Runners");
      int nr = atoi(text.c_str());

      pRunner r = 0;
      if (nr > 0) {
        r = oe->getRunnerByBibOrStartNo(text, true);
      }

      if (nr == 0 && text.size() > 2) {
        stdext::hash_set<int> f1, f2;
        r = oe->findRunner(text, 0, f1, f2);
      }
      if (r != 0) {
        gdi.setText("FindMatch", lang.tl("X (press Ctrl+Space to confirm)#" + r->getCompleteIdentification()), true);
        runnerMatchedId = r->getId();
      }
      else {
        gdi.setText("FindMatch", "", true);
        runnerMatchedId = -1;
      }
    }
  }
  else if(type==GUI_INPUTCHANGE) {

		InputInfo ii=*(InputInfo *)data;
    if (ii.id == "RunnerId") {
      inputId++;
      gdi.addTimeoutMilli(300, ii.id, SportIdentCB).setExtra(inputId);
    }
    else if (ii.id == "Manual") {
      inputId++;
      gdi.addTimeoutMilli(300, ii.id, SportIdentCB).setExtra(inputId);
    }
    else if (ii.id == "CardNo" && mode == ModeAssignCards) {
      gdi.setInputStatus("TieOK", runnerMatchedId != -1);
    }
  }
  else if (type==GUI_INPUT) {
    InputInfo &ii=*(InputInfo *)data;
    if (ii.id == "FinishTime") {
      if (ii.text.empty()) {
        ii.setExtra(1);
        ii.setFgColor(colorGreyBlue);
        gdi.setText(ii.id, lang.tl("Aktuell tid"), true);
      }
    }
    else if (ii.id=="CardNo") {
      int cardNo = gdi.getTextNo("CardNo");

      if (mode == ModeAssignCards) {
        if (runnerMatchedId != -1 && gdi.isChecked("AutoTie") && cardNo>0)
          gdi.addTimeoutMilli(50, "TieCard", SportIdentCB).setExtra(runnerMatchedId);
      }
      else if (cardNo>0 && gdi.getText("Name").empty()) {
        SICard sic;
        sic.clear(0);
        sic.CardNumber = cardNo;
        
        entryCard(gdi, sic);
      }
    }
    else if(ii.id[0]=='*') {
      int si=atoi(ii.text.c_str());
      
      pRunner r=oe->getRunner(int(ii.getExtra()), 0);
      r->synchronize();

      if(r && r->getCardNo()!=si) {
        if (si==0 || !oe->checkCardUsed(gdi, si)) {       
          r->setCardNo(si, false);
          r->getDI().setInt("CardFee", oe->getDI().getInt("CardFee"));
          r->synchronize();
        }

        if(r->getCardNo())
          gdi.setText(ii.id, r->getCardNo());
        else
          gdi.setText(ii.id, "");
      }
    }
  }
	else if (type==GUI_INFOBOX) {
		DWORD loaded;
		if(!gdi.getData("SIPageLoaded", loaded))
			loadPage(gdi);
	}

	return 0;
}


void TabSI::refillComPorts(gdioutput &gdi)
{
	if(!gSI) return;

	list<int> ports;
	gSI->EnumrateSerialPorts(ports);
  
	gdi.clearList("ComPort");
	ports.sort();
	char bf[256];
	int active=0;
	int inactive=0;
	while(!ports.empty())
	{
		int p=ports.front();
		sprintf_s(bf, 256, "COM%d", p);

		if(gSI->IsPortOpen(bf)){
			gdi.addItem("ComPort", string(bf)+" [OK]", p);
			active=p;
		}
		else{
			gdi.addItem("ComPort", bf, p);
			inactive=p;
		}

		ports.pop_front();
	}

  if(gSI->IsPortOpen("TCP"))
    gdi.addItem("ComPort", "TCP [OK]");
  else
    gdi.addItem("ComPort", "TCP");

	if(active){
		gdi.selectItemByData("ComPort", active);
		gdi.setText("StartSI", lang.tl("Koppla ifr�n"));
	}
	else{
		gdi.selectItemByData("ComPort", inactive);
		gdi.setText("StartSI", lang.tl("Aktivera"));
	}
}

void TabSI::showReadPunches(gdioutput &gdi, vector<PunchInfo> &punches, set<string> &dates)
{
  char bf[64];
  int yp = gdi.getCY();
  int xp = gdi.getCX();
  dates.clear();
  for (size_t k=0;k<punches.size(); k++) {
    sprintf_s(bf, "%d.", k+1);
    gdi.addStringUT(yp, xp, 0, bf);

    pRunner r = oe->getRunnerByCard(punches[k].card);
    sprintf_s(bf, "%d", punches[k].card);
    gdi.addStringUT(yp, xp+40, 0, bf, 240);

    if (r!=0)
      gdi.addStringUT(yp, xp+100, 0, r->getName(), 170);
 
    if (punches[k].date[0] != 0) {
      gdi.addStringUT(yp, xp+280, 0, punches[k].date, 75);
      dates.insert(punches[k].date);
    }
    if (punches[k].time>0)
      gdi.addStringUT(yp, xp+360, 0, oe->getAbsTime(punches[k].time));
    else
      gdi.addStringUT(yp, xp+360, 0, MakeDash("-"));

    yp += gdi.getLineHeight();
  }
}

void TabSI::showReadCards(gdioutput &gdi, vector<SICard> &cards)
{
  char bf[64];
  int yp = gdi.getCY();
  int xp = gdi.getCX();
  for (size_t k=0;k<cards.size(); k++) {
    sprintf_s(bf, "%d.", k+1);
    gdi.addStringUT(yp, xp, 0, bf);

    pRunner r = oe->getRunnerByCard(cards[k].CardNumber);
    sprintf_s(bf, "%d", cards[k].CardNumber);
    gdi.addStringUT(yp, xp+40, 0, bf, 240);

    if (r!=0)
      gdi.addStringUT(yp, xp+100, 0, r->getName(), 240);
   
    gdi.addStringUT(yp, xp+300, 0, oe->getAbsTime(cards[k].FinishPunch.Time));
    yp += gdi.getLineHeight();
  }
}

SportIdent &TabSI::getSI(gdioutput &gdi) {
  if (!gSI) {
    HWND hWnd=gdi.getMain();
		gSI = new SportIdent(hWnd, 0);
		gSI->SetZeroTime(gEvent->getZeroTimeNum());
	}
  return *gSI;
}

bool TabSI::loadPage(gdioutput &gdi) {
 	gdi.clearPage(true);
  gdi.selectTab(tabId);
  oe->checkDB();
	gdi.setData("SIPageLoaded", 1);

	if(!gSI) {
		getSI(gdi);
    if (oe->isClient())
      interactiveReadout = false;
	}
#ifdef _DEBUG
	gdi.fillRight();
	gdi.pushX();
	gdi.addInput("SI", "", 10, 0, "SI");
  int s = 3600+(rand()%60)*60;
  int f = s + 1000 + rand()%400;
  gdi.addInput("Start", oe->getAbsTime(s), 6, 0, "Start");
  gdi.addInput("Finish", oe->getAbsTime(f), 6, 0, "M�l");
  gdi.addInput("C1", "33", 5, 0, "#C1");
  gdi.addInput("C2", "34", 5, 0, "#C2");
  gdi.addInput("C3", "45", 5, 0, "#C3");
  gdi.addInput("C4", "50", 5, 0, "#C4");
  gdi.addInput("C5", "61", 5, 0, "#C5");
  gdi.addInput("C6", "62", 5, 0, "#C6");
  gdi.addInput("C7", "67", 5, 0, "#C7");

  gdi.addInput("C8", "100", 5, 0, "#C8");
  gdi.dropLine();
	gdi.addButton("Save", "Spara", SportIdentCB);
  gdi.fillDown();
  
  gdi.addButton("SaveP", "St�mpling", SportIdentCB);
  gdi.popX();
#endif
	gdi.addString("", boldLarge, "SportIdent");
	gdi.dropLine();

	gdi.pushX();
	gdi.fillRight();
	gdi.addSelection("ComPort", 120, 200, SportIdentCB);
  gdi.addButton("StartSI", "#Aktivera+++", SportIdentCB);
	gdi.addButton("SIInfo", "Info", SportIdentCB);
 
	refillComPorts(gdi);
	
	gdi.addButton("AutoDetect", "S�k och starta automatiskt...", SportIdentCB);
  gdi.addButton("PrinterSetup", "Str�cktidsutskrift...", SportIdentCB, "Skrivarinst�llningar f�r str�cktider");

  gdi.popX();
	gdi.fillDown();
	gdi.dropLine(2);
	
  gdi.fillRight();
  gdi.addSelection("ReadType", 180, 200, SportIdentCB);
  gdi.addItem("ReadType", lang.tl("Avl�sning/radiotider"), ModeReadOut);
  gdi.addItem("ReadType", lang.tl("Tilldela hyrbrickor"), ModeAssignCards);
  gdi.addItem("ReadType", lang.tl("Anm�lningsl�ge"), ModeEntry);
  gdi.selectItemByData("ReadType", mode);
  //mode=ModeReadOut;

  gdi.addCheckbox("Interactive", "Interaktiv inl�sning", SportIdentCB, interactiveReadout);
  if (oe->useRunnerDb())
    gdi.addCheckbox("Database", "Anv�nd l�pardatabasen", SportIdentCB, useDatabase);
  
  gdi.addCheckbox("PrintSplits", "Str�cktidsutskrift[check]", SportIdentCB, printSplits);
  gdi.addCheckbox("UseManualInput", "Manuell inmatning", SportIdentCB, manualInput);
  gdi.fillDown();

  gdi.popX();
  gdi.dropLine(2);
  gdi.setRestorePoint("SIPageLoaded");

  if(mode==ModeReadOut) {
    gdi.addButton("Import", "Importera fr�n fil...", SportIdentCB);
 
    gdi.setRestorePoint("Help");
	  gdi.addString("", 10, "help:471101");

    if (gdi.isChecked("UseManualInput"))
      showManualInput(gdi);

    gdi.dropLine();    
  }
  else if(mode==ModeAssignCards) {
    gdi.dropLine(1);
    showAssignCard(gdi, true);
  }
  else {
    entryTips(gdi);
    generateEntryLine(gdi, 0);
    gdi.disableInput("Interactive");
    gdi.disableInput("PrintSplits");
    gdi.disableInput("UseManualInput");
  }

  // Unconditional clear
  activeSIC.clear(0);

  checkMoreCardsInQueue(gdi);
  gdi.refresh();
  return true;
}

void InsertSICard(gdioutput &gdi, SICard &sic)
{
  TabSI &tsi = dynamic_cast<TabSI &>(*gdi.getTabs().get(TSITab));
  tsi.insertSICard(gdi, sic);
}

pRunner TabSI::autoMatch(const SICard &sic, pRunner db_r)
{
  assert(useDatabase);
  //Look up in database.
	if(!db_r)
    db_r = gEvent->dbLookUpByCard(sic.CardNumber);

  pRunner r=0;

	if(db_r) {
		r = gEvent->getRunnerByName(db_r->getName(), db_r->getClub());

    if ( !r ) {
      vector<pClass> classes;
		  int dist = gEvent->findBestClass(sic, classes);

		  if(classes.size()==1 && dist>=-1 && dist<=1) { //Almost perfect match found. Assume it is it!
			  r = gEvent->addRunnerFromDB(db_r, classes[0]->getId(), true);
        r->setCardNo(sic.CardNumber, false);
		  }
		  else r=0; //Do not assume too much...
    }
	}
  if (r && r->getCard()==0)
    return r;
  else return 0;
}

void TabSI::insertSICard(gdioutput &gdi, SICard &sic)
{
  string msg;
  try {
    insertSICardAux(gdi, sic);    
  }
  catch(std::exception &ex) {
    msg = ex.what();
  }
  catch(...) {
    msg = "Ett ok�nt fel intr�ffade.";
  }
  
  if(!msg.empty())
    gdi.alert(msg);
}

void TabSI::insertSICardAux(gdioutput &gdi, SICard &sic)
{
  if (oe->isReadOnly()) {
    gdi.makeEvent("ReadCard", "insertSICard", sic.CardNumber, &sic, true);
    return;
  }

	DWORD loaded;
	bool pageLoaded=gdi.getData("SIPageLoaded", loaded);

  if (pageLoaded && manualInput)
    gdi.restore("ManualInput");

  if (!pageLoaded && !insertCardNumberField.empty()) {
    if (gdi.insertText(insertCardNumberField, itos(sic.CardNumber)))
      return;
  }

  if(mode==ModeAssignCards) {
    if(!pageLoaded) {
  		CardQueue.push_back(sic);
	  	gdi.addInfoBox("SIREAD", "Inl�st bricka st�lld i k�");
    }
    else assignCard(gdi, sic);
    return;
  }
  else if(mode==ModeEntry) {
    if(!pageLoaded) {
  		CardQueue.push_back(sic);
	  	gdi.addInfoBox("SIREAD", "Inl�st bricka st�lld i k�");
    }
    else entryCard(gdi, sic);
    return;
  }
	gEvent->synchronizeList(oLCardId, true, false);
	gEvent->synchronizeList(oLRunnerId, false, true);

	if (sic.PunchOnly) {
    processPunchOnly(gdi, sic);
    return;
	}
	pRunner r;
  if (sic.runnerId == 0)
    r = gEvent->getRunnerByCard(sic.CardNumber, false);
  else {
    r = gEvent->getRunner(sic.runnerId, 0);
    sic.CardNumber = r->getCardNo();
  }

  bool ReadBefore = sic.runnerId == 0 ? gEvent->isCardRead(sic) : false;

	if (!pageLoaded) {
    if (sic.runnerId != 0)
      throw meosException("Internal error");
		//SIPage not loaded...

    if(!r && useDatabase)
      r=autoMatch(sic, 0);

    // Assign a class if not already done
    autoAssignClass(r, sic);

    if(interactiveReadout) {
		  if (r && r->getClassId() && !ReadBefore)	{
			  //We can do a silent read-out...
			  processCard(gdi, r, sic, true);
			  return;
		  }
		  else {
			  CardQueue.push_back(sic);
			  gdi.addInfoBox("SIREAD", "info:readout_action#" + gEvent->getCurrentTimeS()+"#"+itos(sic.CardNumber), 0, SportIdentCB);
        return;
		  }
    }
    else {
      if (!ReadBefore) {
        if(r && r->getClassId())
          processCard(gdi, r, sic, true);
        else
          processUnmatched(gdi, sic, true);
      }
      else
        gdi.addInfoBox("SIREAD", "Brickan redan inl�st.", 0, SportIdentCB);
    }
    return;
	}
	else if (activeSIC.CardNumber)	{
    //We are already in interactive mode...
    
    // Assign a class if not already done
    autoAssignClass(r, sic);

    if (r && r->getClassId() && !ReadBefore)	{
			//We can do a silent read-out...
			processCard(gdi, r, sic, true);
			return;
		}
    
		string name;
		if(r) 
      name = " ("+r->getName()+")";

		//char bf[256];
		//sprintf_s(bf, 256, "SI-%d inl�st%s.\nBrickan har st�llts i k�.", sic.CardNumber, name.c_str());
    name = itos(sic.CardNumber) + name; 
		CardQueue.push_back(sic);
		//gdi.addInfoBox("SIREAD", gEvent->getCurrentTimeS()+": "+bf);
    gdi.addInfoBox("SIREAD", "info:readout_queue#" + gEvent->getCurrentTimeS()+ "#" + name);
		return;
	}

  if (ReadBefore) {
    //We stop processing of new cards, while working...
    // Thus cannot be in interactive mode
	  activeSIC=sic;
		char bf[256];
		
    if (interactiveReadout) {
      sprintf_s(bf, 256,  "SI X �r redan inl�st. Ska den l�sas in igen?#%d", sic.CardNumber);

		  if(!gdi.ask(bf)) {
        if (printSplits) {
          pRunner runner = oe->getRunnerByCard(sic.CardNumber);
          if (runner)
            generateSplits(runner, gdi);
        }
			  activeSIC.clear(0);
        if (manualInput)
          showManualInput(gdi);
        checkMoreCardsInQueue(gdi);
			  return;
		  }
    }
    else {
      if (printSplits) {
        pRunner runner = oe->getRunnerByCard(sic.CardNumber);
        if (runner)
          generateSplits(runner, gdi);
      }

      gdi.dropLine();
      sprintf_s(bf, 256,  "SI X �r redan inl�st. Anv�nd interaktiv inl�sning om du vill l�sa brickan igen.#%d", sic.CardNumber);
      gdi.addString("", 0, bf).setColor(colorRed);
      gdi.dropLine();
      gdi.scrollToBottom();
      gdi.refresh();
      activeSIC.clear(0);
      checkMoreCardsInQueue(gdi);
			return;
    }
	}

	pRunner db_r = 0;
  if (sic.runnerId == 0) {
    r = gEvent->getRunnerByCard(sic.CardNumber, !ReadBefore);

	  if (!r && useDatabase) {
		  //Look up in database.
		  db_r = gEvent->dbLookUpByCard(sic.CardNumber);
		  if (db_r)
        r = autoMatch(sic, db_r);
	  }
  }

  // If there is no class, auto create
  if(interactiveReadout && oe->getNumClasses()==0) {
    gdi.fillDown();
    gdi.dropLine();
    gdi.addString("", 1, "Skapar saknad klass").setColor(colorGreen);
    gdi.dropLine();
    pCourse pc=gEvent->addCourse(lang.tl("Ok�nd klass"));
	  for(unsigned i=0;i<sic.nPunch; i++)
		  pc->addControl(sic.Punch[i].Code);
    gEvent->addClass(lang.tl("Ok�nd klass"), pc->getId())->setType("tmp");
  }

  // Assign a class if not already done   
  autoAssignClass(r, sic);

  if(r && r->getClassId()) {
    SICard copy = sic;
    activeSIC.clear(0); 
		processCard(gdi, r, copy); //Everyting is OK
    if (gdi.isChecked("UseManualInput"))
      showManualInput(gdi);
  }
  else {
    if(interactiveReadout) {
      startInteractive(gdi, sic, r, db_r);
    }
    else {
      SICard copy = sic;
      activeSIC.clear(0); 
		  processUnmatched(gdi, sic, !pageLoaded);
    }
  }
}

void TabSI::startInteractive(gdioutput &gdi, const SICard &sic, pRunner r, pRunner db_r)
{
	if (!r) {
		gdi.setRestorePoint();
    gdi.fillDown();
    gdi.dropLine();
		char bf[256];
    sprintf_s(bf, 256, "SI X inl�st. Brickan �r inte knuten till n�gon l�pare (i skogen).#%d", sic.CardNumber);

		gdi.dropLine();
		gdi.addString("", 1, bf);
		gdi.dropLine();
    gdi.fillRight();
		gdi.pushX();

		gdi.addCombo("Runners", 200, 300, SportIdentCB, "Namn:");
    gEvent->fillRunners(gdi, "Runners", false, oEvent::RunnerFilterOnlyNoResult); 

		if(db_r){
			gdi.setText("Runners", db_r->getName()); //Data from DB
		}
		else if(sic.FirstName[0] || sic.LastName[0]){ //Data from SI-card
			gdi.setText("Runners", string(sic.FirstName)+" "+sic.LastName);
		}

		gdi.addCombo("Club", 200, 300, 0, "Klubb:");
		gEvent->fillClubs(gdi, "Club");

		if (db_r)
			gdi.setText("Club", db_r->getClub()); //Data from DB
		   
    if (gdi.getText("Runners").empty())
      gdi.setInputFocus("Runners");
    else
      gdi.setInputFocus("Club");
    //Process this card.
  	activeSIC=sic;
		gdi.dropLine();
    gdi.setRestorePoint("restOK1");
		gdi.addButton("OK1", "OK", SportIdentCB).setDefault();
		gdi.fillDown();
		gdi.addButton("Cancel", "Avbryt", SportIdentCB).setCancel();
		gdi.popX();
    gdi.addString("FindMatch", 0, "").setColor(colorGreen);
    gdi.registerEvent("AutoComplete", SportIdentCB).setKeyCommand(KC_AUTOCOMPLETE);
    gdi.dropLine();
    gdi.scrollToBottom();
		gdi.refresh();
	}
	else {
    //Process this card.
  	activeSIC=sic;

		//No class. Select...
		gdi.setRestorePoint();

		char bf[256];
    sprintf_s(bf, 256, "SI X inl�st. Brickan tillh�r Y som saknar klass.#%d#%s", 
              sic.CardNumber, r->getName().c_str());

		gdi.dropLine();
		gdi.addString("", 1, bf);

		gdi.fillRight();
		gdi.pushX();
		
		gdi.addSelection("Classes", 200, 300, 0, "Klass:");
		gEvent->fillClasses(gdi, "Classes", oEvent::extraNone, oEvent::filterNone);
		gdi.setInputFocus("Classes");
		//Find matching class...
    vector<pClass> classes;
		gEvent->findBestClass(sic, classes);
		if(classes.size() > 0)	
      gdi.selectItemByData("Classes", classes[0]->getId());

		gdi.dropLine();

		gdi.addButton("OK4", "OK", SportIdentCB).setDefault();
		gdi.fillDown();

		gdi.popX();
		gdi.setData("RunnerId", r->getId());
    gdi.scrollToBottom();
		gdi.refresh();
	}
}

// Insert card without converting times and with/without runner
void TabSI::processInsertCard(const SICard &sic)
{
  if (oe->isCardRead(sic))
    return;

  pRunner runner = oe->getRunnerByCard(sic.CardNumber, true);
	pCard card = oe->allocateCard(runner);
	card->setReadId(sic);
	card->setCardNo(sic.CardNumber);

	if(sic.CheckPunch.Code!=-1)
		card->addPunch(oPunch::PunchCheck, sic.CheckPunch.Time, 0);

	if(sic.StartPunch.Code!=-1)
		card->addPunch(oPunch::PunchStart, sic.StartPunch.Time, 0);

	for(unsigned i=0;i<sic.nPunch;i++)
		card->addPunch(sic.Punch[i].Code, sic.Punch[i].Time, 0);

	if(sic.FinishPunch.Code!=-1)
		card->addPunch(oPunch::PunchFinish, sic.FinishPunch.Time,0 );

  //Update to SQL-source
	card->synchronize();

  if (runner) {
	  vector<int> mp;
	  runner->addPunches(card, mp);
  }
}

bool TabSI::processUnmatched(gdioutput &gdi, const SICard &csic, bool silent)
{
	SICard sic(csic);
	pCard card=gEvent->allocateCard(0);

	card->setReadId(csic);
	card->setCardNo(csic.CardNumber);
	
	char bf[16];
	_itoa_s(sic.CardNumber, bf, 16, 10);
	string cardno(bf);

	string info=lang.tl("Ok�nd bricka ") + cardno + ".";
	string warnings;

  // Write read card to log
  logCard(sic);

	// Convert punch times to relative times.
	gEvent->convertTimes(sic);

	if(sic.CheckPunch.Code!=-1)
		card->addPunch(oPunch::PunchCheck, sic.CheckPunch.Time, 0);

	if(sic.StartPunch.Code!=-1)
		card->addPunch(oPunch::PunchStart, sic.StartPunch.Time, 0);

	for(unsigned i=0;i<sic.nPunch;i++)
		card->addPunch(sic.Punch[i].Code, sic.Punch[i].Time, 0);

	if(sic.FinishPunch.Code!=-1)
		card->addPunch(oPunch::PunchFinish, sic.FinishPunch.Time, 0);
	else
		warnings+=lang.tl("M�lst�mpling saknas.");

	//Update to SQL-source
	card->synchronize();
	
  RECT rc;
  rc.left=15;
  rc.right=gdi.getWidth()-10;
  rc.top=gdi.getCY()+gdi.getLineHeight()-5;
  rc.bottom=rc.top+gdi.getLineHeight()*2+14;
	
  if(!silent) {
    gdi.fillDown();
		//gdi.dropLine();
    gdi.addRectangle(rc, colorLightRed, true);
		gdi.addStringUT(rc.top+6, rc.left+20, 1, info);
    //gdi.dropLine();
    if (gdi.isChecked("UseManualInput"))
      showManualInput(gdi);

		gdi.scrollToBottom();
	}
	else {
    gdi.addInfoBox("SIINFO", "#" + info, 10000);
	}
	gdi.makeEvent("DataUpdate", "sireadout", 0, 0, true);		

	checkMoreCardsInQueue(gdi);
	return true;
}

void TabSI::rentCardInfo(gdioutput &gdi, int width)
{
  RECT rc;
  rc.left=15;
  rc.right=rc.left+width;
  rc.top=gdi.getCY()-7;
  rc.bottom=rc.top+gdi.getLineHeight()+5;

  gdi.addRectangle(rc, colorYellow, true);
  gdi.addString("", rc.top+2, rc.left+width/2, 1|textCenter, "V�nligen �terl�mna hyrbrickan.");			
}

bool TabSI::processCard(gdioutput &gdi, pRunner runner, const SICard &csic, bool silent)
{
	if(!runner)
		return false;
  if (runner->getClubId())
    lastClubId = runner->getClubId();
      
  runner = runner->getMatchedRunner(csic);

  int lh=gdi.getLineHeight();
	//Update from SQL-source
	runner->synchronize();

	if(!runner->getClassId())
		runner->setClassId(gEvent->addClass(lang.tl("Ok�nd klass"))->getId());

  // Choose course from pool
  pClass cls=gEvent->getClass(runner->getClassId());
  if (cls && cls->hasCoursePool()) {
    unsigned leg=runner->legToRun();

    if (leg<cls->getNumStages()) {
      pCourse c = cls->selectCourseFromPool(leg, csic);
      if (c)
        runner->setCourseId(c->getId());
    }
  }

  pClass pclass=gEvent->getClass(runner->getClassId());
  if (!runner->getCourse(false) && !csic.isManualInput()) {
    
    if(pclass && !pclass->hasMultiCourse() && !pclass->hasDirectResult()) {
		  pCourse pcourse=gEvent->addCourse(runner->getClass());		
		  pclass->setCourse(pcourse);		
  		
		  for(unsigned i=0;i<csic.nPunch; i++)
			  pcourse->addControl(csic.Punch[i].Code);

		  char msg[256];

      sprintf_s(msg, lang.tl("Skapade en bana f�r klassen %s med %d kontroller fr�n brickdata (SI-%d)").c_str(),
				  pclass->getName().c_str(), csic.nPunch, csic.CardNumber);

		  if(silent)
        gdi.addInfoBox("SIINFO", string("#") + msg, 15000);
		  else
			  gdi.addStringUT(0, msg);
    }
    else {
      if (!(pclass && pclass->hasDirectResult())) {
        const char *msg="L�pare saknar klass eller bana";

		    if(silent)
			    gdi.addInfoBox("SIINFO", msg, 15000);
		    else
			    gdi.addString("", 0, msg);
      }
    }
	}

  pCourse pcourse=runner->getCourse(false);

  if(pcourse) 
    pcourse->synchronize();
  else if (pclass && pclass->hasDirectResult())
    runner->setStatus(StatusOK, true, false, false);
	//silent=true;
	SICard sic(csic);
  string info, warnings, cardno;
	vector<int> MP;

  if (!csic.isManualInput()) {
	  pCard card=gEvent->allocateCard(runner);

	  card->setReadId(csic);
	  card->setCardNo(sic.CardNumber);
  	
	  char bf[16];
	  _itoa_s(sic.CardNumber, bf, 16, 10);
	  cardno = bf;

	  info=runner->getName() + " (" + cardno + "),   " + runner->getClub() + ",   " + runner->getClass();
	  
    // Write read card to log
    logCard(sic);

	  // Convert punch times to relative times.
	  oe->convertTimes(sic);

	  if(sic.CheckPunch.Code!=-1)
		  card->addPunch(oPunch::PunchCheck, sic.CheckPunch.Time,0);

	  if(sic.StartPunch.Code!=-1)
		  card->addPunch(oPunch::PunchStart, sic.StartPunch.Time,0);

	  for(unsigned i=0;i<sic.nPunch;i++)
		  card->addPunch(sic.Punch[i].Code, sic.Punch[i].Time,0);

	  if(sic.FinishPunch.Code!=-1)
		  card->addPunch(oPunch::PunchFinish, sic.FinishPunch.Time,0);
	  else
		  warnings+=lang.tl("M�lst�mpling saknas.");

    card->synchronize();
	  runner->addPunches(card, MP);
  }
  else {
    //Manual input
    info = runner->getName() + ",   " + runner->getClub() + ",   " + runner->getClass();
    runner->setCard(0);
	  
    if (csic.statusOK) {
      runner->setStatus(StatusOK, true, false);
      runner->setFinishTime(csic.relativeFinishTime);
    }
    else if (csic.statusDNF) {
      runner->setStatus(StatusDNF, true, false);
      runner->setFinishTime(0);
    }
    else {
      runner->setStatus(StatusMP, true, false);
      runner->setFinishTime(csic.relativeFinishTime);
    }

    cardno = MakeDash("-");
    runner->evaluateCard(true, MP, false, false);
  }

	//Update to SQL-source
	runner->synchronize();

  RECT rc;
  rc.left=15;
  rc.right=gdi.getWidth()-10;
  rc.top=gdi.getCY()+gdi.getLineHeight()-5;
  rc.bottom=rc.top+gdi.getLineHeight()*2+14;
	
  if(!warnings.empty())
    rc.bottom+=gdi.getLineHeight();

  if(runner->getStatus()==StatusOK)	{
    gEvent->calculateResults(oEvent::RTClassResult);
    if (runner->getTeam()) 
      gEvent->calculateTeamResults(runner->getLegNumber(), false);
    string placeS = runner->getTeam() ? runner->getTeam()->getLegPlaceS(runner->getLegNumber(), false) : runner->getPlaceS();

		if(!silent) {
      gdi.fillDown();
			//gdi.dropLine();
      gdi.addRectangle(rc, colorLightGreen, true);

      gdi.addStringUT(rc.top+6, rc.left+20, 1, info);			
      if(!warnings.empty()) 
        gdi.addStringUT(rc.top+6+2*lh, rc.left+20, 0, warnings);

      string statusline = lang.tl("Status OK,    ") +
                          lang.tl("Tid: ") + runner->getRunningTimeS() + 
                          lang.tl(",      Prel. placering: ") + placeS;

			if (runner->getCourse(false)->hasRogaining())
				statusline += lang.tl(",     Po�ng: ") + itos(runner->getRogainingPoints());
			else
      statusline += lang.tl(",     Prel. bomtid: ") + runner->getMissedTimeS();
			gdi.addStringUT(rc.top+6+lh, rc.left+20, 0, statusline);
      
      if(runner->getDI().getInt("CardFee") != 0)
        rentCardInfo(gdi, rc.right-rc.left);
      gdi.scrollToBottom();
		}
		else {
      string msg="#" + runner->getName()  + " (" + cardno + ")\n"+
					runner->getClub()+". "+runner->getClass() + 
					"\n" + lang.tl("Tid: ") + runner->getRunningTimeS() + lang.tl(", Plats: ") + placeS;
			if (runner->getCourse(false)->hasRogaining())
				msg += lang.tl(", Po�ng: ") + itos(runner->getRogainingPoints());
			gdi.addInfoBox("SIINFO", msg, 10000);
		}
	}
	else {
		string msg=lang.tl("Status")+": "+ lang.tl(runner->getStatusS());
		
		if(!MP.empty())	{
			msg=msg+", (";
			vector<int>::iterator it;
			char bf[32];

			for(it=MP.begin(); it!=MP.end(); ++it){
				_itoa_s(*it, bf, 32, 10);
				msg=msg+bf+" ";
			}
			msg+=" "+lang.tl("saknas")+".)";
		}

		if(!silent)	{
      gdi.fillDown();
			gdi.dropLine();
      gdi.addRectangle(rc, colorLightRed, true);

			gdi.addStringUT(rc.top+6, rc.left+20, 1, info);
			if(!warnings.empty())
        gdi.addStringUT(rc.top+6+lh*2, rc.left+20, 1, warnings);

			gdi.addStringUT(rc.top+6+lh, rc.left+20, 0, msg);
      
      if(runner->getDI().getInt("CardFee") != 0)
        rentCardInfo(gdi, rc.right-rc.left);

			gdi.scrollToBottom();
		}
		else {
      string statusmsg="#" + runner->getName()  + " (" + cardno + ")\n"+
					runner->getClub()+". "+runner->getClass() + 
					"\n" + msg;

			gdi.addInfoBox("SIINFO", statusmsg, 10000);
		}
	}

  tabForceSync(gdi, gEvent);
  gdi.makeEvent("DataUpdate", "sireadout", runner ? runner->getId() : 0, 0, true);		

  // Print splits
  if (printSplits)
    generateSplits(runner, gdi);

  // Print labels
	if (oe->getDCI().getInt("Labels"))
		generateLabel(runner, gdi);

  activeSIC.clear(&csic);

	checkMoreCardsInQueue(gdi);
	return true;
}
/*
void TabSI::handleDirectResultClass(oFreePunch &ofp) {
  pRunner r = ofp.getTiedRunner();
    
  if (ofp.getTypeCode() == oPunch::PunchFinish && r 
      && r->getClassRef() && r->getClassRef()->hasDirectResult()) {
    RunnerStatus st = r->getStatus();
    if (r->getCourse(false) == 0 && r->getCard() == 0 && r->getRunningTime() > 0 && 
        (st == StatusUnknown || st == StatusDNS)) {
      r->setStatus(StatusOK, true, false, true);
      r->synchronize(true);
    }
    else if (r->getCourse(false) != 0 && r->getCard() == 0) {
      pCard card = oe->allocateCard(r);
      card->setupFromRadioPunches(r);
      vector<int> mp;
      card->synchronize();
      r->addPunches(card, mp);
    }
  }
}*/

void TabSI::processPunchOnly(gdioutput &gdi, const SICard &csic)
{
  SICard sic=csic;
	DWORD loaded;
	gEvent->convertTimes(sic);
	oFreePunch *ofp=0;
  
	if(sic.nPunch==1)
		ofp=gEvent->addFreePunch(sic.Punch[0].Time, sic.Punch[0].Code, sic.CardNumber, true);
	else if (sic.FinishPunch.Time > 0)
		ofp=gEvent->addFreePunch(sic.FinishPunch.Time, oPunch::PunchFinish, sic.CardNumber, true);
  else if (sic.StartPunch.Time > 0)
    ofp=gEvent->addFreePunch(sic.StartPunch.Time, oPunch::PunchStart, sic.CardNumber, true);
  else
    ofp=gEvent->addFreePunch(sic.CheckPunch.Time, oPunch::PunchCheck, sic.CardNumber, true);

  if (ofp) {
    pRunner r = ofp->getTiedRunner();
	  if(gdi.getData("SIPageLoaded", loaded)){
      //gEvent->getRunnerByCard(sic.CardNumber);

		  if (r) {
			  string str=r->getName() + lang.tl(" st�mplade vid ") + ofp->getSimpleString();
			  gdi.addStringUT(0, str);
			  gdi.dropLine();
		  }
		  else {
			  string str="SI " + itos(sic.CardNumber) + lang.tl(" (ok�nd) st�mplade vid ") + ofp->getSimpleString();
			  gdi.addStringUT(0, str);
			  gdi.dropLine(0.3);
		  }	
		  gdi.scrollToBottom();
	  }

    tabForceSync(gdi, gEvent);
    gdi.makeEvent("DataUpdate", "sireadout", r ? r->getId() : 0, 0, true);

  }

  checkMoreCardsInQueue(gdi);
	return;
}


void TabSI::entryCard(gdioutput &gdi, const SICard &sic)
{
  gdi.setText("CardNo", sic.CardNumber);

  string name;
  string club;
  if (useDatabase) {
    pRunner db_r=oe->dbLookUpByCard(sic.CardNumber);

    if(db_r) {
      name=db_r->getName();
      club=db_r->getClub();
    }
  }

  //Else get name from card
  if(name.empty() && (sic.FirstName[0] || sic.LastName[0]))
    name=string(sic.FirstName)+" "+sic.LastName;

  gdi.setText("Name", name);
  gdi.setText("Club", club);
  
  if(name.empty())
    gdi.setInputFocus("Name");
  else if(club.empty())
    gdi.setInputFocus("Club");
  else 
    gdi.setInputFocus("Class");
}

void TabSI::assignCard(gdioutput &gdi, const SICard &sic)
{
  if(oe->checkCardUsed(gdi, sic.CardNumber))
    return;

  if (interactiveReadout) {
    gdi.setText("CardNo", sic.CardNumber);
    if (runnerMatchedId != -1 && gdi.isChecked("AutoTie"))
      tieCard(gdi);
    return;
  }

  //Try first current focus
  BaseInfo *ii=gdi.getInputFocus();
  char sicode[32];
  sprintf_s(sicode, "%d", sic.CardNumber);

  if(ii && ii->id[0]=='*') {
    currentAssignIndex=atoi(ii->id.c_str()+1);
  }
  else { //If not correct focus, use internal counter
    char id[32];
    sprintf_s(id, "*%d", currentAssignIndex++);

    ii=gdi.setInputFocus(id);

    if(!ii) {
      currentAssignIndex=0;
      sprintf_s(id, "*%d", currentAssignIndex++);
      ii=gdi.setInputFocus(id);
    }
  }

  if(ii && ii->getExtra()) {
    pRunner r=oe->getRunner(int(ii->getExtra()), 0);
    if(r) {
      if (r->getCardNo()==0 || 
                gdi.ask("Skriv �ver existerande bricknummer?")) {

        r->setCardNo(sic.CardNumber, false);
        r->getDI().setInt("CardFee", oe->getDI().getInt("CardFee"));
        r->synchronize();
        gdi.setText(ii->id, sicode);
      }
    }
    gdi.TabFocus();
  }

  checkMoreCardsInQueue(gdi);
}

void TabSI::generateEntryLine(gdioutput &gdi, pRunner r)
{
  oe->synchronizeList(oLRunnerId, true, false);
  oe->synchronizeList(oLCardId, false, true);
         
  gdi.restore("EntryLine", false);
  gdi.setRestorePoint("EntryLine");
  gdi.dropLine(1);  
  gdi.fillRight();

  gdi.pushX();
  gdi.addInput("CardNo", "", 8, SportIdentCB, "Bricka:");
  
  gdi.addInput("Name", "", 16, 0, "Namn:");

  gdi.addCombo("Club", 180, 200, 0, "Klubb:", "Skriv f�rsta bokstaven i klubbens namn och tryck pil-ner f�r att leta efter klubben");
  oe->fillClubs(gdi, "Club");
  gdi.selectItemByData("Club", lastClubId);

  gdi.addSelection("Class", 150, 200, 0, "Klass:");  
  oe->fillClasses(gdi, "Class", oEvent::extraNumMaps, oEvent::filterOnlyDirect);
  if(!gdi.selectItemByData("Class", lastClassId))
    gdi.selectFirstItem("Class");

  gdi.addCombo("Fee", 60, 150, SportIdentCB, "Anm. avgift:");
  oe->fillFees(gdi, "Fee");
  gdi.setText("Fee", lastFee);

  gdi.popX();
  gdi.dropLine(3.1);

  gdi.addString("",0, "Starttid:");
  gdi.dropLine(-0.2);
  gdi.addInput("StartTime", "", 5, 0, "");
  
  gdi.setCX(gdi.getCX()+gdi.scaleLength(50));
  gdi.dropLine(0.2);
  gdi.setCX(gdi.getCX()+gdi.scaleLength(5));
  
  gdi.addString("", 0, "Telefon:");
  gdi.dropLine(-0.2);
  gdi.addInput("Phone", "", 12, 0, "");
  gdi.dropLine(0.2);

  gdi.setCX(gdi.getCX()+gdi.scaleLength(50));
  
  gdi.addCheckbox("RentCard", "Hyrbricka", SportIdentCB, false);
  gdi.addCheckbox("Paid", "Kontant betalning", SportIdentCB, false);

  if (r!=0) {
    if (r->getCardNo()>0)
      gdi.setText("CardNo", r->getCardNo());

    gdi.setText("Name", r->getName());
    gdi.selectItemByData("Club", r->getClubId());
    gdi.selectItemByData("Class", r->getClassId());

    oDataConstInterface dci = r->getDCI();

    gdi.setText("Fee", oe->formatCurrency(dci.getInt("Fee")));
    gdi.setText("Phone", dci.getString("Phone"));
    
    gdi.check("RentCard", dci.getInt("CardFee") != 0);
    gdi.check("Paid", dci.getInt("Paid")>0);
  }
  
  gdi.popX();
  gdi.dropLine(2);
  gdi.addButton("EntryOK", "OK", SportIdentCB).setDefault().setExtra(r);
  gdi.addButton("EntryCancel", "Avbryt", SportIdentCB).setCancel();
  gdi.dropLine(0.1);
  gdi.addString("EntryInfo", fontMediumPlus, "").setColor(colorDarkRed);
  updateEntryInfo(gdi);
  gdi.setInputFocus("CardNo");
  gdi.dropLine(2);
  gdi.scrollToBottom();
  gdi.popX();
}

void TabSI::updateEntryInfo(gdioutput &gdi)
{
  int fee = oe->interpretCurrency(gdi.getText("Fee"));
  if (gdi.isChecked("RentCard")) {
    int cardFee = oe->getDI().getInt("CardFee");
    if (cardFee > 0)
      fee += cardFee;
  }
  string method;

  if (gdi.isChecked("Paid"))
    method = lang.tl("Att betala");
  else
    method = lang.tl("Faktureras");

  gdi.setText("EntryInfo", lang.tl("X: Y. Tryck <Enter> f�r att spara#" + 
                      method + "#" + oe->formatCurrency(fee)), true);
}

void TabSI::generateSplits(const pRunner r, gdioutput &gdi) 
{
  gdioutput gdiprint(2.0, gdi.getEncoding(), gdi.getHWND(), splitPrinter);
  vector<int> mp;
  r->evaluateCard(true, mp);
  r->printSplits(gdiprint);
  gdiprint.print(splitPrinter, oe, false, true);
}

void TabSI::generateLabel(const pRunner r, gdioutput &gdi) 
{
  gdioutput gdiprint(2.0, gdi.getEncoding(), gdi.getHWND(), labelPrinter);
  vector<int> mp;
  r->evaluateCard(true, mp);
  r->printLabel(gdiprint);
  gdiprint.print(labelPrinter, oe, false, true);
}
void TabSI::printerSetup(gdioutput &gdi) 
{
  gdi.printSetup(splitPrinter);
}

void TabSI::labelPrinterSetup(gdioutput &gdi) 
{
  gdi.printSetup(labelPrinter);
}

void TabSI::checkMoreCardsInQueue(gdioutput &gdi) {
  // Create a local list to avoid stack overflow
  list<SICard> cards = CardQueue;
  CardQueue.clear();
  std::exception storedEx;
  bool fail = false;

  while (!cards.empty()) {
		SICard c = cards.front();
	  cards.pop_front();
    try {
      gdi.RemoveFirstInfoBox("SIREAD");
		  insertSICard(gdi, c);
    }
    catch (std::exception &ex) {
      fail = true;
      storedEx = ex;
    }
	}

  if (fail)
    throw storedEx;
}

bool TabSI::autoAssignClass(pRunner r, const SICard &sic) {
  if (r && r->getClassId()==0) {
    vector<pClass> classes;
    int dist = oe->findBestClass(sic, classes);

    if (classes.size() == 1 && dist>=-1 && dist<=1) // Allow at most one wrong punch
      r->setClassId(classes[0]->getId());
  }

  return r && r->getClassId() != 0;
}

void TabSI::showManualInput(gdioutput &gdi) {
  runnerMatchedId = -1;
  gdi.setRestorePoint("ManualInput");
  gdi.fillDown();
  gdi.dropLine(0.7);

  int x = gdi.getCX();
  int y = gdi.getCY();

  gdi.setCX(x+gdi.scaleLength(15));
  gdi.dropLine();
  gdi.addString("", 1, "Manuell inmatning");
  gdi.fillRight();
  gdi.pushX();
  gdi.dropLine();
  gdi.addInput("Manual", "", 20, SportIdentCB, "Nummerlapp, SI eller Namn:");
  gdi.addInput("FinishTime", lang.tl("Aktuell tid"), 8, SportIdentCB, "M�ltid:").setFgColor(colorGreyBlue).setExtra(1);
  gdi.dropLine(1.2);
  gdi.addCheckbox("StatusOK", "Godk�nd", SportIdentCB, true);
  gdi.addCheckbox("StatusDNF", "Utg�tt", SportIdentCB, false);
  gdi.dropLine(-0.3);
  gdi.addButton("ManualOK", "OK", SportIdentCB).setDefault();
  gdi.fillDown();
  gdi.dropLine(2);
  gdi.popX();
  gdi.addString("FindMatch", 0, "", 0).setColor(colorDarkGreen);
  gdi.dropLine();

  RECT rc;
  rc.left=x;
  rc.right=gdi.getWidth()-10;
  rc.top=y;
  rc.bottom=gdi.getCY()+gdi.scaleLength(5);
  gdi.dropLine();
  gdi.addRectangle(rc, colorLightBlue);
  //gdi.refresh();
  gdi.scrollToBottom();
}

void TabSI::tieCard(gdioutput &gdi) {
  int card = gdi.getTextNo("CardNo");
  pRunner r = oe->getRunner(runnerMatchedId, 0);
  
  if (r == 0)
    throw meosException("Invalid binding");

  pRunner old = oe->getRunnerByCard(card, true, true);
  if (old && old != r) {
    char bf[256];
    sprintf_s(bf, "Bricka %d anv�nds redan av %s och kan inte tilldelas.", card, old->getName().c_str());
    throw meosException(bf);
  }

  if (r->getCardNo() > 0 && r->getCardNo() != card) {
    if (!gdi.ask("X har redan bricknummer Y. Vill du �ndra det?#" + r->getName() + "#" + itos(r->getCardNo())))
      return;
  }

  bool rent = gdi.isChecked("RentCard");
  r->setCardNo(card, true, false);
  r->getDI().setInt("CardFee", rent ? oe->getDI().getInt("CardFee") : 0);
  r->synchronize(true);

  gdi.restore("ManualTie");
  gdi.pushX();
  gdi.fillRight();
  gdi.addStringUT(italicText, getLocalTimeOnly());
  if (!r->getBib().empty())
    gdi.addStringUT(0, r->getBib(), 0);
  gdi.addStringUT(0, r->getName(), 0);

  if (r->getTeam() && r->getTeam()->getName() != r->getName())
    gdi.addStringUT(0, "(" + r->getTeam()->getName() + ")", 0);
  else if (!r->getClub().empty())
    gdi.addStringUT(0, "(" + r->getClub() + ")", 0);
  
  gdi.addStringUT(1, itos(r->getCardNo()), 0).setColor(colorDarkGreen);
  gdi.addString("EditAssign", 0, "�ndra", SportIdentCB).setExtra(r->getId());
  gdi.dropLine(1.5);
  gdi.popX();
  
  showAssignCard(gdi, false);
}

void TabSI::showAssignCard(gdioutput &gdi, bool showHelp) {
  gdi.enableInput("Interactive");
  gdi.disableInput("Database", true);
  gdi.disableInput("PrintSplits");
  gdi.disableInput("UseManualInput");
  gdi.setRestorePoint("ManualTie");
  gdi.fillDown();
  if (interactiveReadout) {
    if (showHelp)
      gdi.addString("", 10, "Avmarkera 'X' f�r att hantera alla bricktildelningar samtidigt.#" + lang.tl("Interaktiv inl�sning"));
  }
  else {
    if (showHelp)
      gdi.addString("", 10, "Markera 'X' f�r att hantera deltagarna en och en.#" + lang.tl("Interaktiv inl�sning"));
    gEvent->assignCardInteractive(gdi, SportIdentCB);
    gdi.refresh();
    return;
  }

  runnerMatchedId = -1;
  gdi.fillDown();
  gdi.dropLine(0.7);

  int x = gdi.getCX();
  int y = gdi.getCY();

  gdi.setCX(x+gdi.scaleLength(15));
  gdi.dropLine();
  gdi.addString("", 1, "Knyt bricka / deltagare");
  gdi.fillRight();
  gdi.pushX();
  gdi.dropLine();
  gdi.addInput("RunnerId", "", 20, SportIdentCB, "Nummerlapp, lopp-id eller namn:");
  gdi.addInput("CardNo", "", 8, SportIdentCB, "Bricknr:");
  gdi.dropLine(1.2);
  gdi.addCheckbox("AutoTie", "Knyt automatiskt efter inl�sning", SportIdentCB, oe->getPropertyInt("AutoTie", 1) != 0);
  gdi.addCheckbox("RentCard", "Hyrd", SportIdentCB, oe->getPropertyInt("RentCard", 0) != 0);
  
  gdi.dropLine(-0.3);
  gdi.addButton("TieOK", "OK", SportIdentCB).setDefault();
  gdi.disableInput("TieOK");
  gdi.setInputFocus("RunnerId");
  gdi.fillDown();
  gdi.dropLine(2);
  gdi.popX();
  gdi.addString("FindMatch", 0, "", 0).setColor(colorDarkGreen);
  gdi.dropLine();

  RECT rc;
  rc.left=x;
  rc.right=gdi.getWidth()+gdi.scaleLength(5);
  rc.top=y;
  rc.bottom=gdi.getCY()+gdi.scaleLength(5);
  gdi.dropLine();
  gdi.addRectangle(rc, colorLightBlue);
  gdi.scrollToBottom();
}

pRunner TabSI::getRunnerByIdentifier(int identifier) const {
  int id;
  if (identifierToRunnerId.lookup(identifier, id)) {
    pRunner r = oe->getRunner(id, 0);
    if (r && r->getRaceIdentifier() == identifier)
      return r;
    else 
      minRunnerId = 0; // Map is out-of-date
  }

  if (identifier < minRunnerId)
    return 0;

  minRunnerId = MAXINT;
  identifierToRunnerId.clear();

  pRunner ret = 0;
  vector<pRunner> runners;
  oe->autoSynchronizeLists(false);
  oe->getRunners(0, 0, runners, false);
  for ( size_t k = 0; k< runners.size(); k++) {
    if (runners[k]->getRaceNo() == 0) {
      int i = runners[k]->getRaceIdentifier();
      identifierToRunnerId.insert(i, runners[k]->getId());
      minRunnerId = min(minRunnerId, i);
      if (i == identifier)
        ret = runners[k];
    }
  }
  return ret;
}

bool TabSI::askOverwriteCard(gdioutput &gdi, pRunner r) const {
  return gdi.ask("ask:overwriteresult#" + r->getCompleteIdentification()); 
}
