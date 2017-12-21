//
// Copyright(c) 2017-present Iuri Dotta (dotta dot iuri at gmail dot com)
//
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.
//
// Official repository: https://github.com/idotta/edfio
//

#include <edfio/header/HeaderExam.hpp>
#include <edfio/core/Record.hpp>
#include <edfio/reader/ReaderHeader.hpp>
#include <edfio/reader/ReaderRecord.hpp>
#include <edfio/processor/ProcessorHeaderGeneral.hpp>
#include <edfio/processor/ProcessorHeaderSignal.hpp>
#include <edfio/processor/ProcessorHeaderExam.hpp>
#include <edfio/processor/ProcessorDataRecord.hpp>
#include <edfio/processor/ProcessorSignalRecord.hpp>
#include <edfio/processor/ProcessorAnnotationRecord.hpp>

#include <edfio/writer/WriterHeader.hpp>
#include <edfio/writer/WriterRecord.hpp>

#include <fstream>
#include <iterator>

int main()
{
	using namespace edfio;

	std::ifstream is("../sample/Calib5.edf", std::ios::binary);

	if (!is)
		return -1;

	HeaderExam header;

	// Read general fields
	ReaderHeaderGeneral readerGeneral;
	auto generalFields = readerGeneral(is);
	// Process general fields
	ProcessorHeaderGeneral processorGeneral;
	header.m_general = processorGeneral << std::move(generalFields);
	//header.m_general = processorGeneral << generalFields;

	// Read signal fields
	ReaderHeaderSignal readerSignals(header.m_general.m_totalSignals);
	auto signalFields = readerSignals(is);
	// Process signal fields
	ProcessorHeaderSignal processorSignals(header.m_general);
	header.m_signals = processorSignals << std::move(signalFields);
	//header.m_signals = processorSignals << signalFields;

	// Process exam header
	ProcessorHeaderExam processorExam(is);
	header = processorExam << std::move(header);

	is.seekg(header.m_general.m_headerSize, is.beg);

	// Read one data record (Calib5.edf only has one data record)
	size_t sz = header.m_general.m_detail.m_recordSize;
	ReaderRecord readerDataRecord(sz);
	auto recordField = readerDataRecord(is);
	// Process data record
	ProcessorDataRecord procDataRecord(header.m_signals, header.m_general.m_version);
	auto signalRecordFields = procDataRecord << recordField;

	// Process signal records
	std::vector<std::vector<int>> digitalData;
	std::vector<std::vector<double>> physicalData;
	std::vector<std::vector<Annotation>> annotationsData;
	for (size_t idx = 0; idx < signalRecordFields.size(); idx++)
	{
		if (!header.m_signals[idx].m_detail.m_isAnnotation)
		{
			ProcessorSignalRecord<SampleType::Digital> procDigital(header.m_signals[idx], header.m_general.m_version);
			auto digital = std::move(procDigital << signalRecordFields[idx]);
			digitalData.emplace_back(std::move(digital));

			ProcessorSignalRecord<SampleType::Physical> procPhysical(header.m_signals[idx], header.m_general.m_version);
			auto physical = std::move(procPhysical << signalRecordFields[idx]);
			physicalData.emplace_back(std::move(physical));
		}
		else
		{
			ProcessorAnnotationRecord procAnnot;
			auto annotations = std::move(procAnnot << signalRecordFields[idx]);
			annotationsData.emplace_back(std::move(annotations));
		}

	}

	std::ofstream os("../sample/test.edf", std::ios::binary);
	
	// Process general fields
	generalFields = std::move(processorGeneral >> header.m_general);
	// Write general fields
	WriterHeaderGeneral writerGeneral;
	writerGeneral(os, generalFields);
	
	// Process signal fields
	signalFields = std::move(processorSignals >> header.m_signals);
	// Write signal fields
	WriterHeaderSignal writerSignals;
	writerSignals(os, signalFields);

	// Process signal records
	signalRecordFields.clear();
	size_t idxSignal = 0, idxAnnotation = 0;
	for (size_t idx = 0; idx < header.m_signals.size(); idx++)
	{
		if (!header.m_signals[idx].m_detail.m_isAnnotation)
		{
			ProcessorSignalRecord<SampleType::Digital> procDigital(header.m_signals[idx], header.m_general.m_version);
			auto digital = std::move(procDigital >> digitalData[idxSignal++]);
			signalRecordFields.emplace_back(std::move(digital));

			/*ProcessorSignalRecord<SampleType::Physical> procPhysical(header.m_signals[idx], header.m_general.m_version);
			auto physical = std::move(procPhysical >> physicalData[idx]);
			physicalData.emplace_back(std::move(physical));*/
		}
		else
		{
			ProcessorAnnotationRecord procAnnot(header.m_signals[idx].m_samplesInDataRecord * edfio::GetSampleBytes(header.m_general.m_version));
			//auto annotations = std::move(procAnnot << signalRecordFields[idx]);
			auto annotations = std::move(procAnnot >> annotationsData[idxAnnotation++]);
			signalRecordFields.emplace_back(std::move(annotations));
		}
	}
	// Write data record
	WriterRecord writerDataRecord(sz);
	//writerDataRecord(os, recordField);
	for (auto &r : signalRecordFields)
	{
		writerDataRecord(os, r);
	}

	return 0;
}
