IF NOT EXIST WebSite (
   MKDIR WebSite
)

COPY /Y .\License.txt .\WebSite\
COPY /Y .\History.txt .\WebSite\

:: Copy x86 FragExt files...
COPY /Y .\out\objfre_w2k_x86\i386\FragExt-Setup-x86.msi .\WebSite\FragExt-Setup-x86.msi
COPY /Y .\out\objfre_w2k_x86\i386\CoFrag.pdb .\WebSite\CoFrag-x86.pdb
COPY /Y .\out\objfre_w2k_x86\i386\FragEng.pdb .\WebSite\FragEng-x86.pdb
COPY /Y .\out\objfre_w2k_x86\i386\FragMgx.pdb .\WebSite\FragMgx-x86.pdb
COPY /Y .\out\objfre_w2k_x86\i386\FragShx.pdb .\WebSite\FragShx-x86.pdb
COPY /Y .\out\objfre_w2k_x86\i386\FragSvx.pdb .\WebSite\FragSvx-x86.pdb

CALL "C:\PROGRAM FILES\7-ZIP\7z.exe" a -tzip .\WebSite\FragExt-Setup-x86.zip .\WebSite\FragExt-Setup-x86.msi

:: Copy x64 FragExt files...
COPY /Y .\out\objfre_wnet_amd64\amd64\FragExt-Setup-x64.msi .\WebSite\FragExt-Setup-x64.msi
COPY /Y .\out\objfre_wnet_amd64\amd64\CoFrag.pdb .\WebSite\CoFrag-x64.pdb
COPY /Y .\out\objfre_wnet_amd64\amd64\FragEng.pdb .\WebSite\FragEng-x64.pdb
COPY /Y .\out\objfre_wnet_amd64\amd64\FragMgx.pdb .\WebSite\FragMgx-x64.pdb
COPY /Y .\out\objfre_wnet_amd64\amd64\FragShx.pdb .\WebSite\FragShx-x64.pdb
COPY /Y .\out\objfre_wnet_amd64\amd64\FragSvx.pdb .\WebSite\FragSvx-x64.pdb

CALL "C:\PROGRAM FILES\7-ZIP\7z.exe" a -tzip .\WebSite\FragExt-Setup-x64.zip .\WebSite\FragExt-Setup-x64.msi

:: Copy the source code
COPY /Y .\out\FragExt-Source.zip .\WebSite\FragExt-Source.zip

PAUSE