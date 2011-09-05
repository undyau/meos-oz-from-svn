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
#include "TabCompetition.h"
#include "oFreeImport.h"
#include "localizer.h"
#include "oListInfo.h"
#include "download.h"
#include "progress.h"
#include "classconfiginfo.h"

#include <Shellapi.h>
#include <algorithm>
#include <cassert>
#include <cmath>

void Setup(bool overwrite);
void exportSetup();
void resetSaveTimer();

int ListsCB(gdioutput *gdi, int type, void *data);

TabCompetition::TabCompetition(oEvent *poe):TabBase(poe)
{
  eventorBase = poe->getPropertyString("EventorBase", "https://eventor.orientering.se/api/");
  defaultServer="localhost";
  defaultName="meos";
  organizorId = 0;
}

TabCompetition::~TabCompetition(void)
{
}

extern SportIdent *gSI;
extern HINSTANCE hInst;
extern HWND hWndMain;

void runCourseImport(gdioutput& gdi, const string &filename, oEvent *oe) {
	csvparser csv;
  if (csv.iscsv(filename.c_str())) {
	  gdi.addString("", 0, "Importerar OCAD csv-fil...");
	  gdi.refresh();
		
	  if(csv.ImportOCAD_CSV(*oe, filename.c_str())) {
		  gdi.addString("", 0, "Klart.");
	  }
    else gdi.addString("", 0, "Operationen misslyckades.").setColor(colorRed);
  }
  else {
    oe->importXML_EntryData(gdi, filename.c_str());
  }

  gdi.setWindowTitle(oe->getTitleName());
  oe->updateTabs();
	gdi.refresh();
}

bool TabCompetition::save(gdioutput &gdi, bool write)
{
	string name=gdi.getText("Name");

  if(name.empty()) {
		gdi.alert("T�vlingen m�ste ha ett namn");
		return 0;
	}

	oe->setName(gdi.getText("Name"));
	oe->setDate(gdi.getText("Date"));
	oe->setZeroTime(gdi.getText("ZeroTime"));
  
  oe->synchronize();
	if(gSI) gSI->SetZeroTime(oe->getZeroTimeNum());

	gdi.setWindowTitle(oe->getTitleName());

  if (write) {
	  gdi.setWaitCursor(true);
    resetSaveTimer();
	  return oe->save();
  }
  else
    return true;
}

string TabCompetition::browseForOpen(const string &Filter, const string &defext)
{
	InitCommonControls();
	
	char FileName[260];
	FileName[0]=0;
	char sbuff[256];
	OPENFILENAME of;
	sprintf_s(sbuff, 256, "%s", Filter.c_str());

	int sl=strlen(sbuff);
	for(int m=0;m<sl;m++) if(sbuff[m]=='�') sbuff[m]=0;

	of.lStructSize      =sizeof(of);
	of.hwndOwner        =hWndMain;
	of.hInstance        =hInst;
	of.lpstrFilter      =sbuff;
	of.lpstrCustomFilter=NULL;
	of.nMaxCustFilter   =0;
	of.nFilterIndex     =1;
	of.lpstrFile        = FileName;
	of.nMaxFile         =260;
	of.lpstrFileTitle   =NULL;
	of.nMaxFileTitle    =0;
	of.lpstrInitialDir  =NULL;
	of.lpstrTitle       =NULL;
	of.Flags            =OFN_PATHMUSTEXIST|OFN_FILEMUSTEXIST|OFN_HIDEREADONLY;
	of.lpstrDefExt   	  = defext.c_str();
	of.lpfnHook		     =NULL;
	
	
	if(GetOpenFileName(&of)==false)
		return "";
	
	return FileName;
}

string TabCompetition::browseForSave(const string &Filter, const string &defext, int &FilterIndex)
{
	InitCommonControls();
	
	char FileName[260];
	FileName[0]=0;
	char sbuff[256];
	OPENFILENAME of;
//	LoadString(hInst,1016,sbuff,256);
	sprintf_s(sbuff, 256,"%s", Filter.c_str());

	int sl=strlen(sbuff);
	for(int m=0;m<sl;m++) if(sbuff[m]=='�') sbuff[m]=0;

	of.lStructSize      = sizeof(of);
	of.hwndOwner        = hWndMain;
	of.hInstance        = hInst;
	of.lpstrFilter      = sbuff;
	of.lpstrCustomFilter= NULL;
	of.nMaxCustFilter   = 0;
	of.nFilterIndex     = 1;
	of.lpstrFile        = FileName;
	of.nMaxFile         = 260;
	of.lpstrFileTitle   = NULL;
	of.nMaxFileTitle    = 0;
	of.lpstrInitialDir  = NULL;
	of.lpstrTitle       = NULL;
	of.Flags            = OFN_OVERWRITEPROMPT|OFN_HIDEREADONLY;
	of.lpstrDefExt   	  = defext.c_str();
	of.lpfnHook		      = NULL;
	
	if(GetSaveFileName(&of)==false)
		return "";

	FilterIndex=of.nFilterIndex;

	return FileName;
}

bool TabCompetition::importFile(HWND hWnd, gdioutput &gdi)
{
	InitCommonControls();
	
	char FileName[260];
	FileName[0]=0;
	char sbuff[256];
	OPENFILENAME of;
//	LoadString(hInst,1016,sbuff,256);
	sprintf_s(sbuff, 256, "XML-data�*.xml;*.bu?�");

	int sl=strlen(sbuff);
	for(int m=0;m<sl;m++) if(sbuff[m]=='�') sbuff[m]=0;

	of.lStructSize      =sizeof of;
	of.hwndOwner        =hWnd;
	of.hInstance        =hInst;
	of.lpstrFilter      =sbuff;
	of.lpstrCustomFilter=NULL;
	of.nMaxCustFilter   =0;
	of.nFilterIndex     =1;
	of.lpstrFile        = FileName;
	of.nMaxFile         =260;
	of.lpstrFileTitle   =NULL;
	of.nMaxFileTitle    =0;
	of.lpstrInitialDir  =NULL;
	of.lpstrTitle       =NULL;
	of.Flags            =OFN_PATHMUSTEXIST|OFN_FILEMUSTEXIST|OFN_HIDEREADONLY;
	of.lpstrDefExt   	  ="xml";
	of.lpfnHook		     =NULL;
	
	
	if(GetOpenFileName(&of)==false)
		return false;
	
	gdi.setWaitCursor(true);
	if(oe->open(FileName, true)) {
		if(gSI) gSI->SetZeroTime(oe->getZeroTimeNum());
		gdi.setWindowTitle(oe->getTitleName());
    resetSaveTimer();
		return true;
	}

	return false;
}

bool TabCompetition::exportFileAs(HWND hWnd, gdioutput &gdi)
{
	InitCommonControls();
	
	char FileName[260];
	FileName[0]=0;
	char sbuff[256];

	OPENFILENAME of;
	//LoadString(hInst,1016,sbuff,256);
	sprintf_s(sbuff, 256, "XML-data�*.xml�");
	
	int sl=strlen(sbuff);
	for(int m=0;m<sl;m++) if(sbuff[m]=='�') sbuff[m]=0;

	of.lStructSize      =sizeof of;
	of.hwndOwner        =hWnd;
	of.hInstance        =hInst;
	of.lpstrFilter      =sbuff;
	of.lpstrCustomFilter=NULL;
	of.nMaxCustFilter   =0;
	of.nFilterIndex     =1;
	of.lpstrFile        = FileName;
	of.nMaxFile         = 260;
	of.lpstrFileTitle   = NULL;
	of.nMaxFileTitle    = 0;
	of.lpstrInitialDir  = NULL;
	of.lpstrTitle       = NULL;
	of.Flags            = OFN_OVERWRITEPROMPT|OFN_HIDEREADONLY;
	of.lpstrDefExt   	  = "xml";
	of.lpfnHook		      = NULL;
	
	
	if(GetSaveFileName(&of)==false)
		return false;

	gdi.setWaitCursor(true);
	if(!oe->save(FileName)) {
		gdi.alert("Fel: Filen "+string(FileName)+ " kunde inte skrivas.");
		return false;
	}

	return true;
}

int CompetitionCB(gdioutput *gdi, int type, void *data)
{
  TabCompetition &tc = dynamic_cast<TabCompetition &>(*gdi->getTabs().get(TCmpTab));

  return tc.competitionCB(*gdi, type, data);
}

int restoreCB(gdioutput *gdi, int type, void *data)
{
  TabCompetition &tc = dynamic_cast<TabCompetition &>(*gdi->getTabs().get(TCmpTab));

  return tc.restoreCB(*gdi, type, data);
}

void TabCompetition::loadConnectionPage(gdioutput &gdi)
{
  gdi.clearPage(false);
  showConnectionPage=true;    
  gdi.addString("", boldLarge, "Anslutningar");

  if(oe->getServerName().empty()) {
    gdi.addString("", 10, "help:52726");
    gdi.pushX();
    gdi.dropLine();
    defaultServer = oe->getPropertyString("Server", defaultServer);
    defaultName = oe->getPropertyString("UserName", defaultName);
    defaultPort = oe->getPropertyString("Port", defaultPort);
    string client = oe->getPropertyString("Client", oe->getClientName());

    gdi.fillRight();
	  gdi.addInput("Server", defaultServer, 16, 0, "MySQL Server / IP-adress:", "IP-adress eller namn p� en MySQL-server");
	  gdi.addInput("UserName", defaultName, 7, 0, "Anv�ndarnamn:");
    gdi.addInput("PassWord", defaultPwd, 9, 0, "L�senord:").setPassword(true);
	  gdi.addInput("Port", defaultPort, 4, 0, "Port:");
    gdi.fillDown();
    gdi.popX();
    gdi.dropLine(2.5);
    gdi.addInput("ClientName", client, 16, 0, "Klientnamn");
    gdi.dropLine();
    gdi.fillRight();
    gdi.addButton("ConnectToMySQL", "Anslut", CompetitionCB);
  }
  else {
    gdi.addString("", 10, "help:50431");
    gdi.dropLine(1);
    gdi.pushX();
    gdi.fillRight();
    gdi.addString("", 1, "Ansluten till:");
    gdi.addStringUT(1, oe->getServerName()).setColor(colorGreen);
    gdi.popX();
    gdi.dropLine(2);
    gdi.addInput("ClientName", oe->getClientName(), 16, 0, "Klientnamn:");
    gdi.dropLine();
    gdi.addButton("SaveClient", "�ndra", CompetitionCB);
    gdi.dropLine(2.5);

    gdi.popX();
    gdi.addString("", 1, "�ppnad t�vling:");
    
    if(oe->empty())
      gdi.addString("", 1, "Ingen").setColor(colorRed);
    else {
      gdi.addStringUT(1, oe->getName()).setColor(colorGreen);
    
      if(oe->isClient())
        gdi.addString("", 1, "(p� server)");
      else
        gdi.addString("", 1, "(lokalt)");

    }
    gdi.dropLine(2);
    gdi.popX();
    gdi.fillRight();

    if(!oe->isClient())
      gdi.addButton("UploadCmp", "Ladda upp �ppnad t�vling p� server",CompetitionCB);
   
    if(oe->empty())
      gdi.disableInput("UploadCmp");
    else {
      gdi.addButton("CloseCmp", "St�ng t�vlingen", CompetitionCB);
      gdi.addButton("Delete", "Radera t�vlingen", CompetitionCB);
    }
    gdi.dropLine(2);
    gdi.popX();
    if (oe->empty()) {
		  char bf[260];
		  getUserFile(bf, "");
		  oe->enumerateCompetitions(bf, "*.meos");		
  		
      gdi.dropLine(1);
      gdi.fillRight();
      gdi.addListBox("ServerCmp", 320, 210,  CompetitionCB, "Server");
      oe->fillCompetitions(gdi, "ServerCmp", 2);
      gdi.selectItemByData("ServerCmp", oe->getPropertyInt("LastCompetition", 0));
      gdi.fillDown();   
      gdi.addListBox("LocalCmp", 320, 210, CompetitionCB, "Lokalt"); 
      gdi.popX();
      oe->fillCompetitions(gdi, "LocalCmp", 1);
      gdi.selectItemByData("LocalCmp", oe->getPropertyInt("LastCompetition", 0));
      
      gdi.fillRight();
      gdi.addButton("OpenCmp", "�ppna t�vling", CompetitionCB);    
    }
    else if (oe->isClient()) {
      gdi.fillDown();
      gdi.popX();
      oe->listConnectedClients(gdi);
      gdi.registerEvent("Connections", CompetitionCB);
      gdi.fillRight();
    }

    gdi.addButton("DisconnectMySQL", "Koppla ner databas", CompetitionCB);
  }
  gdi.addButton("Cancel", "Till huvudsidan", CompetitionCB);
  gdi.fillDown();
  gdi.refresh();
}

