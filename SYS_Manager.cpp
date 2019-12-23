#include "stdafx.h"
#include "EditArea.h"
#include "SYS_Manager.h"
#include "QU_Manager.h"
#include <iostream>

#include "HustBaseDoc.h"

char cur_db_pathname[233];

// for quiery operations
RM_FileHandle *sys_table_handle = NULL;
RM_FileHandle *sys_colmn_handle = NULL;


RC if_table_exist(char *relName);
RC GetColsInfo(char *relName, char ** attrName, AttrType * attrType, int * attrLength, int * attrOffset, bool * ixFlag, char ** indexName)
{
	char open_path[233];

	Con table_condition;
	table_condition.bLhsIsAttr = 1;
	table_condition.bRhsIsAttr = 0;
	table_condition.compOp = EQual;
	table_condition.attrType = chars;
	table_condition.LattrOffset = 0;
	table_condition.LattrLength = 21;
	table_condition.Rvalue = relName;
	int cur_pos = 0;

	// open syscolumns
	strcpy(open_path, cur_db_pathname);
	strcat(open_path, "\\SYSCOLUMNS");
	RM_OpenFile(open_path, sys_colmn_handle);

	RM_FileScan col_scan;
	RM_Record col_record;
	OpenScan(&col_scan, sys_colmn_handle, 1, &table_condition);
	while (true) {
		RC is_existed = GetNextRec(&col_scan, &col_record);
		if (is_existed != SUCCESS) {
			break;
		}
		auto cur_row = col_record.pData;
		strcpy(attrName[cur_pos], cur_row + TABLENAME_SIZE);
		memcpy(&attrType[cur_pos], cur_row + TABLENAME_SIZE + ATTRNAME_SIZE, sizeof(int));
		memcpy(&attrLength[cur_pos], cur_row + TABLENAME_SIZE + ATTRNAME_SIZE + ATTRTYPE_SIZE, sizeof(int));
		memcpy(&attrOffset[cur_pos], cur_row + TABLENAME_SIZE + ATTRNAME_SIZE + ATTRTYPE_SIZE + ATTRLENGTH_SIZE,
			sizeof(int));
		memcpy(&ixFlag[cur_pos], cur_row + IX_FLAG_OFFSET, 1);
		if (ixFlag[cur_pos] == 1) {
			strcpy(indexName[cur_pos], cur_row + IX_FLAG_OFFSET + 1);
		}
		cur_pos++;
	}
	CloseScan(&col_scan);
	RM_CloseFile(sys_colmn_handle);
	return SUCCESS;
}

Con *convert_conditions(int con_num, Condition *cons, int col_num, char ** col_name, const int *col_length, const int *col_offset, const AttrType *col_types) {
	auto converted_cons = new Con[con_num];
	for (int i = 0; i < con_num; i++) {
		auto cur_con = cons[i];
		auto cur_converted = &converted_cons[i];
		char * left_value = nullptr, *right_value = nullptr;
		int left_length = 0, right_length = 0;
		int left_offset = 0, right_offset = 0;
		AttrType attr_type = ints;

		if (cur_con.bLhsIsAttr == 0) {
			attr_type = cur_con.lhsValue.type;
			left_value = (char *)cur_con.lhsValue.data;
		}
		else {
			for (int j = 0; j < col_num; j++) {
				if (strcmp(cur_con.lhsAttr.attrName, col_name[j]) == 0) {
					attr_type = col_types[j];
					left_length = col_length[j];
					left_offset = col_offset[j];
					break;
				}
			}
		}

		if (cur_con.bRhsIsAttr == 0) {
			attr_type = cur_con.rhsValue.type;
			right_value = (char *)cur_con.rhsValue.data;
		}
		else {
			for (int j = 0; j < col_num; j++) {
				if (strcmp(cur_con.rhsAttr.attrName, col_name[j]) == 0) {
					attr_type = col_types[j];
					right_length = col_length[j];
					right_offset = col_offset[j];
				}
			}
		}

		cur_converted->bLhsIsAttr = cur_con.bLhsIsAttr;
		cur_converted->bRhsIsAttr = cur_con.bRhsIsAttr;
		cur_converted->attrType = attr_type;
		cur_converted->LattrLength = left_length;
		cur_converted->LattrOffset = left_offset;
		cur_converted->RattrLength = right_length;
		cur_converted->RattrOffset = right_offset;
		cur_converted->compOp = cur_con.op;
		cur_converted->Lvalue = left_value;
		cur_converted->Rvalue = right_value;
	}
	return converted_cons;
}



