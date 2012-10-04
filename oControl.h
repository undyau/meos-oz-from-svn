// oControl.h: interface for the oControl class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_OCONTROL_H__E86192B9_78D2_4EEF_AAE1_3BD4A8EB16F0__INCLUDED_)
#define AFX_OCONTROL_H__E86192B9_78D2_4EEF_AAE1_3BD4A8EB16F0__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#include "xmlparser.h"
#include "oBase.h"
#include <limits>

class oControl;

typedef oControl* pControl;
class oDataInterface;
class oDataConstInterface;
class Table;

class oControl : public oBase 
{
public:
  enum ControlStatus {StatusOK=0, StatusBad=1, StatusMultiple=2, 
                      StatusStart = 4, StatusFinish = 5, StatusRogaining = 6};
  bool operator<(const oControl &b) const {return minNumber()<b.minNumber();}
	
protected:
	int nNumbers;
	int Numbers[32];
  bool checkedNumbers[32];

	ControlStatus Status;
	string Name;
	bool decodeNumbers(string s);
	BYTE oData[32];

  int tMissedTimeTotal;
  int tMissedTimeMax;
  int tNumVisitors;
  int tMissedTimeMedian;

  /// Table methods
  void addTableRow(Table &table) const;
  bool inputData(int id, const string &input, 
                 int inputId, string &output, bool noUpdate);

  /// Table methods
  void fillInput(int id, vector< pair<string, size_t> > &elements, size_t &selected);

public:
  void remove();
  bool canRemove() const;

  string getInfo() const;

  int getMissedTimeTotal() const {return tMissedTimeTotal;}
  int getMissedTimeMax() const {return tMissedTimeMax;}
  int getMissedTimeMedian() const {return tMissedTimeMedian;}
  int getNumVisitors() const {return tNumVisitors;}
  
  inline int minNumber() const {
    int m = numeric_limits<int>::max();
    for (int k=0;k<nNumbers;k++)
      m = min(Numbers[k], m);
    return m;
  }

  inline int maxNumber() const {
    int m = 0;
    for (int k=0;k<nNumbers;k++)
      m = max(Numbers[k], m);
    return m;
  }

  //Add unchecked controls to the list
  void addUncheckedPunches(vector<int> &mp, bool supportRogaining) const;
  //Start checking if all punches needed for this control exist
  void startCheckControl();
  //Get the number of a missing punch 
  int getMissingNumber() const;
  /** Returns true if the check of this control is completed
   @param supportRogaining true if rogaining controls are supported
  */
  bool controlCompleted(bool supportRogaiing) const;

	oDataInterface getDI(void);
  oDataConstInterface getDCI(void) const;

	string codeNumbers(char sep=';') const;
	bool setNumbers(const string &numbers);

	ControlStatus getStatus() const {return Status;}
  const string getStatusS() const;

  bool hasName() const {return !Name.empty();}
	string getName() const;

  bool isRogaining(bool useRogaining) const {return useRogaining && (Status == StatusRogaining);}

	void setStatus(ControlStatus st);
	void setName(string name);

  //Returns true if control has number and checks it.
	bool hasNumber(int i);
  //Return true if it has number i and it is unchecked.
  //Checks the number
	bool hasNumberUnchecked(int i);
	// Uncheck a given number
  bool uncheckNumber(int i);

  string getString();
	string getLongString();

  //For a control that requires several punches,
  //return the number of required punches.
  int getNumMulti();

  int getTimeAdjust() const;
  string getTimeAdjustS() const;
  void setTimeAdjust(int v);
  void setTimeAdjust(const string &t);

  int getMinTime() const;
  string getMinTimeS() const;
  void setMinTime(int v);
  void setMinTime(const string &t);

  int getRogainingPoints() const;
  string getRogainingPointsS() const;
  void setRogainingPoints(int v);
  void setRogainingPoints(const string &t);

  /// Return first code number (or zero)
  int getFirstNumber() const;

	void set(const xmlobject *xo);
	void set(int pId, int pNumber, string pName);
	bool write(xmlparser &xml);
	oControl(oEvent *poe);
	oControl(oEvent *poe, int id);

  virtual ~oControl();

	friend class oRunner;
	friend class oCourse;
	friend class oEvent;	
	friend class MeosSQL;
  friend class TabAuto;
  friend class TabSpeaker;
};

#endif // !defined(AFX_OCONTROL_H__E86192B9_78D2_4EEF_AAE1_3BD4A8EB16F0__INCLUDED_)
