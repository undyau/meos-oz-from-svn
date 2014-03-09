/************************************************************************
    MeOS - Orienteering Software
    Copyright (C) 2009-2014 Melin Software HB
    
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

#include <algorithm>

#include "listeditor.h"
#include "metalist.h"
#include "gdioutput.h"
#include "meosexception.h"
#include "gdistructures.h"
#include "meos_util.h"
#include "localizer.h"
#include "gdifonts.h"
#include "oEvent.h"
#include "tabbase.h"
#include "CommDlg.h"

ListEditor::ListEditor(oEvent *oe_) {
  oe = oe_;
  currentList = 0;
  currentIndex = -1;
  dirtyExt = false;
  dirtyInt = false;
  lastSaved = NotSaved;
}

ListEditor::~ListEditor() {
  setCurrentList(0);
}

void ListEditor::setCurrentList(MetaList *lst) {
  delete currentList;
  currentList = lst;
}
/*
void ListEditor::load(MetaList *list) {
  currentList = list;
  currentIndex = -1;
  dirtyInt = true;
  dirtyExt = true;
}*/

void ListEditor::load(const MetaListContainer &mlc, int index) {
  const MetaList &mc = mlc.getList(index);
  setCurrentList(new MetaList());
  if (mlc.isInternal(index))
    currentIndex = -1;
  else
    currentIndex = index;
  *currentList = mc;
  dirtyExt = true;
  dirtyInt = false;
  savedFileName.clear();
}

void ListEditor::show(gdioutput &gdi) {
  
  gdi.setRestorePoint("BeginListEdit");

  gdi.pushX();
  gdi.setCX(gdi.getCX() + gdi.scaleLength(6));

  int bx = gdi.getCX();
  int by = gdi.getCY();

  if (currentList)
    gdi.addString("", boldLarge, MakeDash("Listredigerare - X#") + currentList->getListName());
  else
    gdi.addString("", boldLarge, "Listredigerare");

  gdi.setOnClearCb(editListCB);
  gdi.setData("ListEditorClz", this);
  gdi.dropLine(0.5);
  gdi.fillRight();
  
  gdi.addButton("EditList", "Egenskaper", editListCB);
  gdi.setCX(gdi.getCX() + gdi.scaleLength(32));
  gdi.addButton("OpenFile", "Öppna fil", editListCB);
  gdi.addButton("OpenInside", "Öppna från aktuell tävling", editListCB);
  
  if (savedFileName.empty())
    gdi.addButton("SaveFile", "Spara som fil", editListCB);
  else {
    gdi.addButton("SaveFile", "Spara fil", editListCB, "#" + savedFileName);
    gdi.addButton("SaveFileCopy", "Spara som...", editListCB);
  }

  gdi.addButton("SaveInside", "Spara i aktuell tävling", editListCB);
  gdi.addButton("NewList", "Ny lista", editListCB);
  gdi.addButton("RemoveInside", "Radera", editListCB, "Radera listan från aktuell tävling");
  gdi.setInputStatus("RemoveInside", currentIndex != -1);
  gdi.addButton("Close", "Stäng", editListCB);
  
  gdi.dropLine(2);

  int dx = gdi.getCX();
  int dy = gdi.getCY();

  RECT rc;
  int off = gdi.scaleLength(6);
  rc.left = bx - 2 * off;
  rc.right = dx + 2 * off;

  rc.top = by - off;
  rc.bottom = dy + off;
  
  gdi.addRectangle(rc, colorWindowBar);
  
  gdi.dropLine();
  gdi.popX();


  makeDirty(gdi, NoTouch, NoTouch);
  if (!currentList) {
    gdi.disableInput("EditList");
    gdi.disableInput("SaveFile");
    gdi.disableInput("SaveFileCopy", true);
    gdi.disableInput("SaveInside");
    gdi.refresh();
    return;
  }

  MetaList &list = *currentList;

  const vector< vector<MetaListPost> > &head = list.getHead();
  gdi.fillDown();
  gdi.addString("", 1, "Rubrik");
  gdi.pushX();
  gdi.fillRight();
  const double buttonDrop = 2.2;
  int lineIx = 100;
  for (size_t k = 0; k < head.size(); k++) {
    showLine(gdi, head[k], lineIx++);
    gdi.popX();
    gdi.dropLine(buttonDrop);
  }
  gdi.fillDown();
  gdi.addButton("AddLine0", "Lägg till rad", editListCB);

  gdi.dropLine(0.5);
  gdi.addString("", 1, "Underrubrik");
  gdi.pushX();
  gdi.fillRight();
  const vector< vector<MetaListPost> > &subHead = list.getSubHead();
  lineIx = 200;
  for (size_t k = 0; k < subHead.size(); k++) {
    showLine(gdi, subHead[k], lineIx++);
    gdi.popX();
    gdi.dropLine(buttonDrop);
  }
  gdi.fillDown();
  gdi.addButton("AddLine1", "Lägg till rad", editListCB);

  gdi.dropLine(0.5);
  gdi.addString("", 1, "Huvudlista");
  gdi.pushX();
  gdi.fillRight();
  const vector< vector<MetaListPost> > &mainList = list.getList();
  lineIx = 300;
  for (size_t k = 0; k < mainList.size(); k++) {
    showLine(gdi, mainList[k], lineIx++);
    gdi.popX();
    gdi.dropLine(buttonDrop);
  }
  gdi.fillDown();
  gdi.addButton("AddLine2", "Lägg till rad", editListCB);

  gdi.dropLine(0.5);
  gdi.addString("", 1, "Underlista");
  gdi.pushX();
  gdi.fillRight();
  const vector< vector<MetaListPost> > &subList = list.getSubList();
  lineIx = 400;
  for (size_t k = 0; k < subList.size(); k++) {
    showLine(gdi, subList[k], lineIx++);
    gdi.popX();
    gdi.dropLine(buttonDrop);
  }
  gdi.fillDown();
  gdi.addButton("AddLine3", "Lägg till rad", editListCB);

  gdi.setRestorePoint("EditList");

  gdi.dropLine(2);

  oListInfo li;
  oListParam par;
  par.pageBreak = false;
  par.splitAnalysis = true;
  par.legNumber = -1;
  gdi.fillDown();

  try {
    currentList->interpret(oe, gdi, par, gdi.getLineHeight(), li);
    RECT rc;
    rc.left = gdi.getCX();
    rc.right = gdi.getCX() + gdi.getWidth() - 20;
    rc.top = gdi.getCY();
    rc.bottom = rc.top + 4;

    gdi.addRectangle(rc, colorDarkGreen, false, false);
    gdi.dropLine();

    oe->generateList(gdi, false, li, true);
  }
  catch (std::exception &ex) {
    gdi.addString("", 1, "Listan kan inte visas").setColor(colorRed);
    gdi.addString("", 0, ex.what());
  }
       
  gdi.refresh();
}

