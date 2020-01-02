#include "StdAfx.h"
#include "QU_Manager.h"

void Init_Result(SelResult* res) {
	res->next_res = NULL;
}

void Destory_Result(SelResult* res) {
	for (int i = 0; i < res->row_num; i++) {
		//for (int j = 0; j < res->col_num - 1; j++) {
			//delete[] res->res[i][j];
		//}
		delete[] res->res[i][0];
		delete[] res->res[i];
	}
	if (res->next_res != NULL) {
		Destory_Result(res->next_res);
	}
}

RC Query(char* sql, SelResult* res) {
	RC tempRc;
	sqlstr* tempSqlType = get_sqlstr();
	tempRc = parse(sql, tempSqlType);

	if (tempRc != SUCCESS)
	{
		return tempRc;
	}
	else
	{
		tempRc = Select(tempSqlType->sstr.sel.nSelAttrs, tempSqlType->sstr.sel.selAttrs, tempSqlType->sstr.sel.nRelations,
			tempSqlType->sstr.sel.relations, tempSqlType->sstr.sel.nConditions, tempSqlType->sstr.sel.conditions, res);

		if (tempRc != SUCCESS)return tempRc;
	}
	return SUCCESS;
}

RC Select(int nSelAttrs, RelAttr** selAttrs, int nRelations, char** relations, int nConditions, Condition* conditions, SelResult* res)
{
	RC tempRc;
	tempRc = tableExist(nRelations, relations);
	if (tempRc != SUCCESS)
	{
		//AfxMessageBox("Table Is Not Exist!");
		return tempRc;
	}
	if (nRelations == 1)
	{//single table
		if (nConditions == 0)
		{//no condition
			singleNoCondition(nSelAttrs, selAttrs, nRelations, relations, nConditions, conditions, res);
		}
		else
		{
			singleWithCondition(nSelAttrs, selAttrs, nRelations, relations, nConditions, conditions, res);
		}
	}
	else
	{//multi table
		multiSelect(nSelAttrs, selAttrs, nRelations, relations, nConditions, conditions, res);
	}

	return SUCCESS;
}

RC tableExist(int nRelations, char** relations)
{
	RC tempRc;
	RM_FileHandle* tempFileHandle = (RM_FileHandle*)malloc(sizeof(RM_FileHandle));
	tempFileHandle->bOpen = false;
	RM_FileScan* tempFileScan = (RM_FileScan*)malloc(sizeof(RM_FileScan));
	tempFileScan->bOpen = false;
	RM_Record* tempRec = (RM_Record*)malloc(sizeof(RM_Record));
	tempRec->bValid = false;
	Con tempCon;

	tempCon.bLhsIsAttr = 1;
	tempCon.attrType = chars;
	tempCon.bRhsIsAttr = 0;
	tempCon.LattrOffset = 0;
	tempCon.compOp = EQual;

	tempRc = RM_OpenFile("SYSTABLES", tempFileHandle);
	if (tempRc != SUCCESS)return tempRc;

	for (int i = 0; i < nRelations; i++)
	{
		tempCon.Rvalue = relations[i];
		tempCon.LattrLength = strlen(relations[i]) + 1;

		tempRc = OpenScan(tempFileScan, tempFileHandle, 1, &tempCon);
		if (tempRc != SUCCESS)
			break;

		tempRc = GetNextRec(tempFileScan, tempRec);
		if (tempRc != SUCCESS)
		{
			tempRc = TABLE_NOT_EXIST;
			break;
		}
		CloseScan(tempFileScan);
	}
	if (tempFileScan->bOpen)
	{
		CloseScan(tempFileScan);
	}
	RM_CloseFile(tempFileHandle);
	free(tempFileScan);
	free(tempFileHandle);

	return tempRc;
}

