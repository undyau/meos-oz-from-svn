#include "stdafx.h"
#include "oExtendedEvent.h"
#include "oSSSQuickStart.h"
#include "gdioutput.h"
#include "meos_util.h"
#include "localizer.h"
#include "gdifonts.h"

oExtendedEvent::oExtendedEvent(gdioutput &gdi) : oEvent(gdi)
{
    eventProperties["DoShortenClubNames"] = "0";   // default to behaving like vanilla MEOS
}

oExtendedEvent::~oExtendedEvent(void)
{
}

bool oExtendedEvent::SSSQuickStart(gdioutput &gdi)
{
  oSSSQuickStart qs(*this);
	if (qs.ConfigureEvent(gdi)){
		gdi.alert("Loaded " + itos(qs.ImportCount()) + " competitors onto start list");
		return true;
	}
	return false;
}

void oExtendedEvent::exportCourseOrderedIOFSplits(IOFVersion version, const char *file, bool oldStylePatrolExport, const set<int> &classes, int leg)
{
	// Create new classes names after courses
	std::map<int, int> courseNewClassXref;
	std::map<int, int> runnerOldClassXref;
	std::set<std::string> usedNames;

	for (oClassList::iterator j = Classes.begin(); j != Classes.end(); j++) {
		usedNames.insert(j->getName());
	}

	for (oCourseList::iterator j = Courses.begin(); j != Courses.end(); j++) {			
		std::string name = j->getName();
		if (usedNames.find(name) != usedNames.end()) {
				name = lang.tl("Course ") + name;
			}
		while (usedNames.find(name) != usedNames.end()) {
				name = name + "_";
			}

		usedNames.insert(name);
		pClass newClass = addClass(name, j->getId());
		courseNewClassXref[j->getId()] = newClass->getId();
	}
	
	// Reassign all runners to new classes, saving old ones
	for (oRunnerList::iterator j = Runners.begin(); j != Runners.end(); j++) {
		runnerOldClassXref[j->getId()] = j->getClassId();	
		int id = j->getCourse()->getId();
		j->setClassId(courseNewClassXref[id]);
	}

	// Do the export
	oEvent::exportIOFSplits(version, file, oldStylePatrolExport, classes, leg);

	// Reassign all runners back to original classes
	for (oRunnerList::iterator j = Runners.begin(); j != Runners.end(); j++) {
		j->setClassId(runnerOldClassXref[j->getId()]);
	}

	// Delete temporary classes
	for (std::map<int, int>::iterator j = courseNewClassXref.begin(); j != courseNewClassXref.end(); j++) {
		removeClass(j->second);
	}
}

void oExtendedEvent::analyseDNS(vector<pRunner> &unknown_dns, vector<pRunner> &known_dns, 
																vector<pRunner> &known, vector<pRunner> &unknown, std::list<oFreePunch> &strangers)
{
	oEvent::analyseDNS(unknown_dns, known_dns, known, unknown);
		
	strangers.empty();
	for (oFreePunchList::iterator it = punches.begin(); it!=punches.end(); ++it) {
		if (getCardByNumber(it->getCardNo()) == 0)
			strangers.push_back(*it);
		//strangers.sort();
		}
}


void oEvent::setShortClubNames(bool shorten)
{
  eventProperties["DoShortenClubNames"] = shorten ? "1" : "0";
}

