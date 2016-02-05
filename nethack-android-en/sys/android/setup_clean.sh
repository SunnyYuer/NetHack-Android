#!/bin/sh

ANDROID_NDK_DIR=$(echo ~)/android-ndk-1.6_r1

rm NetHackApp/libs
rm $ANDROID_NDK_DIR/apps/NetHackNative
# rm $ANDROID_NDK_DIR/sources/NetHackNative
rm NetHackNative/nethack
rm NetHackApp/assets/nethackdir
rm ../../src/Android.mk
rm ../../include/androidconf.h
