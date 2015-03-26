#include <stdio.h>
#include <Windows.h>
#include <tchar.h>
#include <Dbghelp.h>
typedef struct
{
    unsigned short usMagic;
    unsigned short usNumSec;
    unsigned long  ulTime;
    unsigned long  ulSymbolOffset;
    unsigned long  ulNumSymbol;
    unsigned short usOptHdrSZ;
    unsigned short usFlags;
} FILEHDR;

DWORD out_fn_count;

extern void* disasm(byte *bin,int length,bool for_lib,char* lib_name,bool is_inlined_function=true,char* from_file=NULL);;

//big-endian <-> little-endian
DWORD convert(DWORD a)
{
    return (a & 0x000000FF)<<24 | (a & 0x0000FF00)<<8 | (a & 0x00FF0000) >>8 | (a & 0xFF000000) >>24;
}

//trim
void ltrim(char* src,int start_pos,char c)
{
    int i=start_pos;
    while (src[i]==c && i>=0) i--;
    src[i+1]='\0';
}

void handle_coff(char* obj_start,TCHAR *filename)
{
    DWORD i;
    char* fn_name;

    FILEHDR* fh = (FILEHDR*)obj_start;
	if (fh->usMagic != 0x014c) return ;
    PIMAGE_SECTION_HEADER sec_header = (PIMAGE_SECTION_HEADER)(obj_start + sizeof(FILEHDR) + fh->usOptHdrSZ);
    PIMAGE_SYMBOL pis = (PIMAGE_SYMBOL)(fh->ulSymbolOffset + obj_start);
    char* string_table = fh->ulSymbolOffset + obj_start + fh->ulNumSymbol*sizeof(IMAGE_SYMBOL);
    i = 0;
    for (i=0;i<fh->ulNumSymbol;i++)
    {		
		if ((pis->SectionNumber>0) && (ISFCN(pis->Type)) 
			&& pis->StorageClass == IMAGE_SYM_CLASS_EXTERNAL
			)
        {
            fn_name = pis->N.Name.Short?(char*)(pis->N.ShortName):(string_table + pis->N.Name.Long);
			char fn_name2[1024];
			int j;
			for (j=0;;j++)
			{
				if (fn_name[j]==0){
					break;
				}
				if (!isascii(fn_name[j]))
				{
					break;
				}
				fn_name2[j]=fn_name[j];
			}
			fn_name2[j]=0;
			//TCHAR szUndecorateName[256];  
			//memset(szUndecorateName,0,256); 
			//UnDecorateSymbolName(fn_name,szUndecorateName,256,0); 
			disasm((byte*)obj_start+sec_header[pis->SectionNumber-1].PointerToRawData+pis->Value,
				sec_header[pis->SectionNumber-1].SizeOfRawData - pis->Value,
				true,
				fn_name2,
				//szUndecorateName,
				false,
				filename);

            //printf("OK!\n");
        }
        pis++;
    }
}

bool build_from_coff(TCHAR *filename)
{
	HANDLE hFile;
    HANDLE hMapping;
    LPVOID ImageBase;
    char tmp[32];
    DWORD i;
	DWORD first_sec_size;
	PIMAGE_ARCHIVE_MEMBER_HEADER pamh;
	char * p;
	DWORD obj_num;
	DWORD* obj_offset;
	DWORD third_sec_size;
	char* longname_offset;
	char* longname_sec;
	DWORD second_sec_size;

    hFile=CreateFile(filename,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,
                     FILE_ATTRIBUTE_NORMAL,0);

    if (!hFile)
        return false;

    hMapping=CreateFileMapping(hFile,NULL,PAGE_READONLY,0,0,NULL);
    if(!hMapping)
    {
        CloseHandle(hFile);
        return false;
    }
    ImageBase=MapViewOfFile(hMapping,FILE_MAP_READ,0,0,0);
    if(!ImageBase)
    {
        CloseHandle(hMapping);
        CloseHandle(hFile);
        return false;
    }

    if (memcmp(ImageBase,IMAGE_ARCHIVE_START,8))
    {
        printf("invalid file format!\n");;
        goto __ex;
    }

    //first section size
    pamh = (PIMAGE_ARCHIVE_MEMBER_HEADER)((char*)ImageBase + IMAGE_ARCHIVE_START_SIZE);
    memcpy(tmp,pamh->Size,sizeof(pamh->Size));
    ltrim(tmp,sizeof(pamh->Size)-1,'\x20');
    first_sec_size = atol(tmp);

    i = convert(*(DWORD*)((char*)ImageBase + IMAGE_ARCHIVE_START_SIZE + IMAGE_SIZEOF_ARCHIVE_MEMBER_HDR));
    out_fn_count =0;

    // goto second section
    p = ((char*)ImageBase + IMAGE_ARCHIVE_START_SIZE + IMAGE_SIZEOF_ARCHIVE_MEMBER_HDR + first_sec_size);
    pamh = (PIMAGE_ARCHIVE_MEMBER_HEADER) p;
    while (memcmp(pamh->EndHeader,IMAGE_ARCHIVE_END,2))
    {
        p++;
        pamh = (PIMAGE_ARCHIVE_MEMBER_HEADER) p;
    }
    memcpy(tmp,pamh->Size,sizeof(pamh->Size));
    ltrim(tmp,sizeof(pamh->Size)-1,'\x20');
    second_sec_size = atol(tmp);
    p += IMAGE_SIZEOF_ARCHIVE_MEMBER_HDR;
    obj_num = *(DWORD *)p;
    p +=sizeof(DWORD);
    obj_offset = (DWORD*)p;

    longname_sec = ((char*)ImageBase + IMAGE_ARCHIVE_START_SIZE + IMAGE_SIZEOF_ARCHIVE_MEMBER_HDR + first_sec_size + IMAGE_SIZEOF_ARCHIVE_MEMBER_HDR + second_sec_size);
    pamh = (PIMAGE_ARCHIVE_MEMBER_HEADER) longname_sec;
    while (memcmp(pamh->EndHeader,IMAGE_ARCHIVE_END,2))
    {
        longname_sec++;
        pamh = (PIMAGE_ARCHIVE_MEMBER_HEADER) longname_sec;
    }
    memcpy(tmp,pamh->Size,sizeof(pamh->Size));
    ltrim(tmp,sizeof(pamh->Size)-1,'\x20');
    third_sec_size = atol(tmp);
   // printf("third section size: %X\n",third_sec_size);
    longname_offset = 0;
    if (memcmp(pamh->Name,IMAGE_ARCHIVE_LONGNAMES_MEMBER,sizeof(pamh->Name))==0)
    {
        longname_offset = longname_sec + IMAGE_SIZEOF_ARCHIVE_MEMBER_HDR;
    }

    // process each obj
    for (i=0; i<obj_num; i++) //414
    {
        char* obj_sec = (char*)ImageBase+obj_offset[i];
        pamh = (PIMAGE_ARCHIVE_MEMBER_HEADER) obj_sec;
        if (memcmp(pamh->EndHeader,IMAGE_ARCHIVE_END,2))
        {
            printf("error!\n");
            goto __ex;
        }

        char* obj_start = (obj_sec+IMAGE_SIZEOF_ARCHIVE_MEMBER_HDR);
		handle_coff(obj_start,filename);
    }
	
__ex:
    if(ImageBase)
        UnmapViewOfFile(ImageBase);

    if(hMapping)
        CloseHandle(hMapping);

    if(hFile)
        CloseHandle(hFile);

    return true;
}