#pragma once
#include "oevent.h"

class oExtendedEvent :
  public oEvent
{
public:
  oExtendedEvent(gdioutput &gdi);
  ~oExtendedEvent(void);


	bool SSSQuickStart(gdioutput &gdi);
	void exportCourseOrderedIOFSplits(IOFVersion version, const char *file, bool oldStylePatrolExport, const set<int> &classes, int leg);
  void simpleDrawRemaining(gdioutput &gdi, const string &firstStart, 
                               const string &minIntervall, const string &vacances);
	void analyseDNS(vector<pRunner> &unknown_dns, vector<pRunner> &known_dns, 
		vector<pRunner> &known, vector<pRunner> &unknown, 
		std::list<oFreePunch> &strangers, std::list<oFreePunch> &unknown_reused);
	void listStrangers(gdioutput &gdi, std::list<oFreePunch> &strangers);
	void listLatePunches(gdioutput &gdi, std::list<oFreePunch> &late_punches);
private:
};
