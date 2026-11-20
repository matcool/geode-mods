#pragma once

#include <arc/prelude.hpp>
#include <Geode/Result.hpp>
#include <vector>
#include <utility>
#include <string_view>

struct SampleInfo {
	std::vector<float> samples;

	using SampleFuture = arc::Future<geode::Result<std::vector<float>>>;

	static std::pair<SampleFuture, float> fromFile(std::string_view songPath, int targetWidth);
};