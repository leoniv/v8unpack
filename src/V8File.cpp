/////////////////////////////////////////////////////////////////////////////
//
//
//	Author:			disa_da
//	E-mail:			disa_da2@mail.ru
//
//
/////////////////////////////////////////////////////////////////////////////

// V8File.cpp: implementation of the CV8File class.
//
//////////////////////////////////////////////////////////////////////


#include "V8File.h"


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CV8File::CV8File()
{
	pElemsAddrs = NULL;
	pElems = NULL;
	IsDataPacked = true;
}


CV8File::CV8File(BYTE *pFileData, bool boolInflate)
{
	LoadFile(pFileData, boolInflate);
}

CV8File::~CV8File()
{
	if (pElemsAddrs)
		delete pElemsAddrs;

	if (pElems)
		delete[] pElems;
}



int CV8File::Inflate(char *in_filename, char *out_filename)
{
	int ret;

	FILE *in_file = fopen(in_filename, "rb");
	if (!in_file)
		return V8UNPACK_INFLATE_IN_FILE_NOT_FOUND;
	
	FILE *out_file = fopen(out_filename, "wb");
	if (!out_file)
	{
		fclose(in_file);
		return V8UNPACK_INFLATE_OUT_FILE_NOT_CREATED;
	}

	ret = Inflate(in_file, out_file);

	fclose(in_file);
	fclose(out_file);

	if (ret == Z_DATA_ERROR)
		return V8UNPACK_INFLATE_DATAERROR;
	if (ret)
		return V8UNPACK_INFLATE_ERROR;

	return 0;
}

int CV8File::Deflate(char *in_filename, char *out_filename)
{

	int ret;

	FILE *in_file = fopen(in_filename, "rb");
	if (!in_file)
		return V8UNPACK_DEFLATE_IN_FILE_NOT_FOUND;

	FILE *out_file = fopen(out_filename, "wb");
	if (!out_file)
	{
		fclose(in_file);
		return V8UNPACK_DEFLATE_OUT_FILE_NOT_CREATED;
	}

	ret = Deflate(in_file, out_file);

	fclose(in_file);
	fclose(out_file);

	if (ret)
		return V8UNPACK_DEFLATE_ERROR;

	return 0;
}


int CV8File::Deflate(FILE *source, FILE *dest)
{

	int ret, flush;
	unsigned have;
	z_stream strm;
	unsigned char in[CHUNK];
	unsigned char out[CHUNK];

	// allocate deflate state
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	//ret = deflateInit(&strm, level);
	ret = deflateInit2(&strm, Z_BEST_COMPRESSION, Z_DEFLATED, -MAX_WBITS, DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY);

	if (ret != Z_OK)
		return ret;

	// compress until end of file 
	do {
		strm.avail_in = fread(in, 1, CHUNK, source);
		if (ferror(source)) {
			(void)deflateEnd(&strm);
			return Z_ERRNO;
		}
		flush = feof(source) ? Z_FINISH : Z_NO_FLUSH;
		strm.next_in = in;

		// run deflate() on input until output buffer not full, finish
		//   compression if all of source has been read in
		do {
			strm.avail_out = CHUNK;
			strm.next_out = out;
			ret = deflate(&strm, flush);    // no bad return value
			assert(ret != Z_STREAM_ERROR);  // state not clobbered 
			have = CHUNK - strm.avail_out;
			if (fwrite(out, 1, have, dest) != have || ferror(dest)) {
				(void)deflateEnd(&strm);
				return Z_ERRNO;
			}
		} while (strm.avail_out == 0);
		assert(strm.avail_in == 0);     // all input will be used

		// done when last data in file processed
	} while (flush != Z_FINISH);
	assert(ret == Z_STREAM_END);        // stream will be complete

	// clean up and return
	(void)deflateEnd(&strm);
	return Z_OK;

}

int CV8File::Inflate(FILE *source, FILE *dest)
{
	int ret;
	unsigned have;
	z_stream strm;
	unsigned char in[CHUNK];
	unsigned char out[CHUNK];

	// allocate inflate state
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	strm.avail_in = 0;
	strm.next_in = Z_NULL;
	//ret = inflateInit(&strm);
	ret = inflateInit2(&strm, -MAX_WBITS);
	if (ret != Z_OK)
		return ret;

	do {
		strm.avail_in = fread(in, 1, CHUNK, source);
		if (ferror(source)) {
			(void)inflateEnd(&strm);
			return Z_ERRNO;
		}
		if (strm.avail_in == 0)
			break;
		strm.next_in = in;

		// run inflate() on input until output buffer not full
		do {
			strm.avail_out = CHUNK;
			strm.next_out = out;
			ret = inflate(&strm, Z_NO_FLUSH);
			assert(ret != Z_STREAM_ERROR);  // state not clobbered
			switch (ret) {
			case Z_NEED_DICT:
				ret = Z_DATA_ERROR;     // and fall through
			case Z_DATA_ERROR:
			case Z_MEM_ERROR:
				(void)inflateEnd(&strm);
				return ret;
			}
			have = CHUNK - strm.avail_out;
			if (fwrite(out, 1, have, dest) != have || ferror(dest)) {
				(void)inflateEnd(&strm);
				return Z_ERRNO;
			}
		} while (strm.avail_out == 0);

		// done when inflate() says it's done
	} while (ret != Z_STREAM_END);

	// clean up and return
	(void)inflateEnd(&strm);
	return ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;

}

