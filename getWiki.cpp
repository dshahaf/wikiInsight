#include "stdafx.h"
#include "utils.h"
//#include "surprise.h"
//#include "plaus.h"
//#include <gsl/gsl_sf_bessel.h>
#include <curl/curl.h>

int FILE_TO_LOAD = 220000;

// Caching previous results (so we don't need to call wiki API all the time)

THash<TStr, TInt> termToID;  //term to id. more than one string can map to the same ID (horses, horse)
TStrV idToTerm; //the mapping back is to the name of the wiki page

// note: content will be in flat files named id.txt

THash<TInt, TInt> idToInDeg; //in-degree of the wiki page
THash<TInt, TStrPrV> idToSentences; // what the web says about this id
int curID;


THashSet<TStr> termNotInWiki; // Words that do not have a wiki entry
THashSet<TStr> termInWiki; // Terms with a wiki entry. 

// stop words!

// if a relation includes a stop phrase or a stop word, we ignore the sentence altogether
THashSet<TStr> relStopWords;
TStrV relStopPhrases;

// if a term include a stop word, ignore that word when scoring surprise
THashSet<TStr> objStopWords;

// google ngrams: frequency of a string
THash<TStr, TInt> ngram; 


static size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream)
{
  int written = fwrite(ptr, size, nmemb, (FILE *)stream);
  return written;
}


void increaseHash(THash<TStr, TInt> *h, TStr sKey, int iIncrease)
{
	int iCount=0;
	if (h->IsKey(sKey))
		iCount= h->GetDat(sKey);
	h->AddDat(sKey,iCount+iIncrease);
}




int getURL(TStr url, TStr* ret)
{
  url.ChangeStrAll(" ","_");

  CURL *curl_handle;
  static const char *headerfilename = "head.out";
  FILE *headerfile;
  static const char *bodyfilename = "body.out";
  FILE *bodyfile;
  THash< TStr, TInt > hCounts;

  curl_global_init(CURL_GLOBAL_ALL);

    
  /* init the curl session */ 
  curl_handle = curl_easy_init();
 
  /* set URL to get */ 
  
  curl_easy_setopt(curl_handle, CURLOPT_URL, url.CStr());
 
  /* no progress meter please */ 
  curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 1L);
 
  /* send all data to this function  */ 
  curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_data);
 
  /* open the files */ 
  headerfile = fopen(headerfilename,"wb");
  if (headerfile == NULL) {
    curl_easy_cleanup(curl_handle);
    return -1;
  }
  bodyfile = fopen(bodyfilename,"wb");
  if (bodyfile == NULL) {
    curl_easy_cleanup(curl_handle);
    return -1;
  }
 
  /* we want the headers be written to this file handle */ 
  curl_easy_setopt(curl_handle,   CURLOPT_WRITEHEADER, headerfile);
 
  /* we want the body be written to this file handle instead of stdout */ 
  curl_easy_setopt(curl_handle,   CURLOPT_WRITEDATA, bodyfile);
 
  /* get it! */ 
  curl_easy_perform(curl_handle);
 
  /* close the header file */ 
  fclose(headerfile);
 
  /* close the body file */ 
  fclose(bodyfile);
 
  /* cleanup curl stuff */ 
  curl_easy_cleanup(curl_handle); 

  TStr fname(bodyfilename);
  TStr Line; (*ret)="";
  TFIn f(fname);
  while (!f.Eof()) {
    f.GetNextLn(Line);
    (*ret)+= Line;  
    }  

  return 0;

}


