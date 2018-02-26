/* only for getTime() */
#include <sys/time.h>
#include <sys/resource.h>

#include "utils/basics.h"
#include "utils/sort_alg.h"
//#include "utils/fileInfo.h"

#include "ilists/ildefconfig.h"
#include "ilists/bc.h"
#include "ilists/ildatatypes.h"
#include "ilists/ilerrors.c"

#define MAXUINT  0x7FFFFFFF



//modifies: answ, i, pos, and bcps.
#define next(answ, i, sampleK, zoneoccs,pos,absBits, bcs, bcpos, bcBits) \
   { uint num; \
     if (! (i%sampleK)) { \
        answ = bitread (zoneoccs, pos, absBits); \
        pos+=absBits; \
        bcpos = bitread (zoneoccs, pos, bcBits); \
        pos +=bcBits; \
     }  \
     else { \
     	BC_TO_DEC(bcs, bcpos, num); \
     	answ +=num; \
     } \
     i++; \
   }


//calculates sampleP, k for a given lenOfList
#define SAMPLEPERIOD(val, k, B, len, maxvalue) \
{	uint value; \
	value = (maxvalue * B / len); \
	/*fprintf(stderr,"\n entro ceillog %d",len);fflush(stderr);*/ \
	CEILLOG_2(value,k); \
	/*fprintf(stderr,"\n salgo ceillog %d",tmp);fflush(stderr);*/ \
	val = 1<<k; \
}

#define OBITS(val, bcsptr, i) \
 {uint tmp, bitsneeded; \
 	tmp = bcsptr[i+1] - bcsptr[i]; \
 	/*fprintf(stderr,"\n tmp %d",tmp);fflush(stderr);*/ \
 	/*if (tmp) { */ \
		CEILLOG_2((tmp+1),bitsneeded); \
		/* value 64 could be encoded in a list and 7 bits would be needed -> tmp = 64 +1 */ \
	/*} \
	else bitsneeded=0; */ \
	/*fprintf(stderr,"\n bitsneeded %d",bitsneeded);fflush(stderr);*/ \
	val = bitsneeded; \
 }
 
#define NUMBUCKETS(val, maxval, k) \
{ val = (maxval + (1<<k) -1) /(1<<k);\
}



/*  *******************************************************************************
    * function used to parse the source posting lists that are loaded in build_il.*
    * Refer to "build_il() comments below" for a more detailed description        */		
	//recreates array lenList, and uint ** occList and variables maxPostValue and nlists.
	int	parseAllPostingList(uint *nlists, uint **lenList, uint ***occList, 
                        uint *maxPostValue, uint *source, uint length);	




/* ****************************************************************************** */
/*  Prototypes of the General INTERFACE:                                          *
/*              interface for using compressed representations of posting lists   *
/*              Any alternative representation must implement all these functions *
/* ****************************************************************************** */

/* Error management */

        /* Returns a string describing the error associated with error number
          e. The string must not be freed, and it will be overwritten with
          subsequent calls. */

char *error_il (int e);


	/* returns a string with the parameters available for building the method. 
	   So that each index tells the user the available parameters.           */

char *showParameters();

    /* Creates a structure for the management of inverted lists from a
      given set of posting lists. 
      Array source has the following format...
      [Nwords=N][maxPostvalue][lenPost1][post11][post12]...[post1k�]
                              [lenPost2][post21][post22]...[post2k��]
                               ... 
                              [lenPostN][postN1][postN2]...[postNk���]
      il is an opaque data type. Any build option must be passed in string 
      build_options, whose syntax depends on the index. The index must 
      always work with some default parameters if build_options is NULL. 
      The returned structure is ready to be queried. */

int build_il (uint *source, uint length, char *build_options, void **ail);
	

    /*  Saves index on disk by using single or multiple files, having 
      proper extensions. */

int save_il (void *ail, char *filebasename);


    /*  Loads index from one or more file(s) named filename, possibly 
      adding the proper extensions. */ 

int load_il (char *filebasename, void **ail);


    /* Frees the memory occupied by index. */

int free_il (void *ail);


    /* Gives the memory occupied by index in bytes. */

int size_il(void *ail, uint *size);

    /* Gives the memory occupied by the index assuming an uncompressed
       representation (uint **)*/
int size_il_uncompressed(void *ail, uint *size);

    /* Gives the length of the longest posting-list*/
int maxSizePosting_il(void *ail, uint *size);


/*****************************************************************************/
/**** Operations (searching) with the posting lists from GENERAL INTERFACE ***/
/*****************************************************************************/


    /* Extracts the id-th list, having id \in [0..maxId-1] */

int extractList_il (void *ail, uint id, uint **list, uint *len);

    /* Extracts the id-th list, having id \in [0..maxId-1] 
       Assumes list has enough space previously allocated. */

int extractListNoMalloc_il (void *ail, uint id, uint *list, uint *len);



    /* Sets options for searches: such as parameters, funtions to use, etc 
       Permits to set different algorithms for intersect2, intersectN and 
       if needed "fsearch-operations needed by svs-type algorithms"     */

int setDefaultSearchOptions_il (void *ail, char *search_options);

	/* Intersects two lists given by id1 and id2  \in [0..maxId-1].
	   The defaultIntersect2 function must be initialized previously (svs, merge, ...)
	   Returns the resulting list of "intersecting positions"  */

int intersect_2_il (void *ail, uint id1, uint id2,  uint *noccs, uint **occs);

	/* Intersects $nids$ lists given by ids in ids[], having ids[i] \in [0..maxId-1].
	   Returns the resulting list of "intersecting positions"  */

int intersect_N_il (void *ail, uint *ids, uint nids, uint *noccs, uint **occs );


	/* Returns the len of the id-th posting list */
int lenlist(void *ail, uint id, uint *len);




/* ************************************************************** */
/* prototypes of search functions implemented for THIS structure. */
/* Most are "private" functions that are used depending on the    *
/* default_configuration set through "setDefaultSearchOptions_il".*
/* the implemented functions depend on the structure implemented. */
/* ************************************************************** */

	/* FSEARCH: searches a value in the posting-list of word-ID  
	   returns: 1 if $value \in list_{id}$,  0 if value not found */

int fsearchUnc (void *ail, uint id, uint value);

int fsearchBitmap (void *ail, uint id, uint value);  /*FOR BITMAPS   */

int fsearchDec (void *ail, uint id, uint value);     /*FOR BYTECODES */
int fsearchSeq (void *ail, uint id, uint value);
int fsearchBin (void *ail, uint id, uint value); 

	/* INTERSECT_2 algorithms */
int svs2   (void *ail, uint id1, uint id2, uint *noccs, uint **occs);
int merge2 (void *ail, uint id1, uint id2, uint *noccs, uint **occs);
int lookup2(void *ail, uint id1, uint id2, uint *noccs, uint **occs);


	/* INTERSECT_N algorithms */
int svsn    (void *ail, uint *ids, uint nids, uint *noccs, uint **occs);
int lookupn (void *ail, uint *ids, uint nids, uint *noccs, uint **occs);



/* ************************************************************** */
/* Other auxiliar functions                                       */
/* ************************************************************** */
	int shouldUseBitmap(t_il *il, uint id);
	//int merge2ByteCodes(void *ail, uint id1, uint id2, uint *noccs, uint **occs);
	//int merge2Hybrid(void *ail, uint idbitmap, uint idbytecode, uint *noccs, uint **occs);
	int mergeNBitmap (void *ail, uint *ids, register uint nids,  uint *noccs, uint **occs);
