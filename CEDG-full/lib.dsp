# Microsoft Developer Studio Project File - Name="lib" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=lib - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "lib.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "lib.mak" CFG="lib - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "lib - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "lib - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "lib - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "lib___Win32_Release"
# PROP BASE Intermediate_Dir "lib___Win32_Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD BASE RSC /l 0x804 /d "NDEBUG"
# ADD RSC /l 0x804 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386 /out:"../Release/lib.exe"

!ELSEIF  "$(CFG)" == "lib - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "lib___Win32_Debug"
# PROP BASE Intermediate_Dir "lib___Win32_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "lib___Win32_Debug"
# PROP Intermediate_Dir "lib___Win32_Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD BASE RSC /l 0x804 /d "_DEBUG"
# ADD RSC /l 0x804 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /out:"../release/lib.exe" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "lib - Win32 Release"
# Name "lib - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Group "vflib"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\vflib2\src2\argedit.cpp
# End Source File
# Begin Source File

SOURCE=..\vflib2\src2\argloader.cpp
# End Source File
# Begin Source File

SOURCE=..\vflib2\src2\argraph.cpp
# End Source File
# Begin Source File

SOURCE=..\vflib2\src2\error.cpp
# End Source File
# Begin Source File

SOURCE=..\vflib2\src2\gene.cpp
# End Source File
# Begin Source File

SOURCE=..\vflib2\src2\gene_mesh.cpp
# End Source File
# Begin Source File

SOURCE=..\vflib2\src2\match.cpp
# End Source File
# Begin Source File

SOURCE=..\vflib2\src2\sd_state.cpp
# End Source File
# Begin Source File

SOURCE=..\vflib2\src2\sortnodes.cpp
# End Source File
# Begin Source File

SOURCE=..\vflib2\src2\sortnodes.h
# End Source File
# Begin Source File

SOURCE=..\vflib2\src2\ull_state.cpp
# End Source File
# Begin Source File

SOURCE=..\vflib2\src2\ull_sub_state.cpp
# End Source File
# Begin Source File

SOURCE=..\vflib2\src2\vf2_mono_state.cpp
# End Source File
# Begin Source File

SOURCE=..\vflib2\src2\vf2_state.cpp
# End Source File
# Begin Source File

SOURCE=..\vflib2\src2\vf2_sub_state.cpp
# End Source File
# Begin Source File

SOURCE=..\vflib2\src2\vf_mono_state.cpp
# End Source File
# Begin Source File

SOURCE=..\vflib2\src2\vf_state.cpp
# End Source File
# Begin Source File

SOURCE=..\vflib2\src2\vf_sub_state.cpp
# End Source File
# Begin Source File

SOURCE=..\vflib2\src2\xsubgraph.cpp
# End Source File
# End Group
# Begin Source File

SOURCE=.\BasicBlock.cpp
# End Source File
# Begin Source File

SOURCE=.\bitset.c
# End Source File
# Begin Source File

SOURCE=.\coff.cpp
# End Source File
# Begin Source File

SOURCE=.\ControlFlowGraph.cpp
# End Source File
# Begin Source File

SOURCE=.\DepGraphNode.cpp
# End Source File
# Begin Source File

SOURCE=.\instruction.cpp
# End Source File
# Begin Source File

SOURCE=.\InstructionComparator.cpp
# End Source File
# Begin Source File

SOURCE=.\Lock.cpp
# End Source File
# Begin Source File

SOURCE=.\main.cpp
# End Source File
# Begin Source File

SOURCE=.\PEFuncs.cpp
# End Source File
# Begin Source File

SOURCE=.\utility.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\BasicBlock.h
# End Source File
# Begin Source File

SOURCE=.\bitset.h
# End Source File
# Begin Source File

SOURCE=.\coff.h
# End Source File
# Begin Source File

SOURCE=.\ControlFlowGraph.h
# End Source File
# Begin Source File

SOURCE=.\define.h
# End Source File
# Begin Source File

SOURCE=.\DepGraphNode.h
# End Source File
# Begin Source File

SOURCE=.\disasm.h
# End Source File
# Begin Source File

SOURCE=.\inline_lib.h
# End Source File
# Begin Source File

SOURCE=.\instruction.h
# End Source File
# Begin Source File

SOURCE=.\InstructionComparator.h
# End Source File
# Begin Source File

SOURCE=.\Lock.h
# End Source File
# Begin Source File

SOURCE=.\PEFuncs.h
# End Source File
# Begin Source File

SOURCE=.\utility.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
