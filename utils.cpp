#include "utils.h"

void increaseHash(THash<TInt, TInt> *h, int iKey, int iIncrease)
{
	int iCount=0;
	if (h->IsKey(iKey))
		iCount= h->GetDat(iKey);
	h->AddDat(iKey,iCount+iIncrease);
}


void increaseHashSt(THash<TStr, TInt> *h, TStr sKey, int iIncrease)
{
	int iCount=0;
	if (h->IsKey(sKey))
		iCount= h->GetDat(sKey);
	h->AddDat(sKey,iCount+iIncrease);
}

void addHashVector(THash<TInt, TIntV> *h, int iKey, int iElement)
{
	
	if (!h->IsKey(iKey))
		h->AddKey(iKey);

	(h->GetDat(iKey)).Add(iElement);
	
}

void addHashVectorSt(THash<TStr, TStrV> *h, TStr iKey, TStr iElement)
{
	if (!h->IsKey(iKey))
		h->AddKey(iKey);

	(h->GetDat(iKey)).Add(iElement);
}