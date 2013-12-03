CONFIG += java
DESTDIR = $$[QT_INSTALL_PREFIX/get]/jar
API_VERSION = android-10

PATHPREFIX = $$PWD/src/org/qtproject/qt5/android/bluetooth

JAVACLASSPATH += $$PWD/src/
JAVASOURCES += \
    $$PATHPREFIX/QtBluetoothBroadcastReceiver.java

# install
target.path = $$[QT_INSTALL_PREFIX]/jar
INSTALLS += target
