/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmExportBuildJSONGenerator.h"

#include <algorithm>
#include <memory> // IWYU pragma: keep
#include <numeric>
#include <sstream>
#include <utility>

#include "cmExportSet.h"
#include "cmGeneratorExpression.h"
#include "cmGeneratorTarget.h"
#include "cmLinkItem.h"
#include "cmGlobalGenerator.h"
#include "cmLocalGenerator.h"
#include "cmMakefile.h"
#include "cmPolicies.h"
#include "cmStateTypes.h"
#include "cmSystemTools.h"
#include "cmTarget.h"
#include "cmake.h"

cmExportBuildJSONGenerator::cmExportBuildJSONGenerator()
{
  this->LG = nullptr;
  this->ExportSet = nullptr;
}

void cmExportBuildJSONGenerator::Compute(cmLocalGenerator* lg)
{
  this->LG = lg;
  if (this->ExportSet) {
    this->ExportSet->Compute(lg);
  }
}

bool cmExportBuildJSONGenerator::GenerateMainFile(std::ostream& os)
{
  {
    std::string expectedTargets;
    std::string sep;
    std::vector<std::string> targets;
    this->GetTargets(targets);
    for (std::string const& tei : targets) {
      cmGeneratorTarget* te = this->LG->FindGeneratorTargetToUse(tei);
      expectedTargets += sep + this->Namespace + te->GetExportName();
      sep = " ";
      if (this->ExportedTargets.insert(te).second) {
        this->Exports.push_back(te);
      } else {
        std::ostringstream e;
        e << "given target \"" << te->GetName() << "\" more than once.";
        this->LG->GetGlobalGenerator()->GetCMakeInstance()->IssueMessage(
          cmake::FATAL_ERROR, e.str(),
          this->LG->GetMakefile()->GetBacktrace());
        return false;
      }
    }
  }

  std::vector<std::string> missingTargets;

  os << "{\n"
     << "  \"targets\": {\n";

  // Create all the imported targets.
  for (auto i = this->Exports.begin(); i != this->Exports.end(); ++i) {
    cmGeneratorTarget* gte = *i;

    this->GenerateImportTargetCode(os, gte);

    gte->Target->AppendBuildInterfaceIncludes();

    ImportPropertyMap properties;

    this->PopulateInterfaceProperty("INTERFACE_INCLUDE_DIRECTORIES", gte,
                                    cmGeneratorExpression::BuildInterface,
                                    properties, missingTargets);
    this->PopulateInterfaceProperty("INTERFACE_SOURCES", gte,
                                    cmGeneratorExpression::BuildInterface,
                                    properties, missingTargets);
    this->PopulateInterfaceProperty("INTERFACE_COMPILE_DEFINITIONS", gte,
                                    cmGeneratorExpression::BuildInterface,
                                    properties, missingTargets);
    this->PopulateInterfaceProperty("INTERFACE_COMPILE_OPTIONS", gte,
                                    cmGeneratorExpression::BuildInterface,
                                    properties, missingTargets);
    this->PopulateInterfaceProperty("INTERFACE_AUTOUIC_OPTIONS", gte,
                                    cmGeneratorExpression::BuildInterface,
                                    properties, missingTargets);
    this->PopulateInterfaceProperty("INTERFACE_COMPILE_FEATURES", gte,
                                    cmGeneratorExpression::BuildInterface,
                                    properties, missingTargets);
    this->PopulateInterfaceProperty("INTERFACE_POSITION_INDEPENDENT_CODE", gte,
                                    properties);
    const bool newCMP0022Behavior =
      gte->GetPolicyStatusCMP0022() != cmPolicies::WARN &&
      gte->GetPolicyStatusCMP0022() != cmPolicies::OLD;
    if (newCMP0022Behavior) {
      this->PopulateInterfaceLinkLibrariesProperty(
        gte, cmGeneratorExpression::BuildInterface, properties,
        missingTargets);
    }
    this->PopulateCompatibleInterfaceProperties(gte, properties);

    this->GenerateInterfaceProperties(gte, os, properties);

    os << "    }";
    if (i != this->Exports.end() - 1)
      os << ",";
    os << "\n";
  }
  os << "  }\n";

  // Generate import file content for each configuration.
  for (std::string const& c : this->Configurations) {
    this->GenerateImportConfig(os, c, missingTargets);
  }

  this->GenerateMissingTargetsCheckCode(os, missingTargets);

  os << "}\n";

  return true;
}

