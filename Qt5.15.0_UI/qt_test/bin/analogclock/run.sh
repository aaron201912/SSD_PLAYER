#!/bin/sh

export QT_DEBUG_PLUGINS=1
export QT_QPA_PLATFORM=linuxfb:fb=/dev/fb0:rotation=0
export QT_QPA_FONTDIR=/mnt/zkEnv/qt_test/font
export LD_LIBRARY_PATH=/mnt/zkEnv/qt_test/lib:$LD_LIBRARY_PATH
chmod 777 analogclock
./analogclock
