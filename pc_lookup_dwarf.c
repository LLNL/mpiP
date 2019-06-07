/* -*- C -*- 

   mpiP MPI Profiler ( http://llnl.github.io/mpiP )

   Please see COPYRIGHT AND LICENSE information at the end of this file.

   ----- 

   pc_lookup_dwarf.c -- functions that use libdwarf for symbol lookup

   $Id$
 */

#ifndef lint
static char *svnid =
  "$Id$";
#endif


#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#include "mpiPi.h"
#include "mpiPconfig.h"

#if defined(USE_LIBDWARF)

#ifndef CEXTRACT
#include "elf.h"
#include "libelf.h"
#include "dwarf.h"
#include "libdwarf.h"
#endif

static Dwarf_Error dw_err;


/*----------------------------------------------------------------------------
 * collection of unique file name strings
 *
 * In the address-to-source line information, we expect to see the same file
 * name several times.  Rather than allocate a separate string for each
 * file name, we maintain a collection of unique file names.
 *
 * Unlike the function or address-to-source line information, we expect
 * only a small number of unique file names.  Whereas we want lookup
 * to be quick, we also want the implementation to be simple, and because
 * we don't have a nice STL-like collection of container classes available,
 * we use a linked list with linear search for lookup for this collection.
 * Because we expect a small number of unique names, the linear search
 * should not be too much of a performance penalty.
 */
struct UniqueFileNameInfo
{
  char *fileName;
};

struct UniqueFileNameListNode
{
  struct UniqueFileNameInfo ufi;
  struct UniqueFileNameListNode *next;
};

static struct UniqueFileNameListNode *uniqueFileNameList = NULL;


static char *
UniqueFileName_Get (const char *testFileName)
{
  char *ret = NULL;
  struct UniqueFileNameListNode *currInfo = NULL;

  assert (testFileName != NULL);

  currInfo = uniqueFileNameList;
  while (currInfo != NULL)
    {
      if (strcmp (testFileName, currInfo->ufi.fileName) == 0)
        {
          /* we found a match */
          break;
        }

      /* advance to the next file name */
      currInfo = currInfo->next;
    }

  if (currInfo != NULL)
    {
      /* 
       * We found a match during our search -
       * use the already-allocated string.
       */
      ret = currInfo->ufi.fileName;
    }
  else
    {
      /*
       * We didn't find a match during our search.
       * Allocate a new string.
       */
      currInfo = (struct UniqueFileNameListNode *)
          malloc (sizeof (struct UniqueFileNameListNode));
      if (currInfo == NULL)
        {
          mpiPi_abort ("malloc failed\n");
        }

      currInfo->ufi.fileName = strdup (testFileName);
      if (currInfo->ufi.fileName == NULL)
        {
          mpiPi_abort ("malloc failed\n");
        }

      /* 
       * link the new entry into our list
       * since the list is unordered, just link it in
       * at the beginning for simplicity.
       */
      currInfo->next = uniqueFileNameList;
      uniqueFileNameList = currInfo;

      ret = currInfo->ufi.fileName;
    }

  assert (ret != NULL);
  return ret;
}


static void
UniqueFileName_Destroy (void)
{
  struct UniqueFileNameListNode *currInfo = uniqueFileNameList;
  while (currInfo != NULL)
    {
      struct UniqueFileNameListNode *nextInfo = currInfo->next;

      free (currInfo->ufi.fileName);
      free (currInfo);

      currInfo = nextInfo;
    }
}




/*----------------------------------------------------------------------------
 * address range map
 * 
 * Collection organized by non-overlapping address ranges.
 * Organized as a red-black tree for quick insertion and quick lookup.
 */

enum AddrRangeMapNodeColor
{
  AddrRangeMapNodeColor_Red,
  AddrRangeMapNodeColor_Black
};

struct AddrRangeMapNode
{
  enum AddrRangeMapNodeColor color;
  Dwarf_Addr lowAddr;
  Dwarf_Addr highAddr;

  void *info;

  struct AddrRangeMapNode *parent;
  struct AddrRangeMapNode *leftChild;
  struct AddrRangeMapNode *rightChild;
};


struct AddrRangeMap
{
  struct AddrRangeMapNode *root;
};


static void
AddrRangeMap_Init (struct AddrRangeMap *map)
{
  assert (map != NULL);
  map->root = NULL;
}


