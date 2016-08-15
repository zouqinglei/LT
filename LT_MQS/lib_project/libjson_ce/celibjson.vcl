<html>
<body>
<pre>
<h1>Build Log</h1>
<h3>
--------------------Configuration: celibjson - Win32 (WCE ARMV4I) Release--------------------
</h3>
<h3>Command Lines</h3>
Creating temporary file "E:\DOCUME~1\zouql\LOCALS~1\Temp\RSP4ED.tmp" with contents
[
/nologo /W3 /D _WIN32_WCE=400 /D "ARM" /D "_ARM_" /D "WCE_PLATFORM_STANDARDSDK" /D "ARMV4I" /D UNDER_CE=400 /D "UNICODE" /D "_UNICODE" /D "NDEBUG" /D "_LIB" /Fp"ARMV4IRel/celibjson.pch" /YX /Fo"ARMV4IRel/" /QRarch4T /QRinterwork-return /O2 /MC /c 
"D:\cvs\CTMIS4.5\代码\plan\project\static_project\lib\libjson_ce\json-c-0.9\arraylist.c"
"D:\cvs\CTMIS4.5\代码\plan\project\static_project\lib\libjson_ce\json-c-0.9\debug.c"
"D:\cvs\CTMIS4.5\代码\plan\project\static_project\lib\libjson_ce\json-c-0.9\json_object.c"
"D:\cvs\CTMIS4.5\代码\plan\project\static_project\lib\libjson_ce\json-c-0.9\json_tokener.c"
"D:\cvs\CTMIS4.5\代码\plan\project\static_project\lib\libjson_ce\json-c-0.9\json_util.c"
"D:\cvs\CTMIS4.5\代码\plan\project\static_project\lib\libjson_ce\json-c-0.9\linkhash.c"
"D:\cvs\CTMIS4.5\代码\plan\project\static_project\lib\libjson_ce\json-c-0.9\printbuf.c"
]
Creating command line "clarm.exe @E:\DOCUME~1\zouql\LOCALS~1\Temp\RSP4ED.tmp" 
Creating command line "link.exe -lib /nologo /out:"ARMV4IRel\celibjson.lib"  ".\ARMV4IRel\arraylist.obj" ".\ARMV4IRel\debug.obj" ".\ARMV4IRel\json_object.obj" ".\ARMV4IRel\json_tokener.obj" ".\ARMV4IRel\json_util.obj" ".\ARMV4IRel\linkhash.obj" ".\ARMV4IRel\printbuf.obj" "
<h3>Output Window</h3>
Compiling...
arraylist.c
debug.c
json_object.c
json_tokener.c
d:\cvs\ctmis4.5\代码\plan\project\static_project\lib\libjson_ce\json-c-0.9\json_tokener.c(279) : warning C4018: '<' : signed/unsigned mismatch
d:\cvs\ctmis4.5\代码\plan\project\static_project\lib\libjson_ce\json-c-0.9\json_tokener.c(280) : warning C4018: '==' : signed/unsigned mismatch
d:\cvs\ctmis4.5\代码\plan\project\static_project\lib\libjson_ce\json-c-0.9\json_tokener.c(442) : warning C4018: '<' : signed/unsigned mismatch
d:\cvs\ctmis4.5\代码\plan\project\static_project\lib\libjson_ce\json-c-0.9\json_tokener.c(443) : warning C4018: '==' : signed/unsigned mismatch
d:\cvs\ctmis4.5\代码\plan\project\static_project\lib\libjson_ce\json-c-0.9\json_tokener.c(450) : warning C4018: '<' : signed/unsigned mismatch
d:\cvs\ctmis4.5\代码\plan\project\static_project\lib\libjson_ce\json-c-0.9\json_tokener.c(451) : warning C4018: '==' : signed/unsigned mismatch
json_util.c
d:\cvs\ctmis4.5\代码\plan\project\static_project\lib\libjson_ce\json-c-0.9\json_util.c(69) : warning C4101: 'fd' : unreferenced local variable
d:\cvs\ctmis4.5\代码\plan\project\static_project\lib\libjson_ce\json-c-0.9\json_util.c(68) : warning C4101: 'buf' : unreferenced local variable
d:\cvs\ctmis4.5\代码\plan\project\static_project\lib\libjson_ce\json-c-0.9\json_util.c(69) : warning C4101: 'ret' : unreferenced local variable
d:\cvs\ctmis4.5\代码\plan\project\static_project\lib\libjson_ce\json-c-0.9\json_util.c(66) : warning C4101: 'pb' : unreferenced local variable
d:\cvs\ctmis4.5\代码\plan\project\static_project\lib\libjson_ce\json-c-0.9\json_util.c(100) : warning C4101: 'wsize' : unreferenced local variable
d:\cvs\ctmis4.5\代码\plan\project\static_project\lib\libjson_ce\json-c-0.9\json_util.c(100) : warning C4101: 'wpos' : unreferenced local variable
d:\cvs\ctmis4.5\代码\plan\project\static_project\lib\libjson_ce\json-c-0.9\json_util.c(99) : warning C4101: 'fd' : unreferenced local variable
d:\cvs\ctmis4.5\代码\plan\project\static_project\lib\libjson_ce\json-c-0.9\json_util.c(99) : warning C4101: 'ret' : unreferenced local variable
d:\cvs\ctmis4.5\代码\plan\project\static_project\lib\libjson_ce\json-c-0.9\json_util.c(98) : warning C4101: 'json_str' : unreferenced local variable
d:\cvs\ctmis4.5\代码\plan\project\static_project\lib\libjson_ce\json-c-0.9\json_util.c(93) : warning C4700: local variable 'obj' used without having been initialized
linkhash.c
d:\cvs\ctmis4.5\代码\plan\project\static_project\lib\libjson_ce\json-c-0.9\linkhash.c(136) : warning C4018: '==' : signed/unsigned mismatch
d:\cvs\ctmis4.5\代码\plan\project\static_project\lib\libjson_ce\json-c-0.9\linkhash.c(167) : warning C4018: '==' : signed/unsigned mismatch
printbuf.c
Creating library...




<h3>Results</h3>
celibjson.lib - 0 error(s), 18 warning(s)
</pre>
</body>
</html>
