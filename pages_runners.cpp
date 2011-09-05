// meos.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "resource.h"


#include "oEvent.h"
#include "xmlparser.h"

#include "gdioutput.h"
#include "commctrl.h"


extern pEvent gEvent;

//extern gdioutput gdi;
void LoadRunnerPage(gdioutput &gdi);
void LoadClassPage(gdioutput &gdi, int Id=0);
void LoadTeamPage(gdioutput &gdi);

void SelectRunner(gdioutput *gdi, pRunner r)
{
	r->Syncronize();
	r->Apply();

	gdi->SetData("RunnerID", r->GetId());

	gdi->SetText("Name", r->GetName());
	gdi->SetText("Club", r->GetClub());

	gEvent->FillClasses(*gdi, "RClass");
	gdi->AddItem("RClass", "Ny klass", 0);
	gdi->SelectedItemByData("RClass", r->GetClassId());

	gEvent->FillCourses(*gdi, "RCourse", true);
	gdi->AddItem("RCourse", "[Klassens bana]", 0);
	gdi->SelectedItemByData("RCourse", r->GetCourseId());


	gdi->SetText("CardNo", r->GetCardNo());
	gdi->SetText("Start", r->GetStartTimeS());
	gdi->SetText("Finish", r->GetFinishTimeS());

	gdi->SetText("Time", r->GetRunningTimeS());

	//gdi->SetText("Status", r->GetStatusS());
	gdi->SelectedItemByData("Status", r->GetStatus());

	pCard pc=r->GetCard();

	if(pc)
	{
		gdi->SetTabStops("Punches", 70);
		pc->FillPunches(*gdi, "Punches");
	}
	else gdi->ClearList("Punches");

	pCourse pcourse=r->GetCourse();

	if(pcourse)
	{
		gdi->SetTabStops("Course", 50);			
		pcourse->FillCourse(*gdi, "Course");
	}
	else
		gdi->ClearList("Course");
}