bool TabCompetition::checkEventor(gdioutput &gdi, ButtonInfo &bi) {
  eventorOrigin = bi.id;

  if (organizorId == 0) {
    int clubId = getOrganizer(true);
    if (clubId == 0) {
      bi.id = "EventorAPI";
      competitionCB(gdi, GUI_BUTTON, &bi);
      return true;
    }
    else if (clubId == -1)
      throw std::exception("Kunde inte ansluta till Eventor");

    organizorId = clubId;
  }
  return false;
}

enum StartMethod {SMCommon = 1, SMDrawn, SMFree, SMCustom};

int TabCompetition::competitionCB(gdioutput &gdi, int type, void *data)
{
  if (type == GUI_LINK) {
    TextInfo ti = *(TextInfo *)data;
    if (ti.id == "link") {
      string url = ti.text;
      ShellExecute(NULL, "open", url.c_str(), NULL, NULL, SW_SHOWNORMAL);
    }
  }
	else if (type==GUI_BUTTON) {
		ButtonInfo bi=*(ButtonInfo *)data;

    if (bi.id=="UseEconomy" || bi.id=="UseSpeaker") {
      oe->getDI().setInt("UseEconomy", gdi.isChecked("UseEconomy"));
      oe->getDI().setInt("UseSpeaker", gdi.isChecked("UseSpeaker"));
      oe->updateTabs();
    }
    else if (bi.id == "CopyLink") {
      string url = gdi.getText("link");
      
      if (OpenClipboard(gdi.getHWND())) {
        EmptyClipboard();
        HGLOBAL hClipboardData;
        hClipboardData = GlobalAlloc(GMEM_DDESHARE, 
                                     url.length()+1);

        char * pchData;
        pchData = (char*)GlobalLock(hClipboardData);
		  
        strcpy_s(pchData, url.length()+1, LPCSTR(url.c_str()));
		
        GlobalUnlock(hClipboardData);
		  
        SetClipboardData(CF_TEXT,hClipboardData);
		    CloseClipboard();
      }
    }
    else if (bi.id=="Print")	{
			gdi.print(oe);			
		}
    else if (bi.id=="Setup") {
      gdi.clearPage(false);

      gdi.addString("", boldLarge, "Inst�llningar MeOS");
      gdi.dropLine();
      gdi.addString("", 10, "help:29191");
      gdi.dropLine();
      char FileNamePath[260];
      getUserFile(FileNamePath, "");
      gdi.addStringUT(0, lang.tl("MeOS lokala datakatalog �r") + ": " + FileNamePath);
      gdi.dropLine();

      char bf[260];
      GetCurrentDirectory(260, bf);
      gdi.fillRight();
      gdi.pushX();
      gdi.dropLine();
      gdi.addInput("Source", bf, 50, 0, "K�llkatalog:");
      gdi.dropLine(0.3);
      gdi.addButton("SourceBrowse", "Bl�ddra...", CompetitionCB); 
      gdi.dropLine(4);
      gdi.popX();
      gdi.fillRight();
      gdi.addButton("DoSetup", "Installera", CompetitionCB);
      gdi.addButton("ExportSetup", "Exportera", CompetitionCB);
      gdi.addButton("Cancel", "Avbryt", CompetitionCB);
      gdi.dropLine(2);
      gdi.refresh();
    }
    else if (bi.id=="SourceBrowse") {
      string s = gdi.browseForFolder(gdi.getText("Source"));
      if (!s.empty())
        gdi.setText("Source", s);
    }
    else if (bi.id=="DoSetup") {
      string source = gdi.getText("Source");
      if(SetCurrentDirectory(source.c_str())) {
        Setup(true);
        gdi.alert("Tillg�ngliga filer installerades. Starta om MeOS.");
        exit(0);
      }
      else
        throw std::exception("Operationen misslyckades");
    }
    else if (bi.id=="ExportSetup") {
      gdi.clearPage(false);

      gdi.addString("", boldLarge, "Exportera inst�llningar och l�pardatabaser");
      gdi.addString("", 10, "help:15491");
      gdi.dropLine();
      char FileNamePath[260];
      getUserFile(FileNamePath, "");
      gdi.addStringUT(0, lang.tl("MeOS lokala datakatalog �r") + ": " + FileNamePath);
      
      gdi.dropLine();

      char bf[260];
      GetCurrentDirectory(260, bf);
      gdi.fillRight();
      gdi.pushX();
      gdi.dropLine();
      gdi.addInput("Source", bf, 50, 0, "Destinationskatalog:");
      gdi.dropLine(0.3);
      gdi.addButton("SourceBrowse", "Bl�ddra...", CompetitionCB); 
      gdi.dropLine(4);
      gdi.popX();
      gdi.fillRight();
      gdi.addButton("DoExportSetup", "Exportera", CompetitionCB);
      gdi.addButton("Cancel", "Avbryt", CompetitionCB);
      gdi.dropLine(2);
      gdi.refresh();
    }
    else if (bi.id=="DoExportSetup") {
      string source = gdi.getText("Source");
      if(SetCurrentDirectory(source.c_str())) {
        exportSetup();
        gdi.alert("Inst�llningarna har exporterats.");
      }
    }
    else if (bi.id=="Report") {
      gdi.clearPage(true);
      oe->generateCompetitionReport(gdi);

      gdi.addButton(gdi.getWidth()+20, 15, gdi.scaleLength(120), "Cancel", 
                    "�terg�", CompetitionCB,  "", true, false);
      gdi.addButton(gdi.getWidth()+20, 18+gdi.getButtonHeight(), gdi.scaleLength(120), "Print", 
                    "Skriv ut...", CompetitionCB,  "Skriv ut rapporten", true, false);
      gdi.refresh();

      //gdi.addButton("Cancel", "Avbryt", CompetitionCB);    
    }
    else if (bi.id=="Settings") {
      gdi.clearPage(false);

      gdi.addString("", boldLarge, "T�vlingsinst�llningar");
      gdi.dropLine(0.5);
      vector<string> fields;
      gdi.pushY();
      gdi.addString("", 1, "Adress och kontakt");
      fields.push_back("Organizer");
      fields.push_back("Street");
      fields.push_back("Address");
      fields.push_back("EMail");
      fields.push_back("Homepage");
      
      oe->getDI().buildDataFields(gdi, fields);
      
      gdi.newColumn();
      gdi.popY();

      gdi.addString("", 1, "Avgifter");
      fields.clear();
      gdi.fillRight();
      gdi.pushX();
      fields.push_back("CardFee");
      fields.push_back("EliteFee");
      fields.push_back("EntryFee");
      fields.push_back("YouthFee");

      oe->getDI().buildDataFields(gdi, fields);
      
      gdi.popX();
      gdi.dropLine(3);
      
      fields.clear();
      fields.push_back("OrdinaryEntry");
      fields.push_back("LateEntryFactor");

      oe->getDI().buildDataFields(gdi, fields);
      
      gdi.fillDown();
      gdi.popX();
      gdi.dropLine(3);
      
      gdi.addString("", 1, "Betalningsinformation");
      fields.clear();
      fields.push_back("Account");
      fields.push_back("PaymentDue");
      
      oe->getDI().buildDataFields(gdi, fields);
      oe->getDI().fillDataFields(gdi);

      gdi.fillRight();
      gdi.addButton("SaveSettings", "Spara", CompetitionCB);
      gdi.addButton("Cancel", "Avbryt", CompetitionCB);
      gdi.dropLine(2);
      gdi.refresh();
    }
    else if(bi.id=="SaveSettings") {
      oe->getDI().saveDataFields(gdi);
      if (oe->isChanged()) {
        oe->setProperty("Organizer", oe->getDCI().getString("Organizer"));
        oe->setProperty("Street", oe->getDCI().getString("Street"));
        oe->setProperty("Address", oe->getDCI().getString("Address"));
        oe->setProperty("EMail", oe->getDCI().getString("EMail"));
        oe->setProperty("Homepage", oe->getDCI().getString("Homepage"));

        oe->setProperty("CardFee", oe->getDCI().getInt("CardFee"));
        oe->setProperty("EliteFee", oe->getDCI().getInt("EliteFee"));
        oe->setProperty("EntryFee", oe->getDCI().getInt("EntryFee"));
        oe->setProperty("YouthFee", oe->getDCI().getInt("YouthFee"));

        oe->setProperty("Account", oe->getDCI().getString("Account"));
        oe->setProperty("LateEntryFactor", oe->getDCI().getString("LateEntryFactor"));
      }
      loadPage(gdi);
    }
    else if (bi.id == "Exit") {
      PostMessage(gdi.getMain(), WM_CLOSE, 0, 0);
    }
    else if (bi.id=="Browse") {
      string f = gdi.browseForOpen("XML-filer�*.xml�", "xml");
      string id;
      InputInfo *ii;
      if (!f.empty()) {
        ii = static_cast<InputInfo *>(bi.getExtra());
        if (ii)
          gdi.setText(ii->id, f);
      }
    }
    else if (bi.id=="DBaseIn") {
      gdi.clearPage(true);
      gdi.addString("", boldText, "Importera l�pare och klubbar / distriktsregister");
      gdi.addString("", 10, "help:687188");
      gdi.dropLine(2);
      gdi.pushX();
      gdi.fillRight();
      InputInfo *ii = &gdi.addInput("ClubFile", "", 20, 0, "Filnamn IOF-XML med klubbar");
      gdi.dropLine();
      gdi.addButton("Browse", "Bl�ddra...", CompetitionCB).setExtra(ii);
      gdi.popX();
      gdi.dropLine(3);
      ii = &gdi.addInput("CmpFile", "", 20, 0, "Filnamn IOF-XML med l�pare");
      gdi.dropLine();
      gdi.addButton("Browse", "Bl�ddra...", CompetitionCB).setExtra(ii);
      
      gdi.dropLine(4);

      gdi.popX();        
      gdi.addCheckbox("Clear", "Nollst�ll databaser", 0, true);
      gdi.dropLine(2);

      gdi.popX();
      gdi.addButton("DoDBaseIn", "Importera", CompetitionCB);
      gdi.addButton("Cancel", "Avbryt", CompetitionCB);
      gdi.dropLine(3);
      gdi.fillDown();
      gdi.popX();
    }
    else if (bi.id=="DoDBaseIn") {
      gdi.enableEditControls(false);
      gdi.disableInput("DoDBaseIn");
      gdi.disableInput("Cancel");

      gdi.setWaitCursor(true);
      gdi.addString("", 0, "Importerar...");
      bool clear = gdi.isChecked("Clear");
      oe->importXML_IOF_Data(gdi.getText("ClubFile").c_str(),
                             gdi.getText("CmpFile").c_str(), clear);
      
      gdi.dropLine();
      gdi.addButton("Cancel", "�terg�", CompetitionCB);
      gdi.setWaitCursor(false);                       
    }
    else if (bi.id=="Reset") {
      if (gdi.ask("Vill d� �terst�lla inst�llningar och skriva �ver egna databaser?"))
        Setup(true);
    }
		else if (bi.id=="ConnectMySQL") 
      loadConnectionPage(gdi);
    else if(bi.id=="SaveClient") { 
      oe->setClientName(gdi.getText("ClientName"));
      if (gdi.getText("ClientName").length()>0) 
          oe->setProperty("Client", gdi.getText("ClientName"));
    }
    else if (bi.id=="ConnectToMySQL") {
			bool s=oe->connectToMySQL(gdi.getText("Server"), 
                                gdi.getText("UserName"),
                                gdi.getText("PassWord"), 
                                gdi.getTextNo("Port"));

      if (s) {
        defaultServer=gdi.getText("Server");
        defaultName=gdi.getText("UserName");
        defaultPwd=gdi.getText("PassWord");
        defaultPort=gdi.getText("Port");

        oe->setClientName(gdi.getText("ClientName"));
        oe->setProperty("Server", defaultServer);
        oe->setProperty("UserName", defaultName);
        oe->setProperty("Port", defaultPort);
        if (gdi.getText("ClientName").length()>0) 
          oe->setProperty("Client", gdi.getText("ClientName"));


        loadConnectionPage(gdi);
      }
		}
    else if (bi.id=="DisconnectMySQL") {
      oe->closeDBConnection();
      loadConnectionPage(gdi);
		}
    else if (bi.id=="UploadCmp") {
      if(oe->uploadSynchronize())
        gdi.setWindowTitle(oe->getTitleName());

			loadConnectionPage(gdi);
    }
    else if (bi.id == "MultiEvent") {
      loadMultiEvent(gdi);
      //string ne = oe->cloneCompetition(true, true, true, true);
    }
    else if (bi.id == "CloneEvent") {
      string ne = oe->cloneCompetition(true, false, false, false);
    }
    else if (bi.id == "UseEventor") {
      if (gdi.isChecked("UseEventor"))
        oe->setProperty("UseEventor", 1);
      else
        oe->setProperty("UseEventor", 2);
      PostMessage(gdi.getTarget(), WM_USER + 2, TCmpTab, 0);
    } 
    else if (bi.id == "EventorAPI") {
      assert(!eventorOrigin.empty());
      //DWORD d;
      //if (gdi.getData("ClearPage", d))
      gdi.clearPage(true);
      gdi.addString("", boldLarge, "Nyckel f�r Eventor");
      gdi.dropLine();
      gdi.addString("", 10, "help:eventorkey");
      gdi.dropLine();
      gdi.addInput("apikey", "", 40, 0, "API-nyckel:");
      gdi.dropLine();
      gdi.fillRight();
      gdi.pushX();
      gdi.setRestorePoint("APIKey");
      gdi.addButton("Cancel", "Avbryt", CompetitionCB).cancelButton();
      gdi.addButton("EventorAPISave", "Spara", CompetitionCB).defaultButton();
      gdi.dropLine(3);
      gdi.popX();
    }
    else if (bi.id == "EventorAPISave") {
      string key = gdi.getText("apikey");
      oe->setPropertyEncrypt("apikey", key);

      int clubId = getOrganizer(false);

      if (clubId > 0) {
        gdi.restore("APIKey", false);
        gdi.fillDown();
        gdi.addString("", 1, "Godk�nd API-nyckel").setColor(colorGreen);
        gdi.addString("", 0, "Klubb: X#" + eventor.name);
        gdi.addStringUT(0, eventor.city);
        gdi.dropLine();
        gdi.addButton("APIKeyOK", "Forts�tt", CompetitionCB);
        gdi.refresh();
      }
      else {
        gdi.fillDown();
        gdi.dropLine();
        oe->setPropertyEncrypt("apikey", "");
        organizorId = 0;
        gdi.addString("", boldText, "Felaktig nyckel").setColor(colorRed);
        gdi.refresh();
      }        
    }
    else if (bi.id == "APIKeyOK") {
      oe->setProperty("Organizer", eventor.name);
      string adr  = eventor.careOf.empty() ? eventor.street : 
                      eventor.careOf + ", " + eventor.street;
      oe->setProperty("Street", adr);
      oe->setProperty("Address", eventor.zipCode + " " + eventor.city);
      if (eventor.account.size() > 0)
        oe->setProperty("Account", eventor.account);
      if (eventor.email.size() > 0)
        oe->setProperty("Email", eventor.email);
      assert(!eventorOrigin.empty());
      bi.id = eventorOrigin;
      eventorOrigin.clear();
      return competitionCB(gdi, type, &bi);
    }
    else if (bi.id == "EventorUpdateDB") {
      gdi.clearPage(false);
      gdi.addString("", boldLarge, "Uppdatera l�pardatabasen");
      gdi.setData("UpdateDB", 1);
      bi.id = "EventorImport";
      return competitionCB(gdi, type, &bi);
    }
    else if (bi.id == "SynchEventor") {
      if (checkEventor(gdi, bi))
        return 0;

      gdi.clearPage(true);
      //gdi.setData("EventorId", (int)oe->getExtIdentifier());
      //gdi.setData("UpdateDB", 1); 
      gdi.addString("", boldLarge, "Utbyt t�vlingsdata med Eventor");
      gdi.dropLine();
      
      ClassConfigInfo cnf;
      oe->getClassConfigurationInfo(cnf);
      
      gdi.fillRight();
      gdi.addButton("EventorEntries", "H�mta efteranm�lningar", CompetitionCB);
      gdi.addButton("EventorUpdateDB", "Uppdatera l�pardatabasen", CompetitionCB);
      gdi.addButton("EventorStartlist", "Publicera startlista", CompetitionCB, "Publicera startlistan p� Eventor");
      
      if (!cnf.hasStartTimes())
        gdi.disableInput("EventorStartlist");
      
      gdi.addButton("EventorResult", "Publicera resultat", CompetitionCB, "Publicera resultat och str�cktider p� Eventor och WinSplits online");
      
      if (!cnf.hasResults())
        gdi.disableInput("EventorResult");

      gdi.addButton("Cancel", "Avbryt", CompetitionCB);
      gdi.popX();
      gdi.dropLine(2);
      bi.id = "EventorImport";
      //competitionCB(gdi, type, &bi);
    }
    else if (bi.id == "EventorEntries") {
      ClassConfigInfo cnf;
      oe->getClassConfigurationInfo(cnf);
      if (cnf.hasResults()) {
        if (!gdi.ask("T�vlingen har redan resultat. Vill du verkligen h�mta anm�lningar?"))
          return 0;
      }
      gdi.enableEditControls(false);
      gdi.enableInput("Cancel");
      gdi.dropLine(2);
      gdi.setData("EventorId", (int)oe->getExtIdentifier());
      gdi.setData("UpdateDB", 0); 
      bi.id = "EventorImport";
      competitionCB(gdi, type, &bi);
    }
    else if (bi.id == "EventorStartlist") {
      gdi.clearPage(true);
      gdi.fillDown();
      gdi.dropLine();
      gdi.addString("", boldLarge, "Publicerar startlistan");

      gdi.dropLine();
      gdi.fillDown();
      gdi.addString("", 1, "Ansluter till Internet").setColor(colorGreen);

      gdi.refreshFast();
      Download dwl;
      dwl.initInternet();

      string startlist = getTempFile();
      oe->exportIOFStartlist(startlist.c_str());
      vector<string> fileList;
      fileList.push_back(startlist);

      string zipped = getTempFile();
      zip(zipped.c_str(), 0, fileList);
      ProgressWindow pw(gdi.getTarget());
      pw.init();
      vector<pair<string,string> > key;
      getAPIKey(key);
      
      string result = getTempFile();
      try {
        dwl.postFile(eventorBase + "import/startlist", zipped, result, key, pw);
      }
      catch (std::exception &ex) {
        gdi.fillRight();
        gdi.pushX();
        gdi.addString("", 1, "Operationen misslyckades: ");
        gdi.addString("", 0, ex.what()).setColor(colorRed);
        gdi.dropLine(2);
        gdi.popX();
        gdi.addButton("Cancel", "Avbryt", CompetitionCB);
        gdi.addButton(bi.id, "F�rs�k igen", CompetitionCB);
        removeTempFile(startlist);
        removeTempFile(zipped);
        gdi.refresh();
        return 0;
      }
      
      removeTempFile(startlist);
      removeTempFile(zipped);
      gdi.addString("", 1, "Klart");

      xmlparser xml;
      xml.read(result.c_str());
      xmlobject obj = xml.getObject("ImportStartListResult");
      if (obj) {
        string url;
        obj.getObjectString("StartListUrl", url);
        if (url.length()>0) {
          gdi.fillRight();
          gdi.pushX();
          gdi.dropLine();
          gdi.addString("", 0, "L�nk till startlistan:");
          gdi.addString("link", 0, url, CompetitionCB).setColor(colorRed);
        }
      }
      gdi.dropLine(3);
      gdi.popX();

      gdi.addButton("CopyLink", "Kopiera l�nken till urklipp", CompetitionCB);
      gdi.addButton("Cancel", "�terg�", CompetitionCB);
      gdi.refresh();      
    }
    else if (bi.id == "EventorResult") {
      ClassConfigInfo cnf;
      oe->getClassConfigurationInfo(cnf);
      if (cnf.hasPatrol()) {
        if (!gdi.ask("N�r denna version av MeOS sl�pptes kunde Eventor "
                     "inte hantera resultat fr�n patrullklasser. Vill du f�rs�ka �nd�?")) 
          return loadPage(gdi);
      }

      gdi.clearPage(true);
      gdi.fillDown();
      gdi.dropLine();
      gdi.addString("", boldLarge, "Publicerar resultat");

      gdi.dropLine();
      gdi.fillDown();
      gdi.addString("", 1, "Ansluter till Internet").setColor(colorGreen);

      gdi.refreshFast();
      Download dwl;
      dwl.initInternet();

      string resultlist = getTempFile();
      set<int> classes;
      oe->exportIOFSplits(resultlist.c_str(), false, classes, -1);
      vector<string> fileList;
      fileList.push_back(resultlist);

      string zipped = getTempFile();
      zip(zipped.c_str(), 0, fileList);
      ProgressWindow pw(gdi.getTarget());
      pw.init();
      vector<pair<string,string> > key;
      getAPIKey(key);
      
      string result = getTempFile();
      
      try {
        dwl.postFile(eventorBase + "import/resultlist", zipped, result, key, pw);   
      }
      catch (std::exception &ex) {
        gdi.fillRight();
        gdi.pushX();
        gdi.addString("", 1, "Operationen misslyckades: ");
        gdi.addString("", 0, ex.what()).setColor(colorRed);
        gdi.dropLine(2);
        gdi.popX();
        gdi.addButton("Cancel", "Avbryt", CompetitionCB);
        gdi.addButton(bi.id, "F�rs�k igen", CompetitionCB);
        removeTempFile(resultlist);
        removeTempFile(zipped);
        gdi.refresh();
        return 0;
      }
      
      removeTempFile(resultlist);
      removeTempFile(zipped);
      gdi.addString("", 1, "Klart");

      xmlparser xml;
      xml.read(result.c_str());
      xmlobject obj = xml.getObject("ImportResultListResult");
      if (obj) {
        string url;
        obj.getObjectString("ResultListUrl", url);
        if (url.length()>0) {
          gdi.fillRight();
          gdi.pushX();
          gdi.dropLine();
          gdi.addString("", 0, "L�nk till resultatlistan:");
          gdi.addString("link", 0, url, CompetitionCB).setColor(colorRed);
        }
      }
      gdi.dropLine(3);
      gdi.popX();

      gdi.addButton("CopyLink", "Kopiera l�nken till urklipp", CompetitionCB);
      gdi.addButton("Cancel", "�terg�", CompetitionCB);
      gdi.refresh();
    }
    else if (bi.id == "Eventor") {
      if (checkEventor(gdi, bi))
        return 0;

      SYSTEMTIME st;
	    GetLocalTime(&st);
      st.wYear--; // Include last years competitions
      getEventorCompetitions(gdi, convertSystemDate(st),  events);
      gdi.clearPage(true);

      gdi.addString("", boldLarge, "H�mta data fr�n Eventor");
     
      gdi.dropLine();
      gdi.addButton("EventorAPI", "Anslutningsinst�llningar...", CompetitionCB);
      gdi.dropLine();
      gdi.fillRight();
      gdi.pushX();
      gdi.addCheckbox("EventorCmp", "H�mta t�vlingsdata", CompetitionCB, true);
      gdi.addSelection("EventorSel", 300, 200);
      sort(events.begin(), events.end());
      st.wYear++; // Restore current time
      string now = convertSystemDate(st);

      int selected = 0; // Select next event by default
      for (int k = events.size()-1; k>=0; k--) {
        string n = events[k].Name + " (" + events[k].Date + ")";
        gdi.addItem("EventorSel", n, k);
        if (now < events[k].Date || selected == 0)
          selected = k;
      }
      gdi.selectItemByData("EventorSel", selected);

      gdi.dropLine(3);
      gdi.popX();
      gdi.addCheckbox("EventorDb", "Uppdatera l�pardatabasen", CompetitionCB, true);
      gdi.dropLine(3);
      gdi.popX();
      gdi.addButton("Cancel", "Avbryt", CompetitionCB);
      gdi.addButton("EventorNext", "N�sta >>", CompetitionCB);
    }
    else if (bi.id == "EventorCmp") {
      gdi.setInputStatus("EventorSel", gdi.isChecked(bi.id));
      gdi.setInputStatus("EventorNext", gdi.isChecked(bi.id) | gdi.isChecked("EventorDb"));
    }
    else if (bi.id == "EventorDb") {
      gdi.setInputStatus("EventorNext", gdi.isChecked(bi.id) | gdi.isChecked("EventorCmp"));
    }
    else if (bi.id == "EventorNext") {
      bool cmp = gdi.isChecked("EventorCmp");
      bool db = gdi.isChecked("EventorDb");
      ListBoxInfo lbi;
      gdi.getSelectedItem("EventorSel", &lbi);
      const CompetitionInfo *ci = 0;      
      if (lbi.data < events.size())
        ci = &events[lbi.data];     

      gdi.clearPage(true);
      gdi.setData("UpdateDB", db);
      gdi.pushX();
      if (cmp && ci) {
        gdi.setData("EventIndex", lbi.data);
        gdi.setData("EventorId", ci->Id);
        gdi.addString("", boldLarge, "H�mta t�vlingsdata f�r X#" + ci->Name);
        gdi.dropLine(0.5);

        gdi.fillRight();
        gdi.pushX();

        int tt = convertAbsoluteTimeHMS(ci->firstStart);
        string ttt = tt>0 ? ci->firstStart : "";
        gdi.addInput("FirstStart", ttt, 10, 0, "F�rsta ordinarie starttid:", "Skriv f�rsta starttid p� formen HH:MM:SS");

        gdi.addSelection("StartType", 200, 150, 0, "Startmetod", "help:startmethod");
        gdi.addItem("StartType", lang.tl("Gemensam start"), SMCommon);
        gdi.addItem("StartType", lang.tl("Lottad startlista"), SMDrawn);
        gdi.addItem("StartType", lang.tl("Fria starttider"), SMFree);
        gdi.addItem("StartType", lang.tl("Jag sk�ter lottning sj�lv"), SMCustom);
        gdi.selectFirstItem("StartType");
        gdi.fillDown();
        gdi.popX();
        gdi.dropLine(3);

        gdi.addInput("LastEntryDate", ci->lastNormalEntryDate, 10, 0, "Sista ordinarie anm�lningsdatum:");

        gdi.addString("", boldText, "Importera banor");
        gdi.addString("", 10, "help:ocad13091");
      	gdi.fillRight();
        gdi.dropLine();
        gdi.addInput("FileName", "", 32, 0, "Filnamn (OCAD banfil):");
			  gdi.dropLine();
			  gdi.fillDown();
			  gdi.addButton("BrowseCourse", "Bl�ddra...", CompetitionCB);
      }
      else {
        gdi.addString("", boldLarge, "H�mta l�pardatabasen");
        gdi.dropLine(0.5);

        bi.id = "EventorImport";
        return competitionCB(gdi, type, &bi);
      }

      gdi.dropLine(1);
      gdi.popX();
      gdi.setRestorePoint("DoEventor");
      gdi.fillRight();
      gdi.addButton("Cancel", "Avbryt", CompetitionCB).cancelButton();
      gdi.addButton("EventorImport", "H�mta data fr�n Eventor", CompetitionCB).defaultButton();
      gdi.fillDown();
      gdi.popX();
    }
    else if (bi.id == "EventorImport") {
      const int diffZeroTime = 3600;
      DWORD id;
      DWORD db;
      gdi.getData("EventorId", id);
      gdi.getData("UpdateDB", db);

      
      DWORD eventIndex;
      gdi.getData("EventIndex", eventIndex);
      const CompetitionInfo *ci = 0;      
      if (eventIndex < events.size())
        ci = &events[eventIndex];  

      string course = gdi.getText("FileName", true    );   
      int startType = 0;
      const bool createNew = oe->getExtIdentifier() != id && id>0;
      int zeroTime = 0;
      int firstStart = 0;
      string lastEntry;
      if (id > 0 && createNew) {
        string fs = gdi.getText("FirstStart");
        int t = oEvent::convertAbsoluteTime(fs);
        if (t<0) {
          string msg = "Ogiltig starttid: X#" + fs;
          throw std::exception(msg.c_str());
        }
        firstStart = t;
        zeroTime = t - diffZeroTime;
        if (zeroTime<0)
          zeroTime += 3600*24;

        ListBoxInfo lbi;
        gdi.getSelectedItem("StartType", &lbi);
        startType = lbi.data;

        lastEntry = gdi.getText("LastEntryDate");
      }

      gdi.disableInput("EventorImport");
      gdi.disableInput("FileName");
      gdi.disableInput("FirstStart");
      
      string tEvent = getTempFile();
      string tClubs = getTempFile();
      string tClass = getTempFile();
      string tEntry = getTempFile();
      string tRunnerDB = db!= 0 ? getTempFile() : "";
      gdi.dropLine(3);
      try {
        getEventorCmpData(gdi, id, tEvent, tClubs, tClass, tEntry, tRunnerDB);
      }
      catch (std::exception &ex) {
        gdi.popX();
        gdi.dropLine();
        gdi.fillDown();
        gdi.addString("", 0, string("Fel: X#") + ex.what()).setColor(colorRed);
        gdi.addButton("Cancel", "�terg�", CompetitionCB);
        gdi.refresh();
        return 0;
      }

      gdi.fillDown();
      gdi.dropLine();

      if (db != 0) {
        gdi.addString("", 1, "Behandlar l�pardatabasen").setColor(colorGreen);
        vector<string> extractedFiles;
        gdi.fillRight();
        gdi.addString("", 0 , "Packar upp l�pardatabas...");
        gdi.refreshFast();
        unzip(tRunnerDB.c_str(), 0, extractedFiles);
        gdi.addString("", 0 , "OK");
        gdi.refreshFast();
        gdi.dropLine();
        gdi.popX();
        gdi.fillDown();
        removeTempFile(tRunnerDB);
        if (extractedFiles.size() != 1) {
          gdi.addString("", 0, "Unexpected file contents: X#" + tRunnerDB).setColor(colorRed);
        }
        if (extractedFiles.empty())
          tRunnerDB.clear();
        else
          tRunnerDB = extractedFiles[0];
      }
        
      oe->importXML_IOF_Data(tClubs.c_str(), tRunnerDB.c_str(), false);
      removeTempFile(tClubs);
      
      if (id > 0) {
        gdi.dropLine();
        gdi.addString("", 1, "Behandlar t�vlingsdata").setColor(colorGreen);

        if (createNew && id>0) {
          gdi.addString("", 1, "Skapar ny t�vling");
          oe->newCompetition("New");
          
          oe->importXML_EntryData(gdi, tEvent.c_str());
          oe->setZeroTime(formatTimeHMS(zeroTime));
          oe->getDI().setDate("OrdinaryEntry", lastEntry);
          if (ci) {
            if (!ci->account.empty()) 
              oe->getDI().setString("Account", ci->account);
            
            if (!ci->url.empty())
              oe->getDI().setString("Homepage", ci->url);

            
          }
        }
        removeTempFile(tEvent);
        
        oe->importXML_EntryData(gdi, tClass.c_str());
        removeTempFile(tClass);

        oe->importXML_EntryData(gdi, tEntry.c_str());
        removeTempFile(tEntry);
      

        if (!course.empty()) {
          gdi.dropLine();
          runCourseImport(gdi, course, oe);
        }

        bool drawn = false;
        if (createNew && startType>0) {
          gdi.scrollToBottom();
          gdi.dropLine();

          switch (startType) {
            case SMCommon:
              oe->automaticDrawAll(gdi, formatTimeHMS(firstStart), "0", "0", false, false, false);
              drawn = true;
              break;

            case SMDrawn:
              ClassConfigInfo cnf;
              oe->getClassConfigurationInfo(cnf);
              bool skip = false;
              if (!cnf.classWithoutCourse.empty()) {
                string cls = "";
                for (size_t k = 0; k < cnf.classWithoutCourse.size(); k++) {
                  if (k>=5) {
                    cls += "...";
                    break;
                  }
                  if (k>0)
                    cls += ", ";
                  cls += cnf.classWithoutCourse[k];
                }
                if (!gdi.ask("ask:missingcourse#" + cls)) {
                  gdi.addString("", 0, "Skipper lottning");
                  skip = true;
                }
              }
              if (!skip)
                oe->automaticDrawAll(gdi, formatTimeHMS(firstStart), "2:00", "2", true, true, false);
              drawn = true;
              break;
          }
        }
      }

      gdi.dropLine();
      gdi.addString("", 1, "Klart").setColor(colorGreen);

      gdi.scrollToBottom();
      gdi.dropLine();
      gdi.disableInput("Cancel"); // Disable "cancel" above
      gdi.fillRight();
      if (id > 0)
        gdi.addButton("StartIndividual", "Visa startlistan", ListsCB);
      gdi.addButton("Cancel", "�terg�", CompetitionCB);
      gdi.refreshFast();
          
    }
		else if (bi.id == "Cancel"){
			loadPage(gdi);
		}
    else if (bi.id == "dbtest") {
      
    }
    else if(bi.id=="FreeImport") {
      gdi.clearPage(true);
      gdi.addString("", 2, "Fri anm�lningsimport");
      gdi.addString("", 10, "help:33940");
      gdi.dropLine(0.5);
      gdi.addInputBox("EntryText", 450, 280, entryText, 0, "");
      gdi.dropLine(0.5);
      gdi.fillRight();
      gdi.addButton("PreviewImport", "Granska inmatning", CompetitionCB, "tooltip:analyze");
      gdi.addButton("Paste", "Klistra in", CompetitionCB, "tooltip:paste");
      gdi.addButton("ImportFile", "Importera fil...", CompetitionCB, "tooltip:import");
      gdi.addButton("ImportDB", "Bygg databaser...", CompetitionCB, "tooltip:builddata");
      gdi.fillDown();
    }
    else if(bi.id=="ImportDB") {
      if(!gdi.ask("help:146122"))
        return 0;

      string file=gdi.browseForOpen("XML-filer�*.xml;*.meos�", "xml");

      if(file.empty())
        return 0;
      gdi.setWaitCursor(true);
      oe->getFreeImporter(fi);
      string info;

      oe->importXMLNames(file.c_str(), fi, info);
      fi.save();
      gdi.alert(info);
      gdi.setWaitCursor(false);
    }
    else if(bi.id=="Paste") {
      gdi.pasteText("EntryText");
    }
    else if(bi.id=="ImportFile") {
      string file=gdi.browseForOpen("Textfiler�*.txt�", "txt");
      ifstream fin(file.c_str());
      char bf[1024];
      bf[0]='\r';
      bf[1]='\n';
      entryText.clear();
      while (fin.good() && !fin.eof()) {
        fin.getline(bf+2, 1024-2);
        entryText+=bf;
      }
      entryText+="\r\n";
      fin.close();

      gdi.setText("EntryText", entryText);
    }
    else if(bi.id=="PreviewImport") {
      oe->getFreeImporter(fi);
      entryText=gdi.getText("EntryText");
      gdi.clearPage(false);
      gdi.addString("", 2, "F�rhandsgranskning, import");
      gdi.dropLine(0.5);
      char *bf=new char[entryText.length()+1];
      strcpy_s(bf, entryText.length()+1, entryText.c_str());      
      fi.extractEntries(bf, entries);
      delete[] bf;
      fi.showEntries(gdi, entries);
      gdi.fillRight();
      gdi.dropLine(1);
      gdi.addButton("DoFreeImport", "Spara anm�lningar", CompetitionCB);
      gdi.addButton("FreeImport", "�ndra", CompetitionCB);
      
      gdi.addButton("Cancel", "Avbryt", CompetitionCB);
      gdi.scrollToBottom();
    }
    else if (bi.id=="DoFreeImport") {
      fi.addEntries(oe, entries);
      entryText.clear();
      loadPage(gdi);
    }
		else if(bi.id=="Startlist") {
      save(gdi, false);
			int FilterIndex = 1;
			string save=browseForSave("IOF Startlista (xml)�*.xml�OE Semikolonseparerad (csv)�*.csv�Webben (html)�*.html�", "xml", FilterIndex);

			if(save.length()>0)	{
				gdi.setWaitCursor(true);
        oe->sanityCheck(gdi, false);
        if (FilterIndex == 1) {
				  oe->exportIOFStartlist(save.c_str());
        }
        else if (FilterIndex == 2) {
				  oe->exportOECSV(save.c_str());
        }
        else {
          oListParam par;
          par.listCode = EStdStartList;
          par.legNumber = -1;
          oListInfo li;
          oe->generateListInfo(par,  gdi.getLineHeight(), li);
          gdioutput tGdi(gdi.getScale());
          oe->generateList(tGdi, true, li);
          tGdi.writeTableHTML(save, oe->getName());
          tGdi.openDoc(save.c_str());
        }
          //gdi.alert("Not implemented");
			}
		}
		else if(bi.id=="Splits") {
      save(gdi, false);
			int FilterIndex=1;
			string save=browseForSave("IOF Resultat (XML)�*.xml�OE Semikolonseparerad (csv)�*.csv�Webben (html)�*.html�", "xml", FilterIndex);

			if(save.length()>0)	{
        oe->sanityCheck(gdi, true);
				gdi.setWaitCursor(true);
        if (FilterIndex == 1) {
          ClassConfigInfo cnf;
          oe->getClassConfigurationInfo(cnf);
          if (!cnf.hasTeamClass()) {
  				  oe->exportIOFSplits(save.c_str(), true, set<int>(), -1);
          }
          else {
            gdi.clearPage(false);
            gdi.addString("", boldLarge, "Export av resultat/str�cktider");
            gdi.dropLine();
            gdi.addString("", 10, "help:splitexport");
            gdi.addInput("SplitFile", save, 40, 0, "Str�cktidsfil");
            gdi.addSelection("Type", 300, 100, 0, "Typ av export");

            gdi.addItem("Type", lang.tl("Totalresultat"), 1);
            gdi.addItem("Type", lang.tl("Alla str�ckor/lopp i separata filer"), 2);
            int legMax = cnf.getNumLegsTotal();
            for (int k = 0; k<legMax; k++) {
              gdi.addItem("Type", lang.tl("Str�cka X#" + itos(k+1)), k+10);
            }
            gdi.selectFirstItem("Type");
            gdi.dropLine();
            gdi.fillRight();
            gdi.addButton("SplitExport", "Exportera", CompetitionCB);
            gdi.addButton("Cancel", "Avbryt", CompetitionCB);
            gdi.refresh();
          }
        }
        else if (FilterIndex == 2) {
 				  oe->exportOECSV(save.c_str());
        }
        else
          gdi.alert("Not implemented");
			}
		}    
		else if (bi.id=="SplitExport") {
      string file = gdi.getText("SplitFile");
      ListBoxInfo lbi;
      gdi.getSelectedItem("Type", &lbi);

      if (lbi.data == 2) {
        string fileBase;
        string fileEnd = file.substr(file.length()-4);
        if (_stricmp(fileEnd.c_str(), ".XML") == 0)
          fileBase = file.substr(0, file.length() - 4);
        else {
          fileEnd = ".xml";
          fileBase = file;
        }
        ClassConfigInfo cnf;
        oe->getClassConfigurationInfo(cnf);
        int legMax = cnf.getNumLegsTotal();
        for (int leg = 0; leg<legMax; leg++) {
          file = fileBase + "_" + itos(leg+1) + fileEnd;
          oe->exportIOFSplits(file.c_str(), true, set<int>(), leg);
        }
      }
      else {
        int leg = lbi.data == 1 ? -1 : lbi.data - 10;
        oe->exportIOFSplits(file.c_str(), true, set<int>(), leg);
      }

      loadPage(gdi);
    }
		else if (bi.id=="SaveAs") {
      oe->sanityCheck(gdi, false);
      save(gdi, true);
			exportFileAs(hWndMain, gdi);
		}
		else if (bi.id=="Import") {
      //Import complete competition
			importFile(hWndMain, gdi);     
			loadPage(gdi);	
		}
    else if (bi.id=="Restore") {
      char bf[260];
		  getUserFile(bf, "");
      gdi.clearPage(false);
      oe->enumerateBackups(bf);

      gdi.addString("", boldLarge, "Lagrade s�kerhetskopior");
      gdi.addString("", 10, "help:restore_backup");
      gdi.addButton("Cancel", "Avbryt", CompetitionCB);
      
      oe->listBackups(gdi, ::restoreCB);
      gdi.refresh();
    }
    else if(bi.id=="Save") {
      save(gdi, true);
      resetSaveTimer();
    }
		else if (bi.id=="CloseCmp") {
			gdi.setWaitCursor(true);
			if(!showConnectionPage)
        save(gdi, false);
      oe->save();
			oe->newCompetition("");
      resetSaveTimer();
      gdi.setWindowTitle("");
			if(showConnectionPage)
        loadConnectionPage(gdi);
      else
        loadPage(gdi);
			gdi.setWaitCursor(false);
		}
		else if (bi.id=="Delete" && 
			gdi.ask("Vill du verkligen radera t�vlingen?")) {
			
      if (oe->isClient())
        oe->dropDatabase();
			else if (!oe->deleteCompetition()) 
				gdi.alert("Operation failed. It is not possible to delete competitions on server");
			
      oe->clearListedCmp();
			oe->newCompetition("");
      gdi.setWindowTitle("");
			loadPage(gdi);	
		}
		else if(bi.id=="NewCmp") {
			oe->newCompetition(lang.tl("Ny t�vling"));
			gdi.setWindowTitle("");

      if (useEventor()) {
        int age = getRelativeDay() - oe->getPropertyInt("DatabaseUpdate", 0);
        if (age>60 && gdi.ask("help:dbage")) {
          bi.id = "EventorUpdateDB";
          if (checkEventor(gdi, bi))
            return 0;
          return competitionCB(gdi, type, &bi);
        }
      }

      loadPage(gdi);
			return 0;			
		}
		else if (bi.id=="OpenCmp") {
			ListBoxInfo lbi;
      int id=0;
      bool frontPage=true;
			if (!gdi.getSelectedItem("CmpSel", &lbi)) {
        frontPage=false;
        if( gdi.getSelectedItem("ServerCmp", &lbi) )
          id=lbi.data;
        else if( gdi.getSelectedItem("LocalCmp", &lbi) )
          id=lbi.data;
      }
      else id=lbi.data;

      if(id==0)
        throw std::exception("Ingen t�vling vald");

			gdi.setWaitCursor(true);
			if(!oe->open(id))
				gdi.alert("Kunde inte �ppna t�vlingen.");
			else {
				if(gSI) gSI->SetZeroTime(oe->getZeroTimeNum());
        resetSaveTimer();
        oe->setProperty("LastCompetition", id);
        gdi.setWindowTitle(oe->getTitleName());
        oe->updateTabs();

        if(frontPage)
				  loadPage(gdi);
        else {
          oe->verifyConnection();
          oe->validateClients();
          loadConnectionPage(gdi);
        }
        return 0;
			}
		}
		else if(bi.id=="BrowseCourse") {
			string file=browseForOpen("OCAD, banexport�*.csv;*.txt;*.xml�", "csv");
			if(file.length()>0)
				gdi.setText("FileName", file);
		}
		else if (bi.id=="BrowseEntries") {
			string file=browseForOpen("Importerbara�*.xml;*.csv�IOF-xml�*.xml�Semikolonseparerad OE-standard�*.csv�", "xml");
      if(file.length()>0) {
        const char *ctrl = (const char *)bi.getExtra();
				if (ctrl != 0)
          gdi.setText(ctrl, file);
      }
		}
		else if(bi.id=="Entries") {
      if(!save(gdi, false))
        return 0;

			gdi.clearPage(true);
			gdi.addString("", 2, "Importera t�vlingsdata");
			gdi.addString("", 10, "help:import_entry_data");
			gdi.dropLine();

      gdi.pushX();
			
			gdi.fillRight();
			gdi.addInput("FileNameCmp", "", 32, 0, "T�vlingsinst�llningar (IOF-XML)");
			gdi.dropLine();
      gdi.addButton("BrowseEntries", "Bl�ddra...", CompetitionCB).setExtra("FileNameCmp");
			gdi.popX();

      gdi.dropLine(2.5);
      gdi.addInput("FileNameCls", "", 32, 0, "Klasser (IOF-XML)");
			gdi.dropLine();
      gdi.addButton("BrowseEntries", "Bl�ddra...", CompetitionCB).setExtra("FileNameCls");
			gdi.popX();

      gdi.dropLine(2.5);
      gdi.addInput("FileNameClb", "", 32, 0, "Klubbar (IOF-XML)");
			gdi.dropLine();
      gdi.addButton("BrowseEntries", "Bl�ddra...", CompetitionCB).setExtra("FileNameClb");
			gdi.popX();

      gdi.dropLine(2.5);
      gdi.addInput("FileName", "", 32, 0, "Anm�lningar (IOF-XML eller OE-CSV)");
			gdi.dropLine();
      gdi.addButton("BrowseEntries", "Bl�ddra...", CompetitionCB).setExtra("FileName");
			gdi.popX();

      gdi.dropLine(2.5);
      gdi.addInput("FileNameRank", "", 32, 0, "Ranking (IOF-XML)");
			gdi.dropLine();
      gdi.addButton("BrowseEntries", "Bl�ddra...", CompetitionCB).setExtra("FileNameRank");
			gdi.popX();

      gdi.dropLine(3);
			gdi.pushX();
			gdi.fillRight();
			gdi.addButton("DoImport", "Importera", CompetitionCB);
			gdi.fillDown();			
			gdi.addButton("Cancel", "Avbryt", CompetitionCB);
			gdi.popX();
		}
		else if(bi.id=="DoImport") {
			csvparser csv;
      gdi.enableEditControls(false);
			gdi.disableInput("DoImport");
			gdi.disableInput("Cancel");
			gdi.disableInput("BrowseEntries");

      string filename[5];
      filename[0] = gdi.getText("FileNameCmp");
      filename[1] = gdi.getText("FileNameCls");
      filename[2] = gdi.getText("FileNameClb");
      filename[3] = gdi.getText("FileName");
      filename[4] = gdi.getText("FileNameRank");

      for (int i = 0; i<5; i++) {
        if (filename[i].empty())
          continue;

        gdi.addString("", 0, "Behandlar: X#" + filename[i]);

        int type=csv.iscsv(filename[i].c_str());
  			
        if (type) {
          const char *File = filename[i].c_str();

				  if (type==1) {
					  gdi.addString("", 0, "Importerar OE2003 csv-fil...");
					  gdi.refresh();
					  gdi.setWaitCursor(true);
					  if (csv.ImportOE_CSV(*oe, File))	{
              gdi.addString("", 0, "Klart. X deltagare importerade.#" + itos(csv.nimport));
					  }
					  else gdi.addString("", 0, "F�rs�ket misslyckades.");
				  }
          else if (type==2) {
					  gdi.addString("", 0, "Importerar OS2003 csv-fil...");
					  gdi.refresh();
					  gdi.setWaitCursor(true);
					  if (csv.ImportOS_CSV(*oe, File)) {						  
              gdi.addString("", 0, "Klart. X lag importerade.#" + itos(csv.nimport));
					  }
					  else gdi.addString("", 0, "F�rs�ket misslyckades.");
          }
          else if (type==3) {
					  gdi.addString("", 0, "Importerar RAID patrull csv-fil...");
					  gdi.setWaitCursor(true);
					  if (csv.ImportRAID(*oe, File)) {						  
              gdi.addString("", 0, "Klart. X patruller importerade.#" + itos(csv.nimport));
					  }
					  else gdi.addString("", 0, "F�rs�ket misslyckades.");

          }
			  }
			  else {
          oe->importXML_EntryData(gdi, filename[i].c_str());
			  }
        gdi.setWindowTitle(oe->getTitleName());
        oe->updateTabs();
      }		  
		  gdi.addButton("Cancel", "OK", CompetitionCB);				
      gdi.refresh();
		}
		else if (bi.id=="Courses") {
      if(!save(gdi, false))
        return 0;
			gdi.clearPage(true);
			gdi.addString("", 2, "Importera banor/klasser");
			gdi.addString("", 0, "help:importcourse");
			gdi.dropLine();

			gdi.fillRight();
			gdi.pushX();
			gdi.addInput("FileName", "", 32, 0, "Filnamn");
			gdi.dropLine();
			gdi.fillDown();
			gdi.addButton("BrowseCourse", "Bl�ddra...", CompetitionCB);
			
			gdi.popX();
			gdi.pushX();
			gdi.dropLine();
			gdi.fillRight();
			gdi.addButton("DoImportCourse", "Importera", CompetitionCB);
			gdi.fillDown();					
			gdi.addButton("Cancel", "Avbryt", CompetitionCB);
			gdi.popX();
		}
		else if (bi.id=="DoImportCourse") {
			gdi.disableInput("DoImportCourses");
			gdi.disableInput("Cancel");
			gdi.disableInput("BrowseCourses");
      string filename = gdi.getText("FileName");
      runCourseImport(gdi, filename, oe);
			gdi.addButton("Cancel", "OK", CompetitionCB);			
		}
    else if (bi.id=="About") {
      loadAboutPage(gdi);
    }
	}
  else if(type==GUI_LISTBOX) {
    ListBoxInfo lbi=*(ListBoxInfo *)data;

    if(lbi.id=="LocalCmp") 
      gdi.selectItemByData("ServerCmp", -1);
    else if(lbi.id=="ServerCmp") 
      gdi.selectItemByData("LocalCmp", -1);
    else if (lbi.id=="TextSize") {
      int textSize = lbi.data;
      oe->setProperty("TextSize", textSize);
      gdi.setFont(textSize, oe->getPropertyString("TextFont", "Arial"));
      //loadPage(gdi);
      PostMessage(gdi.getTarget(), WM_USER + 2, TCmpTab, 0); 
    }
    else if (lbi.id == "Language") {
      //lang.impl->loadTable(lbi.data, lbi.text);
      lang.loadLangResource(lbi.text);
      oe->updateTabs(true);
      oe->setProperty("Language", lbi.text);
      PostMessage(gdi.getTarget(), WM_USER + 2, TCmpTab, 0); 
      //loadPage(gdi);
    }
  }
  else if (type==GUI_EVENT) {
    if( ((EventInfo*)data)->id=="Connections" ) {
      string s=gdi.getText("ClientName");
      loadConnectionPage(gdi);
      gdi.setText("ClientName", s);

    }
  }
  else if(type==GUI_CLEAR) {
    if (gdi.isInputChanged("")) {
      string name=gdi.getText("Name");

      if(!name.empty() && !oe->empty())
        save(gdi, false);
    }
    return 1;
	}
  return 0;
}


