## Getting set up on madmax4:

1. Clone the directory. If necessary for permissions set up an SSH key (https://help.github.com/articles/generating-ssh-keys). Be sure that you have access to /lfs/madmax4/0/

2. Update the Makefile and the top of stdafx.h to point to a valid SNAP implementation -- i.e. ` /lfs/madmax4/0/dshahaf/insight/Snap_Yang`
At this point make clean and make all should work (but running the program will crash because of not-found files).

3. Copy over ngram_tokenToCount and all the wiki_* files from `/lfs/madmax4/0/dshahaf/wikiInsight2/wikiInsight

4. Update getContent in getWiki.cpp to read `TStr fileName = "/lfs/madmax4/0/dshahaf/wikiInsight2/wikiInsight/wikitxt/" + id.GetStr(id) + ".txt";` (or alternatively copy over the large wikitxt directory yourself).

5. Now running the program should work via `make && ./getWiki`
