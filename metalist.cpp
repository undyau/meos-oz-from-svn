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
#include "oListInfo.h"
#include "oEvent.h"
#include "gdioutput.h"
#include "meos_util.h"
#include <cassert>
#include <cmath>
#include "Localizer.h"
#include "metalist.h"
#include <algorithm>
#include "meosexception.h"
#include "gdifonts.h"

oListParam::oListParam() {
  listCode = EStdResultList; //Just need a default
  cb = 0;
  legNumber = 0;
  useControlIdResultTo = 0;
  useControlIdResultFrom = 0;
  filterMaxPer = 0;
  pageBreak = false;
  showInterTimes = false;
  showSplitTimes = false;
  splitAnalysis = false;
  useLargeSize = false;
  saved = false;
}

void oListParam::serialize(xmlparser &xml, const MetaListContainer &container) const {
  xml.startTag("ListParam", "Name", name);
  xml.write("ListId", container.getUniqueId(listCode));
  string sel;
  for (set<int>::const_iterator it = selection.begin(); it != selection.end(); ++it) {
    if (!sel.empty())
      sel += ";";
    sel += itos(*it);
  }
  xml.write("ClassId", sel);
  xml.write("LegNumber", legNumber);
  xml.write("FromControl", useControlIdResultFrom);
  xml.write("ToControl", useControlIdResultTo);
  xml.write("MaxFilter", filterMaxPer);
  xml.write("Title", title);
  xml.writeBool("Large", useLargeSize);
  xml.writeBool("PageBreak", pageBreak);
  xml.writeBool("ShowNamedSplits", showInterTimes);
  xml.writeBool("ShowSplits", showSplitTimes);
  xml.writeBool("ShowAnalysis", splitAnalysis);
  xml.endTag();
}

void oListParam::deserialize(const xmlobject &xml, const MetaListContainer &container) {
  xml.getObjectString("Name", name);
  string id;
  xml.getObjectString("ListId", id);
  listCode = container.getCodeFromUnqiueId(id);
  
  string sel;
  xml.getObjectString("ClassId", sel);
  vector<string> selVec;
  split(sel, ";", selVec);
  for (size_t k = 0; k < selVec.size(); k++) {
    selection.insert(atoi(selVec[k].c_str())); 
  }
  legNumber = xml.getObjectInt("LegNumber");
  useControlIdResultFrom = xml.getObjectInt("FromControl");
  useControlIdResultTo = xml.getObjectInt("ToControl");
  filterMaxPer = xml.getObjectInt("MaxFilter");
  xml.getObjectString("Title", title);
  
  useLargeSize = xml.getObjectBool("Large");
  pageBreak = xml.getObjectBool("PageBreak");
  showInterTimes = xml.getObjectBool("ShowNamedSplits");
  showSplitTimes = xml.getObjectBool("ShowSplits");
  splitAnalysis = xml.getObjectBool("ShowAnalysis");
  saved = true;
}

void oListParam::getCustomTitle(char *t) const 
{
  if (!title.empty())
    strcpy_s(t, 256, MakeDash(title).c_str());
}

const string &oListParam::getCustomTitle(const string &t) const 
{
  if (!title.empty())
    return title;
  else
    return t;
}

MetaList::MetaList() {
  data.resize(4);
  fontFaces.resize(4);
  hasResults_ = false;
  initSymbols();
  listType = oListInfo::EBaseTypeRunner;
  listSubType = oListInfo::EBaseTypeNone;
  sortOrder = SortByName;
}

MetaListPost::MetaListPost(EPostType type_, EPostType align_, int leg_) : type(type_),
    alignType(align_), leg(leg_), minimalIndent(0), alignBlock(true), blockWidth(0), font(formatIgnore)
{}
  
static int checksum(const string &str) {
  int ret = 0;
  for (size_t k = 0; k<str.length(); k++)
    ret = ret * 19 + str[k];
  return ret;
}

void MetaList::initUniqueIndex() {
  __int64 ix = 0;

  for (int i = 0; i<4; i++) {
    const vector< vector<MetaListPost> > &lines = data[i];
    for (size_t j = 0; j<lines.size(); j++) {
      const vector<MetaListPost> &cline = lines[j];
      for (size_t k = 0; k<cline.size(); k++) {
        const MetaListPost &mp = cline[k];
        int value = mp.alignBlock;
        value = value * 31 + mp.type;
        value = value * 31 + mp.leg;
        value = value * 31 + checksum(mp.alignWithText);
        value = value * 31 + mp.alignType;
        value = value * 31 + mp.blockWidth;
        value = value * 31 + mp.font;
        value = value * 31 + checksum(mp.text);
        value = value * 31 + mp.minimalIndent;
        ix = ix * 997 + value;
      }
    }
  }
  
  unsigned yx = 0;
  yx = yx * 31 + checksum(listName);
  yx = yx * 31 + listType;
  yx = yx * 31 + listSubType;
  yx = yx * 31 + sortOrder;
  for (int i = 0; i < 4; i++) {
    yx = yx * 31 + checksum(fontFaces[i].first);
    yx = yx * 31 + fontFaces[i].second;
  }

  uniqueIndex = "A" + itos(ix) + "B" + itos(yx);
}

bool MetaList::isBreak(int x) const {
  return isspace(x) || x == '.' || x == ',' || 
          x == '-'  || x == ':' || x == ';' || x == '('
          || x == ')' || x=='/' || (x>30 && x < 127 && !isalnum(x));
}

string MetaList::encode(const string &input_) const {
  string out;
  string input = lang.tl(input_);
  out.reserve(input.length() + 5);

  for (size_t k = 0; k<input.length(); k++) {
    int c = input[k];
    int p = k > 0 ? input[k-1] : ' ';
    int n = k+1 < input.length() ? input[k+1] : ' ';

    if (c == '%') {
      out.push_back('%');
      out.push_back('%');
    }
    else if (c == 'X' &&  isBreak(n) && isBreak(p) ) {
      out.push_back('%');
      out.push_back('s');
    }
    else
      out.push_back(c);
  }
  return out;
}

MetaListPost &MetaList::add(ListIndex ix, const MetaListPost &post) {
  if (data[ix].empty())
    addRow(ix);
  //  data[ix].resize(1);
  
  vector<MetaListPost> &d = data[ix].back();
  d.push_back(post);
  return d.back();
}

void MetaList::addRow(int ix) {
  data[ix].push_back(vector<MetaListPost>());
}

#ifndef MEOSDB