RC singleNoCondition(int nSelAttrs, RelAttr** selAttrs, int nRelations, char** relations, int nConditions, Condition* conditions, SelResult* res)
{
	RC tempRc;
	//tempRc = tableExist(1, relations);
	//if (tempRc != SUCCESS)return tempRc;
	SelResult* resHead = res;
	RM_FileHandle* tempFileHandle = NULL;
	RM_FileScan* tempFileScan = NULL;
	RM_Record* tempRec = NULL;
	Con tempCon[2];

	if (false)
	{  //has index

	}
	else
	{
		tempCon[0].attrType = chars;
		tempCon[0].bLhsIsAttr = 1;
		tempCon[0].LattrOffset = 0;
		tempCon[0].LattrLength = strlen(*relations) + 1;
		tempCon[0].compOp = EQual;
		tempCon[0].bRhsIsAttr = 0;
		tempCon[0].Rvalue = *relations;

		if (nSelAttrs == 1 && !strcmp((*selAttrs)->attrName, "*"))
		{ //select all
			tempFileHandle = (RM_FileHandle*)malloc(sizeof(RM_FileHandle));
			tempFileHandle->bOpen = false;
			tempRc = RM_OpenFile("SYSTABLES", tempFileHandle);
			if (tempRc != SUCCESS) return tempRc;

			tempFileScan = (RM_FileScan*)malloc(sizeof(RM_FileScan));
			tempFileScan->bOpen = false;
			tempRec = (RM_Record*)malloc(sizeof(RM_Record));
			tempRec->bValid = false;

			OpenScan(tempFileScan, tempFileHandle, 1, tempCon);
			tempRc = GetNextRec(tempFileScan, tempRec);
			if (tempRc != SUCCESS) return tempRc;

			//get num of table's property
			SysTable* table = (SysTable*)tempRec->pData;
			memcpy(&(resHead->col_num), tempRec->pData + 21, sizeof(int));

			CloseScan(tempFileScan);
			RM_CloseFile(tempFileHandle);

			//get type of property
			tempRc = RM_OpenFile("SYSCOLUMNS", tempFileHandle);
			if (tempRc != SUCCESS) return tempRc;

			OpenScan(tempFileScan, tempFileHandle, 1, tempCon);

			for (int i = 0; i < resHead->col_num; i++)
			{
				tempRc = GetNextRec(tempFileScan, tempRec);
				if (tempRc != SUCCESS) return tempRc;

				char* column = tempRec->pData;
				memcpy(&resHead->type[i], column + 42, sizeof(int));
				memcpy(&resHead->fields[i], column + 21, 21);
				memcpy(&resHead->length[i], column + 46, sizeof(int));
				memcpy(&resHead->offset[i], column + 50, sizeof(int));

			}
			CloseScan(tempFileScan);
		}
		else
		{ //not select all
			resHead->col_num = nSelAttrs;  //set column of result
			tempFileHandle = (RM_FileHandle*)malloc(sizeof(RM_FileHandle));
			tempFileHandle->bOpen = false;
			tempFileScan = (RM_FileScan*)malloc(sizeof(RM_FileScan));
			tempFileScan->bOpen = false;
			tempRec = (RM_Record*)malloc(sizeof(RM_Record));
			tempRec->bValid = false;

			tempCon[1].attrType = chars;
			tempCon[1].bLhsIsAttr = 1;
			tempCon[1].LattrOffset = 21;
			tempCon[1].LattrLength = 21;
			tempCon[1].compOp = EQual;
			tempCon[1].bRhsIsAttr = 0;

			tempRc = RM_OpenFile("SYSCOLUMNS", tempFileHandle);
			if (tempRc != SUCCESS)return tempRc;

			//get information of property
			for (int i = 0; i < resHead->col_num; i++)
			{
				tempCon[1].Rvalue = (selAttrs[i])->attrName;//get name of property
				OpenScan(tempFileScan, tempFileHandle, 2, tempCon);
				tempRc = GetNextRec(tempFileScan, tempRec);
				if (tempRc != SUCCESS)return tempRc;

				char* column = tempRec->pData;

				memcpy(&resHead->fields[i], column + 21, 21);
				memcpy(&resHead->type[i], column + 42, sizeof(int));
				memcpy(&resHead->length[i], column + 46, sizeof(int));
				memcpy(&resHead->offset[i], column + 50, sizeof(int));
				CloseScan(tempFileScan);
			}
		}

		//scan table file to find record
		RM_CloseFile(tempFileHandle);
		free(tempFileHandle);
		tempFileHandle = (RM_FileHandle*)malloc(sizeof(RM_FileHandle));
		tempFileHandle->bOpen = false;
		tempRc = RM_OpenFile(*relations, tempFileHandle);
		if (tempRc != SUCCESS) return tempRc;

		OpenScan(tempFileScan, tempFileHandle, 0, NULL);

		int i = 0;
		resHead->row_num = 0;
		SelResult* curRes = resHead;
		while (GetNextRec(tempFileScan, tempRec) == SUCCESS)
		{
			if (curRes->row_num >= 100) //100 records per node
			{ //build new node
				curRes->next_res = (SelResult*)malloc(sizeof(SelResult));
				curRes->next_res->col_num = curRes->col_num;
				for (int j = 0; j < curRes->col_num; j++)
				{
					strncpy(curRes->next_res->fields[i], curRes->fields[i], strlen(curRes->fields[i]));
					curRes->next_res->type[i] = curRes->type[i];
					curRes->next_res->offset[i] = curRes->offset[i];
				}

				curRes = curRes->next_res;
				curRes->next_res = NULL;
				curRes->row_num = 0;
			}

			curRes->res[curRes->row_num] = (char**)malloc(sizeof(char*));
			*(curRes->res[curRes->row_num++]) = tempRec->pData;
		}

		CloseScan(tempFileScan);
		free(tempFileScan);
		RM_CloseFile(tempFileHandle);
		free(tempFileHandle);
		free(tempRec);
	}
	res = resHead;
	return SUCCESS;
}

