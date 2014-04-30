V8Unpack -unpack %1 %1.und
FOR %%1 IN (%1.und\*.data) DO ..\bin\V8Unpack.exe -undeflate %%1 %%1.und
FOR %%1 IN (%1.und\*.und) DO ..\bin\V8Unpack.exe -unpack %%1 %%1.unp