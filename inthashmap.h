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
    Stigbergsvägen 7, SE-75242 UPPSALA, Sweden

************************************************************************/

#include "intkeymap.hpp"

typedef intkeymap<int> inthashmap;
/*
struct keypair {
  int key;
  int value;
};

class inthashmap {
private:
  keypair *keys;
  unsigned siz;
  unsigned used;
  inthashmap *next;
  inthashmap *parent;
  double allocFactor;

  unsigned hash1;
  unsigned hash2;
  int level;
  int optsize(int arg);

  int noValue;
  int &rehash(int size, int key, int value);
  int &get(int key);

  inthashmap(const inthashmap &co);
  const inthashmap &operator=(const inthashmap &co);

public:
  virtual ~inthashmap();
  inthashmap(int size);
  inthashmap();

  bool empty() const;
  int size() const;
  int getAlloc() const {return siz;}
  void clear();

  void resize(int size);
  
  bool lookup(int key, int &value) const;
  void insert(int key, int value);
  void remove(int key);

  int operator[](int key) const 
    {int v = 0; lookup(key,v); return v;}

  int &operator[](int key) {
    return get(key);
  }
};

*/