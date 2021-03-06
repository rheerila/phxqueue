/*
Tencent is pleased to support the open source community by making PhxQueue available.
Copyright (C) 2017 THL A29 Limited, a Tencent company. All rights reserved.
Licensed under the BSD 3-Clause License (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at

<https://opensource.org/licenses/BSD-3-Clause>

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/



/* lock_client.cpp

 Generated by phxrpc_pb2client from lock.proto

*/

#include "lock_client.h"

#include <cstdlib>
#include <iostream>
#include <memory>
#include <mutex>

#include "phxqueue/comm.h"

#include "phxrpc_lock_stub.h"


using namespace std;


static phxrpc::ClientConfig global_lockclient_config_;
static phxrpc::ClientMonitorPtr global_lockclient_monitor_;


bool LockClient::Init(const char *config_file) {
    return global_lockclient_config_.Read(config_file);
}

const char *LockClient::GetPackageName() {
    const char *ret{global_lockclient_config_.GetPackageName()};
    if (0 == strlen(ret)) {
        ret = "phxqueue_phxrpc.lock";
    }
    return ret;
}

LockClient::LockClient() {
    static mutex monitor_mutex;
    if (!global_lockclient_monitor_.get()) {
        monitor_mutex.lock();
        if (!global_lockclient_monitor_.get()) {
            global_lockclient_monitor_ = phxrpc::MonitorFactory::GetFactory()
                    ->CreateClientMonitor(GetPackageName());
        }
        global_lockclient_config_.SetClientMonitor(global_lockclient_monitor_);
        monitor_mutex.unlock();
    }
}

LockClient::~LockClient() {}

int LockClient::PHXEcho(const google::protobuf::StringValue &req,
                        google::protobuf::StringValue *resp) {
    const phxrpc::Endpoint_t *ep{global_lockclient_config_.GetRandom()};

    if(ep != nullptr) {
        phxrpc::BlockTcpStream socket;
        bool open_ret{phxrpc::PhxrpcTcpUtils::Open(&socket, ep->ip, ep->port,
                global_lockclient_config_.GetConnectTimeoutMS(), nullptr, 0,
                *(global_lockclient_monitor_.get()))};
        if (open_ret) {
            socket.SetTimeout(global_lockclient_config_.GetSocketTimeoutMS());

            LockStub stub(socket, *(global_lockclient_monitor_.get()));
            return stub.PHXEcho(req, resp);
        }
    }

    return -1;
}

int LockClient::PhxBatchEcho(const google::protobuf::StringValue &req,
                             google::protobuf::StringValue *resp) {
    int ret{-1};
    size_t echo_server_count{2};
    uthread_begin;
    for (size_t i{0}; echo_server_count > i; ++i) {
        uthread_t [=, &uthread_s, &ret](void *) {
            const phxrpc::Endpoint_t * ep = global_lockclient_config_.GetByIndex(i);
            if (ep != nullptr) {
                phxrpc::UThreadTcpStream socket;
                if (phxrpc::PhxrpcTcpUtils::Open(&uthread_s, &socket, ep->ip, ep->port,
                        global_lockclient_config_.GetConnectTimeoutMS(),
                        *(global_lockclient_monitor_.get()))) {
                    socket.SetTimeout(global_lockclient_config_.GetSocketTimeoutMS());
                    LockStub stub(socket, *(global_lockclient_monitor_.get()));
                    int this_ret{stub.PHXEcho(req, resp)};
                    if (0 == this_ret) {
                        ret = this_ret;
                        uthread_s.Close();
                    }
                }
            }
        };
    }
    uthread_end;
    return ret;
}

int LockClient::GetLockInfo(const phxqueue::comm::proto::GetLockInfoRequest &req,
                            phxqueue::comm::proto::GetLockInfoResponse *resp) {
    const phxrpc::Endpoint_t *ep = global_lockclient_config_.GetRandom();

    if (nullptr != ep) {
        phxrpc::BlockTcpStream socket;
        bool open_ret = phxrpc::PhxrpcTcpUtils::Open(&socket, ep->ip, ep->port,
                global_lockclient_config_.GetConnectTimeoutMS(), nullptr, 0,
                *(global_lockclient_monitor_.get()));
        if (open_ret) {
            socket.SetTimeout(global_lockclient_config_.GetSocketTimeoutMS());

            LockStub stub(socket, *(global_lockclient_monitor_.get()));
            return stub.GetLockInfo(req, resp);
        }
    }

    return -1;
}

