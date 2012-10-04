/************************************************************************
    MeOS - Orienteering Software
    Copyright (C) 2009-2012 Melin Software HB
    
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
    Stigbergsvägen 7, SE-75242 UPPSALA, Sweden
    
************************************************************************/

#include "stdafx.h"

#include "resource.h"

#include <commctrl.h>
#include <commdlg.h> 
#include <cassert>

#include "oEvent.h"
#include "xmlparser.h"
#include "gdioutput.h"
#include "csvparser.h"
#include "SportIdent.h"
#include "gdifonts.h"

#include "TabCourse.h"
#include "TabCompetition.h"
#include "meos_util.h"

TabCourse::TabCourse(oEvent *poe):TabBase(poe)
{
  courseId = 0;
  addedCourse = false;
}

TabCourse::~TabCourse(void)
{
}

void LoadCoursePage(gdioutput &gdi);
void LoadClassPage(gdioutput &gdi);

void TabCourse::selectCourse(gdioutput &gdi, pCourse pc)
{
  gdi.setText("TimeLimit", "");
  gdi.disableInput("TimeLimit");
  gdi.setText("PointLimit", "");
  gdi.disableInput("PointLimit");
  gdi.setText("PointReduction", "");
  gdi.disableInput("PointReduction");
  gdi.selectItemByData("Rogaining", 0);
  if (pc) {
		pc->synchronize();

		gdi.setText("Controls", pc->getControlsUI());
		gdi.setText("Name", pc->getName());

		gdi.setTextZeroBlank("Length", pc->getLength());
    gdi.setTextZeroBlank("NumberMaps", pc->getNumberMaps());

    int rt = pc->getMaximumRogainingTime();
    int rp = pc->getMinimumRogainingPoints();

    if ( rt > 0 ) {
      gdi.selectItemByData("Rogaining", 1);
      gdi.enableInput("TimeLimit");
      gdi.setText("TimeLimit", formatTime(rt));
      gdi.enableInput("PointReduction");
      gdi.setText("PointReduction", itos(pc->getRogainingPointsPerMinute()));
    }
    else if (rp > 0) {
      gdi.selectItemByData("Rogaining", 2);
      gdi.enableInput("PointLimit");
      gdi.setText("PointLimit", itos(rp));
    }

		courseId = pc->getId();
		gdi.enableInput("Remove");
		gdi.enableInput("Save");

    gdi.selectItemByData("Courses", pc->getId());
    gdi.setText("CourseProblem", pc->getCourseProblems(), true);
	  gdi.enableEditControls(true);
	}
  else {
		gdi.setText("Name", "");
		gdi.setText("Controls", "");			
		gdi.setText("Length", "");
    gdi.setText("NumberMaps", "");
		courseId = 0;
		gdi.disableInput("Remove");
		gdi.disableInput("Save");
    gdi.selectItemByData("Courses", -1);
    gdi.setText("CourseProblem", "", true);
	  gdi.enableEditControls(false);
	}
}

int CourseCB(gdioutput *gdi, int type, void *data)
{
  TabCourse &tc = dynamic_cast<TabCourse &>(*gdi->getTabs().get(TCourseTab));

  return tc.courseCB(*gdi, type, data);
}

void TabCourse::save(gdioutput &gdi)
{
  DWORD cid = courseId;

	pCourse pc;			
	string name=gdi.getText("Name");
	
	if(name.empty()) { 
		gdi.alert("Banan måste ha ett namn.");
		return;
	}

	bool create=false;
	if(cid>0)
		pc=oe->getCourse(cid);
	else {
		pc=oe->addCourse(name);
		create=true;
	}

	pc->setName(name);
	pc->importControls(gdi.getText("Controls"));
	pc->setLength(gdi.getTextNo("Length"));
  pc->setNumberMaps(gdi.getTextNo("NumberMaps"));


  string t;
  pc->setMaximumRogainingTime(convertAbsoluteTimeMS(gdi.getText("TimeLimit")));
  pc->setMinimumRogainingPoints(atoi(gdi.getText("PointLimit").c_str()));
  pc->setRogainingPointsPerMinute(atoi(gdi.getText("PointReduction").c_str()));

	pc->synchronize();//Update SQL

	oe->fillCourses(gdi, "Courses");
	oe->reEvaluateCourse(pc->getId(), true);

	if(gdi.getData("FromClassPage", cid)) {	
    assert(false);
	}
  else if(addedCourse || create)
		selectCourse(gdi, 0);
	else
		selectCourse(gdi, pc);

}

