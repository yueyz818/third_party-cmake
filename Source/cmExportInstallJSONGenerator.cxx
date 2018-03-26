/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmExportInstallJSONGenerator.h"

#include <ostream>
#include <stddef.h>

#include "cmExportBuildJSONGenerator.h"
#include "cmExportSet.h"
#include "cmGeneratorTarget.h"
#include "cmInstallExportGenerator.h"
#include "cmInstallTargetGenerator.h"
#include "cmStateTypes.h"
#include "cmSystemTools.h"
#include "cmTarget.h"
#include "cmTargetExport.h"

cmExportInstallJSONGenerator::cmExportInstallJSONGenerator(
  cmInstallExportGenerator* iegen)
  : cmExportInstallFileGenerator(iegen)
{
}

bool cmExportInstallJSONGenerator::GenerateMainFile(std::ostream& os)
{
  std::vector<cmTargetExport*> allTargets;
  {
    std::string expectedTargets;
    std::string sep;
    for (cmTargetExport* te :
         *this->IEGen->GetExportSet()->GetTargetExports()) {
      expectedTargets += sep + this->Namespace + te->Target->GetExportName();
      sep = " ";
      if (this->ExportedTargets.insert(te->Target).second) {
        allTargets.push_back(te);
      } else {
        std::ostringstream e;
        e << "install(EXPORT \"" << this->IEGen->GetExportSet()->GetName()
          << "\" ...) "
          << "includes target \"" << te->Target->GetName()
          << "\" more than once in the export set.";
        cmSystemTools::Error(e.str().c_str());
        return false;
      }
    }
  }

  std::vector<std::string> missingTargets;

  os << "{\n"
     << "  \"targets\": {\n";

  bool requiresConfigFiles = false;
  // Create all the imported targets.
  for (auto i = allTargets.begin(); i != allTargets.end(); ++i) {
    cmTargetExport* te = *i;
    cmGeneratorTarget* gt = te->Target;

    requiresConfigFiles =
      requiresConfigFiles || gt->GetType() != cmStateEnums::INTERFACE_LIBRARY;

    this->GenerateImportTargetCode(os, gt);

    ImportPropertyMap properties;

    this->PopulateIncludeDirectoriesInterface(
      te, cmGeneratorExpression::InstallInterface, properties, missingTargets);
    this->PopulateSourcesInterface(te, cmGeneratorExpression::InstallInterface,
                                   properties, missingTargets);
    this->PopulateInterfaceProperty("INTERFACE_SYSTEM_INCLUDE_DIRECTORIES", gt,
                                    cmGeneratorExpression::InstallInterface,
                                    properties, missingTargets);
    this->PopulateInterfaceProperty("INTERFACE_COMPILE_DEFINITIONS", gt,
                                    cmGeneratorExpression::InstallInterface,
                                    properties, missingTargets);
    this->PopulateInterfaceProperty("INTERFACE_COMPILE_OPTIONS", gt,
                                    cmGeneratorExpression::InstallInterface,
                                    properties, missingTargets);
    this->PopulateInterfaceProperty("INTERFACE_AUTOUIC_OPTIONS", gt,
                                    cmGeneratorExpression::InstallInterface,
                                    properties, missingTargets);
    this->PopulateInterfaceProperty("INTERFACE_COMPILE_FEATURES", gt,
                                    cmGeneratorExpression::InstallInterface,
                                    properties, missingTargets);

    const bool newCMP0022Behavior =
      gt->GetPolicyStatusCMP0022() != cmPolicies::WARN &&
      gt->GetPolicyStatusCMP0022() != cmPolicies::OLD;
    if (newCMP0022Behavior) {
      this->PopulateInterfaceLinkLibrariesProperty(
        gt, cmGeneratorExpression::InstallInterface, properties,
        missingTargets);
    }

    this->PopulateInterfaceProperty("INTERFACE_POSITION_INDEPENDENT_CODE", gt,
                                    properties);

    this->PopulateCompatibleInterfaceProperties(gt, properties);

    this->GenerateInterfaceProperties(gt, os, properties);

    os << "    }";
    if (i != allTargets.end() - 1)
      os << ",";
    os << "\n";
  }
  os << "  }\n";

  this->LoadConfigFiles(os);

  this->CleanupTemporaryVariables(os);
  this->GenerateImportedFileCheckLoop(os);

  bool result = true;
  // Generate an import file for each configuration.
  // Don't do this if we only export INTERFACE_LIBRARY targets.
  if (requiresConfigFiles) {
    for (std::string const& c : this->Configurations) {
      if (!this->GenerateImportFileConfig(c, missingTargets)) {
        result = false;
      }
    }
  }

  this->GenerateMissingTargetsCheckCode(os, missingTargets);

  os << "}\n";

  return result;
}

