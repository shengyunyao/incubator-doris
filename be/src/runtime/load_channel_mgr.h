// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.

#pragma once

#include <unordered_map>
#include <memory>
#include <mutex>
#include <thread>
#include <ctime>

#include "common/status.h"
#include "gen_cpp/Types_types.h"
#include "gen_cpp/PaloInternalService_types.h"
#include "gen_cpp/internal_service.pb.h"
#include "runtime/tablets_channel.h"
#include "util/uid_util.h"

namespace doris {

class Cache;
class LoadChannel;

// LoadChannelMgr -> LoadChannel -> TabletsChannel -> DeltaWrtier
// All dispached load data for this backend is routed from this class
class LoadChannelMgr {
public:
    LoadChannelMgr();
    ~LoadChannelMgr();

    Status init(int64_t process_mem_limit);

    // open a new load channel if not exist
    Status open(const PTabletWriterOpenRequest& request);

    Status add_batch(const PTabletWriterAddBatchRequest& request,
                     google::protobuf::RepeatedPtrField<PTabletInfo>* tablet_vec,
                     int64_t* wait_lock_time_ns);

    // cancel all tablet stream for 'load_id' load
    Status cancel(const PTabletWriterCancelRequest& request);


private:
    // calculate the totol memory limit of all load processes on this Backend
    int64_t _calc_total_mem_limit(int64_t process_mem_limit);
    // calculate the memory limit for a single load process.
    int64_t _calc_load_mem_limit(int64_t mem_limit);

    // check if the total load mem consumption exceeds limit.
    // If yes, it will pick a load channel to try to reduce memory consumption.
    void _handle_mem_exceed_limit();

    Status _start_bg_worker();

private:
    // lock protect the load channel map
    std::mutex _lock;
    // load id -> load channel
    std::unordered_map<UniqueId, std::shared_ptr<LoadChannel>> _load_channels;
    Cache* _lastest_success_channel = nullptr;

    // check the total load mem consumption of this Backend
    std::unique_ptr<MemTracker> _mem_tracker;

    // thread to clean timeout load channels
    std::thread _load_channels_clean_thread;
    Status _start_load_channels_clean();
    std::atomic<bool> _is_stopped;
};

}
