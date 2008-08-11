# The source cannot be distributed:
%{!?nosrc: %define nosrc 0}
# to include the source use:
# rpm -bs --define 'nosrc 0'

# If not provided, use default distribution tag
%{!?disttag: %define disttag %{dist}}

# The following options are supported:
#   --with prefix=prefix # location of root
#   --with username=username # user
#   --with groupname=groupname # group
#   --with[out] default_version - makes this version the default
# with the following defaults:
%define prefix /usr/local/packages
%define username root
%define groupname root
%define default_version 0

%{?_with_prefix:%define prefix %(set -- %{_with_prefix}&& echo $2 | cut -d "=" -f 2)}
%{?_with_username:%define username %(set -- %{_with_username}&& echo $2 | cut -d "=" -f 2)}
%{?_with_groupname:%define groupname %(set -- %{_with_groupname}&& echo $2 | cut -d "=" -f 2)}

# --with/--without processing
# first, error if conflicting options are used
%{?_with_default_version: %{?_without_default_version: %{error: both _with_default_version and _without_default_version}}}

# did we find any --with options?
%{?_with_default_version: %define default_version 1}

# did we find any --without options?
%{?_without_default_version: %define default_version 0}

%ifarch ppc64 sparc64 x86_64 ia64
%define bitness 64
%define is64bit 1
%define lib lib64
%else
%define bitness 32
%define is64bit 0
%define lib lib
%endif

%define program_name tau

Summary: TAU Portable Profiling and Tracing Package
Name: TAU
Version: 2.17.2
Release: 1.%{disttag}
Group: Development/Tools
License: Copyright (c) 1999 University of Oregon, Los Alamos National Laboratory
Source0: http://www.sfr-fresh.com/unix/misc/%{program_name}-%{version}.tar.gz
URL: http://www.cs.uoregon.edu/research/tau
# The source cannot be distributed:
%if %{nosrc}
NoSource: 0
%endif
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-buildroot
# put doc into a custom place
%define docdirectory %{prefix}/share/%{name}
####################################
%if %{is64bit}
%define libdir %{prefix}/lib64
%else
%define libdir %{prefix}/lib
%endif
%define bindir %{prefix}/bin
%define includedir %{prefix}/include
%define mandir %{prefix}/share/man
####################################

Packager: Sameer Shende <sameer@cs.uoregon.edu>
Prefix: %{prefix}
BuildRequires: wget unzip gzip tar tcsh
# skel files contain /bin/@TAUSHELL@ and /bin/@HELL@
Provides: /bin/@TAUSHELL@

%description
TAU (Tuning and Analysis Utilities) provides a framework for integrating
program and performance analysis tools and components. A core tool
component for parallel performance evaluation is a profile measurement and
analysis package. The TAU portable profiling and tracing package was developed jointly by the University of Oregon and LANL for profiling parallel, multi-threaded C++ programs. The package implements a instrumentation library, profile analysis procedures, and a visualization tool. 

It runs on SGI Origin 2000s, Intel PCs running Linux, Sun, Compaq Alpha Tru64,
Compaq Alpha Linux clusters, HP workstations and Cray T3E. The current release 
(v2.6) supports C++, C and Fortran90. It works  with KAI KCC, PGI, g++, egcs, 
Fujitsu and vendor supplied C++ compilers. See INSTALL and README files 
included with the distribution. Documentation can be found at 
http://www.cs.uoregon.edu/research/tau

- Sameer Shende (University of Oregon)
Report bugs to tau-bugs@cs.uoregon.edu

%prep
if [ "$RPM_BUILD_ROOT" != "/" ]; then
    %{__rm} -rf $RPM_BUILD_ROOT
fi

%setup -q -n %{program_name}-%{version}

%build
# Try and figure out architecture
detectarch=unknown
detectarch=`./utils/archfind`
if [ "x$detectarch" = "x" ]
  then
    detectarch=unknown
