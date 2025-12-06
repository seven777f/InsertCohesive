#include "Tools.h"


static int maxElemID2=1;//C3D4单元的最大单元label编号  （因为新插入的内聚力单元的ID都是maxElemID++ 所以用此区分是C3D4单元函数 COH3D6单元）
static ElementSetLabel OutsideTargetElementLabels;
static std::string PartName = "";








bool read3DInp(const std::string& filename, std::vector<Node3D>& nodes, std::vector<Element3D>& elements,
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
        if ( line[0] == '*') {
            if (line.find("*PART, NAME=") != std::string::npos) {
                size_t pos = line.find("NAME=");
                if (pos != std::string::npos) {
                    pos += 5;  // skip "NAME="
                    size_t start = line.find_first_not_of(" \t", pos);
                    if (start != std::string::npos) {
                        size_t end = line.find_first_of(", \t", start);
                        PartName = line.substr(start, end == std::string::npos ? end : end - start);
                        std::cout << "Reading Part:  " << PartName << std::endl;
                    }
                }
            }
            if (line.find("*NODE") != std::string::npos ) {
                node_section = true;
                element_section = elset_section = false;
                continue;
            }
            if (line.find("*ELEMENT,TYPE=C3D4") != std::string::npos || line.find("*ELEMENT, TYPE=C3D4") != std::string::npos) {
                element_section = true;
                node_section = elset_section = false;
                continue;
            }
            // 解析任意 *ELSET
           
            if (line.find("*ELSET, ELSET=") != std::string::npos ) {
                //  检查是否有 generate
                current_elset_generate = (line.find("GENERATE") != std::string::npos);

                current_elset = extractElsetName(line); // 得到 "AGGREGATE"
               
                elset_section = true;
                node_section = element_section = false;
                continue;
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
            Element3D elem;
            if (!(ss >> elem.id >> elem.n1 >> elem.n2 >> elem.n3 >> elem.n4)) {
                std::cerr << "Warning: Invalid element line: " << line << std::endl;
                continue;
            }
            elements.push_back(elem);
            outputSet["BASEELEMENTS"].insert(elem.id);
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
        << " C3D4 elements" << std::endl;
    for (const auto& [name, set] : outputSet) {
        std::cout << "Read element set " << name << " with " << set.size() << " labels." << std::endl;
    }
    return true;
}







bool read3DInpFromHyperMesh(const std::string& filename, std::vector<Node3D>& nodes, std::vector<Element3D>& elements,
    OutputSet& outputSet) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open file " << filename << std::endl;
        return false;
    }

    std::string line;
    bool node_section = false;
    bool element_section = false;
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

        // 跳过注释行或空行
        if (line.empty() || line[0] == '*' || line[0] == '#') {
            if (line.find("*NODE") != std::string::npos) {
                node_section = true;
                element_section = false;
                continue;
            }
            if (line.find("*ELEMENT,TYPE=C3D4") != std::string::npos) {
                // 替换逗号为空格以便按参数分隔
                std::string parse_line = line;
                std::replace(parse_line.begin(), parse_line.end(), ',', ' ');
                std::istringstream ss(parse_line);
                std::string token;
                while (ss >> token) {
                    if (token.find("ELSET=") != std::string::npos) {
                        current_elset = token.substr(6); // 提取 ELSET= 后的名称
                        break;
                    }
                }
                element_section = true;
                node_section = false;
                continue;
            }
            // 结束 element_section
            if (line[0] == '*' && element_section) {
                element_section = false;
                current_elset.clear();
            }
            node_section = false;
            continue;
        }

        // 读取节点
        if (node_section) {
            std::replace(line.begin(), line.end(), ',', ' ');
            std::istringstream ss(line);
            Node3D node;
            if (!(ss >> node.id >> node.x >> node.y >> node.z)) {
                std::cerr << "Warning: Invalid node line: " << line << std::endl;
                continue;
            }
            nodes.push_back(node);
        }

        // 读取单元（从 ELSET 块中收集）
        if (element_section && !current_elset.empty()) {
            
            std::replace(line.begin(), line.end(), ',', ' ');
            std::istringstream ss(line);
            Element3D elem;
            if (!(ss >> elem.id >> elem.n1 >> elem.n2 >> elem.n3 >> elem.n4)) {
                std::cerr << "Warning: Invalid element line: " << line << std::endl;
                continue;
            }
            elements.push_back(elem);
            outputSet[current_elset].insert(elem.id); // 记录到对应 ELSET
        }
    }

    if (file.is_open()) file.close();
    if (nodes.empty() ) {
        std::cerr << "Error: No nodes found." << std::endl;
        return false;
    }
    if (elements.empty() ) {
        std::cerr << "Error: No elements found." << std::endl;
        return false;
    }if ( outputSet.empty()) {
        std::cerr << "Error: No element sets found." << std::endl;
        return false;
    }

    std::cout << "Read " << nodes.size() << " nodes and " << elements.size()
        << " C3D4 elements." << std::endl;
    for (const auto& [name, set] : outputSet) {
        std::cout << "Read element set " << name << " with " << set.size() << " labels." << std::endl;
    }
    return true;
}
bool reWriteINPFile(const std::string& filename, const std::vector<Node3D>& nodes,
    const std::vector<Element3D>& elements, const OutputSet& outputSet) {
    std::ofstream out(filename);
    if (!out.is_open()) {
        std::cerr << "Error: Unable to open file " << filename << " for writing." << std::endl;
        return false;
    }

    // 设置输出格式：固定点，7位小数，右对齐
    out << std::fixed << std::setprecision(7);

    // 写入头部
    out << "*Heading\n";
    out << "** Job name: ReWritten Model name: Job\n";
    out << "** Generated by: C++ Program on " << "09/18/2025 08:56 AM PDT" << "\n";
    out << "*Preprint, echo=NO, model=NO, history=NO, contact=NO\n";
    out << "**\n";
    out << "** PARTS\n";
    out << "**\n";
    out << "*Part, name=Concrete\n";
    std::cout <<  " ====================Write Output Inp file....=================================" << std::endl;
    // 写入节点 (*Node)
    out << "*Node\n";
    for (const auto& node : nodes) {
        out << std::setw(10) << node.id << ","
            << std::setw(15) << node.x << ","
            << std::setw(15) << node.y << ","
            << std::setw(15) << node.z << "\n";
    }
    std::cout << nodes.size() << " Nodes has been writen to inp file!" << std::endl;
 
    // 写入所有单元 (*Element, TYPE=C3D4)
    if (!elements.empty()) {
        out << "*ELEMENT,TYPE=C3D4\n";
        for (const auto& elem : elements) {
            out << std::setw(10) << elem.id << ","
                << std::setw(5) << elem.n1 << ","
                << std::setw(5) << elem.n2 << ","
                << std::setw(5) << elem.n3 << ","
                << std::setw(5) << elem.n4 << "\n";
        }
        std::cout << elements.size() << " Elements has been writen to inp file!" << std::endl;
    }

    // 写入单元集合 (*Elset) 基于 outputSet
    for (const auto& [elsetName, elementSet] : outputSet) {
        if (!elementSet.empty()) {
            out << "*ELSET, ELSET=" << elsetName << "\n";
            size_t i = 0;
            for (const int& label : elementSet) {
                out << " " << label;
                if (i < elementSet.size() - 1) out << ",";
                if ((i + 1) % 16 == 0 || i == elementSet.size() - 1) out << "\n";
                ++i;
            }
            std::cout << " Set name: " << elsetName << ",  " << elementSet.size()<<" has been writen to inp file!" << std::endl;
        }
    }

    // 结束部分
    out << "*End Part\n";

    out.close();

    std::cout << "ReWrite INP File to: " << filename << " successfully!!!" << std::endl;
    return true;
}
// 打印节点信息 (前10个节点，坐标精度6位)
void printNodes(const std::vector<Node3D>& nodes) {
    std::cout << "\n=== Nodes (First 10) ===" << std::endl;
    std::cout << std::fixed << std::setprecision(6);
    size_t num_to_print = std::min<size_t>(nodes.size(), 10);
    for (size_t i = 0; i < num_to_print; ++i) {
        const auto& node = nodes[i];
        std::cout << "Node " << node.id << ": (" << node.x << ", " << node.y << ", " << node.z << ")" << std::endl;
    }
    if (nodes.size() > 10) {
        std::cout << "... (Total: " << nodes.size() << " nodes)" << std::endl;
    }
}
void printNodeMap(const std::unordered_map<int, Node3D*>& node_map) {
    std::cout << "Node Map Contents (Total: " << node_map.size() << " nodes):\n";
    std::cout << "--------------------------------\n";

    // 遍历哈希表
    for (const auto& pair : node_map) {
        int nodeId = pair.first;
        Node3D* node = pair.second;
        if (node) {
            std::cout << "Node ID: " << nodeId << ", Coordinates: ("
                << node->x << ", " << node->y << ", " << node->z << ")\n";
        }
        else {
            std::cout << "Node ID: " << nodeId << ", (NULL pointer)\n";
        }
    }

    std::cout << "--------------------------------\n";
}
void printElementNodeMap(const std::unordered_map<int, std::vector<EN_ID>>& elementNodeMap) {
    std::cout << "Element-Node Map Contents (Total Entries: " << elementNodeMap.size() << "):\n";
    std::cout << "--------------------------------\n";

    // 遍历哈希表
    for (const auto& pair : elementNodeMap) {
        int key = pair.first;         // 键（例如面 ID）
        const std::vector<EN_ID>& enIds = pair.second; // 对应的 EN_ID 向量

        std::cout << "Key: " << key << " (Contains " << enIds.size() << " entries):\n";
        for (const EN_ID& en : enIds) {
            std::cout << "  - Element ID: " << en.element_id
                << ", New Node ID: " << en.NewNode_id << "\n";
        }
        std::cout << "--------------------------------\n";
    }

    std::cout << "--------------------------------\n";
}
// 打印单元信息 (前10个单元)
void printElements(const std::vector<Element3D>& elements) {
    std::cout << "\n=== Elements (First 10) ===" << std::endl;
    size_t num_to_print = std::min<size_t>(elements.size(), 10);
    for (size_t i = 0; i < num_to_print; ++i) {
        const auto& elem = elements[i];
        std::cout << "Element " << elem.id << ": Nodes [" << elem.n1 << ", " << elem.n2 << ", " << elem.n3 << ", " << elem.n4 << "]" << std::endl;
    }
    if (elements.size() > 10) {
        std::cout << "... (Total: " << elements.size() << " elements)" << std::endl;
    }
}

