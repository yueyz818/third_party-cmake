/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmGlobalGNGenerator.h"

#include "cmDocumentationEntry.h"
#include "cmGeneratedFileStream.h"
#include "cmGeneratorTarget.h"
#include "cmLocalGenerator.h"
#include "cmLocalGNGenerator.h"
#include "cmMakefile.h"
#include "cmState.h"
#include "cmVersion.h"

class cmake;

const char* cmGlobalGNGenerator::GN_BUILD_FILE = "BUILD.gn";
const char* cmGlobalGNGenerator::INDENT = "  ";

#if 0
namespace {
class cmGNWriter
{
public:
  cmGNWriter(std::ostream& output, std::size_t level = 0);

private:
  std::ostream& Output;
  std::size_t Level;
};

cmGNWriter::cmGNWriter(std::ostream& output, std::size_t level = 0)
  : Output(output)
  , Level(level)
{
}

void cmGNWriter::StartScope(std::string const& name)
{
  Output << type + "(\"" + name + "\")";
  Output << "{";
}

};
#endif

void cmGlobalGNGenerator::Indent(std::ostream& os, int count)
{
  for (int i = 0; i < count; ++i) {
    os << cmGlobalGNGenerator::INDENT;
  }
}

void cmGlobalGNGenerator::WriteComment(std::ostream& os,
                                       const std::string& comment)
{
  if (comment.empty()) {
    return;
  }

  std::string::size_type lpos = 0;
  std::string::size_type rpos;
  while ((rpos = comment.find('\n', lpos)) != std::string::npos) {
    os << "# " << comment.substr(lpos, rpos - lpos) << "\n";
    lpos = rpos + 1;
  }
  os << "# " << comment.substr(lpos) << "\n";
}

void cmGlobalGNGenerator::WriteVariable(std::ostream& os,
                                        const std::string& name,
                                        const std::string& value,
                                        const std::string& comment,
                                        int indent)
{
  // Make sure we have a name.
  if (name.empty()) {
    cmSystemTools::Error("No name given for WriteVariable! called "
                         "with comment: ",
                         comment.c_str());
    return;
  }

  // Do not add a variable if the value is empty.
  std::string val = cmSystemTools::TrimWhitespace(value);
  if (val.empty()) {
    return;
  }

  cmGlobalGNGenerator::WriteComment(os, comment);
  cmGlobalGNGenerator::Indent(os, indent);
  os << name << " = " << val << "\n";
}

void cmGlobalGNGenerator::WriteVariable(std::ostream& os,
                                        const std::string& name,
                                        const std::vector<std::string>& values,
                                        const std::string& comment,
                                        int indent)
{
  // Make sure we have a name.
  if (name.empty()) {
    cmSystemTools::Error("No name given for WriteVariable! called "
                         "with comment: ",
                         comment.c_str());
    return;
  }

  // Do not add a variable if there are no values.
  if (values.empty()) {
    return;
  }

  cmGlobalGNGenerator::WriteComment(os, comment);
  cmGlobalGNGenerator::Indent(os, indent);
  os << name << " = [\n";
  for (std::vector<std::string>::const_iterator i = values.begin();
       i != values.end(); ++i) {
    std::string val = cmSystemTools::TrimWhitespace(*i);
    cmGlobalGNGenerator::Indent(os, indent + 1);
    os << "\"" << val << "\",\n";
  }
  cmGlobalGNGenerator::Indent(os, indent);
  os << "]\n";
}

void cmGlobalGNGenerator::WriteFunction(std::ostream& os,
                                        const std::string& name,
                                        const std::vector<std::string>& args,
                                        const std::string& comment,
                                        int indent)
{
  // Make sure we have a name.
  if (name.empty()) {
    cmSystemTools::Error("No name given for WriteFunction! called "
                         "with comment: ",
                         comment.c_str());
    return;
  }

  // Do not add a variable if there are no values.
  //if (values.empty()) {
  //  return;
  //}

  cmGlobalGNGenerator::WriteComment(os, comment);
  cmGlobalGNGenerator::Indent(os, indent);
  os << name << "(";
  for (std::vector<std::string>::const_iterator i = args.begin();
       i != args.end(); ++i) {
    std::string val = cmSystemTools::TrimWhitespace(*i);
    os << "\"" << val << "\"";
    if (i + 1 == args.end())
      os << ",";
  }
  os << ")\n";
}

