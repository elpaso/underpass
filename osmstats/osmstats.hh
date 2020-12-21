//
// Copyright (c) 2020, Humanitarian OpenStreetMap Team
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//

#ifndef __OSMSTATS_HH__
#define __OSMSTATS_HH__

// This is generated by autoconf
#ifdef HAVE_CONFIG_H
# include "hotconfig.h"
#endif

#include <string>
#include <vector>
#include <array>
#include <memory>
#include <iostream>
#include <algorithm>
#include <pqxx/pqxx>

#include <boost/date_time.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
using namespace boost::posix_time;
using namespace boost::gregorian;

// #include "hotosm.hh"
#include "osmstats/changeset.hh"
#include "osmstats/osmchange.hh"
#include "data/geoutil.hh"
#include "timer.hh"

using namespace apidb;

// Forward declarations
namespace changeset {
  class ChangeSet;
};
namespace geoutil {
  class GeoUtil;
};

/// \namespace osmstats
namespace osmstats {

/// \file osmstats.hh
/// \brief This file is used to work with the OSM Stats database
///
/// This manages the OSM Stats schema in a postgres database. This
/// includes querying existing data in the database, as well as
/// updating the database.

/// \class RawChangeset
/// \brief This is the data structure for a raw changeset
///
/// The raw_changesets table contains all the calculated statistics
/// for a change. This stores the data as parsed from the database.
class RawChangeset
{
public:
    RawChangeset(pqxx::const_result_iterator &res);
    RawChangeset(const std::string filespec);
    void dump(void);
// protected:
    // These are from the OSM Stats 'raw_changesets' table
    std::map<std::string, long> counters;
    long id;                    ///< The change ID
    std::string editor;         ///< The editor used for this change
    long user_id;               ///< The user ID 
    ptime created_at;           ///< The starting timestamp
    ptime closed_at;            ///< The finished timestamp
    bool verified;              ///< Whether this change has been validated
    std::vector<long> augmented_diffs; ///< The diffs, currently unused
    ptime updated_at;                  ///< Time this change was updated
    //
    long updateCounter(const std::string &key, long value) {
        counters[key] = value;
        // FIXME: this should return a real value
        return 0;
    };
    long updateCounters(std::map<const std::string &, long> data);
    long operator[](const std::string &key) { return counters[key]; };
};

/// \class RawCountry
/// \brief Stores the data from the raw countries table
///
/// The raw_countries table is used to correlate a country ID with
/// with it's name. This stores the data as parsed from the database.
class RawCountry
{
  public:
    RawCountry(void);
    /// Instantiate the Country data from an iterator
    RawCountry(pqxx::const_result_iterator &res) {
        id = res[0].as(int(0));
        name = res[1].c_str();
        abbrev = res[2].c_str();
    }
    /// Instantiate the Country data
    RawCountry(int cid, const std::string &country, const std::string &code) {
        id = cid;
        name = country;
        abbrev = code;
    }

    int id;                     ///< The Country ID column
    std::string name;           ///< The Country name column
    std::string abbrev;         ///< The 3 letter ISO abbreviation for the country
};

/// \class RawUser
/// \brief Stores the data from the raw user table
///
/// The raw_user table is used to coorelate a user ID with their name.
/// This stores the data as parsed from the database.
class RawUser
{
  public:
    RawUser(void) { };
    /// Instantiate the user data from an iterator
    RawUser(pqxx::const_result_iterator &res) {
        id = res[0].as(int(0));
        name = res[1].c_str();
    }
    /// Instantiate the user data
    RawUser(long uid, const std::string &tag) {
        id = uid;
        name = tag;
    }
    int id;                     ///< The users OSM ID
    std::string name;           ///< The users OSM username 
};

/// \class RawHashtag
/// \brief Stores the data from the raw hashtag table
///
/// The raw_hashtag table is used to coorelate a hashtag ID with the
/// hashtag name. This stores the data as parsed from the database.
class RawHashtag
{
  public:
    RawHashtag(void) { };
    /// Instantiate the hashtag data from an iterator
    RawHashtag(pqxx::const_result_iterator &res) {
        id = res[0].as(int(0));
        name = res[1].c_str();
    }
    /// Instantiate the hashtag data
    RawHashtag(int hid, const std::string &tag) {
        id = hid;
        name = tag;
    }
    int id = 0;                 ///< The hashtag ID
    std::string name;           ///< The hashtag value
};

/// \class QueryOSMStats
/// \brief This handles all direct database access
///
/// This class handles all the queries to the OSM Stats database.
/// This includes querying the database for existing data, as
/// well as updating the data whenh applying a replication file.
class QueryOSMStats : public apidb::QueryStats
{
  public:
    QueryOSMStats(void);
    QueryOSMStats(const std::string &dbname) { connect(dbname); };
    /// close the database connection
    ~QueryOSMStats(void) { disconnect(); };
    void disconnect(void) { sdb->close(); };

