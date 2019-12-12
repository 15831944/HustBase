#include "stdafx.h"
#include "IX_Manager.h"

RC CreateIndex(const char* fileName, AttrType attrType, int attrLength) {
	auto pf_fileHandle = new PF_FileHandle();
	RC retCode = CreateFile(fileName);
	if (retCode != SUCCESS) {
		printf("CreateIndex CreateFile, retCode=%d, fileName=%s, attrType=%d, attrLength=%d", retCode, fileName, attrType, attrLength);
		delete pf_fileHandle;
		return retCode;
	}

	retCode = OpenFile((char*)fileName, pf_fileHandle);
	if (retCode != SUCCESS) {
		printf("CreateIndex OpenFile, retCode=%d, fileName=%s, attrType=%d, attrLength=%d", retCode, fileName, attrType, attrLength);
		delete pf_fileHandle;
		return retCode;
	}
	
	int keys_size = (PF_PAGE_SIZE - sizeof(IX_FileHeader) - sizeof(IX_Node)) / (2 * sizeof(RID) + attrLength);

	auto ix_fileHeader = new IX_FileHeader();
	ix_fileHeader->attrLength = attrLength;
	ix_fileHeader->keyLength = attrLength + sizeof(RID);
	ix_fileHeader->attrType = attrType;
	ix_fileHeader->rootPage = 1;
	ix_fileHeader->first_leaf = 1;
	ix_fileHeader->order = keys_size;

	auto ix_node = new IX_Node();
	ix_node->is_leaf = true;
	ix_node->keynum = 0;
	ix_node->parent = 0;
	ix_node->brother = 0;

	auto pf_pageHandle = new PF_PageHandle();
	pf_pageHandle->bOpen = true;

	AllocatePage(pf_fileHandle, pf_pageHandle);
	char* dst_data;
	GetData(pf_pageHandle, &dst_data);
	memcpy(dst_data, ix_fileHeader, sizeof(IX_FileHeader));

	AllocatePage(pf_fileHandle, pf_pageHandle);
	GetData(pf_pageHandle, &dst_data);
	memcpy(dst_data + sizeof(IX_FileHeader), ix_node, sizeof(IX_Node));
	MarkDirty(pf_pageHandle);
	UnpinPage(pf_pageHandle);

	CloseFile(pf_fileHandle);
	delete pf_fileHandle;
	delete pf_pageHandle;
	delete ix_fileHeader;
	delete ix_node;
	return SUCCESS;
}



RC OpenIndex(const char* fileName, IX_IndexHandle* indexHandle) {

	if (OpenFile((char*)fileName, &indexHandle->fileHandle) != SUCCESS) {
		return PF_FILEERR;
	}
	PF_PageHandle pf_pageHandle;
	GetThisPage(&indexHandle->fileHandle, 1, &pf_pageHandle);
	char* src_data;
	GetData(&pf_pageHandle, &src_data);
	memcpy(&indexHandle->fileHeader, src_data, sizeof(IX_FileHeader));

	indexHandle->bOpen = true;
	UnpinPage(&pf_pageHandle);
	return SUCCESS;
}

RC CloseIndex(IX_IndexHandle* indexHandle) {
	if (indexHandle == NULL) {
		printf("CloseIndex indexHandle is NULL, indexHandle=%p", indexHandle);
		return FAIL;
	}

	CloseFile(&indexHandle->fileHandle);
	indexHandle->bOpen = false;
	return SUCCESS;
}


RC InsertEntry(IX_IndexHandle* indexHandle, void* pData, const RID* rid) {

}

RC DeleteEntry(IX_IndexHandle* indexHandle, void* pData, const RID* rid) {

}



RC OpenIndexScan(IX_IndexScan *indexScan,IX_IndexHandle *indexHandle,CompOp compOp,char *value){
	return SUCCESS;
}

RC IX_GetNextEntry(IX_IndexScan *indexScan,RID * rid){
	return SUCCESS;
}

RC CloseIndexScan(IX_IndexScan *indexScan){
		return SUCCESS;
}

RC GetIndexTree(char *fileName, Tree *index){
		return SUCCESS;
}











