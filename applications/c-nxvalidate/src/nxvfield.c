/**
  * This file contains all the code for validating fields
	*
	* copyright: GPL
	*
	* Mark Koennecke, mark.koennecke@psi.ch, and NIAC
	*/
#include "nxvcontext.h"
#include <hdf5.h>
#include <hdf5_hl.h>
#include <string.h>
#include <assert.h>
#include <libxml/tree.h>

static void validateData(pNXVcontext self, hid_t fieldID,
	xmlNodePtr enumNode)
{
	/* TODO */
}
/*-------------------------------------------------------------*/
static int isInteger(char *txt)
{
	int i;

	for(i = 0; i < strlen(txt); i++){
		if(txt[i] != '-' && txt[i] != '+' && !isdigit(txt[i])){
			return 0;
		}
	}
	return 1;
}
/*-------------------------------------------------------------*/
static void validateDimensions(pNXVcontext self, hid_t fieldID,
	xmlNodePtr dimNode)
{
	hid_t dSpace;
	int h5rank, nxdlRank, idx, dimVal, *dimInt;
	hsize_t h5dim[H5S_MAX_RANK];
	xmlChar *data, *val, *index;
	xmlNodePtr dim;
	/*
		get info from HDF5 file
	*/
	dSpace = H5Dget_space(fieldID);
	assert(dSpace >= 0);
	h5rank = H5Sget_simple_extent_ndims(dSpace);
	H5Sget_simple_extent_dims(dSpace,h5dim,NULL);
	H5Sclose(dSpace);

	/*
		now compare with NXDL
	*/
	data = xmlGetProp(dimNode,(xmlChar *)"rank");
	if(data != NULL){
		nxdlRank = atoi((char *)data);
		if(nxdlRank != h5rank){
			NXVsetLog(self,"sev","error");
			NXVprintLog(self,"message","Rank mismatch expected %d, found %d",
				nxdlRank, h5rank);
			NXVlog(self);
			self->errCount++;
			return; /* further dimension checking makes no sense here */
		}
	} else {
		NXVsetLog(self,"sev","error");
		NXVsetLog(self,"message","Invalid NXDL entry, missing rank on dimensions field");
		NXVlog(self);
		self->errCount++;
		return;
	}

	/*
		check the individual dimensions
	*/
	dim = dimNode->xmlChildrenNode;
	while(dim != NULL){
		if(xmlStrcmp(dim->name,(xmlChar *)"dim") == 0){
			index = xmlGetProp(dim,(xmlChar *)"index");
			val = xmlGetProp(dim,(xmlChar *)"value");
			if(index == NULL || val == NULL){
				NXVsetLog(self,"sev","error");
				NXVsetLog(self,"message",
				"Invalid NXDL: dim entry is missing index or value attribute");
				NXVlog(self);
				self->errCount++;
				continue;
			}
			idx = atoi((char *)index);
			idx--; /* start counting at 1, bizarre.... */
			if(isInteger((char *)val)){
				dimVal = atoi((char *)val);
				if(dimVal != h5dim[idx]){
					NXVsetLog(self,"sev","error");
					NXVprintLog(self,"message",
						"Dimension mismatch on %d, should %d, is %d",
						idx+1, dimVal, h5dim[idx]);
					NXVlog(self);
					self->errCount++;
				}
			} else {
				if((dimInt = (int *)hash_lookup((char *)val,&self->dimSymbols)) == NULL){
					dimInt = malloc(sizeof(int));
					*dimInt = h5dim[idx];
					hash_insert((char *)val,dimInt,&self->dimSymbols);
				} else {
					if(*dimInt != h5dim[idx]){
						NXVsetLog(self,"sev","error");
						NXVprintLog(self,"message",
							"Dimension mismatch on %d, should %d, is %d",
							idx+1, *dimInt, h5dim[idx]);
						NXVlog(self);
						self->errCount++;
					}
				}
			}
		}
		dim = dim->next;
	}
}
/*--------------------------------------------------------------*/
static void validateUnits(pNXVcontext self, hid_t fieldID,
	xmlChar *type)
{
	if(!H5LTfind_attribute(fieldID,"units")){
			NXVsetLog(self,"sev","error");
			NXVsetLog(self,"message","Required units attribute missing");
			NXVlog(self);
			self->errCount++;
	}
	/*
	  more cannot be done: the NXDL units are symbols and as NeXus allows
		all UDunits unit strings, it is an impossible task to verify this.
	*/
}
/*--------------------------------------------------------------*/
static void validateType(pNXVcontext self, hid_t fieldID,
	xmlChar *type)
{
	hid_t h5type;
	H5T_class_t h5class;
	char h5dataType[132], dataName[512], data[512];
	size_t len;

	h5type = H5Dget_type(fieldID);
	h5class = H5Tget_class(h5type);

	if(xmlStrcmp(type,(xmlChar *)"NX_FLOAT") == 0 && h5class != H5T_FLOAT){
		NXVsetLog(self,"sev","error");
		len = sizeof(h5dataType);
		memset(h5dataType,0,len*sizeof(char));
		H5LTdtype_to_text(h5type,h5dataType,H5LT_DDL,&len);
		NXVprintLog(self,"message","Data type mismatch, expected %s, got %s",
			(char* )type, h5dataType);
		NXVlog(self);
		self->errCount++;
	}

	if(xmlStrcmp(type,(xmlChar *)"NX_POSINT") == 0 && h5class != H5T_INTEGER){
		NXVsetLog(self,"sev","error");
		len = sizeof(h5dataType);
		memset(h5dataType,0,len*sizeof(char));
		H5LTdtype_to_text(h5type,h5dataType,H5LT_DDL,&len);
		NXVprintLog(self,"message","Data type mismatch, expected %s, got %s",
			(char* )type, h5dataType);
		NXVlog(self);
		self->errCount++;
		/*
			TODO: there could be more testing here. But this would involve reading
			all the data and testing it for positivity. It is to be dicussed if we
			retract POSINT and replace it with UINT
		*/
	}

	if(xmlStrcmp(type,(xmlChar *)"NX_INT") == 0 && h5class != H5T_INTEGER){
		NXVsetLog(self,"sev","error");
		len = sizeof(h5dataType);
		memset(h5dataType,0,len*sizeof(char));
		H5LTdtype_to_text(h5type,h5dataType,H5LT_DDL,&len);
		NXVprintLog(self,"message","Data type mismatch, expected %s, got %s",
			(char* )type, h5dataType);
		NXVlog(self);
		self->errCount++;
	}

	if(xmlStrcmp(type,(xmlChar *)"NX_BOOLEAN") == 0 && h5class != H5T_INTEGER){
		NXVsetLog(self,"sev","error");
		len = sizeof(h5dataType);
		memset(h5dataType,0,len*sizeof(char));
		H5LTdtype_to_text(h5type,h5dataType,H5LT_DDL,&len);
		NXVprintLog(self,"message","Data type mismatch, expected %s, got %s",
			(char* )type, h5dataType);
		NXVlog(self);
		self->errCount++;
		/*
			TODO: more testing could be done here: read the data and test if it
			is 0 or 1
		*/
	}

	if(xmlStrcmp(type,(xmlChar *)"NX_UINT") == 0
		&& h5class != H5T_INTEGER && H5Tget_sign(h5type) != H5T_SGN_NONE){
		NXVsetLog(self,"sev","error");
		len = sizeof(h5dataType);
		memset(h5dataType,0,len*sizeof(char));
		H5LTdtype_to_text(h5type,h5dataType,H5LT_DDL,&len);
		NXVprintLog(self,"message","Data type mismatch, expected %s, got %s",
			(char* )type, h5dataType);
		NXVlog(self);
		self->errCount++;
	}

	if(xmlStrcmp(type,(xmlChar *)"NX_CHAR") == 0 && h5class != H5T_STRING){
		NXVsetLog(self,"sev","error");
		len = sizeof(h5dataType);
		memset(h5dataType,0,len*sizeof(char));
		H5LTdtype_to_text(h5type,h5dataType,H5LT_DDL,&len);
		NXVprintLog(self,"message","Data type mismatch, expected %s, got %s",
			(char* )type, h5dataType);
		NXVlog(self);
		self->errCount++;
	}

	if(xmlStrcmp(type,(xmlChar *)"NX_NUMBER") == 0 &&
		h5class != H5T_INTEGER && h5class != H5T_FLOAT){
		NXVsetLog(self,"sev","error");
		len = sizeof(h5dataType);
		memset(h5dataType,0,len*sizeof(char));
		H5LTdtype_to_text(h5type,h5dataType,H5LT_DDL,&len);
		NXVprintLog(self,"message","Data type mismatch, expected %s, got %s",
			(char* )type, h5dataType);
		NXVlog(self);
		self->errCount++;
	}

	if(xmlStrcmp(type,(xmlChar *)"NX_DATE_TIME") == 0){
		if(h5class != H5T_STRING){
			NXVsetLog(self,"sev","error");
			len = sizeof(h5dataType);
			memset(h5dataType,0,len*sizeof(char));
			H5LTdtype_to_text(h5type,h5dataType,H5LT_DDL,&len);
			NXVprintLog(self,"message","Data type mismatch, expected %s, got %s",
				(char* )type, h5dataType);
				NXVlog(self);
				self->errCount++;
			} else {
				memset(dataName,0,sizeof(dataName));
				H5Iget_name(fieldID,dataName,sizeof(dataName));
				memset(data,0,sizeof(data));
				H5LTread_dataset_string(self->fileID,dataName,data);
				if(!testISO8601(data)){
					NXVsetLog(self,"sev","error");
					NXVsetLog(self,"message","Date/Time not in ISO8601 format");
					NXVlog(self);
					self->errCount++;
				}
			}
	}

	if(xmlStrcmp(type,(xmlChar *)"NX_BINARY") == 0
		&& h5class != H5T_INTEGER && H5Tget_size(h5type) != 1){
		NXVsetLog(self,"sev","error");
		len = sizeof(h5dataType);
		memset(h5dataType,0,len*sizeof(char));
		H5LTdtype_to_text(h5type,h5dataType,H5LT_DDL,&len);
		NXVprintLog(self,"message","Data type mismatch, expected %s, got %s",
			(char* )type, h5dataType);
		NXVlog(self);
		self->errCount++;
	}

	H5Tclose(h5type);

}
/*--------------------------------------------------------------
 For attribute validation we use a little dictionary which maps
 attribute names to attribute data validation functions. We use
 this to process both direct field element XML attributes and
 attributes given as children of the field node. The dictionary
 is initialized below, after the validation functions.
 --------------------------------------------------------------*/