void MetaList::interpret(oEvent *oe, const gdioutput &gdi, const oListParam &par, 
                         int lineHeight, oListInfo &li) const {
  const MetaList &mList = *this;
  Position pos;
  const bool large = par.useLargeSize;

  gdiFonts normal, header, small, italic;
  double s;

  map<pair<gdiFonts, int>, int> fontHeight;
  
  for (size_t k = 0; k < fontFaces.size(); k++) {
    for (map<gdiFonts, string>::const_iterator it = fontToSymbol.begin();
      it != fontToSymbol.end(); ++it) {
        string face = fontFaces[k].first;
        if (fontFaces[k].second > 0 && fontFaces[k].second != 100) {
          face += ";" + itos(fontFaces[k].second/100) + "." + itos(fontFaces[k].second%100);
        }
        fontHeight[make_pair(it->first, int(k))] = gdi.getLineHeight(it->first, face.c_str());
    }
  }

  if (large) {
    s = 1.8;
    normal = fontLarge;
    header = boldLarge;
    small = normalText;
    lineHeight = int(lineHeight *1.6);
    italic = italicMediumPlus;
  }
  else {
    s = 1.0;
    normal = normalText;
    header = boldText;
    small = italicSmall;
    italic = italicText;
  }

  map<EPostType, string> labelMap;
  map<string, string> stringLabelMap;
  
  set<EPostType> skip;
  li.calcResults = false;
  li.calcTotalResults = false;
  li.rogainingResults = false;
  li.calcCourseClassResults = false;

  for (int i = 0; i<4; i++) {
    const vector< vector<MetaListPost> > &lines = mList.data[i];
    gdiFonts defaultFont = normal;
    switch (i) {
      case 0:
      case 1:
        defaultFont = header;
      break;
      case 3: { 
        if (mList.listSubType == oListInfo::EBaseTypePunches)
          defaultFont = small;
        else
          defaultFont = italic;
      }
    }
    for (size_t j = 0; j<lines.size(); j++) {
      const vector<MetaListPost> &cline = lines[j];
      for (size_t k = 0; k<cline.size(); k++) {
        const MetaListPost &mp = cline[k];

        // Automatically determine what needs to be calculated
        if (mp.type == lTeamPlace || mp.type == lRunnerPlace) {
          if (!li.calcResults) {
            oe->calculateResults(oEvent::RTClassResult);
            oe->calculateTeamResults();
          }
          li.calcResults = true;
        }
        else if (mp.type == lRunnerTotalPlace) {
          if (!li.calcTotalResults)
            oe->calculateResults(oEvent::RTTotalResult);

          li.calcTotalResults = true;
        }
        else if (mp.type == lRunnerRogainingPoint) {
          //if (!li.rogainingResults)
          //  oe->calculateRogainingResults();
          li.rogainingResults = true;
        }
        else if (mp.type == lRunnerClassCoursePlace || mp.type == lRunnerClassCourseTimeAfter) {
          if (!li.calcCourseClassResults)
            oe->calculateResults(oEvent::RTClassCourseResult);

          li.calcCourseClassResults = true;
        }

        string label = "P" + itos(i*1000 + j*100 + k);
        
        if (mp.type == lRunnerBib || mp.type == lTeamBib) {
          if (!oe->hasBib(mp.type == lRunnerBib, mp.type == lTeamBib))
            skip.insert(mp.type);
        }

        if (mp.type == lTeamStart || mp.type == lRunnerStart ||
             mp.type == lClassStartTime) {
          bool hasIndStart = false;
          vector<int> classes(par.selection.begin(), par.selection.end());

          if (par.selection.empty()) {
            vector<pClass> cls;
            oe->getClasses(cls);
            for (size_t k = 0; k < cls.size(); k++)
              classes.push_back(cls[k]->getId());
          }
          
          for (vector<int>::const_iterator it = classes.begin(); it != classes.end(); ++it) {
            pClass pc = oe->getClass(*it);
            if (pc) {
              int first, last;
              pc->getStartRange(par.legNumber, first, last);
              if (last != first) {
                hasIndStart = true;
                break;
              }
            }
          }
          
          if (mp.type == lClassStartTime) {
            if (hasIndStart)
              skip.insert(mp.type);
          }
          else {
            if (!hasIndStart)
              skip.insert(mp.type);
          }
        }

        int width = 0;
        if (skip.count(mp.type) == 0) {
          gdiFonts font = defaultFont;
          if (mp.font != formatIgnore)
            font = mp.font;
          width = li.getMaxCharWidth(oe, mp.type, encode(mp.text), font, 
            oPrintPost::encodeFont(fontFaces[i].first, fontFaces[i].second).c_str(), large, mp.blockWidth);
        }

        if (mp.alignType == lNone) {
          pos.add(label, width, width);
          pos.indent(mp.minimalIndent);
        }
        else {
          if (mp.alignType == lAlignNext)
            pos.alignNext(label, width, mp.alignBlock);
          else if (mp.alignType == lString) {
            if (stringLabelMap.count(mp.alignWithText) == 0) {
              throw meosException("Don't know how to align with 'X'#" + mp.alignWithText);
            }
            pos.update(stringLabelMap[mp.alignWithText], label, width, mp.alignBlock, true);
          }
          else {
            if (labelMap.count(mp.alignType) == 0) {
              throw meosException("Don't know how to align with 'X'#" + typeToSymbol[mp.alignType]);
            }

            pos.update(labelMap[mp.alignType], label, width, mp.alignBlock, true);
          }

          pos.indent(mp.minimalIndent);
        }
        labelMap[mp.type] = label;
        if (!mp.text.empty())
          stringLabelMap[mp.text] = label;

      }
      pos.newRow();
    }
    pos.newRow();
  }

  bool c = true;
  while (c) {
    c = pos.postAdjust();
  }

  int dx = 0, next_dx = 0;

  if (large == false && par.pageBreak == false) {
    for (size_t j = 0; j<mList.getHead().size(); j++) {
      const vector<MetaListPost> &cline = mList.getHead()[j];
      next_dx = 0;
      for (size_t k = 0; k<cline.size(); k++) {
        const MetaListPost &mp = cline[k];
        if (skip.count(mp.type) == 1)
          continue;
          
        string label = "P" + itos(0*1000 + j*100 + k);
        string text = encode(cline[k].text);
        gdiFonts font = normalText;
        if (mp.type == lCmpName) {
          font = boldLarge;
          text = MakeDash(par.getCustomTitle(text));
        }
        
        if (mp.font != formatIgnore)
          font = mp.font;

        li.addHead(oPrintPost(cline[k].type, text, font, 
                              pos.get(label), dx, cline[k].leg)).
                              setFontFace(fontFaces[MLHead].first,
                                          fontFaces[MLHead].second);

        next_dx = max(next_dx, fontHeight[make_pair(font, MLHead)]);
      }
      dx += next_dx;
      next_dx = lineHeight;
    }
  }

  dx = lineHeight;
  for (size_t j = 0; j<mList.getSubHead().size(); j++) {
    next_dx = 0;
    const vector<MetaListPost> &cline = mList.getSubHead()[j];
    for (size_t k = 0; k<cline.size(); k++) {
      const MetaListPost &mp = cline[k];
      if (skip.count(mp.type) == 1)
        continue;
        
      string label = "P" + itos(1*1000 + j*100 + k);
      gdiFonts font = header;
      if (mp.font != formatIgnore)
        font = mp.font;

      li.addSubHead(oPrintPost(cline[k].type, encode(cline[k].text), font, 
                                pos.get(label, s), dx, cline[k].leg)).
                                setFontFace(fontFaces[MLSubHead].first,
                                            fontFaces[MLSubHead].second);

      next_dx = max(next_dx, fontHeight[make_pair(font, MLSubHead)]);
    }
    dx += next_dx;
  }

  int sub_dy = mList.getSubList().size() > 0 ? lineHeight/2 : 0;
  dx = 0;
  for (size_t j = 0; j<mList.getList().size(); j++) {
    const vector<MetaListPost> &cline = mList.getList()[j];
    next_dx = 0;
    for (size_t k = 0; k<cline.size(); k++) {
      const MetaListPost &mp = cline[k];
      if (skip.count(mp.type) == 1)
        continue;
        
      string label = "P" + itos(2*1000 + j*100 + k);
      gdiFonts font = normal;
      if (mp.font != formatIgnore)
        font = mp.font;

      next_dx = max(next_dx, fontHeight[make_pair(font, MLList)]);
      li.addListPost(oPrintPost(cline[k].type, encode(cline[k].text), font, 
                                pos.get(label, s),
                                lineHeight*j + sub_dy, cline[k].leg)).
                                setFontFace(fontFaces[MLList].first,
                                            fontFaces[MLList].second);
    }
    dx += next_dx;
  }

  dx = 0;
  for (size_t j = 0; j<mList.getSubList().size(); j++) {
    const vector<MetaListPost> &cline = mList.getSubList()[j];
    next_dx = 0;
    for (size_t k = 0; k<cline.size(); k++) {
      const MetaListPost &mp = cline[k];
      if (skip.count(mp.type) == 1)
        continue;
      gdiFonts font = small;
      if (mp.font != formatIgnore)
        font = mp.font;

      string label = "P" + itos(3*1000 + j*100 + k);
      next_dx = max(next_dx, fontHeight[make_pair(font, MLSubList)]);
      li.addSubListPost(oPrintPost(mp.type, encode(mp.text), font, 
                                pos.get(label, s), lineHeight*j, mp.leg)).
                                setFontFace(fontFaces[MLSubList].first,
                                            fontFaces[MLSubList].second);

      
      if (mp.alignBlock) {
        int width = li.getMaxCharWidth(oe, mp.type, encode(mp.text), font,
                                       oPrintPost::encodeFont(fontFaces[MLSubList].first,
                                                              fontFaces[MLSubList].second).c_str(),
                                       large, mp.blockWidth);
  
        li.subListPost.back().fixedWidth = width;
      }
    }
    dx += next_dx;
  }

  li.listType = listType;
  li.listSubType = listSubType;
  li.sortOrder = sortOrder;
  for (set<EFilterList>::const_iterator it = filter.begin(); 
                                      it != filter.end(); ++it) {
    li.setFilter(*it);
  }
}

