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
    Stigbergsvägen 7, SE-75242 UPPSALA, Sweden
    
************************************************************************/
#include "tabbase.h"
#include "oListInfo.h"

class ListEditor;

class TabList :
	public TabBase
{
protected:
  EStdListType currentListType;
  oListInfo currentList;
  string SelectedList;
  void generateList(gdioutput &gdi);
  void selectGeneralList(gdioutput &gdi, EStdListType type);

  int offsetY;
  int offsetX;
  set<int> lastClassSelection;

  bool hideButtons;
  bool ownWindow;
  ListEditor *listEditor;
public:
	bool loadPage(gdioutput &gdi);
  int listCB(gdioutput &gdi, int type, void *data);
  void loadGeneralList(gdioutput &gdi);
  void rebuildList(gdioutput &gdi);
  void settingsResultList(gdioutput &gdi);

	TabList(oEvent *oe);
	~TabList(void);
  friend int ListsEventCB(gdioutput *gdi, int type, void *data);
};
