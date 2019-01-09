/*
     Wrapper for the sake of Win32 DLL's.
*/

#if defined(XLIBEMU32) 
#   define MAIN GRXMain
#else
#   define MAIN main
#endif

int MAIN (int argc, char *argv[])  {

  Main(argc,argv);

}
