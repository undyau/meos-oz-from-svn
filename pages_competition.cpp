#include "stdafx.h"

#include "resource.h"

#include <commctrl.h>
#include <commdlg.h> 

#include "oEvent.h"
#include "xmlparser.h"

#include "gdioutput.h"
#include "commctrl.h"

#include "oEvent.h"

#include "csvparser.h"
#include "SportIdent.h"
#include "TabCompetition.h"



extern SportIdent *gSI;
extern pEvent gEvent;

void LoadCompetitionPage(gdioutput &gdi);
extern HINSTANCE hInst;
extern HWND hWndMain;


string BrowseForOpen(string Filter, string defext)
{
	InitCommonControls();
	
	char FileName[260];
	FileName[0]=0;
	char sbuff[256];
	OPENFILENAME of;
//	LoadString(hInst,1016,sbuff,256);
	sprintf_s(sbuff, 256, "%s", Filter.c_str());

	int sl=strlen(sbuff);
	for(int m=0;m<sl;m++) if(sbuff[m]=='¤') sbuff[m]=0;

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

string BrowseForSave(string Filter, string defext, int &FilterIndex)
{
	InitCommonControls();
	
	char FileName[260];
	FileName[0]=0;
	char sbuff[256];
	OPENFILENAME of;
//	LoadString(hInst,1016,sbuff,256);
	sprintf_s(sbuff, 256,"%s", Filter.c_str());

	int sl=strlen(sbuff);
	for(int m=0;m<sl;m++) if(sbuff[m]=='¤') sbuff[m]=0;

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
	of.Flags            =OFN_OVERWRITEPROMPT|OFN_HIDEREADONLY;
	of.lpstrDefExt   	  = defext.c_str();
	of.lpfnHook		     =NULL;
	
	
	if(GetSaveFileName(&of)==false)
		return "";

	FilterIndex=of.nFilterIndex;

	return FileName;
}



bool ImportFile(HWND hWnd, gdioutput *gdi)
{
	InitCommonControls();
	
	char FileName[260];
	FileName[0]=0;
	char sbuff[256];
	OPENFILENAME of;
//	LoadString(hInst,1016,sbuff,256);
	sprintf_s(sbuff, 256, "XML-data¤*.xml;*.bu?¤");

	int sl=strlen(sbuff);
	for(int m=0;m<sl;m++) if(sbuff[m]=='¤') sbuff[m]=0;

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
	
//	MapSourceFile msf;
	gdi->SetWaitCursor(true);
	if(gEvent->Open(FileName, true))
	{
		if(gSI) gSI->SetZeroTime(gEvent->GetZeroTimeNum());
		gdi->SetWindowTitle(gEvent->GetName());
		return true;
	}

	return false;
}

bool ExportFileAs(HWND hWnd, gdioutput *gdi)
{
	InitCommonControls();
	

	char FileName[260];
	FileName[0]=0;
	char sbuff[256];

	OPENFILENAME of;
	//LoadString(hInst,1016,sbuff,256);
	sprintf_s(sbuff, 256, "XML-data¤*.xml¤");
	
	int sl=strlen(sbuff);
	for(int m=0;m<sl;m++) if(sbuff[m]=='¤') sbuff[m]=0;


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
	of.Flags            =OFN_OVERWRITEPROMPT|OFN_HIDEREADONLY;
	of.lpstrDefExt   	  ="xml";
	of.lpfnHook		     =NULL;
	
	
	if(GetSaveFileName(&of)==false)
		return false;

	gdi->SetWaitCursor(true);
	if(!gEvent->Save(FileName))
	{
		gdi->alert("Fel: Filen "+string(FileName)+ " kunde inte skrivas.");
		return false;
	}

	return true;
//	return mps.WriteMap(FileName);

	//return WriteMapFile(FileName);
}









int CompetitionCB(gdioutput *gdi, int type, void *data)
{
	if(type==GUI_BUTTON)
	{
		ButtonInfo bi=*(ButtonInfo *)data;

		if(bi.id=="Print")
		{
			//gdi->AddInfoBox("test", "Jag behöver \nhjälp..", 1000, CompetitionCB);
			DATASET ds;
			memset(&ds, 0, sizeof(ds));

			gdi->Print(gEvent);
		}
		else if (bi.id=="ConnectMySQL") {
			gdi->SetRestorePoint();
			gdi->AddInput("Server", "localhost", 16, 0, "MySQL Server / IP-adress:", "IP-adress eller namn på en MySQL-server");
			//gdi->AddInput("Server", "localhost", 16, 0, "Server", "IP-adress eller namn på en MySQL-server");
			gdi->AddButton("ConnectToMySQL", "Anslut", CompetitionCB);  
		}
		else if (bi.id=="ConnectToMySQL") {
			bool s=gEvent->ConnectToMySQL(gdi->getText("Server"), "meos", "", 0);

			if(s){
				LoadCompetitionPage(*gdi);

				if (!gEvent->Empty() && gdi->Ask("Vill du ladda upp [synkornisera!?] den lokala tävlingen på servern?")) {
					gEvent->UploadSyncronize();
				}
			}
			else gdi->alert("Connecting failed.");
		}
		else if(bi.id=="Connect"){
			gEvent->testsql(0);
		}
		else if(bi.id=="ConnectRead"){
			gEvent->testsql(1);
//							if(gSI) gSI->SetZeroTime(gEvent->GetZeroTimeNum());
			gdi->SetWindowTitle(gEvent->GetName());
			LoadCompetitionPage(*gdi);
		}
		else if(bi.id=="Cancel"){
			LoadCompetitionPage(*gdi);
		}
		else if(bi.id=="Results"){
			int FilterIndex=1;
			string save=BrowseForSave("Webben (html)¤*.html¤OE Semikolonseparerad (csv)¤*.csv¤", "html", FilterIndex);

			if(save.length()>0)
			{
				gdi->SetWaitCursor(true);

				if(FilterIndex==1)
					gEvent->GenerateResultlist(save);
				else
					gdi->alert("Not implemented");
				//gEvent->ExportIOFSplits(save.c_str());
			}
		}
		else if(bi.id=="Startlist")
		{
			int FilterIndex=1;
			string save=BrowseForSave("Webben (html)¤*.html¤OE Semikolonseparerad (csv)¤*.csv¤", "html", FilterIndex);

			if(save.length()>0)
			{
				gdi->SetWaitCursor(true);

				if(FilterIndex==1)
					gEvent->GenerateStartlist(save);
				else
					gdi->alert("Not implemented");
				//gEvent->ExportIOFSplits(save.c_str());
			}
		}
		else if(bi.id=="Splits")
		{
			int FilterIndex=1;
			string save=BrowseForSave("IOF Results (XML)¤*.xml¤", "xml", FilterIndex);


			if(save.length()>0)
			{
				gdi->SetWaitCursor(true);
				gEvent->ExportIOFSplits(save.c_str());
			}
		}
		else if(bi.id=="ExportCSV")
		{
			int FilterIndex=1;
			string save=BrowseForSave("OE2003 (CSV)¤*.csv¤", "csv", FilterIndex);

			if(save.length()>0)
			{
				gdi->SetWaitCursor(true);
				gEvent->ExportOECSV(save.c_str());
			}
		}
		else if(bi.id=="SaveAs")
		{
			ExportFileAs(hWndMain, gdi);
		}
		else if(bi.id=="Import")
		{
			ImportFile(hWndMain, gdi);
			LoadCompetitionPage(*gdi);	
		}
		else if(bi.id=="Save")
		{
			string name=gdi->getText("Name");

			if(name.length()==0)
			{
				gdi->alert("Tävlingen måste ha ett namn");
				return 0;
			}

			gEvent->SetName(gdi->getText("Name"));
			gEvent->SetDate(gdi->getText("Date"));
			gEvent->SetZeroTime(gdi->getText("ZeroTime"));
			if(gSI) gSI->SetZeroTime(gEvent->GetZeroTimeNum());

			gEvent->GetEventDI().SaveDataFields(*gdi);

			gdi->SetWindowTitle(gEvent->GetName());

			gdi->SetWaitCursor(true);
			gEvent->Save();
		}
		else if(bi.id=="Close")
		{
			gdi->SetWaitCursor(true);
			gEvent->Save();
			gEvent->NewCompetition("");
			gdi->SetWindowTitle("");
			LoadCompetitionPage(*gdi);	
			gdi->SetWaitCursor(false);
		}
		else if (bi.id=="Delete" && 
			gdi->Ask("Vill du verkligen radera tävlingen?")) {
			
			if (!gEvent->deleteCompetition()) {
				gdi->Notify("Operation failed. It is not possible to delete competitions on server");
			}
			
			gEvent->NewCompetition("");
			gdi->SetWindowTitle("");
			LoadCompetitionPage(*gdi);	
		}
		else if(bi.id=="NewCmp")
		{
			gEvent->NewCompetition("Ny tävling");
			gdi->SetWindowTitle("");
			LoadCompetitionPage(*gdi);
			return 0;			
		}
		else if(bi.id=="OpenCmp")
		{
			ListBoxInfo lbi;
			gdi->getSelectedItem("CmpSel", &lbi);

			gdi->SetWaitCursor(true);
			if(!gEvent->Open(lbi.data))
				gdi->alert("Kunde inte öppna tävlingen");
			else
			{
				if(gSI) gSI->SetZeroTime(gEvent->GetZeroTimeNum());
				gdi->SetWindowTitle(gEvent->GetName());
				LoadCompetitionPage(*gdi);
				return 0;
			}
		}
		else if(bi.id=="BrowseCourse")
		{
			string file=BrowseForOpen("OCAD, banexport¤*.csv¤", "csv");
			if(file.length()>0)
				gdi->setText("FileName", file);
		}
		else if(bi.id=="BrowseEntries")
		{
			string file=BrowseForOpen("IOF-xml¤*.xml¤Semikolonseparerad OE-standard¤*.csv¤", "xml");
			if(file.length()>0)
				gdi->setText("FileName", file);
		}
		else if(bi.id=="Entries")
		{
			gdi->ClearPage();
			gdi->AddString("", 2, "Importera löpare");
			gdi->AddString("", 0, "Du kan importera löpare och anmälningar från en antal olika text- och XML-format.");
			gdi->dropLine();

			gdi->fillRight();
			gdi->pushX();
			gdi->AddInput("FileName", "F:\\downloads\\test_anm.csv", 32, 0, "Filnamn");
			gdi->dropLine();
			gdi->fillDown();
			gdi->AddButton("BrowseEntries", "Bläddra...", CompetitionCB);
			
			gdi->popX();
			gdi->dropLine();
			gdi->pushX();
			gdi->fillRight();
			gdi->AddButton("DoImport", "Importera", CompetitionCB);
			gdi->fillDown();			
			gdi->AddButton("Cancel", "Avbryt", CompetitionCB);
			gdi->popX();
		}
		else if(bi.id=="DoImport")
		{
			csvparser csv;
			int type=csv.iscsv(gdi->getText("FileName").c_str());
			if(type)
			{
				gdi->disableInput("DoImport");
				gdi->disableInput("Cancel");
				gdi->disableInput("BrowseEntries");

				if(type==1){
					gdi->AddString("", 0, "Försöker importera OE2003 csv-fil...");
					gdi->refresh();
					gdi->SetWaitCursor(true);
					if(csv.ImportOE_CSV(*gEvent, gdi->getText("FileName").c_str()))
					{
						char bf[128];
						sprintf_s(bf, 128, "Klart. %d personer importerade", csv.nimport);
						gdi->AddString("", 0, bf);
					}
					else gdi->AddString("", 0, "Försöket misslyckades.");
				}
				else{
					gdi->AddString("", 0, "Försöker importera OS2003 csv-fil...");
					gdi->refresh();
					gdi->SetWaitCursor(true);
					if(csv.ImportOS_CSV(*gEvent, gdi->getText("FileName").c_str()))
					{
						char bf[128];
						sprintf_s(bf, 128, "Klart. %d lag importerade", csv.nimport);
						gdi->AddString("", 0, bf);
					}
					else gdi->AddString("", 0, "Försöket misslyckades.");
				}

				gdi->refresh();
				gdi->AddButton("Cancel", "OK", CompetitionCB);				
			}
			else{
				//xmlparser xml;
				//xml.Read(gdi->getText("FileName").c_str());				
				gEvent->ImportXMLData(gdi->getText("FileName").c_str());
			}
		}
		else if(bi.id=="Courses")
		{
			gdi->ClearPage();
			gdi->AddString("", 2, "Importera banor/klasser");
			gdi->AddString("", 0, "Du kan importera banor och klasser från OCADs exportformat.");
			gdi->dropLine();

			gdi->fillRight();
			gdi->pushX();
			gdi->AddInput("FileName", "F:\\downloads\\test_anm.csv", 32, 0, "Filnamn");
			gdi->dropLine();
			gdi->fillDown();
			gdi->AddButton("BrowseCourse", "Bläddra...", CompetitionCB);
			
			gdi->popX();
			gdi->pushX();
			gdi->dropLine();
			gdi->fillRight();
			gdi->AddButton("DoImportCourse", "Importera", CompetitionCB);
			gdi->fillDown();					
			gdi->AddButton("Cancel", "Avbryt", CompetitionCB);
			gdi->popX();
		}
		else if(bi.id=="DoImportCourse")
		{

				gdi->disableInput("DoImportCourses");
				gdi->disableInput("Cancel");
				gdi->disableInput("BrowseCourses");
				gdi->AddString("", 0, "Försöker importera OCAD csv-fil...");
				gdi->refresh();
				
				csvparser csv;
	
				if(csv.ImportOCAD_CSV(*gEvent, gdi->getText("FileName").c_str()))
				{
					gdi->AddString("", 0, "Klart.");
				}
				else gdi->AddString("", 0, "Försöket misslyckades.");
	
				gdi->refresh();
				gdi->AddButton("Cancel", "OK", CompetitionCB);			
		}
	}
	return 0;
}

void LoadCompetitionPage(gdioutput &gdi)
{
	gdi.ClearPage();
	gdi.fillDown();

	if(gEvent->Empty())
	{
		gdi.AddString("", 2, "Välkomen till MeOS");
		gdi.AddString("", 1, "- ett Mycket Enkelt OrienteringsSystem");
		gdi.dropLine();
	
		gdi.AddSelection("CmpSel", 300, 400, CompetitionCB, "Välj tävling");
	
		char bf[260];
		GetUserFile(bf, "");
		gEvent->EnumerateCompetitions(bf, "*.meos");		
		gEvent->FillCompetitions(gdi, "CmpSel");
		gdi.SelectFirstElement("CmpSel");

		gdi.AddButton("OpenCmp", "Öppna", CompetitionCB);
		gdi.dropLine();

		gdi.pushX();
		gdi.fillRight();
		gdi.AddButton("NewCmp", "Ny tävling", CompetitionCB);
		
		gdi.fillDown();		

		gdi.AddButton("Import", "Importera tävling...", CompetitionCB);

		gdi.popX();

		//gdi.AddButton("ConnectRead", "Läs SQL...", CompetitionCB);
		gdi.AddButton("ConnectMySQL", "Anslut till MySQL...", CompetitionCB);
	}
	else
	{
		gdi.AddString("", 3, "MeOS");

	//	gdi.NewRow();
		//gdi.AddString("", 0, "MEOS");
		gdi.dropLine();
	
		//gdi.AddString("", 1, "Grundinställningar:");

		gdi.AddInput("Name", gEvent->GetName(), 24, 0, "Tävlingsnamn:");

		gdi.pushX();
		gdi.fillRight();
		gdi.AddInput("Date", gEvent->GetDate(), 8, 0, "Datum:");	
		gdi.fillDown();		
		gdi.AddInput("ZeroTime", gEvent->GetZeroTime(), 8, 0, "Nolltid:");

		gdi.popX();
		gdi.AddString("", 0, "Nolltid bör sättas till en timme före första start.");
	
		
		gdi.AddButton("Save", "Spara", CompetitionCB);
		
		gdi.dropLine();
		gdi.AddButton("SaveAs", "Exportera / Säkerhetskopiera...", CompetitionCB);
		gdi.dropLine();
		gdi.dropLine();
		
	gdi.fillRight(); 
		gdi.AddButton("Close", "Stäng", CompetitionCB);
		gdi.AddButton("Delete", "Radera...", CompetitionCB);
		gdi.fillDown();
		gdi.AddButton("ConnectMySQL", "Anslut till MySQL...", CompetitionCB);
		//gdi.AddButton("Connect", "Anslut SQL...", CompetitionCB);
		
		gdi.newColumn();

		gdi.AddButton("Entries", "Importera anmälningar...", CompetitionCB);
		gdi.AddButton("Courses", "Importera banor...", CompetitionCB);
		gdi.dropLine();
		gdi.AddButton("Startlist", "Exportera startlista...", CompetitionCB);
		gdi.AddButton("Results", "Exportera resultat...", CompetitionCB);
		gdi.AddButton("Splits", "Exportera sträcktider...", CompetitionCB);
		gdi.AddButton("ExportCSV", "Exportera datafil (csv)...", CompetitionCB);
		
		gdi.dropLine();
		gdi.AddButton("DBaseIn", "Importera löpardatabas", CompetitionCB);
		gdi.AddButton("DBaseOut", "Exportera löpardatabas", CompetitionCB);
		gdi.disableInput("DBaseIn");
		gdi.disableInput("DBaseOut");


		//gdi.newColumn();

		//gEvent->GetEventDI().BuildDataFields(gdi);
		//gEvent->GetEventDI().FillDataFields(gdi);
	}
	gdi.refresh();
}