void cmGlobalGNGenerator::WriteTarget(
  std::ostream& os, const std::string& comment, const std::string& type,
  const std::string& name, const std::string& output_name,
  const std::vector<std::string>& flags,
  const std::vector<std::string>& defines,
  const std::vector<std::string>& includes,
  const std::vector<std::string>& sources,
  const std::vector<std::string>& deps,
  const std::map<std::string, std::string>& variables,
  int cmdLineLimit)
{
  cmGlobalGNGenerator::WriteComment(os, comment);

  std::string id = type + "(\"" + name + "\")";

  std::ostringstream variable_assignments;
  cmGlobalGNGenerator::WriteVariable(variable_assignments, "cflags", flags, "", 1);
  cmGlobalGNGenerator::WriteVariable(variable_assignments, "defines", defines, "", 1);
  cmGlobalGNGenerator::WriteVariable(variable_assignments, "include_dirs", includes, "", 1);
  cmGlobalGNGenerator::WriteVariable(variable_assignments, "sources", sources, "", 1);
  cmGlobalGNGenerator::WriteVariable(variable_assignments, "deps", deps, "", 1);

  for (std::map<std::string, std::string>::const_iterator i = variables.begin();
       i != variables.end(); ++i) {
    cmGlobalGNGenerator::WriteVariable(variable_assignments, i->first,
                                       i->second, "", 1);
  }
  std::string assignments = variable_assignments.str();

  os << id << " {\n" << assignments << "}\n";
}

cmGlobalGNGenerator::cmGlobalGNGenerator(cmake* cm)
  : cmGlobalCommonGenerator(cm)
  , BuildFileStream(CM_NULLPTR)
{
  this->FindMakeProgramFile = "CMakeGNFindMake.cmake";
}

cmGlobalGNGenerator::~cmGlobalGNGenerator()
{
}

static void WriteDotGN(std::ostream& os)
{
  cmGlobalGNGenerator::WriteVariable(os, "buildconfig", "\"//build/BUILDCONFIG.gn\"");
}

static void WriteBuildConfig(std::ostream& os)
{
  os << "set_default_toolchain(\"//build:host\")";
}

