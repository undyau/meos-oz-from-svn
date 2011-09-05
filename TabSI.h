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
#include "SportIdent.h"

struct PunchInfo;
class csvparser;

class TabSI :
	public TabBase
{

  /** Try to automatcally assign a class to runner (if none is given)
      Return true if runner has a class on exist */
  bool autoAssignClass(pRunner r, const SICard &sic);

  void checkMoreCardsInQueue(gdioutput &gdi);

  pRunner autoMatch(const SICard &sic, pRunner db_r);
  void processPunchOnly(gdioutput &gdi, const SICard &sic);
  void startInteractive(gdioutput &gdi, const SICard &sic, 
                        pRunner r, pRunner db_r);
  bool processCard(gdioutput &gdi, pRunner runner, const SICard &csic, 
                   bool silent=false);
  bool processUnmatched(gdioutput &gdi, const SICard &csic, bool silent);

  void rentCardInfo(gdioutput &gdi, int width);

  bool interactiveReadout;
  bool useDatabase;
  bool printSplits;
  PrinterObject splitPrinter;

  vector<PunchInfo> punches;
  vector<SICard> cards;
  vector<string> filterDate;

  enum SIMode {
    ModeReadOut,
    ModeAssignCards,
    ModeEntry
  };
  //Interactive card assign
  SIMode mode;
  int currentAssignIndex;

  void assignCard(gdioutput &gdi, const SICard &sic);
  void entryCard(gdioutput &gdi, const SICard &sic);

  void updateEntryInfo(gdioutput &gdi);
  void generateEntryLine(gdioutput &gdi, pRunner r);
  int lastClassId;
  int lastClubId;
  int lastFee;

  void showReadPunches(gdioutput &gdi, vector<PunchInfo> &punches, set<string> &dates);
  void showReadCards(gdioutput &gdi, vector<SICard> &cards);

  // Insert card without converting times and with/without runner
  void processInsertCard(const SICard &csic);

  void printerSetup(gdioutput &gdi);
  void generateSplits(const pRunner r, gdioutput &gdi);
  int logcounter;
  csvparser *logger;

  string insertCardNumberField;
public:
  int siCB(gdioutput &gdi, int type, void *data);

  void logCard(const SICard &card);

  void setCardNumberField(const string &fieldId) {insertCardNumberField=fieldId;}

  //SICard CSIC;
  SICard activeSIC;
  list<SICard> CardQueue;

  void insertSICard(gdioutput &gdi, SICard &sic);

  void refillComPorts(gdioutput &gdi);

	bool loadPage(gdioutput &gdi);
	TabSI(oEvent *oe);
	~TabSI(void);
};
