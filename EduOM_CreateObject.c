/******************************************************************************/
/*                                                                            */
/*    ODYSSEUS/EduCOSMOS Educational-Purpose Object Storage System            */
/*                                                                            */
/*    Developed by Professor Kyu-Young Whang et al.                           */
/*                                                                            */
/*    Database and Multimedia Laboratory                                      */
/*                                                                            */
/*    Computer Science Department and                                         */
/*    Advanced Information Technology Research Center (AITrc)                 */
/*    Korea Advanced Institute of Science and Technology (KAIST)              */
/*                                                                            */
/*    e-mail: kywhang@cs.kaist.ac.kr                                          */
/*    phone: +82-42-350-7722                                                  */
/*    fax: +82-42-350-8380                                                    */
/*                                                                            */
/*    Copyright (c) 1995-2013 by Kyu-Young Whang                              */
/*                                                                            */
/*    All rights reserved. No part of this software may be reproduced,        */
/*    stored in a retrieval system, or transmitted, in any form or by any     */
/*    means, electronic, mechanical, photocopying, recording, or otherwise,   */
/*    without prior written permission of the copyright owner.                */
/*                                                                            */
/******************************************************************************/
/*
 * Module : EduOM_CreateObject.c
 *
 * Description :
 *  EduOM_CreateObject() creates a new object near the specified object.
 *
 * Exports:
 *  Four EduOM_CreateObject(ObjectID*, ObjectID*, ObjectHdr*, Four, char*,
 * ObjectID*)
 */

#include <string.h>

#include "BfM.h" /* for the buffer manager call */
#include "EduOM_Internal.h"
#include "EduOM_basictypes.h"
#include "EduOM_common.h"
#include "RDsM.h" /* for the raw disk manager call */

/*@================================
 * EduOM_CreateObject()
 *================================*/
/*
 * Function: Four EduOM_CreateObject(ObjectID*, ObjectID*, ObjectHdr*, Four,
 *char*, ObjectID*)
 *
 * Description :
 * (Following description is for original ODYSSEUS/COSMOS OM.
 *  For ODYSSEUS/EduCOSMOS EduOM, refer to the EduOM project manual.)
 *
 * (1) What to do?
 * EduOM_CreateObject() creates a new object near the specified object.
 * If there is no room in the page holding the specified object,
 * it trys to insert into the page in the available space list. If fail, then
 * the new object will be put into the newly allocated page.
 *
 * (2) How to do?
 *	a. Read in the near slotted page
 *	b. See the object header
 *	c. IF large object THEN
 *	       call the large object manager's lom_ReadObject()
 *	   ELSE
 *		   IF moved object THEN
 *				call this function recursively
 *		   ELSE
 *				copy the data into the buffer
 *		   ENDIF
 *	   ENDIF
 *	d. Free the buffer page
 *	e. Return
 *
 * Returns:
 *  error code
 *    eBADCATALOGOBJECT_OM
 *    eBADLENGTH_OM
 *    eBADUSERBUF_OM
 *    some error codes from the lower level
 *
 * Side Effects :
 *  0) A new object is created.
 *  1) parameter oid
 *     'oid' is set to the ObjectID of the newly created object.
 */
Four EduOM_CreateObject(
    ObjectID *catObjForFile, /* IN file in which object is to be placed */
    ObjectID *nearObj,       /* IN create the new object near this object */
    ObjectHdr *objHdr,       /* IN from which tag is to be set */
    Four length,             /* IN amount of data */
    char *data,              /* IN the initial data for the object */
    ObjectID *oid)           /* OUT the object's ObjectID */
{
  Four e;              /* error number */
  ObjectHdr objectHdr; /* ObjectHdr with tag set from parameter */

  /*@ parameter checking */

  if (catObjForFile == NULL) ERR(eBADCATALOGOBJECT_OM);

  if (length < 0) ERR(eBADLENGTH_OM);

  if (length > 0 && data == NULL) return (eBADUSERBUF_OM);

  /* Error check whether using not supported functionality by EduOM */
  if (ALIGNED_LENGTH(length) > LRGOBJ_THRESHOLD) ERR(eNOTSUPPORTED_EDUOM);

  // File을 구성하는 page들 중 파라미터로 지정한 object와 같은 (또는
  // 인접한) page에 새로운 object를 삽입하고, 삽입된 object의 ID를 반환함

  // 1. 삽입할 object의 header를 초기화함
  objectHdr.properties = 0x0;
  objectHdr.length = 0;
  if (objHdr != NULL)
    objectHdr.tag = objHdr->tag;
  else
    objectHdr.tag = 0;

  // 2. eduom_CreateObject()를 호출하여 page에 object를 삽입하고, 삽입된
  // object의 ID를 반환함
  eduom_CreateObject(catObjForFile, nearObj, &objectHdr, length, data, oid);

  return (eNOERROR);
}

/*@================================
 * eduom_CreateObject()
 *================================*/
