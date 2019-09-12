bin_PROGRAMS = tsim

tsim_SOURCES = AddToggleWidget.c DialogPopup.c file.c resources.c timer.c \
               tsim.c bitmap.c DrawingPalette.c Graph.c sound.c train.c \
               BitmapDirectory.c errcheck.c pipe.c Sprite.c tsim_argp.c

noinst_HEADERS = AddToggleWidget.h DialogPopup.h file.h resources.h \
         timer.h general.h bitmap.h DrawingPalette.h Graph.h sound.h train.h \
         BitmapDirectory.h errcheck.h pipe.h Sprite.h tsim_argp.h

tsim_CFLAGS = -std=gnu99 -pedantic -Wall -Wextra -Wmissing-prototypes \
              -DRESDIR=\"${datadir}/tsim\"

tsim_LDFLAGS = -L/usr/X11R6/lib -lXaw -lXt -lXmu -lX11 -lXext