int LockClient::AcquireLock(const phxqueue::comm::proto::AcquireLockRequest &req,
                            phxqueue::comm::proto::AcquireLockResponse *resp) {
    const phxrpc::Endpoint_t *ep{global_lockclient_config_.GetRandom()};

    if (ep != nullptr) {
        phxrpc::BlockTcpStream socket;
        bool open_ret{phxrpc::PhxrpcTcpUtils::Open(&socket, ep->ip, ep->port,
                global_lockclient_config_.GetConnectTimeoutMS(), nullptr, 0,
                *(global_lockclient_monitor_.get()))};
        if (open_ret) {
            socket.SetTimeout(global_lockclient_config_.GetSocketTimeoutMS());

            LockStub stub(socket, *(global_lockclient_monitor_.get()));
            return stub.AcquireLock(req, resp);
        }
    }

    return -1;
}

phxqueue::comm::RetCode
LockClient::ProtoGetLockInfo(const phxqueue::comm::proto::GetLockInfoRequest &req,
                             phxqueue::comm::proto::GetLockInfoResponse &resp) {
    const char *ip{req.master_addr().ip().c_str()};
    const int port{req.master_addr().port()};

    auto &&socketpool = phxqueue::comm::ResourcePoll<uint64_t, phxrpc::BlockTcpStream>::GetInstance();
    auto &&key = phxqueue::comm::utils::EncodeAddr(req.master_addr());
    auto socket = std::move(socketpool->Get(key));

    if (nullptr == socket.get()) {
        socket.reset(new phxrpc::BlockTcpStream());

        bool open_ret{phxrpc::PhxrpcTcpUtils::Open(socket.get(), ip, port,
                                                   global_lockclient_config_.GetConnectTimeoutMS(), nullptr, 0,
                                                   *(global_lockclient_monitor_.get()))};
        if (!open_ret) {
            QLErr("phxrpc Open err. ip %s port %d", ip, port);

            return phxqueue::comm::RetCode::RET_ERR_SYS;
        }
        socket->SetTimeout(global_lockclient_config_.GetSocketTimeoutMS());
    }

    LockStub stub(*(socket.get()), *(global_lockclient_monitor_.get()));
    stub.SetKeepAlive(true);
    int ret{stub.GetLockInfo(req, &resp)};
    if (0 > ret) {
        QLErr("GetLockInfo err %d", ret);
    }
    if (-1 != ret && -202 != ret) {
        socketpool->Put(key, socket);
    }
    return static_cast<phxqueue::comm::RetCode>(ret);
}

phxqueue::comm::RetCode
LockClient::ProtoAcquireLock(const phxqueue::comm::proto::AcquireLockRequest &req,
                             phxqueue::comm::proto::AcquireLockResponse &resp) {
    const char *ip{req.master_addr().ip().c_str()};
    const int port{req.master_addr().port()};

    auto &&socketpool = phxqueue::comm::ResourcePoll<uint64_t, phxrpc::BlockTcpStream>::GetInstance();
    auto &&key = phxqueue::comm::utils::EncodeAddr(req.master_addr());
    auto socket = std::move(socketpool->Get(key));


    if (nullptr == socket.get()) {
        socket.reset(new phxrpc::BlockTcpStream());

        bool open_ret{phxrpc::PhxrpcTcpUtils::Open(socket.get(), ip, port,
                                                   global_lockclient_config_.GetConnectTimeoutMS(), nullptr, 0,
                                                   *(global_lockclient_monitor_.get()))};
        if (!open_ret) {
            QLErr("phxrpc Open err. ip %s port %d", ip, port);

            return phxqueue::comm::RetCode::RET_ERR_SYS;
        }
        socket->SetTimeout(global_lockclient_config_.GetSocketTimeoutMS());
    }

    LockStub stub(*(socket.get()), *(global_lockclient_monitor_.get()));
    stub.SetKeepAlive(true);
    int ret{stub.AcquireLock(req, &resp)};
    if (0 > ret) {
        QLErr("AcquireLock err %d", ret);
    }
    if (-1 != ret && -202 != ret) {
        socketpool->Put(key, socket);
    }
    return static_cast<phxqueue::comm::RetCode>(ret);
}

