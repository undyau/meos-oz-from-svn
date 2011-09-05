// oClass.h: interface for the oClass class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_OCLASS_H__63E948E3_3C06_4404_8E72_2185582FF30F__INCLUDED_)
#define AFX_OCLASS_H__63E948E3_3C06_4404_8E72_2185582FF30F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

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

#include "oCourse.h"
#include <vector>
#include <set>
#include <map>
#include "inthashmap.h"
class oClass;
typedef oClass* pClass;
class oDataInterface;

enum StartTypes {   
  STTime=0,
  STChange,
  STDrawn,
  STHunting,
  ST_max=STHunting
};
enum { nStartTypes = ST_max + 1 };

enum LegTypes {
  LTNormal=0, 
  LTParallel, 
  LTExtra,
  LTSum,
  LTIgnore,
  LTParallelOptional,
  LT_max=LTParallelOptional
};
enum { nLegTypes = LT_max + 1 };

#ifdef DODECLARETYPESYMBOLS
  const char *StartTypeNames[4]={"ST", "CH", "DR", "HU"};
  const char *LegTypeNames[6]={"NO", "PA", "EX", "SM", "IG", "PO"};
#endif

struct oLegInfo {
	StartTypes startMethod;
	LegTypes legMethod;
  bool isParallel() const {return legMethod == LTParallel || legMethod == LTParallelOptional;}
  bool isOptional() const {return legMethod == LTParallelOptional || legMethod == LTExtra || legMethod == LTIgnore;}
  //Interpreteation depends. Can be starttime/first start
  //or number of earlier legs to consider.
	int legStartData;
  int legRestartTime;
  int legRopeTime;
  int duplicateRunner;

  // Transient, deducable data
  int trueSubLeg;
  int trueLeg;
  string displayLeg;

  oLegInfo():startMethod(STTime), legMethod(LTNormal), legStartData(0), 
             legRestartTime(0), legRopeTime(0), duplicateRunner(-1) {}
	string codeLegMethod() const;
	void importLegMethod(const string &courses);
};

struct ClassResultInfo {
  ClassResultInfo() : nUnknown(0), nFinished(0), lastStartTime(0) {}

  int nUnknown;
  int nFinished;
  int lastStartTime;
};


enum ClassType {oClassIndividual=1, oClassPatrol=2, 
                oClassRelay=3, oClassIndividRelay=4};

enum ClassMetaType {ctElite, ctNormal, ctYouth, ctTraining, 
                    ctExercise, ctOpen, ctUnknown};
 
class Table;
class oClass : public oBase
{
protected:
	string Name;
	pCourse Course;

	vector< vector<pCourse> > MultiCourse;
  vector< oLegInfo > legInfo;

  //First: best time on leg
  //Second: Total leader time (total leader)
  vector< pair<int, int> > tLeaderTime;

  int tSplitRevision;
  map<int, vector<int> > tSplitAnalysisData;
  map<int, vector<int> > tCourseLegLeaderTime;
  map<int, vector<int> > tCourseAccLegLeaderTime;
  mutable vector<int> tFirstStart;
  mutable vector<int> tLastStart;
  // A map with places for given times on given legs
  inthashmap *tLegTimeToPlace;
  inthashmap *tLegAccTimeToPlace;

  void insertLegPlace(int from, int to, int time, int place);
  void insertAccLegPlace(int courseId, int controlNo, int time, int place);

  // For sub split times
  int tLegLeaderTime;
  mutable int tNoTiming;

  // Sort classes for this index
  int tSortIndex;

  BYTE oData[128];

  //Multicourse data
	string codeMultiCourse() const;
  //Fill courseId with id:s of used courses.
	void importCourses(const vector< vector<int> > &multi);
  static void parseCourses(const string &courses, vector< vector<int> > &multi, set<int> &courseId);
  set<int> &getMCourseIdSet(set<int> &in) const;

  //Multicourse leg methods
	string codeLegMethod() const;
	void importLegMethod(const string &courses);

  bool sqlChanged;

  void addTableRow(Table &table) const;
  bool inputData(int id, const string &input, int inputId, 
                        string &output, bool noUpdate);

  void fillInput(int id, vector< pair<string, size_t> > &elements, size_t &selected);
  
  void exportIOFStart(xmlparser &xml);

  /** Setup transient data */
  void reinitialize();

  /** Recalculate derived data */
  void apply();
  
  void calculateSplits();
  void clearSplitAnalysis();

  /** Info about the result in the class for each leg. 
      Use oEvent::analyseClassResultStatus to setup */
  mutable vector<ClassResultInfo> tResultInfo;

public:
  ClassMetaType interpretClassType() const;

  void remove();
  bool canRemove() const;

  /// Return first and last start of runners in class
  void getStartRange(int leg, int &firstStart, int &lastStart) const;


  /** Return true if pure rogaining class, with time limit (sort results by points) */
  bool isRogaining() const;

  /** Get the place for the specified leg (0 == start/finish) and time. 0 if not found */
  int getLegPlace(pControl from, pControl to, int time) const;

  /** Get accumulated leg place */
  int getAccLegPlace(int courseId, int controlNo, int time) const; 

