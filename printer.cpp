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

#include <commdlg.h>
#include "gdioutput.h"
#include "oEvent.h"
#include "meos_util.h"
#include "Table.h"

extern gdioutput *gdi_main;
static bool bPrint;
const double inchmmk=2.54;


BOOL CALLBACK AbortProc(HDC,	int  iError)
{
	if(iError==0)
		return bPrint;
	else
	{
		bPrint=false;
		return false;
	}
}


#define WM_SETPAGE WM_USER+423

BOOL CALLBACK  AbortPrintJob(HWND hDlg, UINT message, WPARAM wParam, LPARAM)
{
	/*switch(message)
	{
	case WM_INITDIALOG:
	return true;

	case WM_SETPAGE:
	SetDlgItemInt(hDlg, IDC_PAGE, wParam, false);
	break;

	case WM_COMMAND:
	switch(LOWORD(wParam))
	{
	case IDCANCEL:
	bPrint=false;
	EndDialog(hDlg,false);
	break;
	}
	return true;

	}*/
	return false;
}

PrinterObject::PrinterObject()
{
  hDC=0;
	hDevMode=0;
	hDevNames=0;

  nPagesPrinted=0;
  nPagesPrintedTotal=0;
  onlyChanged=true;

  memset(&ds, 0, sizeof(ds));
  memset(&DevMode, 0, sizeof(DevMode));
}

PrinterObject::PrinterObject(const PrinterObject &po)
{
  hDC=0;
  hDevMode=po.hDevMode;
  hDevNames=po.hDevNames;
  if (hDevMode)
    GlobalLock(hDevMode);
  if (hDevNames)
    GlobalLock(hDevNames);
  nPagesPrinted=0;
  nPagesPrintedTotal=0;
  onlyChanged=true;

  memcpy(&ds, &po.ds, sizeof(ds));
  memcpy(&DevMode, &po.DevMode, sizeof(DevMode));
}

void PrinterObject::operator=(const PrinterObject &po)
{
  hDC = 0;///???po.hDC;
  hDevMode=po.hDevMode;
  hDevNames=po.hDevNames;
  if (hDevMode)
    GlobalLock(hDevMode);
  if (hDevNames)
    GlobalLock(hDevNames);

  nPagesPrinted = po.nPagesPrinted;
  nPagesPrintedTotal = po.nPagesPrintedTotal;
  onlyChanged = po.onlyChanged;

  memcpy(&ds, &po.ds, sizeof(ds));
  memcpy(&DevMode, &po.DevMode, sizeof(DevMode));
}

PrinterObject::~PrinterObject()
{
  if(hDC)
    DeleteDC(hDC);

  freePrinter();
}

void gdioutput::printPage(PrinterObject &po, int StartY, int &EndY, bool calculate)
{
  HDC hDC=po.hDC;
  PrinterObject::DATASET &ds=po.ds;

	TIList::iterator it;
	int MaxYThisPage=StartY+(ds.PageY-2*ds.MarginY);

  vector<TextInfo *> lastRow;
	ds.LastPage=true;
  bool monotone = true;
	EndY=StartY;
  int lastEndY = EndY;
	for (it=TL.begin();it!=TL.end(); ++it) {
		TextInfo text=*it;
		text.yp=int(it->yp*ds.Scale);
		text.xp=int(it->xp*ds.Scale);

		if(text.format!=10 && text.yp>StartY && text.yp+text.getHeight()*ds.Scale<MaxYThisPage)	{
      if ( text.yp > EndY ) {
        lastEndY = EndY;
        EndY = text.yp;
        lastRow.clear();
      }
      else if (text.yp < EndY)
        monotone = false;
			if (text.format==pageNewPage) {
				ds.LastPage = false;
				return;
			}
      else if (text.format==pageReserveHeight) 
        continue;
      else if (text.format==pagePageInfo) {
        pageInfo=text;
        continue;
      }

      text.yp=text.yp-StartY+ds.MarginY;
		  text.Highlight=0;
		  text.HasCapture=0;
		  text.Active=0;
  	  text.CallBack=0;
      text.HasTimer=false;

  		if (text.format==0)
    		text.format=fontSmall; //Small text

      if (calculate) {
        transformedPageText.push_back(text);
        if ( (EndY-StartY+ds.MarginY) == text.yp) {
          lastRow.push_back(&transformedPageText.back());
        }
        CalculateCS(text);
      }
      else 
			  RenderString(text, hDC);		
		}
		else if(text.yp>=MaxYThisPage)
			ds.LastPage=false;
	}

  bool isTitle = false;
  if (monotone) {
    for (size_t k = 0; k<lastRow.size(); k++) {
      if (lastRow[k]->format>=1 && lastRow[k]->format<=5) {
        isTitle = true;
      }
    }
  }
  if (isTitle && !ds.LastPage) {
    // Move last title row to next page
    for (size_t k = 0; k<lastRow.size(); k++) 
      lastRow[k]->format = formatIgnore;

    EndY = lastEndY;
  }
}

