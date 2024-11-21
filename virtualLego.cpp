////////////////////////////////////////////////////////////////////////////////
//
// File: virtualLego.cpp
//
// Original Author: 박창현 Chang-hyeon Park, 
// Modified by Bong-Soo Sohn and Dong-Jun Kim
// 
// Originally programmed for Virtual LEGO. 
// Modified later to program for Virtual Billiard.
//        
////////////////////////////////////////////////////////////////////////////////

#include "d3dUtility.h"
#include <vector>
#include <ctime>
#include <cstdlib>
#include <cstdio>
#include <cassert>

IDirect3DDevice9* Device = NULL;

// window size
const int Width = 1024;
const int Height = 768;

// -----------------------------------------------------------------------------
// Transform matrices
// -----------------------------------------------------------------------------
D3DXMATRIX g_mWorld;
D3DXMATRIX g_mView;
D3DXMATRIX g_mProj;

#define BALLNUM 20  // 노란공 개수
#define LIFENUM 2  // 생명 개수
#define M_RADIUS 0.15   // sphere 반지름
#define PI 3.14159265
#define M_HEIGHT 0.01
#define DECREASE_RATE 0.9982

// -----------------------------------------------------------------------------
// CSphere class definition
// -----------------------------------------------------------------------------

class CSphere {
private:
    float               center_x, center_y, center_z;
    float                   m_radius;
    float               m_velocity_x;
    float               m_velocity_z;

public:
    CSphere(void)
    {
        D3DXMatrixIdentity(&m_mLocal);
        ZeroMemory(&m_mtrl, sizeof(m_mtrl));
        m_radius = 0;
        m_velocity_x = 0;
        m_velocity_z = 0;
        m_pSphereMesh = NULL;
    }
    ~CSphere(void) {}

public:
    bool create(IDirect3DDevice9* pDevice, D3DXCOLOR color = d3d::WHITE)
    {
        if (NULL == pDevice)
            return false;

        m_mtrl.Ambient = color;
        m_mtrl.Diffuse = color;
        m_mtrl.Specular = color;
        m_mtrl.Emissive = d3d::BLACK;
        m_mtrl.Power = 5.0f;

        if (FAILED(D3DXCreateSphere(pDevice, getRadius(), 50, 50, &m_pSphereMesh, NULL)))
            return false;
        return true;
    }

    void destroy(void)
    {
        if (m_pSphereMesh != NULL) {
            m_pSphereMesh->Release();
            m_pSphereMesh = NULL;
        }
    }

    void draw(IDirect3DDevice9* pDevice, const D3DXMATRIX& mWorld)
    {
        if (NULL == pDevice)
            return;
        pDevice->SetTransform(D3DTS_WORLD, &mWorld);
        pDevice->MultiplyTransform(D3DTS_WORLD, &m_mLocal);
        pDevice->SetMaterial(&m_mtrl);
        m_pSphereMesh->DrawSubset(0);
    }

    bool hasIntersected(CSphere& ball)
    {
        D3DXVECTOR3 center1 = this->getCenter();
        D3DXVECTOR3 center2 = ball.getCenter();
        float distance = D3DXVec3Length(&(center1 - center2)); // 두 공의 중심 사이 거리

        if (distance < (this->getRadius() + ball.getRadius())) {
            // 공이 겹친 경우 위치 보정
            D3DXVECTOR3 collisionNormal = center2 - center1;
            D3DXVec3Normalize(&collisionNormal, &collisionNormal);

            float overlap = (this->getRadius() + ball.getRadius()) - distance;

            // 각 공을 반대 방향으로 이동
            this->setCenter(center1.x - collisionNormal.x * (overlap / 2),
                center1.y,
                center1.z - collisionNormal.z * (overlap / 2));

            return true;
        }
        return false;
    }

    bool hitBy(CSphere& ball)
    {
        if (hasIntersected(ball)) {
            // 두 공의 중심 좌표
            D3DXVECTOR3 center1 = this->getCenter();
            D3DXVECTOR3 center2 = ball.getCenter();

            // 충돌 방향 벡터 계산 (공1 -> 공2)
            D3DXVECTOR3 collisionNormal = center2 - center1;
            D3DXVec3Normalize(&collisionNormal, &collisionNormal); // 정규화

            // 기존 속도 벡터
            D3DXVECTOR3 velocity1(this->getVelocity_X(), 0, this->getVelocity_Z());
            D3DXVECTOR3 velocity2(ball.getVelocity_X(), 0, ball.getVelocity_Z());

            // 충돌 방향 벡터와 기존 속도 벡터의 내적 계산
            float v1_dot_n = D3DXVec3Dot(&velocity1, &collisionNormal);
            float v2_dot_n = D3DXVec3Dot(&velocity2, &collisionNormal);

            // 새로운 속도 계산 (반사 벡터)
            D3DXVECTOR3 newVelocity1 = velocity1 - 2 * v1_dot_n * collisionNormal;
            D3DXVECTOR3 newVelocity2 = velocity2 - 2 * v2_dot_n * collisionNormal;

            // 새로운 속도를 공에 설정
            this->setPower(newVelocity1.x, newVelocity1.z);
            //ball.setPower(newVelocity2.x, newVelocity2.z);    -   정지해 있기에 필요X
            return true;
        }
        return false;
    }

