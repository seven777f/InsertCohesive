#include "Tools2.h"


std::unordered_map<int, COH3D6*> creatElementMap(std::vector<COH3D6>& elements) {
    std::unordered_map<int, COH3D6*> elem_map;
    elem_map.reserve(elements.size());
    for (auto& elem : elements) {
        //  elem_map[elem.id] = &elem;
        elem_map.emplace(elem.id, &elem);  // 直接构造键值对，无多余操作
    }
    return elem_map;
}

bool read3DInpOutput(const std::string& filename, std::vector<Node3D>& nodes, std::vector<COH3D6>& elements,
    OutputSet& outputSet) {



    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open file " << filename << std::endl;
        return false;
    }

    std::string line;
    bool node_section = false;
    bool element_section = false;
    bool elset_section = false;
    bool current_elset_generate = false; // 新增标志
    std::string current_elset = "";

    nodes.clear();
    elements.clear();
    outputSet.clear(); // 清除已有数据

    while (std::getline(file, line)) {
        // 去除首尾空白
        size_t start = line.find_first_not_of(" \t");
        if (start == std::string::npos) continue; // 空行
        line = line.substr(start);
        size_t end = line.find_last_not_of(" \t");
        if (end != std::string::npos) line = line.substr(0, end + 1);
        std::transform(line.begin(), line.end(), line.begin(), ::toupper);//统一转换为大写

        // 跳过空行
        if (line.empty())  continue;

        // 跳过注释行或空行
        if (line[0] == '*') {
            if (line.find("*NODE") != std::string::npos) {
                node_section = true;
                element_section = elset_section = false;
                continue;
            }
            if (line.find("*ELEMENT,TYPE=COH3D6") != std::string::npos || line.find("*ELEMENT, TYPE=COH3D6") != std::string::npos) {
                element_section = true;
                node_section = elset_section = false;
                continue;
            }
            // 解析任意 *ELSET

            if (line.find("*ELSET, ELSET=") != std::string::npos) {
                //  检查是否有 generate
                current_elset_generate = (line.find("GENERATE") != std::string::npos);

                current_elset = extractElsetName(line); // 得到 "AGGREGATE"
                if (current_elset == "COHESIVEINNER" || current_elset == "COHESIVEBOUNDARY") {
                    elset_section = true;
                    node_section = element_section = false;
                    continue;
                }
              
            }
            // 结束 ELSET 或其他节
            if (line[0] == '*' && elset_section) {
                elset_section = false;
                current_elset.clear();
            }
            node_section = element_section = false;
            continue;
        }

        // 读取节点
        if (node_section && line[0] != '*' && line[0] != '#') {
            std::replace(line.begin(), line.end(), ',', ' ');
            std::istringstream ss(line);
            Node3D node;
            if (!(ss >> node.id >> node.x >> node.y >> node.z)) {
                std::cerr << "Warning: Invalid node line: " << line << std::endl;
                continue;
            }
            nodes.push_back(node);
        }

        // 读取单元
        if (element_section && line[0] != '*' && line[0] != '#') {
            std::replace(line.begin(), line.end(), ',', ' ');
            std::istringstream ss(line);
            COH3D6 elem;
            if (!(ss >> elem.id >> elem.n1 >> elem.n2 >> elem.n3 >> elem.n4 >> elem.n5 >> elem.n6)) {
                std::cerr << "Warning: Invalid element line: " << line << std::endl;
                continue;
            }
            elements.push_back(elem);
            //outputSet["BASEELEMENTS"].insert(elem.id);
        }

        // 读取 ELSET 数据
        if (elset_section && !current_elset.empty() && line[0] != '*' && line[0] != '#') {
           
            std::string cleaned = normalizeInpLine(line);
            if (cleaned.empty()) continue;

            std::istringstream iss(cleaned);

            if (current_elset_generate) {

                int start, end, step;
                if (iss >> start >> end >> step) {


                    for (int i = start; i <= end; i += step) {
                        outputSet[current_elset].insert(i);

                    }
                    // std::cout << outputSet.size() << std::endl;
                }
                else {
                    std::cerr << "Warning: invalid GENERATE line in ELSET "
                        << current_elset << ": " << line << std::endl;
                }
            }
            else {
                int id;
                while (iss >> id) {
                    outputSet[current_elset].insert(id);
                }
            }

        }
    }

    if (file.is_open()) file.close();
    if (nodes.empty()) {
        std::cerr << "Error: No nodes found." << std::endl;
        return false;
    }
    if (elements.empty()) {
        std::cerr << "Error: No elements found." << std::endl;
        return false;
    }


    if (outputSet.empty()) {
        std::cerr << "Error: No element sets found." << std::endl;
        return false;
    }

    std::cout << "Read " << nodes.size() << " nodes and " << elements.size()
        << " COH3D6 elements" << std::endl;
    for (const auto& [name, set] : outputSet) {
        std::cout << "Read element set " << name << " with " << set.size() << " labels." << std::endl;
    }
    return true;
}