int TabCompetition::restoreCB(gdioutput &gdi, int type, void *data) {
  BackupInfo *bi =(BackupInfo *)((TextInfo *)(data))->getExtra();
  string fi(bi->FullPath);
  if (!oe->open(fi.c_str(), false)) {
    gdi.alert("Kunde inte �ppna t�vlingen.");
  }
  else {
		if(gSI) gSI->SetZeroTime(oe->getZeroTimeNum());

    const string &name = oe->getName();
    if (name.find_last_of("}") != name.length()-1)
      oe->setName(name + " {" + lang.tl("�terst�lld") +"}");
    
    oe->restoreBackup();

    gdi.setWindowTitle(oe->getTitleName());
    oe->updateTabs();
    resetSaveTimer();
		loadPage(gdi);
  }
  return 0;
}

void TabCompetition::copyrightLine(gdioutput &gdi) const
{   
  gdi.pushX();
  gdi.fillRight();
 
  gdi.addButton("About", "Om MeOS...", CompetitionCB);

  gdi.dropLine(0.4);
  gdi.fillDown(); 
  gdi.addString("", 0, MakeDash("#Copyright � 2007-2011 Melin Software HB"));
  gdi.dropLine(1);
  gdi.popX();

  gdi.addString("", 0, getMeosFullVersion()).setColor(colorDarkRed);
  gdi.dropLine(0.2);
}

