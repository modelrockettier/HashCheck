HashCheck Shell Extension
Copyright (C) Kai Liu.  All rights reserved.


I.  Building from source


    1.  How the "official" binaries were built

        Environment: Visual Studio 2008 Professional SP1


    2.  Building 32-bit binaries with Microsoft Visual Studio

        a.  Visual Studio 6                     - UNSUPPORTED
        b.  Visual Studio 7-8 (2002/2003/2005)  - UNSUPPORTED
        c.  Visual Studio 9 (2008)              - SUPPORTED


    3.  Building 64-bit binaries with Microsoft Visual Studio

        a.  Visual Studio 6                     - UNSUPPORTED
        b.  Visual Studio 7-8 (2002/2003/2005)  - UNSUPPORTED
        c.  Visual Studio 9 (2008)              - SUPPORTED

            Notes:
            * The Express edition does not support 64-bit compilation.


    4.  Building under other environments

        I have not tested this source with other Windows compilers so I do
        not know whether it will compile outside of Microsoft Visual Studio.


    5.  Resources

        The resource script (HashCheck.rc) should be compiled with version 6 or
        newer of Microsoft's Resource Compiler.  All versions of Visual Studio
        2008 or newer should ship with Resource Compiler version 6 or newer.


    6.  Localizations

        Translation strings are stored as string table resources.  These tables
        can be modified by editing HashCheckTranslations.rc.


II. License and miscellanea

    Please refer to LICENSE.txt for details about distribution and modification.

    If you would like to report a bug, make a suggestion, or contribute a
    translation, please add your request or suggestion as a comment on the
    "HashCheck" article on the CodeProject.com web site:
    

      (Note: page doesn't exist yet)

      http://www.codeproject.com/KB/shell/HashCheck.aspx



    Kai Liu's ORIGINAL source code can be found on his web site at:


      http://code.kliu.org/hashcheck/

