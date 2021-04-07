// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include "environment.h"

namespace librealsense
{
    extrinsics_graph::extrinsics_graph()
        : _locks_count(0)
    {
        _id = std::make_shared<lazy<rs2_extrinsics>>([]()
        {
            return identity_matrix();
        });
    }

    extrinsics_graph::extrinsics_lock extrinsics_graph::lock()
    {
        extrinsics_lock l(*this);
        return l;
    }

    void extrinsics_graph::register_same_extrinsics(const stream_interface& from, const stream_interface& to)
    {
        register_extrinsics(from, to, _id);
    }

    // The aim of this method is to add the profile to the graph extrinsics - as a leaf not connected to any other at this stage
    // It is used when a profile is cloned
    // The cloned profile will later on be connected to its owning stream by the method register_extrinsics 
    // (starting from init_stream_profiles method in the device's contructor)
    void extrinsics_graph::register_profile(const stream_interface& profile)
    {
        std::lock_guard<std::mutex> lock(_mutex); 
        
        // First, trim any dead stream, to make sure we are not keep gaining memory
        cleanup_extrinsics();

        // Second, register new extrinsics
        auto profile_idx = find_stream_profile(profile);

        if (_extrinsics.find(profile_idx) == _extrinsics.end())
            _extrinsics.insert({ profile_idx, {} });
    }

    void extrinsics_graph::register_extrinsics(const stream_interface& from, const stream_interface& to, std::weak_ptr<lazy<rs2_extrinsics>> extr)
    {
        std::lock_guard<std::mutex> lock(_mutex);

        // First, trim any dead stream, to make sure we are not keep gaining memory
        cleanup_extrinsics();

        // Second, register new extrinsics
        auto from_idx = find_stream_profile(from);
        // If this is a new index, add it to the map preemptively,
        // This way find on to will be able to return another new index
        if (_extrinsics.find(from_idx) == _extrinsics.end())
            _extrinsics.insert({from_idx, {}});

        auto to_idx = find_stream_profile(to);

        _extrinsics[from_idx][to_idx] = extr;
        _extrinsics[to_idx][from_idx] = std::shared_ptr<lazy<rs2_extrinsics>>(nullptr);
    }

    void extrinsics_graph::register_extrinsics(const stream_interface & from, const stream_interface & to, rs2_extrinsics extr)
    {
        auto lazy_extr = std::make_shared<lazy<rs2_extrinsics>>([=]() {return extr; });
        _external_extrinsics.push_back(lazy_extr);
        register_extrinsics(from, to, lazy_extr);
    }

    void extrinsics_graph::override_extrinsics( const stream_interface& from, const stream_interface& to, rs2_extrinsics const & extr )
    {
        std::lock_guard<std::mutex> lock( _mutex );

        // First, trim any dead stream, to make sure we are not keep gaining memory
        cleanup_extrinsics();

        // The extrinsics must already exist!
        auto from_idx = find_stream_profile( from, false );  // do not add if not there
        auto from_it = _extrinsics.find( from_idx );
        if( from_it == _extrinsics.end() )
            throw std::runtime_error( "override_extrinsics called for invalid <from> stream" );
        auto& from_map = from_it->second;

        auto to_idx = find_stream_profile( to, false );  // do not add if not there
        auto to_it = from_map.find( to_idx );
        if( to_it == from_map.end() )
            throw std::runtime_error( "override_extrinsics called for invalid <to> stream" );
        auto& weak_ptr = to_it->second;
        auto sp = weak_ptr.lock();
        if( !sp )
            throw std::runtime_error( "override_extrinsics called for out-of-date stream" );

        auto & lazy_extr = *sp;
        lazy_extr = [=]() { return extr; };
    }

