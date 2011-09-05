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
#include "gdioutput.h"
#include <vector>
#include <map>
#include <cassert>
#include <algorithm>
#include "meos_util.h"
#include "Localizer.h"

bool gdioutput::writeHTML(const string &file, const string &title) const
{
  ofstream fout(file.c_str());

  if(fout.bad())
    return false;


  fout << "<!DOCTYPE html PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\"\n" <<
	        "          \"http://www.w3.org/TR/html4/loose.dtd\">\n\n";
	
  fout << "<html>\n<head>\n";
  fout << "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=iso-8859-1\">\n";
  fout << "<title>" << title << "</title>\n";

  fout << "<style type=\"text/css\">\n";
  fout << "body {background-color: rgb(250, 250,255)}\n";
  fout << "h1 {font-family:arial,sans-serif;font-size:24px;font-weight:normal;white-space:nowrap}\n";
  fout << "h2 {font-family:arial,sans-serif;font-size:20px;font-weight:normal;white-space:nowrap}\n";
  fout << "h3 {font-family:arial,sans-serif;font-size:16px;font-weight:normal;white-space:nowrap}\n";
  fout << "p {font-family:arial,sans-serif;font-size:12px;font-weight:normal;white-space:nowrap}\n";
  fout << "</style>\n";
  fout << "</head>\n";

  fout << "<body>\n";

  list<TextInfo>::const_iterator it=TL.begin();

 // int currentTableCol=1;

	while (it!=TL.end()) {
    /*int y=it->yp;

    vector<const TextInfo *> row;
    set<int> xcoordinates;
    while (it!=TL.end() && it->yp==y) {
      row.push_back(&*it);
      xcoordinates.insert(it->xp);
      ++it;
    } 

    if(row.size()!=currentTableCol) {
      if(currentTableCol

    }

    row.clear();*/

    string element="p";

    switch(it->format)
    {
    case boldHuge:
      element="h1";
      break;
    case boldLarge:
      element="h2";
      break;
    case fontLarge:
      element="h2";
      break;
    case fontMedium:
      element="h3";
      break;
    case fontSmall:
      element="p";
      break;
    }

    if(it->format!=1 && it->format!=boldSmall) {
      if (it->format & textRight)
        fout << "<" << element << " style=\"position:absolute;right:" 
            << it->xp << "px;top:" <<  int(1.1*it->yp) << "px\">";
      else
        fout << "<" << element << " style=\"position:absolute;left:" 
            << it->xp << "px;top:" <<  int(1.1*it->yp) << "px\">";

    }
    else {
      if (it->format & textRight) 
        fout << "<" << element << " style=\"font-weight:bold;position:absolute;right:" 
              << it->xp << "px;top:" <<  int(1.1*it->yp) << "px\">";
      else
        fout << "<" << element << " style=\"font-weight:bold;position:absolute;left:" 
              << it->xp << "px;top:" <<  int(1.1*it->yp) << "px\">";
    }
    fout << encodeXML(it->text);
    fout << "</" << element << ">\n";
    ++it;
  }

  fout << "<p style=\"position:absolute;left:10px;top:" <<  int(1.1*MaxY)+5 << "px\">";
  
  char bf1[256];
	char bf2[256];
  GetTimeFormat(LOCALE_USER_DEFAULT, 0, NULL, NULL, bf2, 256);
	GetDateFormat(LOCALE_USER_DEFAULT, 0, NULL, NULL, bf1, 256);
  //fout << "Skapad av <i>MeOS</i>: " << bf1 << " "<< bf2 << "\n";	
  fout << lang.tl("Skapad av ") + "<a href=\"http://www.melin.nu/meos\" target=\"_blank\"><i>MeOS</i></a>: " << bf1 << " "<< bf2 << "\n";	
  fout << "</p>\n";
  
  fout << "</body>\n";
  fout << "</html>\n";

  return false;
}

string html_table_code(const string &in)
{
  if(in.size()==0)
    return "&nbsp;";
  else return encodeXML(in);
}

bool sortTL_X(const TextInfo *a, const TextInfo *b)
{
  return a->xp < b->xp;
}

