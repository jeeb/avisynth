#ifdef AVS_WINDOWS
#include "PluginManager.h"
#include <avisynth.h>
#include <unordered_set>
#include <avisynth_c.h>
#include "strings.h"
#include "InternalEnvironment.h"
#include <cassert>
#include <imagehlp.h>

typedef const char* (__stdcall *AvisynthPluginInit3Func)(IScriptEnvironment* env, const AVS_Linkage* const vectors);
typedef const char* (__stdcall *AvisynthPluginInit2Func)(IScriptEnvironment* env);
typedef const char* (AVSC_CC *AvisynthCPluginInitFunc)(AVS_ScriptEnvironment* env);

const char RegAvisynthKey[] = "Software\\Avisynth";
#if defined (__GNUC__)
const char RegPluginDirPlus_GCC[] = "PluginDir+GCC";
#if defined(X86_32)
  #define GCC_WIN32
#endif
#else
const char RegPluginDirClassic[] = "PluginDir2_5";
const char RegPluginDirPlus[] = "PluginDir+";
#endif

/*
---------------------------------------------------------------------------------
---------------------------------------------------------------------------------
                                 Static helpers
---------------------------------------------------------------------------------
---------------------------------------------------------------------------------
*/

// Translates a Windows error code to a human-readable text message.
static std::string GetLastErrorText(DWORD nErrorCode)
{
  char* msg;
  // Ask Windows to prepare a standard message for a GetLastError() code:
  if (0 == FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, nErrorCode, 0, (LPSTR)&msg, 0, NULL))
    return("Unknown error");
  else
  {
    std::string ret(msg);
    LocalFree(msg);
    return ret;
  }
}

static bool GetRegString(HKEY rootKey, const char path[], const char entry[], std::string *result) {
    HKEY AvisynthKey;

    if (RegOpenKeyEx(rootKey, path, 0, KEY_READ, &AvisynthKey))
      return false;

    DWORD size;
    if (ERROR_SUCCESS != RegQueryValueEx(AvisynthKey, entry, 0, 0, 0, &size)) {
      RegCloseKey(AvisynthKey); // Dave Brueck - Dec 2005
      return false;
    }

    char* retStr = new(std::nothrow) char[size];
    if ((retStr == NULL) || (ERROR_SUCCESS != RegQueryValueEx(AvisynthKey, entry, 0, 0, (LPBYTE)retStr, &size))) {
      delete[] retStr;
      RegCloseKey(AvisynthKey); // Dave Brueck - Dec 2005
      return false;
    }
    RegCloseKey(AvisynthKey); // Dave Brueck - Dec 2005

    *result = std::string(retStr);
    delete[] retStr;
    return true;
}

static std::string GetFullPathNameWrap(const std::string &f)
{
  // Get the lenght of the buffer we need
  DWORD len = GetFullPathName(f.c_str(), 0, NULL, NULL);

  // Reserve space and make call
  char *fullPathName = new char[len];
  GetFullPathName(f.c_str(), len, fullPathName, NULL);

  // Wrap into C++ string
  std::string result(fullPathName);

  // Cleanup and return
  delete [] fullPathName;
  return result;
}

static bool IsParameterTypeSpecifier(char c) {
  switch (c) {
  case 'b': case 'i': case 'f': case 's': case 'c': case '.':
#ifdef NEW_AVSVALUE
  case 'a': // Arrays as function parameters
#endif
      return true;
    default:
      return false;
  }
}

static bool IsParameterTypeModifier(char c) {
  switch (c) {
    case '+': case '*':
      return true;
    default:
      return false;
  }
}

static bool IsValidParameterString(const char* p) {
  int state = 0;
  char c;
  while ((c = *p++) != '\0' && state != -1) {
    switch (state) {
      case 0:
        if (IsParameterTypeSpecifier(c)) {
          state = 1;
        }
        else if (c == '[') {
          state = 2;
        }
        else {
          state = -1;
        }
        break;

      case 1:
        if (IsParameterTypeSpecifier(c)) {
          // do nothing; stay in the current state
        }
        else if (c == '[') {
          state = 2;
        }
        else if (IsParameterTypeModifier(c)) {
          state = 0;
        }
        else {
          state = -1;
        }
        break;

      case 2:
        if (c == ']') {
          state = 3;
        }
        else {
          // do nothing; stay in the current state
        }
        break;

      case 3:
        if (IsParameterTypeSpecifier(c)) {
          state = 1;
        }
        else {
          state = -1;
        }
        break;
    }
  }

  // states 0, 1 are the only ending states we accept
  return state == 0 || state == 1;
}

