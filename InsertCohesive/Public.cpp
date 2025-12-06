#include"Public.h"

std::string inpFolder = "InpFolder";
std::string OutputFolder = "OutputFolder";

void initFolders()
{
    std::ifstream file("config.json");
    if (!file.is_open()) {
        std::cout << "config.json not found, using default folders:\n"
            << "  inpFolder    = " << inpFolder << "\n"
            << "  OutputFolder = " << OutputFolder << "\n";
        return;
    }

    try {
        json j;
        file >> j;
        inpFolder = j.value("inpFolder", inpFolder);
        OutputFolder = j.value("OutputFolder", OutputFolder);
        std::cout << "Folders loaded from config.json:\n"
            << "  inpFolder    = " << inpFolder << "\n"
            << "  OutputFolder = " << OutputFolder << "\n";
    }
    catch (...) {
        std::cout << "config.json parse error, using defaults.\n";
    }
}



std::string extractElsetName(const std::string& line) {
    size_t pos = line.find("ELSET=");
    if (pos == std::string::npos) return {};

    size_t start = pos + 6; // 跳过 "ELSET="
    // 名称以 逗号/空白/行尾 结束
    size_t end = line.find_first_of(", \t\r\n", start);
    std::string name = (end == std::string::npos)
        ? line.substr(start)
        : line.substr(start, end - start);

    // 去除首尾空白（保险）
    auto l = name.find_first_not_of(" \t\r\n");
    auto r = name.find_last_not_of(" \t\r\n");
    if (l == std::string::npos) return {};
    return name.substr(l, r - l + 1);
}
// 功能：去除首尾空格，替换逗号为空格，并压缩多余空格
std::string normalizeInpLine(const std::string& line) {
    std::string result = line;

    // 1. 去掉首尾空白
    auto start = result.find_first_not_of(" \t\r\n");
    if (start == std::string::npos)
        return ""; // 全空行
    auto end = result.find_last_not_of(" \t\r\n");
    result = result.substr(start, end - start + 1);

    // 2. 逗号统一转为空格
    std::replace(result.begin(), result.end(), ',', ' ');

    // 3. 压缩连续空格为单一空格
    std::string cleaned;
    cleaned.reserve(result.size());
    bool inSpace = false;
    for (char c : result) {
        if (std::isspace(static_cast<unsigned char>(c))) {
            if (!inSpace) cleaned.push_back(' ');
            inSpace = true;
        }
        else {
            cleaned.push_back(c);
            inSpace = false;
        }
    }

    // 4. 再次去掉首尾空格（以防压缩后前后留空格）
    auto s = cleaned.find_first_not_of(' ');
    if (s == std::string::npos) return "";
    auto e = cleaned.find_last_not_of(' ');
    return cleaned.substr(s, e - s + 1);
}

