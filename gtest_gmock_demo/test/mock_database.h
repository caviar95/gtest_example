#pragma once


#include "database.h"

#include <gmock/gmock.h>

class MockDatabase : public IDatabase {
public:
    MOCK_METHOD(bool, connect, (const std::string& db_uri), (override));
    MOCK_METHOD(bool, save_user, (const std::string& username, int age), (override));
};

