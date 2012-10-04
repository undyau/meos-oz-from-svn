#pragma once

/** Struct with info to draw a class */
struct ClassInfo {
	int classId;
  pClass pc;

	int firstStart; 
	int interval;

	int unique;
	int courseId;
	int nRunners;
  int nRunnersGroup; // Number of runners in group
  int nRunnersCourse; // Number of runners on this course

  bool nVacantSpecified;
  int nVacant;

  bool nExtraSpecified;
  int nExtra;

  int sortFactor;

  // Algorithm status. Extra time needed to start this class.
  int overShoot;

  bool hasFixedTime;

  ClassInfo() {
    memset(this, 0, sizeof(ClassInfo));
    nVacant = -1;
  }

  ClassInfo(pClass pClass) {
    memset(this, 0, sizeof(ClassInfo)); 
    pc = pClass;
    classId = pc->getId();
    nVacant = -1;
  }

  // Selection of sorting method
  static int sSortOrder;
  bool operator<(ClassInfo &ci);
};


/**Structure for optimizing start order */
struct DrawInfo {
  DrawInfo();
  double vacancyFactor;
  double extraFactor;
  int minVacancy;
  int maxVacancy;
  int baseInterval;
  int minClassInterval;
  int maxClassInterval;
  int nFields;
  int firstStart;
  int maxCommonControl;

  // Statistics output from optimize start order
  int numDistinctInit;
  int numRunnerSameInitMax;
  int numRunnerSameCourseMax;
  int minimalStartDepth;

  map<int, ClassInfo> classes;
  string startName;
};
