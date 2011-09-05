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


#include "StdAfx.h"
#include "MeosSQL.h"

#include <fstream>
#include "../oRunner.h"
#include "../oEvent.h"
#include "../meos_util.h"
#include <cassert>
#include "../RunnerDB.h"
#include "../progress.h"

using namespace mysqlpp;

MeosSQL::MeosSQL(void)
{
  monitorId=0;
  warnedOldVersion=false;
  buildVersion=getMeosBuild();
}

MeosSQL::~MeosSQL(void)
{
}

void MeosSQL::alert(const string &s)
{
  errorMessage=s;
}

string C_INT(string name)
{
	return " "+name+" INT NOT NULL DEFAULT 0, ";
} 

string C_INT64(string name)
{
	return " "+name+" BIGINT NOT NULL DEFAULT 0, ";
} 

string C_STRING(string name, int len=64)
{
	char bf[16];
	sprintf_s(bf, "%d", len);
	return " "+name+" VARCHAR("+ bf +") NOT NULL DEFAULT '', ";
}

string C_TEXT(string name)
{
	return " "+name+" TEXT NOT NULL, ";
}

string C_UINT(string name)
{
	return " "+name+" INT UNSIGNED NOT NULL DEFAULT 0, ";
}

string C_START(string name)
{
	return "CREATE TABLE IF NOT EXISTS "+name+" (" +
			"  Id INT AUTO_INCREMENT NOT NULL, PRIMARY KEY (Id), ";// +
			//"  Modified TIMESTAMP, ";
}

string C_START_noid(string name)
{
	return "CREATE TABLE IF NOT EXISTS "+name+" (";
}


string C_END()
{
	return " Modified TIMESTAMP, Counter INT UNSIGNED NOT NULL DEFAULT 0, "
    "INDEX(Counter), INDEX(Modified), Removed BOOL NOT NULL DEFAULT 0) "
    "ENGINE = MyISAM CHARACTER SET utf8 COLLATE utf8_general_ci";
}

string C_END_noindex()
{
	return " Modified TIMESTAMP) "
    "ENGINE = MyISAM CHARACTER SET utf8 COLLATE utf8_general_ci";
}

bool MeosSQL::ListCompetitions(oEvent *oe)
{
  errorMessage.clear();
	CmpDataBase="";
  oe->serverName.clear();
	try {
		con.connect("", oe->MySQLServer.c_str(), oe->MySQLUser.c_str(), 
                oe->MySQLPassword.c_str(), oe->MySQLPort);
	}
	catch (const Exception& er) {
		alert(string(er.what()) + " MySQL Error");
		return false;
	}


  if (!con.connected()) {
    errorMessage = "Internal error connecting to MySQL";
    return false;
  }

  serverName=oe->MySQLServer;
  serverUser=oe->MySQLUser;
  serverPassword=oe->MySQLPassword;
  serverPort=oe->MySQLPort;
  
  //Store verified server name
  oe->serverName=serverName;

	NoExceptions ne(con);

	if(!con.select_db("MeOSMain")){
		con.create_db("MeOSMain");
		con.select_db("MeOSMain");
	}

	Query query = con.query();

  try{
		query << C_START("oEvent")
			<< C_TEXT("Name")
			<< C_TEXT("Date")
			<< C_UINT("ZeroTime") 
			<< C_TEXT("NameId")
			<< " Version INT UNSIGNED DEFAULT 1, " << C_END();
		
		query.execute();
	}
	catch (const Exception& er){
		alert(string(er.what())+ " MySQL Error");
    return false;
	}

  query.reset();

	try {
		query << "SELECT * FROM oEvent";
	
		Result res = query.store();
		
		if (res) {		
			for (int i=0; i<res.num_rows(); i++) {
				Row row=res.at(i);
						
        if (int(row["Version"])==oe->dbVersion) {
          CompetitionInfo ci;
          ci.Name=row["Name"];
				  ci.Id=row["Id"];
				  ci.Date=row["Date"];
				  ci.FullPath=row["NameId"];
				  ci.Server=oe->MySQLServer;
				  ci.ServerPassword=oe->MySQLPassword;
				  ci.ServerUser=oe->MySQLUser;
				  ci.ServerPort=oe->MySQLPort;

				  oe->cinfo.push_front(ci);
        }
        else {
          CompetitionInfo ci;
          ci.Name=row["Name"];
          ci.Date=row["Date"];				  
          ci.Id=0;
          ci.Server="bad";
          ci.FullPath=row["NameId"];
				  oe->cinfo.push_front(ci);
        }
			}
		}	
	}
	catch (const Exception& er) {
    setDefaultDB();
		alert(string(er.what()) + " MySQL Error");
		return false;
	}

	return true;
}

bool MeosSQL::createRunnerDB(oEvent *oe, Query &query) 
{
	query.reset();

  query << C_START_noid("dbRunner") 
	<< C_STRING("Name", 31) << C_INT("CardNo")
  << C_INT("Club") << C_STRING("Nation", 3) 
	<< C_STRING("Sex", 1) << C_INT("BirthYear")
  << C_INT64("ExtId") << C_END_noindex();
		
	query.execute();

	query.reset();
	query << C_START("dbClub")
		<< C_TEXT("Name") 
		<< oe->oClubData->generateSQLDefinition() << C_END_noindex();
	query.execute();

  return true;
}

bool MeosSQL::openDB(oEvent *oe)
{
  errorMessage.clear();
	if(!con.connected() && !ListCompetitions(oe)) 
		return false;

	try{
		con.select_db("MeOSMain");
	}
	catch (const mysqlpp::Exception& er){
		alert(string(er.what()) + " MySQL Error. Select MeosMain");
		setDefaultDB();
    return 0;
	}
  monitorId=0;
	string dbname=oe->CurrentNameId;

	try {
		Query query = con.query();
		query << "SELECT * FROM oEvent WHERE NameId=" << quote << dbname;
		Result res = query.store();
	
		if (res && res.num_rows()>=1) {		
			Row row=res.at(0);

      int version = row["Version"];
      
      if (version != oe->dbVersion) {
        // Wrong version. Drop and reset.
        query.reset();
        query << "DELETE FROM oEvent WHERE Id=" << row["Id"];
        query.execute();
        con.drop_db(dbname);
        return openDB(oe);
      }

			oe->Id=row["Id"]; //Don't synchronize more here...
		}
		else {
			query.reset();
			query << "INSERT INTO oEvent SET Name='-', Date='', NameId=" << quote << dbname
            << ", Version=" << oe->dbVersion;

			ResNSel res=query.execute();

			if(res){
				oe->Id=static_cast<int>(res.insert_id); 
			}
		}
	}	
	catch (const mysqlpp::Exception& er){
    setDefaultDB();
    alert(string(er.what()) + " MySQL Error. Select DB.");
		return 0;
	}

	{
		mysqlpp::NoExceptions ne(con);

		if(!con.select_db(dbname)){
			con.create_db(dbname);
			con.select_db(dbname);
		}
	}

	CmpDataBase=dbname;

	Query query = con.query();
	try {
    //Real version of oEvent db
		query << C_START("oEvent")
		<< C_TEXT("Name")
		<< C_TEXT("Date")
		<< C_UINT("ZeroTime") 
		<< C_TEXT("NameId")
    << C_UINT("BuildVersion")
		<< oe->getDI().generateSQLDefinition() << C_END();
		query.execute();

		query.reset();

		query << C_START("oRunner") 
		<< C_STRING("Name") << C_INT("CardNo")
		<< C_INT("Club") << C_INT("Class") << C_INT("Course") << C_INT("StartNo")
		<< C_INT("StartTime") << C_INT("FinishTime")
		<< C_INT("Status") << C_INT("Card") << C_TEXT("MultiR") 
    << C_INT("InputTime") << C_INT("InputStatus") << C_INT("InputPoints") << C_INT("InputPlace")
		<< oe->oRunnerData->generateSQLDefinition() << C_END();
			
		query.execute();

		query.reset();
		query << C_START("oCard")
			<< C_INT("CardNo")
			<< C_UINT("ReadId")
			<< C_TEXT("Punches") << C_END();
		
		query.execute();

		query.reset();
		query << C_START("oClass")
			<< C_TEXT("Name")
			<< C_UINT("Course")
			<< C_TEXT("MultiCourse")
			<< C_TEXT("LegMethod")			
			<< oe->oClassData->generateSQLDefinition() << C_END();
		query.execute();
		
		query.reset();
		query << C_START("oClub")
			<< C_TEXT("Name") 
			<< oe->oClubData->generateSQLDefinition() << C_END();
		query.execute();

		query.reset();
		query << C_START("oControl")
			<< C_TEXT("Name")
			<< C_TEXT("Numbers")		
			<< C_UINT("Status") 
			<< oe->oControlData->generateSQLDefinition() << C_END();
		query.execute();

		query.reset();
		query << C_START("oCourse")
			<< C_TEXT("Name")
			<< C_TEXT("Controls")		
			<< C_UINT("Length") 
      << C_TEXT("Legs")		
			<< oe->oCourseData->generateSQLDefinition() << C_END();
		query.execute();

		query.reset();
		query << C_START("oTeam")
			<< C_STRING("Name")  << C_STRING("Runners", 256)
			<< C_INT("Club") << C_INT("Class") 
			<< C_INT("StartTime") << C_INT("FinishTime")
			<< C_INT("Status") << C_INT("StartNo") 
			<< oe->oTeamData->generateSQLDefinition() << C_END();
		query.execute();

		query.reset();
		query << C_START("oPunch")
			<< C_INT("CardNo")
			<< C_UINT("Time")
			<< C_UINT("Type") << C_END();
		query.execute();

    query.reset();
		query << C_START("oMonitor")
			<< C_STRING("Client")
			<< C_UINT("Count")
      << C_END();
		query.execute();

    query.reset();
    query << "CREATE TABLE IF NOT EXISTS oCounter ("
          << "CounterId INT NOT NULL, "
          << C_UINT("oControl")
          << C_UINT("oCourse")
          << C_UINT("oClass")
          << C_UINT("oCard")
          << C_UINT("oClub")
          << C_UINT("oPunch")
          << C_UINT("oRunner")
          << C_UINT("oTeam")
          << C_UINT("oEvent")
          << " Modified TIMESTAMP) ENGINE = MyISAM";
		query.execute();
    
    mysqlpp::Result res = query.store("SELECT CounterId FROM oCounter");
    if (res.num_rows()==0) {
      query.reset();
      query << "INSERT INTO oCounter SET CounterId=1, oPunch=1, oTeam=1, oRunner=1";
      query.execute();
    }

    // Create runner/club DB
    createRunnerDB(oe, query); 
	}
	catch (const mysqlpp::Exception& er){
		alert(string(er.what()) + " MySQL Error.");
		return 0;
	}

	return true;
}

