/* ============================================================================

NAME: UPdirTest00.js
REVISION: $Revision: 19 $

AUTHOR: $Author: brix8x $ (автор первого варианта PremierEx)
DATE  : $Date: 2008-03-31 19:46:17 +0700 (РџРЅ, 31 РјР°СЂ 2008) $

COMMENT:

ОГРАНИЧЕНИЯ:

Для выполнения теста необходимо наличие файла V8Unpack.exe, который может находиться
1. в каталоге выполнения теста
2. в каталоге ..\BIN (относительно каталога выполнения теста)
3. в каталоге \BIN (относительно каталога выполнения теста)
4. в каталоге с тестируемыми файлами

Если директорий с файлами CF не указан, то директорием по умолчанию считается
текущий каталог

Данный скрипт тестирует нулевой уровень разбора CF файла - UNPACK-PACK (level=0)

============================================================================ */

/* ============================================================================
Основная идея полного теста

1. Проверяемый CF-файл (TestCF) создает проверяемую ИБ (TestIB)
2. Средствами 1С производится проверка TestIB с сохранением файла проверки (TestLog)
3. TestCF распаковывается с ключом UNPACK в каталог распаковки (UnpackDir)
4. Из UnpackDir при помощи ключа PACK создается новый CF-файл (NewCF)
5. NewCF создает новую ИБ (NewIB)
6. Средствами 1С производится проверка NewIB с сохранением файла проверки (NewLog)
7. Итогом проверки является результат сравнения файлов TestCF vs NewCF и TestLog vs NewLog

============================================================================ */

/* ============================================================================

ВХОДНЫЕ ДАННЫЕ:
1. Каталог, в котором находятся тестируемые файлы *.cf
2. Ключ /TESTIB - заставляет тестировать ИБ, создаваемые из файлов CF
3. Ключ /PARSE - заставляет использовать для распаковки и упаковки связку ключей PARSE\BUILD

============================================================================ */

// ============================================================================
var oShell = new ActiveXObject("WScript.Shell");
var oFS = new ActiveXObject("Scripting.FileSystemObject");

var oError		= new Error;

var sTempDir	= new String();
var sCurrentDir	= new String();
var sCFDir	 	= new String();
var sV8Unpack	= new String();
var sV8Path		= new String();
var sLogName	= new String();

var oLog	 	= null;
var nWndStyle   = 10; //0 - hide

var bTESTIB		= false;
var bParseMode	= false;	//	Режим использования связки PARSE\BUILD вместо UNPACK\PACK

WScript.Quit(main())
// ============================================================================

// ============================================================================
function main()
{
	
	if(!Init())
	{
		e = oError; 
		WScript.Echo("Ошибка инициализации теста. " + e.message);
		return e.number;
	}

	if(!Run())
	{
		e = oError; 
		WScript.Echo("Ошибка выполнения теста. " + e.message);
		return e.number;
	}
	
	if(!Done())
	{
		e = oError; 
		WScript.Echo("Ошибка деинициализации теста. " + e.message);
		return e.number;
	}
	
	return 0;
}	
// ============================================================================

