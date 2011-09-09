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

// xmlparser.cpp: implementation of the xmlparser class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "xmlparser.h"
#include "meos_util.h"
#include "progress.h"
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////


xmlparser::xmlparser()
{
  progress = 0;
  lastIndex = 0;
	tagStackPointer=0;	
  isUTF = false;
  cutMode = false;
}

xmlparser::~xmlparser()
{
  delete progress;
	//if(fin.good())
	fin.close();
	fout.close();
}

void xmlparser::setProgress(HWND hWnd)
{
  progress = new ProgressWindow(hWnd);
}

void xmlparser::access(int index) {
  if (progress && (index-lastIndex)>1000 ) {
    lastIndex = index;
    progress->setProgress(500 + int(500.0 * index/xmlinfo.size()));
  }
}


xmlobject::xmlobject()
{
	parser = 0;
}

xmlobject::~xmlobject()
{
	//MessageBox(NULL, name.c_str(), "Destroying: ", MB_OK);

//	if(objects) delete objects;
}

bool xmlparser::write(const char *tag, const string &Value)
{
	if(!cutMode || Value!="")	{		
		fout << "<" << tag << ">";
		fout << encodeXML(Value);
		fout << "</" << tag << ">" << endl;
	}
	return fout.good();
}

bool xmlparser::write(const char *tag)
{
	fout << "<" << tag << "/>" << endl;
	return fout.good();
}

bool xmlparser::write(const char *tag, const char *Property, const string &Value)
{
	if(!cutMode || Value!="")	{		
		fout << "<" << tag << " " << Property << "=\"";
		fout << encodeXML(Value) << "\"/>" << endl;		
	}
	return fout.good();
}

bool xmlparser::write(const char *tag, const char *Property, const string &PropValue, const string &Value)
{
	if(!cutMode || Value!="")	{		
		fout << "<" << tag << " " << Property << "=\"";
		fout << encodeXML(PropValue) << "\">" << encodeXML(Value);		
    fout << "</" << tag << ">" << endl;
	}
	return fout.good();
}

bool xmlparser::write(const char *tag, int Value)
{
	if(!cutMode || Value!=0) {
		fout << "<" << tag << ">";
		fout << Value;
		fout << "</" << tag << ">" << endl;
	}
	return fout.good();
}

bool xmlparser::write64(const char *tag, __int64 Value)
{
	if(!cutMode || Value!=0) {
		fout << "<" << tag << ">";
		fout << Value;
		fout << "</" << tag << ">" << endl;
	}
	return fout.good();
}

bool xmlparser::writeDWORD(const char *tag, DWORD Value)
{
	if(!cutMode || Value!=0) {
		fout << "<" << tag << ">";
		fout << Value;
		fout << "</" << tag << ">" << endl;
	}
	return fout.good();
}

bool xmlparser::startTag(const char *tag, const char *property, const string &Value)
{
	if(tagStackPointer<32) {
		fout << "<" << tag << " " << property << "=\"" << encodeXML(Value) << "\">" << endl;
		tagStack[tagStackPointer++]=tag;
		return fout.good();
	}
	else return false;
}

bool xmlparser::startTag(const char *tag, const vector<string> &propvalue)
{
	if(tagStackPointer<32) {
		fout << "<" << tag << " ";
    for (size_t k=0;k<propvalue.size(); k+=2) {
      fout << propvalue[k] << "=\"" << encodeXML(propvalue[k+1]) << "\" ";
    }
    fout << ">" << endl;
    tagStack[tagStackPointer++]=tag;
		return fout.good();
	}
	else return false;
}


bool xmlparser::startTag(const char *tag)
{
	if(tagStackPointer<32) {
		fout << "<" << tag << ">" << endl;
		tagStack[tagStackPointer++]=tag;
		return fout.good();
	}
	else return false;
}

bool xmlparser::endTag()
{
	if(tagStackPointer>0)	{
		fout << "</" << tagStack[--tagStackPointer] << ">" << endl;
		return fout.good();
	}
  else throw std::exception("BAD XML CODE");

	return false;
}

bool xmlparser::openOutput(const char *file, bool useCutMode)
{
  return openOutputT(file, useCutMode, "");
}

bool xmlparser::openOutputT(const char *file, bool useCutMode, const string &type)
{
  cutMode = useCutMode;
	fout.open(file);

	tagStackPointer=0;

	if(fout.bad())
		return false;

	fout << "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\n\n\n";

  if (!type.empty()) {
    if (!startTag(type.c_str())) {
      closeOut();
      return false;
    }
  }
	return true;
}

bool xmlparser::closeOut()
{
	while(tagStackPointer>0)
		endTag();

	fout.close();
	return true;
}

xmldata::xmldata(const char *t, char *d) : tag(t), data(d)
{
  parent = -1;
  next = 0;
}

xmlattrib::xmlattrib(const char *t, char *d) : tag(t), data(d) {}

