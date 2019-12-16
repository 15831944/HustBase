#ifndef RM_MANAGER_H_H
#define RM_MANAGER_H_H

#include "PF_Manager.h"
#include "str.h"

typedef int SlotNum;

//��¼ID
typedef struct {	
	PageNum pageNum;	//��¼����ҳ��ҳ��
	SlotNum slotNum;		//��¼�Ĳ�ۺ�
	bool bValid; 			//true��ʾΪһ����Ч��¼�ı�ʶ��
}RID;

//��¼
typedef struct{
	bool bValid;		 // False��ʾ��δ�������¼
	RID  rid; 		 // ��¼�ı�ʶ�� 
	char *pData; 		 //��¼���洢������ 
}RM_Record;

//��¼ɨ��
typedef struct
{
	int bLhsIsAttr,bRhsIsAttr;//���������ҷֱ������ԣ�1������ֵ��0��
	AttrType attrType;//�����������ݵ�����
	int LattrLength,RattrLength;//�������ԵĻ�����ʾ���Եĳ���
	int LattrOffset,RattrOffset;//�������ԵĻ�����ʾ���Ե�ƫ����
	CompOp compOp;//�Ƚϲ�����
	void *Lvalue,*Rvalue;//����ֵ�Ļ���ָ���Ӧ��ֵ
}Con;

//��¼��Ϣ����ҳ 
typedef struct {
	int nRecords;			//��ǰ�ļ��а����ļ�¼��
	int recordSize;			//ÿ����¼�Ĵ�С
	int recordsPerPage;		//ÿ��ҳ�����װ�صļ�¼����
} RM_FileSubHeader;

//�ļ����
typedef struct{
	bool bOpen;//����Ƿ�򿪣��Ƿ����ڱ�ʹ�ã�
	PF_FileHandle *pf_fileHandle;//�ײ�ҳ����ƾ��
	RM_FileSubHeader *rm_fileSubHeader;//ҳ�������Ϣ
	char *rBitmap;//��¼��Ϣ����ҳ��λͼָ��
}RM_FileHandle;

//ɨ������
typedef struct{
	bool  bOpen;		//ɨ���Ƿ�� 
	RM_FileHandle  *pRMFileHandle;		//ɨ��ļ�¼�ļ����
	int  conNum;		//ɨ���漰���������� 
	Con  *conditions;	//ɨ���漰����������ָ��
    PF_PageHandle  PageHandle; //�����е�ҳ����
	PageNum  pn; 	//ɨ�輴�������ҳ���
	SlotNum  sn;		//ɨ�輴������Ĳ�ۺ�
}RM_FileScan;


//�ļ�ɨ�躯��
RC GetNextRec(RM_FileScan *rmFileScan,RM_Record *rec);

RC OpenScan(RM_FileScan *rmFileScan,RM_FileHandle *fileHandle,int conNum,Con *conditions);

RC CloseScan(RM_FileScan *rmFileScan);

//��¼��������
RC UpdateRec (RM_FileHandle *fileHandle,const RM_Record *rec);

RC DeleteRec (RM_FileHandle *fileHandle,const RID *rid);

RC InsertRec (RM_FileHandle *fileHandle, char *pData, RID *rid); 

RC GetRec (RM_FileHandle *fileHandle, RID *rid, RM_Record *rec); 

//��¼�ļ�����
RC RM_CloseFile (RM_FileHandle *fileHandle);

RC RM_OpenFile (char *fileName, RM_FileHandle *fileHandle);

RC RM_CreateFile (char *fileName, int recordSize);

#endif