void gdioutput::printPage(PrinterObject &po, int nPage, int nPageMax)
{
	TIList::iterator it;

  for (it=transformedPageText.begin();it!=transformedPageText.end(); ++it) {
    if (it->format == formatIgnore)
      continue;
	  RenderString(*it, po.hDC);
  }

  char bf[512];
  if (printHeader) {
    if(!pageInfo.text.empty())
      sprintf_s(bf, "MeOS %s, %s, s. %d/%d", getLocalTime().c_str(),
        pageInfo.text.c_str(), nPage, nPageMax);
    else
      sprintf_s(bf, "MeOS %s, s. %d/%d", getLocalTime().c_str(), nPage, nPageMax);
    
    TextInfo t;
    t.yp=po.ds.MarginY;
    t.xp=po.ds.PageX-po.ds.MarginX;
    t.text=bf;
    t.format=textRight|fontSmall;
    RenderString(t, po.hDC);
  }
}

//Uses format, text, xp and yp
void gdioutput::CalculateCS(TextInfo &text)
{
  if(text.format==pageNewPage || 
      text.format==pageReserveHeight ||
      text.text.empty())
    return;
  DWORD localCS=0; 
  DWORD localCS2=0; 

  for(DWORD i=0; i<text.text.size(); i++){
    localCS+=(BYTE(text.text[i])<<(i%24))*(i*i+text.yp);
    localCS2+=(BYTE(text.text[i])<<(i%23))*(i+17)*(i+text.xp);
  }

  localCS+=(text.format*10000000+text.xp+text.yp*1000);  
  globalCS=globalCS+__int64(localCS)+(__int64(localCS2)<<32);
}

void gdioutput::printSetup(PrinterObject &po)
{
  destroyPrinterDC(po);

  PRINTDLG pd;
  memset(&pd, 0, sizeof(pd));

  pd.lStructSize = sizeof(PRINTDLG);
	pd.hDevMode = po.hDevMode;
	pd.hDevNames = po.hDevNames;
  /*
  HGLOBAL devMode = 0, devName = 0;

  if (!po.Device.empty()) {
    devMode = GlobalAlloc(GHND, sizeof(DEVMODE));
    devName = GlobalAlloc(GHND, sizeof(DEVNAMES));
    DEVMODE *dm = (DEVMODE *)GlobalLock(devMode);
    *dm = po.DevMode;
    GlobalUnlock(devMode);
    
  }*/
  
  pd.Flags = PD_RETURNDC|PD_USEDEVMODECOPIESANDCOLLATE|PD_PRINTSETUP;
	pd.hwndOwner = hWndAppMain;
  pd.hDC = (HDC) po.hDC; 
	pd.nFromPage = 1;
	pd.nToPage = 1;
	pd.nMinPage = 1;
	pd.nMaxPage = 1;
	pd.nCopies = 1;
	pd.hInstance = (HINSTANCE) NULL;
	pd.lCustData = 0L;
	pd.lpfnPrintHook = (LPPRINTHOOKPROC) NULL;
	pd.lpfnSetupHook = (LPSETUPHOOKPROC) NULL;
	pd.lpPrintTemplateName = (LPSTR) NULL;
	pd.lpSetupTemplateName = (LPSTR)  NULL;
	pd.hPrintTemplate = (HANDLE) NULL;
	pd.hSetupTemplate = (HANDLE) NULL;

	int iret=PrintDlg(&pd);

	if (iret==false) {
		int error = CommDlgExtendedError();
		if (error!=0) {
			char sb[127];
			sprintf_s(sb, "Printing Error Code=%d", error);
			//MessageBox(hWnd, sb, NULL, MB_OK);
			alert(sb);
      po.freePrinter();
      po.hDC = 0;
		}
		return;
	}
	else {
		//Save settings!
		po.hDevMode = pd.hDevMode;
		po.hDevNames = pd.hDevNames;
    po.hDC=pd.hDC;

    DEVNAMES *dn=(LPDEVNAMES)GlobalLock(pd.hDevNames);
		if(dn) {
      DEVMODE *dm=(LPDEVMODE)GlobalLock(pd.hDevMode);
      if (dm) {
        po.DevMode=*dm;
        po.DevMode.dmSize=sizeof(po.DevMode);       
      }
			po.Driver=(char *)(dn)+dn->wDriverOffset;
			po.Device=(char *)(dn)+dn->wDeviceOffset;      
      //GlobalUnlock(pd.hDevMode);
		}
    //GlobalUnlock(pd.hDevNames);
	}
}

