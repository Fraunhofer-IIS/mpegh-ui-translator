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
#include "schema_validator_helper.h"

// External headers
#include "json/reader.h"
#include "json/value.h"
#include "json/writer.h"

// System headers
#include <algorithm>
#include <cctype>
#include <fstream>
#include <set>
#include <stdexcept>

const std::string TOP_LEVEL_PROPERTY = "";
static constexpr COptionalValue<std::size_t> UUID_LENGTH{36};

std::ostream& operator<<(std::ostream& os, const SProperty& prop) {
  os << prop.name << " {";
  if (prop.type == "array" && !prop.itemType.empty()) {
    os << prop.itemType << "[]";
  } else {
    os << prop.type;
  }
  if (!prop.format.empty()) {
    os << ", " << prop.format;
  }
  if (prop.required) {
    os << ", required";
  }
  if (prop.minLength && prop.maxLength && prop.minLength.value == prop.maxLength.value) {
    os << ", len==" << prop.minLength;
  } else {
    if (prop.minLength) {
      os << ", len>=" << prop.minLength;
    }
    if (prop.maxLength) {
      os << ", len<=" << prop.maxLength;
    }
  }
  if (prop.minValue) {
    os << ", min=" << prop.minValue;
  }
  if (prop.maxValue) {
    os << ", max=" << prop.maxValue;
  }
  os << "}";
  return os;
}

std::ostream& operator<<(std::ostream& os, const SSchema& schema) {
  os << "Schema: " << schema.id << " (" << schema.type << "):";
  for (const auto& prop : schema.properties) {
    os << ' ' << prop << ',';
  }
  return os;
}

static SProperty extractProperty(const Json::Value& json, const std::string& file) {
  SProperty property{};
  if (json["$ref"].isString()) {
    property.type = json["$ref"].asString();
  } else if (json["type"].isString()) {
    property.type = json["type"].asString();
  } else {
    throw std::runtime_error{"Invalid 'type' for property in schema file: " + file};
  }
  if (json["format"].isString()) {
    property.format = json["format"].asString();
  }
  if (json["items"]["$ref"].isString()) {
    property.itemType = json["items"]["$ref"].asString();
  } else if (json["items"]["type"].isString()) {
    property.itemType = json["items"]["type"].asString();
  }
  if (json["minLength"].isUInt()) {
    property.minLength = json["minLength"].asUInt();
  } else if (json["minimum"].isUInt()) {
    property.minValue = json["minimum"].asInt();
  }
  if (json["maxLength"].isUInt()) {
    property.maxLength = json["maxLength"].asUInt();
  } else if (json["maximum"].isUInt()) {
    property.maxValue = json["maximum"].asInt();
  }
  return property;
}

static SSchema extractSchema(const Json::Value& json, const std::string& file) {
  SSchema schema{};
  if (json["$id"].isString()) {
    schema.id = json["$id"].asString();
  } else {
    throw std::runtime_error{"Invalid '$id' for schema file: " + file};
  }
  if (json["type"].isString()) {
    schema.type = json["type"].asString();
  } else {
    throw std::runtime_error{"Invalid 'type' for schema file: " + file};
  }

  if (!json.isMember("properties")) {
    // no properties, assume top-level attributes
    auto property = extractProperty(json, file);
    property.name = TOP_LEVEL_PROPERTY;
    property.required = true;
    schema.properties.push_back(std::move(property));
  } else {
    const auto& properties = json["properties"];
    if (!properties.isObject()) {
      throw std::runtime_error{"Invalid 'properties' for schema file: " + file};
    }
    const auto& requiredProperties = json["required"];
    if (!requiredProperties.empty() && !requiredProperties.isArray()) {
      throw std::runtime_error{"Invalid 'required' for schema file: " + file};
    }
    for (const auto& name : properties.getMemberNames()) {
      const auto& prop = properties[name];
      if (!prop.isObject()) {
        throw std::runtime_error{"Invalid property '" + name + "' for schema file: " + file};
      }

      auto property = extractProperty(prop, file);
      property.name = name;
      property.required = std::find(requiredProperties.begin(), requiredProperties.end(), name) !=
                          requiredProperties.end();
      schema.properties.push_back(std::move(property));
    }
  }
  return schema;
}