int CV8File::Inflate(unsigned char* in_buf, unsigned char** out_buf, unsigned long in_len, unsigned long* out_len)
{
	int ret;
	unsigned have;
	z_stream strm;
	unsigned char out[CHUNK];

	unsigned long out_buf_len = in_len + CHUNK;
	*out_buf = static_cast<unsigned char*> (realloc(*out_buf, out_buf_len));
	*out_len = 0; 


	// allocate inflate state
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	strm.avail_in = 0;
	strm.next_in = Z_NULL;
	ret = inflateInit2(&strm, -MAX_WBITS);
	if (ret != Z_OK)
		return ret;

	strm.avail_in = in_len;
	strm.next_in = in_buf;

	// run inflate() on input until output buffer not full
	do {
		strm.avail_out = CHUNK;
		strm.next_out = out;
		ret = inflate(&strm, Z_NO_FLUSH);
		assert(ret != Z_STREAM_ERROR);  // state not clobbered
		switch (ret) {
		case Z_NEED_DICT:
			ret = Z_DATA_ERROR;     // and fall through
		case Z_DATA_ERROR:
		case Z_MEM_ERROR:
			(void)inflateEnd(&strm);
			return ret;
		}
		have = CHUNK - strm.avail_out;
		if (*out_len + have > out_buf_len) 
		{
			//if (have < sizeof
			out_buf_len = out_buf_len + sizeof(out);
			*out_buf = static_cast<unsigned char*> (realloc(*out_buf, out_buf_len));
			if (!out_buf)
			{
				(void)deflateEnd(&strm);
				return Z_ERRNO;
			}
		}
		memcpy((*out_buf + *out_len), out, have);
		*out_len += have; 
	} while (strm.avail_out == 0);

	// done when inflate() says it's done

	// clean up and return
	(void)inflateEnd(&strm);
	return ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;
}

int CV8File::Deflate(unsigned char* in_buf, unsigned char** out_buf, unsigned long in_len, unsigned long* out_len)
{
	int ret, flush;
	unsigned have;
	z_stream strm;
	unsigned char out[CHUNK];

	unsigned long out_buf_len = in_len + CHUNK;
	*out_buf = static_cast<unsigned char*> (realloc(*out_buf, out_buf_len));
	*out_len = 0; 

	// allocate deflate state
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	ret = deflateInit2(&strm, Z_BEST_COMPRESSION, Z_DEFLATED, -MAX_WBITS, DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY);

	if (ret != Z_OK)
		return ret;


	flush = Z_FINISH;
	strm.next_in = in_buf;
	strm.avail_in = in_len;

	// run deflate() on input until output buffer not full, finish
	//   compression if all of source has been read in
	do {
		strm.avail_out = sizeof(out);
		strm.next_out = out;
		ret = deflate(&strm, flush);    // no bad return value 
		assert(ret != Z_STREAM_ERROR);  // state not clobbered 
		have = sizeof(out) - strm.avail_out;
		if (*out_len + have > out_buf_len) 
		{
			//if (have < sizeof
			out_buf_len = out_buf_len + sizeof(out);
			*out_buf = static_cast<unsigned char*> (realloc(*out_buf, out_buf_len));
			if (!out_buf)
			{
				(void)deflateEnd(&strm);
				return Z_ERRNO;
			}
		}
		memcpy((*out_buf + *out_len), out, have);
		*out_len += have; 
	} while (strm.avail_out == 0);
	assert(strm.avail_in == 0);     // all input will be used

	assert(ret == Z_STREAM_END);        // stream will be complete


	// clean up and return 
	(void)deflateEnd(&strm);
	return Z_OK;

}

int CV8File::LoadFile(BYTE *pFileData, ULONG FileDataSize, bool boolInflate, bool UnpackWhenNeed)
{
	int ret = 0;

	if (!pFileData) {
		//fputs("LoadFile. pFileData = NULL", stderr);
		return V8UNPACK_ERROR;
	}

	if (!IsV8File(pFileData, FileDataSize))
	{
		//fputs("LoadFile. This is not 1C v8 file.", stderr);
		return V8UNPACK_NOT_V8_FILE;
	}

	BYTE *InflateBuffer = NULL;
	ULONG InflateSize = 0;

	stFileHeader *pFileHeader = (stFileHeader*) pFileData;

	stBlockHeader *pBlockHeader;

	pBlockHeader = (stBlockHeader*) &pFileHeader[1];
	memcpy(&FileHeader, pFileData, stFileHeader::Size());

	
	UINT ElemsAddrsSize;
	ReadBlockData(pFileData, pBlockHeader, (BYTE*&)pElemsAddrs, &ElemsAddrsSize);


	ElemsNum = ElemsAddrsSize / stElemAddr::Size();

	pElems = new CV8Elem[ElemsNum];

	for (UINT i = 0; i < ElemsNum; i++)
	{


		if (pElemsAddrs[i].fffffff != 0x7fffffff)
		{
			ElemsNum = i;
			break;
		}


		pBlockHeader = (stBlockHeader*) &pFileData[pElemsAddrs[i].elem_header_addr];

		if (pBlockHeader->EOL_0D != 0x0d || 
			pBlockHeader->EOL_0A != 0x0a || 
			pBlockHeader->space1 != 0x20 || 
			pBlockHeader->space2 != 0x20 || 
			pBlockHeader->space3 != 0x20 || 
			pBlockHeader->EOL2_0D != 0x0d || 
			pBlockHeader->EOL2_0A != 0x0a) {

			//fputs("LoadFile. Header elem is not correct.", stderr);
			ret = V8UNPACK_HEADER_ELEM_NOT_CORRECT;
			break;
		}

	
		ReadBlockData(pFileData, pBlockHeader, pElems[i].pHeader, &pElems[i].HeaderSize);


		//080228 Блока данных может не быть, тогда адрес блока данных равен 0x7fffffff
		if (pElemsAddrs[i].elem_data_addr != 0x7fffffff)
		{
			pBlockHeader = (stBlockHeader*) &pFileData[pElemsAddrs[i].elem_data_addr];
			ReadBlockData(pFileData, pBlockHeader, pElems[i].pData, &pElems[i].DataSize);
		}
		else
			ReadBlockData(pFileData, NULL, pElems[i].pData, &pElems[i].DataSize);

		pElems[i].UnpackedData.IsDataPacked = false;

		if (boolInflate && IsDataPacked)
		{
			ret = Inflate(pElems[i].pData, &InflateBuffer, pElems[i].DataSize, &InflateSize);

			if (ret)
				IsDataPacked = false;
			else
			{

				/*
				if (UnpackWhenNeed)
				{

					delete[] pBlocks[i].pData;
					pBlocks[i].pData = new BYTE[undeflate_size];
					pBlocks[i].DataSize = undeflate_size;
					memcpy(pBlocks[i].pData, DataOut, undeflate_size);

					stFileHeader *pFileHeader = (stFileHeader*) DataOut;
					if (pFileHeader->sig != 0x7fffffff || pFileHeader->sig2 != 0x00000200)
						pBlocks[i].IsV8File = false;
					else
					{
						pBlocks[i].IsV8File = true;
						pBlocks[i].NeedUnpack = true;
					}
				}
				else
				{
				*/
				pElems[i].NeedUnpack = false; // отложенная распаковка не нужна
				delete[] pElems[i].pData; //нераспакованные данные больше не нужны
				pElems[i].pData = NULL;
				if (IsV8File(InflateBuffer, InflateSize))
				{
					ret = pElems[i].UnpackedData.LoadFile(InflateBuffer, InflateSize, boolInflate);
					if (ret)
						break;

					pElems[i].pData = NULL;
					pElems[i].IsV8File = true;
				}
				else
				{
					pElems[i].pData = new BYTE[InflateSize];
					pElems[i].DataSize = InflateSize;
					memcpy(pElems[i].pData, InflateBuffer, InflateSize);
				}
				ret = 0;
				/*
				}
				*/
				
			}
			

		}
		
	}


	if (InflateBuffer)
		free(InflateBuffer);
	

	return ret;
}