bool MeosSQL::getErrorMessage(char *bf)
{
  strcpy_s(bf, 256, errorMessage.c_str());
  return !errorMessage.empty();
}

bool MeosSQL::closeDB()
{
  CmpDataBase="";
  errorMessage.clear();

  try {
    con.close();
  }	
  catch (const mysqlpp::Exception&) {
	}

  return true;
}


//CAN BE RUN IN A SEPARTE THREAD. Access nothing without thinking...
//No other MySQL-call will take place in parallell.
bool MeosSQL::reConnect()
{
  errorMessage.clear();

  if(CmpDataBase.empty()) {
    errorMessage="No database selected.";
    return false;
  }

  try {
    con.close();
  }	
  catch (const mysqlpp::Exception&) {
	}

	try {
		con.connect("", serverName.c_str(), serverUser.c_str(), 
					serverPassword.c_str(), serverPort);
	}
	catch (const Exception& er) {
		errorMessage=er.what();
		return false;
	}

	try {
	  con.select_db(CmpDataBase);
  }
	catch (const Exception& er) {
		errorMessage=er.what();
		return false;
	}

  return true;
}

OpFailStatus MeosSQL::SyncUpdate(oEvent *oe)
{
  errorMessage.clear();
  if(CmpDataBase.empty())
		return opStatusFail;

	try{
		con.select_db("MeOSMain");

		mysqlpp::Query queryset = con.query();
		queryset << "UPDATE oEvent SET Name=" << quote << oe->Name << ", "
				<< " Date="  << quote << oe->Date << ", "
				<< " NameId="  << quote << oe->CurrentNameId << ", "
				<< " ZeroTime=" << oe->ZeroTime
        << " WHERE Id=" << oe->Id; 
		
    queryset.execute();
		//syncUpdate(queryset, "oEvent", oe, true);
	}
	catch (const mysqlpp::Exception& er){
		alert(string(er.what())+" [UPDATING oEvent]");
    setDefaultDB();
		return opStatusFail;
	}

	try{
		con.select_db(CmpDataBase);
	}
	catch (const mysqlpp::Exception& er){
		alert(string(er.what())+" [OPENING] "+CmpDataBase);
		return opStatusFail;
	}

  {
    mysqlpp::Query queryset = con.query();
  	queryset << " Name=" << quote << oe->Name << ", "
				<< " Date="  << quote << oe->Date << ", "
				<< " NameId="  << quote << oe->CurrentNameId << ", "
				<< " ZeroTime=" << oe->ZeroTime << ", "
				<< " BuildVersion=" << buildVersion
        <<  oe->getDI().generateSQLSet();

		syncUpdate(queryset, "oEvent", oe);
  }
  
  con.query().exec("DELETE FROM oCard");
	{
		list<oCard>::iterator it=oe->Cards.begin();
		while(it!=oe->Cards.end()){
			syncUpdate(&*it);
			++it;
		}
	}

  con.query().exec("DELETE FROM oClub");
	{
		list<oClub>::iterator it=oe->Clubs.begin();
		while(it!=oe->Clubs.end()){
			syncUpdate(&*it);
			++it;
		}
	}
  con.query().exec("DELETE FROM oControl");
	{
		list<oControl>::iterator it=oe->Controls.begin();
		while(it!=oe->Controls.end()){
			syncUpdate(&*it);
			++it;
		}
	}
  con.query().exec("DELETE FROM oCourse");
	{
		list<oCourse>::iterator it=oe->Courses.begin();
		while(it!=oe->Courses.end()){
			syncUpdate(&*it);
			++it;
		}
	}
  con.query().exec("DELETE FROM oClass");
	{
		list<oClass>::iterator it=oe->Classes.begin();
		while(it!=oe->Classes.end()){
			syncUpdate(&*it);
			++it;
		}
	}
  con.query().exec("DELETE FROM oRunner");
	{
		list<oRunner>::iterator it=oe->Runners.begin();
		while(it!=oe->Runners.end()){
			syncUpdate(&*it);
			++it;
		}
	}

  con.query().exec("DELETE FROM oTeam");
	{
		list<oTeam>::iterator it=oe->Teams.begin();
		while(it!=oe->Teams.end()){
			syncUpdate(&*it);
			++it;
		}
	}

  con.query().exec("DELETE FROM oPunch");
	{
		list<oFreePunch>::iterator it=oe->punches.begin();
		while(it!=oe->punches.end()){
			syncUpdate(&*it);
			++it;
		}
	}
  return opStatusOK;
}

OpFailStatus MeosSQL::uploadRunnerDB(oEvent *oe)
{
  errorMessage.clear();
  if(CmpDataBase.empty())
		return opStatusFail;

  ProgressWindow pw(oe->hWnd());
  try {
    const vector<oClub> &cdb = oe->runnerDB->getClubDB();
    size_t size = cdb.size();

    const vector<RunnerDBEntry> &rdb = oe->runnerDB->getRunnerDB();
    
    if (cdb.size() + rdb.size() > 2000)
      pw.init();

    size_t tz = cdb.size() + rdb.size();
    int s1 = (1000 * cdb.size())/tz;
    int s2 = (1000 * rdb.size())/tz;

    // Reset databases
    con.query().exec("DELETE FROM dbClub");
	  con.query().exec("DELETE FROM dbRunner");
  	  
    for (size_t k = 0; k<size; k++) {
      mysqlpp::Query query = con.query();
      string setId = "Id=" + itos(cdb[k].Id) + ", ";
      query << "INSERT INTO dbClub SET " << setId << "Name=" << quote << cdb[k].name 
        <<  cdb[k].getDCI().generateSQLSet();

      query.execute();

      if (k%200 == 150)
        pw.setProgress((k*s1)/size);
    }

    size = rdb.size();
    for (size_t k = 0; k<size; k++) {
      mysqlpp::Query query = con.query();
      query << "INSERT INTO dbRunner SET " << 
        "Name=" << quote << rdb[k].name << 
        ", ExtId=" << rdb[k].extId << ", Club="  << rdb[k].clubNo <<
        ", CardNo=" << rdb[k].cardNo << ", Sex=" << quote << rdb[k].getSex() <<
        ", Nation=" << quote << rdb[k].getNationality() << ", BirthYear=" << rdb[k].birthYear;

      query.execute();
      if (k%200 == 150)
        pw.setProgress(s1 + (k*s2)/size);
    }

    Result cnt = con.query().store("SELECT DATE_FORMAT(NOW(),'%Y-%m-%d %H:%i:%s')");
    string dateTime = cnt.at(0).at(0);
    oe->runnerDB->setDataDate(dateTime);
  }
  catch (const mysqlpp::Exception& er) {
    errorMessage = er.what();
    return opStatusFail;
  }
  return opStatusOK;
}

bool MeosSQL::storeData(oDataInterface &odi, const Row &row)
{
  errorMessage.clear();
	list<oVariableInt> varint;
	list<oVariableString> varstring;
	bool success=true;

	try{
		odi.getVariableInt(varint);
		list<oVariableInt>::iterator it_int;
		for(it_int=varint.begin(); it_int!=varint.end(); it_int++)
			*(it_int->Data)=int(row[it_int->Name]);
		
	}
	catch(const BadFieldName&){
		success=false;
	}

	try{
		odi.getVariableString(varstring);
		list<oVariableString>::iterator it_string;
		for(it_string=varstring.begin(); it_string!=varstring.end(); it_string++)
			strncpy_s(it_string->Data, it_string->MaxSize, row[it_string->Name].c_str(), it_string->MaxSize-1);
	}
	catch(const BadFieldName&){ 
		success=false;
	}

	return success;
}

