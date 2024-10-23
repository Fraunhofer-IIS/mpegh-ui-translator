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
#include "scene_changes.h"
#include "xml_helper.h"

// External headers
#include "libxml/tree.h"

// System headers
#include <algorithm>
#include <stdexcept>

static const std::string NO_UUID = "00000000-0000-0000-0000-000000000000";

namespace mpeghuitranslator {

template <typename T>
static const T& assertForId(const std::vector<T>& list, int id) {
  auto it = std::find_if(list.begin(), list.end(), [id](const T& entry) { return entry.id == id; });
  if (it != list.end()) {
    return *it;
  }
  throw std::invalid_argument{"Cannot apply changes for non-existing entry with id: " +
                              std::to_string(id)};
}

template <typename T>
static const T* findActive(const std::vector<T>& list) {
  auto it = std::find_if(list.begin(), list.end(), [](const T& entry) { return entry.isActive; });
  if (it != list.end()) {
    return &*it;
  }
  return nullptr;
}

template <typename T, typename V>
static bool isChanged(const SValueChange<T>& change, const std::unique_ptr<V>& value) {
  if (change.isChanged && !value) {
    // no previous value to compare to
    return true;
  }
  if (value && change.isUpdated(value->currentValue)) {
    // value is different than previous value
    return true;
  }
  return false;
}

static void setNodeProperty(xmlNodePtr node, const char* name, const std::string& value) {
  xmlSetProp(node, reinterpret_cast<const xmlChar*>(name),
             reinterpret_cast<const xmlChar*>(value.data()));
}

static void setNodeProperty(xmlNodePtr node, const char* name, bool value) {
  const auto* tmp = value ? "true" : "false";
  xmlSetProp(node, reinterpret_cast<const xmlChar*>(name), reinterpret_cast<const xmlChar*>(tmp));
}

template <typename T>
static void setNodeProperty(xmlNodePtr node, const char* name, T value) {
  auto tmp = std::to_string(value);
  xmlSetProp(node, reinterpret_cast<const xmlChar*>(name),
             reinterpret_cast<const xmlChar*>(tmp.data()));
}

template <typename Func = void (*)(xmlNodePtr)>
static std::string composeActionEvent(int actionType, const std::string& uuid,
                                      Func&& setProperties) {
  CXmlDocument doc{xmlNewDoc(reinterpret_cast<const xmlChar*>("1.0"))};
  auto root = doc.createRoot("ActionEvent");
  setNodeProperty(root, "uuid", uuid);
  setNodeProperty(root, "actionType", actionType);
  setNodeProperty(root, "version", std::string{"9.0"});
  setProperties(root);

  xmlChar* buffer = nullptr;
  int numChars = 0;
  xmlDocDumpMemory(doc.getDocument(), &buffer, &numChars);
  std::string result(reinterpret_cast<const char*>(buffer));
  xmlFree(buffer);
  return result;
}

// For AudioScene XML version 9.0, we only have the objects for the current preset and on
// AudioScene level. For version 10.0 there is no audio objects and switch groups in AudioScene
// level, but entries for all presets on Preset level.

static const std::vector<SAudioElement>& selectAudioElements(
    const SPreset& preset, const SAudioSceneConfig& asi) noexcept {
  if (preset.isActive && preset.audioElements.empty()) {
    return asi.audioElements;
  }
  return preset.audioElements;
}

static const std::vector<SAudioElementSwitch>& selectSwitchGroups(
    const SPreset& preset, const SAudioSceneConfig& asi) noexcept {
  if (preset.isActive && preset.switchGroups.empty()) {
    return asi.switchGroups;
  }
  return preset.switchGroups;
}

std::vector<std::string> composeActionEvents(const SAudioSceneChanges& sceneChanges,
                                             const SAudioSceneConfig* baseAsi,
                                             const std::string* baseDisplayLanguageCode) {
  std::vector<std::string> result;

  if (sceneChanges.displayLanguage.isChanged &&
      (!baseDisplayLanguageCode ||
       sceneChanges.displayLanguage.isUpdated(*baseDisplayLanguageCode))) {
    result.push_back(
        composeActionEvent(71 /* INTERFACE_LANGUAGE_SELECTED */, NO_UUID, [&](xmlNodePtr node) {
          setNodeProperty(node, "paramText", sceneChanges.displayLanguage.newValue);
          setNodeProperty(node, "paramInt", 0 /* priority */);
        }));
  }

  if (!baseAsi) {
    // cannot generate any other ActionEvent without a valid scene UUID
    return result;
  }

  for (const auto& presetChanges : sceneChanges.presets) {
    const auto& basePreset = assertForId(baseAsi->presets, presetChanges.id);

    if (presetChanges.isActive.newValue) {
      const auto* previousActivePreset = baseAsi ? findActive(baseAsi->presets) : nullptr;
      if (!previousActivePreset || previousActivePreset->id != presetChanges.id) {
        result.push_back(composeActionEvent(
            30 /* PRESET_SELECTED */, sceneChanges.uuid,
            [&](xmlNodePtr node) { setNodeProperty(node, "paramInt", presetChanges.id); }));
      }
    }

    for (const auto& elementChanges : presetChanges.audioElements) {
      if (!elementChanges.prominence.isChanged && !elementChanges.muting.isChanged &&
          !elementChanges.azimuth.isChanged && !elementChanges.elevation.isChanged) {
        continue;
      }
      const auto& baseElement =
          assertForId(selectAudioElements(basePreset, *baseAsi), elementChanges.id);

      if (isChanged(elementChanges.prominence, baseElement.prominence)) {
        result.push_back(composeActionEvent(41 /* AUDIO_ELEMENT_PROMINENCE_LEVEL_CHANGED */,
                                            sceneChanges.uuid, [&](xmlNodePtr node) {
                                              setNodeProperty(node, "paramInt", elementChanges.id);
                                              setNodeProperty(node, "paramFloat",
                                                              elementChanges.prominence.newValue);
                                            }));
      }

      if (isChanged(elementChanges.muting, baseElement.muting)) {
        result.push_back(composeActionEvent(
            40 /* AUDIO_ELEMENT_MUTING_CHANGED */, sceneChanges.uuid, [&](xmlNodePtr node) {
              setNodeProperty(node, "paramInt", elementChanges.id);
              setNodeProperty(node, "paramBool", elementChanges.muting.newValue);
            }));
      }

      if (isChanged(elementChanges.azimuth, baseElement.azimuth)) {
        result.push_back(composeActionEvent(
            42 /* ELEMENT_AZIMUTH_CHANGED */, sceneChanges.uuid, [&](xmlNodePtr node) {
              setNodeProperty(node, "paramInt", elementChanges.id);
              setNodeProperty(node, "paramFloat", elementChanges.azimuth.newValue);
            }));
      }

      if (isChanged(elementChanges.elevation, baseElement.elevation)) {
        result.push_back(composeActionEvent(
            43 /* AUDIO_ELEMENT_ELEVATION_CHANGED */, sceneChanges.uuid, [&](xmlNodePtr node) {
              setNodeProperty(node, "paramInt", elementChanges.id);
              setNodeProperty(node, "paramFloat", elementChanges.elevation.newValue);
            }));
      }
    }

    for (const auto& groupChanges : presetChanges.switchGroups) {
      if (!groupChanges.activeObject.isChanged && !groupChanges.muting.isChanged) {
        continue;
      }
      const auto& baseGroup =
          assertForId(selectSwitchGroups(basePreset, *baseAsi), groupChanges.id);

      if (groupChanges.activeObject.isChanged) {
        const auto* activeItem = findActive(baseGroup.audioElements);
        if (!activeItem || groupChanges.activeObject.isUpdated(activeItem->id)) {
          result.push_back(composeActionEvent(
              60 /* AUDIO_ELEMENT_SWITCH_SELECTED */, sceneChanges.uuid, [&](xmlNodePtr node) {
                setNodeProperty(node, "paramInt", groupChanges.id);
                setNodeProperty(node, "paramFloat", groupChanges.activeObject.newValue);
              }));
        }
      }

      if (isChanged(groupChanges.muting, baseGroup.muting)) {
        result.push_back(composeActionEvent(
            61 /* AUDIO_ELEMENT_SWITCH_MUTING_CHANGED */, sceneChanges.uuid, [&](xmlNodePtr node) {
              setNodeProperty(node, "paramInt", groupChanges.id);
              setNodeProperty(node, "paramBool", groupChanges.muting.newValue);
            }));
      }

      for (const auto& elementChanges : groupChanges.audioElements) {
        if (!elementChanges.prominence.isChanged && !elementChanges.azimuth.isChanged &&
            !elementChanges.elevation.isChanged) {
          continue;
        }

        // Currently there is no way of signaling muting for audio elements in switch groups,
        // therefore muting changes are not listed here.

        if (isChanged(elementChanges.prominence, baseGroup.prominence)) {
          result.push_back(composeActionEvent(
              62 /* AUDIO_ELEMENT_SWITCH_PROMINENCE_LEVEL_CHANGED */, sceneChanges.uuid,
              [&](xmlNodePtr node) {
                setNodeProperty(node, "paramInt", groupChanges.id);
                setNodeProperty(node, "paramFloat", elementChanges.prominence.newValue);
              }));
        }

        if (isChanged(elementChanges.azimuth, baseGroup.azimuth)) {
          result.push_back(composeActionEvent(63 /* AUDIO_ELEMENT_SWITCH_AZIMUTH_CHANGED */,
                                              sceneChanges.uuid, [&](xmlNodePtr node) {
                                                setNodeProperty(node, "paramInt", groupChanges.id);
                                                setNodeProperty(node, "paramFloat",
                                                                elementChanges.azimuth.newValue);
                                              }));
        }

        if (isChanged(elementChanges.elevation, baseGroup.elevation)) {
          result.push_back(composeActionEvent(64 /* AUDIO_ELEMENT_SWITCH_ELEVATION_CHANGED */,
                                              sceneChanges.uuid, [&](xmlNodePtr node) {
                                                setNodeProperty(node, "paramInt", groupChanges.id);
                                                setNodeProperty(node, "paramFloat",
                                                                elementChanges.elevation.newValue);
                                              }));
        }
      }
    }
  }

  return result;
}

}  // namespace mpeghuitranslator
