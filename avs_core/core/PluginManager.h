#ifndef AVSCORE_PLUGINS_H
#define AVSCORE_PLUGINS_H

#include <string>
#include <map>
#include <vector>
#include "internal.h"

class IScriptEnvironment2;
struct PluginFile;

struct StdStriComparer
{
  bool operator() (const std::string& lhs, const std::string& rhs) const
  {
    return (_strcmpi(lhs.c_str(), rhs.c_str()) < 0);
  }
};

typedef std::vector<AVSFunction> FunctionList;
typedef std::map<std::string,FunctionList,StdStriComparer> FunctionMap;
class PluginManager
{
private:
  IScriptEnvironment2 *Env;
  PluginFile *PluginInLoad;
  std::vector<std::string> AutoloadDirs;
  std::vector<PluginFile> AutoLoadedImports;
  std::vector<PluginFile> AutoLoadedPlugins;
  std::vector<PluginFile> LoadedPlugins;
  FunctionMap ExternalFunctions;
  FunctionMap AutoloadedFunctions;
  bool AutoloadExecuted;
  bool Autoloading;

  bool TryAsAvs26(PluginFile &plugin, AVSValue *result);
  bool TryAsAvs25(PluginFile &plugin, AVSValue *result);
  bool TryAsAvsC(PluginFile &plugin, AVSValue *result);
  void UpdateFunctionExports(const AVSFunction &func, const char *exportVar);
  
  const AVSFunction* Lookup(const FunctionMap& map,
    const char* search_name,
    const AVSValue* args,
    size_t num_args,
    bool strict,
    size_t args_names_count,
    const char* const* arg_names) const;


public:
  PluginManager(IScriptEnvironment2* env);
  ~PluginManager();

  void ClearAutoloadDirs();
  void AddAutoloadDir(const std::string &dir, bool toFront);

  bool LoadPlugin(PluginFile &plugin, bool throwOnError, AVSValue *result);
  bool LoadPlugin(const char* path, bool throwOnError, AVSValue *result);

  bool HasAutoloadExecuted() const { return AutoloadExecuted; }

  bool FunctionExists(const char* name) const;
  void AutoloadPlugins();
  void AddFunction(const char* name, const char* params, IScriptEnvironment::ApplyFunc apply, void* user_data, const char *exportVar);
  const AVSFunction* Lookup(const char* search_name,
    const AVSValue* args,
    size_t num_args,
    bool strict,
    size_t args_names_count,
    const char* const* arg_names) const;
};

#endif  // AVSCORE_PLUGINS_H
