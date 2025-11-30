// PathFinderView.cpp : CPathFinderView 클래스의 구현
//

#include "pch.h"
#include "framework.h"
// SHARED_HANDLERS는 미리 보기, 썸네일 및 검색 필터 처리기 구현을 설명하는 ATL 프로젝트에서 정의할 수 있습니다.
#ifndef SHARED_HANDLERS
#include "PathFinder.h"
#endif

#include "PathFinderDoc.h"
#include "PathFinderView.h"
#include <cmath>
#include <queue>
#include <limits>

// std::min, std::max와 Windows 매크로 충돌 방지
#undef max
#undef min

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CPathFinderView

IMPLEMENT_DYNCREATE(CPathFinderView, CView)

BEGIN_MESSAGE_MAP(CPathFinderView, CView)
    ON_WM_LBUTTONDOWN()
    ON_WM_RBUTTONDOWN()
END_MESSAGE_MAP()

// CPathFinderView 생성/소멸

CPathFinderView::CPathFinderView() noexcept
{
    // 멤버 변수 초기화
    m_lastNodeID = -1;
    m_startNodeID = -1;
    m_endNodeID = -1;

    // 지도 이미지 리소스 로드
    HRESULT hr = m_image.Load(_T("map.png"));
    if (FAILED(hr)) {
        MessageBox(_T("지도 이미지(map.png)를 로드할 수 없습니다."), _T("오류"), MB_ICONERROR);
    }
}

CPathFinderView::~CPathFinderView()
{
}

BOOL CPathFinderView::PreCreateWindow(CREATESTRUCT& cs)
{
    return CView::PreCreateWindow(cs);
}

// 두 점 사이의 유클리드 거리 계산
double CPathFinderView::GetDistance(CPoint p1, CPoint p2)
{
    double dx = p1.x - p2.x;
    double dy = p1.y - p2.y;
    return sqrt(dx * dx + dy * dy);
}

// =======================================================
// 화면 출력 (OnDraw)
// =======================================================
void CPathFinderView::OnDraw(CDC* pDC)
{
    CPathFinderDoc* pDoc = GetDocument();
    ASSERT_VALID(pDoc);
    if (!pDoc) return;

    // 1. 지도 이미지 출력
    if (!m_image.IsNull()) {
        m_image.Draw(pDC->GetSafeHdc(), 0, 0);
    }

    // 2. 엣지(Edge) 그리기 - 파란색 실선
    CPen bluePen(PS_SOLID, 2, RGB(0, 0, 255));
    CPen* pOldPen = pDC->SelectObject(&bluePen);

    for (const auto& edge : m_edges) {
        CPoint p1 = m_nodes[edge.from].pt;
        CPoint p2 = m_nodes[edge.to].pt;
        pDC->MoveTo(p1);
        pDC->LineTo(p2);

        // 엣지 가중치(거리) 텍스트 표시
        CString strDist;
        strDist.Format(_T("%.1f"), edge.weight);
        pDC->TextOutW((p1.x + p2.x) / 2, (p1.y + p2.y) / 2, strDist);
    }

    // 3. 최단 경로 표시 - 빨간색 굵은 실선
    if (!m_shortestPath.empty()) {
        CPen redPen(PS_SOLID, 5, RGB(255, 0, 0));
        pDC->SelectObject(&redPen);

        for (size_t i = 0; i < m_shortestPath.size() - 1; i++) {
            int u = m_shortestPath[i];
            int v = m_shortestPath[i + 1];
            pDC->MoveTo(m_nodes[u].pt);
            pDC->LineTo(m_nodes[v].pt);
        }
    }

    // 4. 노드(Node) 그리기
    pDC->SelectObject(pOldPen); // 펜 원상복구
    CBrush whiteBrush(RGB(255, 255, 255));
    CBrush* pOldBrush = pDC->SelectObject(&whiteBrush);

    for (const auto& node : m_nodes) {
        // 시작점(초록색), 도착점(빨간색), 일반노드(흰색) 구분
        if (node.id == m_startNodeID) {
            CBrush startBrush(RGB(0, 255, 0));
            pDC->SelectObject(&startBrush);
            pDC->Ellipse(node.pt.x - 5, node.pt.y - 5, node.pt.x + 5, node.pt.y + 5);
            pDC->SelectObject(&whiteBrush);
        }
        else if (node.id == m_endNodeID) {
            CBrush endBrush(RGB(255, 0, 0));
            pDC->SelectObject(&endBrush);
            pDC->Ellipse(node.pt.x - 5, node.pt.y - 5, node.pt.x + 5, node.pt.y + 5);
            pDC->SelectObject(&whiteBrush);
        }
        else {
            pDC->Ellipse(node.pt.x - 5, node.pt.y - 5, node.pt.x + 5, node.pt.y + 5);
        }
    }
    pDC->SelectObject(pOldBrush);
}

