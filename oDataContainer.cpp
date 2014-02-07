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
#include <set>
#include "oDataContainer.h"
#include "oEvent.h"

#include "gdioutput.h"
#include "xmlparser.h"
#include "Table.h"
#include "meos_util.h"
#include "Localizer.h"
#include "meosException.h"

oDataContainer::oDataContainer(int maxsize)
{
	DataPointer=0;
	DataMaxSize=maxsize;
}

oDataContainer::~oDataContainer(void)
{
}

oDataInfo::oDataInfo() {
  memset(Name, 0, sizeof(Name));
	Index = 0;
	Size = 0;
	Type = 0;
	SubType = 0;
  tableIndex = 0;
  decimalSize = 0;
  decimalScale = 1;
  memset(Description, 0, sizeof(Description));
}

oDataInfo::~oDataInfo() {
}

void oDataContainer::addVariableInt(const char *name, oIntSize isize, const char *description)
{
	oDataInfo odi;
	odi.Index=DataPointer;	
	strcpy_s(odi.Name, name);
  strcpy_s(odi.Description, description);
	
  if (isize == oIS64)
    odi.Size=sizeof(__int64);
  else
    odi.Size=sizeof(int);

	odi.Type=oDTInt;
	odi.SubType=isize;

	if(DataPointer+odi.Size<=DataMaxSize){
		DataPointer+=odi.Size;
		addVariable(odi);
	}
	else 
    throw std::exception("oDataContainer: Out of bounds.");
}

void oDataContainer::addVariableDecimal(const char *name, const char *descr, int fixedDeci) {
  addVariableInt(name, oISDecimal, descr);
  Index[name].decimalSize = fixedDeci;
  int &s = Index[name].decimalScale;
  s = 1;
  for (int k = 0; k < fixedDeci; k++)
    s*=10;
}

void oDataContainer::addVariableString(const char *name, int maxChar, const char *descr)
{
	oDataInfo odi;
	odi.Index = DataPointer;	
	strcpy_s(odi.Name,name);
  strcpy_s(odi.Description,descr);
	odi.Size = maxChar+1;
	odi.Type = oDTString;
  odi.SubType = oSSString;

	if(DataPointer+odi.Size<=DataMaxSize){
		DataPointer+=odi.Size;
		addVariable(odi);
	}
  else 
    throw std::exception("oDataContainer: Out of bounds.");
}

void oDataContainer::addVariableEnum(const char *name, int maxChar, const char *descr, 
                     const vector< pair<string, string> > enumValues) {
  addVariableString(name, maxChar, descr);
  oDataInfo &odi = Index[name];
  odi.SubType = oSSEnum;
  for (size_t k = 0; k<enumValues.size(); k++)
    odi.enumDescription.push_back(enumValues[k]);
}

void oDataContainer::addVariable(oDataInfo &odi)
{
	if(findVariable(odi.Name))
		throw std::exception("oDataContainer: Variable already exist.");

  Index[odi.Name]=odi;
  ordered.push_back(odi.Name);
}

oDataInfo *oDataContainer::findVariable(const char *Name) 
{
  map<string, oDataInfo>::iterator it=Index.find(Name);

  if(it==Index.end())
    return 0;
  else return &(it->second);

	return 0;
}

const oDataInfo *oDataContainer::findVariable(const char *Name) const
{
  if (Name == 0)
    return 0;
  map<string, oDataInfo>::const_iterator it=Index.find(Name);

  if(it==Index.end())
    return 0;
  else return &(it->second);

	return 0;
}

void oDataContainer::initData(void *data, int datasize)
{
	if(datasize<DataPointer)
		throw std::exception("oDataContainer: Buffer too small.");

	memset(data, 0, DataPointer);
}

bool oDataContainer::setInt(void *data, const char *Name, int V)
{
	oDataInfo *odi=findVariable(Name);

	if(!odi) 
		throw std::exception("oDataContainer: Variable not found.");

	if(odi->Type!=oDTInt) 
		throw std::exception("oDataContainer: Variable of wrong type.");

  if(odi->SubType == oIS64)
		throw std::exception("oDataContainer: Variable to large.");

	LPBYTE vd=LPBYTE(data)+odi->Index;
	if(*((int *)vd)!=V){
		*((int *)vd)=V;
		return true;
	}
	else return false;//Not modified
}

