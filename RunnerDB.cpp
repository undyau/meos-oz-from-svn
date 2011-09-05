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

#include "stdafx.h"
#include "RunnerDB.h"
#include "xmlparser.h"
#include "oRunner.h"

#include "io.h"
#include "fcntl.h"
#include "sys/stat.h"
#include "meos_util.h"
#include "oDataContainer.h"

#include <algorithm>
#include <cassert>
#include "intkeymapimpl.hpp"

RunnerDB::RunnerDB(oEvent *oe_): oe(oe_)
{
  loadedFromServer = false;
  dataDate = 20100201;
  dataTime = 222222;
}

RunnerDB::~RunnerDB(void)
{
}

RunnerDBEntry::RunnerDBEntry()
{
  memset(this, 0, sizeof(RunnerDBEntry));  
}

RunnerDBEntryV1::RunnerDBEntryV1()
{
  memset(this, 0, sizeof(RunnerDBEntryV1));  
}


void RunnerDBEntry::getName(string &n) const
{
  n=name;
}

void RunnerDBEntry::setName(const char *n) 
{
  memcpy(name, n, min<int>(strlen(n)+1, baseNameLength));
  name[baseNameLength-1]=0;
}

string RunnerDBEntry::getNationality() const
{
  string n("   ");
  n[0] = national[0];
  n[1] = national[1];
  n[2] = national[2];
	return n;
}

string RunnerDBEntry::getSex() const
{
  string n("W");
  n[0] = sex;
 	return n;
}
string RunnerDBEntry::getGivenName() const
{
  string Name(name);
	return trim(Name.substr(0, Name.find_first_of(' ')));
}

string RunnerDBEntry::getFamilyName() const
{
  string Name(name);
  int k=Name.find_first_of(' ');

  if(k==string::npos)
    return "";
  else return trim(Name.substr(k, string::npos));
}

__int64 RunnerDBEntry::getExtId() const 
{
  return extId;
}

void RunnerDBEntry::setExtId(__int64 id) 
{
  extId = id;
}

void RunnerDBEntry::init(const RunnerDBEntryV1 &dbe)
{
  memcpy(this, &dbe, sizeof(RunnerDBEntryV1));
  extId = 0;
}


RunnerDBEntry *RunnerDB::addRunner(const char *name, 
                                   __int64 extId,
                                   int club, int card)
{
  rdb.push_back(RunnerDBEntry());
  RunnerDBEntry &e=rdb.back();
  e.cardNo = card;
  e.clubNo = club;
  e.setName(name);
  e.extId = extId;

  if (!check(e) ) {
    rdb.pop_back();
    return 0;
  } else {
    if (card>0)
      rhash[card]=rdb.size()-1;
    if (!idhash.empty())
      idhash[int(extId)] = rdb.size()-1;
    if (!nhash.empty())
      nhash.insert(pair<string, int>(canonizeName(e.name), rdb.size()-1));
  }
  return &e;
}

void RunnerDB::addClub(oClub &c)
{
  //map<int,int>::iterator it = chash.find(c.getId());
  //if (it == chash.end()) {
  int value;
  if (!chash.lookup(c.getId(), value)) {
    cdb.push_back(c);
    chash[c.getId()]=cdb.size()-1;
  }
  else {
    cdb[value] = c;
  }
}

void RunnerDB::importClub(oClub &club, bool matchName)
{
  pClub pc = getClub(club.getId());

  if (pc && !pc->sameClub(club)) {
    //The new id is used by some other club.
    //Remap old club first

    int oldId = pc->getId(); 
    int newId = chash.size() + 1;//chash.rbegin()->first + 1;

    for (size_t k=0; k<rdb.size(); k++)
      if (rdb[k].clubNo == oldId)
        rdb[k].clubNo = newId;

    chash[newId] = chash[oldId];
    pc = 0;
  }

  if (pc == 0 && matchName) {
    // Try to find the club under other id
    pc = getClub(club.getName());

    if ( pc!=0 ) {
      // If found under different id, remap to new id
      int oldId = pc->getId(); 
      int newId = club.getId();

      for (size_t k=0; k<rdb.size(); k++)
        if (rdb[k].clubNo == oldId)
          rdb[k].clubNo = newId;
      
      pClub base = &cdb[0];
      int index = (size_t(pc) - size_t(base)) / sizeof(oClub);
      chash[newId] = index;
    }
  }

  if ( pc )
    *pc = club; // Update old club
  else {
    // Completely new club
    chash [club.getId()] = cdb.size();
    cdb.push_back(club);    
  }
}

