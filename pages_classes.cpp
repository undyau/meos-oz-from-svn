#include "stdafx.h"
#include "resource.h"


#include "oEvent.h"
#include "xmlparser.h"

#include "gdioutput.h"
#include "commctrl.h"

#include "oEvent.h"


extern pEvent gEvent;


void LoadCoursePage(gdioutput &gdi, int Id=0);
void LoadClassPage(gdioutput &gdi, int Id=0);
void LoadRunnerPage(gdioutput &gdi);
static bool EditChanged=false;

void MultiCourse(gdioutput &gdi);

int TimeToRel(string s)
{	
	int minute=atoi(s.c_str());

	int second=0;

	int kp=s.find_first_of(':');	
	if(kp>0)
	{		
		string mtext=s.substr(kp+1);
		second=atoi(mtext.c_str());
	}
	return minute*60+second;
}



static list<ClassInfo> cInfo;

//greater<ClassInfo>
bool ClassInfoSortStart(ClassInfo &ci1, ClassInfo &ci2)
{
	return ci1.FirstStart>ci2.FirstStart;
}


// User-defined predicate function for sorting.
/*struct lessThan : public greater<ClassInfo> 
{
    bool operator()(ClassInfo &c1, ClassInfo &c2) const
    { 
		return c1.FirstStart<c2.FirstStart;
    }
};
*/
/*
class CStudentCompare
{
public:
bool operator() (const ClassInfo &x, const ClassInfo &y)
{ return x.FirstStart < y.FirstStart; }

};*/

