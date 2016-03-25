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

/**
* Edit this file to set the versions and other related important data. After
* editing, build and run VersionTool with the project root directory as the
* first command line argument, and it will re-generate a C/C++ header include
* (for rc and source code use) as well as an installer include, all with
* matching data.
*/

#include <string>
#include <vector>
#include <array>
#include <cstddef>

/// This must be exactly four components. They're referred to as major, minor,
/// revision, and build, by convention
static const std::array<std::size_t, 4> VERSION_COMPONENTS = {3, 0, 1, 1};

/// This vector may be as long as desired within reason - it gets joined to
/// make the authors list that appears in various places.
static const std::vector<std::string> AUTHORS = {
    "Kai Liu", "Christopher Gurnee", "David B. Trout", "Tim Schlueter"};

/// This should be whomever's repo is listed below.
static const auto FORK_MAINTAINER = "Tim Schlueter";

static const auto REPO = "https://github.com/modelrockettier/HashCheck/";
static const auto RELEASES =
    "https://github.com/modelrockettier/HashCheck/releases";

/// Used in "manual" installation modes, as far as I can tell, to set an
/// uninstaller.
static const auto HELPURL = RELEASES; // could set to repo, or a homepage, or...