// get the wiki in-degree, capped at 2000
int getInDegree(TInt id, TStr concept)
{

       if (idToInDeg.IsKey(id))
		return idToInDeg.GetDat(id);

	int count=0;
	TInt iLimit=250;
	TInt iMax=0;
	bool bCont= true;
	
	while (bCont && count<2000)
	{
		TStr content = "";
		bCont= false;
  		
		TStr url= "http://en.wikipedia.org/w/api.php?action=query&list=backlinks&bltitle=" + concept + "&bllimit=" + iLimit.GetStr() + "&blfilterredir=nonredirects&format=json";
		url+= "&continue=-||&blcontinue=0|" + concept + "|" + iMax.GetStr();
	
		getURL(url, &content);
 
		//printf("\n\n%s", content.CStr());
		// count them

		PJsonVal json = TJsonVal::GetValFromStr(content);
  
		if (json->IsObj() && json->IsObjKey("query"))
  		{
         		PJsonVal query= json->GetObjKey("query");
	  		if (query->IsObj() && query->IsObjKey("backlinks"))
     	  		{
				PJsonVal backlinks= query->GetObjKey("backlinks");
		     		if (backlinks->GetJsonValType() == jvtArr)
				{
			     		count+= backlinks->GetArrVals();
					if (backlinks->GetArrVals() == iLimit)
					{
						bCont=true;
						PJsonVal entry= backlinks->GetArrVal(backlinks->GetArrVals()-1);
							if (entry->IsObjKey("bl"))
		  						iMax=  (entry->GetObjStr("pageid")).GetInt() +1;
					}
				}

	  		}
		}


	}

  idToInDeg.AddDat(id, count);
  return count;
}


// get wiki content
TStr getContent(TInt id, TStr concept)
{
	TStr content = "";
	TStr fileName = "/lfs/madmax4/0/dshahaf/wikiInsight2/wikiInsight/wikitxt/" + id.GetStr(id) + ".txt";

	if (!TFile::Exists(fileName))
	{	
		TStr url= "http://en.wikipedia.org/w/api.php?action=query&titles=" + concept + "&format=json&prop=revisions&rvprop=content";
		getURL(url, &content);
		TFOut FOut(fileName);
		FOut.PutStr(content);
	}
	else
	{
		TFIn FIn(fileName);
		TChA LnStr;
		// read file
		while (FIn.GetNextLn(LnStr))
			content+= TStr(LnStr) + " ";

	}	

	return content;
}


TStr searchAndRedirect(TStr concept)
{
// http://en.wikipedia.org/w/api.php?action=query&titles=horses&redirects
// http://en.wikipedia.org/w/api.php?action=query&list=search&srsearch=horses&srprop=score&format=json

	TStr content = "";

	TStr url= "http://en.wikipedia.org/w/api.php?action=query&list=search&srsearch=" + concept + "&srprop=score&format=json";
	
	getURL(url, &content);
 
	//printf("\n\n%s", content.CStr());
	// find first result 

	PJsonVal json = TJsonVal::GetValFromStr(content);
  
	if (json->IsObj() && json->IsObjKey("query"))
  	{
         PJsonVal query= json->GetObjKey("query");
	  if (query->IsObj() && query->IsObjKey("search"))
     	  {
		PJsonVal search= query->GetObjKey("search");
     		if (search->GetJsonValType() == jvtArr && search->GetArrVals()>=1)
     		{  
			PJsonVal entry= search->GetArrVal(0);
			if (entry->IsObjKey("title"))
		  		return entry->GetObjStr("title");			
		}
	  }
	}

  return concept;
}


bool checkWikiPageExists(TStr concept)
{
	TStr content = "";
	TStr url = "https://en.wikipedia.org/w/api.php?action=query&titles=" + concept +  "&format=json";
	getURL(url, &content);
	//printf("\n\n%s", content.CStr());
	PJsonVal json = TJsonVal::GetValFromStr(content);
  
	if (json->IsObj() && json->IsObjKey("query")) {
	  PJsonVal query = json->GetObjKey("query");
	  if (query->IsObj() && query->IsObjKey("pages"))
	    {
	      PJsonVal pages = query->GetObjKey("pages");
	      if (pages->IsObj() && !pages->IsObjKey("-1"))
		{
		  /* If the page does not exist, then the obj key is -1. Otherwise, it is some 
		     valid page ID. */
		  printf("YES: %s \n", concept.CStr());
		  return true;
		}
	    }
	}
	printf("NO:  %s \n", concept.CStr());
	return false;
}