void ExecuteAndMessage(char *sql, CEditArea* editArea, CHustBaseDoc* pDoc)
{//����ִ�е���������ڽ�������ʾִ�н�����˺������޸�
	std::string s_sql = sql;

	RC rc = execute(sql, pDoc);
	int row_num = 0;
	char **messages;
	switch (rc) {
	case SUCCESS:
		row_num = 1;
		messages = new char*[row_num];
		messages[0] = "Successful Execution!";
		editArea->ShowMessage(row_num, messages);
		delete[] messages;
		break;
	case SQL_SYNTAX:
		row_num = 1;
		messages = new char*[row_num];
		messages[0] = "Syntax Error!";
		editArea->ShowMessage(row_num, messages);
		delete[] messages;
		break;
	default:
		row_num = 1;
		messages = new char*[row_num];
		messages[0] = "Function Not Implemented!";
		editArea->ShowMessage(row_num, messages);
		delete[] messages;
		break;
	}

}

RC execute(char * sql, CHustBaseDoc* pDoc) {
	sqlstr *sql_str = NULL;
	RC rc;
	sql_str = get_sqlstr();
	rc = parse(sql, sql_str);//ֻ�����ַ��ؽ��SUCCESS��SQL_SYNTAX

	if (rc == SUCCESS)
	{
		switch (sql_str->flag)
		{
		case 1:
			//�ж�SQL���Ϊselect���
			break;
		case 2:
			//�ж�SQL���Ϊinsert���
			Insert(sql_str->sstr.ins.relName, sql_str->sstr.ins.nValues, sql_str->sstr.ins.values);
			pDoc->m_pTreeView->PopulateTree();
			break;
		case 3:
			//�ж�SQL���Ϊupdate���
		{
			updates up = sql_str->sstr.upd;
			Update(up.relName, up.attrName, &up.value, up.nConditions, up.conditions);
			break;
		}

		case 4:
			//�ж�SQL���Ϊdelete���
			Delete(sql_str->sstr.del.relName, sql_str->sstr.del.nConditions, sql_str->sstr.del.conditions);
			break;

		case 5:
			//�ж�SQL���ΪcreateTable���
			CreateTable(sql_str->sstr.cret.relName, sql_str->sstr.cret.attrCount, sql_str->sstr.cret.attributes);
			pDoc->m_pTreeView->PopulateTree(); //������ͼ
			//pDoc->m_pListView->displayTabInfo(sql_str->sstr.cret.relName);//�Ҳ�ˢ�±���
			break;

		case 6:
			//�ж�SQL���ΪdropTable���
			DropTable(sql_str->sstr.cret.relName);
			pDoc->m_pTreeView->PopulateTree(); //������ͼ
			//pDoc->m_pListView->displayTabInfo(sql_str->sstr.cret.relName);//�Ҳ�ˢ�±���
			break;

		case 7:
			//�ж�SQL���ΪcreateIndex���
			CreateIndex(sql_str->sstr.crei.indexName, sql_str->sstr.crei.relName, sql_str->sstr.crei.attrName);
			break;

		case 8:
			//�ж�SQL���ΪdropIndex���
			DropIndex(sql_str->sstr.crei.indexName);
			break;

		case 9:
			//�ж�Ϊhelp��䣬���Ը���������ʾ
			break;

		case 10:
			//�ж�Ϊexit��䣬�����ɴ˽����˳�����
			break;
		}

		return SUCCESS;
	}
	else {
		AfxMessageBox(sql_str->sstr.errors);//���������sql���ʷ�����������Ϣ
		return rc;
	}
}

//- DataBase
// | - SysTable
// | - SysColumn

