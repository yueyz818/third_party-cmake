/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmGNTargetGenerator_h
#define cmGNTargetGenerator_h

#include <cmConfigure.h>

#include "cmCommonTargetGenerator.h"
#include "cmGlobalGNGenerator.h"

class cmGeneratedFileStream;
class cmGeneratorTarget;
class cmLocalGNGenerator;
class cmMakefile;

class cmGNTargetGenerator : public cmCommonTargetGenerator
{
public:
  cmGNTargetGenerator(cmGeneratorTarget* target);

  ~cmGNTargetGenerator() CM_OVERRIDE;

  void Generate();

protected:
  cmGeneratedFileStream& GetBuildFileStream() const;

  cmGeneratorTarget* GetGeneratorTarget() const
  {
    return this->GeneratorTarget;
  }

  cmLocalGNGenerator* GetLocalGenerator() const
  {
    return this->LocalGenerator;
  }

  cmGlobalGNGenerator* GetGlobalGenerator() const;

  cmMakefile* GetMakefile() const { return this->Makefile; }

  std::string ConvertToNinjaPath(const std::string& path) const
  {
    return this->GetGlobalGenerator()->ConvertToGNPath(path);
  }

  void AddIncludeFlags(std::string& flags,
                       std::string const& lang) CM_OVERRIDE;

  std::string ComputeDefines(cmSourceFile const* source,
                             const std::string& language);

private:
  cmLocalGNGenerator* LocalGenerator;
  std::string TargetLinkLanguage;
};

#endif // ! cmGNTargetGenerator_h