bool oDataContainer::setInt64(void *data, const char *Name, __int64 V)
{
	oDataInfo *odi=findVariable(Name);

	if(!odi) 
		throw std::exception("oDataContainer: Variable not found.");

	if(odi->Type!=oDTInt) 
		throw std::exception("oDataContainer: Variable of wrong type.");

  if(odi->SubType != oIS64)
		throw std::exception("oDataContainer: Variable to large.");

	LPBYTE vd=LPBYTE(data)+odi->Index;
	if(*((__int64 *)vd)!=V){
		*((__int64 *)vd)=V;
		return true;
	}
	else return false;//Not modified
}

int oDataContainer::getInt(const void *data, const char *Name) const
{
	const oDataInfo *odi=findVariable(Name);

	if(!odi) 
		throw std::exception("oDataContainer: Variable not found.");

	if(odi->Type!=oDTInt) 
		throw std::exception("oDataContainer: Variable of wrong type.");

  if(odi->SubType == oIS64)
		throw std::exception("oDataContainer: Variable to large.");

	LPBYTE vd=LPBYTE(data)+odi->Index;
	return *((int *)vd);
}

__int64 oDataContainer::getInt64(const void *data, const char *Name) const
{
	const oDataInfo *odi=findVariable(Name);

	if(!odi) 
		throw std::exception("oDataContainer: Variable not found.");

	if(odi->Type!=oDTInt) 
		throw std::exception("oDataContainer: Variable of wrong type.");

  LPBYTE vd=LPBYTE(data)+odi->Index;

  if(odi->SubType == oIS64)
  	return *((__int64 *)vd);
  else {
    int tmp = *((int *)vd);
    return tmp;
  }
}


bool oDataContainer::setString(void *data, const char *Name, const string &V)
{
	oDataInfo *odi=findVariable(Name);

	if(!odi) 
    throw std::exception("oDataContainer: Variable not found.");

	if(odi->Type!=oDTString) 
    throw std::exception("oDataContainer: Variable of wrong type.");

	LPBYTE vd=LPBYTE(data)+odi->Index;	

	if(strcmp((char *)vd, V.c_str())!=0){
		strncpy_s((char *)vd, odi->Size, V.c_str(), odi->Size-1);
		return true;
	}
	else return false;//Not modified
}

string oDataContainer::getString(const void *data, const char *Name) const
{
	const oDataInfo *odi=findVariable(Name);

	if(!odi) 
    throw std::exception("oDataContainer: Variable not found.");

	if(odi->Type!=oDTString) 
    throw std::exception("oDataContainer: Variable of wrong type.");

	LPBYTE vd=LPBYTE(data)+odi->Index;
	return string((char *)vd);
}


bool oDataContainer::setDate(void *data, const char *Name, const string &V)
{
	oDataInfo *odi=findVariable(Name);

	if(!odi) 
		throw std::exception("oDataContainer: Variable not found.");

	if(odi->Type!=oDTInt) 
		throw std::exception("oDataContainer: Variable of wrong type.");

	int year=0,month=0,day=0;

	sscanf_s(V.c_str(), "%d-%d-%d", &year, &month, &day);

	int C=year*10000+month*100+day;


	LPBYTE vd=LPBYTE(data)+odi->Index;
	if(*((int *)vd)!=C){
		*((int *)vd)=C;
		return true;
	}
	else return false;//Not modified
}

string oDataContainer::getDate(const void *data, 
                               const char *Name) const
{
	const oDataInfo *odi=findVariable(Name);

	if(!odi) 
		throw std::exception("oDataContainer: Variable not found.");

	if(odi->Type!=oDTInt) 
		throw std::exception("oDataContainer: Variable of wrong type.");

	LPBYTE vd=LPBYTE(data)+odi->Index;
	int C=*((int *)vd);
	
	char bf[24];
	if(C%10000!=0 || C==0)
		sprintf_s(bf, "%04d-%02d-%02d", C/10000, (C/100)%100, C%100);
	else
		sprintf_s(bf, "%04d", C/10000);
			
	return string(bf);
}

