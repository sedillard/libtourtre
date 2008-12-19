/*
Copyright (c) 2006, Scott E. Dillard
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
 are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice,
    this list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#ifndef CT_COMPONENT_H
#define CT_COMPONENT_H

#include "ctMisc.h"

typedef 
enum ctComponentType 
{ 
    CT_JOIN_COMPONENT, CT_SPLIT_COMPONENT 
} ctComponentType;

typedef struct ctComponent
{
	size_t birth, death, last;
	struct ctComponent *pred, *succ;
	struct ctComponent *nextPred, *prevPred;
	struct ctComponent *uf; /* union find */
        ctComponentType type;
	void * data; /* user data */
} ctComponent;

ctComponent*  ctComponent_new          ( ctComponentType type );
        void  ctComponent_delete       ( ctComponent * self);
        void  ctComponent_addPred      ( ctComponent * self, ctComponent * c );
        void  ctComponent_removePred   ( ctComponent * self, ctComponent * c );

ctComponent*  ctComponent_eatSuccessor ( ctComponent * self ); 
                /* merges this with successor. this becomes the new, merged
                 * component. successor is returned. */

        void  ctComponent_prune        ( ctComponent * self );
         int  ctComponent_isLeaf       ( ctComponent * self );
         int  ctComponent_isRegular    ( ctComponent * self );
ctComponent*  ctComponent_find         ( ctComponent * self );
        void  ctComponent_union        ( ctComponent * a, ctComponent * b );

#endif

