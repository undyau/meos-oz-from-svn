/************************************************************************
    MeOS - Orienteering Software
    Copyright (C) 2009-2013 Melin Software HB
    
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
#include "oListInfo.h"
#include "TabList.h"
#include "TabRunner.h"
#include "TabTeam.h"
#include "TabAuto.h"
#include "meos_util.h"
#include <cassert>
#include "classconfiginfo.h"
#include "metalist.h"
#include "gdifonts.h"
#include "listeditor.h"

TabList::TabList(oEvent *poe):TabBase(poe)
{
  ownWindow = false;
  hideButtons = false;
  currentListType=EStdNone;
  listEditor = 0;
}

TabList::~TabList(void)
{
  delete listEditor;
  listEditor = 0;
}

int ListsCB(gdioutput *gdi, int type, void *data);

int ListsEventCB(gdioutput *gdi, int type, void *data)
{
	if(type!=GUI_EVENT)
		return -1;

  TabList &tc = dynamic_cast<TabList &>(*gdi->getTabs().get(TListTab));

  tc.rebuildList(*gdi);

	return 0;
}

void TabList::rebuildList(gdioutput &gdi)
{
 if (!SelectedList.empty()) {
    ButtonInfo bi;
    bi.id=SelectedList;
    ListsCB(&gdi, GUI_BUTTON, &bi);
  }
}

int openRunnerTeamCB(gdioutput *gdi, int type, void *data)
{
	if (type==GUI_LINK && data) {
    oBase *b = static_cast<oBase*>((static_cast<TextInfo*>(data))->getExtra());

    oTeam *t = dynamic_cast<oTeam*>(b);
    if (t!=0 && gdi->canClear()) {
      TabTeam &tt = dynamic_cast<TabTeam &>(*gdi->getTabs().get(TTeamTab));
      tt.loadPage(*gdi, t->getId());
    }
    oRunner *r = dynamic_cast<oRunner*>(b);
    if (r!=0 && gdi->canClear()) {
      TabRunner &tr = dynamic_cast<TabRunner &>(*gdi->getTabs().get(TRunnerTab));
      tr.loadPage(*gdi, r->getId());
    }
 	}
	return 0;
}

int NoStartRunnerCB(gdioutput *gdi, int type, void *data)
{
	if(type==GUI_LINK)
	{
		TextInfo *ti=(TextInfo *)data;
		int id=atoi(ti->id.c_str());

		TabList &tc = dynamic_cast<TabList &>(*gdi->getTabs().get(TListTab));
		pRunner p = tc.getEvent()->getRunner(id, 0);

		if(p){
			p->setStatus(StatusDNS);
			p->synchronize();
			ti->CallBack=0;
			ti->Highlight=false;
			ti->Active=false;
			ti->color=RGB(255,0,0);
			gdi->setText(ti->id, "Ej start", true);
		}		
	}
	return 0;
}

int ListsCB(gdioutput *gdi, int type, void *data)
{
  TabList &tc = dynamic_cast<TabList &>(*gdi->getTabs().get(TListTab));

  return tc.listCB(*gdi, type, data);
}

void TabList::generateList(gdioutput &gdi)
{
  gdi.clearPage(false);
  currentList.setCallback(ownWindow ? 0 : openRunnerTeamCB);
  oe->generateList(gdi, true, currentList);
  
  if (!hideButtons) {
    gdi.addButton(gdi.getWidth()+20, 15,  gdi.scaleLength(120), 
                  "Cancel", ownWindow ? "St�ng" : "�terg�", ListsCB, "", true, false);

    gdi.addButton(gdi.getWidth()+20, 18+gdi.getButtonHeight(), gdi.scaleLength(120), 
                  "Print", "Skriv ut...", ListsCB, "Skriv ut listan.", true, false);
    gdi.addButton(gdi.getWidth()+20, 21+2*gdi.getButtonHeight(), gdi.scaleLength(120),
                  "HTML", "Webb...", ListsCB, "Spara f�r webben.", true, false);
    int baseY = 21+2*gdi.getButtonHeight();
    if (!ownWindow) {
      gdi.addButton(gdi.getWidth()+20, 27+4*gdi.getButtonHeight(), gdi.scaleLength(120),
                    "Automatic", "Automatisera", ListsCB, "Skriv ut eller exportera listan automatiskt.", true, false);

      gdi.addButton(gdi.getWidth()+20, 24+3*gdi.getButtonHeight(), gdi.scaleLength(120),
                    "Window", "Eget f�nster", ListsCB, "�ppna i ett nytt f�nster.", true, false);
      baseY = 27+4*gdi.getButtonHeight();
    }
        
    gdi.addButton(gdi.getWidth()+20, baseY + 3 + gdi.getButtonHeight(), gdi.scaleLength(120),
                  "AutoScroll", "Automatisk skroll", ListsCB, "Rulla upp och ner automatiskt", true, false);

    gdi.addButton(gdi.getWidth()+20, baseY + 6 + 2*gdi.getButtonHeight(), gdi.scaleLength(120),
                  "FullScreen", "Fullsk�rm", ListsCB, "Visa listan i fullsk�rm", true, false);

    if (!currentList.getParam().saved && !oe->isReadOnly()) {
      gdi.addButton(gdi.getWidth()+20, baseY + 9 + 3*gdi.getButtonHeight(),  gdi.scaleLength(120), 
                    "Remember", "Kom ih�g listan", ListsCB, "Spara den h�r listan som en favoritlista", true, false);
    }
  }
  
  gdi.registerEvent("DataUpdate", ListsEventCB);
  gdi.setData("DataSync", 1);
  if (currentList.needPunchCheck())
    gdi.setData("PunchSync", 1);
  gdi.registerEvent("GeneralList", ListsCB);
  gdi.setOnClearCb(ListsCB);
  SelectedList="GeneralList";
}

int TabList::listCB(gdioutput &gdi, int type, void *data)
{
	int index;

	if (type==GUI_BUTTON || type==GUI_EVENT) {
		BaseInfo bi=*(BaseInfo *)data;

		if (bi.id=="Cancel")	{
      if (ownWindow)
        gdi.closeWindow();
      else {
			  SelectedList = "";
        currentListType = EStdNone;
			  loadPage(gdi);
      }
		}
		else if (bi.id=="Print")	{
			gdi.print(oe);			
		}
    else if (bi.id=="HTML")	{
      vector< pair<string, string> > ext;
      ext.push_back(make_pair("Strukturerat webbdokument (html)", "*.html;*.htm"));
      ext.push_back(make_pair("Formaterat webbdokument (html)", "*.html;*.htm"));

      string file=gdi.browseForSave(ext, "html", index);

      if(!file.empty()) {
        if (index == 1)
          gdi.writeTableHTML(file, oe->getName());
        else {
          assert(index == 2);
          gdi.writeHTML(file, oe->getName());
        }
        gdi.openDoc(file.c_str());
      }
		}
    else if (bi.id == "Window" || bi.id == "AutoScroll" || bi.id == "FullScreen") {  
      gdioutput *gdi_new;
      TabList *tl_new = this;
      if (!ownWindow) {
        gdi_new = createExtraWindow(MakeDash("MeOS -" + currentList.getName()), gdi.getWidth() + 64 + gdi.scaleLength(120));
        if (gdi_new) {
          TabList &tl = dynamic_cast<TabList &>(*gdi_new->getTabs().get(TListTab));
          tl.currentList = currentList;
          tl.SelectedList = SelectedList;
          tl.ownWindow = true;
          tl.loadPage(*gdi_new);
          tl_new = &tl;
        }
      }
      else
        gdi_new = &gdi;

      if (gdi_new && bi.id == "AutoScroll" || bi.id == "FullScreen") {
        tl_new->hideButtons = true;
        
        gdi_new->alert("help:fullscreen"); 
        
        if (bi.id == "FullScreen")
          gdi_new->setFullScreen(true);

        int h = gdi_new->setHighContrastMaxWidth();
        tl_new->loadPage(*gdi_new);
        double sec = 6.0;
        double delta = h * 30. / (1000. * sec);
        gdi_new->setAutoScroll(delta);
      }
    }
    else if (bi.id == "Remember") {
      oListParam &par = currentList.getParam();
      string baseName = par.getDefaultName();
      baseName = oe->getListContainer().makeUniqueParamName(baseName);
      par.setName(baseName);
      oe->synchronize(false);
      oe->getListContainer().addListParam(currentList.getParam());
      oe->synchronize(true);
      gdi.removeControl(bi.id);
    }
    else if (bi.id == "ShowSaved") {
      ListBoxInfo lbi;
      if (gdi.getSelectedItem("SavedInstance", &lbi)) {
        oListParam &par = oe->getListContainer().getParam(lbi.data);
        
        oe->generateListInfo(par, gdi.getLineHeight(), currentList);
        generateList(gdi);
      }    
    }
    else if (bi.id == "RenameSaved") {
      ListBoxInfo lbi;
      if (gdi.getSelectedItem("SavedInstance", &lbi)) {
        const oListParam &par = oe->getListContainer().getParam(lbi.data);
        gdi.clearPage(true);
        gdi.addString("", boldLarge, "D�p om X#" + par.getName());
        gdi.setData("ParamIx", lbi.data);
        gdi.dropLine();
        gdi.fillRight();
        gdi.addInput("Name", par.getName(), 24);
        gdi.setInputFocus("Name", true);
        gdi.addButton("DoRenameSaved", "D�p om", ListsCB);
        gdi.addButton("Cancel", "Avbryt", ListsCB);
        gdi.dropLine(3);
      }    
    }
    else if (bi.id == "DoRenameSaved") {
      int ix = int(gdi.getData("ParamIx"));
      oListParam &par = oe->getListContainer().getParam(ix);
      string name = gdi.getText("Name");
      par.setName(name);
      loadPage(gdi);
    }
    else if (bi.id == "RemoveSaved") {
      ListBoxInfo lbi;
      if (gdi.getSelectedItem("SavedInstance", &lbi)) {
        oe->synchronize(false);
        oe->getListContainer().removeParam(lbi.data);
        oe->synchronize(true);
        loadPage(gdi);
      }
      return 0;
    }
    else if (bi.id == "Automatic") {
      PrintResultMachine prm(60*10, currentList);
      tabAutoAddMachinge(prm);
      gdi.getTabs().get(TAutoTab)->loadPage(gdi);
    }
    else if (bi.id=="SelectAll") {
      set<int> lst;
      lst.insert(-1);
      gdi.setSelection("ListSelection", lst);
    }
    else if (bi.id=="SelectNone") {
      set<int> lst;
      gdi.setSelection("ListSelection", lst);
    }
    else if (bi.id=="Generate") {
      ListBoxInfo lbi;
      if (gdi.getSelectedItem("ListType", &lbi)) {
        currentListType=EStdListType(lbi.data);        
      }
      else if(gdi.getSelectedItem("ResultType", &lbi)) {
        switch(lbi.data) {
          case 1:
            currentListType=EStdResultList;
            break;
          case 2:
            currentListType=EStdPatrolResultList;
            break;
          case 3:
            currentListType=EStdTeamResultListAll;
            break;
          case 4:
            currentListType=EStdTeamResultList;
            break;
          case 5:
            currentListType=EStdTeamResultListLeg;
            break;
          case 6:
            currentListType=EGeneralResultList;
            break;
          default: 
            return 0;
        }

      }
      else return 0;

      oListParam par;
      gdi.getSelectedItem("ResultSpecialTo", &lbi);
      par.useControlIdResultTo = lbi.data;

      gdi.getSelectedItem("ResultSpecialFrom", &lbi);
      par.useControlIdResultFrom = lbi.data;

      gdi.getSelectedItem("LegNumber", &lbi);
      par.legNumber = lbi.data == 1000 ? -1 : lbi.data;
      
      gdi.getSelection("ListSelection", par.selection);
      par.filterMaxPer = gdi.getTextNo("ClassLimit");
      par.pageBreak = gdi.isChecked("PageBreak");
      par.listCode = (EStdListType)currentListType;
      par.showInterTimes = gdi.isChecked("ShowInterResults");
      par.showSplitTimes = gdi.isChecked("ShowSplits");
      par.splitAnalysis = gdi.isChecked("SplitAnalysis");  
      par.setCustomTitle(gdi.getText("Title"));
      par.useLargeSize = gdi.isChecked("UseLargeSize");

      oe->generateListInfo(par, gdi.getLineHeight(), currentList);

      generateList(gdi);
      gdi.refresh();
    }
    else if (bi.id=="GeneralList") {
      if (SelectedList=="GeneralList") {
        int oX=gdi.GetOffsetX();
        int oY=gdi.GetOffsetY();
        generateList(gdi);
        gdi.setOffset(oX, oY, false);
      }
      else
        loadGeneralList(gdi);
    }
    else if (bi.id == "EditList") {
      if (!listEditor)
        listEditor = new ListEditor(oe);

      gdi.clearPage(false);
      listEditor->show(gdi);
      gdi.refresh();
    }
    else if (bi.id=="ResultIndividual") {
      oe->sanityCheck(gdi, true);
      oListParam par;
      ClassConfigInfo cnf;
      oe->getClassConfigurationInfo(cnf);
      cnf.getIndividual(par.selection);
      par.listCode = EStdResultList;
      par.showInterTimes = true;
      par.legNumber = -1;
      par.pageBreak = gdi.isChecked("PageBreak");
      par.splitAnalysis = gdi.isChecked("SplitAnalysis");
      
      ListBoxInfo lbi;
      gdi.getSelectedItem("ClassLimit", &lbi);
      par.filterMaxPer = lbi.data;

      oe->generateListInfo(par, gdi.getLineHeight(), currentList);
      generateList(gdi);
      gdi.refresh();
    }
     else if (bi.id=="ResultIndSplit") {
      oe->sanityCheck(gdi, true);
      oListParam par;
      ClassConfigInfo cnf;
      oe->getClassConfigurationInfo(cnf);
      cnf.getIndividual(par.selection);
      par.listCode = EStdResultList;
      par.showSplitTimes = true;
      par.legNumber = -1;
      par.pageBreak = gdi.isChecked("PageBreak");
      oe->generateListInfo(par, gdi.getLineHeight(), currentList);
      generateList(gdi);
      gdi.refresh();
    }
    else if (bi.id=="StartIndividual") {
      oe->sanityCheck(gdi, false);
      oListParam par;
      par.listCode = EStdStartList;
      par.legNumber = -1;
      par.pageBreak = gdi.isChecked("PageBreak");
      ClassConfigInfo cnf;
      oe->getClassConfigurationInfo(cnf);
      cnf.getIndividual(par.selection);
      oe->generateListInfo(par,  gdi.getLineHeight(), currentList);
      currentList.setCallback(openRunnerTeamCB);
      generateList(gdi);
      gdi.refresh();
    }
    else if (bi.id=="StartClub") {
      oe->sanityCheck(gdi, false);
      oListParam par;
      par.listCode = EStdClubStartList;
      par.pageBreak = gdi.isChecked("PageBreak");
      par.legNumber = -1;
      ClassConfigInfo cnf;
      oe->getClassConfigurationInfo(cnf);
      //cnf.getIndividual(par.selection);
      //cnf.getPatrol(par.selection);
      
     // oListInfo foo = currentList;
      oe->generateListInfo(par,  gdi.getLineHeight(), currentList);
      currentList.setCallback(openRunnerTeamCB);
      //currentList.addList(foo);
      generateList(gdi);
      gdi.refresh();
    }
    else if (bi.id=="ResultClub") {
      oe->sanityCheck(gdi, false);
      oListParam par;
      par.listCode = EStdClubResultList;
      par.pageBreak = gdi.isChecked("PageBreak");
      par.splitAnalysis = gdi.isChecked("SplitAnalysis");
      par.legNumber = -1;
      ClassConfigInfo cnf;
      oe->getClassConfigurationInfo(cnf);
      cnf.getIndividual(par.selection);
      cnf.getPatrol(par.selection);
      
      oe->generateListInfo(par,  gdi.getLineHeight(), currentList);
      currentList.setCallback(openRunnerTeamCB);
      generateList(gdi);
      gdi.refresh();
    }
    else if (bi.id=="PreReport") {
			SelectedList=bi.id;
			gdi.clearPage(false);
			oe->generatePreReport(gdi);

			gdi.addButton(gdi.getWidth()+20, 15, gdi.scaleLength(120),
                   "Cancel", "�terg�", ListsCB, "", true, false);
      gdi.addButton(gdi.getWidth()+20, 18+gdi.getButtonHeight(), gdi.scaleLength(120),
                    "Print", "Skriv ut...", ListsCB, "Skriv ut listan.", true, false);
      gdi.addButton(gdi.getWidth()+20, 21+2*gdi.getButtonHeight(), gdi.scaleLength(120),
                    "HTML", "Webb...",  ListsCB, "Spara f�r webben.", true, false);
      gdi.refresh();
		}
		else if (bi.id=="InForestList") {
			SelectedList=bi.id;
			gdi.clearPage(false);

			gdi.registerEvent("DataUpdate", ListsEventCB);
			gdi.setData("DataSync", 1);
			gdi.registerEvent(bi.id, ListsCB);

			oe->generateInForestList(gdi, openRunnerTeamCB, NoStartRunnerCB);
			gdi.addButton(gdi.getWidth()+20, 15, gdi.scaleLength(120), "Cancel", "�terg�", ListsCB,
                    "", true, false);
      gdi.addButton(gdi.getWidth()+20, 18+gdi.getButtonHeight(), gdi.scaleLength(120), 
                    "Print", "Skriv ut...", ListsCB, 
                    "Skriv ut listan.", true, false);
      gdi.addButton(gdi.getWidth()+20, 21+4*gdi.getButtonHeight(), gdi.scaleLength(120), 
                    "HTML", "Webb...", ListsCB, 
                    "Spara f�r webben.", true, false);
      gdi.refresh();
		}
		else if(bi.id=="TeamStartList")	{
      oe->sanityCheck(gdi, false);
      oListParam par;
      par.listCode = EStdTeamStartList;
      ClassConfigInfo cnf;
      oe->getClassConfigurationInfo(cnf);
      cnf.getRelay(par.selection);
      par.pageBreak = gdi.isChecked("PageBreak");
      oe->generateListInfo(par,  gdi.getLineHeight(), currentList);
      currentList.setCallback(openRunnerTeamCB);
      generateList(gdi);
      gdi.refresh();
		}
    else if(bi.id=="RaceNStart")	{
      oe->sanityCheck(gdi, false);
      oListParam par;
      int race = int(bi.getExtra());
      par.legNumber = race;
      par.listCode = EStdIndMultiStartListLeg;
      par.pageBreak = gdi.isChecked("PageBreak");
      ClassConfigInfo cnf;
      oe->getClassConfigurationInfo(cnf);
      cnf.getRaceNStart(race, par.selection);
      oe->generateListInfo(par,  gdi.getLineHeight(), currentList);
      currentList.setCallback(openRunnerTeamCB);
      generateList(gdi);
      gdi.refresh();
		}
    else if(bi.id=="LegNStart")	{
      oe->sanityCheck(gdi, false);
      oListParam par;
      par.pageBreak = gdi.isChecked("PageBreak");
      int race = int(bi.getExtra());
      par.legNumber = race;
      par.listCode = EStdTeamStartListLeg;
      ClassConfigInfo cnf;
      oe->getClassConfigurationInfo(cnf);
      cnf.getLegNStart(race, par.selection);
      oe->generateListInfo(par,  gdi.getLineHeight(), currentList);
      currentList.setCallback(openRunnerTeamCB);
      generateList(gdi);
      gdi.refresh();
		}
    else if(bi.id=="PatrolStartList")	{
      oe->sanityCheck(gdi, false);
      oListParam par;
      par.pageBreak = gdi.isChecked("PageBreak");
      par.listCode = EStdPatrolStartList;
      ClassConfigInfo cnf;
      oe->getClassConfigurationInfo(cnf);
      cnf.getPatrol(par.selection);
      oe->generateListInfo(par,  gdi.getLineHeight(), currentList);
      currentList.setCallback(openRunnerTeamCB);
      generateList(gdi);
      gdi.refresh();
		}
		else if(bi.id=="TeamResults")	{
      oe->sanityCheck(gdi, true);
      oListParam par;
      par.pageBreak = gdi.isChecked("PageBreak");
      par.splitAnalysis = gdi.isChecked("SplitAnalysis");
      par.listCode = EStdTeamResultListAll;
      
      ListBoxInfo lbi;
      gdi.getSelectedItem("ClassLimit", &lbi);
      par.filterMaxPer = lbi.data;
      
      ClassConfigInfo cnf;
      oe->getClassConfigurationInfo(cnf);
      cnf.getRelay(par.selection);
      oe->generateListInfo(par,  gdi.getLineHeight(), currentList);
      generateList(gdi);
      gdi.refresh();
		}
    else if(bi.id=="MultiResults")	{
      oe->sanityCheck(gdi, true);
      oListParam par;
      par.pageBreak = gdi.isChecked("PageBreak");
      par.splitAnalysis = gdi.isChecked("SplitAnalysis");
      par.listCode = EStdIndMultiResultListAll;
      ClassConfigInfo cnf;
      oe->getClassConfigurationInfo(cnf);
      cnf.getRaceNRes(0, par.selection);
      oe->generateListInfo(par,  gdi.getLineHeight(), currentList);
      generateList(gdi);
      gdi.refresh();
		}
    else if(bi.id=="RaceNRes")	{
      oe->sanityCheck(gdi, true);
      oListParam par;
      par.pageBreak = gdi.isChecked("PageBreak");
      par.splitAnalysis = gdi.isChecked("SplitAnalysis");
      int race = int(bi.getExtra());
      par.legNumber = race;
      par.listCode = EStdIndMultiResultListLeg;
      ClassConfigInfo cnf;
      oe->getClassConfigurationInfo(cnf);
      cnf.getRaceNRes(race, par.selection);
      oe->generateListInfo(par,  gdi.getLineHeight(), currentList);
      currentList.setCallback(openRunnerTeamCB);
      generateList(gdi);
      gdi.refresh();
		}
    else if(bi.id=="LegNResult")	{
      oe->sanityCheck(gdi, true);
      oListParam par;
      par.pageBreak = gdi.isChecked("PageBreak");
      par.splitAnalysis = gdi.isChecked("SplitAnalysis");
      int race = int(bi.getExtra());
      par.legNumber = race;
      ListBoxInfo lbi;
      gdi.getSelectedItem("ClassLimit", &lbi);
      par.filterMaxPer = lbi.data;
      par.listCode = EStdTeamResultListLeg;
      ClassConfigInfo cnf;
      oe->getClassConfigurationInfo(cnf);
      cnf.getLegNRes(race, par.selection);
      oe->generateListInfo(par,  gdi.getLineHeight(), currentList);
      currentList.setCallback(openRunnerTeamCB);
      generateList(gdi);
      gdi.refresh();
		}
    else if(bi.id=="PatrolResultList")	{
      oe->sanityCheck(gdi, false);
      oListParam par;
      par.pageBreak = gdi.isChecked("PageBreak");
      par.splitAnalysis = gdi.isChecked("SplitAnalysis");
      par.listCode = EStdPatrolResultList;
      ClassConfigInfo cnf;
      oe->getClassConfigurationInfo(cnf);
      cnf.getPatrol(par.selection);
      
      ListBoxInfo lbi;
      gdi.getSelectedItem("ClassLimit", &lbi);
      par.filterMaxPer = lbi.data;

      oe->generateListInfo(par,  gdi.getLineHeight(), currentList);
      currentList.setCallback(openRunnerTeamCB);
      generateList(gdi);
      gdi.refresh();
		}
    else if(bi.id=="RogainingResultList")	{
      oe->sanityCheck(gdi, true);
      oListParam par;
      par.pageBreak = gdi.isChecked("PageBreak");
      par.splitAnalysis = gdi.isChecked("SplitAnalysis");
      par.listCode = ERogainingInd;
      ClassConfigInfo cnf;
      oe->getClassConfigurationInfo(cnf);
      cnf.getRogaining(par.selection);
      
      ListBoxInfo lbi;
      gdi.getSelectedItem("ClassLimit", &lbi);
      par.filterMaxPer = lbi.data;

      oe->generateListInfo(par,  gdi.getLineHeight(), currentList);
      currentList.setCallback(openRunnerTeamCB);
      generateList(gdi);
      gdi.refresh();
		}
    else if(bi.id=="CourseReportTeam")	{
      oe->sanityCheck(gdi, false);
     
      ClassConfigInfo cnf;
      oe->getClassConfigurationInfo(cnf);
      oListParam par;
      par.pageBreak = gdi.isChecked("PageBreak");
      par.listCode = ETeamCourseList;
      cnf.getTeamClass(par.selection);
      oe->generateListInfo(par,  gdi.getLineHeight(), currentList);
      generateList(gdi);
      gdi.refresh();
		}
		else if(bi.id=="CourseReportInd")	{
      oe->sanityCheck(gdi, false);
     
      ClassConfigInfo cnf;
      oe->getClassConfigurationInfo(cnf);
      oListParam par;
      par.pageBreak = gdi.isChecked("PageBreak");
      par.listCode = EIndCourseList;
      cnf.getIndividual(par.selection);
      oe->generateListInfo(par,  gdi.getLineHeight(), currentList);
      generateList(gdi);
      gdi.refresh();
		}
    else if(bi.id=="RentedCards")	{
      ClassConfigInfo cnf;
      oe->getClassConfigurationInfo(cnf);
      oListParam par;
      par.listCode = EStdRentedCard;
      oe->generateListInfo(par,  gdi.getLineHeight(), currentList);
      generateList(gdi);
      gdi.refresh();
		}
    else if(bi.id=="PriceList")	{
      ClassConfigInfo cnf;
      oe->getClassConfigurationInfo(cnf);
      oListParam par;
      cnf.getIndividual(par.selection);
      par.listCode = EIndPriceList;
      ListBoxInfo lbi;
      gdi.getSelectedItem("ClassLimit", &lbi);
      par.filterMaxPer = lbi.data;
      oe->generateListInfo(par,  gdi.getLineHeight(), currentList);
      generateList(gdi);
      gdi.refresh();
		}
    else if(bi.id=="MinuteStartList") {
      oe->sanityCheck(gdi, false);
			SelectedList=bi.id;
			gdi.clearPage(false);

			gdi.registerEvent("DataUpdate", ListsEventCB);
			gdi.setData("DataSync", 1);
			gdi.registerEvent(bi.id, ListsCB);
	
      oe->generateMinuteStartlist(gdi);
			gdi.addButton(gdi.getWidth()+20, 15, gdi.scaleLength(120), "Cancel", "�terg�",
                    ListsCB, "", true, false);
      gdi.addButton(gdi.getWidth()+20, 18+gdi.getButtonHeight(), gdi.scaleLength(120), "HTML", "Webb...", 
                    ListsCB, "Spara f�r webben", true, false);
      gdi.addButton(gdi.getWidth()+20, 21+2*gdi.getButtonHeight(), gdi.scaleLength(120),
                    "Print", "Skriv ut...", 
                    ListsCB,  "Skriv ut listan", true, false);			
      gdi.refresh();
		}
		else if(bi.id=="ResultList") {
      settingsResultList(gdi);
		}
		else if(bi.id=="ResultListFT") {
      oe->sanityCheck(gdi, true);
			SelectedList=bi.id;
			gdi.clearPage(false);

			gdi.registerEvent("DataUpdate", ListsEventCB);
			gdi.setData("DataSync", 1);
			gdi.registerEvent(bi.id, ListsCB);

			oe->generateResultlistFinishTime(gdi, false,  0);

			gdi.addButton(gdi.getWidth()+20, 15, gdi.scaleLength(120), "Cancel", "�terg�", 
                    ListsCB, "", true, false);
      gdi.addButton(gdi.getWidth()+20, 18+gdi.getButtonHeight(), gdi.scaleLength(120), "ResultListFT_HTML", "Webb...", 
                    ListsCB, "", true, false);
      gdi.addButton(gdi.getWidth()+20, 21+2*gdi.getButtonHeight(), gdi.scaleLength(120), "Print", "Skriv ut...", 
                    ListsCB, "Skriv ut listan", true, false);
      gdi.addButton(gdi.getWidth()+20, 20+4*gdi.getButtonHeight(), gdi.scaleLength(120), "HTML", "Webb...", 
                    ListsCB,  "Spara f�r webben", true, false);
      gdi.refresh();
		}
		else if (bi.id=="ResultListCFT")	{
      oe->sanityCheck(gdi, true);
			SelectedList=bi.id;
			gdi.clearPage(false);

			gdi.registerEvent("DataUpdate", ListsEventCB);
			gdi.setData("DataSync", 1);
			gdi.registerEvent(bi.id, ListsCB);

			oe->generateResultlistFinishTime(gdi, true,  0);
			gdi.addButton(gdi.getWidth()+20, 15, gdi.scaleLength(120), "Cancel", "�terg�", 
                    ListsCB, "", true, false);
      gdi.addButton(gdi.getWidth()+20, 18+gdi.getButtonHeight(), gdi.scaleLength(120), "ResultListCFT_HTML", "Webb...", 
                    ListsCB, "", true, false);			
      gdi.addButton(gdi.getWidth()+20, 21+2*gdi.getButtonHeight(), gdi.scaleLength(120), "ResultListFT_ONATT", "Dataexport...", 
                    ListsCB, "Exportformat f�r OK Linn�s onsdagsnatt", true, false);			
      gdi.addButton(gdi.getWidth()+20, 24+3*gdi.getButtonHeight(), gdi.scaleLength(120), "Print", "Skriv ut...", 
                    ListsCB, "Skriv ut listan.", true, false);
      gdi.refresh();
		}
    else if (bi.id.substr(0, 7) == "Result:") {
      oe->sanityCheck(gdi, true);
      oListParam par;
      par.listCode = oe->getListContainer().getType(bi.id.substr(7));
      par.pageBreak = gdi.isChecked("PageBreak");
      par.splitAnalysis = gdi.isChecked("SplitAnalysis");
      par.legNumber = -1;
      ClassConfigInfo cnf;
      oe->getClassConfigurationInfo(cnf);
      cnf.getIndividual(par.selection);
      cnf.getPatrol(par.selection);
      oe->generateListInfo(par,  gdi.getLineHeight(), currentList);
      currentList.setCallback(openRunnerTeamCB);
      generateList(gdi);
      gdi.refresh();
    }
    else if (bi.id == "CustomList") {
      oe->sanityCheck(gdi, false);
      oListParam par;
      int index = bi.getExtraInt();
      par.listCode = oe->getListContainer().getType(index);
      par.pageBreak = gdi.isChecked("PageBreak");
      par.splitAnalysis = gdi.isChecked("SplitAnalysis");
      par.legNumber = -1;
      ClassConfigInfo cnf;
      oe->getClassConfigurationInfo(cnf);
      
      oListInfo::EBaseType type = oe->getListContainer().getList(index).getListType();
      if (oListInfo::addRunners(type))
        cnf.getIndividual(par.selection);
      if (oListInfo::addPatrols(type))
        cnf.getPatrol(par.selection);
      if (oListInfo::addTeams(type))
        cnf.getTeamClass(par.selection);

      oe->generateListInfo(par,  gdi.getLineHeight(), currentList);
      currentList.setCallback(openRunnerTeamCB);
      generateList(gdi);
      gdi.refresh();
    }
		else if (bi.id == "ResultListFT_ONATT") {
      vector< pair<string, string> > ext;
      ext.push_back(make_pair("Semikolonseparerad", "*.csv;*.txt"));
			string file=gdi.browseForSave(ext, "txt", index);
      throw std::exception("Not implemented");
		}
    else if (bi.id == "SplitAnalysis") {
      oe->setProperty("splitanalysis", gdi.isChecked(bi.id));
    }
    else if (bi.id == "PageBreak") {
      oe->setProperty("pagebreak", gdi.isChecked(bi.id));
    }
    else if (bi.id == "ShowInterResults"){
      oe->setProperty("intertime", gdi.isChecked(bi.id));
    }
  }
  else if (type==GUI_LISTBOX) {
		ListBoxInfo lbi=*(ListBoxInfo *)data;

		if(lbi.id=="ListType"){
      EStdListType type = EStdListType(lbi.data);
      selectGeneralList(gdi, type);
    }
    else if (lbi.id == "ListSelection") {
      gdi.getSelection(lbi.id, lastClassSelection);
    }
    else if (lbi.id == "ClassLimit"){
      oe->setProperty("classlimit", lbi.data);
    }
    else if (lbi.id=="ResultType") {

      if(lbi.data==1 || lbi.data==6) {
        gdi.enableInput("ResultSpecialFrom");
        oe->fillControls(gdi, "ResultSpecialFrom", 1);
        gdi.addItem("ResultSpecialFrom", lang.tl("Start"), 0);
        gdi.selectItemByData("ResultSpecialFrom", oe->getPropertyInt("ControlFrom", 0));

        gdi.enableInput("ResultSpecialTo");
        oe->fillControls(gdi, "ResultSpecialTo", 1);
        gdi.addItem("ResultSpecialTo", lang.tl("M�l"), 0);
        gdi.selectItemByData("ResultSpecialTo", oe->getPropertyInt("ControlTo", 0));
      }
      else {
        gdi.clearList("ResultsSpecialTo");
        gdi.disableInput("ResultSpecialTo");

        gdi.clearList("ResultsSpecialFrom");
        gdi.disableInput("ResultSpecialFrom");
      }

      if (lbi.data==6) {
        gdi.enableInput("UseLargeSize");
      }
      else {
        gdi.disableInput("UseLargeSize");
        gdi.check("UseLargeSize", false);
      }

      gdi.enableInput("LegNumber");
      oe->fillLegNumbers(gdi, "LegNumber");

      oe->fillClasses(gdi, "ListSelection", oEvent::extraNone, oEvent::filterNone);
      set<int> lst;
      lst.insert(-1);
      gdi.setSelection("ListSelection", lst);
      gdi.enableInput("Generate");
		}
    else if(lbi.id=="ResultsSpecialTo") {
      oe->setProperty("ControlTo", lbi.data);
    }
    else if(lbi.id=="ResultsSpecialFrom") {
      oe->setProperty("ControlFrom", lbi.data);
    }
	}
  else if (type==GUI_CLEAR) {
    offsetY=gdi.GetOffsetY();
    offsetX=gdi.GetOffsetX();
    return true;
  }

	return 0;
}

void TabList::selectGeneralList(gdioutput &gdi, EStdListType type)
{
    oListInfo li;
    oe->getListType(type, li);
    oe->setProperty("ListType", type);
    if (li.supportClasses) {
      gdi.enableInput("ListSelection");
      oe->fillClasses(gdi, "ListSelection", oEvent::extraNone, oEvent::filterNone);
      //set<int> lst;
      //lst.insert(-1);
      if (lastClassSelection.empty())
        lastClassSelection.insert(-1);
      gdi.setSelection("ListSelection", lastClassSelection);
    }
    else {
      gdi.clearList("ListSelection");
      gdi.disableInput("ListSelection");
    }

    if (li.supportLegs) {
      gdi.enableInput("LegNumber");
      oe->fillLegNumbers(gdi, "LegNumber");
    }
    else {
      gdi.disableInput("LegNumber");
      gdi.clearList("LegNumber");
    }
}

void TabList::loadGeneralList(gdioutput &gdi)
{
  oe->sanityCheck(gdi, false);
  gdi.fillDown();
  gdi.clearPage(false);
  gdi.addString("", boldLarge, "Skapa generell lista");

  gdi.pushY();
  gdi.addSelection("ListType", 200, 300, ListsCB, "Lista");
  oe->fillListTypes(gdi, "ListType", 0);
    
  gdi.fillDown();
  gdi.addListBox("ListSelection", 200, 300, ListsCB, "Urval", "", true);

  gdi.pushX();
  gdi.fillRight();
  gdi.dropLine();

  gdi.addButton("SelectAll", "V�lj allt", ListsCB);
  gdi.addButton("SelectNone", "V�lj inget", ListsCB);
  
  gdi.popX();
//  int baseX = gdi.getCX();
  gdi.setCX(gdi.scaleLength(240)+30);
  gdi.pushX();
  gdi.popY();
  gdi.fillDown();
  gdi.addCheckbox("PageBreak", "Sidbrytning mellan klasser / klubbar", ListsCB, oe->getPropertyInt("pagebreak", 0)!=0);
  
  gdi.addCheckbox("ShowInterResults", "Visa mellantider", ListsCB, oe->getPropertyInt("intertime", 1)!=0, "Mellantider visas f�r namngivna kontroller.");
  gdi.addCheckbox("SplitAnalysis", "Med str�cktidsanalys", ListsCB, oe->getPropertyInt("splitanalysis", 1)!=0);
  gdi.addCheckbox("UseLargeSize", "Anv�nd stor font", 0, 0);

  gdi.addInput("ClassLimit", "", 5, 0, "Begr�nsa antal per klass:");  
  gdi.dropLine();

  gdi.addSelection("LegNumber", 140, 300, ListsCB, "Str�cka:");
  gdi.disableInput("LegNumber");

  if (oe->getPropertyInt("ListType", 0) > 0) {
    gdi.selectItemByData("ListType", oe->getPropertyInt("ListType", 0));
    selectGeneralList(gdi, EStdListType(oe->getPropertyInt("ListType", 0)));
  }

  gdi.dropLine(2);
  gdi.addInput("Title", "", 32, ListsCB, "Egen listrubrik:");

  gdi.dropLine();
  gdi.fillRight();
  gdi.addButton("Generate", "Generera", ListsCB);
  gdi.addButton("Cancel", "Avbryt", ListsCB);

  gdi.refresh();
}

void TabList::settingsResultList(gdioutput &gdi)
{
  oe->sanityCheck(gdi, true);
  gdi.fillDown();
  gdi.clearPage(false);
  gdi.addString("", boldLarge, MakeDash("Resultatlista - inst�llningar"));

  //gdi.addSelection("ListType", 200, 300, ListsCB, "Lista");
  //oe->fillListTypes(gdi, "ListType", 0);
  
  gdi.pushY();  
  gdi.fillDown();
  gdi.addListBox("ListSelection", 200, 300, 0, "Urval", "", true);
  
  gdi.dropLine(1.5);
  gdi.fillRight();
  gdi.addButton("SelectAll", "V�lj allt", ListsCB);
  gdi.addButton("SelectNone", "V�lj inget", ListsCB);

  gdi.popY();
  gdi.setCX(gdi.scaleLength(250));
  gdi.fillDown();
  gdi.addString("", 0, "Typ av lista");
  gdi.pushX();
  gdi.fillRight();
  int yp = gdi.getCY();
  gdi.addListBox("ResultType", 180, 160, ListsCB);
  gdi.addItem("ResultType", lang.tl("Individuell"), 1);
  gdi.addItem("ResultType", lang.tl("Patrull"), 2);
  gdi.addItem("ResultType", lang.tl("Stafett - total"), 3);
  gdi.addItem("ResultType", lang.tl("Stafett - sammanst�llning"), 4);
  gdi.addItem("ResultType", lang.tl("Stafett - str�cka"), 5);
  gdi.addItem("ResultType", lang.tl("Allm�nna resultat"), 6);
  gdi.fillDown();
  gdi.addCheckbox("PageBreak", "Sidbrytning mellan klasser", 0, 0);
  gdi.addCheckbox("ShowInterResults", "Visa mellantider", 0, 0, 
            "Mellantider visas f�r namngivna kontroller.");
  gdi.addCheckbox("ShowSplits", "Lista med str�cktider", 0, 0);
  gdi.addCheckbox("UseLargeSize", "Anv�nd stor font", 0, 0);

  gdi.fillRight();
  gdi.setCY(max(gdi.getCY(), yp + gdi.scaleLength(170)));
  gdi.popX();
  gdi.addString("", 0, "Topplista, N b�sta:");
  gdi.dropLine(-0.2);
  gdi.addInput("ClassLimit", "", 5, 0);  
  gdi.popX();
  gdi.dropLine(2);
  gdi.addSelection("ResultSpecialFrom", 140, 300, ListsCB, "Fr�n kontroll:");
  gdi.disableInput("ResultSpecialFrom");

  gdi.addSelection("ResultSpecialTo", 140, 300, ListsCB, "Till kontroll:");
  gdi.disableInput("ResultSpecialTo");

  gdi.popX();
  gdi.dropLine(3);
  gdi.addSelection("LegNumber", 140, 300, ListsCB, "Str�cka:");
  gdi.disableInput("LegNumber");
  gdi.popX();

  gdi.dropLine(3);
  gdi.addInput("Title", "", 32, ListsCB, "Egen listrubrik:");
  gdi.popX();

  gdi.dropLine(3);
  gdi.fillRight();
  gdi.addButton("Generate", "Skapa listan", ListsCB);
  gdi.disableInput("Generate");
  gdi.addButton("Cancel", "Avbryt", ListsCB);

  gdi.refresh();
}

void checkWidth(gdioutput &gdi) {
  int h,w;
  gdi.getTargetDimension(w, h);
  w = max (w, 300);
  if (gdi.getCX() + 80 > w) {
    gdi.popX();
    gdi.dropLine(2.5);
  }
}

bool TabList::loadPage(gdioutput &gdi)
{
  oe->checkDB();
  oe->synchronize();
	gdi.selectTab(tabId);

	gdi.clearPage(false);

  if (SelectedList!="") {
    ButtonInfo bi;
    bi.id=SelectedList;
    ListsCB(&gdi, GUI_BUTTON, &bi);
    //gdi.SendCtrlMessage(SelectedList);
    gdi.setOffset(offsetX, offsetY, false);
    return 0;
  }

  // Make sure all lists are loaded
  oListInfo li;
  oe->getListType(EStdNone, li);
    
  gdi.addString("", boldLarge, "Listor och sammanst�llningar");

  gdi.addString("", 10, "help:30750");

  ClassConfigInfo cnf;
  oe->getClassConfigurationInfo(cnf);

  if (!cnf.empty()) {
    gdi.dropLine(1);
    gdi.addString("", 1, "Startlistor");
	  gdi.fillRight();
	  gdi.pushX();
    if (cnf.hasIndividual()) { 
	    gdi.addButton("StartIndividual", "Individuell", ListsCB);
      checkWidth(gdi);
      gdi.addButton("StartClub", "Klubbstartlista", ListsCB);
    }

    for (size_t k = 0; k<cnf.raceNStart.size(); k++) {
      if (cnf.raceNStart[k].size() > 0) {
        checkWidth(gdi);
        gdi.addButton("RaceNStart", "Lopp X#" + itos(k+1), ListsCB, 
                      "Startlista ett visst lopp.").setExtra(LPVOID(k));
      }
    }
    if (cnf.hasRelay()) {
      checkWidth(gdi);
      gdi.addButton("TeamStartList", "Stafett (sammanst�llning)", ListsCB);
    }
    if (cnf.hasPatrol()) {
      checkWidth(gdi);
      gdi.addButton("PatrolStartList", "Patrull", ListsCB);
    }
    for (size_t k = 0; k<cnf.legNStart.size(); k++) {
      if (cnf.legNStart[k].size() > 0) {
        checkWidth(gdi);
        gdi.addButton("LegNStart", "Str�cka X#" + itos(k+1), ListsCB).setExtra(LPVOID(k));
      }
    }

    checkWidth(gdi);
	  gdi.addButton("MinuteStartList", "Minutstartlista", ListsCB);

    gdi.dropLine(3);
    gdi.fillDown();
    gdi.popX();
    gdi.addString("", 1, "Resultatlistor");
    gdi.fillRight();

    if (cnf.hasIndividual()) {
	    gdi.addButton("ResultIndividual", "Individuell", ListsCB);
      checkWidth(gdi);
      gdi.addButton("ResultClub", "Klubbresultat", ListsCB);
      checkWidth(gdi);
      gdi.addButton("ResultIndSplit", "Str�cktider", ListsCB);

      if (cnf.isMultiStageEvent()) {
        checkWidth(gdi);
        gdi.addButton("Result:stageresult", "Etappresultat", ListsCB);

        checkWidth(gdi);
        gdi.addButton("Result:finalresult", "Slutresultat", ListsCB);
      }
    }
    bool hasMulti = false;
    for (size_t k = 0; k<cnf.raceNStart.size(); k++) {
      if (cnf.raceNRes[k].size() > 0) {
        checkWidth(gdi);
        gdi.addButton("RaceNRes", "Lopp X#" + itos(k+1), ListsCB, 
                      "Resultat f�r ett visst lopp.").setExtra(LPVOID(k));
        hasMulti = true;
      }
    }

    if (hasMulti) {
      checkWidth(gdi);
      gdi.addButton("MultiResults", "Alla lopp", ListsCB, "Individuell resultatlista, sammanst�llning av flera lopp.");
    }
    if (cnf.hasRelay()) {
      checkWidth(gdi);
      gdi.addButton("TeamResults", "Stafett (sammanst�llning)", ListsCB);
    }
    if (cnf.hasPatrol()) {
      checkWidth(gdi);
      gdi.addButton("PatrolResultList", "Patrull", ListsCB);
    }
    for (size_t k = 0; k<cnf.legNStart.size(); k++) {
      if (cnf.legNRes[k].size() > 0) {
        checkWidth(gdi);
        gdi.addButton("LegNResult", "Str�cka X#" + itos(k+1), ListsCB).setExtra(LPVOID(k));
      }
    }

    if (cnf.hasRogaining()) {
      checkWidth(gdi);
      gdi.addButton("RogainingResultList", "Rogaining", ListsCB);
    }

	  gdi.addButton("ResultList", "Avancerat...", ListsCB);

	  gdi.fillDown();	
    gdi.popX();
    gdi.dropLine(3);
  }

  MetaListContainer &lc = oe->getListContainer();
  int numExt = lc.getNumLists(MetaListContainer::ExternalList); 
  if (numExt) {
    gdi.addString("", 1, "Egna listor");

	  gdi.fillRight();
	  gdi.pushX();

    for (int k = 0; k < lc.getNumLists(); k++) {
      if (lc.isExternal(k)) {
        MetaList &mc = lc.getList(k);
        checkWidth(gdi);
        gdi.addButton("CustomList", mc.getListName(), ListsCB).setExtra(k);
      }
    }
    gdi.popX();
    gdi.dropLine(3);
    gdi.fillDown();	
  }

  vector< pair<string, size_t> > savedParams;
  lc.getListParam(savedParams);
  if (savedParams.size() > 0) {
    gdi.addString("", 1, "Sparade listval");
	  gdi.fillRight();
	  gdi.pushX();

    gdi.addSelection("SavedInstance", 250, 200, 0);
    gdi.addItem("SavedInstance", savedParams);
    gdi.selectFirstItem("SavedInstance");
    gdi.addButton("ShowSaved", "Visa", ListsCB);
    gdi.addButton("RenameSaved", "D�p om", ListsCB);
    gdi.addButton("RemoveSaved", "Ta bort", ListsCB);
    gdi.popX();
    gdi.dropLine(3);
    gdi.fillDown();	
  }

  gdi.addString("", 1, "Rapporter");

	gdi.fillRight();
	gdi.pushX();

  gdi.addButton("InForestList", "Kvar-i-skogen", ListsCB, "tooltip:inforest");
	if (cnf.hasIndividual()) {
    gdi.addButton("PriceList", "Prisutdelningslista", ListsCB);
  }
  gdi.addButton("PreReport", "K�r kontroll inf�r t�vlingen...", ListsCB);
  if (cnf.hasMultiCourse && cnf.hasTeamClass()) {
    gdi.addButton("CourseReportTeam", "Bantilldelning", ListsCB);
  }
  if (cnf.hasMultiCourse && cnf.hasIndividual()) {
    gdi.addButton("CourseReportInd", "Bantilldelning", ListsCB);
  }
  if (cnf.hasRentedCard)
    gdi.addButton("RentedCards", "Hyrbricksrapport", ListsCB);

  gdi.popX();
	
  gdi.dropLine(3);
  gdi.addCheckbox("PageBreak", "Sidbrytning mellan klasser / klubbar", ListsCB, oe->getPropertyInt("pagebreak", 0)!=0);
  gdi.addCheckbox("SplitAnalysis", "Med str�cktidsanalys", ListsCB, oe->getPropertyInt("splitanalysis", 1)!=0);

  gdi.popX();
	gdi.fillRight();
  gdi.dropLine(2);
  gdi.addString("", 0, "Begr�nsning, antal visade per klass: ");
  gdi.dropLine(-0.2);
  gdi.addSelection("ClassLimit", 70, 350, ListsCB);
  gdi.addItem("ClassLimit", lang.tl("Ingen"), 0);
  for (int k = 1; k<=12+9; k++) {
    int v = k;
    if (v>12)
      v=(v-11)*10;
    gdi.addItem("ClassLimit", itos(v), v);
  }
  gdi.selectItemByData("ClassLimit", oe->getPropertyInt("classlimit", 0));
 
  gdi.popX();
	
	gdi.dropLine(3);

  gdi.addButton("GeneralList", "Alla listor...", ListsCB);
  gdi.addButton("EditList", "Redigera lista", ListsCB);

	//gdi.registerEvent("DataUpdate", ListsEventCB);	
	gdi.refresh();

  gdi.setOnClearCb(ListsCB);
    
  offsetY=0;
  offsetX=0;
	gdi.refresh();
  return true;
}
