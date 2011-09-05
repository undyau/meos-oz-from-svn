#include "stdafx.h"
#include "resource.h"


#include "oEvent.h"
#include "xmlparser.h"

#include "gdioutput.h"
#include "commctrl.h"

#include "oEvent.h"

#include "SportIdent.h"

#include <shellapi.h>

extern pEvent gEvent;

static string SelectedList;
static char sWaveFolder[260]={0};

void LoadRunnerPage(gdioutput &gdi);

void LoadListsPage(gdioutput &gdi);
int ListsCB(gdioutput *gdi, int type, void *data);

int OpenRunnerCB(gdioutput *gdi, int type, void *data);


int ListsEventCB(gdioutput *gdi, int type, void *data)
{
	if(type!=GUI_EVENT)
		return -1;

	gdi->SendCtrlMessage(SelectedList);

	return 0;
}


int PreWarningCB(gdioutput *gdi, int type, void *data)
{
	//MessageBox(NULL, "DATA UPDATE", NULL, MB_OK);
	list<int> n;
	gEvent->PlayPrewarningSounds(sWaveFolder, n);
	return 0;
}

int OpenRunnerCB(gdioutput *gdi, int type, void *data)
{
	//gdi->ClearPage();
	if(type==GUI_LINK)
	{
		TextInfo *ti=(TextInfo *)data;
		int id=atoi(ti->id.c_str());

		LoadRunnerPage(*gdi);

		gdi->MakeEvent("LoadRunner", id);
	}
	return 0;
}


int NoStartRunnerCB(gdioutput *gdi, int type, void *data)
{
	//gdi->ClearPage();
	if(type==GUI_LINK)
	{
		TextInfo *ti=(TextInfo *)data;
		int id=atoi(ti->id.c_str());

		//LoadRunnerPage();
		//gdi->MakeEvent("LoadRunner", id);
		pRunner p=gEvent->GetRunner(id);
		if(p){
			p->SetStatus(oRunner::StatusDNS);
			p->Syncronize();
			ti->CallBack=0;
			ti->Highlight=false;
			ti->Active=false;
			ti->color=RGB(255,0,0);
			gdi->SetText(ti->id, "Ej start", true);
		}
		
	}
	return 0;
}



int SpeakerListCB(gdioutput *gdi, int type, void *data)
{
	if(type==GUI_BUTTON){
		ButtonInfo bi=*(ButtonInfo *)data;

		if(bi.id=="SelectClass"){
			
			list<pControl> controls;
			int ClassId=0;
			ListBoxInfo lbi;
			if(gdi->GetSelectedItem("SpeakerClass", &lbi)){
				ClassId=lbi.data;
				pClass pc=gEvent->GetClass(ClassId, "NoCreate");
				if(pc){
					//To do: Somehow find relevant controls...

					gdi->ClearPage();	
					gdi->SetData("ClassId", pc->GetId());

					pCourse course=pc->GetCourse();
					if(course){						
						course->GetControls(controls);						
					}
				}
			}
			
			if(controls.empty())
				gdi->Alert("Hittade inga kontroller tillhörande klassen...");

			gdi->FillRight();
			gdi->PushX();
			gdi->AddSelection("SpeakerClass", 200, 400, SpeakerListCB, "Välj klass:");
			gEvent->FillClasses(*gdi, "SpeakerClass");
			gdi->SelectedItemByData("SpeakerClass", lbi.data);
			gdi->DropLine();
			gdi->AddButton("SelectClass", "Välj", SpeakerListCB);
			gdi->FillDown();
			gdi->AddButton("Cancel", "Återgå", ListsCB);
			gdi->PopX();

			list<pControl>::iterator it;

			gdi->FillRight();
			for(it=controls.begin(); it!=controls.end(); ++it){				
				if(gdi->GetCX()>=gdi->GetTargetWidth()-100){
					gdi->PopX();
					gdi->DropLine();
					gdi->DropLine();					
				}
				char bf[16];
				sprintf_s(bf, "_C%d", (*it)->GetId());
				gdi->AddButton(bf, (*it)->GetName(), SpeakerListCB);					
			}

			gdi->FillDown();
			gdi->AddButton("_F", "Mål", SpeakerListCB);
			gdi->PopX();
			gdi->SetRestorePoint();
		}
		else if(bi.id=="_F"){
			gdi->Restore();
			DWORD ClassID;				
			if(gdi->GetData("ClassId", ClassID))
				gEvent->SpeakerList(*gdi, ClassID, oPunch::PunchFinish);
		}
		else if(bi.id.substr(0,2)=="_C"){
			int id=atoi(bi.id.substr(2).c_str());
			gdi->Restore();

			DWORD ClassID;				
			if(gdi->GetData("ClassId", ClassID)){
				gEvent->SpeakerList(*gdi, ClassID, id);
			}
		}
	}
	else if(type==GUI_LISTBOX){
		ListBoxInfo bi=*(ListBoxInfo *)data;

		if(bi.id=="SpeakerClass"){			
			//pControl pc=gEvent->GetControl(bi.data);
			//SelectControl(gdi, pc);

			//gEvent->SpeakerList(*gdi, bi.data);
			//gdi->AddButton("Återgå", gdi->GetWidth()+20, 15, "Cancel", ListsCB, "", "", true);
		}
	}
	return 0;
}


