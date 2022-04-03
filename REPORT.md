# EduOM Report

Name: 권택현

Student id: 20180522

# Problem Analysis

EduCOSMOS DBMS에서 EduOM, object manager와 page에 관련한 구조들에 대한 연산들을 구현하는 프로젝트이다.
구체적으로 object를 저장하기 위한 slotted page관련 구조에 대한 연산들을 구현한다.

# Design For Problem Solving

## High Level

1. EduOM_CreateObject

File을 구성하는 page들 중 파라미터로 지정한 object와 같은 (또는 인접한) 
page에 새로운 object를 삽입하고, 삽입된 object의 ID를 반환함

    1) 삽입할 object의 header를 초기화함
    2) eduom_CreateObject()를 호출하여 page에 object를 삽입하고, 삽입된 object의 ID를 반환함

2. EduOM_CompactPage

Page의 데이터 영역의 모든 자유 공간이 연속된 하나의 contiguous free area를 형성하도록 object들의 offset를 조정함

    1) 파라미터로 주어진 slotNo가 NIL (-1) 이 아닌 경우, slotNo에 대응하는 object를 제외한 page의 모든 object들을 데이터 영역의
    가장 앞부분부터 연속되게 저장함. slotNo에 대응하는 object를 데이터 영역 상에서의 마지막 object로 저장함
    2) 파라미터로 주어진 slotNo가 NIL (-1) 인 경우, Page의 모든 object들을 데이터 영역의 가장 앞부분부터 연속되게 저장함
    3) Page header를 갱신함

3. EduOM_DestroyObject

File을 구성하는 page에서 object를 삭제함

    1) 삭제할 object가 저장된 page를 현재 available space list에서 삭제함
    2) 삭제할 object에 대응하는 slot을 사용하지 않는 빈 slot으로 설정함
    3) Page header를 갱신함
    3-1) 삭제할 object에 대응하는 slot이 slot array의 마지막 slot인 경우, slot array의 크기를 갱신함
    3-2) 삭제할 object의 데이터 영역 상에서의 offset에 따라 free 또는 unused 변수를 갱신함
    4) 삭제된 object가 page의 유일한 object이고, 해당 page가 file의 첫 번째 page가 아닌 경우, Page를 file 구성 page들로 이루어진 list에서 삭제함
    파라미터로 주어진 dlPool에서 새로운 dealloc list element 한 개를 할당받음, 할당 받은 element에 deallocate 할 page 정보를 저장함. Deallocate 할 page 정보가 저장된 element를 dealloc list의 첫 번째 element로 삽입함
    5) 삭제된 object가 page의 유일한 object가 아니거나, 해당 page가 file의 첫번째 page인 경우, Page를 알맞은 available space list에 삽입함

4. EduOM_ReadObject

Object의 데이터 전체 또는 일부를 읽고, 읽은 데이터에 대한 포인터를 반환함

    1) 파라미터로 주어진 oid를 이용하여 object에 접근함
    2) 파라미터로 주어진 start 및 length를 고려하여 접근한 object의 데이터를 읽음
    3) 해당 데이터에 대한 포인터를 반환함

5. EduOM_NextObject

현재 object의 다음 object의 ID를 반환함

    1) 파라미터로 주어진 curOID가 NULL 인 경우, File의 첫 번째 page의 slot array 상에서의 첫 번째 object의 ID를 반환함
    2) 파라미터로 주어진 curOID가 NULL 이 아닌 경우, curOID에 대응하는 object를 탐색함. Slot array 상에서, 탐색한 object의 다음 object의 ID를 반환함
    탐색한 object가 page의 마지막 object인 경우, 다음 page의 첫 번째 object의 ID를 반환함. 탐색한 object가 file의 마지막 page의 마지막 object인 경우, EOS (End Of Scan) 를 반환함

6. EduOM_PrevObject

