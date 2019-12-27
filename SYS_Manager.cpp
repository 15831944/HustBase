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

RC GetColsInfo(char *relName, char **attrName, AttrType *attrType, int *attrLength, int *attrOffset, bool *ixFlag, char **indexName)
{
	char open_path[233];

	// open syscolumns
	strcpy(open_path, cur_db_pathname);
	strcat(open_path, "\\SYSCOLUMNS");
	RM_OpenFile(open_path, sys_colmn_handle);

	Con find_con;
	find_con.bLhsIsAttr = 1;
	find_con.bRhsIsAttr = 0;
	find_con.attrType = chars;
	find_con.LattrLength = TABLENAME_SIZE;
	find_con.LattrOffset = 0;
	find_con.compOp = EQual;
	find_con.Rvalue = relName;

	RM_FileScan colmn_scan;
	OpenScan(&colmn_scan, sys_colmn_handle, 1, &find_con);
	RM_Record colmn_rec;
	for (int i = 0; GetNextRec(&colmn_scan, &colmn_rec) == SUCCESS; i++) {
		char *cur_colmn = colmn_rec.pData;
		strcpy(attrName[i], cur_colmn + ATTRNAME_SIZE);
		memcpy(&attrType[i], cur_colmn + ATTRTYPE_OFFSET, ATTRTYPE_SIZE);
		memcpy(&attrLength[i], cur_colmn + ATTRLENGTH_OFFSET, ATTRLENGTH_SIZE);
		memcpy(&attrOffset[i], cur_colmn + ATTROFFSET_OFFSET, ATTROFFSET_SIZE);
		memcpy(&ixFlag[i], cur_colmn + IX_FLAG_OFFSET, IX_FLAG_SIZE);
		if (ixFlag[i])
			strcpy(indexName[i], cur_colmn + INDEXNAME_OFFSET);
	}

	CloseScan(&colmn_scan);
	RM_CloseFile(sys_colmn_handle);

	return SUCCESS;
}

RC con_from_conditions(int con_num, Condition *conditions, int attr_count, char **attr_name, int *attr_length, int *attr_offset, AttrType *attr_types, Con *cons)
{
	for (int i = 0; i < con_num; i++) {
		Condition cur_condition = conditions[i];

		cons[i].bLhsIsAttr = cur_condition.bLhsIsAttr;
		cons[i].bRhsIsAttr = cur_condition.bRhsIsAttr;
		cons[i].compOp = cur_condition.op;

		if (cur_condition.bLhsIsAttr) {
			for (int j = 0; j < attr_count; j++) {
				if (!strcmp(cur_condition.lhsAttr.attrName, attr_name[j])) {
					cons[i].attrType = attr_types[j];
					cons[i].LattrLength = attr_length[j];
					cons[i].LattrOffset = attr_offset[j];
					break;
				}
			}
		} else {
			//cons[i].attrType = cur_condition.lhsValue.type;
			cons[i].Lvalue = cur_condition.lhsValue.data;
		}

		if (cur_condition.bRhsIsAttr) {
			for (int j = 0; j < attr_count; j++) {
				if (!strcmp(cur_condition.rhsAttr.attrName, attr_name[j])) {
					cons[i].attrType = attr_types[j];
					cons[i].RattrLength = attr_length[j];
					cons[i].RattrOffset = attr_offset[j];
				}
			}
		} else {
			//cons[i].attrType = cur_condition.rhsValue.type;
			cons[i].Rvalue = cur_condition.rhsValue.data;
		}
	}

	return SUCCESS;
}

