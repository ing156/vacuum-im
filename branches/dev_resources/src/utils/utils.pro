TARGET         = utils
TEMPLATE       = lib
CONFIG        += dll
QT            += xml
DEFINES       += UTILS_DLL
LIBS          += -L../libs
LIBS          += -lidn -lminizip -lzlib
DESTDIR        = ../libs
DLLDESTDIR     = ../..
include(utils.pri)

#Translations
include(../translations.inc)