bool oDataContainer::write(const void *data, xmlparser &xml) const 
{
	xml.startTag("oData");

	map<string, oDataInfo>::const_iterator it;

	it=Index.begin();

	while (it!=Index.end()) {
    const oDataInfo &di=it->second;
    if(di.Type==oDTInt){
			LPBYTE vd=LPBYTE(data)+di.Index;	
      if (di.SubType != oIS64) {
        int nr;
        memcpy(&nr, vd, sizeof(int));
			  xml.write(di.Name, nr);
      }
      else {
        __int64 nr;
        memcpy(&nr, vd, sizeof(__int64));
        xml.write64(di.Name, nr);
      }
		}
		else if(di.Type==oDTString){
			LPBYTE vd=LPBYTE(data)+di.Index;	
			xml.write(di.Name, (char *)vd);
		}
		++it;
	}
	
	xml.endTag();

	return true;
}

void oDataContainer::set(void *data, const xmlobject &xo)
{
	xmlList xl;
  xo.getObjects(xl);

	xmlList::const_iterator it;

	for(it=xl.begin(); it != xl.end(); ++it){

		oDataInfo *odi=findVariable(it->getName());

		if(odi){
			if(odi->Type==oDTInt){
				LPBYTE vd=LPBYTE(data)+odi->Index;
        if (odi->SubType != oIS64)
				  *((int *)vd) = it->getInt();
        else
          *((__int64 *)vd) = it->getInt64();
			}
			else if(odi->Type==oDTString){
				LPBYTE vd=LPBYTE(data)+odi->Index;	
				strncpy_s((char *)vd, odi->Size, it->get(), odi->Size-1);
			}
		}
	}
}

void oDataContainer::buildDataFields(gdioutput &gdi) const
{
  buildDataFields(gdi, ordered);
}

void oDataContainer::buildDataFields(gdioutput &gdi, const vector<string> &fields) const
{
	for (size_t k=0;k<fields.size();k++) {
    map<string, oDataInfo>::const_iterator it=Index.find(fields[k]);

    if (it==Index.end())
      throw std::exception( ("Bad key: " + fields[k]).c_str());

    const oDataInfo &di=it->second;
		string Id=di.Name+string("_odc");

		if(di.Type==oDTInt){
      if (di.SubType == oISDate || di.SubType == oISTime)
        gdi.addInput(Id, "", 10, 0, string(di.Description) + ":");
      else
        gdi.addInput(Id, "", 6, 0, string(di.Description) + ":");
		}
		else if(di.Type==oDTString){
      gdi.addInput(Id, "", min(di.Size+2, 30), 0, string(di.Description) +":");
		}
	}
}

int oDataContainer::getDataAmountMeasure(const void *data) const
{
  int amount = 0;
	map<string, oDataInfo>::const_iterator it;
	it=Index.begin();
	while (it!=Index.end()) {
    const oDataInfo &di=it->second;
		if (di.Type==oDTInt) {
    	LPBYTE vd=LPBYTE(data)+di.Index;				    
			int nr;
      memcpy(&nr, vd, sizeof(int));
      if (nr != 0)
        amount++;
		}
		else if (di.Type==oDTString) {
			LPBYTE vd=LPBYTE(data)+di.Index;	
      amount += strlen((char *)vd);
		}
		++it;
	}
  return amount;
}	

void oDataContainer::fillDataFields(const oBase *ob, const void *data, gdioutput &gdi) const
{
	map<string, oDataInfo>::const_iterator it;

	it=Index.begin();

	while(it!=Index.end()){
    const oDataInfo &di=it->second;
	
		string Id=di.Name+string("_odc");
		if(di.Type==oDTInt){
			LPBYTE vd=LPBYTE(data)+di.Index;
      
      if (di.SubType != oIS64) {
        int nr;
        memcpy(&nr, vd, sizeof(int));
        if (di.SubType == oISCurrency) {
          if (strcmp(di.Name, "CardFee") == 0 && nr == -1)
            nr = 0; //XXX CardFee hack. CardFee = 0 is coded as -1
          gdi.setText(Id.c_str(), ob->getEvent()->formatCurrency(nr));
        }
        else {
          char bf[64];
          formatNumber(nr, di, bf);
          gdi.setText(Id.c_str(), bf);        
        }
      }
      else {
        __int64 nr;
        memcpy(&nr, vd, sizeof(__int64));
        gdi.setText(Id.c_str(), itos(nr));
      }
		}
		else if(di.Type==oDTString){
			LPBYTE vd=LPBYTE(data)+di.Index;	
      gdi.setText(Id.c_str(), (char *)vd);
		}
		++it;
	}
}

