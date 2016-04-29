#此android.mk文件仅方便于我在windows上测试，他者请勿使用此文件编译，因为此文件会导致游戏中
#'更多'选项的bug，除非你能解决此问题。

LOCAL_PATH := $(call my-dir)/../../../src

include $(CLEAR_VARS)

HACKCSRC = allmain.c alloc.c apply.c artifact.c attrib.c ball.c bones.c \
	   botl.c cmd.c dbridge.c decl.c detect.c dig.c display.c dlb.c \
	   do_name.c do_wear.c do.c dog.c dogmove.c dokick.c dothrow.c drawing.c \
	   dungeon.c eat.c end.c engrave.c exper.c explode.c extralev.c \
	   files.c fountain.c hack.c hacklib.c invent.c light.c lock.c \
	   mail.c makemon.c mapglyph.c mcastu.c mhitm.c mhitu.c minion.c \
	   mklev.c mkmap.c mkmaze.c mkobj.c mkroom.c mon.c mondata.c monmove.c \
	   mplayer.c mthrowu.c muse.c music.c o_init.c objnam.c \
	   options.c pager.c pickup.c pline.c polyself.c potion.c pray.c \
	   priest.c quest.c questpgr.c read.c rect.c region.c restore.c rip.c \
	   rnd.c role.c rumors.c save.c shk.c shknam.c sit.c sounds.c sp_lev.c \
	   spell.c steal.c steed.c sys.c teleport.c timeout.c topten.c track.c trap.c \
	   u_init.c uhitm.c vault.c version.c vision.c weapon.c were.c wield.c \
	   windows.c wizard.c worm.c worn.c write.c zap.c \
	   monstr.c vis_tab.c tile.c \
	   ../sys/share/ioctl.c \
	   ../sys/share/unixtty.c \
	   ../sys/share/posixregex.c \
	   ../win/tty/getline.c \
	   ../win/tty/termcap.c \
	   ../win/tty/topl.c \
	   ../win/tty/wintty.c

LOCAL_MODULE := nethack

ANDROID_JNI_DIR := ../sys/android

LOCAL_SRC_FILES :=		\
		monst.c \
		objects.c \
		$(HACKCSRC) \
		$(ANDROID_JNI_DIR)/androidmain.c	\
		$(ANDROID_JNI_DIR)/androidunix.c	\
		$(ANDROID_JNI_DIR)/winandroid.c

LOCAL_C_INCLUDES := $(LOCAL_PATH)/../include

include $(BUILD_SHARED_LIBRARY)