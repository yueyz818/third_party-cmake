/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmGNTargetGenerator.h"

#include "cmGeneratedFileStream.h"
#include "cmGeneratorTarget.h"
#include "cmLinkLineComputer.h"
#include "cmLocalGNGenerator.h"
#include "cmMakefile.h"
#include "cmSourceFile.h"
#include "cmState.h"
#include "cmStateDirectory.h"

#include <iostream>

cmGNTargetGenerator::cmGNTargetGenerator(cmGeneratorTarget* target)
  : cmCommonTargetGenerator(target)
  , LocalGenerator(
      static_cast<cmLocalGNGenerator*>(target->GetLocalGenerator()))
{
  this->TargetLinkLanguage = target->GetLinkerLanguage(this->GetConfigName());
}

cmGNTargetGenerator::~cmGNTargetGenerator()
{
}

cmGeneratedFileStream& cmGNTargetGenerator::GetBuildFileStream() const
{
  return *this->GetGlobalGenerator()->GetBuildFileStream();
}

cmGlobalGNGenerator* cmGNTargetGenerator::GetGlobalGenerator() const
{
  return this->LocalGenerator->GetGlobalGNGenerator();
}

void cmGNTargetGenerator::Generate()
{
  std::string type;
  switch (this->GeneratorTarget->GetType()) {
    case cmStateEnums::OBJECT_LIBRARY:
      type = "source_set";
      break;
    case cmStateEnums::STATIC_LIBRARY:
      type = "static_library";
      break;
    case cmStateEnums::SHARED_LIBRARY:
      type = "shared_library";
      break;
    case cmStateEnums::EXECUTABLE:
      type = "executable";
      break;
    case cmStateEnums::MODULE_LIBRARY:
      type = "loadable_module";
      break;
    case cmStateEnums::INTERFACE_LIBRARY:
      type = "config";
      break;
    case cmStateEnums::UTILITY:
      type = "group";
      break;
    // GLOBAL_TARGET
    // UNKNOWN_LIBRARY
    default:
      type = "group";
      break;
  }

  std::vector<std::string> sources;
  std::map<std::string, std::string> variables;

  std::string config = this->Makefile->GetSafeDefinition("CMAKE_BUILD_TYPE");

  std::vector<cmSourceFile const*> cmSources;
  this->GeneratorTarget->GetObjectSources(cmSources, config);
  for (std::vector<cmSourceFile const*>::const_iterator i = cmSources.begin();
       i != cmSources.end(); ++i) {
    std::string sourcePath = this->LocalGenerator->ConvertToRelativePath(
      this->LocalGenerator->GetState()->GetBinaryDirectory(), (*i)->GetFullPath());;
    sources.push_back(sourcePath);
  }

  std::vector<std::string> deps;
  std::vector<std::string> depends;
  this->LocalGenerator->AppendTargetDepends(this->GeneratorTarget, depends);
  for (std::vector<std::string>::const_iterator i = depends.begin();
       i != depends.end(); ++i) {
    deps.push_back(":" + *i);
  }

  // Flags
  
  const std::string& linkLanguage =
    this->GetGeneratorTarget()->GetLinkerLanguage(config);
  
  std::string createRule = this->GeneratorTarget->GetCreateRuleVariable(
    this->TargetLinkLanguage, this->GetConfigName());
  //bool useWatcomQuote = this->Makefile->IsOn(createRule + "_USE_WATCOM_QUOTE");

  CM_AUTO_PTR<cmLinkLineComputer> linkLineComputer(
    this->GetGlobalGenerator()->CreateLinkLineComputer(
      this->LocalGenerator,
      this->LocalGenerator->GetStateSnapshot().GetDirectory()));
  //linkLineComputer->SetUseWatcomQuote(useWatcomQuote);

  std::string cflags1;
  this->LocalGenerator->AddLanguageFlags(cflags1, linkLanguage, config);
  std::cout << "[" << this->GetGeneratorTarget()->GetName() << "] language cflags:" << cflags1 << "\n";

  std::vector<std::string> flags;
  std::string s = this->GetFlags(linkLanguage);
  if (!s.empty())
    flags = cmSystemTools::tokenize(s, " ");
  std::cout << "[" << this->GetGeneratorTarget()->GetName() << "] flags:" << s << "\n";

  std::vector<std::string> defines;
  this->GeneratorTarget->GetCompileDefinitions(defines, this->ConfigName, linkLanguage);

  std::vector<std::string> includes;
  s = this->GetIncludes(linkLanguage);
  if (!s.empty())
    includes = cmSystemTools::tokenize(s, " ");

  std::string libs, cflags, ldflags, frameworkPath, linkPath;
  this->LocalGenerator->GetTargetFlags(
    linkLineComputer.get(), this->ConfigName, libs, cflags, ldflags,
    frameworkPath, linkPath, this->GeneratorTarget);
  std::cout << "[" << this->GetGeneratorTarget()->GetName() << "] target cflags:" << cflags << "\n";
  std::cout << "[" << this->GetGeneratorTarget()->GetName() << "] target ldflags:" << ldflags << "\n";
  std::cout << "[" << this->GetGeneratorTarget()->GetName() << "] target link:" << linkPath << "\n";

  this->GetGlobalGenerator()->WriteTarget(
    this->GetBuildFileStream(), "", type, this->GetGeneratorTarget()->GetName(),
    "", flags, defines, includes, sources, deps, variables);

  this->GetBuildFileStream() << "\n";
}

void cmGNTargetGenerator::AddIncludeFlags(std::string& languageFlags,
                                          std::string const& language)
{
#if 0
  std::vector<std::string> includes;
  this->LocalGenerator->GetIncludeDirectories(includes, this->GeneratorTarget,
                                              language, this->GetConfigName());
  // Add include directory flags.
  std::string includeFlags = this->LocalGenerator->GetIncludeFlags(
    includes, this->GeneratorTarget, language,
    language == "RC", // full include paths for RC needed by cmcldeps
    false, this->GetConfigName());
  if (this->GetGlobalGenerator()->IsGCCOnWindows()) {
    std::replace(includeFlags.begin(), includeFlags.end(), '\\', '/');
  }

  this->LocalGenerator->AppendFlags(languageFlags, includeFlags);
#endif
}
