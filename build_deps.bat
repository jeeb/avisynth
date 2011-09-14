call "%VS100COMNTOOLS%vsvars32.bat"

cd deps\baseclasses
devenv baseclasses.sln /Rebuild "Release|Win32" > ..\..\errwarns_deps.txt
devenv baseclasses.sln /Rebuild "Debug|Win32" >> ..\..\errwarns_deps.txt