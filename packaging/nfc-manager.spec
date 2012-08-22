Name:       nfc-manager
Summary:    NFC framework manager
Version:    0.0.3
Release:    9
Group:      libs
License:    Samsung Proprietary License
Source0:    %{name}-%{version}.tar.gz
Source1:    libnfc-manager-0.init.in
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
BuildRequires: pkgconfig(smartcard-service)
BuildRequires: pkgconfig(smartcard-service-common)
BuildRequires: pkgconfig(libssl)
BuildRequires: pkgconfig(pmapi)
BuildRequires: cmake
BuildRequires: gettext-tools
Requires(post):   /sbin/ldconfig
Requires(post):   /usr/bin/vconftool
requires(postun): /sbin/ldconfig
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
export LDFLAGS+="-Wl,--rpath=%{_prefix}/lib -Wl,--as-needed"
mkdir cmake_tmp
cd cmake_tmp
LDFLAGS="$LDFLAGS" cmake .. -DCMAKE_INSTALL_PREFIX=%{_prefix}

make 

%install
cd cmake_tmp
%make_install
%__mkdir -p  %{buildroot}/etc/init.d/
%__cp -af %SOURCE1  %{buildroot}/etc/init.d/libnfc-manager-0
chmod 755 %{buildroot}/etc/init.d/libnfc-manager-0

%post
/sbin/ldconfig
vconftool set -t bool db/nfc/feature 0 -u 5000
vconftool set -t bool db/nfc/enable 0 -u 5000
vconftool set -t bool db/nfc/sbeam 0 -u 5000

vconftool set -t bool memory/private/nfc-manager/popup_disabled 0 -u 5000

ln -s /etc/init.d/libnfc-manager-0 /etc/rc.d/rc3.d/S81libnfc-manager-0
ln -s /etc/init.d/libnfc-manager-0 /etc/rc.d/rc5.d/S81libnfc-manager-0

%postun
/sbin/ldconfig
mkdir -p /etc/rc.d/rc3.d
mkdir -p /etc/rc.d/rc5.d
rm -f /etc/rc.d/rc3.d/S81libnfc-manager-0
rm -f /etc/rc.d/rc5.d/S81libnfc-manager-0

%post -n nfc-common-lib -p /sbin/ldconfig

%postun -n nfc-common-lib -p /sbin/ldconfig

%files
%defattr(-,root,root,-)
%{_libdir}/libnfc.so.1
%{_libdir}/libnfc.so.1.0.0
%{_prefix}/bin/nfc-manager-daemon
%{_prefix}/bin/ndef-tool
/etc/init.d/libnfc-manager-0

%files devel
%defattr(-,root,root,-)
%{_libdir}/pkgconfig/nfc.pc
%{_includedir}/nfc/*.h
%{_libdir}/libnfc.so


%files -n nfc-common-lib
%defattr(-,root,root,-)
%{_libdir}/libnfc-common-lib.so.1
%{_libdir}/libnfc-common-lib.so.1.0.0

%files -n nfc-common-lib-devel
%defattr(-,root,root,-)
%{_libdir}/libnfc-common-lib.so
%{_libdir}/pkgconfig/nfc-common-lib.pc
%{_includedir}/nfc-common-lib/*.h
