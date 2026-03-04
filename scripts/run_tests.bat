@echo off
if not exist "..\build" mkdir "..\build"

echo Compiling Tests...
gcc ..\tests\test_csv_reader.c ..\src\csv_reader.c ..\src\common.c -I ..\include -o ..\build\test_csv.exe
gcc ..\tests\test_graph_builder.c ..\src\graph_builder.c ..\src\common.c -I ..\include -o ..\build\test_graph.exe

echo Running Tests...
..\build\test_csv.exe
..\build\test_graph.exe
echo Done.