int RunnerCB(gdioutput *gdi, int type, void *data)
{
	if(type==GUI_BUTTON)
	{
		ButtonInfo bi=*(ButtonInfo *)data;

		if(bi.id=="Teams")
			 LoadTeamPage(*gdi);			 
		else if(bi.id=="DoMoveCard")
		{
			DWORD rid;
			if(!gdi->GetData("RunnerID", rid) || rid==0)
			{
				gdi->Alert("Ingen löpare vald.");
				return 0;
			}
			
			pRunner r=gEvent->GetRunner(rid);

			//pCard pc;
			if(r && r->GetCard())
			{
				ListBoxInfo lbi;
				if(gdi->GetSelectedItem("MoveCardTo", &lbi) )
				{
					pRunner r_target=gEvent->GetRunner(lbi.data);
					if(!gEvent->MoveCard(r->GetId(), lbi.data))
						gdi->Alert("Fel: Denna löpare har redan stämplar. De måste först tas bort.");
					else
					{
						LoadRunnerPage(*gdi);
						SelectRunner(gdi, r_target);
						gdi->SelectedItemByData("Runners", r_target->GetId());
					}
					return 0;
				}
				
			}
			else
			{
				gdi->Alert("Ingen bricka hör till löparen.");
				return 0;
			}
		}
		else if(bi.id=="MoveCard")
		{
			gdi->RemoveControl("MoveCard");
			DWORD x,y;
			
			gdi->GetData("MCX", x);
			gdi->GetData("MCY", y);
			gdi->AddSelection(x, y, "MoveCardTo", 200, 200, RunnerCB, "Flytta stämplar till annan löpare:");
			gEvent->FillRunners(*gdi, "MoveCardTo");
			gdi->AddButton("Flytta", x+220, y+gdi->GetLineHeight(), "DoMoveCard", RunnerCB);
			gdi->UpdatePos(x+300, y+50,0,0);
			gdi->Refresh();
		}
		else if(bi.id=="Save")
		{
			DWORD rid;
			if(!gdi->GetData("RunnerID", rid))
			{
	//			gdi->Alert("Ingen löpare vald.");
	//			return;
				rid=0;
				gdi->SetData("RunnerID", 0);
			}
			string name=gdi->GetText("Name");

			if(name.length()==0)
			{
				gdi->Alert("Alla deltagare måste ha ett namn.");
				return 0;
			}
			bool create=false;

			pRunner r;
			if(rid==0)
			{
				r=gEvent->AddRunner("");
				create=true;
			}
			else r=gEvent->GetRunner(rid);

			gdi->SetData("RunnerID", r->GetId());

			if(r)
			{
				
				r->SetName(name);				
				r->SetCardNo(gdi->GetTextNo("CardNo"));
				r->SetStartTime(gdi->GetText("Start"));
				r->SetFinishTime(gdi->GetText("Finish"));



				ListBoxInfo lbi;
				gdi->GetSelectedItem("Club", &lbi);
				
				if(lbi.text!=""){
					pClub pc=gEvent->GetClub(lbi.text);
					pc->Syncronize();										
				}

				r->SetClub(lbi.text);
				gEvent->FillClubs(*gdi, "Club");
				gdi->SetText("Club", lbi.text);


				gdi->GetSelectedItem("Status", &lbi);
			
				//gdi->GetData("Status", dw);

				r->SetStatus((oRunner::RunnerStatus)lbi.data);

				//ListBoxInfo lbi;
				gdi->GetSelectedItem("RClass", &lbi);

				if(lbi.data<=0)
				{
					pClass pc=gEvent->AddClass(gEvent->GetAutoClassName());
					r->SetClassId(pc->GetId());

					LoadClassPage(*gdi, pc->GetId());
					return 0;
				}
				else r->SetClassId(lbi.data);


				//ListBoxInfo lbi;
				gdi->GetSelectedItem("RCourse", &lbi);
				r->SetCourseId(lbi.data);

				list<int> mp;
				r->EvaluateCard(mp);
				//Status??
		
				r->Syncronize();

				gEvent->FillRunners(*gdi, "Runners");
				gdi->SelectedItemByData("Runners", r->GetId());

				gdi->SetText("Time", r->GetRunningTimeS());
				gdi->SelectedItemByData("Status", r->GetStatus());
			}

			if(create)
			{
				//Create one more?!
				gdi->SetData("RunnerID", 0);

				gdi->SetText("Name", "");
			//gdi->SetText("Club", "");
			//gdi->SelectedItemByData("RClass", 0);

				gdi->SetText("CardNo", "");
				gdi->SetText("Start", "-");
				gdi->SetText("Finish", "-");

				gdi->SetText("Time", "-");
				gdi->SelectedItemByData("Status", 0);
		
				gdi->ClearList("Course");
			
				gdi->SelectedItemByData("Runners", -1);
				gdi->SetInputFocus("Name");
			}

			return true;
		}
		else if(bi.id=="Add")
		{
			gdi->SetData("RunnerID", 0);

			gdi->SetText("Name", "");
			gdi->SetText("Club", "");
			gdi->SelectedItemByData("RClass", 0);
			gdi->SelectedItemByData("RCourse", 0);

			gdi->SetText("CardNo", "");
			gdi->SetText("Start", "-");
			gdi->SetText("Finish", "-");

			gdi->SetText("Time", "-");

			//gdi->SetText("Status", r->GetStatusS());
			gdi->SelectedItemByData("Status", 0);
		
			gdi->ClearList("Punches");
			gdi->ClearList("Course");
			
			gdi->SelectedItemByData("Runners", -1);
			gdi->SetInputFocus("Name");
		}
		else if(bi.id=="Remove")
		{
			DWORD rid;
			if(!gdi->GetData("RunnerID", rid))
				return 0;
		
			if(gdi->Ask("Vill du verkligen ta bort löparen?")) {

				if(gEvent->IsRunnerUsed(rid))
					gdi->Alert("Löparen ingår i ett lag och kan inte tas bort.");
				else{
					gEvent->RemoveRunner(rid);
					gEvent->FillRunners(*gdi, "Runners");
				}
			}
		}
		else if(bi.id=="AddC")
		{
			ListBoxInfo lbi;
			gdi->GetSelectedItem("Punches", &lbi);

			DWORD rid;
			if(!gdi->GetData("RunnerID", rid))
				return 0;

			pRunner r=gEvent->GetRunner(rid);

			if(!r) 
			{
				gdi->Alert("Löparen måste sparas innan stämplingar kan läggas till.");
				return 0;
			}

			int type=gdi->GetTextNo("PCode");

			if(type<=0 || type>1024)
			{
				gdi->Alert("Koden måste vara ett positivt heltal");
				return 0;
			}

			pCard card=r->GetCard();

			if(!card)
			{
				if(!gdi->Ask("Ingen bricka har lästs in för denna löpare. Vill du lägga till stämplar manuellt?"))
					return 0;

				//oCard ocard(&gEvent);			
				card=gEvent->AllocateCard();

				card->SetCardNo(r->GetCardNo());
				list<int> MP;
				r->AddPunches(card, MP);

				//card=r->GetCard();
			}

			
			
			int time=gEvent->GetRelativeTime(gdi->GetText("PTime"));

			card->InsertPunchAfter(lbi.data, type, time);

			//Update runner
			list<int> mp;
			r->EvaluateCard(mp);			
			gdi->SetText("Time", r->GetRunningTimeS());
			gdi->SelectedItemByData("Status", r->GetStatus());


			gdi->SetText("PTime", "");
			gdi->SetText("PCode", "");

			card->FillPunches(*gdi, "Punches");
		}
		else if(bi.id=="RemoveC")
		{
			ListBoxInfo lbi;
			gdi->GetSelectedItem("Punches", &lbi);

			DWORD rid;
			if(!gdi->GetData("RunnerID", rid))
				return 0;

			pRunner r=gEvent->GetRunner(rid);

			if(!r) return 0;

			pCard card=r->GetCard();

			if(!card) return 0;

			card->DeletePunch(lbi.data);
			card->FillPunches(*gdi, "Punches");


			//Update runner
			list<int> mp;
			r->EvaluateCard(mp);			
			gdi->SetText("Time", r->GetRunningTimeS());
			gdi->SelectedItemByData("Status", r->GetStatus());

		}
		else if(bi.id=="Check")
		{
		}
	}
	else if(type==GUI_LISTBOX)
	{
		ListBoxInfo bi=*(ListBoxInfo *)data;

		if(bi.id=="Runners")
		{			
			pRunner r=gEvent->GetRunner(bi.data);

			SelectRunner(gdi, r);

/*			gdi->SetData("RunnerID", bi.data);

			gdi->SetText("Name", r->GetName());
			gdi->SetText("Club", r->GetClub());
			gdi->SelectedItemByData("RClass", r->GetClassId());

			gdi->SetText("CardNo", r->GetCardNo());
			gdi->SetText("Start", r->GetStartTimeS());
			gdi->SetText("Finish", r->GetFinishTimeS());

			gdi->SetText("Time", r->GetRunningTimeS());

			//gdi->SetText("Status", r->GetStatusS());
			gdi->SelectedItemByData("Status", r->GetStatus());

			pCard pc=r->GetCard();

			if(pc)
			{
				gdi->SetTabStops("Punches", 70);
				pc->FillPunches(*gdi, "Punches");
			}
			else gdi->ClearList("Punches");

			pCourse pcourse=r->GetCourse();

			if(pcourse)
			{
				gdi->SetTabStops("Course", 50);			
				pcourse->FillCourse(*gdi, "Course");
			}
			else
				gdi->ClearList("Course");*/
		}
		else if (bi.id=="RClass") {
			gdi->SelectedItemByData("RCourse", 0);

			if (bi.data==0) {
				gdi->ClearList("Course");				
			}
			else {
				pClass Class=gEvent->GetClass(bi.data);

				if(Class && Class->GetCourse())
					Class->GetCourse()->FillCourse(*gdi, "Course");
			}
		}
		else if (bi.id=="RCourse") {
			if (bi.data==0) {
				gdi->ClearList("Course");

				ListBoxInfo lbi;
				gdi->GetSelectedItem("RClass", &lbi);
				pClass Class=gEvent->GetClass(lbi.data);
				if(Class && Class->GetCourse())
					Class->GetCourse()->FillCourse(*gdi, "Course");

			}
			else {
				pCourse course=gEvent->GetCourse(bi.data);

				if(course)
					course->FillCourse(*gdi, "Course");
			}
		}
	}
	else if(type==GUI_EVENT)
	{
		EventInfo ei=*(EventInfo *)data;

		if(ei.id=="LoadRunner")
		{
			pRunner r=gEvent->GetRunner(ei.data);
			if(r){
				SelectRunner(gdi, r);
				gdi->SelectedItemByData("Runners", r->GetId());
			}
		}

	}
	/*else if(type==GUI_CLEAR)
	{
		DWORD rid;

		if(gdi->GetData("RunnerID", rid) && rid!=0)
		{
			if( gdi->Ask("Spara ändringar?") )
			{
				if( gdi->SendCtrlMessage("Save") )
					return true;
				else return false;

			}
			else return true;
		}
		return true;
	}<*/

	return 0;
}

