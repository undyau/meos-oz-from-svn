// oRunner.h: interface for the oRunner class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ORUNNER_H__D3B8D6C8_C90A_4F86_B776_7D77E5C76F42__INCLUDED_)
#define AFX_ORUNNER_H__D3B8D6C8_C90A_4F86_B776_7D77E5C76F42__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

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

#include <map>

#include "oBase.h"
#include "oClub.h"
#include "oClass.h"
#include "oCard.h"
#include "oSpeaker.h"

class oRunner;
typedef oRunner* pRunner;
typedef const oRunner* cRunner;

class oTeam;
typedef oTeam* pTeam;
typedef const oTeam* cTeam;

struct SICard;

class oAbstractRunner : public oBase {
protected:
  string Name;	
	pClub Club;
	pClass Class;

	int StartTime;
	int FinishTime;
	RunnerStatus Status;

 	//Used for automatically assigning courses form class. 
	//Set when drawn or by team or...
  int StartNo;

  // Data used for multi-days events
  int inputTime;
  RunnerStatus inputStatus;
  int inputPoints;
  int inputPlace;

  bool sqlChanged;
public:
  // Get the runners team or the team itself
  virtual cTeam getTeam() const = 0;
  virtual pTeam getTeam() = 0;

  virtual string getEntryDate(bool useTeamEntryDate = true) const = 0;

  // Set default fee, from class 
  // a non-zero fee is changed only if resetFee is true
  void addClassDefaultFee(bool resetFees);

  bool hasInputData() const {return inputTime > 0 || inputStatus != StatusOK || inputPoints > 0;}

  // Time
  void setInputTime(const string &time);
  string getInputTimeS() const;

  // Status
  void setInputStatus(RunnerStatus s);
  string getInputStatusS() const;

  // Points
  void setInputPoints(int p);
  int getInputPoints() const {return inputPoints;}

  // Place
  void setInputPlace(int p);
  int getInputPlace() const {return inputPlace;}

  bool isVacant() const;
  
  bool wasSQLChanged() const {return sqlChanged;}
  void markClassChanged() {if (Class) Class->sqlChanged = true; }

  string getInfo() const;

  virtual bool apply(bool sync) = 0;

  //Get time after on leg/for race
  virtual int getTimeAfter(int leg) const = 0;


  virtual void fillSpeakerObject(int leg, int controlId, 
                                 oSpeakerObject &spk) const = 0;

  virtual int getBirthAge() const;

  virtual void setName(const string &n);
	virtual const string &getName() const {return Name;}

  virtual void setFinishTimeS(const string &t);
  virtual	void setFinishTime(int t);	

  virtual void setStartTime(int t);
  virtual void setStartTimeS(const string &t);

  const pClub getClubRef() const {return Club;}
  pClub getClubRef() {return Club;}

  const pClass getClassRef() const {return Class;}
  pClass getClassRef() {return Class;}

  virtual const string &getClub() const {if(Club) return Club->name; else return _EmptyString;}
	virtual int getClubId() const {if(Club) return Club->Id; else return 0;}
	virtual void setClub(const string &Name);
  virtual pClub setClubId(int clubId);

	virtual const string &getClass() const {if(Class) return Class->Name; else return _EmptyString;}
	virtual int getClassId() const {if(Class) return Class->Id; else return 0;}
	virtual void setClassId(int id);
	virtual int getStartNo() const {return StartNo;}
	virtual void setStartNo(int no);

  // Start number is equal to bib-no, but bib
  // is only set when it should be shown in lists etc.
  virtual string getBib() const = 0;
  virtual void setBib(const string &bib, bool updateStartNo) = 0;

	virtual int getStartTime() const {return StartTime;}
	virtual int getFinishTime() const {return FinishTime;}

	virtual string getStartTimeS() const;
  virtual string getStartTimeCompact() const;
	virtual string getFinishTimeS() const;

  virtual	string getTotalRunningTimeS() const;
  virtual	string getRunningTimeS() const;
  virtual int getRunningTime() const {return max(FinishTime-StartTime, 0);}