OpFailStatus MeosSQL::SyncRead(oEvent *oe)
{
  errorMessage.clear();

  if(CmpDataBase.empty())
    return opStatusFail;

	if(!oe || !con.connected())
		return opStatusFail;

  if (oe->HasDBConnection) {
    //We already have established connectation, and just want to sync data.
    return SyncEvent(oe);
  }
  warnedOldVersion=false;

	if(!oe->Id) return SyncUpdate(oe);

  ProgressWindow pw(oe->hWnd());

	try {
		con.select_db("MeOSMain");

		Query query = con.query(); 
		query << "SELECT * FROM oEvent WHERE Id=" << oe->Id;		
		Result res = query.store();

		Row row;
		if(row=res.at(0)){		
			oe->Name=row["Name"];
			oe->Date=row["Date"];
			oe->ZeroTime=row["ZeroTime"];
			strcpy_s(oe->CurrentNameId, row["NameId"].c_str());
    }

		con.select_db(CmpDataBase);
	}
	catch (const mysqlpp::Exception& er){
    setDefaultDB();
		alert(string(er.what())+" [SYNCREAD oEvent]");
		return opStatusFail;
	}
	
	Query query = con.query(); 

  int nRunner = 0;
  int nCard = 0;
  int nTeam = 0;
  int nClubDB = 0;
  int nRunnerDB = 0;

  int nSum = 1;

	try {
    mysqlpp::Result cnt = query.store("SELECT COUNT(*) FROM dbClub");
    nClubDB = cnt.at(0).at(0);

    cnt = query.store("SELECT COUNT(*) FROM dbRunner");
    nRunnerDB = cnt.at(0).at(0);
    
    string time = oe->runnerDB->getDataDate();

    cnt = query.store("SELECT COUNT(*) FROM dbClub WHERE Modified>'" + time + "'");
    int modclub = cnt.at(0).at(0);
    cnt = query.store("SELECT COUNT(*) FROM dbRunner WHERE Modified>'" + time + "'");
    int modrunner = cnt.at(0).at(0);
    
    bool skipDB = modclub==0 && modrunner==0 && nClubDB == oe->runnerDB->getClubDB().size() &&
                                      nRunnerDB == oe->runnerDB->getRunnerDB().size();

    if (skipDB) {
      nClubDB = 0;
      nRunnerDB = 0;
    }

    cnt = query.store("SELECT COUNT(*) FROM oRunner");
    nRunner = cnt.at(0).at(0);

    cnt = query.store("SELECT COUNT(*) FROM oCard");
    nCard = cnt.at(0).at(0);

    cnt = query.store("SELECT COUNT(*) FROM oTeam");
    nTeam = cnt.at(0).at(0);

    nSum = nClubDB + nRunnerDB + nRunner + nTeam + nCard + 50;

    if (nSum > 1000)
      pw.init();
  }
	catch (const mysqlpp::Exception& er){
		alert(string(er.what())+" [SYNCREAD INFO]");
		return opStatusFail;
  }

  int pStart = 0, pPart = 50;

	try {		  
    //Update oEvent
	  query << "SELECT * FROM oEvent";		
	  Result res = query.store();

	  Row row;
	  if (row=res.at(0)) {		
		  oe->Name = row["Name"];
		  oe->Date = row["Date"];
		  oe->ZeroTime = row["ZeroTime"];
		  strcpy_s(oe->CurrentNameId, row["NameId"].c_str());

      oe->sqlUpdated = row["Modified"];
      oe->counter = row["Counter"];
      
		  oDataInterface odi=oe->getDI();
		  storeData(odi, row);

      oe->setCurrency(-1, "");
    }
  }
	catch (const EndOfResults& ) {		
	}
	catch (const mysqlpp::Exception& er){
		alert(string(er.what())+" [SYNCREAD oEvent/Club]");
		return  opStatusFail;
	}
	
  pw.setProgress(20);

  pw.setProgress(2);
  try {
    ResUse res = query.use("SELECT * FROM oClub WHERE Removed=0");

		// Retreive result rows one by one.
		if (res){		
			// Get each row in result set.
			Row row;
			
			while (row = res.fetch_row()) {
				oClub c(oe, row["Id"]);
        storeClub(row, c);
        oe->addClub(c);

        oe->sqlUpdateClubs = max(c.sqlUpdated, oe->sqlUpdateClubs);
        oe->sqlCounterClubs = max(c.counter, oe->sqlCounterClubs);
			}
		}
	}
	catch (const EndOfResults& ) {		
	}
	catch (const mysqlpp::Exception& er){
		alert(string(er.what())+" [SYNCREAD oEvent/Club]");
		return opStatusFail;
	}

  pw.setProgress(30);

  oe->sqlCounterControls=0;
	try {		
		ResUse res = query.use("SELECT * FROM oControl WHERE Removed=0");

		if (res) {		
			// Get each row in result set.
			Row row;
			
			while (row = res.fetch_row()) {
				oControl c(oe, row["Id"]);
        storeControl(row, c);
				oe->Controls.push_back(c);

        oe->sqlUpdateControls = max(c.sqlUpdated, oe->sqlUpdateControls);
        oe->sqlCounterControls = max(c.counter, oe->sqlCounterControls);
			}
		}
	}
	catch (const EndOfResults& ) {		
	}
	catch (const mysqlpp::Exception& er){
		alert(string(er.what())+" [SYNCREAD oEvent/Control]");
		return  opStatusFail;
	}

  oe->sqlCounterCourses = 0;
  pw.setProgress(40);

	try{		
		ResUse res = query.use("SELECT * FROM oCourse WHERE Removed=0");

		if (res){		
			// Get each row in result set.
			Row row;
			
			while (row = res.fetch_row()) {
				oCourse c(oe, row["Id"]);
        storeCourse(row, c, false);
        oe->addCourse(c);
				
        oe->sqlUpdateCourses = max(c.sqlUpdated, oe->sqlUpdateCourses);
        oe->sqlCounterCourses = max(c.counter, oe->sqlCounterCourses);
			}
		}
	}
	catch (const EndOfResults& ) {		
	}
	catch (const mysqlpp::Exception& er){
		alert(string(er.what())+" [SYNCREAD oEvent/Course]");
		return opStatusFail;
	}

  pw.setProgress(50);

  oe->sqlCounterClasses = 0;
	try{		
		ResUse res = query.use("SELECT * FROM oClass WHERE Removed=0");

		if (res) {
			Row row;
			while (row = res.fetch_row()) {
				oClass c(oe, row["Id"]);
        storeClass(row, c, false);

        c.changed = false;
        c.reChanged = false;
        oe->addClass(c);

        oe->sqlUpdateClasses = max(c.sqlUpdated, oe->sqlUpdateClasses);
        oe->sqlCounterClasses = max(c.counter, oe->sqlCounterClasses);
			}
		}
	}
	catch (const EndOfResults& ) {		
	}
	catch (const mysqlpp::Exception& er){
		alert(string(er.what())+" [SYNCREAD oEvent/Class]");
		return opStatusFail;
	}

  oe->sqlCounterCards=0;

	try{		
		ResUse res = query.use("SELECT * FROM oCard WHERE Removed=0");
    int counter = 0;
    pStart += pPart;
    pPart = (1000 * nCard) / nSum;

		if (res){		
			Row row;			
			while (row = res.fetch_row()) {
				oCard c(oe, row["Id"]);
        storeCard(row, c);
				oe->addCard(c);
        assert(!c.changed);
        
        oe->sqlCounterCards = max(c.counter, oe->sqlCounterCards);
        oe->sqlUpdateCards = max(c.sqlUpdated, oe->sqlUpdateCards);

        if (++counter % 100 == 50)
          pw.setProgress(pStart + (counter * pPart) / nCard);
			}
		}

	}
	catch (const EndOfResults& ) {		
	}
	catch (const mysqlpp::Exception& er){
		alert(string(er.what())+" [SYNCREAD oEvent/Card]");
		return opStatusFail;
	}

  oe->sqlCounterRunners = 0;
	try{		
		ResUse res = query.use("SELECT * FROM oRunner WHERE Removed=0");
    int counter = 0;
    pStart += pPart;
    pPart = (1000 * nRunner) / nSum;

		if (res){		
			Row row;
			while (row = res.fetch_row()) {
				oRunner r(oe, row["Id"]);
        storeRunner(row, r, false, false, false);
        assert(!r.changed);
        oe->addRunner(r);

        oe->sqlUpdateRunners = max(r.sqlUpdated, oe->sqlUpdateRunners);
        oe->sqlCounterRunners = max(r.counter,oe->sqlCounterRunners);
        
        if (++counter % 100 == 50)
          pw.setProgress(pStart + (counter * pPart) / nRunner);
			}
		}
	}
	catch (const EndOfResults& ) {		
	}
	catch (const mysqlpp::Exception& er){
		alert(string(er.what())+" [SYNCREAD oEvent/Runner]");
		return opStatusFail;
	}

  oe->sqlCounterTeams=0;

	try{		
		ResUse res = query.use("SELECT * FROM oTeam WHERE Removed=0");
    int counter = 0;
    pStart += pPart;
    pPart = (1000 * nTeam) / nSum;
    
		if (res){		
			Row row;
			while (row = res.fetch_row()) {
        oTeam t(oe, row["Id"]);

        storeTeam(row, t, false);
				
        pTeam at = oe->addTeam(t);
  			
        if (at) {
          at->apply(false, 0);
          at->changed = false;
          for (size_t k = 0; k<at->Runners.size(); k++) {
            if (at->Runners[k]) {
              assert(!at->Runners[k]->changed);
              at->Runners[k]->changed = false;
            }
          }
        }
        oe->sqlUpdateTeams = max(t.sqlUpdated, oe->sqlUpdateTeams); 
        oe->sqlCounterTeams = max(t.counter, oe->sqlCounterTeams);

        if (++counter % 100 == 50)
          pw.setProgress(pStart + (counter * pPart) / nTeam);
			}
		}
	}
	catch (const EndOfResults& ) {		
	}
	catch (const mysqlpp::Exception& er){
		alert(string(er.what())+" [SYNCREAD oEvent/Team]");
		return opStatusFail;
	}

  string dateTime;
  try { 
    if (nClubDB == 0 && nRunnerDB == 0)
      return opStatusOK; // Not  modified

    Result cnt;
    // Note dbRunner is stored after dbClub
    if (nRunnerDB>0)
      cnt = query.store("SELECT DATE_FORMAT(MAX(Modified),'%Y-%m-%d %H:%i:%s') FROM dbRunner");
    else
      cnt = query.store("SELECT DATE_FORMAT(NOW(),'%Y-%m-%d %H:%i:%s')");
    
    dateTime = cnt.at(0).at(0);
   
    oe->runnerDB->prepareLoadFromServer(nRunnerDB, nClubDB);

		ResUse res = query.use("SELECT * FROM dbClub");
    int counter = 0;
    pStart += pPart;
    pPart = (1000 * nClubDB) / nSum;

		if (res) {		
			Row row;
			while (row = res.fetch_row()) {
        oClub t(oe, row["Id"]);
        
        string n = row["Name"];
        t.internalSetName(n);
        storeData(t.getDI(), row);

        oe->runnerDB->addClub(t);

        if (++counter % 100 == 50)
          pw.setProgress(pStart + (counter * pPart) / nClubDB);
			}
		}
	}
	catch (const EndOfResults& ) {		
	}
	catch (const mysqlpp::Exception& er){
		alert(string(er.what())+" [SYNCREAD dbClub]");
		return opStatusFail;
	}

  try {
		ResUse res = query.use("SELECT * FROM dbRunner");
    int counter = 0;
    pStart += pPart;
    pPart = (1000 * nRunnerDB) / nSum;

    if (res) {
			Row row;
			while (row = res.fetch_row()) {
        string name = row["Name"];
        string ext = row["ExtId"];
        string club = row["Club"];
        string card = row["CardNo"];
        string sex = row["Sex"];
        string nat = row["Nation"];
        string birth = row["BirthYear"];
        RunnerDBEntry *db = oe->runnerDB->addRunner(name.c_str(), _atoi64(ext.c_str()), 
                              atoi(club.c_str()), atoi(card.c_str()));
        if (db) {
          if (sex.length()==1)
            db->sex = sex[0];
          db->birthYear = atoi(birth.c_str());
          
          if (nat.length()==3) {
            db->national[0] = nat[0];
            db->national[1] = nat[1];
            db->national[2] = nat[2];
          }
        }

        if (++counter % 100 == 50)
          pw.setProgress(pStart + (counter * pPart) / nRunnerDB);

			}
		}
	}
	catch (const EndOfResults& ) {		
	}
	catch (const mysqlpp::Exception& er) {
		alert(string(er.what())+" [SYNCREAD dbRunner]");
		return opStatusFail;
	}

  oe->runnerDB->setDataDate(dateTime);

	return opStatusOK;
}

