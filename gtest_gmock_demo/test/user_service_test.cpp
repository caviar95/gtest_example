#include "mock_database.h"
#include "user_service.h"

#include <gtest/gtest.h>

TEST(UserServiceTest, RegisterSuccess) {
    MockDatabase mock_db;
    UserService service(&mock_db);

    EXPECT_CALL(mock_db, save_user("alice", 30)).Times(1).WillOnce(::testing::Return(true));

    EXPECT_TRUE(service.register_user("alice", 30));
}

TEST(UserServiceTest, RegisterUserFail_EmptyName) {
    MockDatabase mock_db;
    UserService service(&mock_db);

    EXPECT_CALL(mock_db, save_user(::testing::_, ::testing::_)).Times(0);

    EXPECT_FALSE(service.register_user("", 25));
}

TEST(UserServiceTest, RegisterUserFail_NegativeAge) {
    MockDatabase mock_db;

    UserService service(&mock_db);

    EXPECT_CALL(mock_db, save_user(::testing::_, ::testing::_)).Times(0);

    EXPECT_FALSE(service.register_user("bob", -10));
}