int MultiCB(gdioutput *gdi, int type, void *data)
{
	if(type==GUI_BUTTON)
	{
		ButtonInfo bi=*(ButtonInfo *)data;

		if (bi.id=="SetNStage") {
			int nstages=gdi->GetTextNo("NStage");
			DWORD cid;
			if (!gdi->GetData("ClassID", cid) || cid<=0) {
				gdi->Alert("Ingen klass vald.");
				return 0;
			}
			pClass pc=gEvent->GetClass(cid);

			if (nstages>0 && nstages<32) {

				gdi->SelectedItemByData("Courses", -2);
				gdi->DisableInput("Courses", false);

				pc->SetNumStages(nstages);
				pc->FillStages(*gdi, "Stages");
				gdi->SelectFirstElement("Stages");
				pc->FillStageCourses(*gdi, 0, "StageCourses");

				gdi->DisableInput("MAdd", true);
				gdi->DisableInput("MCourses", true);
				gdi->DisableInput("MRemove", true);

			}
			else if (nstages==0){
				pc->SetNumStages(0);
				gdi->Restore();
				gdi->DisableInput("MultiCourse", true);
				gdi->DisableInput("Courses", true);
			}
			else {
				gdi->Alert("Antalet sträckor måste vara ett heltal mellan 0 och 30.");				
			}
		}
		else if(bi.id=="MAdd"){
			DWORD cid;
			if (!gdi->GetData("ClassID", cid) || cid<=0)	{
				gdi->Alert("Ingen klass vald.");
				return 0;
			}
			pClass pc=gEvent->GetClass(cid);

			ListBoxInfo lbi;
			if(gdi->GetSelectedItem("Stages", &lbi)){
				int stage=lbi.data;

				if(gdi->GetSelectedItem("MCourses", &lbi)) {
					int courseid=lbi.data;

					pc->AddStageCourse(stage, courseid);
					pc->FillStageCourses(*gdi, stage, "StageCourses");

					pc->Syncronize();
					gEvent->CheckOrderIdMultipleCourses(cid);
				}
			}
			
		}
		else if(bi.id=="MRemove"){

			DWORD cid;
			if (!gdi->GetData("ClassID", cid) || cid<=0)	{
				gdi->Alert("Ingen klass vald.");
				return 0;
			}
			pClass pc=gEvent->GetClass(cid);

			ListBoxInfo lbi;
			if(gdi->GetSelectedItem("Stages", &lbi)){
				int stage=lbi.data;

				if(gdi->GetSelectedItem("StageCourses", &lbi)) {
					int courseid=lbi.data;

					pc->RemoveStageCourse(stage, courseid);
					pc->Syncronize();
					pc->FillStageCourses(*gdi, stage, "StageCourses");
				}
			}

		}
	}
	else if(type==GUI_LISTBOX)
	{
		ListBoxInfo bi=*(ListBoxInfo *)data;

		if(bi.id=="Stages")
		{
			DWORD cid;
			if (!gdi->GetData("ClassID", cid) || cid<=0)	{
				gdi->Alert("Ingen klass vald.");
				return 0;
			}
			pClass pc=gEvent->GetClass(cid);

			pc->FillStageCourses(*gdi, bi.data, "StageCourses");

			gdi->DisableInput("MAdd", true);
			gdi->DisableInput("MCourses", true);
			gdi->DisableInput("MRemove", true);
			//ListBoxInfo lbi;
			//gdi->GetSelectedItem("Stages", &lbi);


			/*pClass pc=gEvent->GetClass(bi.data);
			pc->Syncronize();

			if(EditChanged)
			{
				if(gdi->Ask("Spara ändringar?"))
					gdi->SendCtrlMessage("Save");
			}

			gdi->SetText("Name", pc->GetName());

			pCourse pcourse=pc->GetCourse();

			gEvent->FillCourses(*gdi, "Courses", true);
			gdi->AddItem("Courses", "Ny bana...", 0);
			gdi->AddItem("Courses", "Ingen bana", -2);

			if(pcourse)
				gdi->SelectedItemByData("Courses", pcourse->GetId());
			else
				gdi->SelectedItemByData("Courses", -2);

			EditChanged=false;
		
			gdi->SetData("ClassID", bi.data);*/
		}
		else if(bi.id=="Courses")
			EditChanged=true;
	}
	else if(type==GUI_INPUTCHANGE){
		InputInfo ii=*(InputInfo *)data;

		if(ii.id=="NStage")
			gdi->DisableInput("SetNStage", true);
		
	}
	return 0;
}
int ClassesCB(gdioutput *gdi, int type, void *data)
{
	if(type==GUI_BUTTON)
	{
		ButtonInfo bi=*(ButtonInfo *)data;

		if(bi.id=="Cancel"){
			LoadClassPage(*gdi);
			return 0;
		}
		else if(bi.id=="DoDrawAll"){
			list<ClassInfo>::iterator it=cInfo.begin();
			int nVacances=gdi->GetTextNo("Vacances");
			int BaseInterval=TimeToRel(gdi->GetText("BaseInterval"));
			int FirstStart=gEvent->GetRelativeTime(gdi->GetText("FirstStart"));

			while(it!=cInfo.end()){
				gEvent->DrawList(it->ClassId, FirstStart+BaseInterval*it->FirstStart,
					BaseInterval*it->Interval, nVacances);
				++it;
			}
			gdi->ClearPage();

			gdi->AddButton("Cancel", "Återgå", ClassesCB);
			gEvent->GenerateStartlist(*gdi);
		}
		else if(bi.id=="DrawAll"){
			EditChanged=false;
			/*
			if(gEvent->ClassHasResults(cid))
			{
				if(!gdi->Ask("Klassen har redan resultat. Starttider kommer att skrivas över. Vill du verkligen fortsätta?"))
					return 0;
			}*/

			gdi->ClearPage();

			gdi->AddString("", 1, "Lotta alla klasser");
		
			//gdi->PushX();
			//gdi->FillRight();
			gdi->AddInput("FirstStart", gEvent->GetAbsTime(3600), 10, 0, "Första start:");
			gdi->AddInput("BaseInterval", "1:00", 10, 0, "Basintervall (min)");
			gdi->AddInput("MinInterval", "2:00", 10, 0, "Minsta intervall i klass");
			gdi->AddInput("MaxInterval", "8:00", 10, 0, "Största intervall i klass");
			gdi->AddInput("nFields", "5", 10, 0, "Max antal parallellt startande");
			//gdi->FillDown();
			gdi->AddInput("Vacanses", "0", 10, 0, "Antal vakanser");
			//gdi->PopX();
			gdi->DropLine();

			gdi->SetRestorePoint();
			gdi->PushX();
			gdi->FillRight();
			gdi->AddButton("PrepareDrawAll", "Fördela starttider", ClassesCB);
			//gdi->AddButton("DoDrawClumped", "Klungstart", ClassesCB);
		
			gdi->FillDown();
			
			gdi->AddButton("Cancel", "Avbryt", ClassesCB);


			gdi->PopX();
			gdi->DropLine();
			gdi->AddString("", 10, "Fyll i första starttid och startintervall. Vanlig lottning innebär en traditionell lottning. Klumpad lottning innebär att hela klassen startar i småklungor under det intervall du anger (\"utdragen\" masstart). \n\nAnge intervall 0 för gemensam start.");

			gdi->Refresh();
		}
		else if(bi.id=="PrepareDrawAll"){

			gdi->Restore();
			//gEvent->DrawListAll(*gdi, 0, 2,0);
			int nVacances=gdi->GetTextNo("Vacances");
			int BaseInterval=TimeToRel(gdi->GetText("BaseInterval"));
			int MinInterval=TimeToRel(gdi->GetText("MinInterval"));
			int MaxInterval=TimeToRel(gdi->GetText("MaxInterval"));
			int nFields=gdi->GetTextNo("nFields");
			int FirstStart=gEvent->GetRelativeTime(gdi->GetText("FirstStart"));

			gEvent->OptimizeStartOrder(*gdi, nVacances, BaseInterval, MinInterval, MaxInterval, nFields, cInfo, true);
			

			ClassInfo::sSortOrder=1;			
			cInfo.sort();
			ClassInfo::sSortOrder=0;
			list<ClassInfo>::iterator it=cInfo.begin();
			
			int laststart=0;
			while(it!=cInfo.end()){
				laststart=max(laststart, it->FirstStart+it->nRunners*it->Interval);
				++it;
			}

			gdi->AddString("", 1, string("Sista start: ")+gEvent->GetAbsTime((laststart)*BaseInterval+FirstStart));

			gdi->DropLine();
			gdi->AddString("", 1, "Sammanställnig, klasser:");
			it=cInfo.begin();

			int cx=gdi->GetCX();
			while(it!=cInfo.end()){

				char bf[256];
				int laststart=it->FirstStart+it->nRunners*it->Interval;
				string first=gEvent->GetAbsTime(it->FirstStart*BaseInterval+FirstStart);
				string last=gEvent->GetAbsTime((laststart)*BaseInterval+FirstStart);

				sprintf_s(bf, 256, "%s, %d platser. Start %d-[%d]-%d (%s-%s)",
					gEvent->GetClass(it->ClassId)->GetName().c_str(), it->nRunners,
					it->FirstStart, it->Interval, laststart, first.c_str(), last.c_str());
				
				gdi->AddString("", 0, bf);
			/*	int cy=gdi->GetCY();
				gdi->AddString("", 1, bf);

				sprintf(bf, "%d", nVacances);
				gdi->AddInput(bf, cx+100, cy, "id", 5);

				
				gdi->AddInput(gEvent->GetAbsTime(it->FirstStart*BaseInterval+3600), cx+200, cy, "id", 5);
				
				int i=it->Interval*BaseInterval;
				sprintf(bf, "%d:%02d", i/60, i%60);				
				gdi->AddInput(bf, cx+300, cy, "id", 5);
			*/	
			//	gdi->DropLine();
				++it;
			}

			gdi->DropLine();
			gdi->FillRight();
			gdi->AddButton("DoDrawAll", "Lotta allt", ClassesCB);
			gdi->AddButton("DrawAll", "Ändra inställningar", ClassesCB);
			gdi->FillDown();
			gdi->DropLine();

			gdi->Refresh();
			gdi->UpdateScrollbars();
		}
		else if(bi.id=="DoDraw" || bi.id=="DoDrawClumped"){			
			DWORD cid;
			if(!gdi->GetData("ClassID", cid) || cid==0)
			{
				gdi->Alert("Ingen klass vald.");
				return 0;
			}
			gdi->DisableInput("DoDraw");
			gdi->DisableInput("DoDrawClumped");
			gdi->DisableInput("Cancel");
			gdi->Restore();

			gdi->AddButton("Cancel", "Återgå", ClassesCB);

			gdi->PopX();
		
			string m=gdi->GetText("Interval");
			int minute=atoi(m.c_str());

			int second=0;

			int kp=m.find_first_of(':');	
			if(kp>0)
			{		
				string mtext=m.substr(kp+1);
				second=atoi(mtext.c_str());
			}
			int Interval=minute*60+second;

			int Vacanses=gdi->GetTextNo("Vacanses");
			
			if(bi.id=="DoDraw")
				gEvent->DrawList(cid, gEvent->GetRelativeTime(gdi->GetText("FirstStart")), Interval, Vacanses);
			else
				gEvent->DrawListClumped(cid, gEvent->GetRelativeTime(gdi->GetText("FirstStart")), Interval, Vacanses);

			int bib=gdi->GetTextNo("Bib");
			gEvent->AddBib(cid, bib);

			gdi->DropLine();

			gdi->AddString("", 1, "Startlista "+gEvent->GetClass(cid)->GetName());
			
			gdi->DropLine();
		
			gEvent->GenerateStartlist(*gdi, 0, cid);
			gdi->Refresh();
			//LoadClassPage();
			return 0;
		}
		else if(bi.id=="Draw")
		{
			DWORD cid;
			if(!gdi->GetData("ClassID", cid) || cid==0)
			{
				gdi->Alert("Ingen klass vald.");
				return 0;
			}

			if(gEvent->ClassHasResults(cid))
			{
				if(!gdi->Ask("Klassen har redan resultat. Starttider kommer att skrivas över. Vill du verkligen fortsätta?"))
					return 0;
			}

			pClass pc=gEvent->GetClass(cid);
			//gdi->NewColumn();
			gdi->ClearPage();

			gdi->SetData("ClassID", cid);
			gdi->AddString("", 1, "Lotta klassen "+pc->GetName());
			//gdi->AddString("", 1, "Lotta klassen "+pc->GetName());
	
			gdi->PushX();
			gdi->FillRight();
			gdi->AddInput("FirstStart", gEvent->GetAbsTime(3600), 10, 0, "Första start");
			gdi->AddInput("Interval", "2:00", 10, 0, "Startintervall (min)");
			gdi->FillDown();
			gdi->AddInput("Vacanses", "0", 10, 0, "Antal vakanser");
			gdi->AddInput("Bib", "", 10, 0, "Nummerlappar");

			gdi->PopX();
			gdi->DropLine();

			gdi->SetRestorePoint();
			gdi->PushX();
			gdi->FillRight();
			gdi->AddButton("DoDraw", "Vanlig lottning", ClassesCB);
			gdi->AddButton("DoDrawClumped", "Klungstart", ClassesCB);
		
			gdi->FillDown();
			
			gdi->AddButton("Cancel", "Avbryt", ClassesCB);


			gdi->PopX();
			gdi->DropLine();
			gdi->AddString("", 10, "Fyll i första starttid och startintervall. Vanlig lottning innebär en traditionell lottning. Klumpad lottning innebär att hela klassen startar i småklungor under det intervall du anger (\"utdragen\" masstart). \n\nAnge intervall 0 för gemensam start.\n\nNummerlappar: Ange första nummer eller lämna blankt för inga nummerlappar.");

			EditChanged=false;
			gdi->Refresh();
		}
		else if(bi.id=="Bibs"){
			DWORD cid;
			if(!gdi->GetData("ClassID", cid) || cid==0){
				gdi->Alert("Ingen klass vald.");
				return 0;
			}

			pClass pc=gEvent->GetClass(cid);

			gdi->ClearPage();
			gdi->SetData("ClassID", cid);
			gdi->AddString("", 1, "Nummerlappar i " + pc->GetName());
			
			gdi->AddString("", 0, "Ange första nummerlappsnummer eller lämna blankt för inga nummerlappar");
			gdi->AddInput("Bib", "", 10, 0, "Nummerlappar");
	
			gdi->FillRight();
			gdi->AddButton("DoBibs", "Tilldela", ClassesCB);	
			gdi->FillDown();			
			gdi->AddButton("Cancel", "Avbryt", ClassesCB);
			gdi->PopX();
	
			EditChanged=false;
			gdi->Refresh();
		}
		else if(bi.id=="DoBibs"){			
			DWORD cid;
			if(!gdi->GetData("ClassID", cid) || cid==0){
				gdi->Alert("Ingen klass vald.");
				return 0;
			}
			gdi->DisableInput("DoBibs");
			gdi->DisableInput("Cancel");
	
			gdi->DropLine();
			int bib=gdi->GetTextNo("Bib");
			gEvent->AddBib(cid, bib);

			gdi->AddString("", 1, "Utfört");
			gdi->DropLine();
			gdi->AddButton("Cancel", "Återgå", ClassesCB);

	
			return 0;
		}
		else if(bi.id=="MultiCourse"){
			MultiCourse(*gdi);
		}
		else if(bi.id=="Save")
		{
			DWORD cid;
			if(!gdi->GetData("ClassID", cid))
			{
				//gdi->Alert("Ingen klass vald.");
				//return;
				cid=0;
				gdi->SetData("ClassID", 0);
			}
			pClass pc;
			
			string name=gdi->GetText("Name");
			
			if(name=="")
			{
				gdi->Alert("Klassen måste ha ett namn.");
				return 0;
			}

			bool create=false;
			if(cid>0)
				pc=gEvent->GetClass(cid);
			else{
				pc=gEvent->AddClass(name);
				create=true;
			}

			gdi->SetData("ClassID", pc->GetId());
	
			pc->SetName(name);


			string st=gdi->GetText("StartTime");

			if(st.length()>0){
				pc->SetClassStartTime(st);
			}

			ListBoxInfo lbi;
			gdi->GetSelectedItem("Courses", &lbi);

			if(lbi.data==0)
			{
				//Skapa ny bana...
				pCourse pcourse=gEvent->AddCourse(gEvent->GetAutoCourseName());

				pc->SetCourse(pcourse);

				gdi->SetData("ClassID", pc->GetId());

				pc->Syncronize();
				LoadCoursePage(*gdi, pcourse->GetId());

				return 0;
				//r->SetClassId(pc->GetId());
			}
			else if(lbi.data==-2)
				pc->SetCourse(0);
			else if(lbi.data>0)
				pc->SetCourse(gEvent->GetCourse(lbi.data));

			pc->Syncronize();
			gEvent->ReEvaluateClass(pc->GetId(), true);
		
			DWORD RunnerID=0;				
			gdi->GetData("RunnerID", RunnerID);

			if(RunnerID) {
				LoadRunnerPage(*gdi);
				return 0;
			}

			gEvent->FillClasses(*gdi, "Classes");
			EditChanged=false;
		}
		else if(bi.id=="Add")
		{
			if(EditChanged)
			{
				if(gdi->Ask("Spara ändringar?"))
					gdi->SendCtrlMessage("Save");
			}
			gdi->SetText("Name", "");
			gdi->SetInputFocus("Name");

			gdi->DisableInput("Courses", true);
			gdi->DisableInput("MultiCourse", true);
			gdi->Restore(true);

			EditChanged=false;
			gdi->SetData("ClassID", 0);
		}
		else if(bi.id=="Remove")
		{
			EditChanged=false;
			DWORD cid;
			if(!gdi->GetData("ClassID", cid) || cid==0)
			{
				gdi->Alert("Ingen klass vald.");
				return 0;
			}

			if(gEvent->IsClassUsed(cid))
				gdi->Alert("Klassen används och kan inte tas bort.");
			else
				gEvent->RemoveClass(cid);

			gEvent->FillClasses(*gdi, "Classes");
/*
			gdi->SetText("Controls", "");			
			gdi->SetText("Length", "");
			gdi->SetData("CourseID", 0);
*/
		}
	}
	else if(type==GUI_LISTBOX)
	{
		ListBoxInfo bi=*(ListBoxInfo *)data;

		if(bi.id=="Classes")
		{
			pClass pc=gEvent->GetClass(bi.data);
			pc->Syncronize();

			if(EditChanged)
			{
				if(gdi->Ask("Spara ändringar?"))
					gdi->SendCtrlMessage("Save");
			}

			gdi->SetText("Name", pc->GetName());

			pCourse pcourse=pc->GetCourse();

			gEvent->FillCourses(*gdi, "Courses", true);
			gdi->AddItem("Courses", "Ny bana...", 0);
			gdi->AddItem("Courses", "Ingen bana", -2);

			gdi->DisableInput("Courses", true);

			if(pcourse)
				gdi->SelectedItemByData("Courses", pcourse->GetId());
			else
				gdi->SelectedItemByData("Courses", -2);


			if(pc->HasMultiCourse()){				
				gdi->Restore(false);

				MultiCourse(*gdi);
				
				gdi->SetText("StartTime", pc->GetClassStartTimeS());
				gdi->SetText("NStage", pc->GetNumStages());
				pc->FillStages(*gdi, "Stages");

				gdi->SelectFirstElement("Stages");
				pc->FillStageCourses(*gdi, 0, "StageCourses");
				gdi->SelectFirstElement("StageCourses");
				
				gdi->DisableInput("MAdd", true);
				gdi->DisableInput("MCourses", true);
				gdi->DisableInput("MRemove", true);

				gdi->SelectedItemByData("Courses", -2);
				gdi->DisableInput("Courses", false);				
			}
			else{
				gdi->Restore(true);
				gdi->DisableInput("MultiCourse", true);
			}
//			gdi->SelectedItemByData("Courses", pc->GetCourse()->GetId());
			EditChanged=false;
		
			gdi->SetData("ClassID", bi.data);
		}
		else if(bi.id=="Courses")
			EditChanged=true;
	}
	else if(type==GUI_INPUTCHANGE)
	{
		InputInfo ii=*(InputInfo *)data;

		if(ii.id=="Name")
			EditChanged=true;
	}
	else if(type==GUI_CLEAR)
	{
		if(EditChanged)
		{
			if(gdi->Ask("Spara ändringar?"))
				gdi->SendCtrlMessage("Save");
		}
		return true;
	}
	return 0;
}

