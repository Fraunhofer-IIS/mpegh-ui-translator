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

// Internal headers
#include "audio_scene.h"

// External headers
#include "libxml/parser.h"

// System headers
#include <algorithm>
#include <limits>
#include <stdexcept>
#include <type_traits>

static_assert(sizeof(xmlChar) == sizeof(char), "");

namespace mpeghuitranslator {

/*!
 * Returns the next direct child in the given parent (after the given prevChild) with the given node
 * name.
 *
 * Returns NULL if no more matching children in the given parent exist.
 */
static xmlNodePtr findNextChild(xmlNodePtr parent, const std::string& name,
                                xmlNodePtr prevChild = nullptr) {
  for (auto child = prevChild ? prevChild->next : parent->children; child; child = child->next) {
    if (reinterpret_cast<const char*>(child->name) == name) {
      return child;
    }
  }
  return nullptr;
}

/*!
 * Returns the first direct child of the given parent with the given node name.
 */
static xmlNodePtr findFirstChild(xmlNodePtr node, const std::string& name) {
  return findNextChild(node, name);
}

static bool parseNodeProperty(xmlNodePtr node, std::string& outValue, const std::string& name) {
  if (auto val = xmlGetProp(node, reinterpret_cast<const xmlChar*>(name.data()))) {
    outValue = std::string(reinterpret_cast<const char*>(val));
    xmlFree(val);
    return true;
  }
  return false;
}

static bool parseNodeProperty(xmlNodePtr node, bool& outValue, const std::string& name) {
  if (auto val = xmlGetProp(node, reinterpret_cast<const xmlChar*>(name.data()))) {
    outValue = std::string{"true"} == reinterpret_cast<const char*>(val);
    xmlFree(val);
    return true;
  }
  return false;
}

static bool parseNodeProperty(xmlNodePtr node, float& outValue, const std::string& name) {
  if (auto val = xmlGetProp(node, reinterpret_cast<const xmlChar*>(name.data()))) {
    std::string tmpValue(reinterpret_cast<const char*>(val));
    xmlFree(val);

    std::size_t numDigits = std::string::npos;
    auto tmp = std::stof(tmpValue, &numDigits);
    if (numDigits != tmpValue.size()) {
      throw std::invalid_argument{"Property value of '" + name +
                                  "' is not floating-point: " + tmpValue};
    }
    outValue = tmp;
    return true;
  }
  return false;
}

static bool parseNodeProperty(xmlNodePtr node, std::intmax_t& outValue, const std::string& name) {
  if (auto val = xmlGetProp(node, reinterpret_cast<const xmlChar*>(name.data()))) {
    std::string tmpValue(reinterpret_cast<const char*>(val));
    xmlFree(val);

    std::size_t numDigits = std::string::npos;
    auto tmp = std::stoll(tmpValue, &numDigits);
    if (numDigits != tmpValue.size()) {
      throw std::invalid_argument{"Property value of '" + name + "' is not integral: " + tmpValue};
    }
    outValue = tmp;
    return true;
  }
  return false;
}

template <typename T>
static typename std::enable_if<std::is_integral<T>::value, bool>::type parseNodeProperty(
    xmlNodePtr node, T& outValue, const std::string& name) {
  std::intmax_t tmpValue{};
  if (!parseNodeProperty(node, tmpValue, name)) {
    return false;
  }
  if (tmpValue < std::numeric_limits<T>::min() || tmpValue > std::numeric_limits<T>::max()) {
    throw std::invalid_argument{"Property value of '" + name +
                                "' is out of range: " + std::to_string(tmpValue)};
  }
  outValue = static_cast<T>(tmpValue);
  return true;
}

template <typename T>
static void parseMandatoryNodeProperty(xmlNodePtr node, T& outValue, const std::string& name) {
  if (!parseNodeProperty(node, outValue, name)) {
    std::string nodeName(reinterpret_cast<const char*>(node->name));
    throw std::invalid_argument{nodeName + " has no '" + name + "' property"};
  }
}

template <typename T>
static std::unique_ptr<T> parseOptionalChild(xmlNodePtr node, const std::string& name,
                                             T (*parseElement)(xmlNodePtr)) {
  if (auto element = findFirstChild(node, name)) {
    return std::unique_ptr<T>{new T(parseElement(element))};
  }
  return nullptr;
}

static SDrcInfo parseDrcInfo(xmlNodePtr node) {
  SDrcInfo info{};

  xmlNodePtr entry = nullptr;
  while ((entry = findNextChild(node, "drcSetEffectAvailable", entry))) {
    uint32_t index = 0;
    parseMandatoryNodeProperty(entry, index, "index");
    info.availableEffects.push_back(index);
  }

  return info;
}

static SLocalizedString parseLocalizedString(xmlNodePtr node) {
  SLocalizedString string{};
  if (auto content = xmlNodeGetContent(node)) {
    string.value = std::string(reinterpret_cast<const char*>(content));
    xmlFree(content);
  }
  parseMandatoryNodeProperty(node, string.langCode, "langCode");
  return string;
}

static void fillCustomDescriptor(xmlNodePtr node, SCustomDescriptor& outDescriptor) {
  xmlNodePtr descriptor = nullptr;
  while ((descriptor = findNextChild(node, "description", descriptor))) {
    outDescriptor.description.push_back(parseLocalizedString(descriptor));
  }
}

static void fillAbstractTable(xmlNodePtr node, SAbstractTable& outTable) {
  parseMandatoryNodeProperty(node, outTable.code, "code");
  parseNodeProperty(node, outTable.alias, "alias");
}

static SPresetTable parsePresetTable(xmlNodePtr node) {
  SPresetTable table{};
  fillAbstractTable(node, table);
  parseMandatoryNodeProperty(node, table.table, "table");
  if (table.table != "PresetTable") {
    throw std::invalid_argument{"PresetTable has invalid 'table' property value: " + table.table};
  }
  return table;
}

static void fillContentKindTable(xmlNodePtr node, SContentKindTable& outTable) {
  fillAbstractTable(node, outTable);
  parseMandatoryNodeProperty(node, outTable.table, "table");
  if (outTable.table != "ContentKindTable") {
    throw std::invalid_argument{"ContentKindTable has invalid 'table' property value: " +
                                outTable.table};
  }
}

static SSwitchKindTable parseSwitchKindTable(xmlNodePtr node) {
  SSwitchKindTable table{};
  fillAbstractTable(node, table);
  parseMandatoryNodeProperty(node, table.table, "table");
  if (table.table != "SwitchKindTable") {
    throw std::invalid_argument{"SwitchKindTable has invalid 'table' property value: " +
                                table.table};
  }
  return table;
}

static SCustomDescriptor parseCustomDescriptor(xmlNodePtr node) {
  SCustomDescriptor descriptor{};
  fillCustomDescriptor(node, descriptor);
  return descriptor;
}

static void fillPropertyCommon(xmlNodePtr node, SPropertyCommon& property) {
  parseMandatoryNodeProperty(node, property.isActionAllowed, "isActionAllowed");
}

static SProminenceLevelProperty parseProminenceLevel(xmlNodePtr node) {
  SProminenceLevelProperty property{};
  fillPropertyCommon(node, property);
  parseMandatoryNodeProperty(node, property.minValue, "min");
  parseMandatoryNodeProperty(node, property.maxValue, "max");
  parseMandatoryNodeProperty(node, property.currentValue, "val");
  parseMandatoryNodeProperty(node, property.defaultValue, "def");
  return property;
}

static SMutingProperty parseMuting(xmlNodePtr node) {
  SMutingProperty property{};
  fillPropertyCommon(node, property);
  parseMandatoryNodeProperty(node, property.currentValue, "val");
  parseMandatoryNodeProperty(node, property.defaultValue, "def");
  return property;
}

static SAzimuthProperty parseAzimuth(xmlNodePtr node) {
  SAzimuthProperty property{};
  fillPropertyCommon(node, property);
  parseMandatoryNodeProperty(node, property.minValue, "min");
  parseMandatoryNodeProperty(node, property.maxValue, "max");
  parseMandatoryNodeProperty(node, property.currentValue, "val");
  parseMandatoryNodeProperty(node, property.defaultValue, "def");
  return property;
}

static SElevationProperty parseElevation(xmlNodePtr node) {
  SElevationProperty property{};
  fillPropertyCommon(node, property);
  parseMandatoryNodeProperty(node, property.minValue, "min");
  parseMandatoryNodeProperty(node, property.maxValue, "max");
  parseMandatoryNodeProperty(node, property.currentValue, "val");
  parseMandatoryNodeProperty(node, property.defaultValue, "def");
  return property;
}

static SAudioElementKind parseAudioElementKind(xmlNodePtr node) {
  SAudioElementKind kind{};
  fillContentKindTable(node, kind);
  parseNodeProperty(node, kind.langCode, "langCode");
  return kind;
}

static SCustomAudioElementKind parseCustomAudioElementKind(xmlNodePtr node) {
  SCustomAudioElementKind customKind{};
  fillCustomDescriptor(node, customKind);
  parseNodeProperty(node, customKind.langCode, "langCode");
  return customKind;
}

static SAudioElement parseAudioElement(xmlNodePtr node) {
  SAudioElement audioElement{};

  audioElement.prominence = parseOptionalChild(node, "prominenceLevelProp", parseProminenceLevel);
  audioElement.muting = parseOptionalChild(node, "mutingProp", parseMuting);
  audioElement.azimuth = parseOptionalChild(node, "azimuthProp", parseAzimuth);
  audioElement.elevation = parseOptionalChild(node, "elevationProp", parseElevation);
  audioElement.kind = parseOptionalChild(node, "kind", parseAudioElementKind);
  audioElement.customKind = parseOptionalChild(node, "customKind", parseCustomAudioElementKind);

  parseMandatoryNodeProperty(node, audioElement.id, "id");
  parseMandatoryNodeProperty(node, audioElement.isAvailable, "isAvailable");

  return audioElement;
}

static SAudioElementSwitchItem parseAudioElementSwitchItem(xmlNodePtr node, bool interactive) {
  SAudioElementSwitchItem item{};

  item.kind = parseOptionalChild(node, "kind", parseAudioElementKind);
  item.customKind = parseOptionalChild(node, "customKind", parseCustomAudioElementKind);

  parseMandatoryNodeProperty(node, item.id, "id");
  parseMandatoryNodeProperty(node, item.isAvailable, "isAvailable");

  if (interactive) {
    parseMandatoryNodeProperty(node, item.isActive, "isActive");
    parseMandatoryNodeProperty(node, item.isDefault, "isDefault");
    parseNodeProperty(node, item.isSelectable, "isSelectable");
  } else {
    item.isActive = true;
    item.isDefault = true;
    item.isSelectable = true;
  }

  return item;
}

static std::vector<SAudioElementSwitchItem> parseAudioElementSwitchItems(xmlNodePtr node) {
  std::vector<SAudioElementSwitchItem> result;
  xmlNodePtr preset = nullptr;
  while ((preset = findNextChild(node, "audioElement", preset))) {
    result.push_back(parseAudioElementSwitchItem(preset, true /* interactive */));
  }

  return result;
}

static SAudioElementSwitch parseAudioElementSwitchGroup(xmlNodePtr node) {
  SAudioElementSwitch switchGroup{};

  switchGroup.prominence = parseOptionalChild(node, "prominenceLevelProp", parseProminenceLevel);
  switchGroup.muting = parseOptionalChild(node, "mutingProp", parseMuting);
  switchGroup.azimuth = parseOptionalChild(node, "azimuthProp", parseAzimuth);
  switchGroup.elevation = parseOptionalChild(node, "elevationProp", parseElevation);

  if (auto audioElements = findFirstChild(node, "audioElements")) {
    switchGroup.audioElements = parseAudioElementSwitchItems(audioElements);
  } else {
    throw std::invalid_argument{"AudioElementSwitch has no 'audioElements' property"};
  }

  switchGroup.kind = parseOptionalChild(node, "kind", parseSwitchKindTable);
  switchGroup.customKind = parseOptionalChild(node, "customKind", parseCustomDescriptor);

  parseMandatoryNodeProperty(node, switchGroup.id, "id");
  parseMandatoryNodeProperty(node, switchGroup.isAvailable, "isAvailable");
  parseMandatoryNodeProperty(node, switchGroup.isActionAllowed, "isActionAllowed");

  return switchGroup;
}

static SAudioElementSwitch parseNonInteractiveAudioElementSwitchGroup(xmlNodePtr node) {
  SAudioElementSwitch switchGroup{};

  if (auto audioElement = findFirstChild(node, "audioElement")) {
    switchGroup.audioElements.push_back(
        parseAudioElementSwitchItem(audioElement, false /* non-interactive */));
  } else {
    throw std::invalid_argument{"NonInteractiveAudioElementSwitch has no 'audioElement' property"};
  }

  switchGroup.kind = parseOptionalChild(node, "kind", parseSwitchKindTable);
  switchGroup.customKind = parseOptionalChild(node, "customKind", parseCustomDescriptor);

  parseMandatoryNodeProperty(node, switchGroup.id, "id");
  parseMandatoryNodeProperty(node, switchGroup.isAvailable, "isAvailable");
  switchGroup.isActionAllowed = false;

  return switchGroup;
}

static SPreset parsePreset(xmlNodePtr node) {
  SPreset preset{};

  preset.kind = parseOptionalChild(node, "kind", parsePresetTable);
  preset.customKind = parseOptionalChild(node, "customKind", parseCustomDescriptor);
  parseMandatoryNodeProperty(node, preset.id, "id");
  parseMandatoryNodeProperty(node, preset.isActive, "isActive");
  parseMandatoryNodeProperty(node, preset.isAvailable, "isAvailable");
  parseMandatoryNodeProperty(node, preset.isDefault, "isDefault");

  {
    xmlNodePtr audioElement = nullptr;
    while ((audioElement = findNextChild(node, "audioElement", audioElement))) {
      preset.audioElements.push_back(parseAudioElement(audioElement));
    }
    while ((audioElement = findNextChild(node, "nonInteractiveAudioElement", audioElement))) {
      preset.audioElements.push_back(parseAudioElement(audioElement));
    }
  }

  {
    xmlNodePtr audioElementSwitchGroup = nullptr;

    while ((audioElementSwitchGroup =
                findNextChild(node, "nonInteractiveAudioElementSwitch", audioElementSwitchGroup))) {
      preset.switchGroups.push_back(
          parseNonInteractiveAudioElementSwitchGroup(audioElementSwitchGroup));
    }

    while ((audioElementSwitchGroup =
                findNextChild(node, "audioElementSwitch", audioElementSwitchGroup))) {
      preset.switchGroups.push_back(parseAudioElementSwitchGroup(audioElementSwitchGroup));
    }
  }

  return preset;
}

static std::vector<SPreset> parsePresets(xmlNodePtr node) {
  std::vector<SPreset> result;
  xmlNodePtr preset = nullptr;
  while ((preset = findNextChild(node, "preset", preset))) {
    result.push_back(parsePreset(preset));
  }

  return result;
}

SAudioSceneConfig parseAudioScene(xmlNodePtr node) {
  SAudioSceneConfig asi{};

  if (auto drcInfo = findFirstChild(node, "DRCInfo")) {
    asi.drcInfo = parseDrcInfo(drcInfo);
  }

  if (auto presets = findFirstChild(node, "presets")) {
    asi.presets = parsePresets(presets);
  }

  {
    xmlNodePtr audioElement = nullptr;
    while ((audioElement = findNextChild(node, "audioElement", audioElement))) {
      asi.audioElements.push_back(parseAudioElement(audioElement));
    }
  }

  {
    xmlNodePtr audioElementSwitchGroup = nullptr;
    while ((audioElementSwitchGroup =
                findNextChild(node, "audioElementSwitch", audioElementSwitchGroup))) {
      asi.switchGroups.push_back(parseAudioElementSwitchGroup(audioElementSwitchGroup));
    }
  }

  parseMandatoryNodeProperty(node, asi.uuid, "uuid");
  parseMandatoryNodeProperty(node, asi.version, "version");
  if (asi.version.find("9.0") != 0 && asi.version.find("10.0") != 0 &&
      asi.version.find("11.0") != 0) {
    throw std::invalid_argument{"AudioSceneConfig has invalid 'version' property value: " +
                                asi.version};
  }

  if (!parseNodeProperty(node, asi.configChanged, "configChange")) {
    asi.configChanged = false;
  }

  return asi;
}

}  // namespace mpeghuitranslator
