#!/bin/sh

echo "Copying Makefiles."

cp Makefile.top ../../Makefile
cp Makefile.dat ../../dat/Makefile
cp Makefile.doc ../../doc/Makefile
cp Makefile.src ../../src/Makefile
cp Makefile.utl ../../util/Makefile

echo "Setting absolute path"

sed $(echo s@ABSOLUTE_PATH@$(pwd)@g) < NetHackNative/Application0.mk > NetHackNative/Application.mk

echo "Setting up links"

cd NetHackApp
ln -s ../NetHackNative/libs
cd ..

cd NetHackApp/assets
ln -s ../../../../dat nethackdir
cd ../../

ANDROID_NDK_DIR=$(echo ~)/android-ndk-1.6_r1

ln -s $(pwd)/NetHackNative $ANDROID_NDK_DIR/apps/
# Not needed in 1.6:
# ln -s $(pwd)/NetHackNative $ANDROID_NDK_DIR/sources/

# Set up the 'src' directory as project in the Android native directory.
ln -s $(pwd)/../../src NetHackNative/nethack

# Put a link to the Android makefile in the 'src' directory,
# and link in other Android-specific files into the source tree.
ln -s $(pwd)/NetHackNative/nethack_Android.mk ../../src/Android.mk
ln -s $(pwd)/NetHackNative/androidconf.h ../../include/androidconf.h

# These are normally created by a 'make install', in the install
# directory. Note: this makes me think that maybe it's not such a good
# idea to do the symbolic link thing to the 'dat' directory, maybe
# we should treat the 'assets' directory for the application more
# like the install directory?
touch ../../dat/perm
touch ../../dat/record
touch ../../dat/logfile
