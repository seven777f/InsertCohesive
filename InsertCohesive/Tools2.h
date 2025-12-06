#pragma once
#include <filesystem>
#include <fstream>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>
#include <map>
#include <regex>
#include <iostream>
#include "Tools.h"

std::unordered_map<int, COH3D6*> creatElementMap(std::vector<COH3D6>& elements);

bool read3DInpOutput(const std::string& filename, std::vector<Node3D>& nodes, std::vector<COH3D6>& elements,
    OutputSet& outputSet);

float GetCohesiveAreaByID(int id, const std::unordered_map<int, COH3D6*>& ElementMap);

void ComputeCohesiveAreas(const std::unordered_map<int, Node3D*>& node_map, std::vector<COH3D6>& originElements);

bool IsITZCohesiveElement(int id, const OutputSet& outputSet);

bool IsMortarCohesiveElement(int id, const OutputSet& outputSet);
// 创建所需的文件夹，如果有就删除内部内容
void prepareOutputFolder(const std::string& Outputfolder);
std::map<int, std::pair<std::unordered_set<int>, std::unordered_set<int>>> ProcessBrokenCohesiveElements(
    const std::string& ExtractSdegIdsFolder, const OutputSet& outputSet);

void WriteFrameBrokenElements(const std::string& outputFolder, const std::map<int, std::pair<ElementSetLabel, ElementSetLabel>>& frameBrokenElements);


void ComputeAndWriteCrackAreas(const std::map<int, std::pair<std::unordered_set<int>, std::unordered_set<int>>>& frameBrokenElements,
    const std::unordered_map<int, COH3D6*>& ElementMap,
    const std::string& outputFilePath);