int CountMatchingLabels(const std::vector<int>& SingleNodesOwnsElmentsLabel,
    const std::unordered_set<int>& MatchingElementLabels)
{
    int count = 0;
    // 计数SingleNodesOwnsElmentsLabel中的数有多少个在MatchingElementLabels中
    for (int label : SingleNodesOwnsElmentsLabel) {
        if (MatchingElementLabels.find(label) != MatchingElementLabels.end()) {
            ++count;
        }
    }

    return count; // 
}
std::vector<int> UnMatchingLabels(const std::vector<int>& SingleNodesOwnsElmentsLabel,
    const std::unordered_set<int>& MatchingElementLabels)
{
    //返回SingleNodesOwnsElmentsLabel中不在MatchingElementLabels中的元素的集合std::vector<int>
    std::vector<int> v; v.reserve(SingleNodesOwnsElmentsLabel.size());
    for (int label : SingleNodesOwnsElmentsLabel) {
        if (MatchingElementLabels.find(label) == MatchingElementLabels.end()) {
            v.push_back(label);
        }
    }
    return v; // 
}
void CreateCohesiveSets(OutputSet& outputSet, const std::vector<std::pair<std::string, std::string>>& SetInfo, const CohesiveIdentifyInfo& cohesiveIdentifyInfo) {
    std::unordered_map<int, std::string> elementToSet;
    for (const auto& [name, status] : SetInfo) {
        auto it = outputSet.find(name);
        if (it != outputSet.end()) {
            for (int elem : it->second) {
                elementToSet[elem] = name;
            }
        }
    }

    for (const auto& row : cohesiveIdentifyInfo) {
        if (row.size() != 3) continue;
        int coh_id = row[0];
        int ent1 = row[1];
        int ent2 = row[2];

        auto it1 = elementToSet.find(ent1);
        auto it2 = elementToSet.find(ent2);

        if (it1 == elementToSet.end() || it2 == elementToSet.end()) {
            continue; // Cases 3 or 4
        }

        std::string set1 = it1->second;
        std::string set2 = it2->second;

        if (set1 == set2) {
            // Case 1
            std::string newName = set1 + "_COH";
            outputSet[newName].insert(coh_id);
        }
        else {
            // Case 2: Determine order based on position in SetInfo
            size_t idx1 = static_cast<size_t>(-1);
            size_t idx2 = static_cast<size_t>(-1);
            for (size_t i = 0; i < SetInfo.size(); ++i) {
                if (SetInfo[i].first == set1) {
                    idx1 = i;
                }
                else if (SetInfo[i].first == set2) {
                    idx2 = i;
                }
            }
            if (idx1 != static_cast<size_t>(-1) && idx2 != static_cast<size_t>(-1)) {
                std::string prefix = (idx1 < idx2) ? set1 : set2;
                std::string suffix = (idx1 < idx2) ? set2 : set1;
                std::string newName = prefix + "_" + suffix + "_COH";
                outputSet[newName].insert(coh_id);
            }
        }
    }
}



void createBoundaryCohesiveSubsets(
    OutputSet& outputSet,
    const std::vector<std::string>& keys,
    const CohesiveIdentifyInfo& cohesiveIdentifyInfo, const std::string& InsertCohesiveSetName)
{
    if (keys.empty()) {
        std::cout << " No intersecting solid sets found.\n";
        return;
    }

    // 1. 获取 CohesiveBoundary 集合
 
    auto cohIt = outputSet.find(InsertCohesiveSetName + "_Cohesive_Boundary");
    if (cohIt == outputSet.end() || cohIt->second.empty()) {
        std::cout << " No CohesiveBoundary set found or empty.\n";
        return;
    }
    const ElementSetLabel& cohesiveBoundary = cohIt->second;

    // 3. 为每个 key 创建新的边界内聚力子集
    for (const std::string& key : keys) {
        const std::string newSetName = key + "_Boundary_coh";
        ElementSetLabel newCohesiveSet;

        // 遍历 cohesiveIdentifyInfo
        for (const auto& info : cohesiveIdentifyInfo) {
            if (info.size() != 3) continue;

            int cohId = info[0];
            int solid1 = info[1];
            int solid2 = info[2];

            // 必须是 CohesiveBoundary 中的单元
            if (cohesiveBoundary.find(cohId) == cohesiveBoundary.end()) continue;

            // 检查两个实体单元是否属于当前 key 集合
            const ElementSetLabel& targetSet = outputSet.at(key);  // 安全：keys 来自 outputSet
            bool inSolid1 = (targetSet.find(solid1) != targetSet.end());
            bool inSolid2 = (targetSet.find(solid2) != targetSet.end());

            // 任意一个在 key 中，则该内聚力单元属于新集合
            if (inSolid1 || inSolid2) {
                newCohesiveSet.insert(cohId);
            }
        }

        // 插入新集合（仅非空时）
        if (!newCohesiveSet.empty()) {
            outputSet[newSetName] = std::move(newCohesiveSet);
            std::cout << " Created set '" << newSetName << "' with "
                << outputSet[newSetName].size() << " elements.\n";
        }
        else {
            std::cout << " Warning: No cohesive elements for '" << newSetName << "', skipped.\n";
        }
    }
}