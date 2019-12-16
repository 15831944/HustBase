#ifndef SYS_MANAGER_H_H
#define SYS_MANAGER_H_H

#include "IX_Manager.h"
#include "PF_Manager.h"
#include "RM_Manager.h"
#include "str.h"

#define SYS_TABLE_ROW_SIZE (21 + 4)
#define SYS_COLMN_ROW_SIZE (21 + 21 + 4 + 4 + 4 + 1 + 21)

#define TABLENAME_SIZE  (21)
#define ATTRNAME_SIZE   (21)
#define INDEXNAME_SIZE  (21)
#define ATTRTYPE_SIZE   (4)
#define ATTRLENGTH_SIZE (4)
#define ATTROFFSET_SIZE (4)
#define IX_FLAG_SIZE	(1)
#define INDEXNAME_SIZE  (21)

void ExecuteAndMessage(char * ,CEditArea*);
bool CanButtonClick();

RC CreateDB(char *dbpath,char *dbname);
RC DropDB(char *dbname);
RC OpenDB(char *dbname);
RC CloseDB();

RC execute(char * sql);

RC CreateTable(char *relName,int attrCount,AttrInfo *attributes);
RC DropTable(char *relName);
RC CreateIndex(char *indexName,char *relName,char *attrName);
RC DropIndex(char *indexName);
RC Insert(char *relName,int nValues,Value * values);
RC Delete(char *relName,int nConditions,Condition *conditions);
RC Update(char *relName,char *attrName,Value *value,int nConditions,Condition *conditions);

#endif