  /// Get total running time (including earlier stages / races)
  virtual int getTotalRunningTime() const;

  virtual int getPrelRunningTime() const;
	virtual string getPrelRunningTimeS() const;
  
	virtual string getPlaceS() const;
	virtual string getPrintPlaceS() const;

  virtual string getTotalPlaceS() const;
	virtual string getPrintTotalPlaceS() const;
  
  virtual int getPlace() const = 0;
  virtual int getTotalPlace() const = 0;

  virtual RunnerStatus getStatus() const {return Status;}
  inline bool statusOK() const {return Status==StatusOK;}
	virtual void setStatus(RunnerStatus st);

  /// Get total status for this running (including team/earlier races)
  virtual RunnerStatus getTotalStatus() const;

  virtual const string &getStatusS() const;
	virtual string getIOFStatusS() const;

  virtual const string &getTotalStatusS() const;
	virtual string getIOFTotalStatusS() const;


  void setSpeakerPriority(int pri);
  virtual int getSpeakerPriority() const;

  oAbstractRunner(oEvent *poe);
  virtual ~oAbstractRunner() {};

  friend class oListInfo;
};

struct RunnerDBEntry;

struct SplitData {
  enum SplitStatus {OK, Missing, NoTime};
  int time;
  SplitStatus status;
  SplitData() {};
  SplitData(int t, SplitStatus s) : time(t), status(s) {};

  void setPunchTime(int t) {
    time = t;
    status = OK;
  }

  void setPunched() {
    time = -1;
    status = NoTime;
  }

  void setNotPunched() {
    time = -1;
    status = Missing;
  }

  bool hasTime() const {
    return time > 0 && status == OK;
  }

  bool isMissing() const {
    return status == Missing;
  }
};

class oRunner : public oAbstractRunner
{
protected:
	//int clubId; //For DataBase Only.
	pCourse Course;

	int CardNo;
	pCard Card;
  
  vector<pRunner> multiRunner;
  vector<int> multiRunnerId;

  //Can be changed by apply
	mutable int tPlace;
  mutable int tCoursePlace;
	mutable int tTotalPlace;
  mutable int tLeg;
	mutable pTeam tInTeam;
  mutable pRunner tParentRunner;
  mutable bool tNeedNoCard;
  mutable bool tUseStartPunch;
  mutable int tDuplicateLeg;

  //Temporary status and running time
  RunnerStatus tStatus;
  int tRT;
  bool isTemporaryObject;
  int tTimeAfter; // Used in time line calculations, time after "last radio".
  int tInitialTimeAfter; // Used in time line calculations, time after when started.
	//Speaker data
	map<int, int> Priority;
	int cPriority;

	BYTE oData[128];
  bool storeTimes(); // Returns true if best times were updated
  // Adjust times for fixed time controls
  void doAdjustTimes(pCourse course);

  vector<int> adjustTimes;
  vector<SplitData> splitTimes;
  
  vector<int> tLegTimes;
  vector<int> tLegPlaces;

  string codeMultiR() const;
  void decodeMultiR(const string &r);
  
  pRunner getPredecessor() const;

  void markForCorrection() {correctionNeeded = true;}
  //Remove by force the runner from a multirunner definition
  void correctRemove(pRunner r);

  vector<pRunner> getRunnersOrdered() const;
  int getMultiIndex(); //Returns the index into multi runners, 0 - n, -1 on error.

  void exportIOFRunner(xmlparser &xml, bool compact); 
  void exportIOFStart(xmlparser &xml);

  // Revision number of runners statistics cache
  mutable int tSplitRevision;
  // Running time as calculated by evalute. Used to detect changes.
  int tCachedRunningTime;
  // Cached runner statistics
  mutable vector<int> tMissedTime;
  mutable vector<int> tPlaceLeg;
  mutable vector<int> tAfterLeg;
  mutable vector<int> tPlaceLegAcc;
  mutable vector<int> tAfterLegAcc;

