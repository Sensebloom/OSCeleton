----------------------------------------------------------------------
-- Premake4 configuration script for LibLo
-- Adapted from ODE's build script by Jason Perkins.
-- For more information on Premake: http://industriousone.com/premake
----------------------------------------------------------------------

----------------------------------------------------------------------
-- Configuration options
----------------------------------------------------------------------

  -- always clean all of the optional components and toolsets
  if _ACTION == "clean" then
    for action in pairs(premake.actions) do
      os.rmdir(action)
    end
  end
  
  

----------------------------------------------------------------------
-- The solution, and solution-wide settings
----------------------------------------------------------------------

  solution "liblo"

    language "C"
    location ( _OPTIONS["to"] or _ACTION )

    includedirs {
      "../lo",
      "../src"
    }
      
    -- define all the possible build configurations
    configurations {
      "DebugDLL", "ReleaseDLL", 
      "DebugLib", "ReleaseLib", 
    }
    
    configuration { "Debug*" }
      defines { "_DEBUG" }
      flags   { "Symbols" }
      
    configuration { "Release*" }
      flags   { "OptimizeSpeed", "NoFramePointer" }

    configuration { "Windows" }
      defines { "WIN32" }

    -- give each configuration a unique output directory
    for _, name in ipairs(configurations()) do
      configuration { name }
        targetdir ( "../lib/" .. name )
    end
      
    -- disable Visual Studio security warnings
    configuration { "vs*" }
      defines { "_CRT_SECURE_NO_DEPRECATE" }

    -- tell source to use config.h
    configuration { "vs*" }
      defines { "HAVE_CONFIG_H" }

    -- don't remember why we had to do this	(from ODE)
    configuration { "vs2002 or vs2003", "*Lib" }
      flags  { "StaticRuntime" }

----------------------------------------------------------------------
-- Write a custom <config.h> to .., based on the supplied flags
----------------------------------------------------------------------

-- First get the version number from "configure.ac" --

  io.input("../configure.ac")
  text = io.read("*all")
  io.close()
  text = string.sub(text,string.find(text, "AC_INIT.+"))
  version = string.sub(text,string.find(text, "%d+%.%d+"))

-- Replace it in "config.h" --

  io.input("config-msvc.h")
  local text = io.read("*all")

  text = string.gsub(text, '/%*VERSION%*/', '"'..version..'"')

  io.output("../config.h")
  io.write(text)
  io.close()

----------------------------------------------------------------------
-- Copy <lo_endian.h> to ../lo
----------------------------------------------------------------------

  io.input("lo_endian-msvc.h")
  io.output("../lo/lo_endian.h")
  local text = io.read("*all")
  io.write(text)
  io.close()

----------------------------------------------------------------------
-- The LibLo library project
----------------------------------------------------------------------

  project "liblo"

    kind     "StaticLib"
    location ( _OPTIONS["to"] or _ACTION )

    includedirs {
      "..",
    }

    files {
      "../src/*.c",
      "../src/*.h",
      "../lo/*.h",
      "../src/liblo.def",
    }

    excludes {
      "../src/testlo.c",
      "../src/subtest.c",
      "../src/tools",
    }

    configuration { "windows" }
      links   { "user32",
                "wsock32",
                "ws2_32",
                "pthreadVC2",
              }
            
    configuration { "*Lib" }
      kind    "StaticLib"
      defines "LIBLO_LIB"
      
    configuration { "*DLL" }
      kind    "SharedLib"
      defines "LIBLO_DLL"

    configuration { "Debug*" }
      targetname "liblo_d"
      
    configuration { "Release*" }
      targetname "liblo"


----------------------------------------------------------------------
-- The automated test application
----------------------------------------------------------------------


  project "testlo"
  
    kind     "ConsoleApp"
    location ( _OPTIONS["to"] or _ACTION )
    links   { "user32",
              "wsock32",
              "ws2_32",
              "pthreadVC2",
            }

    includedirs { 
      "..",
    }
    
    files { 
      "../src/testlo.c",
    }

    configuration { "DebugDLL" }
      links { "liblo_d" }
      libdirs { "../lib/debugdll" }

    configuration { "DebugLib" }
      links { "liblo_d" }
      libdirs { "../lib/debuglib" }
      
    configuration { "Release*" }
      links { "liblo" }

  project "subtest"
  
    kind     "ConsoleApp"
    location ( _OPTIONS["to"] or _ACTION )
    links   { "user32",
              "wsock32",
              "ws2_32",
              "pthreadVC2",
            }

    includedirs { 
      "..",
    }

    files { 
      "../src/subtest.c",
    }

    configuration { "DebugDLL" }
      links { "liblo_d" }
      libdirs { "../lib/debugdll" }

    configuration { "DebugLib" }
      links { "liblo_d" }
      libdirs { "../lib/debuglib" }
      
    configuration { "Release*" }
      links { "liblo" }
