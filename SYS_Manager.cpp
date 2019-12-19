#include "stdafx.h"
#include "EditArea.h"
#include "SYS_Manager.h"
#include "QU_Manager.h"
#include <iostream>

#include "HustBaseDoc.h"

char cur_db_pathname[233];

// 12/13
// for quiery operations
RM_FileHandle *sys_table_handle;
RM_FileHandle *sys_colmn_handle;

void ExecuteAndMessage(char *sql, CEditArea* editArea, CHustBaseDoc* pDoc)
{//根据执行的语句类型在界面上显示执行结果。此函数需修改
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
	rc = parse(sql, sql_str);//只有两种返回结果SUCCESS和SQL_SYNTAX

	if (rc == SUCCESS)
	{
		switch (sql_str->flag)
		{
		case 1:
			//判断SQL语句为select语句
			break;
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
			//pDoc->m_pListView->displayTabInfo(sql_str->sstr.cret.relName);//右侧刷新表名
			break;

		case 6:
			//判断SQL语句为dropTable语句
			DropTable(sql_str->sstr.cret.relName);
			pDoc->m_pTreeView->PopulateTree(); //更新视图
			//pDoc->m_pListView->displayTabInfo(sql_str->sstr.cret.relName);//右侧刷新表名
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
		AfxMessageBox("The table dose not exist");
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

	// TO DO: delete index files

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

	char *attr_pdata = new char[attrLength * sizeof(char)];
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

RC DropIndex(char *indexName) {
	return SUCCESS;
}

RC Insert(char *relName, int nValues, Value * values) {
	return SUCCESS;
}

RC Delete(char *relName, int nConditions, Condition *conditions) {
	return SUCCESS;
}

RC Update(char *relName, char *attrName, Value *value, int nConditions, Condition *conditions) {
	return SUCCESS;
}

bool CanButtonClick(){//需要重新实现
	//如果当前有数据库已经打开
	return true;
	//如果当前没有数据库打开
	//return false;
}
