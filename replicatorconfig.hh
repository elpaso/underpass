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

#ifndef __REPLICATOR_CONFIG_HH__
#define __REPLICATOR_CONFIG_HH__

// This is generated by autoconf
#ifdef HAVE_CONFIG_H
#include "unconfig.h"
#endif

#include "data/underpass.hh"
#include "osmstats/replication.hh"
#include <boost/format.hpp>
#include <string>
using namespace replication;
using namespace underpass;

namespace replicatorconfig {

///
/// \brief The ReplicatorConfig struct stores replicator configuration
///
struct ReplicatorConfig {

    ///
    /// \brief ReplicatorConfig constructor: will try to initialize from uppercased same-name
    ///        environment variables prefixed by REPLICATOR_ (e.g. REPLICATOR_OSMSTATS_DB_URL)
    ///
    ReplicatorConfig()
    {
        if (getenv("REPLICATOR_OSMSTATS_DB_URL")) {
            osmstats_db_url = getenv("REPLICATOR_OSMSTATS_DB_URL");
        }
        if (getenv("REPLICATOR_TASKINGMANAGER_DB_URL")) {
            taskingmanager_db_url = getenv("REPLICATOR_TASKINGMANAGER_DB_URL");
        }
        if (getenv("REPLICATOR_UNDERPASS_DB_URL")) {
            underpass_db_url = getenv("REPLICATOR_UNDERPASS_DB_URL");
        }
        if (getenv("REPLICATOR_OSM2PGSQL_DB_URL")) {
            osm2pgsql_db_url = getenv("REPLICATOR__OSM2PGSQL_DB_URL");
        }
        if (getenv("REPLICATOR_PLANET_SERVER")) {
            planet_server = getenv("REPLICATOR_PLANET_SERVER");
        }
        if (getenv("REPLICATOR_FREQUENCY")) {
            try {
                frequency = Underpass::freq_from_string(getenv("REPLICATOR_FREQUENCY"));
            } catch (const std::invalid_argument &) {
                // Ignore
            }
        }
        if (getenv("REPLICATOR_STARTING_URL_PATH")) {
            starting_url_path = getenv("REPLICATOR_STARTING_URL_PATH");
        }
        if (getenv("REPLICATOR_TASKINGMANAGER_USERS_UPDATE_FREQUENCY")) {
            try {
                const auto tm_freq{std::atoi(getenv("REPLICATOR_TASKINGMANAGER_USERS_UPDATE_FREQUENCY"))};
                if (tm_freq >= -1) {
                    taskingmanager_users_update_frequency = tm_freq;
                }
            } catch (const std::exception &) {
                // Ignore
            }
        }
    };

    std::string underpass_db_url = "localhost/underpass";
    std::string osmstats_db_url = "localhost/osmstats";
    std::string taskingmanager_db_url = "localhost/taskingmanager";
    std::string osm2pgsql_db_url = "";
    std::string planet_server = "https://planet.maps.mail.ru";
    frequency_t frequency = frequency_t::minutely;
    std::string starting_url_path = ""; ///< Starting URL path (e.g. /000/000/001)
    long taskingmanager_users_update_frequency =
        -1; ///< Users synchronization: -1 (disabled), 0 (single shot), > 0 (interval in seconds)

    ///
    /// \brief dbConfigHelp
    /// \return a string with the names of the environment variables of the available configuration options and their current values.
    ///
    std::string dbConfigHelp() const
    {
        return str(format(R"raw(
REPLICATOR_OSMSTATS_DB_URL=%1%
REPLICATOR_UNDERPASS_DB_URL=%2%
REPLICATOR_TASKINGMANAGER_DB_URL=%3%
REPLICATOR_OSM2PGSQL_DB_URL=%3%
REPLICATOR_PLANET_SERVER=%4%
REPLICATOR_FREQUENCY=%5%
REPLICATOR_STARTING_URL_PATH=%6%
REPLICATOR_TASKINGMANAGER_USERS_UPDATE_FREQUENCY=%7%
      )raw") % osmstats_db_url %
                   underpass_db_url % taskingmanager_db_url % osm2pgsql_db_url % planet_server %
                   Underpass::freq_to_string(frequency) % starting_url_path % taskingmanager_users_update_frequency);
    };
};

} // namespace replicatorconfig

#endif // EOF __REPLICATOR_CONFIG_HH__
