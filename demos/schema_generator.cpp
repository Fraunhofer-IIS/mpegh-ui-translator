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

#include "schema_validator_helper.h"

#include "json/value.h"

#include <cstdlib>
#include <limits>
#include <map>
#include <random>
#include <string>

static std::size_t SEED = []() {
  std::random_device dev;
  return dev();
}();

static bool generateRandomBool() {
  static std::default_random_engine rng{static_cast<std::default_random_engine::result_type>(SEED)};
  static std::uniform_int_distribution<uint16_t> dist{0, 1};
  return dist(rng) == 1U;
}

static float generateRandomNumber() {
  static std::default_random_engine rng{static_cast<std::default_random_engine::result_type>(SEED)};
  static std::uniform_real_distribution<float> dist{-128.0f, 128.0f};
  return dist(rng);
}

static int64_t generateRandomInteger(int64_t minValue = 0,
                                     int64_t maxValue = std::numeric_limits<int64_t>::max()) {
  static std::default_random_engine rng{static_cast<std::default_random_engine::result_type>(SEED)};
  static std::uniform_real_distribution<double> dist{0.0, 1.0f};

  auto valueRange = static_cast<double>(std::max(minValue, maxValue)) -
                    static_cast<double>(std::min(minValue, maxValue));
  auto relativeValue = dist(rng) * valueRange;
  return std::min(minValue, maxValue) + static_cast<int64_t>(std::round(relativeValue));
}

static char generateRandomLetter() {
  static std::default_random_engine rng{static_cast<std::default_random_engine::result_type>(SEED)};
  // only generate lower case letters
  static std::uniform_int_distribution<short> dist{'a', 'z'};
  return static_cast<char>(dist(rng));
}

static Json::Value generateRandomMembers(const SSchema& schema,
                                         const std::map<std::string, SSchema>& schemas);

static std::size_t generateLength(const COptionalValue<std::size_t>& minValue,
                                  const COptionalValue<std::size_t>& maxValue) {
  if (!minValue && !maxValue) {
    // arbitrary upper bound to not have too long lists/strings
    return static_cast<std::size_t>(generateRandomInteger(0, 4));
  } else if (minValue.value == maxValue.value) {
    return minValue.value;
  }
  return static_cast<std::size_t>(generateRandomInteger(static_cast<int64_t>(minValue.value),
                                                        static_cast<int64_t>(maxValue.value)));
}

static Json::Value generateRandomValue(const SProperty& property,
                                       const std::map<std::string, SSchema>& schemas) {
  if (property.type == "array" && property.format.empty() && !property.itemType.empty()) {
    Json::Value array{Json::arrayValue};
    auto itemProp = property;
    itemProp.type = property.itemType;
    auto numItems = generateLength(property.minLength, property.maxLength);
    for (std::size_t i = 0; i < numItems; ++i) {
      array.append(generateRandomValue(itemProp, schemas));
    }
    return array;
  } else if (property.type == "boolean" && property.format.empty()) {
    return Json::Value{generateRandomBool()};
  } else if (property.type == "integer" && property.format.empty()) {
    if (property.minValue || property.maxValue) {
      return Json::Value{generateRandomInteger(property.minValue.value, property.maxValue.value)};
    } else {
      return Json::Value{generateRandomInteger()};
    }
  } else if (property.type == "number" && property.format.empty() && !property.minLength &&
             !property.maxLength) {
    return Json::Value{generateRandomNumber()};
  } else if (property.type == "string" && property.format == "uuid") {
    // random value
    return Json::Value{"123e4567-e89b-12d3-a456-426614174000"};
  } else if (property.type == "string" && property.format.empty()) {
    auto length = generateLength(property.minLength, property.maxLength);
    std::string tmp(length, '\0');
    for (auto& c : tmp) {
      c = generateRandomLetter();
    }
    return Json::Value{tmp};
  } else if (schemas.find(property.type) != schemas.end()) {
    return generateRandomMembers(schemas.at(property.type), schemas);
  } else {
    std::cerr << "Unhandled property: " << property << std::endl;
    return {};
  }
}

static Json::Value generateRandomMembers(const SSchema& schema,
                                         const std::map<std::string, SSchema>& schemas) {
  Json::Value object{};
  if (schema.properties.size() == 1 && schema.properties.front().name == TOP_LEVEL_PROPERTY) {
    return generateRandomValue(schema.properties.front(), schemas);
  }
  for (const auto& prop : schema.properties) {
    if (!prop.required && !generateRandomBool()) {
      // randomly skip non-required properties
      continue;
    }
    object[prop.name] = generateRandomValue(prop, schemas);
  }
  return object;
}

int printHelpAndExit() {
  std::cerr << "Usage: <program> [--seed <numerical seed>] <schema files>..." << std::endl;
  return EXIT_FAILURE;
}

int main(int argc, char** argv) {
  if (argc < 2) {
    return printHelpAndExit();
  }

  int skipArgs = 1;

  if (std::string{"--seed"} == argv[1]) {
    if (argc < 4) {
      return printHelpAndExit();
    }
    SEED = std::strtoull(argv[2], nullptr, 0);
    skipArgs += 2;
  }

  auto schemas = parseSchemas(argc - skipArgs, argv + skipArgs);

  auto root = generateRandomMembers(schemas.root, schemas.schemas);
  printJson(root, std::cout);
  return EXIT_SUCCESS;
}