// void CHustBaseApp::OnCreateDB() in HustBase.cpp already created a directory!
RC CreateDB(char *dbpath,char *dbname)
{
	printf("CreateDB called!\n");

	char path_database_name[233];  
	strcpy(path_database_name, dbpath);
	// --- NOTICE ---
	// void CHustBaseApp::OnCreateDB() in HustBase.cpp will get the full path
	//strcat(path_database_name, "\\");
	//strcat(path_database_name, dbname);

	char path_systable_name[233];
	char path_syscolmn_name[233];

	strcpy(path_systable_name, path_database_name);
	strcat(path_systable_name, "\\SYSTABLES");

	strcpy(path_syscolmn_name, path_database_name);
	strcat(path_syscolmn_name, "\\SYSCOLUMNS");

	RC rc_table = RM_CreateFile(path_systable_name, SYS_TABLE_ROW_SIZE);
	RC rc_colmn = RM_CreateFile(path_syscolmn_name, SYS_COLMN_ROW_SIZE);
	if ((rc_table != SUCCESS) | (rc_colmn != SUCCESS)) {
		AfxMessageBox("�������ݿ�ϵͳ��ʧ�ܣ�");
		return DATABASE_FAILED;
	}

	printf("CreateDB successed!\n");
	return SUCCESS;
}

RC DropDB(char *dbname)
{
	char delete_db[233] = "rmdir /s/q ";
	strcat(delete_db, dbname);
	system(delete_db);

	return SUCCESS;
}

RC OpenDB(char *dbname)
{
	sys_table_handle = new RM_FileHandle;
	sys_colmn_handle = new RM_FileHandle;
	strcpy(cur_db_pathname, dbname);

	printf("database opened!\n");

	return SUCCESS;
}

RC CloseDB()
{
	delete sys_table_handle;
	delete sys_colmn_handle;
	memset(cur_db_pathname, 0, 233);
	return SUCCESS;
}

RC CreateTable(char *relName, int attrCount, AttrInfo *attributes) 
{
	char open_path[233];
	RC rc;
	
	/* for SYSTABLES */

	// open systables
	strcpy(open_path, cur_db_pathname);
	strcat(open_path, "\\SYSTABLES");
	RM_OpenFile(open_path, sys_table_handle);
	
	// if the table already existed
	Con find_con;
	find_con.bLhsIsAttr = 1;
	find_con.bRhsIsAttr = 0;
	find_con.attrType = chars;
	find_con.LattrLength = TABLENAME_SIZE;
	find_con.LattrOffset = 0;
	find_con.compOp = EQual;
	find_con.Rvalue = relName;

	printf("create table name: %s\n", relName);

	RM_FileScan table_scan;
	OpenScan(&table_scan, sys_table_handle, 1, &find_con);
	RM_Record table_rec;
	rc = GetNextRec(&table_scan, &table_rec);
	if (rc == SUCCESS) {
		AfxMessageBox("�ñ��Ѿ�����");
		CloseScan(&table_scan);
		RM_CloseFile(sys_table_handle);
		return TABLE_EXIST;
	}
	CloseScan(&table_scan);

	// insert into systables
	char *new_table = new char[SYS_TABLE_ROW_SIZE];
	strcpy(new_table, relName);
	memcpy(new_table + TABLENAME_SIZE, &attrCount, sizeof(int));

	RID tmp_rid;
	InsertRec(sys_table_handle, new_table, &tmp_rid);

	RM_CloseFile(sys_table_handle);
	delete[] new_table;

	/* for SYSCOLUMNS */

	// open syscolumns
	strcpy(open_path, cur_db_pathname);
	strcat(open_path, "\\SYSCOLUMNS");
	RM_OpenFile(open_path, sys_colmn_handle);

	// insert into syscolumns
	int attr_total_offset = 0;
	for (int i = 0; i < attrCount; i++) {
		char col_info[SYS_COLMN_ROW_SIZE];
		AttrInfo attr_i = attributes[i];
		int offset = 0;
		strcpy(col_info + offset, relName);
		offset += TABLENAME_SIZE;
		strcpy(col_info + offset, attr_i.attrName);
		offset += ATTRNAME_SIZE;
		memcpy(col_info + offset, &attr_i.attrType, ATTRTYPE_SIZE);
		offset += ATTRTYPE_SIZE;
		memcpy(col_info + offset, &attr_i.attrLength, ATTRLENGTH_SIZE);
		offset += ATTRLENGTH_SIZE;
		memcpy(col_info + offset, &attr_total_offset, ATTROFFSET_SIZE);
		// ix is empty
		attr_total_offset += attr_i.attrLength;
		InsertRec(sys_colmn_handle, col_info, &tmp_rid);
	}

	RM_CloseFile(sys_colmn_handle);

	char new_table_path[233];
	strcpy(new_table_path, cur_db_pathname);
	strcat(new_table_path, "\\");
	strcat(new_table_path, relName);
	rc = RM_CreateFile(new_table_path, attr_total_offset);
	if (rc != SUCCESS) {
		AfxMessageBox("����ʧ�ܣ�");
		return TABLE_CREATE_FAILED;
	}

	return SUCCESS;
}