/*
---------------------------------------------------------------------------------
---------------------------------------------------------------------------------
                                 AVSFunction
---------------------------------------------------------------------------------
---------------------------------------------------------------------------------
*/

AVSFunction::AVSFunction(void*) :
    AVSFunction(NULL, NULL, NULL, NULL, NULL, NULL)
{}

AVSFunction::AVSFunction(const char* _name, const char* _plugin_basename, const char* _param_types, apply_func_t _apply) :
    AVSFunction(_name, _plugin_basename, _param_types, _apply, NULL, NULL)
{}

AVSFunction::AVSFunction(const char* _name, const char* _plugin_basename, const char* _param_types, apply_func_t _apply, void *_user_data) :
    AVSFunction(_name, _plugin_basename, _param_types, _apply, _user_data, NULL)
{}

AVSFunction::AVSFunction(const char* _name, const char* _plugin_basename, const char* _param_types, apply_func_t _apply, void *_user_data, const char* _dll_path) :
    apply(_apply), name(NULL), canon_name(NULL), param_types(NULL), user_data(_user_data), dll_path(NULL)
{
    if (NULL != _dll_path)
    {
        size_t len = strlen(_dll_path);
        dll_path = new char[len + 1];
        memcpy(dll_path, _dll_path, len);
        dll_path[len] = 0;
    }

    if (NULL != _name)
    {
        size_t len = strlen(_name);
        name = new char[len + 1];
        memcpy(name, _name, len);
        name[len] = 0;
    }

    if ( NULL != _param_types )
    {
        size_t len = strlen(_param_types);
        param_types = new char[len+1];
        memcpy(param_types, _param_types, len);
        param_types[len] = 0;
    }

    if ( NULL != _name )
    {
		std::string cn(NULL != _plugin_basename ? _plugin_basename : "");
        cn.append("_").append(_name);
        canon_name = new char[cn.size()+1];
        memcpy(canon_name, cn.c_str(), cn.size());
        canon_name[cn.size()] = 0;
    }
}

AVSFunction::~AVSFunction()
{
    delete [] canon_name;
    delete [] name;
    delete [] param_types;
    delete [] dll_path;
}

bool AVSFunction::empty() const
{
    return NULL == name;
}

bool AVSFunction::IsScriptFunction() const
{
#ifdef DEBUG_GSCRIPTCLIP_MT
  /*
  if (!strcmp(this->name, "YPlaneMax"))
    return true;
  if (!strcmp(this->name, "YPlaneMin"))
    return true;
  if (!strcmp(this->name, "LumaDifference"))
    return true;
    */
/*
  if (!stricmp(this->name, "yplanemax"))
    return true;
  if (!stricmp(this->name, "yplanemin"))
    return true;
  if (!stricmp(this->name, "lumadifference"))
    return true;
  */
  //if (!stricmp(this->name, "srestore_inside_1"))
  //  return true;
#endif
  return ( (apply == &(ScriptFunction::Execute))
		  || (apply == &Eval)
          || (apply == &EvalOop)
          || (apply == &Import)
        );
}

#ifdef DEBUG_GSCRIPTCLIP_MT
bool AVSFunction::IsRuntimeScriptFunction() const
{

  if (!strcmp(this->name, "YPlaneMax"))
  return true;
  if (!strcmp(this->name, "YPlaneMin"))
  return true;
  if (!strcmp(this->name, "LumaDifference"))
  return true;

  //if (!stricmp(this->name, "srestore_inside_1"))
  //  return true;

  return (apply == &(ScriptFunction::Execute));
}
#endif

bool AVSFunction::SingleTypeMatch(char type, const AVSValue& arg, bool strict) {
  switch (type) {
    case '.': return true;
    case 'b': return arg.IsBool();
    case 'i': return arg.IsInt();
    case 'f': return arg.IsFloat() && (!strict || !arg.IsInt());
    case 's': return arg.IsString();
    case 'c': return arg.IsClip();
#ifdef NEW_AVSVALUE
    case 'a': return arg.IsArray(); // PF 161028 AVS+ script arrays
#endif
    default:  return false;
  }
}