void gdioutput::print(pEvent oe, Table *t, bool printMeOSHeader, bool noMargin)
{
  printHeader = printMeOSHeader;
  noPrintMargin = noMargin;

  setWaitCursor(true);
	PRINTDLG pd;
    
  PrinterObject &po=po_default;
  po.printedPages.clear();//Don't remember

	pd.lStructSize = sizeof(PRINTDLG);
	pd.hDevMode = po.hDevMode;
	pd.hDevNames = po.hDevNames;
	pd.Flags = PD_RETURNDC;
	pd.hwndOwner = hWndAppMain;
	pd.hDC = (HDC) NULL; 
	pd.nFromPage = 1;
	pd.nToPage = 1;
	pd.nMinPage = 1;
	pd.nMaxPage = 1;
	pd.nCopies = 1;
	pd.hInstance = (HINSTANCE) NULL;
	pd.lCustData = 0L;
	pd.lpfnPrintHook = (LPPRINTHOOKPROC) NULL;
	pd.lpfnSetupHook = (LPSETUPHOOKPROC) NULL;
	pd.lpPrintTemplateName = (LPSTR) NULL;
	pd.lpSetupTemplateName = (LPSTR)  NULL;
	pd.hPrintTemplate = (HANDLE) NULL;
	pd.hSetupTemplate = (HANDLE) NULL;

	int iret=PrintDlg(&pd);

	if(iret==false) {
		int error=CommDlgExtendedError();
		if(error!=0) {
			char sb[128];
			sprintf_s(sb, "Printing Error Code=%d", error);
			alert(sb);
      po.freePrinter();
      po.hDC = 0;
		}
		return;
	}
  setWaitCursor(true);
	//Save settings!
  po.freePrinter();

	po.hDevMode = pd.hDevMode;
	po.hDevNames = pd.hDevNames;
  po.hDC = pd.hDC;

  if (t)
    t->print(*this, po.hDC, 20, 0);

	DEVNAMES *dn=(LPDEVNAMES)GlobalLock(pd.hDevNames);
	if(dn) {
    DEVMODE *dm=(LPDEVMODE)GlobalLock(pd.hDevMode);
    if (dm) {
      po.DevMode=*dm;
      po.DevMode.dmSize=sizeof(po.DevMode);       
    }
		po.Driver=(char *)(dn)+dn->wDriverOffset;
		po.Device=(char *)(dn)+dn->wDeviceOffset;      
    //GlobalUnlock(pd.hDevMode);
    //GlobalUnlock(pd.hDevNames);
	}

  doPrint(po, oe);

	// Delete the printer DC.
	DeleteDC(pd.hDC);
  po.hDC=0;
}

