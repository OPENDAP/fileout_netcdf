Summary: Return a NetCDF File for a DAP Data response
Name: fileout_netcdf
Version: 0.9.3
Release: 1
License: LGPLv2+
Group: System Environment/Daemons
URL: http://www.opendap.org/
Source0: http://www.opendap.org/pub/source/%{name}-%{version}.tar.gz

BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)
BuildRequires:   libdap-devel >= 3.9.0 netcdf-devel
BuildRequires:   bes-devel >= 3.7.0

%description
This is the fileout netCDF response handler for Hyrax - the OPeNDAP data
server. With this handler a server can easily be configured to return
data packaged in a netCDF 3.6.3 file.

%prep
%setup -q

%build
%configure --disable-static --disable-dependency-tracking
make %{?_smp_mflags}

%install
rm -rf $RPM_BUILD_ROOT
make DESTDIR=$RPM_BUILD_ROOT install INSTALL="install -p"

rm $RPM_BUILD_ROOT%{_libdir}/bes/*.la

%clean
rm -rf $RPM_BUILD_ROOT

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%files
%defattr(-,root,root,-)
%{_bindir}/bes-fonc-data.sh
%{_libdir}/bes/libfonc_module.so
%doc COPYING COPYRIGHT NEWS README

%changelog
* Mon Mar 16 2009 James Gallagher <jgallagher@opendap.org> - 
- Initial build.

