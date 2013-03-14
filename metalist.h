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

class oListInfo;
enum EPostType;
#include "oListInfo.h"
#include <map>
class xmlparser;
class xmlobject;
enum gdiFonts;
class oEvent;

class Position 
{
  struct PosInfo {
    PosInfo(int f, int wid) : first(f), width(wid), aligned(false) {}
    int first;   // Actual position
    int width;   // Original block width
    bool aligned;// True if aligned
  };
  map<string, int> pmap;
  vector< PosInfo > pos; // Pair of position, specified (minimal) width
  void update(int ix, const string &newname, int width, bool alignBlock, bool alignLock);

public:
  bool postAdjust();

  void add(const string &name, const int width, int blockWidth);
  void add(const string &name, const int width) {
    add(name, width, width);
  }
  
  void update(const string &oldname, const string &newname, 
              const int width, bool alignBlock, bool alignLock);
  void alignNext(const string &newname, const int width, bool alignBlock);

  void newRow();
  
  int get(const string &name);
  int get(const string &name, double scale);

  void indent(int ind);
};

class MetaListPost {
private:
  void serialize(xmlparser &xml) const;
  void deserialize(const xmlobject &xml);

  EPostType type;
  string text;
  string alignWithText;
  EPostType alignType;
  int leg;
  int minimalIndent;
  bool alignBlock; // True if the next item should also be align (table style block)
  int blockWidth;
  gdiFonts font;

public:

  MetaListPost(EPostType type_, EPostType align_ = lNone, int leg_ = -1);

  MetaListPost &setBlock(int width) {blockWidth = width; return *this;}
  MetaListPost &setText(const string &text_) {text = text_; return *this;}
  MetaListPost &align(EPostType align_, bool alignBlock_ = true) {alignType = align_; alignBlock = alignBlock_; return *this;}
  MetaListPost &align(bool alignBlock_ = true) {return align(lAlignNext, alignBlock_);}
  MetaListPost &alignText(const string &t) {alignWithText = t; return *this;}
  
  MetaListPost &indent(int ind) {minimalIndent = ind; return *this;}
  
  void getTypes(vector< pair<string, size_t> > &types, int &currentType) const;
  
  const string &getType() const;
  MetaListPost &setType(EPostType type_) {type = type_; return *this;}
  
  const string &getText() const {return text;}
  const string &getAlignText() const {return alignWithText;}

  int getLeg() const {return leg;}
  void setLeg(int leg_) {leg = leg_;}
  
  int getMinimalIndent() const {return minimalIndent;}
  bool getAlignBlock() const {return alignBlock;} 
  int getBlockWidth() const {return blockWidth;}

  const string &getFont() const;
  void setFont(gdiFonts font_) {font = font_;}
  
  void getFonts(vector< pair<string, size_t> > &fonts, int &currentFont) const;
 
  friend class MetaList;
};

class MetaList {
private:
  vector< vector< vector<MetaListPost> > > data;
  vector< pair<string, int> > fontFaces;

  string listName;
  string tag;
  string uniqueIndex;

  bool hasResults_;

  oListInfo::EBaseType listType;
  oListInfo::EBaseType listSubType;
  SortOrder sortOrder;

  set<EFilterList> filter;

  enum ListIndex {MLHead = 0, MLSubHead = 1, MLList = 2, MLSubList=3};
  MetaListPost &add(ListIndex ix, const MetaListPost &post);
  void addRow(int ix);
  string encode(const string &input) const;
  bool isBreak(int x) const;

  static map<EPostType, string> typeToSymbol;
  static map<string, EPostType> symbolToType;
  
  static map<oListInfo::EBaseType, string> baseTypeToSymbol;
  static map<string, oListInfo::EBaseType> symbolToBaseType;
  
  static map<SortOrder, string> orderToSymbol;
  static map<string, SortOrder> symbolToOrder;
  
  static map<EFilterList, string> filterToSymbol;
  static map<string, EFilterList> symbolToFilter;

  static map<gdiFonts, string> fontToSymbol;
  static map<string, gdiFonts> symbolToFont;

  static void initSymbols();

  void serialize(xmlparser &xml, const string &tag, 
                 const vector< vector<MetaListPost> > &lp) const;
  void deserialize(const xmlobject &xml, vector< vector<MetaListPost> > &lp);

public:
  MetaList();
  virtual ~MetaList() {}

  void initUniqueIndex();
  const string &getUniqueId() const {return uniqueIndex;}

  void getFilters(vector< pair<string, bool> > &filters) const;
  void setFilters(const vector<bool> &filters);
  void getSortOrder(vector< pair<string, size_t> > &orders, int &currentOrder) const;
  void getBaseType(vector< pair<string, size_t> > &types, int &currentType) const;
  void getSubType(vector< pair<string, size_t> > &types, int &currentType) const;

  const string &getFontFace(int type) const {return fontFaces[type].first;}
  int getFontFaceFactor(int type) const {return fontFaces[type].second;}
  