void MeosSQL::storeClub(const Row &row, oClub &c)
{
  string n = row["Name"];
  c.internalSetName(n);

  c.sqlUpdated = row["Modified"];
  c.counter = row["Counter"];
  c.Removed = row["Removed"];

  c.oe->sqlChangedClubs = true; 
  storeData(c.getDI(), row);
}

void MeosSQL::storeControl(const Row &row, oControl &c)
{
	c.Name = row["Name"];
	c.decodeNumbers(string(row["Numbers"]));				
  c.Status = oControl::ControlStatus(int(row["Status"]));

  c.sqlUpdated = row["Modified"];
  c.counter = row["Counter"];
  c.Removed = row["Removed"];

  c.oe->sqlChangedControls = true;
  storeData(c.getDI(), row);
}

void MeosSQL::storeCard(const Row &row, oCard &c)
{
	c.CardNo = row["CardNo"];
	c.ReadId = row["ReadId"];				
	c.importPunches(string(row["Punches"]));					
  
  c.sqlUpdated = row["Modified"];
  c.counter = row["Counter"];
  c.Removed = row["Removed"];

  c.oe->sqlChangedCards = true;
}

void MeosSQL::storePunch(const Row &row, oFreePunch &p)
{
  p.setCardNo(row["CardNo"], true);
	p.Time=row["Time"];
	p.setType(string(row["Type"]), true);

  p.sqlUpdated = row["Modified"];
  p.counter = row["Counter"];
  p.Removed = row["Removed"];

  pRunner r = p.oe->getRunner(p.tRunnerId, 0);
  if (r)
    r->markClassChanged();

 	p.oe->sqlChangedPunches = true;
}

OpFailStatus MeosSQL::storeClass(const Row &row, oClass &c,
                                  bool readCourses)
{
  OpFailStatus success = opStatusOK;

  c.Name=row["Name"];				
  string multi = row["MultiCourse"];

	string lm(row["LegMethod"]);
  c.importLegMethod(lm);

  set<int> cid;
  vector< vector<int> > multip;
  oClass::parseCourses(multi, multip, cid);
  success = min(success, syncReadClassCourses(&c, row["Course"], cid, readCourses));
  c.importCourses(multip);

	storeData(c.getDI(), row);

  c.sqlUpdated = row["Modified"];
  c.counter = row["Counter"];
  c.Removed = row["Removed"];

  storeData(c.getDI(), row);
  c.changed = false;

  c.sqlChanged = true;
  c.oe->sqlChangedClasses = true;
  return success;
}

OpFailStatus MeosSQL::storeCourse(const Row &row, oCourse &c,
                                  bool readControls)
{
  OpFailStatus success = opStatusOK;
  
  c.Name = row["Name"];
  c.importControls(string(row["Controls"]));
  c.Length = row["Length"];
  c.importLegLengths(string(row["Legs"]));

	for (int i=0;i<c.nControls; i++) {
    if (c.Controls[i]) {
      // Might have been created during last call.
      // Then just read to update
      if (!c.Controls[i]->existInDB()) {
        c.Controls[i]->changed = false;
        success = min(success, syncRead(true, c.Controls[i]));
      }
      else if (readControls)
		    success = min(success, syncRead(false, c.Controls[i]));
    }
	}

  c.sqlUpdated = row["Modified"];
  c.counter = row["Counter"];
  c.Removed = row["Removed"];

  storeData(c.getDI(), row);
  c.changed = false;
  c.oe->sqlChangedCourses = true;
  return success;
}

OpFailStatus MeosSQL::storeRunner(const Row &row, oRunner &r, 
                                  bool readCourseCard, 
                                  bool readClassClub, 
                                  bool readRunners)
{
  OpFailStatus success = opStatusOK;
  oEvent *oe=r.oe;

  // Mark old class as changed
  if (r.Class)
    r.Class->sqlChanged = true;

	r.Name = row["Name"];
	r.CardNo = row["CardNo"];
	r.StartNo = row["StartNo"];
	r.StartTime = row["StartTime"];
	r.FinishTime = row["FinishTime"];
	r.Status = RunnerStatus(int(row["Status"]));
	
  r.inputTime = row["InputTime"];
	r.inputPoints = row["InputPoints"];
	r.inputStatus = RunnerStatus(int(row["InputStatus"]));
	r.inputPlace = row["InputPlace"];

  r.Removed = row["Removed"];
	r.sqlUpdated = row["Modified"];
  r.counter = row["Counter"];

  storeData(r.getDI(), row);
	
	if (int(row["Course"])!=0) {
		r.Course = oe->getCourse(int(row["Course"]));
		if (!r.Course) {
			oCourse oc(oe,  row["Course"]);
      success = min(success, syncRead(true, &oc, readCourseCard));
			r.Course = oe->addCourse(oc);
		}
    else if (readCourseCard)
      success = min(success, syncRead(false, r.Course, true));					
	}
	else r.Course=0;

	if (int(row["Class"])!=0) {
		r.Class=oe->getClass(int(row["Class"]));
		
    if (!r.Class) {
      oClass oc(oe, row["Class"]);
      success = min(success, syncRead(true, &oc, readClassClub));
      r.Class = oe->addClass(oc);
    }
    else if (readClassClub)
		  success = min(success, syncRead(false, r.Class, true));
    
    if (r.tInTeam && r.tInTeam->Class!=r.Class)
      r.tInTeam = 0; //Temporaraly disable belonging. Restored on next apply.
	}
	else r.Class=0;
		
	if(int(row["Club"])!=0){
		r.Club = oe->getClub(int(row["Club"]));

		if (!r.Club) {
			oClub oc(oe, row["Club"]);
      success = min(success, syncRead(true, &oc));
			r.Club = oe->addClub(oc);
		}
    else if (readClassClub)
		  success = min(success, syncRead(false, r.Club));
	}
	else r.Club=0;

  pCard oldCard = r.Card;

	if (int(row["Card"])!=0) {
		r.Card = oe->getCard(int(row["Card"]));

		if(!r.Card){
			oCard oc(oe, row["Card"]);
			oe->Cards.push_back(oc);
			r.Card = &oe->Cards.back();
      r.Card->changed = false;
      success = min(success, syncRead(true, r.Card));
		}
		else if (readCourseCard)
      success = min(success, syncRead(false, r.Card));						
	}
	else r.Card=0;

  // Update card ownership
  if (oldCard && oldCard != r.Card && oldCard->tOwner == &r)
    oldCard->tOwner = 0;

  // This is updated by addRunner if this is a temporary copy.
  if(r.Card)
    r.Card->tOwner=&r;

  // This only loads indexes
  r.decodeMultiR(string(row["MultiR"]));
  
  // We now load/reload required other runners.
  if (readRunners) {
    for (size_t i=0;i<r.multiRunnerId.size();i++) {
      int rid = r.multiRunnerId[i];
      if (rid>0) {
        pRunner pr = oe->getRunner(rid, 0);
        if (pr==0) {
          oRunner or(oe, rid);
          success = min(success, syncRead(true, &or, false, readCourseCard));
          pr = oe->addRunner(or);          
        }
        else
          success = min(success, syncRead(false, pr, false, readCourseCard));
      }
    }
  }

  // Mark new class as changed
  if (r.Class)
    r.Class->sqlChanged = true;

  r.sqlChanged = true;
  r.oe->sqlChangedRunners = true;

  return success;
}

OpFailStatus MeosSQL::storeTeam(const Row &row, oTeam &t, 
                                bool readRecursive)
{
  oEvent *oe=t.oe;
  OpFailStatus success = opStatusOK;

  // Mark old class as changed
  if (t.Class)
    t.Class->sqlChanged = true;

  t.Name=row["Name"];
	t.StartNo=row["StartNo"];
	t.StartTime=row["StartTime"];
	t.FinishTime=row["FinishTime"];
  t.Status=RunnerStatus(int(row["Status"]));				
	storeData(t.getDI(), row);

  t.Removed = row["Removed"];
	t.sqlUpdated = row["Modified"];
  t.counter = row["Counter"];

  int classId = row["Class"];
	if (classId!=0) {
    t.Class = oe->getClass(classId);

    if (!t.Class) {
      oClass oc(oe, classId);
      success = min(success, syncRead(true, &oc, readRecursive));
      t.Class = oe->addClass(oc);
    }
    else if(readRecursive)
		  success = min(success, syncRead(false, t.Class, readRecursive));
	}
	else t.Class=0;
			
  int clubId = row["Club"];
  if (clubId!=0) {
	  t.Club=oe->getClub(clubId);

		if (!t.Club) {
		  oClub oc(oe, clubId);
      success = min(success, syncRead(true, &oc));
      t.Club = oe->addClub(oc);
		}
    else if (readRecursive)
		  success = min(success, syncRead(false, t.Club));
	}
	else t.Club = 0;

  vector<int> rns;
  vector<pRunner> pRns;
	t.decodeRunners(static_cast<string>(row["Runners"]), rns);

  pRns.resize(rns.size());
  for (size_t k=0;k<rns.size(); k++) {
    if (rns[k]>0) {
      pRns[k] = oe->getRunner(rns[k], 0);

      if (!pRns[k]) {
        oRunner or(oe, rns[k]);
        success = min(success, syncRead(true, &or, readRecursive, readRecursive));

        if (or.Name.empty())
          or.Name = "@AutoCorrection";

        pRns[k] = oe->addRunner(or);
        assert(pRns[k] && !pRns[k]->changed);
      }
      else if (readRecursive)
        success = min(success, syncRead(false, pRns[k]));
    }
  }
  t.importRunners(pRns);
	
  // Mark new class as changed.
  if (t.Class)
    t.Class->sqlChanged = true;

  t.sqlChanged = true;
	t.oe->sqlChangedTeams = true;
  return success;
}  	

