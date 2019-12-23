#include "stdafx.h"
#include "RM_Manager.h"
#include "math.h"

#define EPS  1e-8

/*功能函数*/
int count_bit_set(unsigned char v) {
	int num_to_bits[16] = { 0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4 };
	return num_to_bits[v % 16] + num_to_bits[v >> 4];
}

int most_significant_bit_pos(unsigned char v) {
	unsigned int r;
	unsigned int shift;
	r = (unsigned int)(v > 0xF) << 2;      v >>= r;
	shift = (unsigned int)(v > 0x3) << 1;  v >>= shift;    r |= shift;
	r |= (v >> 1);
	return r;
}

int least_significant_bit_pos(unsigned char v) {
	v &= (~v + 1);
	return most_significant_bit_pos(v);
}

bool floatEqual(float a, float b) {
	return abs(a - b) < EPS;
}

bool floatLess(float a, float b) {
	return a + EPS < b;
}

//检查条件是否满足
bool checkConditions(RM_FileScan *rm_fileScan, RM_Record *record) {
	auto conditions = rm_fileScan->conditions;
	auto conNum = rm_fileScan->conNum;
	bool result = true;

	//循环比较每一个扫描条件
	for (int i = 0; i < conNum; i++) {
		auto currentCon = conditions[i];
		char * leftValue, *rightValue;

		if (currentCon.bLhsIsAttr == 0) {//左边是值
			leftValue = (char *)currentCon.Lvalue;
		}
		else {//左边是属性
			leftValue = new char[currentCon.LattrLength+1];
			memcpy(leftValue, record->pData + currentCon.LattrOffset, currentCon.LattrLength+1);
		}

		if (currentCon.bRhsIsAttr == 0) {//右边是值
			rightValue = (char *)currentCon.Rvalue;
		}
		else {//右边是属性
			rightValue = new char[currentCon.RattrLength+1];
			memcpy(rightValue, record->pData + currentCon.RattrOffset, currentCon.RattrLength+1);
		}

		switch (currentCon.attrType) {
		//int型
		case ints: {
			int lValue = *(int *)leftValue;
			int rValue = *(int *)rightValue;
			switch (currentCon.compOp) {
			case EQual: {
				result &= (lValue == rValue);
				break;
			}
			case LEqual: {
				result &= (lValue <= rValue);
				break;
			}
			case NEqual: {
				result &= (lValue != rValue);
				break;
			}
			case LessT: {
				result &= (lValue < rValue);
				break;
			}
			case GEqual: {
				result &= (lValue >= rValue);
				break;
			}
			case GreatT: {
				result &= (lValue > rValue);
				break;
			}
			case NO_OP: {
				break;
			}
			}
			break;
		}
		//float类型
		case floats: {
			float lValue = *(float *)leftValue;
			float rValue = *(float *)rightValue;
			switch (currentCon.compOp) {
			case EQual: {
				result &= floatEqual(lValue, rValue);
				break;
			}
			case LEqual: {
				result &= (floatEqual(lValue, rValue) | floatLess(lValue, rValue));
				break;
			}
			case NEqual: {
				result &= (!floatEqual(lValue, rValue));
				break;
			}
			case LessT: {
				result &= floatLess(lValue, rValue);
				break;
			}
			case GEqual: {
				result &= (floatEqual(lValue, rValue) | floatLess(rValue, lValue));
				break;
			}
			case GreatT: {
				result &= floatLess(rValue, lValue);
				break;
			}
			case NO_OP: {
				break;
			}
			}
			break;
		}
		//char类型
		case chars: {
			switch (currentCon.compOp) {
			case EQual: {
				result &= (strcmp(leftValue, rightValue) == 0);
				break;
			}
			case LEqual: {
				result &= (strcmp(leftValue, rightValue) <= 0);
				break;
			}
			case NEqual: {
				result &= (strcmp(leftValue, rightValue) != 0);
				break;
			}
			case LessT: {
				result &= (strcmp(leftValue, rightValue) < 0);
				break;
			}
			case GEqual: {
				result &= (strcmp(leftValue, rightValue) >= 0);
				break;
			}
			case GreatT: {
				result &= (strcmp(leftValue, rightValue) > 0);
				break;
			}
			case NO_OP: {
				break;
			}
			}
			break;
		}
		}

		if (currentCon.bLhsIsAttr == 1) {
			delete leftValue;
		}
		if (currentCon.bRhsIsAttr == 1) {
			delete rightValue;
		}
	}
	return result;
}