void RunnerDB::compactifyClubs()
{
  chash.clear();

  map<int, int> clubmap;  
  for (size_t k=0;k<cdb.size();k++) 
    clubmap[cdb[k].getId()] = cdb[k].getId();

  for (size_t k=0;k<cdb.size();k++) {
    oClub &ref = cdb[k];    
    vector<int> compacted;
    for (size_t j=k+1;j<cdb.size(); j++) {
      if(_stricmp(ref.getName().c_str(), 
              cdb[j].getName().c_str())==0) 
        compacted.push_back(j);
    }

    if (!compacted.empty()) {
      int ba=ref.getDI().getDataAmountMeasure();  
      oClub *best=&ref;
      for (size_t j=0;j<compacted.size();j++) {
        int nba=cdb[compacted[j]].getDI().getDataAmountMeasure();       
        if (nba>ba) {
          best = &cdb[compacted[j]];
          ba=nba;
        }
      }
      swap(ref, *best);

      //Update map
      for (size_t j=0;j<compacted.size();j++) {
        clubmap[cdb[compacted[j]].getId()] = ref.getId();
      }      
    }
  }
}

RunnerDBEntry *RunnerDB::getRunnerByCard(int card) const
{
  if (card == 0)
    return 0;

  //map<int,int>::const_iterator it = rhash.find(card);

  //if (it!=rhash.end())
  int value;
  if (rhash.lookup(card, value))
    return (RunnerDBEntry *)&rdb[value];

  return 0;
}

RunnerDBEntry *RunnerDB::getRunnerById(int extId) const
{
  if (extId == 0)
    return 0;

  setupIdHash();

  //map<int,int>::const_iterator it = idhash.find(extId);

  //if (it!=idhash.end())
  int value;
  
  if (idhash.lookup(extId, value))
    return (RunnerDBEntry *)&rdb[value];

  return 0;
}

RunnerDBEntry *RunnerDB::getRunnerByName(const string &name, int clubId, 
                                         int expectedBirthYear) const
{
  if (expectedBirthYear>0 && expectedBirthYear<100)
    expectedBirthYear = extendYear(expectedBirthYear);

  setupNameHash();
  vector<int> ix;
  string cname(canonizeName(name.c_str()));
  multimap<string, int>::const_iterator it = nhash.find(cname);

  while (it != nhash.end() && cname == it->first) {
    ix.push_back(it->second);
    ++it;
  }

  if (ix.empty())
    return 0;

  if (clubId == 0) {
    if (ix.size() == 1)
      return (RunnerDBEntry *)&rdb[ix[0]];
    else
      return 0; // Not uniquely defined.
  }

  // Filter on club
  vector<int> ix2;
  for (size_t k = 0;k<ix.size(); k++)
    if (rdb[ix[k]].clubNo == clubId || rdb[ix[k]].clubNo==0)
      ix2.push_back(ix[k]);
  
  if (ix2.empty())
    return 0;
  else if (ix2.size() == 1)
    return (RunnerDBEntry *)&rdb[ix2[0]];
  else if (expectedBirthYear > 0) {
    int bestMatch = 0;
    int bestYear = 0;
    for (size_t k = 0;k<ix2.size(); k++) {
      const RunnerDBEntry &re = rdb[ix2[k]];
      if (abs(re.birthYear-expectedBirthYear) < abs(bestYear-expectedBirthYear)) {
        bestMatch = ix2[k];
        bestYear = re.birthYear;
      }
    }
    if (bestYear>0)
      return (RunnerDBEntry *)&rdb[bestMatch];
  }

  return 0;
}

void RunnerDB::setupIdHash() const
{
  if (!idhash.empty())
    return;

  for (size_t k=0; k<rdb.size(); k++) {
    idhash[int(rdb[k].extId)] = int(k);
  }
}

void RunnerDB::setupNameHash() const
{
  if (!nhash.empty())
    return;

  for (size_t k=0; k<rdb.size(); k++) {
    nhash.insert(pair<string, int>(canonizeName(rdb[k].name), k));
  }
}

void RunnerDB::setupCNHash() const
{
  if (!cnhash.empty())
    return;

  vector<string> split;
  for (size_t k=0; k<cdb.size(); k++) {
    canonizeSplitName(cdb[k].getName(), split);
    for (size_t j = 0; j<split.size(); j++)
      cnhash.insert(pair<string, int>(split[j], k));
  }
}