    void extrinsics_graph::cleanup_extrinsics()
    {
        if (_locks_count.load()) return;

        auto counter = 0;
        std::vector<int> invalid_ids;
        for (auto&& kvp : _streams)
        {
            if (!kvp.second.lock())
            {
                auto invalid_id = kvp.first;
                // Delete all extrinsics going out of this stream
                _extrinsics.erase(invalid_id);
                ++counter;
                invalid_ids.push_back(invalid_id);
            }
        }

        for (auto removed_id : invalid_ids)
        {
            _streams.erase(removed_id);
            for (auto&& elem : _extrinsics)
            {
                // Delete any extrinsics going into the stream
                elem.second.erase(removed_id);
                ++counter;
            }
        }

        if (!invalid_ids.empty())
            LOG_INFO("Found " << invalid_ids.size() << " unreachable streams, " << std::dec << counter << " extrinsics deleted");
    }

    int extrinsics_graph::find_stream_profile(const stream_interface& p, bool add_if_not_there)
    {
        auto sp = p.shared_from_this();
        auto max = 0;
        for (auto&& kvp : _streams)
        {
            if (kvp.second.lock().get() == sp.get())
                return kvp.first;
            max = std::max( max, kvp.first );
        }
        if( !add_if_not_there )
            return -1;
        _streams[max + 1] = sp;
        return max + 1;

    }

    bool extrinsics_graph::try_fetch_extrinsics(const stream_interface& from, const stream_interface& to, rs2_extrinsics* extr)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        cleanup_extrinsics();
        auto from_idx = find_stream_profile(from);
        auto to_idx = find_stream_profile(to);

        if (from_idx == to_idx)
        {
            *extr = identity_matrix();
            return true;
        }

        std::set<int> visited;
        return try_fetch_extrinsics(from_idx, to_idx, visited, extr);
    }

    bool extrinsics_graph::try_fetch_extrinsics(int from, int to, std::set<int>& visited, rs2_extrinsics* extr)
    {
        if (visited.count(from)) return false;

        auto it = _extrinsics.find(from);
        if (it != _extrinsics.end())
        {
            auto back_edge = fetch_edge(to, from);
            auto fwd_edge = fetch_edge(from, to);

            // Make sure both parts of the edge are still available
            if (fwd_edge.get() || back_edge.get())
            {
                if (fwd_edge.get())
                    *extr = fwd_edge->operator*(); // Evaluate the expression
                else
                    *extr = inverse(back_edge->operator*());

                return true;
            }
            else
            {
                visited.insert(from);
                for (auto&& kvp : it->second)
                {
                    auto new_from = kvp.first;
                    auto way = kvp.second;

                    // Lock down the edge in both directions to ensure we can evaluate the extrinsics
                    back_edge = fetch_edge(new_from, from);
                    fwd_edge = fetch_edge(from, new_from);

                    if ((back_edge.get() || fwd_edge.get()) &&
                        try_fetch_extrinsics(new_from, to, visited, extr))
                    {
                        const auto local = [&]() {
                            if (fwd_edge.get())
                                return fwd_edge->operator*(); // Evaluate the expression
                            else
                                return inverse(back_edge->operator*());
                        }();

                        auto pose = to_pose(*extr) * to_pose(local);
                        *extr = from_pose(pose);
                        return true;
                    }
                }
            }
        } // If there are no extrinsics from from, there are none to it, so it is completely isolated
        return false;
    }

    std::shared_ptr<lazy<rs2_extrinsics>> extrinsics_graph::fetch_edge(int from, int to)
    {
        auto it = _extrinsics.find(from);
        if (it != _extrinsics.end())
        {
            auto it2 = it->second.find(to);
            if (it2 != it->second.end())
            {
                return it2->second.lock();
            }
        }

        return nullptr;
    }


    environment& environment::get_instance()
    {
        static environment env;
        return env;
    }

    extrinsics_graph& environment::get_extrinsics_graph()
    {
        return _extrinsics;
    }

    void environment::set_time_service(std::shared_ptr<platform::time_service> ts)
    {
        _ts = ts;
    }

    std::shared_ptr<platform::time_service> environment::get_time_service()
    {
        return _ts;
    }
}
