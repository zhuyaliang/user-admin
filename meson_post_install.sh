#!/bin/sh
if [ -z $MESON_INSTALL_PREFIX ]; then
    echo 'This is meant to be ran from Meson only!'
    exit 1
fi

if [ -z $DESTDIR ]; then
    echo 'Updating icon cache'
    gtk-update-icon-cache -qtf "$MESON_INSTALL_PREFIX/share/icons/hicolor"
    echo 'Updating desktop database'
    update-desktop-database -q "$MESON_INSTALL_PREFIX/share/applications"
fi
