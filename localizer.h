#pragma once
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
#include <map>
#include <set>
#include <string>
#include <vector>

class LocalizerImpl;
class oWordList;

class Localizer {
private:
  map<string, string> langResource;
  LocalizerImpl *impl;  
  LocalizerImpl *implBase;  
  
  bool owning;
  Localizer *user;

public:

  void debugDump(const string &untranslated, const string &translated) const;
 
  vector<string> getLangResource() const;
  void loadLangResource(const string &name);
  void addLangResource(const string &name, const string &resource);

  /** Translate string */
  const string &tl(const string &str);

  void set(Localizer &li);

  /** Get database with given names */
  const oWordList &getGivenNames() const;
 
  Localizer();
  ~Localizer();
};

extern Localizer lang;
