/**
 * SimpleList Library
 * Last modified: 2009/01/04
 * Copyright (C) Kai Liu.  All rights reserved.
 **/

#include "stdafx.h"
#include "SimpleList.h"

/**
 * Control structures
 **/

struct SLITEM
{
    void*     pvNext;       // address of the next item; NULL if there is none
    UINT      cbData;       // size, in bytes, of the current item
    UINT_PTR  data[];       // properly aligned start of data
};
typedef struct SLITEM SLITEM, *PSLITEM;

struct SLBLOCK
{
    void*     pvPrev;       // address of the previous memory block; NULL if there is none
    UINT_PTR  data[];       // properly aligned start of data
};
typedef struct SLBLOCK SLBLOCK, *PSLBLOCK;

struct SLHEADER {
   void*    pvContext;      // optional user context data associated with this list
   PSLITEM  pItemStart;     // the first item; NULL if list is empty
   PSLITEM  pItemCurrent;   // the current item (used only by read/step)
   PSLITEM  pItemLast;      // the last item; NULL if list is empty (used only by add)
   PSLITEM  pItemNew;       // the tail (used only by add)
   PSLBLOCK pBlockCurrent;  // the current block to which new items will be added
   UINT     cbRemaining;    // bytes remaining in the current block
   UINT     cbBlockSize;    // total bytes in each block
   BOOL     bLargeBlock;    // use large blocks allocated by VirtualAlloc?
   VLONG    cRef;           // 0-based reference count (0 == 1 reference, 1 == 2 references, etc.)
};
typedef struct SLHEADER SLHEADER, *PSLHEADER;

/**
 * Memory block management functions
 **/

__inline void* SLInternal_AllocBlock( PSLHEADER pHeader, UINT cbSize )
{
   if (pHeader->bLargeBlock)
      return(VirtualAlloc(NULL, cbSize, MEM_COMMIT, PAGE_READWRITE));
   else
      return(malloc(cbSize));
}

__inline void SLInternal_FreeBlock( PSLHEADER pHeader, void* pvBlock )
{
   if (pHeader->bLargeBlock)
      VirtualFree(pvBlock, 0, MEM_RELEASE);
   else
      free(pvBlock);
}

__inline UINT SLInternal_GetBlockSize( PSLHEADER pHeader )
{
    if (!pHeader->bLargeBlock)
        return SL_SMBLK_SIZE;

    static SYSTEM_INFO si = {0};

    if (!si.dwAllocationGranularity)
        GetSystemInfo( &si );

    return si.dwAllocationGranularity;
}

/**
 * Internal helper functions
 **/

__inline void SLInternal_Destroy( PSLHEADER pHeader )
{
   PSLBLOCK pBlock, pBlockPrev;

   for (pBlock = pHeader->pBlockCurrent; pBlock; pBlock = pBlockPrev)
   {
      pBlockPrev = (PSLBLOCK) pBlock->pvPrev;
      SLInternal_FreeBlock(pHeader, pBlock);
   }

   free(pHeader->pvContext);
   free(pHeader);
}

__inline BOOL SLInternal_Check( PSLHEADER pHeader )
{
   return(pHeader && pHeader->pItemCurrent);
}

__inline UINT SLInternal_Align( UINT uData )
{
   if (sizeof(UINT_PTR) == 4 || sizeof(UINT_PTR) == 8)
   {
      // Round up to the nearest pointer-sized multiple
      return((uData + (sizeof(UINT_PTR) - 1)) & ~(sizeof(UINT_PTR) - 1));
   }
   else
   {
      // In the highly unlikely event that this code is compiled on some
      // architecture that does not have a 4- or 8-byte word size, here is
      // a more general alignment formula; this entire branch will be culled
      // by the compiler's optimizer on normal architectures
      return(((uData + (sizeof(UINT_PTR) - 1)) / sizeof(UINT_PTR)) * sizeof(UINT_PTR));
   }
}

/**
 * SLCreate
 **/

HSIMPLELIST __fastcall SLCreate( BOOL bLargeBlock )
{
   PSLHEADER pHeader = (PSLHEADER) malloc(sizeof(SLHEADER));

   if (pHeader)
   {
      // Since cRef is a 0-based counter (vs. the usual 1-based counter),
      // the initial value of 0 from memset is correct
      memset( pHeader, 0, sizeof( SLHEADER ));

      pHeader->bLargeBlock = bLargeBlock;
      pHeader->cbBlockSize = SLInternal_GetBlockSize(pHeader);
   }

   return(pHeader);
}

/**
 * SLAddRef
 * SLRelease
 **/

void __fastcall SLAddRef( HSIMPLELIST hSimpleList )
{
   const PSLHEADER pHeader = (PSLHEADER)hSimpleList;

   if (pHeader)
      InterlockedIncrement(&pHeader->cRef);
}

