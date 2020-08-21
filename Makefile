#----------------------------------------------
#
#     Makefile pro ff-clone
#
#-----------------------------------------------

BINDIR = /usr/local/bin
DATADIR = /usr/local/share/ff-clone

#-----------------------------------------------

CC = gcc
INCLUDE = -I.
OBJ = main.o loop.o warning.o window.o X.o draw.o gener.o layers.o script.o levelscript.o object.o rules.o moves.o gmoves.o keyboard.o imgsave.o directories.o menudraw.o menuloop.o menuscript.o gsaves.o
LIB = -lX11 -lcairo -llua
PROGNAME = ff-clone

all: $(PROGNAME)

directories.o: directories.c
	$(CC) $(INCLUDE) -g -Wall -c -o directories.o directories.c -DDATADIR="\"$(DATADIR)\""

.c.o:
	$(CC) $(INCLUDE) -g -Wall -c -o $*.o $<

$(PROGNAME): $(OBJ)
	$(CC) $(INCLUDE) $(OBJ) $(LIB) -o $@

clean:
	rm -rf *.o $(PROGNAME)

install: $(PROGNAME)
	install -s $(PROGNAME) $(BINDIR)/
	mkdir -p $(DATADIR)
	cp -r data/* $(DATADIR)/
