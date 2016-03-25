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
* Edit this file to set the versions and other related important data.
*/

#include <string>
#include <vector>
#include <array>
#include <cstddef>

static const std::array<std::size_t, 4> VERSION_COMPONENTS = {3, 0, 1, 1};

static const std::vector<std::string> AUTHORS = {
    "Kai Liu", "Christopher Gurnee", "David B. Trout", "Tim Schlueter"};
static const auto FORK_MAINTAINER = "Tim Schlueter";

static const auto REPO = "https://github.com/modelrockettier/HashCheck/";
static const auto RELEASES =
    "https://github.com/modelrockettier/HashCheck/releases";
static const auto HELPURL = RELEASES; // could set to repo, or a homepage, or...