void TabCompetition::loadAboutPage(gdioutput &gdi) const
{
  gdi.clearPage(false);
  gdi.addString("", 2, MakeDash("Om MeOS - ett Mycket Enkelt OrienteringsSystem")).setColor(colorDarkBlue);
  gdi.dropLine(2);
  gdi.addStringUT(1, MakeDash("Copyright � 2007-2011 Melin Software HB"));
  gdi.dropLine();
  gdi.addStringUT(10, "The database connection used is MySQL++\nCopyright "
                        "(c) 1998 by Kevin Atkinson, (c) 1999, 2000 and 2001 by MySQL AB,"
                        "\nand (c) 2004-2007 by Educational Technology Resources, Inc.\n"
                        "The database used is MySQL, Copyright (c) 2008 Sun Microsystems, Inc."
                        "\n\nGerman Translation by Erik Nilsson-Simkovics");
  
  gdi.dropLine();
  gdi.addString("", 0, "Det h�r programmet levereras utan n�gon som helst garanti. Programmet �r ");
  gdi.addString("", 0, "fritt att anv�nda och du �r v�lkommen att distribuera det under vissa villkor,");
  gdi.addString("", 0, "se license.txt som levereras med programmet.");
	
  gdi.dropLine();
  gdi.addString("", 1, "Vi st�der MeOS");
  vector<string> supp;
  getSupporters(supp);
  for (size_t k = 0; k<supp.size(); k++) 
    gdi.addStringUT(0, supp[k]);

  gdi.dropLine();
  gdi.addButton("Cancel", "St�ng", CompetitionCB);
  gdi.refresh();
}