void cmExportInstallJSONGenerator::GenerateImportHeaderCode(
  std::ostream& os, const std::string&)
{
  std::string installDir = this->IEGen->GetDestination();
  for (cmTargetExport* te : *this->IEGen->GetExportSet()->GetTargetExports()) {
    // Collect import properties for this target.
    if (te->Target->GetType() == cmStateEnums::INTERFACE_LIBRARY) {
      continue;
    }
    std::string dest;
    if (te->LibraryGenerator) {
      dest = te->LibraryGenerator->GetDestination("");
    }
    if (te->ArchiveGenerator) {
      dest = te->ArchiveGenerator->GetDestination("");
    }
    te->Target->Target->SetProperty("__dest", dest.c_str());
  }
}

void cmExportInstallJSONGenerator::GenerateImportFooterCode(std::ostream&)
{
}

void cmExportInstallJSONGenerator::GenerateImportTargetCode(
  std::ostream& os, const cmGeneratorTarget* target)
{
  std::string targetName = this->Namespace;
  targetName += target->GetExportName();
  os << "    \"" << targetName << "\": {\n";
  os << "      \"output\": \"";
  os << target->Target->GetProperty("__dest") << "/";
  std::string config;
  if (!this->Configurations.empty()) {
    config = this->Configurations[0];
  }
  os << target->GetFullName(config) << "\",\n";
}

void cmExportInstallJSONGenerator::GenerateExpectedTargetsCode(
  std::ostream&, const std::string&)
{
}

void cmExportInstallJSONGenerator::GenerateImportPropertyCode(
  std::ostream&, const std::string&, cmGeneratorTarget const*,
  ImportPropertyMap const&)
{
}

void cmExportInstallJSONGenerator::GenerateMissingTargetsCheckCode(
  std::ostream&, const std::vector<std::string>&)
{
}

void cmExportInstallJSONGenerator::GenerateInterfaceProperties(
  cmGeneratorTarget const* target, std::ostream& os,
  const ImportPropertyMap& properties)
{
  std::string config;
  if (!this->Configurations.empty()) {
    config = this->Configurations[0];
  }
  cmExportBuildJSONGenerator::GenerateInterfaceProperties(
    target, os, properties, cmExportBuildJSONGenerator::INSTALL, config);
}

void cmExportInstallJSONGenerator::LoadConfigFiles(std::ostream&)
{
}

void cmExportInstallJSONGenerator::GenerateImportPrefix(std::ostream&)
{
}

void cmExportInstallJSONGenerator::GenerateRequiredCMakeVersion(
  std::ostream&, const char*)
{
}

void cmExportInstallJSONGenerator::CleanupTemporaryVariables(
  std::ostream&)
{
}

void cmExportInstallJSONGenerator::GenerateImportedFileCheckLoop(
  std::ostream&)
{
}

void cmExportInstallJSONGenerator::GenerateImportedFileChecksCode(
  std::ostream&, cmGeneratorTarget*, ImportPropertyMap const&,
  const std::set<std::string>&)
{
}

bool cmExportInstallJSONGenerator::GenerateImportFileConfig(
  const std::string&, std::vector<std::string>&)
{
  return true;
}