//判断是否存在下一个有效的RID
bool GetNextRID(RM_FileHandle *fileHandle, RID *lastRID, RID *nextRID) {
	auto pf_fileHandle = fileHandle->pf_fileHandle;
	auto allocatedMap = pf_fileHandle->pBitmap;

	int lastPagePos = lastRID->pageNum / 8;
	int lastPageInnerPos = lastRID->pageNum % 8;
	int lastSlotPos = lastRID->slotNum / 8;
	int lastSlotInnerPos = lastRID->slotNum % 8;

	int bitmapSize = (fileHandle->rm_fileSubHeader->recordsPerPage + 7) / 8;
	int allocateMapSize = (fileHandle->pf_fileHandle->pFileSubHeader->pageCount + 7) / 8;

	auto lastDataPage = new PF_PageHandle;
	char * src_data;
	GetThisPage(pf_fileHandle, lastRID->pageNum, lastDataPage);
	GetData(lastDataPage, &src_data);
	auto slotBitmap = src_data + sizeof(int);//上一个有效记录所在页面的位图

	//搜索同一byte位图控制的记录中是否存在有效记录
	char curBitByte = slotBitmap[lastSlotPos];
	for (int i = lastSlotInnerPos + 1; i < 8; i++) {
		char innerMask = (char)1 << i;
		if ((curBitByte & innerMask) != 0) {
			nextRID->bValid = true;
			nextRID->pageNum = lastRID->pageNum;
			nextRID->slotNum = lastSlotPos * 8 + i;
			UnpinPage(lastDataPage);
			delete lastDataPage;
			return true;
		}
	}

	//搜索当前页面中是否还存在下一个有效记录
	for (int i = lastSlotPos + 1; i < bitmapSize; i++) {
		curBitByte = slotBitmap[i];
		if (curBitByte == 0) {
			continue;
		}
		for (int j = 0; j < 8; j++) {
			char innerMask = (char)1 << j;
			if ((curBitByte & innerMask) != 0) {
				nextRID->bValid = true;
				nextRID->pageNum = lastRID->pageNum;
				nextRID->slotNum = i * 8 + j;
				UnpinPage(lastDataPage);
				delete lastDataPage;
				return true;
			}
		}
	}
	UnpinPage(lastDataPage);
	delete lastDataPage;

	//搜索同一byte控制的页面是否存在下一个有效记录
	char curAllocateBitByte = allocatedMap[lastPagePos];
	for (int i = lastPageInnerPos + 1; i < 8; i++) {
		char innerAllocateMask = (char)1 << i;
		if ((curAllocateBitByte & innerAllocateMask) != 0) { //寻找下一个存在记录的页面
			PageNum aimPageNum = lastPagePos * 8 + i; //可能的页面
			auto aimPageHandle = new PF_PageHandle;
			GetThisPage(fileHandle->pf_fileHandle, aimPageNum, aimPageHandle);
			char * page_data;
			GetData(aimPageHandle, &page_data); // 获取页面数据
			char * pageBitmap = page_data + sizeof(int);

			for (int j = 0; j < bitmapSize; j++) {
				curBitByte = pageBitmap[j];
				if (curBitByte == 0) {
					continue;
				}

				for (int k = 0; k < 8; k++) {
					char innerMask = (char)1 << k;
					if ((curBitByte & innerMask) != 0) {
						nextRID->bValid = true;
						nextRID->pageNum = aimPageNum;
						nextRID->slotNum = j * 8 + k;
						UnpinPage(aimPageHandle);
						delete aimPageHandle;
						return true;
					}
				}
			}
			UnpinPage(aimPageHandle);
			delete aimPageHandle;
		}
	}

	//搜索后序页面中是否存在下一个有效记录
	for (int i = lastPagePos + 1; i < allocateMapSize; i++) {
		curAllocateBitByte = allocatedMap[i];
		for (int j = 0; j < 8; j++) {
			char innerAllocateMask = (char)1 << j;
			if ((curAllocateBitByte & innerAllocateMask) != 0) {
				auto aimPageNum = (PageNum)(i * 8u + j);
				auto aimPageHandle = new PF_PageHandle;
				GetThisPage(fileHandle->pf_fileHandle, aimPageNum, aimPageHandle);
				char * page_data;
				GetData(aimPageHandle, &page_data);
				char * pageBitmap = page_data + sizeof(int);

				for (int k = 0; k < bitmapSize; k++) {
					curBitByte = pageBitmap[k];
					if (curBitByte == 0) {
						continue;
					} 

					for (int l = 0; l < 8; l++) {
						char innerMask = (char)1 << l;
						if ((curBitByte & innerMask) != 0) {
							nextRID->bValid = true;
							nextRID->pageNum = aimPageNum;
							nextRID->slotNum = k * 8 + l;
							UnpinPage(aimPageHandle);
							delete aimPageHandle;
							return true;
						}
					}
				}
				UnpinPage(aimPageHandle);
				delete aimPageHandle;
			}
		}
	}
	return false;
}