void CV8File::GetErrorMessage(int ret)
{
	/*
	switch (ret) {
	case Z_ERRNO:
		if (ferror(stdin))
			return "error reading stdin\n";
		if (ferror(stdout))
			fputs("error writing stdout\n", stderr);
		break;
	case Z_STREAM_ERROR:
		fputs("invalid compression level\n", stderr);
		break;
	case Z_DATA_ERROR:
		fputs("invalid or incomplete deflate data\n", stderr);
		break;
	case Z_MEM_ERROR:
		fputs("out of memory\n", stderr);
		break;
	case Z_VERSION_ERROR:
		fputs("zlib version mismatch!\n", stderr);
	*/
}

int CV8File::UnpackToFolder(char *filename_in, char *dirname, char *UnpackElemWithName, bool print_progress)
{


	unsigned char *pFileData = NULL;

	int ret = 0;

	struct _stat stat;
	ret = _stat(filename_in, &stat);
	if (ret)
	{	
		fputs("UnpackToFolder. Input file not found!\n", stdout);
		return -1;
	}

	ULONG FileDataSize = stat.st_size; 

	pFileData = new BYTE[FileDataSize];
	if (!pFileData)
	{	
		fputs("UnpackToFolder. Not enough memory!\n", stdout);
		return -1;
	}

	FILE *file_in = fopen(filename_in, "rb");
	ret = fread(pFileData, 1, FileDataSize, file_in);
	if (ret != FileDataSize)
	{
		fputs("UnpackToFolder. Error in reading file!\n", stdout);
		return ret;
	}
	fclose(file_in);

	ret = LoadFile(pFileData, FileDataSize, false);

	if (pFileData)
		delete pFileData; 

	if (ret == V8UNPACK_NOT_V8_FILE)
	{
		fputs("UnpackToFolder. This is not V8 file!\n", stdout);
		return ret;
	}
	if (ret == V8UNPACK_NOT_V8_FILE)
	{
		fputs("UnpackToFolder. Error in load file in memory!\n", stdout);
		return ret;
	}


	FILE *file_out;
	char *cur_dir = dirname;

	ret = _mkdir(cur_dir);
	if (ret && errno == ENOENT)
	{
		fputs("UnpackToFolder. Error in creating directory!\n", stdout);
		return ret;
	}

	char filename_out[MAX_PATH];

	sprintf(filename_out, "%s\\%s", cur_dir, "FileHeader");
	file_out = fopen(filename_out, "wb");
	if (!file_out)
	{
		fputs("UnpackToFolder. Error in creating file!\n", stdout);
		return ret;
	}
	fwrite(&FileHeader,  stFileHeader::Size(), 1, file_out);
	fclose(file_out);

	char ElemName[512];
	UINT ElemNameLen;

	UINT one_percent = ElemsNum / 50;
	if (print_progress && one_percent)
	{
		fputs("Progress (50 points): ", stdout);
	}
	

	UINT ElemNum;
	for(ElemNum = 0; ElemNum < ElemsNum; ElemNum++)
	{

		
		if (print_progress && ElemNum && one_percent && ElemNum%one_percent == 0)
		{
			if (ElemNum % (one_percent*10) == 0)
				fputs("|", stdout);
			else
				fputs(".", stdout);
		}
		
		GetElemName(pElems[ElemNum], ElemName, &ElemNameLen);

		// если передано имя блока для распаковки, пропускаем все остальные
		if (UnpackElemWithName && strcmp(UnpackElemWithName, ElemName))
			continue;

		sprintf(filename_out, "%s\\%s.%s", cur_dir, ElemName, "header");
		file_out = fopen(filename_out, "wb");
		if (!file_out)
		{
			fputs("UnpackToFolder. Error in creating file!", stdout);
			return -1;
		}
		fwrite(pElems[ElemNum].pHeader,  1, pElems[ElemNum].HeaderSize, file_out);
		fclose(file_out);

		sprintf(filename_out, "%s\\%s.%s", cur_dir, ElemName, "data");
		file_out = fopen(filename_out, "wb");
		if (!file_out)
		{
			fputs("UnpackToFolder. Error in creating file!", stdout);
			return -1;
		}
		fwrite(pElems[ElemNum].pData,  1, pElems[ElemNum].DataSize, file_out);
		fclose(file_out);
	}

	
	if (print_progress && one_percent)
	{
		fputs("\n", stdout);
	}
	

	return 0;
}