#endif

void Position::indent(int ind) {
  int end = pos.size() - 1;
  if (end < 1)
    return;
  if (pos[end-1].first < ind) {
    int dx = ind - pos[end-1].first;
    pos[end-1].first += dx;
    pos[end].first += dx;
  }
}

void Position::newRow() {
  if (!pos.empty())
    pos.pop_back();
  pos.push_back(PosInfo(0, 0));
}

void Position::alignNext(const string &newname, const int width, bool alignBlock) {
  if (pos.empty())
    return;
  int p = pos.empty() ? 0 : pos.back().first;
  const int backAlign = 20;
  const int fwdAlign = max(width/2, 40);

  if (p==0)
    p = backAlign;

  int next = 0, prev = 0;
  int next_p = 100000, prev_p = -100000;

  int last = pos.size()-1;
  for (int k = pos.size()-2; k >= 0; k--) {
    last = k;
    if (pos[k+1].first < pos[k].first)
      break;
  }

  for (int k = 0; k <= last; k++) {
    if ( pos[k].first >= p && pos[k].first < next_p ) {
      next = k;
      next_p = pos[k].first;
    }

    if ( pos[k].first < p && pos[k].first > prev_p ) {
      prev = k;
      prev_p = pos[k].first;
    }
  }

  if ( p - prev_p < backAlign) {
    int delta = p - prev_p;
    for (size_t k = 0; k + 1 < pos.size(); k++) {
      if (pos[k].first >= prev_p)
        pos[k].first += delta;
    }
    update(prev, newname, width, alignBlock, false);
  }
  else {
    if (next > 0 && (next_p - p) < fwdAlign)
      update(next, newname, width, alignBlock, false);
    else
      add(newname, width, width);
  }
}

void Position::update(const string &oldname, const string &newname, 
                      const int width, bool alignBlock, bool alignLock) {
  if (pmap.count(oldname) == 0)
    throw std::exception("Invalid position");

  int ix = pmap[oldname];
  
  update(ix, newname, width, alignBlock, alignLock);
}

void Position::update(int ix, const string &newname, const int width,
                      bool alignBlock, bool alignLock) {
  
  int last = pos.size()-1;

  if (alignLock) {
    pos[last].aligned = true;
    pos[ix].aligned = true;
  }
  int xlimit = pos[ix].first;
  if (xlimit < pos[last].first) {
    int delta = pos[last].first - xlimit;
    
    // Find last entry to update (higher row)
    int lastud = last;
    while (lastud>1 && pos[lastud].first >= pos[lastud-1].first)
      lastud--;

    for (int k = 0; k<lastud; k++) {
      if (pos[k].first >= xlimit)
        pos[k].first += delta;
    }
  }
  else
    pos[last].first = pos[ix].first;

  int ow = pos[ix+1].first - pos[ix].first;
  int nw = width;
  if (alignBlock && ow>0) {
    nw = max(ow, nw);
    if (nw > ow) {
      int delta = nw - ow;
      for (size_t k = 0; k<pos.size(); k++) {
        if (pos[k].first > pos[ix].first)
          pos[k].first += delta;
      }
    }
  }
  add(newname, nw, width);
}
  
void Position::add(const string &name, int width, int blockWidth) {
  
  if (pos.empty()) {
    pos.push_back(PosInfo(0, blockWidth));
    pmap[name] = 0;
    pos.push_back(PosInfo(width, 0));
  }
  else {
    pmap[name] = pos.size() - 1;
    pos.back().width = blockWidth;
    pos.push_back(PosInfo(width + pos.back().first, 0));
  }
}

int Position::get(const string &name) {
  return pos[pmap[name]].first;
}

int Position::get(const string &name, double scale) {
  return int(pos[pmap[name]].first * scale);
}

struct PosOrderIndex {
  int pos;
  int index;
  int width;
  int row;
  bool aligned;
  bool operator<(const PosOrderIndex &x) const {return pos < x.pos;}
};

