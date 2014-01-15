#ifndef _UTILS1_H
#define _UTILS1_H

#include "stdafx.h"

void increaseHash(THash<TInt, TInt> *h, int iKey, int iIncrease);
void increaseHashSt(THash<TStr, TInt> *h, TStr sKey, int iIncrease);

void addHashVector(THash<TInt, TIntV> *h, int iKey, int iElement);
void addHashVectorSt(THash<TStr, TStrV> *h, TStr iKey, TStr iElement);


class SortPrBySecond {
private:
  bool IsAsc;
public:
  SortPrBySecond(const bool& AscSort=true) : IsAsc(AscSort) { }
  bool operator () (const TPair<TInt, TFlt> T1, const TPair<TInt, TFlt> T2) const {
    
    if (IsAsc) {
      return T1.Val2 < T2.Val2;
    } else {
      return T2.Val2 < T1.Val2;
    }
  }
};

#endif