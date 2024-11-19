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
#include <string.h>

IDirect3DDevice9* Device = NULL;

// window size
const int Width = 1024;
const int Height = 768;

// There are four balls
#define BALLNUM 40
// initialize the position (coordinate) of each ball (ball0 ~ ball3)
const float spherePos[BALLNUM][2] = {  // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! 배열크기와 위치 설정
    {-1.75f, 3.5f}, {-1.25f, 3.5f}, {-0.75f, 3.5f}, {-0.25f, 3.5f},
    {0.25f, 3.5f}, {0.75f, 3.5f}, {1.25f, 3.5f}, {1.75f, 3.5f},
    {-2.25f, 3.0f}, {2.25f, 3.0f},
    {-2.75f, 2.5f}, {-2.75f, 2.0f}, {-2.75f, 1.5f}, {-2.75f, 1.0f}, {-2.75f, 0.5f}, {-2.75f, 0.0f}, {-2.75f, -0.5f}, {-2.75f, -1.0f}, {-2.75f, -1.5f}, {-2.75f, -2.0f},
    {2.75f, 2.5f}, {2.75f, 2.0f}, {2.75f, 1.5f}, {2.75f, 1.0f}, {2.75f, 0.5f}, {2.75f, 0.0f}, {2.75f, -0.5f}, {2.75f, -1.0f}, {2.75f, -1.5f}, {2.75f, -2.0f},
    {-2.25f, -2.5f}, {2.25f, -2.5f},
    {-1.75f, -3.0f}, {-1.25f, -3.0f}, {-0.75f, -3.0f}, {-0.25f, -3.0f},
    {0.25f, -3.0f}, {0.75f, -3.0f}, {1.25f, -3.0f}, {1.75f, -3.0f}
};

// 10개의 공 모두 노란색
const D3DXCOLOR sphereColor[BALLNUM] = {  // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! 색 설정
    d3d::YELLOW, d3d::YELLOW, d3d::YELLOW, d3d::YELLOW, d3d::YELLOW,
    d3d::YELLOW, d3d::YELLOW, d3d::YELLOW, d3d::YELLOW, d3d::YELLOW,
    d3d::YELLOW, d3d::YELLOW, d3d::YELLOW, d3d::YELLOW, d3d::YELLOW,
    d3d::YELLOW, d3d::YELLOW, d3d::YELLOW, d3d::YELLOW, d3d::YELLOW,
    d3d::YELLOW, d3d::YELLOW, d3d::YELLOW, d3d::YELLOW, d3d::YELLOW,
    d3d::YELLOW, d3d::YELLOW, d3d::YELLOW, d3d::YELLOW, d3d::YELLOW,
    d3d::YELLOW, d3d::YELLOW, d3d::YELLOW, d3d::YELLOW, d3d::YELLOW,
    d3d::YELLOW, d3d::YELLOW, d3d::YELLOW, d3d::YELLOW, d3d::YELLOW
};

//추가 변수
int spaceActivate = 0;

// -----------------------------------------------------------------------------
// Transform matrices
// -----------------------------------------------------------------------------
D3DXMATRIX g_mWorld;
D3DXMATRIX g_mView;
D3DXMATRIX g_mProj;

