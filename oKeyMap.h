#pragma once
#include <map>
#include "oBase.h"

class oBaseMap
{
protected:
	map<int,pBase> index;
public:
	void insert(oBase *base){index.insert(pair<int, pBase>(base->getId(), base));}
		
		//{ KeyData(base->getId(), base)
	oBaseMap(void);
	~oBaseMap(void);
};