RC singleWithCondition(int nSelAttrs, RelAttr** selAttrs, int nRelations, char** relations, int nConditions, Condition* conditions, SelResult* res)
{
	RC tempRc;
	//tempRc = tableExist(1, relations);
	//if (tempRc != SUCCESS)return tempRc;
	SelResult* resHead = res;

	if (false)
	{//index

	}
	else
	{

		RM_FileHandle* tempFileHandle = (RM_FileHandle*)malloc(sizeof(RM_FileHandle));
		tempFileHandle->bOpen = false;
		RM_FileScan* tempFileScan = (RM_FileScan*)malloc(sizeof(RM_FileScan));
		tempFileScan->bOpen = false;
		RM_Record* tempRec = (RM_Record*)malloc(sizeof(RM_Record));
		tempRec->bValid = false;
		Con tempCon[2];

		tempCon[0].attrType = chars;
		tempCon[0].bLhsIsAttr = 1;
		tempCon[0].LattrOffset = 0;
		tempCon[0].LattrLength = strlen(*relations) + 1;
		tempCon[0].compOp = EQual;
		tempCon[0].bRhsIsAttr = 0;
		tempCon[0].Rvalue = *relations;

		if (nSelAttrs == 1 && !strcmp((*selAttrs)->attrName, "*"))
		{ //select all
			tempRc = RM_OpenFile("SYSTABLES", tempFileHandle);
			if (tempRc != SUCCESS) return tempRc;

			OpenScan(tempFileScan, tempFileHandle, 1, tempCon);
			tempRc = GetNextRec(tempFileScan, tempRec);
			if (tempRc != SUCCESS) return tempRc;

			//get num of table's property
			SysTable* table = (SysTable*)tempRec->pData;
			memcpy(&(resHead->col_num), tempRec->pData + 21, sizeof(int));

			CloseScan(tempFileScan);
			RM_CloseFile(tempFileHandle);

			//get type of property
			tempRc = RM_OpenFile("SYSCOLUMNS", tempFileHandle);
			if (tempRc != SUCCESS) return tempRc;

			OpenScan(tempFileScan, tempFileHandle, 1, tempCon);

			for (int i = 0; i < resHead->col_num; i++)
			{
				tempRc = GetNextRec(tempFileScan, tempRec);
				if (tempRc != SUCCESS) return tempRc;

				char* column = tempRec->pData;
				memcpy(&resHead->type[i], column + 42, sizeof(int));
				memcpy(&resHead->fields[i], column + 21, 21);
				memcpy(&resHead->length[i], column + 46, sizeof(int));
				memcpy(&resHead->offset[i], column + 50, sizeof(int));

			}
			CloseScan(tempFileScan);
		}
		else
		{ //not select all
			resHead->col_num = nSelAttrs;  //set column of result

			tempCon[1].attrType = chars;
			tempCon[1].bLhsIsAttr = 1;
			tempCon[1].LattrOffset = 21;
			tempCon[1].LattrLength = 21;
			tempCon[1].compOp = EQual;
			tempCon[1].bRhsIsAttr = 0;

			tempRc = RM_OpenFile("SYSCOLUMNS", tempFileHandle);
			if (tempRc != SUCCESS)return tempRc;

			//get information of property
			for (int i = 0; i < resHead->col_num; i++)
			{
				tempCon[1].Rvalue = (selAttrs[i])->attrName;//get name of property
				OpenScan(tempFileScan, tempFileHandle, 2, tempCon);
				tempRc = GetNextRec(tempFileScan, tempRec);
				if (tempRc != SUCCESS)return tempRc;

				char* column = tempRec->pData;

				memcpy(&resHead->fields[i], column + 21, 21);
				memcpy(&resHead->type[i], column + 42, sizeof(int));
				memcpy(&resHead->length[i], column + 46, sizeof(int));
				memcpy(&resHead->offset[i], column + 50, sizeof(int));
				CloseScan(tempFileScan);
			}
		}
		RM_CloseFile(tempFileHandle);
		free(tempFileHandle);
		tempFileHandle = (RM_FileHandle*)malloc(sizeof(RM_FileHandle));
		tempFileHandle->bOpen = false;

		//init select condition
		tempRc = RM_OpenFile("SYSCOLUMNS", tempFileHandle);
		if (tempRc != SUCCESS)return tempRc;

		tempCon[1].attrType = chars;
		tempCon[1].bLhsIsAttr = 1;
		tempCon[1].LattrOffset = 21;
		tempCon[1].LattrLength = 21;
		tempCon[1].compOp = EQual;
		tempCon[1].bRhsIsAttr = 0;

		Con* selectCon = (Con*)malloc(sizeof(Con) * nConditions);
		for (int i = 0; i < nConditions; i++)
		{
			if (conditions[i].bLhsIsAttr == 0 && conditions[i].bRhsIsAttr == 1)
			{
				tempCon[1].Rvalue = conditions[i].rhsAttr.attrName;
			}
			else if (conditions[i].bLhsIsAttr == 1 && conditions[i].bRhsIsAttr == 0)
			{
				tempCon[1].Rvalue = conditions[i].lhsAttr.attrName;
			}

			OpenScan(tempFileScan, tempFileHandle, 2, tempCon);
			tempRc = GetNextRec(tempFileScan, tempRec);
			if (tempRc != SUCCESS) return tempRc;

			selectCon[i].bLhsIsAttr = conditions[i].bLhsIsAttr;
			selectCon[i].bRhsIsAttr = conditions[i].bRhsIsAttr;
			selectCon[i].compOp = conditions[i].op;
			if (conditions[i].bLhsIsAttr == 1)
			{
				memcpy(&selectCon[i].LattrLength, tempRec->pData + 46, sizeof(int));
				memcpy(&selectCon[i].LattrOffset, tempRec->pData + 50, sizeof(int));
			}
			else
			{
				selectCon[i].attrType = conditions[i].lhsValue.type;
				selectCon[i].Lvalue = conditions[i].lhsValue.data;
			}

			if (conditions[i].bRhsIsAttr == 1)
			{
				memcpy(&selectCon[i].RattrLength, tempRec->pData + 46, sizeof(int));
				memcpy(&selectCon[i].RattrOffset, tempRec->pData + 50, sizeof(int));
			}
			else
			{
				selectCon[i].attrType = conditions[i].rhsValue.type;
				selectCon[i].Rvalue = conditions[i].rhsValue.data;
			}
			CloseScan(tempFileScan);
		}
		RM_CloseFile(tempFileHandle);

		tempRc = RM_OpenFile(*relations, tempFileHandle);
		if (tempRc != SUCCESS)return tempRc;
		//use getNextRec to find data 
		OpenScan(tempFileScan, tempFileHandle, nConditions, selectCon);

		int i = 0;
		resHead->row_num = 0;
		SelResult* curRes = resHead;
		while (GetNextRec(tempFileScan, tempRec) == SUCCESS)
		{
			if (curRes->row_num >= 100) //100 records per node
			{ //build new node
				curRes->next_res = (SelResult*)malloc(sizeof(SelResult));
				curRes->next_res->col_num = curRes->col_num;
				for (int j = 0; j < curRes->col_num; j++)
				{
					strncpy(curRes->next_res->fields[i], curRes->fields[i], strlen(curRes->fields[i]));
					curRes->next_res->type[i] = curRes->type[i];
					curRes->next_res->offset[i] = curRes->offset[i];
				}

				curRes = curRes->next_res;
				curRes->next_res = NULL;
				curRes->row_num = 0;
			}

			curRes->res[curRes->row_num] = (char**)malloc(sizeof(char*));
			*(curRes->res[curRes->row_num++]) = tempRec->pData;
		}

		CloseScan(tempFileScan);
		free(tempFileScan);
		RM_CloseFile(tempFileHandle);
		free(tempFileHandle);
		free(tempRec);
	}
	res = resHead;
	return SUCCESS;
}

