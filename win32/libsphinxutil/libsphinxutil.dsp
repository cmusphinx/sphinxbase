# Microsoft Developer Studio Project File - Name="libsphinxutil" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=libsphinxutil - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "libsphinxutil.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "libsphinxutil.mak" CFG="libsphinxutil - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "libsphinxutil - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "libsphinxutil - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "libsphinxutil - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "../../lib/Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "../../include/win32" /I "../../include" /D "NDEBUG" /D "_LIB" /D "WIN32" /D "_MBCS" /D "HAVE_CONFIG_H" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "libsphinxutil - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "../../lib/Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "../../include/win32" /I "../../include" /D "_DEBUG" /D "_LIB" /D "WIN32" /D "_MBCS" /D "HAVE_CONFIG_H" /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "libsphinxutil - Win32 Release"
# Name "libsphinxutil - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\src\libsphinxutil\bio.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libsphinxutil\bitvec.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libsphinxutil\blas_lite.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libsphinxutil\case.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libsphinxutil\ckd_alloc.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libsphinxutil\cmd_ln.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libsphinxutil\err.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libsphinxutil\f2c_lite.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libsphinxutil\filename.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libsphinxutil\genrand.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libsphinxutil\glist.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libsphinxutil\hash_table.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libsphinxutil\heap.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libsphinxutil\info.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libsphinxutil\linklist.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libsphinxutil\matrix.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libsphinxutil\pio.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libsphinxutil\profile.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libsphinxutil\s3_arraylist.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libsphinxutil\slamch.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libsphinxutil\slapack_lite.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libsphinxutil\strfuncs.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libsphinxutil\unlimit.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# End Group
# End Target
# End Project
