// xmlparser.h: interface for the xmlparser class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_XMLPARSER_H__87834E6D_6AB1_471C_8E1C_E65D67A4F98A__INCLUDED_)
#define AFX_XMLPARSER_H__87834E6D_6AB1_471C_8E1C_E65D67A4F98A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
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

#include <vector>
class xmlobject;

typedef vector<xmlobject> xmlList;
class xmlparser;


struct xmldata
{
  xmldata(const char *t, char *d);
  const char *tag;
  char *data;
  int parent;
  int next;
};

struct xmlattrib
{
  xmlattrib(const char *t, char *d);
  const char *tag;
  char *data;
  operator bool() const {return data!=0;}
  
  int getInt() const {if (data) return atoi(data); else return 0;}
  const char *get() const;
};

class ProgressWindow;

class xmlparser  
{
protected:
	static char *ltrim(char *s);
	static const char *ltrim(const char *s);

	string tagStack[32];
	int tagStackPointer;

  ofstream fout;
	ifstream fin;

	int lineNumber;
	string errorMessage;
	string doctype;

  vector<int> parseStack;
  vector<xmldata> xmlinfo;
  vector<char> xbf;
  
  bool processTag(char *start, char *end);

  void convertString(const char *in, char *out, int maxlen) const;

  // True if empty/zero values are excluded when writing
  bool cutMode;

  bool isUTF;
  char strbuff[2048]; // Temporary buffer for processing (no threading allowed)

  ProgressWindow *progress;
  int lastIndex;
public:
  void access(int index);

  void setProgress(HWND hWnd);

	bool failed(){return errorMessage.length()>0;}

	const xmlobject getObject(const char *pname) const;
	const char *getError();
	
  bool read(const char *file, int maxobj = 0);
	
	bool write(const char *tag, const char *Property, 
             const string &Value);
  bool write(const char *tag, const char *Property, 
             const string &PropValue, const string &Value);
	bool write(const char *tag, const string &Value);
  bool write(const char *tag); // Empty case
	bool write(const char *tag, int);
  bool write64(const char *tag, __int64);

	bool startTag(const char *tag);	
	bool startTag(const char *tag, const char *Property, 
                const string &Value);
  bool startTag(const char *tag, const vector<string> &propvalue);

	bool endTag();
	bool closeOut();
	bool openOutput(const char *file, bool useCutMode);
  bool openOutputT(const char *file, bool useCutMode, const string &type);

  xmlparser();
	virtual ~xmlparser();

  friend class xmlobject;
};

class xmlobject
{
protected:
  xmlobject(xmlparser *p) {parser = p;}
  xmlobject(xmlparser *p, int i) {parser = p; index = i;}

  xmlparser *parser;
  int index;
public:
  const char *getName() const {return parser->xmlinfo[index].tag;}
	xmlobject getObject(const char *pname) const;
	xmlattrib getAttrib(const char *pname) const;

	int getObjectInt(const char *pname) const
	{ 
		xmlobject x(getObject(pname));
		if(x) 
      return x.getInt();
    else {
      xmlattrib xa(getAttrib(pname));
      if (xa)
        return xa.getInt();
    }
		return 0;
	}
	
	string &getObjectString(const char *pname, string &out) const;
	char *getObjectString(const char *pname, char *out, int maxlen) const;
		
	void getObjects(xmlList &objects) const;
	void getObjects(const char *tag, xmlList &objects) const;
	
  bool is(const char *pname) const {
    const char *n = getName();
    return n[0] == pname[0] && strcmp(n, pname)==0;  
  }
	
  const char *get() const {return parser->xmlinfo[index].data;}
  int getInt() const {const char *d = parser->xmlinfo[index].data;
                      return d ? atoi(d) : 0;}
  __int64 getInt64() const {const char *d = parser->xmlinfo[index].data;
                           return d ? _atoi64(d) : 0;}

  bool isnull() const {return parser==0;}
	
  operator bool() const {return parser!=0;}
  
  xmlobject();
	virtual ~xmlobject();
	friend class xmlparser;
};


#endif // !defined(AFX_XMLPARSER_H__87834E6D_6AB1_471C_8E1C_E65D67A4F98A__INCLUDED_)