  // Rogainig results. Control and punch time
  vector< pair<pControl, int> > tRogaining;
  int tRogainingPoints;
  int tPenaltyPoints;
  string tProblemDescription;
  // Sets up mutable data above
  void setupRunnerStatistics() const;
  void printRogainingSplits(gdioutput &gdi) const;

  // Update hash
  void changeId(int newId);

public:
  // Get entry date of runner (or its team)
  string getEntryDate(bool useTeamEntryDate = true) const;

  // Get date of birth

  int getBirthAge() const;

  // Multi day data input
  void setInputData(const oRunner &source);

  int getLegNumber() const {return tLeg;}
  int getSpeakerPriority() const;

  void remove();
  bool canRemove() const;

  cTeam getTeam() const {return tInTeam;}
  pTeam getTeam() {return tInTeam;}

  /// Get total running time for multi/team runner at the given time
  int getTotalRunningTime(int time) const;
  
  // Get total running time at finish time (@override)
  int getTotalRunningTime() const;

  //Get total running time after leg 
  int getRaceRunningTime(int leg) const;
 

  // Get the complete name, including team and club.
  string getCompleteIdentification() const;

  /// Get total status for this running (including team/earlier races)
  RunnerStatus getTotalStatus() const;

  // Return the runner in a multi-runner set matching the card, if course type is extra
  pRunner getMatchedRunner(const SICard &sic) const;

  const int getRogainingPoints() const {return tRogainingPoints;}
  const int getPenaltyPoints() const {return tPenaltyPoints;}
  const string &getProblemDescription() const {return tProblemDescription;}

  // Leg statistics access methods
  string getMissedTimeS() const;
  string getMissedTimeS(int ctrlNo) const;
   
  int getMissedTime(int ctrlNo) const;
  int getLegPlace(int ctrlNo) const;
  int getLegTimeAfter(int ctrlNo) const;
  int getLegPlaceAcc(int ctrlNo) const;
  int getLegTimeAfterAcc(int ctrlNo) const;

  /** Calculate the time when the runners place is fixed, i.e,
      when no other runner can threaten the place.
      Returns -1 if undeterminable.
      Return 0 if place is fixed. */
  int getTimeWhenPlaceFixed() const;

  /** Automatically assign a bib. Returns true if bib is assigned. */
  bool autoAssignBib();

  /** Flag as temporary */
  void setTemporary() {isTemporaryObject=true;}

  /** Init from dbrunner */
  void init(const RunnerDBEntry &entry);

  /** Use db to pdate runner */
  bool updateFromDB(const string &name, int clubId, int classId, 
                    int cardNo, int birthYear);    

  void printSplits(gdioutput &gdi) const;

  /** Take the start time from runner r*/
  void cloneStartTime(const pRunner r);

  /** Clone data from other runner */
  void cloneData(const pRunner r);

  // Leg to run for this runner. Maps into oClass.MultiCourse.
  // Need to check index in bounds.
  int legToRun() const {return tInTeam ? tLeg : tDuplicateLeg;}
  void setName(const string &n);
  void setClassId(int id);
  void setClub(const string &Name);
  pClub setClubId(int clubId);

  string getBib() const;

  // Start number is equal to bib-no, but bib
  // is only set when it should be shown in lists etc.
  // Need not be so for teams. Course depends on start number,
  // which should be more stable.
  void setBib(const string &bib, bool updateStartNo);
  void setStartNo(int no);

  pRunner nextNeedReadout() const;

  // Synchronize this runner and parents/sibllings and team
  bool synchronizeAll();

  void setFinishTime(int t);  
  int getTimeAfter(int leg) const;
  int getTimeAfter() const;
  int getTimeAfterCourse() const;
  
  bool skip() const {return isRemoved() || tDuplicateLeg!=0;}

