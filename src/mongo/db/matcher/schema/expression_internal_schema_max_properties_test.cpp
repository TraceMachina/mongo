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

#include "mongo/base/string_data.h"
#include "mongo/bson/bsonmisc.h"
#include "mongo/bson/bsonobjbuilder.h"
#include "mongo/db/exec/matcher/matcher.h"
#include "mongo/db/matcher/schema/expression_internal_schema_max_properties.h"
#include "mongo/db/matcher/schema/expression_internal_schema_min_properties.h"
#include "mongo/unittest/assert.h"
#include "mongo/unittest/death_test.h"
#include "mongo/unittest/framework.h"
#include "mongo/util/assert_util.h"

namespace mongo {

namespace {

TEST(InternalSchemaMaxPropertiesMatchExpression, RejectsObjectsWithTooManyElements) {
    InternalSchemaMaxPropertiesMatchExpression maxProperties(0);

    ASSERT_FALSE(exec::matcher::matchesBSON(&maxProperties, BSON("b" << 21)));
    ASSERT_FALSE(exec::matcher::matchesBSON(&maxProperties, BSON("b" << 21 << "c" << 3)));
}

TEST(InternalSchemaMaxPropertiesMatchExpression, AcceptsObjectWithLessThanOrEqualToMaxElements) {
    InternalSchemaMaxPropertiesMatchExpression maxProperties(2);

    ASSERT_TRUE(exec::matcher::matchesBSON(&maxProperties, BSONObj()));
    ASSERT_TRUE(exec::matcher::matchesBSON(&maxProperties, BSON("b" << BSONNULL)));
    ASSERT_TRUE(exec::matcher::matchesBSON(&maxProperties, BSON("b" << 21)));
    ASSERT_TRUE(exec::matcher::matchesBSON(&maxProperties, BSON("b" << 21 << "c" << 3)));
}

TEST(InternalSchemaMaxPropertiesMatchExpression, MatchesSingleElementTest) {
    InternalSchemaMaxPropertiesMatchExpression maxProperties(2);

    // Only BSON elements that are embedded objects can match.
    BSONObj match = BSON("a" << BSON("a" << 5 << "b" << 10));
    BSONObj notMatch1 = BSON("a" << 1);
    BSONObj notMatch2 = BSON("a" << BSON("a" << 5 << "b" << 10 << "c" << 25));
    ASSERT_TRUE(maxProperties.matchesSingleElement(match.firstElement()));
    ASSERT_FALSE(maxProperties.matchesSingleElement(notMatch1.firstElement()));
    ASSERT_FALSE(maxProperties.matchesSingleElement(notMatch2.firstElement()));
}

TEST(InternalSchemaMaxPropertiesMatchExpression, MaxPropertiesZeroAllowsEmptyObjects) {
    InternalSchemaMaxPropertiesMatchExpression maxProperties(0);

    ASSERT_TRUE(exec::matcher::matchesBSON(&maxProperties, BSONObj()));
}

TEST(InternalSchemaMaxPropertiesMatchExpression, NestedObjectsAreNotUnwound) {
    InternalSchemaMaxPropertiesMatchExpression maxProperties(1);

    ASSERT_TRUE(
        exec::matcher::matchesBSON(&maxProperties, BSON("b" << BSON("c" << 2 << "d" << 3))));
}

TEST(InternalSchemaMaxPropertiesMatchExpression, EquivalentFunctionIsAccurate) {
    InternalSchemaMaxPropertiesMatchExpression maxProperties1(1);
    InternalSchemaMaxPropertiesMatchExpression maxProperties2(1);
    InternalSchemaMaxPropertiesMatchExpression maxProperties3(2);

    ASSERT_TRUE(maxProperties1.equivalent(&maxProperties1));
    ASSERT_TRUE(maxProperties1.equivalent(&maxProperties2));
    ASSERT_FALSE(maxProperties1.equivalent(&maxProperties3));
}

TEST(InternalSchemaMaxPropertiesMatchExpression, NestedArraysAreNotUnwound) {
    InternalSchemaMaxPropertiesMatchExpression maxProperties(2);

    ASSERT_TRUE(exec::matcher::matchesBSON(&maxProperties,
                                           BSON("a" << (BSON("b" << 2 << "c" << 3 << "d" << 4)))));
}

TEST(InternalSchemaMaxPropertiesMatchExpression, MinPropertiesNotEquivalentToMaxProperties) {
    InternalSchemaMaxPropertiesMatchExpression maxProperties(5);
    InternalSchemaMinPropertiesMatchExpression minProperties(5);

    ASSERT_FALSE(maxProperties.equivalent(&minProperties));
}

DEATH_TEST_REGEX(InternalSchemaMaxPropertiesMatchExpression,
                 GetChildFailsIndexGreaterThanZero,
                 "Tripwire assertion.*6400216") {
    InternalSchemaMaxPropertiesMatchExpression maxProperties(5);

    ASSERT_EQ(maxProperties.numChildren(), 0);
    ASSERT_THROWS_CODE(maxProperties.getChild(0), AssertionException, 6400216);
}

}  // namespace
}  // namespace mongo