bool TabCompetition::useEventor() const {
  return oe->getPropertyInt("UseEventor", 0) == 1;
}

bool TabCompetition::loadPage(gdioutput &gdi)
{
  showConnectionPage=false;
  oe->checkDB();
	gdi.clearPage(true);
	gdi.fillDown();

	if (oe->empty()) {
		gdi.addString("", 2, "V�lkommen till MeOS");
    gdi.addString("", 1, MakeDash("#- ")+ lang.tl("ett Mycket Enkelt OrienteringsSystem")).setColor(colorDarkBlue);
		gdi.dropLine();

    if (oe->getPropertyInt("UseEventor", 0) == 0) {
      if( gdi.ask("eventor:question#" + lang.tl("eventor:help")) )
        oe->setProperty("UseEventor", 1);
      else
        oe->setProperty("UseEventor", 2);
    }

    gdi.fillRight();
    gdi.pushX();

    gdi.addSelection("CmpSel", 300, 400, CompetitionCB, "V�lj t�vling:");

		char bf[260];
		getUserFile(bf, "");
		oe->enumerateCompetitions(bf, "*.meos");
		oe->fillCompetitions(gdi, "CmpSel",0);
		
    gdi.selectFirstItem("CmpSel");
    
    int lastCmp = oe->getPropertyInt("LastCompetition", 0);
    gdi.selectItemByData("CmpSel", lastCmp);

    gdi.dropLine();
    gdi.addButton("OpenCmp", "�ppna", CompetitionCB, "�ppna vald t�vling").setDefault();
		
    gdi.dropLine(4);
		gdi.popX();
    
		gdi.addButton("NewCmp", "Ny t�vling", CompetitionCB, "Skapa en ny, tom, t�vling");
    if (useEventor())
		  gdi.addButton("Eventor", "T�vling fr�n Eventor...", CompetitionCB, "Skapa en ny t�vling med data fr�n Eventor");
		gdi.addButton("ConnectMySQL", "Databasanslutning...", CompetitionCB, "Anslut till en server");

    gdi.popX();
    gdi.dropLine(2.5);  
    gdi.addButton("Import", "Importera t�vling...", CompetitionCB, "Importera en t�vling fr�n fil");
		gdi.addButton("Restore", "�terst�ll s�kerhetskopia...", CompetitionCB, "Visa tillg�ngliga s�kerhetskopior");
  	
    gdi.popX();
    gdi.dropLine(3);
    
    gdi.dropLine(2.3);
    textSizeControl(gdi); 
    
    gdi.setCX(gdi.getCX()+gdi.getLineHeight()*2);
    gdi.dropLine();
    gdi.addButton("Setup", "Inst�llningar...", CompetitionCB);
    
    gdi.popX();
    gdi.dropLine(3);

    gdi.addCheckbox("UseEventor", "Anv�nd Eventor", CompetitionCB, 
          useEventor(), "eventor:help");

    gdi.popX();

    gdi.fillDown();	
    gdi.dropLine(3);
    copyrightLine(gdi);

    gdi.addButton(gdi.GetPageX()-gdi.scaleLength(180), 
                  gdi.getCY()-gdi.getButtonHeight(), 
                  "Exit", "Avsluta", CompetitionCB);
    gdi.setInputFocus("CmpSel", true);
	}
	else {
		gdi.addString("", 3, "MeOS");
		gdi.dropLine();

    oe->synchronize();

		gdi.addInput("Name", oe->getName(), 24, 0, "T�vlingsnamn:");

		gdi.pushX();
		gdi.fillRight();
		gdi.addInput("Date", oe->getDate(), 8, 0, "Datum:");	
		gdi.fillDown();		
		gdi.addInput("ZeroTime", oe->getZeroTime(), 8, 0, "Nolltid:");

    if (oe->isClient()) 
      gdi.disableInput("ZeroTime");
    else {
		  gdi.popX();
		  gdi.addString("", 0, "help:zero_time");
    }

    gdi.fillRight();
    gdi.dropLine();

    if (oe->getExtIdentifier() > 0 && useEventor()) {
      gdi.addButton("SynchEventor", "Eventorkoppling", CompetitionCB, "Utbyt t�vlingsdata med Eventor");
    }

    gdi.addButton("Settings", "T�vlingsinst�llningar", CompetitionCB);
		gdi.addButton("Report", "T�vlingsrapport", CompetitionCB);
	  
    gdi.fillDown();
    gdi.popX();

    gdi.dropLine(3);

    gdi.fillRight();
    gdi.addCheckbox("UseEconomy", "Hantera klubbar och ekonomi", CompetitionCB, oe->useEconomy());
    gdi.addCheckbox("UseSpeaker", "Anv�nd speakerst�d", CompetitionCB, oe->getDCI().getInt("UseSpeaker")!=0);

    gdi.popX();
    gdi.dropLine(2);
    textSizeControl(gdi);
    
    gdi.dropLine(4);
    gdi.popX();
    gdi.fillRight();
		gdi.addButton("Save", "Spara", CompetitionCB, "help:save");
    gdi.addButton("Delete", "Radera", CompetitionCB);
		gdi.addButton("CloseCmp", "St�ng", CompetitionCB);
		
		gdi.dropLine(2.5);
    gdi.popX();

		gdi.fillRight(); 
    gdi.addButton("SaveAs", "S�kerhetskopiera", CompetitionCB);
		gdi.addButton("ConnectMySQL", "Databasanslutning", CompetitionCB);

#ifdef _DEBUG
    gdi.dropLine(2.5);
    gdi.popX();
    gdi.addButton("CloneEvent", "#! Klona", CompetitionCB);
    gdi.addButton("MultiEvent", "Hantera flera etapper", CompetitionCB);
#endif

    gdi.fillDown();
    gdi.popX();

		gdi.newColumn();
    gdi.dropLine(5);
    gdi.setCX(gdi.getCX()+gdi.scaleLength(60));

    RECT rc;
    rc.top = gdi.getCY() - gdi.scaleLength(30);
    rc.left = gdi.getCX() - gdi.scaleLength(30);

    int bw = gdi.scaleLength(150);
    gdi.addString("", 1, "Importera t�vlingsdata");
    gdi.addButton(gdi.getCX(), gdi.getCY(), bw, "Entries", "Anm�lningar", 
                  CompetitionCB, "",  false, false);
		gdi.addButton(gdi.getCX(), gdi.getCY(), bw, "FreeImport", "Fri anm�lningsimport", 
                  CompetitionCB, "", false, false);
    gdi.addButton(gdi.getCX(), gdi.getCY(), bw, "Courses", "Banor",
                  CompetitionCB, "", false, false);
		
    gdi.dropLine();
		gdi.addString("", 1, "Exportera t�vlingsdata");
    gdi.addButton(gdi.getCX(), gdi.getCY(), bw, "Startlist", "Startlista", 
                  CompetitionCB, "Exportera startlista p� fil", false, false);
		gdi.addButton(gdi.getCX(), gdi.getCY(), bw, "Splits", "Resultat && str�cktider", 
                  CompetitionCB, "Exportera resultat p� fil", false, false);
		
		gdi.dropLine();
    gdi.addString("", 1, "L�pardatabasen");
		gdi.addButton(gdi.getCX(), gdi.getCY(), bw, "DBaseIn", "Importera", 
                  CompetitionCB, "", false, false);
		gdi.addButton(gdi.getCX(), gdi.getCY(), bw, "ExportSetup", "Exportera",
                  CompetitionCB, "", false, false);

    rc.bottom = gdi.getCY() + gdi.scaleLength(30);
    rc.right = rc.left + bw + gdi.scaleLength(60);
  	
    gdi.addRectangle(rc, colorLightBlue);

    gdi.popX();
    gdi.dropLine(5);
    copyrightLine(gdi);

    gdi.setOnClearCb(CompetitionCB);
	}
	gdi.refresh();
  return true;
}