static void
AddrRangeMap_DestroyAux (struct AddrRangeMapNode *node,
                         void (*nodeInfoFreeFunc) (void *))
{
  assert (node != NULL);

  /* destroy subtrees of node */
  if (node->leftChild != NULL)
    {
      AddrRangeMap_DestroyAux (node->leftChild, nodeInfoFreeFunc);
      node->leftChild = NULL;
    }
  if (node->rightChild != NULL)
    {
      AddrRangeMap_DestroyAux (node->rightChild, nodeInfoFreeFunc);
      node->rightChild = NULL;
    }

  /* destroy node itself */
  if ((nodeInfoFreeFunc != NULL) && (node->info != NULL))
    {
      nodeInfoFreeFunc (node->info);
    }
  free (node);
}


static void
AddrRangeMap_Destroy (struct AddrRangeMap *map,
                      void (*nodeInfoFreeFunc) (void *))
{
  if (map != NULL)
    {
      if (map->root != NULL)
        {
          AddrRangeMap_DestroyAux (map->root, nodeInfoFreeFunc);
        }
      free (map);
    }
}


static void
AddrRangeMap_LeftRotate (struct AddrRangeMap *map,
                         struct AddrRangeMapNode *node)
{
  struct AddrRangeMapNode *oldRightChild = NULL;

  assert (map != NULL);
  assert (node != NULL);
  assert (node->rightChild != NULL);

  /*
   * save old right child; will become new root of 
   * subtree currently rooted at node 
   */
  oldRightChild = node->rightChild;
  node->rightChild = oldRightChild->leftChild;
  if (oldRightChild->leftChild != NULL)
    {
      oldRightChild->leftChild->parent = node;
    }

  /* do the rotation */
  oldRightChild->parent = node->parent;
  if (node->parent == NULL)
    {
      map->root = oldRightChild;
    }
  else if (node == node->parent->leftChild)
    {
      node->parent->leftChild = oldRightChild;
    }
  else
    {
      node->parent->rightChild = oldRightChild;
    }

  oldRightChild->leftChild = node;
  node->parent = oldRightChild;
}


static void
AddrRangeMap_RightRotate (struct AddrRangeMap *map,
                          struct AddrRangeMapNode *node)
{
  struct AddrRangeMapNode *oldLeftChild = NULL;

  assert (map != NULL);
  assert (node != NULL);
  assert (node->leftChild != NULL);

  /*
   * save old left child; will become new root of 
   * subtree currently rooted at node 
   */
  oldLeftChild = node->leftChild;
  node->leftChild = oldLeftChild->rightChild;
  if (oldLeftChild->rightChild != NULL)
    {
      oldLeftChild->rightChild->parent = node;
    }

  /* do the rotation */
  oldLeftChild->parent = node->parent;
  if (node->parent == NULL)
    {
      map->root = oldLeftChild;
    }
  else if (node == node->parent->rightChild)
    {
      node->parent->rightChild = oldLeftChild;
    }
  else
    {
      node->parent->leftChild = oldLeftChild;
    }

  oldLeftChild->rightChild = node;
  node->parent = oldLeftChild;
}





