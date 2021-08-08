//
// Copyright (c) 2020, 2021 Humanitarian OpenStreetMap Team
//
// This file is part of Underpass.
//
//     Pq is free software: you can redistribute it and/or modify
//     it under the terms of the GNU General Public License as published by
//     the Free Software Foundation, either version 3 of the License, or
//     (at your option) any later version.
//
//     Pq is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU General Public License for more details.
//
//     You should have received a copy of the GNU General Public License
//     along with Pq.  If not, see <https://www.gnu.org/licenses/>.
//

// This is generated by autoconf
#ifdef HAVE_CONFIG_H
# include "unconfig.h"
#endif

#include <string>
#include <vector>
#include <iostream>
#include "data/pq.hh"
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string.hpp>

#include "log.hh"
using namespace logger;

namespace pq {

Pq::Pq(void)
{
    // Validate environment variable is defined.
    char *tmp;
    tmp = std::getenv("PGHOST");
    if (tmp) {
        host = tmp;
    }
    tmp = std::getenv("PGUSER");
    if (tmp) {
        user = tmp;
    }    
    tmp = std::getenv("PGPASSWORD");
    if (tmp) {
        passwd = tmp;
    }
    tmp = std::getenv("PGDATABASE");
    if (tmp) {
        dbname = tmp;
    }
    std::string args;
    if (!user.empty()) {
        args += user;
    }
    if (!passwd.empty()) {
        args += ":" + passwd;
    }
    if (!host.empty()) {
        if (!user.empty()) {
            args += "@" + host;
        } else {
            args += host;
        }
    }
    if (!dbname.empty()) {
        log_error(_("Must supply a database name!"));
        return;
    }
    
    connect(args);
};

Pq::Pq(const std::string &args)
{
    connect(args);
};

Pq::~Pq(void)
{
    // db->disconnect();        // close the database connection
    if (sdb) {
        if (sdb->is_open()) {
            sdb->close();            // close the database connection
        }
    }
}

// Dump internal data to the terminal, used only for debugging
void
Pq::dump(void)
{
    log_debug(_("Database host: %1%"), host);
    log_debug(_("Database name: %1%"), dbname);
    log_debug(_("Database user: %1%"), user);
    log_debug(_("Database password: %1%"), passwd);
}

bool
Pq::parseURL(const std::string &dburl)
{
    if (dburl.empty()) {
        log_error(_("No database connection string!"));
        return false;
    }

    host.clear();
    user.clear();
    passwd.clear();
    dbname.clear();
    
    std::size_t apos = dburl.find('@');
    if (apos != std::string::npos) {
        user = "user=";
        std::size_t cpos = dburl.find(':');
        if (cpos != std::string::npos) {
            user += dburl.substr(0, cpos);
            passwd = "password=";
            passwd += dburl.substr(cpos+1, apos-cpos-1);
        } else {
            user += dburl.substr(0, apos);
        }
    }
    
    std::vector<std::string> result;
    if (apos != std::string::npos) {
        boost::split(result, dburl.substr(apos+1), boost::is_any_of("/"));
    } else {
        boost::split(result, dburl, boost::is_any_of("/"));
    }
    if (result.size() == 1) {
        dbname = "dbname=" + result[0];
    } else if (result.size() == 2) {
        if (result[0] != "localhost") {
            host = "host=" + result[0];
        }
        dbname = "dbname=" + result[1];
    }

    return true;
}

bool
Pq::connect(const std::string &dburl)
{
    std::string args;

    if (parseURL(dburl)) {
        std::string args = host + " " + dbname + " " + user + " " + passwd;
    } else {
        return false;
    }
    
    // log_debug(args);
    try {
	sdb = std::make_shared<pqxx::connection>(args);
	if (sdb->is_open()) {
            log_debug(_("Opened database connection to %1%"), dburl);
	    return true;
	} else {
	    return false;
	}
    } catch (const std::exception &e) {
	log_error(_(" Couldn't open database connection to %1% %2%"), dburl, e.what());
	return false;
   }    
}

pqxx::result
Pq::query(const std::string &query)
{
    if (sdb) {
        if (!sdb->is_open()) {
            log_error(_("Database isn't open!"));
            result.clear();
            return result;
        }
    }
    pqxx::work worker(*sdb);
    result = worker.exec(query);
    worker.commit();
    return result;
}

} // EOF pq namespace

