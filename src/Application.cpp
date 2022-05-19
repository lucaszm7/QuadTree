// LGE library
#include "Application.h"

// Standart Stuff
#include <iostream>
#include <memory>
#include <vector>
#include <array>
#include <list>

struct rect
{
    glm::vec2 pos;
    glm::vec2 size;

    rect(const glm::vec2& p = {0.0f, 0.0f}, const glm::vec2& s = {1.0f, 1.0f})
        : pos(p), size(s) {}

    constexpr bool contains(const glm::vec2& p) const
    {
        return !(p.x < pos.x || p.y < pos.y || p.x >= (pos.x + size.x) || p.y >= (pos.y + size.y));
    }

    // Fully contains another rect
    constexpr bool contains(const rect& r) const
    {
        return (r.pos.x >= pos.x) && ((r.pos.x + r.size.x) < (pos.x + size.x)) &&
            (r.pos.y >= pos.y) && ((r.pos.y + r.size.y) < (pos.y + size.y));
    }

    constexpr bool overlaps(const rect& r) const
    {
        return (pos.x < r.pos.x + r.size.x) && (pos.x + size.x >= r.pos.x) &&
            (pos.y < r.pos.y + r.size.y) && (pos.y + size.y >= r.pos.y);
    }

};


constexpr size_t MAX_DEPTH = 8;

template <typename OBJECT_TYPE>
class StaticQuadTree
{

protected:

    size_t m_Depth = 0;

    // This QuadTree
    rect m_Rect;

    // 4 Child Areas
    std::array<rect, 4> m_rChild{};

    // 4 Potential Children
    std::array<std::shared_ptr<StaticQuadTree<OBJECT_TYPE>>, 4> m_pChild{};

    // Item Storage
    std::vector<std::pair<rect, OBJECT_TYPE>> m_pItems;


public:
    StaticQuadTree(const rect& size = { {0.0f, 0.0f},{100.0f, 100.0f} }, const size_t depth = 0)
    {
        m_Depth = depth;
        resize(size);
    }

    void resize(const rect& r)
    {
        clear();
        m_Rect = r;
        glm::vec2 vChildSize = (m_Rect.size / 2.0f);

        m_rChild =
        {
            rect(m_Rect.pos, vChildSize),
            rect(glm::vec2( m_Rect.pos.x + vChildSize.x, m_Rect.pos.y ), vChildSize),
            rect(glm::vec2( m_Rect.pos.x, m_Rect.pos.y + vChildSize.y ), vChildSize),
            rect(m_Rect.pos + vChildSize, vChildSize)
        };
    }

    void clear()
    {
        m_pItems.clear();
        for (int i = 0; i < 4; ++i)
        {
            if (m_pChild[i])
                m_pChild[i]->clear();
        }
    }

    size_t size()
    {
        size_t nCount = m_pItems.size();
        for (int i = 0; i < 4; ++i)
        {
            if (m_pChild[i])
                nCount += m_pChild->size();
        }
        return nCount;
    }

    void insert(const OBJECT_TYPE& item, const rect& itemSize) // AABB
    {
        for (int i = 0; i < 4; ++i)
        {
            if (m_rChild[i].contains(itemSize))
            {
                if (m_Depth + 1 < MAX_DEPTH)
                {
                    if (!m_pChild[i])
                    {
                        m_pChild[i] = std::make_shared<StaticQuadTree<OBJECT_TYPE>>(m_rChild[i], m_Depth + 1);
                    }
                    m_pChild[i]->insert(item, itemSize);
                    return;
                }
            }
        }
        m_pItems.push_back({ itemSize, item });
    }


    std::list<OBJECT_TYPE> search(rect& rArea) const
    {
        std::list<OBJECT_TYPE> listItems;
        listItems.resize(10);
        search(rArea, listItems);
        return listItems;
    }

    void search(const rect& rArea, std::list<OBJECT_TYPE>& listItems) const
    {
        // First gonna check if the area belongs to this area
        for (const auto& p : m_pItems)
        {
            if (rArea.overlaps(p.first))
                listItems.push_back(p.second);
        }

        // Second we recursivily iterate for the childrens
        #pragma unroll
        for (int i = 0; i < 4; ++i)
        {
            if (m_pChild[i])
            {
                // if the children area enteryly belongs to search area, add whole children
                if (rArea.contains(m_rChild[i]))
                    m_pChild[i]->items(listItems);

                // Now if its not fully contained, its need to be searched
                else if (m_rChild[i].overlaps(rArea))
                    m_pChild[i]->search(rArea, listItems);
            }
        }
    }