bool AVSFunction::TypeMatch(const char* param_types, const AVSValue* args, size_t num_args, bool strict, IScriptEnvironment* env) {

  bool optional = false;

  /*
  { "StackHorizontal", BUILTIN_FUNC_PREFIX, "cc+", StackHorizontal::Create },
  { "Spline", BUILTIN_FUNC_PREFIX, "[x]ff+[cubic]b", Spline },
  { "Select",   BUILTIN_FUNC_PREFIX, "i.+", Select },
  { "Array", BUILTIN_FUNC_PREFIX, ".#", ArrayCreate },  // # instead of +: creates script array

  { "IsArray",   BUILTIN_FUNC_PREFIX, ".", IsArray },
  { "ArrayGet",  BUILTIN_FUNC_PREFIX, "ai", ArrayGet },
  { "ArrayGet",  BUILTIN_FUNC_PREFIX, "as", ArrayGet },
  { "ArraySize", BUILTIN_FUNC_PREFIX, "a", ArraySize },
  */
  // arguments are provided in a flattened way (flattened=array elements extracted)
  // e.g.    string array is provided here string,string,string
  size_t i = 0;
  while (i < num_args) {

    if (*param_types == '\0') {
      // more args than params
      return false;
    }

    if (*param_types == '[') {
      // named arg: skip over the name
      param_types = strchr(param_types+1, ']');
      if (param_types == NULL) {
        env->ThrowError("TypeMatch: unterminated parameter name (bug in filter)");
      }

      ++param_types;
      optional = true;

      if (*param_types == '\0') {
        env->ThrowError("TypeMatch: no type specified for optional parameter (bug in filter)");
      }
    }

    if (param_types[1] == '*') {
      // skip over initial test of type for '*' (since zero matches is ok)
      ++param_types;
    }

    switch (*param_types) {
      case 'b': case 'i': case 'f': case 's': case 'c':
#ifdef NEW_AVSVALUE
      case 'a': // PF Arrays
#endif
        if (   (!optional || args[i].Defined())
            && !SingleTypeMatch(*param_types, args[i], strict))
          return false;
        // fall through
      case '.':
        ++param_types;
        ++i;
        break;
      case '+': case '*':
#ifdef NEW_AVSVALUE
        if (param_types[-1] != '.' && args[i].IsArray()) { // PF new Arrays
          // all elements in the array should match with the type char preceding '+*'
          // only one array level is enough
          for (int j = 0; j < args[i].ArraySize(); j++)
          {
            if (!SingleTypeMatch(param_types[-1], args[i][j], strict))
              return false;
          }
          // we're done with the + or *
          ++param_types;
          ++i;
        }
        else
#endif
        if (!SingleTypeMatch(param_types[-1], args[i], strict)) {
          // we're done with the + or *
          ++param_types;
        }
        else {
          ++i;
        }
        break;
      default:
        env->ThrowError("TypeMatch: invalid character in parameter list (bug in filter)");
    }
  }

  // We're out of args.  We have a match if one of the following is true:
  // (a) we're out of params.
  // (b) remaining params are named i.e. optional.
  // (c) we're at a '+' or '*' and any remaining params are optional.

  if (*param_types == '+'  || *param_types == '*')
    param_types += 1;

  if (*param_types == '\0' || *param_types == '[')
    return true;

  while (param_types[1] == '*') {
    param_types += 2;
    if (*param_types == '\0' || *param_types == '[')
      return true;
  }

  return false;
}

bool AVSFunction::ArgNameMatch(const char* param_types, size_t args_names_count, const char* const* arg_names) {

  for (size_t i=0; i<args_names_count; ++i) {
    if (arg_names[i]) {
      bool found = false;
      size_t len = strlen(arg_names[i]);
      for (const char* p = param_types; *p; ++p) {
        if (*p == '[') {
          p += 1;
          const char* q = strchr(p, ']');
          if (!q) return false;
          if (len == q-p && !_strnicmp(arg_names[i], p, q-p)) {
            found = true;
            break;
          }
          p = q+1;
        }
      }
      if (!found) return false;
    }
  }
  return true;
}

/*
---------------------------------------------------------------------------------
---------------------------------------------------------------------------------
                                 PluginFile
---------------------------------------------------------------------------------
---------------------------------------------------------------------------------
*/

#include <avs/win.h>
struct PluginFile
{
  std::string FilePath;             // Fully qualified, canonical file path
  std::string BaseName;             // Only file name, without extension
  HMODULE Library;                  // LoadLibrary handle

  PluginFile(const std::string &filePath);
};

PluginFile::PluginFile(const std::string &filePath) :
  FilePath(GetFullPathNameWrap(filePath)), BaseName(), Library(NULL)
{
  // Turn all '\' into '/'
  replace(FilePath, '\\', '/');

  // Find position of dot in extension
  size_t dot_pos = FilePath.rfind('.');

  // Find position of last directory slash
  size_t slash_pos = FilePath.rfind('/');

  // Extract basename
  if ((dot_pos != std::string::npos) && (slash_pos != std::string::npos))
  {// we have both a slash and a dot
    if (dot_pos > slash_pos)
      BaseName = FilePath.substr(slash_pos+1, dot_pos - slash_pos - 1);
    else
      BaseName = FilePath.substr(slash_pos+1, std::string::npos);
  }
  else if ((dot_pos == std::string::npos) && (slash_pos != std::string::npos))
  {// we have a slash but no dot
    // Extract basename
    BaseName = FilePath.substr(slash_pos+1, std::string::npos);
  }
  else
  {// everything else
    // Because we have used GetFullPathName, FilePath should contain an absolute path,
    // meaning that this case should be unreachable, but the devil never sleeps.
    assert(0);
    BaseName = FilePath;
  }
}