/*
 * Function: Four eduom_CreateObject(ObjectID*, ObjectID*, ObjectHdr*, Four,
 * char*, ObjectID*)
 *
 * Description :
 * (Following description is for original ODYSSEUS/COSMOS OM.
 *  For ODYSSEUS/EduCOSMOS EduOM, refer to the EduOM project manual.)
 *
 *  eduom_CreateObject() creates a new object near the specified object; the
 * near page is the page holding the near object. If there is no room in the
 * near page and the near object 'nearObj' is not NULL, a new page is allocated
 * for object creation (In this case, the newly allocated page is inserted after
 * the near page in the list of pages consiting in the file). If there is no
 * room in the near page and the near object 'nearObj' is NULL, it trys to
 * create a new object in the page in the available space list. If fail, then
 * the new object will be put into the newly allocated page(In this case, the
 * newly allocated page is appended at the tail of the list of pages cosisting
 * in the file).
 *
 * Returns:
 *  error Code
 *    eBADCATALOGOBJECT_OM
 *    eBADOBJECTID_OM
 *    some errors caused by fuction calls
 */
Boolean isValidPageID(sm_CatOverlayForData *catEntry, PageNo x) {
  if ((x >= catEntry->firstPage) && (x <= catEntry->lastPage))
    return TRUE;
  else
    return FALSE;
}

