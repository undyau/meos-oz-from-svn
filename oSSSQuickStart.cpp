#include "stdafx.h"
#include "oSSSQuickStart.h"
#include "Download.h"
#include "gdioutput.h"
#include "csvparser.h"
#include "meos_util.h"
#include "metalist.h"

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

removeTempFile(file);

// retrieve startlist from web

bool load = gdi.ask("Do you want to load a \"start\" list from the web ?\r\n\r\nIt will contain most regular SSS runners.");
m_ImportCount = 0;

if (load)
	{
	file = getTempFile();
	if (!GetStartListFromWeb(file))
		return false;

	csvparser csv;
	gdi.addString("", 0, "Import �r start-list...");
	gdi.setWaitCursor(true);
	if (!csv.ImportOr_CSV(m_Event, file.c_str())) 
		{
		removeTempFile(file);
		return false;
		}
	else
		m_ImportCount = csv.nimport;

	gdi.alert("Loaded " + itos(ImportCount()) + " competitors onto start list");
	}

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

bool oSSSQuickStart::GetStartListFromWeb(string& a_File)
{
  string url("http://sportident.itsdamp.com/SSSOrStartList.csv");
  
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