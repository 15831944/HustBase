#include "stdafx.h"
#include "EditArea.h"
#include "SYS_Manager.h"
#include "QU_Manager.h"
#include <iostream>

char cur_db_pathname[233];

// 12/13
// for quiery operations
RM_FileHandle *sys_table_handle;
RM_FileHandle *sys_colmn_handle;

void ExecuteAndMessage(char * sql,CEditArea* editArea){//根据执行的语句类型在界面上显示执行结果。此函数需修改
	std::string s_sql = sql;
	RC rc = execute(sql);
	int row_num = 0;
	char**messages;
	switch(rc){
	case SUCCESS:
		row_num = 1;
		messages = new char*[row_num];
		messages[0] = "操作成功";
		editArea->ShowMessage(row_num,messages);
		delete[] messages;
		break;
	case SQL_SYNTAX:
		row_num = 1;
		messages = new char*[row_num];
		messages[0] = "有语法错误";
		editArea->ShowMessage(row_num,messages);
		delete[] messages;
		break;
	default:
		row_num = 1;
		messages = new char*[row_num];
		messages[0] = "功能未实现";
		editArea->ShowMessage(row_num,messages);
	delete[] messages;
		break;
	}
}

RC execute(char * sql){
	sqlstr *sql_str = NULL;
	RC rc;
	sql_str = get_sqlstr();
  	rc = parse(sql, sql_str);//只有两种返回结果SUCCESS和SQL_SYNTAX
	
	if (rc == SUCCESS)
	{
		int i = 0;
		switch (sql_str->flag)
		{
			case 1:
			//判断SQL语句为select语句
			break;

			case 2:
			//判断SQL语句为insert语句
			break;

			case 3:	
			//判断SQL语句为update语句
			break;

			case 4:					
			//判断SQL语句为delete语句
			break;

			case 5:
			//判断SQL语句为createTable语句
			break;

			case 6:	
			//判断SQL语句为dropTable语句
			break;

			case 7:
			//判断SQL语句为createIndex语句
			break;
	
			case 8:	
			//判断SQL语句为dropIndex语句
			break;
			
			case 9:
			//判断为help语句，可以给出帮助提示
			break;
		
			case 10: 
			//判断为exit语句，可以由此进行退出操作
			break;		
		}
	}else{
		AfxMessageBox(sql_str->sstr.errors);//弹出警告框，sql语句词法解析错误信息
		return rc;
	}
}

// 12/12
//- DataBase
// | - SysTable
// | - SysColumn

// 12/12
// --- INTERFACE ---
// Require RM_CreateFile() to create a file in a SPECIFIC PATH!
RC CreateDB(char *dbpath,char *dbname)
{
	char path_database_name[233];  
	strcpy(path_database_name, dbpath);
	strcat(path_database_name, "\\");
	strcat(path_database_name, dbname);

	char path_systable_name[233];
	char path_syscolmn_name[233];

	bool flag = CreateDirectory(path_database_name, NULL);
	if (!flag) {
		printf("create database directory failed!\n");
		return SQL_SYNTAX; // failed
	}

	strcpy(path_systable_name, path_database_name);
	strcat(path_systable_name, "\\SYSTABLES");

	strcpy(path_syscolmn_name, path_database_name);
	strcat(path_syscolmn_name, "\\SYSCOLUMNS");

	// --- DIFFERENCE & Q ---
	// What dose a record contain? 
	// --- A ---
	// yzy will add the length if she needs it :)

	RM_CreateFile(path_systable_name, SYS_TABLE_ROW_SIZE);
	RM_CreateFile(path_syscolmn_name, SYS_COLMN_ROW_SIZE);

	return SUCCESS;
}

// different directory structure, different way to delete
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
	return SUCCESS;
}

RC CloseDB()
{
	delete sys_table_handle;
	delete sys_colmn_handle;
	memset(cur_db_pathname, 0, 233);
	return SUCCESS;
}

// 12/13
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
	Con table_con;
	table_con.bLhsIsAttr = 1;
	table_con.bRhsIsAttr = 0;
	table_con.attrType = chars;
	table_con.LattrLength = 21;
	table_con.LattrOffset = 0;
	table_con.compOp = EQual;
	table_con.Rvalue = relName;

	RM_FileScan table_scan;
	OpenScan(&table_scan, sys_table_handle, 1, &table_con);
	RM_Record table_rec;
	rc = GetNextRec(&table_scan, &table_rec);
	if (rc != SUCCESS) {
		printf("The table already exists!\n");
		CloseScan(&table_scan);
		RM_CloseFile(sys_table_handle);
		return rc;
	}
	CloseScan(&table_scan);

	// insert into systables
	char *new_table = new char[SYS_TABLE_ROW_SIZE];
	strcpy(new_table, relName);
	memcpy(new_table + TABLENAME_SIZE, &attrCount, sizeof(int));

	RID tmp_rid;
	rc = InsertRec(sys_table_handle, new_table, &tmp_rid);
	if (rc != SUCCESS) {
		printf("Insert into systable failed!\n");
		return rc;
	}

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
	RM_CreateFile(new_table_path, attr_total_offset);

	return SUCCESS;
}

RC DropTable(char *relName) {
	return SUCCESS;
}

RC CreateIndex(char *indexName, char *relName, char *attrName) {
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