/*
---------------------------------------------------------------------------------
---------------------------------------------------------------------------------
                                 PluginManager
---------------------------------------------------------------------------------
---------------------------------------------------------------------------------
*/

PluginManager::PluginManager(InternalEnvironment* env) :
  Env(env), PluginInLoad(NULL), AutoloadExecuted(false), Autoloading(false)
{
  env->SetGlobalVar("$PluginFunctions$", AVSValue(""));
}

void PluginManager::ClearAutoloadDirs()
{
  if (AutoloadExecuted)
    Env->ThrowError("Cannot modify directory list after the autoload procedure has already executed.");

  AutoloadDirs.clear();
}

void PluginManager::AddAutoloadDir(const std::string &dirPath, bool toFront)
{
  if (AutoloadExecuted)
    Env->ThrowError("Cannot modify directory list after the autoload procedure has already executed.");

  std::string dir(dirPath);

  // get folder of our executable
  TCHAR ExeFilePath[AVS_MAX_PATH];
  memset(ExeFilePath, 0, sizeof(ExeFilePath[0])*AVS_MAX_PATH);  // WinXP does not terminate the result of GetModuleFileName with a zero, so me must zero our buffer
  GetModuleFileName(NULL, ExeFilePath, AVS_MAX_PATH);
  std::string ExeFileDir(ExeFilePath);
  replace(ExeFileDir, '\\', '/');
  ExeFileDir = ExeFileDir.erase(ExeFileDir.rfind('/'), std::string::npos);

  // variable expansion
  replace_beginning(dir, "SCRIPTDIR", Env->GetVar("$ScriptDir$", ""));
  replace_beginning(dir, "MAINSCRIPTDIR", Env->GetVar("$MainScriptDir$", ""));
  replace_beginning(dir, "PROGRAMDIR", ExeFileDir);

  std::string plugin_dir;
#if defined (__GNUC__)
  if (GetRegString(HKEY_CURRENT_USER, RegAvisynthKey, RegPluginDirPlus_GCC, &plugin_dir))
    replace_beginning(dir, "USER_PLUS_PLUGINS", plugin_dir);
  if (GetRegString(HKEY_LOCAL_MACHINE, RegAvisynthKey, RegPluginDirPlus_GCC, &plugin_dir))
    replace_beginning(dir, "MACHINE_PLUS_PLUGINS", plugin_dir);
#else
  if (GetRegString(HKEY_CURRENT_USER, RegAvisynthKey, RegPluginDirPlus, &plugin_dir))
    replace_beginning(dir, "USER_PLUS_PLUGINS", plugin_dir);
  if (GetRegString(HKEY_LOCAL_MACHINE, RegAvisynthKey, RegPluginDirPlus, &plugin_dir))
    replace_beginning(dir, "MACHINE_PLUS_PLUGINS", plugin_dir);
  if (GetRegString(HKEY_CURRENT_USER, RegAvisynthKey, RegPluginDirClassic, &plugin_dir))
    replace_beginning(dir, "USER_CLASSIC_PLUGINS", plugin_dir);
  if (GetRegString(HKEY_LOCAL_MACHINE, RegAvisynthKey, RegPluginDirClassic, &plugin_dir))
    replace_beginning(dir, "MACHINE_CLASSIC_PLUGINS", plugin_dir);
#endif

  // replace backslashes with forward slashes
  replace(dir, '\\', '/');

  // append terminating slash if needed
  if (dir.size() > 0 && dir[dir.size()-1] != '/')
    dir.append("/");

  // remove double slashes
  while(replace(dir, "//", "/"));

  if (toFront)
    AutoloadDirs.insert(AutoloadDirs.begin(), GetFullPathNameWrap(dir));
  else
    AutoloadDirs.push_back(GetFullPathNameWrap(dir));
}