// 打印所有节点和单元摘要 (数量 + 前10个)
void printMeshSummary(const std::vector<Node3D>& nodes, const std::vector<Element3D>& elements) {
    std::cout << "\n=== Mesh Summary ===" << std::endl;
    std::cout << "Total Nodes: " << nodes.size() << std::endl;
    std::cout << "Total Elements: " << elements.size() << std::endl;
    printNodes(nodes);
    printElements(elements);
}

// 辅助函数：标准化三角面（排序节点ID: min, mid, max）
NormalizedFaceKey normalizeFace(int n1, int n2, int n3) {
    int min_n = std::min({ n1, n2, n3 });
    int max_n = std::max({ n1, n2, n3 });
    int mid_n = n1 + n2 + n3 - min_n - max_n;
    return { min_n, mid_n, max_n };
}


int getMaxNodeId(const std::vector<Node3D>& nodes) {
    int max_node_id = 0;
    for (const auto& node : nodes) {
        max_node_id = std::max(max_node_id, node.id);
    }
    return max_node_id;
}

int getMaxElementId(const std::vector<Element3D>& elements) {
    int max_elem_id = 0;
    for (const auto& elem : elements) {
        max_elem_id = std::max(max_elem_id, elem.id);
    }
    return max_elem_id;
}


// 打印 NormalizedFaceKey
void printNormalizedFaceKey(const NormalizedFaceKey& key, std::ostream& os) {
    os << "NormalizedFaceKey: [" << key.min_node_id << ", "
        << key.mid_node_id << ", " << key.max_node_id << "]" << std::endl;
}

// 打印 Node3D
void printNode3D(const Node3D& node, std::ostream& os) {
    os << "Node3D: ID = " << node.id << ", Coordinates = ("
        << node.x << ", " << node.y << ", " << node.z << ")" << std::endl;
}

// 打印 Element3D
void printElement3D(const Element3D& elem, std::ostream& os ) {
    os << "Element3D (C3D4): ID = " << elem.id << ", Nodes = ["
        << elem.n1 << ", " << elem.n2 << ", " << elem.n3 << ", " << elem.n4 << "]" << std::endl;
}

// 打印 COH3D6
void printCOH3D6(const COH3D6& coh_elem, std::ostream& os) {
    os << "COH3D6: ID = " << coh_elem.id << ", Nodes = ["
        << coh_elem.n1 << ", " << coh_elem.n2 << ", " << coh_elem.n3 << ", "
        << coh_elem.n4 << ", " << coh_elem.n5 << ", " << coh_elem.n6 << "]" << std::endl;
}

// 打印 FaceInfo
void printFaceInfo(const FaceInfo& face, std::ostream& os ) {
    os << "FaceInfo: ElementID = " << face.element_id << ", Nodes = ["
        << face.node1 << ", " << face.node2 << ", " << face.node3 << "]" << std::endl;
}

// 打印 SharedFaceGroup
void printSharedFaceGroup(const SharedFaceGroup& group, std::ostream& os ) {
    os << "SharedFaceGroup: KeyFace = [" << group.key.min_node_id << ", "
        << group.key.mid_node_id << ", " << group.key.max_node_id << "], Infos ("
        << group.infos.size() << " faces):\n";
    for (const auto& face : group.infos) {
        os << "  ";
        printFaceInfo(face, os);
    }
}





FaceInfo GetFace2(const Element3D* elem, FaceConnectivityType type) {

    switch (type)
    {
    case FaceConnectivityType::Con123: return { elem->id, elem->n1, elem->n2, elem->n3 };
    case FaceConnectivityType::Con142: return { elem->id, elem->n1, elem->n4, elem->n2 };
    case FaceConnectivityType::Con243: return { elem->id, elem->n2, elem->n4, elem->n3 };
    case FaceConnectivityType::Con341: return { elem->id, elem->n3, elem->n4, elem->n1 };
    default: return {};
    }

    
}
std::vector<FaceInfo> GetFaces2(const Element3D& elem) {
   

    int id = elem.id;
    return
    {
        {id, elem.n1, elem.n2, elem.n3,FaceConnectivityType::Con123},
        {id, elem.n1, elem.n4, elem.n2 ,FaceConnectivityType::Con142},
        {id, elem.n2, elem.n4, elem.n3 ,FaceConnectivityType::Con243},
        {id, elem.n3, elem.n4, elem.n1,FaceConnectivityType::Con341}
    };
}



std::vector<Element3D*> getSetElements2(const std::unordered_map<int, Element3D*>& Elem_map,
    const std::unordered_set<int>& TargetElementLabelSet) {
    std::vector<Element3D*> SetElements;

    // 预分配空间，优化性能
    SetElements.reserve(TargetElementLabelSet.size());

    // 遍历 TargetElementLabelSet，查找并添加匹配的单元
    for (int label : TargetElementLabelSet) {
        auto it = Elem_map.find(label);
        if (it != Elem_map.end()) {
            SetElements.push_back(it->second); // 解引用指针，创建副本
        }
        else {
            std::cerr << "Warning: Element ID " << label << " not found in Elem_map.\n";
        }
    }

    return SetElements;
}


std::vector<FaceInfoList> GetSharedFaces2(const std::vector<Element3D*>& original_elements, const std::unordered_set<int>& MatchingElementLabels)
{
    if (original_elements.size() == 0)  return {};
    std::unordered_map<NormalizedFaceKey, FaceInfoList> face_to_infos;
    face_to_infos.reserve(original_elements.size() * 4);
    std::vector<FaceInfoList> shared_groups;
    shared_groups.reserve(original_elements.size() * 2);

    // 并行生成面
#pragma omp parallel
    {
        if (omp_get_thread_num() == 0) {
            std::cout << "Number of threads: " << omp_get_num_threads() << std::endl;
        }
        std::unordered_map<NormalizedFaceKey, FaceInfoList> local_face_to_infos;
        local_face_to_infos.reserve(original_elements.size() * 4 / omp_get_num_threads());

#pragma omp for
        for (int i = 0; i < original_elements.size(); ++i) {
            const Element3D* elem = original_elements[i];

            std::vector<FaceInfo> faces = GetFaces2(*elem);

            for (const auto& face : faces) {
                auto keyFace = normalizeFace(face.node1, face.node2, face.node3);
                local_face_to_infos[keyFace].push_back(face);
            }
        }

        // 合并本地映射到全局（需锁）
#pragma omp critical
        for (const auto& [key, infos] : local_face_to_infos) {
            auto& global_infos = face_to_infos[key];
            global_infos.insert(global_infos.end(), infos.begin(), infos.end());
        }
    }

    // 筛选共享面

    for (auto& [key, infos] : face_to_infos) {
        if (infos.size() == 2) {
            COH3D6Type type = GetCohesiveType(infos[0].element_id, infos[1].element_id, MatchingElementLabels);
            if (type != COH3D6Type::Outside) {
                infos[0].type = type;
                shared_groups.push_back(infos);
            }

        }
    }
    std::cout << "Number of shared faces found: " << shared_groups.size() << std::endl;
    return shared_groups;
}