DWORD CV8File::_httoi(char *value)
{

	DWORD result = 0;

	char *s = value;
	BYTE lower_s;
	while (*s != '\0' && *s != ' ')
	{
		lower_s = tolower(*s);
		if (lower_s >= '0' && lower_s <= '9')
		{
			result <<= 4;
			result += lower_s - '0';
		}
		else if (lower_s >= 'a' && lower_s <= 'f')
		{
			result <<= 4;
			result += lower_s - 'a' + 10;
		}
		else
			break;
		s++;
	}
	return result;
}

int CV8File::ReadBlockData(BYTE *pFileData, stBlockHeader *pBlockHeader, BYTE *&pBlockData, UINT *BlockDataSize)
{
	DWORD data_size, page_size, next_page_addr;
	UINT read_in_bytes, bytes_to_read;

	if (pBlockHeader != NULL)
	{
		data_size = _httoi(pBlockHeader->data_size_hex);
		pBlockData = new BYTE[data_size];
		if (!pBlockData)
		{
			fputs("ReadBlockData. BlockData == NULL.", stdout);
			return -1;
		}
	}
	else
		data_size = 0;

	read_in_bytes = 0;
	while (read_in_bytes < data_size)
	{

		page_size = _httoi(pBlockHeader->page_size_hex);
		next_page_addr = _httoi(pBlockHeader->next_page_addr_hex);

		bytes_to_read = MIN(page_size, data_size - read_in_bytes);

		memcpy(&pBlockData[read_in_bytes], (BYTE*)(&pBlockHeader[1]), bytes_to_read);

		read_in_bytes += bytes_to_read;

		if (next_page_addr != 0x7fffffff) // есть следующая страница
			pBlockHeader = (stBlockHeader*) &pFileData[next_page_addr];
		else
			break;
	}

	if (BlockDataSize)
		*BlockDataSize = data_size;

	return 0;
}

bool CV8File::IsV8File(BYTE *pFileData, ULONG FileDataSize)
{

	if (!pFileData) {
		return false;
	}

	// проверим чтобы длина файла не была меньше длины заголовка файла и заголовка блока адресов
	if (FileDataSize < stFileHeader::Size() + stBlockHeader::Size()) 
		return false;

	stFileHeader *pFileHeader = (stFileHeader*) pFileData;

	stBlockHeader *pBlockHeader;

	pBlockHeader = (stBlockHeader*) &pFileHeader[1];

	if (pBlockHeader->EOL_0D != 0x0d || 
		pBlockHeader->EOL_0A != 0x0a || 
		pBlockHeader->space1 != 0x20 || 
		pBlockHeader->space2 != 0x20 || 
		pBlockHeader->space3 != 0x20 || 
		pBlockHeader->EOL2_0D != 0x0d || 
		pBlockHeader->EOL2_0A != 0x0a) {

		//fputs("LoadFile. This is not 1C v8 file.", stderr);
		return false;
	}

	return true;
}

int CV8File::PackFromFolder(char *dirname, char *filename_out)
{

	char cur_dir[MAX_PATH];
	strcpy(cur_dir, dirname);

	struct _finddata_t find_data;
	long hFind;

	char filename[MAX_PATH];

	struct _stat stat;

	FILE *file_in;

	char *point_pos;

	sprintf(filename, "%s\\FileHeader", cur_dir);

	_stat(filename, &stat);

	file_in = fopen(filename, "rb");
	fread(&FileHeader, 1, stat.st_size, file_in);
	fclose(file_in);

	sprintf(filename, "%s\\*.header", cur_dir);
	hFind = _findfirst(filename, &find_data);
	ElemsNum = 0;

	if( hFind != -1 )
	{
		do
		{
			ElemsNum ++;

		} while( _findnext(hFind, &find_data) == 0 );
		_findclose(hFind);
	}

	
	pElems = new CV8Elem[ElemsNum];

	hFind = _findfirst(filename, &find_data);
	UINT ElemNum = 0;

	if( hFind != -1 )
	{
		do
		{
			
			sprintf(filename, "%s\\%s", cur_dir, find_data.name);

			_stat(filename, &stat);
			pElems[ElemNum].HeaderSize = stat.st_size;
			pElems[ElemNum].pHeader = new BYTE[pElems[ElemNum].HeaderSize];
			file_in = fopen(filename, "rb");
			fread(pElems[ElemNum].pHeader, 1, pElems[ElemNum].HeaderSize, file_in);
			fclose(file_in);

			
			point_pos = strrchr(filename, '.');
			filename[point_pos - filename] = 0;
			strcat(filename, ".data");

			_stat(filename, &stat);
			pElems[ElemNum].DataSize = stat.st_size;
			pElems[ElemNum].pData = new BYTE[pElems[ElemNum].DataSize];
			file_in = fopen(filename, "rb");
			fread(pElems[ElemNum].pData, 1, pElems[ElemNum].DataSize, file_in);
			fclose(file_in);

			ElemNum++;
			

		} while( _findnext(hFind, &find_data) == 0 );
		_findclose(hFind);
	}


	SaveFile(filename_out);


	return 0;
}