void PluginManager::AutoloadPlugins()
{
  if (AutoloadExecuted)
    return;

  AutoloadExecuted = true;
  Autoloading = true;

  const char *binaryFilter = "*.dll";
  const char *scriptFilter = "*.avsi";

  // Load binary plugins
  for (const std::string& dir : AutoloadDirs)
  {
    // Append file search filter to directory path
    std::string filePattern = concat(dir, binaryFilter);

    // Iterate through all files in directory
    WIN32_FIND_DATA fileData;
    HANDLE hFind = FindFirstFile(filePattern.c_str(), &fileData);
    for (BOOL bContinue = (hFind != INVALID_HANDLE_VALUE);
          bContinue;
          bContinue = FindNextFile(hFind, &fileData)
        )
    {
      if ((fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)  // do not add directories
      {
        PluginFile p(concat(dir, fileData.cFileName));

        // Search for loaded plugins with the same base name.
        for (size_t i = 0; i < AutoLoadedPlugins.size(); ++i)
        {
          if (streqi(AutoLoadedPlugins[i].BaseName.c_str(), p.BaseName.c_str()))
          {
            // Prevent loading a plugin with a basename that is
            // already loaded (from another autoload folder).
            continue;
          }
        }

        // Try to load plugin
        AVSValue dummy;
        LoadPlugin(p, false, &dummy);
      }
    } // for bContinue
    FindClose(hFind);
  }

  // Load script imports
  for (const std::string& dir : AutoloadDirs)
  {
    CWDChanger cwdchange(dir.c_str());

    // Append file search filter to directory path
    std::string filePattern = concat(dir, scriptFilter);

    // Iterate through all files in directory
    WIN32_FIND_DATA fileData;
    HANDLE hFind = FindFirstFile(filePattern.c_str(), &fileData);
    for (BOOL bContinue = (hFind != INVALID_HANDLE_VALUE);
          bContinue;
          bContinue = FindNextFile(hFind, &fileData)
        )
    {
      if ((fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)  // do not add directories
      {
        PluginFile p(concat(dir, fileData.cFileName));

        // Search for loaded imports with the same base name.
        for (size_t i = 0; i < AutoLoadedImports.size(); ++i)
        {
          if (streqi(AutoLoadedImports[i].BaseName.c_str(), p.BaseName.c_str()))
          {
            // Prevent loading a plugin with a basename that is
            // already loaded (from another autoload folder).
            continue;
          }
        }

        // Try to load script
        Env->Invoke("Import", p.FilePath.c_str());
        AutoLoadedImports.push_back(p);
      }
    } // for bContinue
    FindClose(hFind);
  }

  Autoloading = false;
}

PluginManager::~PluginManager()
{
  // Delete all AVSFunction objects that we created
  std::unordered_set<const AVSFunction*> function_set;
  for (const auto& lists : ExternalFunctions)
  {
      const FunctionList& funcList = lists.second;
      for (const auto& func : funcList)
        function_set.insert(func);
  }
  for (const auto& lists : AutoloadedFunctions)
  {
      const FunctionList& funcList = lists.second;
      for (const auto& func : funcList)
        function_set.insert(func);
  }
  for (const auto& func : function_set)
  {
      delete func;
  }


  // Unload plugin binaries
  for (size_t i = 0; i < LoadedPlugins.size(); ++i)
  {
    assert(LoadedPlugins[i].Library);
    FreeLibrary(LoadedPlugins[i].Library);
    LoadedPlugins[i].Library = NULL;
  }
  for (size_t i = 0; i < AutoLoadedPlugins.size(); ++i)
  {
    assert(AutoLoadedPlugins[i].Library);
    FreeLibrary(AutoLoadedPlugins[i].Library);
    AutoLoadedPlugins[i].Library = NULL;
  }

  Env = NULL;
  PluginInLoad = NULL;
}

void PluginManager::UpdateFunctionExports(const char* funcName, const char* funcParams, const char *exportVar)
{
  if (exportVar == NULL)
    exportVar = "$PluginFunctions$";

  // Update $PluginFunctions$
  const char *oldFnList = Env->GetVar(exportVar, "");
  std::string FnList(oldFnList);
  if (FnList.size() > 0)    // if the list is not empty...
    FnList.push_back(' ');  // ...add a delimiting whitespace
  FnList.append(funcName);
  Env->SetGlobalVar(exportVar, AVSValue( Env->SaveString(FnList.c_str(), (int)FnList.size()) ));

  // Update $Plugin!...!Param$
  std::string param_id;
  param_id.reserve(128);
  param_id.append("$Plugin!");
  param_id.append(funcName);
  param_id.append("!Param$");
  Env->SetGlobalVar(Env->SaveString(param_id.c_str(), (int)param_id.size()), AVSValue(Env->SaveString(funcParams)));
}

bool PluginManager::LoadPlugin(const char* path, bool throwOnError, AVSValue *result)
{
  auto pf = PluginFile { path };
  return LoadPlugin(pf, throwOnError, result);
}

static bool Is64BitDLL(std::string sDLL, bool &bIs64BitDLL)
{
  bIs64BitDLL = false;
  LOADED_IMAGE li;

  if (!MapAndLoad((LPSTR)sDLL.c_str(), NULL, &li, TRUE, TRUE))
  {
    //error handling (check GetLastError())
    return false;
  }

  if (li.FileHeader->FileHeader.Machine != IMAGE_FILE_MACHINE_I386) //64 bit image
    bIs64BitDLL = true;

  UnMapAndLoad(&li);

  return true;
}

bool PluginManager::LoadPlugin(PluginFile &plugin, bool throwOnError, AVSValue *result)
{
  std::vector<PluginFile>& PluginList = Autoloading ? AutoLoadedPlugins : LoadedPlugins;

  for (size_t i = 0; i < PluginList.size(); ++i)
  {
    if (streqi(PluginList[i].FilePath.c_str(), plugin.FilePath.c_str()))
    {
      // Imitate successful loading if the plugin is already loaded
      plugin = PluginList[i];
      return true;
    }
  }

  // Search for dependent DLLs in the plugin's directory too
  size_t slash_pos = plugin.FilePath.rfind('/');
  std::string plugin_dir = plugin.FilePath.substr(0, slash_pos);;
  DllDirChanger dllchange(plugin_dir.c_str());

  // Load the dll into memory
  plugin.Library = LoadLibraryEx(plugin.FilePath.c_str(), 0, LOAD_WITH_ALTERED_SEARCH_PATH);
  if (plugin.Library == NULL)
  {
    DWORD errCode = GetLastError();

    // Bitness mixing always throws an error, regardless of throwOnError state
    // By this new behaviour even plugin auto-load will fail
    bool bIs64BitDLL;
    bool succ = Is64BitDLL(plugin.FilePath, bIs64BitDLL);
    if (succ) {
      const bool selfIs32 = sizeof(void *) == 4;
      if (selfIs32 && bIs64BitDLL)
        Env->ThrowError("Cannot load a 64 bit DLL in 32 bit Avisynth: '%s'.\n", plugin.FilePath.c_str());
      if (!selfIs32 && !bIs64BitDLL)
        Env->ThrowError("Cannot load a 32 bit DLL in 64 bit Avisynth: '%s'.\n", plugin.FilePath.c_str());
    }
    if (throwOnError)
    {
      Env->ThrowError("Cannot load file '%s'. Platform returned code %d:\n%s", plugin.FilePath.c_str(), errCode, GetLastErrorText(errCode).c_str());
    }
    else
      return false;
  }

  // Try to load various plugin interfaces
  if (!TryAsAvs26(plugin, result))
  {
    if (!TryAsAvsC(plugin, result))
    {
      if (!TryAsAvs25(plugin, result))
      {
        FreeLibrary(plugin.Library);
        plugin.Library = NULL;

        if (throwOnError)
          Env->ThrowError("'%s' cannot be used as a plugin for AviSynth.", plugin.FilePath.c_str());
        else
          return false;
      }
    }
  }

  PluginList.push_back(plugin);
  return true;
}

const AVSFunction* PluginManager::Lookup(const FunctionMap& map, const char* search_name, const AVSValue* args, size_t num_args,
                    bool strict, size_t args_names_count, const char* const* arg_names) const
{
    FunctionMap::const_iterator list_it = map.find(search_name);
    if (list_it == map.end())
      return NULL;

    for ( FunctionList::const_reverse_iterator func_it = list_it->second.rbegin();
          func_it != list_it->second.rend();
          ++func_it)
    {
      const AVSFunction *func = *func_it;
      if (AVSFunction::TypeMatch(func->param_types, args, num_args, strict, Env) &&
          AVSFunction::ArgNameMatch(func->param_types, args_names_count, arg_names)
         )
      {
        return func;
      }
    }

    return NULL;
}

const AVSFunction* PluginManager::Lookup(const char* search_name, const AVSValue* args, size_t num_args,
                    bool strict, size_t args_names_count, const char* const* arg_names) const
{
  /* Lookup in non-autoloaded functions first, so that they take priority */
  const AVSFunction* func = Lookup(ExternalFunctions, search_name, args, num_args, strict, args_names_count, arg_names);
  if (func != NULL)
    return func;

  /* If not found, look amongst the autoloaded */
  return Lookup(AutoloadedFunctions, search_name, args, num_args, strict, args_names_count, arg_names);
}

bool PluginManager::FunctionExists(const char* name) const
{
    bool autoloaded = (AutoloadedFunctions.find(name) != AutoloadedFunctions.end());
    return autoloaded || (ExternalFunctions.find(name) != ExternalFunctions.end());
}

// A minor helper function
static bool FunctionListHasDll(const FunctionList &list, const char *dll_path)
{
    for (const auto &f : list) {
        if ( (nullptr == f->dll_path) || (nullptr == dll_path) ) {
            if (f->dll_path == dll_path) {
                return true;
            }
        } else if (streqi(f->dll_path, dll_path)) {
            return true;
        }
    }
    return false;
}

void PluginManager::AddFunction(const char* name, const char* params, IScriptEnvironment::ApplyFunc apply, void* user_data, const char *exportVar)
{
  if (!IsValidParameterString(params))
    Env->ThrowError("%s has an invalid parameter string (bug in filter)", name);

  FunctionMap& functions = Autoloading ? AutoloadedFunctions : ExternalFunctions;

  AVSFunction *newFunc = NULL;
  if (PluginInLoad != NULL)
  {
      newFunc = new AVSFunction(name, PluginInLoad->BaseName.c_str(), params, apply, user_data, PluginInLoad->FilePath.c_str());
  }
  else
  {
      newFunc = new AVSFunction(name, NULL, params, apply, user_data, NULL);
      if(apply != &create_c_video_filter)
        assert(newFunc->IsScriptFunction());
  }

  // Warn user if a function with the same name is already registered by another plugin
  {
      const auto &it = functions.find(newFunc->name);
      if ( (functions.end() != it) && !FunctionListHasDll(it->second, newFunc->dll_path) )
      {
          OneTimeLogTicket ticket(LOGTICKET_W1008, newFunc->name);
          Env->LogMsgOnce(ticket, LOGLEVEL_WARNING, "%s() is defined by multiple plugins. Calls to this filter might be ambiguous and could result in the wrong function being called.", newFunc->name);
      }
  }

  functions[newFunc->name].push_back(newFunc);
  UpdateFunctionExports(newFunc->name, newFunc->param_types, exportVar);

  if (NULL != newFunc->canon_name)
  {
      // Warn user if a function with the same name is already registered by another plugin
      {
          const auto &it = functions.find(newFunc->canon_name);
          if ((functions.end() != it) && !FunctionListHasDll(it->second, newFunc->dll_path))
          {
              OneTimeLogTicket ticket(LOGTICKET_W1008, newFunc->canon_name);
              Env->LogMsgOnce(ticket, LOGLEVEL_WARNING, "%s() is defined by multiple plugins. Calls to this filter might be ambiguous and could result in the wrong function being called.", newFunc->name);
          }
      }

      functions[newFunc->canon_name].push_back(newFunc);
      UpdateFunctionExports(newFunc->canon_name, newFunc->param_types, exportVar);
  }
}

std::string PluginManager::PluginLoading() const
{
    if (NULL == PluginInLoad)
        return std::string();
    else
        return PluginInLoad->BaseName;
}

bool PluginManager::TryAsAvs26(PluginFile &plugin, AVSValue *result)
{
  extern const AVS_Linkage* const AVS_linkage; // In interface.cpp
#ifdef GCC_WIN32
  AvisynthPluginInit3Func AvisynthPluginInit3 = (AvisynthPluginInit3Func)GetProcAddress(plugin.Library, "_AvisynthPluginInit3");
  if (!AvisynthPluginInit3)
    AvisynthPluginInit3 = (AvisynthPluginInit3Func)GetProcAddress(plugin.Library, "AvisynthPluginInit3@8");
#else
  AvisynthPluginInit3Func AvisynthPluginInit3 = (AvisynthPluginInit3Func)GetProcAddress(plugin.Library, "AvisynthPluginInit3");
  if (!AvisynthPluginInit3)
    AvisynthPluginInit3 = (AvisynthPluginInit3Func)GetProcAddress(plugin.Library, "_AvisynthPluginInit3@8");
#endif

  if (AvisynthPluginInit3 == NULL)
    return false;
  else
  {
    PluginInLoad = &plugin;
    *result = AvisynthPluginInit3(Env, AVS_linkage);
    PluginInLoad = NULL;
  }

  return true;
}

bool PluginManager::TryAsAvs25(PluginFile &plugin, AVSValue *result)
{
#ifdef GCC_WIN32
  AvisynthPluginInit2Func AvisynthPluginInit2 = (AvisynthPluginInit2Func)GetProcAddress(plugin.Library, "_AvisynthPluginInit2");
  if (!AvisynthPluginInit2)
    AvisynthPluginInit2 = (AvisynthPluginInit2Func)GetProcAddress(plugin.Library, "AvisynthPluginInit2@4");
#else
  AvisynthPluginInit2Func AvisynthPluginInit2 = (AvisynthPluginInit2Func)GetProcAddress(plugin.Library, "AvisynthPluginInit2");
  if (!AvisynthPluginInit2)
    AvisynthPluginInit2 = (AvisynthPluginInit2Func)GetProcAddress(plugin.Library, "_AvisynthPluginInit2@4");
#endif

  if (AvisynthPluginInit2 == NULL)
    return false;
  else
  {
    PluginInLoad = &plugin;
    *result = AvisynthPluginInit2(Env);
    PluginInLoad = NULL;
  }

  return true;
}

bool PluginManager::TryAsAvsC(PluginFile &plugin, AVSValue *result)
{
#ifdef _WIN64
  AvisynthCPluginInitFunc AvisynthCPluginInit = (AvisynthCPluginInitFunc)GetProcAddress(plugin.Library, "avisynth_c_plugin_init");
  if (!AvisynthCPluginInit)
    AvisynthCPluginInit = (AvisynthCPluginInitFunc)GetProcAddress(plugin.Library, "_avisynth_c_plugin_init@4");
#else // _WIN32
  AvisynthCPluginInitFunc AvisynthCPluginInit = (AvisynthCPluginInitFunc)GetProcAddress(plugin.Library, "_avisynth_c_plugin_init@4");
  if (!AvisynthCPluginInit)
    AvisynthCPluginInit = (AvisynthCPluginInitFunc)GetProcAddress(plugin.Library, "avisynth_c_plugin_init@4");
#endif

  if (AvisynthCPluginInit == NULL)
    return false;
  else
  {
    PluginInLoad = &plugin;
    {
        AVS_ScriptEnvironment e;
        e.env = Env;
        AVS_ScriptEnvironment *pe;
        pe = &e;
        const char *s = NULL;
#if defined(X86_32) && defined(MSVC)
        int callok = 1; // (stdcall)
        __asm // Tritical - Jan 2006
        {
            push eax
            push edx

            push 0x12345678		// Stash a known value

            mov eax, pe			// Env pointer
            push eax			// Arg1
            call AvisynthCPluginInit			// avisynth_c_plugin_init

            lea edx, s			// return value is in eax
            mov DWORD PTR[edx], eax

            pop eax				// Get top of stack
            cmp eax, 0x12345678	// Was it our known value?
            je end				// Yes! Stack was cleaned up, was a stdcall

            lea edx, callok
            mov BYTE PTR[edx], 0 // Set callok to 0 (_cdecl)

            pop eax				// Get 2nd top of stack
            cmp eax, 0x12345678	// Was this our known value?
            je end				// Yes! Stack is now correctly cleaned up, was a _cdecl

            mov BYTE PTR[edx], 2 // Set callok to 2 (bad stack)
    end:
            pop edx
            pop eax
        }
      switch(callok)
      {
      case 0:   // cdecl
#ifdef AVSC_USE_STDCALL
            Env->ThrowError("Avisynth 2 C Plugin '%s' has wrong calling convention! Must be _stdcall.", plugin.BaseName.c_str());
#endif
        break;
      case 1:   // stdcall
#ifndef AVSC_USE_STDCALL
            Env->ThrowError("Avisynth 2 C Plugin '%s' has wrong calling convention! Must be _cdecl.", plugin.BaseName.c_str());
#endif
        break;
      case 2:
        Env->ThrowError("Avisynth 2 C Plugin '%s' has corrupted the stack.", plugin.BaseName.c_str());
      }
#else
      s = AvisynthCPluginInit(pe);
#endif
//	    if (s == 0)
    //	    Env->ThrowError("Avisynth 2 C Plugin '%s' returned a NULL pointer.", plugin.BaseName.c_str());

      *result = AVSValue(s);
    }
    PluginInLoad = NULL;
  }

  return true;
}

/*
---------------------------------------------------------------------------------
---------------------------------------------------------------------------------
                                 LoadPlugin
---------------------------------------------------------------------------------
---------------------------------------------------------------------------------
*/

AVSValue LoadPlugin(AVSValue args, void*, IScriptEnvironment* env)
{
  IScriptEnvironment2 *env2 = static_cast<IScriptEnvironment2*>(env);

  bool success = true;
  for (int i = 0; i < args[0].ArraySize(); ++i)
  {
    AVSValue dummy;
    success &= env2->LoadPlugin(args[0][i].AsString(), true, &dummy);
  }

  return AVSValue(success);
}

extern const AVSFunction Plugin_functions[] = {
  {"LoadPlugin", BUILTIN_FUNC_PREFIX, "s+", LoadPlugin},
  {"LoadCPlugin", BUILTIN_FUNC_PREFIX, "s+", LoadPlugin },          // for compatibility with older scripts
  {"Load_Stdcall_Plugin", BUILTIN_FUNC_PREFIX, "s+", LoadPlugin },  // for compatibility with older scripts
  { 0 }
};
#endif