현재 object의 이전 object의 ID를 반환함

    1) 파라미터로 주어진 curOID가 NULL 인 경우, File의 마지막 page의 slot array상에서의 마지막 object의 ID를 반환함
    2) 파라미터로 주어진 curOID가 NULL 이 아닌 경우, curOID에 대응하는 object를 탐색함. Slot array 상에서, 탐색한 object의 이전 object의 ID를
    반환함, 탐색한 object가 page의 첫 번째 object인 경우, 이전 page의 마지막 object의 ID를 반환함, 탐색한 object가 file의 첫 번째 page의 첫 번째 object인 경우, EOS (End Of Scan) 를 반환함

## Low Level

1. eduOM_CreateObject

File을 구성하는 page들 중 파라미터로 지정한 object와 같은 (또는 인접한) page에 새로운 object를 삽입하고, 삽입된 object의 ID를 반환함

    1) Object 삽입을 위해 필요한 자유 공간의 크기를 계산함 – sizeof(ObjectHdr) + align 된 object 데이터 영역의 크기
    2) Object를 삽입할 page를 선정함
    2-1) 파라미터로 주어진 nearObj가 NULL 이 아닌 경우,
        1) nearObj가 저장된 page에 여유 공간이 있는 경우, 해당 page를 object를 삽입할 page로 선정함. 선정된 page를 현재 available space list에서 삭제함 필요시 선정된 page를 compact 함
        2) nearObj가 저장된 page에 여유 공간이 없는 경우, 새로운 page를 할당 받아 object를 삽입할 page로 선정함. 선정된 page의 header를 초기화함. 선정된 page를 file 구성 page들로 이루어진 list에서 nearObj 가 저장된 page의 다음 page로 삽입함
    2-2) 파라미터로 주어진 nearObj가 NULL 인 경우,
        1) Object 삽입을 위해 필요한 자유 공간의 크기에 알맞은 available space list가 존재하는 경우, 해당 available space list의 첫 번째 page를 object를 삽입할 page로 선정함. 선정된 page를 현재 available space list에서 삭제함. 필요시 선정된 page를 compact 함
        2) Object 삽입을 위해 필요한 자유 공간의 크기에 알맞은 available space list가 존재하지 않고, file의 마지막 page에 여유 공간이 있는 경우, File의 마지막 page를 object를 삽입할 page로 선정함. 필요시 선정된 page를 compact 함
        3) Object 삽입을 위해 필요한 자유 공간의 크기에 알맞은 available space list가 존재하지 않고, file의 마지막 page에 여유 공간이 없는 경우, 새로운 page를 할당 받아 object를 삽입할 page로 선정함. 선정된 page의 header를 초기화함. 선정된 page를 file의 구성 page들로 이루어진 list에서 마지 막 page로 삽입함
    2-3) 선정된 page에 object를 삽입함 – Object의 header를 갱신함
        1) 선정한 page의 contiguous free area에 object를 복사함
        2) Slot array의 빈 slot 또는 새로운 slot 한 개를 할당 받아 복사한 object 의 식별을 위한 정보를 저장함
        3) Page의 header를 갱신함
        4) Page를 알맞은 available space list에 삽입함
    3) 삽입된 object의 ID를 반환함

# Mapping Between Implementation And the Design

1. EduOM_CreateObject

File을 구성하는 page들 중 파라미터로 지정한 object와 같은 (또는 인접한) 
page에 새로운 object를 삽입하고, 삽입된 object의 ID를 반환함

```cpp
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
```

2. eduOM_CreateObject

