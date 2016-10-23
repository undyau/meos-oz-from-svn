#pragma once
#include "oExtendedEvent.h"
#pragma warning (disable: 4512)
class oSSSQuickStart
{
public:
  oSSSQuickStart(oExtendedEvent& a_Event);
  bool ConfigureEvent(gdioutput &gdi);
	int ImportCount() {return m_ImportCount;};
  ~oSSSQuickStart(void);

private:
  oExtendedEvent& m_Event;
  bool GetEventTemplateFromWeb(string& a_File);
	bool GetEventTemplateFromInstall(string& a_File);
	bool GetStartListFromWeb(string& a_File);
	void AddMeosOzCustomList(string a_ReportDef);
	int m_ImportCount;
};
#pragma warning (default: 4512)