fi
%define machine `echo $detectarch`
./configure 2>&1 | tee configure.log

cat <<EOF > $RPM_BUILD_DIR/%{program_name}-%{version}/%{name}-%{version}.sh
# NOTE: This is an automatically-generated file!  (generated by the
# %{name} RPM).  Any changes made here will be lost if the RPM is
# uninstalled or upgraded.

PA=%{prefix}

case \$TAUROOT in
  *\${PA}*)      ;;
  *?*)  TAUROOT=\${PA}:\${TAUROOT} ;;
  *)    TAUROOT=\${PA} ;;
esac
export TAUROOT

PA=%{prefix}/%{machine}/bin

case \$PATH in
  *\${PA}*)      ;;
  *?*)  PATH=\${PA}:\${PATH} ;;
  *)    PATH=\${PA} ;;
esac
export PATH

PA=%{mandir}

case \$MANPATH in
  *\${PA}*)      ;;
  *?*)  MANPATH=\${PA}:\${MANPATH} ;;
  *)    MANPATH=\${PA} ;;
esac
export MANPATH
EOF

cat <<EOF > $RPM_BUILD_DIR/%{program_name}-%{version}/%{name}-%{version}.csh
# NOTE: This is an automatically-generated file!  (generated by the
# %{name} RPM).  Any changes made here will be lost if the RPM is
# uninstalled or upgraded.

set PA=%{prefix}

if (\$?TAUROOT) then
    if ("\$TAUROOT" !~ *\${PA}*) then
        setenv TAUROOT \${PA}:\${TAUROOT}
    endif
else
    setenv TAUROOT \${PA}
endif

unset PA

set PA=%{prefix}/%{machine}/bin

if (\$?PATH) then
    if ("\$PATH" !~ *\${PA}*) then
        setenv PATH \${PA}:\${PATH}
    endif
else
    setenv PATH \${PA}
endif

unset PA

set PA=%{mandir}

if (\$?MANPATH) then
    if ("\$MANPATH" !~ *\${PA}*) then
        setenv MANPATH \${PA}:\${MANPATH}
    endif
else
    setenv MANPATH \${PA}
endif

unset PA
EOF

%install

# Try and figure out architecture
detectarch=unknown
detectarch=`$RPM_BUILD_DIR/%{program_name}-%{version}/utils/archfind`
if [ "x$detectarch" = "x" ]
  then
    detectarch=unknown
fi
%define machine `echo $detectarch`
# redefine lib and bin
####################################
%if %{is64bit}
%define libdir %{prefix}/%{machine}/lib64
%else
%define libdir %{prefix}/%{machine}/lib
%endif
%define bindir %{prefix}/%{machine}/bin
####################################

%{__make} install 2>&1 | tee make_install.log

./tau_validate --html %{machine} &> results_%{machine}.html

# move the files compiled for the current architecture
mkdir -p %{buildroot}%{prefix}
cp -rf $RPM_BUILD_DIR/%{program_name}-%{version}/%{machine} $RPM_BUILD_ROOT%{prefix}
cp -rf $RPM_BUILD_DIR/%{program_name}-%{version}/tools $RPM_BUILD_ROOT%{prefix}
cp -rf $RPM_BUILD_DIR/%{program_name}-%{version}/etc $RPM_BUILD_ROOT%{prefix}