void TabCompetition::textSizeControl(gdioutput &gdi) const 
{
  int s = oe->getPropertyInt("TextSize", 0);
  const char *id="TextSize";
  gdi.fillRight();
  //gdi.addString("", 0, "Textstorlek:");
  gdi.dropLine(-0.5);
  gdi.addSelection(id, 90, 200, CompetitionCB, "Textstorlek:");
  gdi.addItem(id, lang.tl("Normal"), 0);
  gdi.addItem(id, lang.tl("Stor"), 1);
  gdi.addItem(id, lang.tl("St�rre"), 2);
  gdi.addItem(id, lang.tl("St�rst"), 3);
  gdi.selectItemByData(id, s);

  id = "Language";
  gdi.addSelection(id, 90, 200, CompetitionCB, "Spr�k:");
  vector<string> ln = lang.getLangResource();
  string current = oe->getPropertyString("Language", "Svenska");  
  int ix = -1;
  for (size_t k = 0; k<ln.size(); k++) {
    gdi.addItem(id, ln[k], k);
    if (ln[k] == current)
      ix = k;
  }
  //gdi.addItem(id, "English", 104);
  gdi.selectItemByData(id, ix);
}

int TabCompetition::getOrganizer(bool updateEvent) {
  string apikey = oe->getPropertyStringDecrypt("apikey", "");
  if (apikey.empty())
    return 0;
  if (!isAscii(apikey))
    return 0;

  Download dwl;
  dwl.initInternet();
  vector< pair<string, string> > key;
  string file = getTempFile();
  key.push_back(pair<string, string>("ApiKey", apikey));
  string url = eventorBase + "organisation/apiKey";
  try {
    dwl.downloadFile(url, file, key);
  }
  catch (dwException &ex) {
    if (ex.code == 403)
      return 0;
    else {
      throw std::exception("Kunde inte ansluta till Eventor.");
    }
  }
  catch (std::exception &) {
    throw std::exception("Kunde inte ansluta till Eventor.");
  }

  dwl.createDownloadThread();
  while (dwl.isWorking()) {
    Sleep(50);
  }

  int clubId = 0;

  xmlparser xml;
  xmlList xmlEvents;
  try {
    xml.read(file.c_str());
    xmlobject obj = xml.getObject("Organisation");
    if (obj) {
      clubId = obj.getObjectInt("OrganisationId");
      obj.getObjectString("Name", eventor.name);

      xmlobject ads = obj.getObject("Address");
      if (ads) {
        ads.getObjectString("careOf", eventor.careOf);
        ads.getObjectString("street", eventor.street);
        ads.getObjectString("city", eventor.city);
        ads.getObjectString("zipCode", eventor.zipCode);
      }

      xmlobject tele = obj.getObject("Tele");

      if (tele) {
        tele.getObjectString("mailAddress", eventor.email);
      }

      xmlobject aco = obj.getObject("Account");
      if (aco) {
        aco.getObjectString("AccountNo", eventor.account);
      }
    }
  }
  catch (std::exception &) {
    removeTempFile(file);
    throw;
  }

  removeTempFile(file);

  return clubId;
}

