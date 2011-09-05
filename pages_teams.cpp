#include "stdafx.h"
#include "resource.h"

#include "oEvent.h"
#include "xmlparser.h"

#include "gdioutput.h"
#include "commctrl.h"

extern pEvent gEvent;

void LoadRunnerPage(gdioutput &gdi);
void LoadClassPage(gdioutput &gdi, int Id=0);
void UpdateTeamStatus(gdioutput &gdi, pTeam t);

void LoadTeamMembers(gdioutput &gdi, int ClassId, int ClubId, pTeam t);

void SelectTeam(gdioutput *gdi, pTeam t)
{
	if(t){
		t->Syncronize();

		gdi->SetData("TeamID", t->GetId());
		gdi->SetText("Name", t->GetName());
		gdi->SetText("StartNo", t->GetStartNo());

		gdi->SetText("Club", t->GetClub());

		gEvent->FillClasses(*gdi, "RClass");	
		gdi->SelectedItemByData("RClass", t->GetClassId());
		LoadTeamMembers(*gdi, 0, 0, t);
	}
	else{
		gdi->SetData("TeamID", 0);
		gdi->SetText("Name", "");
		gdi->SetText("StartNo", gEvent->GetFreeStartNo());
		gdi->SetText("Club", "");

		ListBoxInfo lbi;
		gdi->GetSelectedItem("RClass", &lbi);
		LoadTeamMembers(*gdi, lbi.data, 0, 0);
	}

	UpdateTeamStatus(*gdi, t);	
}

void UpdateTeamStatus(gdioutput &gdi, pTeam t)
{
	if (!t) {
		gdi.SetText("Start", "-");
		gdi.SetText("Finish", "-");
		gdi.SetText("Time", "-");
		gdi.SelectedItemByData("Status", 0);
		return;
	}

	gdi.SetText("Start", t->GetStartTimeS());
	gdi.SetText("Finish",t->GetFinishTimeS(-1));
	gdi.SetText("Time", t->GetRunningTimeS(-1));
	gdi.SelectedItemByData("Status", t->GetStatus(-1));
}


int TeamCB(gdioutput *gdi, int type, void *data)
{
	if(type==GUI_BUTTON)
	{
		ButtonInfo bi=*(ButtonInfo *)data;

		if (bi.id=="Save")
		{
			DWORD tid=0;
			gdi->GetData("TeamID", tid);
			
			string name=gdi->GetText("Name");

			if (name.length()==0){
				gdi->Alert("Alla deltagare måste ha ett namn.");
				return 0;
			}

			bool create=false;

			pTeam t;
			if (tid==0) {
				t=gEvent->AddTeam(name);
				create=true;
			}
			else t=gEvent->GetTeam(tid);

			gdi->SetData("TeamID", t->GetId());

			if (t) {				
				t->SetName(name);				
				t->SetStartNo(gdi->GetTextNo("StartNo")); 
				t->SetStartTime(gdi->GetText("Start"));
				t->SetFinishTime(gdi->GetText("Finish"));

				ListBoxInfo lbi;
				gdi->GetSelectedItem("Club", &lbi);
				
				if (lbi.text!=""){
					pClub pc=gEvent->GetClub(lbi.text);
					pc->Syncronize();										
				}

				t->SetClub(lbi.text);
				gEvent->FillClubs(*gdi, "Club");
				gdi->SetText("Club", lbi.text);


				gdi->GetSelectedItem("Status", &lbi);
				t->SetTeamStatus((oRunner::RunnerStatus)lbi.data);
				
				gdi->GetSelectedItem("RClass", &lbi);
				t->SetClassId(lbi.data);

				pClass pc=gEvent->GetClass(lbi.data);
				
				for (unsigned i=0;i<pc->GetNumStages(); i++) {
					char bf[16];
					sprintf_s(bf, "R%d", i);
					//gdi->GetText(bf);

					ListBoxInfo lbi;
					if(gdi->GetSelectedItem(bf, &lbi)){
						pRunner r=gEvent->GetRunner(lbi.data);
						
						if(r){
							sprintf_s(bf, "SI%d", i);					
							r->SetCardNo(gdi->GetTextNo(bf));
							r->Syncronize();
							t->SetRunner(i, r);
						}
					}
					else{
						string name=gdi->GetText(bf);

						if (name.length()<=1) { //Remove
							pRunner p_old=t->GetRunner(i);
							t->SetRunner(i, 0);

							if(p_old && !gEvent->IsRunnerUsed(p_old->GetId())){
								if(gdi->Ask(string("Ska ")+p_old->GetName()+" raderas från tävlingen?")){
									gEvent->RemoveRunner(p_old->GetId());
								}
							}
						}
						else {
							pRunner r=t->GetRunner(i);

							if(r) {								
								if(t->GetClub().length()>1)
									r->SetClub(t->GetClub());
								r->ResetPersonalData();
							}
							else {
								r=gEvent->AddRunner(name, t->GetClubId(), t->GetClassId());
							}

							r->SetName(name);

							sprintf_s(bf, "SI%d", i);					
							r->SetCardNo(gdi->GetTextNo(bf));
							r->Syncronize();
							t->SetRunner(i, r);
						}
					}
				}

				t->Apply();
				t->Syncronize();
				
				gEvent->FillTeams(*gdi, "Teams");
				gdi->SelectedItemByData("Teams", t->GetId());

				gdi->SetText("Time", t->GetRunningTimeS(-1));
				gdi->SelectedItemByData("Status", t->GetStatus(-1));
			}

			if (create) {
				SelectTeam(gdi, 0);
				gdi->SelectedItemByData("Runners", -1);
				gdi->SetInputFocus("Name");
			}

			return true;
		}
		else if (bi.id=="Add") {
			SelectTeam(gdi, 0);
			gdi->SelectedItemByData("Teams", -1);
			gdi->SetInputFocus("Name");
		}
		else if (bi.id=="Remove") {
			DWORD tid;
			if(!gdi->GetData("TeamID", tid))
				return 0;
		
			if(gdi->Ask("Vill du verkligen ta bort löparen?")){
				gEvent->RemoveTeam(tid);
				gEvent->FillTeams(*gdi, "Teams");
				SelectTeam(gdi, 0);
				gdi->SelectedItemByData("Teams", -1);
			}
		}
	}
	else if (type==GUI_LISTBOX) {
		ListBoxInfo bi=*(ListBoxInfo *)data;

		if(bi.id=="Teams") {			
			pTeam t=gEvent->GetTeam(bi.data);
			SelectTeam(gdi, t);
		}
		else if(bi.id=="RClass") { //New class selected.
			DWORD tid=0;
			gdi->GetData("TeamID", tid);

			if(tid){
				pTeam t=gEvent->GetTeam(tid);
				if(t && t->GetClassId()==bi.data)
					LoadTeamMembers(*gdi, 0,0,t);
				else
					LoadTeamMembers(*gdi, bi.data, 0, 0);
			}
			else LoadTeamMembers(*gdi, bi.data, 0, 0);
		}
		else {

			ListBoxInfo lbi;
			gdi->GetSelectedItem("RClass", &lbi);

			if(lbi.data>0){
				pClass pc=gEvent->GetClass(lbi.data);

				if(pc){
					for(unsigned i=0;i<pc->GetNumStages();i++){
						char bf[16];
						sprintf_s(bf, "R%d", i);
						if(bi.id==bf){
							pRunner r=gEvent->GetRunner(bi.data);
							sprintf_s(bf, "SI%d", i);
							gdi->SetText(bf, r->GetCardNo());
						}
					}
				}
			}
		}
	}

	return 0;
}