void ExecuteAndMessage(char *sql, CEditArea* editArea, CHustBaseDoc* pDoc)
{//根据执行的语句类型在界面上显示执行结果。此函数需修改
	std::string s_sql = sql;

	RC rc = execute(sql, editArea, pDoc);
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

RC execute(char * sql, CEditArea* editArea, CHustBaseDoc* pDoc) {
	sqlstr *sql_str = NULL;
	RC rc;
	sql_str = get_sqlstr();
	rc = parse(sql, sql_str);//只有两种返回结果SUCCESS和SQL_SYNTAX

	if (rc == SUCCESS)
	{
		switch (sql_str->flag)
		{
		case 1: {
			//判断SQL语句为select语句
			SelResult res;
			Init_Result(&res);
			if (rc = Query(sql, &res))
				return rc;
			int col_num = res.col_num;
			int row_num = 0;
			SelResult *cur_res = &res;
			while (cur_res) {
				row_num += cur_res->row_num;
				cur_res = cur_res->next_res;
			}
			char ** fields = new char *[20];
			for (int i = 0; i < col_num; i++) {
				fields[i] = new char[20];
				memset(fields[i], 0, 20);
				memcpy(fields[i], res.fields[i], 20);
			}
			cur_res = &res;
			char *** rows = new char**[row_num];
			for (int i = 0; i < row_num; i++) {
				rows[i] = new char*[col_num];
				for (int j = 0; j < col_num; j++) {
					rows[i][j] = new char[20];
					memset(rows[i][j], 0, 20);
					memcpy(rows[i][j], cur_res->res[i][j], 20);
				}
			}
			editArea->ShowSelResult(col_num, row_num, fields, rows);
			for (int i = 0; i < 5; i++) {
				delete[] fields[i];
			}
			delete[] fields;
			Destory_Result(&res);
			break;
		}
		case 2:
			//判断SQL语句为insert语句
			Insert(sql_str->sstr.ins.relName, sql_str->sstr.ins.nValues, sql_str->sstr.ins.values);
			pDoc->m_pTreeView->PopulateTree();
			break;
		case 3:
			//判断SQL语句为update语句
		{
			updates up = sql_str->sstr.upd;
			Update(up.relName, up.attrName, &up.value, up.nConditions, up.conditions);
			break;
		}

		case 4:
			//判断SQL语句为delete语句
			Delete(sql_str->sstr.del.relName, sql_str->sstr.del.nConditions, sql_str->sstr.del.conditions);
			break;

		case 5:
			//判断SQL语句为createTable语句
			CreateTable(sql_str->sstr.cret.relName, sql_str->sstr.cret.attrCount, sql_str->sstr.cret.attributes);
			pDoc->m_pTreeView->PopulateTree(); //更新视图
			pDoc->m_pListView->displayTabInfo(sql_str->sstr.cret.relName);//右侧刷新表名
			break;

		case 6:
			//判断SQL语句为dropTable语句
			DropTable(sql_str->sstr.cret.relName);
			pDoc->m_pTreeView->PopulateTree(); //更新视图
			pDoc->m_pListView->displayTabInfo(sql_str->sstr.cret.relName);//右侧刷新表名
			break;

		case 7:
			//判断SQL语句为createIndex语句
			CreateIndex(sql_str->sstr.crei.indexName, sql_str->sstr.crei.relName, sql_str->sstr.crei.attrName);
			break;

		case 8:
			//判断SQL语句为dropIndex语句
			DropIndex(sql_str->sstr.crei.indexName);
			break;

		case 9:
			//判断为help语句，可以给出帮助提示
			break;

		case 10:
			//判断为exit语句，可以由此进行退出操作
			break;
		}
		return SUCCESS;
	}
	else {
		AfxMessageBox(sql_str->sstr.errors);//弹出警告框，sql语句词法解析错误信息
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
	strcat(path_database_name, "\\");
	strcat(path_database_name, dbname);

	char path_systable_name[233];
	char path_syscolmn_name[233];

	strcpy(path_systable_name, path_database_name);
	strcat(path_systable_name, "\\SYSTABLES");

	strcpy(path_syscolmn_name, path_database_name);
	strcat(path_syscolmn_name, "\\SYSCOLUMNS");

	RC rc_table = RM_CreateFile(path_systable_name, SYS_TABLE_ROW_SIZE);
	RC rc_colmn = RM_CreateFile(path_syscolmn_name, SYS_COLMN_ROW_SIZE);
	if ((rc_table != SUCCESS) | (rc_colmn != SUCCESS)) {
		AfxMessageBox("创建数据库系统表失败！");
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
		AfxMessageBox("该表已经存在");
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
		strcpy(col_info, relName);
		strcpy(col_info + ATTRNAME_OFFSET, attr_i.attrName);
		memcpy(col_info + ATTRTYPE_OFFSET, &attr_i.attrType, ATTRTYPE_SIZE);
		memcpy(col_info + ATTRLENGTH_OFFSET, &attr_i.attrLength, ATTRLENGTH_SIZE);
		memcpy(col_info + ATTROFFSET_OFFSET, &attr_total_offset, ATTROFFSET_SIZE);
		memset(col_info + IX_FLAG_OFFSET, 0, IX_FLAG_SIZE + INDEXNAME_SIZE);		// ix is empty
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
		AfxMessageBox("表创建失败！");
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
		AfxMessageBox("该表不存在！");
		CloseScan(&table_scan);
		RM_CloseFile(sys_table_handle);
		return TABLE_NOT_EXIST;
	}
	DeleteRec(sys_table_handle, &table_rec.rid);			// 删除记录

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
		DeleteRec(sys_colmn_handle, &(colmn_rec.rid));		// 删除记录
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
		AfxMessageBox("不存在该表的该属性！");
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
		AfxMessageBox("该索引名已被使用");
		CloseScan(&colmn_scan);
		RM_CloseFile(sys_colmn_handle);
		return INDEX_NAME_REPEAT;
	}
	CloseScan(&colmn_scan);

	// if there is no index on this column
	char *cur_colmn = colmn_rec.pData;
	if (*(cur_colmn + IX_FLAG_OFFSET) == '1') {
		AfxMessageBox("该属性已经建立过索引！");
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
		AfxMessageBox("索引创建失败！");
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
		AfxMessageBox("该索引名不存在！");
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
		AfxMessageBox("该表不存在");
		CloseScan(&table_scan);
		RM_CloseFile(sys_table_handle);
		return TABLE_NOT_EXIST;
	}

	int attr_count = 0;
	memcpy(&attr_count, table_rec.pData + ATTRCOUNT_OFFSET, ATTRCOUNT_SIZE);
	if (nValues < attr_count) {
		AfxMessageBox("插入的字段不足！");
		return FIELD_MISSING;
	}
	if (nValues > attr_count) {
		AfxMessageBox("插入的字段太多！");
		return FIELD_REDUNDAN;
	}

	CloseScan(&table_scan);
	RM_CloseFile(sys_table_handle);

	// 参数的类型和查表的类型是反的

	// 检查插入的值的属性是否符合要求，记录是否需要建立索引
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
		// 检查属性是否相符
		if (attr_type != values[attr_count - 1 - i].type) {
			AfxMessageBox("插入的字段类型有误！");
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

	// 构建元组并插入
	char insert_value[233];
	for (int i = 0, offset = 0; i < attr_count; i++) {
		memcpy(insert_value + offset, values[attr_count - 1 - i].data, attr_len[i]);
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
		AfxMessageBox("插入失败！");
		RM_CloseFile(&table_handle);
		return INSERT_FAILED;
	}
	RM_CloseFile(&table_handle);

	// 构建索引项
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
				AfxMessageBox("构建索引项失败");
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

	RM_FileScan sys_table_scan;
	OpenScan(&sys_table_scan, sys_table_handle, 1, &find_con);
	RM_Record sys_table_rec;
	rc = GetNextRec(&sys_table_scan, &sys_table_rec);
	if (rc != SUCCESS) {
		AfxMessageBox("该表不存在");
		CloseScan(&sys_table_scan);
		RM_CloseFile(sys_table_handle);
		return TABLE_NOT_EXIST;
	}

	int attr_count = 0;
	memcpy(&attr_count, sys_table_rec.pData + ATTRCOUNT_OFFSET, ATTRCOUNT_SIZE);

	CloseScan(&sys_table_scan);
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
	Con *cons = new Con[attr_count];
	con_from_conditions(nConditions, conditions, attr_count, attr_name, attr_length, attr_offset, attr_type, cons);
	
	memset(open_path, 0, 233);
	strcpy(open_path, cur_db_pathname);
	strcat(open_path, "\\");
	strcat(open_path, relName);

	RM_FileHandle table_handle;
	RM_OpenFile(open_path, &table_handle);
	RM_FileScan table_scan;
	OpenScan(&table_scan, &table_handle, nConditions, cons);

	int total_rec = table_handle.rm_fileSubHeader->nRecords;
	int data_size = table_handle.rm_fileSubHeader->recordSize - sizeof(bool) - sizeof(RID);

	RID *removed_rid = new RID [total_rec];
	char **removed_data = new char* [total_rec];
	for (int i = 0; i < total_rec; i++) {
		removed_data[i] = new char [data_size];
	}

	int removed_num = 0;
	RM_Record record;
	while (GetNextRec(&table_scan, &record) == SUCCESS) {
		removed_rid[removed_num] = record.rid;
		memcpy(removed_data[removed_num], record.pData, data_size);
		removed_num++;
	}
	CloseScan(&table_scan);

	for (int i = 0; i < removed_num; i++) {
		DeleteRec(&table_handle, &removed_rid[i]);
	}

	RM_CloseFile(&table_handle);

	for (int i = 0; i < attr_count; i++) {
		if (attr_ix_flag[i]) {
			char index_path[255];
			strcpy(index_path, cur_db_pathname);
			strcat(index_path, "\\");
			strcat(index_path, relName);
			strcat(index_path, ".");
			strcat(index_path, attr_idxname[i]);

			IX_IndexHandle ix_indexHandle;
			OpenIndex(index_path, &ix_indexHandle);
			for (int j = 0; j < removed_num; j++) {
				DeleteEntry(&ix_indexHandle, removed_data[j] + attr_offset[i], &removed_rid[j]);
			}
			CloseIndex(&ix_indexHandle);
		}
	}

	for (int i = 0; i < total_rec; i++) {
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

RC Update(char *relName, char *attrName, Value *value, int nConditions, Condition *conditions) 
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

	RM_FileScan sys_table_scan;
	OpenScan(&sys_table_scan, sys_table_handle, 1, &find_con);
	RM_Record sys_table_rec;
	rc = GetNextRec(&sys_table_scan, &sys_table_rec);
	if (rc != SUCCESS) {
		AfxMessageBox("该表不存在");
		CloseScan(&sys_table_scan);
		RM_CloseFile(sys_table_handle);
		return TABLE_NOT_EXIST;
	}

	int attr_count = 0;
	memcpy(&attr_count, sys_table_rec.pData + ATTRCOUNT_OFFSET, ATTRCOUNT_SIZE);

	CloseScan(&sys_table_scan);
	RM_CloseFile(sys_table_handle);

	char **attr_name = new char*[attr_count];
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
	Con *cons = new Con[attr_count];
	con_from_conditions(nConditions, conditions, attr_count, attr_name, attr_length, attr_offset, attr_type, cons);

	int update_attr = 0;
	while (strcmp(attr_name[update_attr], attrName)) {
		update_attr++;
	}
	
	strcpy(open_path, cur_db_pathname);
	strcat(open_path, "\\");
	strcat(open_path, relName);

	RM_FileHandle table_handle;
	RM_OpenFile(open_path, &table_handle);
	RM_FileScan table_scan;
	OpenScan(&table_scan, &table_handle, nConditions, cons);

	int total_rec = table_handle.rm_fileSubHeader->nRecords;
	int data_size = table_handle.rm_fileSubHeader->recordSize - sizeof(bool) - sizeof(RID);
	int upattr_len = attr_length[update_attr];

	RID *removed_rid = new RID[total_rec];
	char **removed_data = new char*[total_rec];
	char **origin_data = new char*[total_rec];

	for (int i = 0; i < total_rec; i++) {
		removed_data[i] = new char[data_size];
		origin_data[i] = new char[upattr_len];
	}

	int removed_num = 0;
	RM_Record record;
	while (GetNextRec(&table_scan, &record) == SUCCESS) {
		removed_rid[removed_num] = record.rid;
		memcpy(origin_data[removed_num], record.pData + attr_offset[update_attr], upattr_len);
		memcpy(removed_data[removed_num], record.pData, data_size);
		memcpy(removed_data[removed_num] + attr_offset[update_attr], value->data, upattr_len);
		removed_num++;
	}
	CloseScan(&table_scan);

	for (int i = 0; i < removed_num; i++) {
		RM_Record update_rec;
		update_rec.bValid = true;
		update_rec.rid = removed_rid[i];
		update_rec.pData = removed_data[i];
		UpdateRec(&table_handle, &update_rec);
	}

	RM_CloseFile(&table_handle);

	if (attr_ix_flag[update_attr]) {
		char index_path[255];
		strcpy(index_path, cur_db_pathname);
		strcat(index_path, "\\");
		strcat(index_path, relName);
		strcat(index_path, ".");
		strcat(index_path, attr_idxname[update_attr]);

		IX_IndexHandle ix_indexHandle;
		OpenIndex(index_path, &ix_indexHandle);
		for (int j = 0; j < removed_num; j++) {
			DeleteEntry(&ix_indexHandle, origin_data[j], &removed_rid[j]);
			InsertEntry(&ix_indexHandle, removed_data[j] + attr_offset[update_attr], &removed_rid[j]);
		}
		CloseIndex(&ix_indexHandle);
	}

	for (int i = 0; i < total_rec; i++) {
		delete[] removed_data[i];
		delete[] origin_data[i];
	}
	delete[] removed_rid;
	delete[] removed_data;
	delete[] origin_data;

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

bool CanButtonClick()
{
	//如果当前有数据库已经打开
	if ((sys_table_handle != NULL) & (sys_colmn_handle != NULL))
		return true;

	//如果当前没有数据库打开
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


