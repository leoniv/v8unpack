/////////////////////////////////////////////////////////////////////////////
//
//
//	Author:			disa_da
//	E-mail:			disa_da2@mail.ru
//
//
/////////////////////////////////////////////////////////////////////////////

// V8File.h: interface for the CV8File class.
//
//////////////////////////////////////////////////////////////////////

/*#if !defined(AFX_V8FILE_H__935D5C2B_70FA_45F2_BDF2_A0274A8FD60C__INCLUDED_)
#define AFX_V8FILE_H__935D5C2B_70FA_45F2_BDF2_A0274A8FD60C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000*/

#define MIN(X,Y) ((X) < (Y) ? (X) : (Y))
#define MAX(X,Y) ((X) > (Y) ? (X) : (Y))

#include "stdio.h"
#include "windows.h"
#include "assert.h"
#include "sys/stat.h"
#include "malloc.h"
#include "direct.h"
#include "io.h"
#include "string"
#include "zlib.h"



#define CHUNK 16384
#ifndef DEF_MEM_LEVEL
#  if MAX_MEM_LEVEL >= 8
#    define DEF_MEM_LEVEL 8
#  else
#    define DEF_MEM_LEVEL  MAX_MEM_LEVEL
#  endif
#endif


#define V8UNPACK_ERROR -50
#define V8UNPACK_NOT_V8_FILE (V8UNPACK_ERROR-1)
#define V8UNPACK_HEADER_ELEM_NOT_CORRECT (V8UNPACK_ERROR-2)


#define V8UNPACK_INFLATE_ERROR (V8UNPACK_ERROR-20)
#define V8UNPACK_INFLATE_IN_FILE_NOT_FOUND (V8UNPACK_INFLATE_ERROR-1)
#define V8UNPACK_INFLATE_OUT_FILE_NOT_CREATED (V8UNPACK_INFLATE_ERROR-2)
#define V8UNPACK_INFLATE_DATAERROR (V8UNPACK_INFLATE_ERROR-3)

#define V8UNPACK_DEFLATE_ERROR (V8UNPACK_ERROR-30)
#define V8UNPACK_DEFLATE_IN_FILE_NOT_FOUND (V8UNPACK_ERROR-1)
#define V8UNPACK_DEFLATE_OUT_FILE_NOT_CREATED (V8UNPACK_ERROR-2)

#define SHOW_USAGE -22

class CV8Elem;


class CV8File  
{
public:
	int SaveBlockDataToBuffer(BYTE** Buffer, BYTE* pBlockData, UINT BlockDataSize, UINT PageSize = 512);
	int GetData(BYTE **DataBufer, ULONG *DataBuferSize);
	int Pack();
	int SaveFile(char *filename);
	int SetElemName(CV8Elem &Elem, char *ElemName, UINT ElemNameLen);
	int Build(char* dirname, char* filename, int level = 0);
	int LoadFileFromFolder(char* dirname);
	int GetElemName(CV8Elem &Elem, char* ElemName, UINT *ElemNameLen);
	int Parse(char *filename, char *dirname, int level = 0);

	bool IsV8File(BYTE *pFileData, ULONG FileDataSize);

	int BuildCfFile(char *dirname, char *filename);

	struct stFileHeader
	{
		DWORD next_page_addr;
		DWORD page_size;
		DWORD storage_ver;
		DWORD reserved; // всегда 0x00000000 ?
		static const UINT Size()
		{
			return 4 + 4 + 4 + 4;
		}
	};

	struct stElemAddr
	{
		DWORD elem_header_addr;
		DWORD elem_data_addr;
		DWORD fffffff; //всегда 0x7fffffff ?
		static const UINT Size()
		{
			return 4 + 4 + 4;
		}
	};

	struct stBlockHeader
	{
		char EOL_0D;
		char EOL_0A;
		char data_size_hex[8];
		char space1;
		char page_size_hex[8];
		char space2;
		char next_page_addr_hex[8];
		char space3;
		char EOL2_0D;
		char EOL2_0A;
		static const UINT Size()
		{
			return 1 + 1 + 8 + 1 + 8 + 1 + 8 + 1 + 1 + 1;
		};
	};

	void GetErrorMessage(int ret);


	int Deflate(FILE *source, FILE *dest);
	int Inflate(FILE *source, FILE *dest);

	int Deflate(char *in_filename, char *out_filename);
	int Inflate(char *in_filename, char *out_filename);

	int Deflate(unsigned char* in_buf, unsigned char** out_buf, unsigned long in_len, unsigned long* out_len);
	int Inflate(unsigned char* in_buf, unsigned char** out_buf, unsigned long in_len, unsigned long* out_len);

	int LoadFile(BYTE *pFileData, ULONG FileData, bool boolInflate = true, bool UnpackWhenNeed = false);

	int UnpackToFolder(char *filename, char *dirname, char *block_name = NULL, bool print_progress = false);

	DWORD _httoi(char *value);

	int ReadBlockData(BYTE *pFileData, stBlockHeader *pBlockHeader, BYTE *&pBlockData, UINT *BlockDataSize = NULL);
	int PackFromFolder(char *dirname, char *filename);

	int SaveBlockData(FILE *file_out, BYTE *pBlockData, UINT BlockDataSize, UINT PageSize = 512);

	int SaveFileToFolder(char *dirname);

	int PackElem(CV8Elem &pElem);


	CV8File();
	CV8File(BYTE *pFileData, bool boolUndeflate = true);
	virtual ~CV8File();

	stFileHeader	FileHeader;
	stElemAddr		*pElemsAddrs;
	UINT			ElemsNum;
	CV8Elem			*pElems;
	bool			IsDataPacked;
};

class CV8Elem
{
public:

	struct stElemHeaderBegin
	{
		ULONGLONG date_creation;
		ULONGLONG date_modification;
		DWORD res; // всегда 0x000000?
		//изменяемая длина имени блока
		//после имени DWORD res; // всегда 0x000000?
		static const UINT Size()
		{
			return 8 + 8 + 4;
		};
	};

	BYTE	*pHeader;
	UINT	HeaderSize;
	BYTE	*pData;
	UINT	DataSize;
	CV8File UnpackedData;
	bool	IsV8File;
	bool	NeedUnpack;

	CV8Elem()
	{
		pHeader = NULL;
		pData = NULL;
		IsV8File = false;
		HeaderSize = 0;
		DataSize = 0;
	}

	~CV8Elem()
	{
		if (pHeader)
			delete[] pHeader;

		if (pData)
			delete[] pData;
	}
};