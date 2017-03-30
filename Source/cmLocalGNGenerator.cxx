/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmLocalGNGenerator.h"

#include <vector>

#include "cmGeneratorTarget.h"
#include "cmGlobalGenerator.h"
#include "cmGlobalGNGenerator.h"
#include "cmGNTargetGenerator.h"
#include "cmMakefile.h"
#include "cmOutputConverter.h"
#include "cmState.h"

cmLocalGNGenerator::cmLocalGNGenerator(cmGlobalGenerator* gg,
                                       cmMakefile* mf)
  : cmLocalCommonGenerator(gg, mf, mf->GetState()->GetBinaryDirectory())
{
}

cmLocalGNGenerator::~cmLocalGNGenerator()
{
}

// Virtual public methods

void cmLocalGNGenerator::ComputeObjectFilenames(
  std::map<cmSourceFile const*, std::string>& mapping,
  cmGeneratorTarget const* gt)
{
  for (std::map<cmSourceFile const*, std::string>::iterator si =
         mapping.begin();
       si != mapping.end(); ++si) {
    cmSourceFile const* sf = si->first;
    si->second =
      this->GetObjectFileNameWithoutTarget(*sf, gt->ObjectDirectory);
  }
}

void cmLocalGNGenerator::Generate()
{
  // Compute the path to use when referencing the current output
  // directory from the top output directory.
  this->HomeRelativeOutputPath = this->ConvertToRelativePath(
    this->GetBinaryDirectory(), this->GetCurrentBinaryDirectory());
  if (this->HomeRelativeOutputPath == ".") {
    this->HomeRelativeOutputPath = "";
  }

  std::vector<cmGeneratorTarget*> targets = this->GetGeneratorTargets();
  for (std::vector<cmGeneratorTarget*>::iterator t = targets.begin();
       t != targets.end(); ++t) {
    if ((*t)->GetType() == cmStateEnums::INTERFACE_LIBRARY) {
      continue;
    }
    cmGNTargetGenerator tg(*t);
    tg.Generate();
  }
}

// Non-virtual public methods.

const cmGlobalGNGenerator* cmLocalGNGenerator::GetGlobalGNGenerator()
  const
{
  return static_cast<const cmGlobalGNGenerator*>(
    this->GetGlobalGenerator());
}

cmGlobalGNGenerator* cmLocalGNGenerator::GetGlobalGNGenerator()
{
  return static_cast<cmGlobalGNGenerator*>(this->GetGlobalGenerator());
}

void cmLocalGNGenerator::AppendTargetDepends(
  cmGeneratorTarget* target, std::vector<std::string>& outputs)
{
  this->GetGlobalGNGenerator()->AppendTargetDepends(target, outputs);
}
