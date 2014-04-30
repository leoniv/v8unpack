FOR /D %%I IN (%1\*.unp) DO ..\bin\V8Unpack.exe -pack %%I %1\%%~nI_n
FOR %%I IN (%1\*.und_n) DO ..\bin\V8Unpack.exe -deflate %%I %1\%%~nI