bool Position::postAdjust() {
  
  vector<PosOrderIndex> x;
  int row = 0;
  vector<int> aligned(1, -1);

  for (size_t k = 0; k < pos.size(); k++) {
    PosOrderIndex poi;
    poi.pos = pos[k].first;
    poi.index = k;
    poi.row = row;
    if (pos[k].aligned)
      aligned[row] = k;
    if (k + 1 == pos.size() || pos[k+1].first == 0) {
      poi.width = 100000;
      row++;
      aligned.push_back(-1);
    }
    else
      poi.width = pos[k].width;//pos[k+1].first - pos[k].first;//XXX//

    x.push_back(poi);
  }

  sort(x.begin(), x.end());

  // Transfer aligned blocks to x
  for (size_t k = 0; k<x.size(); k++) {
    int r = x[k].row;
    if (r!= -1 && aligned[r]>=x[k].index)
      x[k].aligned = true;
    else
      x[k].aligned = false;
  }

  pair<int, int> smallDiff(100000, -1);
  for (size_t k = 0; k<x.size(); k++) {
    if (k>0 && x[k-1].pos == x[k].pos || x[k].pos == 0)
      continue;
    int diff = 0;
    for (size_t j = k + 1; j < x.size(); j++) {
      if (x[j].pos > x[k].pos) {
        if (x[j].row == x[k].row)
          break;
        else {
          diff = x[j].pos - x[k].pos;

          
          bool skipRow = x[j].aligned || x[k].aligned;
          
          if (skipRow)
            break;
          
          for (size_t i = 0; i<x.size(); i++) {
            if (x[i].pos == x[k].pos && x[i].row == x[j].row) {
              skipRow = true;
              break;
            }
          }

          if (skipRow || diff > x[k].width / 2)
            break;

          if (diff < smallDiff.first) {
            smallDiff.first = diff;
            smallDiff.second = k;
          }
        }
      }
    }
  }
  
  if (smallDiff.second != -1) {
    int minK = smallDiff.second;
    int diff = smallDiff.first;
    int basePos = x[minK].pos;
    set<int> keepRow;
    for (size_t k = 0; k<x.size(); k++) {
      if (x[k].pos == basePos + diff) {
        keepRow.insert(x[k].row);
      }
    }
    bool changed = false;
    assert(keepRow.count(x[minK].row) == 0);
    for (size_t k = 0; k<x.size(); k++) {
      if (x[k].pos >= basePos && keepRow.count(x[k].row) == 0) {
        pos[x[k].index].first += diff;
        changed = true;
      }
    }

    return changed;
  }
  return false;
}

void MetaList::save(const string &file) const {
  xmlparser xml;
  xml.openOutput(file.c_str(), true);
  save(xml);
  xml.closeOut();
}

void MetaList::save(xmlparser &xml) const {
  initSymbols();
  xml.startTag("MeOSListDefinition");
//  xml.write("Title", defaultTitle);
  xml.write("ListName", listName);
  xml.write("Tag", tag);
  xml.write("UID", getUniqueId());
  xml.write("SortOrder", orderToSymbol[sortOrder]);
  xml.write("ListType", baseTypeToSymbol[listType]);
  xml.write("SubListType", baseTypeToSymbol[listSubType]);

  //xml.writeBool("CalculateResults", calcResults);
  //xml.writeBool("Rogaining", rogainingResults);

  for (set<EFilterList>::const_iterator it = filter.begin(); it != filter.end(); ++it)
    xml.write("Filter", "name", filterToSymbol[*it]);

  xml.write("HeadFont", "scale", itos(fontFaces[MLHead].second), fontFaces[MLHead].first);
  xml.write("SubHeadFont", "scale", itos(fontFaces[MLSubHead].second), fontFaces[MLSubHead].first);
  xml.write("ListFont", "scale", itos(fontFaces[MLList].second), fontFaces[MLList].first);
  xml.write("SubListFont", "scale", itos(fontFaces[MLSubList].second), fontFaces[MLSubList].first);
  
  serialize(xml, "Head", getHead());
  serialize(xml, "SubHead", getSubHead());
  serialize(xml, "List", getList());
  serialize(xml, "SubList", getSubList());
  
  xml.endTag();
}

void MetaList::load(const string &file) {
  xmlparser xml;
  xml.read(file.c_str());
  filter.clear();
  xmlobject xDef = xml.getObject("MeOSListDefinition");
  load(xDef);
}

void MetaList::load(const xmlobject &xDef) {
  if (!xDef)
    throw meosException("Ogiltigt filformat");

  xDef.getObjectString("ListName", listName);
  xDef.getObjectString("Tag", tag);
  xDef.getObjectString("UID", uniqueIndex);
  
  string tmp;
  xDef.getObjectString("SortOrder", tmp);
  if (symbolToOrder.count(tmp) == 0) {
    string err = "Invalid sort order X#" + tmp;
    throw std::exception(err.c_str());
  }
  sortOrder = symbolToOrder[tmp];

  xDef.getObjectString("ListType", tmp);
  if (symbolToBaseType.count(tmp) == 0) {
    string err = "Invalid type X#" + tmp;
    throw std::exception(err.c_str());
  }
  listType = symbolToBaseType[tmp];

  xDef.getObjectString("SubListType", tmp);
  if (!tmp.empty()) {
    if (symbolToBaseType.count(tmp) == 0) {
      string err = "Invalid type X#" + tmp;
      throw std::exception(err.c_str());
    }
    
    listSubType = symbolToBaseType[tmp];
  }
  xmlobject xHeadFont = xDef.getObject("HeadFont");
  xmlobject xSubHeadFont = xDef.getObject("SubHeadFont");
  xmlobject xListFont = xDef.getObject("ListFont");
  xmlobject xSubListFont = xDef.getObject("SubListFont");
  
  if (xHeadFont) {
    fontFaces[MLHead].first = xHeadFont.get();
    fontFaces[MLHead].second = xHeadFont.getObjectInt("scale");
  }

  if (xSubHeadFont) {
    fontFaces[MLSubHead].first = xSubHeadFont.get();
    fontFaces[MLSubHead].second = xSubHeadFont.getObjectInt("scale");
  }

  if (xListFont) {
    fontFaces[MLList].first = xListFont.get();
    fontFaces[MLList].second = xListFont.getObjectInt("scale");
  }

  if (xSubListFont) {
    fontFaces[MLSubList].first = xSubListFont.get();
    fontFaces[MLSubList].second = xSubListFont.getObjectInt("scale");
  }

  xmlobject xHead = xDef.getObject("Head");
  xmlobject xSubHead = xDef.getObject("SubHead");
  xmlobject xList = xDef.getObject("List");
  xmlobject xSubList = xDef.getObject("SubList");

  deserialize(xHead, data[MLHead]);
  deserialize(xSubHead, data[MLSubHead]);
  deserialize(xList, data[MLList]);
  deserialize(xSubList, data[MLSubList]);

  // Check if result list
  for (int i = 0; i<4; i++) {
    const vector< vector<MetaListPost> > &lines = data[i];
    for (size_t j = 0; j<lines.size(); j++) {
      const vector<MetaListPost> &cline = lines[j];
      for (size_t k = 0; k<cline.size(); k++) {
        const MetaListPost &mp = cline[k];

        if (mp.type == lTeamPlace || mp.type == lRunnerPlace) {
          hasResults_ = true;
        }
        else if (mp.type == lRunnerTotalPlace) {
          hasResults_ = true;
        }
        else if (mp.type == lRunnerRogainingPoint) {
          hasResults_ = true;
        }
      }
    }
  }

  xmlList f;
  xDef.getObjects("Filter", f);

  for (size_t k = 0; k<f.size(); k++) {
    string attrib = f[k].getAttrib("name").get();
    if (symbolToFilter.count(attrib) == 0) {
      string err = "Invalid filter X#" + attrib; 
      throw std::exception(err.c_str());
    }
    addFilter(symbolToFilter[attrib]);
  }
}

