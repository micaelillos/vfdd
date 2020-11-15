#
# vfdd Makefile

BINARY = vfdd
WEBSITE = www.rvq.fr/linux/$(BINARY).php

ADDDIRS := vfdmod

DEFS  += -DMSG_DEBUG -DMSG_DUMP
#DEFS += -DTRACE_MEM

CPPFLAGS  := -I.
CFLAGS = -O2 -g

LDFLAGS  := -g

LDLIBS   :=

#LDADD := -L$(COMMON)/tracemem -ltracemem

#********************* Files ******************************

COMSRCS := sigmain.c msglog.c appmem.c strmem.c appclass.c strcatdup.c 
COMSRCS += duprintf.c logger.c selloop.c channel.c timerms.c
COMSRCS += dbuf.c mutil.c mdbuf.c fileutil.c stutil.c
COMSRCS += c2hex.c dlist.c jsonnode.c jsonroot.c jsonpath.c utf8.c

LOCALSRCS =

SRCS  := vfddmain.c vfdd.c dotled.c display.c testhci.c

COMHEADERS := sigmain.h msglog.h appmem.h strmem.h appclass.h strcatdup.h
COMHEADERS += duprintf.h logger.h selloop.h channel.h
COMHEADERS += dbuf.h mutil.h mdbuf.h fileutil.h timerms.h stutil.h
COMHEADERS += c2hex.h dlist.h jsonnode.h jsonroot.h jsonpath.h utf8.h

LOCALHEADERS =

HEADERS := vfdd.h dotled.h display.h vfd-glyphs.c.h

FILES := vfdd.conf.in vfdd.runit.in

############################# end files ###################
BINDIR = /usr/bin
ETCDIR = /etc
CONFFILE = vfdd.conf
RUNITFILE = vfdd.runit.in

install_recipe  = if test -f $(BINDIR)/$(BINARY) ; then
install_recipe += mv $(BINDIR)/$(BINARY) $(BINDIR)/$(BINARY).old;
install_recipe += fi;
install_recipe += cp $(BINARY) $(BINDIR);
install_recipe += if test ! -f $(ETCDIR)/$(CONFFILE) ; then
install_recipe += cp $(CONFFILE).in $(ETCDIR)/$(CONFFILE);
install_recipe += fi;
install_recipe += if test ! -f $(ETCDIR)/sv/vfdd/run ; then
install_recipe += mkdir -p $(ETCDIR)/sv/vfdd;
install_recipe += cp $(RUNITFILE) $(ETCDIR)/sv/vfdd/run;
install_recipe += fi;

############################# end install_recipe ###################
#V = 1
   
# include $(shell where-sdk sdklinux)/Makefile.include
include common/Makefile.include

