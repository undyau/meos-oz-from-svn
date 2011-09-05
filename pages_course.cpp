#include "stdafx.h"
#include "resource.h"


#include "oEvent.h"
#include "xmlparser.h"

#include "gdioutput.h"
#include "commctrl.h"

#include "oEvent.h"


extern pEvent gEvent;


void LoadClassPage(gdioutput &gdi, int Id=0);
void LoadRunnerPage(gdioutput &gdi);


void SelectCourse(gdioutput *gdi, pCourse pc)
{
	if(pc){
		pc->Syncronize();

		gdi->SetText("Controls", pc->GetControlsUI());
		gdi->SetText("Name", pc->GetName());

		gdi->SetText("Length", pc->GetLength());
		gdi->SetData("CourseID", pc->GetId());

		gdi->EnableInput("Remove");
		gdi->EnableInput("Save");
	}else{
		gdi->SetText("Name", gEvent->GetAutoCourseName());
		gdi->SetText("Controls", "");			
		gdi->SetText("Length", "");
		gdi->SetData("CourseID", 0);

		gdi->DisableInput("Remove");
		gdi->EnableInput("Save");
	}
}

int CourseCB(gdioutput *gdi, int type, void *data)
{
	if(type==GUI_BUTTON)
	{
		ButtonInfo bi=*(ButtonInfo *)data;

		if(bi.id=="Save")
		{
			DWORD cid;
			if(!gdi->GetData("CourseID", cid)){
				cid=0;
				gdi->SetData("CourseID", 0);
			}

			pCourse pc;			
			string name=gdi->GetText("Name");
			
			if(name==""){
				gdi->Alert("Banan måste ha ett namn");
				return 0;
			}

			bool create=false;
			if(cid>0)
				pc=gEvent->GetCourse(cid);
			else{
				pc=gEvent->AddCourse(name);
				create=true;
			}

			pc->SetName(name);
			pc->ImportControls(gdi->GetText("Controls"));
			pc->SetLength(gdi->GetTextNo("Length"));
			
			pc->Syncronize();//Update SQL

			gEvent->FillCourses(*gdi, "Courses");
			gEvent->ReEvaluateCourse(pc->GetId(), true);

			if(gdi->GetData("FromClassPage", cid))
			{	
				//Return to source page
				DWORD RunnerID=0;				
				gdi->GetData("RunnerID", RunnerID);

				if(RunnerID)	LoadRunnerPage(*gdi);
				else			LoadClassPage(*gdi);
				return 0;
			}
			else if(create)
				SelectCourse(gdi, 0);
			else
				SelectCourse(gdi, pc);
		}
		else if(bi.id=="Add"){
			SelectCourse(gdi, 0);
		}
		else if(bi.id=="Remove"){
			DWORD cid;
			if(!gdi->GetData("CourseID", cid) || cid==0){
				gdi->Alert("Ingen bana vald.");
				return 0;
			}

			if(gEvent->IsCourseUsed(cid))
				gdi->Alert("Banan används och kan inte tas bort.");
			else
				gEvent->RemoveCourse(cid);

			gEvent->FillCourses(*gdi, "Courses");

			SelectCourse(gdi, 0);
		}
	}
	else if(type==GUI_LISTBOX){
		ListBoxInfo bi=*(ListBoxInfo *)data;

		if(bi.id=="Courses"){
			pCourse pc=gEvent->GetCourse(bi.data);
			SelectCourse(gdi, pc);
		}
	}
	return 0;
}

void LoadCoursePage(gdioutput &gdi, int Id)
{	
	gdi.SelectTab(TabPageCourse);

	DWORD ClassID=0, RunnerID=0;

	gdi.GetData("ClassID", ClassID);
	gdi.GetData("RunnerID", RunnerID);

	gdi.ClearPage();

	gdi.SetData("ClassID", ClassID);
	gdi.SetData("RunnerID", RunnerID);
	gdi.AddString("", 3, "Banor");
	gdi.PushY();

	if(Id==0){
		gdi.FillDown();
		gdi.AddListBox("Courses", 200, 300, CourseCB, "Banor:");
		gdi.SetTabStops("Courses", 160);

		gEvent->FillCourses(gdi, "Courses");

		gdi.FillRight();	
		gdi.AddButton("Remove", "Ta bort", CourseCB);
		gdi.AddButton("Add", "Ny bana", CourseCB);
		gdi.DisableInput("Remove");
	
		gdi.NewColumn();
		gdi.FillDown();
	}

	gdi.PopY();
	gdi.AddInput("Name", "", 16, 0, "Namn:");
	gdi.AddInput("Length", "", 16, 0, "Längd (m):");

	gdi.AddInput("Controls", "", 48, 0, "Kontroller:");
	gdi.AddString("", 0, "Lägg till kontroller genom att ange en följd av kontrollnummer.");
	gdi.AddString("", 0, "Exempel: 31, 50, 33, 36, 50, 37, 100");

	gdi.AddButton("Save", "Spara", CourseCB);

	if(Id){
		pCourse pc=gEvent->GetCourse(Id);
		SelectCourse(&gdi, pc);
		gdi.SetData("FromClassPage", 1);
	}
	gdi.Refresh();
}
