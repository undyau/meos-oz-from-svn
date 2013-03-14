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
#include <vector>
#include <set>

#include "oBase.h"

class Table;

struct oDataInfo {
	char Name[20];
	int Index;
	int Size;
	int Type;
	int SubType;
  int tableIndex;
  char Description[48];
  int decimalSize;
  int decimalScale;
  vector< pair<string, string> > enumDescription;

  oDataInfo();
  ~oDataInfo();
};

struct oVariableInt
{
	char Name[20];
	int* Data;
};

struct oVariableString
{
	char Name[20];
	char* Data;
	int MaxSize;
};

class oBase;

class xmlparser;
class xmlobject;
class gdioutput;
class oDataInterface;
class oDataConstInterface;

class oDataContainer
{
protected:
	enum oDataType{oDTInt=1, oDTString=2};
	//void *Data;
	int DataMaxSize;
	int DataPointer;
	map<string, oDataInfo> Index;
  vector<string> ordered;

  oDataInfo *findVariable(const char *Name);
	const oDataInfo *findVariable(const char *Name) const;
  bool formatNumber(int nr, const oDataInfo &di, char bf[64]) const;

	void addVariable(oDataInfo &odi);
	static string C_INT(const string & name);
	static string C_INT64(const string & name);
	static string C_SMALLINT(const string & name);
	static string C_TINYINT(const string & name);
	static string C_SMALLINTU(const string & name);
	static string C_TINYINTU(const string & name);
	static string C_STRING(const string & name, int len);
	static string SQL_quote(const char *in);
public:	
	enum oIntSize{oISDecimal = 28, oISTime = 29, oISCurrency = 30, oISDate = 31, oIS64=64, oIS32=32, oIS16=16, oIS8=8, oIS16U=17, oIS8U=9};
  enum oStringSubType {oSSString = 0, oSSEnum = 1};
  string generateSQLDefinition(const std::set<string> &exclude) const;
  string generateSQLDefinition() const {
    return generateSQLDefinition(std::set<string>());
  }
	
  string generateSQLSet(const void *data) const;
	void getVariableInt(const void *data, list<oVariableInt> &var) const;
	void getVariableString(const void *data, list<oVariableString> &var) const;

	oDataInterface getInterface(void *data, int datasize, oBase *ob);
  oDataConstInterface getConstInterface(const void *data, int datasize, 
                                        const oBase *ob) const;

	void addVariableInt(const char *name, oIntSize isize, const char *descr);
  void addVariableDecimal(const char *name, const char *descr, int fixedDeci);
	void addVariableDate(const char *name,  const char *descr){addVariableInt(name, oISDate, descr);}
  void addVariableCurrency(const char *name,  const char *descr){addVariableInt(name, oISCurrency, descr);}
	void addVariableString(const char *name, int MaxChar, const char *descr);
  void addVariableEnum(const char *name, int maxChar, const char *descr, 
                                  const vector< pair<string, string> > enumValues);

	void initData(void *data, int datasize);

	bool setInt(void *data, const char *Name, int V);
	int getInt(const void *data, const char *Name) const;

  bool setInt64(void *data, const char *Name, __int64 V);
	__int64 getInt64(const void *data, const char *Name) const;

	bool setString(void *data, const char *Name, const string &V);
	string getString(const void *data, const char *Name) const;

	bool setDate(void *data, const char *Name, const string &V);
	string getDate(const void *data, const char *Name) const;

	bool write(const void *data, xmlparser &xml) const;
	void set(void *data, const xmlobject &xo);

  // Get a measure of how much data is stored in this record.
  int getDataAmountMeasure(const void *data) const;
	
	void buildDataFields(gdioutput &gdi) const;
  void buildDataFields(gdioutput &gdi, const vector<string> &fields) const;

	void fillDataFields(const oBase *ob, const void *data, gdioutput &gdi) const;
	bool saveDataFields(const oBase *ob, void *data, gdioutput &gdi);

  int fillTableCol(const void *data, const oBase &owner, Table &table, bool canEdit) const;
  void buildTableCol(Table *table);
	bool inputData(oBase *ob, void *data, int id, const string &input, int inputId, string &output, bool noUpdate);

  // Use id (table internal) or name
  void fillInput(const void *data, int id, const char *name, vector< pair<string, size_t> > &out, size_t &selected) const;

  bool setEnum(void *data, const char *name, int selectedIndex);

  oDataContainer(int maxsize);
	virtual ~oDataContainer(void);

	friend class oDataInterface;
};


class oDataInterface
{
private:
	void *Data;
	oDataContainer *oDC;
	oBase *oB;
public:

	inline bool setInt(const char *Name, int Value)
	{
		if(oDC->setInt(Data, Name, Value)){
			oB->updateChanged();
			return true;
		}
		else return false;
	}

	inline bool setInt64(const char *Name, __int64 Value)
	{
		if(oDC->setInt64(Data, Name, Value)){
			oB->updateChanged();
			return true;
		}
		else return false;
	}

