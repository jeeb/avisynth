#ifndef AVSCORE_PLUGINS_H
#define AVSCORE_PLUGINS_H

#include <string>
#include <map>
#include <vector>
#include <avs/win.h>
#include "internal.h"

class IScriptEnvironment2;

struct PluginFile
{
  std::string FilePath; // Fully qualified, canonocal file path
  std::string BaseName; // Only file name, without extension
  HMODULE Library;      // LoadLibrary handle

  PluginFile(const std::string &filePath);
};

struct stdstricomparer
{
  bool operator() (const std::string& lhs, const std::string& rhs) const
  {
    return (_strcmpi(lhs.c_str(), rhs.c_str()) < 0);
  }
};

typedef std::multimap<std::string,AVSFunction,stdstricomparer> FunctionMap;
class PluginManager
{
private:
  IScriptEnvironment2 *Env;
  PluginFile *PluginInLoad;
  std::vector<std::string> AutoloadDirs;
  std::vector<PluginFile> LoadedPlugins;
  std::vector<PluginFile> LoadedImports;
  FunctionMap PluginFunctions;
  bool AutoloadExecuted;

  bool TryAsAvs26(PluginFile &plugin, AVSValue *result);
  bool TryAsAvs25(PluginFile &plugin, AVSValue *result);
  bool TryAsAvsC(PluginFile &plugin, AVSValue *result);

public:
  PluginManager(IScriptEnvironment2* env);
  ~PluginManager();

  void ClearAutoloadDirs();
  void AddAutoloadDir(const std::string &dir, bool toFront);
  bool LoadPlugin(PluginFile &plugin, bool throwOnError, AVSValue *result);

  bool HasAutoloadExecuted() const { return AutoloadExecuted; }

  bool FunctionExists(const char* name) const;
  void AutoloadPlugins();
  void AddFunction(const char* name, const char* params, IScriptEnvironment::ApplyFunc apply, void* user_data);
  const AVSFunction* Lookup(const char* search_name,
    const AVSValue* args,
    int num_args,
    bool strict,
    int args_names_count,
    const char* const* arg_names) const;
};

#endif  // AVSCORE_PLUGINS_H
