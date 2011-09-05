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

#include <mysql++.h>

#include <string>

class oRunner;
class oEvent;
class oCard;
class oClub;
class oCourse;
class oClass;
class oControl;
class oBase;
class oFreePunch;
class oDataInterface;
class oTeam;

namespace mysqlpp {
  class Query;
}

using namespace std;

enum OpFailStatus {
  opStatusOK = 2,
  opStatusFail = 0,
  opStatusWarning = 1,
  opUnreachable = -1,
};

class MeosSQL
{
protected:
  bool warnedOldVersion;
  int monitorId;
  int buildVersion;
	mysqlpp::Connection con;
	string CmpDataBase;
	void alert(const string &s);

  string errorMessage;

  string serverName;
  string serverUser;
  string serverPassword;
  unsigned int serverPort;

	bool isOld(int counter, const string &time, oBase *ob);
	OpFailStatus updateTime(const char *oTable, oBase *ob);
  // Update object in database with fixed query. If useId is false, Id is ignored (used
	OpFailStatus syncUpdate(mysqlpp::Query &updateqry, const char *oTable, oBase *ob);
	bool storeData(oDataInterface &odi, const mysqlpp::Row &row);
  


  //Set DB to default competition DB
  void setDefaultDB();

  // Update the courses of a class.
  OpFailStatus syncReadClassCourses(oClass *c, int cno, 
                                    const set<int> &multi,
                                    bool readRecursive);
  OpFailStatus syncRead(bool forceRead, oTeam *t, bool readRecursive);
  OpFailStatus syncRead(bool forceRead, oRunner *r, bool readClassClub, bool readCourseCard);
  OpFailStatus syncRead(bool forceRead, oCourse *c, bool readControls);
	OpFailStatus syncRead(bool forceRead, oClass *c, bool readCourses);
	
  void storeClub(const mysqlpp::Row &row, oClub &c);
  void storeControl(const mysqlpp::Row &row, oControl &c);
  void storeCard(const mysqlpp::Row &row, oCard &c);
  void storePunch(const mysqlpp::Row &row, oFreePunch &p);

  OpFailStatus storeTeam(const mysqlpp::Row &row, oTeam &t, 
                         bool readRecursive);

  OpFailStatus storeRunner(const mysqlpp::Row &row, oRunner &r, 
                           bool readCourseCard, 
                           bool readClassClub, 
                           bool readRunners);
  OpFailStatus storeCourse(const mysqlpp::Row &row, oCourse &c,
                           bool readControls);
  OpFailStatus storeClass(const mysqlpp::Row &row, oClass &c,
                           bool readCourses);


public:
  bool dropDatabase(oEvent *oe);
  bool checkConnection(oEvent *oe);

  bool getErrorMessage(char *bf);
  bool reConnect();
	bool ListCompetitions(oEvent *oe);
	bool Remove(oBase *ob);

  // Create database of runners and clubs
  bool createRunnerDB(oEvent *oe, mysqlpp::Query &query);

  // Upload runner database to server
  OpFailStatus uploadRunnerDB(oEvent *oe);

	bool openDB(oEvent *oe);
	
  bool closeDB();

	bool syncListRunner(oEvent *oe);
	bool syncListClass(oEvent *oe);
	bool syncListCourse(oEvent *oe);
	bool syncListControl(oEvent *oe);
	bool syncListCard(oEvent *oe);
	bool syncListClub(oEvent *oe);
	bool syncListPunch(oEvent *oe);
	bool syncListTeam(oEvent *oe);	

  OpFailStatus SyncEvent(oEvent *oe);

	OpFailStatus SyncUpdate(oEvent *oe);
	OpFailStatus SyncRead(oEvent *oe);

	OpFailStatus syncUpdate(oRunner *r);
	OpFailStatus syncRead(bool forceRead, oRunner *r);

	OpFailStatus syncUpdate(oCard *c);
	OpFailStatus syncRead(bool forceRead, oCard *c);

	OpFailStatus syncUpdate(oClass *c);
	OpFailStatus syncRead(bool forceRead, oClass *c);
	
	OpFailStatus syncUpdate(oClub *c);
	OpFailStatus syncRead(bool forceRead, oClub *c);

	OpFailStatus syncUpdate(oCourse *c);
	OpFailStatus syncRead(bool forceRead, oCourse *c);
	
	OpFailStatus syncUpdate(oControl *c);
	OpFailStatus syncRead(bool forceRead, oControl *c);

	OpFailStatus syncUpdate(oFreePunch *c);
	OpFailStatus syncRead(bool forceRead, oFreePunch *c);

	OpFailStatus syncUpdate(oTeam *t);
	OpFailStatus syncRead(bool forceRead, oTeam *t);

	MeosSQL(void);
	virtual ~MeosSQL(void);
};
