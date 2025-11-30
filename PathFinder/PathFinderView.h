#pragma once
#include <vector>
#include <atlimage.h> 

using namespace std;

// 노드(지점) 구조체
struct Node {
    int id;
    CPoint pt; // 좌표 (x, y)
};

// 엣지(선) 구조체
struct Edge {
    int from, to;
    double weight; // 거리
};

class CPathFinderView : public CView
{
protected: 
    CPathFinderView() noexcept;
    DECLARE_DYNCREATE(CPathFinderView)

    // 특성입니다.
public:
    CPathFinderDoc* GetDocument() const;

    CImage m_image;             // 지도 이미지
    vector<Node> m_nodes;       // 노드 저장
    vector<Edge> m_edges;       // 엣지 저장

    int m_lastNodeID;           // 마지막으로 찍은 노드 (선 연결용)

    int m_startNodeID;          // 다익스트라 시작점 (Alt+클릭)
    int m_endNodeID;            // 다익스트라 도착점 (Alt+클릭)
    vector<int> m_shortestPath; // 최단 경로 결과 (노드 인덱스들)

    // 함수들
    double GetDistance(CPoint p1, CPoint p2); // 거리 계산
    void RunDijkstra(); // 다익스트라 알고리즘 실행
    // -----------------------------

// 재정의입니다.
public:
    virtual void OnDraw(CDC* pDC);  // 이 뷰를 그리기 위해 재정의
    virtual BOOL PreCreateWindow(CREATESTRUCT& cs);

protected:

    // 구현입니다.
public:
    virtual ~CPathFinderView();
#ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
#endif

protected:

    // 생성된 메시지 맵 함수
protected:
    DECLARE_MESSAGE_MAP()
public:
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
};

#ifndef _DEBUG  // PathFinderView.cpp의 디버그 버전
inline CPathFinderDoc* CPathFinderView::GetDocument() const
{
    return reinterpret_cast<CPathFinderDoc*>(m_pDocument);
}
#endif