static const SSchema* findTopLevelSchema(const std::map<std::string, SSchema>& schemas) {
  std::set<std::string> referencedTypes{};
  for (const auto& schema : schemas) {
    for (const auto& prop : schema.second.properties) {
      referencedTypes.emplace(prop.type);
      referencedTypes.emplace(prop.itemType);
    }
  }

  std::set<const SSchema*> candidates{};
  for (const auto& schema : schemas) {
    if (referencedTypes.find(schema.first) == referencedTypes.end()) {
      candidates.emplace(&schema.second);
    }
  }
  if (candidates.empty()) {
    std::cerr << "No top-level JSON schema found!";
    return nullptr;
  }
  if (candidates.size() > 1) {
    std::cerr << "Multiple top-level JSON schemas found: ";
    bool firstEntry = true;
    for (const auto* schema : candidates) {
      if (!firstEntry) {
        std::cerr << ", ";
      }
      std::cerr << schema->id;
      firstEntry = false;
    }
    std::cerr << std::endl;
    return nullptr;
  }
  return *candidates.begin();
}

SSchemas parseSchemas(const std::vector<std::string>& files) {
  std::map<std::string, SSchema> schemas{};
  for (const auto& file : files) {
    auto json = readJsonFile(file);
    auto schema = extractSchema(json, file);
    auto id = schema.id;
    if (!schemas.emplace(id, std::move(schema)).second) {
      throw std::runtime_error{"Duplicate schemas for $id '" + id + "' for schema file: " + file};
    }
  }
  if (const auto* root = findTopLevelSchema(schemas)) {
    return SSchemas{std::move(schemas), *root};
  }
  throw std::runtime_error{"No top-level schema found!"};
}

SSchemas parseSchemas(int num_args, char** args) {
  std::vector<std::string> schemaFiles{};
  schemaFiles.assign(args, args + num_args);

  return parseSchemas(schemaFiles);
}

static bool validateLength(std::size_t value, const COptionalValue<std::size_t>& minValue,
                           const COptionalValue<std::size_t>& maxValue) {
  if (!minValue && !maxValue) {
    return true;
  }
  return (!minValue || value >= minValue.value) && (!maxValue || value <= maxValue.value);
}

static bool validateUuid(const std::string& value) {
  // As defined in RFC4122: hexadecimal values in the layout xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
  // where every x represents a nibble (4 bits).
  for (std::size_t i = 0; i < value.size(); ++i) {
    if (i == 8 || i == 13 || i == 18 || i == 23) {
      if (value[i] != '-') {
        return false;
      }
    } else if (!std::isxdigit(static_cast<unsigned char>(value[i]))) {
      return false;
    }
  }
  return true;
}

static bool validateSchema(const Json::Value& value, const SSchema& schema,
                           const std::map<std::string, SSchema>& schemas);