```cpp
  // File을 구성하는 page들 중 파라미터로 지정한 object와 같은 (또는 인접한)
  // page에 새로운 object를 삽입하고, 삽입된 object의 ID를 반환함
  BfM_GetTrain((TrainID *)catObjForFile, (char **)&catPage, PAGE_BUF);
  GET_PTR_TO_CATENTRY_FOR_DATA(catObjForFile, catPage, catEntry);
  MAKE_PAGEID(pFid, catEntry->fid.volNo, catEntry->firstPage);
  RDsM_PageIdToExtNo((PageID *)&pFid, &firstExt);

  // 1. Object 삽입을 위해 필요한 자유 공간의 크기를 계산함
  alignedLen = ALIGNED_LENGTH(length);
  neededSpace = sizeof(ObjectHdr) + alignedLen + sizeof(SlottedPageSlot);

  // 2. Object를 삽입할 page를 선정함
  // 2-1. 파라미터로 주어진 nearObj가 NULL 이 아닌 경우
  if (nearObj != NULL) {
    MAKE_PAGEID(nearPid, nearObj->volNo, nearObj->pageNo);
    BfM_GetTrain((TrainID *)&nearPid, (char **)&apage, PAGE_BUF);

    // nearObj가 저장된 page에 여유 공간이 있는 경우, 해당 page를 object를
    // 삽입할 page로 선정함, 선정된 page를 현재 available space list에서 삭제함
    if (SP_FREE(apage) >= neededSpace) {
      pid = nearPid;
      om_RemoveFromAvailSpaceList(catObjForFile, &pid, apage);

      // 필요시 선정된 page를 compact 함
      if (SP_CFREE(apage) < neededSpace) {
        EduOM_CompactPage(apage, nearObj->slotNo);
      }
    } else {
      // nearObj가 저장된 page에 여유 공간이 없는 경우, 새로운 page를 할당 받아
      // object를 삽입할 page로 선정함, 선정된 page의 header를 초기화함, 선정된
      // page를 file 구성 page들로 이루어진 list에서 nearObj 가 저장된 page의
      // 다음 page로 삽입함
      BfM_FreeTrain((TrainID *)&nearPid, PAGE_BUF);
      RDsM_AllocTrains(catEntry->fid.volNo, firstExt, &nearPid, catEntry->eff,
                       1, PAGESIZE2, &pid);
      BfM_GetNewTrain((TrainID *)&pid, (char **)&apage, PAGE_BUF);

      apage->header.fid = catEntry->fid;
      apage->header.flags = 0x2;
      apage->header.free = 0;
      apage->header.unused = 0;

      om_FileMapAddPage(catObjForFile, &nearPid, &pid);
    }
  } else {
    // 2-2. 파라미터로 주어진 nearObj가 NULL 인 경우, Object 삽입을 위해 필요한
    // 자유 공간의 크기에 알맞은 available space list가 존재하는 경우, 해당
    // available space list의 첫 번째 page를 object를 삽입할 page로 선정함
    // 선정된 page를 현재 available space list에서 삭제함
    PageNo possibleAvailPageNum = NIL;
    if (neededSpace <= SP_10SIZE)
      possibleAvailPageNum = catEntry->availSpaceList10;
    else if (neededSpace <= SP_20SIZE)
      possibleAvailPageNum = catEntry->availSpaceList20;
    else if (neededSpace <= SP_30SIZE)
      possibleAvailPageNum = catEntry->availSpaceList30;
    else if (neededSpace <= SP_40SIZE)
      possibleAvailPageNum = catEntry->availSpaceList40;
    else if (neededSpace <= SP_50SIZE)
      possibleAvailPageNum = catEntry->availSpaceList50;

    if (possibleAvailPageNum != NIL) {
      MAKE_PAGEID(pid, pFid.volNo, possibleAvailPageNum);
      BfM_GetNewTrain((TrainID *)&pid, (char **)&apage, PAGE_BUF);
      om_RemoveFromAvailSpaceList(catObjForFile, &pid, apage);

      // 필요시 선정된 page를 compact 함
      if (SP_CFREE(apage) < neededSpace) {
        EduOM_CompactPage(apage, nearObj->slotNo);
      }
    } else {
      // Object 삽입을 위해 필요한 자유 공간의 크기에 알맞은 available space
      // list가 존재하지 않고, file의 마지막 page에 여유 공간이 있는 경우,
      // File의 마지막 page를 object를 삽입할 page로 선정함
      MAKE_PAGEID(pid, pFid.volNo, catEntry->lastPage);
      BfM_GetTrain((TrainID *)&pid, (char **)&apage, PAGE_BUF);
      if (SP_FREE(apage) >= neededSpace) {
        om_RemoveFromAvailSpaceList(catObjForFile, &pid, apage);

        // 필요시 선정된 page를 compact 함
        if (SP_CFREE(apage) < neededSpace) {
          EduOM_CompactPage(apage, nearObj->slotNo);
        }

      } else {
        // Object 삽입을 위해 필요한 자유 공간의 크기에 알맞은 available space
        // list가 존재하지 않고, file의 마지막 page에 여유 공간이 없 는 경우,
        // 새로운 page를 할당 받아 object를 삽입할 page로 선정함
        // 선정된 page의 header를 초기화함
        // 선정된 page를 file의 구성 page들로 이루어진 list에서 마지막 page로
        // 삽입함
        BfM_FreeTrain((TrainID *)&pid, PAGE_BUF);
        MAKE_PAGEID(nearPid, pFid.volNo, catEntry->lastPage);
        RDsM_AllocTrains(catEntry->fid.volNo, firstExt, &nearPid, catEntry->eff,
                         1, PAGESIZE2, &pid);
        BfM_GetNewTrain((TrainID *)&pid, (char **)&apage, PAGE_BUF);

        apage->header.fid = catEntry->fid;
        apage->header.flags = 0x2;
        apage->header.free = 0;
        apage->header.unused = 0;

        om_FileMapAddPage(catObjForFile, &nearPid, &pid);
      }
    }
  }

  // 3. 선정된 page에 object를 삽입함

  // Object의 header를 갱신함
  obj = (Object *)&(apage->data[apage->header.free]);
  obj->header.properties = objHdr->properties;
  obj->header.length = length;
  obj->header.tag = objHdr->tag;

  // 선정한 page의 contiguous free area에 object를 복사함
  memcpy(obj->data, data, length);

  // Slot array의 빈 slot 또는 새로운 slot 한 개를 할당 받아 복사한 object의
  // 식별을 위한 정보를 저장함
  for (i = 0; i < apage->header.nSlots; ++i) {
    if (apage->slot[-i].offset == EMPTYSLOT) break;
  }
  SlottedPageSlot *insertedSlot = &(apage->slot[-i]);
  insertedSlot->offset = apage->header.free;
  om_GetUnique(&pid, &(insertedSlot->unique));

  // Page의 header를 갱신함
  if (i == apage->header.nSlots) apage->header.nSlots++;
  apage->header.free += (sizeof(ObjectHdr) + alignedLen);

  // Page를 알맞은 available space list에 삽입함
  om_PutInAvailSpaceList(catObjForFile, &pid, apage);

  MAKE_OBJECTID(*oid, pid.volNo, pid.pageNo, i, insertedSlot->unique);

  BfM_SetDirty((TrainID *)&pid, PAGE_BUF);
  BfM_FreeTrain((TrainID *)&pid, PAGE_BUF);
  BfM_FreeTrain((TrainID *)catObjForFile, PAGE_BUF);
```

