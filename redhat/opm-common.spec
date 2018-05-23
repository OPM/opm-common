#
# spec file for package opm-common
#

%define tag final

Name:           opm-common
Version:        2018.04
Release:        0
Summary:        Open Porous Media - common helpers and buildsystem
License:        GPL-3.0
Group:          Development/Libraries/C and C++
Url:            http://www.opm-project.org/
Source0:        https://github.com/OPM/%{name}/archive/release/%{version}/%{tag}.tar.gz#/%{name}-%{version}.tar.gz
BuildRequires:  git doxygen bc devtoolset-6-toolchain ecl-devel openmpi-devel zlib-devel
%{?el6:BuildRequires: cmake3 boost148-devel}
%{!?el6:BuildRequires: cmake boost-devel}
BuildRoot:      %{_tmppath}/%{name}-%{version}-build

%description
The Open Porous Media (OPM) initiative provides a set of open-source tools centered around the simulation of flow and transport of fluids in porous media. The goal of the initiative is to establish a sustainable environment for the development of an efficient and well-maintained software suite.

%package -n libopm-common1
Summary: OPM-common - library
Group:          System/Libraries

%description -n libopm-common1
This package contains library for opm-common

%package -n libopm-common1-openmpi
Summary: OPM-common - library
Group:          System/Libraries

%description -n libopm-common1-openmpi
This package contains library for opm-common

%package devel
Summary:        Development and header files for opm-common
Group:          Development/Libraries/C and C++
Requires:       %{name} = %{version}

%description devel
This package contains the development and header files for opm-common

%package openmpi-devel
Summary:        Development and header files for opm-common
Group:          Development/Libraries/C and C++
Requires:       %{name} = %{version}
Requires:       libopm-common1-openmpi = %{version}

%description openmpi-devel
This package contains the development and header files for opm-common

%package bin
Summary:        Applications for opm-common
Group:          System/Binaries
Requires:       %{name} = %{version}
Requires:	libopm-common1 = %{version}

%description bin
This package the applications for opm-common

%package openmpi-bin
Summary:        Applications for opm-common
Group:          System/Binaries
Requires:       %{name} = %{version}
Requires:       libopm-common1-openmpi = %{version}

%description openmpi-bin
This package the applications for opm-common

%package doc
Summary:        Documentation files for opm-common
Group:          Documentation
BuildArch:	noarch

%description doc
This package contains the documentation files for opm-common

%prep
%setup -q -n %{name}-release-%{version}-%{tag}

# consider using -DUSE_VERSIONED_DIR=ON if backporting
%build
scl enable devtoolset-6 bash
mkdir serial
cd serial
%{?el6:cmake3} %{?!el6:cmake} -DBUILD_SHARED_LIBS=1 -DCMAKE_BUILD_TYPE=RelWithDebInfo  -DSTRIP_DEBUGGING_SYMBOLS=ON -DCMAKE_INSTALL_PREFIX=%{_prefix} -DCMAKE_INSTALL_DOCDIR=share/doc/%{name}-%{version} -DUSE_RUNPATH=OFF -DWITH_NATIVE=OFF -DCMAKE_CXX_COMPILER=/opt/rh/devtoolset-6/root/usr/bin/g++ -DCMAKE_C_COMPILER=/opt/rh/devtoolset-6/root/usr/bin/gcc -DCMAKE_Fortran_COMPILER=/opt/rh/devtoolset-6/root/usr/bin/gfortran %{?el6:-DBOOST_LIBRARYDIR=%{_libdir}/boost148 -DBOOST_INCLUDEDIR=%{_includedir}/boost148} ..
make
make test
cd ..

mkdir openmpi
cd openmpi
%{?el6:module load openmpi-x86_64}
%{?!el6:module load mpi/openmpi-x86_64}
%{?el6:cmake3} %{?!el6:cmake} -DUSE_MPI=1 -DBUILD_SHARED_LIBS=1 -DCMAKE_BUILD_TYPE=RelWithDebInfo  -DSTRIP_DEBUGGING_SYMBOLS=ON -DCMAKE_INSTALL_PREFIX=%{_prefix}/lib64/openmpi -DUSE_RUNPATH=OFF -DWITH_NATIVE=OFF -DCMAKE_CXX_COMPILER=/opt/rh/devtoolset-6/root/usr/bin/g++ -DCMAKE_C_COMPILER=/opt/rh/devtoolset-6/root/usr/bin/gcc -DCMAKE_Fortran_COMPILER=/opt/rh/devtoolset-6/root/usr/bin/gfortran %{?el6:-DBOOST_LIBRARYDIR=%{_libdir}/boost148 -DBOOST_INCLUDEDIR=%{_includedir}/boost148} ..
make
make test

%install
cd serial
make install DESTDIR=${RPM_BUILD_ROOT}
make install-html DESTDIR=${RPM_BUILD_ROOT}
cd ..
cd openmpi
make install DESTDIR=${RPM_BUILD_ROOT}
mv ${RPM_BUILD_ROOT}/usr/lib64/openmpi/include/* ${RPM_BUILD_ROOT}/usr/include/openmpi-x86_64/

%clean
rm -rf %{buildroot}

%files
%doc README.md

%files doc
%{_docdir}/*

%files bin
%{_bindir}/*

%files openmpi-bin
%{_libdir}/openmpi/bin/*

%files -n libopm-common1
%defattr(-,root,root,-)
%{_libdir}/*.so.*

%files -n libopm-common1-openmpi
%defattr(-,root,root,-)
%{_libdir}/openmpi/lib64/*.so.*

%files devel
%defattr(-,root,root,-)
/usr/lib/dunecontrol/*
%{_libdir}/pkgconfig/*
%{_includedir}/*
%{_datadir}/cmake/*
%{_datadir}/opm/*
%{_libdir}/*.so

%files openmpi-devel
%defattr(-,root,root,-)
%{_libdir}/openmpi/lib/dunecontrol/*
%{_libdir}/openmpi/lib64/pkgconfig/*
%{_includedir}/openmpi-x86_64/*
%{_libdir}/openmpi/share/cmake/*
%{_libdir}/openmpi/share/opm/*
%{_libdir}/openmpi/lib64/*.so