RC DropTable(char *relName) 
{
	char open_path[233];
	RC rc;

	int column_num = 0;

	/* for SYSTABLES */

	// open systables
	strcpy(open_path, cur_db_pathname);
	strcat(open_path, "\\SYSTABLES");
	RM_OpenFile(open_path, sys_table_handle);

	Con find_con;
	find_con.bLhsIsAttr = 1;
	find_con.bRhsIsAttr = 0;
	find_con.attrType = chars;
	find_con.LattrLength = TABLENAME_SIZE;
	find_con.LattrOffset = 0;
	find_con.compOp = EQual;
	find_con.Rvalue = relName;

	RM_FileScan table_scan;
	OpenScan(&table_scan, sys_table_handle, 1, &find_con);
	RM_Record table_rec;
	rc = GetNextRec(&table_scan, &table_rec);
	if (rc != SUCCESS) {
		AfxMessageBox("�ñ����ڣ�");
		CloseScan(&table_scan);
		RM_CloseFile(sys_table_handle);
		return TABLE_NOT_EXIST;
	}
	DeleteRec(sys_table_handle, &table_rec.rid);			// ɾ����¼

	CloseScan(&table_scan);
	RM_CloseFile(sys_table_handle);

	/* for SYSCOLUMNS */

	// open syscolumns
	strcpy(open_path, cur_db_pathname);
	strcat(open_path, "\\SYSCOLUMNS");
	RM_OpenFile(open_path, sys_colmn_handle);

	RM_FileScan colmn_scan;
	OpenScan(&colmn_scan, sys_colmn_handle, 1, &find_con);
	RM_Record colmn_rec;
	while (GetNextRec(&colmn_scan, &colmn_rec) == SUCCESS) {
		DeleteRec(sys_colmn_handle, &(colmn_rec.rid));		// ɾ����¼
	}

	CloseScan(&colmn_scan);
	RM_CloseFile(sys_colmn_handle);

	// delete table file
	char delete_table[233] = "del /a /f /q ";
	strcat(delete_table, cur_db_pathname);
	strcat(delete_table, "\\");
	strcat(delete_table, relName);
	system(delete_table);

	// delete index files
	char delete_index[233] = "del /a /f /q ";
	strcat(delete_index, cur_db_pathname);
	strcat(delete_index, "\\");
	strcat(delete_index, relName);
	strcat(delete_index, ".*");

	system(delete_index);

	return SUCCESS;
}