static bool isVowel(int c) {
  return c=='a' || c=='e' || c=='i' || 
         c=='o' || c=='u' || c=='y' || 
         c=='å' || c=='ä' || c=='ö';
}

void RunnerDB::canonizeSplitName(const string &name, vector<string> &split)
{
  split.clear();
  const char *cname = name.c_str();
  int k = 0;
  for (k=0; cname[k]; k++)
    if (cname[k] != ' ')
      break;

  char out[128];
  int outp;
  while (cname[k]) {
    outp = 0;
    while(cname[k] != ' ' && cname[k] && outp<(sizeof(out)-1) ) {
      if (cname[k] == '-') {
        k++;
        break;
      }
      out[outp++] = toLowerStripped(cname[k]);
      k++;
    }
    out[outp] = 0;
    if (outp > 0) {
      for (int j=1; j<outp-1; j++) {
        if (out[j] == 'h') { // Idenity Thor and Tohr
          bool v1 = isVowel(out[j+1]);
          bool v2 = isVowel(out[j-1]);
          if (v1 && !v2)
            out[j] = out[j+1];
          else if (!v1 && v2)
            out[j] = out[j-1];
        }
      }

      if (outp>4 && out[outp-1]=='s')
        out[outp-1] = 0; // Identify Linköping och Linköpings
      split.push_back(out);
    }
    while(cname[k] == ' ') 
      k++;
  }
}

bool RunnerDB::getClub(int clubId, string &club) const
{
  //map<int,int>::const_iterator it = chash.find(clubId);

  int value;
  if (chash.lookup(clubId, value)) {
  //if (it!=chash.end()) {
  //  int i=it->second;
    club=cdb[value].getName();
    return true;
  }
  return false;
}

oClub *RunnerDB::getClub(int clubId) const
{
  //map<int,int>::const_iterator it = chash.find(clubId);

  //if (it!=chash.end())
  int value;
  if (chash.lookup(clubId, value)) 
    return pClub(&cdb[value]);
  
  return 0;
}

oClub *RunnerDB::getClub(const string &name) const
{
  setupCNHash();
  vector<string> names;
  canonizeSplitName(name, names);
  vector< vector<int> > ix(names.size());
  set<int> iset;

  for (size_t k = 0; k<names.size(); k++) {
    multimap<string, int>::const_iterator it = cnhash.find(names[k]);

    while (it != cnhash.end() && names[k] == it->first) {
      ix[k].push_back(it->second);
      ++it;
    }

    if (ix[k].size() == 1 && names[k].length()>3)
      return pClub(&cdb[ix[k][0]]);
  
    if (iset.empty())
      iset.insert(ix[k].begin(), ix[k].end());
    else {
      set<int> im;
      for (size_t j = 0; j<ix[k].size(); j++) {
        if (iset.count(ix[k][j])==1)
          im.insert(ix[k][j]);
      }
      if (im.size() == 1) {
        int i = *im.begin();
        return pClub(&cdb[i]);
      }
      else if (!im.empty())
        swap(iset, im);
    }
  }

  // Exact compare
  for (set<int>::iterator it = iset.begin(); it != iset.end(); ++it) {
    pClub pc = pClub(&cdb[*it]);
    if (_stricmp(pc->getName().c_str(), name.c_str())==0)
      return pc;
  }

  string cname = canonizeName(name.c_str());
  // Looser compare
  for (set<int>::iterator it = iset.begin(); it != iset.end(); ++it) {
    pClub pc = pClub(&cdb[*it]);
    if (strcmp(canonizeName(pc->getName().c_str()), cname.c_str()) == 0 )
      return pc;
  }

  double best = 1;
  double secondBest = 1;
  int bestIndex = -1;
  for (set<int>::iterator it = iset.begin(); it != iset.end(); ++it) {
    pClub pc = pClub(&cdb[*it]);

    double d = stringDistance(cname.c_str(), canonizeName(pc->getName().c_str()));
    
    if (d<best) {
      bestIndex = *it;
      secondBest = best;
      best = d;
    }
    else if (d<secondBest) {
      secondBest = d;
      if (d<=0.4)
        return 0; // Two possible clubs are too close. We cannot choose.
    }
  }

  if (best < 0.2 && secondBest>0.4)
    return pClub(&cdb[bestIndex]);

  return 0;
}