  MetaList &setFontFace(int type, const string &face, int factor) {
    fontFaces[type] = make_pair(face, factor);
    return *this;
  }

  void getExistingTypes(vector< pair<string, size_t> > &types) const;
  
  const string &getListName() const {return listName;}
  oListInfo::EBaseType getListType() const {return listType;}
  bool hasResults() const {return hasResults_;}
  const string &getTag() const {return tag;}

  void getAlignTypes(const MetaListPost &mlp, vector< pair<string, size_t> > &types, int &currentType) const;
  void getIndex(const MetaListPost &mlp, int &gix, int &lix, int &ix) const;

  MetaList &setListType(oListInfo::EBaseType t) {listType = t; return *this;}
  MetaList &setSubListType(oListInfo::EBaseType t) {listSubType = t; return *this;}
  MetaList &setSortOrder(SortOrder so) {sortOrder = so; return *this;}

  MetaList &addFilter(EFilterList f) {filter.insert(f); return *this;}

  void save(const string &file) const;
  void load(const string &file);
  
  bool isValidIx(size_t gIx, size_t lIx, size_t ix) const;

  void save(xmlparser &xml) const;
  void load(const xmlobject &xDef);

  void interpret(oEvent *oe, const gdioutput &gdi, const oListParam &par, 
                 int lineHeight, oListInfo &li) const;

  MetaList &setListName(const string &title) {listName = title; return *this;}

  MetaListPost &addNew(int groupIx, int lineIx, int &ix);
  MetaListPost &getMLP(int groupIx, int lineIx, int ix);
  void removeMLP(int groupIx, int lineIx, int ix);
  void moveOnRow(int groupIx, int lineIx, int &ix, int delta);
  
  MetaListPost &addToHead(const MetaListPost &post) {return add(MLHead, post);}
  MetaListPost &addToSubHead(const MetaListPost &post) {return add(MLSubHead, post).setBlock(10);}
  MetaListPost &addToList(const MetaListPost &post) {return add(MLList, post);}
  MetaListPost &addToSubList(const MetaListPost &post) {return add(MLSubList, post);}

  const vector< vector<MetaListPost> > &getList() const {return data[MLList];}
  const vector< vector<MetaListPost> > &getSubList() const {return data[MLSubList];}
  const vector< vector<MetaListPost> > &getHead() const {return data[MLHead];}
  const vector< vector<MetaListPost> > &getSubHead() const {return data[MLSubHead];}

  vector< vector<MetaListPost> > &getList() {return data[MLList];}
  vector< vector<MetaListPost> > &getSubList() {return data[MLSubList];}
  vector< vector<MetaListPost> > &getHead() {return data[MLHead];}
  vector< vector<MetaListPost> > &getSubHead() {return data[MLSubHead];}

  void newListRow() {addRow(MLList);}
  void newSubListRow() {addRow(MLSubList);}
  void newHead() {addRow(MLHead);}
  void newSubHead() {addRow(MLSubHead);}

  friend class MetaListPost;
};

class MetaListContainer {
public:
  enum MetaListType {InternalList, ExternalList, RemovedList};
private:
  vector< pair<MetaListType, MetaList> > data;
  mutable map<EStdListType, int> globalIndex;
  mutable map<string, EStdListType> tagIndex;
  mutable map<string, EStdListType> uniqueIndex;

  map<int, oListParam> listParam; 
  oEvent *owner;
public:

  MetaListContainer(oEvent *owner);
  virtual ~MetaListContainer();

  string getUniqueId(EStdListType code) const;
  EStdListType getCodeFromUnqiueId(const string &id) const;
  string makeUniqueParamName(const string &nameIn) const;

  int getNumParam() const {return listParam.size();}
  int getNumLists() const {return data.size();}
  int getNumLists(MetaListType) const;
  
  EStdListType getType(const string &tag) const;
  EStdListType getType(const int index) const;

  const MetaList &getList(int index) const;
  MetaList &getList(int index);

  const oListParam &getParam(int index) const;
  oListParam &getParam(int index);

  MetaList &addExternal(const MetaList &ml);
  void clearExternal();

  void getLists( vector< pair<string, size_t> > &lists) const;
  void removeList(int index);
  void saveList(int index, const MetaList &ml);
  bool isInternal(int index) const {return data[index].first == InternalList;}
  bool isExternal(int index) const {return data[index].first == ExternalList;}

  void save(MetaListType type, xmlparser &xml) const;
  void load(MetaListType type, const xmlobject &xDef);
  
  void setupListInfo(int firstIndex, map<EStdListType, oListInfo> &listMap, bool resultsOnly) const;
  void setupIndex(int firstIndex) const;

  void getListParam( vector< pair<string, size_t> > &param) const;
  void removeParam(int index);
  void addListParam(oListParam &listParam);

  bool interpret(oEvent *oe, const gdioutput &gdi, const oListParam &par, 
                 int lineHeight, oListInfo &li) const;
};