int CV8File::SaveBlockData(FILE *file_out, BYTE *pBlockData, UINT BlockDataSize, UINT PageSize)
{

	if (PageSize < BlockDataSize)
		PageSize = BlockDataSize;
	
	stBlockHeader CurBlockHeader;

	CurBlockHeader.EOL_0D = 0xd;
	CurBlockHeader.EOL_0A = 0xa;
	CurBlockHeader.EOL2_0D = 0xd;
	CurBlockHeader.EOL2_0A = 0xa;

	CurBlockHeader.space1 = 0;
	CurBlockHeader.space2 = 0;
	CurBlockHeader.space3 = 0;

	sprintf(CurBlockHeader.data_size_hex, "%08x", BlockDataSize);
	sprintf(CurBlockHeader.page_size_hex, "%08x", PageSize);
	sprintf(CurBlockHeader.next_page_addr_hex, "%08x", 0x7fffffff);

	CurBlockHeader.space1 = ' ';
	CurBlockHeader.space2 = ' ';
	CurBlockHeader.space3 = ' ';

	fwrite((void*)&CurBlockHeader, sizeof(stBlockHeader), 1, file_out);

	fwrite((void*)pBlockData, 1, BlockDataSize, file_out);

	for(UINT i = 0; i < PageSize - BlockDataSize; i++)
	{
		fwrite("\0", 1, 1, file_out);
	}


	return NULL;
}

int CV8File::Parse(char *filename_in, char *dirname, int level)
{
	unsigned char *pFileData = NULL;

	int ret = 0;

	struct _stat stat;
	ret = _stat(filename_in, &stat);
	if (ret)
	{	
		fputs("UnpackToFolder. Input file not found!\n", stdout);
		return -1;
	}

	ULONG FileDataSize = stat.st_size; 

	pFileData = new BYTE[FileDataSize];
	if (!pFileData)
	{	
		fputs("UnpackToFolder. Not enough memory!\n", stdout);
		return -1;
	}

	FILE *file_in = fopen(filename_in, "rb");
	ret = fread(pFileData, 1, FileDataSize, file_in);
	if (ret != FileDataSize)
	{
		fputs("UnpackToFolder. Error in reading file!\n", stdout);
		return ret;
	}
	fclose(file_in);

	ret = LoadFile(pFileData, FileDataSize);
	fputs("LoadFile: ok\n", stdout);

	if (pFileData)
		delete pFileData; 

	if (ret == V8UNPACK_NOT_V8_FILE)
	{
		fputs("UnpackToFolder. This is not V8 file!\n", stdout);
		return ret;
	}
	if (ret == V8UNPACK_NOT_V8_FILE)
	{
		fputs("UnpackToFolder. Error in load file in memory!\n", stdout);
		return ret;
	}

	ret = SaveFileToFolder(dirname);

	return ret;
}


int CV8File::SaveFileToFolder(char* dirname)
{

	int ret = 0;

	ret = _mkdir(dirname);
	if (ret && errno == ENOENT)
	{
		fputs("UnpackToFolder. Error in creating directory!\n", stdout);
		return ret;
	}
	ret = 0;

	char filename_out[MAX_PATH];

	FILE* file_out = NULL;

	char ElemName[512];
	UINT ElemNameLen;

	bool print_progress = true;
	UINT one_percent = ElemsNum / 50;
	if (print_progress && one_percent)
	{
		fputs("Progress (50 points): ", stdout);
	}


	for(UINT ElemNum = 0; ElemNum < ElemsNum; ++ElemNum)
	{

		if (print_progress && ElemNum && one_percent && ElemNum%one_percent == 0)
		{
			if (ElemNum % (one_percent*10) == 0)
				fputs("|", stdout);
			else
				fputs(".", stdout);
		}

		GetElemName(pElems[ElemNum], ElemName, &ElemNameLen);

		sprintf(filename_out, "%s\\%s", dirname, ElemName);
		if (!pElems[ElemNum].IsV8File)
		{
			file_out = fopen(filename_out, "wb");
			if (!file_out)
			{
				fputs("SaveFile. Error in creating file!", stdout);
				return -1;
			}
			fwrite(pElems[ElemNum].pData,  1, pElems[ElemNum].DataSize, file_out);
			fclose(file_out);
		}
		else
		{
			ret = pElems[ElemNum].UnpackedData.SaveFileToFolder(filename_out);
			if (ret)
				break;

		}

	}

	if (print_progress && one_percent)
	{
		fputs("\n", stdout);
	}

	return ret;
}

int CV8File::GetElemName(CV8Elem &Elem, char *ElemName, UINT *ElemNameLen)
{
	*ElemNameLen = (Elem.HeaderSize - CV8Elem::stElemHeaderBegin::Size()) / 2;
	for (UINT j = 0; j < *ElemNameLen * 2; j+=2)
		ElemName[j/2] = Elem.pHeader[CV8Elem::stElemHeaderBegin::Size() + j];

	return 0;
}


int CV8File::SetElemName(CV8Elem &Elem, char *ElemName, UINT ElemNameLen)
{
	UINT stElemHeaderBeginSize = CV8Elem::stElemHeaderBegin::Size();

	for (UINT j = 0; j <ElemNameLen * 2; j+=2, stElemHeaderBeginSize+=2)
	{
		Elem.pHeader[stElemHeaderBeginSize] = ElemName[j/2];
		Elem.pHeader[stElemHeaderBeginSize + 1] = 0;
	}

	return 0;
}