static void
AddrRangeMap_Add (struct AddrRangeMap *map,
                  Dwarf_Addr lowAddr, Dwarf_Addr highAddr, void *info)
{
  struct AddrRangeMapNode *currNode = NULL;
  struct AddrRangeMapNode *newEntry = NULL;


  mpiPi_msg_debug ("AddrRangeMap::Add: [0x%016llx,0x%016llx]\n",
                   lowAddr, highAddr);

  /* build a new entry for this mapping */
  newEntry =
    (struct AddrRangeMapNode *) malloc (sizeof (struct AddrRangeMapNode));

  if (newEntry == NULL)
    {
      mpiPi_abort ("malloc failed\n");
    }
  newEntry->color = AddrRangeMapNodeColor_Red;
  assert (lowAddr <= highAddr);
  newEntry->lowAddr = lowAddr;
  newEntry->highAddr = highAddr;
  newEntry->info = info;
  newEntry->parent = NULL;
  newEntry->leftChild = NULL;
  newEntry->rightChild = NULL;

  /* add the new node to our map */
  if (map->root == NULL)
    {
      /* new node is the first node to be added */
      map->root = newEntry;
    }
  else
    {
      /* new node is not the first node to be added */
      currNode = map->root;
      while (currNode != NULL)
        {
          if (highAddr <= currNode->lowAddr)
            {
              /* target address is below range covered by current node */
              if (currNode->leftChild != NULL)
                {
                  /* compare with ranges smaller than ours */
                  currNode = currNode->leftChild;
                }
              else
                {
                  /* adopt new node as our left child */
                  currNode->leftChild = newEntry;
                  newEntry->parent = currNode;
                  break;
                }
            }
          else if (lowAddr >= currNode->highAddr)
            {
              /* target address is above range covered by current node */
              if (currNode->rightChild != NULL)
                {
                  /* compare with ranges larger than ours */
                  currNode = currNode->rightChild;
                }
              else
                {
                  /* adopt new node as our right child */
                  currNode->rightChild = newEntry;
                  newEntry->parent = currNode;
                  break;
                }
            }
          else
            {
              /* new range overlaps with our range! */
              mpiPi_abort
                  ("new address node range [0x%016llx,0x%016llx] overlaps our range [0x%016llx,0x%016llx]\n",
                   lowAddr, highAddr, currNode->lowAddr, currNode->highAddr);
            }
        }
    }


  /*
   * new node has been inserted, but its insertion
   * may have unbalanced the tree -
   * re-balance it.
   * Based on red-black insert algorithm given in 
   * Cormen, Lieserson, Rivest "Introduction to Algorithms"
   */
  currNode = newEntry;
  while ((currNode != map->root) &&
         (currNode->parent->color == AddrRangeMapNodeColor_Red))
    {
      struct AddrRangeMapNode *uncleNode = NULL;

      if (currNode->parent == currNode->parent->parent->leftChild)
        {
          /* currNode's parent is its parent's left child */
          uncleNode = currNode->parent->parent->rightChild;
          if ((uncleNode != NULL) &&
              (uncleNode->color == AddrRangeMapNodeColor_Red))
            {
              currNode->parent->color = AddrRangeMapNodeColor_Black;
              uncleNode->color = AddrRangeMapNodeColor_Black;
              currNode->parent->parent->color = AddrRangeMapNodeColor_Red;
              currNode = currNode->parent->parent;
            }
          else
            {
              /* uncleNode is NULL (NULL is assumed black) or is black */
              if (currNode == currNode->parent->rightChild)
                {
                  currNode = currNode->parent;
                  AddrRangeMap_LeftRotate (map, currNode);
                }
              currNode->parent->color = AddrRangeMapNodeColor_Black;
              currNode->parent->parent->color = AddrRangeMapNodeColor_Red;
              AddrRangeMap_RightRotate (map, currNode->parent->parent);
            }
        }
      else
        {
          /* currNode's parent is its parent's right child */
          uncleNode = currNode->parent->parent->leftChild;
          if ((uncleNode != NULL) &&
              (uncleNode->color == AddrRangeMapNodeColor_Red))
            {
              currNode->parent->color = AddrRangeMapNodeColor_Black;
              uncleNode->color = AddrRangeMapNodeColor_Black;
              currNode->parent->parent->color = AddrRangeMapNodeColor_Red;
              currNode = currNode->parent->parent;
            }
          else
            {
              /* uncleNode is NULL (NULL is assumed black) or is black */
              if (currNode == currNode->parent->leftChild)
                {
                  currNode = currNode->parent;
                  AddrRangeMap_RightRotate (map, currNode);
                }
              currNode->parent->color = AddrRangeMapNodeColor_Black;
              currNode->parent->parent->color = AddrRangeMapNodeColor_Red;
              AddrRangeMap_LeftRotate (map, currNode->parent->parent);
            }
        }
    }
  assert (map->root != NULL);
  map->root->color = AddrRangeMapNodeColor_Black;
}



static const void *
AddrRangeMap_Find (struct AddrRangeMap *map, void *addr)
{
  struct AddrRangeMapNode *currNode;
  const void *ret = NULL;

  mpiPi_msg_debug ("AddrRangeMap::Find: looking for %p\n", addr);

  currNode = map->root;
  while (currNode != NULL)
    {
      mpiPi_msg_debug
          ("AddrRangeMap::Find: comparing with [0x%016llx,0x%016llx]\n",
           currNode->lowAddr, currNode->highAddr);

      if (((Dwarf_Addr) addr) < currNode->lowAddr)
        {
          /* target address is below range covered by current node */
          /* NOTE: we might not have a left child */
          mpiPi_msg_debug ("AddrRangeMap::Find: going left\n");
          currNode = currNode->leftChild;
        }
      else if (((Dwarf_Addr) addr) > currNode->highAddr)
        {
          /* target address is above range covered by current node */
          /* NOTE: we might not have a right child */
          mpiPi_msg_debug ("AddrRangeMap::Find: going right\n");
          currNode = currNode->rightChild;
        }
      else
        {
          /* target address falls within range of current node's statement */
          mpiPi_msg_debug ("AddrRangeMap::Find: found\n");
          ret = currNode->info;
          break;
        }
    }

  return ret;
}



