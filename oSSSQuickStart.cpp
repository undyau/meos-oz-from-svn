#include "stdafx.h"
#include "oSSSQuickStart.h"
#include "Download.h"
#include "gdioutput.h"
#include "csvparser.h"
#include "meos_util.h"
#include "metalist.h"
#include <ctime>

oSSSQuickStart::oSSSQuickStart(oExtendedEvent& a_Event):m_Event(a_Event)
{
}

oSSSQuickStart::~oSSSQuickStart(void)
{
}

bool oSSSQuickStart::ConfigureEvent(gdioutput &gdi)
{
// retrieve competition from install or web
string file = getTempFile();
if (!GetEventTemplateFromInstall(file))
	if (!GetEventTemplateFromWeb(file))
		return false;


gdi.setWaitCursor(true);
if(m_Event.open(file, true)) 
	{
	m_Event.updateTabs();
	gdi.setWindowTitle(m_Event.getTitleName());
	}
else
	{
	removeTempFile(file);
	return false;
	}
SYSTEMTIME st;
GetSystemTime(&st);

m_Event.setDate(convertSystemDate(st));
CustomiseClasses();

removeTempFile(file);

AddMeosOzCustomList(string("SSS Receipt Results.xml"));
AddMeosOzCustomList(string("SSS Results.xml"));

return true;
}

void oSSSQuickStart::AddMeosOzCustomList(string a_ReportDef)
{
// Add custom lists for SSS
	char path[MAX_PATH];
	if (getUserFile(path, a_ReportDef.c_str()))
		{
		string file(path);
		if (!fileExist(path))
			{
			char exepath[MAX_PATH];
			if (GetModuleFileName(NULL, exepath, MAX_PATH))
				{
				for (int i = strlen(exepath) - 1; i > 1; i--)
					if (exepath[i-1] == '\\')
						{
						exepath[i] = '\0';
						break;
						}
				strcat(exepath,a_ReportDef.c_str());
				if (fileExist(exepath))
					CopyFile(exepath, path, true);
				}
			}
		}
	if (fileExist(path))
		{
    xmlparser xml(0);
    xml.read(path);
    xmlobject xlist = xml.getObject(0);
    m_Event.synchronize();
    m_Event.getListContainer().load(MetaListContainer::ExternalList, xlist, false);
    m_Event.synchronize(true);
		}
}

bool oSSSQuickStart::GetEventTemplateFromInstall(string& a_File)
{
	  TCHAR ownPth[MAX_PATH]; 

    // Will contain exe path
    HMODULE hModule = GetModuleHandle(NULL);
    if (hModule != NULL) {
      GetModuleFileName(hModule,ownPth, (sizeof(ownPth)));
			int pos = strlen(ownPth) - 1;
			while (ownPth[pos] != '\\' && pos > 0)
				--pos;
			if (pos == 0)
				return false;
			ownPth[pos] = '\0';

			string templateFile(ownPth);
			templateFile += "\\sss201230.xml";
			if (!fileExist(templateFile.c_str()))
				return false;
			else
				return !!CopyFile(templateFile.c_str(), a_File.c_str(), FALSE);
		}
		else
			return false;
}

bool oSSSQuickStart::GetEventTemplateFromWeb(string& a_File)
{
  string url = "http://sportident.itsdamp.com/sss201230.xml";

  Download dwl;
  dwl.initInternet();
  std::vector<pair<string,string>> headers;

  try {
    dwl.downloadFile(url, a_File, headers);
  }
  catch (std::exception &) {
    removeTempFile(a_File);
    throw;
  }

  dwl.createDownloadThread();
  while (dwl.isWorking()) {
    Sleep(100);
  }

  return true;
}

void oSSSQuickStart::CustomiseClasses()
{
	// Set age/gender details for each class
  oClassList::iterator it;
  for (it=m_Event.Classes.begin();it!=m_Event.Classes.end();++it) 
		{
		if (it->getName().size() < 4)
			{
			string gender = it->getName().substr(it->getName().size()-1,1);
			if (gender == "W" || gender == "M")
					it->setSex(gender == "W" ? sFemale : sMale);

			int s(0);
			time_t t = time(0);   // get time now
			struct tm * now = localtime( & t );

			if (now->tm_mon + 1 <=6)
				s = 1; // for second half of season split over a year boundary, use last year's age
			if (it->getName().size() == 2)
				{
				switch (it->getName().at(0))
					{
					case 'J' : it->setAgeLimit(0 + s,20 + s); break;
					case 'O' : it->setAgeLimit(21 + s,34 + s); break;
					case 'M' : it->setAgeLimit(35 + s, 44 + s); break;
					case 'V' : it->setAgeLimit(45 + s, 54 +s); break;
					case 'L' : it->setAgeLimit(65 + s, 74 + s); break;
					case 'I' : it->setAgeLimit(74 + s, 200 + s);  break;
					}
				}
			else if (it->getName().substr(0,2) == "SV")
				it->setAgeLimit(55 + s,64 + s);
			}
		}
}