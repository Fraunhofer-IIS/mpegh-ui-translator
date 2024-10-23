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

// Prevent Windows Debug builds from warning on possible unsafe behavior when copying from/to raw
// buffers
#define _SCL_SECURE_NO_WARNINGS
// Internal headers
#include "mpeghuitranslator/mpeghuitranslator_c.h"
#include "mpeghuitranslator/simple.h"
#include "mpeghuitranslator/translator.h"
#include "audio_scene.h"
#include "scene_changes.h"
#include "xml_helper.h"

// External headers
#include "json/json.h"

// System headers
#include <algorithm>
#include <cstdlib>
#include <memory>
#include <mutex>

namespace mpeghuitranslator {

////
// Object-oriented public interface (translator.h)
////

struct SUiTranslatorPimpl {
  explicit SUiTranslatorPimpl(SIso639Code initialDisplayLanguageCodeHint)
      : displayLanguageHint(initialDisplayLanguageCodeHint) {}

  std::mutex lock;
  SIso639Code displayLanguageHint;
  std::unique_ptr<SAudioSceneConfig> lastAudioScene;
};

CUiTranslator::CUiTranslator(const std::string& initialDisplayLanguageCodeHint)
    : m_pimpl(new SUiTranslatorPimpl(initialDisplayLanguageCodeHint)) {}

CUiTranslator::~CUiTranslator() noexcept = default;

Json::Value CUiTranslator::mpeghInteractivityToJson(const std::string& audioSceneXml) {
  if (!m_pimpl) {
    m_pimpl.reset(new SUiTranslatorPimpl(""));
  }

  SXmlString xml{audioSceneXml};
  auto asi = parseAudioScene(xml.getRoot());

  std::lock_guard<std::mutex> guard{m_pimpl->lock};
  m_pimpl->lastAudioScene.reset(new SAudioSceneConfig(std::move(asi)));
  return composeAudioScene(*m_pimpl->lastAudioScene, m_pimpl->displayLanguageHint);
}

std::vector<std::string> CUiTranslator::mpeghInteractivityToXml(
    const Json::Value& sceneChangesJson) {
  if (!m_pimpl) {
    m_pimpl.reset(new SUiTranslatorPimpl(""));
  }

  auto changes = parseAudioSceneChanges(sceneChangesJson);

  std::lock_guard<std::mutex> guard{m_pimpl->lock};
  auto result =
      composeActionEvents(changes, m_pimpl->lastAudioScene.get(), &m_pimpl->displayLanguageHint);

  if (changes.displayLanguage.isChanged) {
    m_pimpl->displayLanguageHint = changes.displayLanguage.newValue;
  }
  return result;
}

////
// Global-state public interface (simple.h)
////

static std::mutex GLOBAL_LOCK{};
static std::string GLOBAL_DISPLAY_LANGUAGE = "eng";
static std::unique_ptr<SAudioSceneConfig> GLOBAL_CONFIG = nullptr;
static std::string GLOBAL_LAST_EXCEPTION = "";

Json::Value mpeghInteractivityToJson(const std::string& audioSceneXml) {
  SXmlString xml{audioSceneXml};
  auto asi = parseAudioScene(xml.getRoot());

  std::lock_guard<std::mutex> guard{GLOBAL_LOCK};
  GLOBAL_CONFIG.reset(new SAudioSceneConfig(std::move(asi)));
  return composeAudioScene(*GLOBAL_CONFIG, GLOBAL_DISPLAY_LANGUAGE);
}

std::vector<std::string> mpeghInteractivityToXml(const Json::Value& sceneChangesJson) {
  auto changes = parseAudioSceneChanges(sceneChangesJson);

  std::lock_guard<std::mutex> guard{GLOBAL_LOCK};
  auto result = composeActionEvents(changes, GLOBAL_CONFIG.get(), &GLOBAL_DISPLAY_LANGUAGE);
  if (changes.displayLanguage.isChanged) {
    GLOBAL_DISPLAY_LANGUAGE = changes.displayLanguage.newValue;
  }

  return result;
}

}  // namespace mpeghuitranslator

////
// Public C interface (mpeghuitranslator_c.h)
////