//检查指定记录是否存在且有效
RC RM_CheckWhetherRecordsExists(RM_FileHandle *fileHandle, RID rid, char ** data, PF_PageHandle ** src_pf_pageHandle) {
	PageNum aimPagePos = rid.pageNum;
	SlotNum aimSlotPos = rid.slotNum;

	//检查目标页是否已分配
	auto pf_pageHandle = *src_pf_pageHandle;
	RC openResult = GetThisPage(fileHandle->pf_fileHandle, aimPagePos, pf_pageHandle);
	if (openResult != SUCCESS) {
		return openResult;
	}

	//检查记录是否存在
	if (aimSlotPos >= fileHandle->rm_fileSubHeader->recordsPerPage) {
		return RM_INVALIDRID;
	}

	//检查记录是否有效
	char * pData;
	GetData(pf_pageHandle, &pData);
	char * theVeryMap = pData + sizeof(int) + (aimSlotPos / 8);
	char innerMask = (char)1 << (aimSlotPos % 8u);
	if ((*theVeryMap & innerMask) == 0) {
		return RM_INVALIDRID;
	}

	*data = pData;
	return SUCCESS;
}

//打开一个文件扫描
RC OpenScan(RM_FileScan *rmFileScan,RM_FileHandle *fileHandle,int conNum,Con *conditions)//初始化扫描
{
	if (fileHandle == NULL) {
		AfxMessageBox("未指定数据库");
		return RM_EOF;
	}
	rmFileScan->bOpen = true;
	rmFileScan->pRMFileHandle = fileHandle;
	rmFileScan->conNum = conNum;
	rmFileScan->conditions = conditions;

	char *pBitMap = fileHandle->pf_fileHandle->pBitmap;
	if (fileHandle->pf_fileHandle->pFileSubHeader->nAllocatedPages <= 2)
	{ //无数据页，即文件中没有存入任何记录
		rmFileScan->pn = 0;
		rmFileScan->sn = 0; //pn、sn等于0，表示文件中没有记录
	}
	else { //有数据页，寻找第一个被分配的数据页页号
		auto pageHandle = new PF_PageHandle();
		int i = 2;
		while (true)
		{
			char x = 1 << (i % 8);
			if ((*(pBitMap + i / 8) & x) != 0)
			{ //找到一个已分配页
				rmFileScan->pn = i;
				GetThisPage(fileHandle->pf_fileHandle, rmFileScan->pn, pageHandle);
				break;
			}
			else
			{
				i++;
			}
		}
		rmFileScan->sn = -1;
	}
	return SUCCESS;
}

//关闭一个扫描
RC CloseScan(RM_FileScan *rmFileScan)//关闭扫描
{
	if (rmFileScan == NULL) {
		printf("CloseScan rmFileScan is NULL, rmFileScan=%p\n", rmFileScan);
		return FAIL;
	}
	rmFileScan->bOpen = false;
	rmFileScan->pRMFileHandle = nullptr;
	rmFileScan->conNum = -1;
	rmFileScan->conditions = nullptr;
	rmFileScan->pn = 2;
	rmFileScan->sn = -1;
	return SUCCESS;
}