void MetaList::serialize(xmlparser &xml, const string &tagp, 
                         const vector< vector<MetaListPost> > &lp) const {
  xml.startTag(tagp.c_str());

  for (size_t k = 0; k<lp.size(); k++) {
    xml.startTag("Line");
    for (size_t j = 0; j<lp[k].size(); j++) {
      lp[k][j].serialize(xml);
    }
    xml.endTag();
  }

  xml.endTag();
}

void MetaList::deserialize(const xmlobject &xml, vector< vector<MetaListPost> > &lp) {
  if (!xml)
    throw meosException("Ogiltigt filformat");

  
  xmlList xLines;
  xml.getObjects(xLines);

  for (size_t k = 0; k<xLines.size(); k++) {
    lp.push_back(vector<MetaListPost>());
    xmlList xBlocks;
    xLines[k].getObjects(xBlocks);
    for (size_t j = 0; j<xBlocks.size(); j++) {
      lp[k].push_back(MetaListPost(lNone));
      lp[k].back().deserialize(xBlocks[j]);
    }
  }
}

const string &MetaListPost::getType() const {
  return MetaList::typeToSymbol[type];
}

void MetaListPost::getTypes(vector< pair<string, size_t> > &types, int &currentType) const {
  currentType = type;
  types.clear();
  types.reserve(MetaList::typeToSymbol.size());
  for (map<EPostType, string>::const_iterator it =   
    MetaList::typeToSymbol.begin(); it != MetaList::typeToSymbol.end(); ++it) {
      if (it->first == lNone)
        continue;
      if (it->first == lAlignNext)
        continue;

      types.push_back(make_pair(lang.tl(it->second), it->first));
  }
}

const string &MetaListPost::getFont() const {
  return MetaList::fontToSymbol[font];
}
  
void MetaListPost::getFonts(vector< pair<string, size_t> > &fonts, int &currentFont) const {
  currentFont = font;
  fonts.clear();
  fonts.reserve(MetaList::fontToSymbol.size());
  for (map<gdiFonts, string>::const_iterator it =   
    MetaList::fontToSymbol.begin(); it != MetaList::fontToSymbol.end(); ++it) {
      fonts.push_back(make_pair(lang.tl(it->second), it->first));
  }
}

void MetaList::getAlignTypes(const MetaListPost &mlp, vector< pair<string, size_t> > &types, int &currentType) const {
  currentType = mlp.alignType;
  types.clear();
  int gix, lix, ix;
  getIndex(mlp, gix, lix, ix);
  set<EPostType> atypes;
  bool q = false;
  for (size_t k = 0; k < data.size(); k++) {
    for (size_t j = 0; j < data[k].size(); j++) {
      if ( k == gix && j == lix) {
        q = true;
        break;
      }
      for (size_t i = 0; i < data[k][j].size(); i++) {
        atypes.insert(data[k][j][i].type);
      }
    }

    if (q)
      break;
  }
  atypes.insert(EPostType(currentType));
  atypes.insert(lNone);

  for (set<EPostType>::iterator it = atypes.begin(); it != atypes.end(); ++it) {
    types.push_back(make_pair(lang.tl(typeToSymbol[*it]), *it));
  }
}

void MetaList::getIndex(const MetaListPost &mlp, int &gix, int &lix, int &ix) const {
  for (size_t k = 0; k < data.size(); k++) {
    for (size_t j = 0; j < data[k].size(); j++) {
      for (size_t i = 0; i < data[k][j].size(); i++) {
        if (&data[k][j][i] == &mlp) {
          gix = k, lix = j, ix = i;
          return;
        }
      }
    }
  }
  throw meosException("Invalid object");
}
void MetaListPost::serialize(xmlparser &xml) const {
  xml.startTag("Block", "Type", MetaList::typeToSymbol[type]);
  xml.write("Text", text);
  if (leg != -1)
    xml.write("Leg", itos(leg));
  if (alignType == lString)
    xml.write("Align", "BlockAlign", alignBlock, alignWithText);
  else
    xml.write("Align", "BlockAlign", alignBlock, MetaList::typeToSymbol[alignType]);
  xml.write("BlockWidth", blockWidth);
  xml.write("IndentMin", minimalIndent);
  if (font != formatIgnore)
    xml.write("Font", getFont());
  xml.endTag();
}

void MetaListPost::deserialize(const xmlobject &xml) {
  if (!xml)
    throw meosException("Ogiltigt filformat");

  string tp = xml.getAttrib("Type").get();
  if (MetaList::symbolToType.count(tp) == 0) {
    string err = "Invalid type X#" + tp; 
    throw std::exception(err.c_str());
  }

  type = MetaList::symbolToType[tp];
  xml.getObjectString("Text", text);
  if (xml.getObject("Leg"))
    leg = xml.getObjectInt("Leg");
  else
    leg = -1;
  xmlobject xAlignBlock = xml.getObject("Align");
  alignBlock = xAlignBlock && xAlignBlock.getObjectBool("BlockAlign"); 
  blockWidth = xml.getObjectInt("BlockWidth");
  minimalIndent = xml.getObjectInt("IndentMin");
  string at;
  xml.getObjectString("Align", at);

  if (!at.empty()) {
    if (MetaList::symbolToType.count(at) == 0) {
      //string err = "Invalid align type X#" + at; 
      //throw std::exception(err.c_str());
      alignType = lString;
      alignWithText = at;
    }
    else alignType = MetaList::symbolToType[at];
  }
  string f;
  xml.getObjectString("Font", f);
  if (!f.empty()) {
    if (MetaList::symbolToFont.count(f) == 0) {
      string err = "Invalid font X#" + f; 
      throw meosException(err);
    }
    else font = MetaList::symbolToFont[f];
  }
}

map<EPostType, string> MetaList::typeToSymbol;
map<string, EPostType> MetaList::symbolToType;
map<oListInfo::EBaseType, string> MetaList::baseTypeToSymbol;
map<string, oListInfo::EBaseType> MetaList::symbolToBaseType;
map<SortOrder, string> MetaList::orderToSymbol;
map<string, SortOrder> MetaList::symbolToOrder;
map<EFilterList, string> MetaList::filterToSymbol;
map<string, EFilterList> MetaList::symbolToFilter;
map<gdiFonts, string> MetaList::fontToSymbol;
map<string, gdiFonts> MetaList::symbolToFont;