// 辅助函数：比较两节点坐标是否相同（考虑浮点误差）
bool areNodesEqual(const Node3D& n1, const Node3D& n2, double tol) {
    return std::abs(n1.x - n2.x) < tol &&
        std::abs(n1.y - n2.y) < tol &&
        std::abs(n1.z - n2.z) < tol;
}


void alignFaceNodes2(FaceInfo& Newf2, const FaceInfo& Newf1, const std::unordered_map<int,Node3D*>& node_map) {
    // 存储 Newf1 和 Newf2 的节点
    int f1_nodes[3] = { Newf1.node1, Newf1.node2, Newf1.node3 };
    int f2_nodes[3] = { Newf2.node1, Newf2.node2, Newf2.node3 };

    /* std::cout << "Original Newf2: " << Newf2.node1 << " " << Newf2.node2 << " " << Newf2.node3 << std::endl;
     std::cout << "Newf1: " << Newf1.node1 << " " << Newf1.node2 << " " << Newf1.node3 << std::endl;*/

     // 匹配 Newf2 节点到 Newf1 节点
    int new_f2_nodes[3] = { 0, 0, 0 }; // 初始化新顺序
    for (int i = 0; i < 3; ++i) {
        auto it1 = node_map.find(f1_nodes[i]);
        if (it1 == node_map.end()) {
            std::cerr << "Error: Node ID " << f1_nodes[i] << " not found in node_map." << std::endl;
            Newf2.node1 = Newf2.node2 = Newf2.node3 = 0;
            return;
        }
        const Node3D& n1 = *(it1->second);
        bool found = false;
        for (int j = 0; j < 3; ++j) {
            auto it2 = node_map.find(f2_nodes[j]);
            if (it2 == node_map.end()) {
                std::cerr << "Error: Node ID " << f2_nodes[j] << " not found in node_map." << std::endl;
                Newf2.node1 = Newf2.node2 = Newf2.node3 = 0;
                return;
            }
            const Node3D& n2 = *(it2->second);
            if (areNodesEqual(n1, n2)) {
                new_f2_nodes[i] = f2_nodes[j]; // 直接将 Newf2 的节点分配到 Newf1 的对应位置
                // std::cout << "Matched: Newf1 Node " << f1_nodes[i] << " with Newf2 Node " << f2_nodes[j] << std::endl;
                found = true;
                break;
            }
        }
        if (!found) {
            std::cerr << "Error: No match found for Newf1 Node " << f1_nodes[i] << std::endl;
            Newf2.node1 = Newf2.node2 = Newf2.node3 = 0;
            return;
        }
    }

    // 更新 Newf2 的节点顺序
    Newf2.node1 = new_f2_nodes[0];
    Newf2.node2 = new_f2_nodes[1];
    Newf2.node3 = new_f2_nodes[2];

    /*  std::cout << "Aligned Newf2: [" << Newf2.node1 << "," << Newf2.node2 << "," << Newf2.node3
          << "] to match Newf1: [" << Newf1.node1 << "," << Newf1.node2 << "," << Newf1.node3 << "]" << std::endl;*/
}

