
%global libnbtplusplus_commit       dc72a20b7efd304d12af2025223fad07b4b78464
%global libnbtplusplus_shortcommit  %(c=%{libnbtplusplus_commit}; echo ${c:0:7})
%global quazip_commit               c9ef32de19bceb58d236f5c22382698deaec69fd
%global quazip_shortcommit          %(c=%{quazip_commit}; echo ${c:0:7})

Name:           polymc
Version:        1.0.5
Release:        2%{?dist}
Summary:        Minecraft launcher with ability to manage multiple instances

#
# CC-BY-SA
# ---------------------------------------
# launcher/resources/multimc/
#
# BSD 3-clause "New" or "Revised" License
# ---------------------------------------
# application/
# libraries/LocalPeer/
# libraries/ganalytics/
#
# Boost Software License (v1.0)
# ---------------------------------------
# cmake/
#
# Expat License
# ---------------------------------------
# libraries/systeminfo/
#
# GNU Lesser General Public License (v2 or later)
# ---------------------------------------
# libraries/rainbow
#
# GNU Lesser General Public License (v2.1 or later)
# ---------------------------------------
# libraries/iconfix/
# libraries/quazip/
#
# GNU Lesser General Public License (v3 or later)
# ---------------------------------------
# libraries/libnbtplusplus/
#
# GPL (v2)
# ---------------------------------------
# libraries/pack200/
#
# ISC License
# ---------------------------------------
# libraries/hoedown/
#
# zlib/libpng license
# ---------------------------------------
# libraries/quazip/quazip/unzip.h
# libraries/quazip/quazip/zip.h
#

License:        CC-BY-SA and ASL 2.0 and BSD and Boost and LGPLv2 and LGPLv2+ and LGPLv3+ and GPLv2 and GPLv2+ and GPLv3 and ISC and zlib
URL:            https://polymc.org
Source0:        https://github.com/PolyMC/PolyMC/archive/%{version}/%{name}-%{version}.tar.gz
Source1:        https://github.com/MultiMC/libnbtplusplus/archive/%{libnbtplusplus_commit}/libnbtplusplus-%{libnbtplusplus_shortcommit}.tar.gz
Source2:        https://github.com/PolyMC/quazip/archive/%{quazip_commit}/quazip-%{quazip_shortcommit}.tar.gz

BuildRequires:  cmake
BuildRequires:  desktop-file-utils
BuildRequires:  gcc-c++

BuildRequires:  java-devel
BuildRequires:  %{?suse_version:lib}qt5-qtbase-devel
BuildRequires:  zlib-devel

# Minecraft <  1.17
Recommends:     java-1.8.0-openjdk-headless
# Minecraft >= 1.17
Recommends:     java-17-openjdk-headless

%description
PolyMC is a free, open source launcher for Minecraft. It allows you to have
multiple, separate instances of Minecraft (each with their own mods, texture
packs, saves, etc) and helps you manage them and their associated options with
a simple interface.


%prep
%autosetup -p1 -n PolyMC-%{version}

tar -xvf %{SOURCE1} -C libraries
tar -xvf %{SOURCE2} -C libraries
rmdir libraries/libnbtplusplus libraries/quazip
mv -f libraries/quazip-%{quazip_commit} libraries/quazip
mv -f libraries/libnbtplusplus-%{libnbtplusplus_commit} libraries/libnbtplusplus


%build
%cmake \
    -DCMAKE_BUILD_TYPE:STRING="RelWithDebInfo" \
    -DLauncher_LAYOUT:STRING="lin-system" \
    -DLauncher_LIBRARY_DEST_DIR:STRING="%{_libdir}/%{name}" \
    -DLauncher_UPDATER_BASE:STRING=""

%cmake_build

%install
%cmake_install

# Proper library linking
mkdir -p %{buildroot}%{_sysconfdir}/ld.so.conf.d/
echo "%{_libdir}/%{name}" > "%{buildroot}%{_sysconfdir}/ld.so.conf.d/%{name}-%{_arch}.conf"


%check
# skip tests on systems that aren't officially supported
%if ! 0%{?suse_version}
%ctest
desktop-file-validate %{buildroot}%{_datadir}/applications/org.polymc.polymc.desktop
%endif


%files
%license COPYING.md
%doc README.md changelog.md
%{_bindir}/%{name}
%{_libdir}/%{name}/*
%{_datadir}/%{name}/*
%{_datadir}/metainfo/org.polymc.PolyMC.metainfo.xml
%{_datadir}/icons/hicolor/scalable/apps/org.polymc.PolyMC.svg
%{_datadir}/applications/org.polymc.polymc.desktop
%config %{_sysconfdir}/ld.so.conf.d/*


%changelog
* Mon Jan 24 2022 Jan Drögehoff <sentrycraft123@gmail.com> - 1.0.5-2
- remove explicit dependencies, correct dependencies to work on OpenSuse

* Sun Jan 09 2022 Jan Drögehoff <sentrycraft123@gmail.com> - 1.0.5-1
- Update to 1.0.5

* Sun Jan 09 2022 Jan Drögehoff <sentrycraft123@gmail.com> - 1.0.4-2
- rework spec

* Fri Jan 7 2022 getchoo <getchoo at tuta dot io> - 1.0.4-1
- Initial polymc spec