/*----------------------------------------------------------------------------
 * address to source line map
 *
 * The DWARF library interface can provide address-to-source line mapping
 * information.  However, it does not necessarily define a mapping for
 * each address in the executable.
 *
 * If address-to-source line information is available, we expect there
 * to be a *lot* of it.  We want to maintain this information in 
 * a data structure so that lookups are quick and inserts are reasonable.
 * We don't modify or delete the mapping information once we've built
 * the data structure (except to tear it all down at the end of the run),
 * so we don't care so much about the cost of modifying or deleting mapping
 * information.
 *
 */

struct AddrToSourceInfo
{
  char *fileName;
  unsigned int lineNumber;
};


static struct AddrRangeMap *addrToSourceMap = NULL;



static void
AddrToSourceMap_Init (void)
{
  addrToSourceMap =
    (struct AddrRangeMap *) malloc (sizeof (struct AddrRangeMap));
  if (addrToSourceMap == NULL)
    {
      mpiPi_abort ("malloc failed\n");
    }
  AddrRangeMap_Init (addrToSourceMap);
}



static void
AddrToSourceMap_Destroy (void)
{
  if (addrToSourceMap != NULL)
    {
      AddrRangeMap_Destroy (addrToSourceMap, free);
      addrToSourceMap = NULL;
    }
}



static void
AddrToSourceMap_Add (Dwarf_Addr addr,
                     const char *fileName, unsigned int lineNumber)
{
  /* build a new entry for this mapping */
  struct AddrToSourceInfo *newEntry =
      (struct AddrToSourceInfo *) malloc (sizeof (struct AddrToSourceInfo));
      if (newEntry == NULL)
        {
          mpiPi_abort ("malloc failed\n");
        }
      newEntry->fileName = UniqueFileName_Get (fileName);
      assert (newEntry->fileName != NULL);
      newEntry->lineNumber = lineNumber;

      mpiPi_msg_debug ("AddrToSourceMap::Add: 0x%016llx => %s: %d\n",
                       addr, newEntry->fileName, newEntry->lineNumber);



      assert (addrToSourceMap != NULL);
      AddrRangeMap_Add (addrToSourceMap, addr, addr,	/* we will patch range ends later */
                        newEntry);
}





static Dwarf_Addr
AddrToSourceMap_PatchRangesAux (struct AddrRangeMapNode *node,
                                Dwarf_Addr maxAddress)
{
  Dwarf_Addr ret;

  assert (node != NULL);

  /* check for address ranges smaller than ours */
  if (node->leftChild != NULL)
    {
      /*
       * patch ranges in our left subtree
       * max address of any range below ours is our min address.
       */
      ret = AddrToSourceMap_PatchRangesAux (node->leftChild, node->lowAddr);
    }
  else
    {
      /*
       * we have no left child,
       * so there are no ranges we know of that are smaller than ours.
       */
      ret = node->lowAddr;
    }

  /* check for ranges greater than ours */
  if (node->rightChild != NULL)
    {
      /*
       * we have a right child,
       * so there are ranges that are larger than ours
       */
      maxAddress = AddrToSourceMap_PatchRangesAux (node->rightChild,
                                                   maxAddress);
    }

  /*
   * limit our range according to our (possibly updated) idea of the
   * minimum address larger than our range.
   */
  mpiPi_msg_debug
    ("AddrRangeMap::PatchRanges: resetting range from [0x%016llx,0x%016llx] to [0x%016llx,0x%016llx]\n",
     node->lowAddr, node->highAddr, node->lowAddr, (maxAddress - 1));
  node->highAddr = (maxAddress - 1);

  return ret;
}



static void
AddrToSourceMap_PatchRanges (void)
{
  assert (addrToSourceMap != NULL);
  if (addrToSourceMap->root != NULL)
    {
      /*
       * set the overall max address
       * we could do better using domain-specific information,
       * such as the max address in the text section if we 
       * knew we were looking at functions in the text section.
       */
      Dwarf_Addr maxAddress = ~(Dwarf_Addr) 0;

      /* patch the address ranges in our map */
      AddrToSourceMap_PatchRangesAux (addrToSourceMap->root, maxAddress);
    }
}





static const struct AddrToSourceInfo *
    AddrToSourceMap_Find (void *addr)
{
  assert (addrToSourceMap != NULL);
  return (const struct AddrToSourceInfo *) AddrRangeMap_Find (addrToSourceMap,
                                                              addr);
}