bool MeosSQL::Remove(oBase *ob)
{
  errorMessage.clear();

  if(CmpDataBase.empty())
		return false;

	if(!ob || !con.connected())
		return false;

	if(!ob->Id)
		return true; //Not in DB.

	Query query = con.query();
	
	string oTable;

	if(typeid(*ob)==typeid(oRunner)){		
		oTable="oRunner";
	}
	else if(typeid(*ob)==typeid(oClass)){
		oTable="oClass";
	}
	else if(typeid(*ob)==typeid(oCourse)){
		oTable="oCourse";
	}
	else if(typeid(*ob)==typeid(oControl)){
		oTable="oControl";
	}
	else if(typeid(*ob)==typeid(oClub)){
		oTable="oClub";
	}
	else if(typeid(*ob)==typeid(oCard)){
		oTable="oCard";
	}
  else if(typeid(*ob)==typeid(oFreePunch)){
		oTable="oPunch";
	}
  else if(typeid(*ob)==typeid(oTeam)){
		oTable="oTeam";
	}
	else if(typeid(*ob)==typeid(oEvent)){
		oTable="oEvent";
		//Must change db!
		return 0;
	}
	
	query << "UPDATE " << oTable << " SET Removed=1 WHERE Id= " << ob->Id;

	try{
		ResNSel res=query.execute();
		ob->Removed = true;
		ob->changed = false;
    ob->reChanged = false;
	}
	catch (const mysqlpp::Exception& er){
		alert(string(er.what())+" [REMOVE " +oTable +"]");
		return false;
	}	

	return true;
}


OpFailStatus MeosSQL::syncUpdate(oRunner *r)
{
  errorMessage.clear();

  if(CmpDataBase.empty())
		return opStatusFail;

	if(!r || !con.connected())
		return opStatusFail;

	mysqlpp::Query queryset = con.query();
	queryset << " Name=" << quote << r->Name << ", "
			<< " CardNo=" << r->CardNo << ", "
			<< " StartNo=" << r->StartNo << ", "
			<< " StartTime=" << r->StartTime << ", "
			<< " FinishTime=" << r->FinishTime << ", "
			<< " Course=" << r->getCourseId() << ", "
			<< " Class=" << r->getClassId() << ", "
			<< " Club=" << r->getClubId() << ", "
			<< " Card=" << r->getCardId() << ", "
			<< " Status=" << r->getStatus() << ", "
      << " InputTime=" << r->inputTime << ", "
      << " InputStatus=" << r->inputStatus << ", "
      << " InputPoints=" << r->inputPoints << ", "
      << " InputPlace=" << r->inputPlace << ", "
      << " MultiR=" << quote << r->codeMultiR()
			<< r->getDI().generateSQLSet();

	return syncUpdate(queryset, "oRunner", r);
}

bool MeosSQL::isOld(int counter, const string &time, oBase *ob)
{
  return counter>ob->counter || time>ob->sqlUpdated;
}

OpFailStatus MeosSQL::syncRead(bool forceRead, oRunner *r)
{
  return syncRead(forceRead, r, true, true);
}

OpFailStatus MeosSQL::syncRead(bool forceRead, oRunner *r, bool readClassClub, bool readCourseCard)
{
  errorMessage.clear();
  if(CmpDataBase.empty())
		return opStatusFail;

	if(!r || !con.connected())
		return opStatusFail;

	if(!forceRead  && !r->existInDB()) 
    return syncUpdate(r);

	try {
		Query query = con.query(); 
		query << "SELECT * FROM oRunner WHERE Id=" << r->Id;		
		Result res = query.store();
		
		Row row;
		if(!res.empty()){
			row=res.at(0);
      if (!r->changed || isOld(row["Counter"], string(row["Modified"]), r)) {

        OpFailStatus success=opStatusOK;
				if (r->changed)
          success=opStatusWarning;

        success = min (success, storeRunner(row, *r, readCourseCard, readClassClub, true));

				r->Modified.Update();
				r->changed = false;
				
				vector<int> mp;
				r->evaluateCard(mp, 0, false);

        //Forget evaluated changes. Not our buisness to update.
        r->changed = false;

				return success;
			}			
			else {
				if(r->Card && readCourseCard) 
					syncRead(false, r->Card);
				if(r->Class && readClassClub)
					syncRead(false, r->Class, readClassClub);
				if(r->Course && readCourseCard)
					syncRead(false, r->Course, readClassClub);
				if(r->Club && readClassClub)
					syncRead(false, r->Club);
			
				if(r->changed)
					return syncUpdate(r);				

				vector<int> mp;
				r->evaluateCard(mp, 0, false);
        r->changed = false;
        r->reChanged = false;
				return  opStatusOK;
			}
		} 
		else{
			//Something is wrong!?
			return syncUpdate(r);
		}
	
		return  opStatusOK;
	}
	catch (const mysqlpp::Exception& er){
		alert(string(er.what())+" [SYNCREAD oRunner]");
		return opStatusFail;
	}

	return opStatusFail;
}

OpFailStatus MeosSQL::syncUpdate(oCard *c)
{
  errorMessage.clear();
  if(CmpDataBase.empty())
		return opStatusFail;

	if(!c || !con.connected())
		return opStatusFail;

	mysqlpp::Query queryset = con.query();
	queryset << " CardNo=" << c->CardNo << ", "
			<< " ReadId=" << c->ReadId << ", "
			<< " Punches=" << quote << c->getPunchString();
			

	return syncUpdate(queryset, "oCard", c);
}

OpFailStatus MeosSQL::syncRead(bool forceRead, oCard *c)
{
  errorMessage.clear();
  if(CmpDataBase.empty())
		return opStatusFail;

	if(!c || !con.connected())
		return opStatusFail;

  if(!forceRead && !c->existInDB()) 
    return syncUpdate(c);

	try{
		Query query = con.query(); 
		query << "SELECT * FROM oCard WHERE Id=" << c->Id;		
		Result res = query.store();
		
		Row row;
		if(!res.empty()){
			row=res.at(0);
			if(!c->changed || isOld(row["Counter"], string(row["Modified"]), c)){

        OpFailStatus success=opStatusOK;
				if(c->changed)
          success=opStatusWarning;
	
        storeCard(row, *c);

        c->Modified.Update();
				c->changed=false;
				return success;
			}
			else if(c->changed){
				return syncUpdate(c);				
			}
		}
		else{
			//Something is wrong!? Deleted?
			return syncUpdate(c);
		}
	
		return  opStatusOK;
	}
	catch (const mysqlpp::Exception& er){
		alert(string(er.what())+" [SYNCREAD oCard]");
		return opStatusFail;
	}

	return opStatusFail;
}


OpFailStatus MeosSQL::syncUpdate(oTeam *t)
{
  errorMessage.clear();

  if(CmpDataBase.empty())
		return opStatusFail;

	if(!t || !con.connected())
		return opStatusFail;

	mysqlpp::Query queryset = con.query();

	queryset << " Name=" << quote << t->Name << ", "
			<< " Runners=" << quote << t->getRunners() << ", "
			<< " StartTime=" << t->StartTime << ", "
			<< " FinishTime=" << t->FinishTime << ", "
			<< " Class=" << t->getClassId() << ", "
			<< " Club=" << t->getClubId() << ", "
			<< " StartNo=" << t->getStartNo() << ", "
      << " Status=" << t->Status
			<< t->getDI().generateSQLSet();			

	return syncUpdate(queryset, "oTeam", t);
}

OpFailStatus MeosSQL::syncRead(bool forceRead, oTeam *t)
{
  return syncRead(forceRead, t, true);
}

OpFailStatus MeosSQL::syncRead(bool forceRead, oTeam *t, bool readRecursive)
{
  errorMessage.clear();

  if(CmpDataBase.empty())
		return opStatusFail;

	oEvent *oe=t->oe;

	if(!t || !con.connected())
		return opStatusFail;

  if(!forceRead && !t->existInDB()) 
    return syncUpdate(t);

	try{
		Query query = con.query(); 
		query << "SELECT * FROM oTeam WHERE Id=" << t->Id;		
		Result res = query.store();
		
		Row row;
		if(!res.empty()){
			row=res.at(0);
			if(!t->changed || isOld(row["Counter"], string(row["Modified"]), t)){

        OpFailStatus success=opStatusOK;
				if(t->changed)
          success=opStatusWarning;
								
        storeTeam(row, *t, readRecursive);
				t->Modified.Update();
				t->changed = false;
        t->reChanged = false;
				return success;
			}
			else {
        OpFailStatus success = opStatusOK;
				
        if (readRecursive) {
          if (t->Class)
            success = min(success, syncRead(false, t->Class, readRecursive));
          if (t->Club)
            success = min(success, syncRead(false, t->Club));
          for (size_t k = 0; k<t->Runners.size(); k++) {
            if (t->Runners[k])
              success = min(success, syncRead(false, t->Runners[k], false, readRecursive));
          }
        }
        if (t->changed)
				  return min(success, syncUpdate(t));
        else
          return success;
			}
		}
		else{
			//Something is wrong!? Deleted?
			return syncUpdate(t);
		}
	
		return  opStatusOK;
	}
	catch (const mysqlpp::Exception& er){
		alert(string(er.what())+" [SYNCREAD oTeam]");
		return opStatusFail;
	}

	return opStatusFail;
}

OpFailStatus MeosSQL::syncUpdate(oClass *c)
{
  errorMessage.clear();

  if(CmpDataBase.empty())
		return opStatusFail;

	if(!c || !con.connected())
		return opStatusFail;
	mysqlpp::Query queryset = con.query();
	
	queryset << " Name=" << quote << c->Name << ","
		<< " Course=" << c->getCourseId() << ","
		<< " MultiCourse=" << quote << c->codeMultiCourse() << ","
    << " LegMethod=" << quote << c->codeLegMethod() 
		<< c->getDI().generateSQLSet();

	return syncUpdate(queryset, "oClass", c);
}

OpFailStatus MeosSQL::syncRead(bool forceRead, oClass *c)
{
  return syncRead(forceRead, c, true);
}

