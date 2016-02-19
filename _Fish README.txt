-------------------------------------------------------------------------------


                              -------
                                TODO
                              -------



  1.  Get rid of all that SimpleString and SimpleList crap and replace with
      much simpler templated CString/CAtlList classes instead.



-------------------------------------------------------------------------------


                           ---------------
                              HASHCHECK
                           ---------------

                            Version 3.0.0


INSTALLING:



   1.  Copy DLL to C:\Windows\System32.


   2.  Open ADMINISTRATOR Command Prompt window, and enter the command:


       C:\Windows\System32>   regsvr32  HashCheck.dll


   3.  DONE!   No need to restart the shell (Windows Explorer).  The
       extension should now be ready for immediate use.




UNINSTALLING:


   1.  Open ADMINISTRATOR Command Prompt window, and enter the command:


       C:\Windows\System32>   regsvr32  /u  HashCheck.dll


   2.  DONE!   No need to restart the shell (Windows Explorer).  The
       extension should now be uninstalled.  Note however, that it may
       take anywhere from 1 to 3 minutes for the DLL to eventually be
       be deleted from the %TEMP% directory (where the uninstall logic
       copies it to under a temporary name beginning with "HC").



   Or, alternatively, you can use the Control Panel "Programs", "Programs
   and Features" applet to uninstall "HashCheck Shell Extension" product
   instead (which simply does the "regsvr32 /u" for you automatically).



-------------------------------------------------------------------------------


                           ---------------
                               CHANGES
                           ---------------



  The following is a brief summary of the modifications made to Kai Liu's
  original HashCheck code:


      Fix tabbing and some indenting.


      Convert all C source modules to C++ and use pre-compiled headers.


      Replace use of hard-coded values with #define constants instead.


      Remove dead code and eliminate struct field duplication.


      Replace all __forceinline with just __inline.
      (It's unwise to try outsmarting the optimizer)


      Use CString where possible.
      Use Product/Version constants.
      Use memset instead of ZeroMemory.


      Compiler warning/errors override cleanup. Fix "unsafe" warnings.


      Add helper and debugging functions and macros.


      Remove undocumented o/s calls (e.g. external 'qsort_s_uptr' library).
      Calculate CRC32 manually and replace all other hashing with native
      Windows Crypto API calls instead. Add SHA-256 support.


      Simplify/fix installation/uninstallation logic.


      Use new COvSeqFile OVERLAPPED Sequential File I/O class for speed:

        300%+ increase in I/O throughput.
        70%+ reduced elapsed time.


-------------------------------------------------------------------------------
