/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmExportBuildJSONGenerator_h
#define cmExportBuildJSONGenerator_h

#include "cmConfigure.h" // IWYU pragma: keep

#include <iosfwd>
#include <string>
#include <vector>

#include "cmExportBuildFileGenerator.h"
#include "cmExportFileGenerator.h"

class cmGeneratorTarget;

/** \class cmExportBuildJSONGenerator
 * \brief Generate a file exporting targets from a build tree.
 *
 * cmExportBuildJSONGenerator generates a file exporting targets from
 * a build tree. This exports the targets to the JSON format that is
 * intended to be consumed by other tools.
 *
 * This is used to implement the EXPORT() command.
 */
class cmExportBuildJSONGenerator : public cmExportBuildFileGenerator
{
public:
  cmExportBuildJSONGenerator();
  // this is so cmExportInstallJSONGenerator can share this
  // function as they are almost the same
  enum GenerateType
  {
    BUILD,
    INSTALL
  };
  static void GenerateInterfaceProperties(cmGeneratorTarget const* target,
                                          std::ostream& os,
                                          const ImportPropertyMap& properties,
                                          GenerateType type,
                                          std::string const& config);

  void Compute(cmLocalGenerator* lg);

protected:
  // Implement virtual methods from the superclass.
  bool GenerateMainFile(std::ostream& os) override;
  void GeneratePolicyHeaderCode(std::ostream&) override {}
  void GeneratePolicyFooterCode(std::ostream&) override {}
  void GenerateImportHeaderCode(std::ostream&, const std::string&) override {};
  void GenerateImportFooterCode(std::ostream&) override {};
  void GenerateImportTargetCode(std::ostream& os,
                                const cmGeneratorTarget* target) override;
  void GenerateImportPropertyCode(
    std::ostream& os, const std::string& config,
    cmGeneratorTarget const* target,
    ImportPropertyMap const& properties) override;
  void GenerateImportedFileChecksCode(
    std::ostream&, cmGeneratorTarget*,
    ImportPropertyMap const&, const std::set<std::string>&) override {};
  void GenerateImportedFileCheckLoop(std::ostream&) override {};
  void GenerateMissingTargetsCheckCode(
    std::ostream& os, const std::vector<std::string>& missingTargets) override;
  void GenerateExpectedTargetsCode(
    std::ostream&, const std::string&) override {};

  void GenerateImportTargetsConfig(
    std::ostream&, const std::string&, std::string const&,
    std::vector<std::string>&) override {};
  void GenerateInterfaceProperties(
    cmGeneratorTarget const* target, std::ostream& os,
    const ImportPropertyMap& properties) override;
};

#endif
