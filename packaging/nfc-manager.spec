Name:       nfc-manager
Summary:    NFC framework manager
Version:    0.1.148
Release:    0
Group:      Network & Connectivity/NFC
License:    Flora-1.1
Source0:    %{name}-%{version}.tar.gz
Source1:    nfc-manager.service
Source2:    nfc-manager.socket
BuildRequires: cmake
BuildRequires: pkgconfig(aul)
BuildRequires: pkgconfig(glib-2.0)
BuildRequires: pkgconfig(gobject-2.0)
BuildRequires: pkgconfig(vconf)
BuildRequires: pkgconfig(dlog)
BuildRequires: pkgconfig(tapi)
BuildRequires: pkgconfig(bluetooth-api)
BuildRequires: pkgconfig(capi-network-bluetooth)
BuildRequires: pkgconfig(mm-sound)
BuildRequires: pkgconfig(appsvc)
BuildRequires: pkgconfig(feedback)
BuildRequires: pkgconfig(capi-media-wav-player)
BuildRequires: pkgconfig(openssl)
BuildRequires: pkgconfig(deviced)
BuildRequires: pkgconfig(mm-keysound)
BuildRequires: pkgconfig(syspopup-caller)
BuildRequires: pkgconfig(notification)
BuildRequires: pkgconfig(capi-network-wifi)
BuildRequires: pkgconfig(capi-network-wifi-direct)
BuildRequires: pkgconfig(capi-system-info)
BuildRequires: pkgconfig(sqlite3)
BuildRequires: pkgconfig(pkgmgr-info)
BuildRequires: pkgconfig(libxml-2.0)
BuildRequires: pkgconfig(libcurl)
BuildRequires: pkgconfig(cynara-client)
BuildRequires: pkgconfig(cynara-creds-gdbus)
BuildRequires: pkgconfig(cynara-session)
BuildRequires: pkgconfig(capi-system-device)
BuildRequires: pkgconfig(libsystemd-daemon)
BuildRequires: python
BuildRequires: python-xml

Requires(post): /sbin/ldconfig
Requires(postun): /sbin/ldconfig


%description
A NFC Service Framework for support Tizen NFC.


%prep
%setup -q


%package -n nfc-common-lib
Summary:    NFC common library
Group:      Development/Libraries


%description -n nfc-common-lib
A library for Tizen NFC Framework and Tizen NFC client library.


%package -n nfc-common-lib-devel
Summary:    NFC common library (devel)
Group:      Network & Connectivity/Development
Requires:   nfc-common-lib = %{version}-%{release}


%description -n nfc-common-lib-devel
This package contains the development files for NFC Common library.


%build
export CFLAGS="$CFLAGS -DTIZEN_DEBUG_ENABLE"
export CXXFLAGS="$CXXFLAGS -DTIZEN_DEBUG_ENABLE"
export FFLAGS="$FFLAGS -DTIZEN_DEBUG_ENABLE"
export CFLAGS="$CFLAGS -DTIZEN_TELEPHONY_ENABLED"

export LDFLAGS+="-Wl,--rpath=%{_prefix}/lib -Wl,--as-needed"
LDFLAGS="$LDFLAGS" cmake . \
		-DTIZEN_ENGINEER_MODE=1 \
		-DCMAKE_INSTALL_PREFIX=%{_prefix} \
		-DTIZEN_TELEPHONY_ENABLED=1 \
%ifarch aarch64 x86_64
		-DTIZEN_ARCH_64=1 \
%endif

%install
%make_install

mkdir -p %{buildroot}%{_libdir}/systemd/system/multi-user.target.wants
cp -af %{SOURCE1} %{buildroot}%{_libdir}/systemd/system/
cp -af %{SOURCE2} %{buildroot}%{_libdir}/systemd/system/

mkdir -p %{buildroot}%{_libdir}/systemd/system/multi-user.target.wants
ln -s ../%{name}.service %{buildroot}%{_libdir}/systemd/system/multi-user.target.wants/%{name}.service

install -D -m 0644 LICENSE.Flora  %{buildroot}/%{_datadir}/license/nfc-common-lib
install -D -m 0644 LICENSE.Flora  %{buildroot}/%{_datadir}/license/%{name}
#install -D -m 0644 LICENSE.Flora  %{buildroot}/%{_datadir}/license/nfc-client-test


%post
/sbin/ldconfig

mkdir -p -m 777 /opt/usr/data/nfc-manager-daemon
chown network_fw:network_fw /opt/usr/data/nfc-manager-daemon
chsmack -a "System" /opt/usr/data/nfc-manager-daemon

mkdir -p -m 744 /opt/usr/share/nfc_debug
chown network_fw:network_fw /opt/usr/share/nfc_debug
chsmack -a "System" /opt/usr/share/nfc_debug

mkdir -p -m 744 /opt/usr/share/nfc-manager-daemon
chown network_fw:network_fw /opt/usr/share/nfc-manager-daemon
chsmack -a "System" /opt/usr/share/nfc-manager-daemon

mkdir -p -m 744 /opt/usr/share/nfc-manager-daemon/message
chown network_fw:network_fw /opt/usr/share/nfc-manager-daemon/message
chsmack -a "System" /opt/usr/share/nfc-manager-daemon/message

systemctl daemon-reload
if [ $1 == 1 ]; then
    systemctl restart %{name}.service
fi

%postun
/sbin/ldconfig

if [ $1 == 0 ]; then
    systemctl stop %{name}.service
fi
systemctl daemon-reload


%post -n nfc-common-lib -p /sbin/ldconfig


%postun -n nfc-common-lib -p /sbin/ldconfig


%files
%manifest %{name}.manifest
%defattr(-,root,root,-)
%{_bindir}/nfc-manager-daemon
%{_libdir}/systemd/system/%{name}.service
%{_libdir}/systemd/system/%{name}.socket
%{_libdir}/systemd/system/multi-user.target.wants/%{name}.service
%{_datadir}/dbus-1/system-services/org.tizen.NetNfcService.service
%{_datadir}/license/%{name}


%files -n nfc-common-lib
%manifest nfc-common-lib.manifest
%defattr(-,root,root,-)
%{_libdir}/libnfc-common-lib.so
%{_libdir}/libnfc-common-lib.so.*
%{_datadir}/license/nfc-common-lib
%{_datadir}/nfc-manager-daemon/sounds/Operation_sdk.wav


%files -n nfc-common-lib-devel
%manifest nfc-common-lib-devel.manifest
%defattr(-,root,root,-)
%{_libdir}/pkgconfig/nfc-common-lib.pc
%{_includedir}/*.h