OpFailStatus MeosSQL::syncRead(bool forceRead, oClass *c, bool readCourses)
{
	errorMessage.clear();
  if(CmpDataBase.empty())
		return opStatusFail;

	if(!c || !con.connected())
		return opStatusFail;

  if(!forceRead && !c->existInDB()) 
    return syncUpdate(c);

	try {
		Query query = con.query(); 
		query << "SELECT * FROM oClass WHERE Id=" << c->Id;		
		Result res = query.store();
		
		oEvent *oe=c->oe;

		Row row;
		if(!res.empty()){
			row=res.at(0);
			if(!c->changed || isOld(row["Counter"], string(row["Modified"]), c)){

        OpFailStatus success = opStatusOK;

				if (c->changed)	
          success=opStatusWarning;
									
        storeClass(row, *c, readCourses);
	
				c->Modified.Update();
				c->changed = false;
        c->reChanged = false;
				return opStatusOK;
			}
			else if (readCourses) {
        set<int> d;
        OpFailStatus success = syncReadClassCourses(c, c->getCourseId(), 
                                    c->getMCourseIdSet(d), readCourses);

				if(c->changed)
					return min(success, syncUpdate(c));				
			}
      else 
        return syncUpdate(c);
		}
		else{
			//Something is wrong!? Deleted?
			return syncUpdate(c);
		}
		return opStatusOK;
	}
	catch (const mysqlpp::Exception& er){
		alert(string(er.what())+" [SYNCREAD oClass]");
		return opStatusFail;
	}

	return opStatusFail;
}

OpFailStatus MeosSQL::syncReadClassCourses(oClass *c, int cno, 
                                           const set<int> &multi,
                                           bool readRecursive)
{
  OpFailStatus success = opStatusOK;
	oEvent *oe = c->oe;

  if (cno!=0) {
	  c->Course=oe->getCourse(cno);
  	
	  if (!c->Course) {
		  oCourse oc(oe, cno);
		  success = min(success, syncRead(true, &oc, readRecursive));
      c->Course = oe->addCourse(oc);
	  }
    else if (readRecursive)
	    success = min(success, syncRead(false, c->Course, true));					
  }
  else 
    c->Course=0;

  //Update courses for multicourse
  for(set<int>::const_iterator it=multi.begin(); it!=multi.end(); ++it) {
    if (*it!=0) {
      pCourse pc = oe->getCourse(*it);  					
	    if(!pc){
		    oCourse oc(oe, *it);
        success = min(success, syncRead(true, &oc, readRecursive));
        oe->addCourse(oc);
 	    }
      else if (readRecursive)
	      success = min(success, syncRead(false, pc, true));					
    }
  }
  return success;
}

OpFailStatus MeosSQL::syncUpdate(oClub *c)
{
  errorMessage.clear();

  if(CmpDataBase.empty())
		return opStatusFail;

	if(!c || !con.connected())
		return opStatusFail;
	mysqlpp::Query queryset = con.query();
	queryset << " Name=" << quote << c->name
			<< c->getDI().generateSQLSet();			

	return syncUpdate(queryset, "oClub", c);
}

OpFailStatus MeosSQL::syncRead(bool forceRead, oClub *c)
{
  errorMessage.clear();

  if(CmpDataBase.empty())
		return opStatusFail;

	if(!c || !con.connected())
		return opStatusFail;

  if(!forceRead && !c->existInDB()) 
    return syncUpdate(c);

	try{
		Query query = con.query(); 
		query << "SELECT * FROM oClub WHERE Id=" << c->Id;		
		Result res = query.store();
		
		Row row;
		if(!res.empty()){
			row=res.at(0);
			if (!c->changed || isOld(row["Counter"], string(row["Modified"]), c)) {

        OpFailStatus success = opStatusOK;
				if(c->changed)
          success = opStatusWarning;
				
        storeClub(row, *c);

				c->Modified.Update();
				c->changed=false;
				return success;
			}
			else if(c->changed){
				return syncUpdate(c);				
			}
		}
		else{
			//Something is wrong!? Deleted?
			return syncUpdate(c);
		}
		return opStatusOK;
	}
	catch (const mysqlpp::Exception& er){
		alert(string(er.what())+" [SYNCREAD oClub]");
		return opStatusFail;
	}

	return opStatusFail;
}

OpFailStatus MeosSQL::syncUpdate(oControl *c)
{
  errorMessage.clear();

  if(CmpDataBase.empty())
		return opStatusFail;

	if(!c || !con.connected())
		return opStatusFail;

	mysqlpp::Query queryset = con.query();
	queryset << " Name=" << quote << c->Name << ", "
		<< " Numbers=" << quote << c->codeNumbers() << ","
		<< " Status=" << c->Status
		<< c->getDI().generateSQLSet();

	return syncUpdate(queryset, "oControl", c);
}

OpFailStatus MeosSQL::syncRead(bool forceRead, oControl *c)
{
  errorMessage.clear();

  if(CmpDataBase.empty())
		return opStatusFail;

	if(!c || !con.connected())
		return opStatusFail;

  if(!forceRead && !c->existInDB()) 
    return syncUpdate(c);

	try{
		Query query = con.query(); 
		query << "SELECT * FROM oControl WHERE Id=" << c->Id;		
		Result res = query.store();
		
		Row row;
		if(!res.empty()){
			row=res.at(0);
			if(!c->changed || isOld(row["Counter"], string(row["Modified"]), c)){

        OpFailStatus success=opStatusOK;
				if(c->changed)	
          success=opStatusWarning;

        storeControl(row, *c);
				
				c->Modified.Update();
				c->changed=false;
				return success;
			}
			else if(c->changed){
				return syncUpdate(c);				
			}
		}
		else {
			//Something is wrong!? Deleted?
			return syncUpdate(c);
		}
		return opStatusOK;
	}
	catch (const mysqlpp::Exception& er){
		alert(string(er.what())+" [SYNCREAD oControl]");
		return opStatusFail;
	}
	return opStatusFail; 
}

OpFailStatus MeosSQL::syncUpdate(oCourse *c)
{
  errorMessage.clear();

  if(CmpDataBase.empty())
		return opStatusFail;

	if(!c || !con.connected())
		return opStatusFail;
	mysqlpp::Query queryset = con.query();
	queryset << " Name=" << quote << c->Name << ", "
		<< " Length=" << c->Length << ", "
		<< " Controls=" << quote << c->getControls() << ", "
    << " Legs=" << quote << c->getLegLengths()
		<< c->getDI().generateSQLSet();
	
	return syncUpdate(queryset, "oCourse", c);
}

OpFailStatus MeosSQL::syncRead(bool forceRead, oCourse *c)
{
  return syncRead(forceRead, c, true);
}

OpFailStatus MeosSQL::syncRead(bool forceRead, oCourse *c, 
                               bool readControls)
{
  errorMessage.clear();

  if(CmpDataBase.empty())
		return opStatusFail;

	if(!c || !con.connected())
		return opStatusFail;

	if(!forceRead && !c->existInDB()) 
    return syncUpdate(c);

	try{
		Query query = con.query(); 
		query << "SELECT * FROM oCourse WHERE Id=" << c->Id;		
		Result res = query.store();
		
		Row row;
		if(!res.empty()){
			row=res.at(0);
			if (!c->changed || isOld(row["Counter"], string(row["Modified"]), c)){

        OpFailStatus success = opStatusOK;
				if (c->changed)	
          success = opStatusWarning;
								
        storeCourse(row, *c, readControls);
				
				c->Modified.Update();
				c->changed=false;
        c->reChanged=false;
				return success;
			}
			else {
        OpFailStatus success = opStatusOK;
				
        // Plain read controls
				for (int i=0;i<c->nControls; i++) {
          if (c->Controls[i])
					  success = min(success, syncRead(false, c->Controls[i]));
				}

				if(c->changed)
					return min(success, syncUpdate(c));
        else 
          return success;
			}
		}
		else{
			//Something is wrong!? Deleted?
			return syncUpdate(c);
		}
	
		return opStatusOK;
	}
	catch (const mysqlpp::Exception& er){
		alert(string(er.what())+" [SYNCREAD oCourse]");
		return opStatusFail;
	}

	return opStatusFail;
}

OpFailStatus MeosSQL::syncUpdate(oFreePunch *c)
{
  errorMessage.clear();

  if(CmpDataBase.empty())
		return opStatusFail;

	if(!c || !con.connected())
		return opStatusFail;

	mysqlpp::Query queryset = con.query();
	queryset << " CardNo=" <<  c->CardNo << ", "
		<< " Type=" << c->Type << ","
		<< " Time=" << c->Time;

	return syncUpdate(queryset, "oPunch", c);
}

OpFailStatus MeosSQL::syncRead(bool forceRead, oFreePunch *c)
{
  errorMessage.clear();

  if(CmpDataBase.empty())
		return opStatusFail;

	if(!c || !con.connected())
		return opStatusFail;

  if(!forceRead && !c->existInDB()) 
    return syncUpdate(c);

	try{
		Query query = con.query(); 
		query << "SELECT * FROM oPunch WHERE Id=" << c->Id;		
		Result res = query.store();
		
		Row row;
		if(!res.empty()){
			row=res.at(0);
			if(!c->changed || isOld(row["Counter"], string(row["Modified"]), c)){

        OpFailStatus success = opStatusOK;
				if(c->changed)	
          success = opStatusWarning;
				
        storePunch(row, *c);

				c->Modified.Update();
				c->changed=false;
				return success;
			}
			else if(c->changed) {
				return syncUpdate(c);				
			}
		}
		else{
			//Something is wrong!? Deleted?
			return syncUpdate(c);
		}
	
		return opStatusOK;
	}
	catch (const mysqlpp::Exception& er){
		alert(string(er.what())+" [SYNCREAD oPunch]");
		return opStatusFail;
	}
	return opStatusFail; 
}

OpFailStatus MeosSQL::updateTime(const char *oTable, oBase *ob)
{
  errorMessage.clear();

	mysqlpp::Query query = con.query();

	query << "SELECT Modified, Counter FROM " << oTable << " WHERE Id=" << ob->Id;
					
	mysqlpp::Result res = query.store();

	if (!res.empty()) {
		ob->sqlUpdated=res.at(0)["Modified"];
    ob->counter = res.at(0)["Counter"];
		ob->changed=false; //Mark as saved.
    return opStatusOK;
	}
  else return opStatusFail;
}

