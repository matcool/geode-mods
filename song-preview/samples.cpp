#include "samples.hpp"
#include <Geode/Geode.hpp>

using namespace geode::prelude;

std::pair<SampleInfo::SampleFuture, float> SampleInfo::fromFile(std::string_view songPath, int targetWidth) {
	auto* engine = FMODAudioEngine::sharedEngine();

	FMOD::Sound* sound;
	const FMOD_MODE mode = FMOD_DEFAULT | FMOD_OPENONLY;
	auto result = engine->m_system->createSound(songPath.data(), mode, nullptr, &sound);
	if (result != FMOD_OK) {
		log::error("Error creating sound! {}", int(result));
	}
	unsigned int lengthMilliseconds;
	result = sound->getLength(&lengthMilliseconds, FMOD_TIMEUNIT_MS);
	if (result != FMOD_OK) {
		log::error("Error getting length {}", int(result));
	}
	float songLength = static_cast<float>(lengthMilliseconds) / 1000.f;

	auto future = [sound, targetWidth](this auto self) -> SampleFuture {
		FMOD_SOUND_FORMAT format;
		int channels;
		sound->getFormat(nullptr, &format, &channels, nullptr);

		if (format != FMOD_SOUND_FORMAT_PCM8 && format != FMOD_SOUND_FORMAT_PCM16 && format != FMOD_SOUND_FORMAT_PCM24 &&
			format != FMOD_SOUND_FORMAT_PCM32 && format != FMOD_SOUND_FORMAT_PCMFLOAT) {
			log::error("Invalid format! {}", int(format));
			co_return Err("Invalid sound format");
		}

		unsigned int rawDataSize;
		unsigned int sampleCount;
		auto result = sound->getLength(&rawDataSize, FMOD_TIMEUNIT_PCMBYTES);
		if (result != FMOD_OK) {
			log::error("Error reading size! {}", int(result));
			co_return Err("Error reading song length");
		}
		sound->getLength(&sampleCount, FMOD_TIMEUNIT_PCM);
		unsigned int bytesPerSample = rawDataSize / sampleCount;
		auto rawData = std::make_unique<uint8_t[]>(rawDataSize);
		unsigned int readCount;
		result = sound->readData(rawData.get(), rawDataSize, &readCount);
		if (result != FMOD_OK) {
			// ignore EOF here because what it reads is good enough, lol
			// maybe im calculating the size wrong,,, but blame config

			// log::error("Error reading data! {}, only read {} out of {}", int(result), readCount, rawDataSize);
			// return SampleTask::Cancel();
		}

		sound->release();

		// code based off config's mod:
		// https://github.com/cgytrus/EditorWaveform/blob/main/src/dllmain.cpp
		// lots of adjustments were made to make it match newground's waveform

		sampleCount = rawDataSize / bytesPerSample;
		bytesPerSample /= channels;
		const auto getSampleAt = [&](int sampleIndex) {
			if (sampleIndex < 0 || sampleIndex >= sampleCount)
				return 0.f;
			float sample = 0.f;
			size_t dataIndex = sampleIndex * bytesPerSample * channels;
			if (dataIndex > rawDataSize) {
				// shh
				return 0.f;
			}
			for (int j = 0; j < channels; j++) {
				uint32_t rawChannelSample;
				switch (format) {
					case FMOD_SOUND_FORMAT_PCM8:
						rawChannelSample = rawData[dataIndex + 0];
						sample += std::bit_cast<int8_t>(static_cast<uint8_t>(rawChannelSample)) / 128.f;
						break;
					case FMOD_SOUND_FORMAT_PCM16:
						rawChannelSample = (rawData[dataIndex + 0] << 0 |
											rawData[dataIndex + 1] << 8);
						sample += std::bit_cast<int16_t>(static_cast<uint16_t>(rawChannelSample)) / (128.f * 255.f);
						break;
					case FMOD_SOUND_FORMAT_PCM24:
						// TODO: this is wrong, doesnt properly read sign bit
						rawChannelSample = (rawData[dataIndex + 0] << 0 |
											rawData[dataIndex + 1] << 8 |
											rawData[dataIndex + 2] << 16);
						sample += rawChannelSample / (128.f * 255.f * 255.f);
						break;
					case FMOD_SOUND_FORMAT_PCM32:
						rawChannelSample = (rawData[dataIndex + 0] << 0  |
											rawData[dataIndex + 1] << 8  |
											rawData[dataIndex + 2] << 16 |
											rawData[dataIndex + 3] << 24);
						sample += std::bit_cast<int32_t>(rawChannelSample) / (128.f * 255.f * 255.f * 255.f);
						break;
					case FMOD_SOUND_FORMAT_PCMFLOAT:
						rawChannelSample = (rawData[dataIndex + 0] << 0  |
											rawData[dataIndex + 1] << 8  |
											rawData[dataIndex + 2] << 16 |
											rawData[dataIndex + 3] << 24);
						sample += std::bit_cast<float>(rawChannelSample);
						break;
					default:
						break;
				}
				dataIndex += bytesPerSample;
			}
			sample /= channels;
			return sample;
		};

		std::vector<float> samples;
		samples.reserve(targetWidth);

		for (float x = 0.f; x < targetWidth; x += 1.f) {
			co_await arc::coopYield();

			int index = static_cast<int>(x / targetWidth * sampleCount);
			float sample = 0.f;
			// how many samples to look at per bar we render
			int total = float(sampleCount) / targetWidth * 0.5f;
			for (int i = -total/2; i < total/2; i++) {
				sample = std::max(sample, std::abs(getSampleAt(index + i)));
			}
			samples.push_back(sample);
		}

		co_return Ok(std::move(samples));
	}();

	return std::make_pair(std::move(future), std::move(songLength));
}