// ============================================================================
function Init()
{
	// В этом методе производится инициализация теста, например, создание временных каталогов
	var bResult = true;
	
	try
	{
		//{ Получение каталога скрипта
		sCurrentDir = oFS.GetParentFolderName(WScript.ScriptFullName);
		//}
		
		//{ Вычисление каталога с CF-файлами
		if (WScript.Arguments.Count())
		{ 
			sCFDir = WScript.Arguments.Item(0);
			
			if(!oFS.FolderExists(sCFDir))
				throw new Error(-1, "Каталог '" + sCFDir + "' не найден.");
				
			for (var i=1; i<WScript.Arguments.Count(); i++) {
				if ("/TESTIB" == WScript.Arguments.Item(i)) {
					bTESTIB = true;	
				}
				if ("/PARSE" == WScript.Arguments.Item(i)) {
					bParseMode = true;	
				}
			}	
		}	
		else
		{
				sCFDir = sCurrentDir;
		}
		//}
		
		//{ Проверка наличия файла V8Unpack.exe
		arrPaths = new Array(sCurrentDir, oFS.GetParentFolderName(sCurrentDir) + "\\bin", sCurrentDir + "\\bin", sCFDir);
		for(nIndex = 0; nIndex < arrPaths.length; nIndex++)
		{
			if(oFS.FileExists(arrPaths[nIndex] + "\\V8Unpack.exe"))
			{
				sV8Unpack = arrPaths[nIndex] + "\\V8Unpack.exe";
				break;
			}
		}
		
		if(!sV8Unpack.length)
			throw new Error(-3, "Файл V8Unpack.exe не найден.");
		//}
		
		//{ Вычисление пути к файлу 1cv8.exe
		sV8Path = Get1CPath();
		if(!sV8Path.length)
			throw oError;
		//}
		
		//{ Создание временного каталога
		sTempDir = sCurrentDir + "\\" + oFS.GetTempName();
		if(!oFS.FolderExists(sTempDir))
			if("object" != typeof(oFS.CreateFolder(sTempDir)))	
				throw new Error(-2, "При создании подкаталога '" + sTempDir + "' произошла ошибка.");
		//}
		
		//{ Создание файла отчета
		sLogName = WScript.ScriptFullName.replace(".js", ".txt");
		oLog = oFS.CreateTextFile(sLogName, true);
			if("object" != typeof(oLog))
				throw new Error(-4, "Неудачная попытка создания файла отчета.");
		//}
	}
	catch(e)
	{
		// Сообщаем движку теста о возникшей ошибке
		// и возвращаем false
		oError = e;
		bResult = false;		
	}
	return bResult;
}

function Done()
{	
	var bResult = true;
	try
	{
		oLog.Close();
		oShell.Run(sLogName, nWndStyle, false);

//		if(oFS.FolderExists(sTempDir))
//			oFS.DeleteFolder(sTempDir, true);
	}
	catch(e)
	{
		// Сообщаем движку теста о возникшей ошибке
		// и возвращаем false
		oError = e;
		bResult = false;		
	}
	return bResult;
}

function Run()
{

	var bResult = true;

	try
	{
		//{ Формирование шапки лога
		oLog.WriteLine("====================================================");
		oLog.WriteLine("Начало: "+Date());
		oLog.WriteLine("");
		oLog.WriteLine("Директорий скрипта: '"+sCurrentDir+"'");
		oLog.WriteLine("Директорий CF     : '"+sCFDir+"'");
		oLog.WriteLine("Путь к V8Unpack   : '"+sV8Unpack+"'");
		oLog.WriteLine("Путь к 1С         : '"+sV8Path+"'");
		oLog.WriteLine("");
		oLog.WriteLine("Тестировать ИБ    : '"+bTESTIB+"'");
		oLog.WriteLine("Исп. PARSE-BUILD  : '"+bParseMode+"'");
		oLog.WriteLine("====================================================");
		//}
		
		//{ Вызываем для каждого файла в каталоге cCFDir функцию тестирования
		var arResults = ForEachFileInFolder(sCFDir, "*.cf", Test);	
		//}
		
		//{ Формирование подвала лога
		oLog.WriteLine("");
		oLog.WriteLine("====================================================");
		oLog.WriteLine("Завершение: "+Date());
		oLog.WriteLine("");

		for (var i = 0; i < arResults[1].length; i++)
		{
			oLog.WriteLine(arResults[0][i] + " : " + arResults[1][i]);
		}					
		
		oLog.WriteLine("====================================================");
		//}
	}
	catch(e)
	{
		// Сообщаем движку теста о возникшей ошибке
		// и возвращаем false
		oError = e;
		bResult = false;		
	}

	return bResult;
}
// ============================================================================