void gdioutput::print(PrinterObject &po, pEvent oe, bool printMeOSHeader, bool noMargin)
{
  printHeader = printMeOSHeader;
  noPrintMargin = noMargin;
  if (po.hDevMode==0) {
  //if (po.Driver.empty()) {
    PRINTDLG pd;
 
	  pd.lStructSize = sizeof(PRINTDLG);
	  pd.hDevMode = 0;
	  pd.hDevNames = 0;
	  pd.Flags = PD_RETURNDEFAULT;
	  pd.hwndOwner = hWndAppMain;
	  pd.hDC = (HDC) NULL; 
	  pd.nFromPage = 1;
	  pd.nToPage = 1;
	  pd.nMinPage = 1;
	  pd.nMaxPage = 1;
	  pd.nCopies = 1;
	  pd.hInstance = (HINSTANCE) NULL;
	  pd.lCustData = 0L;
	  pd.lpfnPrintHook = (LPPRINTHOOKPROC) NULL;
	  pd.lpfnSetupHook = (LPSETUPHOOKPROC) NULL;
	  pd.lpPrintTemplateName = (LPSTR) NULL;
	  pd.lpSetupTemplateName = (LPSTR)  NULL;
	  pd.hPrintTemplate = (HANDLE) NULL;
	  pd.hSetupTemplate = (HANDLE) NULL;

    int iret=PrintDlg(&pd);

    if(iret==false) {
	    int error=CommDlgExtendedError();
	    if(error!=0) {
		    char sb[128];
		    sprintf_s(sb, "Printing Error Code=%d", error);
		    alert(sb);
        po.hDC = 0;
	    }
	    return;
    }
    po.freePrinter();
	  po.hDevMode = pd.hDevMode;
	  po.hDevNames = pd.hDevNames;
  
		DEVNAMES *dn=(LPDEVNAMES)GlobalLock(pd.hDevNames);
		if(dn) {
      DEVMODE *dm=(LPDEVMODE)GlobalLock(pd.hDevMode);
      if (dm) {
        po.DevMode=*dm;
        po.DevMode.dmSize=sizeof(po.DevMode);       
      }

			po.Driver=(char *)(dn)+dn->wDriverOffset;
			po.Device=(char *)(dn)+dn->wDeviceOffset;      
      po.hDC=CreateDC(po.Driver.c_str(), po.Device.c_str(), NULL, dm);      
      GlobalUnlock(pd.hDevMode);
		}
    GlobalUnlock(pd.hDevNames);
  }
  else if (po.hDC==0) {
    po.hDC = CreateDC(po.Driver.c_str(), po.Device.c_str(), NULL, &po.DevMode);
  }
  doPrint(po, oe);
}

void gdioutput::destroyPrinterDC(PrinterObject &po)
{
  if (po.hDC) {
	  // Delete the printer DC.
	  DeleteDC(po.hDC);
    po.hDC=0;
  }
}

bool gdioutput::startDoc(PrinterObject &po)
{
  // Initialize the members of a DOCINFO structure.
	DOCINFO di;
	int nError;
	di.cbSize = sizeof(DOCINFO);

	char sb[256];
	sprintf_s(sb, "MeOS");
	
	di.lpszDocName = sb;
	di.lpszOutput = (LPTSTR) NULL;
	di.lpszDatatype = (LPTSTR) NULL;
	di.fwType = 0;      // Begin a print job by calling the StartDoc function.

	nError = StartDoc(po.hDC, &di);

	if (nError <= 0) {
    nError=GetLastError();
		DeleteDC(po.hDC);
    po.hDC=0;
		//sprintf_s(sb, "Window's StartDoc API returned with error code %d,", nError);
    alert("StartDoc error: " + getErrorMessage(nError));
		return false;
	}
  return true;
}

