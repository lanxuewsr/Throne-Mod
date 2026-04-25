#!/bin/bash
set -e

VERSION="$1"
ARCH="$2"

mkdir -p Throne-Mod/DEBIAN
mkdir -p Throne-Mod/opt
cp -r linux-$ARCH$([[ $3 == "systemqt" ]] && echo "-system-qt") Throne-Mod/opt
mv Throne-Mod/opt/linux-$ARCH$([[ $3 == "systemqt" ]] && echo "-system-qt") Throne-Mod/opt/Throne-Mod
rm Throne-Mod/opt/Throne-Mod/Throne-Mod.debug

# basic
cat >Throne-Mod/DEBIAN/control <<-EOF
Package: Throne-Mod
Version: $VERSION
Architecture: $ARCH
Maintainer: Mahdi Mahdi.zrei@gmail.com
Depends: desktop-file-utils$([[ $3 == "systemqt" ]] && echo ", libqt6core6, libqt6gui6, libqt6network6, libqt6widgets6, qt6-qpa-plugins, qt6-wayland, qt6-gtk-platformtheme, qt6-xdgdesktopportal-platformtheme, libxcb-cursor0, fonts-noto-color-emoji")
Description: Qt based cross-platform GUI proxy configuration manager (backend: sing-box)
EOF

cat >Throne-Mod/DEBIAN/postinst <<-EOF
cat >/usr/share/applications/Throne-Mod.desktop<<-END
[Desktop Entry]
Name=Throne-Mod
Comment=Qt based cross-platform GUI proxy configuration manager (backend: sing-box)
Exec=sh -c "PATH=/opt/Throne-Mod:\$PATH /opt/Throne-Mod/Throne-Mod -appdata"
Icon=/opt/Throne-Mod/Throne.png
Terminal=false
Type=Application
Categories=Network;Application;
END

update-desktop-database
EOF

sudo chmod 0755 Throne-Mod/DEBIAN/postinst

# desktop && PATH

sudo dpkg-deb --build Throne-Mod