int CV8File::LoadFileFromFolder(char* dirname)
{

	char new_dirname[MAX_PATH];

	struct _finddata_t find_data;
	long hFind;

	char filename[MAX_PATH];

	FILE* file_in;

	FileHeader.next_page_addr = 0x7fffffff;
	FileHeader.page_size = 0x200;
	FileHeader.storage_ver = 0;
	FileHeader.reserved = 0;

	sprintf(filename, "%s\\*", dirname);
	hFind = _findfirst(filename, &find_data);
	ElemsNum = 0;

	if( hFind != -1 )
	{
		do
		{
			if (find_data.name[0] == '.')
				continue;

			ElemsNum++;

		} while( _findnext(hFind, &find_data) == 0 );
		_findclose(hFind);
	}
	else
		return -1;

	pElems = new CV8Elem[ElemsNum];

	hFind = _findfirst(filename, &find_data);
	UINT ElemNum = 0;

	if( hFind != -1 )
	{
		do
		{
			
			if (find_data.name[0] == '.')
				continue;

//				fprintf(stdout, "LoadFileFromFolder: %s\n", find_data.name);

			pElems[ElemNum].HeaderSize = CV8Elem::stElemHeaderBegin::Size() + strlen(find_data.name) * 2 + 4; // последние четыре всегда нули?
			pElems[ElemNum].pHeader = new BYTE[pElems[ElemNum].HeaderSize];

			memset(pElems[ElemNum].pHeader, 0, pElems[ElemNum].HeaderSize);

			SetElemName(pElems[ElemNum], find_data.name, strlen(find_data.name));
			if (find_data.attrib & 0x10) // directory
			{
				pElems[ElemNum].IsV8File = true;
				sprintf(new_dirname, "%s\\%s", dirname, find_data.name);
				pElems[ElemNum].UnpackedData.LoadFileFromFolder(new_dirname);

			}
			else
			{
				pElems[ElemNum].IsV8File = false;

				pElems[ElemNum].DataSize = find_data.size;
				pElems[ElemNum].pData = new BYTE[pElems[ElemNum].DataSize];

				sprintf(filename, "%s\\%s", dirname, find_data.name);

				file_in = fopen(filename, "rb");
				fread(pElems[ElemNum].pData, 1, pElems[ElemNum].DataSize, file_in);
				fclose(file_in);
			}


			ElemNum++;
			

		} while( _findnext(hFind, &find_data) == 0 );
		_findclose(hFind);
	}

	return 0;


}

int CV8File::Build(char *dirname, char *filename, int level)
{
	int load;
	load = LoadFileFromFolder(dirname);
	if (load != 0)
		return load;
	fputs("LoadFileFromFolder: ok\n", stdout);

	Pack();
	fputs("Pack: ok\n", stdout);


	SaveFile(filename);

	return 0;
}

int CV8File::SaveFile(char *filename)
{

	FILE* file_out;

	file_out = fopen(filename, "wb");
	if (!file_out)
	{
		fputs("SaveFile. Error in creating file!", stdout);
		return -1;
	}



	// Создаем и заполняем данные по адресам элементов
	if (!pElemsAddrs)
		delete[] pElemsAddrs;

	pElemsAddrs = new stElemAddr[ElemsNum];


	DWORD cur_block_addr = stFileHeader::Size() + stBlockHeader::Size();
	if (sizeof(stElemAddr) * ElemsNum < 512)
		cur_block_addr += 512; // 512 - стандартный размер страницы 0x200
	else
		cur_block_addr += stElemAddr::Size() * ElemsNum;

	for(UINT ElemNum = 0; ElemNum < ElemsNum; ElemNum++)
	{

		pElemsAddrs[ElemNum].elem_header_addr = cur_block_addr;
		cur_block_addr += sizeof(stBlockHeader) + pElems[ElemNum].HeaderSize;

		pElemsAddrs[ElemNum].elem_data_addr = cur_block_addr;
		cur_block_addr += sizeof(stBlockHeader);

		if (pElems[ElemNum].DataSize > 512)
			cur_block_addr += pElems[ElemNum].DataSize;  
		else
			cur_block_addr += 512;

		pElemsAddrs[ElemNum].fffffff = 0x7fffffff;

	}


	// записываем заголовок
	fwrite(&FileHeader, 1, sizeof(stFileHeader), file_out);

	// записываем адреса элементов
	SaveBlockData(file_out, (BYTE*) pElemsAddrs, stElemAddr::Size() * ElemsNum);

	// записываем элементы (заголовок и данные)
	int ElemNum;
	for(ElemNum = 0; ElemNum < ElemsNum; ElemNum++)
	{
		SaveBlockData(file_out, pElems[ElemNum].pHeader, pElems[ElemNum].HeaderSize, pElems[ElemNum].HeaderSize);
		SaveBlockData(file_out, pElems[ElemNum].pData, pElems[ElemNum].DataSize);

	}

	fclose(file_out);


	return 0;

}