/*----------------------------------------------------------------------------
 * 
 *
 */

struct FunctionInfo
{
  char *name;
};

static struct AddrRangeMap *functionMap = NULL;


static void
FunctionMap_Init (void)
{
  assert (functionMap == NULL);
  functionMap = (struct AddrRangeMap *) malloc (sizeof (struct AddrRangeMap));
  if (functionMap == NULL)
    {
      mpiPi_abort ("malloc failed\n");
    }
  AddrRangeMap_Init (functionMap);
}

static void
FunctionInfo_Destroy (void *node)
{
  struct FunctionInfo *fi = (struct FunctionInfo *) node;
  assert (fi != NULL);
  free (fi->name);
  free (fi);
}


static void
FunctionMap_Destroy (void)
{
  if (functionMap != NULL)
    {
      AddrRangeMap_Destroy (functionMap, FunctionInfo_Destroy);
      functionMap = NULL;
    }
}


void
FunctionMap_Add (const char *funcName,
                 Dwarf_Addr lowAddress, Dwarf_Addr highAddress)
{
  /* build a new entry for this function */
  struct FunctionInfo *newEntry =
    (struct FunctionInfo *) malloc (sizeof (struct FunctionInfo));
  if (newEntry == NULL)
    {
      mpiPi_abort ("malloc failed\n");
    }

  mpiPi_msg_debug ("FunctionMap::Add: %s [0x%016llx,0x%016llx]\n",
                   funcName, lowAddress, highAddress);

  newEntry->name = strdup (funcName);
  if (newEntry->name == NULL)
    {
      mpiPi_abort ("malloc failed\n");
    }

  /* add the function to our ordered collection of functions */
  assert (functionMap != NULL);
  AddrRangeMap_Add (functionMap, lowAddress, highAddress, newEntry);
}





static const struct FunctionInfo *
FunctionMap_Find (void *addr)
{
  assert (functionMap != NULL);
  return (const struct FunctionInfo *) AddrRangeMap_Find (functionMap, addr);
}




/*----------------------------------------------------------------------------
 * DWARF source lookup functions
 *
 */
static Dwarf_Debug dwHandle = NULL;	/* DWARF library session handle */
static int dwFd = -1;		/* file descriptor for executable */


static void
HandleFunctionDIE (Dwarf_Debug dwHandle, Dwarf_Die currChildDIE)
{
  char *funcName = NULL;
  Dwarf_Addr lowAddress = 0;
  Dwarf_Addr highAddress = 0;

  int dwDieNameRet, dwDieLowAddrRet, dwDieHighAddrRet;

  dwDieNameRet = dwarf_diename (currChildDIE,
                                &funcName,
                                &dw_err);
  if (dwDieNameRet != DW_DLV_OK)
    mpiPi_msg_debug("Failed to get DIE name : %s\n", dwarf_errmsg(dw_err));

  dwDieLowAddrRet = dwarf_lowpc (currChildDIE,
                                 &lowAddress,
                                 &dw_err);
  if (dwDieLowAddrRet != DW_DLV_OK)
    mpiPi_msg_debug("Failed to get low PC : %s\n", dwarf_errmsg(dw_err));

  dwDieHighAddrRet = dwarf_highpc (currChildDIE,
                                   &highAddress,
                                   &dw_err);

  if (dwDieHighAddrRet != DW_DLV_OK)
    mpiPi_msg_debug("Failed to get high PC : %s\n", dwarf_errmsg(dw_err));

  if ((dwDieNameRet == DW_DLV_OK) &&
      (dwDieLowAddrRet == DW_DLV_OK) && (dwDieHighAddrRet == DW_DLV_OK))
    {
      FunctionMap_Add (funcName, lowAddress, highAddress);
    }

  if ((dwDieNameRet == DW_DLV_OK) && (funcName != NULL))
    {
      dwarf_dealloc (dwHandle, funcName, DW_DLA_STRING);
    }
}