void write3DInp(const std::string& filename, const std::vector<Node3D>& NewNodes,
    const std::vector<Element3D>& NewElements,
    const std::vector<COH3D6>& cohesive_elements,
    const OutputSet& outputSet, const std::string& InsertCohesiveSetName, const bool setByOrder, const bool outputNodeSet) {
    std::ofstream out(filename);
    if (!out.is_open()) {
        std::cerr << "Error: Unable to open file " << filename << " for writing." << std::endl;
        return;
    }
    std::string partname = PartName + "(" + InsertCohesiveSetName + ")";
    // 设置输出格式：固定点，6位小数，右对齐
    out << std::fixed << std::setprecision(7);

    // 写入头部
    out << "*Heading\n";
    out << "** Job name: LJZshabi Model name: Job\n";
    out << "** Generated by: C++ Program\n";
    out << "*Preprint, echo=NO, model=NO, history=NO, contact=NO\n";
    out << "**\n";
    out << "** PARTS\n";
    out << "**\n";
    out << "*Part, name=" << partname << "\n";

    // 写入节点 (*Node)
    out << "*Node\n";
    for (const auto& node : NewNodes) {
        out << std::setw(7) << node.id << ","
            << std::setw(12) << node.x << ","
            << std::setw(12) << node.y << ","
            << std::setw(12) << node.z << "\n";
    }
    std::cout << NewNodes.size() << " Nodes has been writen to inp file!" << std::endl;
   

    // 写入 C3D4 单元 (*Element, type=C3D4)
    if (!NewElements.empty()) {
        out << "*Element, type=C3D4\n";
        for (const auto& elem : NewElements) {
            out << std::setw(7) << elem.id << ","
                << std::setw(3) << elem.n1 << ","
                << std::setw(3) << elem.n2 << ","
                << std::setw(3) << elem.n3 << ","
                << std::setw(3) << elem.n4 << "\n";
        }
        std::cout << NewElements.size() << " Elements[C3D4] has been writen to inp file!" << std::endl;
    }
  
    // 写入 COH3D6 单元 (*Element, type=COH3D6)
    if (!cohesive_elements.empty()) {
        out << "*Element, type=COH3D6\n";
        for (const auto& coh : cohesive_elements) {
            out << std::setw(7) << coh.id << ","
                << std::setw(3) << coh.n1 << ","
                << std::setw(3) << coh.n2 << ","
                << std::setw(3) << coh.n3 << ","
                << std::setw(3) << coh.n4 << ","
                << std::setw(3) << coh.n5 << ","
                << std::setw(3) << coh.n6 << "\n";
        }
        std::cout << cohesive_elements.size() << " Elements[COH3D6] has been writen to inp file!" << std::endl;
    }
    //输出内聚力单元的集合
    int cohesive_num = cohesive_elements.size();
    if (!cohesive_elements.empty()) {
        // 写入所有内聚力单元集合
        out << "*Elset, elset=AllCohesiveElements\n";
        for (size_t i = 0; i < cohesive_num; ++i) {
            out << " " << cohesive_elements[i].id;
            if (i < cohesive_num - 1) out << ",";
            if ((i + 1) % 16 == 0 || i == cohesive_num - 1) out << "\n";

        }
        std::cout << "Write elementSet: AllCohesiveElements[COH3D6]  Element number:" << cohesive_num << std::endl;
    }
  
    
    // 写入集合 (*Elset) 基于 outputSet
    for (const auto& [elsetName, elementSet] : outputSet) {
        if (!elementSet.empty()) {
            // 复制到 vector 以便排序（如果需要）
            std::vector<int> labels(elementSet.begin(), elementSet.end());
            if (setByOrder)   std::sort(labels.begin(), labels.end());
            //写入
            out << "*Elset, elset=" << elsetName << "\n";
            size_t i = 0;
            for (const int& label : labels) {
                out << " " << label;
                if (i < elementSet.size() - 1) out << ",";
                if ((i + 1) % 16 == 0 || i == elementSet.size() - 1) out << "\n";
                ++i;
            }
           
            std::string ElementType = isCohesiveSet(elementSet) ? "[COH3D6]" : "[C3D4]" ;
            std::cout << "Write elementSet: " << elsetName<< ElementType << " , Element number:" << elementSet.size() << std::endl;
        }
       
       // std::cout << elementSet.size() << std::endl;
    }
    if (outputNodeSet) {
        // 找到节点坐标的 ZMin 和 ZMax
        float zMin = std::numeric_limits<float>::max();
        float zMax = std::numeric_limits<float>::lowest();
        for (const auto& node : NewNodes) {
            zMin = std::min(zMin, node.z);
            zMax = std::max(zMax, node.z);
        }

        // 找到节点坐标的 XMin 和 XMax
        float xMin = std::numeric_limits<float>::max();
        float xMax = std::numeric_limits<float>::lowest();
        for (const auto& node : NewNodes) {
            xMin = std::min(xMin, node.x);
            xMax = std::max(xMax, node.x);
        }

        // 找到节点坐标的 YMin 和 YMax
        float yMin = std::numeric_limits<float>::max();
        float yMax = std::numeric_limits<float>::lowest();
        for (const auto& node : NewNodes) {
            yMin = std::min(yMin, node.y);
            yMax = std::max(yMax, node.y);
        }

        // 设置容差（例如 1e-6，可根据需求调整）
        const double tolerance = 1e-6;

        // 写入 ZMin 接近的节点集合
        std::vector<int> zMinNodes;
        zMinNodes.reserve(NewNodes.size());
        for (const auto& node : NewNodes) {
            if (std::abs(node.z - zMin) < tolerance) {
                zMinNodes.push_back(node.id);
            }
        }
        if (!zMinNodes.empty()) {
            out << "*Nset, nset=ZMinNodes\n";
            for (size_t i = 0; i < zMinNodes.size(); ++i) {
                out << " " << zMinNodes[i];
                if (i < zMinNodes.size() - 1) out << ",";
                if ((i + 1) % 16 == 0 || i == zMinNodes.size() - 1) out << "\n";
            }
        }

        // 写入 ZMax 接近的节点集合
        std::vector<int> zMaxNodes;
        zMaxNodes.reserve(NewNodes.size());
        for (const auto& node : NewNodes) {
            if (std::abs(node.z - zMax) < tolerance) {
                zMaxNodes.push_back(node.id);
            }
        }
        if (!zMaxNodes.empty()) {
            out << "*Nset, nset=ZMaxNodes\n";
            for (size_t i = 0; i < zMaxNodes.size(); ++i) {
                out << " " << zMaxNodes[i];
                if (i < zMaxNodes.size() - 1) out << ",";
                if ((i + 1) % 16 == 0 || i == zMaxNodes.size() - 1) out << "\n";
            }
        }

        // 写入 XMin 接近的节点集合
        std::vector<int> xMinNodes;
        xMinNodes.reserve(NewNodes.size());
        for (const auto& node : NewNodes) {
            if (std::abs(node.x - xMin) < tolerance) {
                xMinNodes.push_back(node.id);
            }
        }
        if (!xMinNodes.empty()) {
            out << "*Nset, nset=XMinNodes\n";
            for (size_t i = 0; i < xMinNodes.size(); ++i) {
                out << " " << xMinNodes[i];
                if (i < xMinNodes.size() - 1) out << ",";
                if ((i + 1) % 16 == 0 || i == xMinNodes.size() - 1) out << "\n";
            }
        }

        // 写入 XMax 接近的节点集合
        std::vector<int> xMaxNodes;
        xMaxNodes.reserve(NewNodes.size());
        for (const auto& node : NewNodes) {
            if (std::abs(node.x - xMax) < tolerance) {
                xMaxNodes.push_back(node.id);
            }
        }
        if (!xMaxNodes.empty()) {
            out << "*Nset, nset=XMaxNodes\n";
            for (size_t i = 0; i < xMaxNodes.size(); ++i) {
                out << " " << xMaxNodes[i];
                if (i < xMaxNodes.size() - 1) out << ",";
                if ((i + 1) % 16 == 0 || i == xMaxNodes.size() - 1) out << "\n";
            }
        }

        // 写入 YMin 接近的节点集合
        std::vector<int> yMinNodes;
        yMinNodes.reserve(NewNodes.size());
        for (const auto& node : NewNodes) {
            if (std::abs(node.y - yMin) < tolerance) {
                yMinNodes.push_back(node.id);
            }
        }
        if (!yMinNodes.empty()) {
            out << "*Nset, nset=YMinNodes\n";
            for (size_t i = 0; i < yMinNodes.size(); ++i) {
                out << " " << yMinNodes[i];
                if (i < yMinNodes.size() - 1) out << ",";
                if ((i + 1) % 16 == 0 || i == yMinNodes.size() - 1) out << "\n";
            }
        }

        // 写入 YMax 接近的节点集合
        std::vector<int> yMaxNodes;
        yMaxNodes.reserve(NewNodes.size());
        for (const auto& node : NewNodes) {
            if (std::abs(node.y - yMax) < tolerance) {
                yMaxNodes.push_back(node.id);
            }
        }
        if (!yMaxNodes.empty()) {
            out << "*Nset, nset=YMaxNodes\n";
            for (size_t i = 0; i < yMaxNodes.size(); ++i) {
                out << " " << yMaxNodes[i];
                if (i < yMaxNodes.size() - 1) out << ",";
                if ((i + 1) % 16 == 0 || i == yMaxNodes.size() - 1) out << "\n";
            }
        }
    }
   
    




    // 结束部分
    out << "*End Part\n";

    out.close();

    std::cout << "Write INP File (PartName=" << partname << ") to: " << filename << " successfully!!!" << std::endl;
}







std::unordered_map<int,Element3D*> creatElementMap(std::vector<Element3D>& elements) {
    std::unordered_map<int,Element3D*> elem_map;
    elem_map.reserve(elements.size());
    for (auto& elem : elements) {
      //  elem_map[elem.id] = &elem;
        elem_map.emplace(elem.id, &elem);  // 直接构造键值对，无多余操作
    }
    return elem_map;
}


std::unordered_map<int, Node3D*> creatNodeMap(std::vector<Node3D>& nodes) {
    std::unordered_map<int, Node3D*> node_map;
    node_map.reserve(nodes.size());
    for (auto& node : nodes) {
        node_map[node.id] = &node;
    }
    return node_map;
}