void MultiCourse(gdioutput &gdi)
{
	gdi.DisableInput("MultiCourse");
	gdi.SetRestorePoint();

 	gdi.NewColumn();
	
	gdi.DropLine();
	gdi.AddString("", 1, "Flera banor / stafett");	
	gdi.PushX();
	gdi.FillRight();	
	gdi.AddInput("NStage", "0", 4, MultiCB, "Antal sträckor");
	gdi.FillDown();
	gdi.DropLine();
	gdi.AddButton("SetNStage", "Verkställ", MultiCB);
	gdi.DisableInput("SetNStage");
	gdi.PopX();

	gdi.AddInput("StartTime", "", 6,0, "Starttid");

	gdi.AddSelection("Stages", 180, 100, MultiCB, "Sträckor:");
	gdi.AddListBox("StageCourses", 180, 200, MultiCB, "Banor:");

	gdi.AddButton("MRemove", "Ta bort markerad", MultiCB);
	gdi.FillRight();
	gdi.AddSelection("MCourses", 180, 100, MultiCB, "Banor:");
	gEvent->FillCourses(gdi, "MCourses", true);
	gdi.FillDown();
	gdi.DropLine();
	gdi.AddButton("MAdd", "Lägg till", MultiCB);
	gdi.PopX();

	gdi.DisableInput("MAdd");
	gdi.DisableInput("MCourse");
	gdi.DisableInput("MRemove");
}


