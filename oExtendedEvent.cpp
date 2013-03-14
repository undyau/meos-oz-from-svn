#include "stdafx.h"
#include "oExtendedEvent.h"
#include "oSSSQuickStart.h"
#include "gdioutput.h"
#include "meos_util.h"

oExtendedEvent::oExtendedEvent(gdioutput &gdi) : oEvent(gdi), m_CtrlNamesOnSplits(false)
{
}

oExtendedEvent::~oExtendedEvent(void)
{
}

bool oExtendedEvent::SSSQuickStart(gdioutput &gdi)
{
  oSSSQuickStart qs(*this);
	if (qs.ConfigureEvent(gdi)){
    m_CtrlNamesOnSplits = true;
		gdi.alert("Loaded " + itos(qs.ImportCount()) + " competitors onto start list");
		return true;
	}
	return false;
}

