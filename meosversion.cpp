/************************************************************************
    MeOS - Orienteering Software
    Copyright (C) 2009-2012 Melin Software HB
    
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
    Stigbergsv�gen 7, SE-75242 UPPSALA, Sweden
    
************************************************************************/
#include "stdafx.h"
#include <vector>

//ABCDEFGHIJKLMNO
//V2: ABCDEFGHIHJK
int getMeosBuild() 
{
  string revision("$Rev: 102 $");
  return 174 + atoi(revision.substr(5, string::npos).c_str());
}

//ABCDEFGHIJKILMNOPQRSTUVXYZabcdefghijklmnopqrstuvx
//V2: abcdefg
//V3: abcdefghijklmn
string getMeosDate() 
{
  string date("$Date: 2012-04-29 08:50:28 +0200 (sö, 29 apr 2012) $");
  return date.substr(7,10);
}

string getBuildType() {
  return "RC1";
}

string getMeosFullVersion() {
  char bf[256];
  sprintf_s(bf, "Version X#3.0.%d, %s %s (customised for Big Foot)", getMeosBuild(), getBuildType().c_str(), getMeosDate().c_str());
  return bf;
}

string getMeosCompectVersion() {
  char bf[256];
  sprintf_s(bf, "3.0.%d (%s)", getMeosBuild(), getBuildType().c_str());
  return bf;
}

void getSupporters(vector<string> &supp) 
{
  supp.push_back("Centrum OK");
  supp.push_back("Ove Persson, Pite� IF");
  supp.push_back("OK Rodhen");
  supp.push_back("T�by Extreme Challenge");
  supp.push_back("Thomas Engberg, VK Uvarna");
  supp.push_back("Eilert Edin, Sidensj� IK");
  supp.push_back("G�ran Nordh, Trollh�ttans SK");
  supp.push_back("Roger Gustavsson, OK Tisaren");
  supp.push_back("Sundsvalls OK");
  supp.push_back("OK Gipens OL-skytte");
  supp.push_back("Helsingborgs SOK");
  supp.push_back("OK Gipens OL-skytte");
  supp.push_back("Rune Thur�n, Vallentuna-�sseby OL");
  supp.push_back("Roland Persson, Kalmar OK");
  supp.push_back("Robert Jessen, Fr�mmestads IK");  
  supp.push_back("Almby IK, �rebro");
}