    void ballUpdate(float timeDiff)
    {
        const float TIME_SCALE = 3.3;
        D3DXVECTOR3 cord = this->getCenter();
        double vx = abs(this->getVelocity_X());
        double vz = abs(this->getVelocity_Z());


        if (vx > 0.01 || vz > 0.01)
        {
            float tX = cord.x + TIME_SCALE * timeDiff * m_velocity_x;
            float tZ = cord.z + TIME_SCALE * timeDiff * m_velocity_z;

            //correction of position of ball
            // Please uncomment this part because this correction of ball position is necessary when a ball collides with a wall
            if (tX >= (3 - M_RADIUS))
                tX = 3 - M_RADIUS;
            else if (tX <= (-3 + M_RADIUS))
                tX = -3 + M_RADIUS;
            //else if(tZ <= (-3.5 + M_RADIUS))
            //   tZ = -3.5 + M_RADIUS;
            else if (tZ >= (3.5 - M_RADIUS))
                tZ = 3.5 - M_RADIUS;

            this->setCenter(tX, cord.y, tZ);
        }



        else { this->setPower(0, 0); }
        //this->setPower(this->getVelocity_X() * DECREASE_RATE, this->getVelocity_Z() * DECREASE_RATE);
        double rate = 1 - (1 - DECREASE_RATE) * timeDiff * 400;
        if (rate < 0)
            rate = 0;
        //this->setPower(getVelocity_X() * rate, getVelocity_Z() * rate);
    }

    double getVelocity_X() { return this->m_velocity_x; }
    double getVelocity_Z() { return this->m_velocity_z; }

    void setPower(double vx, double vz)
    {
        this->m_velocity_x = vx;
        this->m_velocity_z = vz;
    }

    void setCenter(float x, float y, float z)
    {
        D3DXMATRIX m;
        center_x = x;   center_y = y;   center_z = z;
        D3DXMatrixTranslation(&m, x, y, z);
        setLocalTransform(m);
    }

    float getRadius(void)  const { return (float)(M_RADIUS); }
    const D3DXMATRIX& getLocalTransform(void) const { return m_mLocal; }
    void setLocalTransform(const D3DXMATRIX& mLocal) { m_mLocal = mLocal; }
    D3DXVECTOR3 getCenter(void) const
    {
        D3DXVECTOR3 org(center_x, center_y, center_z);
        return org;
    }

    bool isNull() const {
        if (this->m_pSphereMesh == NULL) {
            return true;
        }
        return false;
    }

private:
    D3DXMATRIX              m_mLocal;
    D3DMATERIAL9            m_mtrl;
    ID3DXMesh* m_pSphereMesh;

};

// -----------------------------------------------------------------------------
// CWall class definition
// -----------------------------------------------------------------------------

class CWall {

private:

    float               m_x;
    float               m_z;
    float                   m_width;
    float                   m_depth;
    float               m_height;

public:
    CWall(void)
    {
        D3DXMatrixIdentity(&m_mLocal);
        ZeroMemory(&m_mtrl, sizeof(m_mtrl));
        m_width = 0;
        m_depth = 0;
        m_pBoundMesh = NULL;
    }
    ~CWall(void) {}
public:
    bool create(IDirect3DDevice9* pDevice, float ix, float iz, float iwidth, float iheight, float idepth, D3DXCOLOR color = d3d::WHITE)
    {
        if (NULL == pDevice)
            return false;

        m_mtrl.Ambient = color;
        m_mtrl.Diffuse = color;
        m_mtrl.Specular = color;
        m_mtrl.Emissive = d3d::BLACK;
        m_mtrl.Power = 5.0f;

        m_width = iwidth;
        m_depth = idepth;

        if (FAILED(D3DXCreateBox(pDevice, iwidth, iheight, idepth, &m_pBoundMesh, NULL)))
            return false;
        return true;
    }
    void destroy(void)
    {
        if (m_pBoundMesh != NULL) {
            m_pBoundMesh->Release();
            m_pBoundMesh = NULL;
        }
    }
    void draw(IDirect3DDevice9* pDevice, const D3DXMATRIX& mWorld)
    {
        if (NULL == pDevice)
            return;
        pDevice->SetTransform(D3DTS_WORLD, &mWorld);
        pDevice->MultiplyTransform(D3DTS_WORLD, &m_mLocal);
        pDevice->SetMaterial(&m_mtrl);
        m_pBoundMesh->DrawSubset(0);
    }

