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

#include "json/forwards.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <string>
#include <vector>

extern const std::string TOP_LEVEL_PROPERTY;

template <typename T>
struct COptionalValue {
  T value = 0;
  bool isSet = false;

  COptionalValue() = default;
  constexpr explicit COptionalValue(T val) noexcept : value(val), isSet(true) {}

  constexpr explicit operator bool() const noexcept { return isSet; }

  T& operator=(T val) noexcept {
    isSet = true;
    return value = val;
  }

  friend std::ostream& operator<<(std::ostream& os, const COptionalValue& val) {
    if (val.isSet) {
      return os << val.value;
    }
    return os << "(not set)";
  }
};

struct SProperty {
  std::string name;
  std::string type;
  std::string format;
  std::string itemType;
  bool required;
  COptionalValue<std::size_t> minLength{};
  COptionalValue<std::size_t> maxLength{};
  COptionalValue<int64_t> minValue{};
  COptionalValue<int64_t> maxValue{};

  friend std::ostream& operator<<(std::ostream& os, const SProperty& prop);
};

/*!
 * Container for a JSON Schema specification to facilitate validation of JSON data.
 *
 * NOTE: This type does not represent the full JSON Schema specification, but what we use!
 */
struct SSchema {
  std::string id;
  std::string type;
  std::vector<SProperty> properties;

  friend std::ostream& operator<<(std::ostream& os, const SSchema& schema);
};

struct SSchemas {
  std::map<std::string, SSchema> schemas;
  /*! The root JSON Schema, the entry point of the JSON values to validate. */
  const SSchema& root;

  /*!
   * Validate the given root JSON value with the JSON Schemas contained in this object.
   */
  bool validate(const Json::Value& value) const;
};

SSchemas parseSchemas(const std::vector<std::string>& files);
SSchemas parseSchemas(int num_args, char** args);

/*!
 * Helper function to read a single JSON file into a JSON value.
 */
Json::Value readJsonFile(const std::string& file);

/*!
 * Helper function to pretty-print the given JSON value into the given output stream.
 */
void printJson(const Json::Value& value, std::ostream& os);