  pRunner getMultiRunner(int race) const;
  int getNumMulti() const {return multiRunner.size();} //Returns number of  multi runners (zero=no multi)
  void createMultiRunner(bool createMaster, bool sync);
  int getRaceNo() const {return tDuplicateLeg;}
  string getNameAndRace() const;

  void fillSpeakerObject(int leg, int controlId, 
                         oSpeakerObject &spk) const;

  bool needNoCard() const;
  int getPlace() const;
  int getCoursePlace() const;
  int getTotalPlace() const;

  const vector<SplitData> &getSplitTimes() const {return splitTimes;}

  void getSplitAnalysis(vector<int> &deltaTimes) const;
  void getLegPlaces(vector<int> &places) const;
  void getLegTimeAfter(vector<int> &deltaTimes) const;

  void getLegPlacesAcc(vector<int> &places) const;
  void getLegTimeAfterAcc(vector<int> &deltaTimes) const;

  int getSplitTime(int controlNumber) const;
  int getTimeAdjust(int controlNumber) const;

  int getNamedSplit(int controlNumber) const;
  int getPunchTime(int controlNumber) const;
  string getSplitTimeS(int controlNumber) const;
  string getPunchTimeS(int controlNumber) const;
  string getNamedSplitS(int controlNumber) const;

  void addTableRow(Table &table) const;
  bool inputData(int id, const string &input, 
                  int inputId, string &output, bool noUpdate);
  void fillInput(int id, vector< pair<string, size_t> > &elements, size_t &selected);

	bool apply(bool sync);
	void resetPersonalData();

	oDataInterface getDI(void);
  oDataConstInterface getDCI(void) const;

	//Local user data. No Update.
	void SetPriority(int type, int p){Priority[type]=p;}

	string getGivenName() const;
	string getFamilyName() const;
	
	pCourse getCourse() const;
	string getCourseName() const;

	pCard getCard() const {return Card;}
	int getCardId(){if(Card) return Card->Id; else return 0;}

	bool operator<(const oRunner &c);
	bool static CompareSINumber(const oRunner &a, const oRunner &b){return a.CardNo<b.CardNo;}

	bool evaluateCard(vector<int> &MissingPunches, int addpunch=0, bool synchronize=false);
	void addPunches(pCard card, vector<int> &MissingPunches);
	
	void getSplitTime(int controlId, RunnerStatus &stat, int &rt) const;

	//const string &getClubNameDB() const {return ClubName;}

	//Returns only Id of a runner-specific course, not classcourse
	int getCourseId() const {if(Course) return Course->Id; else return 0;}
	void setCourseId(int id);

  int getCardNo() const {return tParentRunner ? tParentRunner->CardNo : CardNo;}
	void setCardNo(int card, bool matchCard, bool updateFromDatabase = false);
  /** Sets the card to a given card. An existing card is marked as unpaired.
      CardNo is updated. Returns id of old card (or 0).
  */
  int setCard(int cardId);

	void Set(const xmlobject &xo);

	bool Write(xmlparser &xml);
	bool WriteSmall(xmlparser &xml);

	oRunner(oEvent *poe);
  oRunner(oEvent *poe, int id);
	
  void setSex(PersonSex sex);
  PersonSex getSex() const;

  void setBirthYear(int year);
  int getBirthYear() const;
  void setNationality(const string &nat);
  string getNationality() const;

  // Return true if the input name is considered equal to output name
  bool matchName(const string &pname) const;

	virtual ~oRunner();

	friend class MeosSQL;
	friend class oEvent;
	friend class oTeam;
  friend class oClass;
	friend bool CompareRunnerSPList(const oRunner &a, const oRunner &b);
	friend bool CompareRunnerResult(const oRunner &a, const oRunner &b);
  friend bool compareClubClassTeamName(const oRunner &a, const oRunner &b);
  friend class RunnerDB;
  friend class oListInfo;
  static bool sortSplit(const oRunner &a, const oRunner &b);

};

#endif // !defined(AFX_ORUNNER_H__D3B8D6C8_C90A_4F86_B776_7D77E5C76F42__INCLUDED_)
