#ifndef OBSERVATION_PLAN_H
#define OBSERVATION_PLAN_H

#include "star_map.h"
#include <string>
#include <vector>

struct observation_session
{
    observer_data anchor{};
    double min_altitude = 30;
    bool above_horizon_only = true;
    std::string selected_target;
    std::vector<std::string> targets;
};

// The local format is versioned CSV. A failed load leaves session unchanged.
bool save_observation_plan(const std::string &path, const observation_session &session,
                           std::string &error);
bool load_observation_plan(const std::string &path, const Star_map_data &catalog,
                           observation_session &session, std::string &error);
// Export geometric estimates sampled every 15 minutes, not visibility promises.
bool export_observation_plan_csv(const std::string &path, const observation_session &session,
                                 const Star_map_data &catalog, std::string &error);

#endif