RC fillCondition(int nRelations, char** relations, int nConditions, Condition* conditions)
{
	RC tempRc;
	RM_FileHandle* tempFileHandle = (RM_FileHandle*)malloc(sizeof(RM_FileHandle));
	tempFileHandle->bOpen = false;
	RM_FileScan* tempFileScan = (RM_FileScan*)malloc(sizeof(RM_FileScan));
	tempFileScan->bOpen = false;
	RM_Record* tempRec = (RM_Record*)malloc(sizeof(RM_Record));
	tempRec->bValid = false;
	Con tempCon[2];
	tempCon[0].attrType = chars;
	tempCon[0].bLhsIsAttr = 1;
	tempCon[0].LattrOffset = 0;
	tempCon[0].LattrLength = 21;
	tempCon[0].bRhsIsAttr = 0;
	tempCon[0].compOp = EQual;

	tempCon[1].attrType = chars;
	tempCon[1].bLhsIsAttr = 1;
	tempCon[1].LattrLength = 21;
	tempCon[1].LattrOffset = 21;
	tempCon[1].bRhsIsAttr = 0;
	tempCon[1].compOp = EQual;

	tempRc = RM_OpenFile("SYSCOLUMNS", tempFileHandle);
	if (tempRc != SUCCESS)return tempRc;
	for (int i = 0; i < nConditions; i++)
	{
		if (conditions[i].bLhsIsAttr == 1 && conditions[i].lhsAttr.relName == NULL)
		{
			tempCon[1].Rvalue = conditions[i].lhsAttr.attrName;
			for (int j = 0; j < nRelations; j++)
			{
				tempCon[0].Rvalue = relations[j];
				OpenScan(tempFileScan, tempFileHandle, 2, tempCon);
				tempRc = GetNextRec(tempFileScan, tempRec);
				if (tempRc == SUCCESS)
				{
					conditions[i].lhsAttr.relName = relations[j];
					CloseScan(tempFileScan);
					break;
				}
				CloseScan(tempFileScan);
			}

		}
		else if (conditions[i].bRhsIsAttr == 1 && conditions[i].rhsAttr.relName == NULL)
		{
			tempCon[1].Rvalue = conditions[i].rhsAttr.attrName;
			for (int j = 0; j < nRelations; j++)
			{
				tempCon[0].Rvalue = relations[j];
				OpenScan(tempFileScan, tempFileHandle, 2, tempCon);
				tempRc = GetNextRec(tempFileScan, tempRec);
				if (tempRc == SUCCESS)
				{
					conditions[i].rhsAttr.relName = relations[j];
					CloseScan(tempFileScan);
					break;
				}
				CloseScan(tempFileScan);
			}
		}
	}
	RM_CloseFile(tempFileHandle);
	free(tempFileHandle);
	free(tempFileScan);
	free(tempRec);
	return SUCCESS;
}

