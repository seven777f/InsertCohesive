#ifndef TOOLS_H
#define TOOLS_H

#include"Public.h"

// 3D节点结构
struct Node3D {
    Node3D() {}
    Node3D(int id, float x, float y, float z) :id(id), x(x), y(y), z(z) {}
    int id;
    float x, y, z;
};

// 3D四面体单元结构 (C3D4: 4节点)
struct Element3D {
    Element3D() {}
    Element3D(int id, int n1, int n2, int n3, int n4) :id(id), n1(n1), n2(n2), n3(n3), n4(n4) {}
    int id;
    int n1, n2, n3, n4;  // 节点ID
};
enum class COH3D6Type {
    Boundary,
    Inner,
    Outside
};
// 内聚力单元
struct  COH3D6 {
    int id;
    int n1, n2, n3, n4, n5, n6; // 节点ID
    COH3D6Type  type;
    float area = 0.0;
};


struct NormalizedFaceKey {
    int min_node_id;  // 最小节点ID
    int mid_node_id;  // 中间节点ID
    int max_node_id;  // 最大节点ID
    bool operator==(const NormalizedFaceKey& other) const {
        return min_node_id == other.min_node_id &&
            mid_node_id == other.mid_node_id &&
            max_node_id == other.max_node_id;
    }
};
namespace std {
    template <>
    struct hash<NormalizedFaceKey> {
        std::size_t operator()(const NormalizedFaceKey& key) const {
            std::size_t seed = 0;
            hash_combine(seed, key.min_node_id);
            hash_combine(seed, key.mid_node_id);
            hash_combine(seed, key.max_node_id);
            return seed;
        }
    private:
        template <typename T>
        void hash_combine(std::size_t& seed, const T& v) const {
            std::hash<T> hasher;
            seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        }
    };
}




enum class FaceConnectivityType {
    Con123,
    Con341,
    Con142,
    Con243,
};

struct FaceInfo {
    //FaceInfo() :element_id(0), node1(0), node2(0), node3(0), faceConnectivityType(FaceConnectivityType::Con123), type(COH3D6Type::Boundary) {}
    int element_id;  // 该面所属的C3D4单元ID
    int node1, node2, node3;  // 该面的原始节点连接（保持原始顺序）
    FaceConnectivityType faceConnectivityType;
    COH3D6Type  type;
};
using FaceInfoList = std::vector<FaceInfo>;


struct SharedFaceGroup {
    NormalizedFaceKey key;       // 标准化的面键
    FaceInfoList infos;          // 该键对应的所有面信息列表
};













// 读取.inp文件中的节点和C3D4单元
// 输入: 文件路径
// 输出: 填充nodes和elements，返回true表示成功
bool read3DInp(const std::string& filename, std::vector<Node3D>& nodes, std::vector<Element3D>& elements,
    OutputSet& outputSet);


bool read3DInpFromHyperMesh(const std::string& filename, std::vector<Node3D>& nodes, std::vector<Element3D>& elements,
    OutputSet& outputSet);
bool reWriteINPFile(const std::string& filename, const std::vector<Node3D>& nodes,
    const std::vector<Element3D>& elements, const OutputSet& outputSet);


// 辅助函数：标准化三角面（排序节点ID: min, mid, max）
NormalizedFaceKey normalizeFace(int n1, int n2, int n3);



int getMaxNodeId(const std::vector<Node3D>& nodes);
int getMaxElementId(const std::vector<Element3D>& elements);









FaceInfo GetFace2(const Element3D* elem, FaceConnectivityType type);

std::vector<FaceInfo> GetFaces2(const Element3D& elem);



std::vector<FaceInfoList> GetSharedFaces2(const std::vector<Element3D*>& original_elements, const std::unordered_set<int>& MatchingElementLabels);

std::vector<Element3D*> getSetElements2(const std::unordered_map<int, Element3D*>& Elem_map,
    const std::unordered_set<int>& TargetElementLabelSet);

// 辅助函数：比较两节点坐标是否相同（考虑浮点误差）
bool areNodesEqual(const Node3D& n1, const Node3D& n2, double tol = 0.001);
// 调整 Newf2 节点顺序，使其与 Newf1 坐标对齐

void alignFaceNodes2(FaceInfo& Newf2, const FaceInfo& Newf1, const std::unordered_map<int, Node3D*>& node_map);


void write3DInp(const std::string& filename, const std::vector<Node3D>& NewNodes,
    const std::vector<Element3D>& NewElements,
    const std::vector<COH3D6>& cohesive_elements,
    const OutputSet& outputSet, const std::string& InsertCohesiveSetName, const bool setByOrder=true,  const bool outputNodeSet=false);



