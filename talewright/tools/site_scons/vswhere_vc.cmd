@echo off
rem Talewright: forwards to the real vswhere but only reports Visual Studio instances that contain the
rem x86/x64 C++ toolset, so SCons never selects an instance without a compiler (see site_init.py).
"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" %* -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64
