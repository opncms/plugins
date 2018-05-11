CC = g++
OPNCMSDIR = ../../opncms
FRAMEWORKDIR = ../../frameworks/bootstrap
CFLAGS += -Wall -g -I$(OPNCMSDIR)/ -I$(OPNCMSDIR)/contrib/crypt_blowfish-1.2 -I$(OPNCMSDIR)/contrib/fmt -I$(OPNCMSDIR)/contrib/mongo-cxx-driver/build/install/include -L$(OPNCMSDIR)/
BASEVIEW = $(FRAMEWORKDIR)/base.tmpl

all: libuser_view.so user.so

user.so: user.o
	$(CC) $(CFLAGS) $(LDFLAGS) -shared -Wl,-soname,user.so \
        -o user.so user.o -lcppcms -lbooster

user.o: user.cpp
	$(CC) $(CFLAGS) $(LDFLAGS) -fPIC -c user.cpp -lbcrypt

libuser_view.so: user_view.o
	$(CC) $(CFLAGS) $(LDFLAGS) -shared -Wl,-soname,libuser_view.so \
        -o libuser_view.so user_view.o -lcppcms -lbooster

user_view.o: user_view.cpp
	$(CC) $(CFLAGS) $(LDFLAGS) -fPIC -c user_view.cpp -lbcrypt

user_view.cpp: user.tmpl $(BASEVIEW)
	cppcms_tmpl_cc -s user_view $(BASEVIEW) user.tmpl -o user_view.cpp

clean:
	rm user.o user_view.cpp user_view.o libuser_view.so user.so

.PHONY: all clean

