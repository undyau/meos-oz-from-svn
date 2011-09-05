// Printer.h: interface for the Printer class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_PRINTER_H__CD7DCD59_EE95_464C_A52A_288E50BEAF88__INCLUDED_)
#define AFX_PRINTER_H__CD7DCD59_EE95_464C_A52A_288E50BEAF88__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "ClubOrder.h"

#define MaxClubs 2048
#define MaxClasses 1024

struct PRINTERINFO
{
	char Name[128];

	char CurrentJob[128];
	DWORD CurrentStatus;

	DWORD Status;
	DWORD AveragePPM;

	DWORD JobQueue;
	DWORD PageQueue;	
};

class PrinterMaster;

class Printer  
{
private:
	HANDLE MSBitmap; //Handle to MS Bitmap
	HANDLE ExtraBitmap; //Handle to other sponsor bitmap Bitmap

	PrinterMaster *Parent;

	char Device[128];
	char Driver[128];
	DEVMODE *devmode;

	HWND hWndButton;
	int xp;
	int yp;
	
	//OLClass *ClassesToPrint[MaxClasses];
	ClassOrder ClassesToPrint[MaxClasses];
	int nClassesToPrint;
	int iFirstClassToPrint;

	lpClubOrder ClubsToPrint[MaxClubs]; //Not owned by us
	int nClubsToPrint;

	lpClubOrder ClubsPrinted[MaxClubs]; //Not owned by us
	int	nClubsPrinted;
	
	//Current status
	int nPagesToPrint;
	bool IsActive;
	bool DoPrintSeparator;
	bool Mod4PagesPrinting;
	int IsPrinting;
	bool RunPrinterThread;
	bool Blink;

	HDC hDCPrinter;

	DWORD ThreadId;
	void PrinterThread();


	int PrinterNR;
public:
	bool BlankPage();
	void PrintSeparator();
	void PrintSeparator(HDC hDCSep);

	bool GetMod4Pages() {return Mod4PagesPrinting;}
	int GetPrinterNR() {return PrinterNR;}
	HANDLE GetSponsor() {return MSBitmap;}
	HANDLE GetExtraSponsor() {return ExtraBitmap;}

	void FillListBoxClassesToPrint(HWND hList);	
	ClassOrder *GetFirstClassToPrint(bool pop); 
	bool AddClassToPrint(OLClass *olc, char *name);

	//bool AddClassToPrint(OLClass *olc);

	void TakeOver();
	void RemoveJob(lpClubOrder co);
	void MarkClubsToPrint();
	void UnMarkClubsToPrint();
	bool BltBitmap(HBITMAP hBM, int x, int y, int width, int height);
	void StopPrintingThread();
	void KillThreadAsync();
	void StartPrintingThread();


	void ListBoxSelectClubsToPrint(HWND hList);
	void UpdatePrintQueue();
	void ResetClubsToPrint();
	void AddClubToPrint(lpClubOrder co);
	void FillListBoxPrintedClubs(HWND hList);
	char * GetJobText(char *bf, DWORD code);
	void PrintStatus(HDC hDC, PRINTERINFO &pi);
	int GetClubPagesToPrint();
	bool GetPrinterInfo(PRINTERINFO &pi);
	void ErrorMessage();
	void ButtonClicked();
	void pEndDoc();
	HDC pStartDoc(char *JobName);
	Printer(PrinterMaster *pmaster);
	virtual ~Printer();

	friend class PrinterMaster;
	friend BOOL CALLBACK ClubJobDlg(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
	friend BOOL CALLBACK PrinterDlg(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
//	friend DWORD __stdcall InitPThread(LPVOID lpP);
	friend VOID __cdecl InitPThread(LPVOID lpP);
};

#endif // !defined(AFX_PRINTER_H__CD7DCD59_EE95_464C_A52A_288E50BEAF88__INCLUDED_)
