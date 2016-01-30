/********************i****************************************************
    MeOS - Orienteering Software
    Copyright (C) 2009-2015 Melin Software HB

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
#include "oEvent.h"
#include "gdioutput.h"
#include "meos_util.h"
#include <cassert>
#include "Localizer.h"
#include <algorithm>
#include "gdifonts.h"
#include "meosexception.h"
#include "liveresult.h"

LiveResult::LiveResult(oEvent *oe) : oe(oe), active(false), lastTime(0), rToWatch(0) {
  baseFont = oe->getPropertyString("LiveResultFont", "Consolas");
  showResultList = -1;
  timerScale = 1.0;
}

int liveEventCB(gdioutput *gdi, int type, void *data)
{
  BaseInfo *bi = (BaseInfo *) data;
  LiveResult *li = (LiveResult *)bi->getExtra();
  if (!li) 
    throw std::exception("Internal error");
  li->timerEvent(*gdi, type, *bi);
  
  return 0;
}

string LiveResult::getFont(const gdioutput &gdi, double relScale) const {
  int h,w;
  gdi.getTargetDimension(w, h);
  if (!gdi.isFullScreen())
    w -=  gdi.scaleLength(160);

  double fact = min(h/180.0, w/300.0);

  double size = relScale * fact;
  char ss[32];
  sprintf_s(ss, "%f", size);
  string font = baseFont + ";" + ss;
  return font;
}

void LiveResult::showDefaultView(gdioutput &gdi) {
  int h,w;
  gdi.getTargetDimension(w, h);
  if (!gdi.isFullScreen())
    w -=  gdi.scaleLength(160);

  RECT rc;
  rc.top = h-24;
  rc.left = 20;
  rc.right = w - 30;
  rc.bottom = h - 22;
  
  string font = getFont(gdi, 1.0);
  gdi.addRectangle(rc, colorLightYellow, true);
  gdi.addString("timing", 50, w / 2, textCenter|boldHuge, "MeOS Timing", 0, 0, font.c_str());

  TextInfo &ti = gdi.addString("measure", 0, 0, boldHuge,  "55:55:55", 0, 0, font.c_str());
  int tw = ti.textRect.right - ti.textRect.left;
  timerScale = double(w) * 0.8 / double(tw); 
  gdi.removeString("measure");
}

void LiveResult::showTimer(gdioutput &gdi, const oListInfo &liIn) {
  li = liIn;
  active = true;
  
  int h,w;
  gdi.getTargetDimension(w, h);
  gdi.clearPage(false);
  showDefaultView(gdi);

  gdi.registerEvent("DataUpdate", liveEventCB).setExtra(this);
  gdi.setData("DataSync", 1);
  gdi.setData("PunchSync", 1);
  gdi.setRestorePoint("LiveResult");

  lastTime = 0;
  vector<const oFreePunch *> pp;
  oe->synchronizeList(oLRunnerId, true, false);
  oe->synchronizeList(oLPunchId, false, true);
  
  oe->getLatestPunches(lastTime, pp);
  processedPunches.clear();

  map<int, pair<vector<int>, vector<int> > > storedPunches;
  int fromPunch = li.getParam().useControlIdResultFrom;
  int toPunch = li.getParam().useControlIdResultTo;
  if (fromPunch == 0)
    fromPunch = oPunch::PunchStart;
  if (toPunch == 0)
    toPunch = oPunch::PunchFinish;
    
  for (size_t k = 0; k < pp.size(); k++) {
    lastTime = max(pp[k]->getModificationTime(), lastTime);
    pRunner r = pp[k]->getTiedRunner();
    if (r) {
      pair<int, int> key = make_pair(r->getId(), pp[k]->getControlId());
      processedPunches[key] = max(processedPunches[key], pp[k]->getAdjustedTime());
      
      if (!li.getParam().selection.empty() && !li.getParam().selection.count(r->getClassId()))
        continue; // Filter class

      if (pp[k]->getTypeCode() == fromPunch) {
        storedPunches[r->getId()].first.push_back(k);
      }
      else if (pp[k]->getTypeCode() == toPunch) {
        storedPunches[r->getId()].second.push_back(k);
      }
    }
  }
  startFinishTime.clear();
  results.clear();
  for (map<int, pair<vector<int>, vector<int> > >::iterator it = storedPunches.begin();
       it != storedPunches.end(); ++it) {
    vector<int> &froms = it->second.first;
    vector<int> &tos = it->second.second;
    pRunner r = oe->getRunner(it->first, 0);
    for (size_t j = 0; j < tos.size(); j++) {
      int fin = pp[tos[j]]->getAdjustedTime();
      int time = 100000000;
      int sta = 0;
      for (size_t k = 0; k < froms.size(); k++) {
        int t = fin - pp[froms[k]]->getAdjustedTime();
        if (t > 0 && t < time) {
          time = t;
          sta = pp[froms[k]]->getAdjustedTime();
        }
      }
      if (time < 100000000 && r->getStatus() <= StatusOK) {
//        results.push_back(Result());
//        results.back().r = r;
//        results.back().time = time;
        startFinishTime[r->getId()].first = sta;
        startFinishTime[r->getId()].second = fin;

      }
    }
  }

  resYPos = h/3;

  calculateResults();
  showResultList = 0;
  gdi.addTimeoutMilli(1000, "res", liveEventCB).setExtra(this);
  gdi.refreshFast();
}

void LiveResult::timerEvent(gdioutput &gdi, int type, BaseInfo &bi) {
  if (type == GUI_EVENT) {
    //EventInfo &ev = dynamic_cast<EventInfo &>(bi);
    vector<const oFreePunch *> pp;
 
    oe->getLatestPunches(lastTime + 1, pp);
    pRunner newRToWatch = 0;
    const oFreePunch *fp = 0;
    bool exit = false;
    int fromPunch = li.getParam().useControlIdResultFrom;
    int toPunch = li.getParam().useControlIdResultTo;
    if (fromPunch == 0)
      fromPunch = oPunch::PunchStart;
    if (toPunch == 0)
      toPunch = oPunch::PunchFinish;
    
    for (size_t k = 0; k < pp.size(); k++) {
      lastTime = max(pp[k]->getModificationTime(), lastTime);
      pRunner r = pp[k]->getTiedRunner();
      if (!r)
        continue;

      if (!li.getParam().selection.empty() && !li.getParam().selection.count(r->getClassId()))
        continue; // Filter class

      pair<int, int> key = make_pair(r->getId(), pp[k]->getControlId());
      
      bool accept = !processedPunches.count(key) || abs(processedPunches[key] - pp[k]->getAdjustedTime()) > 5;
      processedPunches[key] = pp[k]->getAdjustedTime();

      if (accept) {
        if (pp[k]->getTypeCode() == fromPunch) {
          if (fp == 0 || fp->getAdjustedTime() < pp[k]->getAdjustedTime()) {
            fp = pp[k];
            newRToWatch = r;
          }
        }
        else if (pp[k]->getTypeCode() == toPunch) {
          if (fp == 0 && rToWatch ==r) {
            fp = pp[k];
            exit = true;
            break;
          }
        }
      }
    }
    
    int h,w;
    gdi.getTargetDimension(w, h);
    string font = getFont(gdi, timerScale);

    if (newRToWatch && !exit) {
      showResultList = -1;
      rToWatch = newRToWatch;
      gdi.restore("LiveResult", false);
      BaseInfo *bi = gdi.setText("timing", rToWatch->getName(), false);
      dynamic_cast<TextInfo &>(*bi).changeFont(getFont(gdi, 0.7));
      gdi.addTimer(h/2, w/2, boldHuge|textCenter|timeWithTenth, 0, 0, 0, NOTIMEOUT, font.c_str());
      startFinishTime[rToWatch->getId()].first = fp->getAdjustedTime();
      gdi.refreshFast();
    }
    else if (rToWatch && exit) {
      showResultList = -1;
      pair<int,int> &se = startFinishTime[rToWatch->getId()];
      se.second = fp->getAdjustedTime();
      int rt = se.second - se.first;
      gdi.restore("LiveResult", false);
      gdi.addString("", h/2, w/2, boldHuge|textCenter, formatTime(rt), 0, 0, font.c_str()).setColor(colorGreen);
      rToWatch = 0;
      gdi.refreshFast();
      gdi.addTimeout(5, liveEventCB).setExtra(this);
    }
  }
  else if (type == GUI_TIMEOUT) {
    gdi.restore("LiveResult", false);
    int h,w;
    gdi.getTargetDimension(w, h);
    gdi.fillDown();
    BaseInfo *bi = gdi.setTextTranslate("timing", "MeOS Timing", false);
    TextInfo &ti = dynamic_cast<TextInfo &>(*bi);
    ti.changeFont(getFont(gdi, 0.7));
    gdi.refreshFast();
    resYPos = ti.textRect.bottom + gdi.scaleLength(20);
    calculateResults();
    showResultList = 0;
    gdi.addTimeoutMilli(300, "res", liveEventCB).setExtra(this);
  }
  else if (type == GUI_TIMER) {
    if (size_t(showResultList) >= results.size())
      return;
    Result &res = results[showResultList];
    string font = getFont(gdi, 0.7);
    int y = resYPos;
    if (res.place > 0) {
      int h,w;
      gdi.getTargetDimension(w, h);
   
      gdi.takeShownStringsSnapshot();
      TextInfo &ti = gdi.addStringUT(y, 30, fontLarge, itos(res.place) + ".", 0, 0, font.c_str());
      int ht = ti.textRect.bottom - ti.textRect.top;
      gdi.addStringUT(y, 30 + ht * 2 , fontLarge, res.r->getName(), 0, 0, font.c_str());
      //int w = gdi.getWidth();
      gdi.addStringUT(y, w - 4 * ht, fontLarge, formatTime(res.time), 0, 0, font.c_str());
      gdi.refreshSmartFromSnapshot(false);
      resYPos += int (ht * 1.1);
      showResultList++;
      

      int limit = h - ht * 2;
      //OutputDebugString(("w:" + itos(resYPos) + " " + itos(limit) + "\n").c_str());
      if ( resYPos < limit )
        gdi.addTimeoutMilli(300, "res" + itos(showResultList), liveEventCB).setExtra(this);
    }
  }
}

void LiveResult::calculateResults() {
  results.clear();
  results.reserve(startFinishTime.size());
  const int highTime = 10000000;
  for (map<int, pair<int, int> >::iterator it = startFinishTime.begin();
       it != startFinishTime.end(); ++it) {
    pRunner r = oe->getRunner(it->first, 0);
    if (!r)
      continue;
    results.push_back(Result());
    results.back().r = r;
    results.back().time = it->second.second - it->second.first;
    if (results.back().time <= 0 || r->getStatus() > StatusOK)
      results.back().time = highTime;
  }

  sort(results.begin(), results.end());

  int place = 1;
  for (size_t k = 0; k< results.size(); k++) {
    if (results[k].time < highTime) {
      if (k>0 && results[k-1].time < results[k].time)
        place = k + 1;

      results[k].place = place;
    }
    else {
      results[k].place = 0;
    }
  }

}

