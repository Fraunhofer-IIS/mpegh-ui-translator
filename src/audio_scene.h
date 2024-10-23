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

// External headers
#include "json/forwards.h"

// System headers
#include <cstdint>
#include <string>
#include <vector>
#include <memory>

using xmlNodePtr = struct _xmlNode*;

namespace mpeghuitranslator {

/*!
 * Global Unique IDentifier.
 */
using SUuid = std::string;

/*!
 * ISO 639-2 3-letter code.
 */
using SIso639Code = std::string;

struct SLocalizedString {
  SIso639Code langCode;
  std::string value;
};

/*!
 * Contains all available DRC effects for the current MPEG-H content.
 */
struct SDrcInfo {
  std::vector<uint32_t> availableEffects;
};

struct SCustomDescriptor {
  std::vector<SLocalizedString> description;
};

struct SAbstractTable {
  uint8_t code;
  std::string alias;
};

struct SPresetTable : SAbstractTable {
  std::string table = "PresetTable";
};

/*!
 * Common values shared across all properties.
 */
struct SPropertyCommon {
  // Whether a user is allowed to perform interactivity on this property type
  bool isActionAllowed;
};

/*!
 * The prominence level property describes all parameters related to the gain control of an audio
 * element or an audio element switch group in relation to the complete audio scene.
 *
 * If the prominence level is changed, the MPEG-H decoder also adjusts the gains of all other audio
 * elements, so that the overall loudness of the audio scene stays constant.
 *
 * The maximal allowed range of the prominence level is between -63dB and 31dB. The applicable
 * bounds of a single element can be limited further by the minValue and maxValue members.
 */
struct SProminenceLevelProperty : SPropertyCommon {
  float minValue;
  float maxValue;
  float currentValue;
  float defaultValue;
};

/*!
 * The muting property describes parameters related to control muting of an audio element or audio
 * element switch group.
 */
struct SMutingProperty : SPropertyCommon {
  // Current muting state, true for muted, false for not muted
  bool currentValue;
  bool defaultValue;
};

/*!
 * The azimuth property allows azimuth positioning of an audio element or audio element switch group
 * within the maximum allowed azimuth range between -180° and 180° as defined in ISO/IEC 23008-3.
 * The applicable bounds of a single element can be limited further by the minValue and maxValue
 * members.
 */
struct SAzimuthProperty : SPropertyCommon {
  // Rightmost position in degree
  float minValue;
  // Leftmost position in degree
  float maxValue;
  float currentValue;
  float defaultValue;
};

/*!
 * The elevation property allows the elevation positioning of an audio element or audio element
 * switch group within the maximum allowed elevation range between -90° and 90° as defined in
 * ISO/IEC 23008-3. The applicable bounds of a single element can be limited further by the minValue
 * and maxValue members.
 */
struct SElevationProperty : SPropertyCommon {
  // Lowermost position in degree
  float minValue;
  // Uppermost position in degree
  float maxValue;
  float currentValue;
  float defaultValue;
};

struct SContentKindTable : SAbstractTable {
  std::string table = "ContentKindTable";
};

struct SSwitchKindTable : SAbstractTable {
  std::string table = "SwitchKindTable";
};

struct SAudioElementKind : SContentKindTable {
  // The language of the associated audio content
  SIso639Code langCode;
};

struct SCustomAudioElementKind : SCustomDescriptor {
  // The language of the associated audio content
  SIso639Code langCode;
};

/*!
 * Describes all parameters of an audio scene related to an audio element.
 */
struct SAudioElement {
  std::unique_ptr<SProminenceLevelProperty> prominence;
  std::unique_ptr<SMutingProperty> muting;
  std::unique_ptr<SAzimuthProperty> azimuth;
  std::unique_ptr<SElevationProperty> elevation;
  std::unique_ptr<SAudioElementKind> kind;
  std::unique_ptr<SCustomAudioElementKind> customKind;
  int id;
  bool isAvailable;
};

/*!
 * Single selection option in an audio element switch group.
 */
struct SAudioElementSwitchItem {
  std::unique_ptr<SAudioElementKind> kind;
  std::unique_ptr<SCustomAudioElementKind> customKind;
  int id;
  bool isActive;
  bool isAvailable;
  bool isSelectable = true;
  bool isDefault;
};

/*!
 * Describes all parameters of an audio scene related to an audio element switch group.
 */
struct SAudioElementSwitch {
  std::unique_ptr<SProminenceLevelProperty> prominence;
  std::unique_ptr<SMutingProperty> muting;
  std::unique_ptr<SAzimuthProperty> azimuth;
  std::unique_ptr<SElevationProperty> elevation;
  std::vector<SAudioElementSwitchItem> audioElements;
  std::unique_ptr<SSwitchKindTable> kind;
  std::unique_ptr<SCustomDescriptor> customKind;
  int id;
  bool isAvailable;
  bool isActionAllowed;
};

/*!
 * Collected information of a MPEG-H preset.
 */
struct SPreset {
  std::unique_ptr<SPresetTable> kind;
  std::unique_ptr<SCustomDescriptor> customKind;
  // The ID of this preset
  int id;
  // Whether this preset is currently applied
  bool isActive;
  // Whether this preset is currently available for selection
  bool isAvailable;
  // Whether this preset is the default selected one
  bool isDefault;
  // NOTE: Only available in version >= 10 of the AudioScene XML format!
  std::vector<SAudioElement> audioElements;
  // NOTE: Only available in version >= 10 of the AudioScene XML format!
  std::vector<SAudioElementSwitch> switchGroups;
};

/*!
 * NOTE: This structure is only available in version >= 10 of the AudioScene XML format!
 */
struct SAvailableLanguage {
  SIso639Code langCode;
};

struct SAudioSceneConfig {
  SUuid uuid;
  std::string version = "9.0";
  // Whether the AudioScene changed in the MPEG-H bitstream
  bool configChanged;
  SDrcInfo drcInfo;
  std::vector<SPreset> presets;
  // NOTE: Only available in version 9 of the AudioScene XML format, version >= 10 contains audio
  // elements on a per-preset level.
  std::vector<SAudioElement> audioElements;
  // NOTE: Only available in version 9 of the AudioScene XML format, version >= 10 contains switch
  // groups on a per-preset level.
  std::vector<SAudioElementSwitch> switchGroups;
};

SAudioSceneConfig parseAudioScene(xmlNodePtr node);

/*!
 * Composes a JSON object defined by the proposed JSON format for application standards in the
 * json_schema/ project folder from the given AudioScene config.
 *
 * The displayLanguageHint parameter is written as-is to the output JSON.
 */
Json::Value composeAudioScene(const SAudioSceneConfig& asi, const SIso639Code& displayLanguageHint);

}  // namespace mpeghuitranslator