static void WriteToolchainBuild(std::ostream& os)
{
  os << "toolchain(\"host\") {\n";
  cmGlobalGNGenerator::WriteVariable(os, "cc", "\"cc\"", "", 1);
  cmGlobalGNGenerator::WriteVariable(os, "cxx", "\"c++\"", "", 1);
  cmGlobalGNGenerator::WriteVariable(os, "ar", "\"ar\"", "", 1);
  cmGlobalGNGenerator::WriteVariable(os, "ld", "cxx", "", 1);
  os << "  tool(\"cc\") {\n";
  cmGlobalGNGenerator::WriteVariable(os, "depfile", "\"{{output}}.d\"", "", 2);
  cmGlobalGNGenerator::WriteVariable(os, "command", "\"$cc -MMD -MF $depfile {{defines}} {{include_dirs}} {{cflags}} {{cflags_c}} -c {{source}} -o {{output}}\"", "", 2);
  cmGlobalGNGenerator::WriteVariable(os, "depsformat", "\"gcc\"", "", 2);
  cmGlobalGNGenerator::WriteVariable(os, "description", "\"CC {{output}}\"", "", 2);
  cmGlobalGNGenerator::WriteVariable(os, "outputs", std::vector<std::string>{
      "{{source_out_dir}}/{{target_output_name}}.{{source_name_part}}.o",
    }, "", 2);
  os << "  }\n";
  os << "  tool(\"cxx\") {\n";
  cmGlobalGNGenerator::WriteVariable(os, "depfile", "\"{{output}}.d\"", "", 2);
  cmGlobalGNGenerator::WriteVariable(os, "command", "\"$cxx -MMD -MF $depfile {{defines}} {{include_dirs}} {{cflags}} {{cflags_c}} -c {{source}} -o {{output}}\"", "", 2);
  cmGlobalGNGenerator::WriteVariable(os, "depsformat", "\"gcc\"", "", 2);
  cmGlobalGNGenerator::WriteVariable(os, "description", "\"CXX {{output}}\"", "", 2);
  cmGlobalGNGenerator::WriteVariable(os, "outputs", std::vector<std::string>{
      "{{source_out_dir}}/{{target_output_name}}.{{source_name_part}}.o",
    }, "", 2);
  os << "  }\n";
  os << "  tool(\"alink\") {\n";
  cmGlobalGNGenerator::WriteVariable(os, "rspfile", "\"{{output}}.rsp\"", "", 2);
  cmGlobalGNGenerator::WriteVariable(os, "command", "\"rm -f {{output}} && $ar {{arflags}} rcsD {{output}} @\\\"$rspfile\\\"\"", "", 2);
  cmGlobalGNGenerator::WriteVariable(os, "description", "\"AR {{output}}\"", "", 2);
  cmGlobalGNGenerator::WriteVariable(os, "rspfile_content", "\"{{inputs}}\"", "", 2);
  cmGlobalGNGenerator::WriteVariable(os, "outputs", std::vector<std::string>{
      "{{output_dir}}/{{target_output_name}}{{output_extension}}",
    }, "", 2);
  cmGlobalGNGenerator::WriteVariable(os, "default_output_dir", "\"{{root_out_dir}}\"", "", 2);
  cmGlobalGNGenerator::WriteVariable(os, "default_output_extension", "\".a\"", "", 2);
  cmGlobalGNGenerator::WriteVariable(os, "output_prefix", "\"lib\"", "", 2);
  os << "  }\n";
  os << "  tool(\"solink\") {\n";
  cmGlobalGNGenerator::WriteVariable(os, "outname", "\"{{target_output_name}}{{output_extension}}\"", "", 2);
  cmGlobalGNGenerator::WriteVariable(os, "outfile", "\"{{output_dir}}/$outname\"", "", 2);
  cmGlobalGNGenerator::WriteVariable(os, "rspfile", "\"$outfile.rsp\"", "", 2);
  cmGlobalGNGenerator::WriteVariable(os, "command", "\"$ld -shared {{ldflags}} -o \\\"$outfile\\\" -Wl,-soname=\\\"$outname\\\" @\\\"$rspfile\\\"\"", "", 2);
  cmGlobalGNGenerator::WriteVariable(os, "description", "\"SOLINK $outfile\"", "", 2);
  cmGlobalGNGenerator::WriteVariable(os, "rspfile_content", "\"-Wl,--whole-archive {{inputs}} {{solibs}} -Wl,--no-whole-archive {{libs}}\"", "", 2);
  cmGlobalGNGenerator::WriteVariable(os, "outputs", std::vector<std::string>{
      "{{output_dir}}/{{target_output_name}}{{output_extension}}",
    }, "", 2);
  cmGlobalGNGenerator::WriteVariable(os, "default_output_dir", "\"{{root_out_dir}}\"", "", 2);
  cmGlobalGNGenerator::WriteVariable(os, "default_output_extension", "\".so\"", "", 2);
  cmGlobalGNGenerator::WriteVariable(os, "output_prefix", "\"lib\"", "", 2);
  os << "  }\n";
  os << "  tool(\"link\") {\n";
  cmGlobalGNGenerator::WriteVariable(os, "outfile", "\"{{output_dir}}/{{target_output_name}}{{output_extension}}\"", "", 2);
  cmGlobalGNGenerator::WriteVariable(os, "rspfile", "\"$outfile.rsp\"", "", 2);
  cmGlobalGNGenerator::WriteVariable(os, "command", "\"$ld {{ldflags}} -o $outfile -Wl,--start-group @\\\"$rspfile\\\" {{solibs}} -Wl,--end-group {{libs}}\"", "", 2);
  cmGlobalGNGenerator::WriteVariable(os, "description", "\"LINK $outfile\"", "", 2);
  cmGlobalGNGenerator::WriteVariable(os, "default_output_dir", "\"{{root_out_dir}}\"", "", 2);
  cmGlobalGNGenerator::WriteVariable(os, "rspfile_content", "\"{{inputs}}\"", "", 2);
  cmGlobalGNGenerator::WriteVariable(os, "outputs", std::vector<std::string>{
      "{{output_dir}}/{{target_output_name}}{{output_extension}}",
    }, "", 2);
  os << "  }\n";
  os << "}\n";
}

