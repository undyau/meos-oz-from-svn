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
#include <vector>
//ABCDEFGHIJKLMNO
//V2: ABCDEFGHIHJK
int getMeosBuild() 
{
  string revision("$Rev: 43 $");
  return 174 + atoi(revision.substr(5, string::npos).c_str());
}

//ABCDEFGHIJKILMNOPQRSTUVXYZabcdefghijklmnopqrstuv
//V2: abcdefgh
string getMeosDate() 
{
  string date("$Date: 2011-05-28 21:30:30 +0200 (lÃ¶, 28 maj 2011) $");
  return date.substr(7,10);
}

string getMeosFullVersion() {
  char bf[256];
  sprintf_s(bf, "Version X#2.5.%d, %s (customised by Big Foot)", getMeosBuild(), getMeosDate().c_str());
  return bf;
}

void getSupporters(vector<string> &supp) 
{
  supp.push_back("Centrum OK");
  supp.push_back("Ove Persson, Piteå IF");
  supp.push_back("OK Rodhen");
  supp.push_back("Täby Extreme Challenge");
  supp.push_back("Thomas Engberg, VK Uvarna");
  supp.push_back("Eilert Edin, Sidensjö IK");
}
