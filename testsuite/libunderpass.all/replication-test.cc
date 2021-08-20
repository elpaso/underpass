//
// Copyright (c) 2020, 2021 Humanitarian OpenStreetMap Team
//
// This file is part of Underpass.
//
//     Underpass is free software: you can redistribute it and/or modify
//     it under the terms of the GNU General Public License as published by
//     the Free Software Foundation, either version 3 of the License, or
//     (at your option) any later version.
//
//     Underpass is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU General Public License for more details.
//
//     You should have received a copy of the GNU General Public License
//     along with Underpass.  If not, see <https://www.gnu.org/licenses/>.
//

#include <dejagnu.h>
#include <iostream>
#include <pqxx/pqxx>
#include <string>

#include "hottm.hh"
#include "log.hh"
#include "osmstats/replication.hh"

#include "boost/date_time/gregorian/gregorian.hpp"
#include "boost/date_time/posix_time/posix_time.hpp"
#include "boost/filesystem/path.hpp"
#include <boost/date_time.hpp>

using namespace replication;
using namespace boost::posix_time;
using namespace boost::gregorian;
using namespace boost::filesystem;

TestState runtest;

class TestReplication : public Replication
{
  public:
    //! Clear the test DB and fill it with with initial test data
    bool init_test_case()
    {

        const std::string dbconn{getenv("UNDERPASS_TEST_DB_CONN")
                                     ? getenv("UNDERPASS_TEST_DB_CONN")
                                     : ""};
        source_tree_root = getenv("UNDERPASS_SOURCE_TREE_ROOT")
                               ? getenv("UNDERPASS_SOURCE_TREE_ROOT")
                               : ".";

        const std::string test_replication_db_name{"replication_test"};

        {
            pqxx::connection conn{dbconn};
            pqxx::nontransaction worker{conn};
            worker.exec0("DROP DATABASE IF EXISTS " + test_replication_db_name);
            worker.exec0("CREATE DATABASE " + test_replication_db_name);
            worker.commit();
        }

        {
            pqxx::connection conn{dbconn +
                                  " dbname=" + test_replication_db_name};
            pqxx::nontransaction worker{conn};
            worker.exec0("CREATE EXTENSION postgis");
            worker.exec0("CREATE EXTENSION hstore");

            // Create schema
            const path base_path{source_tree_root / "testsuite"};
            const auto schema_path{base_path / "testdata" /
                                   "replication_schema.sql"};
            std::ifstream schema_definition(schema_path);
            std::string sql((std::istreambuf_iterator<char>(schema_definition)),
                            std::istreambuf_iterator<char>());

            assert(!sql.empty());
            worker.exec0(sql);

            // Load a minimal data set for testing
            const auto data_path{base_path / "testdata" /
                                 "replication_test_data.sql"};
            std::ifstream data_definition(data_path);
            std::string data_sql(
                (std::istreambuf_iterator<char>(data_definition)),
                std::istreambuf_iterator<char>());

            assert(!data_sql.empty());
            worker.exec0(data_sql);
        }

        return true;
    };

    std::string source_tree_root;
};

int
main(int argc, char *argv[])
{

    // Test preconditions
    TestReplication testreplication;

    logger::LogFile &dbglogfile = logger::LogFile::getDefaultInstance();
    dbglogfile.setLogFilename("");
    dbglogfile.setVerbosity();

    testreplication.init_test_case();

    const std::string test_replication_db_name{"replication_test"};
    std::string replication_conn;
    if (getenv("PGHOST") && getenv("PGUSER") && getenv("PGPASSWORD")) {
        replication_conn = std::string(getenv("PGUSER")) + ":" +
                           std::string(getenv("PGPASSWORD")) + "@" +
                           std::string(getenv("PGHOST")) + "/" +
                           test_replication_db_name;
    } else {
        replication_conn = test_replication_db_name;
    }

    const path base_path{testreplication.source_tree_root / "testsuite"};
    const auto diff_path{base_path / "testdata" / "pbf" /
                         "haiti-and-domrep-2021-08-14" / "064.osc.gz"};

    const auto osm_changes{
        testreplication.readOsmChangeFile(diff_path.c_str())};
    if (osm_changes.changes.size() > 0) {
        runtest.pass("Replication::readOsmChangeFile()");
    } else {
        runtest.fail("Replication::readOsmChangeFile()");
        exit(1);
    }
}
