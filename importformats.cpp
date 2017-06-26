/************************************************************************
    MeOS - Orienteering Software
    Copyright (C) 2009-2017 Melin Software HB

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
    Eksoppsv�gen 16, SE-75646 UPPSALA, Sweden

************************************************************************/

// oEvent.cpp: implementation of the oEvent class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include "localizer.h"
#include "importformats.h"
#include "oEvent.h"

/*
void ImportFormats::getImportFormats(vector< pair<string, size_t> > &formats) {
  formats.clear();
  formats.push_back(make_pair(lang.tl("Standard"), Default));
  formats.push_back(make_pair(lang.tl("French Orienteering Federation"), FrenchFederationMapping));
}

int ImportFormats::getDefault(oEvent &oe) {
  return oe.getPropertyString("Language", "English") == "Fran�ais" ? FrenchFederationMapping : Default;
}
*/
void ImportFormats::getExportFormats(vector< pair<string, size_t> > &types, bool exportFilter) {
  types.clear();
    
  string v;
  if (exportFilter) 
    v = "Resultat";
  else
    v = "Startlista";
    
  types.push_back(make_pair(lang.tl("IOF " + v + ", version 3.0 (xml)"), IOF30));
  types.push_back(make_pair(lang.tl("IOF " + v + ", version 2.0.3 (xml)"), IOF203));
  types.push_back(make_pair(lang.tl("OE Semikolonseparerad (csv)"), OE));
  types.push_back(make_pair(lang.tl("Webbdokument (html)"), HTML));
}

void ImportFormats::getExportFilters(bool exportFilters, vector< pair<string, string> > &ext) {
  string v;
  if (exportFilters) 
    v = "Resultat";
  else
    v = "Startlista";
    
  ext.push_back(make_pair("IOF " + v + ", version 3.0 (xml)", "*.xml"));
  ext.push_back(make_pair("IOF " + v + ", version 2.0.3 (xml)", "*.xml"));
  ext.push_back(make_pair("OE Semikolonseparerad (csv)", "*.csv"));
  ext.push_back(make_pair("OE/French Federation of Orienteering (csv)", "*.csv"));
  ext.push_back(make_pair("Webbdokument (html)", "*.html"));
}

ImportFormats::ExportFormats ImportFormats::getDefaultExportFormat(oEvent &oe) {
  int def = IOF30;
  return (ExportFormats)oe.getPropertyInt("ExportFormat", def);
}

ImportFormats::ExportFormats ImportFormats::setExportFormat(oEvent &oe, int raw) {
  oe.setProperty("ExportFormat", raw);
  return (ExportFormats)raw;
}

void ImportFormats::getOECSVLanguage(vector< pair<string, size_t> > &typeLanguages) {
  typeLanguages.push_back(make_pair("English", 1));
  typeLanguages.push_back(make_pair("Svenska", 2));
  typeLanguages.push_back(make_pair("Deutsch", 3));
  typeLanguages.push_back(make_pair("Dansk", 4));
  typeLanguages.push_back(make_pair("Fran�ais", 5));
  typeLanguages.push_back(make_pair("Russian", 6));
}
  
int ImportFormats::getDefaultCSVLanguage(oEvent &oe) {
  string currentLanguage = oe.getPropertyString("Language", "English");
  int defaultLanguageType = 1;

  if (currentLanguage == "English")
    defaultLanguageType = 1;
  else if (currentLanguage == "Svenska")
    defaultLanguageType = 2;
  else if (currentLanguage == "Deutsch")
    defaultLanguageType = 3;
  else if (currentLanguage == "Dansk")
    defaultLanguageType = 4;
  else if (currentLanguage == "Fran�ais")
    defaultLanguageType = 5;
  else if (currentLanguage == "Russian(ISO 8859 - 5)")
    defaultLanguageType = 6;

  return defaultLanguageType;
}
