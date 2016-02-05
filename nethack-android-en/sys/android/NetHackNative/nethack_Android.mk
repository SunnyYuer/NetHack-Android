LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

HACKCSRC = allmain.c alloc.c apply.c artifact.c attrib.c ball.c bones.c \
	   botl.c cmd.c dbridge.c decl.c detect.c dig.c display.c dlb.c do.c \
	   do_name.c do_wear.c dog.c dogmove.c dokick.c dothrow.c drawing.c \
	   dungeon.c eat.c end.c engrave.c exper.c explode.c extralev.c \
	   files.c fountain.c hack.c hacklib.c invent.c light.c lock.c \
	   mail.c makemon.c mapglyph.c mcastu.c mhitm.c mhitu.c minion.c \
	   mklev.c mkmap.c \
	   mkmaze.c mkobj.c mkroom.c mon.c mondata.c monmove.c \
	   mplayer.c mthrowu.c muse.c music.c o_init.c objnam.c \
	   options.c pager.c pickup.c pline.c polyself.c potion.c pray.c \
	   priest.c quest.c questpgr.c read.c rect.c region.c restore.c rip.c \
	   rnd.c role.c rumors.c save.c shk.c shknam.c sit.c sounds.c sp_lev.c \
	   spell.c steal.c steed.c teleport.c \
	   tile.c \
	   timeout.c topten.c track.c trap.c \
	   u_init.c uhitm.c vault.c version.c vision.c weapon.c were.c wield.c \
	   windows.c wizard.c worm.c worn.c write.c zap.c \
	   monstr.c vis_tab.c \
	   ../sys/unix/unixunix.c ../sys/share/ioctl.c \
	   ../sys/share/unixtty.c \
	   ../win/tty/getline.c ../win/tty/termcap.c \
	   ../win/tty/topl.c ../win/tty/wintty.c

# - REMOVED FROM ABOVE: monst.c objects.c
# - ADDED: monstr.c vis_tab.c
# - ADDED: unixunix unixtty
# - ADDED: ioctl.c
LOCAL_MODULE    := nethack

ANDROID_NATIVE_DIR := ../sys/android/NetHackNative
LOCAL_SRC_FILES :=		\
		$(ANDROID_NATIVE_DIR)/androidmain.c		\
		$(ANDROID_NATIVE_DIR)/androidrecover.c	\
		$(ANDROID_NATIVE_DIR)/comm.c			\
		$(ANDROID_NATIVE_DIR)/winandroid.c		\
		monst.c									\
		objects.c								\
		$(HACKCSRC)

# The -fno-builtin-printf stuff is here so that out #define remapping
# it to android_printf() works. Would be nice to not rely on #defines
# for this, but even if it was done at link time, we probably still
# would need to use -fno-builtin-printf, maybe.
LOCAL_CFLAGS	:= -I$(LOCAL_PATH)/../include -fno-builtin-printf


include $(BUILD_SHARED_LIBRARY)