bool oDataContainer::saveDataFields(const oBase *ob, void *data, gdioutput &gdi)
{
	map<string, oDataInfo>::iterator it;

	it=Index.begin();

	while(it!=Index.end()){
    const oDataInfo &di=it->second;

		string Id=di.Name+string("_odc");

    if (!gdi.hasField(Id)) {
      ++it;
      continue;
    }
		if(di.Type==oDTInt){
      int no = 0;
      if (di.SubType == oISCurrency) {
        no = ob->getEvent()->interpretCurrency(gdi.getText(Id.c_str()));
      }
      else if (di.SubType == oISDate) {
        no = convertDateYMS(gdi.getText(Id.c_str()));
      }
      else if (di.SubType == oISTime) {
        no = convertAbsoluteTimeHMS(gdi.getText(Id.c_str()));
      }
      else if (di.SubType == oISDecimal) {
        string str = gdi.getText(Id.c_str());
        for (size_t k = 0; k < str.length(); k++) {
          if (str[k] == ',') {
            str[k] = '.';
            break;
          }
        }
        double val = atof(str.c_str());
        no = int(di.decimalScale * val);
      }
      else {
        no = gdi.getTextNo(Id.c_str());
      }

			LPBYTE vd=LPBYTE(data)+di.Index;
			*((int *)vd)=no;							
		}
		else if(di.Type==oDTString){
			LPBYTE vd=LPBYTE(data)+di.Index;	
			
      strncpy_s((char *)vd, di.Size, gdi.getText(Id.c_str()).c_str(), di.Size-1);
		}
		++it;
	}

	return true;
}

string oDataContainer::C_INT64(const string &name)
{
	return " "+name+" BIGINT NOT NULL DEFAULT 0, ";
} 

string oDataContainer::C_INT(const string &name)
{
	return " "+name+" INT NOT NULL DEFAULT 0, ";
} 

string oDataContainer::C_SMALLINT(const string &name)
{
	return " "+name+" SMALLINT NOT NULL DEFAULT 0, ";
} 

string oDataContainer::C_TINYINT(const string &name)
{
	return " "+name+" TINYINT NOT NULL DEFAULT 0, ";
} 

string oDataContainer::C_SMALLINTU(const string &name)
{
	return " "+name+" SMALLINT UNSIGNED NOT NULL DEFAULT 0, ";
} 

string oDataContainer::C_TINYINTU(const string &name)
{
	return " "+name+" TINYINT UNSIGNED NOT NULL DEFAULT 0, ";
} 

string oDataContainer::C_STRING(const string &name, int len)
{
	char bf[16];
	sprintf_s(bf, "%d", len);
	return " "+name+" VARCHAR("+ bf +") NOT NULL DEFAULT '', ";
}

string oDataContainer::SQL_quote(const char *in)
{
	char out[256];
	int o=0;

	while(*in && o<250){
		if(*in=='\'') out[o++]='\'';		
		if(*in=='\\') out[o++]='\\';
		out[o++]=*in;

		in++;
	}
	out[o]=0;

	return out;
}

string oDataContainer::generateSQLDefinition(const std::set<string> &exclude) const {
	map<string, oDataInfo>::const_iterator it;
	it=Index.begin();

	string sql;
  bool addSyntx = !exclude.empty();

	while (it!=Index.end()) {
    if (exclude.count(it->first) == 0) {
      if (addSyntx)
         sql += "ADD COLUMN ";

      const oDataInfo &di=it->second;
		  if(di.Type==oDTInt){
        if(di.SubType==oIS32 || di.SubType==oISDate || di.SubType==oISCurrency || 
           di.SubType==oISTime || di.SubType==oISDecimal)
				  sql+=C_INT(it->first);
			  else if(di.SubType==oIS16)
				  sql+=C_SMALLINT(it->first);
			  else if(di.SubType==oIS8)
				  sql+=C_TINYINT(it->first);
			  else if(di.SubType==oIS64)
				  sql+=C_INT64(it->first);
        else if(di.SubType==oIS16U)
				  sql+=C_SMALLINTU(it->first);
			  else if(di.SubType==oIS8U)
				  sql+=C_TINYINTU(it->first);
		  }
		  else if(di.Type==oDTString){
        sql+=C_STRING(it->first, di.Size-1);
		  }
    }
		++it;
	}

  if (addSyntx && !sql.empty())
    return sql.substr(0, sql.length() - 2); //Remove trailing comma-space
  else
	  return sql;
}


