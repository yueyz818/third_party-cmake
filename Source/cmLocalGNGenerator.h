/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmLocalGNGenerator_h
#define cmLocalGNGenerator_h

#include <cmConfigure.h>

#include <string>

#include "cmLocalCommonGenerator.h"

class cmGeneratorTarget;
class cmGlobalGenerator;
class cmGlobalGNGenerator;
class cmMakefile;

/** \class cmLocalCommonGenerator
 * \brief Common infrastructure for Makefile and Ninja local generators.
 */
class cmLocalGNGenerator : public cmLocalCommonGenerator
{
public:
  cmLocalGNGenerator(cmGlobalGenerator* gg, cmMakefile* mf);

  ~cmLocalGNGenerator() CM_OVERRIDE;

  void Generate() CM_OVERRIDE;

  const cmGlobalGNGenerator* GetGlobalGNGenerator() const;
  cmGlobalGNGenerator* GetGlobalGNGenerator();

  void AppendTargetDepends(cmGeneratorTarget* target,
                           std::vector<std::string>& outputs);

  void ComputeObjectFilenames(
    std::map<cmSourceFile const*, std::string>& mapping,
    cmGeneratorTarget const* gt = CM_NULLPTR) CM_OVERRIDE;

private:
  std::string HomeRelativeOutputPath;
};

#endif
