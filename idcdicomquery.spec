Name: idcdicomquery
Provides: idcdicomquery
Version: 1.0.0
Release: 1%{?dist}
License: LGPL-2.1+
Source: %{name}.tar.gz
URL: http://softus.org/products/idcdicomquery
Vendor: Softus Inc. <contact@softus.org>
Packager: Softus Inc. <contact@softus.org>
Summary: Utility program to query a DICOM server and retrieve studies from the server.

%description
Utility program to query a DICOM server and retrieve studies from the server.

%global debug_package %{nil}

BuildRequires: make, gcc-c++

%{?fedora:BuildRequires: qt5-qtbase-devel, openssl-devel, dcmtk-devel}

%{?rhel:BuildRequires: qt5-qtbase-devel, openssl-devel}

%{?suse_version:BuildRequires: libqt5-linguist, libqt5-qtbase-devel, openssl-devel, dcmtk-devel}

%if 0%{?mageia}
%define qmake qmake
%define lrelease lrelease
BuildRequires: qttools5
%ifarch x86_64 amd64
BuildRequires: lib64qt5base5-devel
%else
BuildRequires: libqt5base5-devel
%endif
%else
%define qmake qmake-qt5
%define lrelease lrelease-qt5
%endif

%prep
%setup -c %{name}
 
%build
%{lrelease} *.ts
%{qmake} PREFIX=%{_prefix} QMAKE_CFLAGS+="%optflags" QMAKE_CXXFLAGS+="%optflags";
make %{?_smp_mflags};

%install
make install INSTALL_ROOT="%buildroot";

%files
%doc doc/*
%{_mandir}/man1/%{name}.1.*
%{_bindir}/%{name}
%{_datadir}/%{name}
%{_datadir}/applications/%{name}.desktop
%{_datadir}/icons/%{name}.png

%posttrans
/bin/touch --no-create %{_datadir}/icons/hicolor &>/dev/null
/usr/bin/gtk-update-icon-cache %{_datadir}/icons/hicolor &>/dev/null || :
/usr/bin/update-desktop-database &> /dev/null || :

%changelog
* Mon Apr 09 2018 Pavel Bludov <pbludov@gmail.com>
+ Version 1.0.0
- First rpm spec for automated builds.