# remove *.py? files because they contain hard-coded path!
for file in $RPM_BUILD_ROOT%{bindir}/*.pyc;
do
rm -f $file
done
for file in $RPM_BUILD_ROOT%{bindir}/*.pyo;
do
rm -f $file
done
# remove the hard-coded TAUROOT from all the scripts in bin
for file in $RPM_BUILD_ROOT%{bindir}/*;
do
# bash
query=`file ${file} | cut -d " " -f 2`
if [ "$query" = "Bourne" ];
then
sed -i "s#$RPM_BUILD_DIR/%{program_name}-%{version}#%{prefix}#g" $file
echo "replacing $RPM_BUILD_DIR/%{program_name}-%{version} by %{prefix} in $file"
fi
# python
query=`file ${file} | cut -d " " -f 3`
if [ "$query" = "python" ];
then
sed -i "s#$RPM_BUILD_DIR/%{program_name}-%{version}#%{prefix}#g" $file
echo "replacing $RPM_BUILD_DIR/%{program_name}-%{version} by %{prefix} in $file"
fi
done
# python again (based on py)
for file in $RPM_BUILD_ROOT%{bindir}/*.py;
do
sed -i "s#$RPM_BUILD_DIR/%{program_name}-%{version}#%{prefix}#g" $file
echo "replacing $RPM_BUILD_DIR/%{program_name}-%{version} by %{prefix} in $file"
done
# more files in lib
for file in $RPM_BUILD_ROOT%{libdir}/derby.properties $RPM_BUILD_ROOT%{libdir}/Makefile.tau;
do
sed -i "s#$RPM_BUILD_DIR/%{program_name}-%{version}#%{prefix}#g" $file
echo "replacing $RPM_BUILD_DIR/%{program_name}-%{version} by %{prefix} in $file"
done
# remove *skel files that reference /bin/@SHELL@
find "${RPM_BUILD_ROOT}%{prefix}/tools" -name "*.skel" | xargs grep "/bin/\@SHELL\@" | cut -d ":" -f 1 | xargs rm

mkdir -p %{buildroot}%{mandir}
cp -rf $RPM_BUILD_DIR/%{program_name}-%{version}/man/* $RPM_BUILD_ROOT%{mandir}

mkdir -p ${RPM_BUILD_ROOT}%{docdirectory}
cp -rf Changes COPYRIGHT CREDITS INSTALL LICENSE README* *.log results_%{machine}.html examples html ${RPM_BUILD_ROOT}%{docdirectory}

cp -f $RPM_BUILD_DIR/%{program_name}-%{version}/%{name}-%{version}.csh $RPM_BUILD_ROOT%{bindir}
cp -f $RPM_BUILD_DIR/%{program_name}-%{version}/%{name}-%{version}.sh $RPM_BUILD_ROOT%{bindir}

%if %{default_version}
[ ! -d %{buildroot}%{_sysconfdir}/profile.d ] && mkdir -p %{buildroot}%{_sysconfdir}/profile.d

cp -f $RPM_BUILD_DIR/%{program_name}-%{version}/%{name}-%{version}.csh $RPM_BUILD_ROOT%{_sysconfdir}/profile.d/%{name}.csh
cp -f $RPM_BUILD_DIR/%{program_name}-%{version}/%{name}-%{version}.sh $RPM_BUILD_ROOT%{_sysconfdir}/profile.d/%{name}.sh
%endif

%clean
if [ "$RPM_BUILD_ROOT" != "/" ]; then
    %{__rm} -rf $RPM_BUILD_ROOT
fi

%files
%defattr(-, %{username}, %{groupname}, 0755)
%doc %{mandir}/man*/*.*
%docdir %{docdirectory}
%{docdirectory}/*
%{prefix}/*/bin
%{prefix}/*/%{lib}
%{prefix}/tools
%{prefix}/etc
%if %{default_version}
%config %{_sysconfdir}/profile.d/%{name}.csh
%config %{_sysconfdir}/profile.d/%{name}.sh
%endif

%changelog
* Mon Aug 11 2008 Marcin Dulak <Marcin.Dulak@fysik.dtu.dk> 2.17.2-1
- system-wide installation available

* Fri Aug  8 2008 Marcin Dulak <Marcin.Dulak@fysik.dtu.dk> 2.17.1-1
- adopted for fys: based on tau-2.17.1/utils/TAU-2.4-0.spec
- not relocatable due to hard-coded paths in the scripts

* Fri Nov 11 2005 Sameer Shende <sameer@cs.uoregon.edu> 2.4-0
- initial release
