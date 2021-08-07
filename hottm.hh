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

#ifndef __HOTTM_HH__
#define __HOTTM_HH__

// This is generated by autoconf
#ifdef HAVE_CONFIG_H
#include "unconfig.h"
#endif

#include <array>
#include <iostream>
#include <memory>
#include <pqxx/pqxx>
#include <string>
#include <vector>

#include "boost/date_time/posix_time/posix_time.hpp"
#include <boost/date_time.hpp>
using namespace boost::posix_time;
using namespace boost::gregorian;

#include "hottm/tmdefs.hh"
#include "hottm/tmprojects.hh"
#include "hottm/tmteams.hh"
#include "hottm/tmusers.hh"

namespace tmdb {

class TaskingManager {
  public:
    bool connect(const std::string &database);

    /**
     * @brief Retrieve the users from TM DB.
     * @param userId, optional user id, default value of 0 synchronizes all
     * users.
     * @return a vector of TMUser objects.
     */
    std::vector<TMUser> getUsers(TaskingManagerIdType userId = 0);

    std::vector<TMTeam>
    getTeams(void) {
        return getTeams(0);
    };

    std::vector<TMTeam> getTeams(long teamid);

    std::vector<TaskingManagerIdType> getTeamMembers(long teamid, bool active);

    std::vector<TMProject>
    getProjects(void) {
        return getProjects(0);
    };

    std::vector<TaskingManagerIdType> getProjectTeams(long projectid);

    std::vector<TMProject> getProjects(TaskingManagerIdType projectid);

    /**
     * @brief getWorker
     * @return the (possibly NULL) pointer to the worker, mainly for testing
     * purposes.
     */
    pqxx::work *getWorker() const;

  private:
    std::unique_ptr<pqxx::connection> db;
    std::unique_ptr<pqxx::work> worker;
};

} // namespace tmdb

#endif // EOF __HOTTM_HH__