float GetCohesiveAreaByID(int id, const std::unordered_map<int, COH3D6*>& ElementMap) {
    auto it1 = ElementMap.find(id);

    if (it1 != ElementMap.end()) {
        return it1->second->area;
    }
    return 0.0f;

   
}
bool IsITZCohesiveElement(int id, const OutputSet& outputSet) {
  
    auto it = outputSet.find("COHESIVEBOUNDARY");
    if (it == outputSet.end()) {
        return false;
    }
    const std::unordered_set<int>& set = it->second;
    return set.count(id) > 0;
}
bool IsMortarCohesiveElement(int id, const OutputSet& outputSet) {
  
    auto it = outputSet.find("COHESIVEINNER");
    if (it == outputSet.end()) {
        return false;
    }
    const std::unordered_set<int>& set = it->second;
    return set.count(id) > 0;
}
void ComputeCohesiveAreas(const std::unordered_map<int, Node3D*>& node_map, std::vector<COH3D6>& originElements) {
    for (auto& elem : originElements) {
        auto it1 = node_map.find(elem.n1);
        auto it2 = node_map.find(elem.n2);
        auto it3 = node_map.find(elem.n3);

        if (it1 == node_map.end() || it2 == node_map.end() || it3 == node_map.end()) {
            elem.area = 0.0f;
            continue;
        }

        const Node3D* A = it1->second;
        const Node3D* B = it2->second;
        const Node3D* C = it3->second;

        // Vector AB
        float abx = B->x - A->x;
        float aby = B->y - A->y;
        float abz = B->z - A->z;

        // Vector AC
        float acx = C->x - A->x;
        float acy = C->y - A->y;
        float acz = C->z - A->z;

        // Cross product AB × AC
        float cx = aby * acz - abz * acy;
        float cy = abz * acx - abx * acz;
        float cz = abx * acy - aby * acx;

        // Magnitude of cross product
        float mag = std::sqrt(cx * cx + cy * cy + cz * cz);

        // Area = 0.5 * magnitude
        elem.area = 0.5f * mag;
        //std::cout << elem.area << std::endl;
    }
}


std::map<int, std::pair<std::unordered_set<int>, std::unordered_set<int>>> ProcessBrokenCohesiveElements(
    const std::string& ExtractSdegIdsFolder, const OutputSet& outputSet) {

    std::map<int, std::pair<std::unordered_set<int>, std::unordered_set<int>>> frameBrokenElements;

    // Iterate over all files in the directory
    for (const auto& entry : std::filesystem::directory_iterator(ExtractSdegIdsFolder)) {
        if (!entry.is_regular_file()) continue;

        std::string filename = entry.path().filename().string();

        // Parse frame number from filename like "sdeg_ids__CONCRETE-1.COHESIVEELEMENTS__frame37.txt"
        std::regex frameRegex(R"(frame(\d+)\.txt$)");
        std::smatch match;
        if (!std::regex_search(filename, match, frameRegex)) {
            continue; // Skip files without frame number
        }

        int frameNum = std::stoi(match[1].str());

        // Read the file
        std::ifstream file(entry.path());
        if (!file.is_open()) {
            std::cerr << "Warning: Could not open file " << filename << std::endl;
            continue;
        }

        std::unordered_set<int> mortarBroken;
        std::unordered_set<int> itzBroken;

        std::string line;
        while (std::getline(file, line)) {
            // Trim whitespace if needed
            line.erase(0, line.find_first_not_of(" \t"));
            line.erase(line.find_last_not_of(" \t") + 1);

            if (line.empty()) continue;

            // Parse ID from "CONCRETE-1.{id}"
            size_t dotPos = line.rfind('.');
            if (dotPos == std::string::npos || dotPos < 10) continue; // Invalid format

            std::string idStr = line.substr(dotPos + 1);
            try {
                int id = std::stoi(idStr);

                // Classify the element
                if (IsITZCohesiveElement(id, outputSet)) {
                    itzBroken.insert(id);
                }
                else if (IsMortarCohesiveElement(id, outputSet)) {
                    mortarBroken.insert(id);
                }
                // Skip if neither
            }
            catch (const std::exception& e) {
                std::cerr << "Warning: Invalid ID in " << filename << ": " << idStr << std::endl;
            }
        }

        if (!mortarBroken.empty() || !itzBroken.empty()) {
            frameBrokenElements[frameNum] = { std::move(mortarBroken), std::move(itzBroken) };
        }

        file.close();
    }

    return frameBrokenElements;
}