#define M_RADIUS 0.15   // ball radius
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

            //ball.setCenter(center2.x + collisionNormal.x * (overlap / 2),
            //    center2.y,
            //    center2.z + collisionNormal.z * (overlap / 2));

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
            //ball.setPower(newVelocity2.x, newVelocity2.z);
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

    bool isNull() {
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
CSphere g_target_redball;
CLight   g_light;

double g_camera_pos[3] = { 0.0, 5.0, -8.0 };

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

ID3DXFont* g_pFont = NULL;

int life;

void resetGame() {
    g_target_redball.create(Device, d3d::RED);
    g_target_redball.setCenter(g_target_whiteball.getCenter().x, g_target_whiteball.getCenter().y, g_target_whiteball.getCenter().z+ g_target_whiteball.getRadius()*2);
    g_target_redball.setPower(0, 0);
    spaceActivate = 0;
}

// initialization
bool Setup()
{
    spaceActivate = 0;
    int i;

    D3DXMatrixIdentity(&g_mWorld);
    D3DXMatrixIdentity(&g_mView);
    D3DXMatrixIdentity(&g_mProj);

    //게임 속성을 설정합니다.
    life = 5;

    //글자의 위치를 설정합니다.
    D3DXFONT_DESC fontDesc = { 24, // Height 
        0, // Width 
        0, // Weight 
        0, // MipLevels 
        FALSE, // Italic 
        DEFAULT_CHARSET, // CharSet 
        OUT_DEFAULT_PRECIS, // OutputPrecision 
        CLIP_DEFAULT_PRECIS, // Quality 
        DEFAULT_PITCH, // PitchAndFamily 
        "Arial" // FaceName 
    }; if (FAILED(D3DXCreateFontIndirect(Device, &fontDesc, &g_pFont))) {
        ::MessageBox(0, "D3DXCreateFontIndirect() - FAILED", 0, 0); 
        return false;
    }

    // create plane and set the position
    if (false == g_legoPlane.create(Device, -1, -1, 6, 0.03f, 7, d3d::GREEN)) return false;
    g_legoPlane.setPosition(0.0f, -0.0006f / 5, 0.0f);

    // create walls and set the position. note that there are four walls
    if (false == g_legowall[0].create(Device, -1, -1, 6, 0.3f, 0.12f, d3d::DARKRED)) return false;
    g_legowall[0].setPosition(0.0f, 0.12f, 3.5);
    if (false == g_legowall[1].create(Device, -1, -1, 0.12f, 0.3f, 7.12, d3d::DARKRED)) return false;
    g_legowall[1].setPosition(3, 0.12f, 0.0f);
    if (false == g_legowall[2].create(Device, -1, -1, 0.12f, 0.3f, 7.12, d3d::DARKRED)) return false;
    g_legowall[2].setPosition(-3, 0.12f, 0.0f);

    // create four balls and set the position
    for (i = 0; i < BALLNUM; i++) {
        if (false == g_sphere[i].create(Device, sphereColor[i])) return false;
        g_sphere[i].setCenter(spherePos[i][0], (float)M_RADIUS, spherePos[i][1]);
        g_sphere[i].setPower(0, 0);
    }

    //create white ball for set direction
    if (false == g_target_redball.create(Device, d3d::RED)) return false;
    g_target_redball.setCenter(0, (float)M_RADIUS, -3.5f + 0.06f + 3 * g_target_redball.getRadius());
    g_target_redball.setPower(0, 0);

    // create blue ball for set direction
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

        //redball을 갱신합니다. (이동)
        g_target_redball.ballUpdate(timeDelta);

        //
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

        //벽에 공이 충돌했는지 확인합니다.
        for (j = 0; j < 3; j++) {
            g_legowall[j].hitBy(g_target_redball);
        }
        //공끼리 충돌했는지 확인합니다.
        g_target_redball.hitBy(g_target_whiteball);
        for (j = 0; j < BALLNUM; j++) {
            if (g_sphere[j].isNull() == false) {
                if (bool temp = g_target_redball.hitBy(g_sphere[j])) {  //충돌했다면 true가 리턴되기에 충돌된 구를 destroy합니다.
                    g_sphere[j].destroy();
                }
            }
        }

        // draw plane, walls, and spheres
        g_legoPlane.draw(Device, g_mWorld);
        for (i = 0; i < BALLNUM; i++) {
            if (g_sphere[i].isNull() == false) {
                //destroyed 된 상태가 아니라면
                g_sphere[i].draw(Device, g_mWorld);
            }
        }
        for (i = 0; i < 3; i++) {
            g_legowall[i].draw(Device, g_mWorld);
        }
        if (g_target_redball.isNull() == true) {
            //경기장 밖으로 나갔다면
        }
        else {
            g_target_redball.draw(Device, g_mWorld);
        }
        g_target_whiteball.draw(Device, g_mWorld);

        g_light.draw(Device);

        // 글자를 출력합니다.
        RECT rect = { 50,50,0,0 };
        char str[100] = "Life : ";
        char buff[100];
        itoa(life, buff, 10);
        strcat(str, buff);
        g_pFont->DrawText(NULL, str, -1, &rect, DT_NOCLIP, D3DCOLOR_XRGB(0, 0, 0));


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
            if (spaceActivate == 0) {
                D3DXVECTOR3 targetpos = g_target_whiteball.getCenter();
                D3DXVECTOR3   whitepos = g_target_redball.getCenter();
                double theta = acos(sqrt(pow(targetpos.x - whitepos.x, 2)) / sqrt(pow(targetpos.x - whitepos.x, 2) +
                    pow(targetpos.z - whitepos.z, 2)));      // 기본 1 사분면
                if (targetpos.z - whitepos.z <= 0 && targetpos.x - whitepos.x >= 0) { theta = -theta; }   //4 사분면
                if (targetpos.z - whitepos.z >= 0 && targetpos.x - whitepos.x <= 0) { theta = PI - theta; } //2 사분면
                if (targetpos.z - whitepos.z <= 0 && targetpos.x - whitepos.x <= 0) { theta = PI + theta; } // 3 사분면
                double distance = sqrt(pow(targetpos.x - whitepos.x, 2) + pow(targetpos.z - whitepos.z, 2));
                //g_sphere[3].setPower(distance * cos(theta), distance * sin(theta));
                g_target_redball.setPower(0, 3);
                spaceActivate = 1;
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

            if (life > 0) {
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