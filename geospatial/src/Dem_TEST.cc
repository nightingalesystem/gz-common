/*
 * Copyright (C) 2016 Open Source Robotics Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
*/

#include <gtest/gtest.h>
#include <limits>
#include <ignition/math/Angle.hh>
#include <ignition/math/Vector3.hh>

#include "ignition/common/geospatial/Dem.hh"
#include "test_config.h"

using namespace ignition;

class DemTest : public common::testing::AutoLogFixture { };

/////////////////////////////////////////////////
TEST_F(DemTest, MissingFile)
{
  common::Dem dem;
  EXPECT_NE(dem.Load("/file/shouldn/never/exist.png"), 0);
}

/////////////////////////////////////////////////
TEST_F(DemTest, NotDem)
{
  common::Dem dem;
  const auto path = common::testing::TestFile("CMakeLists.txt");
  EXPECT_NE(dem.Load(path), 0);
}

/////////////////////////////////////////////////
TEST_F(DemTest, UnsupportedDem)
{
  common::Dem dem;
  const auto path = common::testing::TestFile("data", "heightmap_bowl.png");
  EXPECT_NE(dem.Load(path), 0);
}

/////////////////////////////////////////////////
TEST_F(DemTest, NonSquaredDemPortrait)
{
  common::Dem dem;
  const auto path = common::testing::TestFile("data", "dem_portrait.tif");
  EXPECT_EQ(dem.Load(path), 0);
}

/////////////////////////////////////////////////
TEST_F(DemTest, NonSquaredDemLandscape)
{
  common::Dem dem;
  const auto path = common::testing::TestFile("data", "dem_landscape.tif");
  EXPECT_EQ(dem.Load(path), 0);
}

/////////////////////////////////////////////////
TEST_F(DemTest, SquaredDem)
{
  common::Dem dem;
  const auto path = common::testing::TestFile("data", "dem_squared.tif");
  EXPECT_EQ(dem.Load(path), 0);
}

/////////////////////////////////////////////////
TEST_F(DemTest, BasicAPI)
{
  common::Dem dem;
  const auto path = common::testing::TestFile("data", "dem_squared.tif");
  EXPECT_EQ(dem.Load(path), 0);

  // Check filename
  EXPECT_EQ(path, dem.Filename());

  // Check the heights and widths
  EXPECT_EQ(129, static_cast<int>(dem.Height()));
  EXPECT_EQ(129, static_cast<int>(dem.Width()));
  EXPECT_FLOAT_EQ(3984.4849f, dem.WorldHeight());
  EXPECT_FLOAT_EQ(3139.7456f, dem.WorldWidth());
  EXPECT_FLOAT_EQ(65.3583f, dem.MinElevation());
  EXPECT_FLOAT_EQ(318.441f, dem.MaxElevation());

  // Check Elevation()
  unsigned int width = dem.Width();
  unsigned int height = dem.Height();
  EXPECT_FLOAT_EQ(215.82324f, dem.Elevation(0, 0));
  EXPECT_FLOAT_EQ(216.04961f, dem.Elevation(width - 1, 0));
  EXPECT_FLOAT_EQ(142.2274f, dem.Elevation(0, height - 1));
  EXPECT_FLOAT_EQ(209.14784f, dem.Elevation(width - 1, height - 1));

  // Illegal coordinates
  double inf = std::numeric_limits<double>::infinity();
  EXPECT_DOUBLE_EQ(inf, dem.Elevation(0, height));
  EXPECT_DOUBLE_EQ(inf, dem.Elevation(width, 0));
  EXPECT_DOUBLE_EQ(inf, dem.Elevation(width, height));

  // Check GeoReferenceOrigin()
  ignition::math::Angle latitude, longitude;
  EXPECT_TRUE(dem.GeoReferenceOrigin(latitude, longitude));
  EXPECT_FLOAT_EQ(38.001667f, latitude.Degree());
  EXPECT_FLOAT_EQ(-122.22278f, longitude.Degree());
}

