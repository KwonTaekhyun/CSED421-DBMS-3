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
 * Module: EduOM_NextObject.c
 *
 * Description:
 *  Return the next Object of the given Current Object.
 *
 * Export:
 *  Four EduOM_NextObject(ObjectID*, ObjectID*, ObjectID*, ObjectHdr*)
 */

#include "BfM.h"
#include "EduOM_Internal.h"
#include "EduOM_common.h"

/*@================================
 * EduOM_NextObject()
 *================================*/
/*
 * Function: Four EduOM_NextObject(ObjectID*, ObjectID*, ObjectID*, ObjectHdr*)
 *
 * Description:
 * (Following description is for original ODYSSEUS/COSMOS OM.
 *  For ODYSSEUS/EduCOSMOS EduOM, refer to the EduOM project manual.)
 *
 *  Return the next Object of the given Current Object.  Find the Object in the
 *  same page which has the current Object and  if there  is no next Object in
 *  the same page, find it from the next page. If the Current Object is NULL,
 *  return the first Object of the file.
 *
 * Returns:
 *  error code
 *    eBADCATALOGOBJECT_OM
 *    eBADOBJECTID_OM
 *    some errors caused by function calls
 *
 * Side effect:
 *  1) parameter nextOID
 *     nextOID is filled with the next object's identifier
 *  2) parameter objHdr
 *     objHdr is filled with the next object's header
 */
Four EduOM_NextObject(
    ObjectID *catObjForFile, /* IN informations about a data file */
    ObjectID *curOID,        /* IN a ObjectID of the current Object */
    ObjectID *nextOID,       /* OUT the next Object of a current Object */
    ObjectHdr *objHdr)       /* OUT the object header of next object */
{
  Four e;               /* error */
  Two i;                /* index */
  Four offset;          /* starting offset of object within a page */
  PageID pid;           /* a page identifier */
  PageNo pageNo;        /* a temporary var for next page's PageNo */
  SlottedPage *apage;   /* a pointer to the data page */
  Object *obj;          /* a pointer to the Object */
  PhysicalFileID pFid;  /* file in which the objects are located */
  SlottedPage *catPage; /* buffer page containing the catalog object */
  sm_CatOverlayForData *catEntry; /* data structure for catalog object access */

  /*@
   * parameter checking
   */
  if (catObjForFile == NULL) ERR(eBADCATALOGOBJECT_OM);

  if (nextOID == NULL) ERR(eBADOBJECTID_OM);

  // 현재 object의 다음 object의 ID를 반환함

  BfM_GetTrain((TrainID *)catObjForFile, (char **)&catPage, PAGE_BUF);
  GET_PTR_TO_CATENTRY_FOR_DATA(catObjForFile, catPage, catEntry);
  MAKE_PAGEID(pFid, catEntry->fid.volNo, catEntry->firstPage);

  // 1. 파라미터로 주어진 curOID가 NULL 인 경우,
  // File의 첫 번째 page의 slot array 상에서의 첫 번째 object의 ID를 반환함
  if (curOID == NULL) {
    pageNo = catEntry->firstPage;

    MAKE_PAGEID(pid, pFid.volNo, pageNo);
    BfM_GetTrain((TrainID *)&pid, (char **)&apage, PAGE_BUF);

    nextOID->volNo = pid.volNo;
    nextOID->pageNo = pageNo;
    nextOID->slotNo = 0;
    nextOID->unique = apage->slot[0].unique;

    BfM_FreeTrain((TrainID *)&pid, PAGE_BUF);
  } else {
    // 2. 파라미터로 주어진 curOID가 NULL 이 아닌 경우,
    // curOID에 대응하는 object를 탐색함
    MAKE_PAGEID(pid, curOID->volNo, curOID->pageNo);
    BfM_GetTrain((TrainID *)&pid, (char **)&apage, PAGE_BUF);

    // Slot array 상에서, 탐색한 object의 다음 object의 ID를 반환함
    // 탐색한 object가 page의 마지막 object인 경우, 다음 page의 첫 번째 object의
    // ID를 반환함
    // 탐색한 object가 file의 마지막 page의 마지막 object인 경우, EOS (End Of
    // Scan) 를 반환함
    if (curOID->slotNo + 1 != apage->header.nSlots) {
      nextOID->volNo = curOID->volNo;
      nextOID->pageNo = curOID->pageNo;
      nextOID->slotNo = curOID->slotNo + 1;
      nextOID->unique = apage->slot[(nextOID->slotNo) * -1].unique;

      BfM_FreeTrain((TrainID *)&pid, PAGE_BUF);
    } else {
      if (catEntry->lastPage == apage->header.pid.pageNo) {
        BfM_FreeTrain((TrainID *)&pid, PAGE_BUF);
        BfM_FreeTrain((TrainID *)catObjForFile, PAGE_BUF);
        return (EOS);
      }
      pageNo = apage->header.nextPage;

      BfM_FreeTrain((TrainID *)&pid, PAGE_BUF);
      MAKE_PAGEID(pid, curOID->volNo, pageNo);
      BfM_GetTrain((TrainID *)&pid, (char **)&apage, PAGE_BUF);

      nextOID->volNo = curOID->volNo;
      nextOID->pageNo = pageNo;
      nextOID->slotNo = 0;
      nextOID->unique = apage->slot[0].unique;
      BfM_FreeTrain((TrainID *)&pid, PAGE_BUF);
    }
  }

  BfM_FreeTrain((TrainID *)catObjForFile, PAGE_BUF);
  return (EOS); /* end of scan */

} /* EduOM_NextObject() */