int editListCB(gdioutput *gdi, int type, void *data)
{
  void *clz = gdi->getData("ListEditorClz");
  ListEditor *le = (ListEditor *)clz;
  BaseInfo *bi = (BaseInfo *)data;
  if (le)
    return le->editList(*gdi, type, *bi);

  throw meosException("Unexpected error");
}

void ListEditor::showLine(gdioutput &gdi, const vector<MetaListPost> &line, int ix) const {
  for (size_t k = 0; k < line.size(); k++) {
    addButton(gdi, line[k], gdi.getCX(), gdi.getCY(), ix, k);
  }

  gdi.addButton("AddPost" + itos(ix), "Lägg till ny", editListCB);
}

ButtonInfo &ListEditor::addButton(gdioutput &gdi, const MetaListPost &mlp, int x, int y, int lineIx, int ix) const {
  string cap;
  if (mlp.getType() == "String") {
    cap = "Text: X#" + mlp.getText();
  }
  else {
    const string &text = mlp.getText();
    if (text.length() > 0) {
      cap = text + "#" + lang.tl(mlp.getType());
    }
    else {
      cap = mlp.getType();
    }
  }

  ButtonInfo &bi = gdi.addButton(x, y, "EditPost" + itos(lineIx * 100 + ix), cap, editListCB);
  return bi;
}

static void getPosFromId(int id, int &groupIx, int &lineIx, int &ix) {
  lineIx = id / 100;
  ix = id % 100;
  groupIx = (lineIx / 100) - 1;
  lineIx = lineIx % 100;
}

