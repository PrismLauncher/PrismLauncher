%global _origdir %(pwd)

Name: polymc
Version: 
Release: 1%{?dist}
Summary: A custom launcher for Minecraft
License: GPLv3
URL: https://polymc.org/

BuildArch: x86_64
BuildRequires:  java-devel
BuildRequires:  pkgconfig(gl)
BuildRequires:  pkgconfig(Qt5)
BuildRequires:  pkgconfig(zlib)

Requires: java-headless
Requires: pkgconfig(gl)
Requires: pkgconfig(Qt5)
Requires: pkgconfig(zlib)

%description
A custom launcher for Minecraft that allows you to easily manage multiple installations of Minecraft at once

%prep
mkdir -p %{_builddir}/%{name}
cp -r %{_origdir}/../../* %{_builddir}/%{name}

%build
cd %{_builddir}/%{name}
cmake \
  -DLauncher_LAYOUT=lin-system \
  -DCMAKE_INSTALL_PREFIX=/usr \
  -DLauncher_LIBRARY_DEST_DIR=%{_lib} \
  .

%cmake_build

%install
cd %{_builddir}/%{name}
%cmake_install

%files
%{_bindir}/polymc
%{_datadir}/applications/org.polymc.PolyMC.desktop
%{_datadir}/metainfo/org.polymc.PolyMC.metainfo.xml
%{_datadir}/polymc/jars/*
%{_datadir}/icons/hicolor/scalable/apps/org.polymc.PolyMC.svg
%{_libdir}/libLauncher_nbt++.so
%{_libdir}/libLauncher_quazip.so
%{_libdir}/libLauncher_rainbow.so
%{_libdir}/libLauncher_iconfix.so

%changelog
* Fri Jan 7 2022 getchoo <getchoo at tuta dot io> - 1.0.4
- Initial polymc spec