// =======================================================
// 마우스 왼쪽 버튼 클릭 이벤트 처리
// =======================================================
void CPathFinderView::OnLButtonDown(UINT nFlags, CPoint point)
{
    // 1. Ctrl + 클릭: 노드 생성 및 엣지 연결
    if (nFlags & MK_CONTROL)
    {
        // 클릭한 위치가 기존 노드 근처인지 확인 (기존 노드 재사용)
        int clickedExistingID = -1;
        double detectionRange = 50.0; // 클릭 인식 범위

        for (const auto& node : m_nodes) {
            if (GetDistance(node.pt, point) < detectionRange) {
                clickedExistingID = node.id;
                break;
            }
        }

        // A. 기존 노드를 클릭한 경우 -> 해당 노드와 연결
        if (clickedExistingID != -1) {
            if (m_lastNodeID != -1 && m_lastNodeID != clickedExistingID) {
                // 엣지 추가 (양방향)
                double dist = GetDistance(m_nodes[m_lastNodeID].pt, m_nodes[clickedExistingID].pt);

                Edge newEdge = { m_lastNodeID, clickedExistingID, dist };
                m_edges.push_back(newEdge);

                Edge reverseEdge = { clickedExistingID, m_lastNodeID, dist };
                m_edges.push_back(reverseEdge);
            }
            m_lastNodeID = clickedExistingID; // 마지막 선택 노드 갱신
        }
        // B. 빈 공간을 클릭한 경우 -> 새 노드 생성 후 연결
        else {
            Node newNode;
            newNode.id = (int)m_nodes.size();
            newNode.pt = point;
            m_nodes.push_back(newNode);

            if (m_lastNodeID != -1) {
                double dist = GetDistance(m_nodes[m_lastNodeID].pt, point);

                Edge newEdge = { m_lastNodeID, newNode.id, dist };
                m_edges.push_back(newEdge);

                Edge reverseEdge = { newNode.id, m_lastNodeID, dist };
                m_edges.push_back(reverseEdge);
            }
            m_lastNodeID = newNode.id;
        }
        Invalidate(); // 화면 갱신 요청
    }

    // 2. Alt + 클릭: 시작점/도착점 설정 및 최단 경로 탐색
    else if (GetAsyncKeyState(VK_MENU) & 0x8000)
    {
        int clickedID = -1;
        double minDist = 20.0;

        // 클릭한 위치에 해당하는 노드 검색
        for (const auto& node : m_nodes) {
            double d = GetDistance(node.pt, point);
            if (d < minDist) {
                minDist = d;
                clickedID = node.id;
            }
        }

        if (clickedID != -1) {
            if (m_startNodeID == -1) {
                m_startNodeID = clickedID;
                MessageBox(_T("시작점이 설정되었습니다."), _T("알림"));
            }
            else {
                m_endNodeID = clickedID;
                RunDijkstra(); // 경로 탐색 실행

                // 탐색 후 초기화
                m_startNodeID = -1;
                m_endNodeID = -1;
            }
            Invalidate();
        }
    }

    CView::OnLButtonDown(nFlags, point);
}

// =======================================================
// 마우스 오른쪽 버튼 클릭 이벤트 처리 (경로 끊기)
// =======================================================
void CPathFinderView::OnRButtonDown(UINT nFlags, CPoint point)
{
    m_lastNodeID = -1; // 현재 연결 상태 초기화

    MessageBox(_T("선 연결을 끊었습니다! 다음 점은 새로 시작합니다."), _T("확인"));

    CView::OnRButtonDown(nFlags, point);
}

// =======================================================
// 다익스트라 알고리즘을 이용한 최단 경로 탐색
// =======================================================
void CPathFinderView::RunDijkstra()
{
    if (m_startNodeID == -1 || m_endNodeID == -1) return;

    int n = (int)m_nodes.size();
    vector<double> dist(n, numeric_limits<double>::max()); // 거리 배열 초기화
    vector<int> parent(n, -1); // 경로 역추적용 부모 노드 배열

    // 우선순위 큐 (Min-Heap)
    priority_queue<pair<double, int>, vector<pair<double, int>>, greater<pair<double, int>>> pq;

    dist[m_startNodeID] = 0;
    pq.push({ 0, m_startNodeID });

    while (!pq.empty()) {
        double d = pq.top().first;
        int u = pq.top().second;
        pq.pop();

        if (d > dist[u]) continue;
        if (u == m_endNodeID) break; // 도착점 도달

        // 인접 노드 탐색
        for (const auto& edge : m_edges) {
            if (edge.from == u) {
                int v = edge.to;
                double weight = edge.weight;

                // 더 짧은 경로 발견 시 업데이트
                if (dist[u] + weight < dist[v]) {
                    dist[v] = dist[u] + weight;
                    parent[v] = u;
                    pq.push({ dist[v], v });
                }
            }
        }
    }

    // 경로 역추적 (Backtracking)
    m_shortestPath.clear();
    if (dist[m_endNodeID] != numeric_limits<double>::max()) {
        int curr = m_endNodeID;
        while (curr != -1) {
            m_shortestPath.push_back(curr);
            curr = parent[curr];
        }
        MessageBox(_T("최단 경로 탐색 완료"), _T("알림"));
    }
    else {
        MessageBox(_T("경로가 존재하지 않습니다."), _T("알림"));
    }
}

// CPathFinderView 진단

#ifdef _DEBUG
void CPathFinderView::AssertValid() const
{
    CView::AssertValid();
}

void CPathFinderView::Dump(CDumpContext& dc) const
{
    CView::Dump(dc);
}

CPathFinderDoc* CPathFinderView::GetDocument() const
{
    ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CPathFinderDoc)));
    return (CPathFinderDoc*)m_pDocument;
}
#endif //_DEBUG