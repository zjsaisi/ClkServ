/*************************************************************************
**    (C) 2008 - 2009 IXXAT Automation GmbH, all rights reserved
**************************************************************************
**
**       File: SISmpool.c
**    Summary: SIS Memory Pool Management.
**     Remark: !!! Auto generated source file, do not modify !!!
**
**************************************************************************
**************************************************************************
**
**  Functions: SIS_MemPoolInit
**             SIS_Alloc
**             SIS_AllocHdl
**             SIS_Free
**
**************************************************************************
**    all rights reserved
*************************************************************************/

/*
  memory pool list:
  k_POOL_64: 21 entries, each 64 bytes large
  k_POOL_128: 14 entries, each 128 bytes large
  k_POOL_256: 12 entries, each 256 bytes large
  k_POOL_1536: 2 entries, each 1536 bytes large
*/

/*************************************************************************
**    compiler directives
*************************************************************************/

/*************************************************************************
**    include-files
*************************************************************************/
#include "target.h"
#include "SIS.h"
#include "SISint.h"

/*************************************************************************
**    global variables
*************************************************************************/

/*************************************************************************
**    static constants, types, macros, variables
*************************************************************************/
#define k_MAX_FILE_STRLEN (15)


typedef struct{           /* definition of a memory pool buffer */
/* debug version */
#if( SIS_k_ALLOC_DEBUG_VERS == TRUE )
  BOOLEAN o_isAllc;
  CHAR    ac_allocFile[k_MAX_FILE_STRLEN];
  UINT32  dw_allocLine;
  CHAR    ac_freeFile[k_MAX_FILE_STRLEN];
  UINT32  dw_freeLine;
#endif
  UINT16  w_idx;           /* index to build a list of buffers */
  UINT16  w_hdl;           /* handle to mark each buffer */
  UINT32  adw_data[16]; /* real buffer */
}t_POOL_ENTRY0;

static t_POOL_ENTRY0 a_Pool0[21];
static UINT16        w_StartIdx0;
static UINT16        PoolNumEntries0;

typedef struct{           /* definition of a memory pool buffer */
/* debug version */
#if( SIS_k_ALLOC_DEBUG_VERS == TRUE )
  BOOLEAN o_isAllc;
  CHAR    ac_allocFile[k_MAX_FILE_STRLEN];
  UINT32  dw_allocLine;
  CHAR    ac_freeFile[k_MAX_FILE_STRLEN];
  UINT32  dw_freeLine;
#endif
  UINT16  w_idx;           /* index to build a list of buffers */
  UINT16  w_hdl;           /* handle to mark each buffer */
  UINT32  adw_data[32]; /* real buffer */
}t_POOL_ENTRY1;

static t_POOL_ENTRY1 a_Pool1[14];
static UINT16        w_StartIdx1;
static UINT16        PoolNumEntries1;

typedef struct{           /* definition of a memory pool buffer */
/* debug version */
#if( SIS_k_ALLOC_DEBUG_VERS == TRUE )
  BOOLEAN o_isAllc;
  CHAR    ac_allocFile[k_MAX_FILE_STRLEN];
  UINT32  dw_allocLine;
  CHAR    ac_freeFile[k_MAX_FILE_STRLEN];
  UINT32  dw_freeLine;
#endif
  UINT16  w_idx;           /* index to build a list of buffers */
  UINT16  w_hdl;           /* handle to mark each buffer */
  UINT32  adw_data[64]; /* real buffer */
}t_POOL_ENTRY2;

static t_POOL_ENTRY2 a_Pool2[12];
static UINT16        w_StartIdx2;
static UINT16        PoolNumEntries2;

typedef struct{           /* definition of a memory pool buffer */
/* debug version */
#if( SIS_k_ALLOC_DEBUG_VERS == TRUE )
  BOOLEAN o_isAllc;
  CHAR    ac_allocFile[k_MAX_FILE_STRLEN];
  UINT32  dw_allocLine;
  CHAR    ac_freeFile[k_MAX_FILE_STRLEN];
  UINT32  dw_freeLine;
#endif
  UINT16  w_idx;           /* index to build a list of buffers */
  UINT16  w_hdl;           /* handle to mark each buffer */
  UINT32  adw_data[384]; /* real buffer */
}t_POOL_ENTRY3;