int
open_dwarf_executable (char *fileName)
{
  int dwStatus = -1;


  mpiPi_msg_debug ("enter open_dwarf_executable\n");
  if (fileName == NULL)
    {
      mpiPi_msg_warn ("Executable file name is NULL!\n");
      mpiPi_msg_warn
          ("If this is a Fortran application, you may be using the incorrect mpiP library.\n");
    }

  /* open the executable */
  assert (dwFd == -1);
  mpiPi_msg_debug ("opening file %s\n", fileName);
  dwFd = open (fileName, O_RDONLY);
  if (dwFd == -1)
    {
      mpiPi_msg_warn ("could not open file %s\n", fileName);
      return 0;
    }

  /* initialize the DWARF library */
  assert (dwHandle == NULL);
  dwStatus = dwarf_init (dwFd,	/* exe file descriptor */
                         DW_DLC_READ,	/* desired access */
                         NULL,	/* error handler */
                         NULL,	/* error argument */
                         &dwHandle,	/* session handle */
                         &dw_err);	/* error object */
  if (dwStatus == DW_DLV_ERROR)
    {
      close (dwFd);
      dwFd = -1;
      mpiPi_abort ("could not initialize DWARF library : %s\n", dwarf_errmsg(dw_err));
    }

  if (dwStatus == DW_DLV_NO_ENTRY)
    {
      mpiPi_msg_warn ("No symbols in the executable\n");
      return 0;
    }

  /* initialize our function and addr-to-source mappings */
  AddrToSourceMap_Init ();
  FunctionMap_Init ();

  /* access symbol info */
  while (1)
    {
      Dwarf_Unsigned nextCompilationUnitHeaderOffset = 0;
      Dwarf_Die currCompileUnitDIE = NULL;
      Dwarf_Line *lineEntries = NULL;
      Dwarf_Signed nLineEntries = 0;
      Dwarf_Half currDIETag;
      Dwarf_Die currChildDIE = NULL;
      Dwarf_Die nextChildDIE = NULL;
      Dwarf_Die oldChildDIE = NULL;


      /* access next compilation unit header */
      dwStatus = dwarf_next_cu_header (dwHandle, NULL,	/* cu_header_length */
                                       NULL,	/* version_stamp */
                                       NULL,	/* abbrev_offset */
                                       NULL,	/* address_size */
                                       &nextCompilationUnitHeaderOffset, &dw_err);	/* error object */
      if (dwStatus != DW_DLV_OK)
        {
          if (dwStatus != DW_DLV_NO_ENTRY)
            {
              mpiPi_abort ("failed to access next DWARF cu header : %s\n", dwarf_errmsg(dw_err));
            }
          break;
        }

      /* access the first debug info entry (DIE) for this computation unit */
      dwStatus = dwarf_siblingof (dwHandle, NULL,	/* current DIE */
                                  &currCompileUnitDIE,	/* sibling DIE */
                                  &dw_err);	/* error object */
      if (dwStatus != DW_DLV_OK)
        {
          mpiPi_abort ("failed to access first DWARF DIE : %s\n", dwarf_errmsg(dw_err));
        }

      /* get line number information for this compilation 
       * unit, if available 
       */
      dwStatus = dwarf_srclines (currCompileUnitDIE,
                                 &lineEntries, &nLineEntries, &dw_err);
      if (dwStatus == DW_DLV_OK)
        {
          unsigned int i;


          /*
           * Extract and save address-to-source line mapping.
           *
           * NOTE: At least on the Cray X1, we see line number
           * information with the same address mapping to different lines.
           * It seems like when there are multiple entries for a given
           * address, the last one is the correct one.  (Needs verification.)
           * If we see multiple entries for a given address, we only
           * save the last one.
           */
          for (i = 0; i < nLineEntries; i++)
            {
              Dwarf_Unsigned lineNumber = 0;
              Dwarf_Addr lineAddress = 0;
              char *lineSourceFile = NULL;


              int lineNoStatus, lineAddrStatus, lineSrcFileStatus;

              lineNoStatus = dwarf_lineno (lineEntries[i],
                                           &lineNumber,
                                           &dw_err);

              if (lineNoStatus != DW_DLV_OK)
                mpiPi_msg_debug("Failed to get line number : %s\n", dwarf_errmsg(dw_err));

              lineAddrStatus = dwarf_lineaddr (lineEntries[i],
                                               &lineAddress,
                                               &dw_err);

              if (lineAddrStatus != DW_DLV_OK)
                mpiPi_msg_debug("Failed to get line address : %s\n", dwarf_errmsg(dw_err));

              lineSrcFileStatus = dwarf_linesrc (lineEntries[i],
                                                 &lineSourceFile,
                                                 &dw_err);

              if (lineSrcFileStatus != DW_DLV_OK)
                mpiPi_msg_debug("Failed to get source file status : %s\n", dwarf_errmsg(dw_err));

              if ((lineNoStatus == DW_DLV_OK) &&
                  (lineAddrStatus == DW_DLV_OK)
                  && (lineSrcFileStatus == DW_DLV_OK))
                {
                  int saveCurrentEntry = 0;	/* bool */

                  if (i < (nLineEntries - 1))
                    {
                      /*
                       * We're not on the last line number entry -
                       * check the address associated with the next
                       * entry to see if it differs from this one.
                       * Only save the entry if it the next address
                       * differs.
                       */
                      Dwarf_Addr nextLineAddress = 0;
                      int nextLineAddrStatus =
                          dwarf_lineaddr (lineEntries[i + 1],
                          &nextLineAddress,
                          &dw_err);
                      assert (nextLineAddrStatus == DW_DLV_OK);
                      if (nextLineAddress != lineAddress)
                        {
                          saveCurrentEntry = 1;
                        }
                    }
                  else
                    {
                      /* we're on the last line number entry */
                      saveCurrentEntry = 1;
                    }

                  if (saveCurrentEntry)
                    {
                      /* save the mapping entry */
                      AddrToSourceMap_Add (lineAddress, lineSourceFile,
                                           lineNumber);
                    }
                }

              if (lineSourceFile != NULL)
                {
                  dwarf_dealloc (dwHandle, lineSourceFile, DW_DLA_STRING);
                }
            }

          /* release the line number info */
          for (i = 0; i < nLineEntries; i++)
            {
              dwarf_dealloc (dwHandle, lineEntries[i], DW_DLA_LINE);
            }
          dwarf_dealloc (dwHandle, lineEntries, DW_DLA_LIST);
        }
      else if (dwStatus != DW_DLV_ERROR)
        {
      /*
       * no line information for the current DIE -
       * not an error, just unfortunate
       */
        }
      else
        {
          mpiPi_abort ("failed to obtain line info for the current DIE : %s\n", dwarf_errmsg(dw_err));
        }

      /* discover function information for the current compilation unit */
      /* 
       * NOTE: DWARF debug information entries can be organized in 
       * a hierarchy.  However, we presume that the function entries are
       * all children of the compilation unit DIE.
       */
      dwStatus = dwarf_tag (currCompileUnitDIE, &currDIETag, &dw_err);
      assert ((dwStatus == DW_DLV_OK) && (currDIETag == DW_TAG_compile_unit));

      /* access the first child DIE of the compile unit DIE */
      dwStatus = dwarf_child (currCompileUnitDIE, &currChildDIE, &dw_err);
      if (dwStatus == DW_DLV_NO_ENTRY)
        {
          // On some Cray systems, executables are linked with assembly compile units
          // with no functions.
          // mpiPi_abort ("no child DIEs of compile unit DIE\n");
        }
      else if (dwStatus != DW_DLV_OK)
        {
          mpiPi_abort
              ("failed to access first child DIE of compile unit DIE\n");
        }

      while (dwStatus == DW_DLV_OK)
        {
          /* determine the type of the child DIE */
          dwStatus = dwarf_tag (currChildDIE, &currDIETag, &dw_err);
          if (dwStatus == DW_DLV_OK)
            {
              if ((currDIETag == DW_TAG_subprogram) ||
                  (currDIETag == DW_TAG_entry_point))
                {
                  HandleFunctionDIE (dwHandle, currChildDIE);
                }
            }
          else
            {
              mpiPi_abort ("unable to determine tag of current child DIE : %s \n", dwarf_errmsg(dw_err));
            }

          /* advance to the next child DIE */
          oldChildDIE = currChildDIE;
          dwStatus = dwarf_siblingof (dwHandle,
                                      currChildDIE, &nextChildDIE, &dw_err);
          if (dwStatus == DW_DLV_OK)
            {
              currChildDIE = nextChildDIE;
              nextChildDIE = NULL;
            }
          else if (dwStatus != DW_DLV_NO_ENTRY)
            {
              mpiPi_abort
                  ("unable to access next child DIE of current compilation unit : %s\n", dwarf_errmsg(dw_err));
            }

          if (oldChildDIE != NULL)
            {
              dwarf_dealloc (dwHandle, oldChildDIE, DW_DLA_DIE);
            }
        }

      /* release the current compilation unit DIE */
      dwarf_dealloc (dwHandle, currCompileUnitDIE, DW_DLA_DIE);
    }

  /*
   * DWARF address-to-source line information does not give
   * address ranges, so we need to patch the address ranges
   * in our address-to-source map.
   */
  AddrToSourceMap_PatchRanges ();

  return 1;
}


