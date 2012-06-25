#sbs-git:slp/pkgs/n/nfc-manager nfc-manager 0.0.1 7e42d204f11700974e83188adca03dcd1b0f0e9c
Name:       nfc-manager
Summary:    NFC framework manager
Version: 0.0.1
Release:    1
Group:      libs
License:    Samsung Proprietary License
Source0:    %{name}-%{version}.tar.gz
Source1:    libnfc-manager-0.init.in
Source101:  nfc-manager.service 
Source1001: packaging/nfc-manager.manifest 

Requires(post): /sbin/ldconfig
Requires(post): /usr/bin/systemctl
Requires(post): /usr/bin/vconftool
Requires(postun): /sbin/ldconfig
Requires(postun): /usr/bin/systemctl
Requires(preun): /usr/bin/systemctl

BuildRequires: pkgconfig(aul)
BuildRequires: pkgconfig(glib-2.0)
BuildRequires: pkgconfig(gobject-2.0)
BuildRequires: pkgconfig(security-server)
BuildRequires: pkgconfig(dbus-glib-1)
BuildRequires: pkgconfig(vconf)
BuildRequires: pkgconfig(dlog)
BuildRequires: pkgconfig(tapi)
BuildRequires: pkgconfig(ecore)
BuildRequires: pkgconfig(bluetooth-api)
BuildRequires: pkgconfig(mm-sound)
BuildRequires: pkgconfig(appsvc)
BuildRequires: pkgconfig(heynoti)
BuildRequires: pkgconfig(svi)
BuildRequires: cmake
BuildRequires: gettext-tools

%description
NFC library Manager.

%prep
%setup -q

%package devel
Summary:    Download agent
Group:      Development/Libraries
Requires:   %{name} = %{version}-%{release}

%description devel
NFC library Manager (devel)

%package -n nfc-common-lib
Summary:    NFC common library
Group:      Development/Libraries
Requires:   %{name} = %{version}-%{release}

%description -n nfc-common-lib
NFC Common library.

%package -n nfc-common-lib-devel
Summary:    NFC common library (devel)
Group:      libs
Requires:   %{name} = %{version}-%{release}

%description -n nfc-common-lib-devel
NFC common library (devel)


%build
cp %{SOURCE1001} .
export LDFLAGS+="-Wl,--rpath=%{_prefix}/lib -Wl,--as-needed"
mkdir cmake_tmp
cd cmake_tmp
LDFLAGS="$LDFLAGS" cmake .. -DCMAKE_INSTALL_PREFIX=%{_prefix}

make 

%install
cd cmake_tmp
%make_install
%__mkdir -p  %{buildroot}/%{_sysconfdir}/init.d/
%__cp -af %SOURCE1  %{buildroot}/%{_sysconfdir}/init.d/libnfc-manager-0

%__mkdir -p  %{buildroot}/%{_sysconfdir}/rc.d/rc3.d/
%__mkdir -p  %{buildroot}/%{_sysconfdir}/rc.d/rc5.d/
ln -s ../../init.d/libnfc-manager-0 %{buildroot}/%{_sysconfdir}/rc.d/rc3.d/S81libnfc-manager-0
ln -s ../../init.d/libnfc-manager-0 %{buildroot}/%{_sysconfdir}/rc.d/rc5.d/S81libnfc-manager-0

mkdir -p %{buildroot}%{_libdir}/systemd/system/multi-user.target.wants
install -m 0644 %SOURCE1 %{buildroot}%{_libdir}/systemd/system/nfc-manager.service
ln -s ../nfc-manager.service %{buildroot}%{_libdir}/systemd/system/multi-user.target.wants/nfc-manager.service


%preun
if [ $1 == 0 ]; then
    systemctl stop nfc-manager.service
fi

%post
/sbin/ldconfig
vconftool set -t bool db/nfc/feature 0 -u 5000
systemctl daemon-reload
if [ $1 == 1 ]; then
    systemctl restart nfc-manager.service
fi

%postun
/sbin/ldconfig
systemctl daemon-reload

%post -n nfc-common-lib -p /sbin/ldconfig

%postun -n nfc-common-lib -p /sbin/ldconfig


%files
%manifest nfc-manager.manifest
%defattr(-,root,root,-)
%{_libdir}/libnfc.so.1
%{_libdir}/libnfc.so.1.0.0
%{_prefix}/bin/nfc-agent
%{_prefix}/bin/nfc-manager-daemon
%{_prefix}/share/dbus-1/services/com.samsung.slp.nfc.agent.service
%attr(0755,-,-) %{_sysconfdir}/init.d/libnfc-manager-0
%{_sysconfdir}/rc.d/rc3.d/S81libnfc-manager-0
%{_sysconfdir}/rc.d/rc5.d/S81libnfc-manager-0
%{_libdir}/systemd/system/nfc-manager.service
%{_libdir}/systemd/system/multi-user.target.wants/nfc-manager.service

%files devel
%manifest nfc-manager.manifest
%defattr(-,root,root,-)
%{_libdir}/pkgconfig/nfc.pc
%{_includedir}/nfc/*.h
%{_libdir}/libnfc.so

%files -n nfc-common-lib
%manifest nfc-manager.manifest
%defattr(-,root,root,-)
%{_libdir}/libnfc-common-lib.so.1
%{_libdir}/libnfc-common-lib.so.1.0.0

%files -n nfc-common-lib-devel
%manifest nfc-manager.manifest
%defattr(-,root,root,-)
%{_libdir}/libnfc-common-lib.so
%{_libdir}/pkgconfig/nfc-common-lib.pc
%{_includedir}/nfc-common-lib/*.h