static t_POOL_ENTRY3 a_Pool3[2];
static UINT16        w_StartIdx3;
static UINT16        PoolNumEntries3;

/*************************************************************************
**    static function-prototypes
*************************************************************************/
#if( SIS_k_ALLOC_DEBUG_VERS == TRUE )
static void CopyFileNameFromPath(CHAR* pc_fileName,const CHAR* pc_filePath);
#endif
/*************************************************************************
**    global functions
*************************************************************************/
/*************************************************************************
**
**  Function    : SIS_MemPoolInit
**
**  Description : Init the linked list of each memory pool
**
**  Parameters  : -
**
**  Returnvalue : -
**
**  Remarks     : function is not reentrant
**
*************************************************************************/
void SIS_MemPoolInit(void)
{
  UINT16  i;

  /* init pool k_POOL_64 */
  for(i=0;i<21;i++)
  {
    a_Pool0[i].w_idx    = i+1;      /* link to a list */
    a_Pool0[i].w_hdl    = k_POOL_64;  /* mark buffer with pool handle */
#if( SIS_k_ALLOC_DEBUG_VERS == TRUE )
    a_Pool0[i].o_isAllc = FALSE;    /* mark buffer unallocated */
#endif
  }
  a_Pool0[21-1].w_idx = SIS_NIL;  /* end of list */
  w_StartIdx0 = 0;
  PoolNumEntries0 = 21;

  /* init pool k_POOL_128 */
  for(i=0;i<14;i++)
  {
    a_Pool1[i].w_idx    = i+1;      /* link to a list */
    a_Pool1[i].w_hdl    = k_POOL_128;  /* mark buffer with pool handle */
#if( SIS_k_ALLOC_DEBUG_VERS == TRUE )
    a_Pool1[i].o_isAllc = FALSE;    /* mark buffer unallocated */
#endif
  }
  a_Pool1[14-1].w_idx = SIS_NIL;  /* end of list */
  w_StartIdx1 = 0;
  PoolNumEntries1 = 14;

  /* init pool k_POOL_256 */
  for(i=0;i<12;i++)
  {
    a_Pool2[i].w_idx    = i+1;      /* link to a list */
    a_Pool2[i].w_hdl    = k_POOL_256;  /* mark buffer with pool handle */
#if( SIS_k_ALLOC_DEBUG_VERS == TRUE )
    a_Pool2[i].o_isAllc = FALSE;    /* mark buffer unallocated */
#endif
  }
  a_Pool2[12-1].w_idx = SIS_NIL;  /* end of list */
  w_StartIdx2 = 0;
  PoolNumEntries2 = 12;

  /* init pool k_POOL_1536 */
  for(i=0;i<2;i++)
  {
    a_Pool3[i].w_idx    = i+1;      /* link to a list */
    a_Pool3[i].w_hdl    = k_POOL_1536;  /* mark buffer with pool handle */
#if( SIS_k_ALLOC_DEBUG_VERS == TRUE )
    a_Pool3[i].o_isAllc = FALSE;    /* mark buffer unallocated */
#endif
  }
  a_Pool3[2-1].w_idx = SIS_NIL;  /* end of list */
  w_StartIdx3 = 0;
  PoolNumEntries3 = 2;
}

#if( SIS_k_ALLOC_DEBUG_VERS == TRUE )
/*************************************************************************
**
** Function    : SIS_AllocDbg
**
** Description : Function allocates a buffer with the given size.
**               The function returns a pointer to a buffer from ther pool,
**               which fits mostly to the given size.
**               (buffer_size >= given_size)
**               If the requested buffer size is a constant value, it is
**               recommended to use the more efficient function
**               SIS_AllocHdl().
**               To free the given buffer, SIS_Free() must be called.
**
**               This is the debug version of SIS_Alloc and stores the
**               file and line where it is called.
**
** See Also    : SIS_Free(), SIS_AllocHdlDbg()
**
** Parameters  : w_buf_size (IN) - memory block size in bytes (1..65535)
**               pc_file    (IN) - string contains file name of function call
**               dw_line    (IN) - contains line of function call in file
**
** Returnvalue : pointer to the allocated memory buffer or
**               NULL if no buffer is available.
**
** Remarks     : function is reentrant
**
*************************************************************************/
void* SIS_AllocDbg(UINT16 w_buf_size,const CHAR *pc_file,UINT32 dw_line)
{
  if(w_buf_size <= 64)
  {
    return SIS_AllocHdlDbg(k_POOL_64,pc_file,dw_line);
  }
  if(w_buf_size <= 128)
  {
    return SIS_AllocHdlDbg(k_POOL_128,pc_file,dw_line);
  }
  if(w_buf_size <= 256)
  {
    return SIS_AllocHdlDbg(k_POOL_256,pc_file,dw_line);
  }
  if(w_buf_size <= 1536)
  {
    return SIS_AllocHdlDbg(k_POOL_1536,pc_file,dw_line);
  }
  return NULL;
}