std::unordered_map<int, std::vector<EN_ID>> countNodeUsageInSet4(const std::unordered_map<int, Element3D*>& elem_map,
    const std::unordered_set<int>& TargetElementLabelSet,
    int& maxNodeID, const std::unordered_set<int>& MatchingElementLabels);

std::unordered_map<int,  Element3D*> creatElementMap(std::vector<Element3D>& elements);
std::unordered_map<int,Node3D*> creatNodeMap(std::vector<Node3D>& nodes);




void addNodes(std::vector<Node3D>& nodes, std::unordered_map<int, Node3D*>& node_map, const std::unordered_map<int, std::vector<EN_ID>>& nodeID_EN_IDs);

void ModifyElements(const std::unordered_map<int, std::vector<EN_ID>>& nodeID_EN_IDs, std::unordered_map<int, Element3D*>& elem_map, const std::unordered_set<int>& MatchingElementLabels);

void GetElementsOutsideTargetSet(const std::unordered_map<int, Element3D*>& Elem_map,
    const std::unordered_set<int>& TargetElementLabelSet);


std::unordered_set<int> GetMatchingElements(
    const std::unordered_map<int, Element3D*>& Elem_map,
    const std::unordered_set<int>& TargetElementLabelSet);

void MergeElementLabelSets2(std::unordered_set<int>& TargetElementLabelSet,
    const std::unordered_set<int>& MatchingElementLabels);




std::vector<std::pair<std::string, std::string>> FindInset(const OutputSet& outputSet, const std::string& TargetSetName);
std::vector<std::string> findIntersectingSetKeys(const OutputSet& outputSet, const std::string& TargetSetName);



COH3D6Type GetCohesiveType(const int id1, const int id2, const std::unordered_set<int>& MatchingElementLabels);

bool insertCohesiveElementsToSet2(std::vector<Node3D>& Nodes, std::vector<Element3D>& Elements,
    std::unordered_set<int> TargetElementLabelSet, std::vector<COH3D6>& cohesive_elements,CohesiveIdentifyInfo& cohesiveIdentifyInfo);

std::string Insert3D(const std::string& INPName, const std::string& InsertCohesiveSetName);
std::string ConvertInpFromHyperMesh(const std::string& INPName, const std::string& jsonFilePath);
std::vector<std::pair<std::string, std::vector<std::string>>> load_merge_config(const std::string& filename);
void MergeSetsFromConfig(OutputSet& outputSet,
    const std::vector<std::pair<std::string, std::vector<std::string>>>& merges);
//打印信息的函数
void printSortedTargetElementLabelSet(const std::unordered_set<int>& TargetElementLabelSet);
// 打印节点信息 (前10个节点)
void printNodes(const std::vector<Node3D>& nodes);
void printNodeMap(const std::unordered_map<int, Node3D*>& node_map);
void printElementNodeMap(const std::unordered_map<int, std::vector<EN_ID>>& elementNodeMap);
// 打印单元信息 (前10个单元)
void printElements(const std::vector<Element3D>& elements);

// 打印所有节点和单元摘要 (数量 + 前10个)
void printMeshSummary(const std::vector<Node3D>& nodes, const std::vector<Element3D>& elements);
// 打印 NormalizedFaceKey
void printNormalizedFaceKey(const NormalizedFaceKey& key, std::ostream& os = std::cout);
// 打印 Node3D
void printNode3D(const Node3D& node, std::ostream& os = std::cout);

// 打印 Element3D
void printElement3D(const Element3D& elem, std::ostream& os = std::cout);

// 打印 COH3D6
void printCOH3D6(const COH3D6& coh_elem, std::ostream& os = std::cout);

// 打印 FaceInfo
void printFaceInfo(const FaceInfo& face, std::ostream& os = std::cout);

// 打印 SharedFaceGroup
void printSharedFaceGroup(const SharedFaceGroup& group, std::ostream& os = std::cout);
void printElementNodeCoordinates(const Element3D& elem, const std::unordered_map<int, const Node3D*>& NewNode_map);
void printElementNodeCoordinates(const Element3D& elem, const std::unordered_map<int, Node3D*>& NewNode_map);
void PrintOutputSetInfo(const OutputSet& outputSet);
void MergeSetsFromConfig(OutputSet& outputSet,
    const std::vector<std::pair<std::string, std::vector<std::string>>>& merges);
void populateCohesiveElementSets(OutputSet& outputSet,
    const std::vector<COH3D6>& cohesive_elements, const std::string& InsertCohesiveSetName);
bool isCohesiveSet(const ElementSetLabel& mySet);

#endif // TOOLS_H