RC multiSelect(int nSelAttrs, RelAttr** selAttrs, int nRelations, char** relations, int nConditions, Condition* conditions, SelResult* res)
{
	RC tempRc;
	SelResult* resHead = res;

	if (false)
	{//have index

	}
	else
	{
		//select all with condition
		int selectAllNum;
		RelAttr* selectAllName;

		RM_FileHandle* tempFileHandle = (RM_FileHandle*)malloc(sizeof(RM_FileHandle));
		tempFileHandle->bOpen = false;
		RM_FileScan* tempFileScan = (RM_FileScan*)malloc(sizeof(RM_FileScan));
		tempFileScan->bOpen = false;
		RM_FileHandle** allFileHandle = (RM_FileHandle**)malloc(sizeof(RM_FileHandle) * nRelations);
		for (int i = 0; i < nRelations; i++)
		{
			allFileHandle[i] = (RM_FileHandle*)malloc(sizeof(RM_FileHandle));
			allFileHandle[i]->bOpen = false;
			tempRc = RM_OpenFile(relations[nRelations - i - 1], allFileHandle[i]);
			if (tempRc != SUCCESS)return tempRc;
		}

		int* offset = (int*)malloc(sizeof(int) * (nRelations + 1));

		offset[0] = 0;
		for (int i = 1; i <= nRelations; i++)
		{
			offset[i] = offset[i - 1] + allFileHandle[i - 1]->rm_fileSubHeader->recordSize;
		}
		RM_Record* tempRec = (RM_Record*)malloc(sizeof(RM_Record));
		tempRec->bValid = false;
		Con tempCon[2];
		tempCon[0].attrType = chars;
		tempCon[0].bLhsIsAttr = 1;
		tempCon[0].LattrOffset = 0;
		tempCon[0].LattrLength = 21;
		tempCon[0].bRhsIsAttr = 0;
		tempCon[0].compOp = EQual;

		if (nSelAttrs == 1 && !strcmp((*selAttrs)->attrName, "*"))
		{ //select all
			int tempColNum = 0;
			resHead->col_num = 0;
			resHead->row_num = 0;

			int j = 0;
			for (int i = 0; i < nRelations; i++)
			{
				tempCon[0].Rvalue = relations[i];
				tempRc = RM_OpenFile("SYSTABLES", tempFileHandle);
				if (tempRc != SUCCESS)return tempRc;
				tempRc = OpenScan(tempFileScan, tempFileHandle, 1, tempCon);
				if (tempRc != SUCCESS)return tempRc;
				tempRc = GetNextRec(tempFileScan, tempRec);
				SysTable* table = (SysTable*)tempRec->pData;
				memcpy(&tempColNum, tempRec->pData + 21, sizeof(int));
				CloseScan(tempFileScan);
				RM_CloseFile(tempFileHandle);

				tempRc = RM_OpenFile("SYSCOLUMNS", tempFileHandle);
				if (tempRc != SUCCESS)return tempRc;
				tempCon[0].Rvalue = relations[nRelations - i - 1];
				OpenScan(tempFileScan, tempFileHandle, 1, tempCon);
				tempRc = GetNextRec(tempFileScan, tempRec);
				if (tempRc != SUCCESS)return tempRc;

				resHead->col_num += tempColNum;
				for (; j < resHead->col_num && tempRc == SUCCESS; j++)
				{
					char* column = tempRec->pData;
					memcpy(&resHead->fields[j], column + 21, 21);
					memcpy(&resHead->type[j], column + 42, sizeof(int));
					memcpy(&resHead->length[j], column + 46, sizeof(int));
					memcpy(&resHead->offset[j], column + 50, sizeof(int));
					for (int r = 0; r < nRelations; r++)
					{
						if (!strcmp(relations[nRelations - r - 1], (char*)tempCon[0].Rvalue))
						{
							resHead->offset[j] += offset[r];
							break;
						}
					}
					tempRc = GetNextRec(tempFileScan, tempRec);
				}
				CloseScan(tempFileScan);
				RM_CloseFile(tempFileHandle);
			}

			//build Parameter
			selectAllNum = 0;
			selectAllName = (RelAttr*)malloc(sizeof(selAttrs) * selectAllNum);
			j = 0;
			for (int i = 0; i < nRelations; i++)
			{
				tempCon[0].Rvalue = relations[i];
				tempRc = RM_OpenFile("SYSTABLES", tempFileHandle);
				if (tempRc != SUCCESS)return tempRc;
				tempRc = OpenScan(tempFileScan, tempFileHandle, 1, tempCon);
				if (tempRc != SUCCESS)return tempRc;
				tempRc = GetNextRec(tempFileScan, tempRec);
				SysTable* table = (SysTable*)tempRec->pData;
				memcpy(&tempColNum, tempRec->pData + 21, sizeof(int));
				CloseScan(tempFileScan);
				RM_CloseFile(tempFileHandle);

				tempRc = RM_OpenFile("SYSCOLUMNS", tempFileHandle);
				if (tempRc != SUCCESS)return tempRc;
				tempCon[0].Rvalue = relations[nRelations - i - 1];
				OpenScan(tempFileScan, tempFileHandle, 1, tempCon);
				tempRc = GetNextRec(tempFileScan, tempRec);
				if (tempRc != SUCCESS)return tempRc;

				selectAllNum += tempColNum;
				for (; j < selectAllNum && tempRc == SUCCESS; j++)
				{
					char* column = tempRec->pData;
					selectAllName[j].relName = (char*)malloc(21);
					selectAllName[j].attrName = (char*)malloc(21);
					memcpy(selectAllName[j].relName, column, 21);
					memcpy(selectAllName[j].attrName, column + 21, 21);

					tempRc = GetNextRec(tempFileScan, tempRec);
				}
				CloseScan(tempFileScan);
				RM_CloseFile(tempFileHandle);
			}
		}
		else
		{  //not select all
			resHead->col_num = nSelAttrs;
			resHead->row_num = 0;
			tempCon[1].attrType = chars;
			tempCon[1].bLhsIsAttr = 1;
			tempCon[1].LattrLength = 21;
			tempCon[1].LattrOffset = 21;
			tempCon[1].bRhsIsAttr = 0;
			tempCon[1].compOp = EQual;

			tempRc = RM_OpenFile("SYSCOLUMNS", tempFileHandle);
			if (tempRc != SUCCESS)return tempRc;

			for (int i = 0; i < resHead->col_num; i++)
			{
				tempCon[1].Rvalue = selAttrs[resHead->col_num - i - 1]->attrName;

				if (selAttrs[resHead->col_num - i - 1]->relName != NULL)
				{
					tempCon[0].Rvalue = selAttrs[resHead->col_num - i - 1]->relName;
				}
				else
				{
					for (int y = 0; y < nRelations; y++)
					{
						tempCon[0].Rvalue = relations[y];
						OpenScan(tempFileScan, tempFileHandle, 2, tempCon);
						tempRc = GetNextRec(tempFileScan, tempRec);
						if (tempRc == SUCCESS)
						{
							break;
						}
					}
					CloseScan(tempFileScan);
				}

				OpenScan(tempFileScan, tempFileHandle, 2, tempCon);
				tempRc = GetNextRec(tempFileScan, tempRec);
				if (tempRc != SUCCESS)return tempRc;
				char* column = tempRec->pData;
				memcpy(&resHead->fields[i], column + 21, 21);
				memcpy(&resHead->type[i], column + 42, sizeof(int));
				memcpy(&resHead->length[i], column + 46, sizeof(int));
				memcpy(&resHead->offset[i], column + 50, sizeof(int));
				for (int j = 0; j < nRelations; j++)
				{
					if (!strcmp(relations[nRelations - j - 1], (char*)tempCon[0].Rvalue))
					{
						resHead->offset[i] += offset[j];
						break;
					}
				}
				CloseScan(tempFileScan);
			}//end for
			RM_CloseFile(tempFileHandle);
		}//end else

		//close all file
		for (int i = 0; i < nRelations; i++)
		{
			tempRc = RM_CloseFile(allFileHandle[i]);
			if (tempRc != SUCCESS)return tempRc;
			free(allFileHandle[i]);
		}
		free(allFileHandle);
		free(tempFileHandle);
		free(tempFileScan);
		free(tempRec);

		fillCondition(nRelations, relations, nConditions, conditions);

		if (nSelAttrs == 1 && !strcmp((*selAttrs)->attrName, "*"))
		{ //select all
			helpSelect(selectAllNum, &selectAllName, nRelations, relations, nConditions, conditions,
				res, nRelations - 1, offset, NULL);
		}
		else
		{
			helpSelect(nSelAttrs, selAttrs, nRelations, relations, nConditions, conditions,
				res, nRelations - 1, offset, NULL);
		}
		free(offset);
	}
	res = resHead;
	return SUCCESS;
}