3. EduOM_CompactPage

```cpp
// Page의 데이터 영역의 모든 자유 공간이 연속된 하나의 contiguous free
  // area를 형성하도록 object들의 offset를 조정함
  lastSlot = apage->header.nSlots - 1;

  if (slotNo != NIL) {
    // 1. 파라미터로 주어진 slotNo가 NIL(-1)이 아닌 경우
    // slotNo에 대응하는 object를 제외한 page의 모든 object들을 데이터 영역의
    // 가장 앞부분부터 연속되게 저장함
    // slotNo에 대응하는 object를 데이터 영역 상에서의 마지막 object로 저장함
    char temp[100];

    apageDataOffset = 0;
    for (i = 0; i <= lastSlot; ++i) {
      obj = &(apage->data[apage->slot[-i].offset]);
      len = sizeof(ObjectHdr) + ALIGNED_LENGTH(obj->header.length);

      if (apage->slot[-i].offset == EMPTYSLOT) {
        continue;
      }

      if (i == slotNo) {
        memcpy(temp, (char *)obj, len);
      } else {
        memcpy(&(apage->data[apageDataOffset]), (char *)obj, len);
        (apage->slot[-i]).offset = apageDataOffset;
        apageDataOffset += len;
      }
    }
    memcpy(&(apage->data[apageDataOffset]), temp, len);
    (apage->slot[slotNo]).offset = apageDataOffset;
    apageDataOffset += len;

  } else {
    // 2. 파라미터로 주어진 slotNo가 NIL (-1) 인 경우
    // Page의 모든 object들을 데이터 영역의 가장 앞부분부터 연속되게 저장함
    apageDataOffset = 0;
    for (i = 0; i <= lastSlot; ++i) {
      obj = &(apage->data[apage->slot[-i].offset]);
      len = sizeof(ObjectHdr) + ALIGNED_LENGTH(obj->header.length);

      memcpy(&(apage->data[apageDataOffset]), (char *)obj, len);
      (apage->slot[-i]).offset = apageDataOffset;
      apageDataOffset += len;
    }
  }

  // 3. Page header를 갱신함
  apage->header.free = apageDataOffset;
  apage->header.unused = 0;
```

