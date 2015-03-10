%define _unpackaged_files_terminate_build 0
Name:		xrootd-rados-oss
Version:	0.2.1
Release:	1
Summary:	XRootD OSS plugin for RADOS pools (CEPH)
Prefix:         /usr
Group:		Applications/File
License:	GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

Source:        %{name}-%{version}-%{release}.tar.gz
BuildRoot:     %{_tmppath}/%{name}-root

BuildRequires: cmake >= 2.6
BuildRequires: radosfs-devel >= 0.4
BuildRequires: xrootd-server-devel >= 4.0
BuildRequires: xrootd-private-devel >= 4.0

%if %{?_EXTRA_REQUIRES:1}%{!?_EXTRA_REQUIRES:0}
BuildRequires: %{_EXTRA_REQUIRES}
Requires:      %{_EXTRA_REQUIRES}
%endif

Requires: radosfs >= 0.5 xrootd4 >= 4.0


%description
XRootD OSS plugin for RADOS pools (CEPH)

%prep
%setup -n %{name}-%{version}-%{release}

%build
test -e $RPM_BUILD_ROOT && rm -r $RPM_BUILD_ROOT
%if 0%{?rhel} < 6
export CC=/usr/bin/gcc44 CXX=/usr/bin/g++44
%endif

%if %{?_BOOST_ROOT:1}%{!?_BOOST_ROOT:0}
export BOOST_ROOT=%{_BOOST_ROOT} 
%endif

mkdir -p build
cd build
cmake ../ -DRELEASE=%{release} -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBoost_NO_SYSTEM_PATHS=ON
%{__make} %{_smp_mflags} 

%install
cd build
%{__make} install DESTDIR=$RPM_BUILD_ROOT
echo "Installed!"

%post 
/sbin/ldconfig

%postun
/sbin/ldconfig

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root,-)
/usr/lib64/libRadosOss.so
/usr/lib64/libRadosOss.so