void MetaList::initSymbols() {
  if (typeToSymbol.empty()) {
    typeToSymbol[lAlignNext] = "AlignNext";
    typeToSymbol[lNone] = "None";
    typeToSymbol[lString] = "String";
    typeToSymbol[lCmpName] = "CmpName";
    typeToSymbol[lCmpDate] = "CmpDate";
    typeToSymbol[lCurrentTime] = "CurrentTime";
    typeToSymbol[lClubName] = "ClubName";
    typeToSymbol[lClassName] = "ClassName";
    typeToSymbol[lClassStartName] = "ClassStartName";
    typeToSymbol[lClassStartTime] = "StartTimeForClass";
    typeToSymbol[lClassLength] = "ClassLength";
    typeToSymbol[lClassResultFraction] = "ClassResultFraction";
    typeToSymbol[lCourseLength] = "CourseLength";
    typeToSymbol[lCourseName] = "CourseName";
    typeToSymbol[lCourseClimb] = "CourseClimb";
    typeToSymbol[lRunnerName] = "RunnerName";
    typeToSymbol[lRunnerGivenName] = "RunnerGivenName";
    typeToSymbol[lRunnerFamilyName] = "RunnerFamilyName";
    typeToSymbol[lRunnerCompleteName] = "RunnerCompleteName";
    typeToSymbol[lPatrolNameNames] = "PatrolNameNames";
    typeToSymbol[lPatrolClubNameNames] = "PatrolClubNameNames";
    typeToSymbol[lRunnerFinish] = "RunnerFinish";
    typeToSymbol[lRunnerTime] = "RunnerTime";
    typeToSymbol[lRunnerTimeStatus] = "RunnerTimeStatus";
    typeToSymbol[lRunnerTempTimeStatus] = "RunnerTempTimeStatus";
    typeToSymbol[lRunnerTempTimeAfter] = "RunnerTempTimeAfter";
    typeToSymbol[lRunnerTimeAfter] = "RunnerTimeAfter";
    typeToSymbol[lRunnerClassCourseTimeAfter] = "RunnerClassCourseTimeAfter";
    typeToSymbol[lRunnerMissedTime] = "RunnerTimeLost";
    typeToSymbol[lRunnerPlace] = "RunnerPlace";
    typeToSymbol[lRunnerClassCoursePlace] = "RunnerClassCoursePlace";
    typeToSymbol[lRunnerStart] = "RunnerStart";
    typeToSymbol[lRunnerClub] = "RunnerClub";
    typeToSymbol[lRunnerCard] = "RunnerCard";
    typeToSymbol[lRunnerBib] = "RunnerBib";
    typeToSymbol[lRunnerStartNo] = "RunnerStartNo";
    typeToSymbol[lRunnerRank] = "RunnerRank";
    typeToSymbol[lRunnerCourse] = "RunnerCourse";
    typeToSymbol[lRunnerRogainingPoint] = "RunnerRogainingPoint";
    typeToSymbol[lRunnerPenaltyPoint] = "RunnerPenaltyPoint";
    typeToSymbol[lRunnerUMMasterPoint] = "RunnerUMMasterPoint";
    typeToSymbol[lRunnerTimePlaceFixed] = "RunnerTimePlaceFixed";
    typeToSymbol[lRunnerLegNumberAlpha] = "RunnerLegNumberAlpha";
    typeToSymbol[lTeamName] = "TeamName";
    typeToSymbol[lTeamStart] = "TeamStart";
    typeToSymbol[lTeamTimeStatus] = "TeamTimeStatus";
    typeToSymbol[lTeamTimeAfter] = "TeamTimeAfter";
    typeToSymbol[lTeamPlace] = "TeamPlace";
    typeToSymbol[lTeamLegTimeStatus] = "TeamLegTimeStatus";
    typeToSymbol[lTeamLegTimeAfter] = "TeamLegTimeAfter";
    typeToSymbol[lTeamRogainingPoint] = "TeamRogainingPoint";
    typeToSymbol[lTeamTime] = "TeamTime";
    typeToSymbol[lTeamStatus] = "TeamStatus";
    typeToSymbol[lTeamClub] = "TeamClub";
    typeToSymbol[lTeamRunner] = "TeamRunner";
    typeToSymbol[lTeamRunnerCard] = "TeamRunnerCard";
    typeToSymbol[lTeamBib] = "TeamBib";
    typeToSymbol[lTeamStartNo] = "TeamStartNo";
    typeToSymbol[lPunchNamedTime] = "PunchNamedTime";
    typeToSymbol[lPunchTime] = "PunchTime";
    typeToSymbol[lRogainingPunch] = "RogainingPunch";
    typeToSymbol[lTotalCounter] = "TotalCounter";
    typeToSymbol[lSubCounter] = "SubCounter";
    typeToSymbol[lSubSubCounter] = "SubSubCounter";

    typeToSymbol[lRunnerTotalTime] = "RunnerTotalTime";
    typeToSymbol[lRunnerTimePerKM] = "RunnerTimePerKM";
    typeToSymbol[lRunnerTotalTimeStatus] = "RunnerTotalTimeStatus";
    typeToSymbol[lRunnerTotalPlace] = "RunnerTotalPlace";
    typeToSymbol[lRunnerTotalTimeAfter] = "RunnerTotalTimeAfter";
    typeToSymbol[lRunnerTimeAfterDiff] = "RunnerTimeAfterDiff";
    typeToSymbol[lRunnerPlaceDiff] = "RunnerPlaceDiff";

    for (map<EPostType, string>::iterator it = typeToSymbol.begin();
      it != typeToSymbol.end(); ++it) {
      symbolToType[it->second] = it->first;
    }

    if (typeToSymbol.size() != lLastItem)
      throw std::exception("Bad symbol setup");

    if (symbolToType.size() != lLastItem)
      throw std::exception("Bad symbol setup");

    baseTypeToSymbol[oListInfo::EBaseTypeRunner] = "Runner";
    baseTypeToSymbol[oListInfo::EBaseTypeTeam] = "Team";
    baseTypeToSymbol[oListInfo::EBaseTypeClub] = "ClubRunner";
    baseTypeToSymbol[oListInfo::EBaseTypePunches] = "Punches";
    baseTypeToSymbol[oListInfo::EBaseTypeNone] = "None";

    for (map<oListInfo::EBaseType, string>::iterator it = baseTypeToSymbol.begin();
      it != baseTypeToSymbol.end(); ++it) {
      symbolToBaseType[it->second] = it->first;
    }

    if (baseTypeToSymbol.size() != oListInfo::EBasedTypeLast_)
      throw std::exception("Bad symbol setup");

    if (symbolToBaseType.size() != oListInfo::EBasedTypeLast_)
      throw std::exception("Bad symbol setup");

    orderToSymbol[ClassStartTime] = "ClassStartTime";
    orderToSymbol[ClassStartTimeClub] = "ClassStartTimeClub";
    orderToSymbol[ClassResult] = "ClassResult";
    orderToSymbol[ClassCourseResult] = "ClassCourseResult";
    orderToSymbol[SortByName] = "SortNameOnly";
    orderToSymbol[SortByFinishTime] = "FinishTime";
    orderToSymbol[ClassFinishTime] = "ClassFinishTime";
    orderToSymbol[SortByStartTime] = "StartTime";
    orderToSymbol[ClassPoints] = "ClassPoints";
    orderToSymbol[ClassTotalResult] = "ClassTotalResult";
    orderToSymbol[CourseResult] = "CourseResult";
    orderToSymbol[ClassTeamLeg] = "ClassTeamLeg";
		orderToSymbol[CoursePoints] = "CoursePoints";
   
    for (map<SortOrder, string>::iterator it = orderToSymbol.begin();
      it != orderToSymbol.end(); ++it) {
      symbolToOrder[it->second] = it->first;
    }

    if (orderToSymbol.size() != SortEnumLastItem)
      throw std::exception("Bad symbol setup");

    if (symbolToOrder.size() != SortEnumLastItem)
      throw std::exception("Bad symbol setup");

    filterToSymbol[EFilterHasResult] = "FilterResult";
    filterToSymbol[EFilterRentCard] = "FilterRentCard";
    filterToSymbol[EFilterHasCard] = "FilterHasCard";
    filterToSymbol[EFilterExcludeDNS] = "FilterStarted";
    filterToSymbol[EFilterVacant] = "FilterNotVacant";
    filterToSymbol[EFilterOnlyVacant] = "FilterOnlyVacant";
    filterToSymbol[EFilterHasNoCard] = "FilterNoCard";

    for (map<EFilterList, string>::iterator it = filterToSymbol.begin();
      it != filterToSymbol.end(); ++it) {
      symbolToFilter[it->second] = it->first;
    }

    if (filterToSymbol.size() != _EFilterMax)
      throw std::exception("Bad symbol setup");

    if (symbolToFilter.size() != _EFilterMax)
      throw std::exception("Bad symbol setup");

    fontToSymbol[normalText] = "NormalFont";
    fontToSymbol[boldText] = "Bold";
    fontToSymbol[boldLarge] = "BoldLarge";
    fontToSymbol[boldHuge] = "BoldHuge";
    fontToSymbol[boldSmall] = "BoldSmall";
    fontToSymbol[fontLarge] = "LargeFont";
    fontToSymbol[fontMedium] = "MediumFont";
    fontToSymbol[fontMediumPlus] = "MediumPlus";
    fontToSymbol[fontSmall] = "SmallFont";
    fontToSymbol[italicSmall] = "SmallItalic";
    fontToSymbol[italicText] = "Italic";
    fontToSymbol[italicMediumPlus] = "ItalicMediumPlus";

    fontToSymbol[formatIgnore] = "DefaultFont";

    for (map<gdiFonts, string>::iterator it = fontToSymbol.begin();
      it != fontToSymbol.end(); ++it) {
      symbolToFont[it->second] = it->first;
    }

    if (fontToSymbol.size() != symbolToFont.size())
      throw std::exception("Bad symbol setup");

  }
}

