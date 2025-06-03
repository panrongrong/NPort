/* dllLib.h - doubly linked list library header */

/*
* Copyright (c) 1984-2006, 2009-2010, 2014-2016 Wind River Systems, Inc.
*
* The right to copy, distribute, modify or otherwise make use
* of this software may be licensed only pursuant to the terms
* of an applicable Wind River license agreement.
*/

/*
modification history
--------------------
06oct15,clt  Added cert related function prototype (V7SP-483)
25aug14,clt  vx7-cert clean up
04dec10,pad  Replaced FUNCPTR with fully-qualified function pointer in
             dllEach3()'s signature.
10mar16,wfl  Avoid compilation errors when certain header files are used in C++
             (V7COR-3972)
28apr10,pad  Moved extern C statement after include statements.
26jan09,pcs  Updated to add support for LP64 data model.
24jul06,jpb  Removal of duplicate definition of DL_LIST3 and DL_NODE3.
26jan06,mil  Updated for POSIX namespace conformance (P2).
03jun05,yvp  Added #ifndef	_ASMLANGUAGE.
             Updated copyright. #include now with angle-brackets.
19jan05,vvv  moved some macros here from osdep.h
05may04,cjj  removed non-standard prototypes.
22sep92,rrr  added support for c++
04jul92,jcf  cleaned up.
26may92,rrr  the tree shuffle
04oct91,rrr  passed through the ansification filter
		     -changed VOID to void
		     -changed copyright notice
08apr91,jdi  added NOMANUAL to prevent mangen.
20dec90,gae  fixed declaration of dllRemove.
05oct90,shl  added ANSI function prototypes.
		     added copyright notice.
07aug89,jcf  written.
*/

/*
 * Note: This file contains definitions that must be maintained in both kernel
 * and user space
 */

#ifndef __INCdllLib3h
#define __INCdllLib3h


/* defines */

/* typedefs */
typedef struct _Vx_dlnode3	        /* Node of a linked list. */
    {
    struct _Vx_dlnode3* next;    	/* Points at the next node in the list */
    struct _Vx_dlnode3* previous;    /* Points at the previous node in the list */
    } _Vx_DL_NODE3;


typedef struct			            /* Header for a linked list. */
    {
    _Vx_DL_NODE3* head;	            /* header of list */
    _Vx_DL_NODE3* tail;	            /* tail of list */
    } _Vx_DL_LIST3;

typedef _Vx_DL_NODE3 DL_NODE3;
typedef _Vx_DL_LIST3 DL_LIST3;


/******************************************************************************
*
* DLL_INIT3 - initialize doubly linked list descriptor
*
* Initialize the specified list to an empty list.
*
* RETURNS: N/A
*
* NOMANUAL
*/

#define DLL_INIT3(list)                                  \
    {                                                   \
    ((DL_LIST3 *)(list))->head = NULL;                   \
    ((DL_LIST3 *)(list))->tail = NULL;                   \
    }

/*******************************************************************************
*
* dllFirst - find first node in list
*
* DESCRIPTION
* Finds the first node in a doubly linked list.
*
* RETURNS: Pointer to the first node in a list, or NULL if the list is empty.
*
* NOMANUAL
*/

#define DLL_FIRST3(pList)		       \
    (					               \
    (((DL_LIST3 *)(pList))->head)	   \
    )

/*******************************************************************************
*
* dllLast - find last node in list
*
* Finds the last node in a doubly linked list.
*
* RETURNS: Pointer to the last node in list, or NULL if the list is empty.
*
* NOMANUAL
*/

#define DLL_LAST3(pList)	                \
    (					                \
    (((DL_LIST3 *)(pList))->tail)	    \
    )

/*******************************************************************************
*
* dllNext - find next node in list
*
* Locates the node immediately after the node pointed to by the pNode.
*
* RETURNS: Pointer to the next node in list, or NULL if there is no next node.
*
* NOMANUAL
*/

#define DLL_NEXT3(pNode)			        \
    (					                \
    (((DL_NODE3 *)(pNode))->next)	    \
    )

/*******************************************************************************
*
* dllPrevious - find preceding node in list
*
* Locates the node immediately before the node pointed to by the pNode.
*
* RETURNS: Pointer to the preceding node in the list, or
* NULL if there is no next node.
*
* NOMANUAL
*/

#define DLL_PREVIOUS3(pNode)		        \
    (					                \
    (((DL_NODE3 *)(pNode))->previous)	\
    )

/*******************************************************************************
*
* dllEmpty - boolean function to check for empty list
*
* RETURNS: TRUE if list is empty.  FALSE otherwise
*
* NOMANUAL
*/

#define DLL_EMPTY3(pList)			        \
    (						                \
    (((DL_LIST3 *)pList)->head == NULL)		\
    )

