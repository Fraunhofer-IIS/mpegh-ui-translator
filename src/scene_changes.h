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

// Internal headers
#include "audio_scene.h"

// External headers
#include "json/forwards.h"

// System headers
#include <cstdint>
#include <string>
#include <vector>

namespace mpeghuitranslator {

/*!
 * Container structure to store a value that may or may not be changed.
 *
 * This can be seen as a very simplified version of std::optional in which the member object is
 * always (default-)instantiated, even if no "changed" object is to be stored.
 */
template <typename T>
struct SValueChange {
  SValueChange() = default;

  explicit SValueChange(const T& val) : newValue(val), isChanged(true) {}
  explicit SValueChange(T&& val) : newValue(std::move(val)), isChanged(true) {}

  void set(T&& val) {
    newValue = std::move(val);
    isChanged = true;
  }

  /*!
   * Returns whether this object contains a value different from the given reference value.
   */
  bool isUpdated(const T& value) const { return isChanged && newValue != value; }

  T newValue;
  bool isChanged;
};

/*!
 * Container for changes to a single AudioElement and its properties.
 */
struct SAudioElementChanges {
  int id;
  SValueChange<double> prominence;
  SValueChange<bool> muting;
  SValueChange<double> azimuth;
  SValueChange<double> elevation;
};

/*!
 * Container for changes to a single AudioElementSwitch and its properties.
 */
struct SSwitchGroupChanges {
  int id;
  SValueChange<int> activeObject;
  SValueChange<bool> muting;
  std::vector<SAudioElementChanges> audioElements;
};

/*!
 * Container for changes to a single Preset and its properties.
 */
struct SPresetChanges {
  int id;
  SValueChange<bool> isActive;
  std::vector<SAudioElementChanges> audioElements;
  std::vector<SSwitchGroupChanges> switchGroups;
};

/*!
 * Container for changes to the AudioScene and its properties.
 */
struct SAudioSceneChanges {
  SUuid uuid;
  SValueChange<SIso639Code> displayLanguage;
  std::vector<SPresetChanges> presets;
};

/*!
 * Parses and transforms the given JSON object conforming to the proposed JSON format for
 * application standards in the json_schema/ project folder into a collection of changes to an
 * MPEG-H UI manager AudioScene represented by the JSON values.
 */
SAudioSceneChanges parseAudioSceneChanges(const Json::Value& json);

/*!
 * Converts the given list of changes to the AudioScene to a list of XML strings containing the
 * MPEG-H UI manager ActionEvents to apply to effect the given changes.
 *
 * If the baseAsi parameter is not set (NULL), only "global" ActionEvents are generated,
 * which do not affect a specific AudioScene or its object.
 *
 * If the baseDisplayLanguageCode parameters is not set (NULL), the current display language is
 * considered to always be different from the updated values in the sceneChanges parameter, always
 * generating an ActionEvent if the display language in the sceneChanges is "changed".
 */
std::vector<std::string> composeActionEvents(const SAudioSceneChanges& sceneChanges,
                                             const SAudioSceneConfig* baseAsi,
                                             const std::string* baseDisplayLanguageCode);

}  // namespace mpeghuitranslator