MetaListContainer::MetaListContainer(oEvent *ownerIn): owner(ownerIn) {}

MetaListContainer::~MetaListContainer() {}


const MetaList &MetaListContainer::getList(int index) const {
  return data[index].second;
}

MetaList &MetaListContainer::getList(int index) {
  return data[index].second;
}

MetaList &MetaListContainer::addExternal(const MetaList &ml) {
  data.push_back(make_pair(ExternalList, ml));
  if (owner)
    owner->updateChanged();
  return data.back().second;
}

void MetaListContainer::clearExternal() {
  globalIndex.clear();
  uniqueIndex.clear();
  while(!data.empty() && (data.back().first == ExternalList || data.back().first == RemovedList) )
    data.pop_back();

  listParam.clear();
}

void MetaListContainer::save(MetaListType type, xmlparser &xml) const {
  
  for (size_t k = 0; k<data.size(); k++) {
    if (data[k].first == type)
      data[k].second.save(xml);
  }
  if (type == ExternalList) {
    setupIndex(EFirstLoadedList);
    for (map<int, oListParam>::const_iterator it = listParam.begin(); it != listParam.end(); ++it) {
      it->second.serialize(xml, *this);
    }
  }
}

void MetaListContainer::load(MetaListType type, const xmlobject &xDef) {
  xmlList xList;
  xDef.getObjects("MeOSListDefinition", xList);
  
  if (xList.empty() && strcmp(xDef.getName(), "MeOSListDefinition") == 0)
    xList.push_back(xDef);
  
  for (size_t k = 0; k<xList.size(); k++) {
    data.push_back(make_pair(type, MetaList()));
    data.back().second.load(xList[k]);
  }

  setupIndex(EFirstLoadedList);

  xmlList xParam;
  xDef.getObjects("ListParam", xParam);
  
  for (size_t k = 0; k<xParam.size(); k++) {
    listParam[k].deserialize(xParam[k], *this);
  }
}

bool MetaList::isValidIx(size_t gIx, size_t lIx, size_t ix) const {
  return gIx < data.size() && lIx < data[gIx].size() && (ix == -1 || ix < data[gIx][lIx].size());
}

MetaListPost &MetaList::addNew(int groupIx, int lineIx, int &ix) {
  if (isValidIx(groupIx, lineIx, -1)) {
    ix = data[groupIx][lineIx].size();
    data[groupIx][lineIx].push_back(MetaListPost(lString));
    return data[groupIx][lineIx].back();
  }
  else if (lineIx == -1 && size_t(groupIx) < data.size()) {
    lineIx = data[groupIx].size();
    addRow(groupIx);
    return addNew(groupIx, lineIx, ix);
  }
  throw meosException("Invalid index");
}

MetaListPost &MetaList::getMLP(int groupIx, int lineIx, int ix) {
  if (isValidIx(groupIx, lineIx, ix)) {
    return data[groupIx][lineIx][ix];
  }
  else
    throw meosException("Invalid index");
}

void MetaList::removeMLP(int groupIx, int lineIx, int ix) {
  if (isValidIx(groupIx, lineIx, ix)) {
    data[groupIx][lineIx].erase(data[groupIx][lineIx].begin() + ix);
    if (data[groupIx][lineIx].empty() && data[groupIx].size() == lineIx + 1)
      data[groupIx].pop_back();
  }
  else
    throw meosException("Invalid index");
}

void MetaList::moveOnRow(int groupIx, int lineIx, int &ix, int delta) {
  if (isValidIx(groupIx, lineIx, ix)) {
    if (ix > 0 && delta == -1) {
      ix--;
      swap(data[groupIx][lineIx][ix], data[groupIx][lineIx][ix+1]);
    }
    else if (delta == 1 && size_t(ix + 1) < data[groupIx][lineIx].size()) {
      ix++;
      swap(data[groupIx][lineIx][ix], data[groupIx][lineIx][ix-1]);
    }
  }
  else
    throw meosException("Invalid index");

}
  
string MetaListContainer::getUniqueId(EStdListType code) const {
  if (int(code) < int(EFirstLoadedList))
    return "C" + itos(code);
  else {
    size_t ix = int(code) - int(EFirstLoadedList);
    if (ix < data.size()) {
      if (!data[ix].second.getTag().empty())
        return "T" + data[ix].second.getTag(); 
      else
        return data[ix].second.getUniqueId();
    }
    else
      return "C-1";
  }
}

EStdListType MetaListContainer::getCodeFromUnqiueId(const string &id) const {
  if (id[0] == 'C')
    return EStdListType(atoi(id.substr(1).c_str()));
  else if (id[0] == 'T') {
    string tag = id.substr(1);
    map<string, EStdListType>::const_iterator res = tagIndex.find(tag);
    if (res != tagIndex.end())
      return res->second;
    else
      return EStdNone;
  }
  else {
    map<string, EStdListType>::const_iterator res = uniqueIndex.find(id);
    if (res != uniqueIndex.end())
      return res->second;
    else
      return EStdNone;
  }
}

void MetaListContainer::setupIndex(int firstIndex) const {
  globalIndex.clear();
  uniqueIndex.clear();
  for (size_t k = 0; k<data.size(); k++) {
    if (data[k].first == RemovedList)
      continue;
    const MetaList &ml = data[k].second;
    EStdListType listIx = EStdListType(k + firstIndex);
    if (data[k].first == InternalList) {
      const string &tag = data[k].second.getTag();
      if (!tag.empty())
        tagIndex[tag] = listIx;
    }
    
    globalIndex[listIx] = k;

    if (!ml.getUniqueId().empty())
      uniqueIndex[ml.getUniqueId()] = listIx;
  }
}

