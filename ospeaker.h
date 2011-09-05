#pragma once

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
    Stigbergsv�gen 7, SE-75242 UPPSALA, Sweden
    
************************************************************************/
class oRunner;

class oSpeakerObject
{
public:
  oRunner *owner;
  string name;
  string club;
  string placeS;
  string startTimeS;

  int place;
  RunnerStatus status;
  RunnerStatus finishStatus;
  int runningTime;
  int preliminaryRunningTime;

  int runningTimeLeg;
  int preliminaryRunningTimeLeg;

  bool isRendered;
  int priority;
  bool missingStartTime;
};
