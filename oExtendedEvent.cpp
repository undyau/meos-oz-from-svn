#include "stdafx.h"
#include "oExtendedEvent.h"
#include "oSSSQuickStart.h"
#include "gdioutput.h"
#include "meos_util.h"
#include "localizer.h"

oExtendedEvent::oExtendedEvent(gdioutput &gdi) : oEvent(gdi)
{
    eventProperties["DoShortenNames"] = "0";   // default to behaving like vanilla MEOS
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
		std::string name = j->getName();
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
		j->setClassId(courseNewClassXref[j->getCourseId()]);
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


void oEvent::setShortNames(bool shorten)
{
  eventProperties["DoShortenNames"] = shorten ? "1" : "0";
}

string oEvent::shortenName(string name)
{
  if (eventProperties["DoShortenNames"] != "1")
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