4. EduOM_DestroyObject

```cpp
// File을 구성하는 page에서 object를 삭제함

  // 1. 삭제할 object가 저장된 page를 현재 available space list에서 삭제함
  BfM_GetTrain((TrainID *)catObjForFile, (char **)&catPage, PAGE_BUF);
  GET_PTR_TO_CATENTRY_FOR_DATA(catObjForFile, catPage, catEntry);
  MAKE_PAGEID(pid, oid->volNo, oid->pageNo);
  BfM_GetTrain((TrainID *)&pid, (char **)&apage, PAGE_BUF);

  om_RemoveFromAvailSpaceList(catObjForFile, &pid, apage);

  // 2. 삭제할 object에 대응하는 slot을 사용하지 않는 빈 slot으로 설정함
  obj = (Object *)&(apage->data[apage->slot[(oid->slotNo) * -1].offset]);
  offset = apage->slot[(oid->slotNo) * -1].offset;
  apage->slot[(oid->slotNo) * -1].offset = EMPTYSLOT;

  // 3. Page header를 갱신함
  // 삭제할 object에 대응하는 slot이 slot array의 마지막 slot인 경우, slot
  // array의 크기를 갱신함
  if (apage->header.nSlots == oid->slotNo + 1) {
    apage->header.nSlots--;
  }
  // 삭제할 object의 데이터 영역 상에서의 offset에 따라 free 또는 unused 변수를
  // 갱신함
  if (offset + (sizeof(ObjectHdr) + ALIGNED_LENGTH(obj->header.length)) ==
      apage->header.free) {
    apage->header.free -=
        (sizeof(ObjectHdr) + ALIGNED_LENGTH(obj->header.length));
  } else {
    apage->header.unused +=
        (sizeof(ObjectHdr) + ALIGNED_LENGTH(obj->header.length));
  }

  // 4. 삭제된 object가 page의 유일한 object이고, 해당 page가 file의 첫 번째
  // page가 아닌 경우
  if (apage->header.nSlots == 0 &&
      apage->header.pid.pageNo != catEntry->firstPage) {
    // Page를 file 구성 page들로 이루어진 list에서 삭제함
    // 파라미터로 주어진 dlPool에서 새로운 dealloc list element 한 개를 할당
    // 받음, 할당 받은 element에 deallocate 할 page 정보를 저장함
    // Deallocate 할 page 정보가 저장된 element를 dealloc list의 첫 번째
    // element로 삽입함
    om_FileMapDeletePage(catObjForFile, &pid);
    Util_getElementFromPool(dlPool, &dlElem);

    dlElem->type = DL_PAGE;
    dlElem->elem.pid = pid;
    dlElem->next = dlHead->next;
    dlHead->next = dlElem;
  } else {
    // 5. 삭제된 object가 page의 유일한 object가 아니거나, 해당 page가 file의 첫
    // 번째 page인 경우, Page를 알맞은 available space list에 삽입함
    om_PutInAvailSpaceList(catObjForFile, &pid, apage);
  }

  BfM_SetDirty((TrainID *)&pid, PAGE_BUF);
  BfM_FreeTrain((TrainID *)&pid, PAGE_BUF);
  BfM_FreeTrain((TrainID *)catObjForFile, PAGE_BUF);
```

