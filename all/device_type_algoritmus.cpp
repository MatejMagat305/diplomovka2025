#include <chrono>
#include "device_type_algoritmus.h"
#include "compute.h"


namespace internal {
	static int countStepProcessor = 0, countStepGraphicCard = 0;
}

inline void addInfo(Info& toAdd, Info& add) {
	toAdd.timeRun += add.timeRun;
	toAdd.timeSynchronize += add.timeSynchronize;
}

void checksynchronizeGPU(Map& m, Info& result) {
	if (internal::countStepProcessor > internal::countStepGraphicCard) {
		internal::countStepGraphicCard = internal::countStepProcessor;
		auto start_time = std::chrono::high_resolution_clock::now();
		synchronizeGPUFromCPU(m);
		auto end_time = std::chrono::high_resolution_clock::now();
		result.timeSynchronize += std::chrono::duration<double>(end_time - start_time).count();
	}
	internal::countStepGraphicCard++;
}

void checksynchronizeCPU(Map& m, Info& result) {
	if (internal::countStepProcessor < internal::countStepGraphicCard) {
		internal::countStepProcessor = internal::countStepGraphicCard;
		auto start_time = std::chrono::high_resolution_clock::now();
		synchronizeGPUFromCPU(m);
		auto end_time = std::chrono::high_resolution_clock::now();
		result.timeSynchronize += std::chrono::duration<double>(end_time - start_time).count();
	}
	internal::countStepProcessor++;
}

Info runOnGraphicCard(AlgorithmType algo, Map& m){
	Info result{0,0};
	checksynchronizeGPU(m, result);
	Info resultCompute = computeSYCL(algo, m);
	addInfo(result, resultCompute);
	auto start_time = std::chrono::high_resolution_clock::now();
	synchronizeCPUFromGPU(m);
	auto end_time = std::chrono::high_resolution_clock::now();
	result.timeSynchronize += std::chrono::duration<double>(end_time - start_time).count();
	internal::countStepProcessor = internal::countStepGraphicCard;
	return result;
}

Info runHybrid(AlgorithmType algo, Map& m) {
	Info result{ 0,0 };
	checksynchronizeGPU(m, result);
	Info resultCompute = computeHYBRID(algo, m);
	addInfo(result, resultCompute);
	auto start_time = std::chrono::high_resolution_clock::now();
	synchronizeCPUFromGPU(m);
	auto end_time = std::chrono::high_resolution_clock::now();
	result.timeSynchronize += std::chrono::duration<double>(end_time - start_time).count();
	internal::countStepProcessor = internal::countStepGraphicCard;
	return result;
}

Info runPure(AlgorithmType algo, Map& m){
	Info result{ 0,0 };
	checksynchronizeCPU(m, result);
	Info resultCompute = compute_cpu_primitive(algo, m);
	addInfo(result, resultCompute);
	return result;
}

Info runPureOneThread(AlgorithmType algo, Map& m) {
	Info result{ 0,0 };
	checksynchronizeCPU(m, result);
	Info resultCompute = compute_cpu_primitive_one_thread(algo, m);
	addInfo(result, resultCompute);
	return result;
}

Info runHigh(AlgorithmType algo, Map& m){
	Info result{ 0,0 };
	checksynchronizeCPU(m, result);
	Info resultCompute = computeCPU(algo, m);
	addInfo(result, resultCompute);
	return result;
}

Info letCompute(AlgorithmType algo, ComputeType device, Map& m) {
	switch (device)	{
	case ComputeType::pureGraphicCard:
		return runOnGraphicCard(algo, m);
	case ComputeType::hybridGPUCPU:
		return runHybrid(algo, m);
	case ComputeType::pureProcesor:
		return runPure(algo, m );
	case ComputeType::pureProcesorOneThread:
		return runPureOneThread(algo, m);
	case ComputeType::highProcesor:
		return runHigh(algo, m);
	default:
		break;
	}
	return runHigh(algo, m);
}
