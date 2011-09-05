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
#include "oListInfo.h"
#include "oEvent.h"
#include "gdioutput.h"
#include "meos_util.h"
#include <cassert>
#include <cmath>
#include "Localizer.h"
#include "metalist.h"
#include <algorithm>


MetaList::MetaList() {
  data.resize(4);
}

bool MetaList::isBreak(int x) const {
  return isspace(x) || x == '.' || x == ',' || 
          x == '-'  || x == ':' || x == ';';
}

string MetaList::encode(const string &input) const {
  string out;
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

void MetaList::interpret(const oEvent *oe, const oListParam &par, 
                         int lineHeight, oListInfo &li) const {
  const MetaList &mList = *this;
  Position pos;
  bool large = par.useLargeSize;

  gdiFonts normal, header, small;
  double s;

  if (large) {
    s = 1.8;
    normal = fontLarge;
    header = boldLarge;
    small = normalText;
    lineHeight = int(lineHeight *1.6);
  }
  else {
    s = 1.0;
    normal = normalText;
    header = boldText;
    small = italicSmall;
  }

  map<EPostType, string> labelMap;
  set<EPostType> skip;
  
  for (int i = 0; i<4; i++) {
    const vector< vector<MetaListPost> > &lines = mList.data[i];
    for (size_t j = 0; j<lines.size(); j++) {
      const vector<MetaListPost> &cline = lines[j];
      for (size_t k = 0; k<cline.size(); k++) {
        const MetaListPost &mp = cline[k];
        string label = "P" + itos(i*1000 + j*100 + k);
        
        if (mp.type == lRunnerBib || mp.type == lTeamBib) {
          if (!oe->hasBib(mp.type == lRunnerBib, mp.type == lTeamBib))
            skip.insert(mp.type);
        }

        if (mp.type == lTeamStart || mp.type == lRunnerStart ||
             mp.type == lClassStart) {
          bool hasIndStart = false;
          
          for (set<int>::const_iterator it = par.selection.begin(); it != par.selection.end(); ++it) {
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
          if (mp.type == lClassStart) {
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
          width = li.getMaxCharWidth(oe, mp.type, mp.text, large, mp.blockWidth);
        }
        if (mp.alignType == lNone) {
          pos.add(label, width);
          pos.indent(mp.minimalIndent);
          labelMap[mp.type] = label;
        }
        else {
          if (mp.alignType == lAlignNext)
            pos.alignNext(label, width, mp.alignBlock);
          else
            pos.update(labelMap[mp.alignType], label, width, mp.alignBlock);
          pos.indent(mp.minimalIndent);
          labelMap[mp.type] = label;
        }
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
      for (size_t k = 0; k<cline.size(); k++) {
        const MetaListPost &mp = cline[k];
        if (skip.count(mp.type) == 1)
          continue;
          
        string label = "P" + itos(0*1000 + j*100 + k);
        string text = encode(cline[k].text);
        gdiFonts font = normalText;
        if (mp.type == lCmpName) {
          font = boldLarge;
          next_dx = lineHeight*2;
          text = MakeDash(par.getCustomTitle(text));
        }
        
        li.addHead(oPrintPost(cline[k].type, text, font, 
                              pos.get(label), dx, cline[k].leg));
      }
      dx += next_dx;
      next_dx = lineHeight;
    }
  }

  for (size_t j = 0; j<mList.getSubHead().size(); j++) {
    const vector<MetaListPost> &cline = mList.getSubHead()[j];
    for (size_t k = 0; k<cline.size(); k++) {
      const MetaListPost &mp = cline[k];
      if (skip.count(mp.type) == 1)
        continue;
        
      string label = "P" + itos(1*1000 + j*100 + k);
      li.addSubHead(oPrintPost(cline[k].type, encode(cline[k].text), header, 
                                pos.get(label, s), lineHeight*(j+1), cline[k].leg));
    }
  }

  int sub_dy = mList.getSubList().size() > 0 ? lineHeight/2 : 0;

  for (size_t j = 0; j<mList.getList().size(); j++) {
    const vector<MetaListPost> &cline = mList.getList()[j];
    for (size_t k = 0; k<cline.size(); k++) {
      const MetaListPost &mp = cline[k];
      if (skip.count(mp.type) == 1)
        continue;
        
      string label = "P" + itos(2*1000 + j*100 + k);
      li.addListPost(oPrintPost(cline[k].type, encode(cline[k].text), normal, 
                                pos.get(label, s), lineHeight*j + sub_dy, cline[k].leg));
    }
  }

  for (size_t j = 0; j<mList.getSubList().size(); j++) {
    const vector<MetaListPost> &cline = mList.getSubList()[j];
    for (size_t k = 0; k<cline.size(); k++) {
      const MetaListPost &mp = cline[k];
      if (skip.count(mp.type) == 1)
        continue;
        
      string label = "P" + itos(3*1000 + j*100 + k);
      li.addSubListPost(oPrintPost(cline[k].type, encode(cline[k].text), small, 
                                pos.get(label, s), lineHeight*j, cline[k].leg));
    }
  }
}

void Position::indent(int ind) {
  int end = pos.size() - 1;
  if (end < 1)
    return;
  if (pos[end-1] < ind) {
    int dx = ind - pos[end-1];
    pos[end-1] += dx;
    pos[end] += dx;
  }
}

void Position::newRow() {
  if (!pos.empty())
    pos.pop_back();
  pos.push_back(0);
}

void Position::alignNext(const string &newname, int width, bool alignBlock) {
  if (pos.empty())
    return;
  int p = pos.empty() ? 0 : pos.back();
  const int backAlign = 20;
  const int fwdAlign = max(width/2, 40);

  if (p==0)
    p = backAlign;

  int next = 0, prev = 0;
  int next_p = 100000, prev_p = -100000;

  int last = pos.size()-1;
  for (int k = pos.size()-2; k >= 0; k--) {
    last = k;
    if (pos[k+1] < pos[k])
      break;
  }

  for (int k = 0; k <= last; k++) {
    if ( pos[k] >= p && pos[k] < next_p ) {
      next = k;
      next_p = pos[k];
    }

    if ( pos[k] < p && pos[k] > prev_p ) {
      prev = k;
      prev_p = pos[k];
    }
  }

  if ( p - prev_p < backAlign) {
    int delta = p - prev_p;
    for (size_t k = 0; k + 1 < pos.size(); k++) {
      if (pos[k] >= prev_p)
        pos[k] += delta;
    }
    update(prev, newname, width, alignBlock);
  }
  else {
    if (next > 0 && (next_p - p) < fwdAlign)
      update(next, newname, width, alignBlock);
    else
      add(newname, width);
  }
}

void Position::update(const string &oldname, const string &newname, int width, bool alignBlock) {
  if (pmap.count(oldname) == 0)
    throw std::exception("Invalid position");

  int ix = pmap[oldname];
  
  update(ix, newname, width, alignBlock);
}

void Position::update(int ix, const string &newname, int width, bool alignBlock) {

  int last = pos.size()-1;
  if (pos[ix] < pos[last]) {
    int delta = pos[last] - pos[ix];
    for (int k = 0; k<last; k++) {
      if (pos[k] >= pos[ix])
        pos[k] += delta;
    }
  }
  else
    pos[last] = pos[ix];

  int ow = pos[ix+1] - pos[ix];
  int nw = width;
  if (alignBlock && ow>0) {
    nw = max(ow, nw);
    if (nw > ow) {
      int delta = nw - ow;
      for (size_t k = 0; k<pos.size(); k++) {
        if (pos[k] > pos[ix])
          pos[k] += delta;
      }
    }
  }
  //pos.push_back(pos[ix]);
  add(newname, nw);  
}
  
void Position::add(const string &name, int width) {
  
  if (pos.empty()) {
    pos.push_back(0);
    pmap[name] = 0;
    pos.push_back(width);
  }
  else {
    pmap[name] = pos.size() - 1;
    pos.push_back(width + pos.back());
  }
}

int Position::get(const string &name) {
  return pos[pmap[name]];
}

int Position::get(const string &name, double scale) {
  return int(pos[pmap[name]] * scale);
}

struct PosOrderIndex {
  int pos;
  int index;
  int width;
  int row;
  bool operator<(const PosOrderIndex &x) const {return pos < x.pos;}
};

bool Position::postAdjust() {
  
  vector<PosOrderIndex> x;
  int row = 0;
  for (size_t k = 0; k < pos.size(); k++) {
    PosOrderIndex poi;
    poi.pos = pos[k];
    poi.index = k;
    poi.row = row;
    if (k + 1 == pos.size() || pos[k+1] == 0) {
      poi.width = 100000;
      row++;
    }
    else
      poi.width = pos[k+1] - pos[k];

    x.push_back(poi);
  }

  sort(x.begin(), x.end());

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

          bool skipRow = false;
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
        pos[x[k].index] += diff;
        changed = true;
      }
    }

    return changed;
  }
  return false;
}
