Name:           PolyMC
Version:        1.4
Release:        3%{?dist}
Summary:        A local install wrapper for PolyMC

License:        ASL 2.0
URL:            https://polymc.org
BuildArch:      x86_64

Requires:       zenity qt5-qtbase wget xrandr
Provides:       polymc PolyMC polymc5

%description
A local install wrapper for PolyMC

%prep


%build

%install
mkdir -p %{buildroot}/opt/polymc
install -m 0644 ../ubuntu/polymc/opt/polymc/icon.svg %{buildroot}/opt/polymc/icon.svg
install -m 0755 ../ubuntu/polymc/opt/polymc/run.sh %{buildroot}/opt/polymc/run.sh
mkdir -p %{buildroot}/%{_datadir}/applications
install -m 0644 ../ubuntu/polymc/usr/share/applications/polymc.desktop %{buildroot}/%{_datadir}/applications/polymc.desktop
mkdir -p %{buildroot}/%{_datadir}/metainfo
install -m 0644 ../ubuntu/polymc/usr/share/metainfo/polymc.metainfo.xml %{buildroot}/%{_datadir}/metainfo/polymc.metainfo.xml
mkdir -p %{buildroot}/%{_mandir}/man1
install -m 0644 ../ubuntu/polymc/usr/share/man/man1/polymc.1 %{buildroot}/%{_mandir}/man1/polymc.1

%files
%dir /opt/polymc
/opt/polymc/icon.svg
/opt/polymc/run.sh
%{_datadir}/applications/polymc.desktop
%{_datadir}/metainfo/polymc.metainfo.xml
%dir /usr/share/man/man1
%{_mandir}/man1/polymc.1.gz

%changelog
* Sun Oct 03 2021 imperatorstorm <30777770+ImperatorStorm@users.noreply.github.com>
- added manpage

* Tue Jun 01 2021 kb1000 <fedora@kb1000.de> - 1.4-2
- Add xrandr to the dependencies

* Tue Dec 08 00:34:35 CET 2020 joshua-stone <joshua.gage.stone@gmail.com>
- Add metainfo.xml for improving package metadata

* Wed Nov 25 22:53:59 CET 2020 kb1000 <fedora@kb1000.de>
- Initial version of the RPM package, based on the Ubuntu package