OpFailStatus MeosSQL::syncUpdate(mysqlpp::Query &updateqry, 
                                 const char *oTable, oBase *ob)
{
  errorMessage.clear();

	if(!con.connected())
    return opStatusFail;

	mysqlpp::Query query = con.query();
	try{
    if (!ob->existInDB()) {
      bool setId = false;
      
      if (ob->Id > 0) {
        query << "SELECT Id FROM " << oTable << " WHERE Id=" << ob->Id;
        Result res=query.store();
        if (res && res.num_rows()==0)
          setId = true;
      }
    
      query.reset();
			query << "INSERT INTO " << oTable << " SET " << updateqry.str();				

      if (setId)
        query << ", Id=" << ob->Id;

			mysqlpp::ResNSel res=query.execute();
			if (res) {
        if (ob->Id > 0 && ob->Id!=(int)res.insert_id) {
          ob->correctionNeeded = true;
        }
        

  		  ob->Id=(int)res.insert_id; 
        query.reset();
        query << "UPDATE oCounter, " << oTable << " SET " 
              << oTable << "=" << oTable << "+1, Counter=" << oTable
              << "+1 WHERE Id=" << ob->Id;
        query.execute();
			  ob->oe->updateFreeId(ob);
  			return updateTime(oTable, ob);
			}
			else return opStatusFail;
		}
		else {
      query << "UPDATE oCounter, " << oTable << " SET " 
          << oTable << "=" << oTable << "+1, "
          << "Counter=" << oTable << ", "
				  << updateqry.str();
  		query << " WHERE Id=" << ob->Id;

			mysqlpp::ResNSel res=query.execute();

			if(res){
				if(res.rows==0){
					query.reset();

					query << "SELECT Id FROM " << oTable << " WHERE Id=" << ob->Id;
					mysqlpp::Result store_res = query.store();

					if(store_res.num_rows()==0){					
						query.reset();
						query << "INSERT INTO " << oTable << " SET " << 
								updateqry.str() << ", Id=" << ob->Id;

						res=query.execute();
						if(!res) 
              return opStatusFail;

            query.reset();
            query << "UPDATE oCounter, " << oTable << " SET "
                  << oTable << "=" << oTable << "+1, Counter=" << oTable
                  << " WHERE Id=" << ob->Id;
            query.execute();
					}
				}
			}

			return updateTime(oTable, ob);
		}
	}
	catch (const mysqlpp::Exception& er){
		alert(string(er.what())+" [" + oTable + " \n\n(" + query.str() + ")]");
		return opStatusFail;
	}

	return opStatusFail;	
}

OpFailStatus MeosSQL::SyncEvent(oEvent *oe)
{
  errorMessage.clear();

	if(!con.connected())
		return opStatusOK;

  bool oldVersion=false;
	try{		
		Query query = con.query();
		
		query << "SELECT * FROM oEvent";		
    query << " WHERE Counter>" << oe->counter;

		Result res = query.store();
		
		if (res && res.num_rows()>0) {
			Row row=res.at(0);
			string Modified=row["Modified"];
		  int counter = row["Counter"];

      int dbv=int(row["BuildVersion"]);
      if ( dbv<buildVersion )
        oe->updateChanged();
      else if (dbv>buildVersion)
        oldVersion=true; 

      if (isOld(counter, Modified, oe)) {
        oe->Name=row["Name"];
		    oe->Date=row["Date"];
		    oe->ZeroTime=row["ZeroTime"];
        oe->sqlUpdated=Modified;
        oe->counter = counter;
			  oDataInterface odi=oe->getDI();
  		  storeData(odi, row);
        oe->setCurrency(-1, "");//Init temp data from stored data
        oe->changed=false;
			}
      else if(oe->isChanged()) {
       
        mysqlpp::Query queryset = con.query();
  	    queryset << " Name=" << quote << oe->Name << ", "
			           << " Date="  << quote << oe->Date << ", "
                 << " NameId="  << quote << oe->CurrentNameId << ", "
			           << " ZeroTime=" << oe->ZeroTime << ", "
                 << " BuildVersion=IF(BuildVersion<" << 
                      buildVersion << "," << buildVersion << ",BuildVersion)"
			           <<  oe->getDI().generateSQLSet();

  		  syncUpdate(queryset, "oEvent", oe);

        // Update list database;
        con.select_db("MeOSMain");
        queryset.reset();
  	    queryset << "UPDATE oEvent SET Name=" << quote << oe->Name << ", "
			           << " Date="  << quote << oe->Date << ", "
                 << " NameId="  << quote << oe->CurrentNameId << ", "
			           << " ZeroTime=" << oe->ZeroTime
                 << " WHERE Id=" << oe->Id;
        
        queryset.execute();
    		//syncUpdate(queryset, "oEvent", oe, true);
        con.select_db(CmpDataBase);
      }
		}
    else if( oe->isChanged() ){
      mysqlpp::Query queryset = con.query();
      queryset << " Name=" << quote << oe->Name << ", "
	             << " Date="  << quote << oe->Date << ", "
               << " NameId="  << quote << oe->CurrentNameId << ", "
	             << " ZeroTime=" << oe->ZeroTime << ", "
               << " BuildVersion=IF(BuildVersion<" << 
                    buildVersion << "," << buildVersion << ",BuildVersion)"
	             <<  oe->getDI().generateSQLSet();

	    syncUpdate(queryset, "oEvent", oe);
      
      // Update list database;
      con.select_db("MeOSMain");
      queryset.reset();
	    queryset << "UPDATE oEvent SET Name=" << quote << oe->Name << ", "
		           << " Date="  << quote << oe->Date << ", "
               << " NameId="  << quote << oe->CurrentNameId << ", "
		           << " ZeroTime=" << oe->ZeroTime
               << " WHERE Id=" << oe->Id;

      queryset.execute();
  		//syncUpdate(queryset, "oEvent", oe, true);
      con.select_db(CmpDataBase);
    }
	}
	catch (const mysqlpp::Exception& er){
    setDefaultDB();
		alert(string(er.what())+" [SYNCLIST oEvent]");
    return opStatusFail;
	}

  if (oldVersion && !warnedOldVersion) {
    warnedOldVersion=true;
    alert("Databasen används av en senare version av MeOS. Fortsätt på egen risk.");
    return opStatusWarning;
  }

	return opStatusOK;
}

bool MeosSQL::syncListRunner(oEvent *oe)
{
  errorMessage.clear();

	if(!con.connected())
		return false;

	try{		
		Query query = con.query();
		
		query << "SELECT Id, Counter, Modified, Removed FROM oRunner";
		query << " WHERE Counter > " << oe->sqlCounterRunners;
    query << " OR Modified > '" << oe->sqlUpdateRunners << "'";

		Result res = query.store();
		
		if (res) {		
			for (int i=0; i<res.num_rows(); i++) {
				Row row=res.at(i);
				int Id = row["Id"];
        int counter = row["Counter"];
        string modified = row["Modified"];

				if(int(row["Removed"])==1){
					oRunner *r=oe->getRunner(Id, 0);

          if(r) {
            r->Removed=true;
            if (r->tInTeam) 
              r->tInTeam->correctRemove(r);

            if (r->tParentRunner) {
              r->tParentRunner->correctRemove(r);
            }
          }
				}
				else{
					oRunner *r=oe->getRunner(Id, 0);

					if(r){
						if (isOld(counter, modified, r))
							syncRead(false, r);
					}
					else {
						oRunner or(oe, Id);
            syncRead(true, &or, false, false);
						r = oe->addRunner(or);            
					}
				}
        oe->sqlCounterRunners = max(counter, oe->sqlCounterRunners);
        oe->sqlUpdateRunners = max(modified, oe->sqlUpdateRunners);
			}		
		}
	}
	catch (const mysqlpp::Exception& er){
		alert(string(er.what())+" [SYNCLIST oRunner]");
		return false;
	}
	return true;
}

bool MeosSQL::syncListClass(oEvent *oe)
{
  errorMessage.clear();

	if(!con.connected())
		return false;

	try{		
		Query query = con.query();
		
		query << "SELECT Id, Counter, Modified, Removed FROM oClass";
    query << " WHERE Counter > " << oe->sqlCounterClasses;
    query << " OR Modified > '" << oe->sqlUpdateClasses << "'";

		Result res = query.store();
		
		if (res) {

			for (int i=0; i<res.num_rows(); i++) {
				Row row=res.at(i);
        int counter = row["Counter"];
        string modified = row["Modified"];

				int Id=row["Id"];

				if (int(row["Removed"])) {
					oClass *c=oe->getClass(Id);
					if(c) c->Removed=true;
				}
				else {
					oClass *c=oe->getClass(Id);

          if (!c) {
            oClass oc(oe, Id);
            c=oe->addClass(oc);
            if (c!=0) {
              c->changed = false;
 					    syncRead(true, c, false);
            }
          }
          else if (isOld(counter, modified, c))
					  syncRead(false, c, false);
				}
        oe->sqlCounterClasses = max(counter, oe->sqlCounterClasses);
        oe->sqlUpdateClasses = max(modified, oe->sqlUpdateClasses);
			}		
		}
	}
	catch (const mysqlpp::Exception& er){
		alert(string(er.what())+" [SYNCLIST oClass]");
		return false;
	}
	return true;
}


bool MeosSQL::syncListClub(oEvent *oe)
{
  errorMessage.clear();

	if(!con.connected())
		return false;

	try{		
		Query query = con.query();
		
		query << "SELECT Id, Counter, Modified, Removed FROM oClub";
		query << " WHERE Counter > " << oe->sqlCounterClubs;
    query << " OR Modified > '" << oe->sqlUpdateClubs << "'";

		Result res = query.store();
		
		if (res) {		
			for(int i=0; i<res.num_rows(); i++){
				Row row=res.at(i);

        int counter = row["Counter"];
        string modified = row["Modified"];
				int Id=row["Id"];
      
				if(int(row["Removed"])){
					oClub *c=oe->getClub(Id);

					if(c) c->Removed=true;
				}
				else {
					oClub *c=oe->getClub(Id);

					if (c==0) {
            oClub oc(oe, Id);
            syncRead(true, &oc);
            oe->addClub(oc);
					}
          else if (isOld(counter, modified, c))
            syncRead(false, c);
				}
        oe->sqlCounterClubs = max(counter, oe->sqlCounterClubs);
        oe->sqlUpdateClubs = max(modified, oe->sqlUpdateClubs);
			}
    }
	}
	catch (const mysqlpp::Exception& er){
		alert(string(er.what())+" [SYNCLIST oClub]");
		return false;
	}
	return true;
}