bool gdioutput::doPrint(PrinterObject &po, pEvent oe)
{
  setWaitCursor(true);

  if (!po.hDC)
    return false;
  set<__int64> myPages;

  po.nPagesPrinted=0;
  PrinterObject::DATASET &ds=po.ds;

	//Do the printing
	int xsize = GetDeviceCaps(po.hDC, HORZSIZE);
	int ysize = GetDeviceCaps(po.hDC, VERTSIZE);

	// Retrieve the number of pixels-per-logical-inch in the
	// horizontal and vertical directions for the printer upon which
	// the bitmap will be printed.
	int xtot = GetDeviceCaps(po.hDC, HORZRES);
	int ytot = GetDeviceCaps(po.hDC, VERTRES);

	SetMapMode(po.hDC, MM_ISOTROPIC);

  const bool limitSize = xsize > 100;

  int PageXMax=limitSize ? max(512, MaxX) : MaxX;
	int PageYMax=(ysize*PageXMax)/xsize;
	SetWindowExtEx(po.hDC, int(PageXMax*1.05), PageYMax, 0);
	SetViewportExtEx(po.hDC, xtot, ytot, NULL); 

	ds.PageX=PageXMax;
	ds.PageY=PageYMax;
  ds.MarginX=noPrintMargin ? (limitSize ? PageXMax/30: 5) : PageXMax/25;
  ds.MarginY=noPrintMargin ? 5 : 20;
	ds.Scale=1;
	ds.LastPage=false;
	int EndY=0;
	int StartY=0;

  list< list<TextInfo> > pagesToPrint;
  int sOffsetY = OffsetY;
  int sOffsetX = OffsetX;
  OffsetY = 0;
  OffsetX = 0;

	while (!ds.LastPage) {
    //Calculate checksum for this page to see if it has been already printed.
    globalCS=0;
    transformedPageText.clear();
    pageInfo=TextInfo();
    printPage(po, StartY, EndY, true);
		StartY=EndY;

    if(!transformedPageText.empty()){
      if(!po.onlyChanged || po.printedPages.count(globalCS)==0) {
        
        pagesToPrint.push_back(list<TextInfo>());

        swap(pagesToPrint.back(), transformedPageText);
        pagesToPrint.back().push_back(pageInfo);

        myPages.insert(globalCS);
        po.nPagesPrinted++;
        po.nPagesPrintedTotal++;
      }
    }
    transformedPageText.clear();
	}

  int nPagesToPrint=pagesToPrint.size();

  if (nPagesToPrint>0) {
   
    if (!startDoc(po)) {
      return false;
    }
    int page=1;
    while (!pagesToPrint.empty()) {

      int nError = StartPage(po.hDC);
      if (nError <= 0) {
        nError=GetLastError();
        
	      EndDoc(po.hDC);        
	      DeleteDC(po.hDC);
        po.hDC=0;
        po.freePrinter();
        OffsetY = sOffsetY;
        OffsetX = sOffsetX;
        alert("StartPage error: " + getErrorMessage(nError));
		
	      return false;
      }
      swap(pagesToPrint.front(), transformedPageText);
      pagesToPrint.pop_front();
      swap(transformedPageText.back(), pageInfo);
      transformedPageText.pop_back();
      
      printPage(po, page++, nPagesToPrint);
      EndPage(po.hDC);
    }

	  int nError = EndDoc(po.hDC);
    OffsetY = sOffsetY;
    OffsetX = sOffsetX;

    if (nError <= 0) {
	    nError=GetLastError();
      DeleteDC(po.hDC);
      po.hDC=0;
      alert("EndDoc error: " + getErrorMessage(nError));
      return false;
	  }
  }

  po.printedPages.insert(myPages.begin(), myPages.end()); //Mark all pages as printed
  
  //DeleteDC(po.hDC);
  //po.hDC=0;
        
  return true;
}

UINT CALLBACK PagePaintHook(HWND, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
/*	if(uiMsg==WM_PSD_GREEKTEXTRECT)
	{
		RECT &rc=*LPRECT(lParam);
		HDC hDC=HDC(wParam);
		SelectObject(hDC, GetStockObject(NULL_BRUSH));
		SelectObject(hDC, GetStockObject(BLACK_PEN));

		int top=rc.top+9;

		int antal=(rc.right-rc.left)/16+1;
		float width=float(rc.right-rc.left-1)/float(antal);

		int ay=(rc.bottom-top)/6+1;
		float height=float(rc.bottom-top-1)/float(ay);

		RECT mini;

		int x,y;

		//Print header

		for(x=rc.left;x<rc.left+(rc.right-rc.left)/2;x++)
		{
			MoveToEx(hDC, x+x%3-1, rc.top+6, NULL);
			LineTo(hDC, x, rc.top+(x%3+x%2+x%5));
		}

		//Print calendar
		for(x=0;x<antal;x++)
		{
			mini.left=x*width;
			mini.right=(x+1)*width;

			for(y=0;y<ay-x%2;y++)
			{
				mini.top=y*height;
				mini.bottom=(y+1)*height;

				Rectangle(hDC, rc.left+mini.left, top+mini.top, rc.left+mini.right+1, top+mini.bottom+1);
			}
		}

		//Rectangle(hDC, rc.left+10, rc.top+10, rc.right, rc.bottom);
		//CreateDIBitmap(

		return true;
	}
*/
	return false;
}