void
close_dwarf_executable (void)
{
  int dwStatus = -1;
  Elf *elfHandle = NULL;

  mpiPi_msg_debug ("enter close_dwarf_executable\n");

  assert (dwHandle != NULL);
  assert (dwFd != -1);
  dwStatus = dwarf_get_elf (dwHandle, &elfHandle, &dw_err);
  if (dwStatus != DW_DLV_OK)
    {
      mpiPi_msg_debug ("dwarf_get_elf failed; ignoring : %s\n", dwarf_errmsg(dw_err));
    }

  dwStatus = dwarf_finish (dwHandle, &dw_err);
  if (dwStatus != DW_DLV_OK)
    {
      mpiPi_msg_debug ("dwarf_finish failed; ignoring : %s\n", dwarf_errmsg(dw_err));
    }
  dwHandle = NULL;

  close (dwFd);
  dwFd = -1;

  AddrToSourceMap_Destroy ();
  FunctionMap_Destroy ();
}


int
mpiP_find_src_loc (void *i_addr_hex,
		   char **o_file_str, int *o_lineno, char **o_funct_str)
{
  const struct AddrToSourceInfo *addrToSrcMapping = NULL;
  const struct FunctionInfo *functionInfo = NULL;
  char addr_buf[24];


  /* initialize the out parameters */
  assert (o_file_str != NULL);
  assert (o_lineno != NULL);
  assert (o_funct_str != NULL);

  *o_file_str = NULL;
  *o_lineno = 0;
  *o_funct_str = NULL;

  /* determine if we have source line info for this address */
  addrToSrcMapping = AddrToSourceMap_Find (i_addr_hex);
  if (addrToSrcMapping != NULL)
    {

      *o_file_str = addrToSrcMapping->fileName;
      *o_lineno = addrToSrcMapping->lineNumber;

      if (mpiPi.baseNames == 0 && *o_file_str != NULL)
        {
          char *h;
          h = strrchr (*o_file_str, '/');
          if (h != NULL)
            *o_file_str = h + 1;
        }
    }
  else
    {
      mpiPi_msg_warn ("unable to find source line info for address 0x%p\n",
                      i_addr_hex);
      /*
       * the rest of the mpiP code seems to expect that the filename
       * will be set to "??" if we are unable to associate it with a file
       */
      *o_file_str = "??";
    }

  /* determine if we have function info for this address */
  functionInfo = FunctionMap_Find (i_addr_hex);
  if (functionInfo != NULL)
    {
      *o_funct_str = functionInfo->name;
    }
  else
    {
      mpiPi_msg_warn ("unable to find function info for address %s\n",
                      mpiP_format_address (i_addr_hex, addr_buf));
    }

  return (((addrToSrcMapping != NULL) || (functionInfo != NULL)) ? 0 : 1);
}