std::unordered_map<int, std::vector<EN_ID>> countNodeUsageInSet4(const std::unordered_map<int, Element3D*>& elem_map,
    const std::unordered_set<int>& TargetElementLabelSet,
    int& maxNodeID, const std::unordered_set<int>& MatchingElementLabels) {
    // 第一步：统计节点使用次数
    std::unordered_map<int, int> node_usage; // key: 节点 label, value: 使用次数
    node_usage.reserve(TargetElementLabelSet.size() * 4);

    std::unordered_map<int, std::vector<int>> node_to_elements; // key: 节点 label, value: 拥有该节点的单元 ID 列表
    node_to_elements.reserve(TargetElementLabelSet.size() * 4);

    for (int elem_id : TargetElementLabelSet) {
        auto it = elem_map.find(elem_id);
        if (it != elem_map.end()) {
            const auto& elem = *(it->second);
            node_usage[elem.n1]++;
            node_usage[elem.n2]++;
            node_usage[elem.n3]++;
            node_usage[elem.n4]++;
            node_to_elements[elem.n1].push_back(elem_id);
            node_to_elements[elem.n2].push_back(elem_id);
            node_to_elements[elem.n3].push_back(elem_id);
            node_to_elements[elem.n4].push_back(elem_id);
        }
    }

    // 第二步：构建结果 map，只处理共享节点
    std::unordered_map<int, std::vector<EN_ID>> nodeID_EN_IDs;
    nodeID_EN_IDs.reserve(node_usage.size());

    for (const auto& [node_label, count] : node_usage) {
        if (count > 1) { // 只处理共享节点
            const std::vector<int>& owning_elements = node_to_elements[node_label];
            //int num = CountMatchingLabels(owning_elements, MatchingElementLabels);
            std::vector<int> UV = UnMatchingLabels(owning_elements, MatchingElementLabels);

            std::vector<EN_ID> en_ids;
            if (UV.size() == owning_elements.size()) {
                en_ids.reserve(count - 1); // 只生成 N-1 个新 ID
                // 选择第一个单元保持原始节点 ID，其他生成新 ID
                for (size_t k = 1; k < owning_elements.size(); ++k) { // 从第二个开始修改
                    maxNodeID++;
                    en_ids.emplace_back(owning_elements[k], maxNodeID); // element_id, NewNode_id
                }
            }
            else {
                //std::cout << UV.size() << std::endl;
                en_ids.reserve(UV.size()); //
                for (size_t k = 0; k < UV.size(); ++k) { // 从第二个开始修改
                    maxNodeID++;
                    en_ids.emplace_back(UV[k], maxNodeID); // element_id, NewNode_id
                }

            }
            nodeID_EN_IDs[node_label] = en_ids;

            //std::cout << "MatchingElementLabels: " << num << std::endl;
            //std::cout << "owning_elements.size: " << owning_elements.size() << std::endl;
               
          
           

           
        }
    }

    return nodeID_EN_IDs;
}
void addNodes(std::vector<Node3D>& OriginNodes, std::unordered_map<int, Node3D*>& node_map,const std::unordered_map<int, std::vector<EN_ID>>& nodeID_EN_IDs) {
    //函数作用：基于nodeID_EN_IDs 添加新的复制节点 
    //OriginNodes 是初始的节点列表
    //node_map 是OriginNodes的map
    //nodeID_EN_IDs  ：key ---> 节点的label      
    //     value    ---> 该节点被TargetElementLabelSet中哪些单元所拥有 value[i].element_id 是第i个拥有该节点的单元的label
    //     value[i].NewNode_id 是该节点被新增时的新的节点编号 但其坐标是不变的因为我插的是0厚度的内聚力单元
    // 预分配空间：估算新节点数量 = nodeID_EN_IDs 中 value 的大小之和
    
    size_t totalNewNodes = 0;
    for (const auto& [_, value] : nodeID_EN_IDs) {
        totalNewNodes += value.size(); // 每个 value.size() 表示需要添加的新节点数
    }
    OriginNodes.reserve(OriginNodes.size() + totalNewNodes);
    node_map = creatNodeMap(OriginNodes);
    // 遍历并添加新节点
    for (const auto& [NodeID, value] : nodeID_EN_IDs) {
        auto it = node_map.find(NodeID);
        if (it == node_map.end()) {
            std::cerr << "Error: Node ID " << NodeID << " not found in node_map.\n";
            continue; // 跳过此节点
        }
        const Node3D& originalNode = *(it->second);
        for (const auto& en : value) {
            OriginNodes.emplace_back(en.NewNode_id, originalNode.x, originalNode.y, originalNode.z);
        }
    }
    //重新更新node_map
    for (size_t i = 0; i < OriginNodes.size(); ++i) {
        int newNodeId = OriginNodes[i].id;
        node_map[newNodeId] = &OriginNodes[i];
    }
    
}
void ModifyElements(const std::unordered_map<int, std::vector<EN_ID>>& nodeID_EN_IDs, std::unordered_map<int,  Element3D*>& elem_map, const std::unordered_set<int>& MatchingElementLabels) {
    // 遍历所有共享节点及其新节点映射
    for (const auto& [NodeID, value] : nodeID_EN_IDs) {
        // NodeID 是共享节点的原始 ID，value 包含需要修改的单元和对应新节点 ID
        for (const EN_ID& en : value) {

            int elem_ID = en.element_id;

            if (MatchingElementLabels.find(elem_ID) != MatchingElementLabels.end()) continue;

            int NewNodeID = en.NewNode_id;

            // 查找单元
            auto it = elem_map.find(elem_ID);
            if (it == elem_map.end()) {
                std::cerr << "Warning: Element ID " << elem_ID << " not found in elem_map.\n";
                continue; // 跳过不存在的单元
            }

            Element3D& elem = *(it->second);
            // 只替换第一个匹配的节点，防止重复修改
            bool replaced = false;
            if (!replaced && elem.n1 == NodeID) {
                elem.n1 = NewNodeID;
                replaced = true;
            }
            else if (!replaced && elem.n2 == NodeID) {
                elem.n2 = NewNodeID;
                replaced = true;
            }
            else if (!replaced && elem.n3 == NodeID) {
                elem.n3 = NewNodeID;
                replaced = true;
            }
            else if (!replaced && elem.n4 == NodeID) {
                elem.n4 = NewNodeID;
                replaced = true;
            }

            if (!replaced) {
                std::cerr << "Warning: Node ID " << NodeID << " not found in Element ID " << elem_ID << ".\n";
            }
        }
    }
}

void GetElementsOutsideTargetSet(const std::unordered_map<int, Element3D*>& Elem_map,
    const std::unordered_set<int>& TargetElementLabelSet) {
    

    // 预分配空间，优化性能
    OutsideTargetElementLabels.reserve(Elem_map.size());

    // 遍历 Elem_map，查找不在 TargetElementLabelSet 中的 label
    for (const auto& [label, _] : Elem_map) {
        if (TargetElementLabelSet.count(label) == 0) {
            OutsideTargetElementLabels.insert(label);
        }
    }

 
}


std::unordered_set<int> GetMatchingElements(const std::unordered_map<int, Element3D*>& Elem_map,
    const std::unordered_set<int>& TargetElementLabelSet)
{
    // 第一步：收集目标单元涉及的所有节点
    std::unordered_set<int> NodeLabels;
    NodeLabels.reserve(TargetElementLabelSet.size() * 4); // 每个单元4个节点
    for (int label : TargetElementLabelSet) {
        auto it = Elem_map.find(label);
        if (it != Elem_map.end() && it->second) {
            const Element3D* elem = it->second;
            NodeLabels.insert(elem->n1);
            NodeLabels.insert(elem->n2);
            NodeLabels.insert(elem->n3);
            NodeLabels.insert(elem->n4);
        }
    }

    // 第二步：直接找 Outside 单元并判断是否满足 ≥3 节点匹配
    std::unordered_set<int> MatchingElementLabels;
    MatchingElementLabels.reserve(Elem_map.size()); // 预分配

    for (const auto& [label, elem] : Elem_map) {
        // 跳过目标集中的单元
        if (TargetElementLabelSet.count(label)) continue;

        int matchCount = 0;
        if (NodeLabels.count(elem->n1)) ++matchCount;
        if (NodeLabels.count(elem->n2)) ++matchCount;
        if (NodeLabels.count(elem->n3)) ++matchCount;
        if (NodeLabels.count(elem->n4)) ++matchCount;

        if (matchCount >= 3) {
            MatchingElementLabels.insert(label);
        }
    }

    return MatchingElementLabels;
}



void MergeElementLabelSets2(std::unordered_set<int>& TargetElementLabelSet,
    const std::unordered_set<int>& MatchingElementLabels)
{
    TargetElementLabelSet.insert(MatchingElementLabels.begin(), MatchingElementLabels.end());
}


COH3D6Type GetCohesiveType(const int id1, const int id2, const std::unordered_set<int>& MatchingElementLabels) {
    int count = 0;
    if (MatchingElementLabels.count(id1)) count++;
    if (MatchingElementLabels.count(id2)) count++;
    if (count == 0) {
        return COH3D6Type::Inner;
    }
    else if (count == 1) {
        return COH3D6Type::Boundary;
    }
    else if (count == 2) {
        return COH3D6Type::Outside;
    }
}

//bool FindInset(const OutputSet& outputSet,const std::string& TargetSetName) {
//    //TargetSetName是集合的名称也是outputSet的一个键值
//    //现在的目标是找到outputSet中的集合中单元和TargetSetName集合重叠的集合名称 有序的返回
//    //比如outputSet的集合setA中的单元全部是TargetSetName中的单元 即TargetSetName包含setA  则返回  标记全部在内部  部分重叠也返回 标记部分在内部
//    //有序的返回和TargetSetName包含或重叠的集合的名称和重叠情况
//}
std::vector<std::pair<std::string, std::string>> FindInset(const OutputSet& outputSet, const std::string& TargetSetName) {
    auto it = outputSet.find(TargetSetName);
    if (it == outputSet.end()) {
        return {};
    }
    const auto& target = it->second;

    std::vector<std::pair<std::string, std::string>> results;
    for (const auto& [name, elset] : outputSet) {
        if (name == TargetSetName || name == "BASEELEMENTS" || elset.empty())  continue;
        if(isCohesiveSet(elset))  continue;//如果是内聚力单元的集合 也不用找了
        bool intersects = false;
        bool is_subset = true;
        for (int elem : elset) {
            if (target.count(elem) > 0) {
                intersects = true;
            }
            else {
                is_subset = false;
            }
            // Early exit if possible
            if (!is_subset && intersects) {
                break;
            }
        }

        if (intersects) {
            std::string status = is_subset ? "AllIN" : "PartIN";
            results.emplace_back(name, status);
        }
    }

    // Sort by name (lexicographical order)
    std::sort(results.begin(), results.end());
    // Output the ordered results
    for (const auto& [name, status] : results) {
        std::cout << name << ": " << status << std::endl;
    }
    return results;
}
std::vector<std::string> findIntersectingSetKeys(
    const OutputSet& outputSet,
    const std::string& TargetSetName)
{
    std::vector<std::string> result;
    result.reserve(4);  // 预分配避免小规模扩容
    for (const auto& [key, elemSet] : outputSet) {
        if (elemSet.empty()) continue;
        // 快速跳过指定名称
        if (key == "BASEELEMENTS" || key == TargetSetName) continue;
        // 跳过内聚力集合（假设 isCohesiveSet 已高效实现）
        if (isCohesiveSet(elemSet)) continue;
        // 采样前 3 个元素快速判断
        auto it = elemSet.begin();
        const auto end = elemSet.end();
        bool found = false;

        // 最多检查 3 个
        for (int i = 0; i < 3 && it != end; ++i, ++it) {
            if (OutsideTargetElementLabels.find(*it) != OutsideTargetElementLabels.end()) {
                found = true;
                break;
            }
        }

        // 若前 3 个未命中，检查剩余
        if (!found && it != end) {
            found = std::any_of(it, end, [](int id) {
                return OutsideTargetElementLabels.find(id) != OutsideTargetElementLabels.end();
                });
        }

        if (found) {
            result.push_back(key);
        }
    }
    for (const auto& name : result) {
        std::cout << "Set [" << name << "]: In Outside Target Set" << std::endl;
    }
    return result;
}


