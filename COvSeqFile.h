///////////////////////////////////////////////////////////////////////////////
//  COvSeqFile.h  --  header file for COvSeqFile and related classes
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <atlstr.h>
#include <atlpath.h>
#include <atlcoll.h>

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// (constants)

#ifndef KB
#define KB    1024
#define MB   (KB*KB)
#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// (defaults)

#define OVSEQF_MAX_BUFFIO     (256*MB)     // Unbuffered I/O threshold
#define OVSEQF_DEF_BUFSZ      (256*KB)     // Default I/O buffer size
#define OVSEQF_DEF_BPS        (512)        // Default bytes-per-sector
#define OVSEQF_DEF_SPC        (8)          // Default sectors-per-cluster
#define OVSEQF_MIN_MAXIF      (4)          // Minimum MaxInFlight value

class COvSeqFile;                           // (forward reference)

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Specialized OVERLAPPED object        (helper class of COvSeqFile class)

class COV : protected OVERLAPPED        // NO VIRTUAL FUNCTIONS ALLOWED!
{
    friend class COvSeqFile;            // Controlling class
    friend class COVList;               // (so it can new/delete entries)

public: // Primary use functions        // NO VIRTUAL FUNCTIONS ALLOWED!

    bool   IsIOComplete()  const;       // Has I/O completed yet?
    int    WaitForIO();                 // Wait for complete, return IOBytes
    bool   WasGoodIO();                 // Did I/O complete okay?
    bool   FreeCOV();                   // Discard completed I/O

    COvSeqFile*  GetFile() const;       // Return associated COvSeqFile
    void*  Buffer()        const;       // Return I/O buffer address
    int    BufferSize()    const;       // Return size of I/O buffer
    int    IOBytes()       const;       // Return bytes read/written
    DWORD64 IOOffset()     const;       // Return I/O request offset
    DWORD  LastError()     const;       // Return last error code

    static int MaxInFlight( int nBufferSize );  // (convenience)

protected:

    // Internal helper functions        // NO VIRTUAL FUNCTIONS ALLOWED!

    COV( COvSeqFile* pOvSeqFile );      // Only friends can create
    ~COV();                             // Only friends can destroy

    bool  AllocBuffer();                // Allocate/reallocate I/O buffer
    void  FreeBuffer();                 // Discard allocated I/O buffer
    bool  EnsureNotBusy();              // Ensure overlapped I/O complete
    void  SetOffset( DWORD64 nOffset ); // Set I/O request offset

    // Protected class data

    COvSeqFile*  m_pOvSeqFile;          // Associated COvSeqFile
    void*        m_pBuffer;             // I/O buffer address
    int          m_nBufferSize;         // Size of I/O buffer
    DWORD        m_dwIOBytes;           // *lpNumberOfBytesTransferred
    DWORD        m_dwLastError;         // GetLastError()
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

typedef CAtlMap< COV*,  POSITION >  COVPtr2POSMap;  // (COVList helper class)
typedef CAtlMap< void*, POSITION >  BufAdr2POSMap;  // (COVList helper class)

///////////////////////////////////////////////////////////////////////////////
// List of COV* pointers                (helper class of COvSeqFile class)

class COVList : protected CAtlList < COV* >
{
    friend class COvSeqFile;            // Controlling class

protected:                              // Only friend can access

    COVList( COvSeqFile* pOvSeqFile );  // Only friend can create
    virtual ~COVList();                 // Only friend can destroy

    // Internal helper functions

    virtual POSITION  PushCOV( COV* pCOV );   // Add new entry to head of list
    virtual POSITION  AddCOV( COV* pCOV );    // Add new entry to tail of list
    virtual COV*      PopCOV();               // Remove newest entry from tail
    virtual COV*      RemoveCOV();            // Remove oldest entry from head
    virtual COV*      FindCOV( void* pBuffer);// Locate entry if it is in list
    virtual bool      FindCOV( COV* pCOV );   // Locate entry if it is in list
    virtual bool      RemoveCOV( COV* pCOV ); // Remove specific COV from list
    virtual void      FreeAllCOVs();          // Destructor helper

