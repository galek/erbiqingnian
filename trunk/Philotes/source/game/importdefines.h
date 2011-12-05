//----------------------------------------------------------------------------
// FUNCTIONS TO IMPORT INTO SCRIPTING LANGUAGE
//----------------------------------------------------------------------------
#undef RETVAL_INT
#undef RETVAL_FLOAT
#undef IMPORT_FUNC
#undef IMPORT_END
#undef IMPORT_VMSTATE
#undef IMPORT_CONTEXT
#undef IMPORT_GAME
#undef IMPORT_INT
#undef IMPORT_CONTEXT_UNIT
#undef IMPORT_CONTEXT_STATS
#undef IMPORT_INDEX
#undef IMPORT_INDEXN
#undef IMPORT_STAT_PARAM

#ifndef IMPORT_TABLE
  #define RETVAL_INT									int
  #define RETVAL_FLOAT									float
  #define IMPORT_FUNC(f, ret, n)						ret f(
  #define IMPORT_END									);
  #define IMPORT_CONTEXT								struct SCRIPT_CONTEXT* context
  #define IMPORT_GAME									struct GAME* game
  #define IMPORT_VMSTATE								struct VMSTATE* vm
  #define IMPORT_INT(n)								    int n
  #define IMPORT_CONTEXT_UNIT(n)						struct UNIT* n
  #define IMPORT_CONTEXT_STATS(n)						struct STATS* n
  #define IMPORT_INDEX(t, n)							int n
  #define IMPORT_INDEXN(t, i, n)						int n
  #define IMPORT_STAT_PARAM(n)							int n
#else
  #define RETVAL_INT									TYPE_INT
  #define RETVAL_FLOAT									TYPE_FLOAT
  #define IMPORT_FUNC(f, ret, n)						{ SSYMBOL* sym = AddImportFunction(#n, (void*)f, ret);
  #define IMPORT_END									; EndImportFunction(sym); }
  #define IMPORT_VMSTATE								AddImportVMStateParam(sym)
  #define IMPORT_CONTEXT								AddImportContext(sym)
  #define IMPORT_GAME									AddImportGameParam(sym)
  #define IMPORT_INT(n)									AddImportParam(sym, #n, TYPE_INT)
  #define IMPORT_CONTEXT_UNIT(n)						AddImportCntxParam(sym, #n, PARAM_CONTEXT_UNIT)
  #define IMPORT_CONTEXT_STATS(n)						AddImportCntxParam(sym, #n, PARAM_CONTEXT_STATS)
  #define IMPORT_INDEX(t, n)							AddImportIdxParam(sym, t, #n, TYPE_INT)
  #define IMPORT_INDEXN(t, i, n)						AddImportIdxParam(sym, t, i, #n, TYPE_INT)
  #define IMPORT_STAT_PARAM(n)							AddImportParamParam(sym, #n, TYPE_INT)
#endif