void PageSetup(HWND hWnd, PrinterObject &po)
{
  PrinterObject::DATASET &ds=po.ds;
	PAGESETUPDLG pd;

	memset(&pd, 0, sizeof(pd));

	pd.lStructSize=sizeof(pd);
	pd.hwndOwner=hWnd;
//	pd.hDevMode=po.hDevMode;
//	pd.hDevNames=po.hDevNames;
	pd.Flags=PSD_MARGINS|PSD_ENABLEPAGEPAINTHOOK|PSD_INHUNDREDTHSOFMILLIMETERS;
	pd.lpfnPagePaintHook=PagePaintHook;


	pd.rtMargin.left=int(ds.pMgLeft*float(ds.pWidth_mm)+0.5)*100;
	pd.rtMargin.right=int(ds.pMgRight*float(ds.pWidth_mm)+0.5)*100;
	pd.rtMargin.top=int(ds.pMgTop*float(ds.pHeight_mm)+0.5)*100;
	pd.rtMargin.bottom=int(ds.pMgBottom*float(ds.pHeight_mm)+0.5)*100;


	if(PageSetupDlg(&pd))
	{
		RECT rtMargin=pd.rtMargin;

		if(pd.Flags & PSD_INHUNDREDTHSOFMILLIMETERS)
		{
			rtMargin.top=long(rtMargin.top/inchmmk);
			rtMargin.bottom=long(rtMargin.bottom/inchmmk);
			rtMargin.right=long(rtMargin.right/inchmmk);
			rtMargin.left=long(rtMargin.left/inchmmk);
		}

		po.hDevMode=pd.hDevMode;
		po.hDevNames=pd.hDevNames;


		DEVMODE *dm=(LPDEVMODE)GlobalLock(pd.hDevMode);

		if(dm)
		{/*
			if(dm->dmFields&DM_COLOR)
			{
				/*if(dm->dmColor==DMCOLOR_MONOCHROME)
					ds.bPrintColour=false;
				else
					ds.bPrintColour=true;
			}*/
		}

		DEVNAMES *dn=(LPDEVNAMES)GlobalLock(pd.hDevNames);

		if(dn)
		{
			char *driver=(char *)(dn)+dn->wDriverOffset;
			char *device=(char *)(dn)+dn->wDeviceOffset;

			HDC hDC=CreateDC(driver, device, NULL, dm);

			if(hDC)
			{
				//	int xres = GetDeviceCaps(hDC, LOGPIXELSX);
				// int yres = GetDeviceCaps(hDC, LOGPIXELSY);

				//	int xtot = GetDeviceCaps(hDC, HORZRES);
				//	int ytot = GetDeviceCaps(hDC, VERTRES);

				//rtMagrin 1/1000 inches
				// int xmarg=(rtMargin.left*xres)/1000;
				// int ymarg=(rtMargin.top*yres)/1000;

				//int xsize=xtot-(rtMargin.right*xres)/1000-xmarg;
				//int ysize=ytot-(rtMargin.bottom*yres)/1000-ymarg;

				ds.pWidth_mm=GetDeviceCaps(hDC, HORZSIZE);
				ds.pHeight_mm=GetDeviceCaps(hDC, VERTSIZE);

				ds.pMgLeft=inchmmk*rtMargin.left/float(ds.pWidth_mm)/100.;
				ds.pMgRight=inchmmk*rtMargin.right/float(ds.pWidth_mm)/100.;
				ds.pMgTop=inchmmk*rtMargin.top/float(ds.pHeight_mm)/100.;
				ds.pMgBottom=inchmmk*rtMargin.bottom/float(ds.pHeight_mm)/100.;

				//ds.ydx=float(ysize)/float(xsize);

				DeleteDC(hDC);
			}
		}

//		GlobalUnlock(pd.hDevNames);
//		GlobalUnlock(pd.hDevMode);
	}
}

void PrinterObject::freePrinter() {
  if (hDevNames)
    GlobalUnlock(hDevNames);
  hDevNames = 0;

  if (hDevMode)
		GlobalUnlock(hDevMode);
  hDevMode = 0;
}