string oEvent::shortenName(string name)
{
  if (eventProperties["DoShortenClubNames"] != "1")
    return name;

	std::map<string,string> clubs;
	clubs["ShortNameFor Abominable O-Men ACT"] = "AO.A";
	clubs["ShortNameFor Bushflyers ACT"] = "BS.A";
	clubs["ShortNameFor Canberra Forest Riders ACT"] = "CF.A";
	clubs["ShortNameFor Girls Grammar ACT"] = "GG.A";
	clubs["ShortNameFor Grammar Junior ACT"] = "GJ.A";
	clubs["ShortNameFor Grammar Senior ACT"] = "GS.A";
	clubs["ShortNameFor Parawanga Orienteers ACT"] = "PO.A";
	clubs["ShortNameFor Red Roos ACT"] = "RR.A";
	clubs["ShortNameFor St Edmunds ACT"] = "SE.A";
	clubs["ShortNameFor Weston Emus ACT"] = "WE.A";
	clubs["ShortNameFor Big Foot Orienteers"] = "BF.N";
	clubs["ShortNameFor Bennelong Northside Orienteers"] = "BN.N";
	clubs["ShortNameFor Central Coast Orienteers"] = "CC.N";
	clubs["ShortNameFor Garingal Orienteers"] = "GO.N";
	clubs["ShortNameFor Goldseekers Orienteers"] = "GS.N";
	clubs["ShortNameFor Illawarra Kareelah Orienteers"] = "IK.N";
	clubs["ShortNameFor Western Plains Orienteers"] = "WP.N";
	clubs["ShortNameFor Mountain Devils MTB Orienteering Club"] = "MD.N";
	clubs["ShortNameFor Newcastle Orienteering Club"] = "NC.N";
	clubs["ShortNameFor Northern Tablelands Orienteering Club"] = "NT.N";
	clubs["ShortNameFor Southern Highlands Occasional Orienteers"] = "SH.N";
	clubs["ShortNameFor Uringa Orienteers"] = "UR.N";
	clubs["ShortNameFor Western and Hills Orienteers"] = "WH.N";
	clubs["ShortNameFor Wagga and Riverina Occasional Orienteers"] = "WR.N";
	clubs["ShortNameFor Bullecourt Boulder Bounders"] = "BB.Q";
	clubs["ShortNameFor Bundaberg United Scrub Harriers"] = "BU.Q";
	clubs["ShortNameFor Enoggeroos"] = "EN.Q";
	clubs["ShortNameFor Fraser Region Orienteering Group"] = "FR.Q";
	clubs["ShortNameFor Multi Terrain Bike Orienteers"] = "MT.Q";
	clubs["ShortNameFor Paradise Lost Orienteers"] = "PL.Q";
	clubs["ShortNameFor Range Runners Orienteering Club"] = "RR.Q";
	clubs["ShortNameFor Sunshine Orienteers Club"] = "SO.Q";
	clubs["ShortNameFor Toohey Forest Orienteers"] = "TF.Q";
	clubs["ShortNameFor Townsville Thuringowa Orienteering Club"] = "TT.Q";
	clubs["ShortNameFor Ugly Gully Orienteers"] = "UG.Q";
	clubs["ShortNameFor University of Queensland"] = "UQ.Q";
	clubs["ShortNameFor Far North Orienteers"] = "NT.Q";
	clubs["ShortNameFor Lincoln Orienteers"] = "LI.S";
	clubs["ShortNameFor Onkaparinga Hills Orienteering Club"] = "OH.S";
	clubs["ShortNameFor Saltbush Orienteers"] = "SB.S";
	clubs["ShortNameFor South-East Orienteering Club"] = "SE.S";
	clubs["ShortNameFor Top End Orienteers"] = "TE.NT";
	clubs["ShortNameFor Tjuringa Orienteers"] = "TJ.S";
	clubs["ShortNameFor Tintookies Orienteers"] = "TT.S";
	clubs["ShortNameFor Wallaringa Orienteering Club"] = "WA.S";
	clubs["ShortNameFor Yalanga Orienteers"] = "YA.S";
	clubs["ShortNameFor Australopers Orienteering Club"] = "AL.T";
	clubs["ShortNameFor Esk Valley Orienteering Club"] = "EV.T";
	clubs["ShortNameFor Pathfinders Orienteering Club"] = "PF.T";
	clubs["ShortNameFor Wellington Ranges Orienteering Club"] = "WR.T";
	clubs["ShortNameFor Victorian ARDF Group"] = "AR.V";
	clubs["ShortNameFor Albury-Wodonga Orienteering Club"] = "AW.V";
	clubs["ShortNameFor Bayside Kangaroos Orienteers"] = "BK.V";
	clubs["ShortNameFor Bendigo Orienteers"] = "BG.V";
	clubs["ShortNameFor Brumby Orienteering Club"] = "BR.V";
	clubs["ShortNameFor Central Highlands Orienteering Club"] = "CH.V";
	clubs["ShortNameFor Dandenong Ranges Orienteering Club"] = "DR.V";
	clubs["ShortNameFor Eureka Orienteers"] = "EU.V";
	clubs["ShortNameFor Melbourne Forest Racers"] = "MF.V";
	clubs["ShortNameFor Nillumbik Emus Orienteering Club"] = "NE.V";
	clubs["ShortNameFor Tuckonie Orienteering Club"] = "TK.V";
	clubs["ShortNameFor Yarra Valley Orienteering Club"] = "YV.V";
	clubs["ShortNameFor Bibbulmun Orienteers"] = "BO.W";
	clubs["ShortNameFor Kulgun 225 Orienteers"] = "KO.W";
	clubs["ShortNameFor LOST"] = "LO.W";
	clubs["ShortNameFor South-West Orienteering Trekkers"] = "SW.W";
	clubs["ShortNameFor Wulundigong Orienteers of the West"] = "WO.W";
	clubs["ShortNameFor Canberra Cockatoos"] = "CC.A";
	clubs["ShortNameFor NSW Stingers"] = "ST.N";
	clubs["ShortNameFor Queensland Cyclones"] = "QC.Q";
	clubs["ShortNameFor Southern Arrows"] = "SW.S";
	clubs["ShortNameFor Tasmanian Foresters"] = "TAS.A";
	clubs["ShortNameFor Victorian Nuggets"] = "VN.V";
	clubs["ShortNameFor Western Nomads"] = "WN.W";
	clubs["ShortNameFor Australian Bushrangers"] = "AUB.AUS";
	clubs["ShortNameFor Australian Challenge team"] = "AUC.AUS";
	clubs["ShortNameFor New Zealand Challenge team"] = "NZC.NZL";
	clubs["ShortNameFor New Zealand Pinestars"] = "NZP.NZL";
	clubs["ShortNameFor Auckland"] = "AK.NZL";
	clubs["ShortNameFor Counties Manakau"] = "CM.NZL";
	clubs["ShortNameFor Dunedin"] = "DN.NZL";
	clubs["ShortNameFor Hamilton"] = "HA.NZL";
	clubs["ShortNameFor Hawkes Bay"] = "HB.NZL";
	clubs["ShortNameFor Hutt Valley"] = "HV.NZL";
	clubs["ShortNameFor Malborough"] = "MB.NZL";
	clubs["ShortNameFor Nelson"] = "NL.NZL";
	clubs["ShortNameFor North West"] = "NW.NZL";
	clubs["ShortNameFor Peninsula and Plains"] = "PP.NZL";
	clubs["ShortNameFor Red Kiwi"] = "RK.NZL";
	clubs["ShortNameFor Rotorua"] = "RO.NZL";
	clubs["ShortNameFor Southland"] = "SD.NZL";
	clubs["ShortNameFor Taranaki"] = "TA.NZL";
	clubs["ShortNameFor Taupo"] = "TP.NZL";
	clubs["ShortNameFor Wairarapa"] = "WA.NZL";
	clubs["ShortNameFor Wellington"] = "WN.NZL";
	clubs["ShortNameFor Far North Orienteers"] = "FN.Q";


	// Check for presence in map of shortened names
	string key("ShortNameFor " + name);
	
	if (eventProperties.find(key) != eventProperties.end())
		return eventProperties[name];
	
	if (clubs.find(key) != clubs.end())
		{
		eventProperties[key] = clubs[key];
		return clubs[key];
		}

	// Shorten mechanically
	std::vector<string> words;
	unsigned int pos = 0;
	while ((pos = name.find(" ", pos)) != string::npos)
		{
		string temp = name.substr(0,pos - 1);
		temp = trim(temp);
		if (temp.length() > 0)
			words.push_back(temp);
		if (pos < name.length())
			name = name.substr(pos + 1);
		else
			pos = string::npos;
		}
	if (words.size() > 1)
		{
		if (stringMatch(words[words.size() -1].substr(0,10), "orienteers"))
			words.pop_back();
		string club;
		for (unsigned int i = 0; i < words.size(); i++)
			if (words[i] == "OC" || words[i] == "OK")
				club += words[i];
			else
				club += toupper(words[i][0]);
		return club;
		}
	else
		return name.substr(0,5);
}



