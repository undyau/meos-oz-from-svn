#pragma once

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
    Stigbergsvägen 7, SE-75242 UPPSALA, Sweden
    
************************************************************************/
#include "oBase.h"
#include "oRunner.h"

class oTeam;//:public oBase {};
typedef oTeam* pTeam;
typedef const oTeam* cTeam;

const unsigned int maxRunnersTeam=32;

class oTeam : public oAbstractRunner
{
protected:
	//pRunner Runners[maxRunnersTeam];
  vector<pRunner> Runners;
	void setRunnerInternal(int k, pRunner r); 

	BYTE oData[96];

  // Remove runner r by force and mark as need correction
  void correctRemove(pRunner r);

	int _places[maxRunnersTeam];
	//unsigned _nrunners;
	int _sorttime;
	int _sortstatus;
  int tTotalPlace;
	string getRunners() const;
  bool matchTeam(int number, const char *s_lc) const;
  int tNumRestarts; //Number of restarts for team

  int getLegToUse(int leg) const; // Get the number of the actual 
                                  // runner to consider for a given leg
                                  // Maps -1 to last runner

  void addTableRow(Table &table) const;

  bool inputData(int id, const string &input, 
                 int inputId, string &output, bool noUpdate);

  void fillInput(int id, vector< pair<string, size_t> > &out, size_t &selected);

public:

  /** Remove runner from team (and from competition)
    @param askRemoveRunner ask if runner should be removed from cmp. Otherwise just do it.
    */
  void removeRunner(gdioutput &gdi, bool askRemoveRunner, int runnerIx);
  // Get entry date of team
  string getEntryDate(bool dummy) const;

  // Get the team itself
  cTeam getTeam() const {return this;}
  pTeam getTeam() {return this;}

  int getRunningTime() const;

  /// Input data for multiday event
  void setInputData(const oTeam &t);

  /// Get total status for multiday event
  RunnerStatus getTotalStatus() const;

  void remove();
  bool canRemove() const;

  void prepareRemove();
	oDataInterface getDI(void);
  oDataConstInterface getDCI(void) const;

  bool skip() const {return isRemoved();}
  void setTeamNoStart(bool dns);
	// If apply is triggered by a runner, don't go further than that runner.
  bool apply(bool sync, pRunner source);
  bool apply(bool sync);
	void evaluate(bool sync);
  bool adjustMultiRunners(bool sync);

  int getRogainingPoints() const;

  void fillSpeakerObject(int leg, int controlId, oSpeakerObject &spk) const;

	bool isRunnerUsed(int Id) const;
	void setRunner(unsigned i, pRunner r, bool syncRunner);

  pRunner getRunner(unsigned leg) const;
	int getNumRunners() const {return Runners.size();}
	
  void decodeRunners(const string &rns, vector<int> &rid);
  void importRunners(const vector<int> &rns);
  void importRunners(const vector<pRunner> &rns);

  int getPlace() const {return getLegPlace(-1);}
  int getTotalPlace() const;

  string getDisplayName() const;
  string getDisplayClub() const;

  string getBib() const;
  void setBib(const string &bib, bool updateStartNo);

  int getLegStartTime(int leg) const;
	string getLegStartTimeS(int leg) const;
  string getLegStartTimeCompact(int leg) const;

  string getLegFinishTimeS(int leg) const;
	int getLegFinishTime(int leg) const;
  
  int getTimeAfter(int leg) const;

  //Get total running time after leg 
  string getLegRunningTimeS(int leg) const;
  int getLegRunningTime(int leg) const;
  int getLegPrelRunningTime(int leg) const;
	string getLegPrelRunningTimeS(int leg) const;
  
  RunnerStatus getLegStatus(int leg) const;
	const string &getLegStatusS(int leg) const;

	string getLegPlaceS(int leg) const;
  string getLegPrintPlaceS(int leg) const;
	int getLegPlace(int leg) const { return leg>=0 && leg<maxRunnersTeam ? _places[leg]:_places[Runners.size()-1];}

  static bool compareSNO(const oTeam &a, const oTeam &b);
	static bool compareName(const oTeam &a, const oTeam &b) {return a.Name<b.Name;} 
	static bool compareResult(const oTeam &a, const oTeam &b);
	static bool compareStartTime(const oTeam &a, const oTeam &b);
	
	
	void set(const xmlobject &xo);
	bool write(xmlparser &xml);

  oTeam(oEvent *poe, int id);
	oTeam(oEvent *poe);
	virtual ~oTeam(void);

  friend class oClass;
	friend class oRunner;
	friend class MeosSQL;
	friend class oEvent;
};