    void items(std::list<OBJECT_TYPE>& listItems) const
    {
        for (const auto& p : m_pItems)
        {
            listItems.push_back(p.second);
        }

        for (int i = 0; i < 4; ++i)
        {
            if (m_pChild[i])
                m_pChild[i]->items(listItems);
        }

    }

    const rect& area()
    {
        return m_Rect;
    }
};



class SceneStaticQuadTree : public LGE::Scene_t
{

protected:
    struct SomeObjectWithArea
    {
        glm::vec2 vPos;
        glm::vec2 vVel;
        glm::vec2 vSize;
        Color col;
    };

    std::vector<SomeObjectWithArea> vecObjects;
    StaticQuadTree<SomeObjectWithArea> treeObjects;

    float fArea = 100000.0f;

    double msDrawingTime;
    unsigned int drawCalls;

    bool bUseQuadTree;
    bool bHeld = false;

public:
    SceneStaticQuadTree()
    {

        treeObjects.resize({ { 0.0f, 0.0f }, { fArea, fArea } });

        for (int i = 0; i < 100000; ++i)
        {
            SomeObjectWithArea obj;
            obj.vPos = { LGE::rand(0.0f, fArea), LGE::rand(0.0f, fArea) };
            obj.vSize = { LGE::rand(0.1f, 100.0f), LGE::rand(0.1f, 100.0f) };
            obj.col = { LGE::rand(0.0f, 1.0f), LGE::rand(0.0f, 1.0f), LGE::rand(0.0f, 1.0f), 1.0f };
            vecObjects.push_back(obj);
            treeObjects.insert(obj, rect(obj.vPos, obj.vSize));
        }
        SomeObjectWithArea obj;
        obj.vPos = { 0.0f, 0.0f };
        obj.vSize = { 100.0f, 100.0f };
        obj.col = { 1.0f, 0.0f, 0.0f, 1.0f };
        vecObjects.push_back(obj);
        treeObjects.insert(obj, rect(obj.vPos, obj.vSize));

    }

    void OnUpdate(float fElapsedTime) override
    {

        if (LGE::GetKey(GLFW_KEY_TAB) == GLFW_PRESS && !bHeld)
            bHeld = true;

        if (LGE::GetKey(GLFW_KEY_TAB) == GLFW_RELEASE && bHeld)
        {
            bHeld = false;
            bUseQuadTree = !bUseQuadTree;
        }

        float fWorldTLX, fWorldTLY;
        float fWorldBRX, fWorldBRY;

        tv.ScreenToWorld(0, 0, fWorldTLX, fWorldTLY);
        tv.ScreenToWorld(SCREEN_WIDTH, SCREEN_HEIGHT, fWorldBRX, fWorldBRY);

        glm::vec2 vWorldTL = { fWorldTLX, fWorldTLY };
        glm::vec2 vWorldBR = { fWorldBRX, fWorldBRY };

        rect rScreen = { vWorldTL, vWorldBR - vWorldTL };

        if(bUseQuadTree)
        {
            LGE::Timer time;
            unsigned int calls = 0;
            for (const auto& obj : treeObjects.search(rScreen))
            {
                calls++;
                DrawRect(obj.vPos, obj.vSize, obj.col);
            }
            drawCalls = calls;
            msDrawingTime = time.now();
        }

        // Linear
        else
        {
            LGE::Timer time;
            unsigned int calls = 0;
            for (const auto& obj : vecObjects)
            {
                if (rScreen.overlaps({ obj.vPos, obj.vSize }))
                {
                    calls++;
                    DrawRect(obj.vPos, obj.vSize, obj.col);
                }
            }
            drawCalls = calls;
            msDrawingTime = time.now();
        }

        

    }

    void OnRender() override
    {

    }

    void OnImGuiRender() override
    {
        ImGui::Text("Took %.3f ms", msDrawingTime);
        ImGui::Text("Draw Calls: %.3u", drawCalls);

        if (bUseQuadTree)
            ImGui::Text("USING QUAD TREES :) !!!");
        if(!bUseQuadTree)
            ImGui::Text("NOT USING QUAD TREES ;( ...");

    }

    ~SceneStaticQuadTree()
    {

    }
};

int main(int argc, char** argv)
{
    LGE::Application Demo;
    Demo.RegisterScene<SceneStaticQuadTree>("Static Quad Tree");
    Demo.Run ();

    /*Demo.RegisterScene<LGE::TestClearColor>("Clear Color Test");
    Demo.RegisterScene<LGE::TestDemo>("Texture Test");
    Demo.RegisterScene<PolygonTest>("Polygon Test");
    Demo.RegisterScene<ConvexHull>("ConvexHull");
    Demo.RegisterScene<QuadTree_Scene>("QuadTree");*/


    return 0;
}