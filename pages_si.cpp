#include "stdafx.h"
#include "resource.h"


#include "oEvent.h"
#include "xmlparser.h"

#include "gdioutput.h"
#include "commctrl.h"

#include "oEvent.h"

#include "SportIdent.h"

extern SportIdent *gSI;

extern pEvent gEvent;


void LoadRunnerPage(gdioutput &gdi);

static SICard CSIC;
static list<SICard> CardQueue;

void InsertSICard(gdioutput &gdi, SICard &sic);
void LoadSIPage(gdioutput &gdi, HWND);
bool ProcessCard(gdioutput &gdi, pRunner runner, bool silent=false);
void RefillComPorts(gdioutput &gdi);

int SportIdentCB(gdioutput *gdi, int type, void *data)
{
	if(type==GUI_BUTTON)
	{
		ButtonInfo bi=*(ButtonInfo *)data;

		if(bi.id=="SIPassive"){
			
			string port=gdi->GetText("ComPortName");

			if(gSI->OpenComListen(port.c_str(), gdi->GetTextNo("BaudRate"))){
				gSI->StartMonitorThread(port.c_str());
				LoadSIPage(*gdi, 0);
				gdi->AddString("", 0, string("Lyssnar på ")+port);
				
			}
			else
				gdi->AddString("", 0, "FEL: Porten kunde inte öppnas");

		}
		if(bi.id=="StartSI")
		{
			char bf[64];
			ListBoxInfo lbi;
			if(gdi->GetSelectedItem("ComPort", &lbi)) {
				sprintf_s(bf, 64, "COM%d", lbi.data);
				string port=bf;
				//string port=gdi->GetText("ComPort").c_str();
	
				if (gSI->IsPortOpen(port)) {
					gSI->CloseCom(bf);
					gdi->AddString("",0, "Kopplar ifrån SportIdent på " + port +"... OK");

					RefillComPorts(*gdi);
				}
				else {
					//gdi->RemoveString("SIInfo");
					gdi->AddString("", 0, "Startar SI på "+ port + "...");
					gdi->Refresh();
					if(gSI->OpenCom(port.c_str())){
						gSI->StartMonitorThread(port.c_str());
						//gdi->AddString("", 0 , "SI på "+ port + ": OK");
						gdi->AddString("", 0, "SI på "+ port + ": OK");
						gdi->AddString("", 0, gSI->GetInfoString(port.c_str()));
					}
					else{
						//Retry...
						Sleep(300);
						if(gSI->OpenCom(port.c_str()))
						{
							gSI->StartMonitorThread(port.c_str());
							//gdi->AddString("", 0 , "SI på "+ port + ": OK");
							gdi->AddString("", 0, "SI på "+ port + ": OK");
							gdi->AddString("", 0, gSI->GetInfoString(port.c_str()));
						}
						else {
							gdi->SetRestorePoint();
							gdi->AddString("", 0, "SI på "+ port + ": FEL, inget svar.");
							
							if(gdi->Ask("Fick inget svar. Ska porten öppnas i passivt läge; "
								"ska MeOS lyssna efter inkommande stämplingar?")) {
 
								gdi->PushX();
								gdi->FillRight();
								gdi->AddInput("ComPortName", port, 10, 0, "COM-Port:");
								gdi->AddInput("BaudRate", "4800", 10, 0, "Överföringshastighet/Baudrate: använd 4800 eller 38400.");

								gdi->DropLine();
								gdi->FillDown();
								gdi->AddButton("SIPassive", "Lyssna...", SportIdentCB);
								gdi->PopX();
							}

						}
					}

					RefillComPorts(*gdi);
				}
				gdi->Refresh();
			}
		}
		else if(bi.id=="SIInfo")
		{
			char bf[64];
			ListBoxInfo lbi;
			if(gdi->GetSelectedItem("ComPort", &lbi))
			{
				sprintf_s(bf, 64, "COM%d", lbi.data);
				gdi->AddString("", 0, "Hämtar information om "+string(bf)+".");
				gdi->AddString("", 0, gSI->GetInfoString(bf));
			}
			//gSI->StopMonitorThread();
		}
		else if(bi.id=="AutoDetect")
		{
			gdi->AddString("", 0, "Söker efter SI-enheter... ");
			gdi->Refresh();
			list<int> ports;
			if (!gSI->AutoDetect(ports)) {
				gdi->AddString("SIInfo", 0, "Hittade ingen SI-enhet. Är de inkopplade och startade?");
				gdi->Refresh();
				return 0;
			}
			char bf[128];
			gSI->CloseCom(0);

			while(!ports.empty())
			{
				int p=ports.front();
				sprintf_s(bf, 128, "COM%d", p);

				gdi->AddString(string("SIInfo")+bf, 0, "Startar SI på "+ string(bf) + "...");
				gdi->Refresh();
				if (gSI->OpenCom(bf)) {
					gSI->StartMonitorThread(bf);
					//gdi->AddString("", 0 , "SI på "+ port + ": OK");
					gdi->AddString("",0, "SI på "+ string(bf) + ": OK");
					gdi->AddString("", 0, gSI->GetInfoString(bf));
				}
				else if (gSI->OpenCom(bf)) {
					gSI->StartMonitorThread(bf);
					//gdi->AddString("", 0 , "SI på "+ port + ": OK");
					//gdi->SetText(string("SIInfo")+bf, "SI på "+ string(bf) + ": OK");
					gdi->AddString("", 0, "SI på "+ string(bf) + ": OK");
					gdi->AddString("", 0, gSI->GetInfoString(bf));
				}
				else gdi->AddString("", 0, "SI på "+ string(bf) + ": FEL, inget svar.");
				
				gdi->Refresh();

//				gdi->AddString("", 1, bf);
				ports.pop_front();
			}

		}
		else if(bi.id=="Save")
		{
			SICard sic;

			memset(&sic, 0, sizeof(sic));

			sic.CardNumber=gdi->GetTextNo("SI");
			int f=sic.FinishPunch.Time=gEvent->GetRelativeTime(gdi->GetText("Finish"));
			int s=sic.StartPunch.Time=gEvent->GetRelativeTime(gdi->GetText("Start"));
			sic.Punch[0].Code=gdi->GetTextNo("C1");
			sic.Punch[0].Time=(int)(f*0.3+s*0.7);
			sic.Punch[1].Code=gdi->GetTextNo("C2");
			sic.Punch[1].Time=(int)(f*0.5+s*0.5);
			sic.Punch[2].Code=gdi->GetTextNo("C3");
			sic.Punch[2].Time=(int)(f*0.6+s*0.4);
			sic.Punch[3].Code=gdi->GetTextNo("C4");
			sic.Punch[3].Time=(int)(f*0.8+s*0.2);
			sic.nPunch=4;

			gSI->AddCard(sic);
		}
		else if(bi.id=="Cancel")
		{
			memset(&CSIC, 0, sizeof(CSIC));

			LoadSIPage(*gdi, 0);

			if(!CardQueue.empty())
			{
				SICard c=CardQueue.front();
				CardQueue.pop_front();
				gdi->RemoveFirstInfoBox("SIREAD");
				InsertSICard(*gdi, c);
			}
			return 0;
		}
		else if(bi.id=="OK1")
		{
			string name=gdi->GetText("Runners");
			string club=gdi->GetText("Club");

			if(name.length()==0){
				gdi->Alert("Namnet får inte vara blankt");
				return 0;
			}

		//	if(club.length()==0)
		//		club="Klubblös";

			gdi->DisableInput("OK1");

			pRunner r=0;
			DWORD rid;

			if(gdi->GetData("RunnerId", rid) && rid>0)
			{
				r=gEvent->GetRunner(rid);

				if(r && r->GetName()==name)
				{
					gdi->Restore();
					//We have a match!
					ProcessCard(*gdi, r);
					return 0;
				}
			}
			 
		
			r=gEvent->GetRunnerByName(name);

			if(r && r->GetClub()==club)
			{
				//We have a match!
				gdi->SetData("RunnerId", r->GetId());
				
				gdi->Restore();				
				ProcessCard(*gdi, r);
				return 0;
			}
		
		
			//We have a new runner in our system
			gdi->FillRight();
			gdi->PushX();
	
			SICard si_copy=CSIC;
			gEvent->ConvertTimes(si_copy);
			gdi->AddInput("StartTime", gEvent->GetAbsTime(si_copy.StartPunch.Time), 8, 0, "Starttid:");

			
			gdi->AddSelection("Classes", 200, 300, 0, "Klass:");
			gEvent->FillClasses(*gdi, "Classes");

			
			//Find matching class...
			int dist;	
			pClass rclass=gEvent->FindBestClass(CSIC, dist);
			if(rclass)	gdi->SelectedItemByData("Classes", rclass->GetId());

			gdi->DropLine();

			gdi->AddButton("OK2", "OK", SportIdentCB);
			gdi->FillDown();

			gdi->AddButton("NewClass", "Skapa ny klass", SportIdentCB);

			//gdi.AddButton("Cancel", "Avbryt");
			gdi->PopX();
			gdi->Refresh();
			return 0;
		}
		else if(bi.id=="OK2")
		{
			//New runner in existing class...

			ListBoxInfo lbi;
			gdi->GetSelectedItem("Classes", &lbi);
			
			if(lbi.data==0 || lbi.data==-1)
			{
				gdi->Alert("Du måste välja en klass");
				return 0;
			}

			string club=gdi->GetText("Club");

			if(club.length()==0)
				club="Klubblös";

			pRunner r=gEvent->AddRunner(gdi->GetText("Runners"), club, lbi.data);

			r->SetStartTime(gdi->GetText("StartTime"));
			r->SetCardNo(CSIC.CardNumber);
			
			gdi->Restore();				
			ProcessCard(*gdi, r);
		}
		else if(bi.id=="NewClass")
		{
			gdi->DisableInput("OK2");
			gdi->DisableInput("NewClass");
			gdi->FillRight();
			gdi->PushX();
	
			gdi->AddInput("ClassName", gEvent->GetAutoClassName(), 10,0, "Klassnamn:");

			gdi->DropLine();
			
			gdi->FillDown();
			gdi->AddButton("OK3", "OK", SportIdentCB);

			gdi->Refresh();
			gdi->PopX();
		}
		else if(bi.id=="OK3")
		{
			pCourse pc=gEvent->AddCourse(gdi->GetText("ClassName"));

			for(unsigned i=0;i<CSIC.nPunch; i++)
				pc->AddControl(CSIC.Punch[i].Code);
			
			pClass pclass=gEvent->AddClass(gdi->GetText("ClassName"), pc->GetId());

			pRunner r=gEvent->AddRunner(gdi->GetText("Runners"), gdi->GetText("Club"), pclass->GetId());

			r->SetStartTime(gdi->GetText("StartTime"));
			r->SetCardNo(CSIC.CardNumber);
			gdi->Restore();				
			ProcessCard(*gdi, r);
		}
		else if(bi.id=="OK4")
		{
			//Existing runner in existing class...

			ListBoxInfo lbi;
			gdi->GetSelectedItem("Classes", &lbi);
			
			if(lbi.data==0 || lbi.data==-1)
			{
				gdi->Alert("Du måste välja en klass");
				return 0;
			}

			
			DWORD rid;
			pRunner r;

			if(gdi->GetData("RunnerId", rid) && rid>0)
				r=gEvent->GetRunner(rid);
			else r=gEvent->AddRunner("Oparad bricka", "Okänd");

			r->SetClassId(lbi.data);

			gdi->Restore();				
			ProcessCard(*gdi, r);
		}

	}
	else if(type==GUI_LISTBOX){
		ListBoxInfo bi=*(ListBoxInfo *)data;

		if(bi.id=="Runners"){
			gdi->SetData("RunnerId", bi.data);
			
			pRunner r=gEvent->GetRunner(bi.data);

			gdi->SetText("Club", r->GetClub());
		}
		else if(bi.id=="ComPort"){
			char bf[64];
			sprintf_s(bf, 64, "COM%d", bi.data);
			
			if(gSI->IsPortOpen(bf))			
				gdi->SetText("StartSI", "Koppla ifrån");
			else
				gdi->SetText("StartSI", "Aktivera");
		}

	}
	else if(type==GUI_INFOBOX){
		DWORD loaded;
		if(!gdi->GetData("SIPageLoaded", loaded))
			LoadSIPage(*gdi, 0);
	}

	return 0;
}