int TabCourse::courseCB(gdioutput &gdi, int type, void *data)
{
	if (type==GUI_BUTTON)	{
		ButtonInfo bi=*(ButtonInfo *)data;

		if(bi.id=="Save") {
			save(gdi);
		}
    else if (bi.id=="BrowseCourse") {
      vector< pair<string, string> > ext;
      ext.push_back(make_pair("Alla banfiler", "*.xml;*.csv;*.txt"));
      ext.push_back(make_pair("Banor, OCAD semikolonseparerat", "*.csv;*.txt"));
      ext.push_back(make_pair("Banor, IOF (xml)", "*.xml"));

      string file=gdi.browseForOpen(ext, "csv");

			if(file.length()>0)
				gdi.setText("FileName", file);
		}
  	else if (bi.id=="ImportCourses")	{
      setupCourseImport(gdi, CourseCB);
		}
		else if (bi.id=="DoImportCourse") {
      string filename = gdi.getText("FileName");
      if (filename.empty())
        return 0;
      gdi.disableInput("DoImportCourse");
			gdi.disableInput("Cancel");
			gdi.disableInput("BrowseCourse");
      gdi.disableInput("AddClasses");

      try {
        TabCourse::runCourseImport(gdi, filename, oe, gdi.isChecked("AddClasses"));
      }
      catch (std::exception &) {
        gdi.enableInput("DoImportCourse");
			  gdi.enableInput("Cancel");
			  gdi.enableInput("BrowseCourse");
        gdi.enableInput("AddClasses");
        throw;
      }
			gdi.addButton("Cancel", "OK", CourseCB);
      gdi.dropLine();
      gdi.refresh();
    }  
		else if(bi.id=="Add") {
      if (courseId>0) {
        string ctrl = gdi.getText("Controls");
        string name = gdi.getText("Name");
        pCourse pc = oe->getCourse(courseId);
        if (pc && !name.empty() && !ctrl.empty() &&  pc->getControlsUI() != ctrl) {
          if (name == pc->getName()) {
            // Make name unique if same name
            int len = name.length();
            if (len > 2 && (isdigit(name[len-1]) || isdigit(name[len-2]))) {
              ++name[len-1]; // course 1 ->  course 2, course 1a -> course 1b
            }
            else
              name += " 2";
          }
          if (gdi.ask("Vill du lägga till banan 'X' (Y)?#" + name + "#" + ctrl)) {
            pc = oe->addCourse(name);
            courseId = pc->getId();
            gdi.setText("Name", name);
            save(gdi);
            return true;
          }
        }
        save(gdi);
      }
      pCourse pc = oe->addCourse(oe->getAutoCourseName());
      pc->synchronize();
			oe->fillCourses(gdi, "Courses");
      selectCourse(gdi, pc);
      gdi.setInputFocus("Name", true);
      addedCourse = true;
		}
		else if(bi.id=="Remove"){
			DWORD cid = courseId;
			if(cid==0){
				gdi.alert("Ingen bana vald.");
				return 0;
			}

			if(oe->isCourseUsed(cid))
				gdi.alert("Banan används och kan inte tas bort.");
			else
				oe->removeCourse(cid);

			oe->fillCourses(gdi, "Courses");

			selectCourse(gdi, 0);
		}
    else if(bi.id=="Cancel"){
			LoadPage("Banor");
		}
  }
	else if(type==GUI_LISTBOX){
		ListBoxInfo bi=*(ListBoxInfo *)data;

		if (bi.id=="Courses") {
      if (gdi.isInputChanged(""))
        save(gdi);

			pCourse pc=oe->getCourse(bi.data);
			selectCourse(gdi, pc);
      addedCourse = false;
		}
    else if (bi.id=="Rogaining") {
      string t;
      t = gdi.getText("TimeLimit");
      if (!t.empty() && t != "-")
        time_limit = t;
      t = gdi.getText("PointLimit");
      if (!t.empty())
        point_limit = t;
      t = gdi.getText("PointReduction");
      if (!t.empty())
        point_reduction = t;

      string tl, pl, pr;
      if (bi.data == 1) {
        tl = time_limit;
        pr = point_reduction;
      }
      else if (bi.data == 2) {
        pl = point_limit;  
      }

      gdi.setInputStatus("TimeLimit", !tl.empty());
      gdi.setText("TimeLimit", tl);
    
      gdi.setInputStatus("PointLimit", !pl.empty());
      gdi.setText("PointLimit", pl);
      
      gdi.setInputStatus("PointReduction", !pr.empty());
      gdi.setText("PointReduction", pr);
    }
	}
  else if(type==GUI_CLEAR) {
    if (courseId>0)
      save(gdi);

    return true;
  }
	return 0;
}