#endif /* defined(USE_LIBDWARF) */


/* 

<license>

Copyright (c) 2006, The Regents of the University of California. 
Produced at the Lawrence Livermore National Laboratory 
Written by Jeffery Vetter and Christopher Chambreau. 
UCRL-CODE-223450. 
All rights reserved. 
 
This file is part of mpiP.  For details, see http://llnl.github.io/mpiP. 
 
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:
 
* Redistributions of source code must retain the above copyright
notice, this list of conditions and the disclaimer below.

* Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the disclaimer (as noted below) in
the documentation and/or other materials provided with the
distribution.

* Neither the name of the UC/LLNL nor the names of its contributors
may be used to endorse or promote products derived from this software
without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OF
THE UNIVERSITY OF CALIFORNIA, THE U.S. DEPARTMENT OF ENERGY OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 
 
Additional BSD Notice 
 
1. This notice is required to be provided under our contract with the
U.S. Department of Energy (DOE).  This work was produced at the
University of California, Lawrence Livermore National Laboratory under
Contract No. W-7405-ENG-48 with the DOE.
 
2. Neither the United States Government nor the University of
California nor any of their employees, makes any warranty, express or
implied, or assumes any liability or responsibility for the accuracy,
completeness, or usefulness of any information, apparatus, product, or
process disclosed, or represents that its use would not infringe
privately-owned rights.
 
3.  Also, reference herein to any specific commercial products,
process, or services by trade name, trademark, manufacturer or
otherwise does not necessarily constitute or imply its endorsement,
recommendation, or favoring by the United States Government or the
University of California.  The views and opinions of authors expressed
herein do not necessarily state or reflect those of the United States
Government or the University of California, and shall not be used for
advertising or product endorsement purposes.

</license>

*/



/* eof */