RC CreateIndex(char *indexName, char *relName, char *attrName) 
{
	char open_path[233];
	RC rc;

	/* PREPARATION */

	// open syscolumns
	strcpy(open_path, cur_db_pathname);
	strcat(open_path, "\\SYSCOLUMNS");
	RM_OpenFile(open_path, sys_colmn_handle);

	// if the column of this table exists
	Con colmn_cons[2];
	colmn_cons[0].bLhsIsAttr = 1;
	colmn_cons[0].bRhsIsAttr = 0;
	colmn_cons[0].attrType = chars;
	colmn_cons[0].LattrLength = TABLENAME_SIZE;
	colmn_cons[0].LattrOffset = 0;
	colmn_cons[0].compOp = EQual;
	colmn_cons[0].Rvalue = relName;

	colmn_cons[1].bLhsIsAttr = 1;
	colmn_cons[1].bRhsIsAttr = 0;
	colmn_cons[1].attrType = chars;
	colmn_cons[1].LattrLength = ATTRNAME_SIZE;
	colmn_cons[1].LattrOffset = ATTRNAME_OFFSET;
	colmn_cons[1].compOp = EQual;
	colmn_cons[1].Rvalue = attrName;

	RM_FileScan colmn_scan;
	OpenScan(&colmn_scan, sys_colmn_handle, 2, colmn_cons);
	RM_Record colmn_rec;
	rc = GetNextRec(&colmn_scan, &colmn_rec);
	if (rc != SUCCESS) {
		AfxMessageBox("�����ڸñ�ĸ����ԣ�");
		CloseScan(&colmn_scan);
		RM_CloseFile(sys_colmn_handle);
		return FLIED_NOT_EXIST;
	}
	CloseScan(&colmn_scan);

	// if the index name is not used
	colmn_cons[0].bLhsIsAttr = 1;
	colmn_cons[0].bRhsIsAttr = 0;
	colmn_cons[0].attrType = chars;
	colmn_cons[0].LattrLength = INDEXNAME_SIZE;
	colmn_cons[0].LattrOffset = INDEXNAME_OFFSET;
	colmn_cons[0].compOp = EQual;
	colmn_cons[0].Rvalue = indexName;

	OpenScan(&colmn_scan, sys_colmn_handle, 1, colmn_cons);
	rc = GetNextRec(&colmn_scan, &colmn_rec);
	if (rc == SUCCESS) {
		AfxMessageBox("���������ѱ�ʹ��");
		CloseScan(&colmn_scan);
		RM_CloseFile(sys_colmn_handle);
		return INDEX_NAME_REPEAT;
	}
	CloseScan(&colmn_scan);

	// if there is no index on this column
	char *cur_colmn = colmn_rec.pData;
	if (*(cur_colmn + IX_FLAG_OFFSET) == '1') {
		AfxMessageBox("�������Ѿ�������������");
		RM_CloseFile(sys_colmn_handle);
		return INDEX_EXIST;
	}

	// set index flag & index name
	*(cur_colmn + IX_FLAG_OFFSET) = '1';
	strcpy(cur_colmn + IX_FLAG_OFFSET + 1, indexName);

	// updata record
	UpdateRec(sys_colmn_handle, &colmn_rec);

	RM_CloseFile(sys_colmn_handle);

	/* CREATE & OPEN & INSERT INDEX FILE*/

	// create
	AttrType attrType;
	int attrLength;
	memcpy(&attrType, cur_colmn + ATTRTYPE_OFFSET, ATTRTYPE_SIZE);
	memcpy(&attrLength, cur_colmn + ATTRLENGTH_OFFSET, ATTRLENGTH_SIZE);

	char index_path[233];
	strcpy(index_path, cur_db_pathname);
	strcat(index_path, "\\");
	strcat(index_path, relName);
	strcat(index_path, ".");
	strcat(index_path, indexName);

	rc = CreateIndex(index_path, attrType, attrLength);
	if (rc != SUCCESS) {
		AfxMessageBox("��������ʧ�ܣ�");
		return INDEX_CREATE_FAILED;
	}

	// open
	IX_IndexHandle index_handle;
	OpenIndex(index_path, &index_handle);

	// find in the corresponding table
	strcpy(open_path, cur_db_pathname);
	strcat(open_path, "\\");
	strcat(open_path, relName);
	RM_OpenFile(open_path, sys_table_handle);

	RM_FileHandle cur_table_handle;
	RM_OpenFile(relName, &cur_table_handle);
	RM_FileScan cur_table_scan;
	OpenScan(&cur_table_scan, &cur_table_handle, 0, NULL);

	char *attr_pdata = new char[attrLength];
	int attrOffset;
	memcpy(&attrOffset, cur_colmn + ATTROFFSET_OFFSET, ATTROFFSET_SIZE);

	// insert
	RM_Record index_rec;
	while (GetNextRec(&cur_table_scan, &index_rec) == SUCCESS) {
		memcpy(attr_pdata, index_rec.pData + attrOffset, attrLength);
		InsertEntry(&index_handle, attr_pdata, &index_rec.rid);
	}

	delete[] attr_pdata;

	CloseScan(&cur_table_scan);
	RM_CloseFile(&cur_table_handle);
	CloseIndex(&index_handle);

	return SUCCESS;
}