/* Would like a semi-open interval [min, max) */
int random_in_range (unsigned int min, unsigned int max)
{
  int base_random = rand(); /* in [0, RAND_MAX] */
  if (RAND_MAX == base_random) return random_in_range(min, max);
  /* now guaranteed to be in [0, RAND_MAX) */
  int range       = max - min,
      remainder   = RAND_MAX % range,
      bucket      = RAND_MAX / range;
  /* There are range buckets, plus one smaller interval
     within remainder of RAND_MAX */
  if (base_random < RAND_MAX - remainder) {
    return min + base_random/bucket;
  } else {
    return random_in_range (min, max);
  }
}

void oExtendedEvent::simpleDrawRemaining(gdioutput &gdi, const string &firstStart, 
                               const string &minIntervall, const string &vacances)
{
  gdi.refresh();
  gdi.fillDown();
  gdi.addString("", 1, "MEOS-OZ custom randomised gap filling draw").setColor(colorGreen);
  gdi.addString("", 0, "Inspekterar klasser...");
  gdi.refreshFast();

  int baseInterval = convertAbsoluteTimeMS(minIntervall);
  int iFirstStart = getRelativeTime(firstStart);
  double vacancy = atof(vacances.c_str())/100;

  // Find latest allocated start-time
	int latest(0), toBeSet(0);
  std::map<pCourse, int> counts;
  for (oRunnerList::iterator j = Runners.begin(); j != Runners.end(); j++) {
    latest = max(latest, j->getStartTime());
    if (j->getStartTime() == 0)
      toBeSet++;

    if (j->getCourse() != NULL)
      {
      if (counts.find(j->getCourse()) != counts.end())
         counts[j->getCourse()] = counts[j->getCourse()] + 1;
      else
         counts[j->getCourse()] = 1;
      }
	}

  char bf[256];
  sprintf_s(bf, lang.tl("L�pare utan starttid: %d.").c_str(), toBeSet);
  gdi.addStringUT(1, bf);
  gdi.refreshFast();

  // Find course with most entries
  int largest(0);
  for (std::map<pCourse, int>::iterator i = counts.begin(); i != counts.end(); i++)
     if ((i->second) > largest)
       largest = i->second;


  // Determine required last start - if its earlier than existing last start, use that.
  int range = int ((largest * baseInterval * (100 + vacancy + 1)) /100);
  if (iFirstStart + range < latest)
    range = latest - iFirstStart;

  // Allocate start for each course
	for (oCourseList::iterator j = Courses.begin(); j != Courses.end(); j++) 
    {
    std::map<int, oRunner*> starts;
    std::vector<oRunner*> notset;
    for (oRunnerList::iterator k = Runners.begin(); k != Runners.end(); k++) 
      {
      if (k->getCourse() == &(*j))
        {
        if (k->getStartTime() > 0)
          starts[k->getStartTime()] = &(*k);
        else
          notset.push_back(&(*k));
        }
      }

    for (unsigned int i = 0; i < notset.size(); i++)
      {
      int target = iFirstStart + baseInterval * random_in_range(0, range/baseInterval);
      while (starts.find(target) != starts.end())
        {
        target = iFirstStart + baseInterval * random_in_range(0, range/baseInterval);
        }
      starts[target] = notset[i];
      }

    for (std::map<int, oRunner*>::iterator l = starts.begin(); l != starts.end(); l++)
      l->second->setStartTime(l->first);
    }

  return;
}



