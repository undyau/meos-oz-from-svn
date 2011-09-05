// meos.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "resource.h"

#include "oEvent.h"
#include "xmlparser.h"

#include "gdioutput.h"
#include "commctrl.h"

#include "oEvent.h"

extern pEvent gEvent;

void LoadRunnerPage(gdioutput &gdi);

void SelectControl(gdioutput *gdi,  pControl pc)
{
	if(pc){
		pc->Syncronize();
		if(pc->GetStatus()==oControl::StatusOK)
			gdi->SelectedItemByData("Status", 1);
		else
			gdi->SelectedItemByData("Status", 2);

		gdi->SetText("ControlID", pc->GetId(), true);

		gdi->SetText("Code", pc->CodeNumbers());
		gdi->SetText("Name", pc->GetName());

		gdi->SetData("ControlID", pc->GetId());

		gdi->EnableInput("Remove");
		gdi->EnableInput("Save");
	}
	else{
		gdi->SelectedItemByData("Status", 1);
		gdi->SetText("Code", "");
		gdi->SetText("Name", "");
		gdi->SetData("ControlID", 0);
		gdi->SetText("ControlID", "-", true);

		gdi->DisableInput("Remove");
		gdi->EnableInput("Save");
	}
}

int ControlsCB(gdioutput *gdi, int type, void *data)
{
	if(type==GUI_BUTTON){
		ButtonInfo bi=*(ButtonInfo *)data;

		if(bi.id=="Save"){
			DWORD pcid;
			if(!gdi->GetData("ControlID", pcid)){
				pcid=0;
				gdi->SetData("ControlID", 0);
			}
			
			bool create=false;

			pControl pc;
			if(pcid==0){
				pc=gEvent->AddControl(0, 0, "");
				create=true;
			}
			else pc=gEvent->GetControl(pcid);


			if(!pc->SetNumbers(gdi->GetText("Code")))
				gdi->Alert("Kodsiffran måste vara ett heltal. Flera kodsiffror måste separeras med komma.");				
							
			ListBoxInfo lbi;
			gdi->GetSelectedItem("Status", &lbi);

			if(lbi.data==1)
				pc->SetStatus(oControl::StatusOK);
			else
				pc->SetStatus(oControl::StatusBad);

			pc->SetName(gdi->GetText("Name"));

			pc->Syncronize();
			gEvent->FillControls(*gdi, "Controls");

			if(create)	SelectControl(gdi, 0);
			else		SelectControl(gdi, pc);

			gEvent->ReEvaluateAll(true);
		}
		else if(bi.id=="Add"){
			SelectControl(gdi, 0);
		}
		else if(bi.id=="Remove"){

			DWORD cid;
			if(!gdi->GetData("ControlID", cid) || cid==0){
				gdi->Alert("Ingen kontroll vald vald.");
				return 0;
			}

			if(gEvent->IsControlUsed(cid))
				gdi->Alert("Kontrollen används och kan inte tas bort.");
			else
				gEvent->RemoveControl(cid);

			gEvent->FillControls(*gdi, "Controls");
			SelectControl(gdi, 0);
		}
	}
	else if(type==GUI_LISTBOX){
		ListBoxInfo bi=*(ListBoxInfo *)data;

		if(bi.id=="Controls"){			
			pControl pc=gEvent->GetControl(bi.data);
			SelectControl(gdi, pc);
		}
	}

	return 0;
}


void LoadControlsPage(gdioutput &gdi)
{
	gdi.SelectTab(TabPageControls);

	gdi.ClearPage();
	
	gdi.FillDown();

	gdi.AddString("", 3, "Kontroller");

	gdi.PushY();
	//gdi.AddString("", 1, "OBS: Denna funktion är inte ännu i funktion riktigt...");
	gdi.AddListBox("Controls", 220, 400, ControlsCB, "Kontroller:");
	gdi.SetTabStops("Controls", 150);
	gEvent->FillControls(gdi, "Controls");

	gdi.FillRight();	
	gdi.AddButton("Add", "Ny kontroll", ControlsCB);
	gdi.AddButton("Remove", "Ta bort", ControlsCB);
	gdi.DisableInput("Remove");

	gdi.NewColumn();
	gdi.FillDown();

	//gdi.DropLine();
	//gdi.DropLine();
	gdi.PopY();

	gdi.AddInput("Name", "", 10, 0, "Namn:");

	gdi.AddSelection("Status", 100, 100, ControlsCB, "Status:", "Ange om kontrollen fungerar och ska användas eller inte");
	gdi.AddItem("Status", "OK", 1);
	gdi.AddItem("Status", "Trasig", 2);

	
	gdi.AddString("", 0, "Ange en eller flera kodsiffror (SI-kod) som används");		
	gdi.AddString("", 0, "av den här kontrollen. Exempel 31, 32, 33.");
	
	//gdi.PushX();
	//gdi.FillRight();
	gdi.AddInput("Code", "", 10, 0, "Kodsiffra/or:");
	//gdi.FillDown();


	gdi.PushX();
	gdi.FillRight();
	gdi.AddString("", 0, "Kontrollnummer: ");

	gdi.FillDown();
	gdi.AddString("ControlID", 1, "-");

	gdi.PopX();
	gdi.DropLine();

	//gdi.PopX();
	gdi.AddButton("Save", "Spara", ControlsCB);	

	gdi.AddString("", 10, "Till varje kontroll anger man 1 eller flera kodsiffror (SI-kod). En normal kontroll har en kod... \n\nOm man sätter kontrollens status till 'Trasig', kommer den inte att användas i stämpelkontrollen och löpare som saknar den kommer inte att markeras som felstämplade på grund av det.\n\nVarje kontroll kan tilldelas ett namn om så önskas, t.ex. 'förvarning', för att underlätta administrationen.");



		
}