int CV8File::BuildCfFile(char *in_dirname, char *out_filename){
	//filename can't be empty
	if (!in_dirname){
		fputs("Argument error - Set of `in_dirname' argument \n",stdout);
		return SHOW_USAGE;
	}
	if (!out_filename){
		fputs("Argument error - Set of `out_filename' argument \n",stdout);
		return SHOW_USAGE;
	}
	//Read in_dirname
	struct _finddata_t find_data;
	long hFind;
	char filename[MAX_PATH];
	FILE* file_in;
	sprintf(filename, "%s\\*", in_dirname);

	hFind = _findfirst(filename, &find_data);
	if (hFind == -1){
		fprintf(stdout,"Error `%s' - path not found \n",in_dirname);
		return SHOW_USAGE;
	}
//Считаем количество элементов в корневом контейнере
	ElemsNum = 0;
	do{
		if (find_data.name[0] == '.')
			continue;
		ElemsNum++;
		}while(_findnext(hFind, &find_data) == 0);
	_findclose(hFind);
	if (ElemsNum == 0){
		fprintf(stdout,"Build Error. Directory `%s' is empty",in_dirname);
		return -1;

	}
//Предварительные расчеты длины заголовка т таблицы содержимого TOC файла
	FileHeader.next_page_addr = 0x7fffffff;
	FileHeader.page_size = 0x200;
	FileHeader.storage_ver = 0;
	FileHeader.reserved = 0;
	DWORD cur_block_addr = stFileHeader::Size() + stBlockHeader::Size();
	stElemAddr *pTOC;
	pTOC = new stElemAddr[ElemsNum];
	if (sizeof(stElemAddr) * ElemsNum < 512)
		cur_block_addr += 512; // 512 - стандартный размер страницы 0x200
	else
		cur_block_addr += stElemAddr::Size() * ElemsNum;
//Открываем выходной файл контейнер на запись
	FILE* file_out;
	file_out = fopen(out_filename, "wb");
	if (!file_out)
	{
		fputs("SaveFile. Error in creating file!", stdout);
		return -1;
	}
//Резервируем место в начале файла под заголовок и TOC
	for(int i=0; i < cur_block_addr; i++){
		fwrite("\0", 1, 1, file_out);
	}
//Обходим каталог и создаем элементы контейнера .cf
	CV8Elem pElem;
	hFind = _findfirst(filename, &find_data);
	UINT ElemNum = 0;

	if (hFind == -1){
		fprintf(stdout,"Error `%s' - path not found \n",in_dirname);
		return SHOW_USAGE;
	}

	char new_dirname[MAX_PATH];
	UINT one_percent = ElemsNum / 50;
	if (one_percent)
	{
		fputs("Progress (50 points): ", stdout);
	}

	do{
		if (find_data.name[0] == '.')
			continue;
//Progress bar ->
{
		if (ElemNum && one_percent && ElemNum%one_percent == 0)
		{
			if (ElemNum % (one_percent*10) == 0)
				fputs("|", stdout);
			else
				fputs(".", stdout);
		}
}//<- Progress bar
//Считывем элемент
//			fprintf(stdout, "trace: ReadElement %s\n", find_data.name);
		pElem.HeaderSize = CV8Elem::stElemHeaderBegin::Size() + strlen(find_data.name) * 2 + 4; // последние четыре всегда нули?
		pElem.pHeader = new BYTE[pElem.HeaderSize];

		memset(pElem.pHeader, 0, pElem.HeaderSize);

		SetElemName(pElem, find_data.name, strlen(find_data.name));
		if (find_data.attrib & 0x10) // directory
			{
				pElem.IsV8File = true;
				sprintf(new_dirname, "%s\\%s", in_dirname, find_data.name);
				pElem.UnpackedData.LoadFileFromFolder(new_dirname);

			}
		else
			{
				pElem.IsV8File = false;

				pElem.DataSize = find_data.size;
				pElem.pData = new BYTE[pElem.DataSize];

				sprintf(filename, "%s\\%s", in_dirname, find_data.name);

				file_in = fopen(filename, "rb");
				fread(pElem.pData, 1, pElem.DataSize, file_in);
				fclose(file_in);
			}
//Сжимаем данные
	PackElem(pElem);
//Добавляем элемент в TOC
	pTOC[ElemNum].elem_header_addr = cur_block_addr;
	cur_block_addr += sizeof(stBlockHeader) + pElem.HeaderSize;
	pTOC[ElemNum].elem_data_addr = cur_block_addr;
	cur_block_addr += sizeof(stBlockHeader);
	if (pElem.DataSize > 512)
		cur_block_addr += pElem.DataSize;
	else
		cur_block_addr += 512;
	pTOC[ElemNum].fffffff = 0x7fffffff;
//Записываем элемент в файл
	SaveBlockData(file_out, pElem.pHeader, pElem.HeaderSize, pElem.HeaderSize);
	SaveBlockData(file_out, pElem.pData, pElem.DataSize);
//Освобождаем память
	delete[] pElem.pData;
	pElem.pData = NULL;
	delete[] pElem.pHeader;
	pElem.pHeader = NULL;
	pElem.IsV8File = false;
	pElem.HeaderSize = 0;
	pElem.DataSize = 0;
	ElemNum++;
	}while( _findnext(hFind, &find_data) == 0 );
	_findclose(hFind);
//Записывем заголовок файла
	rewind(file_out);
	fwrite(&FileHeader, sizeof(stFileHeader), 1, file_out);
//Записываем блок TOC
	SaveBlockData(file_out, (BYTE*) pTOC, stElemAddr::Size() * ElemsNum);
//Закрываем выходной файл контейнер на запись
	fclose(file_out);
	fputs("\nBuild OK!", stdout);
	return 0;
}

int CV8File::PackElem(CV8Elem &pElem){
	BYTE *DeflateBuffer = NULL;
	ULONG DeflateSize = 0;

	BYTE *DataBuffer = NULL;
	ULONG DataBufferSize = 0;

	int ret = 0;
		if (!pElem.IsV8File)
		{
			ret = Deflate(pElem.pData, &DeflateBuffer, pElem.DataSize, &DeflateSize);
			if (ret)
				return ret;

			delete[] pElem.pData;
			pElem.pData = new BYTE[DeflateSize];
			pElem.DataSize = DeflateSize;
			memcpy(pElem.pData, DeflateBuffer, DeflateSize);
		}
		else
		{
			pElem.UnpackedData.GetData(&DataBuffer, &DataBufferSize);

			ret = Deflate(DataBuffer, &DeflateBuffer, DataBufferSize, &DeflateSize);
			if (ret)
				return ret;

			//pElem.UnpackedData = CV8File();
			pElem.IsV8File = false;

			pElem.pData = new BYTE[DeflateSize];
			pElem.DataSize = DeflateSize;
			memcpy(pElem.pData, DeflateBuffer, DeflateSize);

		}

	if (DeflateBuffer)
		free(DeflateBuffer);

	if (DataBuffer)
		free(DataBuffer);

	return 0;
}