// Implemented by:
//   cmGlobalUnixMakefileGenerator3
//   cmGlobalGhsMultiGenerator
//   cmGlobalVisualStudio10Generator
//   cmGlobalVisualStudio7Generator
//   cmGlobalXCodeGenerator
// Called by:
//   cmGlobalGenerator::Build()
void cmGlobalGNGenerator::GenerateBuildCommand(
  std::vector<std::string>& makeCommand, const std::string& makeProgram,
  const std::string& /*projectName*/, const std::string& projectDir,
  const std::string& targetName, const std::string& /*config*/, bool /*fast*/,
  bool verbose, std::vector<std::string> const& makeOptions)
{
  makeCommand.push_back(this->SelectMakeProgram(makeProgram));

  //if (verbose) {
  //  makeCommand.push_back("-v");
  //}

  if (!projectDir.empty()) {
    cmGeneratedFileStream *dot = new cmGeneratedFileStream(
      (projectDir + "/.gn").c_str(), false, this->GetMakefileEncoding());
    WriteDotGN(*dot);
    delete dot;
    cmGeneratedFileStream *buildconfig = new cmGeneratedFileStream(
      (projectDir + "/build/BUILDCONFIG.gn").c_str(), false, this->GetMakefileEncoding());
    WriteBuildConfig(*buildconfig);
    delete buildconfig;
    cmGeneratedFileStream *toolchain = new cmGeneratedFileStream(
      (projectDir + "/build/BUILD.gn").c_str(), false, this->GetMakefileEncoding());
    WriteToolchainBuild(*toolchain);
    delete toolchain;
  }

  makeCommand.insert(makeCommand.end(), makeOptions.begin(),
                     makeOptions.end());
  if (!targetName.empty()) {
    if (targetName == "clean") {
      makeCommand.push_back("-t");
      makeCommand.push_back("clean");
    } else {
      makeCommand.push_back("gen");
      makeCommand.push_back(targetName);
    }
  }
}

// Non-virtual public methods

std::string cmGlobalGNGenerator::ConvertToGNPath(
  const std::string& path) const
{
  cmLocalGNGenerator* gn =
    static_cast<cmLocalGNGenerator*>(this->LocalGenerators[0]);
  return gn->ConvertToRelativePath(
    this->LocalGenerators[0]->GetState()->GetBinaryDirectory(), path);
  /*return this->NinjaOutputPath(convPath);*/
}

void cmGlobalGNGenerator::AppendTargetDepends(
  cmGeneratorTarget const* target, std::vector<std::string>& outputs)
{
  if (target->GetType() == cmStateEnums::GLOBAL_TARGET) {
    // These depend only on other CMake-provided targets, e.g. "all".
    std::set<std::string> const& utils = target->GetUtilities();
    for (std::set<std::string>::const_iterator i = utils.begin();
         i != utils.end(); ++i) {
      std::string d =
        target->GetLocalGenerator()->GetCurrentBinaryDirectory() +
        std::string("/") + *i;
      outputs.push_back(this->ConvertToGNPath(d));
    }
  } else {
    std::vector<std::string> outs;
    std::string config =
        target->Target->GetMakefile()->GetSafeDefinition("CMAKE_BUILD_TYPE");

    cmTargetDependSet const& targetDeps = this->GetTargetDirectDepends(target);
    for (cmTargetDependSet::const_iterator i = targetDeps.begin();
         i != targetDeps.end(); ++i) {
      if ((*i)->GetType() == cmStateEnums::INTERFACE_LIBRARY) {
        continue;
      }
      /*outs.push_back(this->ConvertToGNPath(
        (*i)->GetFullPath(config, false, false)));*/
      outs.push_back(this->ConvertToGNPath((*i)->GetName()));
    }
    std::sort(outs.begin(), outs.end());
    outputs.insert(outputs.end(), outs.begin(), outs.end());
  }
}

