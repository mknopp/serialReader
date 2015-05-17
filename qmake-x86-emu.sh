#!/bin/sh
#
# Script hack to capture -query option and return fixed values for i386 platform on X86_64 systems
#
#PROCESS ARGS
arg=$@
while [ $# -gt 1 ]
do
  if [ "$1" = '-query' ] ; then
    if [ $# -gt 0 ] ; then
      shift
      if [ "$1" = 'QT_INSTALL_LIBS' ] ; then
    	echo '/usr/lib/i386-linux-gnu'
        exit 0
      fi
      if [ "$1" = 'QT_INSTALL_PLUGINS' ] ; then
    	echo '/usr/lib/i386-linux-gnu/qt4/plugins'
        exit 0
      fi
    fi
  fi
  shift
done
/usr/bin/qmake $arg