/*******************************************************************************
*
* DLL_INSERT3 - insert node in list after specified node
*
* This macro inserts the specified node in the specified list.
* The new node is placed following the specified 'previous' node in the list.
* If the specified previous node is NULL, the node is inserted at the head
* of the list.
*
* RETURNS: N/A
*
* NOMANUAL
*/

#define DLL_INSERT3(list, previousNode, node)                           \
    {                                                                  \
    DL_NODE3 *temp;                                                     \
    if ((previousNode) == NULL)                                        \
        {                                                              \
        temp = ((DL_LIST3 *)(list))->head;                              \
        ((DL_LIST3 *)(list))->head = (DL_NODE3 *)(node);                 \
        }                                                              \
    else                                                               \
        {                                                              \
        temp = ((DL_NODE3 *)(previousNode))->next;                      \
        ((DL_NODE3 *)(previousNode))->next = (DL_NODE3 *)(node);         \
        }                                                              \
    if (temp == NULL)                                                  \
        {                                                              \
        ((DL_LIST3 *)(list))->tail = (DL_NODE3 *)(node);                 \
        }                                                              \
    else                                                               \
        {                                                              \
        temp->previous = (DL_NODE3 *)(node);                            \
        }                                                              \
    ((DL_NODE3 *)(node))->next = temp;                                  \
    ((DL_NODE3 *)(node))->previous = (DL_NODE3 *)(previousNode);         \
    }

/*******************************************************************************
*
* DLL_ADD3 - add node to end of list
*
* This macro adds the specified node to the end of the specified list.
*
* RETURNS: N/A
*
* NOMANUAL
*/

#define DLL_ADD3(list, node)                                      \
   {                                                             \
   DL_NODE3* listTail = (list)->tail;                             \
   DLL_INSERT3 ((list), listTail, (node));                        \
   }

/*******************************************************************************
*
* DLL_REMOVE3 - remove specified node in list
*
* Remove the specified node in the doubly linked list.
*
* RETURNS: N/A
*
* NOMANUAL
*/

#define DLL_REMOVE3(list, node)                                              \
   {                                                                        \
   if (((DL_NODE3 *)(node))->previous == NULL)                               \
       {                                                                    \
       ((DL_LIST3 *)(list))->head = ((DL_NODE3 *)(node))->next;               \
       }                                                                    \
   else                                                                     \
       {                                                                    \
       ((DL_NODE3 *)(node))->previous->next = ((DL_NODE3 *)(node))->next;     \
       }                                                                    \
   if (((DL_NODE3 *)(node))->next == NULL)                                   \
       {                                                                    \
       ((DL_LIST3 *)(list))->tail = ((DL_NODE3 *)(node))->previous;           \
       }                                                                    \
   else                                                                     \
       {                                                                    \
       ((DL_NODE3 *)(node))->next->previous = ((DL_NODE3 *)(node))->previous; \
       }                                                                    \
   }

/******************************************************************************
*
* DLL_GET3 - get (delete and return) first node from list
*
* This macro gets the first node from the specified list, deletes the node
* from the list, and returns a pointer to the node gotten.
*
* RETURNS: Pointer to the first node in list, or NULL if there is list empty. 
*
* NOMANUAL
*/

#define DLL_GET3(list, node)                                                \
   {                                                                       \
   (node) = (void *)((DL_LIST3 *)(list))->head;                             \
   if ((node) != NULL)                                                     \
       {                                                                   \
       ((DL_LIST3 *)(list))->head = ((DL_NODE3 *)(node))->next;              \
       if (((DL_NODE3 *)(node))->next == NULL)                              \
            {                                                              \
            ((DL_LIST3 *)(list))->tail = NULL;                              \
            }                                                              \
       else                                                                \
            {                                                              \
            ((DL_NODE3 *)(node))->next->previous = NULL;                    \
            }                                                              \
       }                                                                   \
   }

/* function declarations */
/*
extern DL_LIST3 * dllCreate (void);
//extern DL_NODE3 * dllEach3 (DL_LIST3 *pList,
//                          DLL_EACH_FUNC routine,
//                          _Vx_usr_arg_t routineArg);
extern DL_NODE3 * dllGet (DL_LIST3* pList);
extern STATUS  	 dllInit (DL_LIST3* pList);
extern int       dllCount (DL_LIST3* pList);
extern void 	 dllAdd (DL_LIST3* pList, DL_NODE3* pNode);
extern void 	 dllInsert (DL_LIST3* pList, DL_NODE3* pPrev, DL_NODE3* pNode);
extern void 	 dllRemove (DL_LIST3* pList, DL_NODE3* pNode);

extern STATUS 	 dllDelete (DL_LIST3* pList);
extern STATUS 	 dllTerminate (DL_LIST3* pList);
*/

#endif /* __INCdllLib3h */