void oEvent::calculateCourseRogainingResults()
{
	sortRunners(CoursePoints);
	oRunnerList::iterator it;

	int cCourseId=-1;
  int cPlace = 0;
	int vPlace = 0;
	int cTime = numeric_limits<int>::min();;
  int cDuplicateLeg=0;
  bool useResults = false;
  bool isRogaining = false;
  bool invalidClass = false;

	for (it=Runners.begin(); it != Runners.end(); ++it) {
    if (it->isRemoved())
      continue;

    if (it->getCourseId()!=cCourseId || it->tDuplicateLeg!=cDuplicateLeg) {
			cCourseId = it->getCourseId();
      useResults = it->Class ? !it->Class->getNoTiming() : false;
			cPlace = 0;
			vPlace = 0;
			cTime = numeric_limits<int>::min();
      cDuplicateLeg = it->tDuplicateLeg;
      isRogaining = it->Class ? it->Class->isRogaining() : false;
      invalidClass = it->Class ? it->Class->getClassStatus() != oClass::Normal : false;
		}
	
    if (!isRogaining)
      continue;

    if (invalidClass) {
      it->tTotalPlace = 0;
      it->tPlace = 0;
    }
    else if(it->Status==StatusOK) {
			cPlace++;

      int cmpRes = 3600 * 24 * 7 * it->tRogainingPoints - it->getRunningTime();

      if(cmpRes != cTime)
				vPlace = cPlace;

			cTime = cmpRes;

      if (useResults)
			  it->tPlace = vPlace;
      else
        it->tPlace = 0;
		}
		else
			it->tPlace = 99000 + it->Status;
	}
}


void oExtendedEvent::listStrangers(gdioutput &gdi, std::list<oFreePunch> &strangers)
{
  char bf[64];
  int yp = gdi.getCY();
  int xp = gdi.getCX();

	for (std::list<oFreePunch>::const_iterator k = strangers.begin(); k != strangers.end(); k++) {
		sprintf_s(bf, "%d", k->getCardNo());
		gdi.addStringUT(yp, xp, 0, bf);
		gdi.addStringUT(yp, xp+100, 0, k->getTime());
		pRunner runner;
		if (runner = dbLookUpByCard(k->getCardNo()))
			gdi.addStringUT(yp, xp+200, 0, string("possibly ") + runner->getName());
		else
			gdi.addStringUT(yp, xp+200, 0, string("unknown"));
		yp += gdi.getLineHeight();
		}
}