bool xmlparser::read(const char *file, int maxobj)
{
  fin.open(file, ios::binary);

	if(!fin.good())
		return false;

	char bf[1024];
	bf[0]=0;

	do {
		fin.getline(bf, 1024, '>');	
		lineNumber++;
	}
	while(fin.good() && bf[0]==0);

	char *ptr=ltrim(bf);
	isUTF = false;

  if (ptr[0] == -17 && ptr[1]==-69 && ptr[2]==-65) {
    isUTF = true;
    ptr+=3; //Windows UTF attribute
  }
  int p1 = 0;

  if(memcmp(ptr, "<?xml", 5) == 0) {
    int i = 5;
    bool hasEncode = false;
    while (ptr[i]) {
      if ((ptr[i] == 'U' || ptr[i] == 'u') && _memicmp(ptr+i, "UTF-8", 5)==0) {
        isUTF = true;
        break;
      }
      if (ptr[i] == 'e' && memcmp(ptr+i, "encoding", 8)==0) {
        hasEncode = true;
      }
      i++;
    }
    if (!hasEncode) 
      isUTF = true; // Assume UTF
    p1 = fin.tellg();
  }
  else if (ptr[0] == '<' && ptr[1] == '?') {
    // Assume UTF XML if not specified
    isUTF = true;
  }
  else {
    throw std::exception("Invalid XML file.");
  }

  fin.seekg(0, ios::end);
  int p2 = fin.tellg();
  fin.seekg(p1, ios::beg);

  int asize = p2-p1;
  if (maxobj>0)
    asize = min(asize, maxobj*256);

  if (progress && asize>80000)
    progress->init();

  xbf.resize(asize+1);
  xmlinfo.clear();
  xmlinfo.reserve(xbf.size() / 30); // Guess number of tags

  parseStack.clear();

  fin.read(&xbf[0], xbf.size());
  xbf[asize] = 0;

  fin.close();

	lineNumber=0;
  int oldPrg = -50001;
  int pp = 0;
  const int size = xbf.size()-2;
  while (pp < size) {
    while (pp < size && xbf[pp] != '<') pp++;

    // Update progress while parsing
    if (progress && (pp - oldPrg)> 50000) {
      progress->setProgress(int(500.0*pp/size));
      oldPrg = pp;
    }

    // Found tag
    if (xbf[pp] == '<') {
      xbf[pp] = 0;
      char *start = &xbf[pp+1];
      while (pp < size && xbf[pp] != '>') pp++;

      if (xbf[pp] == '>') {
        xbf[pp] = 0;
      }
      if (*start=='!')
        continue; //Comment

      processTag(start, &xbf[pp-1]);
    }

    if (maxobj>0 && int(xmlinfo.size()) >= maxobj) {
      xbf[pp+1] = 0;
      return true;
    }
    pp++;
  }

  lastIndex = 0;
  return true;
}

void inplaceDecodeXML(char *in);

bool xmlparser::processTag(char *start, char *end) {
  static char err[64];
  bool onlyAttrib = *end == '/';
  bool endTag = *start == '/';
  
  char *tag = start;

  if (endTag)
    tag++;
  
  while (start<=end && *start!=' ' && *start!='\t') 
    start++;

  *start = 0;

  if (!endTag && !onlyAttrib) {
    parseStack.push_back(xmlinfo.size());
    xmlinfo.push_back(xmldata(tag, end+2));
    int p = parseStack.size()-2;
    xmlinfo.back().parent = p>=0 ? parseStack[p] : -1;
  }
  else if (endTag) {
    if (!parseStack.empty()){
      xmldata &xd = xmlinfo[parseStack.back()];
      inplaceDecodeXML(xd.data);
      if (strcmp(tag, xd.tag)== 0) {
        parseStack.pop_back();
        xd.next = xmlinfo.size();
      }
      else {
        sprintf_s(err, "Unmatched tag '%s', expected '%s'.", tag, xd.tag);
        throw std::exception(err);
      }
    }
    else
    {
      sprintf_s(err, "Unmatched tag '%s'.", tag);
      throw std::exception(err);
    }
  }
  else if (onlyAttrib) {
    *end = 0;
    xmlinfo.push_back(xmldata(tag, 0));
    int p = parseStack.size() - 1;
    xmlinfo.back().parent = p>=0 ? parseStack[p] : -1;
    xmlinfo.back().next = xmlinfo.size();
  }
  return true;
}

char * xmlparser::ltrim(char *s)
{
	while(*s && isspace(BYTE(*s)))
		s++;

	return s;
}

const char * xmlparser::ltrim(const char *s)
{
	while(*s && isspace(BYTE(*s)))
		s++;

	return s;
}

const char * xmlparser::getError()
{
	return errorMessage.c_str();
}

