//----------------------------------------------------------------------------
//	mapfile.h
//  Copyright 2006, Flagship Studios
//----------------------------------------------------------------------------
#pragma once

class MapFile {

    private:
        HANDLE m_hFile;
        HANDLE m_hMapping;
        void*  m_pBase;
        bool   m_bInitialized;
        LARGE_INTEGER m_FileSize;
    
        MapFile();
        void DefaultInit()
        {
            m_bInitialized = false;
            m_pBase = NULL;
            m_FileSize.HighPart = 0;
            m_FileSize.LowPart = 0;
            m_hFile = INVALID_HANDLE_VALUE;
            m_hMapping = NULL;
        }
		void Cleanup(void) {
            if(m_pBase) {
                UnmapViewOfFile(m_pBase);
            }
            if(m_hMapping) {
                CloseHandle(m_hMapping);
            }
            if(m_hFile != INVALID_HANDLE_VALUE) {
                CloseHandle(m_hFile);
            }
            m_bInitialized = false;
       }
    public:
       MapFile(char* filename) {

            DefaultInit();

            m_hFile = CreateFile(filename ,
                                 GENERIC_READ,
                                 FILE_SHARE_READ,
                                 NULL,
								 OPEN_EXISTING,
                                 FILE_ATTRIBUTE_NORMAL |
                                     FILE_FLAG_SEQUENTIAL_SCAN |
                                     FILE_FLAG_OVERLAPPED,
                                 NULL);

            m_FileSize.LowPart = GetFileSize(m_hFile,(DWORD*)&m_FileSize.HighPart);

            if(m_hFile != INVALID_HANDLE_VALUE) {
                m_hMapping = CreateFileMapping(m_hFile,NULL,
                                               PAGE_READONLY,0,0,NULL);

                if(m_hMapping) {
                        m_pBase = MapViewOfFile(m_hMapping,FILE_MAP_READ,0,0,0);
                }
                else {
                    CloseHandle(m_hFile);
                }
            }
            if(m_pBase) {
                m_bInitialized = true;
            }
            else {
                Cleanup();
            }
       }
       MapFile(DWORD dwHighSize, 
               DWORD dwLowSize,
               char *mappingName,
               SECURITY_ATTRIBUTES *pSa,
               BOOL bFailOnExisting = FALSE)
       {
           DefaultInit();
           m_FileSize.HighPart = dwHighSize;
           m_FileSize.LowPart = dwLowSize;

           m_hMapping = CreateFileMapping(INVALID_HANDLE_VALUE,pSa,
                                           PAGE_READWRITE,
                                           dwHighSize,
                                           dwLowSize,
                                           mappingName);

           if(m_hMapping && !(bFailOnExisting && (GetLastError() == ERROR_ALREADY_EXISTS) ) ) {
               m_pBase = MapViewOfFile(m_hMapping,FILE_MAP_WRITE,0,0,0);
           }
            if(m_pBase) {
                m_bInitialized = true;
            }
            else {
                Cleanup();
            }
       }
       ~MapFile() {
            Cleanup();
        }
        void *Get(LARGE_INTEGER *fileSize) {

            if(!m_bInitialized)
                return NULL;

            *fileSize = m_FileSize;
            return m_pBase;
        }
};
