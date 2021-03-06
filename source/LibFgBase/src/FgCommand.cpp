//
// Copyright (c) 2015 Singular Inversions Inc. (facegen.com)
// Use, modification and distribution is subject to the MIT License,
// see accompanying file LICENSE.txt or facegen.com/base_library_license.txt
//
// Authors: Andrew Beatty
//

#include "stdafx.h"

#include "FgStdString.hpp"
#include "FgStdVector.hpp"
#include "FgCommand.hpp"
#include "FgFileSystem.hpp"
#include "FgTime.hpp"
#include "FgSyntax.hpp"
#include "FgBuild.hpp"
#include "FgTestUtils.hpp"
#include "FgParse.hpp"

using namespace std;

static string       s_breadcrumb;
static string       s_annotateTestDir;
static bool         s_keepTempFiles = false;

void
fgMenu(
    vector<string>          args,
    const vector<FgCmd> &   cmdsUnsorted,
    bool                    optionAll,
    bool                    optionQuiet,
    bool                    optionKeep)
{
    s_breadcrumb += fgToLower(fgPathToBase(args[0]).m_str) + "_";
    string      cl,desc;
    if (optionQuiet) {
        cl +=   "[-s] ";
        desc += "    -s              - Suppress standard output except for errors.\n";
    }
    if (optionKeep) {
        cl +=   "[-k[<desc>]] ";
        desc += "    -k[<desc>]      - Keep test files in dated test directory [suffixed with <desc>].\n";
    }
    if (optionAll) {
        cl +=   "(<command> | all)\n";
        desc += "    -a              - Automated, no interactive feedback or regression updates\n";
    }
    else
        cl += "<command>\n";
    desc += "    <command>:";
    vector<FgCmd>       cmds = cmdsUnsorted;
    std::sort(cmds.begin(),cmds.end());
    for (size_t ii=0; ii<cmds.size(); ++ii) {
        string  si = "\n        " + cmds[ii].name;
        if (!cmds[ii].description.empty()) {
            for (size_t jj=si.length(); jj<28; ++jj)
                si += " ";
            si += "- ";
            si += cmds[ii].description;
        }
        desc += si;
    }
    FgSyntax    syntax(args,cl+desc);
    while (syntax.peekNext()[0] == '-') {
        string  opt = syntax.next();
        if ((opt == "-s") && optionQuiet)
            fgout.setCout(false);
        else if ((opt.substr(0,2) == "-k") && optionKeep) {
            s_keepTempFiles = true;
            if (opt.length() > 2)
                s_annotateTestDir = opt.substr(2);
        }
        else
            syntax.error("Invalid option");
    }
    string      cmd = syntax.next(),
                cmdl = fgToLower(cmd);
    if (optionAll) {
        fgout << fgnl << "Testing: " << fgpush;
        if (cmdl == "all") {
            for (size_t ii=0; ii<cmds.size(); ++ii) {
                fgout << fgnl << cmds[ii].name << ": " << fgpush;
                cmds[ii].func(fgSvec(cmds[ii].name,cmdl));      // Pass on the 'all'
                fgout << fgpop; }
            fgout << fgpop << fgnl << "All Passed.";
            return;
        }
    }
    for (size_t ii=0; ii<cmds.size(); ++ii) {
        if (cmdl == fgToLower(cmds[ii].name)) {
            cmds[ii].func(syntax.rest());
            if (optionAll)
                fgout << fgpop << fgnl << "Passed.";
            return;
        }
    }
    if (optionAll)
        fgout << fgpop;
    syntax.error("Invalid command",cmd);
}

FgTestDir::FgTestDir(const string & name)
{
    FGASSERT(!name.empty());
    path = FgPath(fgDataDir());
    path.dirs.back() = "_log";      // replace 'data' with 'log'
    path.dirs.push_back(s_breadcrumb+name);
    string          dt = fgDateTimePath();
    if (s_annotateTestDir.length() > 0)
        dt += " " + s_annotateTestDir;
    path.dirs.push_back(dt);
    fgCreatePath(path.dir());
    pd.push(path.str());
    if (s_keepTempFiles)
        fgout.logFile("_log.txt");
}

FgTestDir::~FgTestDir()
{
    if (!pd.orig.empty()) {
        pd.pop();
        if (!s_keepTempFiles) {
            // Recursively delete the directory for this test:
            fgRemoveAll(path.str());
            // Remove the test name directory if empty:
            path.dirs.resize(path.dirs.size()-1);
            fgRemoveDirectory(path.str());
        }
    }
}

void
fgTestCopy(const string & relPath)
{
    FgString        name = fgPathToName(relPath);
    if (fgExists(name))
        fgThrow("Test copy filename collision",name);
    FgString        source = fgDataDir() + relPath;
    fgCopyFile(source,name);
}

void
fgRunCmd(const FgCmdFunc & func,const string & argStr)
{
    fgout << fgnl << argStr << " " << fgpush;
    func(fgSplitChar(argStr));
    fgout << fgpop;
}

bool
fgKeepTempFiles()
{return s_keepTempFiles; }