/*************************************************************************
**
**  Function    : SIS_AllocHdlDbg
**
**  Description : Function allocates a buffer from the a memory pool.
**                The memory pool is specified by means of the given
**                pool handle. See "SIShdls.h" for possible pool handles.
**                The function returns a pointer to a buffer.
**                To free the given buffer, SIS_Free() must be called.
**
**                This is the debug version of SIS_AllocHdl and stores the
**                file and line where it is called.
**
**  Parameters  : w_pool_hdl (IN)  - pool handle
**                pc_file    (IN) - string contains file name of function call
**                dw_line    (IN) - contains line of function call in file
**
**  Returnvalue : pointer to the allocated memory buffer or
**                NULL if no buffer is available.
**
**  Remarks     : function is reentrant
**
*************************************************************************/
void* SIS_AllocHdlDbg(UINT16 w_pool_hdl,const CHAR *pc_file,UINT32 dw_line)
{
  UINT16 w_idx;

  switch(w_pool_hdl)
  {
    case k_POOL_64:
    {
      /* get memory buffer from the pool */
      if(PoolNumEntries0)
      {
        w_idx = w_StartIdx0;
        w_StartIdx0 = a_Pool0[w_idx].w_idx;
        a_Pool0[w_idx].w_hdl = k_POOL_64;
        a_Pool0[w_idx].w_idx = w_idx;
        /* store filename and line in file where it got called */
        CopyFileNameFromPath(a_Pool0[w_idx].ac_allocFile,pc_file);
        a_Pool0[w_idx].dw_allocLine = dw_line;
        a_Pool0[w_idx].o_isAllc     = TRUE;
        /* erase free-data */
        SIS_MEMSET(a_Pool0[w_idx].ac_freeFile,'\0',k_MAX_FILE_STRLEN);
        a_Pool0[w_idx].dw_freeLine = 0;
        PoolNumEntries0--;
        return a_Pool0[w_idx].adw_data;
      }
      return NULL;
    }
    case k_POOL_128:
    {
      /* get memory buffer from the pool */
      if(PoolNumEntries1)
      {
        w_idx = w_StartIdx1;
        w_StartIdx1 = a_Pool1[w_idx].w_idx;
        a_Pool1[w_idx].w_hdl = k_POOL_128;
        a_Pool1[w_idx].w_idx = w_idx;
        /* store filename and line in file where it got called */
        CopyFileNameFromPath(a_Pool1[w_idx].ac_allocFile,pc_file);
        a_Pool1[w_idx].dw_allocLine = dw_line;
        a_Pool1[w_idx].o_isAllc     = TRUE;
        /* erase free-data */
        SIS_MEMSET(a_Pool1[w_idx].ac_freeFile,'\0',k_MAX_FILE_STRLEN);
        a_Pool1[w_idx].dw_freeLine = 0;
        PoolNumEntries1--;
        return a_Pool1[w_idx].adw_data;
      }
      return NULL;
    }
    case k_POOL_256:
    {
      /* get memory buffer from the pool */
      if(PoolNumEntries2)
      {
        w_idx = w_StartIdx2;
        w_StartIdx2 = a_Pool2[w_idx].w_idx;
        a_Pool2[w_idx].w_hdl = k_POOL_256;
        a_Pool2[w_idx].w_idx = w_idx;
        /* store filename and line in file where it got called */
        CopyFileNameFromPath(a_Pool2[w_idx].ac_allocFile,pc_file);
        a_Pool2[w_idx].dw_allocLine = dw_line;
        a_Pool2[w_idx].o_isAllc     = TRUE;
        /* erase free-data */
        SIS_MEMSET(a_Pool2[w_idx].ac_freeFile,'\0',k_MAX_FILE_STRLEN);
        a_Pool2[w_idx].dw_freeLine = 0;
        PoolNumEntries2--;
        return a_Pool2[w_idx].adw_data;
      }
      return NULL;
    }
    case k_POOL_1536:
    {
      /* get memory buffer from the pool */
      if(PoolNumEntries3)
      {
        w_idx = w_StartIdx3;
        w_StartIdx3 = a_Pool3[w_idx].w_idx;
        a_Pool3[w_idx].w_hdl = k_POOL_1536;
        a_Pool3[w_idx].w_idx = w_idx;
        /* store filename and line in file where it got called */
        CopyFileNameFromPath(a_Pool3[w_idx].ac_allocFile,pc_file);
        a_Pool3[w_idx].dw_allocLine = dw_line;
        a_Pool3[w_idx].o_isAllc     = TRUE;
        /* erase free-data */
        SIS_MEMSET(a_Pool3[w_idx].ac_freeFile,'\0',k_MAX_FILE_STRLEN);
        a_Pool3[w_idx].dw_freeLine = 0;
        PoolNumEntries3--;
        return a_Pool3[w_idx].adw_data;
      }
      return NULL;
    }
    default:
    {
      assert(0);
      break;
    }
  }
  return NULL;/*lint !e527 */
}

