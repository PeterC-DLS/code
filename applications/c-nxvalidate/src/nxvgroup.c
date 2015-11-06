/**
  * This file contains all the code for validating groups
	*
	* copyright: GPL
	*
	* Mark Koennecke, mark.koennecke@psi.ch, and NIAC
	*/
#include <assert.h>
#include <nxvalidate.h>
#include "nxvcontext.h"
#include <string.h>
#include <hdf5.h>
#include <hdf5_hl.h>
#include <libxml/tree.h>

static void validateGroupAttributes(pNXVcontext self,
	hid_t groupID, xmlNodePtr groupNode)
{
	xmlNodePtr cur;
	xmlChar *type, *name;
	herr_t attID;
	char data[512], gname[512];

	memset(data,0,sizeof(data));
	memset(gname,0,sizeof(gname));
	H5Iget_name(groupID,gname,sizeof(gname));

	type = xmlGetProp(groupNode,(xmlChar *)"type");
	attID = H5LTget_attribute_string(groupID,gname,"NX_class",data);
	if(attID < 0){
		NXVsetLog(self,"sev","error");
		NXVsetLog(self,"message","Required group attribute NX_class missing");
		NXVlog(self);
		self->errCount++;
	} else {
		if(strcmp(data,(char *)type) != 0){
			NXVsetLog(self,"sev","error");
			NXVprintLog(self,"message","Wrong group type, expected %s, got %s",
			(char *)type, data);
			NXVlog(self);
			self->errCount++;
		}
	}
	xmlFree(type);

	/*
		search attribute child nodes
	*/
	cur = groupNode->xmlChildrenNode;
	while(cur != NULL){
		if(xmlStrcmp(cur->name, (xmlChar *)"attribute") == 0){
			name = xmlGetProp(cur,(xmlChar *)"name");
			if(!H5LTfind_attribute(groupID,name)){
					NXVsetLog(self,"sev","error");
					NXVprintLog(self,"message","Required group attribute %s missing",
					name);
					NXVlog(self);
					self->errCount++;
			} else {
				/*
					TODO validate attribute data.
					As we do not use group attributes heavily, deferred for
					now
				*/
			}
			xmlFree(name);
		}
		cur = cur->next;
	}
}
/*--------------------------------------------------------------*/
static int isOptional(xmlNodePtr node)
{
	xmlChar *min= NULL;
	int num;

	min = xmlGetProp(node,(xmlChar *)"minOccurs");
	if(min == NULL){
		return 0;
	}
	num = atoi((char *)min);
	if(num == 0){
		return 1;
	} else {
		return 0;
	}
}
/*----------------------------------------------------------------
 Helper stuff for group Finding
 ----------------------------------------------------------------*/