void RefillComPorts(gdioutput &gdi)
{
	if(!gSI) return;

	list<int> ports;
	gSI->EnumrateSerialPorts(ports);

	gdi.ClearList("ComPort");
	ports.sort();
	char bf[256];
	int active=0;
	int inactive=0;
	while(!ports.empty())
	{
		int p=ports.front();
		sprintf_s(bf, 256, "COM%d", p);

		if(gSI->IsPortOpen(bf)){
			gdi.AddItem("ComPort", string(bf)+" [OK]", p);
			active=p;
		}
		else{
			gdi.AddItem("ComPort", bf, p);
			inactive=p;
		}

		ports.pop_front();
	}

	if(active){
		gdi.SelectedItemByData("ComPort", active);
		//gdi.AddButton("StartSI", "Koppla ifrån", SportIdentCB);
		gdi.SetText("StartSI", "Koppla ifrån");
	}
	else{
		gdi.SelectedItemByData("ComPort", inactive);
		gdi.SetText("StartSI", "Aktivera");
		//gdi.AddButton("StartSI", "Aktivera", SportIdentCB);
	}
}


void LoadSIPage(gdioutput &gdi, HWND hWnd)
{
	gdi.ClearPage();

	gdi.SetData("SIPageLoaded", 1);

	if(!gSI) {
		gSI=new SportIdent(hWnd, 0);
		gSI->SetZeroTime(gEvent->GetZeroTimeNum());
	}

/*	gdi.FillRight();
	gdi.PushX();
	gdi.AddInput("SI", "", 10, 0, "SI");
	gdi.AddInput("Start", "23:40", 10, 0, "Start");
	gdi.AddInput("Finish", "23:50", 10, 0, "Finish");
	gdi.AddInput("C1", "33", 5, 0, "C1");
	gdi.AddInput("C2", "34", 5, 0, "C2");
	gdi.AddInput("C3", "45", 5, 0, "C3");
	gdi.FillDown();
	gdi.AddInput("C4", "100", 5, 0, "C4");
	gdi.PopX();

	//gdi.AddInput("SI");

	gdi.AddButton("Save", "Spara", SportIdentCB);

*/
	gdi.AddString("", 1, "SportIdent");
	gdi.DropLine();

	/*if(gSI->IsOpen())
	{
		gdi.AddButton("StopSI", "Koppla ifrån SportIdent", SportIdentCB);	
		gdi.DropLine();
	}
	else*/
	{
		gdi.PushX();
		gdi.FillRight();
		gdi.AddSelection("ComPort", 120, 200, SportIdentCB);
		gdi.AddButton("StartSI", "Aktivera+++", SportIdentCB);

		gdi.AddButton("SIInfo", "Info", SportIdentCB);

		RefillComPorts(gdi);
		//gdi.DropLine();
/*		list<int> ports;
		gSI->EnumrateSerialPorts(ports);

		ports.sort();
		char bf[256];
		int active=0;
		int inactive=0;
		while(!ports.empty())
		{
			int p=ports.front();
			sprintf(bf, "COM%d", p);

			if(gSI->IsPortOpen(bf)){
				gdi.AddItem("ComPort", string(bf)+" [OK]", p);
				active=p;
			}
			else{
				gdi.AddItem("ComPort", bf, p);
				inactive=p;
			}

			ports.pop_front();
		}

		if(active){
			gdi.SelectedItemByData("ComPort", active);
			gdi.AddButton("StartSI", "Koppla ifrån", SportIdentCB);
		}
		else{
			gdi.SelectedItemByData("ComPort", inactive);
			gdi.AddButton("StartSI", "Aktivera", SportIdentCB);
		}*/
		
		gdi.AddButton("AutoDetect", "Sök och starta automatiskt...", SportIdentCB);

		gdi.PopX();
		gdi.FillDown();
		//gdi.DropLine();
		gdi.DropLine();
		gdi.DropLine();

		gdi.AddString("", 10, "Aktivera SI-enheten genom att välja rätt COM-port, eller genom att söka efter installerade SI-enheter. Info ger dig information om den valda enheten/porten.");

		gdi.DropLine();
	}

	memset(&CSIC, 0, sizeof(CSIC));
	if(!CardQueue.empty())
	{
		SICard c=CardQueue.front();
		CardQueue.pop_front();
		gdi.RemoveFirstInfoBox("SIREAD");
		InsertSICard(gdi, c);
	}


	gdi.Refresh();
}