//获取一个满足条件的记录，并返回到rec(主要rid)
RC GetNextRec(RM_FileScan *rmFileScan,RM_Record *rec)
{

	if (!rmFileScan->bOpen) {
		printf("GetNextRec ,扫描未打开\n");
		return RM_FSCLOSED;
	}
	if (rmFileScan->pRMFileHandle->rm_fileSubHeader->nRecords == 0) {//记录数为0
		return RM_EOF;
	}
	auto rm_fileHandle = rmFileScan->pRMFileHandle;
	int recordSize = rm_fileHandle->rm_fileSubHeader->recordSize;
	int dataSize = recordSize - sizeof(RID) - sizeof(bool);
	RID lastRid, nextRid;
	lastRid.bValid = true;
	lastRid.pageNum = rmFileScan->pn;
	lastRid.slotNum = rmFileScan->sn;
	while (GetNextRID(rm_fileHandle, &lastRid, &nextRid)) {
		auto nextRecord = new RM_Record;
		GetRec(rm_fileHandle, &nextRid, nextRecord);
		if (checkConditions(rmFileScan, nextRecord)) {
			rmFileScan->pn = nextRid.pageNum;
			rmFileScan->sn = nextRid.slotNum;
			rec->bValid = true;
			rec->rid = nextRid;
			rec->pData = new char[dataSize];
			memcpy(rec->pData, nextRecord->pData, sizeof(char) * dataSize);
			return SUCCESS;
		}
		else {
			lastRid = nextRid;
		}
	}

	return RM_EOF;
}

//获取标识符为rid的记录到rec中
RC GetRec (RM_FileHandle *fileHandle,RID *rid, RM_Record *rec) 
{
	char * pData;
	auto pf_pageHandle = new PF_PageHandle;
	RC checkResult = RM_CheckWhetherRecordsExists(fileHandle, *rid, &pData, &pf_pageHandle);
	if (checkResult != SUCCESS) {
		delete pf_pageHandle;
		return checkResult;
	}

	int recordSize = fileHandle->rm_fileSubHeader->recordSize;
	int dataSize = recordSize - sizeof(bool) - sizeof(RID);
	int bitmapSize = (fileHandle->rm_fileSubHeader->recordsPerPage + 7) / 8;

	rec->bValid = true;
	rec->rid.bValid = true;
	rec->rid.pageNum = rid->pageNum;
	rec->rid.slotNum = rid->slotNum;
	rec->pData = new char[dataSize];
	memcpy(rec->pData, pData + sizeof(int) + bitmapSize + rid->slotNum * recordSize + sizeof(bool) + sizeof(RID), dataSize);

	delete pf_pageHandle;
	return SUCCESS;
}

