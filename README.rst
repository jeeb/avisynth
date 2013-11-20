**Avisynth+ features for users**

- Faster script loading/startup
- Support for multiple ("shadow") plugin directories
- Autoloading of C-plugins
- Script language extensions, with support for multiline conditionals and loops.
- Improved still image support
- Improved TimeStretch function
- ... and more to come as development continues
    
**Avisynth+ features for developers**

- Easier compilation, and support for new compilers and IDEs
- 2x increased compilation speed
- Avoidance of C++ exceptions on normal execution paths for improved debugging experience
- Leaner and more logical project structure
- Parts of the code refactored
- Preview of future 64-bit functionality
- … and being continuously refactored

**How to install**

- Install official 32-bit Avisynth
- Replace the avisynth.dll with the one from Avisynth+
- Place plugins for Avisynth+ into a plugin directory
- See the included README.txt

**Compatiblity to Avisynth**

Avisynth+ tries to provide a superset of Avisynth's features while staying compatible to existing code. This means you should be able to use plugins and scripts written for Avisynth without any problems with Avisynth+. The contrary however might not be true: Plugins and scripts written explicitly to take advantage of Avisynth+ might not work using the official Avisynth.

**Shortcomings compared to Avisynth**

- Support for Windows 98/Me/2000 dropped. Oldest supported OS in Windows XP.
- TCPDeliver not included in the sources. However, TCPDeliver from official Avisynth can be used with Avisynth+.
- Ability to load VFAPI filters is missing, hopefully temporarily.

**How to compile (without DirectShowSource)**

- Clone the repository from GitHub ( https://github.com/pylorak/avisynth ) - Get the latest CMake ( http://www.cmake.org/cmake/resources/software.html ).
- Get any Visual C++, but no older than 2005 (8.0). Then update it with available service packs.
- Configure the project using CMake. No manual changes to CMake variables are necessary, just hit Configure then Generate.
- Load generated VC++ solution and compile.
- Done.
- **OR alternatively to the above steps**: For people who prefer to compile from the command line, `qyot27 was kind enough to provide detailed compilation instructions from an Msys environment <http://forum.doom9.org/showthread.php?p=1643929#post1643929>`_.

You are welcome to work on improving Avisynth+. In fact, I hope that I will not be the sole contributor to the project, and I look forward to more people joining from the community. Feel free to work on any feature you'd like, or if you are feeling unsure where to start, choose an open issue from the GitHub page. Testing, documenting, and giving any kind of feedback is also a great help.

**Avisynth Plugin Writing Tips Series**

`Tip #1: Exceptions <http://forum.doom9.org/showthread.php?p=1647262#post1647262>`_

`Tip #2: Parallel execution <http://forum.doom9.org/showthread.php?p=1649886#post1649886>`_