// ============================================================================
function Test(sFileName)
{
	var sResult = "";
	var sTestCFDir = new String();
	var sUnpackDir = new String();

	var sCF 	 = sFileName;
	
	try
	{
		oLog.WriteLine("");
		oLog.WriteLine("### Тестирование файла ### " + sFileName);
		
		//{ Создание каталога теста конкретного файла CF
		sTestCFDir = sTempDir + "\\" + oFS.GetBaseName(sFileName);
		if(!oFS.FolderExists(sTestCFDir))
			if("object" != typeof(oFS.CreateFolder(sTestCFDir)))	
				return("Неудачная попытка создания каталога теста для CF-файла.");
		//}
		
		//{ Создание и проверка ИБ TestIB
		if (bTESTIB)
		{
			var sTestIBDir = sTestCFDir + "\\TestIBDir";
			var sTestLog = sTestCFDir + "\\TestIBDir.txt";
		
			var res = CreateIB(sV8Path, sCF, sTestIBDir, sTestLog);
			if (0 != res) {
				oLog.WriteLine(sCmdLine);
				oLog.WriteLine("res = "+res);
				return("TestIB: ОШИБКА создания и проверки ИБ: "+res);
			}	
		}	
		//}
		
		//{ Определение режимов распаковки и упаковки
		if (bParseMode) {
			sUnpackMode = "-PARSE";
			sPackMode = "-BUILD";
		}	
		else {
			sUnpackMode = "-UNPACK";
			sPackMode = "-PACK";
		}			
		//}
		
		//{ Распаковка CF файла в каталог распаковки
		sUnpackDir = sTestCFDir + "\\udir";
		
		var sCmdLine = "\"" + sV8Unpack + "\" "+ sUnpackMode +" \"" + sCF + "\" \"" + sUnpackDir + "\"";
		
		var res = oShell.Run(sCmdLine, 10, true);
		if (0 != res) {
			oLog.WriteLine(sCmdLine);
			oLog.WriteLine("res = "+res);
			return("UNPACK error number "+res);
		}	
		//}
		
		//{ Создание нового CF файла из каталога распаковки
		var sNewCF 	 = sTestCFDir + "\\2" + oFS.GetFileName(sFileName);
		sCmdLine = "\"" + sV8Unpack + "\"" + sPackMode + " \"" + sUnpackDir + "\" \"" + sNewCF + "\"";
		oShell.Run(sCmdLine, 10, true);
		if (0 != res) {
			oLog.WriteLine(sCmdLine);
			oLog.WriteLine("res = "+res);
			return("PACK error number "+res);
		}	
		
		if(!oFS.FileExists(sNewCF))
			return ("Новый CF-файл не был найден по пути: '" + sNewCF + "'");
		//}
		
		//{ Создание и проверка ИБ NewIB
		if (bTESTIB)
		{
			var sNewIBDir = sTestCFDir + "\\NewIBDir";
			var sNewLog = sTestCFDir + "\\NewIBDir.txt";
		
			var res = CreateIB(sV8Path, sNewCF, sNewIBDir, sNewLog);
			if (0 != res) {
				oLog.WriteLine(sCmdLine);
				oLog.WriteLine("res = "+res);
				return("NewIB : ОШИБКА создания и проверки ИБ: "+res);
			}	
		}	
		//}
		
		//{ Сравнение файлов CF и файлов проверок
		if (oFS.GetFile(sCF).Size == oFS.GetFile(sNewCF).Size)
		{
			sResult = "[CF - OK]"
		}	
		else
		{
			sResult = "[TestCF="+oFS.GetFile(sCF).Size+" "+"NewCF="+oFS.GetFile(sNewCF).Size+"]"
		}
			
		if (bTESTIB)
		{
			if (oFS.GetFile(sTestLog).Size == oFS.GetFile(sNewLog).Size)
			{
				sResult = sResult+" [IB - OK]"
			}	
			else
			{
				sResult = sResult+" [sTestLog="+oFS.GetFile(sTestLog).Size+" "+"sNewLog="+oFS.GetFile(sNewLog).Size+"]"
			}
		}	
		//}

		oLog.WriteLine("##########################");
		oLog.WriteLine("");
	}
	catch(e)
	{
		oError = e;
		sResult = "Test: Необработанная ошибка: "+e.Description;
	}

	return sResult;	
}