bool insertCohesiveElementsToSet2(std::vector<Node3D>& Nodes, std::vector<Element3D>& Elements,
    std::unordered_set<int> TargetElementLabelSet, std::vector<COH3D6>& cohesive_elements,CohesiveIdentifyInfo& cohesiveIdentifyInfo) {
    Timer main_timer("Cohesive element insertion"); // 主程序计时
    main_timer.start();
    
    
    cohesiveIdentifyInfo.clear();

    
    
  
   
    if (TargetElementLabelSet.size() == 0 || Nodes.size() == 0 || Elements.size() == 0) return false;
   
    if (TargetElementLabelSet.size() > Elements.size()) return false;



   
    int maxNodeID = getMaxNodeId(Nodes);
    int maxElemID = getMaxElementId(Elements); maxElemID2 = maxElemID;
  


  
    //步骤一： 获取新的单元列表 和节点列表
    std::unordered_map<int, Element3D*> Elem_map = creatElementMap(Elements);
    GetElementsOutsideTargetSet(Elem_map, TargetElementLabelSet);//不用与插入内聚力单元 用于后续筛分boundary内聚力单元的


   
    
   
    std::unordered_set<int> MatchingElementLabels = GetMatchingElements(Elem_map, TargetElementLabelSet);
   


    //合并MatchingElementLabels和TargetElementLabelSet的函数
   
    MergeElementLabelSets2(TargetElementLabelSet, MatchingElementLabels);
 
    
     //找到OutsideTargetElementLabels中哪些单元有三个节点 都存在于NodeLabels中 返回这些单元的std::unordered_set<int> elementlabel

  
   
    std::vector<Element3D*> SetElements = getSetElements2(Elem_map, TargetElementLabelSet);
    //步骤二：获取共享面：基于原来的单元列表获取共享面
    std::vector<FaceInfoList> SharedFaces = GetSharedFaces2(SetElements, MatchingElementLabels);
    int cohesive_num = SharedFaces.size();   
    if (cohesive_num == 0) {
        std::cerr << "No shared edges found. Insertion failed." << std::endl;
        return true;
    }
    cohesive_elements.resize(cohesive_num);
    cohesiveIdentifyInfo.resize(cohesive_num,std::vector<int>(3,0));
 

   
    std::unordered_map<int, std::vector<EN_ID>> nodeID_EN_IDs = countNodeUsageInSet4(Elem_map, TargetElementLabelSet, maxNodeID, MatchingElementLabels);
   
   // printNodeMap(node_map);

   
    std::unordered_map<int, Node3D*> node_map;
    addNodes(Nodes, node_map, nodeID_EN_IDs);//修改了Nodes和node_map
    std::cout << "Total nodes after insertion: " << Nodes.size() << std::endl;
  

   
    ModifyElements(nodeID_EN_IDs, Elem_map, MatchingElementLabels);//修改了Elem_map和originElements
   
    
    std::cout << "Total elements after insertion (C3D4 ) count unchanged): " << Elem_map.size() << std::endl;

   


    //计算每一个内聚力单元的element label 便于并行化  预分配 ID 范围
    std::vector<int> maxElemIDs(cohesive_num);
    for (size_t i = 0; i < cohesive_num; i++)
    {
        maxElemIDs[i] = ++maxElemID;
    }
    // 错误收集：使用原子计数器记录错误数量
    std::atomic<int> error_count{ 0 };
    std::vector<std::string> errors(cohesive_num);  // 每个线程写入自己的位置，避免竞争
    // 并行寻找内聚力单元
   
    if (true) {
        std::cout << "OpenMP parallel computation enabled" << std::endl;
#pragma omp parallel
        {
            if (omp_get_thread_num() == 0) {
                std::cout << "Number of threads: " << omp_get_num_threads() << std::endl;
            }      
#pragma omp for schedule(static)
            for (int i = 0; i < SharedFaces.size(); ++i) {
                const FaceInfoList& ShareFaceList = SharedFaces[i];
                COH3D6 coh_elem;

                if (ShareFaceList.size() != 2) {
                    errors[i] = "ShareFaceList.size()!=2 exception at index " + std::to_string(i);
                    ++error_count;
                    continue;
                }
                int elem1_id = ShareFaceList[0].element_id;//第一个面所属的单元id
                FaceConnectivityType f1_Type = ShareFaceList[0].faceConnectivityType;
                int elem2_id = ShareFaceList[1].element_id;//第二个面所属的单元id
                FaceConnectivityType f2_Type = ShareFaceList[1].faceConnectivityType;
                COH3D6Type type = ShareFaceList[0].type;
                // 查找新单元
                auto it1 = Elem_map.find(elem1_id);//新的map
                auto it2 = Elem_map.find(elem2_id);
                if (it1 == Elem_map.end() || it2 == Elem_map.end()) {
                    errors[i] = "Error: Element ID " + std::to_string(it1 == Elem_map.end() ? elem1_id : elem2_id)
                        + " not found in NewElements at index " + std::to_string(i);
                    ++error_count;
                    continue;
                }
                // 获取新面
                const Element3D* Elem1 = it1->second;//新的element 节点更新过的
                const Element3D* Elem2 = it2->second;//新的element 节点更新过的

                FaceInfo Newf1 = GetFace2(Elem1, f1_Type);//A B C
                FaceInfo Newf2 = GetFace2(Elem2, f2_Type);// A' B' C'
                // std::cout <<"Before: " << Newf1.node1 << " " << Newf1.node2 << " " << Newf1.node3 << " " << Newf2.node1 << " " << Newf2.node2 << " " << Newf2.node3 << std::endl;
                alignFaceNodes2(Newf2, Newf1, node_map);//调整Newf2向Newf1对齐.. node_map是只读的
                //std::cout << "After: " << Newf1.node1 << " " << Newf1.node2 << " " << Newf1.node3 << " " << Newf2.node1 << " " << Newf2.node2 << " " << Newf2.node3 << std::endl;
                if (Newf2.node1 == 0) { // 无效 Newf2
                    errors[i] = "Error: Failed to align nodes for COH3D6 at index " + std::to_string(i);
                    ++error_count;
                    continue;
                }

                coh_elem.id = maxElemIDs[i];
                coh_elem.n1 = Newf1.node1; coh_elem.n2 = Newf1.node3; coh_elem.n3 = Newf1.node2;
                coh_elem.n4 = Newf2.node1; coh_elem.n5 = Newf2.node3; coh_elem.n6 = Newf2.node2;
                coh_elem.type = type;
                cohesive_elements[i] = coh_elem;

                cohesiveIdentifyInfo[i][0] = coh_elem.id;
                cohesiveIdentifyInfo[i][1] = elem1_id;
                cohesiveIdentifyInfo[i][2] = elem2_id;

            }
        }
       
        
        // 检查并报告错误
        if (error_count > 0) {
            for (size_t i = 0; i < errors.size(); ++i) {
                if (!errors[i].empty()) {
                    std::cerr << errors[i] << std::endl;
                }
            }
          
            return false;
        }
        main_timer.stop(); main_timer.getLastTime("Cohesive element insertion completed, time elapsed: ");
        std::cout << "Successful!!!  Inserted " << cohesive_elements.size() << " COH3D6 elements." << std::endl;
        return true;
    }
    else {
        std::cout << "Serial computation" << std::endl;
        //寻找内聚力单元
       int i = 0;  COH3D6 coh_elem;
       for (const FaceInfoList& ShareFaceList : SharedFaces) {

      
           if (ShareFaceList.size() != 2) {
               std::cout << "ShareFaceList.size()!=2  exception" << std::endl;
               return false;
           }


           int elem1_id = ShareFaceList[0].element_id;//第一个面所属的单元id
           FaceConnectivityType f1_Type = ShareFaceList[0].faceConnectivityType;
           int elem2_id = ShareFaceList[1].element_id;//第二个面所属的单元id
           FaceConnectivityType f2_Type = ShareFaceList[1].faceConnectivityType;

           COH3D6Type type = ShareFaceList[0].type;
           // 查找新单元
           auto it1 = Elem_map.find(elem1_id);//新的map
           auto it2 = Elem_map.find(elem2_id);
           if (it1 == Elem_map.end() || it2 == Elem_map.end()) {
               std::cerr << "Error: Element ID " << (it1 == Elem_map.end() ? elem1_id : elem2_id)
                   << " not found in NewElements." << std::endl;
               return false;
           }

           // 获取新面
           const Element3D* Elem1 = it1->second;//新的element  节点更新过的
           const Element3D* Elem2 = it2->second;//新的element  节点更新过的
     
           FaceInfo Newf1 = GetFace2(Elem1, f1_Type);//A  B   C
           FaceInfo Newf2 = GetFace2(Elem2, f2_Type);// A'  B'  C'

          // std::cout <<"Before: " << Newf1.node1 << " " << Newf1.node2 << " " << Newf1.node3 << " " << Newf2.node1 << " " << Newf2.node2 << " " << Newf2.node3 << std::endl;
           alignFaceNodes2(Newf2, Newf1, node_map);//调整Newf2向Newf1对齐.. node_map是只读的
           //std::cout << "After: " << Newf1.node1 << " " << Newf1.node2 << " " << Newf1.node3 << " " << Newf2.node1 << " " << Newf2.node2 << " " << Newf2.node3 << std::endl;
           if (Newf2.node1 == 0) { // 无效 Newf2
               std::cerr << "Error: Failed to align nodes for COH3D6." << std::endl;
               return false;
           }


     
           coh_elem.id = maxElemIDs[i];
           coh_elem.n1 = Newf1.node1; coh_elem.n2 = Newf1.node3; coh_elem.n3 = Newf1.node2;
           coh_elem.n4 = Newf2.node1; coh_elem.n5 = Newf2.node3; coh_elem.n6 = Newf2.node2;
           coh_elem.type = type;
           cohesive_elements[i] = coh_elem;

           cohesiveIdentifyInfo[i][0] = coh_elem.id;
           cohesiveIdentifyInfo[i][1] = elem1_id;
           cohesiveIdentifyInfo[i][2] = elem2_id;


           i++;

       }
      
       main_timer.stop(); main_timer.getLastTime("Cohesive element insertion completed, time elapsed: ");
       std::cout << "Successful!!!  Inserted " << cohesive_elements.size() << " COH3D6 elements." << std::endl;
       return true;
    }



   
}


