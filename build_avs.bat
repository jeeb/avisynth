call "%VS100COMNTOOLS%vsvars32.bat"

cd src
devenv avisynth.sln /Rebuild "Release|Win32" > ..\errwarns_avs.txt
devenv avisynth.sln /Rebuild "Debug|Win32" >> ..\errwarns_avs.txt