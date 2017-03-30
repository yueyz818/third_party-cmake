/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmGlobalGNGenerator_h
#define cmGlobalGNGenerator_h

#include <cmConfigure.h>

#include "cmGlobalCommonGenerator.h"
#include "cmGlobalGenerator.h"
#include "cmGlobalGeneratorFactory.h"

#include <map>
#include <vector>

class cmGeneratedFileStream;
class cmake;

/** \class cmGlobalCommonGenerator
 * \brief Common infrastructure for Makefile and Ninja global generators.
 */
class cmGlobalGNGenerator : public cmGlobalCommonGenerator
{
public:
  /// The default name of Ninja's build file. Typically: build.ninja.
  static const char* GN_BUILD_FILE;

  /// The indentation string used when generating Ninja's build file.
  static const char* INDENT;

  /// Write @a count times INDENT level to output stream @a os.
  static void Indent(std::ostream& os, int count);

  /**
   * Utilized by the generator factory to determine if this generator
   * supports toolsets.
   */
  static bool SupportsToolset() { return false; }

  /**
   * Utilized by the generator factory to determine if this generator
   * supports platforms.
   */
  static bool SupportsPlatform() { return false; }

  /**
   * Write the given @a comment to the output stream @a os. It
   * handles new line character properly.
   */
  static void WriteComment(std::ostream& os, const std::string& comment);

  /**
   * Write a variable named @a name to @a os with value @a value and an
   * optional @a comment. An @a indent level can be specified.
   * @warning no escaping of any kind is done here.
   */
  static void WriteVariable(std::ostream& os, const std::string& name,
                            const std::string& value,
                            const std::string& comment = "", int indent = 0);

  /**
   * Write a variable named @a name to @a os with value @a value and an
   * optional @a comment. An @a indent level can be specified.
   * @warning no escaping of any kind is done here.
   */
  static void WriteVariable(std::ostream& os, const std::string& name,
                            const std::vector<std::string>& values,
                            const std::string& comment = "", int indent = 0);

  /**
   * Write a build statement to @a os with the @a comment using
   * the @a rule the list of @a outputs files and inputs.
   * It also writes the variables bound to this build statement.
   * @warning no escaping of any kind is done here.
   */
  void WriteTarget(std::ostream& os, const std::string& comment,
                   const std::string& type, const std::string& name,
                   const std::string& output_name,
                   const std::vector<std::string>& flags,
                   const std::vector<std::string>& defines,
                   const std::vector<std::string>& includes,
                   const std::vector<std::string>& sources,
                   const std::vector<std::string>& deps,
                   const std::map<std::string, std::string>& variables,
                   /*const std::vector<std::string>& outputs,
                   const std::vector<std::string>& cflags,
                   const std::vector<std::string>& implicitOuts,
                   const std::vector<std::string>& explicitDeps,
                   const std::vector<std::string>& implicitDeps,
                   const std::vector<std::string>& orderOnlyDeps,
                   const std::map<std::string, std::string>& variables,*/
                   int cmdLineLimit = 0);

  void WriteFunction(std::ostream& os,
                     const std::string& name,
                     const std::vector<std::string>& args,
                     const std::string& comment,
                     int indent);

public:
  cmGlobalGNGenerator(cmake* cm);

  static cmGlobalGeneratorFactory* NewFactory()
  {
    return new cmGlobalGeneratorSimpleFactory<cmGlobalGNGenerator>();
  }

  ~cmGlobalGNGenerator() CM_OVERRIDE;

  cmLocalGenerator* CreateLocalGenerator(cmMakefile* mf) CM_OVERRIDE;

  std::string GetName() const CM_OVERRIDE
  {
    return cmGlobalGNGenerator::GetActualName();
  }

  static std::string GetActualName() { return "GN"; }

  /** Get encoding used by generator for ninja files */
  codecvt::Encoding GetMakefileEncoding() const CM_OVERRIDE;

  static void GetDocumentation(cmDocumentationEntry& entry);

  void GenerateBuildCommand(std::vector<std::string>& makeCommand,
                            const std::string& makeProgram,
                            const std::string& projectName,
                            const std::string& projectDir,
                            const std::string& targetName,
                            const std::string& config, bool fast, bool verbose,
                            std::vector<std::string> const& makeOptions =
                              std::vector<std::string>()) CM_OVERRIDE;

  cmGeneratedFileStream* GetBuildFileStream() const
  {
    return this->BuildFileStream;
  }

  std::string ConvertToGNPath(const std::string& path) const;

  void AppendTargetDepends(
    cmGeneratorTarget const* target, std::vector<std::string>& outputs);

protected:
  void Generate() CM_OVERRIDE;

private:
  void OpenBuildFileStream();
  void CloseBuildFileStream();

  /// Write the common disclaimer text at the top of each build file.
  void WriteDisclaimer(std::ostream& os);

  cmGeneratedFileStream* BuildFileStream;
};

#endif