/* See if the key succeeds, or the key without a leading "a" or "the" */
bool checkWikiPageExistsLoosely(TStr term2) {
  if (checkWikiPageExists(term2)) {
    return true;
  }
  // short circuit eval
  if ((term2.IsPrefix("a ") || term2.IsPrefix("A ")) &&  // TODO an and An if necessary
      checkWikiPageExists(term2.GetSubStr(2))) {
    return true;
  }
  if ((term2.IsPrefix("the ") || term2.IsPrefix("The ")) &&
      checkWikiPageExists(term2.GetSubStr(4)) ) {
    return true;
  }
  return false; 
}


void loadData()
{
	printf("Loading data\n");
	if (TFile::Exists("wiki_termToID"))
	{
		// yup, there has to be a better way
		TFIn f1("wiki_termToID"); termToID.Load(f1);
		TFIn f2("wiki_idToTerm"); idToTerm.Load(f2);
		TFIn f3("wiki_idToInDeg"); idToInDeg.Load(f3);
		TFIn f4("wiki_idToSentences"); idToSentences.Load(f4);
		printf("Loaded!\n");
		return;
	}


}


void loadData(TInt i)
{
	printf("Loading data\n");
	if (TFile::Exists("wiki_termInWiki" + i.GetStr())) {
	  TFIn fYes("wiki_termInWiki" + i.GetStr()); termInWiki.Load(fYes);
	}
	if (TFile::Exists("wiki_termInWiki" + i.GetStr())) {
	  TFIn fNo("wiki_termNotInWiki" + i.GetStr()); termNotInWiki.Load(fNo);
	}
	if (TFile::Exists("wiki_termToID" + i.GetStr()))
	{
		// yup, there has to be a better way
		TFIn f1("wiki_termToID" + i.GetStr()); termToID.Load(f1);
		TFIn f2("wiki_idToTerm" + i.GetStr()); idToTerm.Load(f2);
		TFIn f3("wiki_idToInDeg" + i.GetStr()); idToInDeg.Load(f3);
		TFIn f4("wiki_idToSentences" + i.GetStr()); idToSentences.Load(f4);
		
		printf("Loaded!\n");
		return;
	}


}



// 
TStr cleanText(TStr st)
{
   TStr stClean = st;
   stClean.ToLc();
   stClean.ChangeStrAll("'s","");
   stClean.ChangeStrAll("."," ");
   stClean.ChangeStrAll(","," ");
   stClean.ChangeStrAll("?"," ");
   stClean.ChangeStrAll("!"," ");
   stClean.ChangeStrAll("-"," ");
   stClean.ChangeStrAll("\""," ");
   stClean.ChangeStrAll("'"," ");

   stClean.ChangeStrAll(" the "," ");
   stClean.ChangeStrAll(" is "," ");
   stClean.ChangeStrAll(" are "," ");
   stClean.ChangeStrAll(" was "," ");
   stClean.ChangeStrAll(" were "," ");
   stClean.ChangeStrAll(" am "," ");
   stClean.ChangeStrAll(" this "," ");
   stClean.ChangeStrAll(" it "," ");


   stClean.ChangeStrAll("  "," ");

   return stClean;
}

void saveData()
{

		TFOut f1("wiki_termToID"); termToID.Save(f1);
		TFOut f2("wiki_idToTerm"); idToTerm.Save(f2);
		TFOut f3("wiki_idToInDeg"); idToInDeg.Save(f3);
		TFOut f4("wiki_idToSentences"); idToSentences.Save(f4);
}