function CreateIB(sV8Path, sCfFile, sIBDir, sOutLog)
{
	var sResult = "";
	
	try
	{
		//{ Создание пустой конфигурации
		var sCmdLine = "\"" + sV8Path + "\" CREATEINFOBASE \"File=" + sIBDir + "\" /Out\"" + sOutLog + "\"";
		var res = oShell.Run(sCmdLine, nWndStyle, true);
		if (0 != res) {
			oLog.WriteLine(sCmdLine);
			oLog.WriteLine("res = "+res);
			oLog.WriteLine(readLog(sOutLog));
			return("CREATE EMPTY IB error "+res);
		}	
		//}
		
		//{ Загрузка конфигурации
		var sCmdLine = "\"" + sV8Path + "\" CONFIG /F \"" + sIBDir + "\" /Out\"" + sOutLog + "\" /LoadCfg\"" + sCfFile + "\"";
		var res = oShell.Run(sCmdLine, nWndStyle, true);
		if (0 != res) {
			oLog.WriteLine(sCmdLine);
			oLog.WriteLine("res = "+res);
			oLog.WriteLine(readLog(sOutLog));
			return("LOADCFG IB error "+res);
		}	
		//}
		
		//{ Проверка ИБ средствами 1С
		var sCmdLine = "\"" + sV8Path + "\" CONFIG /F \"" + sIBDir + "\" /Out\"" + sOutLog + "\" -NoTruncate /CheckConfig";
		var res = oShell.Run(sCmdLine, nWndStyle, true);
		if (0 != res) {
			oLog.WriteLine(sCmdLine);
			oLog.WriteLine("res = "+res);
			oLog.WriteLine(readLog(sOutLog));
			return("CHECKCONFIG IB error "+res);
		}	
	}
	catch(e)
	{
		oError = e;
		sResult = "CreateIB error: "+e.Description;
	}
	
	return sResult;	
}

function readLog(sLogFileName)
{
	if (!oFS.FileExists(sLogFileName))
		return "Не найден файл лога '" + sLogFileName + "'";
		
	var oOutLog = oFS.OpenTextFile(sLogFileName, 1, false);
	if("object" != typeof(oOutLog))
		return "Ошибка чтения файла лога";
	
	return oOutLog.ReadAll();
}
// ============================================================================

// ============================================================================
function ForEachFileInFolder(sDir, sMask, sFunction)
{
	var arResults = new Array(new Array(), new Array());
	
	try
	{
		var oFolder = oFS.GetFolder(sDir);
		var cFiles  = new Enumerator(oFolder.Files);
		
		var nNamedArgsCount = Math.min(ForEachFileInFolder.length, arguments.length);
		var nUnnamedArgsCountCount = arguments.length - nNamedArgsCount;
		
		// transform mask to RegExp
		var sRegExp = "^"+sMask+"$";
		sRegExp = sRegExp.replace("\.", "\\.");
		sRegExp = sRegExp.replace("\*", ".*");
		// transform mask to RegExp
	
		// формирование списка произвольных аргументов для функции
		// первый аргумент - ВСЕГДА sFullname, и соответсвует имени файла
		var sArgsList = "sFullName";
		for (i = 0; i < nUnnamedArgsCountCount; i++)
		{
			sArgsList += (sArgsList == "") ? "" : ",";
			sArgsList +=  "arguments[" + (nNamedArgsCount + i) + "]";
		}
		// формирование списка произвольных аргументов для функции
		
		var oRegExp = new RegExp(sRegExp,"i");
		
		for (; !cFiles.atEnd(); cFiles.moveNext())
		{
			var sName = cFiles.item().Name;

			if (oRegExp.test(sName)) 
			{
				var sFullName = sDir + "\\" + cFiles.item().Name;
				eval("var bResult = sFunction(" + sArgsList + ")");
				arResults[0][arResults[0].length] = cFiles.item().Path;
				arResults[1][arResults[1].length] = bResult;
			}

		}

	}
	catch(e)
	{
		oError = e;
	}	
	
	return arResults;
}

function Get1CPath()
{
	var s1CPath = new String();
	
	try
	{
		var sCLSID = oShell.RegRead("HKCR\\V81.Application\\CLSID\\");
		if("undefined" == typeof(sCLSID))
			throw new Error(-1, "При получения данных объекта 'V81.Application' из системного реестра произошла ошибка.");

		var sValue = oShell.RegRead("HKCR\\CLSID\\" + sCLSID + "\\LocalServer32\\");
		if(!oFS.FileExists(sValue))
			throw new Error(-2, "Файл '" + sValue + "' не найден.");

		s1CPath = sValue;	
	}
	catch(e)
	{
		oError = e;
	}
	
	return s1CPath;
}
// ============================================================================
