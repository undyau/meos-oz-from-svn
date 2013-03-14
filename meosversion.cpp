/************************************************************************
    MeOS - Orienteering Software
    Copyright (C) 2009-2013 Melin Software HB
    
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
//V2: ABCDEFGHIHJKMN
int getMeosBuild() 
{
  string revision("$Rev: 133 $");
  return 174 + atoi(revision.substr(5, string::npos).c_str());
}

//ABCDEFGHIJKILMNOPQRSTUVXYZabcdefghijklmnopqrstuvx
//V2: abcdefg
//V3: abcdefghijklmnopq
string getMeosDate() 
{
  string date("$Date: 2013-01-01 11:05:43 +0100 (ti, 01 jan 2013) $");
  return date.substr(7,10);
}

string getBuildType() {
  return "";
}

string getMeosFullVersion() {
  char bf[256];
  if (getBuildType().empty())
    sprintf_s(bf, "Version X#3.0.%d, %s", getMeosBuild(), getMeosDate().c_str());
  else
    sprintf_s(bf, "Version X#3.0.%d, %s %s", getMeosBuild(), getBuildType().c_str(), getMeosDate().c_str());
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
  supp.push_back("Peter Rydes�ter, Rehns BK");
  supp.push_back("IK Hakarpspojkarna");
  supp.push_back("Rydboholms SK");
  supp.push_back("IFK Kiruna");
  supp.push_back("Peter Andersson, S�ders SOL");
  supp.push_back("Bj�rkfors GoIF");
  supp.push_back("OK Ziemelkurzeme");
  supp.push_back("Big Foot Orienteers");
  supp.push_back("FIF Hiller�d");
  supp.push_back("Anne Udd");
}