  /** Get sort index from candidate */
  int getSortIndex(int candidate);

  /// Guess type from class name
  void assignTypeFromName();

  bool isSingleRunnerMultiStage() const;

  bool wasSQLChanged() const {return sqlChanged;}
  
  void getStatistics(int feeLock, int &entries, int &started) const; 
  
  int getBestLegTime(int leg) const;
  int getTotalLegLeaderTime(int leg) const;

  string getInfo() const;
  // Returns true if the class has a pool of courses
  bool hasCoursePool() const;
  // Set whether to use a pool or not
  void setCoursePool(bool p);
  // Get the best matching course from a pool
  pCourse selectCourseFromPool(int leg, const SICard &card) const;

  void resetLeaderTime();

  ClassType getClassType() const;  

  bool startdataIgnored(int i) const;
  bool restartIgnored(int i) const;

  StartTypes getStartType(int leg) const;
  LegTypes getLegType(int leg) const;
  int getStartData(int leg) const;
  int getRestartTime(int leg) const;
  int getRopeTime(int leg) const;
  string getStartDataS(int leg) const;
  string getRestartTimeS(int leg) const;
  string getRopeTimeS(int leg) const;

  // Get the index of the base leg for this leg (=first race of leg's runner)
  int getLegRunner(int leg) const;
  // Get the index of this leg for its runner.
  int getLegRunnerIndex(int leg) const;

  void setLegRunner(int leg, int runnerNo);
  
  //Get number of races run by the runner of given leg
  int getNumMultiRunners(int leg) const;

  //Get the number of parallel runners on a given leg (before and after)
  int getNumParallel(int leg) const;

  // Return the number of distinct runners for one
  // "team" in this class.
  int getNumDistinctRunners() const;

  // Return the minimal number of runners in team
  int getNumDistinctRunnersMinimal() const;
  void setStartType(int leg, StartTypes st);
  void setLegType(int leg, LegTypes lt);
  void setStartData(int leg, const string &s);
  void setRestartTime(int leg, const string &t);
  void setRopeTime(int leg, const string &t);
  
  void setNoTiming(bool noResult);
  bool getNoTiming() const; 


	string getClassResultStatus() const;

	//void SetClassStartTime(const string &t);
	//string GetClassStartTimeS();
	//int GetClassStartTime();
	bool isCourseUsed(int Id) const;
  string getLength(int leg) const;

	bool hasMultiCourse() const {return MultiCourse.size()>0;}
	unsigned getNumStages() const {return MultiCourse.size();}
  /** Get the set of true legs, identifying parallell legs etc. Returns indecs into 
   legInfo of the last leg of the true leg (first), and true leg (second).*/
  void getTrueStages(vector<pair<int, int> > &stages) const;

  unsigned getLastStageIndex() const {return max<signed>(MultiCourse.size(), 1)-1;}

  void setNumStages(int no);

	bool operator<(const oClass &b){return tSortIndex<b.tSortIndex;}
	oDataInterface getDI(void);
  oDataConstInterface getDCI(void) const;

  //Get number of runners running this class
	int getNumRunners() const;

  //Get remaining maps for class (or int::minvalue) 
  int getNumRemainingMaps(bool recalculate) const;

	const string &getName() const {return Name;}
	void setName(const string &name);

	void Set(const xmlobject *xo);
	bool Write(xmlparser &xml);
	
	bool fillStages(gdioutput &gdi, const string &name) const;
	bool fillStageCourses(gdioutput &gdi, int stage, 
                        const string &name) const;

  static void fillStartTypes(gdioutput &gdi, const string &name);
  static void fillLegTypes(gdioutput &gdi, const string &name);

	pCourse getCourse() const {return Course;}
  pCourse getCourse(int leg, unsigned fork=0) const;
	int getCourseId() const {if(Course) return Course->getId(); else return 0;}
	void setCourse(pCourse c);

	bool addStageCourse(int Stage, int CourseId);
	bool removeStageCourse(int Stage, int CourseId, int position);

  void getAgeLimit(int &low, int &high) const;
  void setAgeLimit(int low, int high);
  
  int getExpectedAge() const;

  string getSex() const;
  void setSex(const string &sex);
  
  string getStart() const;
  void setStart(const string &start);
  
  int getBlock() const;
  void setBlock(int block);

  bool getAllowQuickEntry() const;
  void setAllowQuickEntry(bool quick);

  string getType() const;
  void setType(const string &type);

  // Get class default fee from competition, depending on type(?)
  // a non-zero fee is changed only if resetFee is true
  void addClassDefaultFee(bool resetFee);

  // Get entry fee depending on date
  int getEntryFee(const string &date) const;

	oClass(oEvent *poe);
	oClass(oEvent *poe, int id);
  virtual ~oClass();

  friend class oAbstractRunner;
	friend class oEvent;
	friend class oRunner;
	friend class oTeam;
	friend class MeosSQL;
  friend class TabSpeaker;
};

#endif // !defined(AFX_OCLASS_H__63E948E3_3C06_4404_8E72_2185582FF30F__INCLUDED_)
