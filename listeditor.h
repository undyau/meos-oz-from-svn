#pragma once

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
    Stigbergsv�gen 7, SE-75242 UPPSALA, Sweden
    
************************************************************************/

class MetaList;
class MetaListPost;
class MetaListContainer;
class gdioutput;
class BaseInfo;
class ButtonInfo;
class oEvent;

#include <vector>

class ListEditor {
private:
  enum SaveType {NotSaved, SavedInside, SavedFile};
  oEvent *oe;
  MetaList *currentList;
  void setCurrentList(MetaList *lst);
  int currentIndex;
  string savedFileName;
  bool dirtyExt;
  bool dirtyInt;
  SaveType lastSaved;
  void showLine(gdioutput &gdi, const vector<MetaListPost> &line, int ix) const;
  int editList(gdioutput &gdi, int type, BaseInfo &data);
  ButtonInfo &addButton(gdioutput &gdi, const MetaListPost &mlp, int x, int y,
                       int lineIx, int ix) const;

  void editListPost(gdioutput &gdi, const MetaListPost &mlp, int id);
  void editListProp(gdioutput &gdi);

  enum DirtyFlag {MakeDirty, ClearDirty, NoTouch};

  /// Check (and autosave) if there are unsaved changes in a dialog box
  void checkUnsaved(gdioutput &gdi);

  /// Check and ask if there are changes to save
  bool checkSave(gdioutput &gdi);

  void makeDirty(gdioutput &gdi, DirtyFlag inside, DirtyFlag outside); 
public:
  ListEditor(oEvent *oe);
  virtual ~ListEditor();

  //void load(MetaList *list);
  void load(const MetaListContainer &mlc, int index);

  void show(gdioutput &gdi);

  friend int editListCB(gdioutput*, int, void *);

};