    // Protected class data

    COVPtr2POSMap      m_COV2POSMap;    // Use COV* to obtain list POSITION
    BufAdr2POSMap      m_Buf2POSMap;    // Same idea for COV buffer address
    COvSeqFile*        m_pOvSeqFile;    // Ptr to associated COvSeqFile
    mutable CRITICAL_SECTION  m_lock;   // Lock for multi-thread access

    void Lock()   { EnterCriticalSection( &m_lock ); }
    void Unlock() { LeaveCriticalSection( &m_lock ); }
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Class for performing overlapped sequential file I/O

class COvSeqFile
{
private:                                    // ** No copying allowed! **

    COvSeqFile( const COvSeqFile& );        //  (PURPOSELY undefined)
    void operator = ( const COvSeqFile& );  //  (PURPOSELY undefined)

    friend class COV;                       //  (helper class)
    friend class COVList;                   //  (helper class)

public:

    // Default constructor

    COvSeqFile( int nBufferSize = OVSEQF_DEF_BUFSZ, bool bInput = true,
                int nMaxInFlight = 0, DWORD64 nBuffIOMax = OVSEQF_MAX_BUFFIO );

    // Alternate constructor to create an associated opposite of another
    // COvSeqFile object to allow COVs from one to be used in the other.
    // See discussion of Read/Write further below for more information.

    COvSeqFile( COvSeqFile* pOvSeqFile );   // (create associated opposite)
    virtual ~COvSeqFile();                  // (destructor)

    // Open/close functions. Note that an Open() call might fail if there
    // isn't enough storage to allocate all needed buffers as determined
    // by the maximum number of in-flight I/Os times the specified buffer
    // size. Also note that a Close() call might also fail if a previously
    // started (but not yet completed) I/O request encounters an I/O error
    // since the Close() function always waits for all outstanding I/Os to
    // complete first before actually closing the file.

    virtual bool  Open( LPCTSTR pszFileName );  // open specified file
    virtual bool  Close();                      // close an opened file

    // A call to the Read function sets the COV pointer passed by reference
    // to point to the COV for the just completed I/O and returns the number
    // of bytes that were read. (It's not possible to read without waiting
    // for the I/O to complete. All reading ahead is handled internally.)
    // Use the COV class functions to access the I/O buffer address or other
    // information pertaining to the just completed I/O.

    virtual int   Read( COV*& pCOV );   // read file data

    // Use the GetCOV() function further below to obtain a COV that you can
    // use directly for Writes or, if the COvSeqFile object was created as
    // an associated opposite file, you can also use a COV that was returned
    // by the Read() function to directly write the data you just read from
    // the input file without needing to do any any type of buffer copying
    // at all.

    virtual int   Write( COV* pCOV, int nIOBytes = -1 );
    virtual bool  Flush();              // flush output buffers to disk

    // Each COV given to you by either the Read or GetCOV functions MUST
    // eventually be returned back to the system to prevent a memory leak.
    // The FreeCOV() function returns an I/O buffer back to the system.

    virtual COV* GetCOV();              // Obtain an I/O buffer
    virtual bool FreeCOV( COV* pCOV );  // Discard completed I/O

    // Secondary property functions

    virtual DWORD   LastError() const;  // returns last error code
    virtual CString FileName()  const;  // returns filename as CString
    virtual DWORD64 FileSize()  const;  // returns size of file in bytes
    virtual DWORD64 Offset()    const;  // returns logical file offset
    virtual DWORD64 IOOffset();         // returns latest I/O request offset
    virtual DWORD64 BuffIOMax() const;  // returns buffered I/O threshold

    virtual operator LPCTSTR()  const;  // returns pointer to filename
    virtual operator HANDLE()   const;  // returns file HANDLE

