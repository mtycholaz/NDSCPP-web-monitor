#include <gtest/gtest.h>
#include <cpr/cpr.h> // Modern C++ HTTP library
#include <../json.hpp>

// Best guess for Ubuntu:
// sudo apt-get install libgtest-dev libcpr-dev nlohmann-json3-dev
// g++ -o api_tests api_tests.cpp -lgtest -lgtest_main -lcpr -pthread
//
// For MacOS
//
// brew install googletest
// brew install cpr

using json = nlohmann::json;
const std::string BASE_URL = "http://localhost:7777/api";

class APITest : public ::testing::Test {
protected:
    void SetUp() override {
        // Clean up any existing test data
        cleanupTestData();
    }

    void TearDown() override {
        // Clean up after tests
        cleanupTestData();
    }

    void cleanupTestData() {
        // Get all canvases and delete them
        auto response = cpr::Get(cpr::Url{BASE_URL + "/canvases"});
        if (response.status_code == 200) {
            auto canvases = json::parse(response.text);
            for (const auto& canvas : canvases) {
                cpr::Delete(cpr::Url{BASE_URL + "/canvases/" + std::to_string(canvas["id"].get<int>())});
            }
        }
    }
};

// Test Controller endpoint
TEST_F(APITest, GetController) {
    auto response = cpr::Get(cpr::Url{BASE_URL + "/controller"});
    ASSERT_EQ(response.status_code, 200);
    
    auto data = json::parse(response.text);
    ASSERT_TRUE(data.contains("controller"));
}

// Test Sockets endpoints
TEST_F(APITest, GetSockets) {
    auto response = cpr::Get(cpr::Url{BASE_URL + "/sockets"});
    ASSERT_EQ(response.status_code, 200);
    
    auto sockets = json::parse(response.text);
    ASSERT_TRUE(sockets.is_array());
}

TEST_F(APITest, GetSpecificSocket) {
    // First get all sockets
    auto response = cpr::Get(cpr::Url{BASE_URL + "/sockets"});
    auto sockets = json::parse(response.text);
    
    if (!sockets.empty()) {
        int firstSocketId = sockets[0]["id"].get<int>();
        auto socketResponse = cpr::Get(cpr::Url{BASE_URL + "/sockets/" + std::to_string(firstSocketId)});
        ASSERT_EQ(socketResponse.status_code, 200);
        
        auto socketData = json::parse(socketResponse.text);
        ASSERT_EQ(socketData["id"].get<int>(), firstSocketId);
    }
}

/*
// Test Canvas CRUD operations
TEST_F(APITest, CanvasCRUD) {
    // Create canvas
    json canvasData = {
        {"id", 1},
        {"name", "Test Canvas"},
        {"width", 100},
        {"height", 100}
    };
    
    auto createResponse = cpr::Post(
        cpr::Url{BASE_URL + "/canvases"},
        cpr::Body{canvasData.dump()},
        cpr::Header{{"Content-Type", "application/json"}}
    );
    ASSERT_EQ(createResponse.status_code, 201);

    // Read all canvases
    auto listResponse = cpr::Get(cpr::Url{BASE_URL + "/canvases"});
    ASSERT_EQ(listResponse.status_code, 200);
    auto canvases = json::parse(listResponse.text);
    ASSERT_FALSE(canvases.empty());

    // Read specific canvas
    auto getResponse = cpr::Get(cpr::Url{BASE_URL + "/canvases/1"});
    ASSERT_EQ(getResponse.status_code, 200);
    auto canvas = json::parse(getResponse.text);
    ASSERT_EQ(canvas["name"], "Test Canvas");

    // Delete canvas
    auto deleteResponse = cpr::Delete(cpr::Url{BASE_URL + "/canvases/1"});
    ASSERT_EQ(deleteResponse.status_code, 200);

    // Verify deletion
    auto verifyResponse = cpr::Get(cpr::Url{BASE_URL + "/canvases/1"});
    ASSERT_EQ(verifyResponse.status_code, 404);
}

// Test Feature operations within a canvas
TEST_F(APITest, CanvasFeatureOperations) {
    // First create a canvas
    json canvasData = {
        {"id", 1},
        {"name", "Feature Test Canvas"},
        {"width", 100},
        {"height", 100}
    };
    
    cpr::Post(
        cpr::Url{BASE_URL + "/canvases"},
        cpr::Body{canvasData.dump()},
        cpr::Header{{"Content-Type", "application/json"}}
    );

    // Create feature
    json featureData = {
        {"type", "LED"},
        {"x", 50},
        {"y", 50},
        {"color", "#FF0000"}
    };
    
    auto createFeatureResponse = cpr::Post(
        cpr::Url{BASE_URL + "/canvases/1/features"},
        cpr::Body{featureData.dump()},
        cpr::Header{{"Content-Type", "application/json"}}
    );
    ASSERT_EQ(createFeatureResponse.status_code, 200);
    
    auto featureInfo = json::parse(createFeatureResponse.text);
    int featureId = featureInfo["id"].get<int>();

    // Delete feature
    auto deleteFeatureResponse = cpr::Delete(
        cpr::Url{BASE_URL + "/canvases/1/features/" + std::to_string(featureId)}
    );
    ASSERT_EQ(deleteFeatureResponse.status_code, 200);
}

*/

// Test error cases
TEST_F(APITest, ErrorHandling) {
    // Test invalid canvas ID
    auto invalidCanvasResponse = cpr::Get(cpr::Url{BASE_URL + "/canvases/999"});
    ASSERT_EQ(invalidCanvasResponse.status_code, 404);

    // Test invalid JSON in canvas creation
    auto invalidJsonResponse = cpr::Post(
        cpr::Url{BASE_URL + "/canvases"},
        cpr::Body{"invalid json"},
        cpr::Header{{"Content-Type", "application/json"}}
    );
    ASSERT_EQ(invalidJsonResponse.status_code, 400);
}