int ListsCB(gdioutput *gdi, int type, void *data)
{
	int index;

	if(type==GUI_BUTTON || type==GUI_EVENT)
	{
		//ButtonInfo bi=*(ButtonInfo *)data;
		BaseInfo bi=*(BaseInfo *)data;

		if (bi.id=="Cancel")	{
			SelectedList="";
			LoadListsPage(*gdi);
			
		}
		else if (bi.id=="Print")	{
			gdi->Print(gEvent);			
		}
		else if (bi.id=="WaveBrowse") {
			string wf=gdi->BrowseForFolder(gdi->GetText("WaveFolder"));

			if(wf.length()>0)
				gdi->SetText("WaveFolder", wf);
		}
		else if (bi.id=="TryPrewarning") {
			gEvent->TryPrewarningSounds(sWaveFolder, rand()%400+1);
		}
		else if (bi.id=="DoPrewarning") {

			gEvent->SyncronizeList(oLPunchId);
			gEvent->ClearPrewarningSounds();
			strcpy_s(sWaveFolder, gdi->GetText("WaveFolder").c_str());
			
			gdi->DisableInput("WaveFolder");
			gdi->DisableInput("DoPrewarning");
			gdi->DropLine();
			gdi->AddString("", 2, "Förvarningsröst aktiverad");
			gdi->FillRight();
			gdi->PushX();
			gdi->AddButton("TryPrewarning", "Testa rösten", ListsCB);
			gdi->FillDown();
			gdi->AddButton("Cancel", "Avbryt", ListsCB);
			gdi->PopX();

			gdi->RegisterEvent("DataUpdate", PreWarningCB);
			gdi->SetData("DataSync", 1);
			gdi->SetData("PunchSync", 1);	

		}
		else if (bi.id=="Prewarning") {
			SelectedList=bi.id;
			gdi->ClearPage();

			gdi->AddString("", 2, "Förvarningsröst");

			gdi->AddString("", 10, "De stämplar som inkommer till systemet matchas "
				"mot lagnummer och filen <N.wav>, "
				"där N är lagnummret, spelas upp. Filerna hämtas från katalogen nedan. "
				"Om laget tillhör nationen NAT, spelas i första hand filen </NAT/N.wav>. "
				"För svenska lag spelas till exempel i första hand filerna </SWE/N.wav>");

			gdi->PushX();
			gdi->FillRight();
			gdi->AddInput("WaveFolder", sWaveFolder,
							48, 0, "Ljudfiler, baskatalog.");

			gdi->FillDown();
			gdi->DropLine();
			gdi->AddButton("WaveBrowse", "Bläddra...", ListsCB);			
			gdi->PopX();

			//gdi->AddInput("ControlNumbers", "100", 16, 0, "Kontrollnummer att bevaka (tomt=allt)");

			gdi->AddButton("DoPrewarning", "Starta", ListsCB);
			//gEvent->GeneratePreReport(*gdi);

			//gdi->AddButton("Återgå", gdi->GetWidth()+20, 15, "Cancel", ListsCB, "", "", true);
			//gdi->AddButton("Skriv ut...", gdi->GetWidth()+20, 45, "Print", ListsCB, "", "Skriv ut listan.", true);
		}
		else if (bi.id=="PreReport") {
			SelectedList=bi.id;
			gdi->ClearPage();

			gEvent->GeneratePreReport(*gdi);

			gdi->AddButton("Återgå", gdi->GetWidth()+20, 15, "Cancel", ListsCB, "", "", true);
			gdi->AddButton("Skriv ut...", gdi->GetWidth()+20, 45, "Print", ListsCB, "", "Skriv ut listan.", true);
		}
		else if (bi.id=="InForestList") {
			SelectedList=bi.id;
			gdi->ClearPage();

			gdi->RegisterEvent("DataUpdate", ListsEventCB);
			gdi->SetData("DataSync", 1);
			gdi->RegisterEvent(bi.id, ListsCB);

			gEvent->GenerateInForestList(*gdi, OpenRunnerCB, NoStartRunnerCB);
			gdi->AddButton("Återgå", gdi->GetWidth()+20, 15, "Cancel", ListsCB, "", "", true);
			gdi->AddButton("Skriv ut...", gdi->GetWidth()+20, 45, "Print", ListsCB, "", "Skriv ut listan.", true);
		}
		else if(bi.id=="TeamStartList")
		{
			SelectedList=bi.id;
			gdi->ClearPage();

			gdi->RegisterEvent("DataUpdate", ListsEventCB);
			gdi->SetData("DataSync", 1);
			gdi->RegisterEvent(bi.id, ListsCB);

			gEvent->GenerateTeamStartlist(*gdi, OpenRunnerCB, 0);
			gdi->AddButton("Återgå", gdi->GetWidth()+20, 15, "Cancel", ListsCB, "", "", true);
			gdi->AddButton("Skriv ut...", gdi->GetWidth()+20, 45, "Print", ListsCB, "", "Skriv ut listan.", true);
			
		}
		else if(bi.id=="TeamResults")
		{
			SelectedList=bi.id;
			gdi->ClearPage();

			gdi->RegisterEvent("DataUpdate", ListsEventCB);
			gdi->SetData("DataSync", 1);
			gdi->RegisterEvent(bi.id, ListsCB);

			gEvent->GenerateTeamResultlist(*gdi, OpenRunnerCB, 2);
			gdi->AddButton("Återgå", gdi->GetWidth()+20, 15, "Cancel", ListsCB, "", "", true);
			gdi->AddButton("Skriv ut...", gdi->GetWidth()+20, 45, "Print", ListsCB, "", "Skriv ut listan.", true);
		}
		else if(bi.id=="StartList")
		{
			SelectedList=bi.id;
			gdi->ClearPage();
			
			gdi->RegisterEvent("DataUpdate", ListsEventCB);
			gdi->SetData("DataSync", 1);
			gdi->RegisterEvent(bi.id, ListsCB);

			gEvent->GenerateStartlist(*gdi, OpenRunnerCB);
			gdi->AddButton("Återgå", gdi->GetWidth()+20, 15, "Cancel", ListsCB, "", "Välj en annan lista.", true);
			gdi->AddButton("Skriv ut...", gdi->GetWidth()+20, 45, "Print", ListsCB, "", "Skriv ut listan.", true);
			
		}
		else if(bi.id=="MinuteStartList"){
			SelectedList=bi.id;
			gdi->ClearPage();

			gdi->RegisterEvent("DataUpdate", ListsEventCB);
			gdi->SetData("DataSync", 1);
			gdi->RegisterEvent(bi.id, ListsCB);
		
			gEvent->GenerateMinuteStartlist(*gdi);
			gdi->AddButton("Återgå", gdi->GetWidth()+20, 15, "Cancel", ListsCB, "", "", true);
			gdi->AddButton("Webb...", gdi->GetWidth()+20, 45, "MinuteStartList_HTML", ListsCB, "","", true);			
			gdi->AddButton("Skriv ut...", gdi->GetWidth()+20, 75, "Print", ListsCB, "", "Skriv ut listan.", true);			
		}
		else if(bi.id=="ResultList")
		{
			SelectedList=bi.id;
			gdi->ClearPage();
		
			gdi->RegisterEvent("DataUpdate", ListsEventCB);
			gdi->SetData("DataSync", 1);
			gdi->RegisterEvent(bi.id, ListsCB);
			
			gEvent->GenerateResultlist(*gdi,  OpenRunnerCB);			
			gdi->AddButton("Återgå", gdi->GetWidth()+20, 15, "Cancel", ListsCB, "", "", true);
			gdi->AddButton("Skriv ut...", gdi->GetWidth()+20, 45, "Print", ListsCB, "", "Skriv ut listan.", true);
		}
		else if(bi.id=="Autoresult") {
			SelectedList=bi.id;
			gdi->ClearPage();
		
			//gdi->RegisterEvent("DataUpdate", ListsEventCB);
			//gdi->SetData("DataSync", 1);
			//gdi->RegisterEvent(bi.id, ListsCB);
			
			gEvent->GenerateResultlistPrint(*gdi);			
			gdi->AddButton("Återgå", gdi->GetWidth()+20, 15, "Cancel", ListsCB, "", "", true);
			gdi->AddButton("Skriv ut...", gdi->GetWidth()+20, 45, "Print", ListsCB, "", "Skriv ut listan.", true);
		}
		else if(bi.id=="ResultListFT")
		{
			SelectedList=bi.id;
			gdi->ClearPage();

			gdi->RegisterEvent("DataUpdate", ListsEventCB);
			gdi->SetData("DataSync", 1);
			gdi->RegisterEvent(bi.id, ListsCB);

			gEvent->GenerateResultlistFinishTime(*gdi, false,  OpenRunnerCB);

			gdi->AddButton("Återgå", gdi->GetWidth()+20, 15, "Cancel", ListsCB, "", "", true);
			gdi->AddButton("Webb...", gdi->GetWidth()+20, 45, "ResultListFT_HTML", ListsCB, "","", true);
			gdi->AddButton("Skriv ut...", gdi->GetWidth()+20, 75, "Print", ListsCB, "", "Skriv ut listan.", true);
		}
		else if(bi.id=="ResultListCFT")
		{
			SelectedList=bi.id;
			gdi->ClearPage();

			gdi->RegisterEvent("DataUpdate", ListsEventCB);
			gdi->SetData("DataSync", 1);
			gdi->RegisterEvent(bi.id, ListsCB);

			gEvent->GenerateResultlistFinishTime(*gdi, true,  OpenRunnerCB);
			gdi->AddButton("Återgå", gdi->GetWidth()+20, 15, "Cancel", ListsCB, "", "", true);
			gdi->AddButton("Webb...", gdi->GetWidth()+20, 45, "ResultListCFT_HTML", ListsCB, "","", true);			
			gdi->AddButton("Dataexport...", gdi->GetWidth()+20, 75, "ResultListFT_ONATT", ListsCB, 
				"","Exportformat för OK Linnés onsdagsnatt", true);			
			gdi->AddButton("Skriv ut...", gdi->GetWidth()+105, 45, "Print", ListsCB, "", "Skriv ut listan.", true);
		}
		else if(bi.id=="Speaker"){
			SelectedList=bi.id;
			gdi->ClearPage();
			gdi->FillRight();
			gdi->AddSelection("SpeakerClass", 200, 400, SpeakerListCB, "Välj klass:");
			gEvent->FillClasses(*gdi, "SpeakerClass");

			//gdi->AddSelection("SpeakerControl", 200, 400, SpeakerListCB, "Välj klass:");

			gdi->DropLine();

			gdi->AddButton("SelectClass", "Välj", SpeakerListCB);
			gdi->AddButton("Cancel", "Avbryt" , ListsCB);
/*			gdi->RegisterEvent("DataUpdate", ListsEventCB);
			gdi->SetData("DataSync", 1);
			gdi->RegisterEvent(bi.id, ListsCB);
*/
			//gEvent->SpeakerList(*gdi, 2);
//			gdi->AddButton("Avbryt", gdi->GetWidth()+20, 15, "Cancel", ListsCB, "", "", true);
			//gdi->AddButton("Webb...", gdi->GetWidth()+20, 45, "ResultListCFT_HTML", ListsCB, "","", true);			
			//gdi->AddButton("Dataexport...", gdi->GetWidth()+20, 75, "ResultListFT_ONATT", ListsCB, 
			//	"","Exportformat för OK Linnés onsdagsnatt", true);			
			//gdi->AddButton("Skriv ut...", gdi->GetWidth()+105, 45, "Print", ListsCB, "", "Skriv ut listan.", true);
			
		}
		else if(bi.id=="MinuteStartList_HTML"){
			string file=gdi->BrowseForSave("Webbdokument¤*.html;*.htm", "html", index);

			if(file.length()>0){
				gEvent->GenerateMinuteStartlist(file.c_str());
				gdi->OpenDoc(file.c_str());
			}
		}
		else if(bi.id=="ResultHTML")
		{
			string file=gdi->BrowseForSave("Webbdokument¤*.html;*.htm", "html", index);

			if(file.length()>0){
				gEvent->GenerateResultlist(file.c_str());
				gdi->OpenDoc(file.c_str());
			}
		}
		else if(bi.id=="StartHTML"){
			string file=gdi->BrowseForSave("Webbdokument¤*.html;*.htm", "html", index);

			if(file.length()>0){
				gEvent->GenerateStartlist(file.c_str());
				gdi->OpenDoc(file.c_str());
			}
		}
		else if(bi.id=="ResultListFT_HTML"){
			string file=gdi->BrowseForSave("Webbdokument¤*.html;*.htm", "html", index);

			if(file.length()>0){
				gEvent->GenerateResultlistFinishTime(file.c_str(), false);
				gdi->OpenDoc(file.c_str());
			}
		}
		else if(bi.id=="ResultListFT_ONATT"){
			string file=gdi->BrowseForSave("Webbdokument¤*.txt;*.csv", "txt", index);

			if(file.length()>0){
				gEvent->GenerateStartlist(file.c_str());
				gdi->OpenDoc(file.c_str());
			}
		}
		else if(bi.id=="ResultWinSplits"){
			string file=gdi->BrowseForSave("IOF Sträcktider XML¤*.xml", "xml", index);

			if(file.length()>0){
				//gEvent->GenerateStartlist(file.c_str());
				gEvent->ExportIOFSplits(file.c_str());
				//gdi->OpenDoc(file.c_str());
			}
			/*
			char *file="c:\\temp\\splits.xml";
			gEvent->ExportIOFSplits(file);
			ShellExecute(gdi->GetHWND(), "open", 
				"C:\\Program Files\\WinSplits Pro\\WinSplitsPro.exe", 
				file, "c:\\temp", SW_SHOWNORMAL); 			
				*/
		}
	}
	return 0;
}