// Virtual public methods.

cmLocalGenerator* cmGlobalGNGenerator::CreateLocalGenerator(cmMakefile* mf)
{
  return new cmLocalGNGenerator(this, mf);
}

codecvt::Encoding cmGlobalGNGenerator::GetMakefileEncoding() const
{
  return codecvt::None;
}

void cmGlobalGNGenerator::GetDocumentation(cmDocumentationEntry& entry)
{
  entry.Name = cmGlobalGNGenerator::GetActualName();
  entry.Brief = "Generates BUILD.gn files.";
}

void cmGlobalGNGenerator::Generate()
{
  this->OpenBuildFileStream();

  this->cmGlobalGenerator::Generate();

  this->CloseBuildFileStream();
}

// Private methods

void cmGlobalGNGenerator::OpenBuildFileStream()
{
  // Compute GN's build file path.
  std::string buildFilePath =
    this->GetCMakeInstance()->GetHomeOutputDirectory();
  buildFilePath += "/";
  buildFilePath += cmGlobalGNGenerator::GN_BUILD_FILE;

  // Get a stream where to generate things.
  if (!this->BuildFileStream) {
    this->BuildFileStream = new cmGeneratedFileStream(
      buildFilePath.c_str(), false, this->GetMakefileEncoding());
    if (!this->BuildFileStream) {
      // An error message is generated by the constructor if it cannot
      // open the file.
      return;
    }
  }

  // Write the do not edit header.
  this->WriteDisclaimer(*this->BuildFileStream);
}

void cmGlobalGNGenerator::CloseBuildFileStream()
{
  if (this->BuildFileStream) {
    delete this->BuildFileStream;
    this->BuildFileStream = CM_NULLPTR;
  } else {
    cmSystemTools::Error("Build file stream was not open.");
  }
}

// Private virtual overrides

void cmGlobalGNGenerator::WriteDisclaimer(std::ostream& os)
{
  os << "# CMAKE generated file: DO NOT EDIT!\n"
     << "# Generated by \"" << this->GetName() << "\""
     << " Generator, CMake Version " << cmVersion::GetMajorVersion() << "."
     << cmVersion::GetMinorVersion() << "\n\n";
}

#if 0
void cmGlobalNinjaGenerator::AppendTargetOutputs(
  cmGeneratorTarget const* target, cmNinjaDeps& outputs)
{
  std::string configName =
    target->Target->GetMakefile()->GetSafeDefinition("CMAKE_BUILD_TYPE");

  // for frameworks, we want the real name, not smple name
  // frameworks always appear versioned, and the build.ninja
  // will always attempt to manage symbolic links instead
  // of letting cmOSXBundleGenerator do it.
  bool realname = target->IsFrameworkOnApple();

  switch (target->GetType()) {
    case cmStateEnums::EXECUTABLE:
    case cmStateEnums::SHARED_LIBRARY:
    case cmStateEnums::STATIC_LIBRARY:
    case cmStateEnums::MODULE_LIBRARY: {
      outputs.push_back(this->ConvertToNinjaPath(
        target->GetFullPath(configName, false, realname)));
      break;
    }
    case cmStateEnums::OBJECT_LIBRARY:
    case cmStateEnums::GLOBAL_TARGET:
    case cmStateEnums::UTILITY: {
      std::string path =
        target->GetLocalGenerator()->GetCurrentBinaryDirectory() +
        std::string("/") + target->GetName();
      outputs.push_back(this->ConvertToNinjaPath(path));
      break;
    }

    default:
      return;
  }
}
#endif
