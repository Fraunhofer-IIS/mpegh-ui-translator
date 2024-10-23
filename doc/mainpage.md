# Main Page {#mainpage}

## Description

The MPEG-H UI Translator is a library capable of translating MPEG-H UI AudioScene and ActionEvent objects between XML and JSON representation.

The library can convert [MPEG-H UI manager AudioScene XML](https://github.com/Fraunhofer-IIS/mpeghdec/wiki/MPEG-H-UI-manager-XML-format) to a JSON representation in the proposed JSON format for application standards (such as the TV 3.0 standard in Brazil) as specified in the `json_schema/` project folder as well as ActionEvents in the same JSON format back to [MPEG-H UI manager ActionEvent XML](https://github.com/Fraunhofer-IIS/mpeghdec/wiki/MPEG-H-UI-manager-XML-format).

## Minimal Code Example

The following lines provide a simple example showing how to convert a MPEG-H UI manager AudioScene XML to JSON, update some values and convert it back to MPEG-H UI manager ActionEvents XML.

```{.cpp}
#include "mpeghuitranslator/translator.h"
#include "json/value.h"

using namespace mpeghuitranslator;

...

// e.g. retrieve from the MPEG-H UI manager
std::string audioSceneXml = "";

// initialize the translator for the English language
CUiTranslator translator{"eng"};

// convert the AudioScene to JSON
auto audioSceneJson = translator.mpeghInteractivityToJson(audioSceneXml);

// update some values in the audio scene JSON, e.g. change the active Preset
audioSceneJson["audioPresets"][0]["active"] = true;
audioSceneJson["audioPresets"][1]["active"] = true;

// convert the JSON back to a list of ActionEvent XML strings for the differences to the
// translator's internal state
auto actionEvents = translator.mpeghInteractivityToXml(audioSceneJson);

// do something with the ActionEvent XML strings, e.g. send them to the MPEG-H UI manager...
...

```