void saveData(TInt i)
{
		printf("********* Saving %d ********\n", (int) i);
		TFOut f1("wiki_termToID" + i.GetStr()); termToID.Save(f1);
		TFOut f2("wiki_idToTerm" + i.GetStr()); idToTerm.Save(f2);
		TFOut f3("wiki_idToInDeg" + i.GetStr()); idToInDeg.Save(f3);
		TFOut f4("wiki_idToSentences" + i.GetStr()); idToSentences.Save(f4);
}


// trying to find if term2 (or something similar) exists in the content
// TODO: very hacky. improve!
int scoreSurprise(TStr *content, TStr term2)
{
	term2.ToLc();
       term2.ToTrunc();

	int mostInformative=100000000; //smaller is more informative
	bool informativeAppears=false;

	int srchInd= content->SearchStr(term2,0);
       
	if (srchInd<0) // not found. try to see whether parts have been found
       {
		TStrV tokens;
		term2.SplitOnStr(" ", tokens);
  		int numAppear=0; int numTotal=0;
		float percAppear= 0;

		// how many of the words appear?
		for (int t=0; t< tokens.Len(); t++)
		{
			// is this a valid token?
			if (objStopWords.IsKey(tokens[t]))
				continue;

			if (tokens[t].Len()<3)
				continue;

			TStr tokenSing= tokens[t];
			//single, plural. TODO
			if (tokenSing.Len()>=5 && tokenSing.LastCh()=='s')
				tokenSing= tokenSing.GetSubStr(0,tokenSing.Len()-2);

			numTotal++;
			int curInformative;
			if (ngram.IsKey(tokens[t]))
				curInformative= (int) ngram.GetDat(tokens[t]);
			else
				curInformative=-1;

			bool bWordAppears= false;
			if (content->SearchStr(tokenSing,0)>= 0) 
			{	
				numAppear++; bWordAppears=true;
			}

			if (curInformative>0 && curInformative<mostInformative)
			{
				mostInformative= curInformative;
				informativeAppears=bWordAppears;
			}

			
		}
		
		if (numTotal>0)
			percAppear= 1.0*numAppear/numTotal;

		if (percAppear<0.5 && !informativeAppears) 
		// at least 50% don't appear, and the most informative one doesn't too
		{
			return mostInformative;
		}
	
	}		
	return 0;
}


// does this relation pass the stopword test?
bool testRelation(TStr rel)
{
	rel.ToLc();
       rel.ToTrunc();
	TStrV reltokens;
	
	// test for expressions
	for (int i=0; i<relStopPhrases.Len(); i++)
		if (rel.SearchStr(relStopPhrases[i],0) >= 0)
			return false;	

	// test for stopwords
	rel.SplitOnStr(" ", reltokens);
       for (int t=0; t<reltokens.Len(); t++)
		if (relStopWords.IsKey(reltokens[t]))
			return false;

	return true;
}

void addToRelStopWords(char* filename) {
  TStr nextLine;
  TFIn relStopWordsFile("./relStopWords.txt");
  while (!relStopWordsFile.Eof()) {
    relStopWordsFile.GetNextLn(nextLine);  // Without the \n char?
    relStopWords.AddKey(nextLine);
  }
}
void loadStopwords()
{
  addToRelStopWords("./relStopWords.txt");
  // Add opinion words and colloquial words to stop words. These are currently 
  // manually assembled, but could be done computationally instead.
  addToRelStopWords("./opinionWords.txt");
  addToRelStopWords("./colloquialWords.txt");

  // Add phrases and stop words for surprise.
  TStr nextLine;
  TFIn relStopPhrasesFile("./relStopPhrases.txt");
  while (!relStopPhrasesFile.Eof()) {
    relStopPhrasesFile.GetNextLn(nextLine);  // Without the \n char?
    relStopPhrases.Add(nextLine);
  }

  TFIn objStopWordsFile("./objStopWords.txt");
  while (!objStopWordsFile.Eof()) {
    objStopWordsFile.GetNextLn(nextLine);  // Without the \n char?
    objStopWords.AddKey(nextLine);
  }

  // TODO Do we need to close TFIn or does the destructor handle it?
}

