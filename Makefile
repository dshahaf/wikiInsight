CXXFLAGS += -std=c++98 -g -w -Wall 
#-fopenmp
LDFLAGS += -lrt 
#-fopenmp 


SNAPPATH = ../../insight/Snap_Yang/




GSLLIB = /lfs/madmax4/0/dshahaf/gsl/include
#SQLLIB = include

all: getWiki

opt: CXXFLAGS += -O4
opt: LDFLAGS += -O4

debug: CXXFLAGS += -rdynamic -O0 -g -ggdb3 -pg
debug: LDFLAGS += -rdynamic -O0 -g -ggdb3 -pg

SQLFLAGS =   -g -pipe -Wp,-D_FORTIFY_SOURCE=2 -fexceptions -fstack-protector --param=ssp-buffer-size=4 -m64 -D_GNU_SOURCE -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -fno-strict-aliasing -fwrapv -rdynamic -lz -lcrypt -lnsl -lm -L/usr/lib64 -lssl -lcrypto -lcurl 
#-L/usr/lib64/mysql -lmysqlclient -I/usr/include/mysql
#-L/lfs/madmax4/0/dshahaf/gsl/lib -lgsl -lgslcblas -lm 

Snap1.o: 
	g++ -c $(CXXFLAGS) $(SNAPPATH)snap/Snap.cpp -o Snap1.o -I$(SNAPPATH)GLib -I$(SNAPPATH)snap 

%.o: %.cpp
	g++ -c -o $@ $<  $(CXXFLAGS)  -I$(SNAPPATH)GLib -I$(SNAPPATH)snap -I$(GSLLIB) $(SQLFLAGS) 

getWiki: getWiki.o Snap1.o utils.o
	export LD_LIBRARY_PATH=/lfs/madmax4/0/dshahaf/gsl/lib 
	g++ $(LDFLAGS) -o getWiki getWiki.o Snap1.o  utils.o -I$(SNAPPATH)GLib -I$(SNAPPATH)snap -I$(GSLLIB) $(SQLFLAGS)  

clean:  
	rm -f *.o *.png *.plt *.tab $(NAME) $(NAME).exe