void LoadClassPage(gdioutput &gdi, int Id)
{
	gdi.SelectTab(TabPageClass);
	DWORD ClassID=0, RunnerID=0;

	gdi.GetData("ClassID", ClassID);
	gdi.GetData("RunnerID", RunnerID);

	gdi.ClearPage();

	gdi.SetData("ClassID", ClassID);
	gdi.SetData("RunnerID", RunnerID);

	gdi.AddString("", 2, "Klasser");
//	gdi.NewRow();

	if(!Id)
	{
		gdi.FillDown();
		gdi.AddListBox("Classes", 150, 300, ClassesCB, "");
		gEvent->FillClasses(gdi, "Classes");

		gdi.FillRight();	
		gdi.AddButton("Remove", "Ta bort", ClassesCB);
		gdi.AddButton("Add", "Ny klass", ClassesCB);
		
		gdi.NewColumn();
		gdi.DropLine();
		gdi.DropLine();
	}

	gdi.FillDown();
	gdi.AddInput("Name", "", 16, ClassesCB, "Klassnamn:");

	//gdi.AddInput("Course", gEvent->GetName(), 16, 0, "Bana:");
	gdi.AddSelection("Courses", 150, 400, ClassesCB, "Bana:");
	
	gEvent->FillCourses(gdi, "Courses", true);
	gdi.AddItem("Courses", "Ny bana...", 0);
	gdi.AddItem("Courses", "Ingen bana", -2);
	
	gdi.AddButton("MultiCourse", "Flera banor/stafett...", ClassesCB);

	gdi.AddButton("Save", "Spara", ClassesCB);


	if(!Id)
	{
		gdi.DropLine();
		gdi.AddButton("Draw", "Lotta klass...", ClassesCB);
		gdi.AddButton("Bibs", "Nummerlappar...", ClassesCB);

		gdi.DropLine();
		gdi.DropLine();

		gdi.AddButton("DrawAll", "Lotta allt...", ClassesCB);
		
		gdi.SetOnClearCb(ClassesCB);
	}

	if(Id || ClassID)
	{
		if(!Id)	Id=ClassID;

		pClass pc=gEvent->GetClass(Id);

		gdi.SetText("Name", pc->GetName());

		pCourse pcourse=pc->GetCourse();

		gdi.SetData("ClassID", Id);

		if(pcourse)
			gdi.SelectedItemByData("Courses", pcourse->GetId());
		else
			gdi.SelectedItemByData("Courses", 0);
	}

	//gEvent->FillClassesTB(gdi);
	//gdi.AddTable( gEvent->GetClassesTB(), gdi.GetCX(), gdi.GetCY() );
	gdi.SetRestorePoint(); 

	EditChanged=false;
	gdi.Refresh();
}

