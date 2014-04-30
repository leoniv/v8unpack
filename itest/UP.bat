if %1 == P GOTO PACK
if %1 == p GOTO PACK


:UNPACK
..\bin\V8Unpack.exe -unpack      %2                              %2.unp
..\bin\V8Unpack.exe -undeflate   %2.unp\metadata.data            %2.unp\metadata.data.und
..\bin\V8Unpack.exe -unpack      %2.unp\metadata.data.und        %2.unp\metadata.unp
GOTO END


:PACK
..\bin\V8Unpack.exe -pack        %2.unp\metadata.unp            %2.unp\metadata_new.data.und
..\bin\V8Unpack.exe -deflate     %2.unp\metadata_new.data.und   %2.unp\metadata.data
..\bin\V8Unpack.exe -pack        %2.unp                         %2.new.cf


:END
