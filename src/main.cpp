//
// Copyright(c) 2017-present Iuri Dotta (dotta dot iuri at gmail dot com)
//
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.
//
// Official repository: https://github.com/idotta/edfioProj
//

#include <edfio/header/HeaderExam.hpp>
#include <edfio/reader/ReaderHeaderExam.hpp>

#include <edfio/store/DataRecordStore.hpp>
#include <edfio/store/SignalRecordStore.hpp>
#include <edfio/store/SignalSampleStore.hpp>
#include <edfio/store/detail/StoreUtils.hpp>
#include <edfio/store/TalStore.hpp>
#include <edfio/store/TimeStampStore.hpp>

#include <edfio/processor/ProcessorSampleRecord.hpp>
#include <edfio/processor/ProcessorTimeStampRecord.hpp>
#include <edfio/processor/ProcessorTalRecord.hpp>
#include <edfio/processor/ProcessorAnnotation.hpp>
#include <edfio/processor/ProcessorTimeStamp.hpp>
#include <edfio/processor/ProcessorSample.hpp>

#include <fstream>
#include <chrono>

int main()
{
	using namespace edfio;

	std::ifstream is("../../sample/Calib5.edf", std::ios::binary);

	if (!is)
		return -1;

	std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();

	auto header = std::move(ReaderHeaderExam{}(is));

	// DataRecordStore
	auto drs = std::move(detail::CreateDataRecordStore(is, header.m_general));
	for (auto dr : drs)
	{
		auto data(dr);
	}

	for (auto signal : header.m_signals)
	{
		if (!signal.m_detail.m_isAnnotation)
		{
			// SignalRecordStore
			auto srs = std::move(detail::CreateSignalRecordStore(is, header.m_general, signal));
			for (auto sr : srs)
			{
				auto data(sr);
			}

			// SignalSampleStore
			auto sss = std::move(detail::CreateSignalSampleStore(is, header.m_general, signal));
			ProcessorSampleRecord<SampleType::Physical> procSampleRecPhys(signal.m_detail.m_offset, signal.m_detail.m_scaling);
			ProcessorSample<SampleType::Physical> procSamplePhys(signal.m_detail.m_offset, signal.m_detail.m_scaling, GetSampleBytes(header.m_general.m_version));
			for (auto sr : sss)
			{
				auto data = procSampleRecPhys(sr);
				auto rec = procSamplePhys(data);
				int a = 0;
			}
		}
		else // only 1 annotation signal in this exam, if there are more, the timestamp is on the 1st EDF Annotations signal
		{
			// TimeStampStore
			auto tss = detail::CreateTimeStampStore(is, header.m_general, signal);
			long long datarecord = 0;
			ProcessorTimeStampRecord procTimestamp;
			for (auto ts : tss)
			{
				auto data = procTimestamp(ts, datarecord++);
				int a = 0;
			}

			// SignalRecordStore && TalStore
			auto srs = std::move(detail::CreateSignalRecordStore(is, header.m_general, signal));
			datarecord = 0;
			ProcessorTalRecord procTal;
			for (auto itSr = srs.begin(); itSr != srs.end(); itSr++)
			{
				//auto talStream = *itSr;
				TalStore tals(*itSr);
				for (auto tal : tals)
				{
					auto annotations = std::move(procTal(tal, datarecord));
					for (auto &annot : annotations)
					{
						//std::cout << annot.m_dararecord << " | " << annot.m_start << " | " << annot.m_duration << " | " << annot.m_annotation << std::endl;
					}
				}
				datarecord++;
			}
		}
	}

	// ProcessorAnnotation
	Annotation test;
	test.m_annotation = "My first test";
	test.m_dararecord = 0;
	test.m_duration = 10;
	test.m_start = -101.1;
	ProcessorAnnotation procAnnotation;
	auto recAnnotation = procAnnotation(test);

	// ProcessorTimeStamp
	TimeStamp test2;
	test2.m_dararecord = 0;
	test2.m_start = 101.1;
	ProcessorTimeStamp procTimeStamp;
	auto recTS = procTimeStamp(test2);

	std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();

	std::cout << "Processing whole file through Datarecords, Signalrecords, Samples and Timestamps took "
		<< std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
		<< " ms.\n";

	getchar();

	return 0;
}
