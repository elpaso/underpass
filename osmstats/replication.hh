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

#ifndef __REPLICATION_HH__
#define __REPLICATION_HH__

// This is generated by autoconf
#ifdef HAVE_CONFIG_H
#include "unconfig.h"
#endif

#include <filesystem>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
//#include <pqxx/pqxx>
#include <cstdlib>
#include <gumbo.h>
#include <mutex>

#include <osmium/builder/osm_object_builder.hpp>
#include <osmium/handler.hpp>
#include <osmium/io/any_input.hpp>
#include <osmium/io/any_output.hpp>
#include <osmium/visitor.hpp>

#include "boost/date_time/posix_time/posix_time.hpp"
#include <boost/date_time.hpp>
using namespace boost::posix_time;
using namespace boost::gregorian;
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl/stream.hpp>
namespace ssl = boost::asio::ssl; // from <boost/asio/ssl.hpp>

#include "data/threads.hh"
#include "osmstats/changeset.hh"
using namespace changeset;
#include "timer.hh"
// #include "osmstats/osmstats.hh"

namespace net = boost::asio;      // from <boost/asio.hpp>
namespace ssl = boost::asio::ssl; // from <boost/asio/ssl.hpp>
using tcp = net::ip::tcp;         // from <boost/asio/ip/tcp.hpp>

/// \namespace replication
namespace replication {

/// \file replication.hh
/// \brief This class downloads replication files, and adds them to the database
///
/// Replication files are used to update the existing data with changes. There
/// are 3 different intervals, minute, hourly, or daily replication files
/// are available.
/// Data on the directory structure and timestamps of replication files is
/// cached on disk, to shorten the lookup time on program startup.

// OsmChange file structure on planet
// https://planet.openstreetmap.org/replication/day
//    dir/dir/???.osc.gz
//    dir/dir/???.state.txt
// https://planet.openstreetmap.org/replication/hour
//    dir/dir/???.osc.gz
//    dir/dir/???.state.txt
// https://planet.openstreetmap.org/replication/minute/
//    dir/dir/???.osc.gz
//    dir/dir/???.state.txt
//
// Changeset file structure on planet
// https://planet.openstreetmap.org/replication/changesets/
//    dir/dir/???.osm.gz

typedef enum { minutely, hourly, daily, changeset } frequency_t;

/// \class StateFile
/// \brief Data structure for state.text files
///
/// This contains the data in a ???.state.txt file, used to identify the timestamp
/// of the changeset replication file. The replication file uses the same
/// 3 digit number as the state file.
class StateFile
{
  public:
    StateFile(void)
    {
        //timestamp = boost::posix_time::second_clock::local_time();
        timestamp = boost::posix_time::not_a_date_time;
        created_at = boost::posix_time::not_a_date_time;
        closed_at = boost::posix_time::not_a_date_time;
        sequence = 0;
    };

    /// Initialize with a state file from disk or memory
    StateFile(const std::string &file, bool memory);
    StateFile(const std::vector<unsigned char> &data)
        : StateFile(reinterpret_cast<const char *>(data.data()), true){};

    /// Dump internal data to the terminal, used only for debugging
    void dump(void);

    /// Get the first numerical directory
    long getMajor(void)
    {
        std::vector<std::string> result;
        boost::split(result, path, boost::is_any_of("/"));
        return std::stol(result[4]);
    };
    long getMinor(void)
    {
        std::vector<std::string> result;
        boost::split(result, path, boost::is_any_of("/"));
        return std::stol(result[5]);
    };
    long getIndex(void)
    {
        std::vector<std::string> result;
        boost::split(result, path, boost::is_any_of("/"));
        return std::stol(result[6]);
    };

    // protected so testcases can access private data
    //protected:
    std::string path; ///< URL to this file
    ptime timestamp;  ///< The timestamp of the associated changeset file
    long sequence;    ///< The sequence number of the associated changeset file
    std::string frequency; ///< The time interval of this change file
    /// These two values are updated after the changset is parsed
    ptime created_at; ///< The first timestamp in the changefile
    ptime closed_at;  ///< The last timestamp in the changefile
};

/// \class RemoteURL
/// \brief This parses a remote URL into pieces
class RemoteURL
{
  public:
    RemoteURL(void);
    RemoteURL(const RemoteURL &inr);
    RemoteURL(const std::string &rurl) { parse(rurl); }
    /// Parse a URL into it's elements
    void parse(const std::string &rurl);
    std::string domain;
    std::string datadir;
    std::string subpath;
    frequency_t frequency;
    std::string base;
    std::string url;
    int major;
    int minor;
    int index;
    std::string filespec;
    std::string destdir;
    void dump(void);
    void Increment(void);
    RemoteURL &operator=(const RemoteURL &inr);
};

/// \class Planet
/// \brief This stores file paths and timestamps from planet.
class Planet
{
  public:
    Planet(void);
    // Planet(const std::string &planet) { pserver = planet; };
    Planet(const RemoteURL &url)
    {
        remote = url;
        connectServer(url.domain);
    };
    // Planet(const std::string &planet, const std::string &dir) {
    //     pserver = planet;
    //     datadir = dir;
    //     frequency = replication::minutely;
    //     connectServer();
    // };
    // Planet(const std::string &planet, const std::string &dir, frequency_t freq) {
    //     pserver = planet;
    //     datadir = dir;
    //     frequency = freq;
    //     connectServer();
    // };
    ~Planet(void);

