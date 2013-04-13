rem Batch file for my personal use to test that stuff compiles on Visual C++
rem cl /W3 /EHsc /I "C:\Program Files\boost\boost_1_51" example-data-1d.cc /link /LIBPATH:e:\boost_libs_1_51\lib32
cl /W3 /EHsc /I "C:\Program Files\boost\boost_1_51" example-misc.cc /link /LIBPATH:e:\boost_libs_1_51\lib32
