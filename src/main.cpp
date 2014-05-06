// V8Unpack.cpp : Defines the entry point for the console application.
//

#include "V8File.h"
#include "version.h"

using namespace std;

void usage(){
	fputs("\n", stdout);
	fprintf(stdout,"V8Upack Version %s Copyright (c) %s\n",V8P_VERSION,V8P_RIGHT);
	fputs("\n", stdout);
	fputs("Unpack, pack, deflate and inflate 1C v8 file (*.cf)\n", stdout);
	fputs("\n", stdout);
	fputs("V8UNPACK\n", stdout);
	fputs("  -U[NPACK]     in_filename.cf     out_dirname\n", stdout);
	fputs("  -PA[CK]       in_dirname         out_filename.cf\n", stdout);
	fputs("  -I[NFLATE]    in_filename.data   out_filename\n", stdout);
	fputs("  -D[EFLATE]    in_filename        filename.data\n", stdout);
	fputs("  -E[XAMPLE]\n", stdout);
	fputs("  -BAT\n", stdout);
	fputs("  -P[ARSE]      in_filename        out_dirname\n", stdout);
	fputs("  -B[UILD]      in_dirname         out_filename\n", stdout);
	fputs("  -V[ERSION]\n", stdout);
}

void version(){
	fprintf(stdout,"%s\n",V8P_VERSION);
}

int main(int argc, char* argv[])
{

	string cur_mode;
	if (argc > 1)
		cur_mode = argv[1];

	string::iterator p = cur_mode.begin();

	for(;p != cur_mode.end(); ++p)
		*p = tolower(*p);

	int ret = 0;

	if(cur_mode == "-version" || cur_mode == "-v")
	{

		version();

		return 0;
	}


	if(cur_mode == "-inflate" || cur_mode == "-i" || cur_mode == "-und" || cur_mode == "-undeflate")
	{

		CV8File V8File;

		V8File.Inflate(argv[2], argv[3]);


		return ret;
	}

	if(cur_mode == "-deflate" || cur_mode == "-d")
	{

		CV8File V8File;

		ret = V8File.Deflate(argv[2], argv[3]);

		return ret;
	}

	
	if(cur_mode == "-unpack" || cur_mode == "-u" || cur_mode == "-unp")
	{

		CV8File V8File;

		ret = V8File.UnpackToFolder(argv[2], argv[3], argv[4], true);

		//if (ret)
		//	cout << "Error!";

		return ret;
	}

	
	if(cur_mode == "-pack" || cur_mode == "-pa")
	{

		CV8File V8File;

		ret = V8File.PackFromFolder(argv[2], argv[3]);


		return ret;
	}

	if(cur_mode == "-parse" || cur_mode == "-p")
	{

		CV8File V8File;

		ret = V8File.Parse(argv[2], argv[3]);

		return ret;
	}

	if(cur_mode == "-build" || cur_mode == "-b")
	{

		CV8File V8File;

		ret = V8File.BuildCfFile(argv[2], argv[3]);
		if (ret == SHOW_USAGE)
			usage();
		return ret;
	}

	
	if(cur_mode == "-bat")
	{
		
		fputs("if %1 == P GOTO PACK\n", stdout);
		fputs("if %1 == p GOTO PACK\n", stdout);
		fputs("\n", stdout);
		fputs("\n", stdout);
		fputs(":UNPACK\n", stdout);
		fputs("V8Unpack.exe -unpack      %2                              %2.unp\n", stdout);
		fputs("V8Unpack.exe -undeflate   %2.unp\\metadata.data            %2.unp\\metadata.data.und\n", stdout);
		fputs("V8Unpack.exe -unpack      %2.unp\\metadata.data.und        %2.unp\\metadata.unp\n", stdout);
		fputs("GOTO END\n", stdout);
		fputs("\n", stdout);
		fputs("\n", stdout);
		fputs(":PACK\n", stdout);
		fputs("V8Unpack.exe -pack        %2.unp\\metadata.unp            %2.unp\\metadata_new.data.und\n", stdout);
		fputs("V8Unpack.exe -deflate     %2.unp\\metadata_new.data.und   %2.unp\\metadata.data\n", stdout);
		fputs("V8Unpack.exe -pack        %2.unp                         %2.new.cf\n", stdout);
		fputs("\n", stdout);
		fputs("\n", stdout);
		fputs(":END\n", stdout);

		return ret;
	}

	if(cur_mode == "-example" || cur_mode == "-e")
	{
		
		fputs("\n", stdout);
		fputs("\n", stdout);
		fputs("UNPACK\n", stdout);
		fputs("V8Unpack.exe -unpack      1Cv8.cf                         1Cv8.unp\n", stdout);
		fputs("V8Unpack.exe -undeflate   1Cv8.unp\\metadata.data          1Cv8.unp\\metadata.data.und\n", stdout);
		fputs("V8Unpack.exe -unpack      1Cv8.unp\\metadata.data.und      1Cv8.unp\\metadata.unp\n", stdout);
		fputs("\n", stdout);
		fputs("\n", stdout);
		fputs("PACK\n", stdout);
		fputs("V8Unpack.exe -pack        1Cv8.unp\\metadata.unp           1Cv8.unp\\metadata_new.data.und\n", stdout);
		fputs("V8Unpack.exe -deflate     1Cv8.unp\\metadata_new.data.und  1Cv8.unp\\metadata.data\n", stdout);
		fputs("V8Unpack.exe -pack        1Cv8.und                        1Cv8_new.cf\n", stdout);
		fputs("\n", stdout);
		fputs("\n", stdout);

		return ret;
	}
	
	usage();
	return 1;
}
