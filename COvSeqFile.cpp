///////////////////////////////////////////////////////////////////////////////
//  COvSeqFile.cpp  --  implementation file for COvSeqFile and related classes
///////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "COvSeqFile.h"

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int COV::MaxInFlight( int nBufferSize )
{
    ASSERT( nBufferSize );

    MEMORYSTATUSEX msex = { sizeof( MEMORYSTATUSEX ), 0 };
    VERIFY( GlobalMemoryStatusEx( &msex ));

    DWORDLONG mem = msex.ullAvailPageFile;

    if (msex.ullAvailVirtual < mem)
        mem = msex.ullAvailVirtual;

    if (msex.ullAvailPhys < mem)
        mem = msex.ullAvailPhys;

    double dGreed = (((100.0 - (double) msex.dwMemoryLoad) * 0.50) / 100.0);

    mem = (DWORDLONG) ((double) mem * dGreed);

    DWORDLONG  nMaxInFlight  = mem / ((DWORDLONG) nBufferSize + sizeof( COV ));

    if (nMaxInFlight >= INT_MAX)
        nMaxInFlight  = INT_MAX;

    if (nMaxInFlight < OVSEQF_MIN_MAXIF)
        nMaxInFlight = OVSEQF_MIN_MAXIF;

    TRACE_ALWAYS(_T("*** COV::MaxInFlight( %d ) = %d\n"),
        nBufferSize, (int) nMaxInFlight );

    return (int) nMaxInFlight;
}

///////////////////////////////////////////////////////////////////////////////
// Constructor

COV::COV( COvSeqFile* pOvSeqFile )
{
    ASSERT( pOvSeqFile );
    memset( this, 0, sizeof( *this ));
    m_pOvSeqFile = pOvSeqFile;
    hEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
    ASSERT( hEvent );
    VERIFY( AllocBuffer());
}

///////////////////////////////////////////////////////////////////////////////
// Constructor helper

bool COV::AllocBuffer()
{
    int nBufferSize = GetFile()->BufferSize();
    if (nBufferSize > m_nBufferSize || !m_nBufferSize)
    {
        FreeBuffer();
        void* pBuffer = VirtualAlloc( NULL, nBufferSize, MEM_COMMIT, PAGE_READWRITE );
        if ( !pBuffer )
            m_dwLastError = GetLastError();
        else
        {
            m_pBuffer     = pBuffer;
            m_nBufferSize = nBufferSize;
        }
    }
    return  m_pBuffer ? true : false;
}

///////////////////////////////////////////////////////////////////////////////
// Destructor

COV::~COV()
{
    FreeBuffer();
    if (hEvent)
        VERIFY( CloseHandle( hEvent ));
    memset( this, 0, sizeof( *this ));
}

///////////////////////////////////////////////////////////////////////////////
// Destructor helper

