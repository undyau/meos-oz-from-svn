#pragma once
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

#include "tabbase.h"

class TabClass :
	public TabBase
{
  bool EditChanged;
  int ClassId;
  int currentStage;
  string storedNStage;
  string storedStart;
  bool checkClassSelected(const gdioutput &gdi) const;
  void save(gdioutput &gdi);
  void legSetup(gdioutput &gdi); 
  vector<ClassInfo> cInfo;
  oEvent::DrawInfo drawInfo;

  bool tableMode;

  // Generate a table with class settings
  void showClassSettings(gdioutput &gdi);

  // Read input from the table with class settings
  void readClassSettings(gdioutput &gdi);

  // Prepare for drawing by declaring starts and blocks
  void prepareForDrawing(gdioutput &gdi);

public:
  void multiCourse(gdioutput &gdi, int nLeg);
	bool loadPage(gdioutput &gdi);
  void selectClass(gdioutput &gdi, int cid);

  int classCB(gdioutput &gdi, int type, void *data);
  int multiCB(gdioutput &gdi, int type, void *data);

	TabClass(oEvent *oe);
	~TabClass(void);
};
