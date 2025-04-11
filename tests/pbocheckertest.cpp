#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <QFile>
#include <QStringLiteral>

#include "pbochecker.h"

using namespace Qt::StringLiterals;
using namespace testing;
using std::get;

class PboCheckerTest : public Test
{
protected:
    void SetUp() override
    {
    }

    void TearDown() override
    {
    }
};

TEST_F(PboCheckerTest, checkPbo3cb)
{
    PboChecker pboChecker;
    const auto pbo = u":/@3cb_factions/Addons/uk3cb_factions_common.pbo"_s;
    ASSERT_TRUE(QFile::exists(pbo));
    const auto bikey = u":/@3cb_factions/Keys/uk3cb_factions_8_0_1.bikey"_s;
    ASSERT_TRUE(QFile::exists(bikey));
    const auto bisigin = u":/@3cb_factions/Addons/uk3cb_factions_common.pbo.uk3cb_factions_8_0_1.bisign"_s;
    ASSERT_TRUE(QFile::exists(bisigin));
    const bool success = get<0>(pboChecker.checkPbo(pbo, bikey, bisigin));
    ASSERT_TRUE(success);
}

TEST_F(PboCheckerTest, checkPboCba)
{
    PboChecker pboChecker;
    const auto pbo = u":/@CBA_A3/addons/cba_main_a3.pbo"_s;
    ASSERT_TRUE(QFile::exists(pbo));
    const auto bikey = u":/@CBA_A3/Keys/cba_3.18.3.250320.bikey"_s;
    ASSERT_TRUE(QFile::exists(bikey));
    const auto bisigin = u":/@CBA_A3/addons/cba_main_a3.pbo.cba_3.18.3.250320.bisign"_s;
    ASSERT_TRUE(QFile::exists(bisigin));
    const bool success = get<0>(pboChecker.checkPbo(pbo, bikey, bisigin));
    ASSERT_TRUE(success);
}

TEST_F(PboCheckerTest, checkPboTunFiresupport)
{
    PboChecker pboChecker;
    const auto pbo = u":/@tun_firesupport/Addons/firesupport.pbo"_s;
    ASSERT_TRUE(QFile::exists(pbo));
    const auto bikey = u":/@tun_firesupport/Keys/Tun Firesupport v2.1.3.bikey"_s;
    ASSERT_TRUE(QFile::exists(bikey));
    const auto bisigin = u":/@tun_firesupport/Addons/firesupport.pbo.Tun Firesupport v2.1.3.bisign"_s;
    ASSERT_TRUE(QFile::exists(bisigin));
    const bool success = get<0>(pboChecker.checkPbo(pbo, bikey, bisigin));
    ASSERT_TRUE(success);
}

TEST_F(PboCheckerTest, checkPboFacesOfWar)
{
    PboChecker pboChecker;
    const auto pbo = u":/@faces_of_war/Addons/fow_mortars.pbo"_s;
    ASSERT_TRUE(QFile::exists(pbo));
    const auto bikey = u":/@faces_of_war/keys/fow_14_05_2023.bikey"_s;
    ASSERT_TRUE(QFile::exists(bikey));
    const auto bisigin = u":/@faces_of_war/Addons/fow_mortars.pbo.fow_14_05_2023.bisign"_s;
    ASSERT_TRUE(QFile::exists(bisigin));
    const bool success = get<0>(pboChecker.checkPbo(pbo, bikey, bisigin));
    ASSERT_TRUE(success);
}

TEST_F(PboCheckerTest, checkPboAfiCorrupted)
{
    PboChecker pboChecker;
    const auto pbo = u":/@afi_corrupted/addons/afi_logo.pbo"_s;
    ASSERT_TRUE(QFile::exists(pbo));
    const auto bikey = u":/@afi_corrupted/keys/afi_1.1.3.1.bikey"_s;
    ASSERT_TRUE(QFile::exists(bikey));
    const auto bisigin = u":/@afi_corrupted/addons/afi_logo.pbo.afi_1.1.3.1.bisign"_s;
    ASSERT_TRUE(QFile::exists(bisigin));
    const bool success = get<0>(pboChecker.checkPbo(pbo, bikey, bisigin));
    ASSERT_FALSE(success);
}

TEST_F(PboCheckerTest, checkPboIfa)
{
    PboChecker pboChecker;
    const auto pbo = u":/@ifa3_aio_lite/addons/ww2_core_f_if_system_test_f.pbo"_s;
    ASSERT_TRUE(QFile::exists(pbo));
    const auto bikey = u":/@ifa3_aio_lite/keys/ifa3_aio_second_wave_2022_02_22_v2.bikey"_s;
    ASSERT_TRUE(QFile::exists(bikey));
    const auto bisigin = u":/@ifa3_aio_lite/addons/ww2_core_f_if_system_test_f.pbo.ifa3_aio_second_wave_2022_02_22_v2.bisign"_s;
    ASSERT_TRUE(QFile::exists(bisigin));
    const bool success = get<0>(pboChecker.checkPbo(pbo, bikey, bisigin));
    ASSERT_TRUE(success);
}

// This mod has two keys. Both of them should be tried.
TEST_F(PboCheckerTest, checkPboFataA3)
{
    PboChecker pboChecker;
    const auto pbo = u":/@fata_a3/addons/FATALighting.pbo"_s;
    ASSERT_TRUE(QFile::exists(pbo));
    const auto bisigin = u":/@fata_a3/addons/FATALighting.pbo.LatestLightingTunty.bisign"_s;
    ASSERT_TRUE(QFile::exists(bisigin));
    const bool success = get<0>(pboChecker.checkPbo(pbo));
    ASSERT_TRUE(success);
}

TEST_F(PboCheckerTest, checkPboAfiAce3Corrupted)
{
    PboChecker pboChecker;
    const auto pbo = u":/@afi_ace3_corrupted/addons/ace_modules.pbo"_s;
    ASSERT_TRUE(QFile::exists(pbo));
    const auto bikey = u":/@afi_ace3_corrupted/keys/afi_ace3_1744288397.bikey"_s;
    ASSERT_TRUE(QFile::exists(bikey));
    const auto bisign = u":/@afi_ace3_corrupted/addons/ace_modules.pbo.afi_ace3_1744288397.bisign"_s;
    ASSERT_TRUE(QFile::exists(bisign));
    const bool success = get<0>(pboChecker.checkPbo(pbo, bikey, bisign));
    ASSERT_FALSE(success);
}