//插入pData中的新记录，返回rid
RC InsertRec (RM_FileHandle *fileHandle,char *pData, RID *rid)
{
	int availablePageNum = fileHandle->pf_fileHandle->pFileSubHeader->nAllocatedPages;//已分配页面数
	char *allocatedBitmap = fileHandle->pf_fileHandle->pBitmap;//页面信息位图，0表示空闲页，1表示已分配
	char *fullBitmap = fileHandle->rBitmap;//记录信息位图，满页为1，非满页为0
	int curPos = 0;
	PageNum newRecordPagePos = 0;
	bool needNewPage = true;//标记是否需要划分新页

	//检查是否需要新页
	while (availablePageNum != 0) {
		int bitmapCount = count_bit_set((unsigned char)allocatedBitmap[curPos]);
		if (allocatedBitmap[curPos] != fullBitmap[curPos]) {
			char diffMap = allocatedBitmap[curPos] ^ fullBitmap[curPos];
			newRecordPagePos = curPos * 8u + least_significant_bit_pos((unsigned char)diffMap);
			needNewPage = false;
			break;
		}
		curPos++;
		availablePageNum -= bitmapCount;
	}

	auto pf_pageHandle = new PF_PageHandle;
	pf_pageHandle->bOpen = true;
	int maxRecordsPerPage = fileHandle->rm_fileSubHeader->recordsPerPage;//每个页面可装载的记录数
	int bitmapSize = (maxRecordsPerPage + 7) / 8;

	//获取存放新记录的页面
	if (needNewPage) {
		char *tmp;
		AllocatePage(fileHandle->pf_fileHandle, pf_pageHandle);
		GetPageNum(pf_pageHandle, &newRecordPagePos);
		GetData(pf_pageHandle, &tmp);
		memset(tmp, 0, PF_PAGESIZE);
	}
	else {
		GetThisPage(fileHandle->pf_fileHandle, newRecordPagePos, pf_pageHandle);
	}

	//获取存放新记录的记录插槽
	char *dst_data;
	GetData(pf_pageHandle, &dst_data);
	SlotNum newRecordPos = -1;
	int recordsNum = (int)dst_data[0];
	auto recordsMap = dst_data + sizeof(int);

	for (int i = 0; i < bitmapSize; i++) {
		if (recordsMap[i] != (char)0xff) {
			int offset = least_significant_bit_pos((unsigned char)~recordsMap[i]);
			newRecordPos = i * 8u + offset;
			recordsMap[i] |= (1 << offset);
			break;
		}
	}

	rid->bValid = true;
	rid->pageNum = newRecordPagePos;
	rid->slotNum = newRecordPos;

	//存入记录，修改相应控制变量
	int recordSize = fileHandle->rm_fileSubHeader->recordSize;
	int dataSize = recordSize - sizeof(bool) - sizeof(RID);
	auto newRecord = (RM_Record *)(dst_data + sizeof(int) + sizeof(char) * bitmapSize + sizeof(char) * recordSize * newRecordPos);
	newRecord->bValid = true;
	newRecord->rid.bValid = true;
	newRecord->rid.pageNum = newRecordPagePos;
	newRecord->rid.slotNum = newRecordPos;
	memcpy(dst_data + sizeof(int) + sizeof(char) * bitmapSize + sizeof(char) * recordSize * newRecordPos + sizeof(bool) + sizeof(RID),
		pData, sizeof(char) * dataSize);
	recordsNum++;

	memcpy(dst_data, &recordsNum, sizeof(int));

	MarkDirty(pf_pageHandle);
	UnpinPage(pf_pageHandle);

	int fullMapPos = -1;
	if (recordsNum == maxRecordsPerPage) {
		fullMapPos = newRecordPagePos / 8u;
		char fullMapMask = (char)(1 << (newRecordPagePos % 8u));
		fileHandle->rBitmap[fullMapPos] |= fullMapMask;
	}

	fileHandle->rm_fileSubHeader->nRecords++;
	auto rm_header_page = new PF_PageHandle;
	GetThisPage(fileHandle->pf_fileHandle, 1, rm_header_page);
	MarkDirty(rm_header_page);
	UnpinPage(rm_header_page);

	delete pf_pageHandle;
	delete rm_header_page;
	return SUCCESS;
}

//删除rid指向的记录
RC DeleteRec (RM_FileHandle *fileHandle,const RID *rid)
{
	char * src_data;
	auto pf_pageHandle = new PF_PageHandle;
	RC checkResult = RM_CheckWhetherRecordsExists(fileHandle, *rid, &src_data, &pf_pageHandle);
	if (checkResult != SUCCESS) {
		delete pf_pageHandle;
		return checkResult;
	}

	int recordsNum = -1;
	memcpy(&recordsNum, src_data, sizeof(int));
	recordsNum--;
	bool shouldDeleteFull = recordsNum == fileHandle->rm_fileSubHeader->recordsPerPage - 1;//判断满页标志
	bool shouldDelete = recordsNum == 0;//判断空闲页标志

	if (!shouldDelete) {
		char * theVeryMap = src_data + sizeof(int) + (rid->slotNum / 8);
		char innerMask = (char)1 << (rid->slotNum % 8u);
		*theVeryMap &= ~innerMask;
		memcpy(src_data, &recordsNum, sizeof(int));
	}
	else {
		DisposePage(fileHandle->pf_fileHandle, rid->pageNum);//若当前页面无记录，则丢弃当前页面
	}

	auto rm_headerPage = new PF_PageHandle;
	GetThisPage(fileHandle->pf_fileHandle, 1, rm_headerPage);
	fileHandle->rm_fileSubHeader->nRecords--;

	if (shouldDeleteFull) {
		char * theVeryMap = fileHandle->rBitmap + (rid->pageNum / 8) * sizeof(char);
		char innerMask = (char)1 << (rid->pageNum % 8u);
		*theVeryMap &= ~innerMask;
	}

	MarkDirty(rm_headerPage);
	UnpinPage(rm_headerPage);
	MarkDirty(pf_pageHandle);
	UnpinPage(pf_pageHandle);

	delete pf_pageHandle;
	delete rm_headerPage;
	return SUCCESS;
	return SUCCESS;
}