void printSortedTargetElementLabelSet(const std::unordered_set<int>& TargetElementLabelSet) {
    // 将 unordered_set 转换为 vector 并排序
    std::vector<int> sortedLabels(TargetElementLabelSet.begin(), TargetElementLabelSet.end());
    std::sort(sortedLabels.begin(), sortedLabels.end());

    // 打印标题和总数
    std::cout << "Sorted Target Element Labels (Total: " << sortedLabels.size() << "):\n";
    std::cout << "--------------------------------\n";

    // 按顺序打印每个 label
    for (const int& label : sortedLabels) {
        std::cout << label << "\n";
    }

    std::cout << "--------------------------------\n";
}
std::string Insert3D(const std::string& INPName,const std::string& InsertCohesiveSetName) {
    std::string OutputInpFileName = OutputFolder + "/NewOutput_" + INPName;
    Timer main_timer("Main Program"); // 主程序计时
    main_timer.start();
    // 设置线程数
    std::cout << "Hardware concurrency: " << std::thread::hardware_concurrency() << std::endl;
    omp_set_dynamic(0);
    omp_set_num_threads(std::thread::hardware_concurrency());
    //std::cout << "OpenMP version: " << _OPENMP << std::endl;
   
    std::vector<Node3D> originNodes;
    std::vector<Element3D> originElements;
    std::unordered_set<int> TargetElementLabelSet;
    OutputSet outputSet;
    std::vector<COH3D6> cohesive_elements;
    CohesiveIdentifyInfo cohesiveIdentifyInfo;//TargetElementLabelSet中插入的所有内聚力单元
    std::string INPFilePath = inpFolder + "/" + INPName;
    //if (read3DInp(inpFolder + "/" + INPName, originNodes, originElements)) {
    Timer read_timer("Reading Inp File"); // 主程序计时
    read_timer.start();
    if (read3DInp(INPFilePath, originNodes, originElements, outputSet)) 
    {
        read_timer.stop(); read_timer.getLastTime("Read INP file time: ");//记录读取时间
        std::cout << "InsertCohesiveSetName  : " << InsertCohesiveSetName << std::endl;
        std::string InsertCohesiveSetNameUPPER = InsertCohesiveSetName;
        std::transform(InsertCohesiveSetNameUPPER.begin(), InsertCohesiveSetNameUPPER.end(), InsertCohesiveSetNameUPPER.begin(), ::toupper);//统一转换为大写
      
        if (insertCohesiveElementsToSet2(originNodes, originElements, outputSet[InsertCohesiveSetNameUPPER], cohesive_elements, cohesiveIdentifyInfo))
        {
         

           

            //根据 std::vector<std::pair<std::string, std::string>> SetInfo和 CohesiveIdentifyInfo cohesiveIdentifyInfo;//TargetElementLabelSet中插入的所有内聚力单元
           //前提（必成立的条件）：SetInfo中各个集合的单元是不重复的 
            //判断每一个内聚力单元的两个实体单元所属的集合（只考虑SetInfo中的集合）
            //情况一：某个内聚力单元的两个实体单元均在集合"Set-A"中 则创建一个名为"Set-A_COH"的集合    outputSet["Set-A_COH"] 添加其内聚力单元的id号方便后面创建集合
            //情况二：某个内聚力单元的两个实体单元一个在集合"Set-A"中另一个在"Set-B" 中（注："Set-A"和"Set-B"都是SetInfo中的集合 若不是则不考虑） 则依据它们的排列顺序（"Set-A"和"Set-B"在SetInfo中是有序的 比如"Set-A"在前就是"Set-A_Set-B_COH"，若"Set-B"在前就是"Set-B_Set-A_COH"）创建一个名为"Set-A_Set-B_COH"的集合  添加到outputSet中
            //情况四：某个内聚力单元的第一个实体单元在某个集合中 第二个却都不在 不处理
            //情况三：某个内聚力单元的两个实体单元均不在（只考虑SetInfo中的集合） 不处理
            //给出这个函数 直接修改outputSet添加新的集合
            std::vector<std::pair<std::string, std::string>> SetInfo = FindInset(outputSet, InsertCohesiveSetNameUPPER);
            std::vector<std::string> keys = findIntersectingSetKeys(outputSet, InsertCohesiveSetNameUPPER);
            CreateCohesiveSets(outputSet,SetInfo,cohesiveIdentifyInfo);

            //对CohesiveBoundary（边界内聚力单元）中的单元进行再次筛分创建新的set
           // extern ElementSetLabel OutsideTargetElementLabels; 全局变量已经计算好 
           //  OutputSet outputSet;    
           //给出函数 找到outputSet中所有与OutsideTargetElementLabels有交集（不管是全部在OutsideTargetElementLabels内部还是部分在内部）的set的名称返回 即返回它们的key即可
            populateCohesiveElementSets(outputSet, cohesive_elements,InsertCohesiveSetName);   //创建CohesiveInner和CohesiveBoundary集合  
            //using CohesiveIdentifyInfo = std::vector<std::vector<int>>;//内聚力单元的信息 n行3列 n代表有n个内聚力单元 第一列为内聚力单元的id 第二列为连接该内聚力单元的第一个实体单元的id 第三列为连接该内聚力单元的第二个实体单元的id 
            // CohesiveIdentifyInfo cohesiveIdentifyInfo;  std::vector<std::string> keys
            //判断outputSet["CohesiveBoundary"]中的所有内聚力单元进行再次筛分创建新的set
            //对于每一个在outputSet["CohesiveBoundary"]中的内聚力单元（记作C1），基于cohesiveIdentifyInfo信息 遍历 std::vector<std::string> keys （key：keys）
            //若C1的任意一个实体单元（有两个实体单元）在outputSet[key]中存在 则创建新的set名称为key_Boundary_coh
            //(两个都在显然不可能) 给出这个函数 最终修改到变量outputSet中 创建新的集合
            createBoundaryCohesiveSubsets(outputSet, keys, cohesiveIdentifyInfo, InsertCohesiveSetName);//创建Natural_Boundary_coh Inner_Boundary_coh



           
            Timer write_timer("Writing Inp File");  write_timer.start();// 写出计时
            write3DInp(OutputInpFileName, originNodes, originElements, cohesive_elements, outputSet, InsertCohesiveSetName);
            write_timer.stop(); write_timer.getLastTime("Write INP file time: ");//记录读取时间
        }
        else {
            std::cerr << "Failed to insert cohesive elements. Program aborted." << std::endl;
        }
        main_timer.stop();   main_timer.print();
        
    }
    else {
        std::cerr << "Failed to read INP file. Program terminated." << std::endl;
        main_timer.stop();   main_timer.print();
       
    }
    return OutputInpFileName;






    
}
std::string ConvertInpFromHyperMesh(const std::string& INPName, const std::string& jsonFilePath) {
    Timer main_timer("Main Program"); // 主程序计时
    main_timer.start();
    std::string outputName = "Convert_" + INPName;
  
    std::vector<Node3D> originNodes;
    std::vector<Element3D> originElements;
    OutputSet outputSet;

    if (read3DInpFromHyperMesh(inpFolder + "/" + INPName, originNodes, originElements, outputSet)) {
        std::cout << "读取INP文件成功！" << std::endl;
        //添加集合合并功能 比如指定某些集合 再指定新集合的名称 创建新集合 新set的单元即指定Set的所有单元
        //举个例子 读取json文件中的 所以要合并的信息 如 集合A 和 B   要合并到 NEW1 集合  （A B NEW1是集合的名称）集合C和 D   要合并到 NEW2 集合  （C D NEW2是集合的名称）
        auto merges = load_merge_config(jsonFilePath);
        MergeSetsFromConfig(outputSet, merges);
        //基于 OutputSet outputSet; 中的信息合并新的集合 然后添加到outputSet中去 要检测sources set是否存在 不存在自动忽略何必需求即可
        if (reWriteINPFile(inpFolder + "/" + outputName, originNodes, originElements, outputSet)) {
            std::cout << "从HyperMesh转换写出inp文件成功！！" << std::endl;
            main_timer.stop();
            main_timer.print();


            return outputName;

        }
        else {
            std::cout << "转换写出inp文件失败！！" << std::endl;
            return "";
        }
    }
    else {
        std::cout << "读取INP文件失败 ！程序终止" << std::endl;
        return "";
    }
    




}
void printElementNodeCoordinates(const Element3D& elem, const std::unordered_map<int, Node3D*>& NewNode_map) {
    // 定义节点 ID 数组
    int nodeIds[4] = { elem.n1, elem.n2, elem.n3, elem.n4 };

    std::cout << "Element ID: " << elem.id << " Node Coordinates:" << std::endl;

    // 遍历所有节点
    for (int i = 0; i < 4; ++i) {
        int nodeId = nodeIds[i];
        auto it = NewNode_map.find(nodeId);
        if (it != NewNode_map.end()) {
            const Node3D* node = it->second;
            std::cout << "Node " << nodeId << ": ("
                << node->x << ", "
                << node->y << ", "
                << node->z << ")" << std::endl;
        }
        else {
            std::cout << "Node " << nodeId << " not found in NewNode_map." << std::endl;
        }
    }
}
void printElementNodeCoordinates(const Element3D& elem, const std::unordered_map<int, const Node3D*>& NewNode_map) {
    // 定义节点 ID 数组
    int nodeIds[4] = { elem.n1, elem.n2, elem.n3, elem.n4 };

    std::cout << "Element ID: " << elem.id << " Node Coordinates:" << std::endl;

    // 遍历所有节点
    for (int i = 0; i < 4; ++i) {
        int nodeId = nodeIds[i];
        auto it = NewNode_map.find(nodeId);
        if (it != NewNode_map.end()) {
            const Node3D* node = it->second;
            std::cout << "Node " << nodeId << ": ("
                << node->x << ", "
                << node->y << ", "
                << node->z << ")" << std::endl;
        }
        else {
            std::cout << "Node " << nodeId << " not found in NewNode_map." << std::endl;
        }
    }
}

