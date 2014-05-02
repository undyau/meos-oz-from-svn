/************************************************************************
    MeOS - Orienteering Software
    Copyright (C) 2009-2014 Melin Software HB
    
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


#include "stdafx.h"
#include "meos_util.h"
#include "infoserver.h"
#include "xmlparser.h"
#include "oEvent.h"
#include "download.h"
#include "progress.h"
#include "meosException.h"
#include "gdioutput.h"

void base64_encode(const vector<BYTE> &input, string &output);

// Encode a vector vector int {{1}, {1,2,3}, {}, {4,5}} as "1;1,2,3;;4,5"
static void packIntInt(vector< vector<int> > v, string &def) {
  for (size_t j = 0; j < v.size(); j++) {
    if (j>0)
      def += ";";
    for (size_t k = 0; k < v[j].size(); k++) {
      if (k>0)
        def += ",";
      def += itos(v[j][k]);
    }
  }
}

InfoBase::InfoBase(int idIn) : id(idIn), committed(false){
}

InfoBase::InfoBase(const InfoBase &src) : id(src.id), committed(src.committed){
}

void InfoBase::operator=(const InfoBase &src) {
}

InfoBase::~InfoBase() {
}

int InfoBase::convertRelativeTime(const oBase &elem, int t) {
    return t+elem.getEvent()->getZeroTimeNum();
}

InfoCompetition::InfoCompetition(int id) : InfoBase(id) {
  forceComplete = true;
}

InfoRadioControl::InfoRadioControl(int id) : InfoBase(id) {
}

InfoClass::InfoClass(int id) : InfoBase(id) {
}

InfoOrganization::InfoOrganization(int id) : InfoBase(id) {
}

InfoBaseCompetitor::InfoBaseCompetitor(int id) : InfoBase(id) {
  organizationId = 0;
  classId = 0;
  status = 0;
  startTime = 0;
  runningTime = 0;
}

InfoCompetitor::InfoCompetitor(int id) : InfoBaseCompetitor(id) {
  totalStatus = 0;
  inputTime = 0;
}

InfoTeam::InfoTeam(int id) : InfoBaseCompetitor(id) {
}


bool InfoCompetition::synchronize(oEvent &oe, const set<int> &includeCls) {
  bool changed = false;
  if (oe.getName() != name) {
    name = oe.getName();
    changed = true;
  }

  if (oe.getDate() != date) {
    date = oe.getDate();
    changed = true;
  }

  if (oe.getDCI().getString("Organizer") != organizer) {
    organizer = oe.getDCI().getString("Organizer");
    changed = true;
  }

  if (oe.getDCI().getString("Homepage") != homepage) {
    homepage = oe.getDCI().getString("Homepage");
    changed = true;
  }

  if (changed)
    needCommit(*this);

  vector<pControl> ctrl;
  oe.getControls(ctrl);
  set<int> knownId;
  for (size_t k = 0; k < ctrl.size(); k++) {
    if (ctrl[k]->isValidRadio()) {
      int id = ctrl[k]->getId();
      knownId.insert(id);
      map<int, InfoRadioControl>::iterator res = controls.find(id);
      if (res == controls.end())
        res = controls.insert(make_pair(id, InfoRadioControl(id))).first;
      if (res->second.synchronize(*ctrl[k]))
        needCommit(res->second);
    }
  }

  // Check if something was deleted
  for (map<int, InfoRadioControl>::iterator it = controls.begin(); it != controls.end();) {
    if (!knownId.count(it->first)) {
      controls.erase(it++);
      forceComplete = true;
    }
    else
      ++it;
  }
  knownId.clear();

  vector<pClass> cls;
  oe.getClasses(cls);
  for (size_t k = 0; k < cls.size(); k++) {
    int id = cls[k]->getId();
    if (!includeCls.count(id))
      continue;
    knownId.insert(id);
    map<int, InfoClass>::iterator res = classes.find(id);
    if (res == classes.end())
      res = classes.insert(make_pair(id, InfoClass(id))).first;
    if (res->second.synchronize(*cls[k]))
      needCommit(res->second);
  }

  // Check if something was deleted
  for (map<int, InfoClass>::iterator it = classes.begin(); it != classes.end();) {
    if (!knownId.count(it->first)) {
      classes.erase(it++);
      forceComplete = true;
    }
    else
      ++it;
  }
  knownId.clear();

  vector<pClub> clb;
  oe.getClubs(clb, false);
  for (size_t k = 0; k < clb.size(); k++) {
    int id = clb[k]->getId();
    knownId.insert(id);
    map<int, InfoOrganization>::iterator res = organizations.find(id);
    if (res == organizations.end())
      res = organizations.insert(make_pair(id, InfoOrganization(id))).first;
    if (res->second.synchronize(*clb[k]))
      needCommit(res->second);
  }

  // Check if something was deleted
  for (map<int, InfoOrganization>::iterator it = organizations.begin(); it != organizations.end();) {
    if (!knownId.count(it->first)) {
      organizations.erase(it++);
      forceComplete = true;
    }
    else
      ++it;
  }
  knownId.clear();

  vector<pRunner> r;
  oe.getRunners(0, 0, r, false);
  for (size_t k = 0; k < r.size(); k++) {
    if (!includeCls.count(r[k]->getClassId()))
      continue;
    int id = r[k]->getId();
    knownId.insert(id);
    map<int, InfoCompetitor>::iterator res = competitors.find(id);
    if (res == competitors.end())
      res = competitors.insert(make_pair(id, InfoCompetitor(id))).first;
    if (res->second.synchronize(*this, *r[k]))
      needCommit(res->second);
  }

  // Check if something was deleted
  for (map<int, InfoCompetitor>::iterator it = competitors.begin(); it != competitors.end();) {
    if (!knownId.count(it->first)) {
      competitors.erase(it++);
      forceComplete = true;
    }
    else
      ++it;
  }
  knownId.clear();

  vector<pTeam> t;
  oe.getTeams(0, t, false);
  for (size_t k = 0; k < t.size(); k++) {
    if (!includeCls.count(t[k]->getClassId()))
      continue;
    int id = t[k]->getId();
    knownId.insert(id);
    map<int, InfoTeam>::iterator res = teams.find(id);
    if (res == teams.end())
      res = teams.insert(make_pair(id, InfoTeam(id))).first;
    if (res->second.synchronize(*t[k]))
      needCommit(res->second);
  }

  // Check if something was deleted
  for (map<int, InfoTeam>::iterator it = teams.begin(); it != teams.end();) {
    if (!knownId.count(it->first)) {
      teams.erase(it++);
      forceComplete = true;
    }
    else
      ++it;
  }
  knownId.clear();

  return !toCommit.empty() || forceComplete;
}

void InfoCompetition::needCommit(InfoBase &obj) {
  toCommit.push_back(&obj);
}

bool InfoRadioControl::synchronize(oControl &c) {
  const string &n = c.hasName() ? c.getName() : c.getString();
  if (n == name)
    return false;
  else {
    name = n;
    modified();
  }
  return true;
}

void InfoRadioControl::serialize(xmlparser &xml, bool diffOnly) const {
  vector< pair<string, string> > prop;
  prop.push_back(make_pair("id", itos(getId())));
  xml.write("ctrl", prop, name);
}

bool InfoClass::synchronize(oClass &c) {
  const string &n = c.getName();
  int no = c.getSortIndex();
  bool mod = false;
  vector< vector<int> > rc;
  size_t s = c.getNumStages();

  if (s > 0) {
    linearLegNumberToActual.clear();

    for (size_t k = 0; k < s; k++) {
      if (!c.isParallel(k) && !c.isOptional(k)) {      
        pCourse pc = c.getCourse(k, 0, true); // Get a course representative for the leg.

        rc.push_back(vector<int>());
        if (pc) {
          vector<pControl> ctrl;
          pc->getControls(ctrl);
          for (size_t j = 0; j < ctrl.size(); j++) {
            if (ctrl[j]->isValidRadio()) {
              rc.back().push_back(ctrl[j]->getId());
            }
          }
        }      
      }
      // Setup transformation map (flat to 2D)
      linearLegNumberToActual.push_back(max<int>(0, rc.size()-1));

    }
  }
  else {
    // Single stage
    linearLegNumberToActual.resize(1, 0);
    pCourse pc = c.getCourse(true); // Get a course representative for the leg. 
    rc.push_back(vector<int>());
    if (pc) {
      vector<pControl> ctrl;
      pc->getControls(ctrl);
      for (size_t j = 0; j < ctrl.size(); j++) {
        if (ctrl[j]->isValidRadio()) {
          rc.back().push_back(ctrl[j]->getId());
        }
      }
    }      
  }

  if (radioControls != rc) {
    radioControls = rc;
    mod = true;
  }

  if (n != name || no != sortOrder) {
    name = n;
    sortOrder = no;
    mod = true;
  }

  if (mod)
    modified();
  return mod;
}

void InfoClass::serialize(xmlparser &xml, bool diffOnly) const {
  vector< pair<string, string> > prop;
  prop.push_back(make_pair("id", itos(getId())));
  prop.push_back(make_pair("ord", itos(sortOrder)));
  string def;
  packIntInt(radioControls, def);
  prop.push_back(make_pair("radio", def));
  xml.write("cls", prop, name);
}

bool InfoOrganization::synchronize(oClub &c) {
  const string &n = c.getDisplayName();
  if (n == name)
    return false;
  else {
    name = n;
    modified();
  }
  return true;
}

void InfoOrganization::serialize(xmlparser &xml, bool diffOnly) const {
  xml.write("org", "id", itos(getId()), name);
}

void InfoCompetition::serialize(xmlparser &xml, bool diffOnly) const {
  vector< pair<string, string> > prop;
  prop.push_back(make_pair("date", date));
  prop.push_back(make_pair("organizer", organizer));
  prop.push_back(make_pair("homepage", homepage));
  xml.write("competition", prop, name);
}

void InfoBaseCompetitor::serialize(xmlparser &xml, bool diffOnly) const {
  vector< pair<string, string> > prop;
  prop.push_back(make_pair("org", itos(organizationId)));
  prop.push_back(make_pair("cls", itos(classId)));
  prop.push_back(make_pair("stat", itos(status)));
  prop.push_back(make_pair("st", itos(startTime)));
  prop.push_back(make_pair("rt", itos(runningTime)));
  xml.write("base", prop, name);
}

bool InfoBaseCompetitor::synchronizeBase(oAbstractRunner &bc) {
  const string &n = bc.getName();
  bool ch = false;
  if (n != name) {
    name = n;
    ch = true;
  }

  int cid = bc.getClubId();
  if (cid != organizationId) {
    organizationId = cid;
    ch = true;
  }

  int cls = bc.getClassId();
  if (cls != classId) {
    classId = cls;
    ch = true;
  }

  int s = bc.getStatus();
  if (status != s) {
    status = s;
    ch = true;
  }

  int st = convertRelativeTime(bc, bc.getStartTime()) * 10;
  if (st != startTime) {
    startTime = st;
    ch = true;
  }

  int rt = bc.getRunningTime() * 10;
  if (rt != runningTime) {
    runningTime = rt;
    ch = true;
  }
  return ch;
}

bool InfoCompetitor::synchronize(const InfoCompetition &cmp, oRunner &r) {
  bool ch = synchronizeBase(r);

  changeTotalSt = false;
  int s = r.getTotalStatus();
  if (totalStatus != s) {
    totalStatus = s;
    ch = true;
    changeTotalSt = true;
  }

  int legInput = r.getTotalRunningTime() * 10 - runningTime;
  if (legInput != inputTime) {
    inputTime = legInput;
    ch = true;
    changeTotalSt = true;
  }

  vector<RadioTime> newRT;
  if (r.getClassId() > 0)  {
    const vector<int> &radios = cmp.getControls(r.getClassId(), r.getLegNumber());
    for (size_t k = 0; k < radios.size(); k++) {
      RadioTime radioTime;
      RunnerStatus s_split;
      radioTime.radioId = radios[k];
      r.getSplitTime(radioTime.radioId, s_split, radioTime.runningTime);

      if (radioTime.runningTime > 0) {
        radioTime.runningTime*=10;
        newRT.push_back(radioTime);
      }
    }
  }
  changeRadio = false;
  if (newRT != radioTimes) {
    ch = true;
    changeRadio = true;
    radioTimes.swap(newRT);
  }

  if (ch) 
    modified();
  return ch;
}

void InfoCompetitor::serialize(xmlparser &xml, bool diffOnly) const {
  xml.startTag("cmp", "id", itos(getId()));
  InfoBaseCompetitor::serialize(xml, diffOnly);
  if (radioTimes.size() > 0 && (!diffOnly || changeRadio)) {
    string radio;    
    radio.reserve(radioTimes.size() * 12);
    for (size_t k = 0; k < radioTimes.size(); k++) {
      if (k>0)
        radio+=";";
      radio+=itos(radioTimes[k].radioId);
      radio+=",";
      radio+=itos(radioTimes[k].runningTime);
    }
    xml.write("radio", radio);
  }
  if (!diffOnly || changeTotalSt) {
    vector< pair<string, string> > prop;
    prop.push_back(make_pair("it", itos(inputTime)));
    prop.push_back(make_pair("tstat", itos(totalStatus)));
    xml.write("input", prop, "");
  }
  xml.endTag();
}


bool InfoTeam::synchronize(oTeam &t) {
  bool ch = synchronizeBase(t);
  
  const pClass cls = t.getClassRef();
  if (cls) {
    vector< vector<int> > r;

    size_t s = cls->getNumStages();
    for (size_t k = 0; k < s; k++) {
      pRunner rr = t.getRunner(k);
      int rid = rr != 0 ? rr->getId() : 0;

      if (cls->isParallel(k) || cls->isOptional(k)) {
        if (r.empty())
          r.push_back(vector<int>()); // This is not a valid case, really
        r.back().push_back(rid);
      }
      else
        r.push_back(vector<int>(1, rid));
    }

    if (r != competitors) {
      r.swap(competitors);
      ch = true;
    }

  }
  if (ch) 
    modified();
  return ch;
}

void InfoTeam::serialize(xmlparser &xml, bool diffOnly) const {
  xml.startTag("tm", "id", itos(getId()));
  InfoBaseCompetitor::serialize(xml, diffOnly);
  string def;
  packIntInt(competitors, def);
  xml.write("r", def);
  xml.endTag();
}

const vector<int> &InfoCompetition::getControls(int classId, int legNumber) const {
  map<int, InfoClass>::const_iterator res = classes.find(classId);
  
  if (res != classes.end()) {
    legNumber = res->second.linearLegNumberToActual[legNumber];
    const vector< vector<int> > &c = res->second.radioControls;
    if (size_t(legNumber) < c.size())
      return c[legNumber];
  }
  throw meosException("Internal class definition error");
}

int InfoCompetition::getCompleteXML(const string &destination, bool zip, gdioutput &utfConverter) {
  xmlparser xml(utfConverter.getEncoding() == ANSI ? 0 : &utfConverter);
  xml.openOutput(destination.c_str(), false);
  xml.startTag("MOPComplete", "xmlns", "http://www.melin.nu/mop");
  
  serialize(xml, false);

  for(map<int, InfoRadioControl>::iterator it = controls.begin(); it != controls.end(); ++it) {
    it->second.serialize(xml, false);
  }

  for(map<int, InfoClass>::iterator it = classes.begin(); it != classes.end(); ++it) {
    it->second.serialize(xml, false);
  }

  for(map<int, InfoOrganization>::iterator it = organizations.begin(); it != organizations.end(); ++it) {
    it->second.serialize(xml, false);
  }
  
  for(map<int, InfoCompetitor>::iterator it = competitors.begin(); it != competitors.end(); ++it) {
    it->second.serialize(xml, false);
  }

  for(map<int, InfoTeam>::iterator it = teams.begin(); it != teams.end(); ++it) {
    it->second.serialize(xml, false);
  }
  xml.endTag();
  return xml.closeOut();
}

int InfoCompetition::getDiffXML(const string &destination, bool zip, gdioutput &utfConverter) {
  if (forceComplete) {
    int res = getCompleteXML(destination, zip, utfConverter);    
    return res;
  }

  xmlparser xml(utfConverter.getEncoding() == ANSI ? 0 : &utfConverter);
  //string t = getTempFile();
  //xml.openMemoryOutput(false);
  xml.openOutput(destination.c_str(), false);
  xml.startTag("MOPDiff", "xmlns", "http://www.melin.nu/mop");
  for (list<InfoBase *>::iterator it = toCommit.begin(); it != toCommit.end(); ++it) {
    (*it)->serialize(xml, true);
  }
  xml.endTag();
  return xml.closeOut();

/*
  string tzip = getTempFile();
  zip(tzip.c_str(), 0, vector<string>(1, t));

  ifstream f(tzip.c_str(), ifstream::binary|ifstream::in);
  f.seekg(0, std::ifstream::end);
  size_t len = f.tellg();
  f.seekg(0, std::ifstream::beg);
  vector<BYTE> buff(len);
  f.read((char *)&buff[0], len);
  f.close();

  string output;
  base64_encode(buff, output);*/
  /*Download dwl;
  dwl.initInternet();
  ProgressWindow pw(0);
  vector<pair<string,string> > key;
  key.push_back(make_pair<string, string>("foo", "bar"));
  string result = getTempFile();
  dwl.postFile("http://www.melin.nu/online/test.php?id=1", t, result, key, pw);


  removeTempFile(result);
  removeTempFile(t);*/
  //removeTempFile(tzip);
  //xml.getMemoryOutput(res);  
}