void InsertSICard(gdioutput &gdi, SICard &sic)
{
	if(sic.PunchOnly){
		//gEvent->AddPunch(

		DWORD loaded;

		gEvent->ConvertTimes(sic);
		oFreePunch *ofp=0;

		if(sic.nPunch==1)
			ofp=gEvent->AddFreePunch(sic.Punch[0].Time, sic.Punch[0].Code, sic.CardNumber);
		else
			ofp=gEvent->AddFreePunch(sic.FinishPunch.Time, oPunch::PunchFinish, sic.CardNumber);

		ofp->Syncronize();

		if(gdi.GetData("SIPageLoaded", loaded)){
			gEvent->SyncronizeList(oLRunnerId);
			pRunner r=gEvent->GetRunnerByCard(sic.CardNumber);

			if (r) {
				string str=r->GetName() + " stämplade vid " + ofp->GetSimpleString();

				gdi.AddString("", 0, str);
				gdi.DropLine();
			}
			else {
				char nr[16];
				sprintf_s(nr, "%d", sic.CardNumber);
				string str="SI " + string(nr) + "(okänd) stämplade vid " + ofp->GetSimpleString();

				gdi.AddString("", 0, str);
				gdi.DropLine();
			}	
			gdi.ScrollToBottom();
		}
	
		
		gdi.MakeEvent("DataUpdate", 0);
		return;
	}

	gEvent->SyncronizeList(oLCardId);
	gEvent->SyncronizeList(oLRunnerId);

	bool ReadBefore=gEvent->IsCardRead(sic);

	DWORD loaded;
	if(!gdi.GetData("SIPageLoaded", loaded)){
		//SIPage not loaded...
		pRunner r=gEvent->GetRunnerByCard(sic.CardNumber);


		if(!r){
			//Look up in database.
			pRunner db_r=gEvent->DBLookUp(sic.CardNumber);

			if(db_r)
			{
				/*r=gEvent->GetRunnerByName(db_r->GetName());

				if(r && (r->GetClub()==db_r->GetClubNameDB()) ){
					//OK
				}
				else r=0;*/
				int dist;	
				pClass rclass=gEvent->FindBestClass(CSIC, dist);

				if(rclass && dist==0){//Perfect match found. Assume it is it!
					r=gEvent->AddRunner(db_r->GetName(), db_r->GetClubNameDB());
					r->SetCardNo(db_r->GetCardNo());
					r->SetClassId(rclass->GetId());
				}
				else r=0; //Do not assume too much...
			}
		}


		if(r && r->GetClassId() && !ReadBefore)
		{
			//We can do a silent read-out...
			CSIC=sic;
			ProcessCard(gdi, r, true);
			return;
		}
		else
		{
			char bf[256];
			sprintf_s(bf, 256, "SI-%d inläst.\nÅtgärder från operatören krävs...", sic.CardNumber);
			CardQueue.push_back(sic);

			gdi.AddInfoBox("SIREAD", gEvent->GetCurrentTimeS()+": "+bf, 0, SportIdentCB);
			return;
		}
	}
	else if(CSIC.CardNumber)
	{
		pRunner r=gEvent->GetRunnerByCard(sic.CardNumber);
		string name;
		if(r) name=" ("+r->GetName()+")";

		char bf[256];
		sprintf_s(bf, 256, "SI-%d inläst%s.\nBrickan har ställts i kö.", sic.CardNumber, name.c_str());

		CardQueue.push_back(sic);

		//sprinft(id, "SI%d");
		gdi.AddInfoBox("SIREAD", gEvent->GetCurrentTimeS()+": "+bf);
		return;
	}

	

	//We stop processing of new cards, while working...
	CSIC=sic;

	if(ReadBefore){
		char bf[256];
		sprintf_s(bf, 256,  "SI-%d är redan inläst. Ska den läsas in igen?", sic.CardNumber);

		if(!gdi.Ask(bf))
		{
			memset(&CSIC, 0 , sizeof(CSIC));
			return;
		}
	}

	pRunner r=gEvent->GetRunnerByCard(CSIC.CardNumber, !ReadBefore);
	pRunner db_r=0;

	if(!r){
		//Look up in database.
		db_r=gEvent->DBLookUp(CSIC.CardNumber);

		if(db_r)
		{
			r=gEvent->GetRunnerByName(db_r->GetName());

			if(r && (r->GetClub()==db_r->GetClubNameDB()) ){
				//OK
			}
			else if(!r){

				int dist;	
				pClass rclass=gEvent->FindBestClass(CSIC, dist);

				if(rclass && dist==0){//Perfect match found. Assume it is it!
					r=gEvent->AddRunner(db_r->GetName(), db_r->GetClubNameDB());
					r->SetCardNo(db_r->GetCardNo());
					r->SetClassId(rclass->GetId());
				}
				else r=0; //Do not assume too much...
			}
			else r=0;
		}
	}

	if(!r){
		gdi.SetRestorePoint();

		char bf[256];
		sprintf_s(bf, 256, "SI-%d inläst. Brickan är inte knuten till någon löpare (i skogen).", sic.CardNumber);

		gdi.DropLine();
		gdi.AddString("", 1, bf);
		gdi.FillRight();
		gdi.PushX();

		gdi.AddCombo("Runners", 200, 300, SportIdentCB, "Namn:");
		gEvent->FillRunners(gdi, "Runners"); 

		if(db_r){
			gdi.SetText("Runners", db_r->GetName()); //Data from DB
		}
		else if(CSIC.FirstName[0] || CSIC.LastName[0]){ //Data from SI-card
			gdi.SetText("Runners", string(CSIC.FirstName)+" "+CSIC.LastName);
		}

		gdi.AddCombo("Club", 200, 300, 0, "Klubb:");
		gEvent->FillClubs(gdi, "Club");

		if(db_r){
			gdi.SetText("Club", db_r->GetClubNameDB()); //Data from DB
		}

		gdi.DropLine();
		gdi.AddButton("OK1", "OK", SportIdentCB);
		gdi.FillDown();
		gdi.AddButton("Cancel", "Avbryt", SportIdentCB);
		gdi.PopX();
		gdi.Refresh();
	}
	else if(r->GetClassId())		
		ProcessCard(gdi, r); //Everyting is OK
	else{
		//No class. Select...
		gdi.SetRestorePoint();

		char bf[256];
		sprintf_s(bf, 256, "SI-%d inläst. Brickan tillhör %s som saknar klass.", sic.CardNumber, r->GetName().c_str());

		gdi.DropLine();
		gdi.AddString("", 1, bf);

		gdi.FillRight();
		gdi.PushX();
		
		gdi.AddSelection("Classes", 200, 300, 0, "Klass:");
		gEvent->FillClasses(gdi, "Classes");

			
		//Find matching class...
		int dist;	
		pClass rclass=gEvent->FindBestClass(CSIC, dist);
		if(rclass)	gdi.SelectedItemByData("Classes", rclass->GetId());

		gdi.DropLine();

		gdi.AddButton("OK4", "OK", SportIdentCB);
		gdi.FillDown();

		gdi.PopX();

		gdi.SetData("RunnerId", r->GetId());
		gdi.Refresh();
	}
}