void LoadTeamMembers(gdioutput &gdi, int ClassId, int ClubId, pTeam t)
{	
	if(ClassId==0)
		if(t) ClassId=t->GetClassId();

	gdi.Restore(false);

	if(!ClassId) return;

	pClass pc=gEvent->GetClass(ClassId);

	gdi.SetRestorePoint();
 	gdi.NewColumn();	
	
	gdi.DropLine();

	char bf[16];	
	char bf_si[16];

	for(unsigned i=0;i<pc->GetNumStages();i++){
		sprintf_s(bf, "R%d", i);
		gdi.PushX();
		gdi.FillRight();
		gdi.AddCombo(bf, 180, 300, TeamCB);
		gdi.FillDown();
		sprintf_s(bf_si, "SI%d", i);		
		gdi.AddInput(bf_si, "", 8);
		gdi.PopX();
		gEvent->FillRunners(gdi, bf);

		if (t) {
			pRunner r=t->GetRunner(i);
			if (r) {
				gdi.SelectedItemByData(bf, r->GetId());
				gdi.SetText(bf_si, r->GetCardNo());
			}
			
		}
	}

	gdi.AddString("", 10, "Löpare + SI-nummer. Antalet löpare i laget\n ställer man in på sidan Klasser");
	//gdi.AddButton("TeamSave", "Spara laguppställning", TeamCB);
}

void LoadTeamPage(gdioutput &gdi)
{
	gdi.SelectTab(TabPageRunner);
	gdi.ClearPage();
	int basex=gdi.GetCX();

	gdi.FillDown();
	gdi.AddListBox("Teams", 200, 300, TeamCB, "Lag:");
	gEvent->FillTeams(gdi, "Teams");

	gdi.FillRight();	
	gdi.AddButton("Remove", "Ta bort", TeamCB);
	gdi.AddButton("Add", "Nytt lag", TeamCB);

	gdi.NewColumn();
	gdi.FillDown();

	gdi.AddInput("Name", "", 16, 0, "Lagamn:");

	gdi.AddInput("StartNo", "", 16, 0, "Startnummer:");

	gdi.AddCombo("Club", 180, 300, 0, "Klubb:");
	gEvent->FillClubs(gdi, "Club");

	//gdi.AddInput("Class", "", 16, 0, "Klass:");
	gdi.AddSelection("RClass", 180, 300, TeamCB, "Klass:");
	gEvent->FillClasses(gdi, "RClass");	
	gdi.AddItem("RClass", "Ny klass", 0);


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
	gEvent->FillStatus(gdi, "Status");

	/*gdi.AddItem("Status", "-", 0);
	gdi.AddItem("Status", "OK", oRunner::StatusOK);
	gdi.AddItem("Status", "Ej start", oRunner::StatusDNS);
	gdi.AddItem("Status", "Felst.", oRunner::StatusMP);
	gdi.AddItem("Status", "Utg.", oRunner::StatusDNF);
	gdi.AddItem("Status", "Disk.", oRunner::StatusDQ);
	gdi.AddItem("Status", "Maxtid", oRunner::StatusMAX);*/
	gdi.PopX();
	gdi.SelectedItemByData("Status", 0);

	gdi.DropLine();
	gdi.AddButton("Save", "Spara", TeamCB);


	gdi.SetRestorePoint();

	gdi.Refresh();
}