	inline int getInt(const char *Name) const
		{return oDC->getInt(Data, Name);}

	inline __int64 getInt64(const char *Name) const
		{return oDC->getInt64(Data, Name);}

	inline bool setStringNoUpdate(const char *Name, const string &Value)
		{return oDC->setString(Data, Name, Value);}

	inline bool setString(const char *Name, const string &Value)		
	{
		if(oDC->setString(Data, Name, Value)){
			oB->updateChanged();
			return true;
		}
		else return false;
	}

	inline string getString(const char *Name) const
		{return oDC->getString(Data, Name);}

	inline bool setDate(const char *Name, const string &Value)		
	{
		if(oDC->setDate(Data, Name, Value)){
			oB->updateChanged();
			return true;
		}
		else return false;
	}

	inline string getDate(const char *Name) const
		{return oDC->getDate(Data, Name);}

	inline void buildDataFields(gdioutput &gdi) const
		{oDC->buildDataFields(gdi);}

  inline void buildDataFields(gdioutput &gdi, const vector<string> &fields) const
		{oDC->buildDataFields(gdi, fields);}

	inline void fillDataFields(gdioutput &gdi) const
		{oDC->fillDataFields(oB, Data, gdi);}

	inline bool saveDataFields(gdioutput &gdi) 
		{return oDC->saveDataFields(oB, Data, gdi);}

  inline string generateSQLDefinition() const
    {return oDC->generateSQLDefinition(std::set<string>());}

  inline string generateSQLDefinition(const std::set<string> &exclude) const
		{return oDC->generateSQLDefinition(exclude);}

	inline string generateSQLSet() const
		{return oDC->generateSQLSet(Data);}

	inline void getVariableInt(list<oVariableInt> &var) const
		{oDC->getVariableInt(Data, var);}

	inline void getVariableString(list<oVariableString> &var) const
		{oDC->getVariableString(Data, var);}

	inline void initData()
		{oDC->initData(Data, oDC->DataMaxSize);}

	inline bool write(xmlparser &xml) const
		{return oDC->write(Data, xml);}

	inline void set(const xmlobject &xo)
		{oDC->set(Data, xo);}

  void fillInput(const char *name, vector< pair<string, size_t> > &out, size_t &selected) const {
    oDC->fillInput(Data, -1, name, out, selected);
  }

  bool setEnum(const char *name, int selectedIndex) {
    if (oDC->setEnum(Data, name, selectedIndex) ) {
			oB->updateChanged();
			return true;
		}
		else return false;
  }

  int getDataAmountMeasure() const
    {return oDC->getDataAmountMeasure(Data);}
	
	oDataInterface(oDataContainer *odc, void *data, oBase *ob);
	~oDataInterface(void);
};

class oDataConstInterface
{
private:
	const void *Data;
	const oDataContainer *oDC;
	const oBase *oB;
public:

	inline int getInt(const char *Name) const
		{return oDC->getInt(Data, Name);}

  inline __int64 getInt64(const char *Name) const
		{return oDC->getInt64(Data, Name);}

	inline string getString(const char *Name) const
		{return oDC->getString(Data, Name);}

  inline string getDate(const char *Name) const
		{return oDC->getDate(Data, Name);}

	inline int getInt(const string &name) const
    {return oDC->getInt(Data, name.c_str());}

  inline __int64 getInt64(const string &name) const
		{return oDC->getInt64(Data, name.c_str());}

	inline string getString(const string &name) const
		{return oDC->getString(Data, name.c_str());}

  inline string getDate(const string &name) const
		{return oDC->getDate(Data, name.c_str());}

	inline void buildDataFields(gdioutput &gdi) const
		{oDC->buildDataFields(gdi);}

	inline void fillDataFields(gdioutput &gdi) const
		{oDC->fillDataFields(oB, Data, gdi);}

  inline string generateSQLDefinition() const
		{return oDC->generateSQLDefinition(set<string>());}

	inline string generateSQLDefinition(const set<string> &exclude) const
		{return oDC->generateSQLDefinition(exclude);}

	inline string generateSQLSet() const
		{return oDC->generateSQLSet(Data);}

	inline void getVariableInt(list<oVariableInt> &var) const
		{oDC->getVariableInt(Data, var);}

	inline void getVariableString(list<oVariableString> &var) const
		{oDC->getVariableString(Data, var);}

  inline bool write(xmlparser &xml) const
		{return oDC->write(Data, xml);}

  int getDataAmountMeasure() const
    {return oDC->getDataAmountMeasure(Data);}

  void fillInput(const char *name, vector< pair<string, size_t> > &out, size_t &selected) const {
    oDC->fillInput(Data, -1, name, out, selected);
  }

	oDataConstInterface(const oDataContainer *odc, const void *data, const oBase *ob);
	~oDataConstInterface(void);
};
