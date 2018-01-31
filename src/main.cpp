//
// Copyright(c) 2017-present Iuri Dotta (dotta dot iuri at gmail dot com)
//
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.
//
// Official repository: https://github.com/idotta/edfioProj
//

#include "edfio/EdfIO.hpp"

#include <chrono>

void TestReading();
void TestWriting();

int main()
{
	//TestReading();

	TestWriting();

	getchar();

	return 0;
}

void TestReading()
{
	using namespace edfio;

	std::ifstream is("../../sample/Calib5.edf", std::ios::binary);
	//std::ifstream is("testSignalRecord.edf", std::ios::binary);

	if (!is)
		return;

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

	std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();

	std::cout << "Processing whole file through Datarecords, Signalrecords, Samples and Timestamps took "
		<< std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
		<< " ms.\n";
}

void TestWriting()
{
	using namespace edfio;

	std::ifstream is("../../sample/Calib5.edf", std::ios::binary);

	if (!is)
		return;

	std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();

	// DataRecord
	{
		std::ofstream os("testDataRecord.edf", std::ios::binary);
		if (!os)
			return;

		auto header = std::move(ReaderHeaderExam{}(is));

		WriterHeaderExam{}(os, header);

		auto sink = detail::CreateDataRecordSink(os, header.m_general);
		auto oit = sink.end();

		// DataRecordStore
		auto store = std::move(detail::CreateDataRecordStore(is, header.m_general));
		auto it = store.begin();
		oit = *it;
		header.m_general.m_datarecordsFile++;
		oit = *it;
		header.m_general.m_datarecordsFile++;
		oit = *it;
		WriterHeaderExam{}(os, header);

		// Rewrite 2nd datarecord
		auto data = *it;
		for (auto x = data().begin(); x != data().end() - data().size()/2; x++)
			*x = std::rand() % 100;
		oit -= 3;
		oit = data;
	}

	// SignalRecord
	{
		std::ofstream os("testSignalRecord.edf", std::ios::binary);
		if (!os)
			return;
		
		auto header = std::move(ReaderHeaderExam{}(is));

		WriterHeaderExam{}(os, header);

		std::vector<SignalRecordStore> stores;
		std::vector<SignalRecordStore::iterator> itStores;
		for (auto &signal : header.m_signals)
		{
			stores.emplace_back(std::move(detail::CreateSignalRecordStore(is, header.m_general, signal)));
		}
		for (auto &store : stores)
		{
			itStores.push_back(store.begin());
		}

		size_t idxStore = 0;
		header.m_general.m_datarecordsFile = 0;
		for (size_t count = 0; count < 5; count++)
		{
			for (auto &signal : header.m_signals)
			{
				auto sink = detail::CreateSignalRecordSink(os, header.m_general, signal);
				auto oit = sink.end();

				auto it = itStores[idxStore];
				if (it != stores[idxStore].end())
					oit = *it;

				idxStore++;
				idxStore %= stores.size();
			}
			header.m_general.m_datarecordsFile++;
		}

		// Rewrite 2nd signalrecord from 1st signal
		auto sink1 = detail::CreateSignalRecordSink(os, header.m_general, header.m_signals[0]);
		auto oit = sink1.begin() + 1;
		auto data1 = *itStores[0];
		for (auto &x : data1())
			x = std::rand() % 100;
		oit = data1;

		// Rewrite 4th signalrecord from 2nd signal
		auto sink2 = detail::CreateSignalRecordSink(os, header.m_general, header.m_signals[1]);
		oit = sink2.begin() + 3;
		auto data2 = *itStores[1];
		for (auto &x : data2())
			x = std::rand() % 100;
		oit = data2;


		WriterHeaderExam{}(os, header);
	}

	std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();

	std::cout << "Writing process took "
		<< std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
		<< " ms.\n";
}
