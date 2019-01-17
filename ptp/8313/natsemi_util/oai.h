//****************************************************************************
// oai.h
// 
// Copyright (c) 2006-2007 National Semiconductor Corporation.
// All Rights Reserved
// 
// OS Abstraction Interface (OAI) include file.
//
// This can be customized as needed for a target platform.
//****************************************************************************


typedef struct OAI_DEV_HANDLE_STRUCT {
    uint x;
} OAI_DEV_HANDLE_STRUCT;

typedef OAI_DEV_HANDLE_STRUCT * OAI_DEV_HANDLE;

typedef enum {
    INT_NSC_MICRO_MDIO,
    INT_DIRECT_CONNECT,
    INT_FDI
}INTERFACE_TYPE;


void *OAIAlloc( IN NS_UINT sizeInBytes);
void OAIFree( IN void *memPtr);
NS_BOOL	OAIMdioReadBit( IN OAI_DEV_HANDLE oaiDevHandle);
void OAIMdioWriteBit( IN OAI_DEV_HANDLE oaiDevHandle, IN NS_BOOL bit);
NS_UINT
	OAIDirReadReg (
        IN OAI_DEV_HANDLE oaiDevHandle,
        IN NS_UINT regIndex);
void
	OAIDirWriteReg (
        IN OAI_DEV_HANDLE oaiDevHandle,
        IN NS_UINT regIndex,
        IN NS_UINT value);
NS_UINT
	OAIMacReadReg (
        IN OAI_DEV_HANDLE oaiDevHandle,
        IN NS_UINT8 *readRegRequestPacket,
        IN NS_UINT length);
void
	OAIMacWriteReg (
        IN OAI_DEV_HANDLE oaiDevHandle,
        IN NS_UINT8 *writeRegRequestPacket,
        IN NS_UINT length);
void
	OAIManagementError(
        IN OAI_DEV_HANDLE oaiDevHandle);



// Define EXPORTED if we're building for Windows
#ifdef EXPORTED
    #define EXPORT __declspec(dllexport)
#else
    #define EXPORT
#endif

