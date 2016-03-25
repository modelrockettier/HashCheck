/**
* HashCheck Shell Extension - Version Tool
* Shell extension:
* Original work copyright (C) Kai Liu.  All rights reserved.
* Modified work copyright (C) 2014 Christopher Gurnee.  All rights reserved.
* Modified work copyright (C) 2016 Tim Schlueter.  All rights reserved.
* Tool:
* Original work copyright (C) 2016 Ryan Pavlik. All rights reserved.
*
* Please refer to readme.txt for information about this source code.
* Please refer to license.txt for details about distribution and modification.
**/

#include "VersionInput.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

/// Stream some container's elements to a string, joining them with the
/// separator, but don't put the separator at the end.
template <typename T>
inline std::string join(T &&container, std::string const &sep) {
  std::ostringstream os;
  for (auto &&elt : std::forward<T>(container)) {
    os << elt << sep;
  }
  // OK, we actually just remove the separator from the end after initially
  // putting it there. Don't tell anyone.
  auto ret = os.str();
  ret.resize(ret.size() - sep.size());
  return ret;
}

struct ComputedElements {
  std::string versionString;
  std::string authorsList;
};

void generateHeader(std::string const &projectRoot, ComputedElements const &c) {
  auto headerName = projectRoot + "\\version_generated.h";
  std::ofstream header(headerName);
  if (!header) {
    throw std::runtime_error("Could not open " + headerName +
                             " to write the generated header to.");
  }

  auto versionFull = join(VERSION_COMPONENTS, ",");
  std::cout << "Generating " << headerName << std::endl;
  header << "// GENERATED FILE - Edit VersionInput.h, and build and run the "
            "VersionTool to re-generate"
         << std::endl;
  header << "// Do not edit by hand!" << std::endl;
  header << "#define HASHCHECK_ABOUT_URL \"" << REPO << "\"" << std::endl;
  header << "#define HASHCHECK_HELP_URL \"" << HELPURL << "\"" << std::endl;
  header << "#define HASHCHECK_PUBLISHER \"" << FORK_MAINTAINER << "\""
         << std::endl;
  header << "#define HASHCHECK_COPYRIGHT_STR \"" << c.authorsList
         << ". All rights reserved.\"" << std::endl;
  header << "#define HASHCHECK_AUTHOR_STR \"" << c.authorsList << ".\""
         << std::endl;
  header << "#define HASHCHECK_VERSION_FULL " << versionFull << std::endl;
  header << "#define HASHCHECK_VERSION_STR \"" << c.versionString << "\""
         << std::endl;

  // PE version: MUST be in the form of major.minor
  header << "#ifdef _USRDLL" << std::endl;
  header << "#pragma comment(linker, \"/version:" << VERSION_COMPONENTS[0]
         << "." << VERSION_COMPONENTS[1] << "\")" << std::endl;
  header << "#endif" << std::endl;
  header.close();
}

void generateInstallerInclude(std::string const &projectRoot,
                              ComputedElements const &c) {
  auto fileName = projectRoot + "\\installer\\version_generated.nsh";
  std::ofstream include(fileName);
  if (!include) {
    throw std::runtime_error("Could not open " + fileName +
                             " to write the generated NSIS include file to.");
  }
  std::cout << "Generating " << fileName << std::endl;
  include << "; GENERATED FILE - Edit VersionInput.h, and build and run the "
             "VersionTool to re-generate."
          << std::endl;
  include << "; Do not edit by hand!" << std::endl;
  include << "!define APP_VER " << c.versionString << std::endl;
  include << "!define APP_RELEASES_URL " << RELEASES << std::endl;
  include << "!define APP_AUTHORS \"" << c.authorsList << "\"" << std::endl;
  include.close();
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cerr << "Must pass the project root as the first argument!"
              << std::endl;
    return -1;
  }
  std::string projectRoot = argv[1];
  // Trim any leading quote
  if (projectRoot.front() == '"') {
    projectRoot = projectRoot.substr(1);
  }
  // Trim any trailing quote
  if (projectRoot.back() == '"') {
    projectRoot.pop_back();
  }
  // remove trailing slash/backslash.
  if (projectRoot.back() == '\\' || projectRoot.back() == '/') {
    projectRoot.pop_back();
  }

  auto computed =
      ComputedElements{join(VERSION_COMPONENTS, "."), join(AUTHORS, ", ")};

  std::cout << "\nGenerating for version " << computed.versionString
            << std::endl;
  std::cout << "Project root: " << projectRoot << "\n" << std::endl;
  /// Even if one file fails, we'll try the other file.
  int retVal = 0;

  try {
    generateHeader(projectRoot, computed);
  } catch (std::exception const &e) {
    std::cerr << "Error generating header: " << e.what() << std::endl;
    retVal--;
  }

  try {
    generateInstallerInclude(projectRoot, computed);
  } catch (std::exception const &e) {
    std::cerr << "Error generating installer include: " << e.what()
              << std::endl;
    retVal--;
  }

  if (0 == retVal) {
    std::cout << "Completed successfully!" << std::endl;
  }

  return retVal;
}