string oDataContainer::generateSQLSet(const void *data) const
{
	map<string, oDataInfo>::const_iterator it;

	it=Index.begin();
	string sql;
	char bf[256];

	while(it!=Index.end()){
    const oDataInfo &di=it->second;
		if (di.Type==oDTInt) {
			LPBYTE vd=LPBYTE(data)+di.Index;
      if (di.SubType == oIS8U) {
			  sprintf_s(bf, ", %s=%u", di.Name, (*((int *)vd))&0xFF);
			  sql+=bf;
      }
      else if (di.SubType == oIS16U) {
			  sprintf_s(bf, ", %s=%u", di.Name, (*((int *)vd))&0xFFFF);
			  sql+=bf;
      }
      else if (di.SubType == oIS8) {
        char r = (*((int *)vd))&0xFF;
			  sprintf_s(bf, ", %s=%d", di.Name, (int)r);
			  sql+=bf;
      }
      else if (di.SubType == oIS16) {
        short r = (*((int *)vd))&0xFFFF;
			  sprintf_s(bf, ", %s=%d", di.Name, (int)r);
			  sql+=bf;
      }
      else if (di.SubType != oIS64) {
			  sprintf_s(bf, ", %s=%d", di.Name, *((int *)vd));
			  sql+=bf;
      }
      else {
        char tmp[32];
        _i64toa_s(*((__int64 *)vd), tmp, 32, 10);
			  sprintf_s(bf, ", %s=%s", di.Name, tmp);
			  sql+=bf;
      }
		}
		else if(di.Type==oDTString){
			LPBYTE vd=LPBYTE(data)+di.Index;	
			//gdi.setText(Id, (char *)vd);
			sprintf_s(bf, ", %s='%s'", di.Name, SQL_quote((char *)vd).c_str());
			sql+=bf;
		}
		++it;
	}
	return sql;
}


void oDataContainer::getVariableInt(const void *data, 
                                    list<oVariableInt> &var) const
{
	map<string, oDataInfo>::const_iterator it;

	it=Index.begin();
	var.clear();

	while(it!=Index.end()){
    const oDataInfo &di=it->second;
		if(di.Type==oDTInt){
			LPBYTE vd=LPBYTE(data)+di.Index;				
			
			oVariableInt vi;
			memcpy(vi.Name, di.Name, sizeof(vi.Name));
			vi.Data=(int *)vd;			
			var.push_back(vi);		
		}
	
		++it;
	}
}


void oDataContainer::getVariableString(const void *data, 
                                       list<oVariableString> &var) const
{
	map<string, oDataInfo>::const_iterator it;

	it=Index.begin();
	var.clear();

	while(it!=Index.end()){
    const oDataInfo &di=it->second;
		if(di.Type==oDTString){
			LPBYTE vd=LPBYTE(data)+di.Index;				
			
			oVariableString vs;
			memcpy(vs.Name, di.Name, sizeof(vs.Name));
			vs.Data=(char *)vd;			
			vs.MaxSize=di.Size;
			var.push_back(vs);		
		}	
		++it;
	}
}

oDataInterface oDataContainer::getInterface(void *data, int datasize, oBase *ob)
{
	if(datasize<DataPointer) throw std::exception("Out Of Bounds.");

	return oDataInterface(this, data, ob);
}

oDataConstInterface oDataContainer::getConstInterface(const void *data, int datasize, 
                                                      const oBase *ob) const
{
	if(datasize<DataPointer) throw std::exception("Out Of Bounds.");

	return oDataConstInterface(this, data, ob);
}


oDataInterface::oDataInterface(oDataContainer *odc, void *data, oBase *ob)
{
	oB=ob;
	oDC=odc;
	Data=data;
}

oDataConstInterface::~oDataConstInterface(void)
{
}

oDataConstInterface::oDataConstInterface(const oDataContainer *odc, const void *data, const oBase *ob)
{
	oB=ob;
	oDC=odc;
	Data=data;
}

oDataInterface::~oDataInterface(void)
{
}