/*************************************************************************
**
**  Function    : SIS_FreeDbg
**
**  Description : Function frees a previously allocated memory buffer.
**
**                This is the debug version of SIS_Free and stores the
**                file and line where it is called.
**
**  Parameters  : pv_buff (IN) - pointer to the memory buffer
**                pc_file (IN) - string contains file name of function call
**                dw_line (IN) - contains line of function call in file
**
**  Returnvalue : -
**
**  Remarks     : function is reentrant
**
*************************************************************************/
void SIS_FreeDbg(const void *pv_buff,const CHAR *pc_file,UINT32 dw_line)
{
  UINT16 w_hdl;
  UINT16 w_idx;

  assert(pv_buff != 0);
  w_hdl = ((UINT16*)pv_buff)[-1]; /* pool handle */
  w_idx = ((UINT16*)pv_buff)[-2]; /* pool index for pointer checking */

  switch(w_hdl)
  {
    case k_POOL_64:
    {
      assert(pv_buff == a_Pool0[w_idx].adw_data); /* check buffer pointer */
      /* free buffer block */
      a_Pool0[w_idx].w_idx = w_StartIdx0;
      a_Pool0[w_idx].w_hdl = SIS_NIL; /* mark as free */
      /* store filename and line in file where it got called */
      CopyFileNameFromPath(a_Pool0[w_idx].ac_freeFile,pc_file);
      a_Pool0[w_idx].dw_freeLine = dw_line;
      a_Pool0[w_idx].o_isAllc    = FALSE;
      w_StartIdx0 = w_idx;
      PoolNumEntries0++;
      break;
    }
    case k_POOL_128:
    {
      assert(pv_buff == a_Pool1[w_idx].adw_data); /* check buffer pointer */
      /* free buffer block */
      a_Pool1[w_idx].w_idx = w_StartIdx1;
      a_Pool1[w_idx].w_hdl = SIS_NIL; /* mark as free */
      /* store filename and line in file where it got called */
      CopyFileNameFromPath(a_Pool1[w_idx].ac_freeFile,pc_file);
      a_Pool1[w_idx].dw_freeLine = dw_line;
      a_Pool1[w_idx].o_isAllc    = FALSE;
      w_StartIdx1 = w_idx;
      PoolNumEntries1++;
      break;
    }
    case k_POOL_256:
    {
      assert(pv_buff == a_Pool2[w_idx].adw_data); /* check buffer pointer */
      /* free buffer block */
      a_Pool2[w_idx].w_idx = w_StartIdx2;
      a_Pool2[w_idx].w_hdl = SIS_NIL; /* mark as free */
      /* store filename and line in file where it got called */
      CopyFileNameFromPath(a_Pool2[w_idx].ac_freeFile,pc_file);
      a_Pool2[w_idx].dw_freeLine = dw_line;
      a_Pool2[w_idx].o_isAllc    = FALSE;
      w_StartIdx2 = w_idx;
      PoolNumEntries2++;
      break;
    }
    case k_POOL_1536:
    {
      assert(pv_buff == a_Pool3[w_idx].adw_data); /* check buffer pointer */
      /* free buffer block */
      a_Pool3[w_idx].w_idx = w_StartIdx3;
      a_Pool3[w_idx].w_hdl = SIS_NIL; /* mark as free */
      /* store filename and line in file where it got called */
      CopyFileNameFromPath(a_Pool3[w_idx].ac_freeFile,pc_file);
      a_Pool3[w_idx].dw_freeLine = dw_line;
      a_Pool3[w_idx].o_isAllc    = FALSE;
      w_StartIdx3 = w_idx;
      PoolNumEntries3++;
      break;
    }
    default:
    {
      if(w_hdl == SIS_NIL)  /* buffer was already free */
      {
        assert(w_hdl != SIS_NIL);
      }
      else
      {
        assert(0);    /* corrupted buffer pointer */
      }
      break;
    }
  }
}
#else /* #if( SIS_k_ALLOC_DEBUG_VERS == TRUE ) */