    bool hasIntersected(CSphere& ball) {
        float ballX = ball.getCenter().x;
        float ballZ = ball.getCenter().z;
        float ballR = ball.getRadius();

        float wallXmin = this->m_x - (this->m_width / 2 + ballR);
        float wallXmax = this->m_x + (this->m_width / 2 + ballR);
        float wallZmin = this->m_z - (this->m_depth / 2 + ballR);
        float wallZmax = this->m_z + (this->m_depth / 2 + ballR);

        if ((wallXmin <= ballX && ballX <= wallXmax) && (wallZmin <= ballZ && ballZ <= wallZmax)) {
            return true;
        }

        return false;
    }


    void hitBy(CSphere& ball) {
        if (hasIntersected(ball)) {
            float ballX = ball.getCenter().x;
            float ballY = ball.getCenter().y;
            float ballZ = ball.getCenter().z;
            float ballR = ball.getRadius();

            float wallXmin = this->m_x - (this->m_width / 2);
            float wallXmax = this->m_x + (this->m_width / 2);
            float wallZmin = this->m_z - (this->m_depth / 2);
            float wallZmax = this->m_z + (this->m_depth / 2);

            if ((wallXmin <= ballX && ballX <= wallXmax) && !(wallZmin <= ballZ && ballZ <= wallZmax)) {
                if (wallZmin - ballR <= ballZ && ballZ <= this->m_z) {
                    ballZ = wallZmin - ballR - 0.01f;
                    ball.setCenter(ballX, ballY, ballZ);
                }
                else {
                    ball.setCenter((wallXmax - wallXmin) / 2 + wallXmin, 0, wallZmin + 0.1);
                    ball.setPower(0, 0);
                }

                ball.setPower(ball.getVelocity_X(), -ball.getVelocity_Z());
            }

            if (!(wallXmin <= ballX && ballX <= wallXmax) && (wallZmin <= ballZ && ballZ <= wallZmax)) {
                if (wallXmin - ballR <= ballX && ballX <= this->m_x) {
                    ballX = wallXmin - ballR - 0.01f;
                    ball.setCenter(ballX, ballY, ballZ);
                }
                else {
                    ballX = wallXmax + ballR + 0.01f;
                    ball.setCenter(ballX, ballY, ballZ);
                }

                ball.setPower(-ball.getVelocity_X(), ball.getVelocity_Z());
            }

        }

    }

    //bool hasIntersected(CSphere& ball) {
    //    D3DXVECTOR3 center = ball.getCenter(); 
    //    float radius = ball.getRadius(); 
    //    // Check if the sphere's center is within the bounds of the wall, considering the sphere's radius 
    //    if ((abs(center.x - m_x) <= radius+m_width/2) && (abs(center.z - m_z) <= radius + m_depth / 2)) {
    //        return true; 
    //    } 
    //    return false; 
    //} 
    //
    //void hitBy(CSphere& ball) {
    //    if (hasIntersected(ball)) {
    //        D3DXVECTOR3 center = ball.getCenter(); float radius = ball.getRadius(); // 벽 표면의 법선 벡터 결정 
    //        D3DXVECTOR3 velocity(ball.getVelocity_X(), 0, ball.getVelocity_Z());

    //        if ((abs(center.z - m_z) <= radius + m_depth / 2) && m_width >= 6) {
    //            if (center.z < m_z) {
    //                //위쪽 벽
    //                ball.setPower(velocity.x, -velocity.z);
    //                return;
    //            }
    //        } if ((abs(center.x - m_x) <= radius + m_width / 2) && m_height >= 7) {
    //            //왼쪽이던 오른쪽이던
    //            ball.setPower(-velocity.x, velocity.z);
    //        }
    //    }
    //}

    void setPosition(float x, float y, float z)
    {
        D3DXMATRIX m;
        this->m_x = x;
        this->m_z = z;

        D3DXMatrixTranslation(&m, x, y, z);
        setLocalTransform(m);
    }


    float getHeight(void) const { return M_HEIGHT; }



private:
    void setLocalTransform(const D3DXMATRIX& mLocal) { m_mLocal = mLocal; }

    D3DXMATRIX              m_mLocal;
    D3DMATERIAL9            m_mtrl;
    ID3DXMesh* m_pBoundMesh;
};

// -----------------------------------------------------------------------------
// CLight class definition
// -----------------------------------------------------------------------------

