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
#include "json/json.h"

// System headers

namespace mpeghuitranslator {

static Json::Value makeEmptyArray() {
  Json::Value out{};
  // make empty array instead of "null" value
  out.resize(0);
  return out;
}

static Json::Value composeLabel(const SLocalizedString& label) {
  Json::Value out{};

  out["lang"] = label.langCode;
  out["value"] = label.value;

  return out;
}

static Json::Value composeProminence(const SProminenceLevelProperty& prominence) {
  Json::Value out{};

  out["level"] = prominence.currentValue;
  out["min"] = prominence.minValue;
  out["max"] = prominence.maxValue;
  out["default"] = prominence.defaultValue;

  return out;
}

static Json::Value composeMuting(const SMutingProperty& muting) {
  Json::Value out{};

  out["value"] = muting.currentValue;
  out["default"] = muting.defaultValue;

  return out;
}

static Json::Value composeAzimuth(const SAzimuthProperty& azimuth) {
  Json::Value out{};

  out["offset"] = azimuth.currentValue;
  out["min"] = azimuth.minValue;
  out["max"] = azimuth.maxValue;
  out["default"] = azimuth.defaultValue;

  return out;
}

static Json::Value composeElevation(const SElevationProperty& elevation) {
  Json::Value out{};

  out["offset"] = elevation.currentValue;
  out["min"] = elevation.minValue;
  out["max"] = elevation.maxValue;
  out["default"] = elevation.defaultValue;

  return out;
}

static Json::Value composeAudioElement(const SAudioElement& element) {
  Json::Value out{};

  out["id"] = element.id;

  auto& labels = out["labels"] = makeEmptyArray();
  if (element.customKind) {
    for (const auto& label : element.customKind->description) {
      labels.append(composeLabel(label));
    }
    if (!element.customKind->langCode.empty()) {
      out["contentLanguage"] = element.customKind->langCode;
    }
  }

  if (element.kind) {
    out["contentKind"] = element.kind->code;
  }
  if (element.prominence) {
    out["prominence"] = composeProminence(*element.prominence);
  }
  if (element.muting) {
    out["muting"] = composeMuting(*element.muting);
  }
  if (element.azimuth) {
    out["azimuth"] = composeAzimuth(*element.azimuth);
  }
  if (element.elevation) {
    out["elevation"] = composeElevation(*element.elevation);
  }

  return out;
}

static Json::Value composeSwitchGroupItem(const SAudioElementSwitchItem& item,
                                          const SAudioElementSwitch& switchGroup) {
  Json::Value out{};

  out["id"] = item.id;

  auto& labels = out["labels"] = makeEmptyArray();
  if (item.customKind) {
    for (const auto& label : item.customKind->description) {
      labels.append(composeLabel(label));
    }
    if (!item.customKind->langCode.empty()) {
      out["contentLanguage"] = item.customKind->langCode;
    }
  }

  if (item.kind) {
    out["contentKind"] = item.kind->code;
  }
  if (switchGroup.prominence) {
    out["prominence"] = composeProminence(*switchGroup.prominence);
  }
  if (switchGroup.muting) {
    out["muting"] = composeMuting(*switchGroup.muting);
  }
  if (switchGroup.azimuth) {
    out["azimuth"] = composeAzimuth(*switchGroup.azimuth);
  }
  if (switchGroup.elevation) {
    out["elevation"] = composeElevation(*switchGroup.elevation);
  }

  return out;
}

static Json::Value composeSwitchGroup(const SAudioElementSwitch& switchGroup) {
  Json::Value out{};

  out["id"] = switchGroup.id;
  auto& labels = out["labels"] = makeEmptyArray();
  if (switchGroup.customKind) {
    for (const auto& label : switchGroup.customKind->description) {
      labels.append(composeLabel(label));
    }
  }

  if (switchGroup.muting) {
    out["muting"] = composeMuting(*switchGroup.muting);
  }

  auto& objects = out["objects"] = makeEmptyArray();
  for (const auto& element : switchGroup.audioElements) {
    if (element.isDefault) {
      out["defaultObject"] = element.id;
    }
    if (element.isActive) {
      out["activeObject"] = element.id;
    }
    objects.append(composeSwitchGroupItem(element, switchGroup));
  }

  return out;
}

static Json::Value composePreset(const SPreset& preset,
                                 const std::vector<SAudioElement>& additionalAudioElements,
                                 const std::vector<SAudioElementSwitch>& additionalSwitchGroups) {
  Json::Value out{};

  out["id"] = preset.id;
  auto& labels = out["labels"] = makeEmptyArray();
  out["contentLanguages"] = makeEmptyArray();
  if (preset.customKind) {
    for (const auto& label : preset.customKind->description) {
      labels.append(composeLabel(label));
    }
  }

  if (preset.kind) {
    out["contentKind"] = preset.kind->code;
  }
  out["default"] = preset.isDefault;
  out["active"] = preset.isActive;

  auto& objects = out["objects"] = makeEmptyArray();
  for (const auto& audioElement : preset.audioElements) {
    objects.append(composeAudioElement(audioElement));
  }
  for (const auto& audioElement : additionalAudioElements) {
    objects.append(composeAudioElement(audioElement));
  }

  auto& switchGroups = out["switchGroups"] = makeEmptyArray();
  for (const auto& switchGroup : preset.switchGroups) {
    switchGroups.append(composeSwitchGroup(switchGroup));
  }
  for (const auto& switchGroup : additionalSwitchGroups) {
    switchGroups.append(composeSwitchGroup(switchGroup));
  }

  return out;
}

Json::Value composeAudioScene(const SAudioSceneConfig& asi,
                              const SIso639Code& displayLanguageHint) {
  Json::Value out{};

  out["uuid"] = asi.uuid;
  out["displayLanguageHint"] = displayLanguageHint;

  auto& presets = out["audioPresets"] = makeEmptyArray();
  for (const auto& preset : asi.presets) {
    // For AudioScene XML version 9.0, we only have the objects for the current preset and on
    // AudioScene level. For version 10.0 there are no audio objects and switch groups on AudioScene
    // level, but entries for all presets on Preset level.
    if (preset.isActive) {
      presets.append(composePreset(preset, asi.audioElements, asi.switchGroups));
    } else {
      presets.append(composePreset(preset, {}, {}));
    }
  }

  return out;
}

}  // namespace mpeghuitranslator
