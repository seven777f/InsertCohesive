#ifndef TOOLS2_H
#define TOOLS2_H


#include"Public.h"


// 2D节点结构
struct Node2D {
    Node2D() {}
    Node2D(int id, float x, float y) :id(id), x(x), y(y){}
    int id;// 节点ID
    float x, y;//节点坐标
};

// 2D平面单元 CPS3 CPE3
struct Element2D {
    Element2D() {}
    Element2D(int id, int n1, int n2, int n3) :id(id), n1(n1), n2(n2), n3(n3) {}
    int id;//单元ID
    int n1, n2, n3;  // 节点ID
};

enum class COH2D4Type {//对于插入集合而言该内聚力单元的位置
    Boundary,//插入集合的边缘区域的单元
    Inner,//插入集合的内部区域的单元
    Outside//不在插入区域的单元
};
// 内聚力单元
struct  COH2D4 {
    int id;//内聚力单元id
    int n1, n2, n3, n4; // 节点ID
    COH2D4Type  type;//相当于插入集合而言   相当于该内聚力单元的属性
};

// 标准化边键：一条边只有两个节点，按从小到大排序即可唯一标识
struct NormalizedEdgeKey {
    int node_small;  // 较小的节点ID
    int node_large;  // 较大的节点ID

    NormalizedEdgeKey(int a, int b) {
        if (a < b) {
            node_small = a;
            node_large = b;
        }
        else {
            node_small = b;
            node_large = a;
        }
    }

    bool operator==(const NormalizedEdgeKey& other) const {
        return node_small == other.node_small && node_large == other.node_large;
    }
};

// 为 std::unordered_map / unordered_set 提供 hash
namespace std {
    template <>
    struct hash<NormalizedEdgeKey> {
        std::size_t operator()(const NormalizedEdgeKey& key) const noexcept {
            std::size_t h1 = std::hash<int>{}(key.node_small);
            std::size_t h2 = std::hash<int>{}(key.node_large);
            return h1 ^ (h2 << 1);  // 经典组合方式，足够好
        }
    };
}

// 2D 三角形单元的三个边的连接类型（对应 Abaqus CPS3/CPE3 的局部边编号）
enum class EdgeConnectivityType {
    Edge12,  // 节点1-2
    Edge23,  // 节点2-3
    Edge31   // 节点3-1
};

struct EdgeInfo {
    int element_id;                    // 该边所属的三角形单元ID
    int node1, node2;                  // 该边的两个节点（保持原始顺序，用于生成COH2D4时确定方向）
    EdgeConnectivityType edgeType;     // 记录是单元的哪条边
    COH2D4Type type;                // Boundary / Inner / Outside
};

using EdgeInfoList = std::vector<EdgeInfo>;

// 共享边组：所有共享同一条边的三角形单元集合（理论上最多两个）
struct SharedEdgeGroup {
    NormalizedEdgeKey key;   // 标准化边键
    EdgeInfoList infos;      // 通常有 1 或 2 个 EdgeInfo


};




// 辅助函数：比较两节点坐标是否相同（考虑浮点误差）
bool are2DNodesEqual(const Node2D& n1, const Node2D& n2, double tol = 0.001);

void write2DInp(const std::string& filename,
    const std::vector<Node2D>& NewNodes,
    const std::vector<Element2D>& NewElements,
    const std::vector<COH2D4>& cohesive_elements,
    const OutputSet& outputSet, const std::string& InsertCohesiveSetName,
    const bool setByOrder = true, const bool outputNodeSet=false);

bool read2DInp(const std::string& filename,
    std::vector<Node2D>& nodes,
    std::vector<Element2D>& elements,
    OutputSet& outputSet);

std::string Insert2D(const std::string& INPName, const std::string& InsertCohesiveSetName);
bool is2DCohesiveSet(const ElementSetLabel& mySet);
int getMax2DNodeId(const std::vector<Node2D>& nodes);
int getMax2DElementId(const std::vector<Element2D>& elements);

std::unordered_map<int, Element2D*> creat2DElementMap(std::vector<Element2D>& elements);
std::unordered_map<int, Node2D*> creat2DNodeMap(std::vector<Node2D>& nodes);
void Get2DElementsOutsideTargetSet(const std::unordered_map<int, Element2D*>& Elem_map,
    const std::unordered_set<int>& TargetElementLabelSet);

std::unordered_set<int> GetMatching2DElements(const std::unordered_map<int, Element2D*>& Elem_map,
    const std::unordered_set<int>& TargetElementLabelSet);

void Merge2DElementLabelSets(std::unordered_set<int>& TargetElementLabelSet,
    const std::unordered_set<int>& MatchingElementLabels);

std::vector<Element2D*> getSet2DElements(const std::unordered_map<int, Element2D*>& Elem_map,
    const std::unordered_set<int>& TargetElementLabelSet);

NormalizedEdgeKey normalizeEdge(int a, int b);

std::vector<EdgeInfo> GetEdges(const Element2D& elem);


EdgeInfo GetEdge(const Element2D* elem, EdgeConnectivityType type);
COH2D4Type Get2DCohesiveType(const int id1, const int id2, const std::unordered_set<int>& MatchingElementLabels);
std::vector<EdgeInfoList> GetSharedEdges(const std::vector<Element2D*>& original_elements, const std::unordered_set<int>& MatchingElementLabels);

std::unordered_map<int, std::vector<EN_ID>> count2DNodeUsageInSet(const std::unordered_map<int, Element2D*>& elem_map,
    const std::unordered_set<int>& TargetElementLabelSet,
    int& maxNodeID, const std::unordered_set<int>& MatchingElementLabels);


void add2DNodes(std::vector<Node2D>& OriginNodes, std::unordered_map<int, Node2D*>& node_map, const std::unordered_map<int, std::vector<EN_ID>>& nodeID_EN_IDs);


void Modify2DElements(const std::unordered_map<int, std::vector<EN_ID>>& nodeID_EN_IDs, std::unordered_map<int, Element2D*>& elem_map, const std::unordered_set<int>& MatchingElementLabels);


void alignEdgeNodes(EdgeInfo& Newe2, const EdgeInfo& Newe1, const std::unordered_map<int, Node2D*>& node_map);


bool insert2DCohesiveElementsToSet(std::vector<Node2D>& Nodes, std::vector<Element2D>& Elements,
    std::unordered_set<int> TargetElementLabelSet, std::vector<COH2D4>& cohesive_elements, CohesiveIdentifyInfo& cohesiveIdentifyInfo);

std::vector<std::pair<std::string, std::string>> Find2DInset(const OutputSet& outputSet, const std::string& TargetSetName);

std::vector<std::string> find2DIntersectingSetKeys(
    const OutputSet& outputSet,
    const std::string& TargetSetName);

void populate2DCohesiveElementSets(OutputSet& outputSet, const std::vector<COH2D4>& cohesive_elements, const std::string& InsertCohesiveSetName);













































































#endif // TOOLS2_H