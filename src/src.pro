TEMPLATE = subdirs

SUBDIRS += bluetooth
android: SUBDIRS += android

config_bluez:qtHaveModule(dbus) {
    SUBDIRS += tools/sdpscanner
}

#qtHaveModule(quick) {
#    imports.depends += bluetooth
#    SUBDIRS += imports
#}