bool gdioutput::writeTableHTML(const string &file, const string &title) const
{
  ofstream fout(file.c_str());

  if(fout.bad())
    return false;

  fout << "<!DOCTYPE html PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\"\n" <<
	        "          \"http://www.w3.org/TR/html4/loose.dtd\">\n\n";
	
  fout << "<html>\n<head>\n";
  fout << "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=iso-8859-1\">\n";
  fout << "<title>" << title << "</title>\n";

  fout << "<style type=\"text/css\">\n";
  fout << "body {background-color: rgb(250, 250,255)}\n";
  fout << "h1 {font-family:arial,sans-serif;font-size:24px;font-weight:normal;white-space:nowrap}\n";
  fout << "h2 {font-family:arial,sans-serif;font-size:20px;font-weight:normal;white-space:nowrap}\n";
  fout << "h3 {font-family:arial,sans-serif;font-size:16px;font-weight:normal;white-space:nowrap}\n";
  fout << "td {font-family:arial,sans-serif;font-size:12px;font-weight:normal;white-space:nowrap}\n";
  fout << "p {font-family:arial,sans-serif;font-size:12px;font-weight:normal}\n";
  fout << "td.e0 {background-color: rgb(238,238,255)}\n";
  fout << "td.e1 {background-color: rgb(245,245,255)}\n";
  fout << "td.header {line-height:1.7}\n";
  fout << "</style>\n";
  fout << "</head>\n";

  fout << "<body>\n";

  list<TextInfo>::const_iterator it=TL.begin();

 // int currentTableCol=1;
  map<int,int> tableCoordinates;

  //Get x-coordinates
  while (it!=TL.end()) {
    //if(!(it->format&textRight))
      tableCoordinates[it->xp]=0;
    ++it;
  }

  map<int, int>::iterator mit=tableCoordinates.begin();
  int k=0;

  while (mit!=tableCoordinates.end()) {  
    mit->second=k++;
    ++mit;
  }
  tableCoordinates[MaxX]=k;

  vector<bool> sizeSet(k+1, false);

  fout << "<table cellspacing=\"0\" border=\"0\">\n";
  
  int linecounter=0;
  it=TL.begin();

 
	while (it!=TL.end()) {
    int y=it->yp;
    vector<const TextInfo *> row;
    while (it!=TL.end() && it->yp==y) {
      if(it->format!=pageNewPage && it->format!=pageReserveHeight)        
        row.push_back(&*it);      

      ++it;      
      //assert(row.size()==1 || (row[row.size()-1]->xp > row[row.size()-2]->xp));
    } 
   
    if(row.empty())
      continue;

    sort(row.begin(), row.end(), sortTL_X);
    vector<const TextInfo *>::iterator rit;

    //for (rit=row.begin(); rit!=row.end(); ++rit) {

    fout << "<tr>";

    bool isHeader=row.size()<3;
    string lineclass;

    if (isHeader) {
      linecounter=0;
      lineclass=" valign=\"bottom\" class=\"header\"";
    }
    else {
      lineclass=(linecounter&1) ? " class=\"e1\"" : " class=\"e0\"";
      linecounter++;
    }

    for (size_t k=0;k<row.size();k++) {
      int thisCol=tableCoordinates[row[k]->xp];

      if (k==0 && thisCol!=0) 
        fout << "<td" << lineclass << " colspan=\"" << thisCol << "\">&nbsp;</td>";

      int nextCol;      
      if(row.size()==k+1)
        nextCol=tableCoordinates.rbegin()->second;
      else
        nextCol=tableCoordinates[row[k+1]->xp];

      int colspan=nextCol-thisCol;

      assert(colspan>0);

      string style;

      if(row[k]->format&textRight)
        style=" style=\"text-align:right\"";

      if(colspan==1 && !sizeSet[thisCol]) {        
        fout << "<td" << lineclass << style << " width=\"" << int( (k+1<row.size()) ? 
                        (row[k+1]->xp - row[k]->xp) : (MaxX-row[k]->xp)) << "\">";
        sizeSet[thisCol]=true;
      }
      else if(colspan>1)
        fout << "<td" << lineclass << style << " colspan=\"" << colspan << "\">";
      else
        fout << "<td" << lineclass << style << ">";
      

      if ((row[k]->format&0xFF)==1)
        fout << "<b>" << html_table_code(row[k]->text) << "</b></td>";
      else if ((row[k]->format&0xFF)==0)
        fout << html_table_code(row[k]->text) << "</td>";
      else if ((row[k]->format&0xFF)==italicSmall)
        fout << "<i>" << html_table_code(row[k]->text) << "</i></td>";
      else {
        string element;
        switch( (row[k]->format&0xFF ))
        {
          case boldHuge:
            element="h1";
            break;
          case boldLarge:
            element="h2";
            break;
          case fontLarge:
            element="h2";
            break;
          case fontMedium:
            element="h3";
            break;
        }    
        assert(element.size()>0);
        if (element.size()>0) {
          fout << "<" << element << ">";
          fout << html_table_code(row[k]->text) << "</" << element << "></td>";
        }
        else {
          fout << html_table_code(row[k]->text) << "</td>";
        }
      }       
    }
    fout << "</tr>\n";
     
    row.clear();
/*
    string element="p";

    switch(it->format)
    {
    case boldHuge:
      element="h1";
      break;
    case boldLarge:
      element="h2";
      break;
    case fontLarge:
      element="h2";
      break;
    case fontMedium:
      element="h3";
      break;
    case fontSmall:
      element="p";
      break;
    }

    if(it->format!=1 && it->format!=boldSmall) {
      if (it->format & textRight)
        fout << "<" << element << " style=\"position:absolute;right:" 
            << it->xp << "px;top:" <<  int(1.1*it->yp) << "px\">";
      else
        fout << "<" << element << " style=\"position:absolute;left:" 
            << it->xp << "px;top:" <<  int(1.1*it->yp) << "px\">";

    }
    else {
      if (it->format & textRight) 
        fout << "<" << element << " style=\"font-weight:bold;position:absolute;right:" 
              << it->xp << "px;top:" <<  int(1.1*it->yp) << "px\">";
      else
        fout << "<" << element << " style=\"font-weight:bold;position:absolute;left:" 
              << it->xp << "px;top:" <<  int(1.1*it->yp) << "px\">";
    }
    fout << it->text;
    fout << "</" << element << ">\n";
    ++it;*/
  }

  fout << "<table>\n";

  fout << "<br><p>";  
  char bf1[256];
	char bf2[256];
  GetTimeFormat(LOCALE_USER_DEFAULT, 0, NULL, NULL, bf2, 256);
	GetDateFormat(LOCALE_USER_DEFAULT, 0, NULL, NULL, bf1, 256);
  fout << lang.tl("Skapad av ") + "<a href=\"http://www.melin.nu/meos\" target=\"_blank\"><i>MeOS</i></a>: " << bf1 << " "<< bf2 << "\n";	
  fout << "</p><br>\n";
  
  fout << "</body>\n";
  fout << "</html>\n";

  return false;
}