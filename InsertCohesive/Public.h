#pragma once
#include <stdio.h>
#include <vector>
#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <map>
#include <sstream>
#include <thread>
#include <algorithm>  // for std::max
#include <iomanip>  // for std::fixed, std::setprecision
#include <unordered_map>
#include <unordered_set>
#include <map>
#include <tuple>
#include <atomic>
#include <cctype>
#include <functional>
#include <mutex>
#include <queue>
#include <set>
#include <omp.h>
#include"Timer.h"
#include "nlohmann/json.hpp"
using json = nlohmann::json;
struct EN_ID {
    EN_ID(int element_id, int NewNode_id) :element_id(element_id), NewNode_id(NewNode_id) {}
    int element_id;  // 单元ID
    int NewNode_id;  // 新的节点的ID

};
using ElementSetLabel = std::unordered_set<int>;
using OutputSet = std::unordered_map<std::string, ElementSetLabel>;
using CohesiveIdentifyInfo = std::vector<std::vector<int>>;//内聚力单元的信息 n行3列 n代表有n个内聚力单元 第一列为内聚力单元的id 第二列为连接该内聚力单元的第一个实体单元的id 第三列为连接该内聚力单元的第二个实体单元的id 



extern std::string inpFolder;
extern std::string OutputFolder;


void initFolders();
std::string extractElsetName(const std::string& line);
// 功能：去除首尾空格，替换逗号为空格，并压缩多余空格
std::string normalizeInpLine(const std::string& line);

int CountMatchingLabels(const std::vector<int>& SingleNodesOwnsElmentsLabel,
    const std::unordered_set<int>& MatchingElementLabels);
std::vector<int> UnMatchingLabels(const std::vector<int>& SingleNodesOwnsElmentsLabel,
    const std::unordered_set<int>& MatchingElementLabels);

void CreateCohesiveSets(OutputSet& outputSet, const std::vector<std::pair<std::string, std::string>>& SetInfo, const CohesiveIdentifyInfo& cohesiveIdentifyInfo);


void createBoundaryCohesiveSubsets(
    OutputSet& outputSet,
    const std::vector<std::string>& keys,
    const CohesiveIdentifyInfo& cohesiveIdentifyInfo, const std::string& InsertCohesiveSetName);