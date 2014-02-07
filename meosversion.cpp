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
    Stigbergsvägen 7, SE-75242 UPPSALA, Sweden
    
************************************************************************/
#include "stdafx.h"
#include <vector>

//ABCDEFGHIJKLMNO
//V2: ABCDEFGHIHJKMN
int getMeosBuild() 
{
  string revision("$Rev: 142 $");
  return 174 + atoi(revision.substr(5, string::npos).c_str());
}

//ABCDEFGHIJKILMNOPQRSTUVXYZabcdefghijklmnopqrstuvx
//V2: abcdefg
//V3: abcdefghijklmnopqrs
string getMeosDate() 
{
  string date("$Date: 2013-07-04 09:19:13 +0200 (to, 04 jul 2013) $");
  return date.substr(7,10);
}

string getBuildType() {
  return "Beta 1";
}

string getMeosFullVersion() {
  char bf[256];
  if (getBuildType().empty())
    sprintf_s(bf, "Version X#3.1.%d, %s (+ minor Australian customisations)", getMeosBuild(), getMeosDate().c_str());
  else
    sprintf_s(bf, "Version X#3.1.%d, %s %s (+ minor Australian customisations)", getMeosBuild(), getBuildType().c_str(), getMeosDate().c_str());
  return bf;
}

string getMeosCompectVersion() {
  char bf[256];
  if (getBuildType().empty())
    sprintf_s(bf, "3.1.%d", getMeosBuild());
  else
    sprintf_s(bf, "3.1.%d (%s)", getMeosBuild(), getBuildType().c_str());
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
  supp.push_back("Göran Nordh, Trollhättans SK");
  supp.push_back("Roger Gustavsson, OK Tisaren");
  supp.push_back("Sundsvalls OK");
  supp.push_back("OK Gipens OL-skytte");
  supp.push_back("Helsingborgs SOK");
  supp.push_back("OK Gipens OL-skytte");
  supp.push_back("Rune Thurén, Vallentuna-Össeby OL");
  supp.push_back("Roland Persson, Kalmar OK");
  supp.push_back("Robert Jessen, Främmestads IK");  
  supp.push_back("Almby IK, Örebro");
  supp.push_back("Peter Rydesäter, Rehns BK");
  supp.push_back("IK Hakarpspojkarna");
  supp.push_back("Rydboholms SK");
  supp.push_back("IFK Kiruna");
  supp.push_back("Peter Andersson, Söders SOL");
  supp.push_back("Björkfors GoIF");
  supp.push_back("OK Ziemelkurzeme");
  supp.push_back("Big Foot Orienteers");
  supp.push_back("FIF Hillerød");
  supp.push_back("Anne Udd");
  supp.push_back("OK Orinto");
  supp.push_back("SOK Träff");
  supp.push_back("Gamleby OK");
  supp.push_back("Vänersborgs SK");
  supp.push_back("Henrik Ortman, Västerås SOK");
  supp.push_back("Leif Olofsson, Sjuntorp");
  supp.push_back("Vallentuna/Össeby OL");
  supp.push_back("Oskarström OK");
  supp.push_back("OK Milan");
}
 