/*************************************************************************
**
** Function    : SIS_Alloc
**
** Description : Function allocates a buffer with the given size.
**               The function returns a pointer to a buffer from ther pool,
**               which fits mostly to the given size.
**               (buffer_size >= given_size)
**               If the requested buffer size is a constant value, it is
**               recommended to use the more efficient function
**               SIS_AllocHdl().
**               To free the given buffer, SIS_Free() must be called.
**
** See Also    : SIS_Free(), SIS_AllocHdl()
**
** Parameters  : w_buf_size (IN)  - memory block size in bytes (1..65535)
**
** Returnvalue : pointer to the allocated memory buffer or
**               NULL if no buffer is available.
**
** Remarks     : function is reentrant
**
*************************************************************************/
void* SIS_Alloc(UINT16 w_buf_size)
{
  if(w_buf_size <= 64)
  {
    return SIS_AllocHdl(k_POOL_64);
  }
  if(w_buf_size <= 128)
  {
    return SIS_AllocHdl(k_POOL_128);
  }
  if(w_buf_size <= 256)
  {
    return SIS_AllocHdl(k_POOL_256);
  }
  if(w_buf_size <= 1536)
  {
    return SIS_AllocHdl(k_POOL_1536);
  }
  return 0;
}

/*************************************************************************
**
**  Function    : SIS_AllocHdl
**
**  Description : Function allocates a buffer from the a memory pool.
**                The memory pool is specified by means of the given
**                pool handle. See "SIShdls.h" for possible pool handles.
**                The function returns a pointer to a buffer.
**                To free the given buffer, SIS_Free() must be called.
**
**  Parameters  : w_pool_hdl (IN)  - pool handle
**
**  Returnvalue : pointer to the allocated memory buffer or
**                NULL if no buffer is available.
**
**  Remarks     : function is reentrant
**
*************************************************************************/
void *SIS_AllocHdl(UINT16 w_pool_hdl)
{
  UINT16 w_idx;

  switch(w_pool_hdl)
  {
    case k_POOL_64:
    {
      /* get memory buffer from the pool */
      if(PoolNumEntries0)
      {
        w_idx = w_StartIdx0;
        w_StartIdx0 = a_Pool0[w_idx].w_idx;
        a_Pool0[w_idx].w_hdl = k_POOL_64;
        a_Pool0[w_idx].w_idx = w_idx;
        PoolNumEntries0--;
        return a_Pool0[w_idx].adw_data;
      }
      return NULL;
    }
    case k_POOL_128:
    {
      /* get memory buffer from the pool */
      if(PoolNumEntries1)
      {
        w_idx = w_StartIdx1;
        w_StartIdx1 = a_Pool1[w_idx].w_idx;
        a_Pool1[w_idx].w_hdl = k_POOL_128;
        a_Pool1[w_idx].w_idx = w_idx;
        PoolNumEntries1--;
        return a_Pool1[w_idx].adw_data;
      }
      return NULL;
    }
    case k_POOL_256:
    {
      /* get memory buffer from the pool */
      if(PoolNumEntries2)
      {
        w_idx = w_StartIdx2;
        w_StartIdx2 = a_Pool2[w_idx].w_idx;
        a_Pool2[w_idx].w_hdl = k_POOL_256;
        a_Pool2[w_idx].w_idx = w_idx;
        PoolNumEntries2--;
        return a_Pool2[w_idx].adw_data;
      }
      return NULL;
    }
    case k_POOL_1536:
    {
      /* get memory buffer from the pool */
      if(PoolNumEntries3)
      {
        w_idx = w_StartIdx3;
        w_StartIdx3 = a_Pool3[w_idx].w_idx;
        a_Pool3[w_idx].w_hdl = k_POOL_1536;
        a_Pool3[w_idx].w_idx = w_idx;
        PoolNumEntries3--;
        return a_Pool3[w_idx].adw_data;
      }
      return NULL;
    }
    default:
    {
      assert(0);
      break;
    }
  }
  return NULL;/*lint !e527*/
}
/*************************************************************************
**
** Function    : SIS_Free
**
** Description : Function frees a previously allocated memory buffer.
**
** See Also    : SIS_Alloc(), SIS_AllocHdl()
**
** Parameters  : pv_buff (IN)  - pointer to the memory buffer
**
** Returnvalue : -
**
** Remarks     : function is reentrant
**
*************************************************************************/
void SIS_Free(const void *pv_buff)
{
  UINT16 w_hdl;
  UINT16 w_idx;

  assert(pv_buff != 0);
  w_hdl = ((UINT16*)pv_buff)[-1]; /* pool handle */
  w_idx = ((UINT16*)pv_buff)[-2]; /* pool index for pointer checking */

  switch(w_hdl)
  {
    case k_POOL_64:
    {
      assert(pv_buff == a_Pool0[w_idx].adw_data); /* check buffer pointer */
      /* free buffer block */
      a_Pool0[w_idx].w_idx = w_StartIdx0;
      a_Pool0[w_idx].w_hdl = SIS_NIL; /* mark as free */
      w_StartIdx0 = w_idx;
      PoolNumEntries0++;
      break;
    }
    case k_POOL_128:
    {
      assert(pv_buff == a_Pool1[w_idx].adw_data); /* check buffer pointer */
      /* free buffer block */
      a_Pool1[w_idx].w_idx = w_StartIdx1;
      a_Pool1[w_idx].w_hdl = SIS_NIL; /* mark as free */
      w_StartIdx1 = w_idx;
      PoolNumEntries1++;
      break;
    }
    case k_POOL_256:
    {
      assert(pv_buff == a_Pool2[w_idx].adw_data); /* check buffer pointer */
      /* free buffer block */
      a_Pool2[w_idx].w_idx = w_StartIdx2;
      a_Pool2[w_idx].w_hdl = SIS_NIL; /* mark as free */
      w_StartIdx2 = w_idx;
      PoolNumEntries2++;
      break;
    }
    case k_POOL_1536:
    {
      assert(pv_buff == a_Pool3[w_idx].adw_data); /* check buffer pointer */
      /* free buffer block */
      a_Pool3[w_idx].w_idx = w_StartIdx3;
      a_Pool3[w_idx].w_hdl = SIS_NIL; /* mark as free */
      w_StartIdx3 = w_idx;
      PoolNumEntries3++;
      break;
    }
    default:
    {
      if(w_hdl == SIS_NIL)  /* buffer was already free */
      {
        assert(w_hdl != SIS_NIL);
      }
      else
      {
        assert(0);    /* corrupted buffer pointer */
      }
      break;
    }
  }
}
#endif /* #if( SIS_k_ALLOC_DEBUG_VERS == TRUE ) */