    virtual bool IsOpen()       const;  // returns true if file is opened
    virtual bool IsClosed()     const;  // returns true if file is closed
    virtual bool IsEndOfFile()  const;  // returns true if end-of-file
    virtual bool IsInput()      const;  // returns true if input file
    virtual bool IsOutput()     const;  // returns true if output file
    virtual bool IsBuffered()   const;  // returns true if buffered I/O
    virtual bool IsUnBuffered() const;  // returns true if unbuffered I/O

    virtual int  BPS()          const;  // returns bytes per sector
    virtual int  SPC()          const;  // returns sectors per cluster
    virtual int  BufferSize()   const;  // returns the possibly adjusted I/O buffer size
    virtual int  MaxInFlight()  const;  // returns maximum number of simultaneous I/Os allowed
    virtual int  InFlight()     const;  // returns number of currently active simultaneous I/Os
    virtual int  InFlightHWM()  const;  // returns most ever simultaneously active I/Os
    virtual int  StolenCOVs()   const;  // returns count of stolen user COVs

protected:

    // Protected class data

    bool      m_bInput;                 // (duh!)
    bool      m_bEndOfFile;             // ERROR_HANDLE_EOF
    bool      m_bAutoMaxInFlight;       // automatic MaxInFlight handling

    int       m_nBPS;                   // bytes per sector
    int       m_nSPC;                   // sectors per cluster
    int       m_nBufferSize;            // I/O buffer size (adjusted)
    int       m_nMaxInFlight;           // defined maximum outstanding I/Os
    int       m_nInFlightHWM;           // outstanding I/Os high watermark
    int       m_nStolenCOVs;            // count of stolen user COVs

    DWORD64   m_nBuffIOMax;             // FILE_FLAG_NO_BUFFERING threshold
    DWORD64   m_nFileSize;              // size of file in bytes
    CString   m_strFileName;            // file name
    HANDLE    m_hFile;                  // file handle
    DWORD     m_dwLastError;            // GetLastError()
    DWORD64   m_nNextOffset;            // current I/O offset
    DWORD64   m_nFileOffset;            // current file offset

    // Internal helper functions

    virtual void  AddBusyCOV( COV* pCOV );    // Append another COV to busy list
    virtual COV*  RemoveBusyCOV();            // Get oldest from busy list or NULL if empty

    virtual void  AddUserCOV( COV* pCOV );    // Append done COV to user list
    virtual bool  RemoveUserCOV( COV* pCOV ); // Remove COV from user list
    virtual bool  FreeUserCOV( COV* pCOV );   // Move user COV to free list
    virtual COV*  StealUserCOV();             // Repossess oldest user COV

    virtual void  PushFreeCOV( COV* pCOV );   // Append recently used COV to free list
    virtual COV*  PopFreeCOV();               // Pop most recently pushed COV from free list

    virtual bool  StartNextRead( COV* pCOV ); // Get the next read started
    virtual void  TrackInFlight();            // Track most ever active I/Os

    virtual void  Init( int nBufferSize, bool bInput, int nMaxInFlight,
                        DWORD64 nBuffIOMax, COvSeqFile* pOvSeqFile );
    virtual void  Free();

    // The busy list contains COVs for currently active I/Os, whereas the
    // free list contains COVs that aren't currently being used. The user
    // list contains COVs we have given to the user in response to either
    // a call to the Read function or a call to the GetCOV function. When
    // they eventually call FreeCOV we then move it off the user list and
    // onto the free list.

    long      m_lRefCount;      // shared reference count
    COVList   m_BusyList;       // FIFO list of busy COVs
    COVList*  m_pUserList;      // FIFO list of user COVs  (possibly shared)
    COVList*  m_pFreeList;      // LIFO list of free COVs  (possibly shared)

    COvSeqFile*  m_pAssocFile;  // associated file
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