void DisablePunchCourse(gdioutput &gdi)
{
	gdi.DisableInput("SaveC");
	gdi.DisableInput("RemoveC");
	gdi.SetText("PTime", "");
	gdi.DisableInput("AddC");
	gdi.DisableInput("AddAllC");
}

void UpdateStatus(gdioutput &gdi, pRunner r)
{
	if(!r) return;

	gdi.SetText("Start", r->GetStartTimeS());
	gdi.SetText("Finish", r->GetFinishTimeS());
	gdi.SetText("Time", r->GetRunningTimeS());
	gdi.SelectedItemByData("Status", r->GetStatus());
}


int PunchesCB(gdioutput *gdi, int type, void *data)
{
	DWORD rid;
	if(!gdi->GetData("RunnerID", rid))
		return 0;

	pRunner r=gEvent->GetRunner(rid);

	if(!r){
		gdi->Alert("Löparen måste sparas innan stämplingar kan hanteras.");
		return 0;
	}

	
	if(type==GUI_LISTBOX){
		ListBoxInfo bi=*(ListBoxInfo *)data;

		if(bi.id=="Punches"){
			if(bi.data>=0 && bi.data<1000){
				pCard card=r->GetCard();

				if(!card) return 0;
				
				//oPunch *p=card->GetPunch(bi.data);
				string ptime=card->GetPunchTime(bi.data);

				if(ptime!=""){
					gdi->EnableInput("SaveC");
					gdi->SetText("PTime", ptime);
				}
				gdi->EnableInput("RemoveC");
			}
			else{
				gdi->DisableInput("SaveC");
				gdi->DisableInput("RemoveC");
				gdi->SetText("PTime", "");
			}
		}
		else if(bi.id=="Course"){
			if(bi.data>=0){
				pCourse pc=r->GetCourse();
				
				if(!pc) return 0;

				gdi->EnableInput("AddC");
				gdi->EnableInput("AddAllC");
			}
			else{
				gdi->DisableInput("AddC");
				gdi->DisableInput("AddAllC");
			}


		}
	}
	else if(type==GUI_BUTTON){
		ButtonInfo bi=*(ButtonInfo *)data;
		pCard card=r->GetCard();

		if(!card){
			if(!gdi->Ask("Ingen bricka har lästs in för denna löpare. Vill du lägga till stämplar manuellt?"))
				return 0;

			//oCard ocard(&gEvent);			
			card=gEvent->AllocateCard();

			card->SetCardNo(r->GetCardNo());
			list<int> mp;
			r->AddPunches(card, mp);
		}
	
		if(bi.id=="AddC"){
			list<int> mp;
			r->EvaluateCard(mp);
			list<int>::iterator it=mp.begin();
			
			pCourse pc=r->GetCourse();

			if(!pc) return 0;

			ListBoxInfo lbi;

			if(!gdi->GetSelectedItem("Course", &lbi))
				return 0;
			
			const oControl *oc=pc->GetControl(lbi.data);

			if(!oc) return 0;

			while(it!=mp.end()){
				if(oc->HasNumber(*it)){
					list<int> nmp;
					r->EvaluateCard(nmp, 0, *it); //Add this punch					
				}
				++it;
			}
			//Syncronize SQL
			card->Syncronize();

			card->FillPunches(*gdi, "Punches");
			UpdateStatus(*gdi, r);
		}
		else if(bi.id=="AddAllC"){
			list<int> mp;
			r->EvaluateCard(mp);
			list<int>::iterator it=mp.begin();
	

			while(it!=mp.end()){
				list<int> nmp;
				r->EvaluateCard(nmp, 0, *it); //Add this punch					
				++it;
			}

			//Syncronize SQL
			card->Syncronize();

			card->FillPunches(*gdi, "Punches");
			UpdateStatus(*gdi, r);
		}
		else if(bi.id=="SaveC"){						
			//int time=gEvent->GetRelTime();

			ListBoxInfo lbi;

			if(!gdi->GetSelectedItem("Punches", &lbi))
				return 0;
			
			pCard pc=r->GetCard();
			
			if(!pc) return 0;
			
			pc->SetPunchTime(lbi.data, gdi->GetText("PTime"));
			/*oPunch *op=pc->GetPunch(lbi.data);

			if(!op) return 0;

			op->SetTime(gdi->GetText("PTime"));*/
			//card->InsertPunchAfter(lbi.data, type, time);
			list<int> mp;
			r->EvaluateCard(mp);

			//Syncronize SQL
			card->Syncronize();
			r->Syncronize();

			card->FillPunches(*gdi, "Punches");
			UpdateStatus(*gdi, r);		
		}

	}
	

	return 0;
}

