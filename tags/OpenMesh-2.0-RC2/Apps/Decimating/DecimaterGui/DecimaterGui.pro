################################################################################
#
################################################################################

contains( OPENFLIPPER , OpenFlipper ){
	include( $$TOPDIR/qmake/all.include )
} else {
	include( $$TOPDIR/OpenMesh/qmake/all.include )
}

INCLUDEPATH += ../../..

Application()
glew()
glut()
openmesh()

DIRECTORIES = ../../QtViewer ../

# Input
HEADERS += $$getFilesFromDir($$DIRECTORIES,*.hh)
SOURCES += ../../QtViewer/QGLViewerWidget.cc ../../QtViewer/MeshViewerWidgetT.cc ../DecimaterViewerWidget.cc
SOURCES += ../decimaterviewer.cc


################################################################################