void cmExportBuildJSONGenerator::GenerateImportTargetCode(
  std::ostream& os, const cmGeneratorTarget* target)
{
  std::string targetName = this->Namespace;
  targetName += target->GetExportName();
  os << "    \"" << targetName << "\": {\n";
  std::string path =
    cmSystemTools::ConvertToOutputPath(target->GetFullPath().c_str());
  os << "      \"output\": \"" << path << "\",\n";
}

void cmExportBuildJSONGenerator::GenerateImportPropertyCode(
  std::ostream& os, const std::string&, cmGeneratorTarget const*,
  ImportPropertyMap const& properties)
{
  for (auto const& property : properties) {
    if (property.first == "IMPORTED_SONAME") {
    } else if (property.first == "IMPORTED_LINK_INTERFACE_LANGUAGES") {
    } else if (property.first == "IMPORTED_LOCATION") {
    }
    os << "  " << property.first << " " << (property.second) << "\n";
  }
}

void cmExportBuildJSONGenerator::GenerateMissingTargetsCheckCode(
  std::ostream&, const std::vector<std::string>&)
{
}

void cmExportBuildJSONGenerator::GenerateInterfaceProperties(
  const cmGeneratorTarget* target, std::ostream& os,
  const ImportPropertyMap& properties)
{
  std::string config;
  if (!this->Configurations.empty()) {
    config = this->Configurations[0];
  }
  cmExportBuildJSONGenerator::GenerateInterfaceProperties(
    target, os, properties, cmExportBuildJSONGenerator::BUILD, config);
}