    bool connectServer(void) { return connectServer(remote.domain); }
    bool connectServer(const std::string &server);
    bool disconnectServer(void)
    {
        // db->disconnect();           // close the database connection
        // db->close();                // close the database connection
        ioc.reset();       // reset the I/O conhtext
        stream.shutdown(); // shutdown the socket used by the stream
        // FIXME: this should return a real value
        return false;
    }

    std::shared_ptr<std::vector<unsigned char>>
    downloadFile(const std::string &file);

    /// Since the data files don't have a consistent time interval, this
    /// attempts to do a rough calculation of the probably data file,
    /// and downloads *.state.txt files till the right data file is found.
    /// Note that this can be slow as it has to download multiple files.
    std::shared_ptr<StateFile> findData(frequency_t freq, long sequence);
    std::shared_ptr<StateFile> findData(frequency_t freq, ptime starttime);
    std::shared_ptr<StateFile> findData(frequency_t freq,
                                        const std::string &path);

    /// Dump internal data to the terminal, used only for debugging
    void dump(void);

    /// Scan remote directory from planet
    std::shared_ptr<std::vector<std::string>>
    scanDirectory(const std::string &dir);

    /// Extract the links in an HTML document. This is used
    /// to find the directories on planet for replication files
    std::shared_ptr<std::vector<std::string>> &
    getLinks(GumboNode *node, std::shared_ptr<std::vector<std::string>> &links);

    // private:
    RemoteURL remote;
    //    std::string pserver;        ///< The replication file server
    //    std::string datadir;        ///< Default top level path to the data files
    //    replication::frequency_t frequency;
    int port = 443;   ///< Network port on the server, note SSL only allowed
    int version = 11; ///< HTTP version
    std::map<ptime, std::string> minute;
    std::map<ptime, std::string> changeset;
    std::map<ptime, std::string> hour;
    std::map<ptime, std::string> day;
    std::map<frequency_t, std::map<ptime, std::string>> states;
    std::map<frequency_t, std::string> frequency_tags;
    // These are for the boost::asio data stream
    boost::asio::io_context ioc;
    ssl::context ctx{ssl::context::sslv23_client};
    tcp::resolver resolver{ioc};
    boost::asio::ssl::stream<tcp::socket> stream{ioc, ctx};
    //    std::string baseurl;        ///< URL for the planet server
};

// These are the columns in the pgsnapshot.replication_changes table
// id | tstamp | nodes_modified | nodes_added | nodes_deleted | ways_modified | ways_added | ways_deleted | relations_modified | relations_added | relations_deleted | changesets_applied | earliest_timestamp | latest_timestamp

/// \class Replication
/// \brief Handle replication files from the OSM planet server.
///
/// This class handles identifying the right replication file to download,
/// and downloading it.
class Replication
{
  public:
    Replication(void)
    {
        last_run = boost::posix_time::second_clock::local_time();
        sequence = 0;
        port = 443;
        version = 11; ///! HTTP version
    };
    // Downloading a replication requires either a sequence
    // number or a starting timestamp
    Replication(ptime last, long seq) : Replication()
    {
        if (!last.is_not_a_date_time()) {
            last_run = last;
        }
        if (seq > 0) {
            sequence = seq;
        }
    }

    Replication(ptime last) { last_run = last; };
    Replication(long seq) { sequence = seq; };

    /// parse a replication file containing changesets
    bool readChanges(const std::string &file);

    /// Add this replication data to the changeset database
    bool mergeToDB();

    std::vector<StateFile> states; ///< Stored state.txt files
    const ChangeSetFile &getChangeset() const;

  private:
    std::string pserver;     ///< The replication file server
    int port;                ///< Network port for the server
    ptime last_run;          ///< Timestamp of the replication file
    long sequence;           ///< Sequence number of the replication
    int version;             ///< Version number of the replication
    ChangeSetFile changeset; ///< Changeset read by readChanges()
};

struct membuf : std::streambuf {
    membuf(char *base, std::ptrdiff_t n) { this->setg(base, base, base + n); }
};

} // namespace replication

#endif // EOF __REPLICATION_HH__
