#include <Windows.h>
#include <tchar.h>
#include <fstream>
#include <sstream>
using namespace std;
static int dot_graph_count = 0;

void dot_graph(stringstream &dot)
{
//#ifndef _DEBUG
//	return;
//#endif
    TCHAR szTempPath[MAX_PATH],szTempfile[MAX_PATH],filename[MAX_PATH];
    GetTempPath(MAX_PATH, szTempPath);
    GetTempFileName(szTempPath,_T("my_"),0,szTempfile);

    ofstream f(szTempfile);
    if (!f)
    {
        return;
    }
    f<<dot.str();
    f.close();

    // execute
    STARTUPINFO  si;
    PROCESS_INFORMATION pi;

    ZeroMemory( &si, sizeof(si) );
    si.cb=sizeof(si);
    si.dwFlags   =   STARTF_USESHOWWINDOW ;
    si.wShowWindow   =   SW_HIDE;
    ZeroMemory( &pi, sizeof(pi) );

    GetTempPath(MAX_PATH, szTempPath);
    wsprintf(filename,_T("%sDOT_GRAPH%d.png"),szTempPath,dot_graph_count++);

    //DeleteFile(szTempPath);
    TCHAR cmd[MAX_PATH*2];
    wsprintf(cmd,_T("c:\\kuaipan\\lib\\Graphviz2.27\\bin\\dot.exe -Tpng \"%s\" -o \"%s\""),szTempfile,filename);

    if (!CreateProcess(NULL, cmd, NULL, NULL, FALSE,  0, NULL, NULL, &si, &pi))
    {
        return ;
    }

    WaitForSingleObject(pi.hProcess,  INFINITE);
    CloseHandle( pi.hProcess );
    CloseHandle( pi.hThread );

    ShellExecute(NULL,_T("open"),filename,NULL,NULL,SW_SHOWNORMAL);
    //DeleteFile(szTempfile);
}

void gdl_graph(stringstream &dot)
{
    TCHAR szTempPath[MAX_PATH],szTempfile[MAX_PATH];
    GetTempPath(MAX_PATH, szTempPath);
    GetTempFileName(szTempPath,_T("my_"),0,szTempfile);
    lstrcat(szTempfile,_T(".gdl"));

    ofstream f(szTempfile);
    if (!f)
    {
        return;
    }
    f<<dot.str();
    f.close();

    ShellExecute(NULL,_T("open"),szTempfile,NULL,NULL,SW_SHOWNORMAL);
}

/* ===============================================================================*/
/*
/*      Convert Relative Virtual Address to offset in the file
/*          works fine even in naughty binaries
/*          BeatriX manufacture :)
/*
/* ===============================================================================*/

int RVA2OFFSET(int RVA, unsigned char *pBuff)
{
    int RawSize, VirtualBorneInf, RawBorneInf, SectionHeader;
    int OffsetNtHeaders, OffsetSectionHeaders,
        NumberOfSections, SizeOfOptionalHeaders, VirtualAddress;

    OffsetNtHeaders = (int) * ((int *) (pBuff + 0x3c));
    NumberOfSections = (int) * ((unsigned short *) (pBuff + OffsetNtHeaders + 6));
    SizeOfOptionalHeaders = (int) * ((unsigned short *) (pBuff + OffsetNtHeaders + 0x14));
    OffsetSectionHeaders = OffsetNtHeaders + SizeOfOptionalHeaders + 0x18;
    VirtualBorneInf = 0;
    RawBorneInf = 0;
    VirtualAddress = 0;
    SectionHeader = 0;
    while (VirtualAddress <= RVA)
    {
        if (VirtualAddress != 0)
        {
            VirtualBorneInf = VirtualAddress;
            RawSize = (int) * ((unsigned int *) (pBuff + OffsetSectionHeaders + 0x10));
            RawBorneInf = (int) * ((unsigned int *) (pBuff + OffsetSectionHeaders + 0x14));
        }
        VirtualAddress = (int) * ((unsigned int *) (pBuff + OffsetSectionHeaders
                                  + SectionHeader * 0x28 + 0x0C));
        SectionHeader ++;
    }
    if ((RVA - VirtualBorneInf) > RawSize) return -1;
    RawBorneInf = RawBorneInf >> 8;
    if (RawBorneInf & 1) RawBorneInf--;
    RawBorneInf = RawBorneInf << 8;
	
    return RVA - VirtualBorneInf + RawBorneInf + (int) pBuff;
}