/*************************************************************************
**
** Function    : SIS_MemPoolQuery
**
** Description : Function returns the available number of memory buffers
**               in the given pool.
**
** See Also    : SIS_Alloc(), SIS_AllocHdl()
**
** Parameters  : w_pool_hdl (IN)  - pool handle
**
** Returnvalue : number of available memory buffers or zero
**
** Remarks     : function is reentrant
**
*************************************************************************/
UINT16 SIS_MemPoolQuery(UINT16 w_pool_hdl)
{
  switch(w_pool_hdl)
  {
    case k_POOL_64:
    {
      return PoolNumEntries0;
    }
    case k_POOL_128:
    {
      return PoolNumEntries1;
    }
    case k_POOL_256:
    {
      return PoolNumEntries2;
    }
    case k_POOL_1536:
    {
      return PoolNumEntries3;
    }
    default:
    {
      assert(0);
      return 0;/*lint !e527*/
    }
  }
}

/*************************************************************************
**
** Function    : SIS_MemPoolDebugPrint
**
** Description : Only useful for debugging purposes!
**               Function emits all available memory buffer indexes
**               of the given pool to stdout.
**
** Parameters  : w_pool_hdl (IN)  - pool handle
**
** Returnvalue : -
**
** Remarks     : function is useful for debugging purposes.
**               function is NOT reentrant.
**
*************************************************************************/
void SIS_MemPoolDebugPrint(UINT16 w_pool_hdl)
{
  UINT16 w_idx;

  switch(w_pool_hdl)
  {
    case k_POOL_64:
    {
      printf("\n k_POOL_64 (%u):", PoolNumEntries0);
      w_idx = w_StartIdx0;
      while(a_Pool0[w_idx].w_idx != SIS_NIL)
      {
        printf(" ->%u", w_idx);
        w_idx = a_Pool0[w_idx].w_idx;
      }
      printf(" ->%u\n", w_idx);
      break;
    }
    case k_POOL_128:
    {
      printf("\n k_POOL_128 (%u):", PoolNumEntries1);
      w_idx = w_StartIdx1;
      while(a_Pool1[w_idx].w_idx != SIS_NIL)
      {
        printf(" ->%u", w_idx);
        w_idx = a_Pool1[w_idx].w_idx;
      }
      printf(" ->%u\n", w_idx);
      break;
    }
    case k_POOL_256:
    {
      printf("\n k_POOL_256 (%u):", PoolNumEntries2);
      w_idx = w_StartIdx2;
      while(a_Pool2[w_idx].w_idx != SIS_NIL)
      {
        printf(" ->%u", w_idx);
        w_idx = a_Pool2[w_idx].w_idx;
      }
      printf(" ->%u\n", w_idx);
      break;
    }
    case k_POOL_1536:
    {
      printf("\n k_POOL_1536 (%u):", PoolNumEntries3);
      w_idx = w_StartIdx3;
      while(a_Pool3[w_idx].w_idx != SIS_NIL)
      {
        printf(" ->%u", w_idx);
        w_idx = a_Pool3[w_idx].w_idx;
      }
      printf(" ->%u\n", w_idx);
      break;
    }
    default:
    {
      assert(0);
    }
  }
}

