/*------------------------------------------------------------------------------------------------*/
/* UNICENS XML Parser                                                                             */
/* Copyright 2017, Microchip Technology Inc. and its subsidiaries.                                */
/*                                                                                                */
/* Redistribution and use in source and binary forms, with or without                             */
/* modification, are permitted provided that the following conditions are met:                    */
/*                                                                                                */
/* 1. Redistributions of source code must retain the above copyright notice, this                 */
/*    list of conditions and the following disclaimer.                                            */
/*                                                                                                */
/* 2. Redistributions in binary form must reproduce the above copyright notice,                   */
/*    this list of conditions and the following disclaimer in the documentation                   */
/*    and/or other materials provided with the distribution.                                      */
/*                                                                                                */
/* 3. Neither the name of the copyright holder nor the names of its                               */
/*    contributors may be used to endorse or promote products derived from                        */
/*    this software without specific prior written permission.                                    */
/*                                                                                                */
/* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"                    */
/* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE                      */
/* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE                 */
/* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE                   */
/* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL                     */
/* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR                     */
/* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER                     */
/* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,                  */
/* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE                  */
/* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.                           */
/*------------------------------------------------------------------------------------------------*/
#ifndef UCSXML_H_
#define UCSXML_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include "ucs_api.h"

/** Structure holding informations to startup UNICENS (UCS).
 *  Pass all these variables to the UCS manager structure, but not pInternal.
 *  */
typedef struct
{
    /** The amount of bytes assigned to the async channel*/
    uint16_t packetBw;
    /** Array of routes */
    Ucs_Rm_Route_t *pRoutes;
    /** Route array size */
    uint16_t routesSize;
    /** Array of nodes */
    Ucs_Rm_Node_t *pNod;
    /** Node array size */
    uint16_t nodSize;
    /** Internal data, to be ignored */
    void *pInternal;
} UcsXmlVal_t;

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/
/*                            Public API                                */
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/

/**
 * \brief Initializes UNICENS XML parser module, parses the given string and
 *        generate the data needed to run UNICENS (UCS) library.
 *
 * \note In case of errors the callback UcsXml_CB_OnError will be raised.
 * \param xmlString - Zero terminated XML string. The string will not be used
 *                    after this function call.
 * \return Structure holding the needed data for UCS. NULL, if there was an error.
 *         The structure will be created dynamically, to free the data call UcsXml_FreeVal.
 */
UcsXmlVal_t *UcsXml_Parse(const char *xmlString);

/**
 * \brief Frees the given structure, generated by UcsXml_Parse.
 *
 * \note In case of errors the callback UcsXml_CB_OnError will be raised.
 * \param val - The structure to be freed.
 */
void UcsXml_FreeVal(UcsXmlVal_t *val);

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/
/*                        CALLBACK SECTION                              */
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/

/**
 * \brief Callback whenever a parser error occurs. The message is human readable.
 * \note This function must be implemented by the integrator.
 *
 * \param format - Zero terminated format string (following printf rules)
 * \param vargsCnt - Amount of parameters stored in "..."
 */
extern void UcsXml_CB_OnError(const char format[], uint16_t vargsCnt, ...);

#ifdef __cplusplus
}
#endif

#endif /* UCSXML_H_ */