void InfoCompetition::commitComplete() {
  toCommit.clear();
  forceComplete = false;
}

static char encoding_table[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
                                'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
                                'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
                                'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
                                'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
                                'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
                                'w', 'x', 'y', 'z', '0', '1', '2', '3',
                                '4', '5', '6', '7', '8', '9', '+', '/'};

static int mod_table[] = {0, 2, 1};

void base64_encode(const vector<BYTE> &input, string &encoded_data) {

  size_t input_length = input.size();
  size_t output_length = 4 * ((input_length + 2) / 3);
  encoded_data.resize(output_length);

  for (size_t i = 0, j = 0; i < input_length;) {
    unsigned octet_a = i < input_length ? input[i++] : 0;
    unsigned octet_b = i < input_length ? input[i++] : 0;
    unsigned octet_c = i < input_length ? input[i++] : 0;

    unsigned triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;

    encoded_data[j++] = encoding_table[(triple >> 3 * 6) & 0x3F];
    encoded_data[j++] = encoding_table[(triple >> 2 * 6) & 0x3F];
    encoded_data[j++] = encoding_table[(triple >> 1 * 6) & 0x3F];
    encoded_data[j++] = encoding_table[(triple >> 0 * 6) & 0x3F];
  }

  for (int i = 0; i < mod_table[input_length % 3]; i++)
    encoded_data[output_length - 1 - i] = '=';
}