void TabCompetition::getAPIKey(vector< pair<string, string> > &key) const {  
  string apikey = oe->getPropertyStringDecrypt("apikey", "");

  if (apikey.empty() || organizorId == 0)
    throw std::exception("Internal error");

  key.clear();
  key.push_back(pair<string, string>("ApiKey", apikey));
}

void TabCompetition::getEventorCompetitions(gdioutput &gdi,
                                            const string &fromDate,
                                            vector<CompetitionInfo> &events) const 
{
  events.clear();

  vector< pair<string, string> > key;
  getAPIKey(key);

  string file = getTempFile();
  string url = eventorBase + "events?fromDate=" + fromDate +
              "&organisationIds=" + itos(organizorId) + "&includeEntryBreaks=true";
  Download dwl;
  dwl.initInternet();

  try {
    dwl.downloadFile(url, file, key);
  }
  catch (std::exception &) {
    removeTempFile(file);
    throw;
  }

  dwl.createDownloadThread();
  while (dwl.isWorking()) {
    Sleep(100);
  }
  xmlparser xml;
  xmlList xmlEvents;
  
  try {
    xml.read(file.c_str());
    xmlobject obj = xml.getObject("EventList");
    obj.getObjects("Event", xmlEvents);
  }
  catch (std::exception &) {
    removeTempFile(file);
    throw;
  }

  removeTempFile(file);

  for (size_t k = 0; k < xmlEvents.size(); k++) {
    CompetitionInfo ci;
    xmlEvents[k].getObjectString("Name", ci.Name);
    ci.Id = xmlEvents[k].getObjectInt("EventId");
    xmlobject date = xmlEvents[k].getObject("StartDate");
    date.getObjectString("Date", ci.Date);
    if (date.getObject("Clock"))
      date.getObjectString("Clock", ci.firstStart);

    xmlEvents[k].getObjectString("WebURL", ci.url);
    xmlobject aco = xmlEvents[k].getObject("Account");
    if (aco) {
      string type = aco.getAttrib("type").get();
      string no;
      aco.getObjectString("AccountNo", no);

      if (type == "bankGiro")
        ci.account = "BG " + no;
      else if (type == "postalGiro")
        ci.account = "PG " + no;
      else
        ci.account = no;
    }

    ci.lastNormalEntryDate = "";
    xmlList entryBreaks;
    xmlEvents[k].getObjects("EntryBreak", entryBreaks);
    /* Mats Troeng explains Entry Break 2011-04-03:
    Efteranm�lan i detta fall �r satt som en till�ggsavgift (+50%) p� ordinarie avgift. 
    Till�ggsavgiften �r aktiv 2011-04-13 -- 2011-04-20, medan den ordinarie avgiften �r aktiv -- 2011-04-20. Man kan ocks� 
    definiera enligt ditt andra exempel i Eventor om man vill, men d� m�ste man s�tta ett fixt belopp i st�llet f�r en 
    procentsats f�r efteranm�lan eftersom det inte finns n�got belopp att ber�kna procentsatsen p�.

    F�r att f� ut anm�lningsstoppen f�r en t�vling tittar man allts� p� unionen av alla (ValidFromDate - 1 sekund) 
    samt ValidToDate. I normalfallet �r det tv� stycken, varav det f�rsta �r ordinarie anm�lningsstopp. 
    F�r t ex O-Ringen som har flera anm�lningsstopp blir det mer �n tv� EntryBreaks.
    */
    for (size_t k = 0; k<entryBreaks.size(); k++) {
      xmlobject eBreak = entryBreaks[k].getObject("ValidFromDate");
      if (eBreak) {
        string breakDate;
        eBreak.getObjectString("Date", breakDate);

        SYSTEMTIME st;
        convertDateYMS(breakDate, st);
        __int64 time = SystemTimeToInt64Second(st) - 1;        
        breakDate = convertSystemDate(Int64SecondToSystemTime(time));

        if (ci.lastNormalEntryDate.empty() || ci.lastNormalEntryDate >= breakDate)
          ci.lastNormalEntryDate = breakDate;
      }

      eBreak = entryBreaks[k].getObject("ValidToDate");
      if (eBreak) {
        string breakDate;
        eBreak.getObjectString("Date", breakDate);
        if (ci.lastNormalEntryDate.empty() || ci.lastNormalEntryDate >= breakDate)
          ci.lastNormalEntryDate = breakDate;

      }
    }

    events.push_back(ci);
  }
}