xmlobject xmlobject::getObject(const char *pname) const
{
  if (pname == 0)
    return *this;
  if (isnull())
    throw std::exception("Null pointer exception");
  
  vector<xmldata> &xmlinfo = parser->xmlinfo;
  
  parser->access(index);

  unsigned child = index+1;
  while (child < xmlinfo.size() && xmlinfo[child].parent == index) {
    if (strcmp(xmlinfo[child].tag, pname)==0)
      return xmlobject(parser, child);
    else
      child = xmlinfo[child].next;
  }
  return xmlobject(0);
}


void xmlobject::getObjects(xmlList &obj) const
{
  obj.clear();

  if (isnull())
    throw std::exception("Null pointer exception");
  
  vector<xmldata> &xmlinfo = parser->xmlinfo;
  unsigned child = index+1;
  parser->access(index);

  while (child < xmlinfo.size() && xmlinfo[child].parent == index) {
    obj.push_back(xmlobject(parser, child));
    child = xmlinfo[child].next;
  }
}

void xmlobject::getObjects(const char *tag, xmlList &obj) const
{
  obj.clear();

  if (isnull())
    throw std::exception("Null pointer exception");
  
  vector<xmldata> &xmlinfo = parser->xmlinfo;
  unsigned child = index+1;
  parser->access(index);

  while (child < xmlinfo.size() && xmlinfo[child].parent == index) {
    if (strcmp(tag, xmlinfo[child].tag) == 0)
      obj.push_back(xmlobject(parser, child));
    child = xmlinfo[child].next;
  }
}


const xmlobject xmlparser::getObject(const char *pname) const
{
  if(xmlinfo.size()>0){
		if(strcmp(xmlinfo[0].tag, pname) == 0) 
      return xmlobject(const_cast<xmlparser *>(this), 0);
		else return xmlobject(const_cast<xmlparser *>(this), 0).getObject(pname);
	}
	else return xmlobject(0);	
}

xmlattrib xmlobject::getAttrib(const char *pname) const 
{
 
  char *start = const_cast<char *>(parser->xmlinfo[index].tag);
  const char *end = parser->xmlinfo[index].data;

  if (end) 
    end-=2;
  else {
    if (size_t(index + 1) < parser->xmlinfo.size())
      end = parser->xmlinfo[index+1].tag;
    else
      end = &parser->xbf.back();
  }

  // Scan past tag.
  while (start<end && *start != 0)
    start++;
  start++;

  char *oldStart = start;
  while (start<end) {
    while(start<end && (*start==' ' || *start=='\t'))
      start++;

    char *tag = start;
    
    while(start<end && *start!='=' && *start!=0)
      start++;

    if (start<end && (start[1]=='"' || start[1] == 0)) {
      *start = 0;
      ++start;
      char *value = ++start;
    
      while(start<end && (*start!='"' && *start != 0))
        start++;

      if (start<=end) {
        *start = 0;
        if (strcmp(pname, tag) == 0)
          return xmlattrib(tag, value);
        start++;
      }
      else {//Error
      }
    }

    if (oldStart == start)
      break;
    else
      oldStart = start;
  }
  return xmlattrib(0,0);
}

static int unconverted = 0;

void xmlparser::convertString(const char *in, char *out, int maxlen) const
{
  if (in==0)
    throw std::exception("Null pointer exception");

  if (!isUTF) {
    strncpy_s(out, maxlen, in, maxlen);
    out[maxlen-1] = 0;
    return;
  }

  wchar_t buff[2048];
  int len = strlen(in);
  len = min(min(len+1, maxlen), 2000);

  int wlen = MultiByteToWideChar(CP_UTF8, 0, in, len, buff, 2048);
  buff[wlen-1] = 0;

  BOOL untranslated = false;
  WideCharToMultiByte(CP_ACP, 0, buff, wlen, out, 2048, "?", &untranslated);
  out[wlen-1] = 0;

  if (untranslated)
    unconverted++;
}

string &xmlobject::getObjectString(const char *pname, string &out) const
{ 
  xmlobject x=getObject(pname);
  if(x) {
    const char *bf = x.get();
    if (bf) {
      parser->convertString(x.get(), parser->strbuff, 2048);
      out = parser->strbuff;
      return out;
    }
  }

  xmlattrib xa(getAttrib(pname));
  if (xa && xa.data) {
    parser->convertString(xa.get(), parser->strbuff, 2048);
    out = parser->strbuff;
  }
  else 
    out = "";

  return out;
}

char *xmlobject::getObjectString(const char *pname, char *out, int maxlen) const
{ 
  xmlobject x=getObject(pname);
  if(x) {
    const char *bf = x.get();
    if (bf) {
      parser->convertString(bf, out, maxlen);
      return out;
    }
  }
  else {
    xmlattrib xa(getAttrib(pname));
    if (xa && xa.data) {
      parser->convertString(xa.data, out, maxlen);
      inplaceDecodeXML(out);
    } else 
       out[0] = 0;
  }
  return out;
}

const char *xmlattrib::get() const
{
  if (data)
    return decodeXML(data);
  else
    return 0;
}