#ifndef MEOSDB

void MetaListContainer::setupListInfo(int firstIndex, 
                                      map<EStdListType, oListInfo> &listMap, 
                                      bool resultsOnly) const {
  setupIndex(firstIndex);

  for (size_t k = 0; k<data.size(); k++) {
    if (data[k].first == RemovedList)
      continue;
    const MetaList &ml = data[k].second;
    EStdListType listIx = EStdListType(k + firstIndex);
   
    if (!resultsOnly || ml.hasResults()) {
      oListInfo &li = listMap[listIx];
      li.Name = lang.tl(ml.getListName());
      li.listType = ml.getListType();
      li.supportClasses = true;
      li.supportLegs = true;
      li.supportExtra = false;
    }
  }
}

string MetaListContainer::makeUniqueParamName(const string &nameIn) const {
  int maxValue = -1;
  size_t len = nameIn.length();
  for (map<int, oListParam>::const_iterator it = listParam.begin(); it != listParam.end(); ++it) {
    if (it->second.name.length() >= len) {
      if (nameIn == it->second.name)
        maxValue = max(1, maxValue);
      else {
        if (it->second.name.substr(0, len) == nameIn) {
          int v = atoi(it->second.name.substr(len + 1).c_str());
          if (v > 0)
            maxValue = max(v, maxValue);
        }
      }
    }
  }
  if (maxValue == -1)
    return nameIn;
  else
    return nameIn + " " + itos(maxValue + 1);
}


bool MetaListContainer::interpret(oEvent *oe, const gdioutput &gdi, const oListParam &par, 
                                  int lineHeight, oListInfo &li) const {
                                    
  map<EStdListType, int>::const_iterator it = globalIndex.find(par.listCode);
  if (it != globalIndex.end()) {
    data[it->second].second.interpret(oe, gdi, par, lineHeight, li);
    return true;
  }
  return false;
}

EStdListType MetaListContainer::getType(const std::string &tag) const {
  map<string, EStdListType>::iterator it = tagIndex.find(tag);

  if (it == tagIndex.end())
    throw meosException("Could not load list 'X'.#" + tag);

  return it->second;
}

EStdListType MetaListContainer::getType(const int index) const {
  return EStdListType(index + EFirstLoadedList);
}

void MetaListContainer::getLists( vector<pair<string, size_t> > &lists) const {
  lists.clear();
  for (size_t k = 0; k<data.size(); k++) {
    if (data[k].first == RemovedList)
      continue;
    
    if (data[k].first == InternalList)
      lists.push_back( make_pair("[" + lang.tl(data[k].second.getListName()) + "]", k) );
    else
      lists.push_back( make_pair(data[k].second.getListName(), k) );
  }
}

void MetaListContainer::removeList(int index) {
  if (size_t(index) >= data.size())
    throw meosException("Invalid index");

  if (data[index].first != ExternalList)
    throw meosException("Invalid list type");

  data[index].first = RemovedList;
}

void MetaListContainer::saveList(int index, const MetaList &ml) {
  if (size_t(index) >= data.size())
    throw meosException("Invalid index");

  if (data[index].first == InternalList)
    throw meosException("Invalid list type");

  data[index].first = ExternalList;
  data[index].second = ml;
  data[index].second.initUniqueIndex();
  if (owner)
    owner->updateChanged();
}

void MetaList::getFilters(vector< pair<string, bool> > &filters) const {
  filters.clear();
  for (map<EFilterList, string>::const_iterator it = filterToSymbol.begin();
                             it != filterToSymbol.end(); ++it) {
    bool has = this->filter.count(it->first) == 1;
    filters.push_back(make_pair(it->second, has));
  }
}

void MetaList::setFilters(const vector<bool> &filters) {
  int k = 0;
  filter.clear();
  assert(filters.size() == filterToSymbol.size());
  for (map<EFilterList, string>::const_iterator it = filterToSymbol.begin();
                             it != filterToSymbol.end(); ++it) {
    bool has = filters[k++];
    if (has)
      filter.insert(it->first);
  }
}

void MetaList::getSortOrder(vector< pair<string, size_t> > &orders, int &currentOrder) const {
  orders.clear();
  for(map<SortOrder, string>::const_iterator it = orderToSymbol.begin();
    it != orderToSymbol.end(); ++it) {
      orders.push_back(make_pair(lang.tl(it->second), it->first));
  }

  currentOrder = sortOrder;
}

void MetaList::getBaseType(vector< pair<string, size_t> > &types, int &currentType) const {
  types.clear();
  for(map<oListInfo::EBaseType, string>::const_iterator it = baseTypeToSymbol.begin();
    it != baseTypeToSymbol.end(); ++it) {
      if (it->first == oListInfo::EBaseTypeNone || it->first == oListInfo::EBaseTypePunches)
        continue;
      types.push_back(make_pair(lang.tl(it->second), it->first));
  }

  currentType = listType;
}

void MetaList::getSubType(vector< pair<string, size_t> > &types, int &currentType) const {
  types.clear();
   
  oListInfo::EBaseType t;
  set<oListInfo::EBaseType> tt;

  t = oListInfo::EBaseTypeNone;
  tt.insert(t);
  types.push_back(make_pair(lang.tl(baseTypeToSymbol[t]), t));

  t = oListInfo::EBaseTypePunches;
  tt.insert(t);
  types.push_back(make_pair(lang.tl(baseTypeToSymbol[t]), t));

  t = oListInfo::EBaseTypeRunner;
  tt.insert(t);
  types.push_back(make_pair(lang.tl(baseTypeToSymbol[t]), t));

  if (tt.count(listSubType) == 0) {
    t = listSubType;
    types.push_back(make_pair(lang.tl(baseTypeToSymbol[t]), t));
  }

  currentType = listSubType;
}

int MetaListContainer::getNumLists(MetaListType t) const {
  int num = 0;
  for (size_t k = 0; k<data.size(); k++) {
    if (data[k].first == t)
      num++;
  }
  return num;
}

void MetaListContainer::getListParam( vector< pair<string, size_t> > &param) const {
  for (map<int, oListParam>::const_iterator it = listParam.begin(); it != listParam.end(); ++it) {
    param.push_back(make_pair(it->second.getName(), it->first));
  }
}

void MetaListContainer::removeParam(int index) {
  if (listParam.count(index)) {
    listParam.erase(index);
    if (owner)
      owner->updateChanged();
  }
  else
    throw meosException("No such parameters exist");
}

void MetaListContainer::addListParam(oListParam &param) {
  param.saved = true;
  int ix = 0;
  if (!listParam.empty())
    ix = listParam.rbegin()->first + 1;

  listParam[ix] = param;
  if (owner)
    owner->updateChanged();
}


const oListParam &MetaListContainer::getParam(int index) const {
  if (!listParam.count(index))
    throw meosException("Internal error");
  return listParam.find(index)->second;
}

oListParam &MetaListContainer::getParam(int index) {
  if (!listParam.count(index))
    throw meosException("Internal error");

  return listParam.find(index)->second;
}

#endif
