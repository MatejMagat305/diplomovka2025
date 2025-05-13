#include <chrono>
#include "device_type_algoritmus.h"
#include "compute.h"


namespace internal {
	static int countStepProcessor = 0, countStepGraphicCard = 0;
}

inline Info addInfo(Info toAdd, Info add) {
	Info result{ 0,0, ""};
	result.timeRun = toAdd.timeRun + add.timeRun;
	result.timeSynchronize = toAdd.timeSynchronize + add.timeSynchronize;
	return result;
}

Info checksynchronizeGPU(Map* m) {
	Info result{ 0,0, ""};
	if (internal::countStepProcessor > internal::countStepGraphicCard) {
		internal::countStepGraphicCard = internal::countStepProcessor;
		auto start_time = std::chrono::high_resolution_clock::now();
		synchronizeGPUFromCPU(m);
		auto end_time = std::chrono::high_resolution_clock::now();
		result.timeSynchronize += std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();
	}
	internal::countStepGraphicCard++;
	return result;
}

Info checksynchronizeCPU(Map* m) {
	Info result{ 0,0, ""};
	if (internal::countStepProcessor < internal::countStepGraphicCard) {
		internal::countStepProcessor = internal::countStepGraphicCard;
		auto start_time = std::chrono::high_resolution_clock::now();
		synchronizeGPUFromCPU(m);
		auto end_time = std::chrono::high_resolution_clock::now();
		result.timeSynchronize += std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();
	}
	internal::countStepProcessor++;
	return result;
}

Info runOnGraphicCard(Map* m){
	Info result = checksynchronizeGPU(m);
	Info resultCompute = computeSYCL(m);
	addInfo(result, resultCompute);
	auto start_time = std::chrono::high_resolution_clock::now();
	synchronizeCPUFromGPU(m);
	result = addInfo(result, resultCompute);
	auto end_time = std::chrono::high_resolution_clock::now();
	if (m->CPUMemory.minSize_maxtimeStep_error[ERROR] != 0) {
		result.error = "Memory error in GPU computation in agent: " + std::to_string(m->CPUMemory.minSize_maxtimeStep_error[ERROR]);
		return result;
	}
	result.timeSynchronize += std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();
	internal::countStepProcessor = internal::countStepGraphicCard;
	return result;
}

Info runHybrid(Map* m) {
	Info result = checksynchronizeGPU(m);
	Info resultCompute = computeHYBRID(m);
	result = addInfo(result, resultCompute);
	if (m->CPUMemory.minSize_maxtimeStep_error[ERROR] != 0) {
		result.error = "Memory error in GPU computation in agent: " + std::to_string(m->CPUMemory.minSize_maxtimeStep_error[ERROR]);
	}
	return result;
}

Info runPure(Map* m){
	Info result = checksynchronizeCPU(m);
	Info resultCompute = compute_cpu_primitive(m);
	result = addInfo(result, resultCompute);
	if (m->CPUMemory.minSize_maxtimeStep_error[ERROR] != 0) {
		result.error = "Memory error in GPU computation in agent: " + std::to_string(m->CPUMemory.minSize_maxtimeStep_error[ERROR]);
	}
	return result;
}

Info runPureOneThread(Map* m) {
	Info result = checksynchronizeCPU(m);
 	Info resultCompute = compute_cpu_primitive_one_thread(m);
	result = addInfo(result, resultCompute);
	if (m->CPUMemory.minSize_maxtimeStep_error[ERROR] != 0) {
		result.error = "Memory error in GPU computation in agent: " + std::to_string(m->CPUMemory.minSize_maxtimeStep_error[ERROR]) ;
	}
	return result;
}

Info runHigh(Map *m){
	Info result = checksynchronizeCPU(m);
	Info resultCompute = computeCPU(m);
	return addInfo(result, resultCompute);
}

Info letCompute(ComputeType device, Map* m) {
	switch (device)	{
	case ComputeType::pureGraphicCard:
		return runOnGraphicCard(m);
	case ComputeType::hybridGPUCPU:
		return runHybrid(m);
	case ComputeType::pureProcesor:
		return runPure(m);
	case ComputeType::pureProcesorOneThread:
		return runPureOneThread(m);
	case ComputeType::highProcesor:
		return runHigh(m);
	default:
		break;
	}
	return runHigh(m);
}