void TabCompetition::getEventorCmpData(gdioutput &gdi, int id, 
                                       const string &eventFile,
                                       const string &clubFile,
                                       const string &classFile,
                                       const string &entryFile,
                                       const string &dbFile) const 
{
  ProgressWindow pw(gdi.getHWND());
  pw.init();
  gdi.fillDown();
  gdi.addString("", 1, "Ansluter till Internet").setColor(colorGreen);
  gdi.dropLine(0.5);
  gdi.refreshFast();
  Download dwl;
  dwl.initInternet();

  pw.setProgress(1);
  vector< pair<string, string> > key;
  string apikey = oe->getPropertyStringDecrypt("apikey", ""); 
  key.push_back(pair<string, string>("ApiKey", apikey));
  
  gdi.fillRight();
  
  int prg = 0;
  int event_prg = dbFile.empty() ? 1000 / 4 : 1000/6;
  int club_prg = event_prg;

  if (id > 0) {
    gdi.addString("", 0, "H�mtar t�vling...");
    gdi.refreshFast();
    dwl.downloadFile(eventorBase + "export/event?eventId=" + itos(id), eventFile, key);
    dwl.createDownloadThread();
    while (dwl.isWorking()) {
      Sleep(100);
    }
    if (!dwl.successful())
      throw std::exception("Download failed");

    prg += int(event_prg * 0.2);
    pw.setProgress(prg);
    gdi.addString("", 0, "OK");
    gdi.popX();
    gdi.dropLine();

    gdi.addString("", 0, "H�mtar klasser...");
    gdi.refreshFast();
    dwl.downloadFile(eventorBase + "export/classes?eventId=" + itos(id), classFile, key);
    dwl.createDownloadThread();
    while (dwl.isWorking()) {
      Sleep(100);
    }
    
    if (!dwl.successful())
      throw std::exception("Download failed");

    prg += event_prg;
    pw.setProgress(prg);
    gdi.addString("", 0, "OK");
    gdi.popX();
    gdi.dropLine();

    
    gdi.addString("", 0, "H�mtar anm�lda...");
    gdi.refreshFast();
    dwl.downloadFile(eventorBase + "/export/entries?eventId=" + itos(id), entryFile, key);
    dwl.createDownloadThread();
    while (dwl.isWorking()) {
      Sleep(100);
    }
    if (!dwl.successful())
      throw std::exception("Download failed");

    prg += int(event_prg * 1.8);
    pw.setProgress(prg);
    gdi.addString("", 0, "OK");
    gdi.popX();
    gdi.dropLine();
  }


  gdi.addString("", 0, "H�mtar klubbar...");
  gdi.refreshFast();
  dwl.downloadFile(eventorBase + "export/clubs", clubFile, key);
  dwl.createDownloadThread();
  while (dwl.isWorking()) {
    Sleep(100);
  }
  if (!dwl.successful())
    throw std::exception("Download failed");

  prg += club_prg;
  pw.setProgress(prg);
  gdi.addString("", 0, "OK");
  gdi.popX();
  gdi.dropLine();

  if (dbFile.length() > 0) {
    gdi.addString("", 0, "H�mtar l�pardatabasen...");
    gdi.refreshFast();
    dwl.downloadFile(eventorBase + "export/cachedcompetitors?organisationIds=1&includePreselectedClasses=false&zip=true", dbFile, key);
    dwl.createDownloadThread();
    while (dwl.isWorking()) {
      Sleep(100);
    }
  
    if (!dwl.successful())
      throw std::exception("Download failed");

    pw.setProgress(1000);
    gdi.addString("", 0, "OK");
  }
 
  gdi.popX();
  gdi.dropLine();
}

void TabCompetition::loadMultiEvent(gdioutput &gdi) {
  gdi.clearPage(false);

  gdi.addString("", boldLarge, "Hantera flera etapper");

  gdi.dropLine();

  gdi.pushX();
  gdi.fillRight();

  gdi.addSelection("PreEvent", 200, 200, CompetitionCB, "F�reg�ende etapp:", "V�lj den etapp som f�reg�r denna t�vling");
  oe->fillCompetitions(gdi, "PreEvent", 0);
  gdi.addItem("PreEvent", "Ingen / ok�nd", -2);
  gdi.selectItemByData("PreEvent", -2);
  
  gdi.addSelection("PostEvent", 200, 200, CompetitionCB, "N�sta etapp:", "V�lj den etapp som kommer efter denna t�vling");
  oe->fillCompetitions(gdi, "PostEvent", 0);  
  gdi.addItem("PostEvent", "Ingen / ok�nd", -2);
  gdi.selectItemByData("PostEvent", -2);

  gdi.dropLine(4);
  gdi.popX();

  gdi.addButton("OpenPost", "�ppna f�reg�ende", CompetitionCB, "�ppna f�reg�ende etapp");
  gdi.addButton("OpenPre", "�ppna n�sta", CompetitionCB, "�ppna n�sta etapp");

  gdi.dropLine(3);
  gdi.popX();

  gdi.addButton("CloneCmp", "L�gg till ny etapp...", CompetitionCB);
  gdi.addButton("TransferData", "�verf�r resultat till n�sta etapp", CompetitionCB);

  gdi.addButton("Cancel", "�terg�", CompetitionCB);

  gdi.refresh();
}