void LoadListsPage(gdioutput &gdi)
{
	gdi.SelectTab(TabPageLists);

	gdi.ClearPage();

	gdi.FillRight();
	gdi.PushX();
	gdi.AddButton("StartList", "Startlista", ListsCB);
	gdi.AddButton("MinuteStartList", "Minutlista", ListsCB);

	gdi.AddButton("ResultList", "Resultat", ListsCB);

	//gdi.AddButton("StartHTML", "Startlista i webbläsare", ListsCB);
	//gdi.AddButton("ResultHTML", "Resultat i webbläsare", ListsCB);

	gdi.FillDown();	
	gdi.AddButton("ResultWinSplits", "Resultat i WinSplits", ListsCB);
	
	gdi.PopX();
	gdi.FillRight();
	gdi.PushX();

	gdi.AddButton("ResultListFT", "Resultat, målgång gemensam", ListsCB, "Resultatlista sorterad efter måltid, alla klasser gemensamt.");
	gdi.AddButton("ResultListCFT", "Resultat, målgång klassvis", ListsCB, "Resultatlista sorterad efter måltid (först i mål), klassvis.");

	gdi.FillDown();
	gdi.AddButton("InForestList", "Kvar i skogen", ListsCB, "Lista över löpare i skogen och ej startande löpare.");

	gdi.PopX();
	gdi.FillRight();
	gdi.PushX();


	gdi.AddButton("Speaker", "Speaker", ListsCB, "Lista över löpare i skogen och ej startande löpare.");
	gdi.AddButton("TeamStartList", "Stafett: Startlista", ListsCB);
	gdi.AddButton("TeamResults", "Stafett: Resultatlista", ListsCB);

	gdi.FillDown();
	gdi.AddButton("Prewarning", "Förvarningsröst...", ListsCB);

	gdi.PopX();
	gdi.FillRight();
	gdi.PushX();
	gdi.AddButton("Autoresult", "Automatisk resultatutskrift...", ListsCB);


	gdi.PopX();
	gdi.DropLine();
	gdi.DropLine();
	gdi.DropLine();
	gdi.DropLine();
	gdi.AddButton("PreReport", "Kör kontroll inför tävlingen...", ListsCB);

	gdi.RegisterEvent("DataUpdate", ListsEventCB);	
	gdi.Refresh();

	

	if(SelectedList!="")
	{
		gdi.SendCtrlMessage(SelectedList);
	}
	gdi.Refresh();
}