void RunnerDB::saveClubs(const char *file)
{
	xmlparser xml;

	if(!xml.openOutputT(file, true, "meosclubs"))
    throw std::exception((string("Fel. Kan inte öppna: ") + file).c_str());

  vector<oClub>::iterator it;	

	xml.startTag("ClubList");

	for (it=cdb.begin(); it != cdb.end(); ++it)
		it->write(xml);

	xml.endTag();

	xml.closeOut();
}

string RunnerDB::getDataDate() const 
{
  char bf[128];
  if (dataTime<=0 && dataDate>0)
    sprintf_s(bf, "%04d-%02d-%02d", dataDate/10000,
                                    (dataDate/100)%100,
                                    dataDate%100);
  else if (dataDate>0)
    sprintf_s(bf, "%04d-%02d-%02d %02d:%02d:%02d", dataDate/10000,
                                                 (dataDate/100)%100,
                                                 dataDate%100,
                                                 (dataTime/3600)%24,
                                                 (dataTime/60)%60,
                                                 (dataTime)%60);
  else
    return "2011-01-01 00:00:00";

  return bf;
}

void RunnerDB::setDataDate(const string &date)
{
   int d = convertDateYMS(date.substr(0, 10));
   int t = date.length()>11 ? convertAbsoluteTimeHMS(date.substr(11)) : 0;

   if (d<=0)
     throw std::exception("Felaktigt datumformat");
   
   dataDate = d;
   if (t>0)
     dataTime = t;
   else
     dataTime = 0;
}

void RunnerDB::saveRunners(const char *file)
{
  int f=-1;
  _sopen_s(&f, file, _O_BINARY|_O_CREAT|_O_TRUNC|_O_WRONLY, 
            _SH_DENYWR, _S_IREAD|_S_IWRITE);
 
  if (f!=-1) {
    int version = 5460002;
    _write(f, &version, 4);
    _write(f, &dataDate, 4);
    _write(f, &dataTime, 4);
    if (!rdb.empty())
      _write(f, &rdb[0], rdb.size()*sizeof(RunnerDBEntry));
    _close(f);
  }
  else throw std::exception("Could not save runner database.");
}

void RunnerDB::loadClubs(const char *file)
{
  xmlparser xml;

	if(!xml.read(file)) 
  	throw std::exception("Could not load club database.");

	xmlobject xo;

	//Get clubs
	xo=xml.getObject("ClubList");
	if (xo) {
    clearClubs();
    loadedFromServer = false;

	  xmlList xl;
    xo.getObjects(xl);

		xmlList::const_iterator it;
    cdb.clear();
    chash.clear();
    cdb.reserve(xl.size());
		for (it=xl.begin(); it != xl.end(); ++it) {
			if(it->is("Club")){
				oClub c(oe);
				c.set(*it);
        int value;
        //if (chash.find(c.getId()) == chash.end()) {
        if (!chash.lookup(c.getId(), value)) {
          chash[c.getId()]=cdb.size();
          cdb.push_back(c);
        }
			}
		}
  }

  bool checkClubs = false;

  if (checkClubs) {
    vector<string> problems;

    for (size_t k=0; k<cdb.size(); k++) {
      pClub pc = &cdb[k];
      pClub pc2 = getClub(pc->getName());
      if (!pc2)
        problems.push_back(pc->getName());
      else if (pc != pc2)
        problems.push_back(pc->getName() + "-" + pc2->getName());
    }
    problems.begin();
  }
}