// 创建所需的文件夹，如果有就删除内部内容
void prepareOutputFolder(const std::string& Outputfolder) {
    namespace fs = std::filesystem;

    if (fs::exists(Outputfolder)) {
        // 删除文件夹中的所有内容
        for (const auto& entry : fs::directory_iterator(Outputfolder)) {
            fs::remove_all(entry);
        }
    }
    else {
        // 递归创建文件夹
        if (!fs::create_directories(Outputfolder)) {
            std::cerr << "无法创建输出文件夹：" << Outputfolder << std::endl;
        }
    }
}

void WriteFrameBrokenElements(const std::string& outputFolder, const std::map<int, std::pair<ElementSetLabel, ElementSetLabel>>& frameBrokenElements) {
    // Ensure the output folder exists (assuming prepareOutputFolder already handles this, but just in case)
    // std::filesystem::create_directories(outputFolder);

    const std::string prefix = "";

    if (frameBrokenElements.empty()) {
        return;
    }

    int maxFrame = frameBrokenElements.rbegin()->first;

    for (int frame = 0; frame <= maxFrame; ++frame) {
        auto it = frameBrokenElements.find(frame);
        const auto& mortarBroken = (it != frameBrokenElements.end()) ? it->second.first : ElementSetLabel{};
        const auto& itzBroken = (it != frameBrokenElements.end()) ? it->second.second : ElementSetLabel{};

        // Write mortar broken elements (always create file, even if empty)
        std::string mortarFile = outputFolder + "/sdeg_ids_mortar_frame" + std::to_string(frame) + ".txt";
        std::ofstream mortarOut(mortarFile);
        for (int id : mortarBroken) {
            mortarOut << prefix << id << std::endl;
        }
        mortarOut.close();

        // Write ITZ broken elements (always create file, even if empty)
        std::string itzFile = outputFolder + "/sdeg_ids_itz_frame" + std::to_string(frame) + ".txt";
        std::ofstream itzOut(itzFile);
        for (int id : itzBroken) {
            itzOut << prefix << id << std::endl;
        }
        itzOut.close();
    }
}

void ComputeAndWriteCrackAreas(const std::map<int, std::pair<std::unordered_set<int>, std::unordered_set<int>>>& frameBrokenElements,
    const std::unordered_map<int, COH3D6*>& ElementMap,
    const std::string& outputFilePath) {
    if (frameBrokenElements.empty()) {
        return;
    }

    int maxFrame = frameBrokenElements.rbegin()->first;

    std::ofstream outFile(outputFilePath);
    if (!outFile.is_open()) {
        std::cerr << "Error: Could not open output file " << outputFilePath << std::endl;
        return;
    }

    outFile << std::fixed << std::setprecision(6);

    for (int frame = 0; frame <= maxFrame; ++frame) {
        auto it = frameBrokenElements.find(frame);
        double mortarArea = 0.0;
        double itzArea = 0.0;

        if (it != frameBrokenElements.end()) {
            const auto& mortarSet = it->second.first;
            for (int id : mortarSet) {
                auto elemIt = ElementMap.find(id);
                if (elemIt != ElementMap.end()) {
                    mortarArea += elemIt->second->area;
                }
            }

            const auto& itzSet = it->second.second;
            for (int id : itzSet) {
                auto elemIt = ElementMap.find(id);
                if (elemIt != ElementMap.end()) {
                    itzArea += elemIt->second->area;
                }
            }
        }

        outFile << frame << " " << mortarArea << " " << itzArea << "\n";
    }

    outFile.close();
    std::cout << "Crack areas written to " << outputFilePath << std::endl;
}