class CLight {
public:
    CLight(void)
    {
        static DWORD i = 0;
        m_index = i++;
        D3DXMatrixIdentity(&m_mLocal);
        ::ZeroMemory(&m_lit, sizeof(m_lit));
        m_pMesh = NULL;
        m_bound._center = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
        m_bound._radius = 0.0f;
    }
    ~CLight(void) {}
public:
    bool create(IDirect3DDevice9* pDevice, const D3DLIGHT9& lit, float radius = 0.1f)
    {
        if (NULL == pDevice)
            return false;
        if (FAILED(D3DXCreateSphere(pDevice, radius, 10, 10, &m_pMesh, NULL)))
            return false;

        m_bound._center = lit.Position;
        m_bound._radius = radius;

        m_lit.Type = lit.Type;
        m_lit.Diffuse = lit.Diffuse;
        m_lit.Specular = lit.Specular;
        m_lit.Ambient = lit.Ambient;
        m_lit.Position = lit.Position;
        m_lit.Direction = lit.Direction;
        m_lit.Range = lit.Range;
        m_lit.Falloff = lit.Falloff;
        m_lit.Attenuation0 = lit.Attenuation0;
        m_lit.Attenuation1 = lit.Attenuation1;
        m_lit.Attenuation2 = lit.Attenuation2;
        m_lit.Theta = lit.Theta;
        m_lit.Phi = lit.Phi;
        return true;
    }
    void destroy(void)
    {
        if (m_pMesh != NULL) {
            m_pMesh->Release();
            m_pMesh = NULL;
        }
    }
    bool setLight(IDirect3DDevice9* pDevice, const D3DXMATRIX& mWorld)
    {
        if (NULL == pDevice)
            return false;

        D3DXVECTOR3 pos(m_bound._center);
        D3DXVec3TransformCoord(&pos, &pos, &m_mLocal);
        D3DXVec3TransformCoord(&pos, &pos, &mWorld);
        m_lit.Position = pos;

        pDevice->SetLight(m_index, &m_lit);
        pDevice->LightEnable(m_index, TRUE);
        return true;
    }

    void draw(IDirect3DDevice9* pDevice)
    {
        if (NULL == pDevice)
            return;
        D3DXMATRIX m;
        D3DXMatrixTranslation(&m, m_lit.Position.x, m_lit.Position.y, m_lit.Position.z);
        pDevice->SetTransform(D3DTS_WORLD, &m);
        pDevice->SetMaterial(&d3d::WHITE_MTRL);
        m_pMesh->DrawSubset(0);
    }

    D3DXVECTOR3 getPosition(void) const { return D3DXVECTOR3(m_lit.Position); }

private:
    DWORD               m_index;
    D3DXMATRIX          m_mLocal;
    D3DLIGHT9           m_lit;
    ID3DXMesh* m_pMesh;
    d3d::BoundingSphere m_bound;
};


// -----------------------------------------------------------------------------
// Global variables
// -----------------------------------------------------------------------------
CWall   g_legoPlane;
CWall   g_legowall[3];
CSphere   g_sphere[BALLNUM];
CSphere   g_target_whiteball;
CSphere   g_target_redball;
CSphere   g_target_blueball;
CLight   g_light;

ID3DXFont* g_pFont_life = NULL;
ID3DXFont* g_pFont_level = NULL;
ID3DXFont* g_pFont_start = NULL;
ID3DXFont* g_pFont_endMess = NULL;

int spaceActivate = 0;
int life;  // life 의 수
int destroyNum;  // 파괴된 공 수 (노란공 카운트를 위함)
bool win;  // 승리 여부 확인
bool blueActivated = false;  // 파란 공 (생명추가) 활성화 여부
int level = 1;  // 초기 레벨
double speed = 2;  // 초기 빨간 공 속도

double g_camera_pos[3] = { 0.0, 5.0, -8.0 };

const float WALL_X_MIN = -2.8f, WALL_X_MAX = 2.8f;  // WALL 두께를 고려해서 임의로 잡은 너비 최소, 너비 최대값
const float WALL_Z_MIN = -2.6f, WALL_Z_MAX = 3.3f;  // 높이 최소가 다른 이유는 빨간 공 + 흰 공 의 위치 고려
float spherePos[BALLNUM][2];  // 노란 공들의 x, y 좌표

std::vector<D3DXCOLOR> sphereColor(BALLNUM, d3d::YELLOW);  // 노란 공 색 동적할당 (벡터)

// -----------------------------------------------------------------------------
// Functions
// -----------------------------------------------------------------------------


void destroyAllLegoBlock(void)
{
    for (int i = 0; i < BALLNUM; i++) {
        if (!g_sphere->isNull()) {
            g_sphere[i].destroy();
        }
    }
    g_target_redball.destroy();
    g_target_whiteball.destroy();
    g_legoPlane.destroy();
}

void resetGame() {  //생명이 줄어들 때마다 호출되는 함수. 
    g_target_redball.create(Device, d3d::RED);      //새로 공을 만들고
    g_target_redball.setCenter(g_target_whiteball.getCenter().x, g_target_whiteball.getCenter().y, g_target_whiteball.getCenter().z + g_target_whiteball.getRadius() * 2); //기존 흰공의 위에다 생성
    g_target_redball.setPower(0, 0);    //가속도 없음
    spaceActivate = 0;   //발사를 위해 다시 spaceActivate = 0으로
}

// 공 간의 거리 계산 함수
bool isColliding(float x1, float z1, float x2, float z2) {
    float dx = x1 - x2;
    float dz = z1 - z2;
    return (dx * dx + dz * dz) < (4 * M_RADIUS * M_RADIUS);  // 거리 제곱과 두 반지름의 합 제곱 비교로 충돌 여부 판별
}