int ListEditor::editList(gdioutput &gdi, int type, BaseInfo &data) {
  int lineIx, groupIx, ix;
  if (type == GUI_BUTTON) {
    ButtonInfo bi = dynamic_cast<ButtonInfo &>(data);
    ButtonInfo &biSrc = dynamic_cast<ButtonInfo &>(data);
    
    if (bi.id == "Color") {
      CHOOSECOLOR cc;
      memset(&cc, 0, sizeof(cc));
      cc.lStructSize = sizeof(cc);
      cc.hwndOwner = gdi.getHWND();
      cc.rgbResult = COLORREF(bi.getExtra());
      if (GDICOLOR((int)bi.getExtra()) != colorDefault) 
        cc.Flags |= CC_RGBINIT;

      COLORREF staticColor[16];
      memset(staticColor, 0, 16*sizeof(COLORREF));

      const string &c = oe->getPropertyString("Colors", "");
      const char *end = c.c_str() + c.length();
      const char * pEnd = c.c_str();
      int pix = 0;
      while(pEnd < end && pix < 16) {
        staticColor[pix++] = strtol(pEnd,(char **)&pEnd,16);
      }
 
      //vector<string> splitvector;
      //split(c, ";", splitvector);
      cc.lpCustColors = staticColor;
      if (ChooseColor(&cc)) {
        data.setExtra((int)cc.rgbResult);
      
        string co;
        for (ix = 0; ix < 16; ix++) {
          char bf[16];
          sprintf_s(bf, "%x ", staticColor[ix]);
          co += bf;
        }
        oe->setProperty("Colors", co);
      }
    }
    if ( bi.id.substr(0, 8) == "EditPost" ) {
      int id = atoi(bi.id.substr(8).c_str());
      getPosFromId(id, groupIx, lineIx, ix);
      MetaListPost &mlp = currentList->getMLP(groupIx, lineIx, ix);
      editListPost(gdi, mlp, id);
    }
    else if ( bi.id.substr(0, 7) == "AddPost" ) {
      checkUnsaved(gdi);
      gdi.restore("EditList", true);
      gdi.pushX();
      lineIx = atoi(bi.id.substr(7).c_str());
      groupIx = (lineIx / 100) - 1;
      int ixOutput = 0;
      MetaListPost &mlp = currentList->addNew(groupIx, lineIx % 100, ixOutput);
      int xp = bi.xp;
      int yp = bi.yp;
      ButtonInfo &nb = addButton(gdi, mlp, xp, yp, lineIx, ixOutput);
        //gdi.addButton(xp, yp, string("Foo"), string("FoooBar"), 0);
      int w, h;
      nb.getDimension(gdi, w, h);
      biSrc.moveButton(gdi, xp+w, yp);
      gdi.popX();
      gdi.setRestorePoint("EditList");
      makeDirty(gdi, MakeDirty, MakeDirty);
    }
    else if ( bi.id.substr(0, 7) == "AddLine" ) {
      checkUnsaved(gdi);
      groupIx = atoi(bi.id.substr(7).c_str());
      int ixOutput = 0;
      currentList->addNew(groupIx, -1, ixOutput);

      gdi.restore("BeginListEdit", false);
      makeDirty(gdi, MakeDirty, MakeDirty);
      show(gdi);      
    }
    else if ( bi.id == "Remove" ) {
      DWORD id;
      gdi.getData("CurrentId", id);
      getPosFromId(id, groupIx, lineIx, ix);
      currentList->removeMLP(groupIx, lineIx, ix);
      gdi.restore("BeginListEdit", false);
      makeDirty(gdi, MakeDirty, MakeDirty);
      show(gdi);
    }
    else if (bi.id == "UseLeg") {
      gdi.setInputStatus("Leg", gdi.isChecked(bi.id));
    }
    else if (bi.id == "Cancel") {
      gdi.restore("EditList");
      gdi.enableInput("EditList");
    }
    else if (bi.id == "CancelNew") {
      gdi.clearPage(false);
      currentList = 0;
      show(gdi);
    }
    else if (bi.id == "Apply" || bi.id == "MoveLeft" || bi.id == "MoveRight") {
      DWORD id;
      gdi.getData("CurrentId", id);
      getPosFromId(id, groupIx, lineIx, ix);

      if (bi.id == "MoveLeft")
        currentList->moveOnRow(groupIx, lineIx, ix, -1);
      else if (bi.id == "MoveRight")
        currentList->moveOnRow(groupIx, lineIx, ix, 1);

      MetaListPost &mlp = currentList->getMLP(groupIx, lineIx, ix);
      
      ListBoxInfo lbi;
      bool force = false;
      gdi.getSelectedItem("Type", &lbi);

      EPostType ptype = EPostType(lbi.data);

      string str = gdi.getText("Text");
      if (ptype != lString) {
        if (!str.empty() && str.find_first_of('X') == string::npos) {
          throw meosException("Texten ska innehålla tecknet X, som byts ut mot tävlingssspecifik data");
        }
      }

      string t1 = mlp.getType();
      mlp.setType(EPostType(lbi.data));
      if (t1 != mlp.getType())
        force = true;
      mlp.setText(str);

      gdi.getSelectedItem("AlignType", &lbi);
      mlp.align(EPostType(lbi.data), gdi.isChecked("BlockAlign"));
      mlp.alignText(gdi.getText("AlignText"));
      mlp.mergePrevious(gdi.isChecked("MergeText"));

      gdi.getSelectedItem("TextAdjust", &lbi);
      mlp.setTextAdjust(lbi.data);
      
      mlp.setColor(GDICOLOR((int)gdi.getExtra("Color")));

      if (gdi.isChecked("UseLeg")) {
        int leg = gdi.getTextNo("Leg");
        if (leg < 1 || leg > 1000)
          throw meosException("X är inget giltigt sträcknummer#" + itos(leg));
        mlp.setLeg(leg - 1);
      }
      else
        mlp.setLeg(-1);

      mlp.setBlock(gdi.getTextNo("BlockSize"));
      mlp.indent(gdi.getTextNo("MinIndeent"));
      
      gdi.getSelectedItem("Fonts", &lbi);
      mlp.setFont(gdiFonts(lbi.data));
      makeDirty(gdi, MakeDirty, MakeDirty);
      if (!gdi.hasData("NoRedraw") || force) {
        gdi.restore("BeginListEdit", false);
        show(gdi);
      }
    }
    else if (bi.id == "ApplyListProp") {
      string name = gdi.getText("Name");

      if (name.empty())
        throw meosException("Namnet kan inte vara tomt");

      MetaList &list = *currentList;
      list.setListName(name);
      ListBoxInfo lbi;

      if (gdi.getSelectedItem("SortOrder", &lbi))
        list.setSortOrder(SortOrder(lbi.data));

      if (gdi.getSelectedItem("BaseType", &lbi))
        list.setListType(oListInfo::EBaseType(lbi.data));

      if (gdi.getSelectedItem("SubType", &lbi))
        list.setSubListType(oListInfo::EBaseType(lbi.data));

      vector< pair<string, bool> > filtersIn;
      vector< bool > filtersOut;
      list.getFilters(filtersIn);
      for (size_t k = 0; k < filtersIn.size(); k++) 
        filtersOut.push_back(gdi.isChecked("filter" + itos(k)));

      list.setFilters(filtersOut);
  
      for (int k = 0; k < 4; k++) {
        list.setFontFace(k, gdi.getText("Font" + itos(k)),
                            gdi.getTextNo("FontFactor" + itos(k)));
      }
      makeDirty(gdi, MakeDirty, MakeDirty);

      if (!gdi.hasData("NoRedraw")) {
        gdi.clearPage(false);
        show(gdi);
      }
    }
    else if (bi.id == "EditList") {
      editListProp(gdi, false);
    }
    else if (bi.id == "NewList") {
      if (!checkSave(gdi))
        return 0;
      gdi.clearPage(false);
      gdi.setData("ListEditorClz", this);
      gdi.addString("", boldLarge, "Ny lista");

      setCurrentList(new MetaList());
      currentIndex = -1;
      lastSaved = NotSaved;
      makeDirty(gdi,  ClearDirty, ClearDirty);

      editListProp(gdi, true);
    }
    else if (bi.id == "MakeNewList") {
      /*
      currentList->setListName(lang.tl("Ny lista"));
      currentIndex = -1;
      
      lastSaved = NotSaved;
      makeDirty(gdi,  ClearDirty, ClearDirty);

      gdi.clearPage(false);
      show(gdi);*/
    }
    else if (bi.id == "SaveFile" || bi.id == "SaveFileCopy") {
      if (!currentList)
        return 0;

      bool copy = bi.id == "SaveFileCopy";

      string fileName = copy ? "" : savedFileName;

      if (fileName.empty()) {
        int ix = 0;
        vector< pair<string, string> > ext;
        ext.push_back(make_pair("xml-data", "*.xml"));
        fileName = gdi.browseForSave(ext, "xml", ix);
        if (fileName.empty())
          return 0;
      }
      
      currentList->save(fileName);
 
      lastSaved = SavedFile;
      makeDirty(gdi, NoTouch, ClearDirty);
      savedFileName = fileName;
      return 1;
    }
    else if (bi.id == "OpenFile") {
      if (!checkSave(gdi))
        return 0;

      vector< pair<string, string> > ext;
      ext.push_back(make_pair("xml-data", "*.xml"));
      string fileName = gdi.browseForOpen(ext, "xml");
      if (fileName.empty())
        return 0;

      MetaList *tmp = new MetaList();
      try {
        tmp->setListName(lang.tl("Ny lista"));
        tmp->load(fileName);
      }
      catch(...) {
        delete tmp;
        throw;
      }

      setCurrentList(tmp);
      currentIndex = -1;
      gdi.clearPage(false);
      lastSaved = SavedFile;

      savedFileName = fileName;
      makeDirty(gdi, ClearDirty, ClearDirty);
      show(gdi);
    }
    else if (bi.id == "OpenInside") {
      if (!checkSave(gdi))
        return 0;

      savedFileName.clear();
      gdi.clearPage(true);
      gdi.setOnClearCb(editListCB);
      gdi.setData("ListEditorClz", this);

      gdi.pushX();
      vector< pair<string, size_t> > lists;
      oe->getListContainer().getLists(lists);
      
      gdi.fillRight();
      gdi.addSelection("OpenList", 250, 400, 0, "Välj lista:");
      gdi.addItem("OpenList", lists);
      gdi.selectFirstItem("OpenList");

      gdi.dropLine();
      gdi.addButton("DoOpen", "Öppna", editListCB);
      gdi.addButton("CancelReload", "Avbryt", editListCB);
      gdi.dropLine(4);
      gdi.popX();
    }
    else if (bi.id == "DoOpen") {
      ListBoxInfo lbi;
      if (gdi.getSelectedItem("OpenList", &lbi)) {
        load(oe->getListContainer(), lbi.data);
      }

      lastSaved = SavedInside;
      makeDirty(gdi, ClearDirty, ClearDirty);
      gdi.clearPage(false);
      show(gdi);
    }
    else if (bi.id == "CancelReload") {
      gdi.clearPage(false);
      show(gdi);
    }
    else if (bi.id == "SaveInside") {
      if (currentList == 0)
        return 0;
      savedFileName.clear();
      oe->synchronize(false);

      if (currentIndex != -1) {
        oe->getListContainer().saveList(currentIndex, *currentList);
      }
      else {
        oe->getListContainer().addExternal(*currentList);
        currentIndex = oe->getListContainer().getNumLists() - 1;
        oe->getListContainer().saveList(currentIndex, *currentList);
      }
      
      oe->synchronize(true); 
      lastSaved = SavedInside;
      makeDirty(gdi, ClearDirty, NoTouch);
    }
    else if (bi.id == "RemoveInside") {
      if (currentIndex != -1) {
        oe->getListContainer().removeList(currentIndex);
        currentIndex = -1;
        savedFileName.clear();
        if (lastSaved == SavedInside)
          lastSaved = NotSaved;

        gdi.alert("Listan togs bort från tävlingen.");
        makeDirty(gdi, MakeDirty, NoTouch);
        gdi.setInputStatus("RemoveInside", false);
      }
    }
    else if (bi.id == "Close") {
      if (!checkSave(gdi))
        return 0;

      setCurrentList(0);
      makeDirty(gdi, ClearDirty, ClearDirty);
      currentIndex = -1;
      savedFileName.clear();
      gdi.getTabs().get(TListTab)->loadPage(gdi);
      return 0;
    }
    /*else if (bi.id == "BrowseFont") {
      InitCommonControls();
      CHOOSEFONT cf;
      memset(&cf, 0, sizeof(cf));
      cf.lStructSize = sizeof(cf);
      cf.hwndOwner = gdi.getHWND();
      ChooseFont(&cf);
      EnumFontFamilies(
    }*/
  }
  else if (type == GUI_LISTBOX) {
    ListBoxInfo &lbi = dynamic_cast<ListBoxInfo &>(data);
    
    if (lbi.id == "AlignType")
      gdi.setInputStatus("AlignText", lbi.data == lString);
  }
  else if(type==GUI_CLEAR) {
    return checkSave(gdi);
  }
  
  return 0;
}