RC DropIndex(char *indexName) 
{
	char open_path[233];
	RC rc;

	// open syscolumns
	strcpy(open_path, cur_db_pathname);
	strcat(open_path, "\\SYSCOLUMNS");
	RM_OpenFile(open_path, sys_colmn_handle);

	// if the index exists
	Con colmn_con;
	colmn_con.bLhsIsAttr = 1;
	colmn_con.bRhsIsAttr = 0;
	colmn_con.attrType = chars;
	colmn_con.LattrLength = INDEXNAME_SIZE;
	colmn_con.LattrOffset = INDEXNAME_OFFSET;
	colmn_con.compOp = EQual;
	colmn_con.Rvalue = indexName;

	RM_FileScan colmn_scan;
	OpenScan(&colmn_scan, sys_colmn_handle, 1, &colmn_con);
	RM_Record colmn_rec;
	rc = GetNextRec(&colmn_scan, &colmn_rec);
	if (rc != SUCCESS) {
		AfxMessageBox("�������������ڣ�");
		CloseScan(&colmn_scan);
		RM_CloseFile(sys_colmn_handle);
		return INDEX_NOT_EXIST;
	}
	CloseScan(&colmn_scan);

	// clean syscolumn
	char *cur_colmn = colmn_rec.pData;
	char relName[21];
	strcpy(relName, cur_colmn);
	*(cur_colmn + IX_FLAG_OFFSET) = '0';
	memset(cur_colmn + INDEXNAME_OFFSET, 0, INDEXNAME_SIZE);

	// updata record
	UpdateRec(sys_colmn_handle, &colmn_rec);

	RM_CloseFile(sys_colmn_handle);

	// delete index file
	char delete_index[233] = "del /a /f /q ";
	strcat(delete_index, cur_db_pathname);
	strcat(delete_index, "\\");
	strcat(delete_index, relName);
	strcat(delete_index, ".");
	strcat(delete_index, indexName);

	system(delete_index);

	return SUCCESS;
}

RC Insert(char *relName, int nValues, Value * values) 
{
	char open_path[233];
	RC rc;

	// IF THE TABLE EXISTS
	strcpy(open_path, cur_db_pathname);
	strcat(open_path, "\\SYSTABLES");
	RM_OpenFile(open_path, sys_table_handle);

	Con find_con;
	find_con.bLhsIsAttr = 1;
	find_con.bRhsIsAttr = 0;
	find_con.attrType = chars;
	find_con.LattrLength = TABLENAME_SIZE;
	find_con.LattrOffset = 0;
	find_con.compOp = EQual;
	find_con.Rvalue = relName;

	RM_FileScan table_scan;
	OpenScan(&table_scan, sys_table_handle, 1, &find_con);
	RM_Record table_rec;
	rc = GetNextRec(&table_scan, &table_rec);
	if (rc != SUCCESS) {
		AfxMessageBox("�ñ�����");
		CloseScan(&table_scan);
		RM_CloseFile(sys_table_handle);
		return TABLE_NOT_EXIST;
	}

	int attr_count = 0;
	memcpy(&attr_count, table_rec.pData + ATTRCOUNT_OFFSET, ATTRCOUNT_SIZE);
	if (nValues < attr_count) {
		AfxMessageBox("������ֶβ��㣡");
		return FIELD_MISSING;
	}
	if (nValues > attr_count) {
		AfxMessageBox("������ֶ�̫�࣡");
		return FIELD_REDUNDAN;
	}

	CloseScan(&table_scan);
	RM_CloseFile(sys_table_handle);

	// TO DO
	// ���������ͺͲ��������Ƿ��ģ���
	for (int i = 0; i < attr_count; i++) {
		if (values[i].type == chars)
			printf("chars ");
		else if (values[i].type == ints)
			printf("ints ");
		else printf("floats ");
	}

	// �������ֵ�������Ƿ����Ҫ�󣬼�¼�Ƿ���Ҫ��������
	strcpy(open_path, cur_db_pathname);
	strcat(open_path, "\\SYSCOLUMNS");
	RM_OpenFile(open_path, sys_colmn_handle);

	int *attr_len = new int[attr_count];
	bool *is_idx = new bool[attr_count];
	char (*idx_name)[233] = new char[attr_count][233];
	
	RM_FileScan colmn_scan;
	OpenScan(&colmn_scan, sys_colmn_handle, 1, &find_con);
	RM_Record colmn_rec;
	for (int i = 0; (GetNextRec(&colmn_scan, &colmn_rec) == SUCCESS) && (i < attr_count); i++) {
		char *cur_colmn = colmn_rec.pData;
		AttrType attr_type;
		memcpy(&attr_type, cur_colmn + ATTRTYPE_OFFSET, ATTRTYPE_SIZE);
		// ��������Ƿ����
		if (attr_type != values[i].type) {
			AfxMessageBox("������ֶ���������");
			CloseScan(&colmn_scan);
			RM_CloseFile(sys_colmn_handle);
			return FIELD_TYPE_MISMATCH;
		}
		memcpy(&attr_len[i], cur_colmn + ATTRLENGTH_OFFSET, ATTRLENGTH_SIZE);
		memcpy(&is_idx[i], cur_colmn + IX_FLAG_OFFSET, IX_FLAG_SIZE);
		memcpy(idx_name[i], cur_colmn + INDEXNAME_OFFSET, INDEXNAME_SIZE);
	}
	CloseScan(&colmn_scan);
	RM_CloseFile(sys_colmn_handle);

	// ����Ԫ�鲢����
	char insert_value[233];
	for (int i = 0, offset = 0; i < attr_count; i++) {
		memcpy(insert_value + offset, values[i].data, attr_len[i]);
		offset += attr_len[i];
	}

	char table_path[233];
	strcpy(table_path, cur_db_pathname);
	strcat(table_path, "\\");
	strcat(table_path, relName);

	RM_FileHandle table_handle;
	RM_OpenFile(table_path, &table_handle);
	RID value_rid;
	rc = InsertRec(&table_handle, insert_value, &value_rid);
	if (rc != SUCCESS) {
		AfxMessageBox("����ʧ�ܣ�");
		RM_CloseFile(&table_handle);
		return INSERT_FAILED;
	}
	RM_CloseFile(&table_handle);

	// ����������
	for (int i = 0; i < attr_count; i++) {
		if (is_idx[i] == 1) {
			char idx_path[233];
			strcpy(idx_path, cur_db_pathname);
			strcat(idx_path, "\\");
			strcat(idx_path, relName);
			strcat(idx_path, ".");
			strcat(idx_path, idx_name[i]);
			IX_IndexHandle insert_idx_handle;
			OpenIndex(idx_path, &insert_idx_handle);
			rc = InsertEntry(&insert_idx_handle, (char *)values[i].data, &value_rid);
			if (rc != SUCCESS) {
				AfxMessageBox("����������ʧ��");
				return INDEX_ADD_FAILED;
			}
			CloseIndex(&insert_idx_handle);
		}
	}

	delete[] attr_len;
	delete[] is_idx;
	delete[] idx_name;

	return SUCCESS;
}