// 공 위치 생성 함수
void generateRandomPositions(float spherePos[BALLNUM][2]) {
    std::srand(static_cast<unsigned int>(std::time(nullptr)));  // 랜덤 시드 설정

    for (int i = 0; i < BALLNUM; ++i) {
        bool validPosition = false;
        float x, z;

        while (!validPosition) {
            // 랜덤 위치 생성  ( wall의 왼쪽/아랫쪽 값 (즉, 최솟값) + 0.0~1.0 사이의 랜덤 부동소수점 수 * 범위 )
            x = WALL_X_MIN + static_cast<float>(std::rand()) / RAND_MAX * (WALL_X_MAX - WALL_X_MIN);
            z = WALL_Z_MIN + static_cast<float>(std::rand()) / RAND_MAX * (WALL_Z_MAX - WALL_Z_MIN);

            // 기존 공들과의 충돌 여부 확인
            validPosition = true;
            for (int j = 0; j < i; ++j) {
                if (isColliding(x, z, spherePos[j][0], spherePos[j][1])) {
                    validPosition = false;
                    break;
                }
            }
        }

        // 위치 저장
        spherePos[i][0] = x;
        spherePos[i][1] = z;
    }

}


bool SetupBlueBall(CSphere& blueBall, float spherePos[BALLNUM][2], LPDIRECT3DDEVICE9 device) {  // 파란 공 위치 설정
    std::srand(static_cast<unsigned int>(std::time(nullptr)));  // 랜덤 시드 설정
    //blue Activated 여부를 random으로 결정

    blueBall.destroy();

    blueActivated = (std::rand() % 3 == 0);

    if (false == blueActivated || false == blueBall.create(device, d3d::BLUE)) return false;

    bool validPosition = false;
    float blueX, blueZ;

    while (!validPosition) {
        // 랜덤 위치 생성
        blueX = WALL_X_MIN + static_cast<float>(std::rand()) / RAND_MAX * (WALL_X_MAX - WALL_X_MIN);
        blueZ = WALL_Z_MIN + static_cast<float>(std::rand()) / RAND_MAX * (WALL_Z_MAX - WALL_Z_MIN);

        // sphere 배열의 공들과의 충돌 여부 확인
        validPosition = true;
        for (int i = 0; i < BALLNUM; ++i) {
            if (isColliding(blueX, blueZ, spherePos[i][0], spherePos[i][1])) {
                validPosition = false;
                break;
            }
        }
    }

    blueBall.setCenter(blueX, (float)M_RADIUS, blueZ);
    blueBall.setPower(0, 0);
    return true;
}

bool checkLevelUp(int destroyNum) {  // 레벨업 가능 여부 확인  (기준을 BALLNUM 의 절반 이상 제거해야하는걸로 세웠으나 변경 가능)
    const int score_per_level = BALLNUM / 2;

    if (destroyNum >= level * score_per_level) {  // 레벨업 시 점수가 초기화되지 않고 누적되기 때문에 level 을 곱해줌
        return true;
    }
    return false;
}

void levelUp() {  // 레벨업 시 빨간 공 속도는 +2 가 된다.
    life = LIFENUM;
    level++;
    speed += 1.5;

    resetGame();

    generateRandomPositions(spherePos);
    for (int i = 0; i < BALLNUM; ++i) {  // 노란 공 생성
        g_sphere[i].create(Device, sphereColor[i]);
        g_sphere[i].setCenter(spherePos[i][0], (float)M_RADIUS, spherePos[i][1]);
        g_sphere[i].setPower(0, 0);
    }

    SetupBlueBall(g_target_blueball, spherePos, Device);
}

bool SetupFonts(LPDIRECT3DDEVICE9 Device, ID3DXFont*& g_pFont_life, ID3DXFont*& g_pFont_endMess, ID3DXFont*& g_pFont_level, ID3DXFont*& g_pFont_start) {  // 화면에 글자 삽입을 위한 font 객체 생성
    D3DXFONT_DESC fontDesc = {
        24, // Height
        0,  // Width
        0,  // Weight
        0,  // MipLevels
        FALSE, // Italic
        DEFAULT_CHARSET, // CharSet
        OUT_DEFAULT_PRECIS, // OutputPrecision
        CLIP_DEFAULT_PRECIS, // Quality
        DEFAULT_PITCH, // PitchAndFamily
        "Arial" // FaceName
    };

    // Create font for start
    if (FAILED(D3DXCreateFontIndirect(Device, &fontDesc, &g_pFont_start))) {
        ::MessageBox(0, "D3DXCreateFontIndirect() for g_pFont_start - FAILED", 0, 0);
        return false;
    }
    // Create font for level
    if (FAILED(D3DXCreateFontIndirect(Device, &fontDesc, &g_pFont_level))) {
        ::MessageBox(0, "D3DXCreateFontIndirect() for g_pFont_level - FAILED", 0, 0);
        return false;
    }

    // Create font for life & score
    if (FAILED(D3DXCreateFontIndirect(Device, &fontDesc, &g_pFont_life))) {
        ::MessageBox(0, "D3DXCreateFontIndirect() for g_pFont_life - FAILED", 0, 0);
        return false;
    }

    // Create font for end message
    if (FAILED(D3DXCreateFontIndirect(Device, &fontDesc, &g_pFont_endMess))) {
        ::MessageBox(0, "D3DXCreateFontIndirect() for g_pFont_endMess - FAILED", 0, 0);
        return false;
    }

    return true;
}

