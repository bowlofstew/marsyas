######################################################################
# Automatically generated by qmake (2.00a) Tue Apr 11 11:09:58 2006
######################################################################

TEMPLATE = app
TARGET += 
DEPENDPATH += .
INCLUDEPATH += ../../marsyas
unix:LIBS += -lmarsyas -L../../marsyas
!macx:LIBS += -lasound
macx:LIBS += -framework CoreAudio -framework CoreMidi -framework CoreFoundation
LIBS += -lm


# Input
HEADERS += MarSystemWrapper.h Marx2DGraph.h TopPanelNew.h
SOURCES += main.cpp \
           MarSystemWrapper.cpp \
           Marx2DGraph.cpp \
           TopPanelNew.cpp
