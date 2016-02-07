#!/bin/sh

echo "Copying Makefiles."

cp Makefile.top ../../Makefile
cp Makefile.dat ../../dat/Makefile
cp Makefile.doc ../../doc/Makefile
cp Makefile.src ../../src/Makefile
cp Makefile.utl ../../util/Makefile

# These are normally created by a 'make install', in the install
# directory. Note: this makes me think that maybe it's not such a good
# idea to do the symbolic link thing to the 'dat' directory, maybe
# we should treat the 'assets' directory for the application more
# like the install directory?
touch ../../dat/perm
touch ../../dat/record
touch ../../dat/logfile