// 초기화
bool Setup()
{
    spaceActivate = 0;
    destroyNum = 0;
    win = false;
    std::srand(static_cast<unsigned int>(std::time(nullptr)));  // 랜덤 시드 설정
    //blue Activated 여부를 random으로 결정
    blueActivated = (std::rand() % 3 == 0);
    int i;
    life = LIFENUM;

    D3DXMatrixIdentity(&g_mWorld);
    D3DXMatrixIdentity(&g_mView);
    D3DXMatrixIdentity(&g_mProj);

    // 글자 생성
    if (!SetupFonts(Device, g_pFont_life, g_pFont_endMess, g_pFont_level, g_pFont_start)) {
        return false;
    }

    // create plane and set the position
    if (false == g_legoPlane.create(Device, -1, -1, 6, 0.03f, 7, d3d::GREEN)) return false;
    g_legoPlane.setPosition(0.0f, -0.0006f / 5, 0.0f);

    // 벽 3개 생성
    if (false == g_legowall[0].create(Device, -1, -1, 6, 0.3f, 0.12f, d3d::DARKRED)) return false;
    g_legowall[0].setPosition(0.0f, 0.12f, 3.5);
    if (false == g_legowall[1].create(Device, -1, -1, 0.12f, 0.3f, 7.12, d3d::DARKRED)) return false;
    g_legowall[1].setPosition(3, 0.12f, 0.0f);
    if (false == g_legowall[2].create(Device, -1, -1, 0.12f, 0.3f, 7.12, d3d::DARKRED)) return false;
    g_legowall[2].setPosition(-3, 0.12f, 0.0f);

    generateRandomPositions(spherePos);  // 노란 공 랜덤 위치 설정

    for (int i = 0; i < BALLNUM; ++i) {  // 노란 공 생성
        if (false == g_sphere[i].create(Device, sphereColor[i])) return false;
        g_sphere[i].setCenter(spherePos[i][0], (float)M_RADIUS, spherePos[i][1]);
        g_sphere[i].setPower(0, 0);
    }

    // 파란 공 생성
    if (SetupBlueBall(g_target_blueball, spherePos, Device)) {
        g_target_blueball.setPower(0, 0);
    }

    // 빨간 공 생성
    if (false == g_target_redball.create(Device, d3d::RED)) return false;
    g_target_redball.setCenter(0, (float)M_RADIUS, -3.5f + 0.06f + 3 * g_target_redball.getRadius());
    g_target_redball.setPower(0, 0);

    // 흰 공 생성
    if (false == g_target_whiteball.create(Device, d3d::WHITE)) return false;
    g_target_whiteball.setCenter(0, (float)M_RADIUS, -3.5f + 0.06f + 1 * g_target_redball.getRadius());

    // light setting 
    D3DLIGHT9 lit;
    ::ZeroMemory(&lit, sizeof(lit));
    lit.Type = D3DLIGHT_POINT;
    lit.Diffuse = d3d::WHITE;
    lit.Specular = d3d::WHITE * 0.9f;
    lit.Ambient = d3d::WHITE * 0.9f;
    lit.Position = D3DXVECTOR3(0.0f, 3.0f, 0.0f);
    lit.Range = 100.0f;
    lit.Attenuation0 = 0.0f;
    lit.Attenuation1 = 0.9f;
    lit.Attenuation2 = 0.0f;
    if (false == g_light.create(Device, lit))
        return false;

    // Position and aim the camera.
    D3DXVECTOR3 pos(0.0f, 5.0f, -8.0f);
    D3DXVECTOR3 target(0.0f, 0.0f, 0.0f);
    D3DXVECTOR3 up(0.0f, 2.0f, 0.0f);
    D3DXMatrixLookAtLH(&g_mView, &pos, &target, &up);
    Device->SetTransform(D3DTS_VIEW, &g_mView);

    // Set the projection matrix.
    D3DXMatrixPerspectiveFovLH(&g_mProj, D3DX_PI / 4,
        (float)Width / (float)Height, 1.0f, 100.0f);
    Device->SetTransform(D3DTS_PROJECTION, &g_mProj);

    // Set render states.
    Device->SetRenderState(D3DRS_LIGHTING, TRUE);
    Device->SetRenderState(D3DRS_SPECULARENABLE, TRUE);
    Device->SetRenderState(D3DRS_SHADEMODE, D3DSHADE_GOURAUD);

    g_light.setLight(Device, g_mWorld);
    return true;
}

void Cleanup(void)
{
    g_legoPlane.destroy();
    for (int i = 0; i < 3; i++) {
        g_legowall[i].destroy();
    }
    destroyAllLegoBlock();
    g_light.destroy();
}

