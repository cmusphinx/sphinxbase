# Microsoft Developer Studio Project File - Name="sphinxbase" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=sphinxbase - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "sphinxbase.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "sphinxbase.mak" CFG="sphinxbase - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "sphinxbase - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "sphinxbase - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "sphinxbase - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "../../lib/Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "SPHINXBASE_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "../../include/win32" /I "../../include" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "SPHINXDLL"  /D "SPHINXBASE_EXPORTS" /D "HAVE_CONFIG_H" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib winmm.lib /nologo /dll /machine:I386

!ELSEIF  "$(CFG)" == "sphinxbase - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "../../lib/Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "SPHINXBASE_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "../../include/win32" /I "../../include" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "SPHINXDLL" /D "SPHINXBASE_EXPORTS" /D "HAVE_CONFIG_H" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib winmm.lib /nologo /dll /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "sphinxbase - Win32 Release"
# Name "sphinxbase - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE="..\..\src\libsphinxfe\add-table.c"
# End Source File
# Begin Source File

SOURCE=..\..\src\libsphinxfeat\agc.c
# End Source File
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

SOURCE=..\..\src\libsphinxfeat\cmn.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libsphinxfeat\cmn_prior.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libsphinxad\cont_ad_base.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libsphinxutil\err.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libsphinxutil\f2c_lite.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libsphinxfe\fe_interface.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libsphinxfe\fe_sigproc.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libsphinxfe\fe_warp.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libsphinxfe\fe_warp_affine.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libsphinxfe\fe_warp_inverse_linear.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libsphinxfe\fe_warp_piecewise_linear.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libsphinxfeat\feat.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libsphinxutil\filename.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libsphinxfe\fixlog.c
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

SOURCE=..\..\src\libsphinxfeat\lda.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libsphinxutil\linklist.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libsphinxutil\matrix.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libsphinxad\mulaw_base.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libsphinxutil\pio.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libsphinxad\play_win32.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libsphinxutil\profile.c
# End Source File
# Begin Source File

SOURCE=..\..\src\libsphinxad\rec_win32.c
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
# Begin Source File

SOURCE=..\..\include\ad.h
# End Source File
# Begin Source File

SOURCE=..\..\include\agc.h
# End Source File
# Begin Source File

SOURCE=..\..\include\bio.h
# End Source File
# Begin Source File

SOURCE=..\..\include\bitvec.h
# End Source File
# Begin Source File

SOURCE=..\..\include\byteorder.h
# End Source File
# Begin Source File

SOURCE=..\..\include\case.h
# End Source File
# Begin Source File

SOURCE=..\..\include\ckd_alloc.h
# End Source File
# Begin Source File

SOURCE=..\..\include\clapack_lite.h
# End Source File
# Begin Source File

SOURCE=..\..\include\cmd_ln.h
# End Source File
# Begin Source File

SOURCE=..\..\include\cmn.h
# End Source File
# Begin Source File

SOURCE=..\..\include\win32\config.h
# End Source File
# Begin Source File

SOURCE=..\..\include\cont_ad.h
# End Source File
# Begin Source File

SOURCE=..\..\include\err.h
# End Source File
# Begin Source File

SOURCE=..\..\include\f2c.h
# End Source File
# Begin Source File

SOURCE=..\..\include\fe.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libsphinxfe\fe_internal.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libsphinxfe\fe_warp.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libsphinxfe\fe_warp_affine.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libsphinxfe\fe_warp_inverse_linear.h
# End Source File
# Begin Source File

SOURCE=..\..\src\libsphinxfe\fe_warp_piecewise_linear.h
# End Source File
# Begin Source File

SOURCE=..\..\include\feat.h
# End Source File
# Begin Source File

SOURCE=..\..\include\filename.h
# End Source File
# Begin Source File

SOURCE=..\..\include\fixpoint.h
# End Source File
# Begin Source File

SOURCE=..\..\include\genrand.h
# End Source File
# Begin Source File

SOURCE=..\..\include\glist.h
# End Source File
# Begin Source File

SOURCE=..\..\include\hash_table.h
# End Source File
# Begin Source File

SOURCE=..\..\include\heap.h
# End Source File
# Begin Source File

SOURCE=..\..\include\info.h
# End Source File
# Begin Source File

SOURCE=..\..\include\libutil.h
# End Source File
# Begin Source File

SOURCE=..\..\include\linklist.h
# End Source File
# Begin Source File

SOURCE=..\..\include\matrix.h
# End Source File
# Begin Source File

SOURCE=..\..\include\mulaw.h
# End Source File
# Begin Source File

SOURCE=..\..\include\pio.h
# End Source File
# Begin Source File

SOURCE=..\..\include\prim_type.h
# End Source File
# Begin Source File

SOURCE=..\..\include\profile.h
# End Source File
# Begin Source File

SOURCE=..\..\include\s3_arraylist.h
# End Source File
# Begin Source File

SOURCE=..\..\include\win32\sphinx_config.h
# End Source File
# Begin Source File

SOURCE=..\..\include\sphinx_types.h
# End Source File
# Begin Source File

SOURCE=..\..\include\strfuncs.h
# End Source File
# Begin Source File

SOURCE=..\..\include\unlimit.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
