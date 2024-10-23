
add_executable(xml_to_json xml_to_json.cpp)
target_link_libraries(xml_to_json mpeghuitranslator)

add_executable(json_to_xml json_to_xml.cpp)
target_link_libraries(json_to_xml mpeghuitranslator)

add_executable(schema_generator
  schema_generator.cpp
  schema_validator_helper.cpp
)
target_include_directories(schema_generator PRIVATE .)
target_link_libraries(schema_generator jsoncpp_static)

add_executable(schema_validator
  schema_validator.cpp
  schema_validator_helper.cpp
)
target_include_directories(schema_validator PRIVATE .)
target_link_libraries(schema_validator jsoncpp_static)
