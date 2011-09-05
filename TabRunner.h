#pragma once
/************************************************************************
    MeOS - Orienteering Software
    Copyright (C) 2009-2011 Melin Software HB
    
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

class Table;

class TabRunner :
	public TabBase
{
private:
  void addToolbar(gdioutput &gdi);

  void setCardNo(gdioutput &gdi, int cardNo);

  void enableControlButtons(gdioutput &gdi, bool enable, bool vacant);

  void cellAction(gdioutput &gdi, DWORD id, oBase *obj);

  void selectRunner(gdioutput &gdi, pRunner r);
  int runnerCB(gdioutput &gdi, int type, void *data);
  int punchesCB(gdioutput &gdi, int type, void *data);
  int vacancyCB(gdioutput &gdi, int type, void *data);

  int currentMode;
  //bool tableMode;
  //bool formMode;
  pRunner save(gdioutput &gdi, int runnerId);
  void listRunners(gdioutput &gdi, const vector<pRunner> &r, bool filterVacant) const;

  int cardModeStartY;
  int lastRace;
  int runnerId;

  vector<pRunner> unknown_dns;
  vector<pRunner> known_dns;
  vector<pRunner> known;
  vector<pRunner> unknown;
  void clearInForestData();

  PrinterObject splitPrinter;

  void showVacancyList(gdioutput &gdi, const string &method="", int classId=0);
  void showCardsList(gdioutput &gdi);
public:
  void showInForestList(gdioutput &gdi);
  
	bool loadPage(gdioutput &gdi);
  bool loadPage(gdioutput &gdi, int runnerId);

	TabRunner(oEvent *oe);
	~TabRunner(void);

  friend int RunnerCB(gdioutput *gdi, int type, void *data);
  friend int PunchesCB(gdioutput *gdi, int type, void *data);
  friend int VacancyCB(gdioutput *gdi, int type, void *data);
};
