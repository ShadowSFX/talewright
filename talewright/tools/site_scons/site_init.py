# Talewright SCons site init, loaded via `scons --site-dir=talewright/tools/site_scons` (talewright/tools/build.py).
#
# Problem: SCons ranks Visual Studio instances Enterprise > Professional > Community > BuildTools and does not
# fall back when the preferred instance has no C++ compiler (SCons/Tool/MSCommon/vc.py, component ranking), so
# a Community install without the C++ workload hides a working BuildTools install and
# platform/windows/detect.py:179 then falls back to MinGW ("No valid compilers found").
# Fix without touching Godot files: register a vswhere shim that only reports instances containing the
# x86/x64 C++ toolset (public API SCons.Tool.MSCommon.vswhere_register_executable).

import os

if os.name == "nt" and os.environ.get("TALEWRIGHT_VSWHERE_SHIM"):
    from SCons.Tool.MSCommon import vswhere_register_executable

    vswhere_register_executable(os.environ["TALEWRIGHT_VSWHERE_SHIM"], priority="high", freeze=True)