void ListEditor::checkUnsaved(gdioutput &gdi) {
 if (gdi.hasData("IsEditing")) {
    if (gdi.isInputChanged("")) {
      gdi.setData("NoRedraw", 1);
      gdi.sendCtrlMessage("Apply");
    }
  }
  if (gdi.hasData("IsEditingList")) {
    if (gdi.isInputChanged("")) {
      gdi.setData("NoRedraw", 1);
      gdi.sendCtrlMessage("ApplyListProp");
    }
  }
}

void ListEditor::editListPost(gdioutput &gdi, const MetaListPost &mlp, int id) {
  checkUnsaved(gdi);
  gdi.restore("EditList", false);
  gdi.dropLine();
  
  gdi.enableInput("EditList");
  int groupIx, lineIx, ix;
  getPosFromId(id, groupIx, lineIx, ix);

  int x1 = gdi.getCX();
  int y1 = gdi.getCY();
  int margin = gdi.scaleLength(10);
  gdi.setCX(x1+margin);
  
  gdi.dropLine();
  gdi.pushX();
  gdi.fillRight();
  gdi.addString("", boldLarge, "Listpost").setColor(colorDarkGrey);
  gdi.setCX(gdi.getCX() + gdi.scaleLength(20));
  
  gdi.addButton("MoveLeft", "<< Flytta vänster", editListCB);
  gdi.addButton("MoveRight", "Flytta höger >>", editListCB);
  
  gdi.dropLine(3);
  gdi.popX();
  vector< pair<string, size_t> > types;
  int currentType;
  mlp.getTypes(types, currentType);
  sort(types.begin(), types.end());
  gdi.pushX();
  gdi.fillRight();
  int boxY = gdi.getCY();
  gdi.addSelection("Type", 290, 500, 0, "Typ:");
  gdi.addItem("Type", types);
  gdi.selectItemByData("Type", currentType);
  gdi.addInput("Text", mlp.getText(), 16, 0, "Egen text:", "Använd symbolen X där MeOS ska fylla i typens data.");
  int boxX = gdi.getCX();
  gdi.popX();
  gdi.fillRight();
  gdi.dropLine(3);
  currentList->getAlignTypes(mlp, types, currentType);
  sort(types.begin(), types.end());
  gdi.addSelection("AlignType", 290, 500, editListCB, "Justera mot:");
  gdi.addItem("AlignType", types);
  gdi.selectItemByData("AlignType", currentType);

  gdi.addInput("AlignText", mlp.getAlignText(), 16, 0, "Text:");
  if (currentType != lString)
    gdi.disableInput("AlignText");
  gdi.popX();
  gdi.dropLine(3);
  gdi.fillRight();
  gdi.addCheckbox("BlockAlign", "Justera blockvis", 0, mlp.getAlignBlock());
  gdi.addInput("BlockSize", itos(mlp.getBlockWidth()), 5, 0, "", "Blockbredd");
  gdi.dropLine(2.1);
  gdi.popX();
  gdi.fillRight();
  int leg = mlp.getLeg();
  gdi.addCheckbox("UseLeg", "Applicera för specifik sträcka", editListCB, leg != -1);
  gdi.addInput("Leg", leg>=0 ? itos(leg + 1) : "", 4);
  gdi.dropLine(2);
  if (ix>0) {
    gdi.popX();
    gdi.addCheckbox("MergeText", "Slå ihop text med föregående", 0, mlp.isMergePrevious());
    gdi.dropLine(2);
  }
  int maxY = gdi.getCY();
  gdi.popX();
  gdi.fillDown();
  gdi.setCX(boxX + gdi.scaleLength(24));
  gdi.setCY(boxY);
  gdi.pushX();
  gdi.addString("", 1, "Formateringsregler");
  gdi.dropLine(0.5);
  gdi.fillRight();
  gdi.addInput("MinIndeent", itos(mlp.getMinimalIndent()), 7, 0, "Minsta intabbning:");

  vector< pair<string, size_t> > fonts;
  int currentFont;
  mlp.getFonts(fonts, currentFont);
  
  gdi.addSelection("Fonts", 150, 500, 0, "Format:");
  gdi.addItem("Fonts", fonts);
  gdi.selectItemByData("Fonts", currentFont);
  int maxX = gdi.getCX();
  
  gdi.popX();
  gdi.dropLine(3);

  gdi.addSelection("TextAdjust", 150, 100, 0, "Textjustering:");
  gdi.addItem("TextAdjust", lang.tl("Vänster"), 0);
  gdi.addItem("TextAdjust", lang.tl("Höger"), textRight);
  gdi.addItem("TextAdjust", lang.tl("Centrera"), textCenter);
  gdi.selectItemByData("TextAdjust", mlp.getTextAdjustNum());

  //gdi.popX();
  //gdi.dropLine(2);
  gdi.dropLine();
  gdi.addButton("Color", "Färg...", editListCB).setExtra(mlp.getColorValue());

  
  maxX = max(maxX, gdi.getCX());
  gdi.popX();
  gdi.dropLine(3);

  gdi.setData("CurrentId", id);
  gdi.addButton("Remove", "Radera", editListCB, "Ta bort listposten");
  gdi.addButton("Cancel", "Avbryt", editListCB).setCancel();
  
  gdi.updatePos(gdi.getCX(), gdi.getCY(), gdi.scaleLength(20), 0);
  gdi.addButton("Apply", "OK", editListCB).setDefault();

  gdi.dropLine(3);
  maxY = max(maxY, gdi.getCY());
  maxX = max(gdi.getCX(), maxX);

  gdi.fillDown();
  gdi.popX();
  gdi.setData("IsEditing", 1);
  
  RECT rc;
  rc.top = y1;
  rc.left = x1;
  rc.right = maxX + gdi.scaleLength(6);
  rc.bottom = maxY + gdi.scaleLength(6);

  gdi.addRectangle(rc, colorLightBlue, true);

  gdi.scrollToBottom();
  gdi.refresh();
}