bool TabCourse::loadPage(gdioutput &gdi)
{	
  oe->checkDB();
  gdi.selectTab(tabId);

	DWORD ClassID=0, RunnerID=0;

  time_limit = "60:00:00";
  point_limit = "10";
  point_reduction = "1";

	gdi.getData("ClassID", ClassID);
	gdi.getData("RunnerID", RunnerID);

	gdi.clearPage(false);

	gdi.setData("ClassID", ClassID);
	gdi.setData("RunnerID", RunnerID);
	gdi.addString("", boldLarge, "Banor");
	gdi.pushY();

	gdi.fillDown();
  gdi.addListBox("Courses", 200, 360, CourseCB, "Banor (antal kontroller)").isEdit(false).ignore(true);
	gdi.setTabStops("Courses", 160);

	oe->fillCourses(gdi, "Courses");

  gdi.dropLine(1);
  gdi.addButton("ImportCourses", "Importera från fil...", CourseCB);

	gdi.newColumn();
	gdi.fillDown();

	gdi.popY();
	
  gdi.addString("", 10, "help:25041");

  gdi.dropLine(0.5);
  gdi.pushX();
  gdi.fillRight();
  gdi.addInput("Name", "", 16, 0, "Namn:");
  gdi.addInput("Length", "", 8, 0, "Längd (m):");  
  gdi.fillDown();	
  gdi.addInput("NumberMaps", "", 6, 0, "Antal kartor:"); 
  gdi.popX();

	gdi.addInput("Controls", "", 54, 0, "Kontroller:");
  gdi.dropLine(0.5);
	gdi.addString("", 10, "help:12662");
  gdi.dropLine(1.5);

  RECT rc;
  rc.top = gdi.getCY() -5;
  rc.left = gdi.getCX();
  gdi.setCX(gdi.getCX()+gdi.scaleLength(10));
  
  gdi.addString("", 1, "Rogaining");
  gdi.dropLine(0.5);
  gdi.fillRight();
  gdi.addSelection("Rogaining", 120, 80, CourseCB);
  gdi.addItem("Rogaining", lang.tl("Ingen rogaining"), 0);
  gdi.addItem("Rogaining", lang.tl("Tidsgräns"), 1);
  gdi.addItem("Rogaining", lang.tl("Poänggräns"), 2);
  
  gdi.setCX(gdi.getCX()+gdi.scaleLength(20));
  gdi.dropLine(-0.8);
  gdi.addInput("PointLimit", "", 8, 0, "Poänggräns:").isEdit(false);
  gdi.addInput("TimeLimit", "", 8, 0, "Tidsgräns:").isEdit(false);
  gdi.addInput("PointReduction", "", 8, 0, "Poängavdrag (per minut):").isEdit(false);
  gdi.dropLine(3.5);
  
  rc.bottom = gdi.getCY() + 5;
  rc.right = gdi.getCX() + 5;
  gdi.addRectangle(rc, colorLightBlue, true);

  gdi.popX();
  gdi.fillDown();
  gdi.dropLine(2);
	gdi.addString("CourseProblem", 1, "").setColor(colorRed);
  gdi.dropLine(2);
	gdi.fillRight();	
  gdi.addButton("Save", "Spara", CourseCB, "help:save").setDefault();
	gdi.addButton("Remove", "Radera", CourseCB);
	gdi.addButton("Add", "Ny bana", CourseCB);
	gdi.disableInput("Remove");
  gdi.disableInput("Save");
  
  selectCourse(gdi, oe->getCourse(courseId));
	gdi.setOnClearCb(CourseCB);

	gdi.refresh();

  return true;
}

void TabCourse::runCourseImport(gdioutput& gdi, const string &filename, 
                                oEvent *oe, bool addClasses) {
	csvparser csv;
  if (csv.iscsv(filename.c_str())) {
    gdi.fillRight();
    gdi.pushX();
	  gdi.addString("", 0, "Importerar OCAD csv-fil...");
	  gdi.refreshFast();
		
	  if(csv.ImportOCAD_CSV(*oe, filename.c_str(), addClasses)) {
      gdi.addString("", 1, "Klart.").setColor(colorGreen);
	  }
    else gdi.addString("", 0, "Operationen misslyckades.").setColor(colorRed);
    gdi.popX();
    gdi.dropLine(2.5);
    gdi.fillDown();
  }
  else {
    oe->importXML_EntryData(gdi, filename.c_str(), addClasses);
  }

  gdi.setWindowTitle(oe->getTitleName());
  oe->updateTabs();
	gdi.refresh();
}

void TabCourse::setupCourseImport(gdioutput& gdi, GUICALLBACK cb) {

	gdi.clearPage(true);
	gdi.addString("", 2, "Importera banor/klasser");
	gdi.addString("", 0, "help:importcourse");
	gdi.dropLine();

	gdi.fillRight();
	gdi.pushX();
  gdi.addInput("FileName", "", 32, 0, "Filnamn:");
	gdi.dropLine();
	gdi.fillDown();
	gdi.addButton("BrowseCourse", "Bläddra...", CourseCB);
	
  gdi.dropLine(0.5);
	gdi.popX();
	
  gdi.fillDown();
  gdi.addCheckbox("AddClasses", "Lägg till klasser", 0, true);

	gdi.dropLine();
	gdi.fillRight();
  gdi.addButton("DoImportCourse", "Importera", cb).setDefault();
	gdi.fillDown();					
  gdi.addButton("Cancel", "Avbryt", cb).setCancel();
  gdi.setInputFocus("FileName");
  gdi.popX();
}