static bool validateProperty(const Json::Value& value, const SProperty& property,
                             const std::map<std::string, SSchema>& schemas) {
  if (property.type == "array" && property.format.empty() && !property.itemType.empty()) {
    if (!value.isArray()) {
      std::cerr << "JSON value for array property is not a JSON array (" << property
                << "): " << value << std::endl;
      return false;
    }
    if (!validateLength(value.size(), property.minLength, property.maxLength)) {
      std::cerr << "JSON array size of " << value.size() << "' exceeds bounds: " << property
                << std::endl;
      return false;
    }
    auto itemProp = property;
    itemProp.type = property.itemType;
    for (const auto& item : value) {
      if (!validateProperty(item, itemProp, schemas)) {
        return false;
      }
    }
    return true;
  } else if (property.type == "boolean" && property.format.empty()) {
    if (!value.isBool()) {
      std::cerr << "JSON value for bool property is not a JSON bool (" << property << "): " << value
                << std::endl;
      return false;
    }
    return true;
  } else if (property.type == "integer" && property.format.empty()) {
    if (!value.isInt()) {
      std::cerr << "JSON value for integer property is not a JSON integer (" << property
                << "): " << value << std::endl;
      return false;
    }
    if ((property.minValue && value.asInt64() < property.minValue.value) ||
        (property.maxValue && value.asInt64() > property.maxValue.value)) {
      std::cerr << "JSON integer exceeds bounds (" << property << "): " << value << std::endl;
      return false;
    }
    return true;
  } else if (property.type == "number" && property.format.empty() && !property.minLength &&
             !property.maxLength) {
    if (!value.isNumeric()) {
      std::cerr << "JSON value for number property is not a JSON number (" << property
                << "): " << value << std::endl;
      return false;
    }
    return true;
  } else if (property.type == "string" && property.format == "uuid") {
    if (!value.isString()) {
      std::cerr << "JSON value for string property is not a JSON string (" << property
                << "): " << value << std::endl;
      return false;
    }
    if (!validateLength(value.asString().size(), UUID_LENGTH, UUID_LENGTH)) {
      std::cerr << "JSON string length of " << value.asString().size() << "' exceeds bounds ("
                << property << "): " << value << std::endl;
      return false;
    }
    if (!validateUuid(value.asString())) {
      std::cerr << "JSON string is not a valid UUID (" << property << "): " << value << std::endl;
      return false;
    }
    return true;
  } else if (property.type == "string" && property.format.empty()) {
    if (!value.isString()) {
      std::cerr << "JSON value for string property is not a JSON string (" << property
                << "): " << value << std::endl;
      return false;
    }
    if (!validateLength(value.asString().size(), property.minLength, property.maxLength)) {
      std::cerr << "JSON string length of " << value.asString().size() << "' exceeds bounds ("
                << property << "): " << value << std::endl;
      return false;
    }
    return true;
  } else if (schemas.find(property.type) != schemas.end()) {
    return validateSchema(value, schemas.at(property.type), schemas);
  } else {
    std::cerr << "Unhandled property (" << property << "): " << value << std::endl;
    return false;
  }
}

static bool validateSchema(const Json::Value& value, const SSchema& schema,
                           const std::map<std::string, SSchema>& schemas) {
  if (schema.properties.size() > 1 && !value.isObject()) {
    std::cerr << "JSON for schema '" << schema.id << "' is not an object!" << std::endl;
    return false;
  }

  if (schema.properties.size() == 1 && schema.properties.front().name == TOP_LEVEL_PROPERTY) {
    return validateProperty(value, schema.properties.front(), schemas);
  }

  auto presentMembers = value.getMemberNames();
  for (const auto& prop : schema.properties) {
    auto memberIt = std::find(presentMembers.begin(), presentMembers.end(), prop.name);
    if (prop.required && memberIt == presentMembers.end()) {
      std::cerr << "Required member for schema '" << schema.id
                << "' not present in JSON object: " << prop << std::endl;
      return false;
    } else if (memberIt != presentMembers.end()) {
      presentMembers.erase(memberIt);

      if (!validateProperty(value[prop.name], prop, schemas)) {
        return false;
      }
    }
  }

  if (!presentMembers.empty()) {
    std::cerr << "Additional members in JSON object for schema '" << schema.id << "': ";
    for (const auto& member : presentMembers) {
      std::cerr << ' ' << member << ',';
    }
    std::cerr << std::endl;
    return false;
  }
  return true;
}

bool SSchemas::validate(const Json::Value& value) const {
  return validateSchema(value, root, schemas);
}

Json::Value readJsonFile(const std::string& file) {
  std::ifstream fis{file};
  Json::Value value{};
  fis >> value;

  return value;
}

void printJson(const Json::Value& value, std::ostream& os) {
  Json::StreamWriterBuilder builder;
  builder["indentation"] = "  ";
  std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
  writer->write(value, &os);
  os << std::endl;
}