bool MeosSQL::syncListCourse(oEvent *oe)
{
  errorMessage.clear();

	if(!con.connected())
		return false;

	try{		
		Query query = con.query();
		
		query << "SELECT Id, Counter, Modified, Removed FROM oCourse";
		query << " WHERE Counter > " << oe->sqlCounterCourses;
    query << " OR Modified > '" << oe->sqlUpdateCourses << "'";

		Result res = query.store();
		
		if (res) {
			for(int i=0; i<res.num_rows(); i++){
				Row row=res.at(i);
        int counter = row["Counter"];
        string modified(row["Modified"]);

				int Id = row["Id"];

				if(int(row["Removed"])){
					oCourse *c=oe->getCourse(Id);

					if(c) c->Removed=true;
				}
				else{
					oCourse *c=oe->getCourse(Id);

          if (c==0) {
            oCourse oc(oe, Id);
            syncRead(true, &oc, false);
            oe->addCourse(oc);
          } 
					else if (isOld(counter, modified, c))
					  syncRead(false, c, false);
				}
        oe->sqlCounterCourses = max(counter, oe->sqlCounterCourses);
        oe->sqlUpdateCourses = max(modified, oe->sqlUpdateCourses);
			}
		}
	}
	catch (const mysqlpp::Exception& er){
		alert(string(er.what())+" [SYNCLIST oCourse]");
		return false;
	}
	return true;
}


bool MeosSQL::syncListCard(oEvent *oe)
{
  errorMessage.clear();

	if(!con.connected())
		return false;

	try{		
		Query query = con.query();
		
		query << "SELECT Id, Counter, Modified, Removed FROM oCard";
		query << " WHERE Counter > " << oe->sqlCounterCards;
    query << " OR Modified > '" << oe->sqlUpdateCards << "'";

		Result res = query.store();
		
		if (res) {
			for (int i=0; i<res.num_rows(); i++) {
				Row row = res.at(i);
        int counter = row["Counter"];
        string modified(row["Modified"]);
        int Id = row["Id"];

				if (int(row["Removed"])) {
					oCard *c=oe->getCard(Id);
					if(c) c->Removed=true;
				}
				else {
					oCard *c=oe->getCard(Id);

					if(c) {
						if(isOld(counter, modified, c))
							syncRead(false, c);
					}
					else {
						oCard oc(oe, Id);
            c = oe->addCard(oc);
            if (c!=0)
						  syncRead(true, c);
					}
				}
        oe->sqlCounterCards = max(counter, oe->sqlCounterCards);
        oe->sqlUpdateCards = max(modified, oe->sqlUpdateCards);
			}		
		}
	}
	catch (const mysqlpp::Exception& er){
		alert(string(er.what())+" [SYNCLIST oCard]");
		return false;
	}
	return true;
}

bool MeosSQL::syncListControl(oEvent *oe)
{
  errorMessage.clear();

	if(!con.connected())
		return false;

	try{		
		Query query = con.query();
		
		query << "SELECT Id, Counter, Modified, Removed FROM oControl";
		query << " WHERE Counter > " << oe->sqlCounterControls;
    query << " OR Modified > '" << oe->sqlUpdateControls << "'";

		Result res = query.store();
		
		if (res) {		
			for(int i=0; i<res.num_rows(); i++){
				Row row=res.at(i);
        int counter = row["Counter"];
        string modified(row["Modified"]);
				int Id = row["Id"];

				if(int(row["Removed"])){
					oControl *c=oe->getControl(Id, false);

					if(c) c->Removed=true;
				}
				else {
					oControl *c=oe->getControl(Id, false);
					if (c) {
						if(isOld(counter, modified, c))
							syncRead(false, c);
					}
          else {
            oControl oc(oe, Id);
            c = oe->addControl(oc);
            if (c!=0)
              syncRead(true, c);
          }
				}
        oe->sqlCounterControls = max(counter, oe->sqlCounterControls);
        oe->sqlUpdateControls = max(modified, oe->sqlUpdateControls);
			}		
		}
	}
	catch (const mysqlpp::Exception& er){
		alert(string(er.what())+" [SYNCLIST oControl]");
		return false;
	}
	return true;
}

bool MeosSQL::syncListPunch(oEvent *oe)
{
  errorMessage.clear();

	if(!con.connected())
		return false;

	try{		
		Query query = con.query();
		
		query << "SELECT Id, Counter, Modified, Removed FROM oPunch";
		query << " WHERE Counter > " << oe->sqlCounterPunches;
    query << " OR Modified > '" << oe->sqlUpdatePunches << "' ORDER BY Id";

		Result res = query.store();
		
		if (res) {
			for(int i=0; i<res.num_rows(); i++){
				Row row=res.at(i);
        int counter = row["Counter"];
        string modified(row["Modified"]);
        int Id=row["Id"];

				if (int(row["Removed"])) {
					oFreePunch *c=oe->getPunch(Id);
          if (c) {
            c->Removed=true;
            pRunner r = oe->getRunner(c->tRunnerId, 0);
            if (r)
              r->markClassChanged();
          }
				}
				else {
					oFreePunch *c=oe->getPunch(Id);

					if (c) {
						if(isOld(counter, modified, c))
							syncRead(false, c);
					}
					else {
						oFreePunch p(oe, Id);
            syncRead(true, &p);
						oe->addFreePunch(p);              
					}
				}
        oe->sqlCounterPunches = max(counter, oe->sqlCounterPunches);
        oe->sqlUpdatePunches = max(modified, oe->sqlUpdatePunches);
			}		
		}
	}
	catch (const mysqlpp::Exception& er){
		alert(string(er.what())+" [SYNCLIST oPunch]");
		return false;
	}
	return true;
}

bool MeosSQL::syncListTeam(oEvent *oe)
{
  errorMessage.clear();

	if(!con.connected())
		return false;

	try{		
		Query query = con.query();
		
		query << "SELECT Id, Counter, Modified, Removed FROM oTeam";
		query << " WHERE Counter > " << oe->sqlCounterTeams;
    query << " OR Modified > '" << oe->sqlUpdateTeams << "'";

		Result res = query.store();
		
		if (res) {		
			for (int i=0; i<res.num_rows(); i++) {
				Row row=res.at(i);
        int counter = row["Counter"];
        string modified(row["Modified"]);
				int Id = row["Id"];

				if(int(row["Removed"])){
					oTeam *t=oe->getTeam(Id);
          if(t) {
            t->prepareRemove();
            t->Removed=true;
          }
				}
				else {
					oTeam *t=oe->getTeam(Id);

					if (t) {
						if(isOld(counter, modified, t))
							syncRead(false, t, false);
					}
					else{
						oTeam ot(oe, Id);
            t = oe->addTeam(ot);
            if (t) { 
              syncRead(true, t, false);
              t->apply(false, 0);
              t->changed = false;
            }
					}
				}
        oe->sqlCounterTeams = max(counter, oe->sqlCounterTeams);
        oe->sqlUpdateTeams = max(modified, oe->sqlUpdateTeams);
			}		
		}
	}
	catch (const mysqlpp::Exception& er){
		alert(string(er.what())+" [SYNCLIST oTeam]");
		return false;
	}
	return true;
}

bool MeosSQL::checkConnection(oEvent *oe)
{
  errorMessage.clear();

  if(!oe) {
    if (monitorId && con.connected()) {
      try {
        Query query = con.query();
        query << "Update oMonitor SET Removed=1 WHERE Id = " << monitorId;
		    query.execute();
      }
      catch(...) {
        return false; //Not an important error.
      }
    }
    return true;
  }

  oe->connectedClients.clear();
  if (monitorId==0) {
	  try {
      Query query = con.query();
      query << "INSERT INTO oMonitor SET Count=1, Client=" << quote << oe->clientName;
		  ResNSel res=query.execute();
		  if(res)
			  monitorId=static_cast<int>(res.insert_id); 
	  }	
	  catch (const mysqlpp::Exception& er){
      oe->connectedClients.push_back(er.what());
		  return false;
	  }    
  }
  else {
	  try {
      Query query = con.query();
      query << "Update oMonitor SET Count=Count+1, Client=" << quote << oe->clientName
            << " WHERE Id = " << monitorId;
		  query.execute();
	  }	
	  catch (const mysqlpp::Exception& er){
		  oe->connectedClients.push_back(er.what());
      return false;
	  }
  }
	bool callback=false;

  try {
    Query query = con.query();
    query << "SELECT Id, Client FROM oMonitor WHERE Modified>TIMESTAMPADD(SECOND, -30, NOW())"
             " AND Removed=0 ORDER BY Client";
   
		Result res = query.store();

		if (res) {		
			for (int i=0; i<res.num_rows(); i++) {
				Row row=res.at(i);
        oe->connectedClients.push_back(string(row["Client"]));

        if(int(row["Id"])==monitorId)
          callback=true;
			}
    }
  }
  catch (const mysqlpp::Exception& er){
	  oe->connectedClients.push_back(er.what());
    return false;
  }
  return callback;
}

void MeosSQL::setDefaultDB()
{
  errorMessage.clear();

  try {    
    if(!con.connected())
		  return;

		con.select_db(CmpDataBase);
  }
  catch(...) {
  }
}

bool MeosSQL::dropDatabase(oEvent *oe)
{
  // Check if other cients are connected.
  if( !checkConnection(oe) ) {
    if (!oe->connectedClients.empty())
      alert(oe->connectedClients[0]);

    return false;
  }

  if (oe->connectedClients.size()!=1) {
    alert("Database is used and cannot be deleted");
    return false;
  }

  try {
		con.select_db("MeOSMain");
	}
	catch (const mysqlpp::Exception& er) {
		alert(string(er.what()) + " MySQL Error. Select MeosMain");
		setDefaultDB();
    return 0;
	}
  
  try {
		con.drop_db(CmpDataBase);
	}
	catch (const mysqlpp::Exception& ) {
    //Don't care if we fail.
	}
  
  try {
    Query query = con.query();
		query << "DELETE FROM oEvent WHERE NameId=" << quote << CmpDataBase;
    query.execute();
	}
	catch (const mysqlpp::Exception& ) {
    //Don't care if we fail.
	}

  CmpDataBase.clear();

  errorMessage.clear();

  try {
    con.close();
  }	
  catch (const mysqlpp::Exception&) {
	}

  return true;
}

