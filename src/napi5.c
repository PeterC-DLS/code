#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <time.h>

  typedef struct __NexusFile5 {
        struct iStack5 {
          int *iRefDir;
          int *iTagDir;
          char irefn[1024];
          int iVref;
        } iStack5[NXMAXSTACK];
        struct iStack5 iAtt5;
        int iVID;
        int iFID;
        int iCurrentG;
        int iCurrentD;
        int iCurrentS;
        int iCurrentT;
        int iCurrentIDX;
        unsigned int iCurrentA_IDX;
        int iCurrentA;
        int iNX;
        int iNXID;
        int iStackPtr;
        char *iCurrentLG;
        char *iCurrentLGG;
        char *iCurrentLD;
        char name_ref[1024];
        char iAccess[2];
  } NexusFile5, *pNexusFile5;

    /*---------------------------------------------------------------------*/

  static void NXNXNXReportError5(void *pData, char *string)
  {
    printf("%s \n",string);
  }
     
  /*---------------------------------------------------------------------*/

  void *NXpData5 = NULL;
  void (*NXIReportError5)(void *pData, char *string) = NXNXNXReportError5;

  /*---------------------------------------------------------------------*/

  void CALLING_STYLE NXMSetError5(void *pData, void (*NewError)(void *pD, char *text))
  {
    NXpData5 = pData;
    NXIReportError5 = NewError;
  }
  
  /*--------------------------------------------------------------------*/

  static pNexusFile5 NXI5assert(NXhandle fid)
  {
    pNexusFile5 pRes;
  
    assert(fid != NULL);
    pRes = (pNexusFile5)fid;
    assert(pRes->iNXID == NX5SIGNATURE);
    return pRes;
  }
  
  /*--------------------------------------------------------------------*/

   static void NXI5KillDir (pNexusFile5 self)
  {
    if (self->iStack5[self->iStackPtr].iRefDir) {
      free (self->iStack5[self->iStackPtr].iRefDir);
      self->iStack5[self->iStackPtr].iRefDir = NULL;
    }
    if (self->iStack5[self->iStackPtr].iTagDir) {
      free (self->iStack5[self->iStackPtr].iTagDir);
      self->iStack5[self->iStackPtr].iTagDir = NULL;
    }
  }
  
  /*--------------------------------------------------------------------*/

  static void NXI5KillAttDir (pNexusFile5 self)
  {
    if (self->iAtt5.iRefDir) {
      free (self->iAtt5.iRefDir);
      self->iAtt5.iRefDir = NULL;
    }
    if (self->iAtt5.iTagDir) {
      free (self->iAtt5.iTagDir);
      self->iAtt5.iTagDir = NULL;
    }
  }

  /* ---------------------------------------------------------------------- 
  
                          Definition of NeXus API

   ---------------------------------------------------------------------*/

  NXstatus NX5open(CONSTCHAR *filename, NXaccess am, NXhandle* pHandle)
  {
  hid_t attr1,aid1, aid2;
  pNexusFile5 pNew = NULL;
  char pBuffer[512], time_buffer[64];
  char version_nr[10];
  int iRet;
  time_t timer;
  struct tm *time_info;
  const char* time_format;
  long gmt_offset;
  unsigned int vers_major, vers_minor, vers_release, am1 ;

#ifdef USE_FTIME
    struct timeb timeb_struct;
#endif 

    *pHandle = NULL;

    pNew = (pNexusFile5) malloc (sizeof (NexusFile5));
    if (!pNew) {
      NXIReportError5 (NXpData5,"ERROR: no memory to create File datastructure");
      return NX_ERROR;
    }
    memset (pNew, 0, sizeof (NexusFile5));

#ifdef NEED_TZSET
    tzset();
#endif 
    time(&timer);
#ifdef USE_FTIME
    ftime(&timeb_struct);
    gmt_offset = -timeb_struct.timezone * 60;
    if (timeb_struct.dstflag != 0)
    {
        gmt_offset += 3600;
    }
#else
    time_info = gmtime(&timer);
    if (time_info != NULL)
    {
        gmt_offset = difftime(timer, mktime(time_info));
    }
    else
    {
        NXIReportError5 (NXpData5, 
        "Your gmtime() function does not work ... timezone information will be incorrect\n");
        gmt_offset = 0;
    }
#endif 
    time_info = localtime(&timer);
    if (time_info != NULL)
    {
        if (gmt_offset < 0)
        {
            time_format = "%04d-%02d-%02d %02d:%02d:%02d-%02d%02d";
        }
        else
        {
            time_format = "%04d-%02d-%02d %02d:%02d:%02d+%02d%02d";
        }
        sprintf(time_buffer, time_format,
            1900 + time_info->tm_year,
            1 + time_info->tm_mon,
            time_info->tm_mday,
            time_info->tm_hour,
            time_info->tm_min,
            time_info->tm_sec,
            abs(gmt_offset / 3600),
            abs((gmt_offset % 3600) / 60)
        );
    }
    else
    {
        strcpy(time_buffer, "1970-01-01 00:00:00+0000");
    }
    /* start HDF5 interface */
    if (am == NXACC_CREATE5) {  
       am1 = H5F_ACC_TRUNC;
       pNew->iFID = H5Fcreate (filename, am1,H5P_DEFAULT,H5P_DEFAULT);
    } else {
       if (am == NXACC_READ) {
          am1 = H5F_ACC_RDONLY;
        } else {
          am1 = H5F_ACC_RDWR;
        }    
        pNew->iFID = H5Fopen (filename, am1, H5P_DEFAULT);
    }  
    if (pNew->iFID <= 0) {
      sprintf (pBuffer, "ERROR: cannot open file: %s", filename);
      NXIReportError5 (NXpData5, pBuffer);
      free (pNew);
      return NX_ERROR;
    }

/*
 * need to create global attributes         file_name file_time NeXus_version 
 * at some point for new files
 */
    if (am1 != H5F_ACC_RDONLY) 
    {
        pNew->iVID=H5Gopen(pNew->iFID,"/");
        aid2 = H5Screate(H5S_SCALAR);
        aid1 = H5Tcopy(H5T_C_S1);
        H5Tset_size(aid1, strlen(NEXUS_VERSION));
        if (am1 == H5F_ACC_RDWR)
        {
        H5Adelete(pNew->iVID, "NeXus_version"); 
        }
        attr1= H5Acreate(pNew->iVID, "NeXus_version", aid1, aid2, H5P_DEFAULT);
        if (attr1<0)
        {
          NXIReportError5 (NXpData5, 
                         "ERROR: HDF failed to store NeXus_version attribute ");
          return NX_ERROR;
        } 
        if (H5Awrite(attr1, aid1,NEXUS_VERSION)<0)
        {
          NXIReportError5 (NXpData5, 
                         "ERROR: HDF failed to store NeXus_version attribute ");
          return NX_ERROR;
        }
       /* Close attribute dataspace  */
       iRet = H5Tclose(aid1);
       iRet = H5Sclose(aid2); 
       /* Close attribute */
       iRet = H5Aclose(attr1); 
       H5Gclose(pNew->iVID); 
    }
    if (am1 == H5F_ACC_TRUNC) 
    {
        pNew->iVID=H5Gopen(pNew->iFID,"/");
        aid2=H5Screate(H5S_SCALAR);
        aid1 = H5Tcopy(H5T_C_S1);
        H5Tset_size(aid1, strlen(filename));
        attr1= H5Acreate(pNew->iVID, "file_name", aid1, aid2, H5P_DEFAULT);
        if (attr1 < 0)
        {
          NXIReportError5 (NXpData5, "ERROR: HDF failed to store file_name attribute ");
          return NX_ERROR;
        }
        if (H5Awrite(attr1, aid1, (char*)filename) < 0)
        {
          NXIReportError5 (NXpData5, "ERROR: HDF failed to store file_name attribute ");
          return NX_ERROR;
        }
        H5get_libversion(&vers_major, &vers_minor, &vers_release);
        sprintf (version_nr, "%d.%d.%d", vers_major,vers_minor,vers_release);
        aid2=H5Screate(H5S_SCALAR);
        aid1 = H5Tcopy(H5T_C_S1);
        H5Tset_size(aid1, strlen(version_nr));
        attr1= H5Acreate(pNew->iVID, "HDF5_Version", aid1, aid2, H5P_DEFAULT);
        if (attr1 < 0)
        {
          NXIReportError5 (NXpData5, "ERROR: HDF failed to store file_name attribute ");
          return NX_ERROR;
        }
        if (H5Awrite(attr1, aid1, (char*)version_nr) < 0)
        {
          NXIReportError5 (NXpData5, "ERROR: HDF failed to store file_name attribute ");
          return NX_ERROR;
        }
        H5Tset_size(aid1, strlen(time_buffer));
        attr1=H5Acreate(pNew->iVID, "file_time", aid1, aid2, H5P_DEFAULT);
        if (attr1 < 0)
        {
          NXIReportError5 (NXpData5, "ERROR: HDF failed to store file_time attribute ");
          return NX_ERROR;
        }
        if (H5Awrite(attr1, aid1, time_buffer) < 0)
        {
          NXIReportError5 (NXpData5, "ERROR: HDF failed to store file_time attribute ");
          return NX_ERROR;
        }
        /* Close attribute dataspace */
       iRet = H5Tclose(aid1);
       iRet = H5Sclose(aid2); 
       /* Close attribute */
       iRet = H5Aclose(attr1); 
       H5Gclose(pNew->iVID); 
    }
    /* Set HDFgroup access mode */
    if (am1 == H5F_ACC_RDONLY) {
      strcpy(pNew->iAccess,"r");
    } else {
      strcpy(pNew->iAccess,"w");
    }
    pNew->iNXID = NX5SIGNATURE;
    pNew->iStack5[0].iVref = 0;    /* root! */
    *pHandle = (NXhandle)pNew;
     return NX_OK;
  }
 
  /* ------------------------------------------------------------------------- */

  NXstatus CALLING_STYLE NX5close (NXhandle* fid)
  {
    pNexusFile5 pFile = NULL;
    int iRet;
 
     pFile=NXI5assert(*fid);
    iRet=0;
    iRet = H5Fclose(pFile->iFID);
    if (iRet < 0) {
      NXIReportError5 (NXpData5, "ERROR: HDF cannot close HDF file");
    }
    /* release memory */
    NXI5KillDir (pFile);
    free (pFile);
    *fid = NULL;
    return NX_OK;
  }

 /*-----------------------------------------------------------------------*/   

  NXstatus CALLING_STYLE NX5makegroup (NXhandle fid, CONSTCHAR *name, char *nxclass) 
  {
    pNexusFile5 pFile;
    hid_t iRet;
    hid_t attr1,aid1, aid2;
    
    pFile = NXI5assert (fid);
    /* create and configure the group */
    if (pFile->iCurrentG==0)
    {
       pFile->iVID = H5Gcreate(pFile->iFID,(const char*)name, 0);
    } else
    {
       strcat(pFile->name_ref,"/");
       strcat(pFile->name_ref,name);
       pFile->iVID = H5Gcreate(pFile->iFID,(const char*)pFile->name_ref, 0);
    }   
    if (pFile->iVID < 0) {
      NXIReportError5 (NXpData5, "ERROR: HDF could not create Group");
      return NX_ERROR;
    }
    aid2 = H5Screate(H5S_SCALAR);
    aid1 = H5Tcopy(H5T_C_S1);
    H5Tset_size(aid1, strlen(nxclass));
    attr1= H5Acreate(pFile->iVID, "NX_class", aid1, aid2, H5P_DEFAULT);
    if (attr1 < 0)
       {
       NXIReportError5 (NXpData5, "ERROR: HDF failed to store class name!");
       return NX_ERROR;
       }
    if (H5Awrite(attr1, aid1, (char*)nxclass) < 0)
      {
      NXIReportError5 (NXpData5, "ERROR: HDF failed to store class name!");
      return NX_ERROR;
      }
    /* close group */
    iRet=H5Sclose(aid2);
    iRet=H5Tclose(aid1);
    iRet=H5Aclose(attr1);
    iRet=H5Gclose(pFile->iVID);
    return NX_OK;
  }
  
  /*------------------------------------------------------------------------*/

  herr_t attr_check (hid_t loc_id, const char *member_name, void *opdata)
  {
    char attr_name[8];
    
    strcpy(attr_name,"NX_class");
    return strstr(member_name, attr_name) ? 1 : 0;
  }

  NXstatus CALLING_STYLE NX5opengroup (NXhandle fid, CONSTCHAR *name, char *nxclass)
  {

    pNexusFile5 pFile;
    hid_t iRet, attr1, atype;
    char data[80];
          
    pFile = NXI5assert (fid);
    if (pFile->iCurrentG==0)
    {
       pFile->iCurrentG = H5Gopen (pFile->iFID,(const char *)name);
       /* check group attribute */
       iRet=H5Aiterate(pFile->iCurrentG,NULL,attr_check,NULL);
       if (iRet < 0) {
          NXIReportError5 (NXpData5, "ERROR iterating thourgh group!");
          return NX_ERROR;  
       } else if (iRet == 1) {
         /* group attribute was found */
       } else {
         /* no group attribute available */
         NXIReportError5 (NXpData5, "No group attribute available");
         return NX_ERROR;
       }
       /* check contains of group attribute */
       attr1 = H5Aopen_name(pFile->iCurrentG, "NX_class");
       atype=H5Tcopy(H5T_C_S1);
       H5Tset_size(atype,64);  
       iRet = H5Aread(attr1, atype, data);
       if (strcmp(data, nxclass) == 0) {
          /* test OK */
       } else {
          NXIReportError5 (NXpData5, "Group class is not identical!");
          return NX_ERROR; 
       }          
       iRet = H5Tclose(atype);
       iRet = H5Aclose(attr1); 
       pFile->iStack5[pFile->iStackPtr].iVref=0;
       strcpy(pFile->iStack5[pFile->iStackPtr].irefn,"");
       strcpy(pFile->name_ref,name);
    } else {
       pFile->iCurrentG = H5Gopen (pFile->iFID,(const char *)pFile->name_ref);
       /* check group attribute */
       iRet=H5Aiterate(pFile->iCurrentG,NULL,attr_check,NULL);
       if (iRet < 0) {
          NXIReportError5 (NXpData5, "ERROR iterating thourgh group!");
          return NX_ERROR;  
       } else if (iRet == 1) {
         /* group attribute was found */
       } else {
         /* no group attribute available */
         NXIReportError5 (NXpData5, "No group attribute available");
         return NX_ERROR;
       }
       /* check contains of group attribute */
       attr1 = H5Aopen_name(pFile->iCurrentG, "NX_class");
       atype=H5Tcopy(H5T_C_S1);
       H5Tset_size(atype,64);  
       iRet = H5Aread(attr1, atype, data);
       if (strcmp(data, nxclass) == 0) {
          /* test OK */
       } else {
          NXIReportError5 (NXpData5, "Group class is not identical!");
          return NX_ERROR; 
       }          
       iRet = H5Tclose(atype);
       iRet = H5Aclose(attr1); 
    }
    pFile->iStackPtr++;
    pFile->iStack5[pFile->iStackPtr].iVref=pFile->iCurrentG;
    strcpy(pFile->iStack5[pFile->iStackPtr].irefn,name);
    pFile->iCurrentIDX=0;
    pFile->iCurrentA_IDX=0;
    if (pFile->iCurrentLG != NULL) {
       free(pFile->iCurrentLG);
    }
    if (pFile->iCurrentLGG != NULL) {
       free(pFile->iCurrentLGG);
    }
    pFile->iCurrentLG = strdup(name);
    pFile->iCurrentLGG = strdup(name);
    NXI5KillDir (pFile);
    return NX_OK;
  }

  /* ------------------------------------------------------------------- */

  NXstatus CALLING_STYLE NX5closegroup (NXhandle fid)
  {
    pNexusFile5 pFile;
    int i,ii;
    char *uname;
    
    pFile = NXI5assert (fid);
    /* first catch the trivial case: we are at root and cannot get 
       deeper into a negative directory hierarchy (anti-directory)
     */
    if (pFile->iCurrentG == 0) {
      NXI5KillDir (pFile);
      return NX_OK;
    } else {                      
      /* close the current group and decrement name_ref */
      H5Gclose (pFile->iCurrentG);
      i=0;
      i=strlen(pFile->iStack5[pFile->iStackPtr].irefn);
      ii=strlen(pFile->name_ref);
      ii=ii-i;
      uname=strdup(pFile->name_ref);
      strcpy(pFile->name_ref,"");
      if (ii>1)
      {
        strncpy(pFile->name_ref,uname,ii);
      } else {
        strcpy(pFile->name_ref,"");
      }
      pFile->iStackPtr--;
      if (pFile->iStackPtr>0)
      {
      pFile->iCurrentG=pFile->iStack5[pFile->iStackPtr].iVref;
      } else {
      pFile->iCurrentG=0;
      }
      NXI5KillDir (pFile);
    }
    pFile->iCurrentIDX=0;
    pFile->iCurrentA_IDX=0;
    return NX_OK;
  }
  
  /* --------------------------------------------------------------------- */

  NXstatus CALLING_STYLE NX5makedata (NXhandle fid, CONSTCHAR *name, int datatype, 
                                  int rank, int dimensions[])
  {
  pNexusFile5 pFile;
  
  pFile = NXI5assert (fid);
  return NX5compmakedata (fid, name, datatype, rank, dimensions, NX_COMP_NONE, 1);
    
  return NX_OK;
  }

 /* --------------------------------------------------------------------- */

  NXstatus CALLING_STYLE NX5compmakedata (NXhandle fid, CONSTCHAR *name, int datatype, 
                           int rank, int dimensions[],int compress_type, int chunk_size)
  {
    hid_t datatype1, dataspace, iNew;
    hid_t type,cparms;
    pNexusFile5 pFile;
    char pBuffer[256];
    int i, iRet, byte_zahl;
    hsize_t chunkdims[3]={chunk_size,chunk_size,chunk_size};
    hsize_t mydim[H5S_COMPLEX], mydim1[H5S_COMPLEX];  
    hsize_t size[2];
    hsize_t maxdims[1] = {H5S_UNLIMITED};

    pFile = NXI5assert (fid);
  
    if (datatype == NX_CHAR)
    {
        type=H5T_C_S1;
    }
    else if (datatype == NX_INT8)
    {
        type=H5T_NATIVE_CHAR;
    }
    else if (datatype == NX_UINT8)
    {
        type=H5T_NATIVE_UCHAR;
    }
    else if (datatype == NX_INT16)
    {
        type=H5T_NATIVE_SHORT;
    }
    else if (datatype == NX_UINT16)
    {
        type=H5T_NATIVE_USHORT;
    }
    else if (datatype == NX_INT32)
    {
        type=H5T_NATIVE_INT;
    }
    else if (datatype == NX_UINT32)
    {
        type=H5T_NATIVE_UINT;
    }
    else if (datatype == NX_FLOAT32)
    {
        type=H5T_NATIVE_FLOAT;
    }
    else if (datatype == NX_FLOAT64)
    {
        type=H5T_NATIVE_DOUBLE;
    }
    if (rank <= 0) {
      sprintf (pBuffer, "ERROR: invalid rank specified %s",
               name);
      NXIReportError5 (NXpData5, pBuffer);
      return NX_ERROR;
    }
    /*
      Check dimensions for consistency. The first dimension may be 0
      thus denoting an unlimited dimension.
    */
    for (i = 1; i < rank; i++) {
      if (dimensions[i] <= 0) {
        sprintf (pBuffer,
                 "ERROR: invalid dimension %d, value %d given for Dataset %s",
                 i, dimensions[i], name);
        NXIReportError5 (NXpData5, pBuffer);
        return NX_ERROR;
      }
    }
    if (datatype == NX_CHAR)
    {
      byte_zahl=dimensions[0]; 
      dimensions[0]=rank;
      for(i = 0; i < rank; i++)
         {
         mydim1[i] = dimensions[i];
         }
         dataspace=H5Screate_simple(rank,mydim1,NULL);
    } else {
      if (dimensions[0] == NX_UNLIMITED)
      {
        mydim[0]=0;
        dataspace=H5Screate_simple(rank, mydim, maxdims);
      } else {
        for(i = 0; i < rank; i++)
        {
        mydim[i] = dimensions[i];
        }
        /* dataset creation */
        dataspace=H5Screate_simple(rank, mydim, NULL);  
        }
    }  
    datatype1=H5Tcopy(type);
    if (datatype == NX_CHAR)
    {
       H5Tset_size(datatype1, byte_zahl);
    }
    if(compress_type == NX_COMP_LZW)
    {
      cparms = H5Pcreate(H5P_DATASET_CREATE);
      iNew = H5Pset_chunk(cparms,rank,chunkdims);
      H5Pset_deflate(cparms,6); 
      pFile->iCurrentD = H5Dcreate (pFile->iCurrentG, (char*)name, datatype1, 
                     dataspace, cparms);   
    }
    else if (compress_type == NX_COMP_NONE)
    {
      NXIReportError5 (NXpData5, "No compression method selected!");
      if (dimensions[0] == NX_UNLIMITED) {
         cparms = H5Pcreate(H5P_DATASET_CREATE);
         iNew = H5Pset_chunk(cparms,rank,chunkdims);
         pFile->iCurrentD = H5Dcreate (pFile->iCurrentG, (char*)name, datatype1, 
                     dataspace, cparms);   
      } else {
         pFile->iCurrentD = H5Dcreate (pFile->iCurrentG, (char*)name, datatype1, 
                     dataspace, H5P_DEFAULT);
      }               
    }
    else 
    {
      NXIReportError5 (NXpData5, "HDF5 don't support selected compression method!");
      pFile->iCurrentD = H5Dcreate (pFile->iCurrentG, (char*)name, datatype1, 
                     dataspace, H5P_DEFAULT); 
    }
    if (dimensions[0] == NX_UNLIMITED)
    {      
      size[0]   = 1; 
      size[1]   = 1; 
      iNew = H5Dextend (pFile->iCurrentD, size);
    }
    if (iNew < 0) {
      sprintf (pBuffer, "ERROR: cannot create Dataset %s, check arguments",
               name);
      NXIReportError5 (NXpData5, pBuffer);
      return NX_ERROR;
    }
    iRet=H5Sclose(dataspace);
    iRet=H5Tclose(datatype1);
    iRet=H5Dclose(pFile->iCurrentD);
    pFile->iCurrentD=0;
    if (iRet < 0) {
        NXIReportError5 (NXpData5, "ERROR: HDF cannot close Dataset");
        return NX_ERROR;
     }
     return NX_OK;
  }

  /* --------------------------------------------------------------------- */

  NXstatus CALLING_STYLE NX5opendata (NXhandle fid, CONSTCHAR *name)
  {
    pNexusFile5 pFile;
    char pBuffer[256];
      
    pFile = NXI5assert (fid);
    /* clear pending attribute directories first */
    NXI5KillAttDir (pFile);
    /* find the ID number and open the dataset */
    pFile->iCurrentD = H5Dopen(pFile->iCurrentG, name);
    if (pFile->iCurrentD < 0) {
      sprintf (pBuffer, "ERROR: Dataset %s not found at this level", name);
      NXIReportError5 (NXpData5, pBuffer);
      return NX_ERROR;
    }
     /* find the ID number of datatype */
    pFile->iCurrentT = H5Dget_type(pFile->iCurrentD);
    if (pFile->iCurrentT < 0) {
      NXIReportError5 (NXpData5, "ERROR:HDF error opening Dataset");
      pFile->iCurrentT=0;
      return NX_ERROR;
    }
    /* find the ID number of dataspace */
    pFile->iCurrentS = H5Dget_space(pFile->iCurrentD);
    if (pFile->iCurrentS < 0) {
      NXIReportError5 (NXpData5, "ERROR:HDF error opening Dataset");
      pFile->iCurrentS=0;
      return NX_ERROR;
    }
    pFile->iCurrentLD = strdup(name); 
    return NX_OK;
  }
  
  /* ----------------------------------------------------------------- */

  NXstatus CALLING_STYLE NX5closedata (NXhandle fid)
  {
    pNexusFile5 pFile;
    int iRet;
  
    pFile = NXI5assert (fid);
    iRet = H5Sclose(pFile->iCurrentS);
    iRet = H5Tclose(pFile->iCurrentT);
    iRet = H5Dclose(pFile->iCurrentD);
    if (iRet < 0) {
        NXIReportError5 (NXpData5, "ERROR: HDF cannot end access to Dataset");
        return NX_ERROR;
     }
    pFile->iCurrentD=0;
    return NX_OK;
  }
  
  /* ------------------------------------------------------------------- */
  
  NXstatus CALLING_STYLE NX5putdata (NXhandle fid, void *data)
  {
    pNexusFile5 pFile;
    NXname pBuffer;
    hid_t iRet;
    
    char pError[512];
      
    pFile = NXI5assert (fid);
    
    /* actually write */
    iRet = H5Dwrite (pFile->iCurrentD, pFile->iCurrentT, H5S_ALL, H5S_ALL, 
                     H5P_DEFAULT, data);
    if (iRet < 0) {
      sprintf (pError, "ERROR: failure to write data to %s", pBuffer);
      NXIReportError5 (NXpData5, pError);
      return NX_ERROR;
    }
    return NX_OK;
  }
  
  /* ------------------------------------------------------------------- */

  NXstatus CALLING_STYLE NX5putattr (NXhandle fid, CONSTCHAR *name, void *data, 
                                  int datalen, int iType)
  {
    pNexusFile5 pFile;
    hid_t  attr1, aid1, aid2;
    hid_t type;
    int iRet;
  
    pFile = NXI5assert (fid);
    if (iType == NX_CHAR)
    {
        type=H5T_C_S1;
    }
    else if (iType == NX_INT8)
    {
        type=H5T_NATIVE_CHAR;
    }
    else if (iType == NX_UINT8)
    {
        type=H5T_NATIVE_UCHAR;
    }
    else if (iType == NX_INT16)
    {
        type=H5T_NATIVE_SHORT;
    }
    else if (iType == NX_UINT16)
    {
        type=H5T_NATIVE_USHORT;
    }
    else if (iType == NX_INT32)
    {
        type=H5T_NATIVE_INT;
    }
    else if (iType == NX_UINT32)
    {
        type=H5T_NATIVE_UINT;
    }
    else if (iType == NX_FLOAT32)
    {
        type=H5T_NATIVE_FLOAT;
    }
    else if (iType == NX_FLOAT64)
    {
        type=H5T_NATIVE_DOUBLE;
    }
    if (pFile->iCurrentD != 0) {
       /* Dataset attribute */
       aid2=H5Screate(H5S_SCALAR);
       aid1=H5Tcopy(type);
       if (iType == NX_CHAR)
         {
           H5Tset_size(aid1,datalen); 
         }         
       attr1 = H5Acreate(pFile->iCurrentD, name, aid1, aid2, H5P_DEFAULT);
       if (H5Awrite(attr1,aid1,data) < 0) 
       {
          NXIReportError5 (NXpData5, "ERROR: HDf failed to store attribute ");
          return NX_ERROR;
       }
       /* Close attribute dataspace */
       iRet=H5Tclose(aid1);
       iRet=H5Sclose(aid2); 
       /* Close attribute  */
       iRet=H5Aclose(attr1); 
    } else {
       /* global attribute */
       pFile->iVID=H5Gopen(pFile->iFID,"/");
       aid2=H5Screate(H5S_SCALAR);
       aid1=H5Tcopy(type);
       if (iType == NX_CHAR)
         {
           H5Tset_size(aid1,datalen); 
         }         
       attr1 = H5Acreate(pFile->iVID, name, aid1, aid2, H5P_DEFAULT);
       if (H5Awrite(attr1,aid1,data) < 0) 
       {
          NXIReportError5 (NXpData5, "ERROR: HDf failed to store attribute ");
          return NX_ERROR;
       } 
       /* Close attribute dataspace */
       iRet=H5Tclose(aid1);
       iRet=H5Sclose(aid2); 
        /* Close attribute */
       iRet=H5Aclose(attr1); 
       H5Gclose(pFile->iVID);
    }
    return NX_OK;
  }
  
  /* ------------------------------------------------------------------- */
 
  NXstatus CALLING_STYLE NX5putslab (NXhandle fid, void *data, int iStart[], int iSize[])
  {
    pNexusFile5 pFile;
    int iRet, i;
    int rank;
    hssize_t myStart[H5S_COMPLEX];
    hsize_t mySize[H5S_COMPLEX];
    hsize_t size[1],maxdims[2];
    hid_t   filespace,dataspace; 
  
    pFile = NXI5assert (fid);
     /* check if there is an Dataset open */
    if (pFile->iCurrentD == 0) {
      NXIReportError5 (NXpData5, "ERROR: no Dataset open");
      return NX_ERROR;
    }
    rank = H5Sget_simple_extent_ndims(pFile->iCurrentS);    
    for(i = 0; i < rank; i++)
    {
       myStart[i] = iStart[i];
       mySize[i]  = iSize[i];
    }
    iRet = H5Sget_simple_extent_dims(pFile->iCurrentS, NULL, maxdims);
    if (maxdims[0] == NX_UNLIMITED)
    {
       size[0]=iStart[0] + iSize[0];
       iRet = H5Dextend(pFile->iCurrentD, size);
       filespace = H5Dget_space(pFile->iCurrentD);
       
       /* define slab */
       iRet = H5Sselect_hyperslab(filespace, H5S_SELECT_SET, myStart,
                               NULL, mySize, NULL);
        /* deal with HDF errors */
       if (iRet < 0) 
       {
       NXIReportError5 (NXpData5, "ERROR: selecting slab failed");
       return NX_ERROR;
       }
       dataspace = H5Screate_simple (rank, mySize, NULL); 
       /* write slab */ 
       iRet = H5Dwrite(pFile->iCurrentD, pFile->iCurrentT, dataspace, 
                    filespace, H5P_DEFAULT,data);
       iRet = H5Sclose(dataspace); 
       iRet = H5Sclose(filespace);             
   } else {
       /* define slab */
       iRet = H5Sselect_hyperslab(pFile->iCurrentS, H5S_SELECT_SET, myStart,
                               NULL, mySize, NULL);
       /* deal with HDF errors */
       if (iRet < 0) 
       {
       NXIReportError5 (NXpData5, "ERROR: selecting slab failed");
       return NX_ERROR;
       }
       /* write slab */ 
       iRet = H5Dwrite(pFile->iCurrentD, pFile->iCurrentT, pFile->iCurrentS, 
                    pFile->iCurrentS, H5P_DEFAULT,data);
   }
   /* deal with HDF errors */
   if (iRet < 0) 
   {
      NXIReportError5 (NXpData5, "ERROR: writing slab failed");
      return NX_ERROR;
   }
    return NX_OK;
  }
 
  /* ------------------------------------------------------------------- */

  NXstatus CALLING_STYLE NX5getdataID (NXhandle fid, NXlink* sRes)
  {
    pNexusFile5 pFile;
  
    pFile = NXI5assert (fid);
    strcpy(sRes->iRef5,pFile->iCurrentLG);
    strcpy(sRes->iRefd,pFile->iCurrentLD);
    return NX_OK;
  }

  /* ------------------------------------------------------------------- */
 
  NXstatus CALLING_STYLE NX5makelink (NXhandle fid, NXlink* sLink)
  {
    pNexusFile5 pFile;
    int iRet;
    herr_t status;
    int size_type;
  
    pFile = NXI5assert (fid);
    if (pFile->iCurrentG == 0) { /* root level, can not link here */
      return NX_ERROR;
    }
    size_type = strlen(sLink->iRefd); 
    if (size_type>0)
    {
       /* dataset link */
       strcpy(sLink->iTag5,pFile->iCurrentLG);
    } else {
     /* group link */
       strcpy(sLink->iTag5,pFile->iCurrentLGG);
    }
    iRet=H5Gclose(pFile->iCurrentG);
    if (size_type>0)
    {
    strcat(sLink->iRef5,"/");
    strcat(sLink->iRef5,sLink->iRefd);
    }
    if (size_type>0)
    {
    strcat(sLink->iTag5,"/");
    strcat(sLink->iTag5,sLink->iRefd);
    } else {
    H5Gmove(pFile->iFID,sLink->iTag5,"00");
    H5Gunlink(pFile->iFID,"00");
    }
    status=H5Glink(pFile->iFID, H5G_LINK_HARD, sLink->iRef5, sLink->iTag5);
    pFile->iCurrentG = H5Gopen(pFile->iFID, pFile->iCurrentLG);
    return NX_OK;
   }
  
  /*----------------------------------------------------------------------*/

  NXstatus CALLING_STYLE NX5flush(NXhandle *pHandle)
  {
    pNexusFile5 pFile = NULL;
    int iRet;
   
    pFile = NXI5assert (*pHandle);    
    if (pFile->iCurrentD != 0)
    {    
    iRet=H5Fflush(pFile->iCurrentD,H5F_SCOPE_LOCAL);
    }
    else if (pFile->iCurrentG != 0)
    {    
    iRet=H5Fflush(pFile->iCurrentG,H5F_SCOPE_LOCAL);
    }
    else
    { 
    iRet=H5Fflush(pFile->iFID,H5F_SCOPE_LOCAL);
    }
    if (iRet < 0){
      NXIReportError5 (NXpData5, "ERROR: The object cannot be flushed");
      return NX_ERROR; 
    }
    return NX_OK;
  }   
  
  /*-------------------------------------------------------------------------*/
  
  /* Operator function. */
           
  herr_t nxgroup_info(hid_t loc_id, const char *name, void *op_data)
  {
    H5G_stat_t statbuf;
    pinfo self;
    
    self = (pinfo)op_data;
    H5Gget_objinfo(loc_id, name, 0, &statbuf);
    switch (statbuf.type) 
    {
      case H5G_GROUP: 
         self->iname = strdup(name);
         self->type = H5G_GROUP;
         break;
      case H5G_DATASET: 
         self->iname = strdup(name);
         self->type = H5G_DATASET;
         break;
      default:
         self->type=0;
         break;     
    }
    return 1; 
  }

  /*-------------------------------------------------------------------------*/

  NXstatus CALLING_STYLE NX5getnextentry (NXhandle fid,NXname name, NXname nxclass, int *datatype)
  {
    pNexusFile5 pFile;
    hid_t grp, attr1,type,atype;
    int iRet,iPtype;
    int idx,data_id,size_id, sign_id;
    char data[20];
    char *ph_name;
    info_type op_data;
     
    pFile = NXI5assert (fid);
    idx=pFile->iCurrentIDX;
    iRet=H5Giterate(pFile->iFID,pFile->name_ref,&idx,nxgroup_info,&op_data);
    strcpy(nxclass,"");
    if (iRet>0)
      {
        pFile->iCurrentIDX++;
        strcpy(name,op_data.iname);
        if (op_data.type==H5G_GROUP)
        {
           ph_name=pFile->iStack5[pFile->iStackPtr].irefn;
           strcat(ph_name,"/");
           strcat(ph_name,name);
           grp=H5Gopen(pFile->iFID,ph_name);
           attr1 = H5Aopen_name(grp, "NX_class");
           type=H5T_C_S1;
           atype=H5Tcopy(type);
           H5Tset_size(atype,20);  
           iRet = H5Aread(attr1, atype, data);
           strcpy(nxclass,data);
           H5Tclose(atype);
           H5Gclose(grp);
        } else if (op_data.type==H5G_DATASET)
        {
           grp=H5Dopen(pFile->iCurrentG,name);
           type=H5Dget_type(grp);
           atype=H5Tcopy(type);
           data_id = H5Tget_class(atype);
           if (data_id==H5T_STRING)
           {
             iPtype=NX_CHAR;
           }
           if (data_id==H5T_INTEGER)
          {
             size_id=H5Tget_size(atype);
             sign_id=H5Tget_sign(atype);
             if (size_id==1)
             {
                if (sign_id==H5T_SGN_2)
                {
                   iPtype=NX_INT8;
                } else {
                   iPtype=NX_UINT8;
                }
             } 
             else if (size_id==2) 
             {
                if (sign_id==H5T_SGN_2)
                {
                   iPtype=NX_INT16;
                } else {
                   iPtype=NX_UINT16;
                }
             }
             else if (size_id==4) 
             {
                 if (sign_id==H5T_SGN_2)
                 {
                    iPtype=NX_INT32;
                 } else {
                    iPtype=NX_UINT32;
                 }
             }
        } else if (data_id==H5T_FLOAT)     
            {
            size_id=H5Tget_size(atype);
            if (size_id==4)
            {
               iPtype=NX_FLOAT32;
            } 
            else if (size_id==8) 
            {
               iPtype=NX_FLOAT64;
            }
        }
           *datatype=iPtype;
           H5Tclose(atype);
           H5Dclose(grp);
           free(op_data.iname);
        }
        return NX_OK;
      }
      else if (iRet==0)
      {
        return NX_EOD;
      }
      else
      { 
        NXIReportError5 (NXpData5, 
                           "ERROR: Iteration was not successful");
        return NX_ERROR;              
      }
  }

  /*-------------------------------------------------------------------------*/

  NXstatus CALLING_STYLE NX5getdata (NXhandle fid, void *data)
  {
    pNexusFile5 pFile;
    int iStart[H5S_MAX_RANK];
    
    pFile = NXI5assert (fid);
    /* check if there is an Dataset open */
    if (pFile->iCurrentD == 0) 
      {
        NXIReportError5 (NXpData5, "ERROR: no Dataset open");
        return NX_ERROR;
      }
    memset (iStart, 0, H5S_MAX_RANK * sizeof(int));
    /* actually read */
    H5Dread (pFile->iCurrentD, pFile->iCurrentT, H5S_ALL, H5S_ALL,H5P_DEFAULT, data);
    return NX_OK;
  }
    
  /*-------------------------------------------------------------------------*/
 
  NXstatus CALLING_STYLE NX5getinfo (NXhandle fid, int *rank, int dimension[], int *iType)
  {
    pNexusFile5 pFile;
    int i, iRank, mType, iRet;
    hsize_t myDim[H5S_COMPLEX]; 
    hid_t data_id,size_id,sign_id;
    
    pFile = NXI5assert (fid);
    /* check if there is an Dataset open */
    if (pFile->iCurrentD == 0) {
      NXIReportError5 (NXpData5, "ERROR: no Dataset open");
      return NX_ERROR;
    }
    
    /* read information */
    data_id = H5Tget_class(pFile->iCurrentT);
    if (data_id==H5T_STRING)
    {
      mType=NX_CHAR;
    }
    if (data_id==H5T_INTEGER)
    {
      size_id=H5Tget_size(pFile->iCurrentT);
      sign_id=H5Tget_sign(pFile->iCurrentT);
      if (size_id==1)
      {
         if (sign_id==H5T_SGN_2)
         {
          mType=NX_INT8;
         } else {
          mType=NX_UINT8;
         }
      } 
      else if (size_id==2) 
      {
         if (sign_id==H5T_SGN_2)
         {
          mType=NX_INT16;
         } else {
          mType=NX_UINT16;
         }
      }
      else if (size_id==4) 
      {
      if (sign_id==H5T_SGN_2)
         {
          mType=NX_INT32;
         } else {
          mType=NX_UINT32;
         }
      }
    } else if (data_id==H5T_FLOAT)     
      {
      size_id=H5Tget_size(pFile->iCurrentT);
      if (size_id==4)
      {
      mType=NX_FLOAT32;
      } 
      else if (size_id==8) 
      {
      mType=NX_FLOAT64;
      }
    } 
    iRank = H5Sget_simple_extent_ndims(pFile->iCurrentS);
    iRet = H5Sget_simple_extent_dims(pFile->iCurrentS, myDim, NULL);   
    /* conversion to proper ints for the platform */ 
    *iType = (int)mType;
    *rank = (int)iRank;
    for (i = 0; i < iRank; i++)
    {
       dimension[i] = (int)myDim[i];
    }
    return NX_OK;
  }
  
  /*-------------------------------------------------------------------------*/
 
  NXstatus CALLING_STYLE NX5getslab (NXhandle fid, void *data, int iStart[], int iSize[])
  {
    pNexusFile5 pFile;
    hssize_t myStart[H5S_COMPLEX];
    hsize_t mySize[H5S_COMPLEX];
    hssize_t mStart[H5S_COMPLEX];
    hid_t   memspace, iRet;
    int i, iRank;
      
    pFile = NXI5assert (fid);
    /* check if there is an Dataset open */
    if (pFile->iCurrentD == 0) 
      {
        NXIReportError5 (NXpData5, "ERROR: no Dataset open");
        return NX_ERROR;
      }
    iRank = H5Sget_simple_extent_ndims(pFile->iCurrentS); 
    for (i = 0; i < iRank; i++)
       {
         myStart[i] = iStart[i];
         mySize[i]  = iSize[i];
       }
             
     /* define slab */
     iRet = H5Sselect_hyperslab(pFile->iCurrentS, H5S_SELECT_SET, myStart,
                               NULL, mySize, NULL);
     /* deal with HDF errors */
     if (iRet < 0) 
       {
         NXIReportError5 (NXpData5, "ERROR: selecting slab failed");
         return NX_ERROR;
       }
     
     memspace=H5Screate_simple(iRank,mySize,NULL);
     mStart[0]=0;
     mStart[1]=0;
     iRet = H5Sselect_hyperslab(memspace, H5S_SELECT_SET, mStart,
                               NULL, mySize, NULL);
     if (iRet < 0) 
       {
         NXIReportError5 (NXpData5, "ERROR: Select memspace failed");
         return NX_ERROR;
       }
       
     /* read slab */ 
     iRet = H5Dread(pFile->iCurrentD, pFile->iCurrentT, memspace, 
                    pFile->iCurrentS, H5P_DEFAULT,data);
    
     if (iRet < 0) 
       {
         NXIReportError5 (NXpData5, "ERROR: Reading slab failed");
         return NX_ERROR;
       }
     return NX_OK;
  }
  
  /*-------------------------------------------------------------------------*/
 
  /* Operator function. */
 
  herr_t attr_info(hid_t loc_id, const char *name, void *opdata)
  {
    *((char**)opdata)=strdup(name);
    return 1;
  }
  
  NXstatus CALLING_STYLE NX5getnextattr (NXhandle fileid, NXname pName,
                                      int *iLength, int *iType)
  {
    pNexusFile5 pFile;
    hid_t attr_id,size_id,sign_id;
    hid_t iRet, atype, aspace;
    int iPType,rank;
    char *iname; 
    unsigned int idx;
      
    pFile = NXI5assert (fileid);
    idx=pFile->iCurrentA_IDX;
    if ((pFile->iCurrentD == 0) && (pFile->iCurrentG==0)) 
    {
    /* global attribute */
       pFile->iVID=H5Gopen(pFile->iFID,"/");
       iRet=H5Aiterate(pFile->iVID,&idx,attr_info,&iname);
    } else { 
    iRet=H5Aiterate(pFile->iCurrentD,&idx,attr_info,&iname);
    }
    if (iRet>0)
      {
        pFile->iCurrentA_IDX++;
        strcpy(pName, iname);
        if ((pFile->iCurrentD == 0) && (pFile->iCurrentG==0))
        {
        /* global attribute */
        pFile->iCurrentA = H5Aopen_name(pFile->iVID, pName);
        } else { 
        pFile->iCurrentA = H5Aopen_name(pFile->iCurrentD, pName);
        }
        atype  = H5Aget_type(pFile->iCurrentA);
        aspace = H5Aget_space(pFile->iCurrentA);
        rank = H5Sget_simple_extent_ndims(aspace);
          
        attr_id = H5Tget_class(atype);
        if (attr_id==H5T_STRING)
        {
           iPType=NX_CHAR;
        }
        if (attr_id==H5T_INTEGER)
           {
           size_id=H5Tget_size(atype);
           sign_id=H5Tget_sign(atype);
           if (size_id==1)
           {
              if (sign_id==H5T_SGN_2)
              {
              iPType=NX_INT8;
              } else {
              iPType=NX_UINT8;
              }
           } 
           else if (size_id==2) 
           {
              if (sign_id==H5T_SGN_2)
              {
              iPType=NX_INT16;
              } else {
              iPType=NX_UINT16;
              }
           }
           else if (size_id==4) 
           {
              if (sign_id==H5T_SGN_2)
              {
              iPType=NX_INT32;
              } else {
              iPType=NX_UINT32;
              }
           }
        } else if (attr_id==H5T_FLOAT)     
          {
          size_id=H5Tget_size(atype);
          if (size_id==4)
          {
          iPType=NX_FLOAT32;
          } 
          else if (size_id==8) 
          {
          iPType=NX_FLOAT64;
          }
        } 
        *iType=iPType;
        *iLength=rank;
        H5Tclose(atype);
        H5Sclose(aspace);
        H5Aclose(pFile->iCurrentA);
        if ((pFile->iCurrentD == 0) && (pFile->iCurrentG==0))
        {
        /* close group for global attribute */
        H5Gclose(pFile->iVID);
        } 
        return NX_OK;
      }
      else if (iRet==0)
      {
        if ((iname==NULL) && (idx==0))
        {
        NXIReportError5 (NXpData5, "Dataset has not attributes!");
        return NX_EOD;
        }
        
        return NX_EOD;
      }
      else
      { 
        NXIReportError5 (NXpData5, 
                           "ERROR: Iteration was not successful");
        return NX_ERROR;              
      }
  }
  
  

  /*-------------------------------------------------------------------------*/

  NXstatus CALLING_STYLE NX5getattr (NXhandle fid, char *name, void *data, int* datalen, int* iType)
  {
    pNexusFile5 pFile;
    int iNew, iRet;
    hid_t type, atype, glob;
    char pBuffer[256];
  
    pFile = NXI5assert (fid);
    type = *iType;
    glob = 0;  
    if (type == NX_CHAR)
    {
        type=H5T_C_S1;
    }
    else if (type == NX_INT8)
    {
        type=H5T_NATIVE_CHAR;
    }
    else if (type == NX_UINT8)
    {
        type=H5T_NATIVE_UCHAR;
    }
    else if (type == NX_INT16)
    {
        type=H5T_NATIVE_SHORT;
    }
    else if (type == NX_UINT16)
    {
        type=H5T_NATIVE_USHORT;
    }
    else if (type == NX_INT32)
    {
        type=H5T_NATIVE_INT;
    }
    else if (type == NX_UINT32)
    {
        type=H5T_NATIVE_UINT;
    }
    else if (type == NX_FLOAT32)
    {
        type=H5T_NATIVE_FLOAT;
    }
    else if (type == NX_FLOAT64)
    {
        type=H5T_NATIVE_DOUBLE;
    }
    /* find attribute */
    if (pFile->iCurrentD != 0) 
      {
      /* Dataset attribute */
      iNew = H5Aopen_name(pFile->iCurrentD, name);
      } 
      else 
      {
        /* globale and group attributes */
        if (pFile->iCurrentG != 0) {
         /* group attribute */
         iNew = H5Aopen_name(pFile->iCurrentG, name);
      } else {
         /* global attributes */
         glob=H5Gopen(pFile->iFID,"/");
         iNew = H5Aopen_name(glob, name);
      }
    }
    if (iNew < 0) {
      sprintf (pBuffer, "ERROR: attribute %s not found", name);
      NXIReportError5 (NXpData5, pBuffer);
      return NX_ERROR;
    }
    pFile->iCurrentA = iNew;
    /* finally read the data */
    if (type==H5T_C_S1)
    {
       atype=H5Tcopy(type);
       H5Tset_size(atype,*datalen);  
       iRet = H5Aread(pFile->iCurrentA, atype, data);
       *datalen=strlen(data);
    } else {
      iRet = H5Aread(pFile->iCurrentA, type, data);
      *datalen=1;
    }
    
    if (iRet < 0) {
      sprintf (pBuffer, "ERROR: HDF could not read attribute data");
      NXIReportError5 (NXpData5, pBuffer);
      return NX_ERROR;
    }
    
    iRet = H5Aclose(pFile->iCurrentA);
    if (glob > 0)
    {
    H5Gclose(glob);
    }
    if (type==H5T_C_S1)
    {
      H5Tclose(atype);
    }
    return NX_OK;
  }  

  /*-------------------------------------------------------------------------*/

  NXstatus CALLING_STYLE NX5getattrinfo (NXhandle fid, int *iN)
  {
    pNexusFile5 pFile;
    char *iname; 
    unsigned int idx;
    int iRet;
     
    pFile = NXI5assert (fid);
    if (pFile->iCurrentD == 0) {
      NXIReportError5 (NXpData5, "Attribut number can't fixed! No Dataset open!");
      return NX_ERROR;
    }
    idx=0;
    iRet=H5Aiterate(pFile->iCurrentD,&idx,attr_info,&iname);
    if (iRet<0) {
      NXIReportError5 (NXpData5, "Attribut number cannot be fixed!");
      return NX_ERROR;
    }
    if ((idx==0) && (iRet==0)) {
       *iN=idx;
       return NX_OK;
    }
    do
    {  
      iRet=H5Aiterate(pFile->iCurrentD,&idx,attr_info,&iname);
      idx=idx+1;
    } while (iRet==0);
    if (idx>0) {
      *iN=idx;
    }
    else {
      *iN=1;
    }  
    return NX_OK;
  }

  /*-------------------------------------------------------------------------*/

  NXstatus CALLING_STYLE NX5getgroupID (NXhandle fileid, NXlink* sRes)
  {
    pNexusFile5 pFile;
  
    pFile = NXI5assert (fileid);
    if (pFile->iCurrentG == 0) {
      return NX_ERROR;
    } 
    else {
      strcpy(sRes->iRef5,pFile->iCurrentLGG);
      strcpy(sRes->iRefd,"");
      return NX_OK;
    }
    /* not reached */
    return NX_ERROR;
  }  
  
  /* --------------------------------------------------------------------- */

  /* Operator function. */

  herr_t group_info1(hid_t loc_id, const char *name, void *opdata)
  {
    NexusFile5 self;
    H5G_stat_t statbuf;
    self.iNX = *((int*)opdata);
    H5Gget_objinfo(loc_id, name, 0, &statbuf);
    
    switch (statbuf.type) 
    {
      case H5G_GROUP: 
        self.iNX++;
        *((int*)opdata)=self.iNX;
        break;
      case H5G_DATASET:
        self.iNX++;
        *((int*)opdata)=self.iNX;
        break;
    }
    return 0; 
  }
  
  /*-------------------------------------------------------------------------*/

  NXstatus CALLING_STYLE NX5getgroupinfo (NXhandle fid, int *iN, NXname pName, NXname pClass)
  {
    pNexusFile5 pFile;
    hid_t atype,attr_id;
    char data[64];
    int iRet;
        
    pFile = NXI5assert (fid);
    /* check if there is a group open */
    if (pFile->iCurrentG == 0) {
       strcpy (pName, "root");
       strcpy (pClass, "NXroot");
       pFile->iNX=0;
       iRet=H5Giterate(pFile->iFID,"/",0,group_info1,&pFile->iNX);
       *iN=pFile->iNX;
    }
    else {
      strcpy (pName,pFile->iCurrentLG);
      attr_id = H5Aopen_name(pFile->iCurrentG,"NX_class");
      if (attr_id<0) {
         strcpy(pClass,"non");
      } 
      else {
        atype=H5Tcopy(H5T_C_S1);
        H5Tset_size(atype,64);  
        H5Aread(attr_id, atype, data);
        strcpy(pClass,data);
        pFile->iNX=0;
        iRet=H5Giterate(pFile->iFID,pFile->iCurrentLG,0,group_info1, &pFile->iNX);
        *iN=pFile->iNX;
        H5Aclose(attr_id);
      }
    }
    return NX_OK;
  }
 
  /*-------------------------------------------------------------------------*/
 
  NXstatus CALLING_STYLE NX5initattrdir (NXhandle fid)
  {
    pNexusFile5 pFile;
        
    pFile = NXI5assert (fid);
    NXI5KillAttDir (fid);
    pFile->iCurrentA_IDX=0;
    return NX_OK;
  }
 
  /*-------------------------------------------------------------------------*/
 
  NXstatus CALLING_STYLE NX5initgroupdir (NXhandle fid)
  {
    pNexusFile5 pFile;
        
    pFile = NXI5assert (fid);
    NXI5KillDir (fid);
    pFile->iCurrentIDX=0;
    return NX_OK;
  }