/*-----------------------------------------------------------------------------
Software License for The Fraunhofer FDK MPEG-H Software

Copyright (c) 2019 - 2024 Fraunhofer-Gesellschaft zur Förderung der angewandten
Forschung e.V. and Contributors
All rights reserved.

1. INTRODUCTION

The "Fraunhofer FDK MPEG-H Software" is software that implements the ISO/MPEG
MPEG-H 3D Audio standard for digital audio or related system features. Patent
licenses for necessary patent claims for the Fraunhofer FDK MPEG-H Software
(including those of Fraunhofer), for the use in commercial products and
services, may be obtained from the respective patent owners individually and/or
from Via LA (www.via-la.com).

Fraunhofer supports the development of MPEG-H products and services by offering
additional software, documentation, and technical advice. In addition, it
operates the MPEG-H Trademark Program to ease interoperability testing of end-
products. Please visit www.mpegh.com for more information.

2. COPYRIGHT LICENSE

Redistribution and use in source and binary forms, with or without modification,
are permitted without payment of copyright license fees provided that you
satisfy the following conditions:

* You must retain the complete text of this software license in redistributions
of the Fraunhofer FDK MPEG-H Software or your modifications thereto in source
code form.

* You must retain the complete text of this software license in the
documentation and/or other materials provided with redistributions of
the Fraunhofer FDK MPEG-H Software or your modifications thereto in binary form.
You must make available free of charge copies of the complete source code of
the Fraunhofer FDK MPEG-H Software and your modifications thereto to recipients
of copies in binary form.

* The name of Fraunhofer may not be used to endorse or promote products derived
from the Fraunhofer FDK MPEG-H Software without prior written permission.

* You may not charge copyright license fees for anyone to use, copy or
distribute the Fraunhofer FDK MPEG-H Software or your modifications thereto.

* Your modified versions of the Fraunhofer FDK MPEG-H Software must carry
prominent notices stating that you changed the software and the date of any
change. For modified versions of the Fraunhofer FDK MPEG-H Software, the term
"Fraunhofer FDK MPEG-H Software" must be replaced by the term "Third-Party
Modified Version of the Fraunhofer FDK MPEG-H Software".

3. No PATENT LICENSE

NO EXPRESS OR IMPLIED LICENSES TO ANY PATENT CLAIMS, including without
limitation the patents of Fraunhofer, ARE GRANTED BY THIS SOFTWARE LICENSE.
Fraunhofer provides no warranty of patent non-infringement with respect to this
software. You may use this Fraunhofer FDK MPEG-H Software or modifications
thereto only for purposes that are authorized by appropriate patent licenses.

4. DISCLAIMER

This Fraunhofer FDK MPEG-H Software is provided by Fraunhofer on behalf of the
copyright holders and contributors "AS IS" and WITHOUT ANY EXPRESS OR IMPLIED
WARRANTIES, including but not limited to the implied warranties of
merchantability and fitness for a particular purpose. IN NO EVENT SHALL THE
COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE for any direct, indirect,
incidental, special, exemplary, or consequential damages, including but not
limited to procurement of substitute goods or services; loss of use, data, or
profits, or business interruption, however caused and on any theory of
liability, whether in contract, strict liability, or tort (including
negligence), arising in any way out of the use of this software, even if
advised of the possibility of such damage.

5. CONTACT INFORMATION

Fraunhofer Institute for Integrated Circuits IIS
Attention: Division Audio and Media Technologies - MPEG-H FDK
Am Wolfsmantel 33
91058 Erlangen, Germany
www.iis.fraunhofer.de/amm
amm-info@iis.fraunhofer.de
-----------------------------------------------------------------------------*/

#pragma once
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  /*! The operation was executed successfully  */
  MPEGHUITRANSLATOR_OK,
  /*!
   * At least one of the function arguments is invalid.
   *
   * This status is e.g. returned when a NULL pointer or an empty size is given for non-optional
   * buffer arguments.
   */
  MPEGHUITRANSLATOR_INVALID_ARGUMENT,
  /*! The output buffer argument is not large enough to hold the output value */
  MPEGHUITRANSLATOR_INSUFFICIENT_SPACE,
  /*! An internal error occurred during processing of the operation */
  MPEGHUITRANSLATOR_INTERNAL_ERROR,
} MpeghUiTranslatorStatusCode;

typedef struct MpeghUiTranslatorStringList {
  char** strings;
  size_t numStrings;
} MpeghUiTranslatorStringList;

/*!
 * Simple conversion of the given MPEG-H UI manager AudioScene XML to the proposed JSON format for
 * application standards defined in the json_schema/ project folder.
 *
 * If the output buffer is too small, this function returns MPEGHUITRANSLATOR_INSUFFICIENT_SPACE
 * and sets the outJsonBufferSize output parameter to the number of bytes that would be required.
 *
 * NOTE: This function reads and updates the thread-safe INTERNAL GLOBAL STATE shared with calls
 * to #mpeghInteractivityToXml().
 *
 * See <a href="https://github.com/Fraunhofer-IIS/mpeghdec/wiki/MPEG-H-UI-manager-XML-format">the
 * MPEG-H decoder wiki</a> for the specification of the MPEG-H UI manager AudioScene XML format.
 */
MpeghUiTranslatorStatusCode mpeghUiTranslatorToJson(const char* audioSceneXml,
                                                    size_t audioSceneXmlSize, char* outJsonBuffer,
                                                    size_t* outJsonBufferSize);

/*!
 * Simple conversion of the given proposed JSON format for application standards defined in the
 * json_schema/ project folder to a list of XML ActionEvent objects to be sent to the MPEG-H UI
 * manager.
 *
 * If the outActionScenes output parameter is allocated (has a non-zero numStrings member) but
 * cannot hold enough entries, this function returns MPEGHUITRANSLATOR_INSUFFICIENT_SPACE and sets
 * the outActionScenes#numStrings member to the required number of entries.
 *
 * NOTE: This function reads and updates the thread-safe INTERNAL GLOBAL STATE shared with calls to
 * #mpeghInteractivityToJson().
 *
 * NOTE: The output strings are allocated on the heap and need to be freed by the caller! This can
 * be done e.g. by calling mpeghUiTranslatorFreeStrings().
 *
 * See <a href="https://github.com/Fraunhofer-IIS/mpeghdec/wiki/MPEG-H-UI-manager-XML-format">the
 * MPEG-H decoder wiki</a> for the specification of the MPEG-H UI manager ActionEvent XML format.
 */
MpeghUiTranslatorStatusCode mpeghUiTranslatorToXml(const char* sceneChangesJson,
                                                   size_t sceneChangesJsonSize,
                                                   MpeghUiTranslatorStringList* outActionScenes);

/*!
 * Frees the strings and the #strings member of the given string list via free() and resets the
 * #numStrings member.
 *
 * NOTE: The given string list pointer itself is not freed!
 *
 * NOTE: This function should only be called on a string list where the entries are allocated on the
 * heap (via malloc()).
 */
void mpeghUiTranslatorFreeStrings(MpeghUiTranslatorStringList* list);

/*!
 * Returns a human-readable error message for the given error status code.
 *
 * If the given status code is not an error or unknown, NULL is returned.
 *
 * NOTE: The returned pointer references internal static memory and must therefore NOT be freed by
 * the caller!
 */
const char* mpeghUiTranslatorLastError(MpeghUiTranslatorStatusCode code);

#ifdef __cplusplus
}  // extern "C"
#endif
