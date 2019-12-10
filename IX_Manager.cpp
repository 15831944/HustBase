#include "stdafx.h"
#include "IX_Manager.h"

RC CreateIndex(const char* fileName, AttrType attrType, int attrLength){
	auto pf_fileHandle = new PF_FileHandle();
	RC createFileCode = CreateFile(fileName);
	if (createFileCode != SUCCESS) {
		delete pf_fileHandle;
		return createFileCode;
	}
	OpenFile((char*)fileName, pf_fileHandle);
}

RC OpenIndex(const char* fileName, IX_IndexHandle* indexHandle){

}

RC CloseIndex(IX_IndexHandle* indexHandle) {

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