// timeDelta represents the time between the current image frame and the last image frame.
// the distance of moving balls should be "velocity * timeDelta"
bool Display(float timeDelta)
{
    int i = 0;
    int j = 0;


    if (Device)
    {
        Device->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0x00afafaf, 1.0f, 0);
        Device->BeginScene();

        // redball 갱신
        g_target_redball.ballUpdate(timeDelta);

        // 필드를 벗어나면 공을 destroy하고 life를 감소
        if (g_target_redball.getCenter().z < -3.5) {
            g_target_redball.destroy();
            if (life == 1) {
                life--;
            }
            if (life > 1) {
                life--;
                resetGame();
            }
        }

        // 벽에 공이 충돌했는지 확인
        for (j = 0; j < 3; j++) {
            g_legowall[j].hitBy(g_target_redball);
        }

        // 흰 공으로 빨간 공 치기
        g_target_redball.hitBy(g_target_whiteball);

        // 파란 공 (생명 추가) 충돌했는지 확인
        if (!g_target_blueball.isNull()) {
            if (g_target_redball.hasIntersected(g_target_blueball)) {
                g_target_blueball.destroy();
                life++;
            }
        }

        for (j = 0; j < BALLNUM; j++) {  // 빨간공과 노란공 충돌했는지 확인
            if (g_sphere[j].isNull() == false) {
                if (g_target_redball.hitBy(g_sphere[j])) {  // 충돌했다면 true가 리턴되기에 충돌된 구를 destroy
                    g_sphere[j].destroy();
                    destroyNum++;
                }
            }
        }

        // draw plane, walls, and spheres
        g_legoPlane.draw(Device, g_mWorld);
        for (i = 0; i < BALLNUM; i++) {
            if (g_sphere[i].isNull() == false) {
                // destroyed 된 상태가 아니라면
                g_sphere[i].draw(Device, g_mWorld);
            }
        }
        for (i = 0; i < 3; i++) {
            g_legowall[i].draw(Device, g_mWorld);
        }
        if (g_target_redball.isNull() == true) {
            // 경기장 밖으로 나갔다면
        }
        else {
            g_target_redball.draw(Device, g_mWorld);
        }
        g_target_whiteball.draw(Device, g_mWorld);

        if (!g_target_blueball.isNull()) {
            // 파란공이 없거나 부서지지 않았다면
            g_target_blueball.draw(Device, g_mWorld);
        }

        g_light.draw(Device);


        // Life 및 Score 텍스트 출력
        RECT rect_life = { 50, 50, 0, 0 };  // 화면 좌측 상단에 위치
        char str[100] = "Life : ";
        char buff[100];
        itoa(life, buff, 10);
        strcat(str, buff);
        strcat(str, "\nScore : ");
        itoa(destroyNum, buff, 10);
        strcat(str, buff);

        g_pFont_life->DrawText(NULL, str, -1, &rect_life, DT_NOCLIP, D3DCOLOR_XRGB(0, 0, 0));

        // Level 텍스트 출력
        RECT rect_level = { 800, 50, 0, 0 };  // 화면 우측 상단에 위치
        char levelStr[50] = "Level: ";
        itoa(level, buff, 10);
        strcat(levelStr, buff);
        g_pFont_level->DrawText(NULL, levelStr, -1, &rect_level, DT_NOCLIP, D3DCOLOR_XRGB(0, 0, 0));

        // Press Space to Start 메시지 출력 (화면 하단 중앙)
        RECT rect_start = { 400, 650, 0, 0 };  // 하단 중앙 위치
        g_pFont_start->DrawTextA(NULL, "Press Space to Start", -1, &rect_start, DT_NOCLIP, D3DCOLOR_XRGB(0, 0, 0));


        // 종료 조건
        if (destroyNum == BALLNUM * level) {  // 모든 공을 제거했을 때 (level 을 곱해주는 이유는 destroyNum (score) 을 초기화하지 않기 때문)
            levelUp();
        }

        else {  // 모든 공을 제거하지 못했을 경우
            if (life == 0) {  // 생명이 0인 경우
                if (checkLevelUp(destroyNum)) {  // 레벨업 가능 여부 판별 (BALLNUM 의 절반 이상을 제거)
                    if (level == 5) {  // 이미 레벨 5 (최종레벨) 인 경우 WIN
                        win = true;
                        RECT rect_endMess = { 330, 300,0,0 };
                        g_target_redball.setPower(0, 0);
                        g_pFont_endMess->DrawTextA(NULL, "YOU WIN! Press ESC to quit game", -1, &rect_endMess, DT_NOCLIP, D3DCOLOR_XRGB(0, 0, 0));
                    }
                    else {  // 레벨 5 가 아닌 경우 레벨업
                        levelUp();
                    }
                }
                else {  // BALLNUM 의 절반 이상 제거하지 못한 경우 LOSE
                    RECT rect_endMess = { 330, 300,0,0 };
                    g_pFont_endMess->DrawTextA(NULL, "Defeated. Press ESC to quit game", -1, &rect_endMess, DT_NOCLIP, D3DCOLOR_XRGB(0, 0, 0));
                }
            }
        }

        Device->EndScene();
        Device->Present(0, 0, 0, 0);
        Device->SetTexture(0, NULL);
    }
    return true;
}