void PrintOutputSetInfo(const OutputSet& outputSet) {
    for (const auto& [name, elemSet] : outputSet) {
        std::cout << name << ": " << elemSet.size() << std::endl;
    }
}
std::vector<std::pair<std::string, std::vector<std::string>>> load_merge_config(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open merge config file: " << filename << std::endl;
        return {};
    }

    nlohmann::json j;
    try {
        file >> j;
    }
    catch (const nlohmann::json::parse_error& e) {
        std::cerr << "JSON parse error: " << e.what() << std::endl;
        return {};
    }

    std::vector<std::pair<std::string, std::vector<std::string>>> merges;
    std::cout << "Loading merge configuration from " << filename << ":" << std::endl;

    for (const auto& item : j) {
        if (!item.contains("target") || !item.contains("sources")) {
            std::cerr << "Warning: Skipping invalid merge rule (missing target/sources)" << std::endl;
            continue;
        }

        std::string target = item["target"].get<std::string>();
        std::vector<std::string> sources = item["sources"].get<std::vector<std::string>>();

        merges.emplace_back(target, sources);

        std::cout << "  Merge ";
        for (size_t i = 0; i < sources.size(); ++i) {
            std::cout << sources[i];
            if (i < sources.size() - 1) std::cout << " + ";
        }
        std::cout << " to " << target << std::endl;
    }

    std::cout << "Loaded " << merges.size() << " merge rule(s)." << std::endl;
    return merges;
}

void MergeSetsFromConfig(OutputSet& outputSet,
    const std::vector<std::pair<std::string, std::vector<std::string>>>& merges) {
    for (const auto& [target, sources] : merges) {
        ElementSetLabel newSet;
        bool hasValidSource = false;

        for (const auto& src : sources) {
            auto it = outputSet.find(src);
            if (it != outputSet.end()) {
                newSet.insert(it->second.begin(), it->second.end());
                hasValidSource = true;
            }
            else {
                std::cout << "  Warning: Source set '" << src << "' not found, skipping." << std::endl;
            }
        }

        if (hasValidSource) {
            outputSet[target] = std::move(newSet);
            std::cout << "  Created merged set '" << target << "' with "
                << outputSet[target].size() << " elements." << std::endl;
        }
        else {
            std::cout << "  Warning: No valid sources for '" << target << "', not creating." << std::endl;
        }
    }
}

void populateCohesiveElementSets(OutputSet& outputSet,
    const std::vector<COH3D6>& cohesive_elements, const std::string& InsertCohesiveSetName) {
    // 预估容量：每个元素只进一个集合，总容量不超过 cohesive_elements.size()
    ElementSetLabel innerSet;
    ElementSetLabel boundarySet;
    innerSet.reserve(cohesive_elements.size());
    boundarySet.reserve(cohesive_elements.size());

    for (const COH3D6& coh : cohesive_elements) {
        if (coh.type == COH3D6Type::Inner) {
            innerSet.insert(coh.id);
        }
        else if (coh.type == COH3D6Type::Boundary) {
            boundarySet.insert(coh.id);
        }
    }

    // 仅在非空时插入，避免空集合污染 outputSet
    if (!innerSet.empty()) {
        outputSet[InsertCohesiveSetName + "_Cohesive_Inner"] = std::move(innerSet);
    }
    if (!boundarySet.empty()) {
        outputSet[InsertCohesiveSetName + "_Cohesive_Boundary"] = std::move(boundarySet);
    }
}

bool isCohesiveSet(const ElementSetLabel& mySet) {
 
    int label1 = *mySet.begin();  // 获取第一个元素
    return label1 > maxElemID2;//返回真即为内聚力单元的集合
    
}
