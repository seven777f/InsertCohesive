#include "ToolFor2D.h"

static int maxElemID2 = 1;//C3D4单元的最大单元label编号  （因为新插入的内聚力单元的ID都是maxElemID++ 所以用此区分是C3D4单元函数 COH3D6单元）
static ElementSetLabel OutsideTargetElementLabels;
static std::string Element2DName = "";
static std::string PartName = "";
bool read2DInp(const std::string& filename,
    std::vector<Node2D>& nodes,
    std::vector<Element2D>& elements,
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

      

        // 直接转大写，和你3D版一模一样
        std::transform(line.begin(), line.end(), line.begin(), ::toupper);

        // 跳过空行
        if (line.empty()) continue;

        // 跳过注释行或空行
        if (line[0] == '*') {
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
            if (line.find("*NODE") != std::string::npos) {
                node_section = true;
                element_section = elset_section = false;
                continue;
            }
            if (line.find("*ELEMENT,TYPE=CPE3") != std::string::npos ||
                line.find("*ELEMENT, TYPE=CPE3") != std::string::npos) {
                Element2DName = "CPE3";                     // 记住是 CPE3
                element_section = true;
                node_section = elset_section = false;
                continue;
            }
            if (line.find("*ELEMENT,TYPE=CPS3") != std::string::npos ||
                line.find("*ELEMENT, TYPE=CPS3") != std::string::npos) {
                Element2DName = "CPS3";                     // 记住是 CPS3
                element_section = true;
                node_section = elset_section = false;
                continue;
            }
            // 解析任意 *ELSET
            if (line.find("*ELSET, ELSET=") != std::string::npos) {
                // 检查是否有 generate
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
            Node2D node;
            if (!(ss >> node.id >> node.x >> node.y)) {
                std::cerr << "Warning: Invalid node line: " << line << std::endl;
                continue;
            }
            nodes.push_back(node);
        }

        // 读取单元 CPE3/CPS3
        if (element_section && line[0] != '*' && line[0] != '#') {
            std::replace(line.begin(), line.end(), ',', ' ');
            std::istringstream ss(line);
            Element2D elem;
            if (!(ss >> elem.id >> elem.n1 >> elem.n2 >> elem.n3)) {
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

    std::cout << "Read " << nodes.size() << " nodes and " << elements.size()
        << " "<< Element2DName <<" elements" << std::endl;
    for (const auto& [name, set] : outputSet) {
        std::cout << "Read element set " << name << " with " << set.size() << " labels." << std::endl;
    }

    return true;
}

void write2DInp(const std::string& filename,
    const std::vector<Node2D>& NewNodes,
    const std::vector<Element2D>& NewElements,
    const std::vector<COH2D4>& cohesive_elements,
    const OutputSet& outputSet, const std::string& InsertCohesiveSetName,
    const bool setByOrder, const bool outputNodeSet) {
    std::string partname = PartName + "(" + InsertCohesiveSetName + ")";
    std::ofstream out(filename);
    if (!out.is_open()) {
        std::cerr << "Error: Unable to open file " << filename << " for writing." << std::endl;
        return;
    }

    // 设置输出格式：固定点，7位小数（你原来就是7位）
    out << std::fixed << std::setprecision(7);

    // 写入头部（和你3D版一模一样）
    out << "*Heading\n";
    out << "** Job name: LJZshabi Model name: Job\n";
    out << "** Generated by: C++ Program\n";
    out << "*Preprint, echo=NO, model=NO, history=NO, contact=NO\n";
    out << "**\n";
    out << "** PARTS\n";
    out << "**\n";
    out << "*Part, name="<< partname << "\n";

    // 写入节点 (*Node) —— 2D 只有 x,y
    out << "*Node\n";
    for (const auto& node : NewNodes) {
        out << std::setw(7) << node.id << ","
            << std::setw(15) << node.x << ","
            << std::setw(15) << node.y << "\n";
    }
    std::cout << NewNodes.size() << " Nodes has been writen to inp file!" << std::endl;

    // 写入 CPE3/CPS3 单元
    if (!NewElements.empty()) {
        out << "*Element, type=" << Element2DName << "\n";   // 自动写 CPE3 或 CPS3
        for (const auto& elem : NewElements) {
            out << std::setw(7) << elem.id << ","
                << std::setw(7) << elem.n1 << ","
                << std::setw(7) << elem.n2 << ","
                << std::setw(7) << elem.n3 << "\n";
        }
        std::cout << NewElements.size() << " Elements[" << Element2DName
            << "] has been writen to inp file!" << std::endl;
    }

    // 写入 COH2D4 单元
    if (!cohesive_elements.empty()) {
        out << "*Element, type=COH2D4\n";
        for (const auto& coh : cohesive_elements) {
            out << std::setw(7) << coh.id << ","
                << std::setw(7) << coh.n1 << ","
                << std::setw(7) << coh.n2 << ","
                << std::setw(7) << coh.n3 << ","
                << std::setw(7) << coh.n4 << "\n";
        }
        std::cout << cohesive_elements.size() << " Elements[COH2D4] has been writen to inp file!" << std::endl;
    }

    // 输出内聚力单元的集合
    int cohesive_num = cohesive_elements.size();
    if (!cohesive_elements.empty()) {
        out << "*Elset, elset=AllCohesiveElements\n";
        for (size_t i = 0; i < cohesive_num; ++i) {
            out << " " << cohesive_elements[i].id;
            if (i < cohesive_num - 1) out << ",";
            if ((i + 1) % 16 == 0 || i == cohesive_num - 1) out << "\n";
        }
        std::cout << "Write elementSet: AllCohesiveElements[COH2D4] Element number:" << cohesive_num << std::endl;
    }

    // 写入集合 (*Elset) —— 完全复用你3D逻辑
    for (const auto& [elsetName, elementSet] : outputSet) {
        if (!elementSet.empty()) {
            std::vector<int> labels(elementSet.begin(), elementSet.end());
            if (setByOrder) std::sort(labels.begin(), labels.end());

            out << "*Elset, elset=" << elsetName << "\n";
            size_t i = 0;
            for (const int& label : labels) {
                out << " " << label;
                if (i < elementSet.size() - 1) out << ",";
                if ((i + 1) % 16 == 0 || i == elementSet.size() - 1) out << "\n";
                ++i;
            }

            std::string ElementType = is2DCohesiveSet(elementSet) ? "[COH2D4]" :("["+ Element2DName+ "]") ;
            std::cout << "Write elementSet: " << elsetName << ElementType << " , Element number:" << elementSet.size() << std::endl;
        }
    }
    //输出Node的集合
    if (outputNodeSet) {
        // 2D 边界节点集合：Xmin, Xmax, Ymin, Ymax（Z 方向没有了）
        float xMin = std::numeric_limits<float>::max();
        float xMax = std::numeric_limits<float>::lowest();
        float yMin = std::numeric_limits<float>::max();
        float yMax = std::numeric_limits<float>::lowest();

        for (const auto& node : NewNodes) {
            xMin = std::min(xMin, node.x);
            xMax = std::max(xMax, node.x);
            yMin = std::min(yMin, node.y);
            yMax = std::max(yMax, node.y);
        }

        const double tolerance = 1e-6;





        std::vector<int> xMinNodes, xMaxNodes, yMinNodes, yMaxNodes;
        for (const auto& node : NewNodes) {
            if (std::abs(node.x - xMin) < tolerance) xMinNodes.push_back(node.id);
            if (std::abs(node.x - xMax) < tolerance) xMaxNodes.push_back(node.id);
            if (std::abs(node.y - yMin) < tolerance) yMinNodes.push_back(node.id);
            if (std::abs(node.y - yMax) < tolerance) yMaxNodes.push_back(node.id);
        }

        auto writeNodeSet = [&](const std::vector<int>& ns, const std::string& name) {
            if (!ns.empty()) {
                out << "*Nset, nset=" << name << "\n";
                for (size_t i = 0; i < ns.size(); ++i) {
                    out << " " << ns[i];
                    if (i < ns.size() - 1) out << ",";
                    if ((i + 1) % 16 == 0 || i == ns.size() - 1) out << "\n";
                }
            }
            };

        writeNodeSet(xMinNodes, "XMinNodes");
        writeNodeSet(xMaxNodes, "XMaxNodes");
        writeNodeSet(yMinNodes, "YMinNodes");
        writeNodeSet(yMaxNodes, "YMaxNodes");
    }
    

    // 结束
    out << "*End Part\n";
    out.close();
    
    std::cout << "Write INP File (PartName="<< partname <<") to: " << filename << " successfully!!!" << std::endl;
}



bool is2DCohesiveSet(const ElementSetLabel& mySet) {

    int label1 = *mySet.begin();  // 获取第一个元素
    return label1 > maxElemID2;//返回真即为内聚力单元的集合

}


std::string Insert2D(const std::string& INPName, const std::string& InsertCohesiveSetName) {
    std::string OutputInpFileName = OutputFolder + "/NewOutput_" + INPName;
    std::string INPFilePath = inpFolder + "/" + INPName;
    Timer main_timer("Main Program");  main_timer.start();// 主程序计时
   
    // 设置线程数
    std::cout << "Hardware concurrency: " << std::thread::hardware_concurrency() << std::endl;
    omp_set_dynamic(0);
    omp_set_num_threads(std::thread::hardware_concurrency());
    //std::cout << "OpenMP version: " << _OPENMP << std::endl;

    std::vector<Node2D> originNodes; std::vector<Element2D> originElements;
    std::unordered_set<int> TargetElementLabelSet; OutputSet outputSet;
    std::vector<COH2D4> cohesive_elements; CohesiveIdentifyInfo cohesiveIdentifyInfo;//TargetElementLabelSet中插入的所有内聚力单元
   
   

    Timer read_timer("Reading Inp File"); // 主程序计时
    read_timer.start();
    if (read2DInp(INPFilePath, originNodes, originElements, outputSet))
    {
        
        read_timer.stop(); read_timer.getLastTime("Read INP file time: ");//记录读取时间
        std::cout << "InsertCohesiveSetName  : " << InsertCohesiveSetName << std::endl;
        std::string InsertCohesiveSetNameUPPER = InsertCohesiveSetName;
        std::transform(InsertCohesiveSetNameUPPER.begin(), InsertCohesiveSetNameUPPER.end(), InsertCohesiveSetNameUPPER.begin(), ::toupper);//统一转换为大写
        if (insert2DCohesiveElementsToSet(originNodes, originElements, outputSet[InsertCohesiveSetNameUPPER], cohesive_elements, cohesiveIdentifyInfo))
        {
            std::vector<std::pair<std::string, std::string>> SetInfo = Find2DInset(outputSet, InsertCohesiveSetNameUPPER);
            std::vector<std::string> keys = find2DIntersectingSetKeys(outputSet, InsertCohesiveSetNameUPPER);
            CreateCohesiveSets(outputSet, SetInfo, cohesiveIdentifyInfo);
           //给出函数 找到outputSet中所有与OutsideTargetElementLabels有交集（不管是全部在OutsideTargetElementLabels内部还是部分在内部）的set的名称返回 即返回它们的key即可
            populate2DCohesiveElementSets(outputSet, cohesive_elements, InsertCohesiveSetName);   //创建CohesiveInner和CohesiveBoundary集合  
            createBoundaryCohesiveSubsets(outputSet, keys, cohesiveIdentifyInfo, InsertCohesiveSetName);//创建Natural_Boundary_coh Inner_Boundary_coh


            Timer write_timer("Writing Inp File");  write_timer.start();// 写出计时
            write2DInp(OutputInpFileName, originNodes, originElements, cohesive_elements, outputSet, InsertCohesiveSetName);
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

int getMax2DNodeId(const std::vector<Node2D>& nodes) {
    int max_node_id = 0;
    for (const auto& node : nodes) {
        max_node_id = std::max(max_node_id, node.id);
    }
    return max_node_id;
}

int getMax2DElementId(const std::vector<Element2D>& elements) {
    int max_elem_id = 0;
    for (const auto& elem : elements) {
        max_elem_id = std::max(max_elem_id, elem.id);
    }
    return max_elem_id;
}

std::unordered_map<int, Element2D*> creat2DElementMap(std::vector<Element2D>& elements) {
    std::unordered_map<int, Element2D*> elem_map;
    elem_map.reserve(elements.size());
    for (auto& elem : elements) {
        //  elem_map[elem.id] = &elem;
        elem_map.emplace(elem.id, &elem);  // 直接构造键值对，无多余操作
    }
    return elem_map;
}


std::unordered_map<int, Node2D*> creat2DNodeMap(std::vector<Node2D>& nodes) {
    std::unordered_map<int, Node2D*> node_map;
    node_map.reserve(nodes.size());
    for (auto& node : nodes) {
        node_map[node.id] = &node;
    }
    return node_map;
}
void Get2DElementsOutsideTargetSet(const std::unordered_map<int, Element2D*>& Elem_map,
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
//找到目标集合外圈的一层单元
std::unordered_set<int> GetMatching2DElements(const std::unordered_map<int, Element2D*>& Elem_map,
    const std::unordered_set<int>& TargetElementLabelSet)
{
    // 第一步：收集目标单元涉及的所有节点
    std::unordered_set<int> NodeLabels;
    NodeLabels.reserve(TargetElementLabelSet.size() * 4); // 每个单元4个节点
    for (int label : TargetElementLabelSet) {
        auto it = Elem_map.find(label);
        if (it != Elem_map.end() && it->second) {
            const Element2D* elem = it->second;
            NodeLabels.insert(elem->n1);
            NodeLabels.insert(elem->n2);
            NodeLabels.insert(elem->n3);
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
      

        if (matchCount >= 2) {
            MatchingElementLabels.insert(label);
        }
    }

    return MatchingElementLabels;
}
void Merge2DElementLabelSets(std::unordered_set<int>& TargetElementLabelSet,
    const std::unordered_set<int>& MatchingElementLabels)
{
    TargetElementLabelSet.insert(MatchingElementLabels.begin(), MatchingElementLabels.end());
}


std::vector<Element2D*> getSet2DElements(const std::unordered_map<int, Element2D*>& Elem_map,
    const std::unordered_set<int>& TargetElementLabelSet) {
    std::vector<Element2D*> SetElements;

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

NormalizedEdgeKey normalizeEdge(int a, int b) {
    return (a < b) ? NormalizedEdgeKey{ a, b } : NormalizedEdgeKey{ b, a };
}
std::vector<EdgeInfo> GetEdges(const Element2D& elem) {


    int id = elem.id;
    return
    {
        {id, elem.n1, elem.n2,EdgeConnectivityType::Edge12},
        {id, elem.n2, elem.n3 ,EdgeConnectivityType::Edge23},
        {id, elem.n3, elem.n1 ,EdgeConnectivityType::Edge31},

    };
}
EdgeInfo GetEdge(const Element2D* elem, EdgeConnectivityType type) {

    switch (type)
    {
        case EdgeConnectivityType::Edge12: return { elem->id, elem->n1, elem->n2 };
        case EdgeConnectivityType::Edge23: return { elem->id, elem->n2, elem->n3};
        case EdgeConnectivityType::Edge31: return { elem->id, elem->n3, elem->n1 };
        default: return {};
    }


}

COH2D4Type Get2DCohesiveType(const int id1, const int id2, const std::unordered_set<int>& MatchingElementLabels) {
    int count = 0;
    if (MatchingElementLabels.count(id1)) count++;
    if (MatchingElementLabels.count(id2)) count++;
    if (count == 0) {
        return COH2D4Type::Inner;
    }
    else if (count == 1) {
        return COH2D4Type::Boundary;
    }
    else if (count == 2) {
        return COH2D4Type::Outside;
    }
}
// 辅助函数：比较两节点坐标是否相同（考虑浮点误差）
bool are2DNodesEqual(const Node2D& n1, const Node2D& n2, double tol ) {
    return std::abs(n1.x - n2.x) < tol &&
        std::abs(n1.y - n2.y) < tol;
}
std::vector<EdgeInfoList> GetSharedEdges(const std::vector<Element2D*>& original_elements, const std::unordered_set<int>& MatchingElementLabels)
{
    if (original_elements.size() == 0)  return {};
    std::unordered_map<NormalizedEdgeKey, EdgeInfoList> edge_to_infos;
    edge_to_infos.reserve(original_elements.size() * 4);
    std::vector<EdgeInfoList> shared_groups;
    shared_groups.reserve(original_elements.size() * 2);

    // 并行生成面
#pragma omp parallel
    {
        if (omp_get_thread_num() == 0) {
            std::cout << "Number of threads: " << omp_get_num_threads() << std::endl;
        }
        std::unordered_map<NormalizedEdgeKey, EdgeInfoList> local_edge_to_infos;
        local_edge_to_infos.reserve(original_elements.size() * 4 / omp_get_num_threads());

#pragma omp for
        for (int i = 0; i < original_elements.size(); ++i) {
            const Element2D* elem = original_elements[i];

            std::vector<EdgeInfo> edges = GetEdges(*elem);//得到一个单元(CPS3/CPE3)单元的所有边的信息

            for (const auto& edge : edges) {
                NormalizedEdgeKey keyEdge = normalizeEdge(edge.node1, edge.node2);
                local_edge_to_infos[keyEdge].push_back(edge);
            }
        }

        // 合并本地映射到全局（需锁）
#pragma omp critical
        for (const auto& [key, infos] : local_edge_to_infos) {
            EdgeInfoList& global_infos = edge_to_infos[key];
            global_infos.insert(global_infos.end(), infos.begin(), infos.end());
        }
    }

    // 筛选共享边

    for (auto& [_, infos] : edge_to_infos) {
        if (infos.size() == 2) {
            COH2D4Type type = Get2DCohesiveType(infos[0].element_id, infos[1].element_id, MatchingElementLabels);
            if (type != COH2D4Type::Outside) {
                infos[0].type = type;
                shared_groups.push_back(infos);
            }

        }
    }
    std::cout << "Number of shared edges found: " << shared_groups.size() << std::endl;
    return shared_groups;
}

std::unordered_map<int, std::vector<EN_ID>> count2DNodeUsageInSet(const std::unordered_map<int, Element2D*>& elem_map,
    const std::unordered_set<int>& TargetElementLabelSet,
    int& maxNodeID, const std::unordered_set<int>& MatchingElementLabels) {
    // 第一步：统计节点使用次数
    std::unordered_map<int, int> node_usage; // key: 节点 label, value: 使用次数
    node_usage.reserve(TargetElementLabelSet.size() * 4);
    //得到每个节点 被哪些单元所拥有的单元id
    std::unordered_map<int, std::vector<int>> node_to_elements; // key: 节点 label, value: 拥有该节点的单元 ID 列表
    node_to_elements.reserve(TargetElementLabelSet.size() * 4);

    for (int elem_id : TargetElementLabelSet) {
        auto it = elem_map.find(elem_id);
        if (it != elem_map.end()) {
            const auto& elem = *(it->second);
            node_usage[elem.n1]++;
            node_usage[elem.n2]++;
            node_usage[elem.n3]++;
            
            node_to_elements[elem.n1].push_back(elem_id);
            node_to_elements[elem.n2].push_back(elem_id);
            node_to_elements[elem.n3].push_back(elem_id);
          
        }
    }

    // 第二步：构建结果 map，只处理共享节点
    std::unordered_map<int, std::vector<EN_ID>> nodeID_EN_IDs;//旧的node id <---->
    nodeID_EN_IDs.reserve(node_usage.size());

    for (const auto& [node_label, count] : node_usage) {
        if (count > 1) { // 被使用多次的节点 只处理共享节点
            const std::vector<int>& owning_elements = node_to_elements[node_label];
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

         





        }
    }

    return nodeID_EN_IDs;
}

void add2DNodes(std::vector<Node2D>& OriginNodes, std::unordered_map<int, Node2D*>& node_map, const std::unordered_map<int, std::vector<EN_ID>>& nodeID_EN_IDs) {
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
    node_map = creat2DNodeMap(OriginNodes);
    // 遍历并添加新节点
    for (const auto& [NodeID, value] : nodeID_EN_IDs) {
        auto it = node_map.find(NodeID);
        if (it == node_map.end()) {
            std::cerr << "Error: Node ID " << NodeID << " not found in node_map.\n";
            continue; // 跳过此节点
        }
        const Node2D& originalNode = *(it->second);
        for (const auto& en : value) {
            OriginNodes.emplace_back(en.NewNode_id, originalNode.x, originalNode.y);
        }
    }
    //重新更新node_map
    for (size_t i = 0; i < OriginNodes.size(); ++i) {
        int newNodeId = OriginNodes[i].id;
        node_map[newNodeId] = &OriginNodes[i];
    }

}
void Modify2DElements(const std::unordered_map<int, std::vector<EN_ID>>& nodeID_EN_IDs, std::unordered_map<int, Element2D*>& elem_map, const std::unordered_set<int>& MatchingElementLabels) {
    // 遍历所有共享节点及其新节点映射
    for (const auto& [NodeID, value] : nodeID_EN_IDs) {
        // NodeID 是共享节点的原始 ID，value 包含需要修改的单元和对应新节点 ID
        for (const EN_ID& en : value) {

            int elem_ID = en.element_id;//需要修改的单元的id

            if (MatchingElementLabels.find(elem_ID) != MatchingElementLabels.end()) continue;

            int NewNodeID = en.NewNode_id;//新的节点的id

            // 查找单元
            auto it = elem_map.find(elem_ID);
            if (it == elem_map.end()) {
                std::cerr << "Warning: Element ID " << elem_ID << " not found in elem_map.\n";
                continue; // 跳过不存在的单元
            }

            Element2D& elem = *(it->second);
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
            if (!replaced) {
                std::cerr << "Warning: Node ID " << NodeID << " not found in Element ID " << elem_ID << ".\n";
            }
        }
    }
}


void alignEdgeNodes(EdgeInfo& Newe2, const EdgeInfo& Newe1, const std::unordered_map<int, Node2D*>& node_map) {
    // 存储 Newf1 和 Newf2 的节点
    int e1_nodes[2] = { Newe1.node1, Newe1.node2 };
    int e2_nodes[2] = { Newe2.node1, Newe2.node2};

    /* std::cout << "Original Newf2: " << Newf2.node1 << " " << Newf2.node2 << " " << Newf2.node3 << std::endl;
     std::cout << "Newf1: " << Newf1.node1 << " " << Newf1.node2 << " " << Newf1.node3 << std::endl;*/

     // 匹配 Newf2 节点到 Newf1 节点
    int new_e2_nodes[2] = { 0, 0 }; // 初始化新顺序
    for (int i = 0; i < 2; ++i) {
        auto it1 = node_map.find(e1_nodes[i]);
        if (it1 == node_map.end()) {
            std::cerr << "Error: Node ID " << e1_nodes[i] << " not found in node_map." << std::endl;
            Newe2.node1 = Newe2.node2  = 0;
            return;
        }
        const Node2D& n1 = *(it1->second);
        bool found = false;
        for (int j = 0; j < 2; ++j) {
            auto it2 = node_map.find(e2_nodes[j]);
            if (it2 == node_map.end()) {
                std::cerr << "Error: Node ID " << e2_nodes[j] << " not found in node_map." << std::endl;
                Newe2.node1 = Newe2.node2  = 0;
                return;
            }
            const Node2D& n2 = *(it2->second);
            if (are2DNodesEqual(n1, n2)) {
                new_e2_nodes[i] = e2_nodes[j]; // 直接将 Newf2 的节点分配到 Newf1 的对应位置
                // std::cout << "Matched: Newf1 Node " << f1_nodes[i] << " with Newf2 Node " << f2_nodes[j] << std::endl;
                found = true;
                break;
            }
        }
        if (!found) {
            std::cerr << "Error: No match found for Newf1 Node " << e1_nodes[i] << std::endl;
            Newe2.node1 = Newe2.node2  = 0;
            return;
        }
    }

    // 更新 Newf2 的节点顺序
    Newe2.node1 = new_e2_nodes[0];
    Newe2.node2 = new_e2_nodes[1];
   

    /*  std::cout << "Aligned Newf2: [" << Newf2.node1 << "," << Newf2.node2 << "," << Newf2.node3
          << "] to match Newf1: [" << Newf1.node1 << "," << Newf1.node2 << "," << Newf1.node3 << "]" << std::endl;*/
}


bool insert2DCohesiveElementsToSet(std::vector<Node2D>& Nodes, std::vector<Element2D>& Elements,
    std::unordered_set<int> TargetElementLabelSet, std::vector<COH2D4>& cohesive_elements, CohesiveIdentifyInfo& cohesiveIdentifyInfo) {
    Timer main_timer("Cohesive element insertion"); // 主程序计时
    main_timer.start();


    cohesiveIdentifyInfo.clear();





    if (TargetElementLabelSet.size() == 0 || Nodes.size() == 0 || Elements.size() == 0) return false;

    if (TargetElementLabelSet.size() > Elements.size()) return false;




    int maxNodeID = getMax2DNodeId(Nodes);
    int maxElemID = getMax2DElementId(Elements); maxElemID2 = maxElemID;//记录普通单元的最大编号 用于区分是否为内聚力单元还是普通单元




    //步骤一： 获取新的单元列表 和节点列表
    std::unordered_map<int, Element2D*> Elem_map = creat2DElementMap(Elements);
    Get2DElementsOutsideTargetSet(Elem_map, TargetElementLabelSet);//不用于插入内聚力单元 用于后续筛分boundary内聚力单元的




    //找到目标集合外圈的一层单元
    std::unordered_set<int> MatchingElementLabels = GetMatching2DElements(Elem_map, TargetElementLabelSet);



    //合并MatchingElementLabels（外圈单元）到目标集合中
    Merge2DElementLabelSets(TargetElementLabelSet, MatchingElementLabels);




    std::vector<Element2D*> SetElements = getSet2DElements(Elem_map, TargetElementLabelSet);
    //步骤二：获取共享面：基于原来的单元列表获取共享面
    std::vector<EdgeInfoList> SharedEdges = GetSharedEdges(SetElements, MatchingElementLabels);
    int cohesive_num = SharedEdges.size();  
    if (cohesive_num == 0) {
        std::cerr << "No shared edges found. Insertion failed." << std::endl;
        return true;
    }
    cohesive_elements.resize(cohesive_num);
    cohesiveIdentifyInfo.resize(cohesive_num, std::vector<int>(3, 0));



    std::unordered_map<int, std::vector<EN_ID>> nodeID_EN_IDs = count2DNodeUsageInSet(Elem_map, TargetElementLabelSet, maxNodeID, MatchingElementLabels);

    // printNodeMap(node_map);


    std::unordered_map<int, Node2D*> node_map;
    add2DNodes(Nodes, node_map, nodeID_EN_IDs);//修改了Nodes和node_map
    std::cout << "Total nodes after insertion: " << Nodes.size() << std::endl;



    Modify2DElements(nodeID_EN_IDs, Elem_map, MatchingElementLabels);//修改了Elem_map和originElements
 
    std::cout << "Total elements after insertion (" << Element2DName << " count unchanged): " << Elem_map.size() << std::endl;





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


    std::cout << "OpenMP parallel computation enabled" << std::endl;
#pragma omp parallel
    {
        if (omp_get_thread_num() == 0) {
            std::cout << "Number of threads: " << omp_get_num_threads() << std::endl;
        }
#pragma omp for schedule(static)
        for (int i = 0; i < SharedEdges.size(); ++i) {
            const EdgeInfoList& ShareEdgeList = SharedEdges[i];
            COH2D4 coh_elem;

            if (ShareEdgeList.size() != 2) {
                errors[i] = "ShareEdgeList.size() != 2 exception at index " + std::to_string(i);
                ++error_count;
                continue;
            }
            int elem1_id = ShareEdgeList[0].element_id;//第一个边所属的单元id
            EdgeConnectivityType f1_Type = ShareEdgeList[0].edgeType;
            int elem2_id = ShareEdgeList[1].element_id;//第二个边所属的单元id
            EdgeConnectivityType f2_Type = ShareEdgeList[1].edgeType;
            COH2D4Type type = ShareEdgeList[0].type;
            // 查找新单元
            auto it1 = Elem_map.find(elem1_id);//新的map
            auto it2 = Elem_map.find(elem2_id);
            if (it1 == Elem_map.end() || it2 == Elem_map.end()) {
                errors[i] = "Error: Element ID " + std::to_string(it1 == Elem_map.end() ? elem1_id : elem2_id)
                    + " not found in NewElements at index " + std::to_string(i);
                ++error_count;
                continue;
            }
            // 获取新边
            const Element2D* Elem1 = it1->second;//新的element 节点更新过的
            const Element2D* Elem2 = it2->second;//新的element 节点更新过的

            EdgeInfo Newf1 = GetEdge(Elem1, f1_Type);//A B C
            EdgeInfo Newf2 = GetEdge(Elem2, f2_Type);// A' B' C'
           //  std::cout <<"Before: " << Newf1.node1 << " " << Newf1.node2  << " " << Newf2.node1 << " " << Newf2.node2  << std::endl;
            alignEdgeNodes(Newf2, Newf1, node_map);//调整Newf2向Newf1对齐.. node_map是只读的
            //std::cout << "After: " << Newf1.node1 << " " << Newf1.node2  << " " << Newf2.node1 << " " << Newf2.node2  << std::endl;
            if (Newf2.node1 == 0) { // 无效 Newf2
                errors[i] = "Error: Failed to align nodes for COH3D6 at index " + std::to_string(i);
                ++error_count;
                continue;
            }

            coh_elem.id = maxElemIDs[i];
            coh_elem.n1 = Newf1.node2; coh_elem.n2 = Newf1.node1; 
            coh_elem.n3 = Newf2.node1; coh_elem.n4 = Newf2.node2; 
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
    std::cout << "Successful!!!  Inserted " << cohesive_elements.size() << " COH2D4 elements." << std::endl;
    return true;
    
    




}

std::vector<std::pair<std::string, std::string>> Find2DInset(const OutputSet& outputSet, const std::string& TargetSetName) {
    auto it = outputSet.find(TargetSetName);
    if (it == outputSet.end()) {
        return {};
    }
    const auto& target = it->second;

    std::vector<std::pair<std::string, std::string>> results;
    for (const auto& [name, elset] : outputSet) {
        if (name == TargetSetName || name == "BASEELEMENTS" || elset.empty())  continue;
        if (is2DCohesiveSet(elset))  continue;//如果是内聚力单元的集合 也不用找了
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
std::vector<std::string> find2DIntersectingSetKeys(
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
        if (is2DCohesiveSet(elemSet)) continue;
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

void populate2DCohesiveElementSets(OutputSet& outputSet, const std::vector<COH2D4>& cohesive_elements, const std::string& InsertCohesiveSetName)
{
    // 预估容量：每个元素只进一个集合，总容量不超过 cohesive_elements.size()
    ElementSetLabel innerSet;
    ElementSetLabel boundarySet;
    innerSet.reserve(cohesive_elements.size());
    boundarySet.reserve(cohesive_elements.size());

    for (const COH2D4& coh : cohesive_elements) {
        if (coh.type == COH2D4Type::Inner) {
            innerSet.insert(coh.id);
        }
        else if (coh.type == COH2D4Type::Boundary) {
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