int main(int argc, char* argv[]) 
{
  printf("********* Running\n");

  // go over linked extractions
  TStrV newTerms;

  loadStopwords();
  loadData(FILE_TO_LOAD);
  curID= idToTerm.Len();

  // load n-grams
  TFIn f("ngram_tokenToCount"); ngram.Load(f);

  
  printf("Sorting \n");

  // sort hash idToInDeg

  TVec< TPair<TInt, TFlt> > vecInDeg;
  for (THash<TInt, TInt>::TIter NI = idToInDeg.BegI(); NI<idToInDeg.EndI(); NI++)
   {
	TPair<TInt, TFlt> pr(NI.GetKey(), 1.0*NI.GetDat());
	vecInDeg.Add(pr);
   }

  vecInDeg.SortCmp(SortPrBySecond(false)); // descending

  // pick top 10%, go over sentences

  for (int i=0; i<vecInDeg.Len()/10; i++)
  {

  int id= vecInDeg[i].Val1;
  //printf("id %d\n", id);

  // get content, try figuring out if wiki already knows about it

  TStr content= getContent(id, idToTerm[id]);
  // remove some markup
  content.ChangeStrAll("[[","");
  content.ChangeStrAll("]]","");
  content= cleanText(content);

    if (!idToSentences.IsKey(id))
    {
		printf("********** couldn't find key\n");
		continue;
     }


   TStrPrV sentences = idToSentences.GetDat(id);


    TVec< TPair<TInt, TFlt> > sentScore;
    
    // iterate over sentences about this term. score by surprise
    // and store in sentScore
    int iCount= (idToSentences.GetDat(id)).Len();

    for (int j=0; j< iCount; j++)
    {
  

  // TODO: a reverse search.
  	
	// start by looking at the relation
	TStr rel = sentences[j].Val2.CStr();
       if (!testRelation(rel))
		continue;

       TStr term2=  cleanText(sentences[j].Val1);     //(idToSentences.GetDat(i))[j].Val1;
       term2.ToTrunc();  // TODO is crashing with illegal instruction


	int surp= scoreSurprise(&content, term2);

	if (surp>0)
	  {
	    //printf("Searching key: %s\n", term2.CStr());
	    /* Only consider the phrase if the second term is also a wiki key. */
	    /* TODO: checking whether the second key is a term is currently here so we can see 
	       more clearly the impact. It fits better before the surprise score, though. */
	    if (termNotInWiki.IsKey(term2)) { // check the cache
	      continue; // invalidate this term2
	    }
	    if (!termInWiki.IsKey(term2)) { // key is unknown
	      if (checkWikiPageExistsLoosely(term2)) {
		termInWiki.AddKey(term2); // Cache the response -- adds the original form of the term
		// Save the term in wiki and term no wiki maps -- TODO this is very inefficient, 
		// but creates the files for new keys for now. Remove once the files are generated.
		TFOut fYes("wiki_termInWiki" + FILE_TO_LOAD); termInWiki.Save(fYes);
	      } else {
		termNotInWiki.AddKey(term2);
		TFOut fNo("wiki_termNotInWiki" + FILE_TO_LOAD); termNotInWiki.Save(fNo);
		continue; // invalidate this term2
	      }
	    }
	    TPair<TInt, TFlt> pr(j, 1.0*surp);
	    sentScore.Add(pr);
	  }
    }
    

    sentScore.SortCmp(SortPrBySecond(true)); // ascending
  
  //output 
  if (sentScore.Len()>0)
  	printf("\n");
  for (int j=0; j<sentScore.Len(); j++)
  	printf("%s %s (%s) %d \n", idToTerm[id].CStr(), sentences[sentScore[j].Val1].Val2.CStr(), sentences[sentScore[j].Val1].Val1.CStr(), (int)sentScore[j].Val2);

  }

  printf("\n** DONE **\n");



  return 0;
}

