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

private:
};