RC Delete(char *relName, int nConditions, Condition *conditions) 
{
	char open_path[233];
	RC rc;

	// IF THE TABLE EXISTS
	strcpy(open_path, cur_db_pathname);
	strcat(open_path, "\\SYSTABLES");
	RM_OpenFile(open_path, sys_table_handle);

	Con find_con;
	find_con.bLhsIsAttr = 1;
	find_con.bRhsIsAttr = 0;
	find_con.attrType = chars;
	find_con.LattrLength = TABLENAME_SIZE;
	find_con.LattrOffset = 0;
	find_con.compOp = EQual;
	find_con.Rvalue = relName;

	RM_FileScan table_scan;
	OpenScan(&table_scan, sys_table_handle, 1, &find_con);
	RM_Record table_rec;
	rc = GetNextRec(&table_scan, &table_rec);
	if (rc != SUCCESS) {
		AfxMessageBox("�ñ�����");
		CloseScan(&table_scan);
		RM_CloseFile(sys_table_handle);
		return TABLE_NOT_EXIST;
	}

	int attr_count = 0;
	memcpy(&attr_count, table_rec.pData + ATTRCOUNT_OFFSET, ATTRCOUNT_SIZE);

	CloseScan(&table_scan);
	RM_CloseFile(sys_table_handle);
	
	char **attr_name = new char* [attr_count];
	int *attr_length = new int[attr_count];
	int *attr_offset = new int[attr_count];
	AttrType *attr_type = new AttrType[attr_count];
	bool *attr_ix_flag = new bool[attr_count];
	char **attr_idxname = new char*[attr_count];

	for (int i = 0; i < attr_count; i++) {
		attr_name[i] = new char[ATTRNAME_SIZE];
		attr_idxname[i] = new char[INDEXNAME_SIZE];
	}

	GetColsInfo(relName, attr_name, attr_type, attr_length, attr_offset, attr_ix_flag, attr_idxname);
	//GetColsInfo(relName, col_name, col_types, col_length, col_offset, col_is_indx, col_indx_name);
	auto cons = convert_conditions(nConditions, conditions, attr_count, attr_name, attr_length, attr_offset, attr_type);
	//auto cons = convert_conditions(nConditions, conditions, col_num, col_name, col_length, col_offset, col_types);

	char full_tab_name[255];
	strcpy(full_tab_name, cur_db_pathname);
	strcat(full_tab_name, "\\");
	strcat(full_tab_name, relName);

	RM_FileHandle rm_fileHandle;
	RM_FileScan rm_fileScan;
	RM_OpenFile(full_tab_name, &rm_fileHandle);
	OpenScan(&rm_fileScan, &rm_fileHandle, nConditions, cons);

	RID *removed_rid = new RID[rm_fileHandle.rm_fileSubHeader->nRecords];
	char **removed_data = new char*[rm_fileHandle.rm_fileSubHeader->nRecords];
	for (int i = 0; i < rm_fileHandle.rm_fileSubHeader->nRecords; i++) {
		removed_data[i] = new char[rm_fileHandle.rm_fileSubHeader->recordSize - sizeof(bool) - sizeof(RID)];
	}

	int removed_num = 0;

	while (true) {
		RM_Record record;
		RC result = GetNextRec(&rm_fileScan, &record);
		if (result != SUCCESS) {
			break;
		}
		removed_rid[removed_num] = record.rid;
		memcpy(removed_data[removed_num], record.pData, (size_t)rm_fileHandle.rm_fileSubHeader->recordSize - sizeof(bool) - sizeof(RID));
		removed_num++;
	}
	CloseScan(&rm_fileScan);

	for (int i = 0; i < removed_num; i++) {
		DeleteRec(&rm_fileHandle, &removed_rid[i]);
	}
	RM_CloseFile(&rm_fileHandle);

	for (int i = 0; i < attr_count; i++) {
		if (attr_ix_flag[i]) {
			char full_index_name[255];
			strcpy(full_index_name, cur_db_pathname);
			strcat(full_index_name, "\\");
			strcat(full_index_name, relName);
			strcat(full_index_name, ".");
			strcat(full_index_name, attr_idxname[i]);

			auto ix_indexHandle = new IX_IndexHandle;
			OpenIndex(full_index_name, ix_indexHandle);
			for (int j = 0; j < removed_num; j++) {
				printf("Deleting %d\n", j);
				DeleteEntry(ix_indexHandle, removed_data[j] + attr_offset[i], &removed_rid[j]);
			}
			CloseIndex(ix_indexHandle);
			delete ix_indexHandle;
		}
	}

	for (int i = 0; i < rm_fileHandle.rm_fileSubHeader->nRecords; i++) {
		delete[] removed_data[i];
	}

	delete[] removed_rid;
	delete[] removed_data;

	for (int i = 0; i < attr_count; i++) {
		delete[] attr_name[i];
		delete[] attr_idxname[i];
	}

	delete[] attr_name;
	delete[] attr_length;
	delete[] attr_offset;
	delete[] attr_type;
	delete[] attr_ix_flag;
	delete[] attr_idxname;

	return SUCCESS;
}

RC Update(char *relName, char *attrName, Value *value, int nConditions, Condition *conditions) {
	return SUCCESS;
}

bool CanButtonClick()
{
	//�����ǰ�����ݿ��Ѿ���
	if ((sys_table_handle != NULL) & (sys_colmn_handle != NULL))
		return true;

	//�����ǰû�����ݿ��
	return false;
}

RC if_table_exist(char *relName)
{
	char open_path[233];
	RC rc;

	// open systables
	strcpy(open_path, cur_db_pathname);
	strcat(open_path, "\\SYSTABLES");
	RM_OpenFile(open_path, sys_table_handle);

	// if the table already existed
	Con find_con;
	find_con.bLhsIsAttr = 1;
	find_con.bRhsIsAttr = 0;
	find_con.attrType = chars;
	find_con.LattrLength = TABLENAME_SIZE;
	find_con.LattrOffset = 0;
	find_con.compOp = EQual;
	find_con.Rvalue = relName;

	printf("create table name: %s\n", relName);

	RM_FileScan table_scan;
	OpenScan(&table_scan, sys_table_handle, 1, &find_con);
	RM_Record table_rec;
	rc = GetNextRec(&table_scan, &table_rec);

	return rc;
}