void LoadRunnerPage(gdioutput &gdi)
{
	gdi.SelectTab(TabPageRunner);
	gdi.ClearPage();
	int basex=gdi.GetCX();

	//gdi.SetOnClearCb(RunnerCB);
	gdi.FillDown();
	gdi.AddListBox("Runners", 200, 300, RunnerCB, "Deltagare:");
	gEvent->FillRunners(gdi, "Runners");

	gdi.PushX();
	gdi.FillRight();	
//	gdi.AddButton("Change", "Ändra", RunnerCB);
	gdi.AddButton("Remove", "Ta bort", RunnerCB);
	gdi.FillDown();
	gdi.AddButton("Add", "Ny löpare", RunnerCB);
	gdi.PopX();

	gdi.AddButton("Teams", "Hantera lag...", RunnerCB);
	gdi.NewColumn();
	gdi.FillDown();

	gdi.AddInput("Name", "", 16, 0, "Namn:");
	//gdi.AddInput("Club", "", 20, 0, "Klubb:");
	gdi.AddCombo("Club", 180, 300, 0, "Klubb:");
	gEvent->FillClubs(gdi, "Club");

	//gdi.AddInput("Class", "", 16, 0, "Klass:");
	gdi.AddSelection("RClass", 180, 300, RunnerCB, "Klass:");
	
	gEvent->FillClasses(gdi, "RClass");	
	gdi.AddItem("RClass", "Ny klass", 0);

/*	gdi.AddSelection("RCourse", 180, 300, 0, "Bana:");
	gEvent->FillCourses(gdi, "RCourse");
	gdi.AddItem("RCourse", "Klassens bana", 0);
*/
	gdi.AddSelection("RCourse", 180, 300, RunnerCB, "Bana:");
	gEvent->FillCourses(gdi, "RCourse", true);
	gdi.AddItem("RCourse", "[Klassens bana]", 0);
	

	gdi.AddInput("CardNo", "", 8, 0, "Bricka:");

	gdi.AddString("", 0, "");

	gdi.PushX();
	gdi.FillRight();

	gdi.AddInput("Start", "", 6, 0, "Starttid:");
	
	gdi.FillDown();
	gdi.AddInput("Finish", "", 6, 0, "Måltid:");
	
	gdi.PopX();
	
	gdi.PushX();
	gdi.FillRight();

	gdi.AddInput("Time", "", 6, 0, "Löptid:");
	gdi.DisableInput("Time");

	gdi.FillDown();
	//gdi.AddInput("Status", "", 6, 0, "Status:");
	gdi.AddSelection("Status", 90, 160, 0, "Status:");
	gdi.AddItem("Status", "-", 0);
	gdi.AddItem("Status", "OK", oRunner::StatusOK);
	gdi.AddItem("Status", "Ej start", oRunner::StatusDNS);
	gdi.AddItem("Status", "Felst.", oRunner::StatusMP);
	gdi.AddItem("Status", "Utg.", oRunner::StatusDNF);
	gdi.AddItem("Status", "Disk.", oRunner::StatusDQ);
	gdi.AddItem("Status", "Maxtid", oRunner::StatusMAX);
	gdi.PopX();
	gdi.SelectedItemByData("Status", 0);

	gdi.AddButton("Save", "Spara", RunnerCB);

	gdi.NewColumn();
	gdi.AddListBox("Punches", 140, 300, PunchesCB, "Stämplar:");

	gdi.AddButton("RemoveC", "Ta bort stämpel >>", RunnerCB);

	gdi.PushX();
	gdi.FillRight();
	gdi.AddInput("PTime", "", 5, 0, "Tid:");
	gdi.FillDown();
	gdi.DropLine();
	gdi.AddButton("SaveC", "Spara", PunchesCB);

	gdi.PopX();
	//gdi.PushX();
	//gdi.FillRight();
	gdi.PushX();
	gdi.PushY();

/*	gdi.PushX();
	gdi.FillRight();
	gdi.AddInput("PCode", "", 3, 0, "Kod:");

	gdi.FillDown();
	gdi.AddInput("PTime", "", 5, 0, "Tid:");
	
	gdi.PopX();
	
	gdi.PushX();
	gdi.FillRight();
	
	gdi.AddButton("AddC", "Lägg till", RunnerCB);
	
	gdi.FillDown();	
	gdi.AddButton("RemoveC", "Ta bort", RunnerCB);
	gdi.PopX();

	int x=gdi.GetCX();
	int y=gdi.GetCY();

	gdi.SetData("MCX", x);
	gdi.SetData("MCY", y);

	gdi.FillRight();
	gdi.AddButton("MoveCard", "Flytta stämplar", RunnerCB);
*/
	gdi.NewColumn();
	gdi.FillDown();
	gdi.AddListBox("Course", 140, 300, PunchesCB, "Banmall:");
	gdi.AddButton("AddC", "<< Lägg till stämpel", PunchesCB);
	gdi.AddButton("AddAllC", "<< Lägg till alla", PunchesCB);
	

	DisablePunchCourse(gdi);
	gdi.PopX();
	gdi.PopY();


	gdi.FillDown();
	//gdi.AddButton("PassRunner", "Godkänn!", RunnerCB);
	//gdi.PopX();
	gdi.SetCX(basex+100);
	gdi.DropLine();
	gdi.AddString("", 10, "Markera en stämpel i stämpellistan för att ta bort den eller ändra tiden. Från banmallen kan saknade stämplar läggas till. Saknas måltid får löparen status utgått. Saknas stämpel får löparen status felstämplat. Det går inte att sätta en status på löparen som inte överensstämmer med stämpeldata. Finns målstämpling måste tiden för denna ändras för att ändra måltiden; samma princip gäller för startstämpling.");		

	gdi.RegisterEvent("LoadRunner", RunnerCB);
	gdi.Refresh();
}