void oDataContainer::buildTableCol(Table *table)
{
	map<string, oDataInfo>::iterator it;

	it=Index.begin();

	while (it!=Index.end()) {
    oDataInfo &di=it->second;

		if(di.Type==oDTInt){
      bool right = di.SubType == oISCurrency;
      bool numeric = di.SubType != oISDate && di.SubType != oISTime;
      int w;
      if (di.SubType == oISDecimal)
        w = max( (di.decimalSize+4)*10, 70);
      else
        w = 70;

      w = max(int(strlen(di.Description))*6, w);
      di.tableIndex = table->addColumn(di.Description, w, numeric, right);
		}
		else if(di.Type==oDTString){
      int w = max(max(di.Size+1, int(strlen(di.Description)))*6, 70);

      for (size_t k = 0; k < di.enumDescription.size(); k++)
        w = max<int>(w, lang.tl(di.enumDescription[k].second).length() * 6);

      di.tableIndex = table->addColumn(di.Description, w, false);
		}
		++it;
	}
}

bool oDataContainer::formatNumber(int nr, const oDataInfo &di, char bf[64]) const {
  if (di.SubType == oISDate) {
    if (nr>0) {
      sprintf_s(bf, 64, "%d-%02d-%02d", nr/(100*100), (nr/100)%100, nr%100);
    }
    else {
      bf[0] = '-';
      bf[1] = 0;
    }
    return true;
  }
  else if (di.SubType == oISTime) {
    if (nr>0 && nr<(30*24*3600)) {
      if (nr < 24*3600)
        sprintf_s(bf, 64, "%02d:%02d:%02d", nr/3600, (nr/60)%60, nr%60);
      else {
        int days = nr / (24*3600);
        nr = nr % (24*3600);
        sprintf_s(bf, 64, "%d+%02d:%02d:%02d", days, nr/3600, (nr/60)%60, nr%60);
      }
    }
    else {
      bf[0] = '-';
      bf[1] = 0;
    }
    return true;
  }
  else if (di.SubType == oISDecimal) {
    if (nr) {
      int whole = nr / di.decimalScale;
      int part = nr - whole * di.decimalScale;
      string deci = ",";
      string ptrn = "%d" + deci + "%0" + itos(di.decimalSize) + "d";
      sprintf_s(bf, 64, ptrn.c_str(), whole, abs(part));
    }
    else
      bf[0] = 0;

    return true;
  }
  else {
    if(nr) 
      sprintf_s(bf, 64, "%d", nr);
    else 
      bf[0] = 0;
    return true;
  }
}

int oDataContainer::fillTableCol(const void *data, const oBase &owner, Table &table, bool canEdit) const
{
	map<string, oDataInfo>::const_iterator it;
  int nextIndex = 0;
	it=Index.begin();
  char bf[64];
	oBase &ob = *(oBase *)&owner;
  while(it!=Index.end()){
    const oDataInfo &di=it->second;
		if (di.Type==oDTInt) {
			LPBYTE vd=LPBYTE(data)+di.Index;
      if (di.SubType != oIS64) {
        int nr;
        memcpy(&nr, vd, sizeof(int));
        if (di.SubType == oISCurrency) {
          table.set(di.tableIndex, ob, 1000+di.tableIndex, ob.getEvent()->formatCurrency(nr), canEdit);
        }
        else {
          formatNumber(nr, di, bf);
          table.set(di.tableIndex, ob, 1000+di.tableIndex, bf, canEdit);
        }
      }
      else {
        __int64 nr;
        memcpy(&nr, vd, sizeof(__int64));
        table.set(di.tableIndex, ob, 1000+di.tableIndex, itos(nr), canEdit);
      }
		}
		else if (di.Type==oDTString) {
			LPBYTE vd=LPBYTE(data)+di.Index;
      if (di.SubType == oSSString || !canEdit) {
        table.set(di.tableIndex, *((oBase*)&owner), 1000+di.tableIndex, (char *)vd, canEdit, cellEdit);
      }
      else {
        string str((char *)vd);
        for (size_t k = 0; k<di.enumDescription.size(); k++) {
          if ( str == di.enumDescription[k].first ) {
            str = lang.tl(di.enumDescription[k].second);
            break;
          }
        }
        table.set(di.tableIndex, *((oBase*)&owner), 1000+di.tableIndex, str, true, cellSelection);
      }
		}
    nextIndex = di.tableIndex + 1;   
		++it;
	}
  return nextIndex;
}

