/**
 * SimpleList Library
 * Last modified: 2009/01/04
 * Copyright (C) Kai Liu.  All rights reserved.
 *
 * This library implements a highly efficient, fast, light-weight, and simple
 * list structure in C, with the goal of replacing the slower, heavier and
 * overly complex STL list.
 *
 * This list is intended as a write-once list; once an item has been added,
 * removing it or altering the data in a way that changes the size of the data
 * is unsupported.
 **/

#pragma once

#define SL_ENABLE_LGBLK
#define SL_SMBLK_SIZE       (4 * KB)

/**
 * The SimpleList handle.
 **/

typedef void* HSIMPLELIST, *PHSIMPLELIST;

/**
 * SLCreate: Returns the handle to a newly created and initialized list using
 * either small heap-allocated blocks or large high-granularity blocks; NULL
 * is returned if allocation failed.
 **/

HSIMPLELIST __fastcall SLCreate( BOOL bLargeBlock );

/**
 * SLAddRef: Increments the reference counter; the initial value of the counter
 * after SLInit is 1, so there is no need to call SLAddRef after SLInit.
 *
 * SLRelease: Decrements the reference counter; SLRelease will destroy the list
 * and free the memory if the reference counter reaches zero, in which case,
 * the list handle will no longer be valid.
 **/

void __fastcall SLAddRef( HSIMPLELIST hSimpleList );
void __fastcall SLRelease( HSIMPLELIST hSimpleList );

/**
 * SLReset: Resets the current position to the position of the first item.
 **/

void __fastcall SLReset( HSIMPLELIST hSimpleList );

/**
 * SLCheck: Returns TRUE if the current item is valid; FALSE otherwise.
 **/

BOOL __fastcall SLCheck( HSIMPLELIST hSimpleList );

/**
 * SLGetData*: Returns a pointer to the data of the current item; optionally,
 * the you can obtain the size of the data block, and the current position can
 * also be incremented after the data has been retrieved; NULL is returned if
 * the current position is invalid (e.g., past the end).
 *
 * NOTE: Do not free the pointer that was returned.
 **/

void* __fastcall SLGetDataAndStep( HSIMPLELIST hSimpleList );
void* __fastcall SLGetDataAndStepEx( HSIMPLELIST hSimpleList, PUINT pcbData );

/**
 * SLAddItem: Adds a block of data to the end of the list; a pointer to a
 * newly-created data block of cbData bytes long is returned if memory
 * allocation was successful (otherwise, NULL is returned).  NULL may be passed
 * in pvData if one wishes to use the returned pointer to copy the data.
 *
 * NOTE: Do not free the pointer that was returned.
 **/

void* __fastcall SLAddItem( HSIMPLELIST hSimpleList, const void* pvData, UINT cbData );

__inline void* __fastcall SLAddStringI( HSIMPLELIST hSimpleList, LPCTSTR psz )
{
   return(SLAddItem(hSimpleList, psz, ((UINT)wcslen(psz) + 1) * sizeof(TCHAR)));
}

/**
 * SLBuildIndex: Fills an array with the data pointers of every item; the array
 * is provided by the caller and must be at least cItems * sizeof(void*) bytes
 * in size.  The caller is thus also responsible for maintaining a count of the
 * number of elements in the list.  The current list position is NOT affected.
 **/

void __fastcall SLBuildIndex( HSIMPLELIST hSimpleList, void** ppvIndex );

/**
 * SLSetContextSize: Sets the amount of data allocated for an arbitrary block
 * of user data associated with the list; if successful, a pointer to the data
 * block is returned, otherwise, NULL is returned.  No memory is allocated for
 * the user data block when the list is first created.
 *
 * SLGetContextData: Returns a pointer to the user data block associated with
 * the list; NULL is returned if the data block had not been allocated.
 *
 * NOTE: Do not free the pointer that was returned; the pointer may also become
 * invalid after a SLRelease or SLSetContextSize call.
 **/

void* __fastcall SLSetContextSize( HSIMPLELIST hSimpleList, UINT cbContextSize );
void* __fastcall SLGetContextData( HSIMPLELIST hSimpleList );