//更新记录
RC UpdateRec (RM_FileHandle *fileHandle,const RM_Record *rec)
{
	char * pData;
	auto pf_fileHandle = new PF_PageHandle();
	RC checkResult = RM_CheckWhetherRecordsExists(fileHandle, rec->rid, &pData, &pf_fileHandle);
	if (checkResult != SUCCESS) {
		return checkResult;
	}

	int recordSize = fileHandle->rm_fileSubHeader->recordSize;
	int dataSize = recordSize - sizeof(bool) - sizeof(RID);
	int bitmapSize = (fileHandle->rm_fileSubHeader->recordsPerPage + 7) / 8;
	memcpy(pData + sizeof(int) + bitmapSize + recordSize * rec->rid.slotNum + sizeof(bool) + sizeof(RID),
		rec->pData, dataSize);

	MarkDirty(pf_fileHandle);
	UnpinPage(pf_fileHandle);
	delete pf_fileHandle;
	return SUCCESS;
}

//创建记录文件
RC RM_CreateFile (char *fileName, int recordSize)
{
	auto pf_fileHandle = new PF_FileHandle();
	RC retCode = CreateFile(fileName);
	if (retCode != SUCCESS) {
		printf("RM_CreateFile CreateFile, retCode=%d, fileName=%s, recordSize=%d\n", retCode, fileName, recordSize);
		delete pf_fileHandle;
		return PF_FILEERR;
	}
	
	retCode = OpenFile(fileName, pf_fileHandle);//打开一个分页文件
	if (retCode != SUCCESS) {
		printf("RM_CreateFile OpenFile, retCode=%d, fileName=%s, recordSize=%d\n", retCode, fileName, recordSize);
		delete pf_fileHandle;
		return PF_FILEERR;
	}
	RM_FileSubHeader rm_fileSubHeader;
	rm_fileSubHeader.nRecords = 0;
	rm_fileSubHeader.recordSize = recordSize + sizeof(RID) + sizeof(bool);
	rm_fileSubHeader.recordsPerPage = (PF_PAGE_SIZE - sizeof(int)) / (rm_fileSubHeader.recordSize + 1);
	
	auto pf_pageHandle = new PF_PageHandle();
	AllocatePage(pf_fileHandle, pf_pageHandle);//在指定文件中分配一个新的页面，并将其放入缓冲区，返回页面句柄指针
	pf_pageHandle->bOpen = true;
	char *pData;
	char bitMap = 0x3;//0,1两页存放控制信息，算作满页
	GetData(pf_pageHandle, &pData);
	memcpy(pData,&rm_fileSubHeader, sizeof(RM_FileSubHeader));//存放记录文件控制信息
	memcpy(pData + sizeof(RM_FileSubHeader), &bitMap, sizeof(char));//存放位图信息
	MarkDirty(pf_pageHandle);//标记为脏页
	UnpinPage(pf_pageHandle);//解除驻留缓冲区限制
	CloseFile(pf_fileHandle);
	delete pf_fileHandle;
	delete pf_pageHandle;
	return SUCCESS;
}

//打开记录文件，返回句柄指针
RC RM_OpenFile(char *fileName, RM_FileHandle *fileHandle)
{
	auto pf_fileHandle = new PF_FileHandle();
	RC retCode = OpenFile(fileName, pf_fileHandle);//打开一个分页文件
	if (retCode != SUCCESS) {
		printf("RM_OpenFile OpenFile, retCode=%d, fileName=%s, fileHandle=%p\n", retCode, fileName, fileHandle);
		delete pf_fileHandle;
		return PF_FILEERR;
	}
	auto pf_pageHandle = new PF_PageHandle();
	GetThisPage(pf_fileHandle, 1, pf_pageHandle);//获取指定页面到缓冲区
	fileHandle->bOpen = true;
	char *pData;
	GetData(pf_pageHandle, &pData);//获取数据区指针
	fileHandle->rm_fileSubHeader = (RM_FileSubHeader *)pData;
	fileHandle->pf_fileHandle = pf_fileHandle;
	fileHandle->rBitmap = pData + sizeof(RM_FileSubHeader);
	delete pf_pageHandle;
	return SUCCESS;
}

//关闭句柄对应的记录文件
RC RM_CloseFile(RM_FileHandle *fileHandle)
{
	if (fileHandle == NULL) {
		printf("RM_CloseFile fileHandle is NULL, fileHandle=%p\n", fileHandle);
		return FAIL;
	}
	CloseFile(fileHandle->pf_fileHandle);
	fileHandle->bOpen = false;
	return SUCCESS;
}
