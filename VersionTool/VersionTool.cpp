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

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cerr << "Must pass the project root as the first argument!"
              << std::endl;
    return -1;
  }
  std::string projectRoot = argv[1];

  auto versionFull = join(VERSION_COMPONENTS, ",");
  auto versionString = join(VERSION_COMPONENTS, ".");
  auto authorsList = join(AUTHORS, ", ");
  auto headerName = projectRoot + "/version_generated.h";
  {
    std::ofstream header(headerName);
    if (!header) {
      std::cerr << "Could not open " << headerName
                << " to write the generated header to." << std::endl;
    }
    std::cout << "Generating " << headerName << " for version " << versionString
              << std::endl;
    header << "#define HASHCHECK_ABOUT_URL \"" << REPO << "\"" << std::endl;
    header << "#define HASHCHECK_HELP_URL \"" << HELPURL << "\"" << std::endl;
    header << "#define HASHCHECK_PUBLISHER \"" << FORK_MAINTAINER << "\""
           << std::endl;
    header << "#define HASHCHECK_COPYRIGHT_STR \"" << authorsList
        << ". All rights reserved.\"" << std::endl;
    header << "#define HASHCHECK_AUTHOR_STR \"" << authorsList << ".\""
           << std::endl;
    header << "#define HASHCHECK_VERSION_FULL " << versionFull << std::endl;
    header << "#define HASHCHECK_VERSION_STR \"" << versionString << "\""
           << std::endl;

    // PE version: MUST be in the form of major.minor
    header << "#ifdef _USRDLL" << std::endl;
    header << "#pragma comment(linker, \"/version:" << VERSION_COMPONENTS[0]
           << "." << VERSION_COMPONENTS[1] << "\")" << std::endl;
    header << "#endif" << std::endl;
    header.close();
  }
  return 0;
}