void cmExportBuildJSONGenerator::GenerateInterfaceProperties(
  const cmGeneratorTarget* target, std::ostream& os,
  const ImportPropertyMap& properties, GenerateType type,
  std::string const& config)
{
  const bool newCMP0022Behavior =
    target->GetPolicyStatusCMP0022() != cmPolicies::WARN &&
    target->GetPolicyStatusCMP0022() != cmPolicies::OLD;
  if (!newCMP0022Behavior) {
    std::ostringstream w;
    if (type == cmExportBuildJSONGenerator::BUILD) {
      w << "export(TARGETS ... JSON) called with policy CMP0022";
    } else {
      w << "install(JSON ...) called with policy CMP0022";
    }
    w << " set to OLD for target " << target->Target->GetName() << ". "
      << "The export will only work with CMP0022 set to NEW.";
    target->Makefile->IssueMessage(cmake::AUTHOR_WARNING, w.str());
  }
  if (!properties.empty()) {
    for (auto const& property : properties) {
      if (property.first == "INTERFACE_COMPILE_OPTIONS") {
        os << "LOCAL_CPP_FEATURES += ";
        os << (property.second) << "\n";
      } else if (property.first == "INTERFACE_LINK_LIBRARIES") {
        // need to look at list in pi->second and see if static or shared
        // FindTargetToLink
        // target->GetLocalGenerator()->FindGeneratorTargetToUse()
        // then add to LOCAL_CPPFLAGS
        std::vector<std::string> libraries;
        cmSystemTools::ExpandListArgument(property.second, libraries);
        std::vector<std::string> deps;
        std::vector<std::string> staticLibs;
        std::vector<std::string> sharedLibs;
        std::vector<std::string> ldlibs;
        for (std::string const& lib : libraries) {
          cmGeneratorTarget* gt =
            target->GetLocalGenerator()->FindGeneratorTargetToUse(lib);
          if (gt) {
            deps.push_back(lib);
            if (gt->GetType() == cmStateEnums::SHARED_LIBRARY ||
                gt->GetType() == cmStateEnums::MODULE_LIBRARY) {
              sharedLibs.push_back(lib);
            } else {
              staticLibs.push_back(lib);
            }
          } else {
            // evaluate any generator expressions with the current
            // build type of the makefile
            cmGeneratorExpression ge;
            std::unique_ptr<cmCompiledGeneratorExpression> cge = ge.Parse(lib);
            std::string evaluated =
              cge->Evaluate(target->GetLocalGenerator(), config);
            bool relpath = false;
            if (type == cmExportBuildJSONGenerator::INSTALL) {
              relpath = lib.substr(0, 3) == "../";
            }
            // check for full path or if it already has a -l, or
            // in the case of an install check for relative paths
            // if it is full or a link library then use string directly
            if (cmSystemTools::FileIsFullPath(evaluated) || evaluated.substr(0, 2) == "-l" || relpath) {
              ldlibs.push_back(evaluated);
              // if it is not a path and does not have a -l then add -l
            } else if (!evaluated.empty()) {
              ldlibs.push_back("-l" + evaluated);
            }
          }
        }
        auto join = [](std::string &ss, std::string &s) {
          return ss.empty() ? s : ss + "\", \"" + s;
        };
        if (!deps.empty()) {
          std::string str = std::accumulate(std::begin(deps), std::end(deps), std::string(), join);
          os << "      \"deps\": [\"" << str << "\"],\n";
        }
        //if (!sharedLibs.empty()) {
        //  std::string str = std::accumulate(std::begin(sharedLibs), std::end(sharedLibs), std::string(), join);
        //  os << "      \"shared_libraries\": [\"" << str << "\"],\n";
        //}
        //if (!staticLibs.empty()) {
        //  std::string str = std::accumulate(std::begin(staticLibs), std::end(staticLibs), std::string(), join);
        //  os << "      \"static_libraries\": [\"" << str << "\"],\n";
        //}
        if (!ldlibs.empty()) {
          std::string str = std::accumulate(std::begin(ldlibs), std::end(ldlibs), std::string(), join);
          os << "      \"libs\": [\"" << str << "\"],\n";
        }
      } else if (property.first == "INTERFACE_INCLUDE_DIRECTORIES") {
        std::string includes = property.second;
        std::vector<std::string> includeList;
        cmSystemTools::ExpandListArgument(includes, includeList);
        auto join = [](std::string &ss, std::string &s) {
          return ss.empty() ? s : ss + "\", \"" + s;
        };
        std::string str = std::accumulate(std::begin(includeList), std::end(includeList), std::string(), join);
        os << "        \"include_dirs\": [\"" << str << "\"],\n";
      } else {
        os << "# " << property.first << " " << (property.second) << "\n";
      }
    }
  }

  if (target->GetType() == cmStateEnums::STATIC_LIBRARY) {
    cmLinkImplementation const* li = target->GetLinkImplementation(config);
    if (std::find(li->Languages.begin(), li->Languages.end(), "CXX") !=
        li->Languages.end()) {
    }
  }

  switch (target->GetType()) {
    case cmStateEnums::SHARED_LIBRARY:
      os << "      \"target_type\": \"shared_library\"\n";
      break;
    case cmStateEnums::MODULE_LIBRARY:
      os << "      \"target_type\": \"module_library\"\n";
      break;
    case cmStateEnums::STATIC_LIBRARY:
      os << "      \"target_type\": \"static_library\"\n";
      break;
    case cmStateEnums::EXECUTABLE:
      os << "      \"target_type\": \"executable\"\n";
      break;
    case cmStateEnums::UTILITY:
    case cmStateEnums::OBJECT_LIBRARY:
    case cmStateEnums::GLOBAL_TARGET:
    case cmStateEnums::INTERFACE_LIBRARY:
    case cmStateEnums::UNKNOWN_LIBRARY:
      break;
  }
}