Four eduom_CreateObject(
    ObjectID *catObjForFile, /* IN file in which object is to be placed */
    ObjectID *nearObj,       /* IN create the new object near this object */
    ObjectHdr *objHdr,       /* IN from which tag & properties are set */
    Four length,             /* IN amount of data */
    char *data,              /* IN the initial data for the object */
    ObjectID *oid)           /* OUT the object's ObjectID */
{
  Four e;                  /* error number */
  Four neededSpace;        /* space needed to put new object [+ header] */
  SlottedPage *apage;      /* pointer to the slotted page buffer */
  Four alignedLen;         /* aligned length of initial data */
  Boolean needToAllocPage; /* Is there a need to alloc a new page? */
  PageID pid;              /* PageID in which new object to be inserted */
  PageID nearPid;
  Four firstExt;                  /* first Extent No of the file */
  Object *obj;                    /* point to the newly created object */
  Two i;                          /* index variable */
  sm_CatOverlayForData *catEntry; /* pointer to data file catalog information */
  SlottedPage *catPage;           /* pointer to buffer containing the catalog */
  FileID fid; /* ID of file where the new object is placed */
  Two eff;    /* extent fill factor of file */
  Boolean isTmp;
  PhysicalFileID pFid;

  /*@ parameter checking */

  if (catObjForFile == NULL) ERR(eBADCATALOGOBJECT_OM);

  if (objHdr == NULL) ERR(eBADOBJECTID_OM);

  /* Error check whether using not supported functionality by EduOM */
  if (ALIGNED_LENGTH(length) > LRGOBJ_THRESHOLD) ERR(eNOTSUPPORTED_EDUOM);

  // File을 구성하는 page들 중 파라미터로 지정한 object와 같은 (또는 인접한)
  // page에 새로운 object를 삽입하고, 삽입된 object의 ID를 반환함
  e = BfM_GetTrain((TrainID *)catObjForFile, (char **)&catPage, PAGE_BUF);
  if (e < 0) ERR(e);
  catEntry =
      (sm_CatOverlayForData
           *)(catPage
                  ->data[catPage->slot[-1 * (catObjForFile->slotNo)].offset]);

  // 1. Object 삽입을 위해 필요한 자유 공간의 크기를 계산함
  alignedLen = ALIGNED_LENGTH(length);
  neededSpace = sizeof(ObjectHdr) + alignedLen + sizeof(SlottedPageSlot);

  // 2. Object를 삽입할 page를 선정함
  if (nearObj != NULL) {
    // 2-1. 파라미터로 주어진 nearObj가 NULL 이 아닌 경우
    e = BfM_GetTrain(nearObj, (char **)&apage, PAGE_BUF);
    if (e < 0) ERR(e);
    pid = apage->header.pid;
    if (SP_FREE(apage) >= neededSpace) {
      // 2-1-1. nearObj가 저장된 page에 여유 공간이 있는 경우, 해당 page를 선정
      // 선정된 page를 현재 available space list에서 삭제함
      // 필요시 선정된 page를 compact 함
      om_RemoveFromAvailSpaceList(nearObj, &pid, apage);
      if (e < 0) ERRB1(e, &pid, PAGE_BUF);
      if (apage->header.unused != 0) EduOM_CompactPage(apage, NIL);
    } else {
      // 2-1-2. nearObj가 저장된 page에 여유 공간이 없는 경우, page를 새로
      // 할당해 선정
      // 선정된 page의 header를 초기화함, 선정된 page를 file 구성 page들로
      // 이루어진 list에서 nearObj 가 저장된 page의 다음 page로 삽입함
      e = RDsM_AllocTrains(catEntry->fid.volNo, firstExt, &nearPid,
                           catEntry->eff, 1, PAGESIZE2, &pid);
      if (e < 0) ERR(e);
      e = BfM_GetNewTrain(&pid, (char **)&apage, PAGE_BUF);
      if (e < 0) ERR(e);
      e = om_FileMapAddPage(catObjForFile, (PageID *)nearObj, &pid);
      if (e < 0) ERRB1(e, &pid, PAGE_BUF);
    }
  } else {
    // 2-2 파라미터로 주어진 nearObj가 NULL인 경우
    Four neededFreeProportion = (neededSpace / (PAGESIZE - SP_FIXED)) * 100;

    // 2-2-1. Object 삽입을 위해 필요한 자유 공간의 크기에 알맞은 available
    // space list가 존재하는 경우, 해당 list의 첫 page를 선정
    // 선정된 page를 현재 available space list에서 삭제함
    // 필요시 선정된 page를 compact 함
    pid.volNo = catEntry->fid.volNo;
    if (neededFreeProportion >= 50) {
      if (isValidPageID(catEntry, neededFreeProportion))
        pid.pageNo = catEntry->availSpaceList50;
      else
        needToAllocPage = TRUE;
    } else if (neededFreeProportion >= 40) {
      if (isValidPageID(catEntry, neededFreeProportion))
        pid.pageNo = catEntry->availSpaceList40;
      else
        needToAllocPage = TRUE;
    } else if (neededFreeProportion >= 30) {
      if (isValidPageID(catEntry, neededFreeProportion))
        pid.pageNo = catEntry->availSpaceList30;
      else
        needToAllocPage = TRUE;
    } else if (neededFreeProportion >= 20) {
      if (isValidPageID(catEntry, neededFreeProportion))
        pid.pageNo = catEntry->availSpaceList20;
      else
        needToAllocPage = TRUE;
    } else if (neededFreeProportion >= 10) {
      if (isValidPageID(catEntry, neededFreeProportion))
        pid.pageNo = catEntry->availSpaceList10;
      else
        needToAllocPage = TRUE;
    } else {
      needToAllocPage = TRUE;
    }

    if (needToAllocPage != TRUE) {
      om_RemoveFromAvailSpaceList(catObjForFile, &pid, apage);
    } else {
      pid.pageNo = catEntry->lastPage;
      e = BfM_GetTrain(&pid, (char **)&apage, PAGE_BUF);
      if (e < 0) ERR(e);

      if (SP_FREE(apage) >= neededSpace) {
        // 2-2-2. Object 삽입을 위해 필요한 자유 공간의 크기에 알맞은 available
        // space list가 존재하지 않고, file의 마지막 page에 여유 공간이 있는
        // 경우, file의 마지막 page를 선정
        // if (apage->header.unused != 0) EduOM_CompactPage(apage, NIL);
        if (apage->header.unused != 0) EduOM_CompactPage(apage, NIL);
      } else {
        // 2-2-3. Object 삽입을 위해 필요한 자유 공간의 크기에 알맞은 available
        // space list가 존재하지 않고, file의 마지막 page에 여유 공간이 없는
        // 경우 새로운 page를 할당해 선정
        // 선정된 page의 header를 초기화함, 선정된 page를 file 구성 page들로
        // 이루어진 list에서 마지막 page로 삽입함
        e = RDsM_AllocTrains(catEntry->fid.volNo, firstExt, &pid, catEntry->eff,
                             1, PAGESIZE2, &pid);
        if (e < 0) ERR(e);
        e = BfM_GetNewTrain(&pid, (char **)&apage, PAGE_BUF);
        if (e < 0) ERR(e);

        e = om_GetUnique(&pid, &(apage->slot[-i].unique));
        if (e < 0) ERRB1(e, &pid, PAGE_BUF);

        e = om_FileMapAddPage(catObjForFile, &pid, &pid);
        if (e < 0) ERRB1(e, &pid, PAGE_BUF);
      }
    }
  }

  // 3. 선정된 page에 object를 삽입함
  // Object의 header를 갱신함
  // 선정한 page의 contiguous free area에 object를 복사함
  objHdr->length = alignedLen;
  for (Two i = 0; i < length; ++i) {
    apage->data[apage->header.free] = data[i];
  }
  // Slot array의 빈 slot 또는 새로운 slot 한 개를 할당 받아 복사한 object 의
  // 식별을 위한 정보를 저장함
  i = 0;
  for (; i < apage->header.nSlots; ++i) {
    if (apage->slot[-i].offset >= 0)
      continue;
    else {
      break;
    }
  }
  // Page의 header를 갱신함
  SlottedPageSlot *insertedSlot = &(apage->slot)[i * -1];
  insertedSlot->offset = apage->header.free;
  e = om_GetUnique(&pid, insertedSlot->unique);
  if (e < 0) ERRB1(e, &pid, PAGE_BUF);

  apage->header.free += alignedLen;
  apage->header.unique = insertedSlot->unique;
  if (i == apage->header.nSlots) apage->header.nSlots++;

  // Page를 알맞은 available space list에 삽입함
  e = om_PutInAvailSpaceList(catObjForFile, &pid, apage);
  if (e < 0) ERRB1(e, &pid, PAGE_BUF);

  // 4. 삽입된 object의 ID를 반환함
  oid->volNo = pid.volNo;
  oid->pageNo = pid.pageNo;
  oid->slotNo = i;
  oid->unique = insertedSlot->unique;

  return (eNOERROR);

} /* eduom_CreateObject() */
