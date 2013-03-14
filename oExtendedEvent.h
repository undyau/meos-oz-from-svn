#pragma once
#include "oevent.h"

class oExtendedEvent :
  public oEvent
{
public:
  oExtendedEvent(gdioutput &gdi);
  ~oExtendedEvent(void);

  bool CtrlNamesOnSplits() {return m_CtrlNamesOnSplits;};
  void CtrlNamesOnSplits(bool a_CtrlNamesOnSplits) {m_CtrlNamesOnSplits = a_CtrlNamesOnSplits;};
  bool RoundLateTimesToMinute() {return m_RoundLateTimesToMinute;};
	void RoundLateTimesToMinute(bool a_RoundLateTimesToMinute) {m_RoundLateTimesToMinute = a_RoundLateTimesToMinute;};
	bool SSSQuickStart(gdioutput &gdi);

private:
  bool m_CtrlNamesOnSplits;
	bool m_RoundLateTimesToMinute;
};