void RunnerDB::loadRunners(const char *file)
{
  string ex=string("Bad runner database. ")+file;
  int f=-1;
  _sopen_s(&f, file, _O_BINARY|_O_RDONLY, 
            _SH_DENYWR, _S_IREAD|_S_IWRITE);
 
  if (f!=-1) {
    clearRunners();
    loadedFromServer = false;

    int len = _filelength(f);
    if ( (len%sizeof(RunnerDBEntryV1) != 0) && (len % sizeof(RunnerDBEntry) != 12)) {
      _close(f);
      return;//Failed
    }
    int nentry = 0;

    if (len % sizeof(RunnerDBEntry) == 12) {
      nentry = (len-12) / sizeof(RunnerDBEntry);

      rdb.resize(nentry);
      if (rdb.empty()) {
         _close(f);
        return;
      }
      int version;
      _read(f, &version, 4);
      _read(f, &dataDate, 4);
      _read(f, &dataTime, 4);
      _read(f, &rdb[0], len-12);
      _close(f);
      for (int k=0;k<nentry;k++) {
        if (!check(rdb[k]))
          throw std::exception(ex.c_str());
      }
    }
    else {
      dataDate = 0;
      dataTime = 0;

      nentry = len / sizeof(RunnerDBEntryV1);

      rdb.resize(nentry);
      if (rdb.empty()) {
         _close(f);
        return;
      }
      vector<RunnerDBEntryV1> rdbV1(nentry);

      _read(f, &rdbV1[0], len);
      _close(f);
      for (int k=0;k<nentry;k++) {
        rdb[k].init(rdbV1[k]);
        if (!check(rdb[k]))
          throw std::exception(ex.c_str());
      }
    }

    int ncard = 0;
    for (int k=0;k<nentry;k++)
      if (rdb[k].cardNo>0) 
        ncard++;
      
    rhash.resize(ncard);
//    map<int, int> tmap;

    for (int k=0;k<nentry;k++)
      if (rdb[k].cardNo>0) {
        rhash[rdb[k].cardNo]=k;
//        tmap[rdb[k].cardNo]=k;
      }

/*    int value;

    assert(rhash.size() == tmap.size());

    vector<int> tduplicate;
    vector<int> duplicate;
    for (int k=0;k<nentry;k++)
      if (rdb[k].cardNo>0) {
        assert(rhash.lookup(rdb[k].cardNo, value));
        assert(rdb[k].cardNo == rdb[value].cardNo);
        
        if (k!=value)
          duplicate.push_back(k);

        if (tmap[rdb[k].cardNo] != k)
          tduplicate.push_back(k);
      }
      duplicate.size();*/
  }
  else throw std::exception(ex.c_str());
}

bool RunnerDB::check(const RunnerDBEntry &rde) const
{
  if (rde.cardNo<0 || rde.cardNo>99999999 
           || rde.name[baseNameLength-1]!=0 || rde.clubNo<0)
    return false;
  return true;
}

void RunnerDB::updateAdd(const oRunner &r)
{ 
  if (r.getExtIdentifier() > 0) {
    RunnerDBEntry *dbe = getRunnerById(int(r.getExtIdentifier()));
    if (dbe) {
      dbe->cardNo = r.CardNo;
      return; // Do not change too much in runner from national database
    }
  }

  const pClub pc = r.Club;

  pClub dbClub = getClub(r.getClubId());

  if (dbClub == 0) {
    dbClub = getClub(r.getClub());
  }

  if (dbClub == 0 && pc != 0) {
    addClub(*pc);
  }
  
  RunnerDBEntry *dbe = getRunnerByCard(r.getCardNo());

  if (dbe == 0) {
    dbe = addRunner(r.getName().c_str(), 0, r.getClubId(), r.getCardNo());
    if (dbe) 
      dbe->birthYear = r.getDCI().getInt("BirthYear");
  }
  else {
    if (dbe->getExtId() == 0) { // Only update entries not in national db.
      dbe->setName(r.getName().c_str());
      dbe->clubNo = (r.getClubId());
      dbe->birthYear = r.getDCI().getInt("BirthYear");
    }
  }
}

void RunnerDB::getAllNames(vector<string> &givenName, vector<string> &familyName) 
{
  givenName.reserve(rdb.size());
  familyName.reserve(rdb.size());
  for (size_t k=0;k<rdb.size(); k++) {
    string gname(rdb[k].getGivenName());
    string fname(rdb[k].getFamilyName());
    if(!gname.empty())
      givenName.push_back(gname);
    if(!fname.empty())
      familyName.push_back(fname);
  }
}


void RunnerDB::clearClubs()
{
  clearRunners(); // Runners refer to clubs. Clear runners
  cnhash.clear();
  chash.clear();
  cdb.clear();
}

void RunnerDB::clearRunners()
{
  nhash.clear();
  idhash.clear();
  rhash.clear();
  rdb.clear();
}

const vector<oClub> &RunnerDB::getClubDB() {
  return cdb;
}

const vector<RunnerDBEntry> &RunnerDB::getRunnerDB() {
  return rdb;
}

void RunnerDB::prepareLoadFromServer(int nrunner, int nclub) {
  loadedFromServer = true;
  clearClubs();
  cdb.reserve(nclub);
  rdb.reserve(nrunner);
}

