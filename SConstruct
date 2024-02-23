import os
from SCons.Script import GetBuildFailures
import subprocess as subproc
import atexit

env = Environment()
env.Append(CPPPATH=	["src/", "include/"])


# SETTINGS:-------------------------

debug = int(ARGUMENTS.get("debug", 0))
no_raylib = int(ARGUMENTS.get("no_raylib", 0))
# opt 3 should be fine too but 2 is safe
opt = int(ARGUMENTS.get("o", 2))

profiler = int(ARGUMENTS.get("p", 0))
debug_symbols = int(ARGUMENTS.get("debug_symbols", 0))


asan = int(ARGUMENTS.get("asan", 0))

#-----------------------------------

if debug:
	# debug macro
	env.Append(CPPDEFINES=["XDEBUG"])	
	# print("debug")

if no_raylib == 1:
	env.Append(CPPDEFINES=["NO_GUI"])	

sources = Glob("src/*.cpp")

cpu_count = os.cpu_count()
env.SetOption("num_jobs", cpu_count)



# msvc compiler on windows
if env["PLATFORM"] == "win32":
	env.Replace(CXX= "cl")
	env.Append(LIBPATH=["lib/windows"])
	# the optimization levels on msvc are more restrictive
	env.Append(CCFLAGS=["/std:c++17", "/O2", "/DEBUG"])

	# debug info
	env.Append(CCPDBFLAGS=["/Zi", "/Fd${TARGET}.pdb"])	
	env.Append(PDB="${TARGET.base}.pdb")

	if no_raylib == 1:
		# env.Append(LIBS=["kernel32"])
		pass
	else:
		# env.Append(LIBS=["kernel32", "raylibdll"])
		env.Append(LIBS=["raylibdll"])

# g++ compiler on linux
elif env["PLATFORM"] == "posix":
	env.Replace(CXX= "g++")
	env.Append(LIBPATH=["lib/linux"])
	if no_raylib == 1:
		#env.Append(LIBS=["libc"])
		pass
	else:
		#env.Append(LIBS=["libc", "raylib.a"])
		env.Append(LIBS=["raylib.a"])
	
	track = int(ARGUMENTS.get("track", 0))
	# -fno-exceptions
	#-Wenum-compare is disabled because of raygui.h
	env.Append(CCFLAGS=["-std=c++17", "-O"+str(opt), "-Wno-enum-compare", "-Wno-unused-result", "-fdiagnostics-color",
	 "-fmax-errors=3", "-Warray-bounds", "-Walloca", "-Wuninitialized", "-Wreturn-type", "-Werror=return-type"])
	
	if asan == 1:
		env.Append(CCFLAGS=["-fsanitize=address"])	
		env.Append(LINKFLAGS=["-fsanitize=address"])

	#env.Append(CXXFLAGS=["-fsanitize=address"])
	if debug_symbols:
		env.Append(CCFLAGS=["-g"])
	
	if profiler:
			env.Append(CCFLAGS=["-pg"])
			env.Append(LINKFLAGS=["-pg"])
	if track:
		env.Append(CPPDEFINES=["XTRACK"])

else:
	print("UNSUPPORTED PLATFORM")


# program = env.Program("prog", source = sources, variant_dir = bdir)
PROG_NAME = "prog"
program = env.Program(PROG_NAME, source = sources)

#import data_track.data_graph as dg

def at_exit():
	failed = GetBuildFailures()
	run = int(ARGUMENTS.get("run", 0))
	# no failures
	if len(failed) == 0 and run:
		if env["PLATFORM"] == "posix":
			prog = subproc.Popen("./"+PROG_NAME) # can also .read()
			prog.wait()
		elif env["PLATFORM"] == "win32":
			prog = subproc.Popen(PROG_NAME)
			prog.wait()

atexit.register(at_exit)
 	
		
#Return(program)