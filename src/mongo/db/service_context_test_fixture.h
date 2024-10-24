/**
 *    Copyright (C) 2018-present MongoDB, Inc.
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the Server Side Public License, version 1,
 *    as published by MongoDB, Inc.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    Server Side Public License for more details.
 *
 *    You should have received a copy of the Server Side Public License
 *    along with this program. If not, see
 *    <http://www.mongodb.com/licensing/server-side-public-license>.
 *
 *    As a special exception, the copyright holders give permission to link the
 *    code of portions of this program with the OpenSSL library under certain
 *    conditions as described in each individual source file and distribute
 *    linked combinations including the program with the OpenSSL library. You
 *    must comply with the Server Side Public License in all respects for
 *    all of the code used other than as permitted herein. If you modify file(s)
 *    with this exception, you may extend this exception to your version of the
 *    file(s), but you are not obligated to do so. If you do not wish to do so,
 *    delete this exception statement from your version. If you delete this
 *    exception statement from all source files in the program, then also delete
 *    it in the license file.
 */

#pragma once

#include "mongo/db/client.h"
#include "mongo/db/operation_context.h"
#include "mongo/db/service_context.h"
#include "mongo/unittest/unittest.h"

namespace mongo {
namespace service_context_test {

/**
 * "Literal" and "structural" type to stand-in for a `ClusterRole` value.
 * Necessary until `ClusterRole` can be used as a NTTP.
 */
enum class ServerRoleIndex { shard, router };

inline ClusterRole getClusterRole(ServerRoleIndex i) {
    switch (i) {
        case ServerRoleIndex::shard:
            return {ClusterRole::ShardServer, ClusterRole::RouterServer};
        case ServerRoleIndex::router:
            return ClusterRole::RouterServer;
    }
    MONGO_UNREACHABLE;
}

/**
 * Virtual base for tests that need to set SC role before the
 * ServiceContextTest constructor. Necessary since ServiceContextTest
 * is often a virtual base and those run before all non-virtual bases.
 */
template <ServerRoleIndex roleIndex>
class RoleOverride {
public:
    ~RoleOverride() {
        serverGlobalParams.clusterRole = _saved;
    }

private:
    ClusterRole _saved{std::exchange(serverGlobalParams.clusterRole, getClusterRole(roleIndex))};
};

using ShardRoleOverride = RoleOverride<ServerRoleIndex::shard>;
using RouterRoleOverride = RoleOverride<ServerRoleIndex::router>;

class ScopedGlobalServiceContextForTest {
public:
    ScopedGlobalServiceContextForTest();
    explicit ScopedGlobalServiceContextForTest(ServiceContext::UniqueServiceContext serviceContext);
    virtual ~ScopedGlobalServiceContextForTest();

    /**
     * Returns a service context, which is only valid for this instance of the test.
     */
    ServiceContext* getServiceContext() const;

    Service* getService() const;
};

/**
 * Test fixture for tests that require a properly initialized global service context.
 */
class ServiceContextTest : public unittest::Test {
public:
    /**
     * Returns the default Client for this test.
     */
    Client* getClient() {
        return Client::getCurrent();
    }

    ServiceContext::UniqueOperationContext makeOperationContext() {
        return getClient()->makeOperationContext();
    }

    ServiceContext* getServiceContext() const {
        return _scopedServiceContext->getServiceContext();
    }

    Service* getService() const {
        return _scopedServiceContext->getService();
    }

protected:
    ServiceContextTest();
    explicit ServiceContextTest(
        std::unique_ptr<ScopedGlobalServiceContextForTest> scopedServiceContext,
        std::shared_ptr<transport::Session> session = nullptr);
    ~ServiceContextTest() override;

    ScopedGlobalServiceContextForTest* scopedServiceContext() const {
        return _scopedServiceContext.get();
    }

private:
    std::unique_ptr<ScopedGlobalServiceContextForTest> _scopedServiceContext;
    ThreadClient _threadClient;
};

/**
 * Test fixture for tests that require a properly-initialized global service context with
 * the fast and precise clock sources set to instances of ClockSourceMock and the tick source
 * set to an instance of TickSourceMock.
 */
class ClockSourceMockServiceContextTest : public ServiceContextTest {
protected:
    ClockSourceMockServiceContextTest();
};

/**
 * Test fixture for tests that require a properly-initialized global service context with
 * the fast and precise clock sources set to instances of SharedClockSourceAdapter with the
 * same underlying clock source.
 */
class SharedClockSourceAdapterServiceContextTest : public ServiceContextTest {
protected:
    explicit SharedClockSourceAdapterServiceContextTest(std::shared_ptr<ClockSource> clock);
};

}  // namespace service_context_test

using service_context_test::ClockSourceMockServiceContextTest;
using service_context_test::ScopedGlobalServiceContextForTest;
using service_context_test::ServiceContextTest;
using service_context_test::SharedClockSourceAdapterServiceContextTest;

}  // namespace mongo