MpeghUiTranslatorStatusCode mpeghUiTranslatorToJson(const char* audioSceneXml,
                                                    size_t audioSceneXmlSize, char* outJsonBuffer,
                                                    size_t* outJsonBufferSize) try {
  if (audioSceneXml == nullptr || audioSceneXmlSize == 0 || outJsonBufferSize == nullptr) {
    return MPEGHUITRANSLATOR_INVALID_ARGUMENT;
  }

  Json::StreamWriterBuilder builder{};
  auto json = Json::writeString(builder, mpeghuitranslator::mpeghInteractivityToJson(std::string(
                                             audioSceneXml, audioSceneXml + audioSceneXmlSize)));

  if (*outJsonBufferSize < json.size()) {
    *outJsonBufferSize = json.size();
    return MPEGHUITRANSLATOR_INSUFFICIENT_SPACE;
  } else if (outJsonBuffer == nullptr) {
    return MPEGHUITRANSLATOR_INVALID_ARGUMENT;
  }

  std::copy(json.begin(), json.end(), outJsonBuffer);
  *outJsonBufferSize = json.size();
  return MPEGHUITRANSLATOR_OK;

} catch (const std::exception& err) {
  std::lock_guard<std::mutex> guard{mpeghuitranslator::GLOBAL_LOCK};
  mpeghuitranslator::GLOBAL_LAST_EXCEPTION = err.what();
  return MPEGHUITRANSLATOR_INTERNAL_ERROR;
}

MpeghUiTranslatorStatusCode mpeghUiTranslatorToXml(
    const char* sceneChangesJson, size_t sceneChangesJsonSize,
    MpeghUiTranslatorStringList* outActionScenes) try {
  if (sceneChangesJson == nullptr || sceneChangesJsonSize == 0 || outActionScenes == nullptr) {
    return MPEGHUITRANSLATOR_INVALID_ARGUMENT;
  }

  std::unique_ptr<Json::CharReader> jsonReader{Json::CharReaderBuilder{}.newCharReader()};
  Json::Value json{};
  if (!jsonReader->parse(sceneChangesJson, sceneChangesJson + sceneChangesJsonSize, &json,
                         nullptr)) {
    return MPEGHUITRANSLATOR_INVALID_ARGUMENT;
  }

  std::string oldDisplayLanguage;
  {
    std::lock_guard<std::mutex> guard{mpeghuitranslator::GLOBAL_LOCK};
    oldDisplayLanguage = mpeghuitranslator::GLOBAL_DISPLAY_LANGUAGE;
  }

  auto events = mpeghuitranslator::mpeghInteractivityToXml(json);

  if (outActionScenes->numStrings > 0 && outActionScenes->numStrings < events.size()) {
    std::lock_guard<std::mutex> guard{mpeghuitranslator::GLOBAL_LOCK};
    mpeghuitranslator::GLOBAL_DISPLAY_LANGUAGE = oldDisplayLanguage;
    outActionScenes->numStrings = events.size();
    return MPEGHUITRANSLATOR_INSUFFICIENT_SPACE;
  } else if (outActionScenes->numStrings && !outActionScenes->strings) {
    std::lock_guard<std::mutex> guard{mpeghuitranslator::GLOBAL_LOCK};
    mpeghuitranslator::GLOBAL_DISPLAY_LANGUAGE = oldDisplayLanguage;
    return MPEGHUITRANSLATOR_INVALID_ARGUMENT;
  } else if (outActionScenes->numStrings == 0) {
    outActionScenes->strings = reinterpret_cast<char**>(malloc(events.size() * sizeof(char*)));
    outActionScenes->numStrings = events.size();
  }

  outActionScenes->numStrings = events.size();
  for (std::size_t i = 0; i < events.size(); ++i) {
    outActionScenes->strings[i] = reinterpret_cast<char*>(malloc(events[i].size() * sizeof(char)));
    std::copy(events[i].begin(), events[i].end(), outActionScenes->strings[i]);
  }
  return MPEGHUITRANSLATOR_OK;

} catch (const std::exception& err) {
  std::lock_guard<std::mutex> guard{mpeghuitranslator::GLOBAL_LOCK};
  mpeghuitranslator::GLOBAL_LAST_EXCEPTION = err.what();
  return MPEGHUITRANSLATOR_INTERNAL_ERROR;
}

void mpeghUiTranslatorFreeStrings(MpeghUiTranslatorStringList* list) {
  if (!list || !list->strings) {
    return;
  }

  for (std::size_t i = 0; i < list->numStrings; ++i) {
    free(list->strings[i]);
  }

  free(list->strings);
  list->strings = nullptr;
  list->numStrings = 0;
}

const char* mpeghUiTranslatorLastError(MpeghUiTranslatorStatusCode code) {
  switch (code) {
    case MPEGHUITRANSLATOR_INSUFFICIENT_SPACE:
      return "Insufficient space in output parameter";
    case MPEGHUITRANSLATOR_INVALID_ARGUMENT:
      return "Invalid argument";
    case MPEGHUITRANSLATOR_INTERNAL_ERROR: {
      std::lock_guard<std::mutex> guard{mpeghuitranslator::GLOBAL_LOCK};
      return mpeghuitranslator::GLOBAL_LAST_EXCEPTION.data();
    }
    case MPEGHUITRANSLATOR_OK:
    default:
      return nullptr;
  }
}