typedef int (*AttributeDataValidator)(pNXVcontext self,
	hid_t fieldID, char *testValue);

typedef struct {
	char *name;
	AttributeDataValidator dataValidator;
}AttributeValidationData;

/*--------------------------------------------------------------*/
static int SignalValidator(pNXVcontext self, hid_t fieldID,
	char *testValue)
{
	return 1;
}
/*----------------------------------------------------------------*/
static AttributeValidationData attValData[] = {
	{"signal",SignalValidator},
	{"axis",SignalValidator},
	NULL};
/*--------------------------------------------------------------*/
static int findAttValidator(char *name){
	int i;
	for(i=0;attValData[i].name != NULL;i++){
		if(strcmp(attValData[i].name,name) == 0){
			return i;
		}
	}
	return -1;
}
/*--------------------------------------------------------------*/
static void validateAttributes(pNXVcontext self, hid_t fieldID,
	xmlNodePtr fieldNode)
{
	xmlChar *data = NULL, *name = NULL;
	xmlNodePtr cur, attData, item;
	int status, attOK = 0, i;

	/*
		Type and units attributes are special...
	*/
	data = xmlGetProp(fieldNode,(xmlChar *)"type");
	if(data != NULL){
		validateType(self,fieldID,data);
	}

	data = xmlGetProp(fieldNode,(xmlChar *)"units");
	if(data != NULL){
		validateUnits(self,fieldID,data);
	}

	/*
		test and process other attributes...
	*/
	for(i = 0; attValData[i].name != NULL; i++){
			data = xmlGetProp(fieldNode,(xmlChar *) attValData[i].name);
			if(data != NULL){
					if(!H5LTfind_attribute(fieldID, attValData[i].name)){
						NXVsetLog(self,"sev","error");
						NXVprintLog(self,"message","Required attribute %s missing",
							attValData[i].name);
						NXVlog(self);
						self->errCount++;
					} else {
						status = attValData[i].dataValidator(self,fieldID, data);
						if(status != 1){
							NXVsetLog(self,"sev","error");
							NXVprintLog(self,"message",
								"Invalid value for attribute %s, should be %s",
								attValData[i].name,(char *)data);
							NXVlog(self);
							self->errCount++;
						}
					}
			}
	}

	/*
		validate attributes in attribute elements
	*/
	cur = fieldNode->xmlChildrenNode;
	while(cur != NULL){
		if(xmlStrcmp(cur->name,(xmlChar *)"attribute") == 0){
			name = xmlGetProp(cur,(xmlChar *)"name");
			if(!H5LTfind_attribute(fieldID,(char*)name)){
				NXVsetLog(self,"sev","error");
				NXVprintLog(self,"message","Required attribute %s missing",
					(char *)name);
				NXVlog(self);
				self->errCount++;
			} else {
				/*
					find enumeration node
				*/
				attData = cur->xmlChildrenNode;
				while(attData != NULL){
					if(xmlStrcmp(attData->name,(xmlChar *)"enumeration") == 0){
							/*
								find the item nodes
							*/
							attOK = 0;
							item = attData->xmlChildrenNode;
							i = findAttValidator((char *)name);
							while(item != NULL){
								data = xmlGetProp(item,(xmlChar *)"value");
									if(data != NULL && i >= 0){
										status = attValData[i].dataValidator(self,fieldID,data);
										if(status == 1){
											attOK = 1;
											break;
										}
									}
									item = item->next;
							}
							if(attOK == 0){
									NXVsetLog(self,"sev","error");
									NXVprintLog(self,"message",
										"Invalid value for attribute %s",
										attValData[i].name);
										NXVlog(self);
										self->errCount++;
									}
						}
					attData = attData->next;
				}
			}
		}
		cur = cur->next;
	}
}
/*--------------------------------------------------------------*/
int NXVvalidateField(pNXVcontext self, hid_t fieldID,
  	xmlNodePtr fieldNode)
{
  xmlNodePtr cur;
  char fName[256], nxdlName[512];
  xmlChar *name;
	char *myPath;

	name = xmlGetProp(fieldNode,(xmlChar *)"name");
	snprintf(nxdlName,sizeof(nxdlName),"%s/%s",
		self->nxdlPath,name);
	H5Iget_name(fieldID,fName,sizeof(fName));
	NXVsetLog(self,"sev","debug");
	NXVsetLog(self,"message","Validating field");
	NXVsetLog(self,"dataPath",fName);
	NXVsetLog(self,"nxdlPath",nxdlName);
	NXVlog(self);

	myPath = self->nxdlPath;
	self->nxdlPath = nxdlName;

  validateAttributes(self,fieldID,fieldNode);

	cur = fieldNode->xmlChildrenNode;
	while(cur != NULL){
		if(xmlStrcmp(cur->name,(xmlChar *)"enumeration") == 0){
			validateData(self,fieldID,cur);
		}
		if(xmlStrcmp(cur->name,(xmlChar *)"dimensions") == 0){
			validateDimensions(self,fieldID,cur);
		}
		cur = cur->next;
	}

	self->nxdlPath = myPath;
	return 0;
}