    bool readGeoBoundaries(const std::string &rawfile) {
        return false;
    };

    /// Connect to the database
    bool connect(const std::string &database);

    /// Populate internal storage of a few heavily used data, namely
    /// the indexes for each user, country, or hashtag.
    bool populate(void);

    /// Read changeset data from the osmstats database
    bool getRawChangeSets(std::vector<long> &changeset_id);

    /// Add a user to the internal data store
    int addUser(long id, const std::string &user) {
        RawUser ru(id, user);
        users.push_back(ru);
        return users.size();
    };
    /// Add a country to the internal data store
    int addCountry(long id, const std::string &name, const std::string &code) {
        RawCountry rc(id, name, code);
        countries.push_back(rc);
        return countries.size();
    };
    /// Add a hashtag to the internal data store
    int addHashtag(int id, const std::string &tag) {
        RawHashtag rh(id, tag);
        hashtags[tag] = rh;
        return hashtags.size();
    };

    /// Add a comment and their ID to the database
    int addComment(long id, const std::string &user);

    /// Write the list of hashtags to the database
    int updateRawHashtags(void);

    /// Populate the raw_country database from the data file of
    /// of boundaries
    int updateCountries(std::vector<RawCountry> &countries);

    // Accessor classes to extract country data from the database

    /// Get the country ID. name, and ISO abbreviation from the
    // raw_countries table.
    RawCountry &getCountryData(long id) { return countries[id]; }
    RawUser &getUserData(long id) { return users[id]; }
    // RawHashtag &getHashtag(long id) { return hashtags.find(id); }
    long getHashtagID(const std::string name) { return hashtags[name].id; }

    /// Apply a change to the database
    bool applyChange(changeset::ChangeSet &change);
    bool applyChange(osmchange::OsmChange &change);

    int lookupHashtag(const std::string &hashtag);

    RawChangeset &operator[](int index){ return ostats[index]; }

    /// Dump internal data, debugging usage only!
    void dump(void);

    // Get the timestamp of the last update in the database
    ptime getLastUpdate(void);
//private:
    bool updateCounters(long cid, std::map<std::string, long> data);
    bool updateChangeset(const RawChangeset &stats);

    // Subqueries take too much time, it's faster to query the data field we
    // need and update it.
    long getRoadsAdded(long uid) { return queryData(uid, "roads_added"); };
    long getRoadsModified(long uid) { return queryData(uid, "roads_modified"); };
    long getRoadsKMAdded(long uid) { return queryData(uid, "roads_km_added"); };
    long getRoadsKMModified(long uid) { return queryData(uid, "roads_km_modified"); };

    long getWaterwaysAdded(long uid) { return queryData(uid, "waterways_added"); };
    long getWaterwaysModified(long uid) { return queryData(uid, "waterways_modified"); };
    long getWaterwaysKMAdded(long uid) { return queryData(uid, "waterways_km_added"); };
    long getWaterwaysKMModified(long uid) { return queryData(uid, "waterewsys_km_modified"); };

    long getBuildingsAdded(long uid) { return queryData(uid, "buildings_added"); };
    long getBuildingsModified(long uid)  { return queryData(uid, "buildings_modifed"); };
    long getPOIsAdded(long uid);
    long getPOIsModified(long uid);

    long queryData(long cid, const std::string &column) {
        std::string query = "SELECT " + column + " FROM raw_changesets";
        query += " WHERE id=" + std::to_string(cid);
        std::cout << "QUERY: " << query << std::endl;
        pqxx::work worker(*sdb);
        pqxx::result result = worker.exec(query);
        worker.commit();

        // FIXME: this should return a real value
        return 0;
    }

    long updateData(long uid, const std::string &column, long value) {
        std::string query = "UPDATE raw_changesets SET " + column + "=";
        query += std::to_string(value) + " WHERE id=" + std::to_string(uid);
        std::cout << "QUERY: " << query << std::endl;
        pqxx::work worker(*sdb);
        pqxx::result result = worker.exec(query);
        worker.commit();

        // FIXME: this should return a real value
        return 0;
    }

    std::shared_ptr<pqxx::connection> sdb;
    std::vector<RawChangeset> ostats;  ///< All the raw changset data
    std::vector<RawCountry> countries; ///< All the raw country data
    std::vector<RawUser> users;        ///< All the raw user data
    std::map<std::string, RawHashtag> hashtags;
};

    
}       // EOF osmstatsdb

#endif  // EOF __OSMSTATS_HH__