void __fastcall SLRelease( HSIMPLELIST hSimpleList )
{
   const PSLHEADER pHeader = (PSLHEADER)hSimpleList;

   if (pHeader)
   {
      if (InterlockedDecrement(&pHeader->cRef) < 0)
         SLInternal_Destroy(pHeader);
   }
}

/**
 * SLReset
 **/

void __fastcall SLReset( HSIMPLELIST hSimpleList )
{
   const PSLHEADER pHeader = (PSLHEADER)hSimpleList;

   if (pHeader)
      pHeader->pItemCurrent = pHeader->pItemStart;
}

/**
 * SLCheck
 **/

BOOL __fastcall SLCheck( HSIMPLELIST hSimpleList )
{
   return(SLInternal_Check((PSLHEADER)hSimpleList));
}

/**
 * SLGetDataAndStep
 * SLGetDataAndStepEx
 **/

void* __fastcall SLGetDataAndStep( HSIMPLELIST hSimpleList )
{
   const PSLHEADER pHeader = (PSLHEADER)hSimpleList;

   if (SLInternal_Check(pHeader))
   {
      void* pvData = pHeader->pItemCurrent->data;
      pHeader->pItemCurrent = (PSLITEM) pHeader->pItemCurrent->pvNext;
      return(pvData);
   }

   return(NULL);
}

void* __fastcall SLGetDataAndStepEx( HSIMPLELIST hSimpleList, PUINT pcbData )
{
   const PSLHEADER pHeader = (PSLHEADER)hSimpleList;

   if (SLInternal_Check(pHeader))
   {
      void* pvData = pHeader->pItemCurrent->data;
      *pcbData = pHeader->pItemCurrent->cbData;
      pHeader->pItemCurrent = (PSLITEM) pHeader->pItemCurrent->pvNext;
      return(pvData);
   }

   return(NULL);
}

/**
 * SLAddItem
 **/

void* __fastcall SLAddItem( HSIMPLELIST hSimpleList, const void* pvData, UINT cbData )
{
   const PSLHEADER pHeader = (PSLHEADER)hSimpleList;

   // Note that adding items larger than the block size is not supported
   const UINT cbRequired = SLInternal_Align(sizeof(SLITEM) + cbData);

   if (pHeader && cbRequired <= pHeader->cbBlockSize - sizeof(SLBLOCK))
   {
      if (cbRequired > pHeader->cbRemaining)
      {
         PSLBLOCK pBlockNew = (PSLBLOCK) SLInternal_AllocBlock(pHeader, pHeader->cbBlockSize);

         if (pBlockNew)
         {
            pBlockNew->pvPrev = pHeader->pBlockCurrent;
            pHeader->pItemNew = (PSLITEM)pBlockNew->data;
            pHeader->pBlockCurrent = pBlockNew;
            pHeader->cbRemaining = pHeader->cbBlockSize - sizeof(SLBLOCK);
         }
         else
         {
            // Allocation failure
            return(NULL);
         }
      }

      // Initialize the new SLITEM

      pHeader->pItemNew->pvNext = NULL;
      pHeader->pItemNew->cbData = cbData;

      if (pvData)
          memcpy(pHeader->pItemNew->data, pvData, cbData);

      if (!pHeader->pItemStart)
      {
         // If this is the first item in the list...
         pHeader->pItemStart = pHeader->pItemNew;
         pHeader->pItemCurrent = pHeader->pItemNew;
      }
      else
      {
         // If this is not the first item, then link it to the previous item
         pHeader->pItemLast->pvNext = pHeader->pItemNew;
      }

      pHeader->pItemLast = pHeader->pItemNew;
      pHeader->pItemNew = (PSLITEM)((PBYTE)pHeader->pItemNew + cbRequired);
      pHeader->cbRemaining -= cbRequired;

      return(pHeader->pItemLast->data);
   }

   return(NULL);
}

/**
 * SLBuildIndex
 **/

void __fastcall SLBuildIndex( HSIMPLELIST hSimpleList, void** ppvIndex )
{
   const PSLHEADER pHeader = (PSLHEADER)hSimpleList;

   if (pHeader)
   {
      PSLITEM pItem;

      for (pItem = pHeader->pItemStart; pItem; pItem = (PSLITEM) pItem->pvNext)
      {
         *ppvIndex = pItem->data;
         ++ppvIndex;
      }
   }
}

/**
 * SLSetContextSize
 * SLGetContextData
 **/

void* __fastcall SLSetContextSize( HSIMPLELIST hSimpleList, UINT cbContextSize )
{
   const PSLHEADER pHeader = (PSLHEADER)hSimpleList;

   if (pHeader)
   {
      void* pvNew = realloc(pHeader->pvContext, cbContextSize);

      if (pvNew || !cbContextSize)
         return(pHeader->pvContext = pvNew);
   }

   return(NULL);
}

void* __fastcall SLGetContextData( HSIMPLELIST hSimpleList )
{
   const PSLHEADER pHeader = (PSLHEADER)hSimpleList;

   if (pHeader)
      return(pHeader->pvContext);

   return(NULL);
}
