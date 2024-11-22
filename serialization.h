#pragma once

// Serialization
//
// To be able to create a JSON representation of the LEDFeature object, we need to provide
// a to_json function that converts the object into a JSON object.  This function is then
// used by the nlohmann::json library to serialize the object.

#include <nlohmann/json.hpp>
#include "interfaces.h"

// Note that in theory, for proper separation of concerns, these functions should NOT need
// access to headers that are not part of the interfaces - i.e., everything we need should
// be on an interface

// Our Canvas class has a vector of shared pointers to ILEDFeature objects.  When we serialize
// the Canvas object, we want to serialize the ILEDFeature objects as well.  To do this, we
// need to provide a to_json function that converts the shared pointer to an ILEDFeature object
// into a JSON object.  This function is then used by the nlohmann::json library to serialize
// the shared pointer.


// to_json for serialization


void to_json(nlohmann::json& j, const std::shared_ptr<ILEDFeature>& feature) 
{
    if (feature) 
    {
        // Manually serialize fields from the ILEDFeature interface
        j = nlohmann::json{
            { "hostName",     feature->HostName()     },
            { "friendlyName", feature->FriendlyName() },
            { "width",        feature->Width()        },
            { "height",       feature->Height()       },
            { "offsetX",      feature->OffsetX()      },
            { "offsetY",      feature->OffsetY()      },
            { "reversed",     feature->Reversed()     },
            { "channel",      feature->Channel()      },
            { "redGreenSwap", feature->RedGreenSwap() },
            { "batchSize",    feature->BatchSize()    }
        };
    } 
    else 
    {
        j = nullptr; // Handle null shared pointers
    }
}


void to_json(nlohmann::json& j, const ICanvas& canvas)
{
    // Serialize the features vector as an array of JSON objects
    std::vector<nlohmann::json> featuresJson;
    for (const auto& feature : canvas.Features())
    {
        if (feature) 
        {
            nlohmann::json featureJson;
            to_json(featureJson, feature); // Call the shared_ptr version
            featuresJson.push_back(std::move(featureJson));
        } 
        else 
        {
            featuresJson.push_back(nullptr); // Handle null pointers
        }
    }

    j = nlohmann::json{
        { "width",    canvas.Graphics().Width()   },
        { "height",   canvas.Graphics().Height()  },
        { "features", featuresJson                }
    };
}