LRESULT CALLBACK d3d::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static bool wire = false;
    static bool isReset = true;
    static int old_x = 0;
    static int old_y = 0;
    static enum { WORLD_MOVE, LIGHT_MOVE, BLOCK_MOVE } move = WORLD_MOVE;

    switch (msg) {
    case WM_DESTROY:
    {
        ::PostQuitMessage(0);
        break;
    }
    case WM_KEYDOWN:
    {
        switch (wParam) {
        case VK_ESCAPE:
            ::DestroyWindow(hwnd);
            break;
        case VK_RETURN:
            if (NULL != Device) {
                wire = !wire;
                Device->SetRenderState(D3DRS_FILLMODE,
                    (wire ? D3DFILL_WIREFRAME : D3DFILL_SOLID));
            }
            break;
        case VK_SPACE:
            if (spaceActivate == 0 && win == false) {
                D3DXVECTOR3 targetpos = g_target_whiteball.getCenter();
                D3DXVECTOR3   whitepos = g_target_redball.getCenter();
                double theta = acos(sqrt(pow(targetpos.x - whitepos.x, 2)) / sqrt(pow(targetpos.x - whitepos.x, 2) +
                    pow(targetpos.z - whitepos.z, 2)));      // 기본 1 사분면
                if (targetpos.z - whitepos.z <= 0 && targetpos.x - whitepos.x >= 0) { theta = -theta; }   //4 사분면
                if (targetpos.z - whitepos.z >= 0 && targetpos.x - whitepos.x <= 0) { theta = PI - theta; } //2 사분면
                if (targetpos.z - whitepos.z <= 0 && targetpos.x - whitepos.x <= 0) { theta = PI + theta; } // 3 사분면
                double distance = sqrt(pow(targetpos.x - whitepos.x, 2) + pow(targetpos.z - whitepos.z, 2));
                //g_sphere[3].setPower(distance * cos(theta), distance * sin(theta));
                g_target_redball.setPower(0, speed);  // 빨간 공 속도 조절 가능
                spaceActivate = 1;  // 처음 발사 시에만 space 사용 후 비활성화
            }
            break;

        }
        break;
    }

    case WM_MOUSEMOVE:
    {
        int new_x = LOWORD(lParam);
        int new_y = HIWORD(lParam);
        float dx;
        float dy;

        if (LOWORD(wParam) & MK_RBUTTON) {

            if (isReset) {
                isReset = false;
            }
            else {
                D3DXVECTOR3 vDist;
                D3DXVECTOR3 vTrans;
                D3DXMATRIX mTrans;
                D3DXMATRIX mX;
                D3DXMATRIX mY;

                switch (move) {
                case WORLD_MOVE:
                    dx = (old_x - new_x) * 0.01f;
                    dy = (old_y - new_y) * 0.01f;
                    D3DXMatrixRotationY(&mX, dx);
                    D3DXMatrixRotationX(&mY, dy);
                    g_mWorld = g_mWorld * mX * mY;

                    break;
                }
            }

            old_x = new_x;
            old_y = new_y;

        }
        else {
            isReset = true;

            if (life > 0 && win == false) {
                dx = (old_x - new_x);// * 0.01f;
                D3DXVECTOR3 coord3d = g_target_whiteball.getCenter();
                D3DXVECTOR3 coord3d_W = g_target_redball.getCenter();
                if ((coord3d.x + dx * (-0.007f) > -3 + g_target_whiteball.getRadius() + 0.06f) && (coord3d.x + dx * (-0.007f) < 3 - g_target_whiteball.getRadius() - 0.06f)) {
                    g_target_whiteball.setCenter(coord3d.x + dx * (-0.007f), coord3d.y, coord3d.z);
                    if (spaceActivate == 0) {
                        g_target_redball.setCenter(coord3d_W.x + dx * (-0.007f), coord3d_W.y, coord3d_W.z);
                    }
                }
            }
            old_x = new_x;

            move = WORLD_MOVE;
        }
        break;
    }
    }

    return ::DefWindowProc(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hinstance,
    HINSTANCE prevInstance,
    PSTR cmdLine,
    int showCmd)
{
    srand(static_cast<unsigned int>(time(NULL)));

    if (!d3d::InitD3D(hinstance,
        Width, Height, true, D3DDEVTYPE_HAL, &Device))
    {
        ::MessageBox(0, "InitD3D() - FAILED", 0, 0);
        return 0;
    }

    if (!Setup())
    {
        ::MessageBox(0, "Setup() - FAILED", 0, 0);
        return 0;
    }

    d3d::EnterMsgLoop(Display);

    Cleanup();

    Device->Release();

    return 0;
}