void COV::FreeBuffer()
{
    if (m_pBuffer)
    {
        EnsureNotBusy();
        void* pBuffer = m_pBuffer;
        m_pBuffer = NULL;
        m_nBufferSize = 0;
        if (!VirtualFree( pBuffer, 0, MEM_RELEASE ))
        {
            // (don't modify m_dwLastError)
            DWORD dwLastError = GetLastError();
            ASSERT( false );
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
// Internal helper  --  Must never modify of free an active I/O's OVERLAPPED!

bool COV::EnsureNotBusy()
{
    // PROGRAMMING NOTE: attempting to wait (by calling GetOverlappedResult)
    // on for an I/O that has already completed is a *VERY* SERIOUS ERROR!
    // You must ONLY call GetOverlappedResult() whenever your original I/O
    // request "fails" with a ERROR_IO_PENDING return code. If the original
    // I/O request succeeds immediately (does not fail) or fails with any
    // other error code besides ERROR_IO_PENDING then you must NOT call the
    // GetOverlappedResult function! If you mistakenly do, you will end up
    // waiting forever (hanging) for an I/O to complete that isn't pending.
    // http://blogs.msdn.com/b/oldnewthing/archive/2011/03/03/10136241.aspx

    if (ERROR_IO_PENDING == m_dwLastError)
    {
        if (GetOverlappedResult( GetFile()->m_hFile, this, &m_dwIOBytes, TRUE ))
        {
            m_dwLastError = IOBytes() ? ERROR_SUCCESS : ERROR_HANDLE_EOF;
            return true;
        }

        m_dwLastError = GetLastError();
    }

    if (ERROR_SUCCESS == m_dwLastError)
    {
        if (!IOBytes())
            m_dwLastError = ERROR_HANDLE_EOF;
        return true;
    }

    if (ERROR_HANDLE_EOF == m_dwLastError)
    {
        m_dwIOBytes = 0;
        return true;
    }

    return false;
}

///////////////////////////////////////////////////////////////////////////////
// Wait for I/O completion

int COV::WaitForIO()
{
    // PROGRAMMING NOTE: with unbuffered direct I/O (FILE_FLAG_NO_BUFFERING)
    // it is possible to read BEYOND the end of the file since the very last
    // cluster might not be completely used. Thus for unbuffered direct I/O
    // (and ONLY for unbuffered direct I/O) we need to always check to make
    // sure we don't return more data than what logically exists in the file.

    // Note too that we must ALWAYS do this REGARDLESS of whether or not the
    // EnsureNotBusy (GetOverlappedResult) call returns success or failure!

    bool bSuccess = EnsureNotBusy();

    if (GetFile()->IsUnBuffered())
    {
        if (IOOffset() >= GetFile()->FileSize())
        {
            m_dwIOBytes = 0;

            if (bSuccess)
                m_dwLastError = ERROR_HANDLE_EOF;
        }
        else if ((IOOffset() + m_dwIOBytes) > GetFile()->FileSize())
            m_dwIOBytes = (DWORD) (GetFile()->FileSize() - IOOffset());
    }

    return bSuccess ? IOBytes() : -1;
}

///////////////////////////////////////////////////////////////////////////////
// Discard completed I/O

bool COV::FreeCOV()
{
    EnsureNotBusy();
    return GetFile()->FreeUserCOV( this );
}

///////////////////////////////////////////////////////////////////////////////
// Has I/O completed yet?

bool COV::IsIOComplete() const
{
    return HasOverlappedIoCompleted( this );
}

///////////////////////////////////////////////////////////////////////////////
// Did I/O complete okay?

bool COV::WasGoodIO()
{
    return WaitForIO() >= 0;
}

///////////////////////////////////////////////////////////////////////////////
// Retrieve associated COvSeqFile

COvSeqFile* COV::GetFile() const
{
    return m_pOvSeqFile;
}

///////////////////////////////////////////////////////////////////////////////
// Retrieve I/O buffer address

void*  COV::Buffer() const
{
    return m_pBuffer;
}

///////////////////////////////////////////////////////////////////////////////
// Retrieve size of I/O buffer

int COV::BufferSize() const
{
    return m_nBufferSize;
}

///////////////////////////////////////////////////////////////////////////////
// Retrieve bytes read/written

int COV::IOBytes() const
{
    return (int) m_dwIOBytes;
}

///////////////////////////////////////////////////////////////////////////////
// Set I/O request offset

void COV::SetOffset( DWORD64 nOffset )
{
    OffsetHigh = (nOffset >> 32) & 0xFFFFFFFF;
    Offset     = (nOffset >>  0) & 0xFFFFFFFF;
}

///////////////////////////////////////////////////////////////////////////////
// Return I/O request offset

DWORD64 COV::IOOffset() const
{
    DWORD64 nOffset = (((DWORD64) OffsetHigh) << 32)
                    | (((DWORD64) Offset)     <<  0);
    return  nOffset;
}

///////////////////////////////////////////////////////////////////////////////
// Retrieve completion code

DWORD COV::LastError() const
{
    return m_dwLastError;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Constructor

COVList::COVList( COvSeqFile* pOvSeqFile )
{
    ASSERT( pOvSeqFile );
    InitializeCriticalSection( &m_lock );
    m_pOvSeqFile = pOvSeqFile;
}

///////////////////////////////////////////////////////////////////////////////
// Destructor

COVList::~COVList()
{
    Lock();
    {
        m_pOvSeqFile = NULL;
        FreeAllCOVs();
    }
    Unlock();
    DeleteCriticalSection( &m_lock );
}

///////////////////////////////////////////////////////////////////////////////
// Destructor helper

void COVList::FreeAllCOVs()
{
    Lock();
    {
        while (!IsEmpty())
            delete RemoveHead();
        m_COV2POSMap.RemoveAll();
        m_Buf2POSMap.RemoveAll();
    }
    Unlock();
}

///////////////////////////////////////////////////////////////////////////////
// Add new entry to head of list

POSITION COVList::PushCOV( COV* pCOV )
{
    ASSERT( pCOV );
    POSITION pos;
    Lock();
    {
        if (m_COV2POSMap.Lookup( pCOV, pos))
            RemoveAt( pos );
        m_COV2POSMap[ pCOV ] = m_Buf2POSMap[ pCOV->Buffer() ] = pos = AddHead( pCOV );
    }
    Unlock();
    return pos;
}

///////////////////////////////////////////////////////////////////////////////
// Add new entry to tail of list

POSITION COVList::AddCOV( COV* pCOV )
{
    ASSERT( pCOV );
    POSITION pos;
    Lock();
    {
        if (m_COV2POSMap.Lookup( pCOV, pos))
            RemoveAt( pos );
        m_COV2POSMap[ pCOV ] = m_Buf2POSMap[ pCOV->Buffer() ] = pos = AddTail( pCOV );
    }
    Unlock();
    return pos;
}

///////////////////////////////////////////////////////////////////////////////
// Remove newest entry from tail

COV* COVList::PopCOV()
{
    COV* pCOV = NULL;
    Lock();
    {
        if (!IsEmpty())
        {
            pCOV = RemoveTail();
            VERIFY( m_COV2POSMap.RemoveKey( pCOV ));
            VERIFY( m_Buf2POSMap.RemoveKey( pCOV->Buffer()));
        }
    }
    Unlock();
    return pCOV;
}

///////////////////////////////////////////////////////////////////////////////
// Remove oldest entry from head

COV* COVList::RemoveCOV()
{
    COV* pCOV = NULL;
    Lock();
    {
        if (!IsEmpty())
        {
            pCOV = RemoveHead();
            VERIFY( m_COV2POSMap.RemoveKey( pCOV ));
            VERIFY( m_Buf2POSMap.RemoveKey( pCOV->Buffer()));
        }
    }
    Unlock();
    return pCOV;
}

///////////////////////////////////////////////////////////////////////////////
// Remove specific COV from list

bool COVList::RemoveCOV( COV* pCOV )
{
    bool bFound;
    Lock();
    {
        POSITION pos;
        bFound = m_COV2POSMap.Lookup( pCOV, pos );
        if (bFound)
        {
            RemoveAt( pos );
            VERIFY( m_COV2POSMap.RemoveKey( pCOV ));
            VERIFY( m_Buf2POSMap.RemoveKey( pCOV->Buffer()));
        }
    }
    Unlock();
    return bFound;
}

///////////////////////////////////////////////////////////////////////////////
// Locate entry if it is in list

COV* COVList::FindCOV( void* pBuffer )
{
    ASSERT( pBuffer );
    COV* pCOV = NULL;
    Lock();
    {
        POSITION pos;
        if (m_Buf2POSMap.Lookup( pBuffer, pos ))
            pCOV = GetAt( pos );
    }
    Unlock();
    return pCOV;
}

///////////////////////////////////////////////////////////////////////////////
// Locate entry if it is in list

bool COVList::FindCOV( COV* pCOV )
{
    ASSERT( pCOV );
    bool bFound;
    Lock();
    {
        POSITION pos;
        bFound = m_COV2POSMap.Lookup( pCOV, pos );
    }
    Unlock();
    return bFound;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Primary preferred constructor

COvSeqFile::COvSeqFile( int nBufferSize, bool bInput, int nMaxInFlight,
                         DWORD64 nBuffIOMax )

    : m_BusyList( this )    // (set list identify)
    , m_lRefCount( 1 )
    , m_pAssocFile( NULL )
{
    Init( nBufferSize, bInput, nMaxInFlight, nBuffIOMax, this );
}

///////////////////////////////////////////////////////////////////////////////
// Alternate constructor to create an associated opposite of another
// COvSeqFile object, both of which share the same user/free lists.

COvSeqFile::COvSeqFile( COvSeqFile* pOvSeqFile )

    : m_BusyList( this )                    // (set list identify)
    , m_lRefCount( 1 )
    , m_pAssocFile( NULL )
{
    ASSERT( pOvSeqFile );
    InterlockedIncrement( &(m_pAssocFile = pOvSeqFile)->m_lRefCount );

    Init( pOvSeqFile->BufferSize(),         // (same buffer size)
         !pOvSeqFile->IsInput(),            // (opposite of other)
          pOvSeqFile->MaxInFlight(),        // (same max in-flight)
          pOvSeqFile->BuffIOMax(),          // (same buffered i/o threshold)
          pOvSeqFile );                     // (init association)
}

///////////////////////////////////////////////////////////////////////////////
// (common constructor helper)

void  COvSeqFile::Init( int nBufferSize, bool bInput, int nMaxInFlight,
                        DWORD64 nBuffIOMax, COvSeqFile* pOvSeqFile )
{
    if ((m_nBufferSize = nBufferSize) <= 0)
         m_nBufferSize = OVSEQF_DEF_BUFSZ;

    m_nBPS = OVSEQF_DEF_BPS;
    m_nSPC = OVSEQF_DEF_SPC;

    m_bInput       = bInput;
    m_nBuffIOMax   = nBuffIOMax;
    m_nMaxInFlight = nMaxInFlight;

    m_bAutoMaxInFlight = (nMaxInFlight <= 0);

    m_nInFlightHWM = 0;
    m_nStolenCOVs  = 0;
    m_nFileSize    = 0;
    m_hFile        = NULL;
    m_dwLastError  = 0;
    m_nNextOffset  = 0;
    m_nFileOffset  = 0;
    m_bEndOfFile   = false;

    if (pOvSeqFile != this)                     // (associated create?)
    {
        m_pUserList = pOvSeqFile->m_pUserList;  // (use associate's list)
        m_pFreeList = pOvSeqFile->m_pFreeList;  // (use associate's list)
    }
    else                                        // (otherwise...)
    {
        m_pUserList = new COVList( this );      // (create our own list)
        m_pFreeList = new COVList( this );      // (create our own list)
    }
}

///////////////////////////////////////////////////////////////////////////////
// Destructor

COvSeqFile::~COvSeqFile()
{
    Close();
    Free();
}

///////////////////////////////////////////////////////////////////////////////
// Destructor helper: free all resources

void COvSeqFile::Free()
{
    if (m_hFile)
        VERIFY( CancelIoEx( m_hFile, NULL )
           ||   ERROR_NOT_FOUND == (m_dwLastError = GetLastError()));

    m_BusyList.FreeAllCOVs();
    ASSERT( m_BusyList.IsEmpty() );

    if (m_hFile)
    {
        VERIFY( CloseHandle( m_hFile ));
        m_hFile = NULL;
    }

    if (InterlockedDecrement( &m_lRefCount ) <= 0)
    {
        // Note: COVList destructor does FreeAllCOVs()
        delete m_pUserList; m_pUserList = NULL;
        delete m_pFreeList; m_pFreeList = NULL;
    }
}

///////////////////////////////////////////////////////////////////////////////
// Track most ever active I/Os

void COvSeqFile::TrackInFlight()
{
    int                   nInFlight;
    if (m_nInFlightHWM < (nInFlight = (int) m_BusyList.GetCount()))
        m_nInFlightHWM =  nInFlight;
}

///////////////////////////////////////////////////////////////////////////////
// Append another COV to busy list

void COvSeqFile::AddBusyCOV( COV* pCOV )
{
    ASSERT( pCOV );
    m_BusyList.AddCOV( pCOV );
    TrackInFlight();
}

///////////////////////////////////////////////////////////////////////////////
// Get oldest from busy list or NULL if empty

COV* COvSeqFile::RemoveBusyCOV()
{
    COV* pCOV = NULL;
    if (InFlight())
    {
        pCOV = m_BusyList.RemoveCOV();
        ASSERT( pCOV );
    }
    return pCOV;
}

///////////////////////////////////////////////////////////////////////////////
// Append recently used COV to free list

void COvSeqFile::PushFreeCOV( COV* pCOV )
{
    ASSERT( pCOV );
    pCOV->m_dwIOBytes = 0;      // (reset)
    pCOV->m_dwLastError = 0;    // (reset)
    m_pFreeList->AddCOV( pCOV );
}

///////////////////////////////////////////////////////////////////////////////
// Pop most recently used COV from free list

COV* COvSeqFile::PopFreeCOV()
{
    COV* pCOV = m_pFreeList->PopCOV();
    if (pCOV)
    {
        pCOV->m_pOvSeqFile = this;  // (we now own it)
        pCOV->m_dwIOBytes = 0;      // (reset)
        pCOV->m_dwLastError = 0;    // (reset)

        if (!pCOV->AllocBuffer())    // (make sure it's big enough)
        {
            m_pFreeList->RemoveCOV( pCOV );
            delete pCOV;
            pCOV = NULL;
        }
        ASSERT( !pCOV || pCOV->BufferSize() >= BufferSize() );
    }
    return pCOV;
}

///////////////////////////////////////////////////////////////////////////////
// Give user a COV to work with

COV* COvSeqFile::GetCOV()
{
    COV* pCOV = PopFreeCOV();
    if (!pCOV)
    {
        m_dwLastError = ERROR_OUTOFMEMORY;
        return NULL;
    }
    AddUserCOV( pCOV );
    return pCOV;
}

///////////////////////////////////////////////////////////////////////////////
// Append done COV to user list

void COvSeqFile::AddUserCOV( COV* pCOV )
{
    ASSERT( pCOV );
    m_pUserList->AddCOV( pCOV );
}

///////////////////////////////////////////////////////////////////////////////
// User is done with their COV

bool COvSeqFile::FreeCOV( COV* pCOV )
{
    ASSERT( pCOV );
    return FreeUserCOV( pCOV );
}

///////////////////////////////////////////////////////////////////////////////
// Remove COV from user list

bool COvSeqFile::RemoveUserCOV( COV* pCOV )
{
    ASSERT( pCOV );
    if (!m_pUserList->RemoveCOV( pCOV ))
        return  false;
    pCOV->EnsureNotBusy();
    return true;
}

///////////////////////////////////////////////////////////////////////////////
// Move user COV to free list

bool COvSeqFile::FreeUserCOV( COV* pCOV )
{
    ASSERT( pCOV );
    pCOV->EnsureNotBusy(); // (ALWAYS, regardless)
    if (!RemoveUserCOV( pCOV ))
    {
        if (!m_BusyList.FindCOV( pCOV ))
            return false;
        VERIFY( m_BusyList.RemoveCOV( pCOV ));
    }
    PushFreeCOV( pCOV );
    return true;
}

///////////////////////////////////////////////////////////////////////////////
// Repossess oldest user COV

COV* COvSeqFile::StealUserCOV()
{
    COV* pCOV = m_pUserList->RemoveCOV();
    if (pCOV)
        m_nStolenCOVs++;
    return pCOV;
}

///////////////////////////////////////////////////////////////////////////////
// Open file

bool COvSeqFile::Open( LPCTSTR pszFileName )
{
    // Check parameters

    if (!pszFileName || !pszFileName[0])
    {
        ASSERT( false );
        m_dwLastError = ERROR_INVALID_PARAMETER;
        return false;
    }

    if (m_hFile)
        if (!Close())
            return false;

    // Open the file

    m_hFile        = NULL;
    m_bEndOfFile   = false;
    m_nFileSize    = 0;
    m_nNextOffset  = 0;
    m_nFileOffset  = 0;
    m_nInFlightHWM = 0;
    m_strFileName  = pszFileName;
    m_nStolenCOVs  = 0;

    WIN32_FILE_ATTRIBUTE_DATA info = {0};

    if (GetFileAttributesEx( m_strFileName, GetFileExInfoStandard, &info ))
    {
        // (size needed by below 'dwFlagsAndAttr' "IsBuffered()" call)

        m_nFileSize = ((DWORD64) info.nFileSizeHigh << 32) | info.nFileSizeLow;
    }
    else
    {
        m_dwLastError = GetLastError();  // (probably file not found)

        if (IsInput())
            return false;
    }

    DWORD  dwAccessMode   = (IsInput() ? GENERIC_READ  : GENERIC_WRITE );
    DWORD  dwCreateDisp   = (IsInput() ? OPEN_EXISTING : CREATE_ALWAYS );

    DWORD  dwShareMode    = FILE_SHARE_READ | (IsInput() ? FILE_SHARE_WRITE : 0);

    DWORD  dwFlagsAndAttr = FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED |
        (IsBuffered() ? FILE_FLAG_SEQUENTIAL_SCAN : FILE_FLAG_NO_BUFFERING );

    m_hFile = CreateFile
    (
        m_strFileName,
        dwAccessMode,
        dwShareMode,
        NULL,
        dwCreateDisp,
        dwFlagsAndAttr,
        NULL
    );

    if (INVALID_HANDLE_VALUE == m_hFile)
    {
        m_dwLastError = GetLastError();
        m_hFile = NULL;
        return false;
    }

    // Retrieve sector information

    DWORD  dwSectorsPerCluster;
    DWORD  dwBytesPerSector;
    DWORD  dwNumberOfFreeClusters;
    DWORD  dwTotalNumberOfClusters;

    CPath  strRootPath( m_strFileName );  strRootPath.StripToRoot();

    if (GetDiskFreeSpace( strRootPath,
                          &dwSectorsPerCluster,
                          &dwBytesPerSector,
                          &dwNumberOfFreeClusters,
                          &dwTotalNumberOfClusters ))
    {
        m_nBPS = (int) dwBytesPerSector;
        m_nSPC = (int) dwSectorsPerCluster;
    }
    else
    {
        m_nBPS = (int) OVSEQF_DEF_BPS;
        m_nSPC = (int) OVSEQF_DEF_SPC;
    }

    // Recalculate buffer size to be multiple of cluster size

    int nBPC = m_nBPS * m_nSPC; // (bytes per cluster)
    m_nBufferSize = ((m_nBufferSize + nBPC - 1) / nBPC) * nBPC;

    if (m_bAutoMaxInFlight)
        m_nMaxInFlight = COV::MaxInFlight( m_nBufferSize );

    // Pre-allocate all needed buffer space ahead of time. Note that
    // the m_nBufferSize value must have been previously calculated.

    ASSERT( m_BusyList  .  IsEmpty() );     // (sanity check)
    ASSERT( m_pUserList -> IsEmpty() );     // (sanity check)

    COV* pCOV;
    int i, k;

    m_pFreeList->Lock();
    {
        // (free buffers that we don't need)

        k = (int) m_pFreeList->GetCount();        // (how many we have)

        while (k > MaxInFlight())                 // (while too many)
        {
            delete m_pFreeList->RemoveCOV();      // (discard buffer)
            k = (int) m_pFreeList->GetCount();    // (adjust remaining)
        }

        // (expand existing buffers if needed)

        k = (int) m_pFreeList->GetCount();        // (how many there are)

        for (i=0; i < k; ++i)                     // (process entire list)
        {
            pCOV = m_pFreeList->RemoveCOV();      // (remove oldest from head)
            VERIFY( pCOV->AllocBuffer() );        // (re-allocate i/o buffer)
            m_pFreeList->AddCOV( pCOV );          // (add back to end of list)
        }

        // (allocate more buffers if needed)

        k = (int) MaxInFlight();

        for (i = (int) m_pFreeList->GetCount(); i < k; ++i)
        {
            if (!(pCOV = new COV( this )) || !pCOV->Buffer())
            {
                delete pCOV;
                Close();
                m_dwLastError = ERROR_OUTOFMEMORY;
                m_pFreeList->Unlock();
                return false;
            }
            m_pFreeList->AddCOV( pCOV );
        }
    }
    m_pFreeList->Unlock();

    return true;
}

///////////////////////////////////////////////////////////////////////////////
// close file

bool COvSeqFile::Close()
{
    bool bSuccess = true;

    if (m_hFile)
    {
        COV* pCOV;

        // Steal back all COVs the user never returned back to us

        while (!m_pUserList->IsEmpty())
            AddBusyCOV( pCOV = StealUserCOV() );

        // Wait for all oustanding I/Os to finish (hopefully successfully)

        while (InFlight())
        {
            pCOV = RemoveBusyCOV();

            if (pCOV->WaitForIO() < 0)
            {
                bSuccess = false;
                m_dwLastError = pCOV->m_dwLastError;
            }

            PushFreeCOV( pCOV );
        }

        if (IsOutput() && !Flush())
            bSuccess = false;

        VERIFY( CloseHandle( m_hFile ));
        m_hFile = NULL;
    }

    return bSuccess;
}

///////////////////////////////////////////////////////////////////////////////
// Get the next read started

bool COvSeqFile::StartNextRead( COV* pCOV )
{
    ASSERT( pCOV );                       // (sanity check)

    AddBusyCOV( pCOV );                   // (ALWAYS regardless of success)
    pCOV->m_dwLastError = 0;              // (ALWAYS before attempting I/O)

    pCOV->SetOffset( m_nNextOffset );     // (where THIS I/O should begin)
    m_nNextOffset += pCOV->BufferSize();  // (where NEXT I/O should begin)

    // (start the i/o)

    bool bSuccess =
        ReadFile( m_hFile, pCOV->Buffer(), pCOV->BufferSize(), &pCOV->m_dwIOBytes, pCOV )
            ? true : false;

    // (set last error and check for eof)

    if (ERROR_HANDLE_EOF == (pCOV->m_dwLastError = bSuccess ? ERROR_SUCCESS : GetLastError()))
    {
        bSuccess = true;
        ASSERT( !pCOV->IOBytes() );
        pCOV->m_dwIOBytes = 0;
    }
    else if (ERROR_IO_PENDING == pCOV->LastError())
        bSuccess = true;

    return bSuccess;
}

///////////////////////////////////////////////////////////////////////////////

int COvSeqFile::Read( COV*& pCOV )
{
    // Remove oldest previously started I/O

    if (!(pCOV = RemoveBusyCOV()))
    {
        // Start first I/O to get the ball rolling...

        if (!(pCOV = PopFreeCOV()))
        {
            // There aren't any active I/Os still in progress but yet
            // there aren't any free COVs either.
            //
            // The ONLY way this can possibly happen is if the user is
            // consuming COVs without giving them back to us (i.e. they
            // are failing to call FreeCOV() when they're done with the
            // COV we previously gave them).
            //
            // We are really left with no choice but to steal back the
            // oldest COVs we previously gave them.

            if (!(pCOV = StealUserCOV()))
            {
                TRACE_ALWAYS(_T("*** COvSeqFile::Read(): +++ WTF?! +++\n"));
                BREAK_INTO_DEBUGGER();
                m_dwLastError = ERROR_OUTOFMEMORY;
                return -1;
            }
        }
        else // (free COV obtained; start first I/O)
        {
            bool bSuccess = StartNextRead( pCOV );
            pCOV = RemoveBusyCOV();
            ASSERT( pCOV );

            if (!bSuccess)
            {
                m_nFileOffset = pCOV->IOOffset();
                AddUserCOV( pCOV );
                m_dwLastError = pCOV->LastError();
                return -1;
            }
        }
    }

    m_nFileOffset = pCOV->IOOffset();

    AddUserCOV( pCOV );

    // This is a read: must wait for the I/O to complete...

    int  rc;
    bool bHadToWait = !pCOV->IsIOComplete();

    if ((rc = pCOV->WaitForIO()) < 0)
    {
        m_dwLastError = pCOV->LastError();
        return -1;
    }

    if (0 == rc)
    {
        m_bEndOfFile = true;
        ASSERT( !pCOV->IOBytes() );  // (sanity check)
        return 0;
    }

    if (m_nNextOffset <= FileSize())
    {
        // Preemptively start next I/O to replace the one that finished

        COV*  pOurCOV;
        if (!(pOurCOV = PopFreeCOV()))
            return rc;

        // PROGRAMMING NOTE: it is not important whether either of the below
        // "StartNextRead" calls succeed or not since we'll detect the error
        // when we next come across the same COV the next time we're called.

        if (StartNextRead( pOurCOV ))   // (keep us caught up)
        {
            // If we had to wait for the current I/O to complete it means
            // the caller is processing data faster than we can deliver it,
            // so we need to preemptively start yet ANOTHER I/O to try and
            // get ahead of the game (so that, ideally, the next time they
            // call us, their I/O will already be completed). Also note we
            // must check m_nNextOffset value again because the preceding
            // "StartNextRead" call automatically increments it each time.

            if (m_nNextOffset <= FileSize() && bHadToWait && InFlight() < MaxInFlight())
            {
                if (!(pOurCOV = PopFreeCOV()))
                    return rc;

                StartNextRead( pOurCOV );   // (this gets us ahead)
            }
        }
    }

    return rc;    // (return bytes read for this I/O)
}

///////////////////////////////////////////////////////////////////////////////

int COvSeqFile::Write( COV* pCOV, int nIOBytes /* = -1 */ )
{
    ASSERT( pCOV );

    m_nFileOffset = m_nNextOffset;

    // The COV the user passes MUST be one we gave them...

    if (!RemoveUserCOV( pCOV ))
    {
        if (!m_BusyList.FindCOV( pCOV ))
        {
            m_dwLastError = ERROR_NOT_FOUND;
            return -1; // (not ours!)
        }
        VERIFY( m_BusyList.RemoveCOV( pCOV ));
    }

    pCOV->EnsureNotBusy();    // (ALWAYS before using any COV)
    pCOV->m_dwIOBytes = 0;    // (ALWAYS before starting an I/O)
    pCOV->m_dwLastError = 0;  // (ALWAYS before starting an I/O)
    m_dwLastError = 0;        // (ALWAYS before starting an I/O)

    // PROGRAMMING NOTE: we now have a freestanding COV in our hands.
    // We need to be careful not to lose track of it. That is to say,
    // we need to be careful to EVENTUALLY add it to either our busy
    // list (if the I/O is successfully started) or else to the user
    // list (if an error occurs or the i/o completes immediately).

    // Verify they're not asking for too much...

    if (nIOBytes <= 0)
        nIOBytes = pCOV->BufferSize();

    if (nIOBytes > pCOV->BufferSize())
    {
        m_dwLastError = pCOV->m_dwLastError = ERROR_BUFFER_OVERFLOW;
        AddUserCOV( pCOV );  // (give back to user)
        return -1;
    }

    // Before starting this request, check to make sure they're
    // not about to exceed their maximum in-flight quota. If so,
    // then we must wait for the oldest previously started I/O
    // to finish before starting this one...

    if (InFlight() >= MaxInFlight())
    {
        COV* pOldestBusyCOV = RemoveBusyCOV();
        if (pOldestBusyCOV)
        {
            if (pOldestBusyCOV->WaitForIO() < 0)
            {
                m_dwLastError = pOldestBusyCOV->LastError();

                // CAREFUL! Must not lose track of their COV!

                PushFreeCOV( pCOV );    // (save theirs in free list)
                pCOV = pOldestBusyCOV;  // (switch to the error COV)
                AddUserCOV( pCOV );     // (they now own this COV)
                return -1;              // (return with I/O error)
            }

            PushFreeCOV( pOldestBusyCOV );
        }
    }

    // Start their I/O and return without waiting...

    pCOV->SetOffset( m_nNextOffset ); // (where THIS I/O should begin)
    m_nNextOffset += nIOBytes;        // (where NEXT I/O should begin)

    if (!WriteFile( m_hFile, pCOV->Buffer(), nIOBytes, &pCOV->m_dwIOBytes, pCOV ))
    {
        int rc = (ERROR_IO_PENDING ==
            (m_dwLastError = pCOV->m_dwLastError = GetLastError())) ? 0 : -1;

        if (rc < 0)
            AddUserCOV( pCOV );     // (error: i/o done; user owns COV)
        else
        {
            AddBusyCOV( pCOV );     // (i/o in progress: we own the COV)

            m_nFileOffset = m_nNextOffset;
        }

        return rc;                  // (return -1=error or 0=in-progress)
    }

    m_nFileOffset = m_nNextOffset;

    AddUserCOV( pCOV );             // (i/o complete: user owns the COV)
    return (int) pCOV->IOBytes();   // (return number of bytes written)
}

///////////////////////////////////////////////////////////////////////////////
// flush output buffers to disk

bool COvSeqFile::Flush()
{
    BOOL bSuccess;
    if (!(bSuccess = FlushFileBuffers( m_hFile )))
        m_dwLastError = GetLastError();
    return bSuccess ? true : false;
}

///////////////////////////////////////////////////////////////////////////////
// returns true if file is opened

bool COvSeqFile::IsOpen() const
{
    return m_hFile ? true : false;
}

///////////////////////////////////////////////////////////////////////////////
// returns true if file is close

bool COvSeqFile::IsClosed() const
{
    return !IsOpen();
}

///////////////////////////////////////////////////////////////////////////////
// returns true if at EOF

bool COvSeqFile::IsEndOfFile() const
{
    return m_bEndOfFile;
}

///////////////////////////////////////////////////////////////////////////////
// returns true if input file

bool COvSeqFile::IsInput() const
{
    return m_bInput;
}

///////////////////////////////////////////////////////////////////////////////
// returns true if output file

bool COvSeqFile::IsOutput() const
{
    return !IsInput();
}

///////////////////////////////////////////////////////////////////////////////
// returns true if buffered I/O

bool COvSeqFile::IsBuffered() const
{
    return m_pAssocFile ? m_pAssocFile->IsBuffered()
                        : IsInput() ? FileSize() <= BuffIOMax()
                                    : true; // (default)
}

///////////////////////////////////////////////////////////////////////////////
// returns true if unbuffered I/O

bool COvSeqFile::IsUnBuffered() const
{
    return !IsBuffered();
}

///////////////////////////////////////////////////////////////////////////////
// returns file HANDLE

COvSeqFile::operator HANDLE() const
{
    return m_hFile;
}

///////////////////////////////////////////////////////////////////////////////
// returns pointer to filename

COvSeqFile::operator LPCTSTR() const
{
    return FileName();
}

///////////////////////////////////////////////////////////////////////////////
// returns logical file offset

DWORD64 COvSeqFile::Offset() const
{
    return m_nFileOffset;
}

///////////////////////////////////////////////////////////////////////////////
// returns latest I/O request offset

DWORD64 COvSeqFile::IOOffset()
{
    DWORD64 nIOOffset = 0;
    m_BusyList.Lock();
    {
        if (!m_BusyList.IsEmpty())
        {
            // (use most recent I/O request)
            nIOOffset = m_BusyList.GetTail()->IOOffset();
        }
    }
    m_BusyList.Unlock();
    return nIOOffset;
}

///////////////////////////////////////////////////////////////////////////////
// returns last error code

DWORD COvSeqFile::LastError() const
{
    return m_dwLastError;
}

///////////////////////////////////////////////////////////////////////////////
// returns buffered I/O threshold

DWORD64 COvSeqFile::BuffIOMax() const
{
    return m_nBuffIOMax;
}

///////////////////////////////////////////////////////////////////////////////
// returns bytes per sector

int COvSeqFile::BPS() const
{
    return m_nBPS;
}

///////////////////////////////////////////////////////////////////////////////
// returns sectors per cluster

int COvSeqFile::SPC() const
{
    return m_nSPC;
}

///////////////////////////////////////////////////////////////////////////////
// returns the possibly adjusted I/O buffer size

int COvSeqFile::BufferSize() const
{
    return m_nBufferSize;
}

///////////////////////////////////////////////////////////////////////////////
// returns maximum number of simultaneous I/Os allowed

int COvSeqFile::MaxInFlight() const
{
    return m_nMaxInFlight;
}

///////////////////////////////////////////////////////////////////////////////
// returns number of currently active simultaneous I/Os

int COvSeqFile::InFlight() const
{
    return (int) m_BusyList.GetCount();
}

///////////////////////////////////////////////////////////////////////////////
// returns most ever simultaneously active I/Os

int COvSeqFile::InFlightHWM() const
{
    return m_nInFlightHWM;
}

///////////////////////////////////////////////////////////////////////////////
// returns count of stolen user COVs

int COvSeqFile::StolenCOVs() const
{
    return m_nStolenCOVs;
}

///////////////////////////////////////////////////////////////////////////////
// returns size of file in bytes

DWORD64 COvSeqFile::FileSize() const
{
    return IsInput() ? m_nFileSize : m_nFileOffset;
}

///////////////////////////////////////////////////////////////////////////////
// returns filename as CString

CString COvSeqFile::FileName() const
{
    return m_strFileName;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