int CV8File::Pack()
{
	BYTE *DeflateBuffer = NULL;
	ULONG DeflateSize = 0;

	BYTE *DataBuffer = NULL;
	ULONG DataBufferSize = 0;
	
	int ret = 0;

	bool print_progress = true;
	UINT one_percent = ElemsNum / 50;
	if (print_progress && one_percent)
	{
		fputs("Progress (50 points): ", stdout);
	}


	for(UINT ElemNum = 0; ElemNum < ElemsNum; ++ElemNum)
	{

		if (print_progress && ElemNum && one_percent && ElemNum%one_percent == 0)
		{
			if (ElemNum % (one_percent*10) == 0)
				fputs("|", stdout);
			else
				fputs(".", stdout);
		}

		if (!pElems[ElemNum].IsV8File)
		{
			ret = Deflate(pElems[ElemNum].pData, &DeflateBuffer, pElems[ElemNum].DataSize, &DeflateSize);
			if (ret)
				return ret;

			delete[] pElems[ElemNum].pData;
			pElems[ElemNum].pData = new BYTE[DeflateSize];
			pElems[ElemNum].DataSize = DeflateSize;
			memcpy(pElems[ElemNum].pData, DeflateBuffer, DeflateSize);
		}
		else
		{
			pElems[ElemNum].UnpackedData.GetData(&DataBuffer, &DataBufferSize);

			ret = Deflate(DataBuffer, &DeflateBuffer, DataBufferSize, &DeflateSize);
			if (ret)
				return ret;

			//pElems[ElemNum].UnpackedData = CV8File();
			pElems[ElemNum].IsV8File = false;

			pElems[ElemNum].pData = new BYTE[DeflateSize];
			pElems[ElemNum].DataSize = DeflateSize;
			memcpy(pElems[ElemNum].pData, DeflateBuffer, DeflateSize);

		}


	}

	if (print_progress && one_percent)
	{
		fputs("\n", stdout);
	}

	if (DeflateBuffer)
		free(DeflateBuffer);

	if (DataBuffer)
		free(DataBuffer);

	return 0;
}


int CV8File::GetData(BYTE **DataBuffer, ULONG *DataBufferSize)
{

	UINT ElemNum;

	ULONG NeedDataBufferSize = 0;
	NeedDataBufferSize += stFileHeader::Size();
	
	// заголовок блока и данные блока - адреса элементов с учетом минимальной страницы 512 байт
	NeedDataBufferSize += stBlockHeader::Size() + MAX(stElemAddr::Size() * ElemsNum, 512); 

	for(ElemNum = 0; ElemNum < ElemsNum; ElemNum++)
	{
		// заголовок блока и данные блока - заголовок элемента
		NeedDataBufferSize += stBlockHeader::Size()  + pElems[ElemNum].HeaderSize; 

		// заголовок блока и данные блока - данные элемента с учетом минимальной страницы 512 байт
		NeedDataBufferSize += stBlockHeader::Size()  + MAX(pElems[ElemNum].DataSize, 512); 
	}


	// Создаем и заполняем данные по адресам элементов
	stElemAddr *pTempElemsAddrs = new stElemAddr[ElemsNum];


	DWORD cur_block_addr = stFileHeader::Size() + stBlockHeader::Size();
	if (stElemAddr::Size() * ElemsNum < 512)
		cur_block_addr += 512; // 512 - стандартный размер страницы 0x200
	else
		cur_block_addr += stElemAddr::Size() * ElemsNum;

	for(ElemNum = 0; ElemNum < ElemsNum; ElemNum++)
	{

		pTempElemsAddrs[ElemNum].elem_header_addr = cur_block_addr;
		cur_block_addr += sizeof(stBlockHeader) + pElems[ElemNum].HeaderSize;

		pTempElemsAddrs[ElemNum].elem_data_addr = cur_block_addr;
		cur_block_addr += sizeof(stBlockHeader);

		if (pElems[ElemNum].DataSize > 512)
			cur_block_addr += pElems[ElemNum].DataSize;  
		else
			cur_block_addr += 512;

		pTempElemsAddrs[ElemNum].fffffff = 0x7fffffff;

	}


	*DataBuffer = static_cast<unsigned char*> (realloc(*DataBuffer, NeedDataBufferSize));


	BYTE *cur_pos = *DataBuffer;
	

	// записываем заголовок
	memcpy(cur_pos, (BYTE*) &FileHeader, stFileHeader::Size());
	cur_pos += stFileHeader::Size();

	// записываем адреса элементов
	SaveBlockDataToBuffer(&cur_pos, (BYTE*) pTempElemsAddrs, stElemAddr::Size() * ElemsNum);

	// записываем элементы (заголовок и данные)
	for(ElemNum = 0; ElemNum < ElemsNum; ElemNum++)
	{
		SaveBlockDataToBuffer(&cur_pos, pElems[ElemNum].pHeader, pElems[ElemNum].HeaderSize, pElems[ElemNum].HeaderSize);
		SaveBlockDataToBuffer(&cur_pos, pElems[ElemNum].pData, pElems[ElemNum].DataSize);
	}

	//fclose(file_out);

	if (pTempElemsAddrs)
		delete[] pTempElemsAddrs;

	*DataBufferSize = NeedDataBufferSize;

	return 0;

}


int CV8File::SaveBlockDataToBuffer(BYTE **cur_pos, BYTE *pBlockData, UINT BlockDataSize, UINT PageSize)
{

	if (PageSize < BlockDataSize)
		PageSize = BlockDataSize;
	
	stBlockHeader CurBlockHeader;

	CurBlockHeader.EOL_0D = 0xd;
	CurBlockHeader.EOL_0A = 0xa;
	CurBlockHeader.EOL2_0D = 0xd;
	CurBlockHeader.EOL2_0A = 0xa;

	CurBlockHeader.space1 = 0;
	CurBlockHeader.space2 = 0;
	CurBlockHeader.space3 = 0;

	sprintf(CurBlockHeader.data_size_hex, "%08x", BlockDataSize);
	sprintf(CurBlockHeader.page_size_hex, "%08x", PageSize);
	sprintf(CurBlockHeader.next_page_addr_hex, "%08x", 0x7fffffff);

	CurBlockHeader.space1 = ' ';
	CurBlockHeader.space2 = ' ';
	CurBlockHeader.space3 = ' ';


	memcpy(*cur_pos, (BYTE*)&CurBlockHeader, stBlockHeader::Size());
	*cur_pos += stBlockHeader::Size();


	memcpy(*cur_pos, pBlockData, BlockDataSize);
	*cur_pos += BlockDataSize;

	for(UINT i = 0; i < PageSize - BlockDataSize; i++)
	{
		**cur_pos = 0;
		++*cur_pos;
	}

	return 0;
}