/*************************************************************************
**    static functions
*************************************************************************/

#if( SIS_k_ALLOC_DEBUG_VERS == TRUE )
/***********************************************************************
**
** Function    : CopyFileNameFromPath
**
** Description : Extracts file name from complete file path and copies
**               it in destination array.
**
** Parameters  : pc_fileName (OUT) - Extracted file name
**               pc_filePath (IN)  - File path
**
** Returnvalue : -
**
** Remarks     : -
**
***********************************************************************/
static void CopyFileNameFromPath(CHAR* pc_fileName,const CHAR* pc_filePath)
{
  UINT32 dw_strlen;
  UINT32 dw_lastPos = 0;
  UINT32 dw_i;
  UINT32 dw_pos;

  /* get string length */
  dw_strlen = (UINT32)strlen(pc_filePath);
  /* search for the last slash in filepath */
  for( dw_i = 0 ; dw_i < dw_strlen ; dw_i++ )
  {
    if( (pc_filePath[dw_i] == '\\') ||
        (pc_filePath[dw_i] == '/'))
    {
      dw_lastPos = dw_i;
    }
  }
  /* if slash found, copy after last slash */
  if( dw_lastPos < dw_strlen )
  {
    dw_i = dw_strlen - dw_lastPos;
    if( dw_i > k_MAX_FILE_STRLEN )
    {
      dw_i = k_MAX_FILE_STRLEN;
    }
    SIS_MEMCPY(pc_fileName,&pc_filePath[dw_lastPos+1],dw_i);
  }
  /* if no slash found, copy as much from the string as possible */
  else
  {
    if( k_MAX_FILE_STRLEN > dw_strlen )
    {
      dw_pos = 0;
      dw_i   = dw_strlen;
    }
    else
    {
      dw_pos = dw_strlen - k_MAX_FILE_STRLEN;
      dw_i   = k_MAX_FILE_STRLEN;
    }
    SIS_MEMCPY(pc_fileName,&pc_filePath[dw_pos],dw_i);
  }
}
#endif