5. EduOM_ReadObject

```cpp
  // Object의 데이터 전체 또는 일부를 읽고, 읽은 데이터에 대한 포인터를
  // 반환함
  // 1. 파라미터로 주어진 oid를 이용하여 object에 접근함
  MAKE_PAGEID(pid, oid->volNo, oid->pageNo);
  BfM_GetTrain((TrainID *)&pid, (char **)&apage, PAGE_BUF);
  obj = (Object *)&(apage->data[apage->slot[(oid->slotNo) * -1].offset]);

  // 2. 파라미터로 주어진 start 및 length를 고려하여 접근한 object의 데이터를
  // 읽음
  // 3. 해당 데이터에 대한 포인터를 반환함
  if (length == REMAINDER) length = obj->header.length;
  memcpy(buf, &(obj->data[start]), length);

  BfM_FreeTrain((TrainID *)&pid, PAGE_BUF);
```

6. EduOM_NextObject

```cpp
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
```

7. EduOM_PrevObject

```cpp
// 현재 object의 이전 object의 ID를 반환함

  BfM_GetTrain((TrainID *)catObjForFile, (char **)&catPage, PAGE_BUF);
  GET_PTR_TO_CATENTRY_FOR_DATA(catObjForFile, catPage, catEntry);

  // 1. 파라미터로 주어진 curOID가 NULL 인 경우, File의 마지막 page의 slot array
  // 상에서의 마지막 object의 ID를 반환함
  if (curOID == NULL) {
    pageNo = catEntry->lastPage;

    MAKE_PAGEID(pid, catEntry->fid.volNo, pageNo);
    BfM_GetTrain((TrainID *)&pid, (char **)&apage, PAGE_BUF);

    prevOID->volNo = pid.volNo;
    prevOID->pageNo = pageNo;
    prevOID->slotNo = apage->header.nSlots - 1;
    prevOID->unique = apage->slot[(prevOID->slotNo) * -1].unique;

    BfM_FreeTrain((TrainID *)&pid, PAGE_BUF);
  } else {
    // 2. 파라미터로 주어진 curOID가 NULL 이 아닌 경우, curOID에 대응하는
    // object를 탐색함
    MAKE_PAGEID(pid, curOID->volNo, curOID->pageNo);
    BfM_GetTrain((TrainID *)&pid, (char **)&apage, PAGE_BUF);

    // Slot array 상에서, 탐색한 object의 이전 object의 ID를
    // 반환함, 탐색한 object가 page의 첫 번째 object인 경우, 이전 page의 마지막
    // object의 ID를 반환함, 탐색한 object가 file의 첫 번째 page의 첫 번째
    // object인 경우, EOS (End Of Scan) 를 반환함
    if (curOID->slotNo != 0) {
      prevOID->volNo = curOID->volNo;
      prevOID->pageNo = curOID->pageNo;
      prevOID->slotNo = curOID->slotNo - 1;
      prevOID->unique = apage->slot[(prevOID->slotNo) * -1].unique;

      BfM_FreeTrain((TrainID *)&pid, PAGE_BUF);
    } else {
      if (apage->header.pid.pageNo == catEntry->firstPage) {
        BfM_FreeTrain((TrainID *)&pid, PAGE_BUF);
        BfM_FreeTrain((TrainID *)catObjForFile, PAGE_BUF);
        return (EOS);
      } else {
        pageNo = apage->header.prevPage;

        BfM_FreeTrain((TrainID *)&pid, PAGE_BUF);
        MAKE_PAGEID(pid, curOID->volNo, pageNo);
        BfM_GetTrain((TrainID *)&pid, (char **)&apage, PAGE_BUF);

        prevOID->volNo = curOID->volNo;
        prevOID->pageNo = pageNo;
        prevOID->slotNo = apage->header.nSlots - 1;
        prevOID->unique = apage->slot[(prevOID->slotNo) * -1].unique;

        BfM_FreeTrain((TrainID *)&pid, PAGE_BUF);
      }
    }
  }

  BfM_FreeTrain((TrainID *)catObjForFile, PAGE_BUF);
  return (EOS);
```