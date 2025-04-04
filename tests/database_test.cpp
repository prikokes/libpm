#include <gtest/gtest.h>
#include "procmine/database.h"
#include <filesystem>
#include <string>

namespace {
    using namespace procmine;

    std::string create_test_db() {
        std::string db_path = "test_database.db";

        if (std::filesystem::exists(db_path)) {
            std::filesystem::remove(db_path);
        }

        Database db(db_path);
        db.execute("CREATE TABLE test_table (id INTEGER PRIMARY KEY, name TEXT, value REAL)");

        db.execute("INSERT INTO test_table (name, value) VALUES ('item1', 10.5)");
        db.execute("INSERT INTO test_table (name, value) VALUES ('item2', 20.3)");
        db.execute("INSERT INTO test_table (name, value) VALUES ('item3', 30.7)");
        
        return db_path;
    }
}

TEST(DatabaseTest, BasicOperations) {
    std::string db_path = create_test_db();

    Database db(db_path);

    auto result = db.query("SELECT * FROM test_table ORDER BY id");

    EXPECT_EQ(result->get_row_count(), 3);
    EXPECT_EQ(result->get_column_count(), 3);

    auto column_names = result->get_column_names();
    EXPECT_EQ(column_names.size(), 3);
    EXPECT_EQ(column_names[0], "id");
    EXPECT_EQ(column_names[1], "name");
    EXPECT_EQ(column_names[2], "value");

    EXPECT_EQ(result->get_int(0, "id"), 1);
    EXPECT_EQ(result->get_string(0, "name"), "item1");
    EXPECT_DOUBLE_EQ(result->get_double(0, "value"), 10.5);
    
    EXPECT_EQ(result->get_int(1, "id"), 2);
    EXPECT_EQ(result->get_string(1, "name"), "item2");
    EXPECT_DOUBLE_EQ(result->get_double(1, "value"), 20.3);
    
    EXPECT_EQ(result->get_int(2, "id"), 3);
    EXPECT_EQ(result->get_string(2, "name"), "item3");
    EXPECT_DOUBLE_EQ(result->get_double(2, "value"), 30.7);

    std::filesystem::remove(db_path);
}

TEST(DatabaseTest, PreparedStatement) {
    std::string db_path = create_test_db();

    Database db(db_path);

    auto stmt = db.prepare("SELECT * FROM test_table WHERE value > ?");

    stmt->bind(1, 20.0);
    auto result = stmt->query();

    EXPECT_EQ(result->get_row_count(), 2);
    stmt->bind(1, 25.0);
    result = stmt->query();

    EXPECT_EQ(result->get_row_count(), 1);
    EXPECT_EQ(result->get_string(0, "name"), "item3");

    std::filesystem::remove(db_path);
}

TEST(DatabaseTest, Transactions) {
    std::string db_path = create_test_db();

    Database db(db_path);

    EXPECT_TRUE(db.begin_transaction());

    EXPECT_TRUE(db.execute("INSERT INTO test_table (name, value) VALUES ('item4', 40.1)"));

    auto result = db.query("SELECT COUNT(*) FROM test_table");
    EXPECT_EQ(result->get_int(0, 0), 4);

    EXPECT_TRUE(db.rollback());

    result = db.query("SELECT COUNT(*) FROM test_table");
    EXPECT_EQ(result->get_int(0, 0), 3);

    EXPECT_TRUE(db.begin_transaction());

    EXPECT_TRUE(db.execute("INSERT INTO test_table (name, value) VALUES ('item4', 40.1)"));

    EXPECT_TRUE(db.commit());

    result = db.query("SELECT COUNT(*) FROM test_table");
    EXPECT_EQ(result->get_int(0, 0), 4);

    std::filesystem::remove(db_path);
} 