RC helpSelect(int nSelAttrs, RelAttr** selAttrs, int nRelations, char** relations, int nConditions, Condition* conditions, SelResult* res, int helpFlag, int* offsets, char* curResult)
{
	if (helpFlag < 0)
	{ //out
		SelResult* curRes = res;
		while (curRes->next_res != NULL)
		{
			curRes = curRes->next_res;
		}

		if (curRes->row_num >= 100)
		{
			curRes->next_res = (SelResult*)malloc(sizeof(SelResult));
			curRes = curRes->next_res;
			curRes->next_res = NULL;
			curRes->row_num = 0;
		}

		curRes->res[curRes->row_num] = (char**)malloc(sizeof(char*));
		*(curRes->res[curRes->row_num]) = (char*)malloc(sizeof(char) * offsets[nRelations]);
		memcpy(*(curRes->res[curRes->row_num]), curResult, offsets[nRelations]);
		curRes->row_num++;
		return SUCCESS;
	}
	RC tempRc;
	RM_FileHandle* tempFileHandle = (RM_FileHandle*)malloc(sizeof(RM_FileHandle));
	RM_FileScan* tempFileScan = (RM_FileScan*)malloc(sizeof(RM_FileScan));
	RM_Record* tempRec = (RM_Record*)malloc(sizeof(RM_Record));
	tempFileHandle->bOpen = false;
	tempFileScan->bOpen = false;
	tempRec->bValid = false;

	int nSelectCons = 0;
	Con tempCon[2];
	Con** selectCon = (Con**)malloc(sizeof(Con*) * nConditions);
	RelAttr anotherTable;
	int nAnotherTable;

	tempRc = RM_OpenFile("SYSCOLUMNS", tempFileHandle);
	if (tempRc != SUCCESS) return tempRc;

	tempCon[0].attrType = chars;
	tempCon[0].bLhsIsAttr = 1;
	tempCon[0].LattrOffset = 0;
	tempCon[0].LattrLength = 21;
	tempCon[0].bRhsIsAttr = 0;
	tempCon[0].Rvalue = relations[helpFlag];
	tempCon[0].compOp = EQual;

	tempCon[1].attrType = chars;
	tempCon[1].bLhsIsAttr = 1;
	tempCon[1].LattrOffset = 21;
	tempCon[1].LattrLength = 21;
	tempCon[1].bRhsIsAttr = 0;
	tempCon[1].compOp = EQual;

	for (int i = 0; i < nConditions; i++)
	{
		if (!((conditions[i].bLhsIsAttr == 1 && !strcmp(conditions[i].lhsAttr.relName, relations[helpFlag]))
			|| (conditions[i].bRhsIsAttr == 1 && !strcmp(conditions[i].rhsAttr.relName, relations[helpFlag]))))
		{
			continue;
		}

		selectCon[nSelectCons] = (Con*)malloc(sizeof(Con));

		if (conditions[i].bLhsIsAttr == 0 && conditions[i].bRhsIsAttr == 1)
		{
			tempCon[1].Rvalue = conditions[i].rhsAttr.attrName;
		}
		else if (conditions[i].bLhsIsAttr == 1 && conditions[i].bRhsIsAttr == 0)
		{
			tempCon[1].Rvalue = conditions[i].lhsAttr.attrName;
		}
		else if (conditions[i].bLhsIsAttr == 1 && conditions[i].bLhsIsAttr == 1)
		{
			if (!strcmp(conditions[i].lhsAttr.relName, relations[helpFlag]))
			{
				tempCon[1].Rvalue = conditions[i].lhsAttr.attrName;
				anotherTable.relName = conditions[i].rhsAttr.relName;
				anotherTable.attrName = conditions[i].rhsAttr.attrName;
			}
			else
			{
				tempCon[1].Rvalue = conditions[i].rhsAttr.attrName;
				anotherTable.relName = conditions[i].lhsAttr.relName;
				anotherTable.attrName = conditions[i].lhsAttr.attrName;
			}

			int j = 0;
			for (; j < nRelations; j++)
			{
				if (!strcmp(relations[j], anotherTable.relName))
				{
					break;
				}
			}
			if (j >= nRelations)
			{
				return TABLE_NOT_EXIST;
			}

			if (j < helpFlag)
			{
				continue;
			}
			nAnotherTable = j;
		}//end else if

		OpenScan(tempFileScan, tempFileHandle, 2, tempCon);
		tempRc = GetNextRec(tempFileScan, tempRec);
		if (tempRc != SUCCESS)return tempRc;

		if (conditions[i].bLhsIsAttr == 0 && conditions[i].bRhsIsAttr == 1)
		{
			selectCon[nSelectCons]->bLhsIsAttr = 0;
			selectCon[nSelectCons]->bRhsIsAttr = 1;
			selectCon[nSelectCons]->compOp = conditions[i].op;

			memcpy(&selectCon[nSelectCons]->RattrLength, tempRec->pData + 46, sizeof(int));
			memcpy(&selectCon[nSelectCons]->RattrOffset, tempRec->pData + 50, sizeof(int));
			selectCon[nSelectCons]->attrType = conditions[i].lhsValue.type;
			selectCon[nSelectCons]->Lvalue = conditions[i].lhsValue.data;
		}
		else if (conditions[i].bLhsIsAttr == 1 && conditions[i].bRhsIsAttr == 0)
		{
			selectCon[nSelectCons]->bLhsIsAttr = 1;
			selectCon[nSelectCons]->bRhsIsAttr = 0;
			selectCon[nSelectCons]->compOp = conditions[i].op;

			memcpy(&selectCon[nSelectCons]->LattrLength, tempRec->pData + 46, sizeof(int));
			memcpy(&selectCon[nSelectCons]->LattrOffset, tempRec->pData + 50, sizeof(int));
			selectCon[nSelectCons]->attrType = conditions[i].rhsValue.type;
			selectCon[nSelectCons]->Rvalue = conditions[i].rhsValue.data;
		}
		else if (conditions[i].bLhsIsAttr == 1 && conditions[i].bRhsIsAttr == 1)
		{
			selectCon[nSelectCons]->bLhsIsAttr = 1;
			selectCon[nSelectCons]->bRhsIsAttr = 0;
			selectCon[nSelectCons]->compOp = conditions[i].op;
			memcpy(&selectCon[nSelectCons]->attrType, tempRec->pData + 42, 4);
			memcpy(&selectCon[nSelectCons]->LattrLength, tempRec->pData + 46, sizeof(int));
			memcpy(&selectCon[nSelectCons]->LattrOffset, tempRec->pData + 50, sizeof(int));

			RM_FileHandle* columnFileHandle = (RM_FileHandle*)malloc(sizeof(RM_FileHandle));
			columnFileHandle->bOpen = false;
			tempRc = RM_OpenFile("SYSCOLUMNS", columnFileHandle);
			if (tempRc != SUCCESS)return tempRc;

			RM_FileScan* columnFileScan = (RM_FileScan*)malloc(sizeof(RM_FileScan));

			tempCon[0].Rvalue = anotherTable.relName;
			tempCon[1].Rvalue = anotherTable.attrName;
			OpenScan(columnFileScan, columnFileHandle, 2, tempCon);
			tempRc = GetNextRec(columnFileScan, tempRec);
			if (tempRc != SUCCESS)return tempRc;

			int offset;
			memcpy(&offset, tempRec->pData + 50, sizeof(int));
			selectCon[nSelectCons]->Rvalue = curResult + offsets[nRelations - nAnotherTable - 1] + offset;

			CloseScan(columnFileScan);
			free(columnFileScan);
			RM_CloseFile(columnFileHandle);
			free(columnFileHandle);
		}//end else if
		nSelectCons++;
		CloseScan(tempFileScan);
	}//end condition for
	RM_CloseFile(tempFileHandle);

	tempRc = RM_OpenFile(relations[helpFlag], tempFileHandle);
	if (tempRc != SUCCESS)return tempRc;

	OpenScan(tempFileScan, tempFileHandle, nSelectCons, *selectCon);
	tempRc = GetNextRec(tempFileScan, tempRec);
	while (tempRc == SUCCESS)
	{
		char* result = (char*)malloc(sizeof(char) * offsets[nRelations - helpFlag - 1 + 1]);
		memcpy(result, curResult, offsets[nRelations - helpFlag - 1]);
		memcpy(result + offsets[nRelations - helpFlag - 1], tempRec->pData, tempFileHandle->rm_fileSubHeader->recordSize);
		helpSelect(nSelAttrs, selAttrs, nRelations, relations, nConditions, conditions, res, helpFlag - 1, offsets, result);
		free(result);
		tempRc = GetNextRec(tempFileScan, tempRec);
	}
	CloseScan(tempFileScan);
	RM_CloseFile(tempFileHandle);
	free(tempFileHandle);
	free(tempFileScan);
	free(tempRec);
	for (int i = 0; i < nSelectCons; i++)
	{
		free(selectCon[i]);
	}
	selectCon = NULL;
	return SUCCESS;
}