bool oDataContainer::inputData(oBase *ob, void *data, int id, 
                               const string &input, int inputId, 
                               string &output, bool noUpdate)
{
  map<string, oDataInfo>::iterator it;
  it=Index.begin();

	while (it!=Index.end()) {
    const oDataInfo &di=it->second;

    if (di.tableIndex+1000==id) {
		  if (di.Type==oDTInt) {
        LPBYTE vd=LPBYTE(data)+di.Index;
        
        int no = 0;
        if (di.SubType == oISCurrency) {
          no = ob->getEvent()->interpretCurrency(input);
        }
        else if (di.SubType == oISDate) {
          no = convertDateYMS(input);
        }
        else if (di.SubType == oISTime) {
          no = convertAbsoluteTimeHMS(input);
        }
        else if (di.SubType == oISDecimal) {
          string str = input;
          for (size_t k = 0; k < str.length(); k++) {
            if (str[k] == ',') {
              str[k] = '.';
              break;
            }
          }
          double val = atof(str.c_str());
          no = int(di.decimalScale * val);
        }
        else if (di.SubType == oIS64) {
          __int64 no64 = _atoi64(input.c_str());
          __int64 k64;
          memcpy(&k64, vd, sizeof(__int64));
          memcpy(vd, &no64, sizeof(__int64));
          __int64 out64 = no64;
          if (k64 != no64) {
            ob->updateChanged();
            if (noUpdate == false)
              ob->synchronize(true);

            memcpy(&out64, vd, sizeof(__int64));
          }
          output = itos(out64);
          return k64 != no64;
        }
        else
          no = atoi(input.c_str());

			  int k;
        memcpy(&k, vd, sizeof(int));
        memcpy(vd, &no, sizeof(int));
        char bf[128];
        int outN = no;

        if (k != no) {
          ob->updateChanged();
          if (noUpdate == false)
            ob->synchronize(true);

          memcpy(&outN, vd, sizeof(int));
        }

        formatNumber(outN, di, bf);
        output = bf;
        return k != no;
      }
		  else if (di.Type==oDTString) {
			  LPBYTE vd=LPBYTE(data)+di.Index;	
        const char *str = input.c_str();
        
        if (di.SubType == oSSEnum) {
          size_t ix = inputId-1;
          if (ix < di.enumDescription.size()) {
            str = di.enumDescription[ix].first.c_str();
          }
        }

        if (strcmp((char *)vd, str)!=0) {
		      strncpy_s((char *)vd, di.Size, str, di.Size-1);
		      
          ob->updateChanged();
          if (noUpdate == false)
            ob->synchronize(true);

          if (di.SubType == oSSEnum) {
            size_t ix = inputId-1;
            if (ix < di.enumDescription.size()) {
              output = lang.tl(di.enumDescription[ix].second); 
              // This might be incorrect if data was changed on server,
              // but this issue is minor, I think: conflicts have no resolution
              // Anyway, the row will typically be reloaded
            }
            else
              output=(char *)vd;
          }
          else
            output=(char *)vd;
          return true;
        }
        else 
          output=input;

        return false;
      }
	  }
		++it;
	}
  return true;
}

void oDataContainer::fillInput(const void *data, int id, const char *name, 
                               vector< pair<string, size_t> > &out, size_t &selected) const {


  map<string, oDataInfo>::const_iterator it;
  it = Index.begin();
  const oDataInfo * info = findVariable(name);
  
  if (!info) {
	  while (it!=Index.end()) {
      const oDataInfo &di=it->second;
      if (di.tableIndex+1000==id && di.Type == oDTString && di.SubType == oSSEnum) {
        info = &di;
        break;
      }
      ++it;
    }
  }

  if (info && info->Type == oDTString && info->SubType == oSSEnum) {
    char *vd = (char *)(LPBYTE(data)+info->Index);	
          
    selected = -1;
    for (size_t k = 0; k < info->enumDescription.size(); ++k) {
      out.push_back(make_pair(lang.tl(info->enumDescription[k].second), k+1));
      if (info->enumDescription[k].first == string(vd))
        selected = k+1;
    }
  }
  else
    throw meosException("Invalid enum");
}

bool oDataContainer::setEnum(void *data, const char *name, int selectedIndex) {
  const oDataInfo * info = findVariable(name);
  
  if (info  && info->Type == oDTString && info->SubType == oSSEnum) {
    if (size_t(selectedIndex - 1) < info->enumDescription.size()) {
      return setString(data, name, info->enumDescription[selectedIndex-1].first);
    }
  }
  throw meosException("Invalid enum");
}