bool ProcessCard(gdioutput &gdi, pRunner runner, bool silent)
{
	if(!runner)
		return false;

	//Update from SQL-source
	runner->Syncronize();

	if(!runner->GetClassId())
		runner->SetClassId(gEvent->AddClass("Okänd klass")->GetId());
	

	if(!runner->GetCourse())
	{
		pClass pclass=gEvent->GetClass(runner->GetClassId());

		pCourse pcourse=gEvent->AddCourse(runner->GetClass());
		
		pclass->SetCourse(pcourse);
		
		
		for(unsigned i=0;i<CSIC.nPunch; i++)
			pcourse->AddControl(CSIC.Punch[i].Code);

		char msg[256];

		sprintf_s(msg, 256, "Skapade en bana för klassen %s med %d kontroller från brickdata (SI-%d)",
				pclass->GetName().c_str(), CSIC.nPunch, CSIC.CardNumber);

		if(silent)
			gdi.AddInfoBox("SIINFO", msg, 15000);
		else
			gdi.AddString("", 0, msg);

	}

	//silent=true;
	SICard &sic=CSIC;

	pCard card=gEvent->AllocateCard();

	card->SetReadId(CSIC);
	card->SetCardNo(sic.CardNumber);
	
	char bf[16];
	_itoa_s(sic.CardNumber, bf, 16, 10);
	string cardno=bf;


	string info=runner->GetName() + " (" + cardno + "),   " + runner->GetClub() + ",   " + runner->GetClass();
	string warnings;

	//Convert punch times to relative times.
	gEvent->ConvertTimes(sic);

	if(sic.CheckPunch.Code!=-1)
		card->AddPunch(oPunch::PunchCheck, sic.CheckPunch.Time);

	if(sic.StartPunch.Code!=-1)
		card->AddPunch(oPunch::PunchStart, sic.StartPunch.Time);

	for(unsigned i=0;i<sic.nPunch;i++)
		card->AddPunch(sic.Punch[i].Code, sic.Punch[i].Time);

	if(sic.FinishPunch.Code!=-1)
		card->AddPunch(oPunch::PunchFinish, sic.FinishPunch.Time);
	else
		warnings+="Målstämpling saknas.";
//		gdi.AddString("", 0, "Målstämpling saknas.");
	//Varna målstämpling saknas?

	list<int> MP;
	runner->AddPunches(card, MP);

	//Update to SQL-source
	card->Syncronize();
	runner->Syncronize();

	if(runner->GetStatus()==oRunner::StatusOK)
	{
		gEvent->CalculateResults();

		if(!silent)
		{
			gdi.DropLine();
		//	gdi.DropLine();
			gdi.AddString("", 1, info);
			if(warnings.length()>0) gdi.AddString("", 1, warnings);
			gdi.AddString("", 0, "Status OK,    Tid: " + runner->GetRunningTimeS() + " Plats: " + runner->GetPlaceS());
		}
		else
		{
			string msg=runner->GetName()  + " (" + cardno + ")\n"+
					runner->GetClub()+". "+runner->GetClass() + 
					"\nTid:  " + runner->GetRunningTimeS() + ", Plats  " + runner->GetPlaceS();

			gdi.AddInfoBox("SIINFO", msg, 10000);
		}
	}
	else
	{
		string msg="Status: "+ runner->GetStatusS();

		
		if(!MP.empty())
		{
			msg=msg+", (";
			list<int>::iterator it;
			char bf[32];

			for(it=MP.begin(); it!=MP.end(); ++it){
				_itoa_s(*it, bf, 32, 10);
				msg=msg+bf+" ";
			}
			msg+=" saknas.)";
		}

		if(!silent)
		{
			gdi.DropLine();
			gdi.AddString("", 1, info);
			if(warnings.length()>0) gdi.AddString("", 1, warnings);
			gdi.AddString("", 0, msg);
			gdi.ScrollToBottom();
			

		}
		else
		{
			string statusmsg=runner->GetName()  + " (" + cardno + ")\n"+
					runner->GetClub()+". "+runner->GetClass() + 
					"\n" + msg;

			gdi.AddInfoBox("SIINFO", statusmsg, 10000);

		}
	}

	gdi.MakeEvent("DataUpdate", 0);
	
	
	memset(&CSIC, 0, sizeof(CSIC));

	if(!CardQueue.empty())
	{
		SICard c=CardQueue.front();
		CardQueue.pop_front();
		gdi.RemoveFirstInfoBox("SIREAD");
		InsertSICard(gdi, c);
	}

	return true;
}
