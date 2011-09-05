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

class oListInfo;


class Position 
{
  map<string, int> pmap;
  vector<int> pos;
  void update(int ix, const string &newname, int width, bool alignBlock);

public:
  bool postAdjust();

  void add(const string &name, int width);
  void update(const string &oldname, const string &newname, int width, bool alignBlock);
  void alignNext(const string &newname, int width, bool alignBlock);

  void newRow();
  
  int get(const string &name);
  int get(const string &name, double scale);

  void indent(int ind);
};

struct MetaListPost {
  EPostType type;
  string text;
  EPostType alignType;
  int leg;
  int minimalIndent;
  bool alignBlock; // True if the next item should also be align (table style block)
  int blockWidth;
  MetaListPost(EPostType type_, EPostType align_ = lNone, int leg_ = 0) : type(type_), alignType(align_), leg(leg_), minimalIndent(0), alignBlock(true), blockWidth(0) {}
  
  MetaListPost &setBlock(int width) {blockWidth = width; return *this;}
  MetaListPost &setText(const string &text_) {text = text_; return *this;}
  MetaListPost &align(EPostType align_, bool alignBlock_ = true) {alignType = align_; alignBlock = alignBlock_; return *this;}
  MetaListPost &align(bool alignBlock_ = true) {return align(lAlignNext, alignBlock_);}
  MetaListPost &indent(int ind) {minimalIndent = ind; return *this;}
  
  //MetaListPost(EPostType type_, const string &text_,  EPostType align_ = lNone, int leg_ = 0) : type(type_), text(text_), leg(leg_), align(align_) {}
};

class MetaList {
public:
  vector< vector< vector<MetaListPost> > > data;
  string defaultTitle;
  
  enum ListIndex {MLHead = 0, MLSubHead = 1, MLList = 2, MLSubList=3};
  MetaListPost &add(ListIndex ix, const MetaListPost &post);
  void addRow(int ix);
  string encode(const string &input) const;
  bool isBreak(int x) const;


public:
  MetaList();
  virtual ~MetaList() {}

  void interpret(const oEvent *oe, const oListParam &par, 
                 int lineHeight, oListInfo &li) const;

  MetaList &setDefaultTitle(const string &title) {defaultTitle = title; return *this;}
  MetaListPost &addToHead(const MetaListPost &post) {return add(MLHead, post);}
  MetaListPost &addToSubHead(const MetaListPost &post) {return add(MLSubHead, post).setBlock(10);}
  MetaListPost &addToList(const MetaListPost &post) {return add(MLList, post);}
  MetaListPost &addToSubList(const MetaListPost &post) {return add(MLSubList, post);}

  void newListRow() {addRow(MLList);}
  void newSubListRow() {addRow(MLSubList);}
  void newHead() {addRow(MLHead);}
  void newSubHead() {addRow(MLSubHead);}

  const vector< vector<MetaListPost> > &getList() const {return data[MLList];}
  const vector< vector<MetaListPost> > &getSubList() const {return data[MLSubList];}
  const vector< vector<MetaListPost> > &getHead() const {return data[MLHead];}
  const vector< vector<MetaListPost> > &getSubHead() const {return data[MLSubHead];}
};

