#include "stdafx.h"
#include "oSSSQuickStart.h"
#include "Download.h"
#include "gdioutput.h"
#include "csvparser.h"
#include "meos_util.h"

oSSSQuickStart::oSSSQuickStart(oExtendedEvent& a_Event):m_Event(a_Event)
{
}

oSSSQuickStart::~oSSSQuickStart(void)
{
}

bool oSSSQuickStart::ConfigureEvent(gdioutput &gdi)
{
// retrieve competition from web
string file = getTempFile();
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

removeTempFile(file);

// retrieve startlist from web
m_ImportCount = 0;
file = getTempFile();
if (!GetStartListFromWeb(file))
	return false;

csvparser csv;
gdi.addString("", 0, "Import Ór start-list...");
gdi.setWaitCursor(true);
if (!csv.ImportOr_CSV(m_Event, file.c_str())) {
	removeTempFile(file);
	return false;
}
else
	m_ImportCount = csv.nimport;

  //gdi.addString("", 0, "Klart. X deltagare importerade.#" + itos(csv.nimport));

//m_Event.liveResult(true);
//m_Event.liveResultUrl(string("http://sportident.itsdamp.com/LiveResultFromXML.php"));

return true;
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