// Copyright (c) 2016-2020 The Rain Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <amount.h>
#include <policy/feerate.h>
#include <policy/policy.h>

#include <test/setup_common.h>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(amount_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(MoneyRangeTest)
{
    BOOST_CHECK_EQUAL(MoneyRange(CAmount(-1)), false);
    BOOST_CHECK_EQUAL(MoneyRange(CAmount(0)), true);
    BOOST_CHECK_EQUAL(MoneyRange(CAmount(1)), true);
    BOOST_CHECK_EQUAL(MoneyRange(MAX_MONEY), true);
    BOOST_CHECK_EQUAL(MoneyRange(MAX_MONEY + CAmount(1)), false);
}

BOOST_AUTO_TEST_CASE(GetFeeTest)
{
    CFeeRate feeRate, altFeeRate;

    feeRate = CFeeRate(CAmountMap());
    // Must always return 0
    BOOST_CHECK_EQUAL(feeRate.GetFee(0), CAmountMap());
    BOOST_CHECK_EQUAL(feeRate.GetFee(1e5), CAmountMap());
    CAmountMap map1000 = populateMap(1000);
    CAmountMap map1 = populateMap(1);
    CAmountMap map123 = populateMap(123);
    CAmountMap map2 = populateMap(2);
    CAmountMap map26 = populateMap(26);
    CAmountMap map27 = populateMap(27);
    CAmountMap map32 =populateMap(32) ;
    CAmountMap map34 = populateMap(34);
    CAmountMap mapMAX_MONEY = populateMap(MAX_MONEY);
    CAmountMap negmap1 = map1*-1;

    feeRate = CFeeRate(map1000);
    // Must always just return the arg
    BOOST_CHECK_EQUAL(feeRate.GetFee(0), populateMap(0));
    BOOST_CHECK_EQUAL(feeRate.GetFee(1), populateMap(1));
    BOOST_CHECK_EQUAL(feeRate.GetFee(121), populateMap(121));
    BOOST_CHECK_EQUAL(feeRate.GetFee(999), populateMap(999));
    BOOST_CHECK_EQUAL(feeRate.GetFee(1e3), populateMap(1e3));
    BOOST_CHECK_EQUAL(feeRate.GetFee(9e3), populateMap(9e3));

    feeRate = CFeeRate(map1000*-1);
    // Must always just return -1 * arg
    BOOST_CHECK_EQUAL(feeRate.GetFee(0), populateMap(0));
    BOOST_CHECK_EQUAL(feeRate.GetFee(1), populateMap(-1));
    BOOST_CHECK_EQUAL(feeRate.GetFee(121), populateMap(-121));
    BOOST_CHECK_EQUAL(feeRate.GetFee(999), populateMap(-999));
    BOOST_CHECK_EQUAL(feeRate.GetFee(1e3), populateMap(-1e3));
    BOOST_CHECK_EQUAL(feeRate.GetFee(9e3), populateMap(-9e3));

    feeRate = CFeeRate(map123);
    // Truncates the result, if not integer
    BOOST_CHECK_EQUAL(feeRate.GetFee(0), populateMap(0));
    BOOST_CHECK_EQUAL(feeRate.GetFee(8), populateMap(1)); // Special case: returns 1 instead of 0
    BOOST_CHECK_EQUAL(feeRate.GetFee(9), populateMap(1));
    BOOST_CHECK_EQUAL(feeRate.GetFee(121), populateMap(14));
    BOOST_CHECK_EQUAL(feeRate.GetFee(122), populateMap(15));
    BOOST_CHECK_EQUAL(feeRate.GetFee(999), populateMap(122));
    BOOST_CHECK_EQUAL(feeRate.GetFee(1e3), populateMap(123));
    BOOST_CHECK_EQUAL(feeRate.GetFee(9e3), populateMap(1107));

    feeRate = CFeeRate(map123*-1);
    // Truncates the result, if not integer
    BOOST_CHECK_EQUAL(feeRate.GetFee(0), populateMap(0));
    BOOST_CHECK_EQUAL(feeRate.GetFee(8), populateMap(-1)); // Special case: returns -1 instead of 0
    BOOST_CHECK_EQUAL(feeRate.GetFee(9), populateMap(-1));

    // check alternate constructor
    feeRate = CFeeRate(map1000);
    altFeeRate = CFeeRate(feeRate);
    BOOST_CHECK_EQUAL(feeRate.GetFee(100), altFeeRate.GetFee(100));

    // Check full constructor
    BOOST_CHECK(CFeeRate(negmap1, 0) == CFeeRate(CAmountMap()));
    BOOST_CHECK(CFeeRate(CAmountMap(), 0) == CFeeRate(CAmountMap()));
    BOOST_CHECK(CFeeRate(map1, 0) == CFeeRate(CAmountMap()));
    // default value

    BOOST_CHECK(CFeeRate(negmap1, 1000) == CFeeRate(negmap1));
    BOOST_CHECK(CFeeRate(CAmountMap(), 1000) == CFeeRate(CAmountMap()));
    BOOST_CHECK(CFeeRate(map1, 1000) == CFeeRate(map1));
    // lost precision (can only resolve satoshis per kB)
    BOOST_CHECK(CFeeRate(map1, 1001) == CFeeRate(CAmountMap()));
    BOOST_CHECK(CFeeRate(map2, 1001) == CFeeRate(map1));
    // some more integer checks
    BOOST_CHECK(CFeeRate(map26, 789) == CFeeRate(map32));
    BOOST_CHECK(CFeeRate(map27, 789) == CFeeRate(map34));
    // Maximum size in bytes, should not crash
    CFeeRate(mapMAX_MONEY, std::numeric_limits<size_t>::max() >> 1).GetFeePerK();
}

BOOST_AUTO_TEST_CASE(BinaryOperatorTest)
{
    CAmountMap map1 = populateMap(1);
    CAmountMap map2 = populateMap(2);

    CFeeRate a, b;
    a = CFeeRate(map1);
    b = CFeeRate(map2);
    BOOST_CHECK(a < b);
    BOOST_CHECK(b > a);
    BOOST_CHECK(a == a);
    BOOST_CHECK(a <= b);
    BOOST_CHECK(a <= a);
    BOOST_CHECK(b >= a);
    BOOST_CHECK(b >= b);
    // a should be 0.00000002 BTC/kB now
    a += a;
    BOOST_CHECK(a == b);
}

BOOST_AUTO_TEST_CASE(ToStringTest)
{
    CFeeRate feeRate;
    CAmountMap map1 = populateMap(1);

    feeRate = CFeeRate(map1);
    BOOST_CHECK_EQUAL(feeRate.ToString(), "0.00000001 BTC/kB");
}

BOOST_AUTO_TEST_SUITE_END()