/////////////////////////////////////////////////
TEST_F(DemTest, FillHeightmap)
{
  common::Dem dem;
  const auto path = common::testing::TestFile("data", "dem_squared.tif");
  EXPECT_EQ(dem.Load(path), 0);

  // Use FillHeightMap() to retrieve a vector<float> after some transformations
  int subsampling;
  unsigned vertSize;
  ignition::math::Vector3d size;
  ignition::math::Vector3d scale;
  bool flipY;
  std::vector<float> elevations;

  subsampling = 2;
  vertSize = (dem.Width() * subsampling) - 1;
  size.X(dem.WorldWidth());
  size.Y(dem.WorldHeight());
  size.Z(dem.MaxElevation() - dem.MinElevation());
  scale.X(size.X() / vertSize);
  scale.Y(size.Y() / vertSize);

  if (ignition::math::equal(dem.MaxElevation(), 0.0f))
    scale.Z(fabs(size.Z()));
  else
    scale.Z(fabs(size.Z()) / dem.MaxElevation());
  flipY = false;

  dem.FillHeightMap(subsampling, vertSize, size, scale, flipY, elevations);

  // Check the size of the returned vector
  EXPECT_EQ(vertSize * vertSize, elevations.size());

  // Check the elevation of some control points
  EXPECT_FLOAT_EQ(184.94113f, elevations.at(0));
  EXPECT_FLOAT_EQ(179.63583f, elevations.at(elevations.size() - 1));
  EXPECT_FLOAT_EQ(213.42966f, elevations.at(elevations.size() / 2));
}

/////////////////////////////////////////////////
TEST_F(DemTest, UnfinishedDem)
{
  common::Dem dem;
  auto path = common::testing::TestFile("data", "dem_unfinished.tif");
  EXPECT_EQ(dem.Load(path), 0);

  // Check that the min and max elevations are valid for an unfinished
  // and unfilled dem.
  EXPECT_EQ(33, static_cast<int>(dem.Height()));
  EXPECT_EQ(33, static_cast<int>(dem.Width()));
  EXPECT_FLOAT_EQ(111287.59f, dem.WorldHeight());
  EXPECT_FLOAT_EQ(88878.297f, dem.WorldWidth());
  // gdal reports min elevation as -32768 but this is treated as a nodata
  // by our dem class and ignored when computing the min elevation
  EXPECT_FLOAT_EQ(-10.0f, dem.MinElevation());
  EXPECT_FLOAT_EQ(1909.0f, dem.MaxElevation());

  // test another dem file with multiple nodata values
  common::Dem demNoData;

  path = common::testing::TestFile("data", "dem_nodata.dem");
  EXPECT_EQ(demNoData.Load(path), 0);

  // Check that the min and max elevations are valid for a dem with multiple
  // nodata values
  EXPECT_EQ(65, static_cast<int>(demNoData.Height()));
  EXPECT_EQ(65, static_cast<int>(demNoData.Width()));

  // relaxed tolerances for macOS & windows
  EXPECT_NEAR(7499.8281, demNoData.WorldHeight(), 0.1);
  EXPECT_NEAR(14150.225, demNoData.WorldWidth(), 0.1);

  // gdal reports min elevation as -32767 but this is treated as a nodata
  // by our dem class and ignored when computing the min elevation
  EXPECT_FLOAT_EQ(682.0f, demNoData.MinElevation());
  EXPECT_FLOAT_EQ(2932.0f, demNoData.MaxElevation());
}

/////////////////////////////////////////////////
TEST_F(DemTest, NonEarthDem)
{
  // moon
  common::Dem dem;
  auto path = common::testing::TestFile("data", "dem_moon.tif");
  EXPECT_EQ(dem.Load(path), 0);

  EXPECT_EQ(33, static_cast<int>(dem.Height()));
  EXPECT_EQ(33, static_cast<int>(dem.Width()));
  EXPECT_FLOAT_EQ(-212.29616f, dem.MinElevation());
  EXPECT_FLOAT_EQ(-205.44009f, dem.MaxElevation());

  // unable to determne world width/height of non earth DEM
  EXPECT_FLOAT_EQ(-1, dem.WorldHeight());
  EXPECT_FLOAT_EQ(-1, dem.WorldWidth());

  // unable to get coordinates in WGS84
  ignition::math::Angle latitude, longitude;
  EXPECT_FALSE(dem.GeoReferenceOrigin(latitude, longitude));
}