typedef struct {
		char *nxClass;
		char *name;
} findByClassData;
/*--------------------------------------------------------------*/
static int FindByClassIterator(hid_t groupID,
	const char *name,
	const H5L_info_t *info, void *op_data)
{
	findByClassData *fbcb = (findByClassData *)op_data;
	H5O_info_t obj_info;
	hid_t attrID;
	char nxClass[132];

	H5Oget_info_by_name(groupID, name, &obj_info,H5P_DEFAULT);
	if(obj_info.type == H5O_TYPE_GROUP){
			/*
				work the NX_class attribute
			*/
			attrID = H5LTget_attribute_string(groupID,name,
				"NX_class", nxClass);
			if(attrID >= 0){
				if(strcmp(nxClass,fbcb->nxClass) == 0){
					fbcb->name = strdup(name);
					return 1;
				}
			}
  }
	return 0;
}
/*--------------------------------------------------------------
 Finding groups is hideous:
 * They may be specified by name. This seems easy but is complicated
   by the fact that the group name can either be a name attribute or
	 an attribute field.  A design flaw of NXDL, IMHO.
 * They may be specified by type and I need to search by NX_class.
---------------------------------------------------------------*/
static hid_t findGroup(pNXVcontext self, hid_t parentGroup, xmlNodePtr groupNode)
{
	xmlChar *name = NULL, *nxClass = NULL, *nodePath = NULL;
	xmlNodePtr cur = NULL;
	findByClassData fbcd;
	hid_t gid;
	hsize_t idx = 0;

  name = xmlGetProp(groupNode,(xmlChar *)"name");
	if(name == NULL){
		cur = groupNode->xmlChildrenNode;
		while(cur != NULL){
			if(xmlStrcmp(cur->name,(xmlChar *)"attribute") == 0){
				name = xmlGetProp(cur,(xmlChar *)"name");
				if(name != NULL){
					break;
				}
			}
			cur = cur->next;
		}
	}
	if(name != NULL){
		if(H5LTpath_valid(parentGroup,(char *)name, 1)){
			return H5Gopen(parentGroup,(char *)name,H5P_DEFAULT);
		} else {
			return -1;
		}
	}

	/*
		no name to be found: search by type
	*/
	nxClass = xmlGetProp(groupNode,(xmlChar *)"type");
	if(nxClass == NULL){
		NXVsetLog(self,"sev","error");
		nodePath = xmlGetNodePath(cur);
		NXVsetLog(self,"nxdlPath", (char *)nodePath);
		NXVsetLog(self,"message","Malformed group entry, type missing");
		NXVlog(self);
		xmlFree(nodePath);
		self->errCount++;
		return -1;
	}

	fbcd.nxClass = (char *)nxClass;
	fbcd.name = NULL;
	H5Literate(parentGroup, H5_INDEX_NAME, H5_ITER_INC, &idx,
		FindByClassIterator, &fbcd);
	if(fbcd.name != NULL){
		gid = H5Gopen(parentGroup,fbcd.name,H5P_DEFAULT);
		free(fbcd.name);
		return gid;
	}

	return -1;
}
/*--------------------------------------------------------------*/
static void validateLink(pNXVcontext self, hid_t groupID,
		xmlChar *name, xmlChar *target)
{
	hid_t objID;
	herr_t att;
	char linkTarget[512], dataPath[512];

	if(H5LTpath_valid(groupID,(char *)name, 1)){
		/*
		  The positive test means that the link exists and is pointing to
			a valid HDF5 object. This is alread good.
		*/
		objID = H5Oopen(groupID,(char *)name,H5P_DEFAULT);
		assert(objID >= 0); /* we just tested for existence, didn't we? */
		memset(linkTarget,0,sizeof(linkTarget));
		att = H5LTget_attribute_string(groupID,name,"target",linkTarget);
		if(att < 0){
				NXVsetLog(self,"sev","error");
				H5Iget_name(objID,dataPath,sizeof(dataPath));
				NXVsetLog(self,"dataPath",dataPath);
				NXVsetLog(self,"message","Link is missing required attribute target");
				NXVlog(self);
				self->errCount++;
		} else {
			/*
				test that the target attribute really points to something
				real. It could be that the link was done right but the
				target attribute set sloppily.
			*/
			if(!H5LTpath_valid(self->fileID,linkTarget,1)){
				NXVsetLog(self,"sev","error");
				H5Iget_name(objID,dataPath,sizeof(dataPath));
				NXVsetLog(self,"dataPath",dataPath);
				NXVprintLog(self,"message","Link target %s is invalid",
					linkTarget);
				NXVlog(self);
				self->errCount++;
			} else {
			/*
			  TODO check that the link actually points to where the
				application definition requires it to point. This is rather
				tricky for a number of reasons:
				- In the application definition we have a path defined by group
				  types, in linkTarget by actual names
				- The starting point is unclear: we may start at root, but also
				  at a sub entry.

				I could implement this by working myself backwards from the
				linkTarget and checking that all the groups down the path have
				the right class.

			*/
			}
		}
		H5Oclose(objID);
	} else {
		NXVsetLog(self,"sev","error");
		NXVprintLog(self,"message",
			"Required link %s missing or not pointing to an HDF5 object",
			(char *)name);
		NXVlog(self);
		self->errCount++;
	}

}
/*--------------------------------------------------------------*/
static void validateDependsOn(pNXVcontext self, hid_t groupID,
	hid_t fieldID)
{
	/* TODO */
}
/*---------------------------------------------------------------
We need two passes:

* In the first pass we try to find all the stuff in the NXDL
  group
* In the second pass we scan the HDF5 group in order to
  locate extra stuff

I keep the names already seen in the first pass in a
hash table

----------------------------------------------------------------*/
int NXVvalidateGroup(pNXVcontext self, hid_t groupID,
	xmlNodePtr groupNode)
{
		hash_table namesSeen;
		xmlNodePtr cur = NULL;
		xmlChar *name = NULL;
		xmlChar *target = NULL;
		hid_t childID;
		char fName[256], childName[512], nxdlChildPath[512], childPath[512];
		char mynxdlPath[512];
		char *savedNXDLPath;
		/*
			manage nxdlPath, xmlGetNodePath does not work
		*/
		savedNXDLPath = self->nxdlPath;
		name = xmlGetProp(groupNode,(xmlChar *)"type");
		if(self->nxdlPath == NULL) {
			snprintf(mynxdlPath,sizeof(mynxdlPath),"/%s", (char *) name);
		} else {
			snprintf(mynxdlPath,sizeof(mynxdlPath),"%s/%s",
				self->nxdlPath, (char *) name);
		}
		xmlFree(name);
		self->nxdlPath = mynxdlPath;

		/*
			tell what we are doing
		*/
		H5Iget_name(groupID,fName,sizeof(fName));
		NXVsetLog(self,"sev","debug");
		NXVsetLog(self,"message","Validating group");
		NXVsetLog(self,"nxdlPath",self->nxdlPath);
		NXVsetLog(self,"dataPath",fName);
		NXVlog(self);


		validateGroupAttributes(self, groupID, groupNode);
		hash_construct_table(&namesSeen,100);

		/* first pass */
		cur = groupNode->xmlChildrenNode;
		while(cur != NULL){
			if(xmlStrcmp(cur->name,(xmlChar *) "group") == 0){
					childID = findGroup(self, groupID, cur);
					if(childID >= 0){
							H5Iget_name(childID, childName,sizeof(childName));
							hash_insert(childName,strdup(""),&namesSeen);
							NXVvalidateGroup(self,childID,cur);
					} else {
						name = xmlGetProp(cur,(xmlChar *)"type");
						snprintf(nxdlChildPath,sizeof(nxdlChildPath),"%s/%s",
							self->nxdlPath, (char *)name);
						xmlFree(name);
						NXVsetLog(self,"dataPath",fName);
						NXVsetLog(self,"nxdlPath", nxdlChildPath);
						if(!isOptional(cur)){
							NXVsetLog(self,"sev","error");
							NXVsetLog(self,"message","Required group missing");
							NXVlog(self);
							self->errCount++;
						} else {
							NXVsetLog(self,"sev","warnopt");
							NXVsetLog(self,"message","Optional group missing");
							NXVlog(self);
						}
					}
			}
			if(xmlStrcmp(cur->name,(xmlChar *) "field") == 0){
					name = xmlGetProp(cur,(xmlChar *)"name");
					if(H5LTfind_dataset(groupID,(char *)name) ) {
						childID = H5Dopen(groupID,(char *)name,H5P_DEFAULT);
					} else {
						childID = -1;
					}
					snprintf(childPath,sizeof(childPath),"%s/%s",
						fName,name);
					if(childID < 0){
						NXVsetLog(self,"dataPath",childPath);
						snprintf(nxdlChildPath,sizeof(nxdlChildPath),
							"%s/%s", self->nxdlPath, name);
						NXVsetLog(self,"nxdlPath", nxdlChildPath);
						if(!isOptional(cur)){
									NXVsetLog(self,"sev","error");
									NXVsetLog(self,"message","Required field missing");
									NXVlog(self);
									self->errCount++;
						} else {
							NXVsetLog(self,"sev","warnopt");
							NXVsetLog(self,"message","Optional field missing");
							NXVlog(self);
						}
					} else {
						if(xmlStrcmp(name,(xmlChar *)"depends_on") == 0){
							/*
								This must b validated from the field level. As
								it might point to fields which are not in the
								application definition
							*/
							validateDependsOn(self,groupID,childID);
						} else {
							NXVvalidateField(self,childID, cur);
						}
						hash_insert((char *)name,strdup(""),&namesSeen);
					}
			}
			if(xmlStrcmp(cur->name,(xmlChar *) "link") == 0){
				name = xmlGetProp(cur,(xmlChar *)"name");
				target = xmlGetProp(cur,(xmlChar *)"target");
				hash_insert((char *)name,strdup(""),&namesSeen);
				validateLink(self,groupID,name, target);
				xmlFree(name);
				xmlFree(target);
			}
			cur = cur->next;
		}

		/*
			TODO: second pass Do we want this? We will warn on this
			anyway only. Defer for the time being...
		*/
		hash_free_table(&namesSeen,free);
		/*
			restore my paths...
		*/
		self->nxdlPath = savedNXDLPath;
		return 0;
	}