void ListEditor::editListProp(gdioutput &gdi, bool newList) {
  checkUnsaved(gdi);
 
  if (!currentList)
    return;

  MetaList &list = *currentList;

  if (!newList) {
    gdi.restore("EditList", false);  
    gdi.disableInput("EditList");
  }

  gdi.dropLine(0.8);
  
  int x1 = gdi.getCX();
  int y1 = gdi.getCY();
  int margin = gdi.scaleLength(10);
  gdi.setCX(x1+margin);

  if (!newList) {
    gdi.dropLine();
    gdi.fillDown();
    gdi.addString("", boldLarge, "Listegenskaper").setColor(colorDarkGrey);
    gdi.dropLine();
  }

  gdi.fillRight();
  gdi.pushX();
  
  gdi.addInput("Name", list.getListName(), 20, 0, "Listnamn:");
  
  if (newList) {
    gdi.dropLine(3.5);
    gdi.popX();
  }

  vector< pair<string, size_t> > types;
  int currentType = 0;

  int maxX = gdi.getCX();
  
  list.getBaseType(types, currentType);
  gdi.addSelection("BaseType", 170, 400, 0, "Listtyp:");
  gdi.addItem("BaseType", types);
  gdi.selectItemByData("BaseType", currentType);

  list.getSortOrder(types, currentType);
  gdi.addSelection("SortOrder", 200, 400, 0, "Global sorteringsordning:");
  gdi.addItem("SortOrder", types);
  gdi.selectItemByData("SortOrder", currentType);

  list.getSubType(types, currentType);
  gdi.addSelection("SubType", 170, 400, 0, "Sekundär typ:");
  gdi.addItem("SubType", types);
  gdi.selectItemByData("SubType", currentType);

  maxX = max(maxX, gdi.getCX());
  gdi.popX();
  gdi.dropLine(3);
  gdi.fillDown();
  gdi.addString("", 1, "Filter");
  gdi.dropLine(0.5);
  vector< pair<string, bool> > filters;
  list.getFilters(filters);
  gdi.fillRight();
  int xp = gdi.getCX();
  int yp = gdi.getCY();
  const int w = gdi.scaleLength(130);
  for (size_t k = 0; k < filters.size(); k++) {
    gdi.addCheckbox(xp, yp, "filter" + itos(k), filters[k].first, 0, filters[k].second);
    xp += w;
    maxX = max(maxX, xp);
    if (k % 10 == 9) {
      xp = x1 + margin;
      yp += int(1.3 * gdi.getLineHeight());
    }
  }

  gdi.popX();
  gdi.dropLine(2);

  gdi.fillDown();
  gdi.addString("", 1, "Typsnitt");
  gdi.dropLine(0.5);
  gdi.fillRight();
  const char *expl[4] = {"Rubrik", "Underrubrik", "Lista", "Underlista"};
  vector< pair<string, size_t> > fonts;
  sort(fonts.begin(), fonts.end());
  gdi.getEnumeratedFonts(fonts);

  for (int k = 0; k < 4; k++) {
    string id("Font" + itos(k));
    gdi.addCombo(id, 200, 300, 0, expl[k]);
    gdi.addItem(id, fonts);
    
    gdi.setText(id, list.getFontFace(k));
    gdi.setCX(gdi.getCX()+20);
    int f = list.getFontFaceFactor(k);
    string ff = f == 0 ? "100 %" : itos(f) + " %";
    gdi.addInput("FontFactor" + itos(k), ff, 4, 0, "Skalfaktor", "Relativ skalfaktor för typsnittets storlek i procent");
    if (k == 1) {
      gdi.dropLine(3);
      gdi.popX();
    }
  }

  if (!newList) {
    gdi.dropLine(0.8);
    gdi.setCX(gdi.getCX()+20);
    gdi.addButton("ApplyListProp", "OK", editListCB);
    gdi.addButton("Cancel", "Avbryt", editListCB);
  }
  else {
    gdi.setCX(x1);
    gdi.setCY(gdi.getHeight());
    gdi.dropLine();
    gdi.addButton("ApplyListProp", "Skapa", editListCB);
    gdi.addButton("CancelNew", "Avbryt", editListCB);
  }

  gdi.dropLine(3);
  int maxY = gdi.getCY();

  gdi.fillDown();
  gdi.popX();
  gdi.setData("IsEditingList", 1);

  RECT rc;
  rc.top = y1;
  rc.left = x1;
  rc.right = maxX + gdi.scaleLength(6);
  rc.bottom = maxY;

  if (!newList) {
    gdi.addRectangle(rc, colorLightBlue, true);
  }

  gdi.scrollToBottom();
  gdi.refresh();
  gdi.setInputFocus("Name");
}

void ListEditor::makeDirty(gdioutput &gdi, DirtyFlag inside, DirtyFlag outside) {
  if (inside == MakeDirty)
    dirtyInt = true;
  else if (inside == ClearDirty)
    dirtyInt = false;

  if (outside == MakeDirty)
    dirtyExt = true;
  else if (outside == ClearDirty)
    dirtyExt = false;
    
  if (gdi.hasField("SaveInside")) {
    gdi.setInputStatus("SaveInside", dirtyInt || lastSaved != SavedInside);
  }

  if (gdi.hasField("SaveFile")) {
    gdi.setInputStatus("SaveFile", dirtyExt || lastSaved != SavedFile);
  }
}

bool ListEditor::checkSave(gdioutput &gdi) {
  if (dirtyInt || dirtyExt) {
    gdioutput::AskAnswer answer = gdi.askCancel("Vill du spara ändringar?");
    if (answer == gdioutput::AnswerCancel)
      return false;

    if (answer == gdioutput::AnswerYes) {
      if (currentIndex >= 0)
        gdi.sendCtrlMessage("SaveInside");
      else if (gdi.sendCtrlMessage("SaveFile") == 0)
        return false;
    }
    makeDirty(gdi, ClearDirty, ClearDirty);
  }

  return true;
}
